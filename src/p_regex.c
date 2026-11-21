/** @file p_regex.c
 *
 * Implementation of regex related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "fbstrings.h"
#include "hashtab.h"
#include "inst.h"
#include "interp.h"

/*
 * We have a bundled pcre for Windows or OSes that do not have PCRE installed
 * in some fashion already.
 */
#define PCRE2_CODE_UNIT_WIDTH 8

#ifdef WIN32
# define __STDC__ 1
# include "./pcre2.h"
#else
# ifdef HAVE_PCREINCDIR
#  include <pcre/pcre2.h>
# else
#  include <pcre2.h>
# endif
#endif

/*
 * TODO: These globals really probably shouldn't be globals.  I can only guess
 *       this is either some kind of primitive code re-use because all the
 *       functions use them, or it's some kind of an optimization to avoid
 *       local variables.  But it kills the thread safety and (IMO) makes the
 *       code harder to read/follow.
 */

/**
 * @private
 * @var used to store the parameters passed into the primitives
 *      This seems to be used for conveinance, but makes this probably
 *      not threadsafe.
 */
static struct inst *oper1, *oper2, *oper3, *oper4;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

/**
 * @private
 * @var This breaks thread safety for fun.
 *      We use this to copy pcre compile error strings into before
 *      passing back up to abort() and losing our frame context.
 */
static char errbuf[BUFFER_LEN];

/*
 * Definition for a MUF regex pattern, including flags and PCRE object.
 * So we can easily pass this stuff around
 */
typedef struct {
    struct shared_string *pattern;  /* The pattern we are using      */
    int flags;                      /* Flags associated with pattern */
    pcre2_code *re;                 /* Our underling PCRE object     */
    pcre2_match_data *md;           /* Associated match data object  */
} muf_re;

/**
 * @private
 * @var the cache of previously compiled regex items
 */
static muf_re muf_re_cache[MUF_RE_CACHE_ITEMS];

/**
 * Return a muf_re structure for a given pattern and flag combination.
 *
 * This will re-use existing patterns if applicable.  If there is an error,
 * NULL will be returned and 'errmsg' will point to a string buffer with the
 * error message.
 *
 * This does not allocate memory or require its returned muf_re be freed;
 * the struct muf_re's are preallocated in muf_re_cache.
 *
 * @private
 * @param pattern the pattern to make
 * @param flags the bitvector of flags (MUF_RE_* defines)
 * @param errmsg a pointer to a char* for populating error message
 * @return a struct muf_re or NULL if failed.
 */
static muf_re *
muf_re_get(struct shared_string *pattern, int flags)
{
    int idx = (hash(DoNullInd(pattern), MUF_RE_CACHE_ITEMS)
               + (unsigned int)flags) % MUF_RE_CACHE_ITEMS;
    muf_re *re = &muf_re_cache[idx];
    size_t erroff;
    int errcode = 0;

    if (re->pattern) {
	/* Make sure the hash returned a match to pattern & flags. */
        if ((flags != re->flags) ||
            strcmp(DoNullInd(pattern), DoNullInd(re->pattern))) {
            pcre2_code_free(re->re);

	    /* We're going to remove a link count here, even if we
	     * have the same pattern (because different flags) since
	     * we'll be adding to the link count later if we compile. */
            if (re->pattern && (--re->pattern->links == 0)) {
                free(re->pattern);
	    }
        } else {
	    /* Return good match. */
            return re;
	}
    }

    if (!re->md) {
	/* We never free this, as it can get reused by ANY pattern/code.
	 * We'll never need more than the number of hash units we have. */
        re->md = pcre2_match_data_create(MATCH_ARR_SIZE, NULL);
        if (!re->md) {
            pcre2_get_error_message(PCRE2_ERROR_NOMEMORY, errbuf, BUFFER_LEN);
	    re->pattern = NULL;
	    re->re = NULL;
	    return NULL;
        }
    }

    re->re = pcre2_compile(DoNullInd(pattern), PCRE2_ZERO_TERMINATED, flags, &errcode, &erroff, NULL);
    if (errcode != 0) {
	pcre2_get_error_message(errcode, errbuf, BUFFER_LEN);
    } else {
	errbuf[0] = '\0';
    }

    if (re->re == NULL) {
        re->pattern = NULL;
        return NULL;
    }

    re->pattern = pattern;
    re->pattern->links++;

    re->flags = flags;

    return re;
}

