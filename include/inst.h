/** @file inst.h
 *
 * Header for various defines and data structures used by MUF programs.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef INST_H
#define INST_H

#include <stddef.h>

#include "boolexp.h"
#include "config.h"

#define IN_JMP        1
#define IN_READ       2
#define IN_SLEEP      3
#define IN_CALL       4
#define IN_EXECUTE    5
#define IN_RET        6
#define IN_EVENT_WAITFOR 7
#define IN_CATCH      8
#define IN_CATCH_DETAILED  9

struct line {
    const char *this_line;      /* the line itself */
    struct line *next, *prev;   /* the next line and the previous line */
};

struct shared_string {          /* for sharing strings in programs */
    int links;                  /* number of pointers to this struct */
    size_t length;              /* length of string data */
    char data[1];               /* shared string data */
};

struct prog_addr {              /* for 'address references */
    int links;                  /* number of pointers */
    dbref progref;              /* program dbref */
    struct inst *data;          /* pointer to the code */
};

struct stack_addr {             /* for the system callstack */
    dbref progref;              /* program call was made from */
    struct inst *offset;        /* the address of the call */
};

struct stk_array_t;

struct muf_proc_data {
    char *procname;
    int vars;
    int args;
    const char **varnames;
};

/* stack and object declarations */
/* Integer types go here */
#define PROG_CLEARED     0
#define PROG_PRIMITIVE   1      /* forth prims and hard-coded C routines */
#define PROG_INTEGER     2      /* integer types */
#define PROG_FLOAT       3      /* float types */
#define PROG_OBJECT      4      /* database objects */
#define PROG_VAR         5      /* variables */
#define PROG_LVAR        6      /* local variables, unique per program */
#define PROG_SVAR        7      /* scoped variables, unique per procedure */

/* Pointer types go here, numbered *AFTER* PROG_STRING */
#define PROG_STRING      9      /* string types */
#define PROG_FUNCTION    10     /* function names for debugging. */
#define PROG_LOCK        11     /* boolean expression */
#define PROG_ADD         12     /* program address - used in calls&jmps */
#define PROG_IF          13     /* A low level IF statement */
#define PROG_EXEC        14     /* EXECUTE shortcut */
#define PROG_JMP         15     /* JMP shortcut */
#define PROG_ARRAY       16     /* Array of other stack items. */
#define PROG_MARK        17     /* Stack marker for [ and ] */
#define PROG_SVAR_AT     18     /* @ shortcut for scoped vars */
#define PROG_SVAR_AT_CLEAR 19   /* @ for scoped vars, with var clear optim */
#define PROG_SVAR_BANG   20     /* ! shortcut for scoped vars */
#define PROG_TRY         21     /* TRY shortcut */
#define PROG_LVAR_AT     22     /* @ shortcut for local vars */
#define PROG_LVAR_AT_CLEAR 23   /* @ for local vars, with var clear optim */
#define PROG_LVAR_BANG   24     /* ! shortcut for local vars */

#define MAX_VAR         54      /* maximum number of variables including the
                                 * basic ME, LOC, TRIGGER, and COMMAND vars */
#define RES_VAR          4      /* no of reserved variables */

#define STACK_SIZE       1024   /* maximum size of stack */

#define DoNullInd(x) ((x) ? (x) -> data : "")

struct inst {                   /* instruction */
    short type;
    int line;
    union {
        struct shared_string *string;   /* strings */
        struct boolexp *lock;   /* boolean lock expression */
        int number;             /* used for both primitives and integers */
        double fnumber;         /* used for float storage */
        dbref objref;           /* object reference */
        struct stk_array_t *array;      /* pointer to muf array */
        struct inst *call;      /* use in IF and JMPs */
        struct prog_addr *addr; /* the result of 'funcname */
        struct muf_proc_data *mufproc;  /* Data specific to each procedure */
    } data;
};

#endif /* !INST_H */
