/** @file p_error.h
 *
 * Declaration of Floating Point Error-Handling Operations for MUF
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
#ifndef P_ERROR_H
#define P_ERROR_H

#include "interp.h"

/**
 * Implementation of MUF CLEAR
 *
 * Clears all the error flags for floating point math operations on the
 * current frame.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_clear(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CLEAR_ERROR
 *
 * Consumes either an integer or a string.
 *
 * Clear a specific error flag for floating point math operations.
 *
 * You can use one of the following integers or strings:
 *
 * DIV_ZERO: 0
 * NAN: 1
 * IMAGINARY: 2
 * FBOUNDS: 3
 * IBOUNDS: 4
 *
 * Pushes 1 on the stack if a bit was cleared, 0 if not.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_clear_error(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SET_ERROR
 *
 * Consumes either an integer or a string.
 *
 * Set a specific error flag for floating point math operations.
 *
 * You can use one of the following integers or strings:
 *
 * DIV_ZERO: 0
 * NAN: 1
 * IMAGINARY: 2
 * FBOUNDS: 3
 * IBOUNDS: 4
 *
 * Pushes 1 on the stack if a bit was set, 0 if not.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_set_error(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ERROR?
 *
 * Returns boolean true or false.  If true, there is a floating point error.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_is_error(PRIM_PROTOTYPE);

/**
 * Implementation of MUF IS_SET?
 *
 * Consumes either an integer or a string.
 *
 * Returns true or false, based on if the provided bit is set or not.
 *
 * You can use one of the following integers or strings:
 *
 * DIV_ZERO: 0
 * NAN: 1
 * IMAGINARY: 2
 * FBOUNDS: 3
 * IBOUNDS: 4
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_is_set(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ERROR_STR
 *
 * Consumes either an integer or a string.
 *
 * Converts it to a descriptive string.
 *
 * You can use one of the following integers or strings:
 *
 * DIV_ZERO: 0
 * NAN: 1
 * IMAGINARY: 2
 * FBOUNDS: 3
 * IBOUNDS: 4
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_error_str(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ERROR_NAME
 *
 * Consumes an integer
 *
 * Converts it to a string that is the "short name" of the error.
 *
 * This is the mapping
 *
 * DIV_ZERO: 0
 * NAN: 1
 * IMAGINARY: 2
 * FBOUNDS: 3
 * IBOUNDS: 4
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_error_name(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ERROR_BIT
 *
 * Consumes a string
 *
 * Converts it to an integer which is the equivalent bit
 *
 * This is the mapping
 *
 * DIV_ZERO: 0
 * NAN: 1
 * IMAGINARY: 2
 * FBOUNDS: 3
 * IBOUNDS: 4
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_error_bit(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ERROR_NUM
 *
 * Returns the total number of floating point error flag types.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_error_num(PRIM_PROTOTYPE);

#define PRIMS_ERROR_FUNCS prim_clear, prim_clear_error, prim_is_error,    \
        prim_set_error, prim_is_set, prim_error_str, prim_error_name,     \
        prim_error_bit, prim_error_num

#define PRIMS_ERROR_NAMES "CLEAR","CLEAR_ERROR","ERROR?","SET_ERROR",     \
        "IS_SET?","ERROR_STR","ERROR_NAME","ERROR_BIT","ERROR_NUM"

#endif /* !P_ERROR_H */
