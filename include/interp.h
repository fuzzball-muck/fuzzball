#ifndef _INTERP_H
#define _INTERP_H

#include "fbstrings.h"
#include "inst.h"

/* Some icky machine/compiler #defines. --jim */
#ifdef MIPS
typedef char *voidptr;

#define MIPSCAST (char *)
#else
typedef void *voidptr;

#define MIPSCAST
#endif

void RCLEAR(struct inst *oper, char *file, int line);

#define CLEAR(oper) RCLEAR(oper, __FILE__, __LINE__)
#define ProgMLevel(x) (find_mlev(x, fr, fr->caller.top))
#define ProgUID find_uid(player, fr, fr->caller.top, program)


typedef struct inst vars[MAX_VAR];

struct forvars {
    int didfirst;
    struct inst cur;
    struct inst end;
    int step;
    struct forvars *next;
};

struct tryvars {
    int depth;
    int call_level;
    int for_count;
    struct inst *addr;
    struct tryvars *next;
};

struct stack {
    int top;
    struct inst st[STACK_SIZE];
};

struct sysstack {
    int top;
    struct stack_addr st[STACK_SIZE];
};

struct callstack {
    int top;
    dbref st[STACK_SIZE];
};

struct localvars {
    struct localvars *next;
    struct localvars **prev;
    dbref prog;
    vars lvars;
};

struct forstack {
    int top;
    struct forvars *st;
};

struct trystack {
    int top;
    struct tryvars *st;
};

#define MAX_BREAKS 16
struct debuggerdata {
    unsigned debugging:1;	/* if set, this frame is being debugged */
    unsigned force_debugging:1;	/* if set, debugger is active, even if not set Z */
    unsigned bypass:1;		/* if set, bypass breakpoint on starting instr */
    unsigned isread:1;		/* if set, the prog is trying to do a read */
    unsigned showstack:1;	/* if set, show stack debug line, each inst. */
    unsigned dosyspop:1;	/* if set, fix up system stack before returning. */
    int lastlisted;		/* last listed line */
    char *lastcmd;		/* last executed debugger command */
    short breaknum;		/* the breakpoint that was just caught on */

    dbref lastproglisted;	/* What program's text was last loaded to list? */
    struct line *proglines;	/* The actual program text last loaded to list. */

    short count;		/* how many breakpoints are currently set */
    short temp[MAX_BREAKS];	/* is this a temp breakpoint? */
    short level[MAX_BREAKS];	/* level breakpnts.  If -1, no check. */
    struct inst *lastpc;	/* Last inst interped.  For inst changes. */
    struct inst *pc[MAX_BREAKS];	/* pc breakpoint.  If null, no check. */
    int pccount[MAX_BREAKS];	/* how many insts to interp.  -2 for inf. */
    int lastline;		/* Last line interped.  For line changes. */
    int line[MAX_BREAKS];	/* line breakpts.  -1 no check. */
    int linecount[MAX_BREAKS];	/* how many lines to interp.  -2 for inf. */
    dbref prog[MAX_BREAKS];	/* program that breakpoint is in. */
};

struct scopedvar_t {
    int count;
    const char **varnames;
    struct scopedvar_t *next;
    struct inst vars[1];
};

struct dlogidlist {
    struct dlogidlist *next;
    char dlogid[32];
};

struct mufwatchpidlist {
    struct mufwatchpidlist *next;
    int pid;
};

#define dequeue_prog(x,i) dequeue_prog_real(x,i,__FILE__,__LINE__)

#define STD_REGUID 0
#define STD_SETUID 1
#define STD_HARDUID 2

union error_mask {
    struct {
        unsigned int div_zero:1;        /* Divide by zero */
        unsigned int nan:1;     /* Result would not be a number */
        unsigned int imaginary:1;       /* Result would be imaginary */
        unsigned int f_bounds:1;        /* Float boundary error */
        unsigned int i_bounds:1;        /* Integer boundary error */
    } error_flags;
    int is_flags;
};

