#include "config.h"

#include "compile.h"
#include "db.h"
#include "edit.h"
#include "fbstrings.h"
#include "game.h"
#include "hashtab.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#ifdef MCP_SUPPORT
#include "mcp.h"
#endif
#include "params.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

#include <stdarg.h>

#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'
#define BEGINDIRECTIVE '$'
#define BEGINESCAPE '\\'

#define SUBSTITUTIONS 20 

#define CTYPE_IF    1
#define CTYPE_ELSE  2
#define CTYPE_BEGIN 3
#define CTYPE_FOR   4
#define CTYPE_WHILE 5
#define CTYPE_TRY   6
#define CTYPE_CATCH 7

#define ADDRLIST_ALLOC_CHUNK_SIZE 256

#define abort_compile(ST,C) { do_abort_compile(ST,C); return 0; }
#define v_abort_compile(ST,C) { do_abort_compile(ST,C); return; }

static hash_tab primitive_list[COMP_HASH_SIZE];

struct CONTROL_STACK {
    short type;
    struct INTERMEDIATE *place;
    struct CONTROL_STACK *next;
    struct CONTROL_STACK *extra;
};

/* This structure is an association list that contains both a procedure
   name and the place in the code that it belongs.  A lookup to the procedure
   will see both it's name and it's number and so we can generate a
   reference to it.  Since I want to disallow co-recursion,  I will not allow
   forward referencing.
   */

struct PROC_LIST {
    const char *name;
    int returntype;
    struct INTERMEDIATE *code;
    struct PROC_LIST *next;
};

/* The intermediate code is code generated as a linked list
   when there is no hint or notion of how much code there
   will be, and to help resolve all references.
   There is always a pointer to the current word that is
   being compiled kept.
   */

#define INTMEDFLG_DIVBYZERO 1
#define INTMEDFLG_MODBYZERO 2
#define INTMEDFLG_INTRY         4

#define IMMFLAG_REFERENCED	1	/* Referenced by a jump */

struct INTERMEDIATE {
    int no;                     /* which number instruction this is */
    struct inst in;             /* instruction itself */
    short line;                 /* line number of instruction */
    short flags;
    struct INTERMEDIATE *next;  /* next instruction */
};

