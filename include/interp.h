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

/**
 * This is used to free memory for the given instruction.
 *
 * It underpins the CLEAR macro which injects file and line automatically.
 * The memory freed is the data associated with the instruction rather than
 * the instruction itself, which remains forever as long as the program
 * is in memory.
 *
 * The instruction type is set to PROG_CLEARED after it has been cleared,
 * the 'line' field of the struct set to 'line', and the data set to
 * 'file' so that if we try to clear something that is already cleared,
 * we can produce a nice error message.
 *
 * @param oper the instruction who's data we are clearing.
 * @param file the file from which this function was called.
 * @param line the line in the file from which this function was called.
 */
void RCLEAR(struct inst *oper, char *file, int line);

/**
 * This is a wrapper around RCLEAR to automatically inject file and line
 *
 * Clears the data associated with instruction 'oper'.
 *
 * @see RCLEAR
 *
 * @param oper the struct inst whom we are clearing out
 */
#define CLEAR(oper) RCLEAR(oper, __FILE__, __LINE__)

/**
 * Determine the program effective MUCKER level and "return" it.
 *
 * This is a thin wrapper around find_mlev, injecting items from the
 * 'fr' frame which is assumed to be in scope.
 *
 * @see find_mlev
 *
 * @param x the program DBREF
 */
#define ProgMLevel(x) (find_mlev(x, fr, fr->caller.top))

/**
 * Determine the program effective user ID and "return" it
 *
 * This is a thin wrapper around find_uid, injecting items from the
 * 'fr' frame which is assumed to be in scope.
 *
 * @see find_uid
 */
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
 * MUF interp's 'nosleeps' parameter options for different program states.
 */

/**
 * @var pre-empt means the MUF will run til its over, not letting anything
 *      else timeshare with it.  This comes with it certain limitations to
 *      make sure the MUCK doesn't get locked up by a program.
 */
#define PREEMPT 0

/**
 * @var run the MUF in the foreground, which allows it to run READ commands
 */
#define FOREGROUND 1

/**
 * @var run the MUF in the background, which means it cannot run READ
 * commands.  Some things can only run in the background, like propqueue
 * programs.
 */
#define BACKGROUND 2
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
    struct callstack caller;    /* caller prog stack */
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

#ifdef DEBUG
/**
 * If DEBUG is set, we'll do a little extra tracking of the pop.
 *
 * These defines implement a simple pop that is used by MUF primitives
 * to remove items from the stack and 'return' them.
 */
#define POP() (++fr->actual_pop, arg + --(*top))
#else
#define POP() (arg + --(*top))
#endif

/**
 * Abort the interpreter, passing in file and line information, and also
 * passing along the standard parameters that are passed into all MUF
 * programs to do_abort_interp
 *
 * @see do_abort_interp
 *
 * Assumes the following variables are in scope:
 *
 * player, pc, arg, top, fr, oper1, oper2, oper2, oper4, nargs, program
 *
 * This will 'return' from whatever function it is in.
 *
 * @param C string error message to display
 */
#define abort_interp(C) \
{ \
  do_abort_interp(player, (C), pc, arg, *top, fr, oper1, oper2, oper3, oper4, \
                  nargs, program, __FILE__, __LINE__); \
  return; \
}

/**
 * This automates a check to make sure there are at least 'N' items on the stack
 *
 * Used by primitives to see if there's enough items on the stack to run
 * the primitive, doing an abort_interp with a stack underflow if there
 * aren't enough arguments.  This is for "read only" operations that aren't
 * going to change the stack.
 *
 * @see abort_interp
 *
 * @param N integer number of arguments to check for
 */
#define EXPECT_READ_STACK(N) \
{ \
    int depth = (N); \
    if (*top < depth) { \
        abort_interp("Stack underflow."); \
    } \
}

/**
 * This automates a check to make sure there are at least N items on the stack
 *
 * This checks for potential stack protection faults.  This is for if you
 * are going to alter the top N items on the stack.
 *
 * @see abort_interp
 *
 * @param N integer number of arguments to push on the stack.
 */
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
/**
 * This call varies based on if we are in debug mode or not.
 *
 * This does a check, expecting to pop N items off the stack.  Some
 * extra debug info is stored in DEBUG mode.  This is otherwise a very
 * thin wrapper around EXPECT_WRITE_STACK
 *
 * @see EXPECT_WRITE_STACK
 *
 * @param N the number of items we're expecting to pop.
 */
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


