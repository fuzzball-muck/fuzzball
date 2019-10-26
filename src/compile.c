/** @file compile.c
 *
 * Definition of the various functions that handles the MUF compiler.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "commands.h"
#include "compile.h"
#include "db.h"
#include "edit.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "game.h"
#include "hashtab.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "mcp.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

/*
 * Symbols that are special in MUF -- changing any of these will be
 * hilariously catastrophic.
 */
#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'
#define BEGINDIRECTIVE '$'
#define BEGINESCAPE '\\'

/* Maximum number of macro substitutions allowed on a single line */
#define SUBSTITUTIONS 20

/*
 * Parameters for add_control_structure to support different branch/loop
 * and related structural items.
 */
#define CTYPE_IF    1
#define CTYPE_ELSE  2
#define CTYPE_BEGIN 3
#define CTYPE_FOR   4
#define CTYPE_WHILE 5
#define CTYPE_TRY   6
#define CTYPE_CATCH 7

/*
 * Chunk list for the address list structure.  This number balances between
 * memory usage vs. reallocs needed to increase the structure size as your
 * program size grows.
 */
#define ADDRLIST_ALLOC_CHUNK_SIZE 256

#define abort_compile(ST,C) { do_abort_compile(ST,C); return 0; }
#define v_abort_compile(ST,C) { do_abort_compile(ST,C); return; }
#define free_prog(i) free_prog_real(i,__FILE__,__LINE__);

static hash_tab primitive_list[COMP_HASH_SIZE];

/**
 * @var the array that has all of our primitive names.
 */
const char *base_inst[] = {
    "JMP", "READ", "SLEEP", "CALL", "EXECUTE", "EXIT", "EVENT_WAITFOR",
    "CATCH", "CATCH_DETAILED", PRIMS_CONNECTS_NAMES, PRIMS_DB_NAMES,
    PRIMS_MATH_NAMES, PRIMS_MISC_NAMES, PRIMS_PROPS_NAMES, PRIMS_STACK_NAMES,
    PRIMS_STRINGS_NAMES, PRIMS_ARRAY_NAMES, PRIMS_FLOAT_NAMES,
    PRIMS_ERROR_NAMES, PRIMS_MCP_NAMES,
#ifdef MCPGUI_SUPPORT
    PRIMS_MCPGUI_NAMES,
#endif
    PRIMS_REGEX_NAMES, PRIMS_INTERNAL_NAMES
};

/* This keeps track of nested control structures */
struct CONTROL_STACK {
    short type;                     /* One of the CTYPE constants   */
    struct INTERMEDIATE *place;     /* Instruction pointer          */
    struct CONTROL_STACK *next;     /* The next link in the chain   */
    struct CONTROL_STACK *extra;    /* Not entirely sure about this */
};

/*
 * This structure is an association list that contains both a procedure
 * name and the place in the code that it belongs.  A lookup to the procedure
 * will see both it's name and it's number and so we can generate a
 * reference to it.  Since I want to disallow co-recursion,  I will not allow
 * forward referencing.
 */
struct PROC_LIST {
    const char *name;
    struct INTERMEDIATE *code;
    struct PROC_LIST *next;
};

/*
 * The intermediate code is code generated as a linked list when there is no
 * hint or notion of how much code there will be, and to help resolve all
 * references.  There is always a pointer to the current word that is
 * being compiled kept.
 */
#define INTMEDFLG_DIVBYZERO     1
#define INTMEDFLG_MODBYZERO     2
#define INTMEDFLG_INTRY         4
#define INTMEDFLG_OVERFLOW      8

#define IMMFLAG_REFERENCED      1   /* Referenced by a jump */

struct INTERMEDIATE {
    int no;                     /* which number instruction this is */
    struct inst in;             /* instruction itself */
    short line;                 /* line number of instruction */
    short flags;                /* The various INTMEDFLG's */
    struct INTERMEDIATE *next;  /* next instruction */
};

/* The state structure for a compile. */
typedef struct COMPILE_STATE_T {
    struct CONTROL_STACK *control_stack;
    struct PROC_LIST *procs;
    struct PROC_LIST *alt_entrypoint;

    int nowords;                        /* number of words compiled */
    struct INTERMEDIATE *curr_word;     /* word being compiled */
    struct INTERMEDIATE *first_word;    /* first word of the list */
    struct INTERMEDIATE *curr_proc;     /* first word of curr. proc. */
    struct publics *currpubs;
    int nested_fors;
    int nested_trys;

    /* Address resolution data.  Used to relink addresses after compile. */
    struct INTERMEDIATE **addrlist;     /* list of addresses to resolve */
    int *addroffsets;                   /* list of offsets from instrs */
    size_t addrmax;                     /* size of current addrlist array */
    unsigned int addrcount;             /* number of allocated addresses */

    /* variable names.  The index into cstat->variables give you what position
     * the variable holds.
     */
    const char *variables[MAX_VAR];
    const char *localvars[MAX_VAR];
    const char *scopedvars[MAX_VAR];

    struct line *curr_line;     /* current line */
    int lineno;                 /* current line number */
    int start_comment;          /* Line last comment started at */
    int force_comment;          /* Only attempt certain compile */
    const char *next_char;      /* next char * */
    dbref player, program;      /* player and program for this compile */

    int compile_err;            /* 1 if error occured */

    char *line_copy;
    int macrosubs;              /* Safeguard for macro-subst. infinite loops */
    int descr;                  /* the descriptor that initiated compiling */
    int force_err_display;      /* If true, always show compiler errors. */
    struct INTERMEDIATE *nextinst;
    hash_tab defhash[DEFHASHSIZE];
} COMPSTATE;

/* These are globally available as externs */

/**
 * @var integer primitive ID for FOR primitive
 */
int IN_FOR;

/**
 * @var integer primitive ID for FOREACH primitive
 */
int IN_FOREACH;

/**
 * @var integer primitive ID for FORITER primitive
 */
int IN_FORITER;

/**
 * @var integer primitive ID for FORPOP primitive
 */
int IN_FORPOP;

/**
 * @var integer primitive ID for TRYPROP primitive
 */
int IN_TRYPOP;

/* See definition for implementation details */
static void free_prog_real(dbref, const char *, const int);

/* See definition for implementation details */
static const char *next_token(COMPSTATE *);

/* See definition for implementation details */
static struct INTERMEDIATE *process_special(COMPSTATE *, const char *);

/**
 * Clean up publics linked list
 *
 * @private
 * @param mypub the linked list to free up
 */
static void
cleanpubs(struct publics *mypub)
{
    struct publics *tmppub;

    while (mypub) {
        tmppub = mypub->next;
        free(mypub->subname);
        free(mypub);
        mypub = tmppub;
    }
}

/**
 * Free the memory of an intermediate node
 *
 * This is complicated by the different data types that an INTERMEDIATE
 * can contain; it will free memory appropriately to delete whatever
 * data it contains.
 *
 * @private
 * @param wd the node to delete
 */
static void
free_intermediate_node(struct INTERMEDIATE *wd)
{
    int varcnt;

    if (wd->in.type == PROG_STRING) {
            free(wd->in.data.string);
    }

    if (wd->in.type == PROG_FUNCTION) {
        free(wd->in.data.mufproc->procname);
        varcnt = wd->in.data.mufproc->vars;

        if (wd->in.data.mufproc->varnames) {
            for (int j = 0; j < varcnt; j++) {
                free((void *) wd->in.data.mufproc->varnames[j]);
            }

            free((void *) wd->in.data.mufproc->varnames);
        }

        free(wd->in.data.mufproc);
    }

    free(wd);
}

/**
 * Free all the intermediates on a linked list
 *
 * @private
 * @param wd the head of the linked list
 */
static void
free_intermediate_chain(struct INTERMEDIATE *wd)
{
    struct INTERMEDIATE *tempword;

    while (wd) {
        tempword = wd->next;
        free_intermediate_node(wd);
        wd = tempword;
    }
}

/**
 * Free the address information out of a COMPSTATE structure.
 *
 * Specifically it deletes the 'addrlist', 'addroffsets', and
 * zeros out the count and max.
 *
 * @private
 * @param cstat compstate to delete address information out of
 */
static void
free_addresses(COMPSTATE * cstat)
{
    cstat->addrcount = 0;
    cstat->addrmax = 0;

    free(cstat->addrlist);
    cstat->addrlist = NULL;

    free(cstat->addroffsets);
}

/**
 * Cleanup a COMPSTATE structure, freeing all memory used by it.
 *
 * @private
 * @param cstat the structure to delete
 */
static void
cleanup(COMPSTATE * cstat)
{
    struct CONTROL_STACK *tempif;
    struct PROC_LIST *tempp;

    free_intermediate_chain(cstat->first_word);
    cstat->first_word = 0;

    for (struct CONTROL_STACK *eef = cstat->control_stack; eef; eef = tempif) {
        tempif = eef->next;

        free(eef->extra);
        free(eef);
    }

    cstat->control_stack = 0;

    for (struct PROC_LIST *p = cstat->procs; p; p = tempp) {
        tempp = p->next;
        free((void *) p->name);
        free(p);
    }

    cstat->procs = 0;

    kill_hash(cstat->defhash, DEFHASHSIZE, 1);
    free_addresses(cstat);

    for (int i = RES_VAR; i < MAX_VAR && cstat->variables[i]; i++) {
        free((void *) cstat->variables[i]);
        cstat->variables[i] = 0;
    }

    for (int i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++) {
        free((void *) cstat->scopedvars[i]);
        cstat->scopedvars[i] = 0;
    }

    for (int i = 0; i < MAX_VAR && cstat->localvars[i]; i++) {
        free((void *) cstat->localvars[i]);
        cstat->localvars[i] = 0;
    }
}

/**
 * Abort the compile, displaying an error message explaining why
 *
 * This does all the cleanup process associated with shutting down
 * the compiler, freeing all memory.
 *
 * @private
 * @param cstat the COMPSTATE to delete
 * @param c a message to display to the user - line number is injected for you
 */
static void
do_abort_compile(COMPSTATE * cstat, const char *c)
{
    static char _buf[BUFFER_LEN];

    /*
     * If the problem is a comment that is hanging open, we can give an
     * additional hint.
     */
    if (cstat->start_comment) {
        snprintf(_buf, sizeof(_buf), "Error in line %d: %s  Comment starting at line %d.",
                 cstat->lineno, c, cstat->start_comment);
        cstat->start_comment = 0;
    } else {
        snprintf(_buf, sizeof(_buf), "Error in line %d: %s", cstat->lineno, c);
    }

    free(cstat->line_copy);
    cstat->line_copy = NULL;
    cstat->next_char = NULL;

    /*
     * Determine how to display the compile error to the user, based
     * on if the user is in the interactive editor (not presently reading)
     * or if errors are being forced.
     *
     * If no errors are to be displayed, log them instead.
     */
    if (((FLAGS(cstat->player) & INTERACTIVE) 
          && !(FLAGS(cstat->player) & READMODE)) || cstat->force_err_display) {
        notify_nolisten(cstat->player, _buf, 1);
    } else {
        log_muf("%s(#%d) [%s(#%d)] %s(#%d) %s",
                NAME(OWNER(cstat->program)), OWNER(cstat->program),
                NAME(cstat->program), cstat->program,
                NAME(cstat->player), cstat->player, _buf);
    }

    cstat->compile_err++;

    /* Only de-allocate on the first compile error */
    if (cstat->compile_err > 1) {
        return;
    }

    /* Delete instructions */
    if (cstat->nextinst) {
        struct INTERMEDIATE *ptr;

        while (cstat->nextinst) {
            ptr = cstat->nextinst;
            cstat->nextinst = ptr->next;
            free(ptr);
        }

        cstat->nextinst = NULL;
    }

    /* Run cleanups */
    cleanup(cstat);
    cleanpubs(cstat->currpubs);
    cstat->currpubs = NULL;
    free_prog(cstat->program);
    cleanpubs(PROGRAM_PUBS(cstat->program));
    PROGRAM_SET_PUBS(cstat->program, NULL);
    clean_mcpbinds(PROGRAM_MCPBINDS(cstat->program));
    PROGRAM_SET_MCPBINDS(cstat->program, NULL);
    PROGRAM_SET_PROFTIME(cstat->program, 0, 0);
}

/**
 * Get an address for a given INTERMEDIATE and offset pair
 *
 * The address is an index into the 'addrlist' and 'addroffsets' table.
 * If a given INTERMEDIATE and offset is already exits, its index is
 * returned.
 *
 * Otherwise, it is added to the end of the addrlist (Which is grown to
 * accomodate it if needed) and the new index is returned.
 *
 * The addrlist / addroffsets are grown in chunks of ADDRLIST_ALLOC_CHUNK_SIZE
 * size.
 *
 * @private
 * @param cstat the compstate
 * @param dest the intermediate to store
 * @param offset the offset for the intermediate.
 * @return int index for addrlist/addroffsets in cstat
 */
static int
get_address(COMPSTATE * cstat, struct INTERMEDIATE *dest, int offset)
{
    if (!cstat->addrlist) {
        cstat->addrcount = 0;
        cstat->addrmax = ADDRLIST_ALLOC_CHUNK_SIZE;
        cstat->addrlist = malloc(cstat->addrmax * sizeof(struct INTERMEDIATE *));
        cstat->addroffsets = malloc(cstat->addrmax * sizeof(int));
    }

    for (unsigned int i = 0; i < cstat->addrcount; i++)
        if (cstat->addrlist[i] == dest && cstat->addroffsets[i] == offset)
            return i;

    if (cstat->addrcount >= cstat->addrmax) {
        cstat->addrmax += ADDRLIST_ALLOC_CHUNK_SIZE;
        cstat->addrlist = (struct INTERMEDIATE **)
                        realloc(cstat->addrlist,
                                cstat->addrmax * sizeof(struct INTERMEDIATE *));
        cstat->addroffsets = (int *)
                    realloc(cstat->addroffsets, cstat->addrmax * sizeof(int));
    }

    cstat->addrlist[cstat->addrcount] = dest;
    cstat->addroffsets[cstat->addrcount] = offset;
    return cstat->addrcount++;
}

/**
 * "Fix" the addresses after the compile completes
 *
 * This ensures all the instructions are sequential and all references
 * for control structures are pointed correctly.
 *
 * @private
 * @param cstat the compile state object
 */
static void
fix_addresses(COMPSTATE * cstat)
{
    int count = 0;

    /* renumber the instruction chain */
    for (struct INTERMEDIATE *ptr = cstat->first_word; ptr; ptr = ptr->next)
        ptr->no = count++;

    /* repoint publics to targets */
    for (struct publics *pub = cstat->currpubs; pub; pub = pub->next)
        pub->addr.no = cstat->addrlist[pub->addr.no]->no
                       + cstat->addroffsets[pub->addr.no];

    /* repoint addresses to targets */
    for (struct INTERMEDIATE *ptr = cstat->first_word; ptr; ptr = ptr->next) {
        switch (ptr->in.type) {
            case PROG_ADD:
            case PROG_IF:
            case PROG_TRY:
            case PROG_JMP:
            case PROG_EXEC:
                ptr->in.data.number = cstat->addrlist[ptr->in.data.number]->no
                                        +
                                      cstat->addroffsets[ptr->in.data.number];
                break;
            default:
                break;
        }
    }
}

/**
 * Fix references to publics
 *
 * This sets pointers to the start instruction for each public.
 *
 * @private
 * @param mypubs the list of publics
 * @param offset the start of the program
 */
static void
fixpubs(struct publics *mypubs, struct inst *offset)
{
    while (mypubs) {
        mypubs->addr.ptr = offset + mypubs->addr.no;
        mypubs = mypubs->next;
    }
}

/**
 * Get the size, in bytes, of all the public structures in the linked list
 *
 * @private
 * @param mypubs the head of the list
 * @return integer size of mypubs list
 */
static size_t
size_pubs(struct publics *mypubs)
{
    size_t bytes = 0;

    while (mypubs) {
        bytes += sizeof(*mypubs);
        mypubs = mypubs->next;
    }

    return bytes;
}

/**
 * Expand a definition
 *
 * A definition can either be a $ definition or a macro definition
 * $ definitions live in the 'defhash'.  Macro definitions start with
 * BEGINMACRO which is usually a . and are expanded using
 * macro_expansion -- whatever comes from that call is returned.
 *
 * @see macro_expansion
 *
 * @private
 * @param cstat the compile state
 * @param defname the definition to look up
 * @return the expanded code
 */
static char *
expand_def(COMPSTATE * cstat, const char *defname)
{
    hash_data *exp = find_hash(defname, cstat->defhash, DEFHASHSIZE);

    /*
     * If the definition is not in our definition hash, is it a macro?
     *
     * If not, then we couldn't resolve it.
     */
    if (!exp) {
        if (*defname == BEGINMACRO) {
            return (macro_expansion(&defname[1]));
        } else {
            return (NULL);
        }
    }

    return (strdup((char *) exp->pval));
}

/**
 * Remove a definition
 *
 * This is basically $undef
 *
 * Does not cause an error condition if 'defname' isn't found.
 *
 * @private
 * @param cstat the compile state struct
 * @param defname the definition to delete.
 */
static void
kill_def(COMPSTATE * cstat, const char *defname)
{
    hash_data *exp = find_hash(defname, cstat->defhash, DEFHASHSIZE);

    if (exp) {
        free(exp->pval);
        (void) free_hash(defname, cstat->defhash, DEFHASHSIZE);
    }
}

/**
 * Add a definition to our definition hash
 *
 * This is basically $define
 *
 * @private
 * @param cstat the compile state structure
 * @param defname the definition to set
 * @param deff the definition value
 */
static void
insert_def(COMPSTATE * cstat, const char *defname, const char *deff)
{
    hash_data hd;

    (void) kill_def(cstat, defname);
    hd.pval = strdup(deff);
    (void) add_hash(defname, hd, cstat->defhash, DEFHASHSIZE);
}

/**
 * Set an integer definition
 *
 * Converts the integer to string
 *
 * @private
 * @param cstat the compile state
 * @param defname the definition name
 * @param deff the integer to set
 */
static void
insert_intdef(COMPSTATE * cstat, const char *defname, int deff)
{
    char buf[sizeof(int) * 3];

    snprintf(buf, sizeof(buf), "%d", deff);
    insert_def(cstat, defname, buf);
}

/**
 * Import prop-based definitions into our cstat defhash
 *
 * The DEFINES_PROPDIR (usually _defs/) is iterated over on object 'i',
 * and any set props are imported as definitions.  Subdirs are not
 * iterated into.
 *
 * @private
 * @param cstat the compile state
 * @param i the object to get definitions off of
 */
static void
include_defs(COMPSTATE * cstat, dbref i)
{
    char dirname[BUFFER_LEN];
    char temp[BUFFER_LEN];
    const char *tmpptr;
    PropPtr j, pptr;

    snprintf(dirname, sizeof(dirname), "/%s/", DEFINES_PROPDIR);
    j = first_prop(i, dirname, &pptr, temp, sizeof(temp));

    while (j) {
        snprintf(dirname, sizeof(dirname), "/%s/", DEFINES_PROPDIR);
        strcatn(dirname, sizeof(dirname), temp);
        tmpptr = get_property_class(i, dirname);

        if (tmpptr && *tmpptr)
            insert_def(cstat, temp, (char *) tmpptr);

        j = next_prop(pptr, j, temp, sizeof(temp));
    }
}

/**
 * Include 'internal' defines.  These are the ones set by the compiler.
 *
 * Basically built-ins.
 *
 * @private
 * @param cstat the compile state
 */