/**
 * Convert a PCRE error code to an error string.
 *
 * Returned string is a constant, no need to free it.
 *
 * @private
 * @param err the error code
 * @return string constant error message
 */
static const char *
muf_re_error(int err)
{
    switch (err) {
        case PCRE2_ERROR_NOMATCH:
            return "No matches";

        case PCRE2_ERROR_NULL:
            return "Internal error: NULL arg to pcre2_match()";

        case PCRE2_ERROR_BADOPTION:
            return "Invalid regexp option.";

        case PCRE2_ERROR_BADMAGIC:
            return "Internal error: bad magic number.";

        case PCRE2_ERROR_NOMEMORY:
            return "Out of memory.";

        case PCRE2_ERROR_NOSUBSTRING:
            return "No substring.";

        case PCRE2_ERROR_MATCHLIMIT:
            return "Match recursion limit exceeded.";

        case PCRE2_ERROR_CALLOUT:
            return "Internal error: callout error.";

        default:
            return "Unknown error";
    }
}

/**
 * Implementation of MUF REGEXP
 *
 * Consumes some text, a Perl-style pattern, and an integer flags.
 * Returns a list of submatch values and a list of submatch indexes.
 *
 * The flags can be a bitwise OR of the MUF_RE_* defines.  See the comments
 * for those defines to see what they do exactly.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_regexp(PRIM_PROTOTYPE)
{
    stk_array *nu_val = 0;
    stk_array *nu_idx = 0;
    size_t *matches;
    muf_re *re;
    char *text;
    int flags = 0;
    int len;
    int matchcnt = 0;

    CHECKOP(3);

    oper3 = POP();  /* int:Flags */
    oper2 = POP();  /* str:Pattern */
    oper1 = POP();  /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");

    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= PCRE2_CASELESS;

    if (oper3->data.number & MUF_RE_EXTENDED)
        flags |= PCRE2_EXTENDED;

    /* Try to get a MUF RE structure, reusing or caching if possible. */
    if ((re = muf_re_get(oper2->data.string, flags)) == NULL)
        abort_interp(errbuf);

    text = DoNullInd(oper1->data.string);
    len = strlen(text);

    /* Do our PCRE match */
    if ((matchcnt =
         pcre2_match(re->re, (PCRE2_SPTR)text, len, 0, 0,
                   re->md, NULL)) < 0) {
        /* Failed match -- display error or push up empty results */
        if (matchcnt != PCRE2_ERROR_NOMATCH) {
            abort_interp(muf_re_error(matchcnt));
        }

        if (((nu_val = new_array_packed(0, fr->pinning)) == NULL) ||
            ((nu_idx = new_array_packed(0, fr->pinning)) == NULL)) {
            array_free(nu_val);
            array_free(nu_idx);

            abort_interp("Out of memory");
        }
    } else {
        if (((nu_val = new_array_packed(matchcnt, fr->pinning)) == NULL) ||
            ((nu_idx = new_array_packed(matchcnt, fr->pinning)) == NULL)) {
            array_free(nu_val);
            array_free(nu_idx);

            abort_interp("Out of memory");
        }

	matches = pcre2_get_ovector_pointer(re->md);
        /* Pack result information into the return arrays */
        for (int i = 0; i < matchcnt; i++) {
            int substart = matches[i * 2];
            int subend = matches[i * 2 + 1];
            struct inst idx, val;
            stk_array *nu;

            if ((substart >= 0) && (subend >= 0) && (substart < len))
                snprintf(buf, BUFFER_LEN, "%.*s", (subend - substart),
                         &text[substart]);
            else
                buf[0] = '\0';

            idx.type = PROG_INTEGER;
            idx.data.number = i;
            val.type = PROG_STRING;
            val.data.string = alloc_prog_string(buf);

            array_setitem(&nu_val, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            if ((nu = new_array_packed(2, fr->pinning)) == NULL) {
                array_free(nu_val);
                array_free(nu_idx);

                abort_interp("Out of memory");
            }

            idx.type = PROG_INTEGER;
            idx.data.number = 0;
            val.type = PROG_INTEGER;
            val.data.number = substart + 1;

            array_setitem(&nu, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            idx.type = PROG_INTEGER;
            idx.data.number = 1;
            val.type = PROG_INTEGER;
            val.data.number = subend - substart;

            array_setitem(&nu, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            idx.type = PROG_INTEGER;
            idx.data.number = i;
            val.type = PROG_ARRAY;
            val.data.array = nu;

            array_setitem(&nu_idx, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);
        }
    }

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu_val);
    PushArrayRaw(nu_idx);
}

/**
 * Implementation of MUF REGSUB
 *
 * Consumes some text, a Perl-style pattern, a replacement string,
 * and an integer flags.  Returns a string that has replaced the
 * pattern with the replacement string.
 *
 * The flags can be a bitwise OR of the MUF_RE_* defines.  See the comments
 * for those defines to see what they do exactly.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_regsub(PRIM_PROTOTYPE)
{
    size_t *matches;
    int flags = 0;
    char *write_ptr = buf;
    int write_left = BUFFER_LEN - 1;
    muf_re *re;
    char *text;
    char *textstart;
    int matchcnt, len;

    CHECKOP(4);

    oper4 = POP();  /* int:Flags */
    oper3 = POP();  /* str:Replace */
    oper2 = POP();  /* str:Pattern */
    oper1 = POP();  /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (oper3->type != PROG_STRING)
        abort_interp("Non-string argument (3)");

    if (oper4->type != PROG_INTEGER)
        abort_interp("Non-integer argument (4)");

    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper4->data.number & MUF_RE_ICASE)
        flags |= PCRE2_CASELESS;

    if (oper4->data.number & MUF_RE_EXTENDED)
        flags |= PCRE2_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags)) == NULL)
        abort_interp(errbuf);

    textstart = text = DoNullInd(oper1->data.string);

    len = strlen(textstart);

    /* Iterate and replace */
    while ((*text != '\0') && (write_left > 0)) {
        if ((matchcnt =
             pcre2_match(re->re, (PCRE2_SPTR)textstart, len, text - textstart, 0,
                       re->md, NULL)) < 0) {
            if (matchcnt != PCRE2_ERROR_NOMATCH) {
                abort_interp(muf_re_error(matchcnt));
            }

            while ((write_left > 0) && (*text != '\0')) {
                *write_ptr++ = *text++;
                write_left--;
            }

            /* No more matches to do, break out */
            break;
        } else {
	    matches = pcre2_get_ovector_pointer(re->md);
            int allstart = matches[0];
            int allend = matches[1];
            int substart = -1;
            int subend = -1;
            char *read_ptr = DoNullInd(oper3->data.string);
            int count;

            for (count = allstart - (text - textstart);
                 (write_left > 0) && (*text != '\0') && (count > 0); count--) {
                *write_ptr++ = *text++;
                write_left--;
            }

            /* Do the replacements, checking for overflow along the way */
            while ((write_left > 0) && (*read_ptr != '\0')) {
                if (*read_ptr == '\\') {
                    if (!isdigit(*(++read_ptr))) {
                        *write_ptr++ = *read_ptr++;
                        write_left--;
                    } else {
                        int idx = (*read_ptr++) - '0';

                        if ((idx < 0) || (idx >= matchcnt)) {
                            abort_interp("Invalid \\subexp in substitution "
                                         "string. (3)");
                        }

                        substart = matches[idx * 2];
                        subend = matches[idx * 2 + 1];

                        if ((substart >= 0) && (subend >= 0)
                                            && (substart < len)) {
                            char *ptr = &textstart[substart];

                            count = subend - substart;

                            if (count > write_left) {
                                abort_interp("Operation would result in "
                                             "overflow");
                            }

                            for (; (write_left > 0) && (count > 0)
                                                    && (*ptr != '\0');
                                                    count--) {
                                *write_ptr++ = *ptr++;
                                write_left--;
                            }
                        }
                    }
                } else {
                    *write_ptr++ = *read_ptr++;
                    write_left--;
                }
            }

            for (count = allend - allstart; (*text != '\0') && (count > 0);
                 count--)
                text++;

            if (allstart == allend && *text) {
                *write_ptr++ = *text++;
                write_left--;
            }
        }

        if ((oper4->data.number & MUF_RE_ALL) == 0) {
            while ((write_left > 0) && (*text != '\0')) {
                *write_ptr++ = *text++;
                write_left--;
            }

            break;
        }
    }

    if (*text != '\0')
        abort_interp("Operation would result in overflow");

    *write_ptr = '\0';

    CLEAR(oper4);
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushString(buf);
}