/**
 * Check the top 'N' items on the stack for read-only operation.
 *
 * This also sets the 'nargs' variable to 'N'
 *
 * @see EXPECT_READ_STACK
 *
 * @param N integer number of stack arguments to check for
 */
#define CHECKOP_READONLY(N) \
{ \
    nargs = (0); \
    EXPECT_READ_STACK(N); \
    nargs = (N); \
}

/**
 * Check the top 'N' items on the stack for write operation.
 *
 * This also sets the 'nargs' variable to 'N'
 *
 * @see EXPECT_POP_STACK
 *
 * @param N integer number of stack arguments to check for
 */
#define CHECKOP(N) \
{ \
    nargs = (0); \
    EXPECT_POP_STACK(N); \
    nargs = (N); \
}

/**
 * Check if dbref 'x' is considered remote.
 *
 * This will abort the interpreter and return the calling function if 'x' is
 * remote and we don't have permission to operate on remote items.
 *
 * @param x the dbref to check for remoteness
 */
#define CHECKREMOTE(x) if ((mlev < 2) && ((x) != HOME) && \
                           (LOCATION(x) != player) &&  \
                           (LOCATION(x) != LOCATION(player)) && \
                           ((x) != LOCATION(player)) && ((x) != player) \
                 && !controls(ProgUID, x)) \
                 abort_interp("Mucker Level 2 required to get remote info.");

/**
 * Push an object (dbref) 'x' onto the stack
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x a dbref to push onto the stack.
 */
#define PushObject(x)   push(arg, top, PROG_OBJECT, &x)

/**
 * Push an integer 'x' onto the stack
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x a integer to push onto the stack.
 */
#define PushInt(x)      push(arg, top, PROG_INTEGER, &x)

/**
 * Push a float 'x' onto the stack
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x a float to push onto the stack.
 */
#define PushFloat(x)    push(arg, top, PROG_FLOAT, &x)

/**
 * Push a parsed lock 'x' onto the stack
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x a lock to push onto the stack.
 */
#define PushLock(x)     push(arg, top, PROG_LOCK, copy_bool(x))

/**
 * Push a maker onto the stack
 *
 * Assumes 'arg' and 'top' are defined.
 */
#define PushMark()      push(arg, top, PROG_MARK, 0)

/**
 * Push a string 'x' WITHOUT COPYING IT onto the stack
 *
 * Assumes 'arg' and 'top' are defined.  Memory will belong to the stack.
 *
 * @param x a string to push onto the stack.
 */
#define PushStrRaw(x)   push(arg, top, PROG_STRING, x)

/**
 * COPIES and pushes a string 'x' onto the stack
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x a string to push onto the stack.
 */
#define PushString(x)   PushStrRaw(alloc_prog_string(x))

/**
 * Push an empty string onto the stack
 */
#define PushNullStr     PushStrRaw(0)

/**
 * Pushes an array 'x' onto the stack WITHOUT copying it
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x the array to push onto the stack
 */
#define PushArrayRaw(x) push(arg, top, PROG_ARRAY, x)

/**
 * Copies and pushes an instruction 'x' onto the stack.
 *
 * Assumes 'arg' and 'top' are defined.
 *
 * @param x the instruction to push onto the stack
 */
#define PushInst(x) copyinst(x, &arg[((*top)++)])

#ifdef DEBUG
/**
 * Check for overflow if 'x' items were to be pushed onto the stack
 *
 * Do this before pushing items onto the stack to make sure you don't
 * segfault the MUCK.  It will do an abort_interp if there's not enough
 * room, which will return your function for you.
 *
 * @see abort_interp
 *
 * If DEBUG is defined, this will set the number of expected pushes
 * on the frame structure.
 */
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

/*
 * All primitives use a common set of parameters.  This is the list of
 * parameters.
 *
 * The player running the program, the program, the effective MUCKER level,
 * the program counter instruction (pointer to the instruction we are
 * running, which is used for interpreter abort), the stack, the stack size,
 * and the program frame.
 */