static void
include_internal_defs(COMPSTATE * cstat)
{
    /* Create standard server defines */
    insert_def(cstat, "__version", VERSION);
    insert_def(cstat, "__muckname", tp_muckname);
    insert_intdef(cstat, "__fuzzball__", 1);
    insert_intdef(cstat, "max_variable_count", MAX_VAR);
    insert_intdef(cstat, "sorttype_caseinsens", SORTTYPE_CASEINSENS);
    insert_intdef(cstat, "sorttype_descending", SORTTYPE_DESCENDING);
    insert_intdef(cstat, "sorttype_case_ascend", SORTTYPE_CASE_ASCEND);
    insert_intdef(cstat, "sorttype_nocase_ascend", SORTTYPE_NOCASE_ASCEND);
    insert_intdef(cstat, "sorttype_case_descend", SORTTYPE_CASE_DESCEND);
    insert_intdef(cstat, "sorttype_nocase_descend", SORTTYPE_NOCASE_DESCEND);
    insert_intdef(cstat, "sorttype_shuffle", SORTTYPE_SHUFFLE);
    insert_def(cstat, "notify_except", "1 swap notify_exclude");

    /* Make defines for compatability to removed primitives */
    insert_def(cstat, "desc", "\"" MESGPROP_DESC "\" getpropstr");
    insert_def(cstat, "idesc", "\"" MESGPROP_IDESC "\" getpropstr");
    insert_def(cstat, "succ", "\"" MESGPROP_SUCC "\" getpropstr");
    insert_def(cstat, "osucc", "\"" MESGPROP_OSUCC "\" getpropstr");
    insert_def(cstat, "fail", "\"" MESGPROP_FAIL "\" getpropstr");
    insert_def(cstat, "ofail", "\"" MESGPROP_OFAIL "\" getpropstr");
    insert_def(cstat, "drop", "\"" MESGPROP_DROP "\" getpropstr");
    insert_def(cstat, "odrop", "\"" MESGPROP_ODROP "\" getpropstr");
    insert_def(cstat, "oecho", "\"" MESGPROP_OECHO "\" getpropstr");
    insert_def(cstat, "pecho", "\"" MESGPROP_PECHO "\" getpropstr");
    insert_def(cstat, "setdesc", "\"" MESGPROP_DESC "\" swap setprop");
    insert_def(cstat, "setidesc", "\"" MESGPROP_IDESC "\" swap setprop");
    insert_def(cstat, "setsucc", "\"" MESGPROP_SUCC "\" swap setprop");
    insert_def(cstat, "setosucc", "\"" MESGPROP_OSUCC "\" swap setprop");
    insert_def(cstat, "setfail", "\"" MESGPROP_FAIL "\" swap setprop");
    insert_def(cstat, "setofail", "\"" MESGPROP_OFAIL "\" swap setprop");
    insert_def(cstat, "setdrop", "\"" MESGPROP_DROP "\" swap setprop");
    insert_def(cstat, "setodrop", "\"" MESGPROP_ODROP "\" swap setprop");
    insert_def(cstat, "setoecho", "\"" MESGPROP_OECHO "\" swap setprop");
    insert_def(cstat, "setpecho", "\"" MESGPROP_PECHO "\" swap setprop");

    /* Events */
    insert_def(cstat, "event_wait", "0 array_make event_waitfor");
    insert_def(cstat, "tread",
               "\"__tread\" timer_start { \"TIMER.__tread\" \"READ\" }list event_waitfor nip \"READ\" strcmp if \"\" 0 else read 1 \"__tread\" timer_stop then");

    /* MUF Error defines */
    insert_def(cstat, "err_divzero?", "0 is_set?");
    insert_def(cstat, "err_nan?", "1 is_set?");
    insert_def(cstat, "err_imaginary?", "2 is_set?");
    insert_def(cstat, "err_fbounds?", "3 is_set?");
    insert_def(cstat, "err_ibounds?", "4 is_set?");

    /* Array convenience defines */
    insert_def(cstat, "}array", "} array_make");
    insert_def(cstat, "}list", "} array_make");
    insert_def(cstat, "}dict", "} 2 / array_make_dict");
    insert_def(cstat, "}join", "} array_make \"\" array_join");
    insert_def(cstat, "}cat", "} array_make array_interpret");
    insert_def(cstat, "}tell", "} array_make me @ 1 array_make array_notify");
    insert_def(cstat, "[]", "array_getitem");
    insert_def(cstat, "->[]", "array_setitem");
    insert_def(cstat, "[]<-", "array_appenditem");
    insert_def(cstat, "[..]", "array_getrange");
    insert_def(cstat, "array_diff", "2 array_ndiff");
    insert_def(cstat, "array_union", "2 array_nunion");
    insert_def(cstat, "array_intersect", "2 array_nintersect");

    /* Case support */
    insert_def(cstat, "case", "begin dup");
    insert_def(cstat, "when", "if pop");
    insert_def(cstat, "end", "break then dup");
    insert_def(cstat, "default", "pop 1 if");
    insert_def(cstat, "endcase", "pop pop 1 until");

#ifdef MCPGUI_SUPPORT
    /* GUI dialog types */
    insert_def(cstat, "d_simple", "\"simple\"");
    insert_def(cstat, "d_tabbed", "\"tabbed\"");
    insert_def(cstat, "d_helper", "\"helper\"");

    /* GUI control types */
    insert_def(cstat, "c_menu", "\"menu\"");
    insert_def(cstat, "c_datum", "\"datum\"");
    insert_def(cstat, "c_label", "\"text\"");
    insert_def(cstat, "c_image", "\"image\"");
    insert_def(cstat, "c_hrule", "\"hrule\"");
    insert_def(cstat, "c_vrule", "\"vrule\"");
    insert_def(cstat, "c_button", "\"button\"");
    insert_def(cstat, "c_checkbox", "\"checkbox\"");
    insert_def(cstat, "c_radiobtn", "\"radio\"");
    insert_def(cstat, "c_password", "\"password\"");
    insert_def(cstat, "c_edit", "\"edit\"");
    insert_def(cstat, "c_multiedit", "\"multiedit\"");
    insert_def(cstat, "c_combobox", "\"combobox\"");
    insert_def(cstat, "c_spinner", "\"spinner\"");
    insert_def(cstat, "c_scale", "\"scale\"");
    insert_def(cstat, "c_listbox", "\"listbox\"");
    insert_def(cstat, "c_tree", "\"tree\"");
    insert_def(cstat, "c_frame", "\"frame\"");
    insert_def(cstat, "c_notebook", "\"notebook\"");

    /* Backwards compatibility for old GUI dialog creation prims */
    insert_def(cstat, "gui_dlog_simple", "d_simple swap 0 array_make_dict gui_dlog_create");
    insert_def(cstat, "gui_dlog_tabbed",
               "d_tabbed swap \"panes\" over array_keys array_make \"names\" 4 rotate array_vals array_make 2 array_make_dict gui_dlog_create");
    insert_def(cstat, "gui_dlog_helper",
               "d_helper swap \"panes\" over array_keys array_make \"names\" 4 rotate array_vals array_make 2 array_make_dict gui_dlog_create");
#endif
    /* Regex */
    insert_intdef(cstat, "reg_icase", MUF_RE_ICASE);
    insert_intdef(cstat, "reg_all", MUF_RE_ALL);
    insert_intdef(cstat, "reg_extended", MUF_RE_EXTENDED);

    /* Deprecations */
    insert_def(cstat, "dbcmp", "=");
    insert_def(cstat, "name-ok?", "\"exit\" ext-name-ok?");
    insert_def(cstat, "pname-ok?", "\"player\" ext-name-ok?");
    insert_def(cstat, "truename", "name");
    insert_intdef(cstat, "bg_mode", BACKGROUND);
    insert_intdef(cstat, "fg_mode", FOREGROUND);
    insert_intdef(cstat, "pr_mode", PREEMPT);
}

/**
 * Initialize definitions that exist prior to even looking at the code
 *
 * Loads built in defines and prop-based defines into defhash
 *
 * @private
 * @param cstat our compile state struct
 */
static void
init_defs(COMPSTATE * cstat)
{
    /* initialize hash table */
    for (int i = 0; i < DEFHASHSIZE; i++) {
        cstat->defhash[i] = NULL;
    }

    /* Create standard server defines */
    include_internal_defs(cstat);

    /* Include any defines set in #0's _defs/ propdir. */
    include_defs(cstat, (dbref) 0);

    /* Include any defines set in program owner's _defs/ propdir. */
    include_defs(cstat, OWNER(cstat->program));
}

/**
 * Delete a program from memory
 *
 * "Uncompile" is a really awkward way to put this, because (to me) it
 * sounds a lot like "decompile".  It has absolutely nothing to do with
 * decompiling.  "free_program" is probably more accurate.
 *
 * @param i the program ref to decompile
 */
void
uncompile_program(dbref i)
{
    if (PROGRAM_INSTANCES_IN_PRIMITIVE(i) > 0) {
        return;
    }

    /* free program */
    (void) dequeue_prog(i, 1);
    free_prog(i);
    cleanpubs(PROGRAM_PUBS(i));
    PROGRAM_SET_PUBS(i, NULL);
    clean_mcpbinds(PROGRAM_MCPBINDS(i));
    PROGRAM_SET_MCPBINDS(i, NULL);
    PROGRAM_SET_PROFTIME(i, 0, 0);
    PROGRAM_SET_CODE(i, NULL);
    PROGRAM_SET_SIZ(i, 0);
    PROGRAM_SET_START(i, NULL);
}

/**
 * Implementation of @uncompile command
 *
 * Removes all compiled programs from memory.  Does not do permission
 * checking.
 *
 * @param player the player to notify when the uncompile is done.
 */
void
do_uncompile(dbref player)
{
    for (dbref i = 0; i < db_top; i++) {
        if (Typeof(i) == TYPE_PROGRAM) {
            uncompile_program(i);
        }
    }

    notify_nolisten(player, "All programs decompiled.", 1);
}

/**
 * Free ("uncompile") unused programs
 *
 * An unused program is one who's program object has a ts_lastused time
 * that is older than tp_clean_interval seconds and also is not set
 * ABODE (Autostart) or INTERNAL.
 *
 * This scans over the entire database and is thus kind of expensive
 */
void
free_unused_programs()
{
    time_t now = time(NULL);

    /* @TODO Consider putting all programs on a linked list of programs,
     *       so that we do not have to scan the ENTIRE DB everytime we
     *       do this.  We could even just put the active programs on the
     *       list, which for most MUCKs will be a relatively small and
     *       static list.
     *
     *       Yah, this is a lil easier said than done :)
     */
    for (dbref i = 0; i < db_top; i++) {
        if ((Typeof(i) == TYPE_PROGRAM) && !(FLAGS(i) & (ABODE | INTERNAL)) &&
            (now - DBFETCH(i)->ts_lastused > tp_clean_interval)
            && (PROGRAM_INSTANCES(i) == 0)) {
            uncompile_program(i);
        }
    }
}

/**
 * This attempts to optimize certain variable resolution calls
 *
 * This does an optimization that maps certain instruction types,
 * specifically PROG_LVAR_BANG and PROG_SVAR_BANG, will be converted
 * to PROG_LVAR_AT_CLEAR or PROG_SVAR_AT_CLEAR as appropriate.
 *
 * In other words, any var@'s that have a following var! become var@-clear's
 *
 * The purpose of this seems to be a memory optimization, though this
 * function could benefit substantially from notes from its original
 * author.
 *
 * There are many cases where the optimization can't happen; by my
 * estimation it looks like the optimization is pretty rare.  The
 * reasoning by each skip is fairly well documented within the code
 * and doesn't really bear repeating here.
 *
 * The compile state structure will be potentially modified by this call.
 *
 * @private
 * @param cstat the compile state structure
 * @param first the instruction to start with
 * @param AtNo the instruction number for @
 * @param BangNo the instruction number for !
 */
static void
MaybeOptimizeVarsAt(COMPSTATE * cstat, struct INTERMEDIATE *first, int AtNo,
                    int BangNo)
{
    struct INTERMEDIATE *curr = first->next;
    struct INTERMEDIATE *ptr;
    int farthest = 0;
    int i;
    int lvarflag = 0;

    if (first->flags & INTMEDFLG_INTRY)
        return;

    if (first->in.type == PROG_LVAR_AT || first->in.type == PROG_LVAR_AT_CLEAR)
        lvarflag = 1;

    for (; curr; curr = curr->next) {
        if (curr->flags & INTMEDFLG_INTRY)
            return;

        switch (curr->in.type) {
            case PROG_PRIMITIVE:
                /*
                 * Don't trust any physical @ or !'s in the code, someone
                 * may be indirectly referencing the scoped variable
                 *
                 * Don't trust any explicit jmp's in the code.
                 */
                if ((curr->in.data.number == AtNo) ||
                    (curr->in.data.number == BangNo) ||
                    (curr->in.data.number == IN_JMP)) {
                    return;
                }

                if (lvarflag) {
                    /*
                     * For lvars, don't trust the following prims...
                     *
                     *   EXITs escape the code path without leaving lvar scope.
                     *   EXECUTEs escape the code path without leaving lvar
                     *            scope.
                     *   CALLs cause re-entrancy problems.
                     */
                    if (curr->in.data.number == IN_RET ||
                        curr->in.data.number == IN_EXECUTE ||
                        curr->in.data.number == IN_CALL) {
                        return;
                    }
                }

                break;
            case PROG_LVAR_AT:
            case PROG_LVAR_AT_CLEAR:
                if (lvarflag) {
                    if (curr->in.data.number == first->in.data.number) {
                        /* Can't optimize if references to the variable 
                         * found before a var!
                         */
                        return;
                    }
                }

                break;
            case PROG_SVAR_AT:
            case PROG_SVAR_AT_CLEAR:
                if (!lvarflag) {
                    if (curr->in.data.number == first->in.data.number) {
                        /* Can't optimize if references to the variable found
                         * before a var!
                         */
                        return;
                    }
                }

                break;
            case PROG_LVAR_BANG:
                if (lvarflag) {
                    if (first->in.data.number == curr->in.data.number) {
                        if (curr->no <= farthest) {
                            /* cannot optimize as we are within a branch */
                            return;
                        } else {
                            /* Optimize it! */
                            first->in.type = PROG_LVAR_AT_CLEAR;
                            return;
                        }
                    }
                }

                break;
            case PROG_SVAR_BANG:
                if (!lvarflag) {
                    if (first->in.data.number == curr->in.data.number) {
                        if (curr->no <= farthest) {
                            /* cannot optimize as we are within a branch */
                            return;
                        } else {
                            /* Optimize it! */
                            first->in.type = PROG_SVAR_AT_CLEAR;
                            return;
                        }
                    }
                }

                break;
            case PROG_EXEC:
                if (lvarflag) {
                    /* Don't try to optimize lvars over execs */
                    return;
                }

                break;
            case PROG_IF:
            case PROG_TRY:
            case PROG_JMP:
                ptr = cstat->addrlist[curr->in.data.number];
                i = cstat->addroffsets[curr->in.data.number];

                while (ptr->next && i-- > 0)
                    ptr = ptr->next;

                if (ptr->no <= first->no) {
                    /* Can't optimize as we've exited the code branch the @
                     * is in.
                     */
                    return;
                }

                if (ptr->no > farthest)
                    farthest = ptr->no;

                break;
            case PROG_FUNCTION:
                /* Don't try to optimize over functions */
                return;
        }
    }
}

/**
 * Remove the next intermediate after 'curr' in the linked list.
 *
 * This also shifts all the addresses in addrlist down and frees
 * the memory of the removed item in the linked list.  The
 * number of words book-keeping is also updated.
 *
 * The offset lists aren't updated which is a little fishy to me,
 * but this seems to work so I won't complain about it.
 *
 * NULL checking is done so if the next node does not exist, this
 * will not crash.
 *
 * @private
 * @param cstat the compile state struct
 * @param curr the current node, the node after this is deleted
 */
static void
RemoveNextIntermediate(COMPSTATE * cstat, struct INTERMEDIATE *curr)
{
    struct INTERMEDIATE *tmp;

    if (!curr->next) {
        return;
    }

    tmp = curr->next;

    for (unsigned int i = 0; i < cstat->addrcount; i++) {
        if (cstat->addrlist[i] == tmp) {
            cstat->addrlist[i] = curr;
        }
    }

    curr->next = curr->next->next;
    free_intermediate_node(tmp);
    cstat->nowords--;
}

/**
 * Remove the selected INTERMEDIATE from the linked list
 *
 * Note that this does not work on the *last* intermediate; in that
 * case, this just returns.  This is because it is a single-y linked
 * list, and how it actualy works is it copies ->next into the 'curr'ent
 * node and then frees the ->next node, effectively moving the whole
 * list up a node without requiring any knowledge of previous nodes.
 *
 * I guess the use case of the last node in the list doesn't matter!
 *
 * @private
 * @param cstat the compile state structure
 * @param curr the INTERMEDIATE node to remove from the list
 */
static void
RemoveIntermediate(COMPSTATE * cstat, struct INTERMEDIATE *curr)
{
    /*
     * Our method of removal does not allow us to support removing the
     * last node.
     */
    if (!curr->next) {
        return;
    }

    curr->in.line = curr->next->in.line;
    curr->in.type = curr->next->in.type;

    memcpy(&(curr->in.data), &(curr->next->in.data), sizeof(curr->in.data));

    curr->next->in.type = PROG_INTEGER;
    curr->next->in.data.number = 0;

    RemoveNextIntermediate(cstat, curr);
}

/**
 * Checks to see if the next 'count' INTERMEDIATE's in a list are contiguous
 *
 * This means two things:
 *
 * * That there are 'count' INTERMEDIATEs left in the list, starting with
 *   'ptr'
 *
 * * That none of the 'count' intermediates starting with 'ptr' are
 *   referenced by a JUMP (IMMFLAG_REFERENCED)
 *
 * @private
 * @param Flags an array of flag information for the intermediates in question
 * @param ptr the start of where we want to start counting contiguous structs
 * @param count the number of contiguous structs we want
 * @return boolean true if there are 'count' contiguous, false otherwise.
 */
static int
ContiguousIntermediates(int *Flags, struct INTERMEDIATE *ptr, int count)
{
    while (count-- > 0) {
        if (!ptr) {
            return 0;
        }

        if ((Flags[ptr->no] & IMMFLAG_REFERENCED)) {
            return 0;
        }

        ptr = ptr->next;
    }

    return 1;
}

/**
 * Is the given INTERMEDIATE a primitive of type 'primnum'?
 *
 * @private
 * @param ptr the intermediate to check
 * @param primnum the primitive to check for
 * @return boolean true if ptr is a primitive of type primnum, false otherwise
 */
static int
IntermediateIsPrimitive(struct INTERMEDIATE *ptr, int primnum)
{
    if (ptr && ptr->in.type == PROG_PRIMITIVE) {
        if (ptr->in.data.number == primnum) {
            return 1;
        }
    }

    return 0;
}

/**
 * Is the given INTERMEDIATE an integer of value 'val'?
 *
 * @private
 * @param ptr the intermediate to check
 * @param val the value to check for
 * @return boolean true if ptr is an integer of value val, false otherwise
 */
static int
IntermediateIsInteger(struct INTERMEDIATE *ptr, int val)
{
    if (ptr && ptr->in.type == PROG_INTEGER) {
        if (ptr->in.data.number == val) {
            return 1;
        }
    }

    return 0;
}

/**
 * Is the given INTERMEDIATE an string of value 'val'?
 *
 * @private
 * @param ptr the intermediate to check
 * @param val the value to check for
 * @return boolean true if ptr is an string of value val, false otherwise
 */