/**
 * The underpinning for REGSPLIT and REGSPLIT_NOEMPTY
 *
 * Consumes a string, a pattern, and an integer which is the addition
 * of MUF_RE_* defines.  Returns a list which is the string split by
 * the pattern.  If empty is true, then it will push empty pieces
 * on the stack for consecutive matches.  If false, it will not put
 * empty pieces on the stack.
 *
 * @private
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 * @param empty boolean true to return empty pieces for consecutive matches
 */
static void
_prim_regsplit(PRIM_PROTOTYPE, int empty)
{
    stk_array* nu_val = 0;
    size_t *matches;
    muf_re *re;
    char *text, *textstart;
    int flags = 0;
    int len;
    int matchcnt = 0, pos = 0;
    struct inst val;

    CHECKOP(3);

    oper3 = POP(); /* int:Flags */
    oper2 = POP(); /* str:Pattern */
    oper1 = POP(); /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");

    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= PCRE2_CASELESS;

    if (oper3->data.number & MUF_RE_EXTENDED)
        flags |= PCRE2_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags)) == NULL)
        abort_interp(errbuf);

    textstart = text = DoNullInd(oper1->data.string);
    len = strlen(text);

    if ((nu_val = new_array_packed(0, fr->pinning)) == NULL) {
        array_free(nu_val);
        abort_interp("Out of memory");
    }

    while (*text != '\0') {
        if ((matchcnt = pcre2_match(re->re, textstart, len, text-textstart,
                                  0, re->md, NULL)) < 0) {
            if (matchcnt != PCRE2_ERROR_NOMATCH) {
                array_free(nu_val);
                abort_interp(muf_re_error(matchcnt));
            }

            val.type = PROG_STRING;
            val.data.string = alloc_prog_string(&textstart[pos]);
            array_appenditem(&nu_val, &val);
            CLEAR(&val);

            break;
        } else {
	    matches = pcre2_get_ovector_pointer(re->md);
            int start = matches[0];
            int end = matches[1];

            if (empty && pos == start) {
                val.type = PROG_STRING;
                val.data.string = alloc_prog_string("");
                array_appenditem(&nu_val, &val);
                CLEAR(&val);
            } else if (start - pos > 0) {
                snprintf(buf, sizeof(buf), "%.*s", start - pos, &textstart[pos]);
                val.type = PROG_STRING;
                val.data.string = alloc_prog_string(buf);
                array_appenditem(&nu_val, &val);
                CLEAR(&val);
            }

            if (end == pos) {
                /* always advance where we match from by at least one char */
                text = &textstart[end + 1];
                if (!*text) {
                    val.type = PROG_STRING;
                    val.data.string = alloc_prog_string(&textstart[pos]);
                    array_appenditem(&nu_val, &val);
                    CLEAR(&val);
                }
            } else {
                text = &textstart[end];
            }
            pos = end;
        }
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu_val);
}

/**
 * Implementation of MUF REGSPLIT_NOEMPTY
 *
 * Consumes a string, a pattern, and an integer which is the addition
 * of MUF_RE_* defines.  Returns a list which is the string split by
 * the pattern.  It will not put empty pieces on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_regsplit_noempty(PRIM_PROTOTYPE)
{
    _prim_regsplit(player, program, mlev, pc, arg, top, fr, 0);
}

/**
 * Implementation of MUF REGSPLIT
 *
 * Consumes a string, a pattern, and an integer which is the addition
 * of MUF_RE_* defines.  Returns a list which is the string split by
 * the pattern.  It will push empty pieces on the stack for consecutive
 * matches.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_regsplit(PRIM_PROTOTYPE)
{
    _prim_regsplit(player, program, mlev, pc, arg, top, fr, 1);
}