#define PRIM_PROTOTYPE dbref player, dbref program, int mlev, \
                       struct inst *pc, struct inst *arg, int *top, \
                       struct frame *fr

#define SORTTYPE_CASEINSENS     0x1     /* Case insensitive sort constant */
#define SORTTYPE_DESCENDING     0x2     /* Descending ordering sort       */

#define SORTTYPE_CASE_ASCEND    0       /* Ascending, case sensitive */
#define SORTTYPE_NOCASE_ASCEND  (SORTTYPE_CASEINSENS)   /* Insensitive+Ascend */
#define SORTTYPE_CASE_DESCEND   (SORTTYPE_DESCENDING)   /* CaseSense+Desc.    */
/* Insitive + descending */
#define SORTTYPE_NOCASE_DESCEND (SORTTYPE_CASEINSENS | SORTTYPE_DESCENDING)
#define SORTTYPE_SHUFFLE        4   /* Randomize */

/**
 * @var the current highest PID.  The next MUF program will get this PID.
 */
extern int top_pid;

/**
 * @var Number of arguments a primitive uses, for cleanup purposes.
 *      This number is used by abort_interp to clean up if something fails.
 */
extern int nargs;

/**
 * @var the total number of primitives defined
 */
extern int prim_count;

/**
 * @var the array of instructions (primitives)
 */
extern const char *base_inst[];

/**
 * Make a copy of a given stack of forvars structures
 *
 * This is used by the MUF fork primitive and has very little utility
 * otherwise at this time.
 *
 * @param forstack the stack of forvars to copy
 * @return a copy of forstack
 */
struct forvars *copy_fors(struct forvars *);

/**
 * Make a copy of a given stack of tryvars structures
 *
 * This is used by the MUF fork primitive and has very little utility
 * otherwise at this time.
 *
 * @param trystack the stack of tryvars to copy
 * @return a copy of trystack
 */
struct tryvars *copy_trys(struct tryvars *);

/**
 * Copy an instruction from instruction 'from' to instruction 'to'
 *
 * This does a 'shallow copy', meaning, in most cases it just increases
 * the reference counter for stuff like arrays and strings.
 *
 * This is used for stuff like 'dup' to copy an instruction on the stack
 * to another entry in the stack (because the stack is fully pre-allocated)
 * It is truly used all over the place, though.
 *
 * Locks and functions get a deeper copy due to the nature of them.
 *
 * @param from the source instruction
 * @param to the destination instruction
 */
void copyinst(struct inst *from, struct inst *to);

/**
 * Deep copy an instruction from instruction 'from' to instruction 'to'
 *
 * Does a deep copy of 'in' to 'out'.  This iterates into arrays and
 * dictionaries to copy all the instructions within them in a recursive
 * fashion.  Otherwise it operates similar to copyinst, this really only
 * impacts arrays/dictionaries.
 *
 * @see copyinst
 *
 * @param from the source instruction
 * @param to the destination instruction
 * @param pinned boolean passed to any new arrays made.  -1 will use default
 */
void deep_copyinst(struct inst *from, struct inst *to, int pinned);

/**
 * Dump debugging data about an instruction
 *
 * This is used by DARK mode programs to dump data in a nicely formatted,
 * buffer-friendly kind of way.  It is only called in the interpreter loop.
 *
 * @param fr the program frame
 * @param lev this is passed into insttotext but is 0 everywhere this function
 *            is called so I'm not sure why we pass it.
 * @param pc the current instruction "program counter"
 * @param pid the running PID
 * @param stack the stack
 * @param buffer our output buffer
 * @param buflen the size of the buffer
 * @param sp the top of the stack
 * @param program the program dbref
 * @return the start of the debug string
 */
char *debug_inst(struct frame *, int, struct inst *, int, struct inst *,
                 char *, size_t, int, dbref);