/* The state structure for a compile. */
typedef struct COMPILE_STATE_T {
    struct CONTROL_STACK *control_stack;
    struct PROC_LIST *procs;

    int nowords;                /* number of words compiled */
    struct INTERMEDIATE *curr_word;     /* word being compiled */
    struct INTERMEDIATE *first_word;    /* first word of the list */
    struct INTERMEDIATE *curr_proc;     /* first word of curr. proc. */
    struct publics *currpubs;
    int nested_fors;
    int nested_trys;

    /* Address resolution data.  Used to relink addresses after compile. */
    struct INTERMEDIATE **addrlist;     /* list of addresses to resolve */
    int *addroffsets;           /* list of offsets from instrs */
    int addrmax;                /* size of current addrlist array */
    int addrcount;              /* number of allocated addresses */

    /* variable names.  The index into cstat->variables give you what position
     * the variable holds.
     */
    const char *variables[MAX_VAR];
    int variabletypes[MAX_VAR];
    const char *localvars[MAX_VAR];
    int localvartypes[MAX_VAR];
    const char *scopedvars[MAX_VAR];
    int scopedvartypes[MAX_VAR];

    struct line *curr_line;     /* current line */
    int lineno;                 /* current line number */
    int start_comment;          /* Line last comment started at */
    int force_comment;          /* Only attempt certain compile. */
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

int IN_FOR;
int IN_FOREACH;
int IN_FORITER;
int IN_FORPOP;
int IN_TRYPOP;

static void free_prog_real(dbref, const char *, const int);
static const char *next_token(COMPSTATE *);
static struct INTERMEDIATE *process_special(COMPSTATE *, const char *);

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

static void
free_intermediate_node(struct INTERMEDIATE *wd)
{
    int varcnt;

    if (wd->in.type == PROG_STRING) {
	if (wd->in.data.string)
	    free((void *) wd->in.data.string);
    }
    if (wd->in.type == PROG_FUNCTION) {
	free((void *) wd->in.data.mufproc->procname);
	varcnt = wd->in.data.mufproc->vars;
	if (wd->in.data.mufproc->varnames) {
	    for (int j = 0; j < varcnt; j++) {
		free((void *) wd->in.data.mufproc->varnames[j]);
	    }
	    free((void *) wd->in.data.mufproc->varnames);
	}
	free((void *) wd->in.data.mufproc);
    }
    free((void *) wd);
}

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

static void
free_addresses(COMPSTATE * cstat)
{
    cstat->addrcount = 0;
    cstat->addrmax = 0;
    if (cstat->addrlist)
	free(cstat->addrlist);
    if (cstat->addroffsets)
	free(cstat->addroffsets);
    cstat->addrlist = NULL;
}

static void
purge_defs(COMPSTATE * cstat)
{
    kill_hash(cstat->defhash, DEFHASHSIZE, 1);
}

static void
cleanup(COMPSTATE * cstat)
{
    struct CONTROL_STACK *eef, *tempif;
    struct PROC_LIST *p, *tempp;

    free_intermediate_chain(cstat->first_word);
    cstat->first_word = 0;

    for (eef = cstat->control_stack; eef; eef = tempif) {
	tempif = eef->next;
	free((void *) eef);
    }
    cstat->control_stack = 0;

    for (p = cstat->procs; p; p = tempp) {
	tempp = p->next;
	free((void *) p->name);
	free((void *) p);
    }
    cstat->procs = 0;

    purge_defs(cstat);
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

static void
do_abort_compile(COMPSTATE * cstat, const char *c)
{
    static char _buf[BUFFER_LEN];

    if (cstat->start_comment) {
	snprintf(_buf, sizeof(_buf), "Error in line %d: %s  Comment starting at line %d.",
		 cstat->lineno, c, cstat->start_comment);
	cstat->start_comment = 0;
    } else {
	snprintf(_buf, sizeof(_buf), "Error in line %d: %s", cstat->lineno, c);
    }
    if (cstat->line_copy) {
	free((void *) cstat->line_copy);
	cstat->line_copy = NULL;
    }
    if (((FLAGS(cstat->player) & INTERACTIVE) && !(FLAGS(cstat->player) & READMODE)) ||
	cstat->force_err_display) {
	notify_nolisten(cstat->player, _buf, 1);
    } else {
	log_muf("%s(#%d) [%s(#%d)] %s(#%d) %s",
		NAME(OWNER(cstat->program)), OWNER(cstat->program),
		NAME(cstat->program), cstat->program,
		NAME(cstat->player), cstat->player, _buf);
    }
    cstat->compile_err++;
    if (cstat->compile_err > 1) {
	return;
    }
    if (cstat->nextinst) {
	struct INTERMEDIATE *ptr;
	while (cstat->nextinst) {
	    ptr = cstat->nextinst;
	    cstat->nextinst = ptr->next;
	    free(ptr);
	}
	cstat->nextinst = NULL;
    }
    cleanup(cstat);
    cleanpubs(cstat->currpubs);
    cstat->currpubs = NULL;
    free_prog(cstat->program);
    cleanpubs(PROGRAM_PUBS(cstat->program));
    PROGRAM_SET_PUBS(cstat->program, NULL);
#ifdef MCP_SUPPORT
    clean_mcpbinds(PROGRAM_MCPBINDS(cstat->program));
    PROGRAM_SET_MCPBINDS(cstat->program, NULL);
#endif
    PROGRAM_SET_PROFTIME(cstat->program, 0, 0);
}

static void
compiler_warning(COMPSTATE * cstat, char *text, ...)
{
    char buf[BUFFER_LEN];
    va_list vl;

    va_start(vl, text);
    vsnprintf(buf, sizeof(buf), text, vl);
    va_end(vl);

    notify_nolisten(cstat->player, buf, 1);
}

static int
get_address(COMPSTATE * cstat, struct INTERMEDIATE *dest, int offset)
{
    if (!cstat->addrlist) {
	cstat->addrcount = 0;
	cstat->addrmax = ADDRLIST_ALLOC_CHUNK_SIZE;
	cstat->addrlist = (struct INTERMEDIATE **)
		malloc(cstat->addrmax * sizeof(struct INTERMEDIATE *));
	cstat->addroffsets = (int *)
		malloc(cstat->addrmax * sizeof(int));
    }

    for (int i = 0; i < cstat->addrcount; i++)
	if (cstat->addrlist[i] == dest && cstat->addroffsets[i] == offset)
	    return i;

    if (cstat->addrcount >= cstat->addrmax) {
	cstat->addrmax += ADDRLIST_ALLOC_CHUNK_SIZE;
	cstat->addrlist = (struct INTERMEDIATE **)
		realloc(cstat->addrlist, cstat->addrmax * sizeof(struct INTERMEDIATE *));
	cstat->addroffsets = (int *)
		realloc(cstat->addroffsets, cstat->addrmax * sizeof(int));
    }

    cstat->addrlist[cstat->addrcount] = dest;
    cstat->addroffsets[cstat->addrcount] = offset;
    return cstat->addrcount++;
}

static void
fix_addresses(COMPSTATE * cstat)
{
    struct INTERMEDIATE *ptr;
    struct publics *pub;
    int count = 0;

    /* renumber the instruction chain */
    for (ptr = cstat->first_word; ptr; ptr = ptr->next)
	ptr->no = count++;

    /* repoint publics to targets */
    for (pub = cstat->currpubs; pub; pub = pub->next)
	pub->addr.no = cstat->addrlist[pub->addr.no]->no + cstat->addroffsets[pub->addr.no];

    /* repoint addresses to targets */
    for (ptr = cstat->first_word; ptr; ptr = ptr->next) {
	switch (ptr->in.type) {
	case PROG_ADD:
	case PROG_IF:
	case PROG_TRY:
	case PROG_JMP:
	case PROG_EXEC:
	    ptr->in.data.number = cstat->addrlist[ptr->in.data.number]->no +
		    cstat->addroffsets[ptr->in.data.number];
	    break;
	default:
	    break;
	}
    }
}

static void
fixpubs(struct publics *mypubs, struct inst *offset)
{
    while (mypubs) {
	mypubs->addr.ptr = offset + mypubs->addr.no;
	mypubs = mypubs->next;
    }
}

static int
size_pubs(struct publics *mypubs)
{
    int bytes = 0;

    while (mypubs) {
	bytes += sizeof(*mypubs);
	mypubs = mypubs->next;
    }
    return bytes;
}

static char *
expand_def(COMPSTATE * cstat, const char *defname)
{
    hash_data *exp = find_hash(defname, cstat->defhash, DEFHASHSIZE);

    if (!exp) {
	if (*defname == BEGINMACRO) {
	    return (macro_expansion(macrotop, &defname[1]));
	} else {
	    return (NULL);
	}
    }
    return (strdup((char *) exp->pval));
}

static void
kill_def(COMPSTATE * cstat, const char *defname)
{
    hash_data *exp = find_hash(defname, cstat->defhash, DEFHASHSIZE);

    if (exp) {
	free(exp->pval);
	(void) free_hash(defname, cstat->defhash, DEFHASHSIZE);
    }
}

static void
insert_def(COMPSTATE * cstat, const char *defname, const char *deff)
{
    hash_data hd;

    (void) kill_def(cstat, defname);
    hd.pval = (void *) strdup(deff);
    (void) add_hash(defname, hd, cstat->defhash, DEFHASHSIZE);
}

static void
insert_intdef(COMPSTATE * cstat, const char *defname, int deff)
{
    char buf[sizeof(int) * 3];

    snprintf(buf, sizeof(buf), "%d", deff);
    insert_def(cstat, defname, buf);
}

static void
include_defs(COMPSTATE * cstat, dbref i)
{
    char dirname[BUFFER_LEN];
    char temp[BUFFER_LEN];
    const char *tmpptr;
    PropPtr j, pptr;

    strcpyn(dirname, sizeof(dirname), "/_defs/");
    j = first_prop(i, dirname, &pptr, temp, sizeof(temp));
    while (j) {
	strcpyn(dirname, sizeof(dirname), "/_defs/");
	strcatn(dirname, sizeof(dirname), temp);
	tmpptr = get_property_class(i, dirname);
	if (tmpptr && *tmpptr)
	    insert_def(cstat, temp, (char *) tmpptr);
	j = next_prop(pptr, j, temp, sizeof(temp));
    }
}

static void
include_internal_defs(COMPSTATE * cstat)
{
    /* Create standard server defines */
    insert_def(cstat, "__version", VERSION);
    insert_def(cstat, "__muckname", tp_muckname);
    insert_intdef(cstat, "__fuzzball__", 1);
    insert_def(cstat, "strip", "striplead striptail");
    insert_intdef(cstat, "bg_mode", BACKGROUND);
    insert_intdef(cstat, "fg_mode", FOREGROUND);
    insert_intdef(cstat, "pr_mode", PREEMPT);
    insert_intdef(cstat, "max_variable_count", MAX_VAR);
    insert_intdef(cstat, "sorttype_caseinsens", SORTTYPE_CASEINSENS);
    insert_intdef(cstat, "sorttype_descending", SORTTYPE_DESCENDING);
    insert_intdef(cstat, "sorttype_case_ascend", SORTTYPE_CASE_ASCEND);
    insert_intdef(cstat, "sorttype_nocase_ascend", SORTTYPE_NOCASE_ASCEND);
    insert_intdef(cstat, "sorttype_case_descend", SORTTYPE_CASE_DESCEND);
    insert_intdef(cstat, "sorttype_nocase_descend", SORTTYPE_NOCASE_DESCEND);
    insert_intdef(cstat, "sorttype_shuffle", SORTTYPE_SHUFFLE);

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
    insert_def(cstat, "preempt", "pr_mode setmode");
    insert_def(cstat, "background", "bg_mode setmode");
    insert_def(cstat, "foreground", "fg_mode setmode");
    insert_def(cstat, "notify_except", "1 swap notify_exclude");
    insert_def(cstat, "event_wait", "0 array_make event_waitfor");
    insert_def(cstat, "tread",
	       "\"__tread\" timer_start { \"TIMER.__tread\" \"READ\" }list event_waitfor swap pop \"READ\" strcmp if \"\" 0 else read 1 \"__tread\" timer_stop then");
    insert_def(cstat, "truename", "name");
    insert_def(cstat, "dbcmp", "=");

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

#ifdef MCP_SUPPORT
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
}

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

void
uncompile_program(dbref i)
{
    /* free program */
    (void) dequeue_prog(i, 1);
    free_prog(i);
    cleanpubs(PROGRAM_PUBS(i));
    PROGRAM_SET_PUBS(i, NULL);
#ifdef MCP_SUPPORT
    clean_mcpbinds(PROGRAM_MCPBINDS(i));
    PROGRAM_SET_MCPBINDS(i, NULL);
#endif
    PROGRAM_SET_PROFTIME(i, 0, 0);
    PROGRAM_SET_CODE(i, NULL);
    PROGRAM_SET_SIZ(i, 0);
    PROGRAM_SET_START(i, NULL);
}

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

void
free_unused_programs()
{
    time_t now = time(NULL);

    for (dbref i = 0; i < db_top; i++) {
	if ((Typeof(i) == TYPE_PROGRAM) && !(FLAGS(i) & (ABODE | INTERNAL)) &&
	    (now - DBFETCH(i)->ts_lastused > tp_clean_interval)
	    && (PROGRAM_INSTANCES(i) == 0)) {
	    uncompile_program(i);
	}
    }
}

/* Checks code for valid fetch-and-clear optim changes, and does them. */
static void
MaybeOptimizeVarsAt(COMPSTATE * cstat, struct INTERMEDIATE *first, int AtNo, int BangNo)
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
	    /* Don't trust any physical @ or !'s in the code, someone
	       may be indirectly referencing the scoped variable */
	    /* Don't trust any explicit jmp's in the code. */

	    if ((curr->in.data.number == AtNo) ||
		(curr->in.data.number == BangNo) || (curr->in.data.number == IN_JMP)) {
		return;
	    }

	    if (lvarflag) {
		/* For lvars, don't trust the following prims... */
		/*   EXITs escape the code path without leaving lvar scope. */
		/*   EXECUTEs escape the code path without leaving lvar scope. */
		/*   CALLs cause re-entrancy problems. */
		if (curr->in.data.number == IN_RET ||
		    curr->in.data.number == IN_EXECUTE || curr->in.data.number == IN_CALL) {
		    return;
		}
	    }
	    break;

	case PROG_LVAR_AT:
	case PROG_LVAR_AT_CLEAR:
	    if (lvarflag) {
		if (curr->in.data.number == first->in.data.number) {
		    /* Can't optimize if references to the variable found before a var! */
		    return;
		}
	    }
	    break;

	case PROG_SVAR_AT:
	case PROG_SVAR_AT_CLEAR:
	    if (!lvarflag) {
		if (curr->in.data.number == first->in.data.number) {
		    /* Can't optimize if references to the variable found before a var! */
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
		/* Can't optimize as we've exited the code branch the @ is in. */
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

void
RemoveNextIntermediate(COMPSTATE * cstat, struct INTERMEDIATE *curr)
{
    struct INTERMEDIATE *tmp;

    if (!curr->next) {
	return;
    }

    tmp = curr->next;
    for (int i = 0; i < cstat->addrcount; i++) {
	if (cstat->addrlist[i] == tmp) {
	    cstat->addrlist[i] = curr;
	}
    }
    curr->next = curr->next->next;
    free_intermediate_node(tmp);
    cstat->nowords--;
}

static void
RemoveIntermediate(COMPSTATE * cstat, struct INTERMEDIATE *curr)
{
    if (!curr->next) {
	return;
    }

    curr->no = curr->next->no;
    curr->in.line = curr->next->in.line;
    curr->in.type = curr->next->in.type;
    switch (curr->in.type) {
    case PROG_STRING:
	curr->in.data.string = curr->next->in.data.string;
	break;
    case PROG_FLOAT:
	curr->in.data.fnumber = curr->next->in.data.fnumber;
	break;
    case PROG_FUNCTION:
	curr->in.data.mufproc = curr->next->in.data.mufproc;
	break;
    case PROG_ADD:
	curr->in.data.addr = curr->next->in.data.addr;
	break;
    case PROG_IF:
    case PROG_TRY:
    case PROG_JMP:
    case PROG_EXEC:
	curr->in.data.call = curr->next->in.data.call;
	break;
    default:
	curr->in.data.number = curr->next->in.data.number;
	break;
    }
    curr->next->in.type = PROG_INTEGER;
    curr->next->in.data.number = 0;
    RemoveNextIntermediate(cstat, curr);
}

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

static int
OptimizeIntermediate(COMPSTATE * cstat, int force_err_display)
{
    struct INTERMEDIATE *curr;
    int *Flags;
    int i;
    int count = 0;
    int old_instr_count = cstat->nowords;
    int AtNo = get_primitive("@");	/* Wince */
    int BangNo = get_primitive("!");
    int SwapNo = get_primitive("swap");
    int RotNo = get_primitive("rot");
    int NotNo = get_primitive("not");
    int StrcmpNo = get_primitive("strcmp");
    int EqualsNo = get_primitive("=");
    int PlusNo = get_primitive("+");
    int MinusNo = get_primitive("-");
    int MultNo = get_primitive("*");
    int DivNo = get_primitive("/");
    int ModNo = get_primitive("%");
    int DecrNo = get_primitive("--");
    int IncrNo = get_primitive("++");
    int NotequalsNo = get_primitive("!=");

    /* Code assumes everything is setup nicely, if not, bad things will happen */

    if (!cstat->first_word)
	return 0;

    /* renumber the instruction chain */
    for (curr = cstat->first_word; curr; curr = curr->next)
	curr->no = count++;

    if ((Flags = (int *) malloc(sizeof(int) * count)) == 0)
	return 0;

    memset(Flags, 0, sizeof(int) * count);

    /* Mark instructions which jumps reference */

    for (curr = cstat->first_word; curr; curr = curr->next) {
	switch (curr->in.type) {
	case PROG_ADD:
	case PROG_IF:
	case PROG_TRY:
	case PROG_JMP:
	case PROG_EXEC:
	    i = cstat->addrlist[curr->in.data.number]->no +
		    cstat->addroffsets[curr->in.data.number];
	    Flags[i] |= IMMFLAG_REFERENCED;
	    break;
	}
    }

    for (curr = cstat->first_word; curr;) {
	int advance = 1;
	switch (curr->in.type) {
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
	    /* "" strcmp 0 =  ==>  not */
	    if (IntermediateIsString(curr, "")) {
		if (ContiguousIntermediates(Flags, curr->next, 3)) {
		    if (IntermediateIsPrimitive(curr->next, StrcmpNo)) {
			if (IntermediateIsInteger(curr->next->next, 0)) {
			    if (IntermediateIsPrimitive(curr->next->next->next, EqualsNo)) {
				if (curr->in.data.string)
				    free((void *) curr->in.data.string);
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
				    compiler_warning(cstat,
						     "Warning on line %i: Divide by zero",
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

		    /* Int Int %  ==>  Div  */
		    if (IntermediateIsPrimitive(curr->next->next, ModNo)) {
			if (curr->next->in.data.number == 0) {
			    if (!(curr->next->next->flags & INTMEDFLG_MODBYZERO)) {
				curr->next->next->flags |= INTMEDFLG_MODBYZERO;

				if (force_err_display) {
				    compiler_warning(cstat,
						     "Warning on line %i: Modulus by zero",
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

    for (curr = cstat->first_word; curr; curr = curr->next)
	if (curr->in.type == PROG_SVAR_AT || curr->in.type == PROG_LVAR_AT)
	    MaybeOptimizeVarsAt(cstat, curr, AtNo, BangNo);

    free(Flags);
    return (old_instr_count - cstat->nowords);
}

static void
set_start(COMPSTATE * cstat)
{
    PROGRAM_SET_SIZ(cstat->program, cstat->nowords);

    /* address instr no is resolved before this gets called. */
    PROGRAM_SET_START(cstat->program, (PROGRAM_CODE(cstat->program) + cstat->procs->code->no));
}

/* Genericized Optimizer ideas:
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

/* allocate an address */
static struct prog_addr *
alloc_addr(COMPSTATE * cstat, int offset, struct inst *codestart)
{
    struct prog_addr *nu;

    nu = (struct prog_addr *) malloc(sizeof(struct prog_addr));

    nu->links = 1;
    nu->progref = cstat->program;
    nu->data = codestart + offset;
    return nu;
}

/* copy program to an array */
static void
copy_program(COMPSTATE * cstat)
{
    /*
     * Everything should be peachy keen now, so we don't do any error checking
     */
    struct INTERMEDIATE *curr;
    struct inst *code;
    int i, varcnt;

    if (!cstat->first_word)
	v_abort_compile(cstat, "Nothing to compile.");

    code = (struct inst *) malloc(sizeof(struct inst) * (cstat->nowords + 1));

    i = 0;
    for (curr = cstat->first_word; curr; curr = curr->next) {
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
	    code[i].data.mufproc =
		    (struct muf_proc_data *) malloc(sizeof(struct muf_proc_data));
	    code[i].data.mufproc->procname = strdup(curr->in.data.mufproc->procname);
	    code[i].data.mufproc->vars = varcnt = curr->in.data.mufproc->vars;
	    code[i].data.mufproc->args = curr->in.data.mufproc->args;
	    if (varcnt) {
		if (curr->in.data.mufproc->varnames) {
		    code[i].data.mufproc->varnames =
			    (const char **) calloc(varcnt, sizeof(char *));
		    for (int j = 0; j < varcnt; j++) {
			code[i].data.mufproc->varnames[j] =
				strdup(curr->in.data.mufproc->varnames[j]);
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

/* allocate and initialize data linked structure. */
static struct INTERMEDIATE *
alloc_inst(void)
{
    struct INTERMEDIATE *nu;

    nu = (struct INTERMEDIATE *) malloc(sizeof(struct INTERMEDIATE));

    nu->next = 0;
    nu->no = 0;
    nu->line = 0;
    nu->flags = 0;
    nu->in.type = 0;
    nu->in.line = 0;
    nu->in.data.number = 0;
    return nu;
}

static struct INTERMEDIATE *
new_inst(COMPSTATE * cstat)
{
    struct INTERMEDIATE *nu;

    nu = cstat->nextinst;
    if (!nu) {
	nu = alloc_inst();
    }
    cstat->nextinst = nu->next;
    nu->next = NULL;

    nu->flags |= (cstat->nested_trys > 0) ? INTMEDFLG_INTRY : 0;

    return nu;
}

/* return primitive word. */
static struct INTERMEDIATE *
primitive_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu, *cur;
    int pnum;

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

/* return self pushing word (string) */
static struct INTERMEDIATE *
string_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_STRING;
    nu->in.line = cstat->lineno;
    nu->in.data.string = alloc_prog_string(token);
    return nu;
}

/* return self pushing word (float) */
static struct INTERMEDIATE *
float_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_FLOAT;
    nu->in.line = cstat->lineno;
    sscanf(token, "%lg", &(nu->in.data.fnumber));
    return nu;
}

/* return self pushing word (number) */
static struct INTERMEDIATE *
number_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_INTEGER;
    nu->in.line = cstat->lineno;
    nu->in.data.number = atoi(token);
    return nu;
}

/* do a subroutine call --- push address onto stack, then make a primitive
   CALL.
   */
static struct INTERMEDIATE *
call_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    struct PROC_LIST *p;

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_EXEC;
    nu->in.line = cstat->lineno;
    for (p = cstat->procs; p; p = p->next)
	if (!strcasecmp(p->name, token))
	    break;

    if (p) nu->in.data.number = get_address(cstat, p->code, 0);
    return nu;
}

static struct INTERMEDIATE *
quoted_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    struct PROC_LIST *p;

    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_ADD;
    nu->in.line = cstat->lineno;
    for (p = cstat->procs; p; p = p->next)
	if (!strcasecmp(p->name, token))
	    break;

    if (p) nu->in.data.number = get_address(cstat, p->code, 0);
    return nu;
}

/* returns number corresponding to variable number.
   We assume that it DOES exist */
static struct INTERMEDIATE *
var_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int var_no = 0;

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

static struct INTERMEDIATE *
svar_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int var_no = 0;

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

static struct INTERMEDIATE *
lvar_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int var_no = 0;

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

/* check if object is in database before putting it in */
static struct INTERMEDIATE *
object_word(COMPSTATE * cstat, const char *token)
{
    struct INTERMEDIATE *nu;
    int objno;

    objno = atol(token + 1);
    nu = new_inst(cstat);
    nu->no = cstat->nowords++;
    nu->in.type = PROG_OBJECT;
    nu->in.line = cstat->lineno;
    nu->in.data.objref = objno;
    return nu;
}

/* predicates for procedure calls */
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

/* see if procedure call */
static int
call(COMPSTATE * cstat, const char *token)
{
    struct PROC_LIST *i;

    for (i = cstat->procs; i; i = i->next)
	if (!strcasecmp(i->name, token))
	    return 1;

    return 0;
}

/* see if it's a quoted procedure name */
static int
quoted(COMPSTATE * cstat, const char *token)
{
    return (*token == '\'' && call(cstat, token + 1));
}

/* see if it's an object # */
static int
object(const char *token)
{
    if (*token == NUMBER_TOKEN && number(token + 1))
	return 1;
    else
	return 0;
}

/* see if string */
static int
string(const char *token)
{
    return (token[0] == BEGINSTRING);
}

static int
variable(COMPSTATE * cstat, const char *token)
{
    for (int i = 0; i < MAX_VAR && cstat->variables[i]; i++)
	if (!strcasecmp(token, cstat->variables[i]))
	    return 1;

    return 0;
}

static int
scopedvar(COMPSTATE * cstat, const char *token)
{
    for (int i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++)
	if (!strcasecmp(token, cstat->scopedvars[i]))
	    return 1;

    return 0;
}

static int
localvar(COMPSTATE * cstat, const char *token)
{
    for (int i = 0; i < MAX_VAR && cstat->localvars[i]; i++)
	if (!strcasecmp(token, cstat->localvars[i]))
	    return 1;

    return 0;
}

/* see if token is primitive */
int
primitive(const char *token)
{
    int primnum;

    primnum = get_primitive(token);
    return (primnum && primnum <= prim_count - 3); /* don't match internal prims */
}

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

/* overall control code.  Does piece-meal tokenization parsing and
   backward checking.                                            */
void
do_compile(int descr, dbref player_in, dbref program_in, int force_err_display)
{
    const char *token;
    struct INTERMEDIATE *new_word;
    COMPSTATE cstat;

    /* set all compile state variables */
    cstat.force_err_display = force_err_display;
    cstat.descr = descr;
    cstat.control_stack = 0;
    cstat.procs = 0;
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
	cstat.variabletypes[i] = 0;
	cstat.localvars[i] = NULL;
	cstat.localvartypes[i] = 0;
	cstat.scopedvars[i] = NULL;
	cstat.scopedvartypes[i] = 0;
    }
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
    cstat.variabletypes[0] = PROG_OBJECT;
    cstat.variables[1] = "LOC";
    cstat.variabletypes[1] = PROG_OBJECT;
    cstat.variables[2] = "TRIGGER";
    cstat.variabletypes[2] = PROG_OBJECT;
    cstat.variables[3] = "COMMAND";
    cstat.variabletypes[3] = PROG_STRING;

    /* free old stuff */
    (void) dequeue_prog(cstat.program, 1);
    free_prog(cstat.program);
    cleanpubs(PROGRAM_PUBS(cstat.program));
    PROGRAM_SET_PUBS(cstat.program, NULL);
#ifdef MCP_SUPPORT
    clean_mcpbinds(PROGRAM_MCPBINDS(cstat.program));
    PROGRAM_SET_MCPBINDS(cstat.program, NULL);
#endif
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

    /* restart AUTOSTART program. */
    if ((FLAGS(cstat.program) & ABODE) && TrueWizard(OWNER(cstat.program)))
	add_muf_queue_event(-1, OWNER(cstat.program), NOTHING, NOTHING,
			    cstat.program, "Startup", "Queued Event.", 0);

    if (force_err_display)
	notify_nolisten(cstat.player, "Program compiled successfully.", 1);
}

/* Little routine to do the line_copy handling right */
static void
advance_line(COMPSTATE * cstat)
{
    cstat->curr_line = cstat->curr_line->next;
    cstat->lineno++;
    cstat->macrosubs = 0;
    if (cstat->line_copy) {
	free((void *) cstat->line_copy);
	cstat->line_copy = NULL;
    }
    if (cstat->curr_line)
	cstat->next_char = (cstat->line_copy = alloc_string(cstat->curr_line->this_line));
    else
	cstat->next_char = (cstat->line_copy = NULL);
}


/* return string */
static const char *
do_string(COMPSTATE * cstat)
{
    static char buf[BUFFER_LEN];
    int i = 0, quoted = 0;

    buf[i] = *cstat->next_char;
    cstat->next_char++;
    i++;
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

/* Old-style comment parser */
static int
do_old_comment(COMPSTATE * cstat)
{
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

/* skip comments, recursive style */
static int
do_new_comment(COMPSTATE * cstat, int depth)
{
    int retval = 0;
    int in_str = 0;
    const char *ptr;

    if (!*cstat->next_char || *cstat->next_char != BEGINCOMMENT)
	return 2;
    if (depth >= 7 /*arbitrary */ )
	return 3;
    cstat->next_char++;		/* Advance past BEGINCOMMENT */

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

    cstat->next_char++;		/* Advance past ENDCOMMENT */
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
	compiler_warning(cstat,
			 "Warning on line %i: Unterminated string may indicate unterminated comment. Comment starts on line %i.",
			 cstat->lineno, cstat->start_comment);
    }
    if (!(*cstat->next_char))
	advance_line(cstat);
    if (depth && !cstat->curr_line)	/* EOF? Don't care if done (depth==0) */
	return 1;
    return 0;
}

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

/* skip comments */
static void
do_comment(COMPSTATE * cstat, int depth)
{
    unsigned int next_char = 0;	/* Save state if needed. */
    int lineno = 0;
    struct line *curr_line = NULL;
    int macrosubs = 0;
    int retval = 0;

    if (!depth && !cstat->force_comment) {
	next_char = cstat->line_copy ? cstat->next_char - cstat->line_copy : 0;
	macrosubs = cstat->macrosubs;
	lineno = cstat->lineno;
	curr_line = cstat->curr_line;
    }

    if (!depth) {
	if ((retval = do_new_comment(cstat, 0))) {
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
					alloc_string(cstat->curr_line->this_line)) + next_char;
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

/* Skips comments, grabs strings, returns NULL when no more tokens to grab. */
static const char *
next_token_raw(COMPSTATE * cstat)
{
    static char buf[BUFFER_LEN];
    int i;

    if (!cstat->curr_line)
	return (char *) 0;

    if (!cstat->next_char)
	return (char *) 0;

    /* skip white space */
    while (*cstat->next_char && isspace(*cstat->next_char))
	cstat->next_char++;

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
static void
do_directive(COMPSTATE * cstat, char *direct)
{
    char temp[BUFFER_LEN];
    char *tmpname = NULL;
    char *tmpptr = NULL;
    int i = 0;
    int j;

    strcpyn(temp, sizeof(temp), ++direct);

    if (!(temp[0])) {
	v_abort_compile(cstat, "I don't understand that compiler directive!");
    }
    if (!strcasecmp(temp, "define")) {
	tmpname = (char *) next_token_raw(cstat);
	if (!tmpname)
	    v_abort_compile(cstat, "Unexpected end of file looking for $define name.");
	i = 0;
	while ((tmpptr = (char *) next_token_raw(cstat)) &&
	       (strcasecmp(tmpptr, "$enddef"))) {
	    char *cp;

	    for (cp = tmpptr; i < (BUFFER_LEN / 2) && *cp;) {
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
	if (!tmpptr)
	    v_abort_compile(cstat, "Unexpected end of file in $define definition.");
	free(tmpptr);
	(void) insert_def(cstat, tmpname, temp);
	free(tmpname);

    } else if (!strcasecmp(temp, "cleardefs")) {
	char nextToken[BUFFER_LEN];

	purge_defs(cstat);	/* Get rid of all defs first. */
	include_internal_defs(cstat);	/* Always include internal defs. */
	while (*cstat->next_char && isspace(*cstat->next_char))
	    cstat->next_char++;	/* eating leading spaces */
	strcpyn(nextToken, sizeof(nextToken), cstat->next_char);
	tmpname = nextToken;
	while (*cstat->next_char)
	    cstat->next_char++;
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

	if (strcasecmp(tmpname, ":") &&
	    (index(tmpname, '/') ||
	     index(tmpname, ':') ||
	     Prop_SeeOnly(tmpname) || Prop_Hidden(tmpname) || Prop_System(tmpname))) {
	    free(tmpname);
	    v_abort_compile(cstat, "Invalid $pubdef name.  No /, :, @ nor ~ are allowed.");
	} else {
	    if (!strcasecmp(tmpname, ":")) {
		remove_property(cstat->program, "/_defs", 0);
	    } else {
		const char *defstr = NULL;
		char propname[BUFFER_LEN];
		int doitset = 1;

		while (*cstat->next_char && isspace(*cstat->next_char))
		    cstat->next_char++;	/* eating leading spaces */
		defstr = cstat->next_char;

		if (*tmpname == BEGINESCAPE) {
		    char *temppropstr = NULL;

		    (void) *tmpname++;
		    snprintf(propname, sizeof(propname), "/_defs/%s", tmpname);
		    temppropstr = (char *) get_property_class(cstat->program, propname);
		    if (temppropstr) {
			doitset = 0;
		    }
		} else {
		    snprintf(propname, sizeof(propname), "/_defs/%s", tmpname);
		}

		if (doitset) {
		    if (defstr != NULL && *defstr) {
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

	if (index(tmpname, '/') ||
	    index(tmpname, ':') ||
	    Prop_SeeOnly(tmpname) || Prop_Hidden(tmpname) || Prop_System(tmpname)) {
	    free(tmpname);
	    v_abort_compile(cstat, "Invalid $libdef name.  No /, :, @, nor ~ are allowed.");
	} else {
	    char propname[BUFFER_LEN];
	    char defstr[BUFFER_LEN];
	    int doitset = 1;

	    while (*cstat->next_char && isspace(*cstat->next_char))
		cstat->next_char++;	/* eating leading spaces */

	    if (*tmpname == BEGINESCAPE) {
		char *temppropstr = NULL;

		(void) *tmpname++;
		snprintf(propname, sizeof(propname), "/_defs/%s", tmpname);
		temppropstr = (char *) get_property_class(cstat->program, propname);
		if (temppropstr) {
		    doitset = 0;
		}
	    } else {
		snprintf(propname, sizeof(propname), "/_defs/%s", tmpname);
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
	while (*cstat->next_char && isspace(*cstat->next_char))
	    cstat->next_char++;	/* eating leading spaces */
	tmpname = (char *) cstat->next_char;
	if (tmpname && *tmpname) {
	    v_abort_compile(cstat, tmpname);
	} else {
	    v_abort_compile(cstat, "Forced abort for the compile.");
	}

    } else if (!strcasecmp(temp, "version")) {
	tmpname = (char *) next_token_raw(cstat);
	if (!ifloat(tmpname))
	    v_abort_compile(cstat, "Expected a floating point number for the version.");
	add_property(cstat->program, MUF_VERSION_PROP, tmpname, 0);
	while (*cstat->next_char)
	    cstat->next_char++;
	advance_line(cstat);
	free(tmpname);

    } else if (!strcasecmp(temp, "lib-version")) {
	tmpname = (char *) next_token_raw(cstat);
	if (!ifloat(tmpname))
	    v_abort_compile(cstat, "Expected a floating point number for the version.");
	add_property(cstat->program, MUF_LIB_VERSION_PROP, tmpname, 0);
	while (*cstat->next_char)
	    cstat->next_char++;
	advance_line(cstat);
	free(tmpname);

    } else if (!strcasecmp(temp, "author")) {
	while (*cstat->next_char && isspace(*cstat->next_char))
	    cstat->next_char++;	/* eating leading spaces */
	tmpname = (char *) cstat->next_char;
	while (*cstat->next_char)
	    cstat->next_char++;
	add_property(cstat->program, MUF_AUTHOR_PROP, tmpname, 0);
	advance_line(cstat);

    } else if (!strcasecmp(temp, "note")) {
	while (*cstat->next_char && isspace(*cstat->next_char))
	    cstat->next_char++;	/* eating leading spaces */
	tmpname = (char *) cstat->next_char;
	while (*cstat->next_char)
	    cstat->next_char++;
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
	    if (tmpptr)
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
			    "I don't understand what program you want to check in ifcancall.");
	tmpname = (char *) next_token_raw(cstat);
	if (!tmpname || !*tmpname) {
	    if (tmpptr) {
		free(tmpptr);
	    }
	    v_abort_compile(cstat, "I don't understand what function you want to check for.");
	}
	while (*cstat->next_char)
	    cstat->next_char++;
	advance_line(cstat);
	if (!PROGRAM_CODE(i)) {
	    struct line *tmpline;

	    tmpline = PROGRAM_FIRST(i);
	    PROGRAM_SET_FIRST(i, ((struct line *) read_program(i)));
	    do_compile(cstat->descr, OWNER(i), i, 0);
	    free_prog_text(PROGRAM_FIRST(i));
	    PROGRAM_SET_FIRST(i, tmpline);
	}
	j = 0;
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
	    tmpptr = (char *) malloc(4 * sizeof(char));
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
	match_me(&md);
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
	/* TODO - move pragmas to its own section for easy expansion. */
	while (*cstat->next_char && isspace(*cstat->next_char))
	    cstat->next_char++;
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
	    compiler_warning(cstat,
			     "Warning on line %i: Pragma %.64s unrecognized.  Ignoring.",
			     cstat->lineno, tmpptr);
	    while (*cstat->next_char)
		cstat->next_char++;
	}
	free(tmpptr);
	if (*cstat->next_char) {
	    compiler_warning(cstat,
			     "Warning on line %i: Ignoring extra pragma arguments: %.256s",
			     cstat->lineno, cstat->next_char);
	    advance_line(cstat);
	}
    } else {
	v_abort_compile(cstat, "Unrecognized compiler directive.");
    }
}

static const char *
next_token(COMPSTATE * cstat)
{
    char *expansion, *temp;
    int elen = 0;

    temp = (char *) next_token_raw(cstat);
    if (!temp)
	return NULL;

    if (temp[0] == BEGINDIRECTIVE) {
	do_directive(cstat, temp);
	free(temp);
	return next_token(cstat);
    }
    if (temp[0] == BEGINESCAPE) {
	if (temp[1]) {
	    expansion = temp;
	    elen = strlen(expansion);
	    temp = (char *) malloc(elen);
	    strcpyn(temp, elen, (expansion + 1));
	    free(expansion);
	}
	return (temp);
    }
    if ((expansion = expand_def(cstat, temp))) {
	free(temp);
	if (++cstat->macrosubs > SUBSTITUTIONS) {
	    abort_compile(cstat, "Too many macro substitutions.");
	} else {
	    int templen = strlen(cstat->next_char) + strlen(expansion) + 21;
	    temp = (char *) malloc(templen);
	    strcpyn(temp, templen, expansion);
	    strcatn(temp, templen, cstat->next_char);
	    free((void *) expansion);
	    if (cstat->line_copy) {
		free((void *) cstat->line_copy);
	    }
	    cstat->next_char = cstat->line_copy = temp;
	    return next_token(cstat);
	}
    } else {
	return (temp);
    }
}

/* adds variable.  Return 0 if no space left */
static int
add_variable(COMPSTATE * cstat, const char *varname, int valtype)
{
    int i;

    for (i = RES_VAR; i < MAX_VAR; i++)
	if (!cstat->variables[i])
	    break;

    if (i == MAX_VAR)
	return 0;

    cstat->variables[i] = alloc_string(varname);
    cstat->variabletypes[i] = valtype;
    return i;
}


/* adds local variable.  Return 0 if no space left */
static int
add_scopedvar(COMPSTATE * cstat, const char *varname, int valtype)
{
    int i;

    for (i = 0; i < MAX_VAR; i++)
	if (!cstat->scopedvars[i])
	    break;

    if (i == MAX_VAR)
	return -1;

    cstat->scopedvars[i] = alloc_string(varname);
    cstat->scopedvartypes[i] = valtype;
    return i;
}


/* adds local variable.  Return 0 if no space left */
static int
add_localvar(COMPSTATE * cstat, const char *varname, int valtype)
{
    int i;

    for (i = 0; i < MAX_VAR; i++)
	if (!cstat->localvars[i])
	    break;

    if (i == MAX_VAR)
	return -1;

    cstat->localvars[i] = alloc_string(varname);
    cstat->localvartypes[i] = valtype;
    return i;
}

/* pops first while off the innermost control structure, if it's a loop. */
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
    free((void *) tofree);
    return temp;
}

/* checks if topmost loop stack item is a for */
static struct INTERMEDIATE *
innermost_control_place(COMPSTATE * cstat, int type1)
{
    struct CONTROL_STACK *ctrl;

    ctrl = cstat->control_stack;
    if (!ctrl)
	return 0;
    if (ctrl->type != type1)
	return 0;

    return ctrl->place;
}

static void
resolve_loop_addrs(COMPSTATE * cstat, int where)
{
    struct INTERMEDIATE *eef;

    while ((eef = pop_loop_exit(cstat)))
	eef->in.data.number = where;
    eef = innermost_control_place(cstat, CTYPE_FOR);
    if (eef) {
	eef->next->in.data.number = where;
    }
}

/* support routines for internal data structures. */

/* add procedure to procedures list */
static void
add_proc(COMPSTATE * cstat, const char *proc_name, struct INTERMEDIATE *place, int rettype)
{
    struct PROC_LIST *nu;

    nu = (struct PROC_LIST *) malloc(sizeof(struct PROC_LIST));

    nu->name = alloc_string(proc_name);
    nu->returntype = rettype;
    nu->code = place;
    nu->next = cstat->procs;
    cstat->procs = nu;
}

/* add if to control stack */
static void
add_control_structure(COMPSTATE * cstat, int typ, struct INTERMEDIATE *place)
{
    struct CONTROL_STACK *nu;

    nu = (struct CONTROL_STACK *) malloc(sizeof(struct CONTROL_STACK));

    nu->place = place;
    nu->type = typ;
    nu->next = cstat->control_stack;
    nu->extra = 0;
    cstat->control_stack = nu;
}

/* add while to current loop's list of exits remaining to be resolved. */
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

    nu = (struct CONTROL_STACK *) malloc(sizeof(struct CONTROL_STACK));

    nu->place = place;
    nu->type = CTYPE_WHILE;
    nu->next = 0;
    nu->extra = loop->extra;
    loop->extra = nu;
}

/* Returns true if a loop start is in the control structure stack. */
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

/* Returns the type of the innermost nested control structure. */
static int
innermost_control_type(COMPSTATE * cstat)
{
    struct CONTROL_STACK *ctrl;

    ctrl = cstat->control_stack;
    if (!ctrl)
	return 0;

    return ctrl->type;
}

/* Returns number of TRYs before topmost Loop */
static int
count_trys_inside_loop(COMPSTATE * cstat)
{
    struct CONTROL_STACK *loop;
    int count = 0;

    loop = cstat->control_stack;

    while (loop) {
	if (loop->type == CTYPE_FOR || loop->type == CTYPE_BEGIN) {
	    break;
	}
	if (loop->type == CTYPE_TRY) {
	    count++;
	}
	loop = loop->next;
    }

    return count;
}

/* returns topmost begin or for off the stack */
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

/* Pops off the innermost control structure and returns the place. */
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

static struct INTERMEDIATE *
prealloc_inst(COMPSTATE * cstat)
{
    struct INTERMEDIATE *ptr;
    struct INTERMEDIATE *nu;

    /* only allocate at most one extra instr */
    if (cstat->nextinst)
	return NULL;

    nu = alloc_inst();

    nu->flags |= (cstat->nested_trys > 0) ? INTMEDFLG_INTRY : 0;

    if (!cstat->nextinst) {
	cstat->nextinst = nu;
    } else {
	for (ptr = cstat->nextinst; ptr->next; ptr = ptr->next) ;
	ptr->next = nu;
    }

    nu->no = cstat->nowords;

    return nu;
}

/* process special.  Performs special processing.
   It sets up FOR and IF structures.  Remember --- for those,
   we've got to set aside an extra argument space.         */
static struct INTERMEDIATE *
process_special(COMPSTATE * cstat, const char *token)
{
    static char buf[BUFFER_LEN];
    const char *tok;
    struct INTERMEDIATE *nu;

    if (!strcasecmp(token, ":")) {
	const char *proc_name;
	int argsflag = 0;

	if (cstat->curr_proc)
	    abort_compile(cstat, "Definition within definition.");
	proc_name = next_token(cstat);
	if (!proc_name)
	    abort_compile(cstat, "Unexpected end of file within procedure.");

	strcpyn(buf, sizeof(buf), proc_name);
	if (proc_name)
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
	nu->in.data.mufproc = (struct muf_proc_data *) malloc(sizeof(struct muf_proc_data));
	nu->in.data.mufproc->procname = strdup(proc_name);
	nu->in.data.mufproc->vars = 0;
	nu->in.data.mufproc->args = 0;
	nu->in.data.mufproc->varnames = NULL;

	cstat->curr_proc = nu;

	if (argsflag) {
	    const char *varspec;
	    const char *varname;
	    int argsdone = 0;
	    int outflag = 0;

	    do {
		varspec = next_token(cstat);
		if (!varspec)
		    abort_compile(cstat,
				  "Unexpected end of file within procedure arguments declaration.");

		if (!strcmp(varspec, "]")) {
		    argsdone = 1;
		} else if (!strcmp(varspec, "--")) {
		    outflag = 1;
		} else if (!outflag) {
		    varname = index(varspec, ':');
		    if (varname) {
			varname++;
		    } else {
			varname = varspec;
		    }
		    if (*varname) {
			if (add_scopedvar(cstat, varname, PROG_UNTYPED) < 0)
			    abort_compile(cstat, "Variable limit exceeded.");

			nu->in.data.mufproc->vars++;
			nu->in.data.mufproc->args++;
		    }
		}
		if (varspec) {
		    free((void *) varspec);
		}
	    } while (!argsdone);
	}

	add_proc(cstat, proc_name, nu, PROG_UNTYPED);

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
	    cstat->curr_proc->in.data.mufproc->varnames =
		    (const char **) calloc(varcnt, sizeof(char *));
	    for (int i = 0; i < varcnt; i++) {
		cstat->curr_proc->in.data.mufproc->varnames[i] = cstat->scopedvars[i];
		cstat->scopedvars[i] = 0;
	    }
	}
	cstat->curr_proc = 0;
	return nu;
    } else if (!strcasecmp(token, "IF")) {
	nu = new_inst(cstat);
	nu->no = cstat->nowords++;
	nu->in.type = PROG_IF;
	nu->in.line = cstat->lineno;
	nu->in.data.call = 0;
	add_control_structure(cstat, CTYPE_IF, nu);
	return nu;
    } else if (!strcasecmp(token, "ELSE")) {
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
	prealloc_inst(cstat);
	add_control_structure(cstat, CTYPE_BEGIN, cstat->nextinst);
	return NULL;
    } else if (!strcasecmp(token, "FOR")) {
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
	nu = new_inst(cstat);
	nu->no = cstat->nowords++;
	nu->in.type = PROG_TRY;
	nu->in.line = cstat->lineno;
	nu->in.data.number = 0;

	add_control_structure(cstat, CTYPE_TRY, nu);
	cstat->nested_trys++;

	return nu;
    } else if (!strcasecmp(token, "CATCH") || !strcasecmp(token, "CATCH_DETAILED")) {
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
	nu = new_inst(cstat);
	nu->no = cstat->nowords++;
	nu->in.type = PROG_PRIMITIVE;
	nu->in.line = cstat->lineno;
	nu->in.data.number = IN_CALL;
	return nu;
    } else if (!strcasecmp(token, "WIZCALL") || !strcasecmp(token, "PUBLIC")) {
	struct PROC_LIST *p;
	struct publics *pub;
	int wizflag = 0;

	if (!strcasecmp(token, "WIZCALL"))
	    wizflag = 1;
	if (cstat->curr_proc)
	    abort_compile(cstat, "PUBLIC  or WIZCALL declaration within procedure.");
	tok = next_token(cstat);
	if ((!tok) || !call(cstat, tok))
	    abort_compile(cstat, "Subroutine unknown in PUBLIC or WIZCALL declaration.");
	for (p = cstat->procs; p; p = p->next)
	    if (!strcasecmp(p->name, tok))
		break;
	if (!p)
	    abort_compile(cstat, "Subroutine unknown in PUBLIC or WIZCALL declaration.");
	if (!cstat->currpubs) {
	    cstat->currpubs = (struct publics *) malloc(sizeof(struct publics));

	    cstat->currpubs->next = NULL;
	    cstat->currpubs->subname = (char *) strdup(tok);
	    if (tok)
		free((void *) tok);
	    cstat->currpubs->addr.no = get_address(cstat, p->code, 0);
	    cstat->currpubs->mlev = wizflag ? 4 : 1;
	    return 0;
	}
	for (pub = cstat->currpubs; pub;) {
	    if (!strcasecmp(tok, pub->subname)) {
		abort_compile(cstat, "Function already declared public.");
	    } else {
		if (pub->next) {
		    pub = pub->next;
		} else {
		    pub->next = (struct publics *) malloc(sizeof(struct publics));

		    pub = pub->next;
		    pub->next = NULL;
		    pub->subname = (char *) strdup(tok);
		    if (tok)
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
	    if (add_scopedvar(cstat, tok, PROG_UNTYPED) < 0)
		abort_compile(cstat, "Variable limit exceeded.");
	    if (tok)
		free((void *) tok);
	    cstat->curr_proc->in.data.mufproc->vars++;
	} else {
	    tok = next_token(cstat);
	    if (!tok)
		abort_compile(cstat, "Unexpected end of program.");
	    if (!add_variable(cstat, tok, PROG_UNTYPED))
		abort_compile(cstat, "Variable limit exceeded.");
	    if (tok)
		free((void *) tok);
	}
	return 0;
    } else if (!strcasecmp(token, "VAR!")) {
	if (cstat->curr_proc) {
	    struct INTERMEDIATE *nu;

	    tok = next_token(cstat);
	    if (!tok)
		abort_compile(cstat, "Unexpected end of program.");
	    if (add_scopedvar(cstat, tok, PROG_UNTYPED) < 0)
		abort_compile(cstat, "Variable limit exceeded.");
	    if (tok)
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
	if (!tok || (add_localvar(cstat, tok, PROG_UNTYPED) == -1))
	    abort_compile(cstat, "Local variable limit exceeded.");
	if (tok)
	    free((void *) tok);
	return 0;
    } else {
	snprintf(buf, sizeof(buf), "Unrecognized special form %s found. (%d)", token,
		 cstat->lineno);
	abort_compile(cstat, buf);
    }
}

/* return primitive instruction */
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

static void
append_intermediate_chain(struct INTERMEDIATE *chain, struct INTERMEDIATE *add)
{
    while (chain->next)
	chain = chain->next;
    chain->next = add;
}

static void
free_prog_real(dbref prog, const char *file, const int line)
{
    int i;
    struct inst *c = PROGRAM_CODE(prog);
    int siz = PROGRAM_SIZ(prog);

    if (c) {
	if (PROGRAM_INSTANCES(prog)) {
	    log_status("WARNING: freeing program %s with %d instances reported from %s:%d",
		       unparse_object(GOD, prog), PROGRAM_INSTANCES(prog), file, line);
	}
	i = scan_instances(prog);
	if (i) {
	    log_status("WARNING: freeing program %s with %d instances found from %s:%d",
		       unparse_object(GOD, prog), i, file, line);
	}
	for (i = 0; i < siz; i++) {
	    if (c[i].type == PROG_ADD) {
		if (c[i].data.addr->links != 1) {
		    log_status("WARNING: Freeing address in %s with link count %d from %s:%d",
			       unparse_object(GOD, prog), c[i].data.addr->links, file, line);
		}
		free(c[i].data.addr);
	    } else {
		CLEAR(c + i);
	    }
	}
	free((void *) c);
    }
    PROGRAM_SET_CODE(prog, 0);
    PROGRAM_SET_SIZ(prog, 0);
    PROGRAM_SET_START(prog, 0);
}

long
size_prog(dbref prog)
{
    struct inst *c;
    long varcnt, siz, byts = 0;

    c = PROGRAM_CODE(prog);
    if (!c)
	return 0;
    siz = PROGRAM_SIZ(prog);
    for (long i = 0L; i < siz; i++) {
	byts += sizeof(*c);
	if (c[i].type == PROG_FUNCTION) {
	    byts += strlen(c[i].data.mufproc->procname) + 1;
	    varcnt = c[i].data.mufproc->vars;
	    if (c[i].data.mufproc->varnames) {
		for (long j = 0; j < varcnt; j++) {
		    byts += strlen(c[i].data.mufproc->varnames[j]) + 1;
		}
		byts += sizeof(char **) * varcnt;
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

static void
add_primitive(int val)
{
    hash_data hd;

    hd.ival = val;
    if (add_hash(base_inst[val - 1], hd, primitive_list, COMP_HASH_SIZE) == NULL)
	panic("Out of memory");
    else
	return;
}

void
clear_primitives(void)
{
    kill_hash(primitive_list, COMP_HASH_SIZE, 0);
    return;
}

const char *base_inst[] = {
    "JMP", "READ", "SLEEP", "CALL", "EXECUTE", "EXIT", "EVENT_WAITFOR", "CATCH", "CATCH_DETAILED",
    PRIMS_CONNECTS_NAMES, PRIMS_DB_NAMES, PRIMS_MATH_NAMES, PRIMS_MISC_NAMES, PRIMS_PROPS_NAMES, PRIMS_STACK_NAMES,
    PRIMS_STRINGS_NAMES, PRIMS_ARRAY_NAMES, PRIMS_FLOAT_NAMES, PRIMS_ERROR_NAMES,
#ifdef MCP_SUPPORT
    PRIMS_MCP_NAMES,
#endif
    PRIMS_REGEX_NAMES, PRIMS_INTERNAL_NAMES
};

void
init_primitives(void)
{
    clear_primitives();

    prim_count = ARRAYSIZE(base_inst);
    for (int i = 1; i <= prim_count; i++) {
	add_primitive(i);
    }
    IN_FORPOP = get_primitive(" FORPOP");
    IN_FORITER = get_primitive(" FORITER");
    IN_FOR = get_primitive(" FOR");
    IN_FOREACH = get_primitive(" FOREACH");
    IN_TRYPOP = get_primitive(" TRYPOP");
    log_status("MUF: %d primitives exist.", prim_count);
}
