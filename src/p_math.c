/** @file p_math.c
 *
 * Implementation of Math Operations for MUF
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include "config.h"

#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "inst.h"
#include "interp.h"

/*
 * TODO: These globals really probably shouldn't be globals.  I can only guess
 *       this is either some kind of primitive code re-use because all the
 *       functions use them, or it's some kind of an optimization to avoid
 *       local variables.  But it kills the thread safety and (IMO) makes the
 *       code harder to read/follow.
 */

/**
 * @private
 * @var used to store the parameters passed into the primitives
 *      This seems to be used for conveinance, but makes this probably
 *      not threadsafe.
 */
static struct inst *oper1, *oper2, *oper3, *oper4;

/**
 * @private
 * @var to store the result which is not a local variable for some reason
 *      This makes things very not threadsafe.
 */
static int tmp, result;

/**
 * @private
 * @var this is used to store results sometimes.  Very not threadsafe.
 */
static double tl;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

/**
 * @private
 * @var threadsafety, shmedsafety
 */
static double fresult, tf1, tf2;

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
void
prim_add(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    /* Handle the string-cat case first */
    if (oper1->type == PROG_STRING && oper2->type == PROG_STRING) {
        struct shared_string *string;

        if (!oper1->data.string && !oper2->data.string)
            string = NULL; /* Both empty strings */
        else if (!oper2->data.string) {
            oper1->data.string->links++; /* Operand 2 empty */
            string = oper1->data.string;
        } else if (!oper1->data.string) {
            oper2->data.string->links++; /* Operand 1 empty */
            string = oper2->data.string;
        } else if (oper1->data.string->length + oper2->data.string->length
                   > (BUFFER_LEN) - 1) { /* Strings too long */
            abort_interp("Operation would result in overflow.");
        } else { /* Do the string concat */
            memmove(buf, oper2->data.string->data, oper2->data.string->length);
            memmove(buf + oper2->data.string->length, oper1->data.string->data,
                    oper1->data.string->length + 1);
            string = alloc_prog_string(buf);
        }

        CLEAR(oper1);
        CLEAR(oper2);
        PushStrRaw(string); /* Push result */
        return;
    }

    /* Only numeric types from this point on */
    if (!arith_type(oper2->type, oper1->type))
        abort_interp("Invalid argument type.");

    if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
        tf1 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber
                                          : oper1->data.number;
        tf2 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber
                                          : oper2->data.number;

        if (!no_good(tf1) && !no_good(tf2)) {
            fresult = tf1 + tf2;
        } else {
            fresult = 0.0;
            fr->error.error_flags.f_bounds = 1;
        }
    } else {
        result = oper1->data.number + oper2->data.number;
        tl = (double) oper1->data.number + (double) oper2->data.number;

        if (!arith_good(tl))
            fr->error.error_flags.i_bounds = 1;
    }

    tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT)
          ? PROG_FLOAT : oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);

    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, &fresult);
    else
        push(arg, top, tmp, &result);
}

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
void
prim_subtract(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!arith_type(oper2->type, oper1->type))
        abort_interp("Invalid argument type.");

    if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
        tf1 = (oper2->type == PROG_FLOAT)
              ? oper2->data.fnumber : oper2->data.number;
        tf2 = (oper1->type == PROG_FLOAT)
              ? oper1->data.fnumber : oper1->data.number;

        if (!no_good(tf1) && !no_good(tf2)) {
            fresult = tf1 - tf2;
        } else {
            fresult = 0.0;
            fr->error.error_flags.f_bounds = 1;
        }
    } else {
        result = oper2->data.number - oper1->data.number;
        tl = (double) oper2->data.number - (double) oper1->data.number;

        if (!arith_good(tl))
            fr->error.error_flags.i_bounds = 1;
    }

    tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT)
          ? PROG_FLOAT : oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);

    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, &fresult);
    else
        push(arg, top, tmp, &result);
}

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
void
prim_multiply(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    /* Handle the case of a string and integer */
    if ((oper1->type == PROG_STRING && oper2->type == PROG_INTEGER) ||
        (oper1->type == PROG_INTEGER && oper2->type == PROG_STRING)) {
        struct shared_string *string;

        if (oper1->type == PROG_INTEGER) {
            tmp = oper1->data.number;
            string = oper2->data.string;
        } else {
            tmp = oper2->data.number;
            string = oper1->data.string;
        }

        if (tmp < 0) {
            abort_interp("Must use a positive amount to repeat.");
        }

        if (string && string->length > 0 && tmp > 0) {
            /* this check avoids integer overflow with tmp * length */
            if (string->length > (size_t)(INT_MAX / tmp)) {
                abort_interp("Operation would result in overflow.");
            }

            if ((size_t)tmp * string->length > (BUFFER_LEN) - 1) {
                abort_interp("Operation would result in overflow.");
            }

            for (size_t i = 0; i < (size_t)tmp; i++) {
                memmove(buf + i * string->length, DoNullInd(string), string->length);
            }

            buf[(size_t)tmp * string->length] = 0;
        } else {
            buf[0] = 0;
        }

        CLEAR(oper1);
        CLEAR(oper2);
        PushStrRaw(alloc_prog_string(buf));
        return;
    }

    /* Otherwise, we can only handle numeric types */
    if (!arith_type(oper2->type, oper1->type))
        abort_interp("Invalid argument type.");

    if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
        tf1 = (oper1->type == PROG_FLOAT)
              ? oper1->data.fnumber : oper1->data.number;
        tf2 = (oper2->type == PROG_FLOAT)
              ? oper2->data.fnumber : oper2->data.number;

        if (!no_good(tf1) && !no_good(tf2)) {
            fresult = tf1 * tf2;
        } else {
            fresult = 0.0;
            fr->error.error_flags.f_bounds = 1;
        }
    } else {
        result = oper1->data.number * oper2->data.number;
        tl = (double) oper1->data.number * (double) oper2->data.number;

        if (!arith_good(tl))
            fr->error.error_flags.i_bounds = 1;
    }

    tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);

    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, &fresult);
    else
        push(arg, top, tmp, &result);
}

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
void
prim_divide(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!arith_type(oper2->type, oper1->type))
        abort_interp("Invalid argument type.");

    if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
        if ((oper1->type == PROG_INTEGER && !oper1->data.number) ||
            (oper1->type == PROG_FLOAT &&
             fabs(oper1->data.fnumber) < DBL_EPSILON)) {
            /* FIXME: This should be NaN.  */
            fresult = INF;
            fr->error.error_flags.div_zero = 1;
        } else {
            tf1 = (oper2->type == PROG_FLOAT)
                  ? oper2->data.fnumber : oper2->data.number;
            tf2 = (oper1->type == PROG_FLOAT)
                  ? oper1->data.fnumber : oper1->data.number;

            if (!no_good(tf1) && !no_good(tf2)) {
                fresult = tf1 / tf2;
            } else {
                fresult = 0.0;
                fr->error.error_flags.f_bounds = 1;
            }
        }
    } else {
        if (oper1->data.number == -1 && oper2->data.number == INT_MIN) {
            result = 0;
            fr->error.error_flags.i_bounds = 1;
        } else if (oper1->data.number) {
            result = oper2->data.number / oper1->data.number;
        } else {
            result = 0;
            fr->error.error_flags.div_zero = 1;
        }
    }

    tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);

    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, &fresult);
    else
        push(arg, top, tmp, &result);
}

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
void
prim_mod(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if ((!arith_type(oper2->type, oper1->type))
        || (oper1->type == PROG_FLOAT)
        || (oper2->type == PROG_FLOAT))
        abort_interp("Invalid argument type.");

    if (oper1->data.number == -1 && oper2->data.number == INT_MIN) {
        result = 1;
        fr->error.error_flags.i_bounds = 1;
    } else if (oper1->data.number)
        result = oper2->data.number % oper1->data.number;
    else
        result = 0;

    tmp = oper2->type;

    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, &result);
}

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
void
prim_bitor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Non-integer argument (2)");

    result = oper2->data.number | oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_bitxor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Non-integer argument (2)");

    result = oper2->data.number ^ oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_bitand(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Non-integer argument (2)");

    result = oper2->data.number & oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_bitshift(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Non-integer argument (2)");

    int shiftBy = oper1->data.number;
    int maxShift = sizeof(int) * 8;

    if (shiftBy >= maxShift) {
        result = 0;
    } else if (shiftBy <= -maxShift) {
        result = oper2->data.number > 0 ? 0 : -1;
    } else if (shiftBy > 0) {
        result = (int) ((unsigned) oper2->data.number << (unsigned) shiftBy);
    } else if (shiftBy < 0) {
        result = oper2->data.number >> (-shiftBy);
    } else {
        result = oper2->data.number;
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF AND
 *
 * Consumes two values on the stack.  Returns true if both items evaluate
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
void
prim_and(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    result = !false_inst(oper1) && !false_inst(oper2);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_or(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    result = !false_inst(oper1) || !false_inst(oper2);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_xor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (false_inst(oper1)) {
        result = !false_inst(oper2);
    } else {
        result = false_inst(oper2);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_not(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = false_inst(oper1);

    CLEAR(oper1);
    PushInt(result);
}

/*
 * TODO: These next few functions make use of some really complicated
 *       inline-if (the .... ? ... : syntax) that actually chains the inline
 *       if's together.  Personally I find that really hard to read.  I would
 *       suggest breaking them out into real if's even if that means more
 *       lines of code, just so the reader can more easily see what is going
 *       on.
 *
 *       Or don't and just delete this TODO :)  (-tanabi)
 */

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
void
prim_lessthan(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!comp_t(oper1->type) || !comp_t(oper2->type))
        abort_interp("Invalid argument type.");

    if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
        /* Load either the float or integer value based on type */
        tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
              (oper2->type == PROG_INTEGER) ? oper2->data.number :
               oper2->data.objref;
        tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
              (oper1->type == PROG_INTEGER) ? oper1->data.number :
              oper1->data.objref;
        result = tf1 < tf2;
    } else {
        result = (((oper2->type == PROG_INTEGER) ? oper2->data.number :
                  oper2->data.objref)
                  <
                 ((oper1->type == PROG_INTEGER) ? oper1->data.number :
                  oper1->data.objref));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_greathan(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!comp_t(oper1->type) || !comp_t(oper2->type))
        abort_interp("Invalid argument type.");

    if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
        tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
               (oper2->type == PROG_INTEGER) ? oper2->data.number :
               oper2->data.objref;
        tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
               (oper1->type == PROG_INTEGER) ? oper1->data.number :
               oper1->data.objref;
        result = tf1 > tf2;
    } else {
        result = (((oper2->type == PROG_INTEGER) ? oper2->data.number :
                  oper2->data.objref)
                  >
                 ((oper1->type == PROG_INTEGER) ? oper1->data.number :
                  oper1->data.objref));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_equal(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type == PROG_STRING && oper2->type == PROG_STRING) {
        if (oper1->data.string == oper2->data.string) {
            result = 1;
        } else {
            result = strcmp(DoNullInd(oper1->data.string),
                            DoNullInd(oper2->data.string));
            result = result ? 0 : 1;
        }
    } else {
        if (!comp_t(oper1->type) || !comp_t(oper2->type))
            abort_interp("Invalid argument type.");

        if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
            tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
                   (oper2->type == PROG_INTEGER) ? oper2->data.number :
                    oper2->data.objref;
            tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
                   (oper1->type == PROG_INTEGER) ? oper1->data.number :
                    oper1->data.objref;
            result = tf1 == tf2;
        } else {
            result = (((oper2->type == PROG_INTEGER) ? oper2->data.number :
                      oper2->data.objref)
                      ==
                     ((oper1->type == PROG_INTEGER) ? oper1->data.number :
                      oper1->data.objref));
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_lesseq(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!comp_t(oper1->type) || !comp_t(oper2->type))
        abort_interp("Invalid argument type.");

    if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
        tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
               (oper2->type == PROG_INTEGER) ? oper2->data.number :
                oper2->data.objref;
        tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
               (oper1->type == PROG_INTEGER) ? oper1->data.number :
                oper1->data.objref;
        result = tf1 <= tf2;
    } else {
        result = (((oper2->type == PROG_INTEGER) ? oper2->data.number :
                  oper2->data.objref)
                  <=
                 ((oper1->type == PROG_INTEGER) ? oper1->data.number :
                  oper1->data.objref));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_greateq(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!comp_t(oper1->type) || !comp_t(oper2->type))
        abort_interp("Invalid argument type.");

    if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
        tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
               (oper2->type == PROG_INTEGER) ? oper2->data.number :
                oper2->data.objref;
        tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
               (oper1->type == PROG_INTEGER) ? oper1->data.number :
                oper1->data.objref;
        result = tf1 >= tf2;
    } else {
        result = (((oper2->type == PROG_INTEGER) ? oper2->data.number :
                  oper2->data.objref)
                  >=
                 ((oper1->type == PROG_INTEGER) ? oper1->data.number :
                  oper1->data.objref));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

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
void
prim_random(PRIM_PROTOTYPE)
{
    result = RANDOM();
    CHECKOFLOW(1);
    PushInt(result);
}

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
void
prim_srand(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    if (!(fr->rndbuf)) {
        fr->rndbuf = init_seed(NULL);
    }

    result = (int) rnd(fr->rndbuf);
    PushInt(result);
}

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
void
prim_getseed(PRIM_PROTOTYPE)
{
    char buf[33];
    char buf2[17];

    CHECKOP(0);
    CHECKOFLOW(1);

    if (!(fr->rndbuf)) {
        PushNullStr;
    } else {
        memcpy(buf2, fr->rndbuf, 16);
        buf2[16] = '\0';

        for (int loop = 0; loop < 16; loop++) {
            buf[loop * 2] = (buf2[loop] & 0x0F) + 65;
            buf[(loop * 2) + 1] = ((buf2[loop] & 0xF0) >> 4) + 65;
        }

        buf[32] = '\0';
        PushString(buf);
    }
}

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
void
prim_setseed(PRIM_PROTOTYPE)
{
    int slen;
    char holdbuf[33];
    char buf[17];

    CHECKOP(1);
    oper1 = POP();

    if (!(oper1->type == PROG_STRING))
        abort_interp("Invalid argument type.");

    if (fr->rndbuf) {
        free(fr->rndbuf);
        fr->rndbuf = NULL;
    }

    if (!oper1->data.string) {
        fr->rndbuf = init_seed(NULL);
        CLEAR(oper1);
        return;
    } else {
        slen = strlen(oper1->data.string->data);

        if (slen > 32)
            slen = 32;

        for (int sloop = 0; sloop < 32; sloop++)
            holdbuf[sloop] = oper1->data.string->data[sloop % slen];

        for (int sloop = 0; sloop < 16; sloop++)
            buf[sloop] = ((holdbuf[sloop * 2] - 65) & 0x0F) |
                         (((holdbuf[(sloop * 2) + 1] - 65) & 0x0F) << 4);

        buf[16] = '\0';
        fr->rndbuf = init_seed(buf);
    }

    CLEAR(oper1);
}

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
void
prim_int(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!(oper1->type == PROG_OBJECT || oper1->type == PROG_VAR ||
          oper1->type == PROG_LVAR || oper1->type == PROG_FLOAT))
        abort_interp("Invalid argument type.");

    if ((!(oper1->type == PROG_FLOAT)) ||
        (oper1->type == PROG_FLOAT && arith_good((double) oper1->data.fnumber))) {
        result = (int) ((oper1->type == PROG_OBJECT) ?
                 oper1->data.objref : (oper1->type == PROG_FLOAT) ?
                 oper1->data.fnumber : oper1->data.number);
    } else {
        result = 0;
        fr->error.error_flags.i_bounds = 1;
    }

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_plusplus(PRIM_PROTOTYPE)
{
    struct inst *tmp;
    struct inst temp1;
    struct inst temp2;
    int vnum;

    CHECKOP(1)
    temp1 = *(oper1 = POP());

    if (oper1->type == PROG_VAR || oper1->type == PROG_SVAR
        || oper1->type == PROG_LVAR)
        if (oper1->data.number > MAX_VAR || oper1->data.number < 0)
            abort_interp("Variable number out of range.");

    /* Load the correct type -- fetch from variable or from a steck item */
    switch (oper1->type) {
        case PROG_VAR:
            copyinst(&(fr->variables[temp1.data.number]), &temp2);
            break;

        case PROG_SVAR:
            tmp = scopedvar_get(fr, 0, temp1.data.number);
            copyinst(tmp, &temp2);
            break;

        case PROG_LVAR:{
            struct localvars *tmp2 = localvars_get(fr, program);
            copyinst(&(tmp2->lvars[temp1.data.number]), &temp2);
            break;
        }

        case PROG_INTEGER:
            oper1->data.number++;
            result = oper1->data.number;
            CLEAR(oper1);
            PushInt(result);
            return;

        case PROG_OBJECT:
            oper1->data.objref++;
            result = oper1->data.objref;
            CLEAR(oper1);
            PushObject(result);
            return;

        case PROG_FLOAT:
            oper1->data.fnumber++;
            fresult = oper1->data.fnumber;
            CLEAR(oper1);
            PushFloat(fresult);
            return;

        default:
            abort_interp("Invalid datatype.");
    }

    vnum = temp1.data.number;

    /* Do the correct increment */
    switch (temp2.type) {
        case PROG_INTEGER:
            temp2.data.number++;
            break;

        case PROG_OBJECT:
            temp2.data.objref++;
            break;

        case PROG_FLOAT:
            temp2.data.fnumber++;
            break;

        default:
            abort_interp("Invalid datatype in variable.");
    }

    /* If it's a var, we need to put it back in */
    switch (temp1.type) {
        case PROG_VAR:{
            CLEAR(&(fr->variables[vnum]));
            copyinst(&temp2, &(fr->variables[vnum]));
            break;
        }

        case PROG_SVAR:{
            struct inst *tmp2;
            tmp2 = scopedvar_get(fr, 0, vnum);

            if (!tmp2)
                abort_interp("Scoped variable number out of range.");

            CLEAR(tmp2);
            copyinst(&temp2, tmp2);
            break;
        }

        case PROG_LVAR:{
            struct localvars *tmp2 = localvars_get(fr, program);
            CLEAR(&(tmp2->lvars[vnum]));
            copyinst(&temp2, &(tmp2->lvars[vnum]));
            break;
        }
    }

    CLEAR(oper1);
}

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
void
prim_minusminus(PRIM_PROTOTYPE)
{
    struct inst *tmp;
    struct inst temp1;
    struct inst temp2;
    int vnum;

    /*
     * TODO: This code is almost 100% overlap with prim_plusplus.  We
     *       could combine the calls with a simple if statement around
     *       the ++ / -- part and use the thin wrappers you know I love
     *       so much :)  (but in this case, it makes a lot of sense)
     */

    CHECKOP(1)
    temp1 = *(oper1 = POP());

    if (oper1->type == PROG_VAR || oper1->type == PROG_SVAR
        || oper1->type == PROG_LVAR)
        if (oper1->data.number > MAX_VAR || oper1->data.number < 0)
            abort_interp("Variable number out of range.");

    /* Load the correct type -- fetch from variable or from a steck item */
    switch (oper1->type) {
        case PROG_VAR:
            copyinst(&(fr->variables[temp1.data.number]), &temp2);
            break;

        case PROG_SVAR:
            tmp = scopedvar_get(fr, 0, temp1.data.number);
            copyinst(tmp, &temp2);
            break;

        case PROG_LVAR: {
            struct localvars *tmp2 = localvars_get(fr, program);
            copyinst(&(tmp2->lvars[temp1.data.number]), &temp2);
            break;
        }

        case PROG_INTEGER:
            oper1->data.number--;
            result = oper1->data.number;
            CLEAR(oper1);
            PushInt(result);
            return;

        case PROG_OBJECT:
            oper1->data.objref--;
            result = oper1->data.objref;
            CLEAR(oper1);
            PushObject(result);
            return;

        case PROG_FLOAT:
            oper1->data.fnumber--;
            fresult = oper1->data.fnumber;
            CLEAR(oper1);
            PushFloat(fresult);
            return;

        default:
            abort_interp("Invalid datatype.");
    }

    vnum = temp1.data.number;

    /* Do the correct decrement */
    switch (temp2.type) {
        case PROG_INTEGER:
            temp2.data.number--;
            break;

        case PROG_OBJECT:
            temp2.data.objref--;
            break;

        case PROG_FLOAT:
            temp2.data.fnumber--;
            break;

        default:
            abort_interp("Invalid datatype in variable.");
    }

    /* If it's a var, we need to put it back in */
    switch (temp1.type) {
        case PROG_VAR: {
            CLEAR(&(fr->variables[vnum]));
            copyinst(&temp2, &(fr->variables[vnum]));
            break;
        }

        case PROG_SVAR: {
            struct inst *tmp2;
            tmp2 = scopedvar_get(fr, 0, vnum);

            if (!tmp2)
                abort_interp("Scoped variable number out of range.");

            CLEAR(tmp2);
            copyinst(&temp2, tmp2);
            break;
        }

        case PROG_LVAR: {
            struct localvars *tmp2 = localvars_get(fr, program);
            CLEAR(&(tmp2->lvars[vnum]));
            copyinst(&temp2, &(tmp2->lvars[vnum]));
            break;
        }
    }

    CLEAR(oper1);
}

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
void
prim_abs(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument.");

    result = oper1->data.number;

    if (result < 0)
        result = -result;

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_sign(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument.");

    if (oper1->data.number > 0) {
        result = 1;
    } else if (oper1->data.number < 0) {
        result = -1;
    } else {
        result = 0;
    }

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_notequal(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    /*
     * TODO: see the comment above about complex inline ifs ... this is another
     *       one that is implemented really "gross" and could use some cleanup
     */

    if (oper1->type == PROG_STRING && oper2->type == PROG_STRING) {
        if (oper1->data.string == oper2->data.string) {
            result = 0;
        } else {
            result = strcmp(DoNullInd(oper1->data.string),
                            DoNullInd(oper2->data.string));
            result = result ? 1 : 0;
        }
    } else {
        if (!comp_t(oper1->type) || !comp_t(oper2->type))
            abort_interp("Invalid argument type.");

        if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
            tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
                    (oper2->type == PROG_INTEGER) ? oper2->data.number :
                     oper2->data.objref;
            tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
                    (oper1->type == PROG_INTEGER) ? oper1->data.number :
                     oper1->data.objref;
            result = tf1 != tf2;
        } else {
            result = (((oper2->type == PROG_INTEGER) ? oper2->data.number :
                        oper2->data.objref)
                      !=
                      ((oper1->type ==
                        PROG_INTEGER) ? oper1->data.number :
                        oper1->data.objref));
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}