/**
 * Abort the interpreter.  Do error notifications and stack traces as needed.
 *
 * This is rarely, if ever, called directly; the define abort_interp is
 * used instead.  This function only makes sense when called within the
 * interpreter loop.
 *
 * @see abort_interp
 *
 * @param player the player running teh program
 * @param msg the message to display to the player
 * @param pc the current running instruction (program counter)
 * @param arg the argument stack
 * @param atop the top of the argument stack
 * @param fr the running frame
 * @param oper1 if provided, this will be RCLEAR'd
 * @param oper2 if provided, this will be RCLEAR'd
 * @param oper3 if provided, this will be RCLEAR'd
 * @param oper4 if provided, this will be RCLEAR'd
 * @param nargs the number of operands provided, 0 through 4 are valid
 * @param program the program dbref
 * @param file the file name
 * @param line the line number
 */
void do_abort_interp(dbref player, const char *msg, struct inst *pc,
                     struct inst *arg, int atop, struct frame *fr,
                     struct inst *oper1, struct inst *oper2, struct inst *oper3,
                     struct inst *oper4, int nargs, dbref program, char *file,
                     int line);

/**
 * Silently abort the interpreter loop.
 *
 * Errors set with this will not be caught.
 *
 * This will always result in program termination the next time
 * interp_loop() checks for this.
 */
void do_abort_silent(void);

/**
 * Does the given instruction evaluate to false?
 *
 * This returns true, oddly enough, if 'p' evaluates to false.
 *
 * Empty strings, markers, empty arrays, locks that equal TRUE_BOOLEXP,
 * integers equal to 0, floats equal to 0.0, and dbrefs equal to NOTHING
 * all evaluate to false.
 *
 * @param p the instruction to evaluate
 * @return boolean true if 'p' is considered a false value, false otherwise.
 */
int false_inst(struct inst *p);

/**
 * Determine the effective MUCKER level of the program
 *
 * This is complicated by the STICKY / HAVEN flags and relative MUCKER levels
 * of program vs. owner.  But the result of this will be the MUCKER level the
 * program should run at.
 *
 * Usually you would use ProgMLevel instead of this function, as that define
 * is a shortcut wrapper around this.  @see ProgMLevel
 *
 * @param prog the ref of the program running
 * @param fr the frame of the running program
 * @param st the stack position of the program if one program called another
 * @return the effective MUCKER level as an integer
 */
dbref find_mlev(dbref prog, struct frame *fr, int st);

/**
 * Figure out effective dbref of running program's user
 *
 * This handles various nuances such as if the frame is SETUID, the
 * program is STICKY or HAVEN.  There's a lot of little complexities
 * here.
 *
 * If the program is sticky, or the frame is SETUID, and the program is
 * haven and owned by a wizard,, then the UID will be the UID Of the calling
 * program.
 *
 * If the program is sticky but not HAVEN, then the owner of the program
 * will be used as the UID.
 *
 * If the MUCKER level is less than 2, then it will be the UID of the
 * owner of the program.
 *
 * If the program is HAVEN, then the owner of the trigger is used if possible.
 * Otherwise, return owner of player.
 *
 * @param player the player running the program
 * @param fr the frame of the running program
 * @param st the calling stack level of the running program
 * @param program the dbref of the program
 * @return the effective program runner dbref.
 */
dbref find_uid(dbref player, struct frame *fr, int st, dbref program);

/**
 * Convert an instruction to a displayable string
 *
 * Converts an instruction into a printable string, stores the string in
 * buffer and returns a pointer to it.
 *
 * The first byte of the return value will be NULL if a buffer overflow
 * would have occured.  There's tons of logic here, though what it does is
 * basically straight forward.
 *
 * @param fr the program frame
 * @param lev the function level -- this is passed in to look up scoped args
 * @param theinst the instruction to convert
 * @param buffer the buffer to use
 * @param buflen the length of the buffer
 * @param strmax the maximum string length to display -- longer strings will
 *               truncate the string.  This only applies to string stack items
 * @param program the program ref
 * @param expandarrs boolean if true show items in array
 * @return a pointer to 'buffer'
 */
char *insttotext(struct frame *, int, struct inst *, char *, int, int, dbref, int);

