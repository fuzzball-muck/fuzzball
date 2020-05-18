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

#define IN_JMP        1         /**< Jump the program counter */
#define IN_READ       2         /**< Do a READ  */
#define IN_SLEEP      3         /**< Do a SLEEP */
#define IN_CALL       4         /**< Call a public function */
#define IN_EXECUTE    5         /**< EXECUTE a program */
#define IN_RET        6         /**< Internal return instruction */
#define IN_EVENT_WAITFOR 7      /**< Wait for an event */
#define IN_CATCH      8         /**< Catch block */
#define IN_CATCH_DETAILED  9    /**< More detailed catch */

/**
 * A line of code
 */
struct line {
    const char *this_line;      /**< the line itself */
    struct line *next;          /**< the next line */
    struct line *prev;          /**< the previous line */
};

/**
 * For sharing strings in programs
 */
struct shared_string {
    int links;                  /**< number of pointers to this struct */
    size_t length;              /**< length of string data */
    char data[1];               /**< shared string data */
};

/**
 * For address references
 */
struct prog_addr {
    int links;                  /**< number of pointers */
    dbref progref;              /**< program dbref */
    struct inst *data;          /**< pointer to the code */
};

/**
 * For system callstack
 */
struct stack_addr {
    dbref progref;              /**< program call was made from */
    struct inst *offset;        /**< the address of the call */
};

struct stk_array_t; /**< The MUF array stack type */

/**
 * MUF Process information
 */
struct muf_proc_data {
    char *procname;         /**< Process name */
    int vars;               /**< Number of variables */
    int args;               /**< Number of arguments */
    const char **varnames;  /**< table of variable names */
};

/* stack and object declarations */
/* Integer types go here */
#define PROG_CLEARED     0      /**< Null */
#define PROG_PRIMITIVE   1      /**< forth prims and hard-coded C routines */
#define PROG_INTEGER     2      /**< integer types */
#define PROG_FLOAT       3      /**< float types */
#define PROG_OBJECT      4      /**< database objects */
#define PROG_VAR         5      /**< variables */
#define PROG_LVAR        6      /**< local variables, unique per program */
#define PROG_SVAR        7      /**< scoped variables, unique per procedure */

/* Pointer types go here, numbered *AFTER* PROG_STRING */
#define PROG_STRING      9      /**< string types */
#define PROG_FUNCTION    10     /**< function names for debugging. */
#define PROG_LOCK        11     /**< boolean expression */
#define PROG_ADD         12     /**< program address - used in calls&jmps */
#define PROG_IF          13     /**< A low level IF statement */
#define PROG_EXEC        14     /**< EXECUTE shortcut */
#define PROG_JMP         15     /**< JMP shortcut */
#define PROG_ARRAY       16     /**< Array of other stack items. */
#define PROG_MARK        17     /**< Stack marker for [ and ] */
#define PROG_SVAR_AT     18     /**< \@ shortcut for scoped vars */
#define PROG_SVAR_AT_CLEAR 19   /**< \@ for scoped vars, with var clear optim */
#define PROG_SVAR_BANG   20     /**< ! shortcut for scoped vars */
#define PROG_TRY         21     /**< TRY shortcut */
#define PROG_LVAR_AT     22     /**< \@ shortcut for local vars */
#define PROG_LVAR_AT_CLEAR 23   /**< \@ for local vars, with var clear optim */
#define PROG_LVAR_BANG   24     /**< ! shortcut for local vars */

#define MAX_VAR         54      /**< maximum number of variables including the
                                 *  basic ME, LOC, TRIGGER, and COMMAND vars
                                 */
#define RES_VAR          4      /**< no of reserved variables */

#define STACK_SIZE       1024   /**< maximum size of stack */

/**
 * If 'x' is NULL, return an empty string, otherwise return ->data
 *
 * @param x the variable to fetch data for
 * @return either x->data or "" if x is NULL
 */ 
#define DoNullInd(x) ((x) ? (x) -> data : "")

/**
 * An instruction in a MUF program
 */
struct inst {
    short type;     /**< Instruction type */
    int line;       /**< Line number */
    union {
        struct shared_string *string;   /**< strings */
        struct boolexp *lock;   /**< boolean lock expression */
        int number;             /**< used for both primitives and integers */
        double fnumber;         /**< used for float storage */
        dbref objref;           /**< object reference */
        struct stk_array_t *array;      /**< pointer to muf array */
        struct inst *call;      /**< use in IF and JMPs */
        struct prog_addr *addr; /**< the result of 'funcname */
        struct muf_proc_data *mufproc;  /**< Data specific to each procedure */
    } data; /**< Different kinds of instructions */
};

#endif /* !INST_H */
