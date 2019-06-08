/** @file interp.h
 *
 * Header that supports the MUF interpreter.  This has a lot of #defines,
 * data structures, and functions that support MUF interpretation.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef INTERP_H
#define INTERP_H

#include <stddef.h>
#include <time.h>

#include "array.h"
#include "config.h"
#include "fbstrings.h"
#include "inst.h"

/*
 * Some icky machine/compiler #defines. --jim
 * I think we can get rid of them though.  TODO!  --mainecoon
 *
 * @TODO Get rid of these or move 'em to one of the config files
 */
#ifdef MIPS
typedef char *voidptr;

#define MIPSCAST (char *)
#else
typedef void *voidptr;

#define MIPSCAST
#endif /* !MIPS */

void RCLEAR(struct inst *oper, char *file, int line);

#define CLEAR(oper) RCLEAR(oper, __FILE__, __LINE__)
#define ProgMLevel(x) (find_mlev(x, fr, fr->caller.top))
#define ProgUID find_uid(player, fr, fr->caller.top, program)


typedef struct inst vars[MAX_VAR];

/*
 * This structure keeps track of 'for' and 'foreach' structures to
 * support keeping track of the iteration variable, supporting
 * nested fors.
 */
struct forvars {
    int didfirst;       /* Boolean, initally false.  Done first iteration? */
    struct inst cur;    /* Current loop value                              */
    struct inst end;    /* The target end value                            */
    int step;           /* The step increment                              */
    struct forvars *next;   /* Linked list support for nesting             */
};

/*
 * This structure keeps track of try/catch blocks to support nesting and
 * to keep track of various try related issues.
 */
struct tryvars {
    int depth;              /* The stack lock depth of the try block       */
    int call_level;         /* The system stack top at the TRY             */
    int for_count;          /* How many for loops are in this try          */
    struct inst *addr;      /* Pointer to program counter instruction data */
    struct tryvars *next;   /* Linked list support for nested try's        */
};

/*
 * A program's "argument stack" which is the MUF stack from the developer's
 * perspective.
 */
struct stack {
    int top;                    /* Top index number in stack */
    struct inst st[STACK_SIZE]; /* The stack itself          */
};

/*
 * A program's "system stack" which is used to track program execution
 * such as function calls.
 */
struct sysstack {
    int top;                            /* Top index number in stack */
    struct stack_addr st[STACK_SIZE];   /* The stack itself          */
};

/*
 * The call stack is the stack of programs that have been CALL'd
 */
struct callstack {
    int top;
    dbref st[STACK_SIZE];
};

/*
 * Keep track of local variables for each program
 */
struct localvars {
    struct localvars *next;
    struct localvars **prev;
    dbref prog;
    vars lvars;
};

/*
 * Stack to keep track of variables in for loop.  There's one entry for
 * each level of for.
 */
struct forstack {
    int top;                /* Count of how many levels of for's we have */
    struct forvars *st;     /* The linked list that is the stack         */
};

/*
 * Stack to keep track of try/catch block nesting
 */
struct trystack {
    int top;                /* Count of how many levels of try's we have */
    struct tryvars *st;     /* The linked list that is the stack         */
};

/* Maximum debug break points allowed */
#define MAX_BREAKS 16

/*
 * Note: the :1 here is to indicate bit fields.  This is a way to make these
 * unsigned fields act like booleans, thus taking less space.
 *
 * This keeps track of MUF debugger operational 'stuff'.
 */
struct debuggerdata {
    unsigned debugging:1;       /* if set, this frame is being debugged */
    unsigned force_debugging:1; /* if set, debugger is active, even if not Z */
    unsigned bypass:1;          /* if set, bypass breakpoint on starting instr */
    unsigned isread:1;          /* if set, the prog is trying to do a read */
    unsigned showstack:1;       /* if set, show stack debug line, each inst. */
    unsigned dosyspop:1;        /* if set, fix up system stack before return. */
    int lastlisted;             /* last listed line */
    char *lastcmd;              /* last executed debugger command */
    short breaknum;             /* the breakpoint that was just caught on */

    dbref lastproglisted;       /* What program's text was last loaded to list? */
    struct line *proglines;     /* The actual program text last loaded to list. */

    short count;                /* how many breakpoints are currently set */
    short temp[MAX_BREAKS];     /* is this a temp breakpoint? */
    short level[MAX_BREAKS];    /* level breakpnts.  If -1, no check. */
    struct inst *lastpc;        /* Last inst interped.  For inst changes. */
    struct inst *pc[MAX_BREAKS];    /* pc breakpoint.  If null, no check. */
    int pccount[MAX_BREAKS];    /* how many insts to interp.  -2 for inf. */
    int lastline;               /* Last line interped.  For line changes. */
    int line[MAX_BREAKS];       /* line breakpts.  -1 no check. */
    int linecount[MAX_BREAKS];  /* how many lines to interp.  -2 for inf. */
    dbref prog[MAX_BREAKS];     /* program that breakpoint is in. */
};

/*
 * For tracking variables within a function scope.  These scopes form a
 * stack, hence the linked list.
 */
struct scopedvar_t {
    int count;                  /* Keep count of how many variables we have */
    const char **varnames;      /* The names of each variable in an array   */
    struct scopedvar_t *next;   /* Linked list of scoped vars               */
    struct inst vars[1];        /* The variables in this scope              */
};