/**
 * Set up a frame for MUF program interpretation
 *
 * The frame structure is the information for a running program.  This
 * function gets everything set up, the program counter in the right place,
 * and makes the program ready to run.
 *
 * Frames are re-used to improve allocation speed.  Up to tp_free_frames_pool
 * may be held in reserve at a time.
 *
 * @param descr the descriptor of the person calling the program
 * @param player the dbref of the person calling the program
 * @param location the dbref of the triggering location
 * @param program the dbref of the program to call
 * @param source the dbref of the triggering object
 * @param nosleeps PREEMPT, FOREGROUND, or BACKGROUND for initial program state
 * @param whichperms STD_REGUID, STD_SETUID, or STD_HARDUID
 * @param forced_pid if 0, make a new pid - otherwise use this number as pid
 *
 * @return constructed frame structure
 */
struct frame *interp(int descr, dbref player, dbref location, dbref program,
                     dbref source, int nosleeping, int whichperms,
                     int forced_pid);

/**
 * The MUF interpreter loop - run a program until it completes or yields
 *
 * This is where the magic happens.  There's a lot of nuances here that are
 * difficult to sum up into a little sound bite.  Here's the highlights:
 *
 * A PREEMPT or BOUND program runs until a certain number of instructions
 * (at which point it hard stops), until it has gotten too many nested
 * interpreter calls, or until it finishes completely.
 *
 * Otherwise, a program runs for awhile until it either blocks for input
 * or sleep, or it gets forcibly "0 sleep" injected to make it yield.  This
 * is called a slice.  All these numbers are tunable with @tune but the
 * defaults are pretty much always used.
 *
 * This will parse the instructions using a godawful switch statement.
 *
 * @param player the player running the program
 * @param program the program being run
 * @param fr the frame for the current running program
 * @param rettyp boolean if true we will return an instruction rather than
 *               a simple true or NULL
 * @return an instruction return value or NULL
 */
struct inst *interp_loop(dbref player, dbref program, struct frame *fr, int rettyp);

/**
 * Is the given instruction a ref to the special 'HOME' ref?
 *
 * @param oper the instruction to check
 * @return boolean true if oper is HOME, false otherwise.
 */
int is_home(struct inst *oper);

/**
 * Copy local vars from one frame to another
 *
 * This is, so far, just used by the fork primitive to copy lvar's from
 * one program to another.
 *
 * @param fr the new frame
 * @param oldfr the source frame
 */
void localvar_dupall(struct frame *fr, struct frame *oldfr);

/**
 * Get the local (lvar) variables for a given frame and program combination.
 *
 * @param fr the frame structure
 * @param prog the program DB ref
 * @return localvars structure
 */
struct localvars *localvars_get(struct frame *fr, dbref prog);

/**
 * Check to see if 'player' has ownership of 'thing'
 *
 * This is like a lame version of 'controls'.  It doesn't check Wizard
 * permissions; it just checks some basic things.
 *
 * If player == thing, thing == HOME, or the owner of player owns 'thing',
 * then this returns true.  If owner thing is NOTHING, that's true as well.
 *
 * This will always be false if player != thing and thing is a player.
 *
 * @param player the player to check permissions for
 * @param thing the thing to check player's permissions of.
 * @return boolean as described above
 */
int permissions(dbref player, dbref thing);

/**
 * Remove a forvars struct from a forstack
 *
 * The removed forvars struct is put at the head of the for_pool list.
 * It should be considered deleted at that point and no longer referenced.
 *
 * The calling pattern for this should be thus:
 *
 * stack = pop_for(stack)
 *
 * as this will return what should be the new top of the forstack.
 * The result may be NULL if all items are popped off.
 *
 * @param forstack the stack to remove from
 * @return the new head of the forstack with the top item removed.
 */
struct forvars *pop_for(struct forvars *);

/**
 * Remove a tryvars struct from a trystack
 *
 * The removed tryvars struct is put at the head of the try_pool list.
 * It should be considered deleted at that point and no longer referenced.
 *
 * The calling pattern for this should be thus:
 *
 * stack = pop_try(stack)
 *
 * as this will return what should be the new top of the trystack.
 * The result may be NULL if all items are popped off.
 *
 * @param trystack the stack to remove from
 * @return the new head of the trystack with the top item removed.
 */
struct tryvars *pop_try(struct tryvars *);

