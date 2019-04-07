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

#ifdef WIN32
# define __STDC__ 1
# include "./pcre.h"
#else
# ifdef HAVE_PCREINCDIR
#  include <pcre/pcre.h>
# else
#  include <pcre.h>
# endif
#endif

#define MUF_RE_CACHE_ITEMS 64

static struct inst *oper1, *oper2, *oper3, *oper4;
static char buf[BUFFER_LEN];

typedef struct {
    struct shared_string *pattern;
    int flags;
    pcre *re;
} muf_re;

static muf_re muf_re_cache[MUF_RE_CACHE_ITEMS];

static muf_re *
muf_re_get(struct shared_string *pattern, int flags, const char **errmsg)
{
    int idx = (hash(DoNullInd(pattern), MUF_RE_CACHE_ITEMS) + (unsigned int)flags) % MUF_RE_CACHE_ITEMS;
    muf_re *re = &muf_re_cache[idx];
    int erroff;

    if (re->pattern) {
	if ((flags != re->flags) || strcmp(DoNullInd(pattern), DoNullInd(re->pattern))) {
	    pcre_free(re->re);

	    if (re->pattern && (--re->pattern->links == 0))
		free(re->pattern);
	} else
	    return re;
    }

    re->re = pcre_compile(DoNullInd(pattern), flags, errmsg, &erroff, NULL);
    if (re->re == NULL) {
	re->pattern = NULL;
	return NULL;
    }

    re->pattern = pattern;
    re->pattern->links++;

    re->flags = flags;

    return re;
}

static const char *
muf_re_error(int err)
{
    switch (err) {
    case PCRE_ERROR_NOMATCH:
	return "No matches";
    case PCRE_ERROR_NULL:
	return "Internal error: NULL arg to pcre_exec()";
    case PCRE_ERROR_BADOPTION:
	return "Invalid regexp option.";
    case PCRE_ERROR_BADMAGIC:
	return "Internal error: bad magic number.";
    case PCRE_ERROR_UNKNOWN_NODE:
	return "Internal error: bad regexp node.";
    case PCRE_ERROR_NOMEMORY:
	return "Out of memory.";
    case PCRE_ERROR_NOSUBSTRING:
	return "No substring.";
    case PCRE_ERROR_MATCHLIMIT:
	return "Match recursion limit exceeded.";
    case PCRE_ERROR_CALLOUT:
	return "Internal error: callout error.";
    default:
	return "Unknown error";
    }
}

#define MATCH_ARR_SIZE 30

