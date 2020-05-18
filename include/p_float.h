/** @file p_float.h
 *
 * Header for the floating point primitives in MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_FLOAT_H
#define P_FLOAT_H

#include "interp.h"

/**
 * Implementation of MUF CEIL
 *
 * Consumes a float and pushes the next highest integer (as a float) on
 * the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ceil(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FLOOR
 *
 * Consumes a float and pushes the next lowest integer (as a float) on
 * the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_floor(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FLOAT
 *
 * Consumes an integer and pushes on an equivalent float.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_float(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SQRT
 *
 * Consumes a float and pushes the square root result on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_sqrt(PRIM_PROTOTYPE);

/**
 * Implementation of MUF POW
 *
 * Consumes two floats.  Returns f1 to the power of 2.  If f1 is zero,
 * then f2 must be greater than zero.  If f1 is less than zero, then f2
 * must be an integer value.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pow(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FRAND
 *
 * Returns a random floating point number between 0 and 1.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_frand(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GAUSSIAN
 *
 * Consumes two floats; a standard deviation and mean.  Returns a floating
 * point random number with the given normalization.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gaussian(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SIN
 *
 * Consumes a float, and pushes the computed sine value onto the stack.
 * Only operates in the range of -Pi/4 to Pi/4
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_sin(PRIM_PROTOTYPE);

/**
 * Implementation of MUF COS
 *
 * Consumes a float, and pushes the computed cosine value onto the stack.
 * Only operates in the range of -Pi/4 to Pi/4
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_cos(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TAN
 *
 * Consumes a float, and pushes the computed tangent value onto the stack.
 * Only operates in the range of K*pi + pi/2
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_tan(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ASIN
 *
 * Consumes a float, and pushes the computed inverse sine value onto the stack.
 * Only operates in the range of -Pi/2 to Pi/2
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_asin(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ACOS
 *
 * Consumes a float, and pushes the computed inverse cosine value onto the
 * stack.  Only operates in the range of 0 to Pi
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_acos(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ATAN
 *
 * Consumes a float, and pushes the computed inverse tangent value onto the
 * stack.  Only operates in the range of -Pi/2 to Pi/2
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_atan(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ATAN2
 *
 * Consumes a pair float-y and float-x and computes the inverse tangent
 * of fy / fx, taking into account the signs of both values and avoiding
 * divide-by-zero problems.  This is used to get an angle from X, Y
 * coordinates.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_atan2(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXP
 *
 * Consumes a float and returns the value of 'e' raised to the power of
 * the passed float.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_exp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LOG
 *
 * Consumes a float and returns its natural log.  Value must be greater than
 * 0.  Very small values will return INF.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_log(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LOG10
 *
 * Consumes a float and returns its log base 10.  Value must be greater than
 * 0.  Very small values will return INF.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_log10(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FABS
 *
 * Consumes a float and returns its absolute value.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_fabs(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRTOF
 *
 * Consumes a string and converts it to a floating point type.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strtof(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FTOSTR
 *
 * Consumesa float and converts it to a string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ftostr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FTOSTRC
 *
 * Consumes a float and converts it to a string.  Trailing zeros are removed
 * from the end of the number when no mantissa is shown.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ftostrc(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FMOD
 *
 * Consumes two floating points and returns the result of f1 mod f2
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_fmod(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MODF
 *
 * Consumes a floating point number, and pushes the integral part and the
 * fractional part as separate floats.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_modf(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PI
 *
 * Pushes the value of Pi onto the stack
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pi(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EPSILON
 *
 * Returns the smallest number such that 1.0 + Epsilon is distinct from 1.0
 * in the internal representation for floating point numbers.  This is the
 * precision error.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_epsilon(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INF
 *
 * Pushes the Infinitevalue onto the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_inf(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ROUND
 *
 * Consumes a float and an integer, and pushes a float onto the stack.
 * The integer is the number of places to the right of the decimal point
 * to round to.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_round(PRIM_PROTOTYPE);

/**
 * Implementation of MUF XYZ_TO_POLAR
 *
 * Consumes three floats, which are an x, y, z coordinates.  Converts
 * them to the coresponding spherical polar coordinates
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_xyz_to_polar(PRIM_PROTOTYPE);

/**
 * Implementation of MUF POLAR_TO_XYZ
 *
 * Consumes three floats, which are a set of polar coordinates.  Converts
 * them to the coresponding x, y, z coordinates.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_polar_to_xyz(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DIST3D
 *
 * Consumes three floats, float-x, float-y, and float-z.  Returns the
 * distance of this XYZ coordinate set from the origin.  Use float-z
 * of 0 for 2D computation.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dist3d(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DIFF3
 *
 * Consumes 6 floats representing two points; fx1, fy1, fz1, fx2, fy2, fz2
 * And pushes three floats on the stack, being fx1 - fx2, fy1 - fy2, and
 * fz1 - fz2
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_diff3(PRIM_PROTOTYPE);

/**
 * Primitive callbacks
 */
#define PRIMS_FLOAT_FUNCS prim_ceil, prim_floor, prim_float, prim_sqrt,  \
    prim_pow, prim_frand, prim_gaussian, prim_sin, prim_cos, prim_tan, prim_asin,   \
    prim_acos, prim_atan, prim_atan2, prim_exp, prim_log, prim_log10,\
    prim_fabs, prim_strtof, prim_ftostr, prim_fmod, prim_modf,       \
    prim_pi, prim_inf, prim_round, prim_dist3d, prim_xyz_to_polar,  \
    prim_polar_to_xyz, prim_diff3, prim_epsilon, prim_ftostrc, prim_pow

/**
 * Primitive names -- must be in same order as corresponding function
 */
#define PRIMS_FLOAT_NAMES "CEIL", "FLOOR", "FLOAT", "SQRT", \
    "POW", "FRAND", "GAUSSIAN", "SIN", "COS", "TAN", "ASIN",        \
    "ACOS", "ATAN", "ATAN2", "EXP", "LOG", "LOG10",     \
    "FABS", "STRTOF", "FTOSTR", "FMOD", "MODF",         \
    "PI", "INF", "ROUND", "DIST3D", "XYZ_TO_POLAR",     \
    "POLAR_TO_XYZ", "DIFF3", "EPSILON", "FTOSTRC", "**"

#endif /* !P_FLOAT_H */