static int
IntermediateIsString(struct INTERMEDIATE *ptr, const char *val)
{
    const char *myval;

    if (ptr && ptr->in.type == PROG_STRING) {
        myval = DoNullInd(ptr->in.data.string);

        if (!strcmp(myval, val)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Iterates over all the intermediates in a COMPSTATE and tries to optimize
 *
 * Optimizations are mostly in the form of taking certain common (or sometimes
 * uncommon) series of primitives or other operations and boiling them
 * down.  For instance, the code "me @ swap notify" can be optimized to
 * simply "tell".  For older MUFs, this combination of operations is
 * incredibly common.
 *
 * This potentially modifies the intermediates list in cstat.
 *
 * @private
 * @param cstat the compile state structure
 * @param force_err_display boolean if true, errors will be displayed
 *        to cstat->player
 * @return integer number of instructions we have optimized away
 */
static int
OptimizeIntermediate(COMPSTATE * cstat, int force_err_display)
{
    int *Flags;
    unsigned int i;
    size_t count = 0;
    int old_instr_count = cstat->nowords;
    int AtNo = get_primitive("@");
    int BangNo = get_primitive("!");
    int SwapNo = get_primitive("swap");
    int PopNo = get_primitive("pop");
    int OverNo = get_primitive("over");
    int PickNo = get_primitive("pick");
    int NipNo = get_primitive("nip");
    int TuckNo = get_primitive("tuck");
    int DupNo = get_primitive("dup");
    int RotNo = get_primitive("rot");
    int RrotNo = get_primitive("-rot");
    int RotateNo = get_primitive("rotate");
    int NotNo = get_primitive("not");
    int StrcmpNo = get_primitive("strcmp");
    int StringcmpNo = get_primitive("stringcmp");
    int EqualsNo = get_primitive("=");
    int PlusNo = get_primitive("+");
    int MinusNo = get_primitive("-");
    int MultNo = get_primitive("*");
    int DivNo = get_primitive("/");
    int ModNo = get_primitive("%");
    int DecrNo = get_primitive("--");
    int IncrNo = get_primitive("++");
    int NotequalsNo = get_primitive("!=");
    int NotifyNo = get_primitive("notify");
    int TellNo = get_primitive("tell");

    /*
     * Code assumes everything is setup nicely, if not, bad things will happen
     */
    if (!cstat->first_word)
        return 0;

    /* renumber the instruction chain */
    for (struct INTERMEDIATE *curr = cstat->first_word; curr; curr = curr->next)
        curr->no = count++;

    if ((Flags = malloc(sizeof(int) * count)) == 0)
        return 0;

    memset(Flags, 0, sizeof(int) * count);

    /* Mark instructions which jumps reference */
    for (struct INTERMEDIATE *curr = cstat->first_word; curr; curr = curr->next) {
        switch (curr->in.type) {
            case PROG_ADD:
            case PROG_IF:
            case PROG_TRY:
            case PROG_JMP:
            case PROG_EXEC:
                i = cstat->addrlist[curr->in.data.number]->no +
                    cstat->addroffsets[curr->in.data.number];

                /*
                 * unreachable code (outside of any word, like ': x ; if then'
                 * can have references one-past-the-end of a file
                 */
                if (i < count)
                    Flags[i] |= IMMFLAG_REFERENCED;

                break;
        }
    }

    for (struct INTERMEDIATE *curr = cstat->first_word; curr;) {
        int advance = 1;

        /* @TODO So the structure here is very long form and seems pretty
         *       copy/paste.  At the very least, instead of layering 3 if's
         *       around an if-else, we could reduce these to a single if
         *       around the if/else to make this less of a nested nightmare.
         *
         *       A better structure would probably be to make some kind of
         *       mapping of in.type to callback functions to break this up
         *       a bit.  There's a lot of copy/paste but for some of these
         *       (like the integer and primitive calls) there's enough logic
         *       variety that it probably can't be reduced much.
         *
         *       The primitive ID's loaded at the start of this function
         *       could be put in a 'optimization context' struct so they are
         *       easily passed around and added to.
         */
        switch (curr->in.type) {
            case PROG_VAR:
                /* me @ swap notify  ==>  tell */
                /* me @ s notify  ==>  s tell */
                if (curr->in.data.number == 0 &&
                    IntermediateIsPrimitive(curr->next, AtNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 2)) {
                        if (IntermediateIsPrimitive(curr->next->next->next, NotifyNo)) {
                            if (curr->next->next->in.type == PROG_STRING) {
                                curr->next->next->next->in.data.number = TellNo;
                                RemoveNextIntermediate(cstat, curr);
                                RemoveIntermediate(cstat, curr);
                            } else if (IntermediateIsPrimitive(curr->next->next, SwapNo)) {
                                curr->in.type = PROG_PRIMITIVE;
                                curr->in.data.number = TellNo;
                                RemoveNextIntermediate(cstat, curr);
                                RemoveNextIntermediate(cstat, curr);
                                RemoveNextIntermediate(cstat, curr);
                            }
                        }
                    }
                }

                break;
            case PROG_LVAR:
                /* lvar !  ==>  lvar! */
                /* lvar @  ==>  lvar@ */
                if (curr->next && curr->next->in.type == PROG_PRIMITIVE) {
                    if (curr->next->in.data.number == AtNo) {
                        if (ContiguousIntermediates(Flags, curr->next, 1)) {
                            curr->in.type = PROG_LVAR_AT;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }

                    if (curr->next->in.data.number == BangNo) {
                        if (ContiguousIntermediates(Flags, curr->next, 1)) {
                            curr->in.type = PROG_LVAR_BANG;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                break;
            case PROG_SVAR:
                /* svar !  ==>  svar! */
                /* svar @  ==>  svar@ */
                if (curr->next && curr->next->in.type == PROG_PRIMITIVE) {
                    if (curr->next->in.data.number == AtNo) {
                        if (ContiguousIntermediates(Flags, curr->next, 1)) {
                            curr->in.type = PROG_SVAR_AT;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }

                    if (curr->next->in.data.number == BangNo) {
                        if (ContiguousIntermediates(Flags, curr->next, 1)) {
                            curr->in.type = PROG_SVAR_BANG;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                break;
            case PROG_STRING:
                if (IntermediateIsString(curr, "")) {
                    if (ContiguousIntermediates(Flags, curr->next, 3)) {
                        /* "" strcmp 0 =  ==>   not */
                        if (IntermediateIsPrimitive(curr->next, StrcmpNo)) {
                            if (IntermediateIsInteger(curr->next->next, 0)) {
                                if (IntermediateIsPrimitive(curr->next->next->next,
                                    EqualsNo)) {
                                    free(curr->in.data.string);

                                    curr->in.type = PROG_PRIMITIVE;
                                    curr->in.data.number = NotNo;
                                    RemoveNextIntermediate(cstat, curr);
                                    RemoveNextIntermediate(cstat, curr);
                                    RemoveNextIntermediate(cstat, curr);
                                    advance = 0;
                                    break;
                                }
                            }
                        }

                        /* "" stringcmp 0 =  ==>   not */
                        if (IntermediateIsPrimitive(curr->next, StringcmpNo)) {
                            if (IntermediateIsInteger(curr->next->next, 0)) {
                                if (IntermediateIsPrimitive(curr->next->next->next,
                                    EqualsNo)) {
                                    free(curr->in.data.string);

                                    curr->in.type = PROG_PRIMITIVE;
                                    curr->in.data.number = NotNo;
                                    RemoveNextIntermediate(cstat, curr);
                                    RemoveNextIntermediate(cstat, curr);
                                    RemoveNextIntermediate(cstat, curr);
                                    advance = 0;
                                    break;
                                }
                            }
                        }
                    }
                }

                break;
            case PROG_INTEGER:
                /* consolidate constant integer calculations */
                if (ContiguousIntermediates(Flags, curr->next, 2)) {
                    if (curr->next->in.type == PROG_INTEGER) {
                        /* Int Int +  ==>  Sum */
                        if (IntermediateIsPrimitive(curr->next->next, PlusNo)) {
                            curr->in.data.number += curr->next->in.data.number;
                            RemoveNextIntermediate(cstat, curr);
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }

                        /* Int Int -  ==>  Diff */
                        if (IntermediateIsPrimitive(curr->next->next, MinusNo)) {
                            curr->in.data.number -= curr->next->in.data.number;
                            RemoveNextIntermediate(cstat, curr);
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }

                        /* Int Int *  ==>  Prod */
                        if (IntermediateIsPrimitive(curr->next->next, MultNo)) {
                            curr->in.data.number *= curr->next->in.data.number;
                            RemoveNextIntermediate(cstat, curr);
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }

                        /* Int Int /  ==>  Div  */
                        if (IntermediateIsPrimitive(curr->next->next, DivNo)) {
                            if (curr->next->in.data.number == 0) {
                                if (!(curr->next->next->flags & INTMEDFLG_DIVBYZERO)) {
                                    curr->next->next->flags |= INTMEDFLG_DIVBYZERO;

                                    if (force_err_display) {
                                        notifyf(cstat->player,
                                                "Warning on line %i: Divide by zero",
                                                curr->next->next->in.line);
                                    }
                                }
                            } else if (curr->next->in.data.number == -1 &&
                                       curr->in.data.number == INT_MIN) {
                                if (!(curr->next->next->flags & INTMEDFLG_OVERFLOW)) {
                                    curr->next->next->flags |= INTMEDFLG_OVERFLOW;

                                    if (force_err_display) {
                                        notifyf(cstat->player,
                                                "Warning on line %i: Integer overflow",
                                                curr->next->next->in.line);
                                    }
                                }
                            } else {
                                curr->in.data.number /= curr->next->in.data.number;
                                RemoveNextIntermediate(cstat, curr);
                                RemoveNextIntermediate(cstat, curr);
                                advance = 0;
                            }

                            break;
                        }

                        /* Int Int %  ==>  Mod */
                        if (IntermediateIsPrimitive(curr->next->next, ModNo)) {
                            if (curr->next->in.data.number == 0) {
                                if (!(curr->next->next->flags & INTMEDFLG_MODBYZERO)) {
                                    curr->next->next->flags |= INTMEDFLG_MODBYZERO;

                                    if (force_err_display) {
                                        notifyf(cstat->player,
                                                "Warning on line %i: Modulus by zero",
                                                curr->next->next->in.line);
                                    }
                                }
                            } else if (curr->next->in.data.number == -1 &&
                                       curr->in.data.number == INT_MIN) {
                                if (!(curr->next->next->flags & INTMEDFLG_OVERFLOW)) {
                                    curr->next->next->flags |= INTMEDFLG_OVERFLOW;

                                    if (force_err_display) {
                                        notifyf(cstat->player,
                                                "Warning on line %i: Integer overflow",
                                                curr->next->next->in.line);
                                    }
                                }
                            } else {
                                curr->in.data.number %= curr->next->in.data.number;
                                RemoveNextIntermediate(cstat, curr);
                                RemoveNextIntermediate(cstat, curr);
                                advance = 0;
                            }

                            break;
                        }
                    }
                }

                /* 0 =  ==>  not */
                if (IntermediateIsInteger(curr, 0)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, EqualsNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = NotNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                /* 1 +  ==>  ++ */
                if (IntermediateIsInteger(curr, 1)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, PlusNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = IncrNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                /* 1 -  ==>  -- */
                if (IntermediateIsInteger(curr, 1)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, MinusNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = DecrNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                /* 1 pick  ==>  dup */
                if (IntermediateIsInteger(curr, 1)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, PickNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = DupNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                        }
                    }
                }

                /* 2 pick  ==>  over */
                if (IntermediateIsInteger(curr, 2)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, PickNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = OverNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                        }
                    }
                }

                /* 3 rotate  ==>  rot */
                if (IntermediateIsInteger(curr, 3)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, RotateNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = RotNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                        }
                    }
                }

                /* -3 rotate  ==>  -rot */
                if (IntermediateIsInteger(curr, -3)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, RotateNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = RrotNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                        }
                    }
                }

                /* 2|-2 rotate  ==>  swap */
                if (IntermediateIsInteger(curr, 2) || IntermediateIsInteger(curr, -2)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, RotateNo)) {
                            curr->in.type = PROG_PRIMITIVE;
                            curr->in.data.number = SwapNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                        }
                    }
                }

                /* 1|0|-1 rotate  ==>  (nothing) */
                if (IntermediateIsInteger(curr, 1) || IntermediateIsInteger(curr, 0) || IntermediateIsInteger(curr, -1)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, RotateNo)) {
                            RemoveNextIntermediate(cstat, curr);
                            RemoveIntermediate(cstat, curr);
                            advance = 0;
                        }
                    }
                }

                break;
            case PROG_PRIMITIVE:
                /* rot rot swap  ==>  swap rot */
                if (IntermediateIsPrimitive(curr, RotNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 2)) {
                        if (IntermediateIsPrimitive(curr->next, RotNo)) {
                            if (IntermediateIsPrimitive(curr->next->next, SwapNo)) {
                                curr->in.data.number = SwapNo;
                                curr->next->in.data.number = RotNo;
                                RemoveNextIntermediate(cstat, curr->next);
                                advance = 0;
                                break;
                            }
                        }
                    }
                }

                /* rot rot  ==>  -rot */
                if (IntermediateIsPrimitive(curr, RotNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, RotNo)) {
                            curr->in.data.number = RrotNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                /* -rot -rot  ==>  rot */
                if (IntermediateIsPrimitive(curr, RrotNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, RrotNo)) {
                            curr->in.data.number = RotNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                if (IntermediateIsPrimitive(curr, SwapNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        /* swap pop  ==>  nip */
                        if (IntermediateIsPrimitive(curr->next, PopNo)) {
                            curr->in.data.number = NipNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }

                        /* swap over  ==>  tuck */
                        if (IntermediateIsPrimitive(curr->next, OverNo)) {
                            curr->in.data.number = TuckNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                /* not not if  ==>  if */
                if (IntermediateIsPrimitive(curr, NotNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 2)) {
                        if (IntermediateIsPrimitive(curr->next, NotNo)) {
                            if (curr->next->next->in.type == PROG_IF) {
                                RemoveIntermediate(cstat, curr);
                                RemoveIntermediate(cstat, curr);
                                advance = 0;
                                break;
                            }
                        }
                    }
                }

                /* = not  ==>  != */
                if (IntermediateIsPrimitive(curr, EqualsNo)) {
                    if (ContiguousIntermediates(Flags, curr->next, 1)) {
                        if (IntermediateIsPrimitive(curr->next, NotNo)) {
                            curr->in.data.number = NotequalsNo;
                            RemoveNextIntermediate(cstat, curr);
                            advance = 0;
                            break;
                        }
                    }
                }

                break;
        }

        if (advance) {
            curr = curr->next;
        }
    }

    /* Turn all var@'s which have a following var! into a var@-clear */

    for (struct INTERMEDIATE *curr = cstat->first_word; curr; curr = curr->next)
        if (curr->in.type == PROG_SVAR_AT || curr->in.type == PROG_LVAR_AT)
            MaybeOptimizeVarsAt(cstat, curr, AtNo, BangNo);

    free(Flags);
    return (old_instr_count - cstat->nowords);
}

/**
 * Set the starting address of the program
 *
 * This takes into account an alternate entry point
 *
 * @private
 * @param cstat the compile state
 */
static void
set_start(COMPSTATE * cstat)
{
    PROGRAM_SET_SIZ(cstat->program, cstat->nowords);

    /* address instr no is resolved before this gets called. */
    if (cstat->alt_entrypoint) {
        PROGRAM_SET_START(cstat->program,
                          (PROGRAM_CODE(cstat->program)
                           + cstat->alt_entrypoint->code->no));
    } else {
        PROGRAM_SET_START(cstat->program,
                          (PROGRAM_CODE(cstat->program)
                           + cstat->procs->code->no));
    }
}

/* @TODO So this comment was already in here when I started.  I made a note
 *       above thinking about how to make the optimizers more generic and
 *       this idea takes it a step further.  Maybe a hybrid idea, of having
 *       a mapping of functions and then a few different helper methods to
 *       reduce redundant code would work.  I'm leaving this as-is as
 *       ideas for the future.
 *
 *
 * Genericized Optimizer ideas:
 *
 * const int OI_ANY = -121314;   // arbitrary unlikely-to-be-needed value.
 *
 * typedef enum {
 *     OI_KEEP,
 *     OI_CHGVAL,
 *     OI_CHGTYPE,
 *     OI_REPLACE,
 *     OI_DELETE
 * } OI_ACTION;
 *
 * OPTIM* option_new();
 * void optim_free(OPTIM* optim);
 * void optim_add_raw  (OPTIM* optim, struct INTERMEDIATE* originst,
 *                      OI_ACTION action, struct INTERMEDIATE* newinst);
 * void optim_add_type (OPTIM* optim, int origtype,
 *                      OI_ACTION action, int newtype);
 * void optim_add_prim (OPTIM* optim, const char* origprim,
 *                      OI_ACTION action, int newval);
 * void optim_add_int  (OPTIM* optim, int origval,
 *                      OI_ACTION action, int newval);
 * void optim_add_str  (OPTIM* optim, const char* origval,
 *                      OI_ACTION action, int newval);
 *
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_str (optim, "",       OI_DELETE, 0);
 * optim_add_prim(optim, "strcmp", OI_CHGVAL, get_primitive("not"));
 * optim_add_int (optim, 0,        OI_DELETE, 0);
 * optim_add_prim(optim, "=",      OI_DELETE, 0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_str(optim, "",        OI_DELETE, 0);
 * optim_add_prim(optim, "strcmp", OI_DELETE, 0);
 * optim_add_prim(optim, "not",    OI_KEEP,   0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_prim(optim, "rot",  OI_CHGVAL, get_primitive("swap"));
 * optim_add_prim(optim, "rot",  OI_KEEP,   0);
 * optim_add_prim(optim, "swap", OI_DELETE, 0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_type(optim, PROG_SVAR, OI_CHGTYPE, PROG_SVAR_AT);
 * optim_add_prim(optim, "@",       OI_DELETE,   0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_type(optim, PROG_SVAR, OI_CHGTYPE, PROG_SVAR_BANG);
 * optim_add_prim(optim, "!",       OI_DELETE,   0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_int (optim, 0,   OI_DELETE, 0);
 * optim_add_prim(optim, "=", OI_CHGVAL, get_primitive("not"));
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_prim(optim, "not",   OI_DELETE, 0);
 * optim_add_prim(optim, "not",   OI_DELETE, 0);
 * optim_add_type(optim, PROG_IF, OI_KEEP,   0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_int (optim, 0,       OI_DELETE,  0);
 * optim_add_type(optim, PROG_IF, OI_CHGTYPE, PROG_JMP);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_int (optim, 1,       OI_DELETE, 0);
 * optim_add_type(optim, PROG_IF, OI_DELETE, 0);
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_int (optim, 1,   OI_DELETE, 0);
 * optim_add_prim(optim, "+", OI_CHGVAL, get_primitive("++"));
 *
 * OPTIM* optim = optim_new(cstat);
 * optim_add_int (optim, 1,   OI_DELETE, 0);
 * optim_add_prim(optim, "-", OI_CHGVAL, get_primitive("--"));
 *
 */

/**
 * Allocate an address structure
 *
 * @private
 * @param cstat the compile state
 * @param offset address offset from 'codestart' instruction
 * @param codestart the first instruction of the program
 * @return newly allocated and initialized prog_addr structure
 */
static struct prog_addr *
alloc_addr(COMPSTATE * cstat, int offset, struct inst *codestart)
{
    struct prog_addr *nu;

    nu = malloc(sizeof(struct prog_addr));

    nu->links = 1;
    nu->progref = cstat->program;
    nu->data = codestart + offset;
    return nu;
}

/**
 * This copies the compiled code into an array of 'struct inst'
 *
 * The result is put in cstat->program
 *
 * This is one of the final steps in the compile process and the result
 * is what is later run as "compiled code".
 *
 * @private
 * @param cstat the compile state struct
 */
static void
copy_program(COMPSTATE * cstat)
{
    /*
     * Everything should be peachy keen now, so we don't do any error checking
     */
    struct inst *code;
    int i, varcnt;

    /* Except for this error checking :) */
    if (!cstat->first_word)
        v_abort_compile(cstat, "Nothing to compile.");

    code = malloc(sizeof(struct inst) * (size_t)(cstat->nowords + 1));

    i = 0;
    for (struct INTERMEDIATE *curr = cstat->first_word; curr; curr = curr->next) {
        code[i].type = curr->in.type;
        code[i].line = curr->in.line;

        switch (code[i].type) {
            case PROG_PRIMITIVE:
            case PROG_INTEGER:
            case PROG_SVAR:
            case PROG_SVAR_AT:
            case PROG_SVAR_AT_CLEAR:
            case PROG_SVAR_BANG:
            case PROG_LVAR:
            case PROG_LVAR_AT:
            case PROG_LVAR_AT_CLEAR:
            case PROG_LVAR_BANG:
            case PROG_VAR:
                code[i].data.number = curr->in.data.number;
                break;
            case PROG_FLOAT:
                code[i].data.fnumber = curr->in.data.fnumber;
                break;
            case PROG_STRING:
                code[i].data.string = curr->in.data.string ?
                    alloc_prog_string(curr->in.data.string->data) : 0;
                break;
            case PROG_FUNCTION:
                code[i].data.mufproc = malloc(sizeof(struct muf_proc_data));
                code[i].data.mufproc->procname = strdup(curr->in.data.mufproc->procname);
                code[i].data.mufproc->vars = varcnt = curr->in.data.mufproc->vars;
                code[i].data.mufproc->args = curr->in.data.mufproc->args;

                if (varcnt) {
                    if (curr->in.data.mufproc->varnames) {
                        code[i].data.mufproc->varnames = calloc((size_t)varcnt, sizeof(char *));

                        for (int j = 0; j < varcnt; j++) {
                            code[i].data.mufproc->varnames[j] = strdup(curr->in.data.mufproc->varnames[j]);
                        }
                    } else {
                        code[i].data.mufproc->varnames = NULL;
                    }
                } else {
                    code[i].data.mufproc->varnames = NULL;
                }

                break;
            case PROG_OBJECT:
                code[i].data.objref = curr->in.data.objref;
                break;
            case PROG_ADD:
                code[i].data.addr = alloc_addr(cstat, curr->in.data.number, code);
                break;
            case PROG_IF:
            case PROG_JMP:
            case PROG_EXEC:
            case PROG_TRY:
                code[i].data.call = code + curr->in.data.number;
                break;
            default:
                free(code);
                v_abort_compile(cstat, "Unknown type compile!  Internal error.");
        }

        i++;
    }

    PROGRAM_SET_CODE(cstat->program, code);
}

/**
 * Create a new instruction, or return an already available one.
 *
 * A list of pre-allocated instructions is kept, so we use those first.
 * When expended, we will allocate new instructions.
 *
 * @private
 * @param cstat the compile state struct
 * @return an INTERMEDIATE struct ready for use
 */
static struct INTERMEDIATE *
new_inst(COMPSTATE * cstat)
{
    struct INTERMEDIATE *nu;

    if (!cstat->nextinst) {
        nu = calloc(1, sizeof(struct INTERMEDIATE));
    } else {
        nu = cstat->nextinst;
        cstat->nextinst = nu->next;
        nu->next = NULL;
    }

    nu->flags |= (cstat->nested_trys > 0) ? INTMEDFLG_INTRY : 0;

    return nu;
}

/**
 * Return an INTERMEDIATE representing a primitive word.
 *
 * Given a token 'token' which is presumed to be a primitive if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the primitive we will be making here
 * @return an INTERMEDIATE structure representing primitive 'token'
 */
static struct INTERMEDIATE *
primitive_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu, *cur;
    int pnum;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Primitive outside procedure.");

    pnum = get_primitive(token);
    cur = nu = new_inst(cstat);

    if (pnum == IN_RET || pnum == IN_JMP) {
        for (int loop = 0; loop < cstat->nested_trys; loop++) {
            cur->no = cstat->nowords++;
            cur->in.type = PROG_PRIMITIVE;
            cur->in.line = cstat->lineno;
            cur->in.data.number = IN_TRYPOP;
            cur->next = new_inst(cstat);
            cur = cur->next;
        }

        for (int loop = 0; loop < cstat->nested_fors; loop++) {
            cur->no = cstat->nowords++;
            cur->in.type = PROG_PRIMITIVE;
            cur->in.line = cstat->lineno;
            cur->in.data.number = IN_FORPOP;
            cur->next = new_inst(cstat);
            cur = cur->next;
        }
    }

    cur->no = cstat->nowords++;
    cur->in.type = PROG_PRIMITIVE;
    cur->in.line = cstat->lineno;
    cur->in.data.number = pnum;

    return nu;
}

/**
 * Return an INTERMEDIATE representing a "self-pushing word" of type string
 *
 * Given a token 'token' which is presumed to be a string if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * The string memory is copied so the original token string does not have to
 * be kept.  The string creates is of type "struct shared_string" which has
 * reference count capabilities.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the string we will be making here
 * @return an INTERMEDIATE structure representing string 'token'
 */
static struct INTERMEDIATE *
string_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;

    if (!cstat->curr_proc)
        abort_compile(cstat, "String outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_STRING;
    nu->in.line = cstat->lineno;
    nu->in.data.string = alloc_prog_string(token);
    return nu;
}

/**
 * Return an INTERMEDIATE representing a "self-pushing word" of type float
 *
 * Given a token 'token' which is presumed to be a float if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the float we will be making here
 * @return an INTERMEDIATE structure representing float 'token'
 */
static struct INTERMEDIATE *
float_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Floating point number outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_FLOAT;
    nu->in.line = cstat->lineno;
    sscanf(token, "%lg", &(nu->in.data.fnumber));
    return nu;
}

