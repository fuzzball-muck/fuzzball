/** @file p_math.h
 *
 * Definition of Math Operations for MUF
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_MATH_H
#define P_MATH_H

#include "interp.h"

/**
 * Implementation of MUF +
 *
 * Consumes two numeric operands or two strings.
 *
 * Who knew adding was so complicated?  The complexity is in types.
 * If both operands are int or float, this will add them.  If either
 * operand is a float, then a float will be returned.
 *
 * Dbrefs and variable numbers can also be manipulated, but only by
 * integers.
 *
 * If, and only if, both operands are strings, then it will concatinate
 * them.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_add(PRIM_PROTOTYPE);

/**
 * Implementation of MUF -
 *
 * Consumes two numeric values.
 *
 * If both operands are int or float, this will subtract the top value from
 * the lower value.  If either operand is a float, then a float will be
 * returned.
 *
 * Dbrefs and variable numbers can also be manipulated, but only by
 * integers.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_subtract(PRIM_PROTOTYPE);

/**
 * Implementation of MUF *
 *
 * Consumes two numeric values, or a string and an integer.
 *
 * If both operands are int or float, this will multiply the two values.
 * If either operand is a float, then a float will be returned.
 *
 * If a string and an integer are provided, the string is duplicated
 * the integer number of times.
 *
 * Dbrefs and variable numbers can also be manipulated, but only by
 * integers.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_multiply(PRIM_PROTOTYPE);

/**
 * Implementation of MUF /
 *
 * Consumes two numeric values.
 *
 * If both operands are int or float, this will divide the top value from
 * the lower value.  If either operand is a float, then a float will be
 * returned.
 *
 * Dbrefs and variable numbers can also be manipulated, but only by
 * integers.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_divide(PRIM_PROTOTYPE);

/**
 * Implementation of MUF %
 *
 * Consumes two integer values.  Floats cannot use this.
 *
 * @see prim_fmod
 * @see prim_modf
 *
 * Returns the modulo of the lower number by the top number.  
 *
 * Dbrefs and variable numbers can also be manipulated with this.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mod(PRIM_PROTOTYPE);

/**
 * Implementation of MUF BITOR
 *
 * Consumes two integer values.  Floats or other semi-integer types like
 * dbrefs/variables cannot use this.
 *
 * Returns the bitwise OR of the two values.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_bitor(PRIM_PROTOTYPE);

/**
 * Implementation of MUF BITXOR
 *
 * Consumes two integer values.  Floats or other semi-integer types like
 * dbrefs/variables cannot use this.
 *
 * Returns the bitwise XOR of the bottom value by the top value.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_bitxor(PRIM_PROTOTYPE);

/**
 * Implementation of MUF BITAND
 *
 * Consumes two integer values.  Floats or other semi-integer types like
 * dbrefs/variables cannot use this.
 *
 * Returns the bitwise AND of the two values.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_bitand(PRIM_PROTOTYPE);

/**
 * Implementation of MUF BITSHIFT
 *
 * Consumes two integer values.  Floats or other semi-integer types like
 * dbrefs/variables cannot use this.
 *
 * Shifts the lower value by the upper value number of bits.  If the upper
 * value is out of range (more bits than sizeof(int)*8), then this will
 * return 0.  If the shift number is positive, shift left.  Otherwise,
 * if it is negative, shift right.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_bitshift(PRIM_PROTOTYPE);

/**
 * Implementation of MUF AND
 *
 * Consumes two values on the stack.  Returns true if both items evaluate
 * to true.
 *
 * @see false_instr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_and(PRIM_PROTOTYPE);

/**
 * Implementation of MUF OR
 *
 * Consumes two values on the stack.  Returns true if either items evaluate
 * to true.
 *
 * @see false_inst
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_or(PRIM_PROTOTYPE);

/**
 * Implementation of MUF XOR
 *
 * Consumes two values on the stack.  Returns true if only one item evaluates
 * to true.
 *
 * @see false_inst
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_xor(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NOT
 *
 * Consumes a single stack item.  Returns true if that item evalues to false.
 *
 * @see false_inst
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_not(PRIM_PROTOTYPE);

/**
 * Implementation of MUF <
 *
 * Consumes two values on the stack.  Returns true if the bottom number is
 * less than the top number.  Works with floats, integers, and dbrefs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_lessthan(PRIM_PROTOTYPE);

/**
 * Implementation of MUF >
 *
 * Consumes two values on the stack.  Returns true if the bottom number is
 * greater than the top number.  Works with floats, integers, and dbrefs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_greathan(PRIM_PROTOTYPE);

/**
 * Implementation of MUF =
 *
 * Consumes two values on the stack.  Returns true if the bottom number is
 * equal to the top number.  Works with strings, floats, integers, and dbrefs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_equal(PRIM_PROTOTYPE);

/**
 * Implementation of MUF <=
 *
 * Consumes two values on the stack.  Returns true if the bottom number is
 * less than or equal to the top number.  Works with strings, floats, integers,
 * and dbrefs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_lesseq(PRIM_PROTOTYPE);

/**
 * Implementation of MUF >=
 *
 * Consumes two values on the stack.  Returns true if the bottom number is
 * greater than or equal to the top number.  Works with strings, floats,
 * integers, and dbrefs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_greateq(PRIM_PROTOTYPE);

/**
 * Implementation of MUF RANDOM
 *
 * Pushes a random integer onto the stack.  The number can range from 0
 * to MAXINT.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_random(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INT
 *
 * Consumes a stack item and tries to turn it into an int.  Returns 0 if
 * it cannot, setting the i_bounds error flag.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_int(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SRAND
 *
 * Generates a seeded random number and pushes it on the stack.  It will be
 * an integer.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_srand(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETSEED
 *
 * Returns the current SRAND seed string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getseed(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETSEED
 *
 * Consumes a string from the stack and sets it as the SRAND seed.
 * Only the first 32 characters are significant.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setseed(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ++
 *
 * Consumes an integer, float, or dbref, adds one to it, and returns the result.
 * Also works on a variable, in which case the contents are fetched,
 * incremented, and automatically stored back in the variable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_plusplus(PRIM_PROTOTYPE);

/**
 * Implementation of MUF --
 *
 * Consumes an integer, float, or dbref, subtracts one from it, and returns
 * the result.
 *
 * Also works on a variable, in which case the contents are fetched,
 * decremented, and automatically stored back in the variable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_minusminus(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ABS
 *
 * Consumes an integer and pushes its absolute value on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_abs(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SIGN
 *
 * Consumes an integer.  Pushes 1 for positive, -1 for negative, or 0 for 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_sign(PRIM_PROTOTYPE);

/**
 * Implementation of MUF =
 *
 * Consumes two values on the stack.  Returns true if the bottom number is
 * not equal to the top number.  Works with strings, floats, integers, and
 * dbrefs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_notequal(PRIM_PROTOTYPE);

/**
 * Primitive callback functions
 */
#define PRIMS_MATH_FUNCS prim_add, prim_subtract, prim_multiply, prim_divide, \
    prim_mod, prim_bitor, prim_bitxor, prim_bitand, prim_bitshift, prim_and,  \
    prim_or, prim_not, prim_lessthan, prim_greathan, prim_equal, prim_lesseq, \
    prim_greateq, prim_random, prim_int, prim_srand, prim_setseed, prim_xor,  \
    prim_getseed, prim_plusplus, prim_minusminus, prim_abs, prim_sign,        \
    prim_notequal, prim_bitand, prim_bitor, prim_bitxor, prim_bitshift

/**
 * Primitive names - must be in same order as the callback functions
 */
#define PRIMS_MATH_NAMES  "+",  "-",  "*",  "/", "%", "BITOR", \
    "BITXOR", "BITAND", "BITSHIFT", "AND", "OR",  "NOT",  "<", \
     ">",  "=",  "<=", ">=", "RANDOM", "INT", "SRAND",         \
     "SETSEED", "XOR", "GETSEED", "++", "--", "ABS", "SIGN",   \
     "!=", "&", "|", "^", "<<"

#endif /* !P_MATH_H */