/* frame data structure necessary for executing programs */
struct frame {
    struct frame *next;
    struct sysstack system;	/* system stack */
    struct stack argument;	/* argument stack */
    struct callstack caller;	/* caller prog stack */
    struct forstack fors;	/* for loop stack */
    struct trystack trys;	/* try block stack */
    struct localvars *lvars;	/* local variables */
    vars variables;		/* global variables */
    struct inst *pc;		/* next executing instruction */
    short writeonly;		/* This program should not do reads */
    short multitask;		/* This program's multitasking mode */
    short timercount;		/* How many timers currently exist. */
    short level;		/* prevent interp call loops */
    int perms;			/* permissions restrictions on program */
    short already_created;	/* this prog already created an object */
    short been_background;	/* this prog has run in the background */
    short skip_declare;		/* tells interp to skip next scoped var decl */
    short wantsblanks;		/* specifies program will accept blank READs */
    dbref trig;			/* triggering object */
    dbref supplicant;		/* object for lock evaluation */
    struct shared_string *cmd;	/* Original command passed to the program, vars[3] */
    time_t started;		/* When this program started. */
    int instcnt;		/* How many instructions have run. */
    int pid;			/* what is the process id? */
    char *errorstr;		/* the error string thrown */
    char *errorinst;		/* the instruction name that threw an error */
    dbref errorprog;		/* the program that threw an error */
    int errorline;		/* the program line that threw an error */
    int descr;			/* what is the descriptor that started this? */
    void *rndbuf;		/* buffer for seedable random */
    struct scopedvar_t *svars;	/* Variables with function scoping. */
    struct debuggerdata brkpt;	/* info the debugger needs */
    struct timeval proftime;	/* profiling timing code */
    struct timeval totaltime;	/* profiling timing code */
    struct mufevent *events;	/* MUF event list. */
    struct dlogidlist *dlogids;	/* List of dlogids this frame uses. */
    struct mufwatchpidlist *waiters;
    struct mufwatchpidlist *waitees;
    union error_mask error;
    short pinning;		/* Whether new arrays/dicts should be pinned by default. */
#ifdef DEBUG
    int expect_pop;
    int actual_pop;
    int expect_push_to;
#endif
};

struct publics {
    char *subname;
    int mlev;
    union {
        struct inst *ptr;
        int no;
    } addr;
    struct publics *next;
};

#define abort_loop(S, C1, C2) \
{ \
	do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
	if (fr && fr->trys.top) \
		break; \
	else \
		return 0; \
}

#define abort_loop_hard(S, C1, C2) \
{ \
	int tmptop = 0; \
	if (fr) { tmptop = fr->trys.top; fr->trys.top = 0; } \
	do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
	if (fr) fr->trys.top = tmptop; \
	return 0; \
}

#ifdef DEBUG
#define POP() (++fr->actual_pop, arg + --(*top))
#else
#define POP() (arg + --(*top))
#endif

#define abort_interp(C) \
{ \
  do_abort_interp(player, (C), pc, arg, *top, fr, oper1, oper2, oper3, oper4, \
                  nargs, program, __FILE__, __LINE__); \
  return; \
}

#define EXPECT_READ_STACK(N) \
do { \
    int depth = (N); \
    if (*top < depth) { \
        abort_interp("Stack underflow."); \
    } \
} while (0)

#define EXPECT_WRITE_STACK(N) \
do { \
    int depth = (N); \
    if (*top < depth) { \
        abort_interp("Stack underflow."); \
    } \
    if (fr->trys.top && *top - fr->trys.st->depth < depth)  { \
        abort_interp("Stack protection fault."); \
    } \
} while (0)

#ifdef DEBUG
#define EXPECT_POP_STACK(N) \
{ \
    EXPECT_WRITE_STACK(N); \
    fr->expect_pop += (N); \
}
#else
#define EXPECT_POP_STACK(N) \
{ \
    EXPECT_WRITE_STACK(N); \
}
#endif


#define CHECKOP_READONLY(N) \
{ \
    nargs = (0); \
    EXPECT_READ_STACK(N); \
    nargs = (N); \
}

#define CHECKOP(N) \
{ \
    nargs = (0); \
    EXPECT_POP_STACK(N); \
    nargs = (N); \
}

#define CHECKREMOTE(x) if ((mlev < 2) && ((x) != HOME) && \
                           (LOCATION(x) != player) &&  \
                           (LOCATION(x) != LOCATION(player)) && \
                           ((x) != LOCATION(player)) && ((x) != player) \
			   && !controls(ProgUID, x)) \
                 abort_interp("Mucker Level 2 required to get remote info.");

#define PushObject(x)   push(arg, top, PROG_OBJECT, MIPSCAST &x)
#define PushInt(x)      push(arg, top, PROG_INTEGER, MIPSCAST &x)
#define PushFloat(x)    push(arg, top, PROG_FLOAT, MIPSCAST &x)
#define PushLock(x)     push(arg, top, PROG_LOCK, MIPSCAST copy_bool(x))

