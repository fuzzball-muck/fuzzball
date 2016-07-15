#ifndef _COMPILE_H
#define _COMPILE_H

#include "db.h"
#include "hashtab.h"
#include "inst.h"

/* Character defines */
#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'
#define BEGINDIRECTIVE '$'
#define BEGINESCAPE '\\'

#define SUBSTITUTIONS 20	/* How many nested macros will we allow? */

#define CTYPE_IF    1
#define CTYPE_ELSE  2
#define CTYPE_BEGIN 3
#define CTYPE_FOR   4
#define CTYPE_WHILE 5
#define CTYPE_TRY   6
#define CTYPE_CATCH 7

/* These would be constants, but their value isn't known until runtime. */
static int IN_FORITER;
static int IN_FOREACH;
static int IN_FORPOP;
static int IN_FOR;
static int IN_TRYPOP;

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
#define INTMEDFLG_INTRY		4

struct INTERMEDIATE {
    int no;			/* which number instruction this is */
    struct inst in;		/* instruction itself */
    short line;			/* line number of instruction */
    short flags;
    struct INTERMEDIATE *next;	/* next instruction */
};

/* The state structure for a compile. */
typedef struct COMPILE_STATE_T {
    struct CONTROL_STACK *control_stack;
    struct PROC_LIST *procs;

    int nowords;		/* number of words compiled */
    struct INTERMEDIATE *curr_word;	/* word being compiled */
    struct INTERMEDIATE *first_word;	/* first word of the list */
    struct INTERMEDIATE *curr_proc;	/* first word of curr. proc. */
    struct publics *currpubs;
    int nested_fors;
    int nested_trys;

    /* Address resolution data.  Used to relink addresses after compile. */
    struct INTERMEDIATE **addrlist;	/* list of addresses to resolve */
    int *addroffsets;		/* list of offsets from instrs */
    int addrmax;		/* size of current addrlist array */
    int addrcount;		/* number of allocated addresses */

    /* variable names.  The index into cstat->variables give you what position
     * the variable holds.
     */
    const char *variables[MAX_VAR];
    int variabletypes[MAX_VAR];
    const char *localvars[MAX_VAR];
    int localvartypes[MAX_VAR];
    const char *scopedvars[MAX_VAR];
    int scopedvartypes[MAX_VAR];

    struct line *curr_line;	/* current line */
    int lineno;			/* current line number */
    int start_comment;		/* Line last comment started at */
    int force_comment;		/* Only attempt certain compile. */
    const char *next_char;	/* next char * */
    dbref player, program;	/* player and program for this compile */

    int compile_err;		/* 1 if error occured */

    char *line_copy;
    int macrosubs;		/* Safeguard for macro-subst. infinite loops */
    int descr;			/* the descriptor that initiated compiling */
    int force_err_display;	/* If true, always show compiler errors. */
    struct INTERMEDIATE *nextinst;
    hash_tab defhash[DEFHASHSIZE];
} COMPSTATE;

#define free_prog(i) free_prog_real(i,__FILE__,__LINE__);

void clear_primitives(void);
void do_compile(int descr, dbref in_player, dbref in_program, int force_err_disp);
void free_unused_programs(void);
int get_primitive(const char *);
void init_primitives(void);
int primitive(const char *s);
long size_prog(dbref prog);
void uncompile_program(dbref i);

#endif				/* _COMPILE_H */