void
prim_regexp(PRIM_PROTOTYPE)
{
    stk_array *nu_val = 0;
    stk_array *nu_idx = 0;
    int matches[MATCH_ARR_SIZE];
    muf_re *re;
    char *text;
    int flags = 0;
    int len;
    int matchcnt = 0;
    const char *errstr;

    CHECKOP(3);

    oper3 = POP();		/* int:Flags */
    oper2 = POP();		/* str:Pattern */
    oper1 = POP();		/* str:Text */

    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_INTEGER)
	abort_interp("Non-integer argument (3)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");

    if (oper3->data.number & MUF_RE_ICASE)
	flags |= PCRE_CASELESS;
    if (oper3->data.number & MUF_RE_EXTENDED)
	flags |= PCRE_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags, &errstr)) == NULL)
	abort_interp(errstr);

    text = DoNullInd(oper1->data.string);
    len = strlen(text);

    if ((matchcnt = pcre_exec(re->re, NULL, text, len, 0, 0, matches, MATCH_ARR_SIZE)) < 0) {
	if (matchcnt != PCRE_ERROR_NOMATCH) {
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

	for (int i = 0; i < matchcnt; i++) {
	    int substart = matches[i * 2];
	    int subend = matches[i * 2 + 1];
	    struct inst idx, val;
	    stk_array *nu;

	    if ((substart >= 0) && (subend >= 0) && (substart < len))
		snprintf(buf, BUFFER_LEN, "%.*s", (subend - substart), &text[substart]);
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

void
prim_regsub(PRIM_PROTOTYPE)
{
    int matches[MATCH_ARR_SIZE];
    int flags = 0;
    char *write_ptr = buf;
    int write_left = BUFFER_LEN - 1;
    muf_re *re;
    char *text;
    char *textstart;
    const char *errstr;
    int matchcnt, len;

    CHECKOP(4);

    oper4 = POP();		/* int:Flags */
    oper3 = POP();		/* str:Replace */
    oper2 = POP();		/* str:Pattern */
    oper1 = POP();		/* str:Text */

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
	flags |= PCRE_CASELESS;
    if (oper4->data.number & MUF_RE_EXTENDED)
	flags |= PCRE_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags, &errstr)) == NULL)
	abort_interp(errstr);

    textstart = text = DoNullInd(oper1->data.string);

    len = strlen(textstart);
    while ((*text != '\0') && (write_left > 0)) {
	if ((matchcnt =
	     pcre_exec(re->re, NULL, textstart, len, text - textstart, 0, matches,
		       MATCH_ARR_SIZE)) < 0) {
	    if (matchcnt != PCRE_ERROR_NOMATCH) {
		abort_interp(muf_re_error(matchcnt));
	    }

	    while ((write_left > 0) && (*text != '\0')) {
		*write_ptr++ = *text++;
		write_left--;
	    }

	    break;
	} else {
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

	    while ((write_left > 0) && (*read_ptr != '\0')) {
		if (*read_ptr == '\\') {
		    if (!isdigit(*(++read_ptr))) {
			*write_ptr++ = *read_ptr++;
			write_left--;
		    } else {
			int idx = (*read_ptr++) - '0';

			if ((idx < 0) || (idx >= matchcnt)) {
			    abort_interp("Invalid \\subexp in substitution string. (3)");
			}

			substart = matches[idx * 2];
			subend = matches[idx * 2 + 1];

			if ((substart >= 0) && (subend >= 0) && (substart < len)) {
			    char *ptr = &textstart[substart];

			    count = subend - substart;

			    if (count > write_left) {
				abort_interp("Operation would result in overflow");
			    }

			    for (; (write_left > 0) && (count > 0) && (*ptr != '\0'); count--) {
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

	    for (count = allend - allstart; (*text != '\0') && (count > 0); count--)
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

static void
_prim_regsplit(PRIM_PROTOTYPE, int empty)
{
    stk_array* nu_val = 0;
    int matches[MATCH_ARR_SIZE];
    muf_re *re;
    char *text, *textstart;
    int flags = 0;
    int len;
    int matchcnt = 0, pos = 0;
    const char *errstr;
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
	flags |= PCRE_CASELESS;
    if (oper3->data.number & MUF_RE_EXTENDED)
	flags |= PCRE_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags, &errstr)) == NULL)
	abort_interp(errstr);

    textstart = text = DoNullInd(oper1->data.string);
    len = strlen(text);

    if ((nu_val = new_array_packed(0, fr->pinning)) == NULL) {
	array_free(nu_val);
	abort_interp("Out of memory");
    }

    while(*text != '\0') {
	if ((matchcnt = pcre_exec(re->re, NULL, textstart, len, text-textstart, 0, matches, MATCH_ARR_SIZE)) < 0) {
	    if (matchcnt != PCRE_ERROR_NOMATCH) {
                array_free(nu_val);
		abort_interp(muf_re_error(matchcnt));
	    }
	    val.type = PROG_STRING;
	    val.data.string = alloc_prog_string(&textstart[pos]);
	    array_appenditem(&nu_val, &val);
            CLEAR(&val);

	    break;
	} else {
	    int start = matches[0];
	    int end = matches[1];

	    if (empty && pos == start) {
		val.type = PROG_STRING;
		val.data.string	= alloc_prog_string("");
		array_appenditem(&nu_val, &val);
                CLEAR(&val);
	    } else if (start - pos > 0) {
		snprintf(buf, sizeof(buf), "%.*s", start - pos, &textstart[pos]);
		val.type = PROG_STRING;
		val.data.string	= alloc_prog_string(buf);
		array_appenditem(&nu_val, &val);
                CLEAR(&val);
	    }

	    text = &textstart[end];
	    pos = end;
	}
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu_val);
}

void
prim_regsplit_noempty(PRIM_PROTOTYPE)
{
    _prim_regsplit(player, program, mlev, pc, arg, top, fr, 0);
}

void
prim_regsplit(PRIM_PROTOTYPE)
{
    _prim_regsplit(player, program, mlev, pc, arg, top, fr, 1);
}