/**
 * Return an INTERMEDIATE representing a "self-pushing word" of type number
 *
 * Given a token 'token' which is presumed to be a number if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the number we will be making here
 * @return an INTERMEDIATE structure representing number 'token'
 */
static struct INTERMEDIATE *
number_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Number outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_INTEGER;
    nu->in.line = cstat->lineno;
    nu->in.data.number = atoi(token);
    return nu;
}

/**
 * Return an INTERMEDIATE representing a subroutine call
 *
 * Do a subroutine call --- push address onto stack, then make a primitive
 * CALL.  Returns an INTERMEDIATE structure for the subroutine call.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the subroutine call we will be making here
 * @return an INTERMEDIATE structure representing the subroutine call.
 */
static struct INTERMEDIATE *
call_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    struct PROC_LIST *p;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Procedure call outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_EXEC;
    nu->in.line = cstat->lineno;

    /* Find the procedure to call */
    for (p = cstat->procs; p; p = p->next)
        if (!strcasecmp(p->name, token))
            break;

    /*
     * What happens if the procedure isn't found?  We get a PROG_EXEC
     * with no target?  Guess that never comes up.
     */
    if (p)
        nu->in.data.number = get_address(cstat, p->code, 0);

    return nu;
}

/**
 * Return an INTERMEDIATE representing a subroutine address pushed on the stack
 *
 * I.e. 'funcname
 *
 * Given a token 'token' which is presumed to be a subroutine name if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the subroutine name we will be making here
 * @return an INTERMEDIATE structure representing address 'token'
 */
static struct INTERMEDIATE *
quoted_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    struct PROC_LIST *p;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Address outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_ADD;
    nu->in.line = cstat->lineno;

    for (p = cstat->procs; p; p = p->next)
        if (!strcasecmp(p->name, token))
            break;

    if (p)
        nu->in.data.number = get_address(cstat, p->code, 0);

    return nu;
}

/**
 * Return an INTERMEDIATE representing a variable number pushed on the stack
 *
 * Given a token 'token' which is presumed to be a variable name if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * It is assumed the variable exists.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the variable name we will be making here
 * @return an INTERMEDIATE structure representing the appropriate variable num
 */
static struct INTERMEDIATE *
var_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int var_no = 0;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Local variable outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_VAR;
    nu->in.line = cstat->lineno;

    for (int i = 0; i < MAX_VAR; i++) {
        if (!cstat->variables[i])
            break;

        if (!strcasecmp(token, cstat->variables[i]))
            var_no = i;
    }

    nu->in.data.number = var_no;

    return nu;
}

/**
 * Return an INTERMEDIATE representing a variable number pushed on the stack
 *
 * Given a token 'token' which is presumed to be a variable name if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * It is assumed the variable exists.
 *
 * This call is for scoped variables.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the variable name we will be making here
 * @return an INTERMEDIATE structure representing the appropriate variable num
 */
static struct INTERMEDIATE *
svar_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int var_no = 0;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Scoped variable outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_SVAR;
    nu->in.line = cstat->lineno;

    for (int i = 0; i < MAX_VAR; i++) {
        if (!cstat->scopedvars[i])
            break;

        if (!strcasecmp(token, cstat->scopedvars[i]))
            var_no = i;
    }

    nu->in.data.number = var_no;

    return nu;
}

/**
 * Return an INTERMEDIATE representing a variable number pushed on the stack
 *
 * Given a token 'token' which is presumed to be a variable name if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * It is assumed the variable exists.
 *
 * This call is for local variables.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the variable name we will be making here
 * @return an INTERMEDIATE structure representing the appropriate variable num
 */
static struct INTERMEDIATE *
lvar_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int var_no = 0;

    if (!cstat->curr_proc)
        abort_compile(cstat, "Local variable outside procedure.");

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_LVAR;
    nu->in.line = cstat->lineno;

    for (int i = 0; i < MAX_VAR; i++) {
        if (!cstat->localvars[i])
            break;

        if (!strcasecmp(token, cstat->localvars[i]))
            var_no = i;
    }

    nu->in.data.number = var_no;

    return nu;
}

/**
 * Return an INTERMEDIATE representing a "self-pushing word" of type dbref
 *
 * Given a token 'token' which is presumed to be a dbref starting with # if you
 * have called this function, we will generate an INTERMEDIATE structure
 * for this word and return it.
 *
 * The format is assumed to be good.
 *
 * @private
 * @param cstat the compile state structure
 * @param token the text containing the dbref we will be making here
 * @return an INTERMEDIATE structure representing dbref 'token'
 */
static struct INTERMEDIATE *
object_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int objno;

    if (!cstat->curr_proc)
        abort_compile(cstat, "DBRef constant outside procedure.");

    objno = atol(token + 1);
    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_OBJECT;
    nu->in.line = cstat->lineno;
    nu->in.data.objref = objno;
    return nu;
}

/**
 * Determine if a given token is a 'special' keyword
 *
 * Special keywords aren't primitives; they are basically reserved words
 * or control structures such as :, ;, IF, ELSE, etc.
 *
 * @private
 * @param token the token to check
 * @return boolean, true if token is a special keyword
 */
static int
special(const char *token)
{
    return (token && !(strcasecmp(token, ":")
                  && strcasecmp(token, ";")
                  && strcasecmp(token, "IF")
                  && strcasecmp(token, "ELSE")
                  && strcasecmp(token, "THEN")
                  && strcasecmp(token, "BEGIN")
                  && strcasecmp(token, "FOR")
                  && strcasecmp(token, "FOREACH")
                  && strcasecmp(token, "UNTIL")
                  && strcasecmp(token, "WHILE")
                  && strcasecmp(token, "BREAK")
                  && strcasecmp(token, "CONTINUE")
                  && strcasecmp(token, "REPEAT")
                  && strcasecmp(token, "TRY")
                  && strcasecmp(token, "CATCH")
                  && strcasecmp(token, "CATCH_DETAILED")
                  && strcasecmp(token, "ENDCATCH")
                  && strcasecmp(token, "CALL")
                  && strcasecmp(token, "PUBLIC")
                  && strcasecmp(token, "WIZCALL")
                  && strcasecmp(token, "LVAR")
                  && strcasecmp(token, "VAR!")
                  && strcasecmp(token, "VAR")));
}

/**
 * Check to see if 'token' is a procedure call
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a procedure name, false otherwise
 */
static int
call(COMPSTATE * cstat, const char *token)
{
    for (struct PROC_LIST *i = cstat->procs; i; i = i->next)
        if (!strcasecmp(i->name, token))
            return 1;

    return 0;
}

/**
 * Check to see if 'token' is a quoted procedure name ('whatever)
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a quoted procedure name, false otherwise
 */
static int
quoted(COMPSTATE * cstat, const char *token)
{
    return (*token == '\'' && call(cstat, token + 1));
}

/**
 * Check to see if 'token' is a #dbref
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a #dbref, false otherwise
 */
static int
object(const char *token)
{
    return (*token == NUMBER_TOKEN && number(token + 1));
}

/**
 * See if 'token' starts a new string
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' starts with a '"', false otherwise
 */
static int
string(const char *token)
{
    return (token[0] == BEGINSTRING);
}


/**
 * See if 'token' is a variable name ('var' type of variable)
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a 'var' variable, false otherwise
 */
static int
variable(COMPSTATE * cstat, const char *token)
{
    for (int i = 0; i < MAX_VAR && cstat->variables[i]; i++)
        if (!strcasecmp(token, cstat->variables[i]))
            return 1;

    return 0;
}

/**
 * See if 'token' is a scoped variable name (function variable)
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a function variable, false otherwise
 */
static int
scopedvar(COMPSTATE * cstat, const char *token)
{
    for (int i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++)
        if (!strcasecmp(token, cstat->scopedvars[i]))
            return 1;

    return 0;
}

/**
 * See if 'token' is a variable name ('lvar' type of variable)
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a 'lvar' variable, false otherwise
 */
static int
localvar(COMPSTATE * cstat, const char *token)
{
    for (int i = 0; i < MAX_VAR && cstat->localvars[i]; i++)
        if (!strcasecmp(token, cstat->localvars[i]))
            return 1;

    return 0;
}

/**
 * See if 'token' is a primitive
 *
 * @private
 * @param cstat the compile state structure
 * @param token the token to check
 * @return boolean true if 'token' is a primitive, false otherwise
 */
int
primitive(const char *token)
{
    int primnum;

    primnum = get_primitive(token);

    /* don't match internal prims */
    return (primnum && primnum <= prim_count - 3);
}

/**
 * Creates an INTERMEDIATE from the given token, producing the next word
 *
 * Figures out what 'token' is (what kind of operation) and creates an
 * INTERMEDIATE structure for it; otherwise, generates an error and
 * aborts the compile (unrecognized word)
 *
 * @private
 * @param cstat our compile state structure
 * @param token the token we are compiling
 * @return an INTERMEDIATE structure that corresponds to the token
 */
static struct INTERMEDIATE *
next_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *new_word;
    static char buf[BUFFER_LEN];

    if (!token)
        return 0;

    if (call(cstat, token))
        new_word = call_word(cstat, token);
    else if (scopedvar(cstat, token))
        new_word = svar_word(cstat, token);
    else if (localvar(cstat, token))
        new_word = lvar_word(cstat, token);
    else if (variable(cstat, token))
        new_word = var_word(cstat, token);
    else if (special(token))
        new_word = process_special(cstat, token);
    else if (primitive(token))
        new_word = primitive_word(cstat, token);
    else if (string(token))
        new_word = string_word(cstat, token + 1);
    else if (number(token))
        new_word = number_word(cstat, token);
    else if (ifloat(token))
        new_word = float_word(cstat, token);
    else if (object(token))
        new_word = object_word(cstat, token);
    else if (quoted(cstat, token))
        new_word = quoted_word(cstat, token + 1);
    else {
        snprintf(buf, sizeof(buf), "Unrecognized word %s.", token);
        abort_compile(cstat, buf);
    }

    return new_word;
}

/**
 * Compile MUF code associated with a given dbref
 *
 * Live the dream, compile some MUF.  According to the original docs, this
 * does piece-meal tokenization parsing and backward checking.
 *
 * This takes some program text associated with 'program_in', converts it
 * to INTERMEDIATE's, and then does some optimizations before turning it
 * into an array of struct inst's.
 *
 * Once compiled, the program is ready to run.
 *
 * A few notes:
 *
 * - When compile starts, it kills any running copies of the program.
 * - If the program is both set A (ABODE/AUTOSTART) and owned by a wizard,
 *   it will be run automatically if successfully compiled.
 * - force_err_display when true will display all compile errors to the
 *   player.  Error messages are still displayed anyway if the player
 *   is INTERACTIVE but not READMODE (which I believe means they are in the
 *   @program/@edit editor).
 *
 * @param descr the descriptor of the person compiling
 * @param player_in the player compiling
 * @param program_in the program to compile
 * @param force_err_display boolean - true to always show compile errors
 */
void
do_compile(int descr, dbref player_in, dbref program_in, int force_err_display)
{
    const char *token;
    struct INTERMEDIATE *new_word;
    COMPSTATE cstat;

    if (PROGRAM_INSTANCES_IN_PRIMITIVE(program_in) > 0) {
        /*
         * @TODO This force_err_display || interactive and not readmode is
         *       done in a few different places.  Good candidate for a
         *       function that takes 'player_in' anad 'force_err_display'
         *       and notifies (or not) accordingly.
         */
        if (force_err_display ||
            ((FLAGS(player_in) & INTERACTIVE) && !(FLAGS(player_in) & READMODE))) {
            notify_nolisten(player_in, "Error: Cannot compile program from primitive it is running.", 1);
        }

        return;
    }

    /* set all compile state variables */
    cstat.force_err_display = force_err_display;
    cstat.descr = descr;
    cstat.control_stack = 0;
    cstat.procs = 0;
    cstat.alt_entrypoint = 0;
    cstat.nowords = 0;
    cstat.curr_word = cstat.first_word = NULL;
    cstat.curr_proc = NULL;
    cstat.currpubs = NULL;
    cstat.nested_fors = 0;
    cstat.nested_trys = 0;
    cstat.addrcount = 0;
    cstat.addrmax = 0;

    for (int i = 0; i < MAX_VAR; i++) {
        cstat.variables[i] = NULL;
        cstat.localvars[i] = NULL;
        cstat.scopedvars[i] = NULL;
    }

    /*
     * This is where the program "text" comes from.  This is loaded when
     * the program is loaded by db.c
     *
     * grep for PROGRAM_SET_FIRST in db.c if you are curious.  Under the
     * hood it uses read_program
     *
     * @see read_program
     */
    cstat.curr_line = PROGRAM_FIRST(program_in);
    cstat.lineno = 1;
    cstat.start_comment = 0;
    cstat.force_comment = tp_muf_comments_strict ? 1 : 0;
    cstat.next_char = NULL;

    if (cstat.curr_line)
        cstat.next_char = cstat.curr_line->this_line;

    cstat.player = player_in;
    cstat.program = program_in;
    cstat.compile_err = 0;
    cstat.line_copy = NULL;
    cstat.macrosubs = 0;
    cstat.nextinst = NULL;
    cstat.addrlist = NULL;
    cstat.addroffsets = NULL;
    init_defs(&cstat);

    cstat.variables[0] = "ME";
    cstat.variables[1] = "LOC";
    cstat.variables[2] = "TRIGGER";
    cstat.variables[3] = "COMMAND";

    /* free old stuff */
    (void) dequeue_prog(cstat.program, 1);
    free_prog(cstat.program);
    cleanpubs(PROGRAM_PUBS(cstat.program));
    PROGRAM_SET_PUBS(cstat.program, NULL);
    clean_mcpbinds(PROGRAM_MCPBINDS(cstat.program));
    PROGRAM_SET_MCPBINDS(cstat.program, NULL);

    PROGRAM_SET_PROFTIME(cstat.program, 0, 0);
    PROGRAM_SET_PROFSTART(cstat.program, time(NULL));
    PROGRAM_SET_PROF_USES(cstat.program, 0);

    if (!cstat.curr_line)
        v_abort_compile(&cstat, "Missing program text.");

    /* do compilation */
    while ((token = next_token(&cstat))) {
        /* test for errors */
        if (cstat.compile_err) {
            free((void *) token);
            return;
        }

        new_word = next_word(&cstat, token);

        /* test for errors */
        if (cstat.compile_err) {
            free((void *) token);
            return;
        }

        if (new_word) {
            if (!cstat.first_word)
                cstat.first_word = cstat.curr_word = new_word;
            else {
                cstat.curr_word->next = new_word;
                cstat.curr_word = cstat.curr_word->next;
            }
        }

        while (cstat.curr_word && cstat.curr_word->next)
            cstat.curr_word = cstat.curr_word->next;

        free((void *) token);
    }

    if (cstat.curr_proc)
        v_abort_compile(&cstat, "Unexpected end of file.");

    if (!cstat.procs)
        v_abort_compile(&cstat, "Missing procedure definition.");

    if (tp_optimize_muf) {
        int maxpasses = 5;
        int passcount = 0;
        int optimcount = 0;
        int optcnt = 0;

        do {
            optcnt = OptimizeIntermediate(&cstat, force_err_display);
            optimcount += optcnt;
            passcount++;
        } while (optcnt > 0 && --maxpasses > 0);

        if (force_err_display && optimcount > 0) {
            notifyf_nolisten(cstat.player,
                             "Program optimized by %d instructions in %d passes.", optimcount,
                             passcount);
        }
    }

    /* do copying over */
    fix_addresses(&cstat);
    copy_program(&cstat);
    fixpubs(cstat.currpubs, PROGRAM_CODE(cstat.program));
    PROGRAM_SET_PUBS(cstat.program, cstat.currpubs);

    if (cstat.nextinst) {
        struct INTERMEDIATE *ptr;

        while (cstat.nextinst) {
            ptr = cstat.nextinst;
            cstat.nextinst = ptr->next;
            free(ptr);
        }

        cstat.nextinst = NULL;
    }

    if (cstat.compile_err)
        return;

    set_start(&cstat);
    cleanup(&cstat);

    /* Set PROGRAM_INSTANCES to zero (cuz they don't get set elsewhere) */
    PROGRAM_SET_INSTANCES(cstat.program, 0);

    if (force_err_display)
        notify_nolisten(cstat.player, "Program compiled successfully.", 1);

    /* restart AUTOSTART program. */
    if ((FLAGS(cstat.program) & ABODE) && TrueWizard(OWNER(cstat.program))) {
        add_muf_queue_event(-1, OWNER(cstat.program), NOTHING, NOTHING,
                            cstat.program, "Startup", "Queued Event.", 0);
        notify_nolisten(cstat.player, "Program autostarted.", 1);
    }

}