/*
 * This is used by MCP-GUI.  It associates with "DLog Data" which seems
 * to be like a session ID for MCP-GUI.
 */
struct dlogidlist {
    struct dlogidlist *next;    /* The linked list of dlogidlist items */
    char dlogid[32];            /* This particular dlogid entry        */
};

/*
 * This is used to implement WATCHPID which triggers events based on
 * process exiting.
 */
struct mufwatchpidlist {
    struct mufwatchpidlist *next;   /* Linked list implementation */
    int pid;                        /* The PID watched/watching   */
};

/*
 * These appear to be used by the interpreter to determine what UID
 * settings to used.  In the DB these are implemented as flags.
 */
#define STD_REGUID  0   /* Regular UID settings     */
#define STD_SETUID  1   /* Use UID of program owner */
#define STD_HARDUID 2   /* Use UID of trigger owner */

/*
 * This is for having MUFs work with floating point error conditions.
 */
union error_mask {
    struct {
        unsigned int div_zero:1;    /* Divide by zero */
        unsigned int nan:1;         /* Result would not be a number */
        unsigned int imaginary:1;   /* Result would be imaginary */
        unsigned int f_bounds:1;    /* Float boundary error */
        unsigned int i_bounds:1;    /* Integer boundary error */
    } error_flags;

    int is_flags;
};

/* Frame data structure necessary for executing programs */
struct frame {
    struct frame *next;         /* Linked list implementation */
    struct sysstack system;     /* system stack */
    struct stack argument;      /* argument stack */
    struct callstack caller;	/* caller prog stack */
    struct forstack fors;       /* for loop stack */
    struct trystack trys;       /* try block stack */
    struct localvars *lvars;    /* local variables */
    vars variables;             /* global variables */
    struct inst *pc;            /* next executing instruction */
    short writeonly;            /* This program should not do reads */
    short multitask;            /* This program's multitasking mode */
    short timercount;           /* How many timers currently exist. */
    short level;                /* prevent interp call loops */
    int perms;                  /* permissions restrictions on program */
    short already_created;      /* this prog already created an object */
    short been_background;      /* this prog has run in the background */
    short skip_declare;         /* tells interp to skip next scoped var decl */
    short wantsblanks;          /* specifies program will accept blank READs */
    dbref trig;                 /* triggering object */
    dbref supplicant;           /* object for lock evaluation */
    struct shared_string *cmd;  /* Original command passed to the program, vars[3] */
    time_t started;             /* When this program started. */
    int instcnt;                /* How many instructions have run. */
    int pid;                    /* what is the process id? */
    char *errorstr;             /* the error string thrown */
    char *errorinst;            /* the instruction name that threw an error */
    dbref errorprog;            /* the program that threw an error */
    int errorline;              /* the program line that threw an error */
    int descr;                  /* what is the descriptor that started this? */
    void *rndbuf;               /* buffer for seedable random */
    struct scopedvar_t *svars;  /* Variables with function scoping. */
    struct debuggerdata brkpt;  /* info the debugger needs */
    struct timeval proftime;    /* profiling timing code */
    struct timeval totaltime;   /* profiling timing code */
    struct mufevent *events;    /* MUF event list. */
    struct dlogidlist *dlogids; /* List of dlogids this frame uses. */
    struct mufwatchpidlist *waiters;    /* MUFs waiting for a pid */
    struct mufwatchpidlist *waitees;    /* MUFs being waited for  */
    union error_mask error;     /* Floating point error mask */
    short pinning;              /* Are arrays/dicts are be pinned by default? */
#ifdef DEBUG
    int expect_pop;             /* Used to track expected number of POPs */
    int actual_pop;             /* Actual number of pops */
    int expect_push_to;         /* Expected number of push to's */
#endif
    stk_array_list array_active_list;       /* Active array list */
    stk_array_list *prev_array_active_list; /* Previous active list */
};

/*
 * Structure to keep track of public functions (basically library functions)
 */
struct publics {
    char *subname;          /* Subroutine for this public */
    int mlev;               /* This is either 4 or 1.  4 for wizard MUF */

    union {
        struct inst *ptr;   /* A pointer to the entrypoint instruction */
        int no;             /* Index into instruction array */
    } addr;

    struct publics *next;   /* Next item on the linked list */
};

/*
 * @TODO abort_loop and abort_loop_hard should be relocated to interp.c
 *       because they wrap do_abort_loop which is a static in interp.c
 */

/**
 * Handles an abort / exception in the interpreter loop, supporting try/catch
 *
 * This will either issue a 'break' or it will return 0
 *
 * @private
 * @param S the message for aborting
 * @param C1 instruction to clear or NULL
 * @param C2 instruction to clear or NULL
 */
#define abort_loop(S, C1, C2) \
{ \
    do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
    if (fr && fr->trys.top) \
        break; \
    else \
        return 0; \
}

/**
 * Handles an abort / exception in the interpreter loop, ignoring try/catch
 *
 * This will cause the function to return 0
 *
 * @private
 * @param S the message for aborting
 * @param C1 instruction to clear or NULL
 * @param C2 instruction to clear or NULL
 */
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
#endif /* !INTERP_H */