/**
 * Clean up a given frame, and return it to the free frames list.
 *
 * This does the heavy lifting of cleaning up a program.  This
 * includes stuff like freeing the program text, cleaning up variables,
 * etc. etc.
 *
 * This does NOT dequeue a running process and should NOT be used as
 * a "kill" command.
 *
 * @param fr the frame to clean up
 */
void prog_clean(struct frame *fr);

/**
 * Clean up the entire free frames pool
 *
 * Like purge_free_frames, except it deletes all of them.  Only defined if
 * MEMORY_CLEANUP is defined.  This is used when shutting down the MUCK.
 */
void purge_all_free_frames(void);

/**
 * Clean up extra free frames
 *
 * The MUCK keeps a set of allocated frames in memory to re-use for
 * performance reasons.  There's a tune parameter, tp_free_frames_pool,
 * which indicates the maximum size of this pool.  This call will shrink
 * the pool to tp_free_frames_pool in size.
 */
void purge_free_frames(void);

/**
 * Clean up the for memory pool
 *
 * This only purges up to the most recently used.  Run a second time to
 * purge everything.
 */
void purge_for_pool(void);

/**
 * Clean up the try memory pool
 *
 * This only purges up to the most recently used.  Run a second time to
 * purge everything.
 */
void purge_try_pool(void);

/**
 * Push a value onto the given instruction stack
 *
 * If a string is pushed onto the stack, it is not copied; it is assumed
 * to be a struct shared_string.
 *
 * @param stack the stack to push to
 * @param top the top stack number -- this will be updated by this call
 * @param type the type, PROG_FLOAT, PROG_STRING, etc.
 * @param res the value to push
 */
void push(struct inst *stack, int *top, int type, void *res);

/**
 * 'Push' an empty forvars struct into forstack and return it.
 *
 * Push is in quotes because it doesn't really push it -- you need to
 * do a call structure like this:
 *
 * stack = push_for(stack)
 *
 * This call may recycle previously allocated forstack structs.
 *
 * @param forstack the current top of the forstack
 * @return the new top of the forstack
 */
struct forvars *push_for(struct forvars *);

/**
 * 'Push' an empty tryvars struct into trystack and return it.
 *
 * Push is in quotes because it doesn't really push it -- you need to
 * do a call structure like this:
 *
 * stack = push_try(stack)
 *
 * This call may recycle previously allocated trystack structs.
 *
 * @param trystack the current top of the trystack
 * @return the new top of the trystack
 */
struct tryvars *push_try(struct tryvars *);

/**
 * Duplicate all scoped variables from one frame to another.
 *
 * This is currently only used for fork.
 *
 * @param fr the destination frame
 * @param oldfr the source frame
 */
void scopedvar_dupall(struct frame *fr, struct frame *oldfr);

/**
 * Get a scoped variable for the given number, with the given number
 *
 * @param fr the frame to get the variable from
 * @param level the function level to get from
 * @param varnum the number "id" of the variable to get
 * @return the instruction associated with the requested variable or NULL
 */
struct inst *scopedvar_get(struct frame *fr, int level, int varnum);

/**
 * Get a function scoped variable name by frame, function level, and number
 *
 * @param fr the frame to look up
 * @param level the function level
 * @param varnum the variable number
 * @return constant variable name - do not free this memory
 */
const char *scopedvar_getname(struct frame *fr, int level, int varnum);

/**
 * For a given program location and var number, get the var name
 *
 * This is for function-scoped variables.  This will find the variable
 * in the current scope of 'pc'.
 *
 * @param pc the current program location
 * @param varnum the variable number to fetch
 * @return constant variable name - do not free this memory
 */
const char *scopedvar_getname_byinst(struct inst *pc, int varnum);

/**
 * Is the given instruction a valid object ref?
 *
 * @param oper the instruction to check
 * @return boolean true if oper is a valid object ref
 */
int valid_object(struct inst *oper);

/**
 * Is the given instruction a valid player ref?
 *
 * @param oper the instruction to check
 * @return boolean true if oper is a valid player ref
 */
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
#include "p_mcpgui.h"
#include "p_strings.h"
#include "p_regex.h"
#endif /* !INTERP_H */