/**
 * Little routine to do the line_copy handling right
 *
 * @private
 * @param cstat the compile state object
 */
static void
advance_line(COMPSTATE * cstat)
{
    cstat->curr_line = cstat->curr_line->next;
    cstat->lineno++;
    cstat->macrosubs = 0;

    free(cstat->line_copy);
    cstat->line_copy = NULL;

    if (cstat->curr_line)
        cstat->next_char = (cstat->line_copy = alloc_string(cstat->curr_line->this_line));
    else
        cstat->next_char = (cstat->line_copy = NULL);
}


/**
 * Produce a string from the current position in cstat
 *
 * The way string compiling works, is when we detect a " we know we're
 * doing a string word.  But the string isn't actually scooped up at that
 * point; this method advances the pointer past that first character and
 * then tries to read it until we get to an end character.
 *
 * It handles all the escaping, and if no end is found, throws a compile
 * error.
 *
 * You are given a copy of the string, and so you should free the returned
 * memory.
 *
 * @private
 * @param cstat the compile state object
 * @return pointer to newly allocated string
 */
static const char *
do_string(COMPSTATE * cstat)
{
    char buf[BUFFER_LEN];
    int i = 0, quoted = 0;

    buf[i] = *cstat->next_char;
    cstat->next_char++;
    i++;

    /* @TODO This doesn't check for buffer overflows on buf ... that may
     *       not be a problem because the source string may be the same
     *       size as 'buf' (which is likely, but I haven't verified it).
     *       Still, relying on these two things being the same size is
     *       probably a really bad idea.
     */
    while ((quoted || *cstat->next_char != ENDSTRING) && *cstat->next_char) {
        if (*cstat->next_char == BEGINESCAPE && !quoted) {
            quoted++;
            cstat->next_char++;
        } else if (*cstat->next_char == 'r' && quoted) {
            buf[i++] = '\r';
            cstat->next_char++;
            quoted = 0;
        } else if (*cstat->next_char == '[' && quoted) {
            buf[i++] = ESCAPE_CHAR;
            cstat->next_char++;
            quoted = 0;
        } else {
            buf[i] = *cstat->next_char;
            i++;
            cstat->next_char++;
            quoted = 0;
        }
    }

    if (!*cstat->next_char) {
        abort_compile(cstat, "Unterminated string found at end of line.");
    }

    cstat->next_char++;
    buf[i] = '\0';
    return alloc_string(buf);
}

/**
 * This is the "old style" comment parser
 *
 * If cstat->force_comment == 0, or if do_new_comment has an error, this
 * code is used by do_comment.  force_comment is set to tp_muf_comments_strict
 *
 * It very simply chugs along until it finds a ) and handles the comment.
 * It doesn't handle recursive comments ( like (this) )
 *
 * @see do_comment
 * @see do_new_comment
 *
 * @private
 * @param cstat our compile state object
 * @return integer - 0 on success, 1 on error
 */
static int
do_old_comment(COMPSTATE * cstat)
{		
    if (!cstat->next_char)		
        return 1;		

     while (*cstat->next_char && *cstat->next_char != ENDCOMMENT)		
        cstat->next_char++;		

     if (!(*cstat->next_char)) {		
        advance_line(cstat);		

         if (!cstat->curr_line) {		
            return 1;		
        }		

         return do_old_comment(cstat);		
    } else {		
        cstat->next_char++;		

         if (!(*cstat->next_char))		
            advance_line(cstat);		
    }		

     return 0;		
}		

/**		
 * The "new" comment parser, supporting recursive comments.
 *
 * I believe this means that you can have comments inside comments,
 * (which is nice because you can (do things) like this).  However,
 * it appears that comments aren't documented (as far as I can tell)
 * in 'man'.
 *
 * @TODO Make a DefDets entry on COMMENTS, or a man entry for (
 *
 * The maximum depth of the recursive comments is arbitrarily 7
 *
 * This uses return codes, which are defined as follows:
 *
 * 0 - success
 * 1 - Unterminated comment
 * 2 - Expected comment (C programmer error only, I think ... calling this
 *     function when the first character is not an open paren)
 * 3 - Comments nested too deep
 *
 * @private
 * @param cstat the compile state struct
 * @param depth the recursion depth -- start with 0
 * @return integer status, as described above
 */
static int
do_new_comment(COMPSTATE * cstat, int depth)
{
    int retval = 0;
    int in_str = 0;
    const char *ptr;

    /* We called this with no comment to process */
    if (!*cstat->next_char || *cstat->next_char != BEGINCOMMENT)
        return 2;

    if (depth >= 7 /* arbitrary */ )
        return 3;

    cstat->next_char++;     /* Advance past BEGINCOMMENT */

    while (*cstat->next_char != ENDCOMMENT) {
        if (!(*cstat->next_char)) {
            do {
                advance_line(cstat);

                if (!cstat->curr_line) {
                    return 1;
                }
            } while (!(*cstat->next_char));
        } else if (*cstat->next_char == BEGINCOMMENT) {
            retval = do_new_comment(cstat, depth + 1);

            if (retval) {
                return retval;
            }
        } else {
            cstat->next_char++;
        }
    };

    cstat->next_char++;     /* Advance past ENDCOMMENT */
    ptr = cstat->next_char;

    while (*ptr) {
        if (in_str) {
            if (*ptr == ENDSTRING) {
                in_str = 0;
            }
        } else {
            if (*ptr == BEGINSTRING) {
                in_str = 1;
            } else if (*ptr == ENDSTRING) {
                in_str = 1;
                break;
            }
        }

        ptr++;
    }

    if (in_str) {
        notifyf(cstat->player,
                "Warning on line %i: Unterminated string may indicate unterminated comment. Comment starts on line %i.",
                cstat->lineno, cstat->start_comment);
    }

    if (!(*cstat->next_char))
        advance_line(cstat);

    if (depth && !cstat->curr_line) /* EOF? Don't care if done (depth==0) */
        return 1;

    return 0;
}

/**		
  * This implements the skipping of comments in the code by the compiler.
  *
  * 'Depth' is a funny thing here.  It isn't really used the same as depth
  * in, say, do_new_comment where it is the recursion depth.  What it really
  * means is, All we are doing is controlling if we use the recursive comment
  * code or not.  It is, in fact, really kind of superfluous because
  * it is set according to the binary value of cstat->force_comment which
  * is also used in this call.
  *		
  * @TODO Remove 'depth' from the calling signature and use
  *       !cstat->force_comment in the two relevant if statements instead.
  *		
  * Basically, force_comment and depth should either both be true or both
  * be false, otherwise this code doesn't work "properly".
  *		
  * The "do_new_comment" code is used for parsing recursive comments, otherwise
  * the "do_old_comment" code is used.
  *		
  * @see do_new_comment
  * @see do_old_comment
  *		
  * @private
  * @param cstat compile state structure
  * @param depth boolean that should be the same as cstat->force_comment
  */		
static void
do_comment(COMPSTATE * cstat, int depth)
{
    unsigned int next_char = 0; /* Save state if needed. */
    int lineno = 0;
    struct line *curr_line = NULL;
    int macrosubs = 0;
    int retval = 0;

    /*
     * @TODO depth and cstat->force_comment are always the same.  Put the
     *       contents of this 'if' into the next 'if' because the extra
     *       check is completely unnecessary.
     */
    if (!depth && !cstat->force_comment) {
        next_char = cstat->line_copy ? cstat->next_char - cstat->line_copy : 0;
        macrosubs = cstat->macrosubs;
        lineno = cstat->lineno;
        curr_line = cstat->curr_line;
    }

    if (!depth) {
        if ((retval = do_new_comment(cstat, 0))) {
            /*
             * @TODO Remove this if.  If we got to this part of the code,
             *       cstat->force_comment is *always* 0
             *
             *       I don't understand why this falls back to the old
             *       parser if force_comment = 0, it would seem to make
             *       more sense to error here.
             *		
             *       But frankly this whole function is written strange
             *       so I don't really understand the logic behind any
             *       of this structure.
             *		
             *       Its clear to me that "depth" was supposed to mean
             *       something else but it wound up de-facto being a
             *       boolean clone of cstat->force_comment.  Because
             *       there is no original documentation, the original
             *       thought process is lost or at least limited to the
             *       original programmer.
             */		
            if (cstat->force_comment) {
                switch (retval) {
                    case 1:
                        v_abort_compile(cstat, "Unterminated comment.");
                    case 2:
                        v_abort_compile(cstat, "Expected comment.");
                    case 3:
                        v_abort_compile(cstat, "Comments nested too deep (more than 7 levels).");
                }

                 return;
            } else {
                /* Set back up, drop through for retry. */
                if (cstat->line_copy) {
                    free((void *) cstat->line_copy);
                    cstat->line_copy = NULL;
                }		

                cstat->curr_line = curr_line;
                cstat->macrosubs = macrosubs;
                cstat->lineno = lineno;

                if (cstat->curr_line) {
                    cstat->next_char = (cstat->line_copy =
                                       alloc_string(cstat->curr_line->this_line))
                                       + next_char;
                } else {
                    cstat->next_char = (cstat->line_copy = NULL);
                }
            }
        } else {
            /* Comment hunt worked, new-style. */
            return;
        }
    }

     if (do_old_comment(cstat)) {
        v_abort_compile(cstat, "Unterminated comment.");
    }
}

/**
 * Checks to see if a given string is a pre-processor conditional.
 *
 * Such as $ifdef or similar $-conditionals
 *
 * @private
 * @param tmpptr the string to check
 * @return boolean true if is a preprocessor conditional, false otherwise
 */
static int
is_preprocessor_conditional(const char *tmpptr)
{
    if (*tmpptr != BEGINDIRECTIVE)
        return 0;

    if (!strcasecmp(tmpptr + 1, "ifdef"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifndef"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "iflib"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifnlib"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifver"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "iflibver"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifnver"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifnlibver"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifcancall"))
        return 1;
    else if (!strcasecmp(tmpptr + 1, "ifncancall"))
        return 1;

    return 0;
}

/**
 * Skips comments, grabs strings, returns NULL when no more tokens to grab.
 *
 * This is perhaps the heart of the compile process. It finds the next
 * token and returns it as a string.
 *
 * A few notes: This is recursive, and theoretically you could kill the
 * stack with a program that has too many blank lines in it (one thing that
 * causes it to recurse) or too many sequential comment blocks (another thing
 * that causes recursion).  You would have to be really malicious to do this
 * though, it wouldn't happen by accident.
 *
 * The string returned is a copy of the memory, so you should make sure to
 * free it.
 *
 * @param cstat the compile state structure
 * @return string next token, or NULL if no more tokens to get.
 */
static const char *
next_token_raw(COMPSTATE * cstat)
{
    static char buf[BUFFER_LEN];
    int i;

    if (!cstat->curr_line)
        return (char *) 0;

    if (!cstat->next_char)
        return (char *) 0;

    skip_whitespace(&cstat->next_char);

    if (!(*cstat->next_char)) {
        advance_line(cstat);
        return next_token_raw(cstat);
    }

    /* take care of comments */
    if (*cstat->next_char == BEGINCOMMENT) {
        cstat->start_comment = cstat->lineno;

        if (cstat->force_comment == 1) {
            do_comment(cstat, -1);
        } else {
            do_comment(cstat, 0);
        }

        cstat->start_comment = 0;

        return next_token_raw(cstat);
    }

    if (*cstat->next_char == BEGINSTRING)
        return do_string(cstat);

    for (i = 0; *cstat->next_char && !isspace(*cstat->next_char); i++) {
        buf[i] = *cstat->next_char;
        cstat->next_char++;
    }

    buf[i] = '\0';

    return alloc_string(buf);
}

/* handle compiler directives */

/**
 * Replace __PROG__ with the current program's dbref in next_char
 *
 * Basically a "special wrapper" around next_char
 *
 * Do not free the memory from this call, as it uses a static buffer.
 * That means it is also not threadsafe.  Copy the buffer if you need it.
 *
 * @private
 * @param cstat the compile state object.
 * @return the string with __PROG__ replaced as described above.
 */
static char *
next_char_special(COMPSTATE *cstat)
{
    static char _buf[BUFFER_LEN];
    char progstr[BUFFER_LEN];
    snprintf(progstr, sizeof(progstr), "#%d", cstat->program);
    return string_substitute(cstat->next_char, "__PROG__", progstr, _buf, sizeof(_buf));
}

/**
 * This handles the processing of compiler directives ($tokens)
 *
 * Given a token 'direct', this processes the directive and takes
 * the appropriate action.
 *
 * Understood directives are:
 *
 * $define - Accepts tokens up until $enddef as a definition
 * $cleardefs - clear defines EXCEPT for internal ones
 *              @see include_internal_defs
 * $enddef - If found independently of a $define, this is an error
 * $def - a single line definition
 * $pubdef - this makes a "public define" which winds up being a prop under
 *           _defs/ for the definition on the given program object.
 * $libdef - this is a shortcut for making a public function callable
 *           outside the library - you still need to make it public, but
 *           this makes it so you don't have to set the _def props like
 *           you had to in the olden days.
 * $include - include the public defines of another object - @see include_defs
 * $undef - Remove a definition
 * $echo - Echo a string when doing a compile.
 * $abort - Abort the compilation with a message
 * $version - set the _version property to the given float
 * $lib-version - sets the _lib-version property to the given float
 * $author - sets the _author property to the given string
 * $doccmd - Sets the _docs property to the contents of the rest of the line
 * $note - sets the _note property to the contents of the rest of the line
 * $ifdef - A limited 'if' conditional that supports certain comparison
 *          operators: '=', '<', and '>'.  These operators must be directly
 *          touching (no space) the define.  You can leave off the operator
 *          just to check if something was defined.
 *
 *          Supports $else and is ended with $endif
 * $ifndef - The 'NOT' version of $ifdef
 * $ifcancall - given a library (dbref or $ref) and a public function name,
 *              run the block of code if we can call it.  Supports $else
 *              and uses $endif as a terminal.
 * $ifncancall - the NOT version of $ifcancall
 * $ifver - Checks the version of either 'this' or a library (dbref or $ref)
 *          If it is greater than or equal the provided float, then the
 *          block is compiled.  $else is supported, and $endif is the terminal
 * $ifnver - the NOT version of $ifver
 * $iflibver - the libver version of $ifver
 * $ifnlibver - the NOT version of $iflibver
 * $iflib - checks to see if a library exists and is a program (dbref or $ref)
 *          $else is supported, and $endif is the terminal
 * $ifnlib - the NOT version of $iflib
 * $else - if found by itself, this is an error
 * $endif - if found by itself, this is an error
 * $pragma - Sets compiler options
 * $entrypoint - sets the function name to run as the entrypoint to the program
 *               We default to the last function.
 * $language - Set the compile language.  Only the string "MUF" works right now
 *
 * @TODO This function is a monster, and there is a lot of redundancy with
 *       the different varities of $if.  I would recommend having an
 *       array of structures that map strings to callbacks, and loop over
 *       that array, matching 'direct' to each string until you find it.
 *       Then run the callback.
 *
 *       The different 'if' varities can use the same callback, and maybe
 *       use another callback for comparison code as that is the primary
 *       variance (I think)
 *
 * @private
 * @param cstat the compile state struct
 * @param direct the directive we're trying to process.  Must start with $
 */
