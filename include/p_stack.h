#ifndef P_STACK_H
#define P_STACK_H

#include "interp.h"

/**
 * Implementation of MUF POP
 *
 * Consumes the top-most item on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pop(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DUP
 *
 * Duplicates the top-most item on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dup(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ?DUP
 *
 * Duplicates the top-most item on the stack; if it resolves to true.
 * Otherwise, does nothing.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pdup(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NIP
 *
 * Consumes the second-to-top-most item from the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_nip(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TUCK
 *
 * Consumes the top-most stack item and places it under the next-top-most.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_tuck(PRIM_PROTOTYPE);

/**
 * Implementation of MUF @
 *
 * Consumes a variable, and returns its value.
 *
 * @see localvars_get
 * @see scopedvar_get
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_at(PRIM_PROTOTYPE);

/**
 * Implementation of MUF !
 *
 * Consumes a stack item and a variable, and sets the variable's value to the
 * stack item.
 *
 * @see localvars_get
 * @see scopedvar_get
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_bang(PRIM_PROTOTYPE);

/**
 * Implementation of MUF VARIABLE
 *
 * Consumes an integer, and returns the associated basic variable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_variable(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LOCALVAR
 *
 * Consumes an integer, and returns the associated local variable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_localvar(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SWAP
 *
 * Swaps the top two stack items.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_swap(PRIM_PROTOTYPE);

/**
 * Implementation of MUF OVER
 *
 * Duplicates the second-to-top-most stack item.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_over(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PICK
 *
 * Consumes a positive integer, and duplicates the stack item at that
 * position.  1 is top, 2 is next-to-top, and so on.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pick(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PUT
 *
 * Consumes a stack item and a positive integer, and replaces the stack item
 * at the given position.  1 is top, 2 is next-to-top, and so on.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_put(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ROT
 *
 * Rotates the top three stack items to the left.  (3) (2) (1) becomes (2) (1) (3).
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rot(PRIM_PROTOTYPE);

/**
 * Implementation of MUF RROT
 *
 * Rotates the top three stack items to the right.  (3) (2) (1) becomes (1) (3) (2).
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rrot(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ROTATE
 *
 * Consumes an integer, and rotates that many stack items.  A positive integer
 * rotates to the left, and a negative integer rotates to the right.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rotate(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DBTOP
 *
 * Returns the first dbref numerically after the highest dbref in the database.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dbtop(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DEPTH
 *
 * Returns the number of unlocked items currently on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_depth(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FULLDEPTH
 *
 * Returns the number of items currently on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_fulldepth(PRIM_PROTOTYPE);

/**
 * Implementation of MUF VERSION
 *
 * Returns the version string of the MUCK.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_version(PRIM_PROTOTYPE);

/**
 * Implementation of MUF VERSION
 *
 * Returns the dbref of the currently-running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_prog(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TRIG
 *
 * Returns the dbref of the object that originally triggered execution.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_trig(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CALLER
 *
 * Returns the dbref of the program that called this one, or the trigger if no
 * caller exists.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_caller(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INT?
 *
 * Consumes a stack item, and returns a boolean that represents if it is an
 * integer.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_intp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY?
 *
 * Consumes a stack item, and returns a boolean that represents if it is an
 * array (sequential list or dictionary).
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_arrayp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DICTIONARY?
 *
 * Consumes a stack item, and returns a boolean that represents if it is a
 * dictionary.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dictionaryp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FLOAT?
 *
 * Consumes a stack item, and returns a boolean that represents if it is a
 * floating point number.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_floatp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRING?
 *
 * Consumes a stack item, and returns a boolean that represents if it is a
 * string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_stringp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DBREF?
 *
 * Consumes a stack item, and returns a boolean that represents if it is a
 * dbref.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dbrefp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ADDRESS?
 *
 * Consumes a stack item, and returns a boolean that represents if it is a
 * function address.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_addressp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LOCK?
 *
 * Consumes a stack item, and returns a boolean that represents if it is a
 * lock.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_lockp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CHECKARGS
 *
 * Consumes an string representing the expected stack state, and aborts if
 * the current stack does not match.
 *
 * Also aborts if the string is not well-formed or is overly long.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_checkargs(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MODE
 *
 * Returns the multitasking mode of the current program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mode(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETMODE
 *
 * Consumes an integer, and sets the multitasking mode of the current program
 * accordingly.  Intended to be used with PR_MODE (0), FG_MODE (1), and BG_MODE (2).
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setmode(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INTERP
 *
 * Consumes two dbrefs (a program and a trigger) and a string, and invokes the
 * program (as PREEMPT) with the string as its initial stack item.
 *
 * Returns the final stack item of the program if it is successful, or "" if not.
 *
 * Requires MUCKER level 2 to bypass basic ownership checks on the trigger.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_interp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF REVERSE
 *
 * Consumes a non-negative integer, and reverses that many stack items.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_reverse(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LREVERSE
 *
 * Reverses a stackrange.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_lreverse(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DUPN
 *
 * Consumes a non-negative integer, and duplicates that many stack items.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dupn(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LDUP
 *
 * Duplicates a stackrange.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ldup(PRIM_PROTOTYPE);

/**
 * Implementation of MUF POPN
 *
 * Consumes a non-negative integer, and pops that many items off the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_popn(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FOR
 *
 * Consumes three integers, and initiates a basic loop.  The integers represent
 * the start index, end index, and the step.
 *
 * Uses IF and internal primitives FORITER and FORPOP to create the control flow.
 *
 * Aborts if the number of nested FOR loops is at maximum.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_for(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FOREACH
 *
 * Consumes an array, and initiates a loop iterating over its elements.
 *
 * Uses IF and internal primitives FORITER and FORPOP to create the control flow.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_foreach(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SECURE_SYSVARS
 *
 * Restores the original values of the variables me, loc, trigger, and command.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_secure_sysvars(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CMD
 *
 * Returns the command, without arguments, that started execution.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_cmd(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PREEMPT
 *
 * Turns off multitasking for the current program, disabling context switches.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_preempt(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FOREGROUND
 *
 * Turns on multitasking for the current program, enabling context switches.
 * This is the default for non-BOUND programs invoked by regular commands.
 *
 * Aborts if the program has ever been sent to the background via BACKGROUND.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_foreground(PRIM_PROTOTYPE);

/**
 * Implementation of MUF BACKGROUND
 *
 * Turns on multitasking for the current program, enabling context switches.
 * In addition, programs that are in the background cannot use READ primitives.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_background(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SHALLOW_COPY
 *
 * Duplicates the top-most item on the stack; if it is an array, makes a
 * shallow copy of it that is decoupled from the original.
 *
 * @see prim_dup
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_shallow_copy(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DEEP_COPY
 *
 * Duplicates the top-most item on the stack; if it is an array, makes a
 * deep copy of it that is decoupled from the original.
 *
 * @see prim_dup
 * @see deep_copyinst
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_deep_copy(PRIM_PROTOTYPE);

/**
 * Implementation of internal MUF primitive FORITER
 *
 * Processes the next iteration of the current FOR/FOREACH loop.
 *
 * Not recognized by the compiler.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_foriter(PRIM_PROTOTYPE);

/**
 * Implementation of internal MUF primitive FORPOP
 *
 * Cleans up the current FOR/FOREACH structure.
 *
 * Not recognized by the compiler.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_forpop(PRIM_PROTOTYPE);

/**
 * Implementation of MUF {
 *
 * Pushes a marker on the top of the stack, to be used in the creation or use
 * of stackranges.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mark(PRIM_PROTOTYPE);

/**
 * Implementation of MUF }
 *
 * Locates a matching marker ({) on the stack, and converts the items between into a
 * stackrange.  The marker is removed from the stack.
 *
 * Aborts if there is no marker currently on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_findmark(PRIM_PROTOTYPE);

/**
 * Implementation of internal MUF primitive TRYPOP
 *
 * Cleans up the current TRY structure.
 *
 * Not recognized by the compiler.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_trypop(PRIM_PROTOTYPE);

#define PRIMS_STACK_FUNCS prim_pop, prim_dup, prim_pdup, prim_nip, prim_tuck, \
    prim_at, prim_bang, prim_variable, prim_localvar, prim_swap, prim_over, \
    prim_pick, prim_put, prim_rot, prim_rrot, prim_rotate, prim_dbtop, \
    prim_depth, prim_version, prim_prog, prim_trig, prim_caller, prim_intp, \
    prim_stringp, prim_dbrefp, prim_addressp, prim_lockp, prim_checkargs, \
    prim_mode, prim_setmode, prim_interp, prim_for, prim_foreach, prim_floatp, \
    prim_reverse, prim_popn, prim_dupn, prim_ldup, prim_lreverse, \
    prim_arrayp, prim_dictionaryp, prim_mark, prim_findmark, prim_fulldepth, \
    prim_secure_sysvars, prim_cmd, prim_preempt, prim_foreground, \
    prim_background, prim_shallow_copy, prim_deep_copy

#define PRIMS_STACK_NAMES "POP", "DUP", "?DUP", "NIP", "TUCK",	\
    "@", "!", "VARIABLE", "LOCALVAR", "SWAP", "OVER", "PICK",	\
    "PUT", "ROT", "-ROT", "ROTATE", "DBTOP", "DEPTH", "VERSION",\
    "PROG", "TRIG", "CALLER", "INT?", "STRING?", "DBREF?",	\
    "ADDRESS?", "LOCK?", "CHECKARGS", "MODE", "SETMODE",	\
    "INTERP", " FOR", " FOREACH", "FLOAT?", "REVERSE", "POPN",	\
    "DUPN", "LDUP", "LREVERSE", "ARRAY?", "DICTIONARY?", "{",	\
    "}", "FULLDEPTH", "SECURE_SYSVARS", "CMD", "PREEMPT",	\
    "FOREGROUND", "BACKGROUND", "SHALLOW_COPY", "DEEP_COPY"

#define PRIMS_INTERNAL_FUNCS prim_foriter, prim_forpop, prim_trypop

#define PRIMS_INTERNAL_NAMES " FORITER", " FORPOP", " TRYPOP"

#endif /* !P_STACK_H */