#define PushMark()      push(arg, top, PROG_MARK, MIPSCAST 0)
#define PushStrRaw(x)   push(arg, top, PROG_STRING, MIPSCAST x)
#define PushString(x)   PushStrRaw(alloc_prog_string(x))
#define PushNullStr     PushStrRaw(0)

#define PushArrayRaw(x) push(arg, top, PROG_ARRAY, MIPSCAST x)
#define PushInst(x)	copyinst(x, &arg[((*top)++)])

#ifdef DEBUG
#define CHECKOFLOW(x) do { \
        if((*top + (x - 1)) >= STACK_SIZE) \
            abort_interp("Stack Overflow!"); \
        fr->expect_push_to = *top + (x); \
    } while (0)
#else
#define CHECKOFLOW(x) do { \
        if((*top + (x - 1)) >= STACK_SIZE) \
            abort_interp("Stack Overflow!"); \
    } while (0)
#endif

#define PRIM_PROTOTYPE dbref player, dbref program, int mlev, \
                       struct inst *pc, struct inst *arg, int *top, \
                       struct frame *fr

#define SORTTYPE_CASEINSENS     0x1
#define SORTTYPE_DESCENDING     0x2

#define SORTTYPE_CASE_ASCEND    0
#define SORTTYPE_NOCASE_ASCEND  (SORTTYPE_CASEINSENS)
#define SORTTYPE_CASE_DESCEND   (SORTTYPE_DESCENDING)
#define SORTTYPE_NOCASE_DESCEND (SORTTYPE_CASEINSENS | SORTTYPE_DESCENDING)
#define SORTTYPE_SHUFFLE        4

extern int top_pid;
extern int nargs;
extern int prim_count;
extern const char *base_inst[];

struct forvars *copy_fors(struct forvars *);
struct tryvars *copy_trys(struct tryvars *);
void copyinst(struct inst *from, struct inst *to);
void deep_copyinst(struct inst *from, struct inst *to, int pinned);
char *debug_inst(struct frame *, int, struct inst *, int, struct inst *, char *, size_t,
                        int, dbref);
void do_abort_interp(dbref player, const char *msg, struct inst *pc,
			    struct inst *arg, int atop, struct frame *fr,
			    struct inst *oper1, struct inst *oper2, struct inst *oper3,
			    struct inst *oper4, int nargs, dbref program, char *file,
			    int line);
void do_abort_silent(void);
int false_inst(struct inst *p);
dbref find_mlev(dbref prog, struct frame *fr, int st);
dbref find_uid(dbref player, struct frame *fr, int st, dbref program);
char *insttotext(struct frame *, int, struct inst *, char *, int, int, dbref, int);
struct frame *interp(int descr, dbref player, dbref location, dbref program,
                            dbref source, int nosleeping, int whichperms, int forced_pid);
struct inst *interp_loop(dbref player, dbref program, struct frame *fr, int rettyp);
int is_home(struct inst *oper);
void localvar_dupall(struct frame *fr, struct frame *oldfr);
struct localvars *localvars_get(struct frame *fr, dbref prog);
int permissions(dbref player, dbref thing);
struct forvars *pop_for(struct forvars *);
struct tryvars *pop_try(struct tryvars *);
void prog_clean(struct frame *fr);
void purge_all_free_frames(void);
void purge_free_frames(void);
void purge_for_pool(void);
void purge_try_pool(void);
void push(struct inst *stack, int *top, int type, voidptr res);
struct forvars *push_for(struct forvars *);
struct tryvars *push_try(struct tryvars *);
void scopedvar_dupall(struct frame *fr, struct frame *oldfr);
struct inst *scopedvar_get(struct frame *fr, int level, int varnum);
const char *scopedvar_getname(struct frame *fr, int level, int varnum);
const char *scopedvar_getname_byinst(struct inst *pc, int varnum);
int valid_object(struct inst *oper);
int valid_player(struct inst *oper);

#include "p_array.h"
#include "p_connects.h"
#include "p_db.h"
#include "p_error.h"
#include "p_float.h"
#include "p_math.h"
#include "p_misc.h"
#include "p_props.h"
#include "p_stack.h"
#include "p_mcp.h"
#include "p_strings.h"
#include "p_regex.h"
#endif				/* _INTERP_H */