static void
do_directive(COMPSTATE * cstat, char *direct)
{
    char temp[BUFFER_LEN];
    char *tmpname = NULL;
    char *tmpptr = NULL;
    int i = 0;
    int j;

    /*
     * Due to how this is written, if a direct is passed that doesn't start
     * with a $, this will segfault.  That shouldn't be a problem since the
     * existance of $ determines if this function is called, but I feel
     * like I should point that out.
     */
    strcpyn(temp, sizeof(temp), ++direct);

    if (!(temp[0])) {
        v_abort_compile(cstat, "I don't understand that compiler directive!");
    }

    /*
     * These directives are really poorly commented, but I'm recommending
     * we gut this function so I'm not going to put a big effort into
     * documenting this.  Whomever takes on the @TODO at the head of this
     * function, please add some decent docs.
     */
    if (!strcasecmp(temp, "define")) {
        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file looking for $define name.");

        i = 0;

        while ((tmpptr = (char *) next_token_raw(cstat)) &&
               (strcasecmp(tmpptr, "$enddef"))) {
            for (char *cp = tmpptr; i < (BUFFER_LEN / 2) && *cp;) {
                if (*tmpptr == BEGINSTRING && cp != tmpptr &&
                    (*cp == ENDSTRING || *cp == BEGINESCAPE)) {
                    temp[i++] = BEGINESCAPE;
                }

                temp[i++] = *cp++;
            }

            if (*tmpptr == BEGINSTRING)
                temp[i++] = ENDSTRING;

            temp[i++] = ' ';
            free(tmpptr);

            if (i > (BUFFER_LEN / 2))
                v_abort_compile(cstat, "$define definition too long.");
        }

        if (i)
            i--;

        temp[i] = '\0';

        if (!tmpptr) {
            free(tmpname);
            v_abort_compile(cstat, "Unexpected end of file in $define definition.");
        }

        free(tmpptr);
        (void) insert_def(cstat, tmpname, temp);
        free(tmpname);
    } else if (!strcasecmp(temp, "cleardefs")) {
        char nextToken[BUFFER_LEN];

        kill_hash(cstat->defhash, DEFHASHSIZE, 1); /* Get rid of all defs first. */
        include_internal_defs(cstat);   /* Always include internal defs. */
        skip_whitespace(&cstat->next_char);
        strcpyn(nextToken, sizeof(nextToken), cstat->next_char);
        tmpname = nextToken;
        skip_whitespace(&cstat->next_char);
        advance_line(cstat);

        if (!tmpname || !*tmpname || MLevel(OWNER(cstat->program)) < 4) {
            include_defs(cstat, OWNER(cstat->program));
            include_defs(cstat, (dbref) 0);
        }
    } else if (!strcasecmp(temp, "enddef")) {
        v_abort_compile(cstat, "$enddef without a previous matching $define.");
    } else if (!strcasecmp(temp, "def")) {
        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file looking for $def name.");

        skip_whitespace(&cstat->next_char);

        (void) insert_def(cstat, tmpname, cstat->next_char);

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);
        free(tmpname);
    } else if (!strcasecmp(temp, "pubdef")) {
        char *holder = NULL;

        tmpname = (char *) next_token_raw(cstat);
        holder = tmpname;

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file looking for $pubdef name.");

        if (strcasecmp(tmpname, (char[]){PROP_DELIMITER,0}) &&
            (strchr(tmpname, PROPDIR_DELIMITER) ||
             strchr(tmpname, PROP_DELIMITER) ||
             Prop_SeeOnly(tmpname) || Prop_Hidden(tmpname) || Prop_System(tmpname))) {
            char buf[BUFFER_LEN];
            free(tmpname);
            snprintf(buf, sizeof(buf), "Invalid $pubdef name.  No %c, %c, %c nor %c are allowed.",
                     PROPDIR_DELIMITER, PROP_DELIMITER, PROP_HIDDEN, PROP_SEEONLY);
            v_abort_compile(cstat, buf);
        } else {
            if (!strcasecmp(tmpname, ":")) {
                remove_property(cstat->program, DEFINES_PROPDIR, 0);
            } else {
                char defstr[BUFFER_LEN];
                char propname[BUFFER_LEN];
                int doitset = 1;

                skip_whitespace(&cstat->next_char);
                strcpyn(defstr, BUFFER_LEN, next_char_special(cstat));

                if (*tmpname == BEGINESCAPE) {
                    char *temppropstr = NULL;

                   (void) *tmpname++;
                    snprintf(propname, sizeof(propname), "%s/%s", DEFINES_PROPDIR, tmpname);
                    temppropstr = (char *) get_property_class(cstat->program, propname);

                    if (temppropstr) {
                        doitset = 0;
                    }
                } else {
                    snprintf(propname, sizeof(propname), "/%s/%s", DEFINES_PROPDIR, tmpname);
                }

                if (doitset) {
                    if (*defstr) {
                        add_property(cstat->program, propname, defstr, 0);
                    } else {
                        remove_property(cstat->program, propname, 0);
                    }
                }
            }
        }

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);
        free(holder);
    } else if (!strcasecmp(temp, "libdef")) {
        char *holder = NULL;

        tmpname = (char *) next_token_raw(cstat);
        holder = tmpname;

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file looking for $libdef name.");

        if (strchr(tmpname, PROPDIR_DELIMITER) ||
            strchr(tmpname, PROP_DELIMITER) ||
            Prop_SeeOnly(tmpname) || Prop_Hidden(tmpname) || Prop_System(tmpname)) {
            char buf[BUFFER_LEN];
            free(tmpname);
            snprintf(buf, sizeof(buf), "Invalid $libdef name.  No %c, %c, %c nor %c are allowed.",
                     PROPDIR_DELIMITER, PROP_DELIMITER, PROP_HIDDEN, PROP_SEEONLY);
            v_abort_compile(cstat, buf);
        } else {
            char propname[BUFFER_LEN];
            char defstr[BUFFER_LEN];
            int doitset = 1;

            skip_whitespace(&cstat->next_char);

            if (*tmpname == BEGINESCAPE) {
                char *temppropstr = NULL;

                (void) *tmpname++;
                snprintf(propname, sizeof(propname), "/%s/%s", DEFINES_PROPDIR,tmpname);
                temppropstr = (char *) get_property_class(cstat->program, propname);

                if (temppropstr) {
                    doitset = 0;
                }
            } else {
                snprintf(propname, sizeof(propname), "/%s/%s", DEFINES_PROPDIR, tmpname);
            }

            snprintf(defstr, sizeof(defstr), "#%i \"%s\" call", cstat->program, tmpname);

            if (doitset) {
                if (*defstr) {
                    add_property(cstat->program, propname, defstr, 0);
                } else {
                    remove_property(cstat->program, propname, 0);
                }
            }
        }

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);

        free(holder);
    } else if (!strcasecmp(temp, "include")) {
        struct match_data md;

        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file while doing $include.");

        {
            char tempa[BUFFER_LEN], tempb[BUFFER_LEN];

            strcpyn(tempa, sizeof(tempa), match_args);
            strcpyn(tempb, sizeof(tempb), match_cmdname);
            init_match(cstat->descr, cstat->player, tmpname, NOTYPE, &md);
            match_registered(&md);
            match_absolute(&md);
            match_me(&md);
            i = (int) match_result(&md);
            strcpyn(match_args, sizeof(match_args), tempa);
            strcpyn(match_cmdname, sizeof(match_cmdname), tempb);
        }

        free(tmpname);

        if (!OkObj(i))
            v_abort_compile(cstat, "I don't understand what object you want to $include.");

        include_defs(cstat, (dbref) i);
    } else if (!strcasecmp(temp, "undef")) {
        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file looking for name to $undef.");

        kill_def(cstat, tmpname);
        free(tmpname);
    } else if (!strcasecmp(temp, "echo")) {
        notify_nolisten(cstat->player, cstat->next_char, 1);

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);
    } else if (!strcasecmp(temp, "abort")) {
        skip_whitespace(&cstat->next_char);
        tmpname = (char *) cstat->next_char;

        if (tmpname && *tmpname) {
            v_abort_compile(cstat, tmpname);
        } else {
            v_abort_compile(cstat, "Forced abort for the compile.");
        }
    } else if (!strcasecmp(temp, "version")) {
        tmpname = (char *) next_token_raw(cstat);

        if (!ifloat(tmpname)) {
            free(tmpname);
            v_abort_compile(cstat, "Expected a floating point number for the version.");
        }

        add_property(cstat->program, MUF_VERSION_PROP, tmpname, 0);

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);
        free(tmpname);
    } else if (!strcasecmp(temp, "lib-version")) {
        tmpname = (char *) next_token_raw(cstat);

        if (!ifloat(tmpname)) {
            free(tmpname);
            v_abort_compile(cstat, "Expected a floating point number for the version.");
        }

        add_property(cstat->program, MUF_LIB_VERSION_PROP, tmpname, 0);

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);
        free(tmpname);
    } else if (!strcasecmp(temp, "author")) {
        skip_whitespace(&cstat->next_char);
        tmpname = (char *) cstat->next_char;
        skip_whitespace(&cstat->next_char);
        add_property(cstat->program, MUF_AUTHOR_PROP, tmpname, 0);
        advance_line(cstat);
    } else if (!strcasecmp(temp, "doccmd")) {
        skip_whitespace(&cstat->next_char);
        tmpname = next_char_special(cstat);
        skip_whitespace(&cstat->next_char);
        add_property(cstat->program, MUF_DOCCMD_PROP, tmpname, 0);
        advance_line(cstat);
    } else if (!strcasecmp(temp, "note")) {
        skip_whitespace(&cstat->next_char);
        tmpname = (char *) cstat->next_char;
        skip_whitespace(&cstat->next_char);
        add_property(cstat->program, MUF_NOTE_PROP, tmpname, 0);
        advance_line(cstat);
    } else if (!strcasecmp(temp, "ifdef") || !strcasecmp(temp, "ifndef")) {
        int invert_flag = !strcasecmp(temp, "ifndef");
        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname) {
            v_abort_compile(cstat, "Unexpected end of file looking for $ifdef condition.");
        }

        if (*tmpname == BEGINSTRING) {
            strcpyn(temp, sizeof(temp), tmpname + 1);
        } else {
            strcpyn(temp, sizeof(temp), tmpname);
        }

        free(tmpname);

        for (i = 1; temp[i] && (temp[i] != '=') && (temp[i] != '>') && (temp[i] != '<'); i++) ;

        tmpname = &(temp[i]);

        i = (temp[i] == '>') ? 1 : ((temp[i] == '=') ? 0 : ((temp[i] == '<') ? -1 : -2));
        *tmpname = '\0';
        tmpname++;
        tmpptr = (char *) expand_def(cstat, temp);

        if (i == -2) {
            j = (!tmpptr);

            free(tmpptr);
        } else {
            if (!tmpptr) {
                j = 1;
            } else {
                j = strcasecmp(tmpptr, tmpname);
                j = !((!i && !j) || ((i * j) > 0));
                free(tmpptr);
            }
        }

        if (invert_flag) {
            j = !j;
        }

        if (j) {
            i = 0;

            while ((tmpptr = (char *) next_token_raw(cstat)) &&
                   (i || ((strcasecmp(tmpptr, "$else"))
                  && (strcasecmp(tmpptr, "$endif"))))) {
                if (is_preprocessor_conditional(tmpptr))
                    i++;
                else if (!strcasecmp(tmpptr, "$endif"))
                    i--;

                free(tmpptr);
            }

            if (!tmpptr) {
                v_abort_compile(cstat, "Unexpected end of file in $ifdef clause.");
            }

            free(tmpptr);
        }
    } else if (!strcasecmp(temp, "ifcancall") || !strcasecmp(temp, "ifncancall")) {
        struct match_data md;

        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file for ifcancall.");

        char tempa[BUFFER_LEN], tempb[BUFFER_LEN];

        strcpyn(tempa, sizeof(tempa), match_args);
        strcpyn(tempb, sizeof(tempb), match_cmdname);
        init_match(cstat->descr, cstat->player, tmpname, NOTYPE, &md);
        match_registered(&md);
        match_absolute(&md);
        i = (int) match_result(&md);
        strcpyn(match_args, sizeof(match_args), tempa);
        strcpyn(match_cmdname, sizeof(match_cmdname), tempb);
        free(tmpname);

        if (!OkObj(i))
            v_abort_compile(cstat,
                            "I don't understand what object you want to check in ifcancall.");

        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname || !*tmpname) {
            free(tmpptr);

            v_abort_compile(cstat, "I don't understand what function you want to check for.");
        }

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);

        j = 0;

        if (Typeof(i) == TYPE_PROGRAM) {
            if (!PROGRAM_CODE(i)) {
                struct line *tmpline;

                tmpline = PROGRAM_FIRST(i);
                PROGRAM_SET_FIRST(i, ((struct line *) read_program(i)));
                do_compile(cstat->descr, OWNER(i), i, 0);
                free_prog_text(PROGRAM_FIRST(i));
                PROGRAM_SET_FIRST(i, tmpline);
            }

            if (MLevel(OWNER(i)) > 0 &&
                (MLevel(OWNER(cstat->program)) >= 4 || OWNER(i) == OWNER(cstat->program)
                || Linkable(i))
            ) {
                struct publics *pbs;

                pbs = PROGRAM_PUBS(i);

                while (pbs) {
                    if (!strcasecmp(tmpname, pbs->subname))
                        break;

                    pbs = pbs->next;
                }

                if (pbs && MLevel(OWNER(cstat->program)) >= pbs->mlev)
                    j = 1;
            }
        }

        free(tmpname);

        if (!strcasecmp(temp, "ifncancall")) {
            j = !j;
        }

        if (!j) {
            i = 0;

            while ((tmpptr = (char *) next_token_raw(cstat)) &&
                   (i || ((strcasecmp(tmpptr, "$else"))
                      && (strcasecmp(tmpptr, "$endif"))))) {
                if (is_preprocessor_conditional(tmpptr))
                    i++;
                else if (!strcasecmp(tmpptr, "$endif"))
                    i--;

                free(tmpptr);
            }

            if (!tmpptr) {
                v_abort_compile(cstat, "Unexpected end of file in $ifcancall clause.");
            }

            free(tmpptr);
        }
    } else if (!strcasecmp(temp, "ifver") || !strcasecmp(temp, "iflibver") ||
               !strcasecmp(temp, "ifnver") || !strcasecmp(temp, "ifnlibver")) {
        struct match_data md;
        double verflt = 0;
        double checkflt = 0;
        int needFree = 0;

        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file while doing $ifver.");

        if (strcasecmp(tmpname, "this")) {
            char tempa[BUFFER_LEN], tempb[BUFFER_LEN];

            strcpyn(tempa, sizeof(tempa), match_args);
            strcpyn(tempb, sizeof(tempb), match_cmdname);
            init_match(cstat->descr, cstat->player, tmpname, NOTYPE, &md);
            match_registered(&md);
            match_absolute(&md);
            match_me(&md);
            i = (int) match_result(&md);
            strcpyn(match_args, sizeof(match_args), tempa);
            strcpyn(match_cmdname, sizeof(match_cmdname), tempb);
        } else {
            i = cstat->program;
        }

        free(tmpname);

        if (!OkObj(i))
            v_abort_compile(cstat,
                            "I don't understand what object you want to check with $ifver.");

        if (!strcasecmp(temp, "ifver") || !strcasecmp(temp, "ifnver")) {
            tmpptr = (char *) get_property_class(i, MUF_VERSION_PROP);
        } else {
            tmpptr = (char *) get_property_class(i, MUF_LIB_VERSION_PROP);
        }

        if (!tmpptr || !*tmpptr) {
            tmpptr = malloc(4 * sizeof(char));
            strcpyn(tmpptr, 4 * sizeof(char), "0.0");
            needFree = 1;
        }

        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname || !*tmpname) {
            free(tmpptr);
            free(tmpname);
            v_abort_compile(cstat,
                            "I don't understand what version you want to compare to with $ifver.");
        }

        if (!tmpptr || !ifloat(tmpptr)) {
            verflt = 0.0;
        } else {
            sscanf(tmpptr, "%lg", &verflt);
        }

        if (needFree)
            free(tmpptr);

        if (!tmpname || !ifloat(tmpname)) {
            checkflt = 0.0;
        } else {
            sscanf(tmpname, "%lg", &checkflt);
        }

        free(tmpname);

        while (*cstat->next_char)
            cstat->next_char++;

        advance_line(cstat);
        j = checkflt <= verflt;

        if (!strcasecmp(temp, "ifnver") || !strcasecmp(temp, "ifnlibver")) {
            j = !j;
        }

        if (!j) {
            i = 0;

            while ((tmpptr = (char *) next_token_raw(cstat)) &&
                   (i || ((strcasecmp(tmpptr, "$else"))
                      && (strcasecmp(tmpptr, "$endif"))))) {
                if (is_preprocessor_conditional(tmpptr))
                    i++;
                else if (!strcasecmp(tmpptr, "$endif"))
                    i--;

                free(tmpptr);
            }

            if (!tmpptr) {
                v_abort_compile(cstat, "Unexpected end of file in $ifver clause.");
            }

            free(tmpptr);
        }
    } else if (!strcasecmp(temp, "iflib") || !strcasecmp(temp, "ifnlib")) {
        struct match_data md;
        char tempa[BUFFER_LEN], tempb[BUFFER_LEN];

        tmpname = (char *) next_token_raw(cstat);

        if (!tmpname)
            v_abort_compile(cstat, "Unexpected end of file in $iflib/$ifnlib clause.");

        strcpyn(tempa, sizeof(tempa), match_args);
        strcpyn(tempb, sizeof(tempb), match_cmdname);
        init_match(cstat->descr, cstat->player, tmpname, NOTYPE, &md);
        match_registered(&md);
        match_absolute(&md);
        i = (int) match_result(&md);
        strcpyn(match_args, sizeof(match_args), tempa);
        strcpyn(match_cmdname, sizeof(match_cmdname), tempb);

        free(tmpname);

        if (OkObj(i) && Typeof(i) == TYPE_PROGRAM) {
            j = 1;
        } else {
            j = 0;
        }

        if (!strcasecmp(temp, "ifnlib")) {
            j = !j;
        }

        if (!j) {
            i = 0;

            while ((tmpptr = (char *) next_token_raw(cstat)) &&
                   (i || ((strcasecmp(tmpptr, "$else"))
                      && (strcasecmp(tmpptr, "$endif"))))) {
                if (is_preprocessor_conditional(tmpptr))
                    i++;
                else if (!strcasecmp(tmpptr, "$endif"))
                    i--;

                free(tmpptr);
            }

            if (!tmpptr) {
                v_abort_compile(cstat, "Unexpected end of file in $iflib clause.");
            }

            free(tmpptr);
        }
    } else if (!strcasecmp(temp, "else")) {
        i = 0;

        while ((tmpptr = (char *) next_token_raw(cstat)) &&
               (i || (strcasecmp(tmpptr, "$endif")))) {
            if (is_preprocessor_conditional(tmpptr))
                i++;
            else if (!strcasecmp(tmpptr, "$endif"))
                i--;

            free(tmpptr);
        }

        if (!tmpptr) {
            v_abort_compile(cstat, "Unexpected end of file in $else clause.");
        }

        free(tmpptr);
    } else if (!strcasecmp(temp, "endif")) {
    } else if (!strcasecmp(temp, "pragma")) {
        skip_whitespace(&cstat->next_char);

        /**
         * @TODO Re-implement $pragmas on program_specific struct.
         **/

        if (!*cstat->next_char || !(tmpptr = (char *) next_token_raw(cstat)))
            v_abort_compile(cstat, "Pragma requires at least one argument.");

        if (!strcasecmp(tmpptr, "comment_strict")) {
            /* Do non-recursive comments (old style) */
            cstat->force_comment = 1;
        } else if (!strcasecmp(tmpptr, "comment_recurse")) {
            /* Do recursive comments ((new) style) */
            cstat->force_comment = 2;
        } else if (!strcasecmp(tmpptr, "comment_loose")) {
            /* Try to compile with recursive and non-recursive comments
               doing recursive first, then strict on a comment-based
               compile error.  Only throw an error if both fail.  This is
               the default mode. */
            cstat->force_comment = 0;
        } else {
            /* If the pragma is not recognized, it is ignored, with a warning. */
            notifyf(cstat->player,
                    "Warning on line %i: Pragma %.64s unrecognized.  Ignoring.",
                    cstat->lineno, tmpptr);
            while (*cstat->next_char)
                cstat->next_char++;
        }

        free(tmpptr);

        if (*cstat->next_char) {
            notifyf(cstat->player,
                    "Warning on line %i: Ignoring extra pragma arguments: %.256s",
                    cstat->lineno, cstat->next_char);
            advance_line(cstat);
        }
    } else if (!strcasecmp(temp, "entrypoint")) {
        struct PROC_LIST *p;
        char buf[BUFFER_LEN];
        skip_whitespace(&cstat->next_char);

        if (!*cstat->next_char || !(tmpptr = (char *) next_token_raw(cstat)))
            v_abort_compile(cstat, "$entrypoint - function name is required.");

        for (p = cstat->procs; p; p = p->next)
            if (!strcasecmp(p->name, tmpptr))
                break;

        if (!p) {
            snprintf(buf, sizeof(buf), "$entrypoint - unrecognized function name '%s'.", tmpptr);
            v_abort_compile(cstat, buf);
        }

        cstat->alt_entrypoint = p;
    } else if (!strcasecmp(temp, "language")) {
        char buf[BUFFER_LEN];
        skip_whitespace(&cstat->next_char);

        if (!*cstat->next_char || !(tmpptr = (char *) next_token_raw(cstat)))
            v_abort_compile(cstat, "$language - argument is required.");

        if (!string(tmpptr)) {
            v_abort_compile(cstat, "$language - argument must be enclosed in double quotes.");
        }

        if (strcasecmp(tmpptr+1, "muf")) {
            snprintf(buf, sizeof(buf), "$language - '%s' is not implemented on this server.", tmpptr+1);
            v_abort_compile(cstat, buf);
        }
    } else {
        v_abort_compile(cstat, "Unrecognized compiler directive.");
    }
}

/**
 * Finds the next token, but does some extras
 *
 * Specifically, it checks for BEGINDIRECTIVE and BEGINESCAPE to handle
 * directives and escapes.  @see do_directive
 *
 * It also does macro expansions.  @see expand_def
 *
 * Under the hood, this uses next_token_raw.  @see next_token_raw
 *
 * This is more of a 'glue' function than something with functional logic
 * in it.
 *
 * @private
 * @param cstat the compile state
 * @return the next token to compile
 */
static const char *
next_token(COMPSTATE * cstat)
{
    char *expansion, *temp;
    size_t elen = 0;

    temp = (char *) next_token_raw(cstat);

    if (!temp)
        return NULL;

    if (temp[0] == BEGINDIRECTIVE) {
        do_directive(cstat, temp);

        free(temp);

        if (cstat->compile_err)
            return NULL;
        else
            return next_token(cstat);
    }

    if (temp[0] == BEGINESCAPE) {
        if (temp[1]) {
            expansion = temp;
            elen = strlen(expansion);
            temp = malloc(elen);
            strcpyn(temp, elen, (expansion + 1));
            free(expansion);
        }

        return (temp);
    }

    if ((expansion = expand_def(cstat, temp))) {
        free(temp);

        if (++cstat->macrosubs > SUBSTITUTIONS) {
            free(expansion);
            abort_compile(cstat, "Too many macro substitutions.");
        } else {
            size_t templen = strlen(cstat->next_char) + strlen(expansion) + 21;
            temp = malloc(templen);
            strcpyn(temp, templen, expansion);
            strcatn(temp, templen, cstat->next_char);
            free(expansion);
            free(cstat->line_copy);
            cstat->next_char = cstat->line_copy = temp;
            return next_token(cstat);
        }
    } else {
        return (temp);
    }
}

/**
 * Adds variable (var).  Return 0 if no space left.
 *
 * @private
 * @param cstat the compile state
 * @param varname the name of the variable to add
 * @return variable number or 0 if no space left
 */
static int
add_variable(COMPSTATE * cstat, const char *varname)
{
    int i;

    for (i = RES_VAR; i < MAX_VAR; i++)
        if (!cstat->variables[i])
            break;

    if (i == MAX_VAR)
        return 0;

    cstat->variables[i] = alloc_string(varname);
    return i;
}


/**
 * Adds scoped variable (function vars).  Return -1 if no space left.
 *
 * @private
 * @param cstat the compile state
 * @param varname the name of the variable to add
 * @return variable number or -1 if no space left
 */
static int
add_scopedvar(COMPSTATE * cstat, const char *varname)
{
    int i;

    for (i = 0; i < MAX_VAR; i++)
        if (!cstat->scopedvars[i])
            break;

    if (i == MAX_VAR)
        return -1;

    cstat->scopedvars[i] = alloc_string(varname);
    return i;
}


/**
 * Adds local variable (lvar).  Return -1 if no space left.
 *
 * @private
 * @param cstat the compile state
 * @param varname the name of the variable to add
 * @return variable number or -1 if no space left
 */
static int
add_localvar(COMPSTATE * cstat, const char *varname)
{
    int i;

    for (i = 0; i < MAX_VAR; i++)
        if (!cstat->localvars[i])
            break;

    if (i == MAX_VAR)
        return -1;

    cstat->localvars[i] = alloc_string(varname);
    return i;
}

/**
 * Pops first while off the innermost control structure, if it is a loop.
 *
 * This has to do with linking 'while' to its proper exit point.
 * How this works, exactly, is pretty clear as mud to this commenter. (tanabi)
 * Note that in the CONTROL_STACK structure, I noted I didn't know what
 * 'extra' is for.  I still don't.
 *
 * @TODO Document the way loop nesting works better per my note above
 *       and the notes on struct CONTROL_STACK.
 *
 * @private
 * @param cstat the compile state struct
 * @return the intermediate associated with the parent's type
 */
static struct INTERMEDIATE *
pop_loop_exit(COMPSTATE * cstat)
{
    struct INTERMEDIATE *temp;
    struct CONTROL_STACK *tofree;
    struct CONTROL_STACK *parent;

    parent = cstat->control_stack;

    if (!parent)
        return 0;

    if (parent->type != CTYPE_BEGIN && parent->type != CTYPE_FOR)
        return 0;

    if (!parent->extra)
        return 0;

    if (parent->extra->type != CTYPE_WHILE)
        return 0;

    temp = parent->extra->place;
    tofree = parent->extra;
    parent->extra = parent->extra->extra;
    free(tofree);
    return temp;
}

/**
 * Function of slightly unknown purpose related to nested loops
 *
 * The purpose of this seems to be to resolve the 'target' locations of
 * loops when they exit.  Truth be told, I'm uncertain of how this works.
 * The original documentation around this was minimal.  (tanabi)
 *
 * @TODO I invite someone else to take a whack at understanding this!
 *
 * @private
 * @param cstat the control state structure
 * @param where the address of the instruction to return to (probably)
 */
static void
resolve_loop_addrs(COMPSTATE * cstat, int where)
{
    struct INTERMEDIATE *eef;

    while ((eef = pop_loop_exit(cstat)))
        eef->in.data.number = where;

    if (!cstat->control_stack || cstat->control_stack->type != CTYPE_FOR)
        return;

    if ((eef = cstat->control_stack->place)) {
        eef->next->in.data.number = where;
    }
}

/* support routines for internal data structures. */

/**
 * Add procedure to procedures list
 *
 * proc_name is copied, so you may do what you wish with that memory after
 * calling this.
 *
 * @private
 * @param cstat the compile state structure
 * @param proc_name the procedure name to add
 * @param place the INTERMEDIATE associated with this procedure.
 */
static void
add_proc(COMPSTATE * cstat, const char *proc_name, struct INTERMEDIATE *place)
{
    struct PROC_LIST *nu;

    nu = malloc(sizeof(struct PROC_LIST));

    nu->name = alloc_string(proc_name);
    nu->code = place;
    nu->next = cstat->procs;
    cstat->procs = nu;
}

/**
 * Add control structure (if, begin, for, try, etc.) to control stack
 *
 * @private
 * @param cstat the compile state struct
 * @param typ the type of conditional - CTYPE_*
 * @param place the INTERMEDIATE associated with this IF
 */
static void
add_control_structure(COMPSTATE * cstat, short typ, struct INTERMEDIATE *place)
{
    struct CONTROL_STACK *nu;

    nu = malloc(sizeof(struct CONTROL_STACK));

    nu->place = place;
    nu->type = typ;
    nu->next = cstat->control_stack;
    nu->extra = 0;
    cstat->control_stack = nu;
}

/**
 * Add to current loop's list of exits remaining to be resolved.
 *
 * This originally said "add while to current loop's..." which is
 * inaccurate since this also handles 'FOR' loops it looks like.
 *
 * It looks like, for a given loop entry in the control stack, this
 * function makes a note of an exit point that needs to be linked
 * to the end of the loop.
 *
 * The 'extra' field which has been something of a mystery for me (tanabi)
 * appears to be used to keep track of loop exit points for a given
 * loop entry.
 *
 * @TODO Verify this comment is true!
 *
 * @private
 * @param cstat the compile state struct
 * @param place the place we have encounter a loop exit point
 */
static void
add_loop_exit(COMPSTATE * cstat, struct INTERMEDIATE *place)
{
    struct CONTROL_STACK *nu;
    struct CONTROL_STACK *loop;

    loop = cstat->control_stack;

    while (loop && loop->type != CTYPE_BEGIN && loop->type != CTYPE_FOR) {
        loop = loop->next;
    }

    if (!loop)
        return;

    nu = malloc(sizeof(struct CONTROL_STACK));

    nu->place = place;
    nu->type = CTYPE_WHILE;
    nu->next = 0;
    nu->extra = loop->extra;
    loop->extra = nu;
}

/**
 * Returns true if a loop start is in the control structure stack.
 *
 * @private
 * @param cstat the compile state structure
 * @return boolean true if the control structure stack is 'in a loop'.
 */
static int
in_loop(COMPSTATE * cstat)
{
    struct CONTROL_STACK *loop;

    loop = cstat->control_stack;

    while (loop && loop->type != CTYPE_BEGIN && loop->type != CTYPE_FOR) {
        loop = loop->next;
    }

    return (loop != NULL);
}

/**
 * Returns the type of the innermost nested control structure.
 *
 * @private
 * @param cstat the compile state structure
 * @return integer the CTYPE_* of the innermost nested control structure.
 */
static int
innermost_control_type(COMPSTATE * cstat)
{
    struct CONTROL_STACK *ctrl;

    ctrl = cstat->control_stack;

    if (!ctrl)
        return 0;

    return ctrl->type;
}

/**
 * Returns number of TRYs before topmost Loop
 *
 * If there is no loop, it's the number of TRYs before the topmost control
 * structure.
 *
 * @private
 * @param cstat the compile state object
 * @return integer number of TRYs before topmost loop.
 */
static int
count_trys_inside_loop(COMPSTATE * cstat)
{
    struct CONTROL_STACK *loop;
    int count = 0;

    for (loop = cstat->control_stack; loop; loop = loop->next) {
        if (loop->type == CTYPE_FOR || loop->type == CTYPE_BEGIN) {
            break;
        }

        if (loop->type == CTYPE_TRY) {
            count++;
        }
    }

    return count;
}

/**
 * Returns topmost CTYPE_* 'type1' or 'type2' from the control stack
 *
 * This is apparently used for BEGIN and FOR mostly, but the way this
 * is written is pretty generic so there's no reason you couldn't use
 * it for finding something else in the control stack.
 *
 * @private
 * @param cstat the compile state structure
 * @param type1 the first CTYPE_* to check for
 * @param type2 the second CTYPE_* to check for
 * @return the struct INTERMEDIATE that belongs to the first stack item match.
 */
static struct INTERMEDIATE *
locate_control_structure(COMPSTATE * cstat, int type1, int type2)
{
    struct CONTROL_STACK *loop;

    loop = cstat->control_stack;

    while (loop) {
        if (loop->type == type1 || loop->type == type2) {
            return loop->place;
        }

        loop = loop->next;
    }

    return 0;
}

/**
 * Pops off the innermost control structure and returns the place.
 *
 * If the control structure isn't type1 OR type2, then this returns NULL
 * and does nothing.  Also returns NULL if control stack is empty.
 *
 * @private
 * @param cstat the compile state struct
 * @param type1 the first CTYPE_* to check for
 * @param type2 the second CTYPE_* to check for
 * @return the struct INTERMEDIATE of the control structure's place.
 */
static struct INTERMEDIATE *
pop_control_structure(COMPSTATE * cstat, int type1, int type2)
{
    struct CONTROL_STACK *ctrl;
    struct INTERMEDIATE *place;

    ctrl = cstat->control_stack;

    if (!ctrl)
        return NULL;

    if (ctrl->type != type1 && ctrl->type != type2)
        return NULL;

    place = ctrl->place;
    cstat->control_stack = ctrl->next;
    free(ctrl);

    return place;
}

/**
 * Preallocate an instruction.
 *
 * This will only preallocate a single instruction.  So if the
 * cstat->nextinst pointer is already set, this will immediately return
 * NULL.
 *
 * @private
 * @param cstat the compile state struct
 * @return an INTERMEDIATE structure that is preallocated or NULL.
 */
static struct INTERMEDIATE *
prealloc_inst(COMPSTATE * cstat)
{
    struct INTERMEDIATE *nu;

    /* only allocate at most one extra instr */
    if (cstat->nextinst)
        return NULL;

    nu = calloc(1, sizeof(struct INTERMEDIATE));

    nu->flags |= (cstat->nested_trys > 0) ? INTMEDFLG_INTRY : 0;

    cstat->nextinst = nu;

    nu->no = cstat->nowords;

    return nu;
}

/**
 * Process "special" tokens
 *
 * This is for certain things that, from a MUF programmer's perspective
 * are basically primitives, but from a compiler perspective are special
 * constructs.
 *
 * This includes procedure definitions (':' and ';'), and all the different
 * loop/branch constructs (IF, BEGIN, WHILE, etc.)
 *
 * The original documentation had the following note regarding branches:
 * Remember --- for those,we've got to set aside an extra argument space.
 *
 * And further, it includes VAR, LVAR, and VAR! but not the ! and @ operators.
 *
 * The majority of this code involves properly tracking the different
 * forms of addresses and code-linkages that these different operations
 * require.
 *
 * NOTE: A return value of 'NULL' is actually valid for this function,
 *       in certain cases.  abort_compile is used for errors.
 *
 * @see abort_compile
 *
 * @private
 * @param cstat the compile state struct
 * @param token the token we are working on
 * @return the resultant intermediate created or NULL which isn't an error.
 */
static struct INTERMEDIATE *
process_special(COMPSTATE * cstat, const char *token)
{
    static char buf[BUFFER_LEN];
    const char *tok;
    struct INTERMEDIATE *nu;

    /*
     * @TODO So this is another huge messy if statement much like
     *       do_directive.  A similar remediation is recommended.
     *
     *       An array of structures that map strings to callbacks would
     *       likely be optimal here; loop over the array and call the
     *       correct callback.
     *
     *       There isn't really a lot of overlap code, however; to my
     *       reading, there's a lot of similar code in these if blocks,
     *       but they differ sufficiently that trying to boil it down
     *       or achieve code reuse is probably more hassle than it is
     *       worth.
     *
     *       Still, taking this from a huge function to a relatively simple
     *       loop and a function per "special" token would be a solid win.
     *
     *       Also, the different code blocks could stand to be documented
     *       better; because of the remediation recommendation, I didn't
     *       go too deep into documenting things here.  Whomever splits
     *       this up, add documents.  Also, the use of "eef" to try and
     *       make a variable name "if" seems obscure to me; I would
     *       recommend using something like "if_ptr" or the like.  The
     *       "eef" convention is used elsewhere in this file and it wasn't
     *       until I saw the comments here that I understood that "eef"
     *       means "if" ... we can do better :)  (tanabi)
     */

    if (!strcasecmp(token, ":")) {
        const char *proc_name;
        int argsflag = 0;

        if (cstat->curr_proc)
            abort_compile(cstat, "Definition within definition.");

        proc_name = next_token(cstat);

        if (!proc_name)
            abort_compile(cstat, "Unexpected end of file within procedure.");

        strcpyn(buf, sizeof(buf), proc_name);

        free((void *) proc_name);

        proc_name = buf;

        if (*proc_name && buf[strlen(buf) - 1] == '[') {
            argsflag = 1;
            buf[strlen(buf) - 1] = '\0';

            if (!*proc_name)
                abort_compile(cstat, "Bad procedure name.");
        }

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_FUNCTION;
        nu->in.line = cstat->lineno;
        nu->in.data.mufproc = malloc(sizeof(struct muf_proc_data));
        nu->in.data.mufproc->procname = strdup(proc_name);
        nu->in.data.mufproc->vars = 0;
        nu->in.data.mufproc->args = 0;
        nu->in.data.mufproc->varnames = NULL;

        cstat->curr_proc = nu;

        /* Handle initial variables in [ ... ] */
        if (argsflag) {
            const char *varspec;
            const char *varname;
            int argsdone = 0;
            int outflag = 0;

            do {
                varspec = next_token(cstat);

                if (!varspec) {
                    free_intermediate_node(nu);
                    abort_compile(cstat,
                                  "Unexpected end of file within procedure arguments declaration.");
                }

                if (!strcmp(varspec, "]")) {
                    argsdone = 1;
                } else if (!strcmp(varspec, "--")) {
                    outflag = 1;
                } else if (!outflag) {
                    varname = strchr(varspec, ':');

                    if (varname) {
                        varname++;
                    } else {
                        varname = varspec;
                    }

                    if (*varname) {
                        if (add_scopedvar(cstat, varname) < 0) {
                            free((void *) varspec);
                            free_intermediate_node(nu);
                            abort_compile(cstat, "Variable limit exceeded.");
                        }

                        nu->in.data.mufproc->vars++;
                        nu->in.data.mufproc->args++;
                    }
                }

                if (varspec) {
                    free((void *) varspec);
                }
            } while (!argsdone);
        }

        add_proc(cstat, proc_name, nu);

        return nu;
    } else if (!strcasecmp(token, ";")) {
        int varcnt;

        if (cstat->control_stack)
            abort_compile(cstat, "Unexpected end of procedure definition.");

        if (!cstat->curr_proc)
            abort_compile(cstat, "Procedure end without body.");

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_PRIMITIVE;
        nu->in.line = cstat->lineno;
        nu->in.data.number = IN_RET;

        varcnt = cstat->curr_proc->in.data.mufproc->vars;

        if (varcnt) {
            cstat->curr_proc->in.data.mufproc->varnames = calloc((size_t)varcnt, sizeof(char *));

            for (int i = 0; i < varcnt; i++) {
                cstat->curr_proc->in.data.mufproc->varnames[i] = cstat->scopedvars[i];
                cstat->scopedvars[i] = 0;
            }
        }

        cstat->curr_proc = 0;

        return nu;
    } else if (!strcasecmp(token, "IF")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "IF outside procedure.");

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_IF;
        nu->in.line = cstat->lineno;
        nu->in.data.call = 0;
        add_control_structure(cstat, CTYPE_IF, nu);

        return nu;
    } else if (!strcasecmp(token, "ELSE")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "ELSE outside procedure.");

        struct INTERMEDIATE *eef;
        int ctrltype = innermost_control_type(cstat);

        switch (ctrltype) {
            case CTYPE_IF:
                break;

            case CTYPE_TRY:
                abort_compile(cstat, "Unterminated TRY-CATCH block at ELSE.");

            case CTYPE_CATCH:
                abort_compile(cstat, "Unterminated CATCH-ENDCATCH block at ELSE.");

            case CTYPE_FOR:
            case CTYPE_BEGIN:
                abort_compile(cstat, "Unterminated Loop at ELSE.");

            default:
                abort_compile(cstat, "ELSE without IF.");
        }

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_JMP;
        nu->in.line = cstat->lineno;
        nu->in.data.call = 0;

        eef = pop_control_structure(cstat, CTYPE_IF, 0);
        add_control_structure(cstat, CTYPE_ELSE, nu);
        eef->in.data.number = get_address(cstat, nu, 1);

        return nu;
    } else if (!strcasecmp(token, "THEN")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "THEN outside procedure.");

        /* can't use 'if' because it's a reserved word */
        struct INTERMEDIATE *eef;
        int ctrltype = innermost_control_type(cstat);

        switch (ctrltype) {
            case CTYPE_IF:
            case CTYPE_ELSE:
                break;

            case CTYPE_TRY:
                abort_compile(cstat, "Unterminated TRY-CATCH block at THEN.");

            case CTYPE_CATCH:
                abort_compile(cstat, "Unterminated CATCH-ENDCATCH block at THEN.");

            case CTYPE_FOR:
            case CTYPE_BEGIN:
                abort_compile(cstat, "Unterminated Loop at THEN.");

            default:
                abort_compile(cstat, "THEN without IF.");
        }

        prealloc_inst(cstat);
        eef = pop_control_structure(cstat, CTYPE_IF, CTYPE_ELSE);
        eef->in.data.number = get_address(cstat, cstat->nextinst, 0);

        return NULL;
    } else if (!strcasecmp(token, "BEGIN")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "BEGIN outside procedure.");

        prealloc_inst(cstat);
        add_control_structure(cstat, CTYPE_BEGIN, cstat->nextinst);

        return NULL;
    } else if (!strcasecmp(token, "FOR")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "FOR outside procedure.");

        struct INTERMEDIATE *new2, *new3;

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_PRIMITIVE;
        nu->in.line = cstat->lineno;
        nu->in.data.number = IN_FOR;

        new2 = (nu->next = new_inst(cstat));
        new2->no = cstat->nowords++;
        new2->in.type = PROG_PRIMITIVE;
        new2->in.line = cstat->lineno;
        new2->in.data.number = IN_FORITER;

        new3 = (new2->next = new_inst(cstat));
        new3->no = cstat->nowords++;
        new3->in.line = cstat->lineno;
        new3->in.type = PROG_IF;
        new3->in.data.number = 0;

        add_control_structure(cstat, CTYPE_FOR, new2);
        cstat->nested_fors++;

        return nu;
    } else if (!strcasecmp(token, "FOREACH")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "FOREACH outside procedure.");

        struct INTERMEDIATE *new2, *new3;

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_PRIMITIVE;
        nu->in.line = cstat->lineno;
        nu->in.data.number = IN_FOREACH;

        new2 = (nu->next = new_inst(cstat));
        new2->no = cstat->nowords++;
        new2->in.type = PROG_PRIMITIVE;
        new2->in.line = cstat->lineno;
        new2->in.data.number = IN_FORITER;

        new3 = (new2->next = new_inst(cstat));
        new3->no = cstat->nowords++;
        new3->in.line = cstat->lineno;
        new3->in.type = PROG_IF;
        new3->in.data.number = 0;

        add_control_structure(cstat, CTYPE_FOR, new2);
        cstat->nested_fors++;

        return nu;
    } else if (!strcasecmp(token, "UNTIL")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "UNTIL outside procedure.");

        /* can't use 'if' because it's a reserved word */
        struct INTERMEDIATE *eef;
        struct INTERMEDIATE *curr;

        int ctrltype = innermost_control_type(cstat);

        switch (ctrltype) {
            case CTYPE_FOR:
                cstat->nested_fors--;

            case CTYPE_BEGIN:
                break;

            case CTYPE_TRY:
                abort_compile(cstat, "Unterminated TRY-CATCH block at UNTIL.");

            case CTYPE_CATCH:
                abort_compile(cstat, "Unterminated CATCH-ENDCATCH block at UNTIL.");

            case CTYPE_IF:
            case CTYPE_ELSE:
                abort_compile(cstat, "Unterminated IF-THEN at UNTIL.");

            default:
                abort_compile(cstat, "Loop start not found for UNTIL.");
        }

        prealloc_inst(cstat);
        resolve_loop_addrs(cstat, get_address(cstat, cstat->nextinst, 1));
        eef = pop_control_structure(cstat, CTYPE_BEGIN, CTYPE_FOR);

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_IF;
        nu->in.line = cstat->lineno;
        nu->in.data.number = get_address(cstat, eef, 0);

        if (ctrltype == CTYPE_FOR) {
            curr = (nu->next = new_inst(cstat));
            curr->no = cstat->nowords++;
            curr->in.type = PROG_PRIMITIVE;
            curr->in.line = cstat->lineno;
            curr->in.data.number = IN_FORPOP;
        }

        return nu;
    } else if (!strcasecmp(token, "WHILE")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "WHILE outside procedure.");

        struct INTERMEDIATE *curr;
        int trycount;

        if (!in_loop(cstat))
            abort_compile(cstat, "Can't have a WHILE outside of a loop.");

        trycount = count_trys_inside_loop(cstat);
        nu = curr = NULL;

        while (trycount-- > 0) {
            if (!nu) {
                nu = curr = new_inst(cstat);
            } else {
                nu = (nu->next = new_inst(cstat));
            }

            nu->no = cstat->nowords++;
            nu->in.type = PROG_PRIMITIVE;
            nu->in.line = cstat->lineno;
            nu->in.data.number = IN_TRYPOP;
        }

        if (nu) {
            nu = (nu->next = new_inst(cstat));
        } else {
            curr = nu = new_inst(cstat);
        }

        nu->no = cstat->nowords++;
        nu->in.type = PROG_IF;
        nu->in.line = cstat->lineno;
        nu->in.data.number = 0;

        add_loop_exit(cstat, nu);

        return curr;
    } else if (!strcasecmp(token, "BREAK")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "BREAK outside procedure.");

        int trycount;
        struct INTERMEDIATE *curr;

        if (!in_loop(cstat))
            abort_compile(cstat, "Can't have a BREAK outside of a loop.");

        trycount = count_trys_inside_loop(cstat);
        nu = curr = NULL;

        while (trycount-- > 0) {
            if (!nu) {
                nu = curr = new_inst(cstat);
            } else {
                nu = (nu->next = new_inst(cstat));
            }

            nu->no = cstat->nowords++;
            nu->in.type = PROG_PRIMITIVE;
            nu->in.line = cstat->lineno;
            nu->in.data.number = IN_TRYPOP;
        }

        if (nu) {
            nu = (nu->next = new_inst(cstat));
        } else {
            curr = nu = new_inst(cstat);
        }

        nu->no = cstat->nowords++;
        nu->in.type = PROG_JMP;
        nu->in.line = cstat->lineno;
        nu->in.data.number = 0;

        add_loop_exit(cstat, nu);

        return curr;
    } else if (!strcasecmp(token, "CONTINUE")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "CONTINUE outside procedure.");

        /* can't use 'if' because it's a reserved word */
        struct INTERMEDIATE *beef;
        struct INTERMEDIATE *curr;
        int trycount;

        if (!in_loop(cstat))
            abort_compile(cstat, "Can't CONTINUE outside of a loop.");

        beef = locate_control_structure(cstat, CTYPE_FOR, CTYPE_BEGIN);
        trycount = count_trys_inside_loop(cstat);
        nu = curr = NULL;

        while (trycount-- > 0) {
            if (!nu) {
                nu = curr = new_inst(cstat);
            } else {
                nu = (nu->next = new_inst(cstat));
            }

            nu->no = cstat->nowords++;
            nu->in.type = PROG_PRIMITIVE;
            nu->in.line = cstat->lineno;
            nu->in.data.number = IN_TRYPOP;
        }

        if (nu) {
            nu = (nu->next = new_inst(cstat));
        } else {
            curr = nu = new_inst(cstat);
        }

        nu->no = cstat->nowords++;
        nu->in.type = PROG_JMP;
        nu->in.line = cstat->lineno;
        nu->in.data.number = get_address(cstat, beef, 0);

        return curr;
    } else if (!strcasecmp(token, "REPEAT")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "REPEAT outside procedure.");

        /* can't use 'if' because it's a reserved word */
        struct INTERMEDIATE *eef;
        struct INTERMEDIATE *curr;
        int ctrltype = innermost_control_type(cstat);

        switch (ctrltype) {
            case CTYPE_FOR:
                cstat->nested_fors--;

            case CTYPE_BEGIN:
                break;

            case CTYPE_TRY:
                abort_compile(cstat, "Unterminated TRY-CATCH block at REPEAT.");

            case CTYPE_CATCH:
                abort_compile(cstat, "Unterminated CATCH-ENDCATCH block at REPEAT.");

            case CTYPE_IF:
            case CTYPE_ELSE:
                abort_compile(cstat, "Unterminated IF-THEN at REPEAT.");

            default:
                abort_compile(cstat, "Loop start not found for REPEAT.");
        }

        prealloc_inst(cstat);
        resolve_loop_addrs(cstat, get_address(cstat, cstat->nextinst, 1));
        eef = pop_control_structure(cstat, CTYPE_BEGIN, CTYPE_FOR);

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_JMP;
        nu->in.line = cstat->lineno;
        nu->in.data.number = get_address(cstat, eef, 0);

        if (ctrltype == CTYPE_FOR) {
            curr = (nu->next = new_inst(cstat));
            curr->no = cstat->nowords++;
            curr->in.type = PROG_PRIMITIVE;
            curr->in.line = cstat->lineno;
            curr->in.data.number = IN_FORPOP;
        }

        return nu;
    } else if (!strcasecmp(token, "TRY")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "TRY outside procedure.");

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_TRY;
        nu->in.line = cstat->lineno;
        nu->in.data.number = 0;

        add_control_structure(cstat, CTYPE_TRY, nu);
        cstat->nested_trys++;

        return nu;
    } else if (!strcasecmp(token, "CATCH") || !strcasecmp(token, "CATCH_DETAILED")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "CATCH outside procedure.");

        /* can't use 'if' because it's a reserved word */
        struct INTERMEDIATE *eef;
        struct INTERMEDIATE *curr;
        struct INTERMEDIATE *jump;

        int ctrltype = innermost_control_type(cstat);

        switch (ctrltype) {
            case CTYPE_TRY:
                break;

            case CTYPE_FOR:
            case CTYPE_BEGIN:
                abort_compile(cstat, "Unterminated Loop at CATCH.");

            case CTYPE_IF:
            case CTYPE_ELSE:
                abort_compile(cstat, "Unterminated IF-THEN at CATCH.");

            case CTYPE_CATCH:
            default:
                abort_compile(cstat, "No TRY found for CATCH.");
        }

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_PRIMITIVE;
        nu->in.line = cstat->lineno;
        nu->in.data.number = IN_TRYPOP;

        jump = (nu->next = new_inst(cstat));
        jump->no = cstat->nowords++;
        jump->in.type = PROG_JMP;
        jump->in.line = cstat->lineno;
        jump->in.data.number = 0;

        curr = (jump->next = new_inst(cstat));
        curr->no = cstat->nowords++;
        curr->in.type = PROG_PRIMITIVE;
        curr->in.line = cstat->lineno;
        curr->in.data.number = IN_CATCH;

        if (!strcasecmp(token, "CATCH_DETAILED")) {
            curr->in.data.number = IN_CATCH_DETAILED;
        }

        eef = pop_control_structure(cstat, CTYPE_TRY, 0);
        cstat->nested_trys--;
        eef->in.data.number = get_address(cstat, curr, 0);
        add_control_structure(cstat, CTYPE_CATCH, jump);

        return nu;
    } else if (!strcasecmp(token, "ENDCATCH")) {
        /* can't use 'if' because it's a reserved word */
        struct INTERMEDIATE *eef;
        int ctrltype = innermost_control_type(cstat);

        switch (ctrltype) {
            case CTYPE_CATCH:
                break;

            case CTYPE_FOR:
            case CTYPE_BEGIN:
                abort_compile(cstat, "Unterminated Loop at ENDCATCH.");

            case CTYPE_IF:
            case CTYPE_ELSE:
                abort_compile(cstat, "Unterminated IF-THEN at ENDCATCH.");

            case CTYPE_TRY:
            default:
                abort_compile(cstat, "No CATCH found for ENDCATCH.");
        }

        prealloc_inst(cstat);
        eef = pop_control_structure(cstat, CTYPE_CATCH, 0);
        eef->in.data.number = get_address(cstat, cstat->nextinst, 0);

        return NULL;
    } else if (!strcasecmp(token, "CALL")) {
        if (!cstat->curr_proc)
            abort_compile(cstat, "CALL outside procedure.");

        nu = new_inst(cstat);
        nu->no = cstat->nowords++;
        nu->in.type = PROG_PRIMITIVE;
        nu->in.line = cstat->lineno;
        nu->in.data.number = IN_CALL;

        return nu;
    } else if (!strcasecmp(token, "WIZCALL") || !strcasecmp(token, "PUBLIC")) {
        struct PROC_LIST *p;
        int wizflag = 0;

        if (!strcasecmp(token, "WIZCALL"))
            wizflag = 1;

        if (cstat->curr_proc)
            abort_compile(cstat, "PUBLIC  or WIZCALL declaration within procedure.");

        tok = next_token(cstat);

        if (!tok) {
            abort_compile(cstat, "Subroutine unknown in PUBLIC or WIZCALL declaration.");
        }

        for (p = cstat->procs; p; p = p->next)
            if (!strcasecmp(p->name, tok))
                break;

        if (!p) {
            free((void *) tok);
            abort_compile(cstat, "Subroutine unknown in PUBLIC or WIZCALL declaration.");
        }

        if (!cstat->currpubs) {
            cstat->currpubs = malloc(sizeof(struct publics));
            cstat->currpubs->next = NULL;
            cstat->currpubs->subname = strdup(tok);

            free((void *) tok);

            cstat->currpubs->addr.no = get_address(cstat, p->code, 0);
            cstat->currpubs->mlev = wizflag ? 4 : 1;
            return 0;
        }

        for (struct publics *pub = cstat->currpubs; pub;) {
            if (!strcasecmp(tok, pub->subname)) {
                free((void *) tok);
                abort_compile(cstat, "Function already declared public.");
            } else {
                if (pub->next) {
                    pub = pub->next;
                } else {
                    pub->next = malloc(sizeof(struct publics));
                    pub = pub->next;
                    pub->next = NULL;
                    pub->subname = strdup(tok);

                    free((void *) tok);

                    pub->addr.no = get_address(cstat, p->code, 0);
                    pub->mlev = wizflag ? 4 : 1;
                    pub = NULL;
                }
            }
        }

        return 0;
    } else if (!strcasecmp(token, "VAR")) {
        if (cstat->curr_proc) {
            tok = next_token(cstat);

            if (!tok)
                abort_compile(cstat, "Unexpected end of program.");

            if (add_scopedvar(cstat, tok) < 0) {
                free((void *) tok);
                abort_compile(cstat, "Variable limit exceeded.");
            }

            free((void *) tok);

            cstat->curr_proc->in.data.mufproc->vars++;
        } else {
            tok = next_token(cstat);

            if (!tok)
                abort_compile(cstat, "Unexpected end of program.");

            if (!add_variable(cstat, tok)) {
                free((void *) tok);
                abort_compile(cstat, "Variable limit exceeded.");
            }

            free((void *) tok);
        }

        return 0;
    } else if (!strcasecmp(token, "VAR!")) {
        if (cstat->curr_proc) {
            tok = next_token(cstat);

            if (!tok)
                abort_compile(cstat, "Unexpected end of program.");

            if (add_scopedvar(cstat, tok) < 0) {
                free((void *) tok);
                abort_compile(cstat, "Variable limit exceeded.");
            }

            free((void *) tok);

            nu = new_inst(cstat);
            nu->no = cstat->nowords++;
            nu->in.type = PROG_SVAR_BANG;
            nu->in.line = cstat->lineno;
            nu->in.data.number = cstat->curr_proc->in.data.mufproc->vars++;

            return nu;
        } else {
            abort_compile(cstat, "VAR! used outside of procedure.");
        }
    } else if (!strcasecmp(token, "LVAR")) {
        if (cstat->curr_proc)
            abort_compile(cstat, "Local variable declared within procedure.");

        tok = next_token(cstat);

        if (!tok || (add_localvar(cstat, tok) == -1)) {
            free((void *) tok);
            abort_compile(cstat, "Local variable limit exceeded.");
        }

        free((void *) tok);

        return 0;
    } else {
        snprintf(buf, sizeof(buf), "Unrecognized special form %s found. (%d)", token,
                 cstat->lineno);
        abort_compile(cstat, buf);
    }
}

/**
 * Return primitive instruction number
 *
 * @param token the primitive token name such as "GETPROPSTR", etc.
 * @return integer token instruction number
 */
int
get_primitive(const char *token)
{
    hash_data *hd;

    if ((hd = find_hash(token, primitive_list, COMP_HASH_SIZE)) == NULL)
        return 0;
    else {
        return (hd->ival);
    }
}

/**
 * Implementation of "free_prog" macro that actually frees program memory
 *
 * This is the underpinning of a lot of different calls such as
 * free_unused_programs, uncompile_program, and various internal compiler
 * calls.
 *
 * @see free_unused_programs
 * @see uncompile_program
 *
 * This frees all the memory associated with a given dbref's in-memory
 * byte compiled program code.
 *
 * The macro free_prog(dbref) is always used instead of a direct call
 * to this function, and it provides __FILE__ and __LINE__ to the appropriate
 * parameters.
 *
 * You should do proper cleanup before running this -- for instance, make
 * sure there are no running instances of this program first.
 *
 * @private
 * @param prog the DBREF of the program
 * @param file the intention is to use __FILE__ here
 * @param line the intention is to use __LINE__ here
 */
static void
free_prog_real(dbref prog, const char *file, const int line)
{
    int i;
    struct inst *c = PROGRAM_CODE(prog);
    int siz = PROGRAM_SIZ(prog);

    if (c) {
        char unparse_buf[BUFFER_LEN];
        unparse_object(GOD, prog, unparse_buf, sizeof(unparse_buf));

        /*
         * These sanity checks are to prevent faulty C programming and to
         * make sure proper cleanup was done before this was run.
         */
        if (PROGRAM_INSTANCES(prog)) {
            log_status("WARNING: freeing program %s with %d instances reported from %s:%d",
                       unparse_buf, PROGRAM_INSTANCES(prog), file, line);
        }

        i = scan_instances(prog);

        if (i) {
            log_status("WARNING: freeing program %s with %d instances found from %s:%d",
                       unparse_buf, i, file, line);
        }

        for (i = 0; i < siz; i++) {
            if (c[i].type == PROG_ADD) {
                if (c[i].data.addr->links != 1) {
                    log_status("WARNING: Freeing address in %s with link count %d from %s:%d",
                               unparse_buf, c[i].data.addr->links, file, line);
                }

                free(c[i].data.addr);
            } else {
                CLEAR(c + i);
            }
        }

        free(c);
    }

    PROGRAM_SET_CODE(prog, 0);
    PROGRAM_SET_SIZ(prog, 0);
    PROGRAM_SET_START(prog, 0);
}

/**
 * Calculate the in-memory size of a program
 *
 * This may be 0 if the program is not loaded (PROGRAM_CODE(prog) is NULL)
 * Otherwise it is a (hopefully) accurate representation of a program's
 * memory usage.
 *
 * @param prog the dbref of the program we are size checking
 * @return the size in bytes of consumed memory
 */
size_t
size_prog(dbref prog)
{
    struct inst *c;
    int varcnt, siz;
    size_t byts = 0;

    c = PROGRAM_CODE(prog);

    if (!c)
        return 0;

    siz = PROGRAM_SIZ(prog);

    for (int i = 0; i < siz; i++) {
        byts += sizeof(*c);

        if (c[i].type == PROG_FUNCTION) {
            byts += strlen(c[i].data.mufproc->procname) + 1;
            varcnt = c[i].data.mufproc->vars;

            if (c[i].data.mufproc->varnames) {
                for (long j = 0; j < varcnt; j++) {
                    byts += strlen(c[i].data.mufproc->varnames[j]) + 1;
                }

                byts += sizeof(char **) * (size_t)varcnt;
            }

            byts += sizeof(struct muf_proc_data);
        } else if (c[i].type == PROG_STRING && c[i].data.string) {
            byts += strlen(c[i].data.string->data) + 1;
            byts += sizeof(struct shared_string);
        } else if (c[i].type == PROG_ADD)
            byts += sizeof(struct prog_addr);
    }

    byts += size_pubs(PROGRAM_PUBS(prog));
    return byts;
}

/**
 * Free the memory used by the primitive hash
 */
void
clear_primitives(void)
{
    kill_hash(primitive_list, COMP_HASH_SIZE, 0);
}

/**
 * Initialize the hash of primitives
 *
 * This must run before the compile will work.  It first clears anything
 * currently in the primitive hash, then loads all the primitives from
 * base_inst into the hash.
 *
 * This also loads the value of variables IN_FORPOP, IN_FORITER, IN_FOR,
 * IN_FOREACH, and IN_TRYPOP.  The number of primitives is logged via
 * log_status
 *
 * @see log_status
 */
void
init_primitives(void)
{
    clear_primitives();

    prim_count = ARRAYSIZE(base_inst);

    for (int i = 1; i <= prim_count; i++) {
        hash_data hd;
        hd.ival = i;

        if (add_hash(base_inst[i - 1], hd, primitive_list, COMP_HASH_SIZE) == NULL)
            panic("Out of memory");
    }

    IN_FORPOP = get_primitive(" FORPOP");
    IN_FORITER = get_primitive(" FORITER");
    IN_FOR = get_primitive(" FOR");
    IN_FOREACH = get_primitive(" FOREACH");
    IN_TRYPOP = get_primitive(" TRYPOP");
    log_status("MUF: %d primitives exist.", prim_count);
}
