/** @file p_error.c
 *
 * Implementations of Floating Point Error-Handling Operations for MUF
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
#include "config.h"

#include "db.h"
#include "inst.h"
#include "interp.h"

/**
 * @private
 * @var used to store the parameters passed into the primitives
 *      This seems to be used for conveinance, but makes this probably
 *      not threadsafe. Currently used in the abort_interp macro.
 */
static struct inst *oper1, *oper2, *oper3, *oper4;

/**
 * @private
 * @var the maximum "error name" length, which is the define-ish short name
 *      of an error message.
 */
#define ERROR_NAME_MAX_LEN 15

/**
 * @private
 * @var the maximum error message length
 */
#define ERROR_STRING_MAX_LEN 80

/**
 * @private
 * @var the number of error message types we have
 */
#define ERROR_NUM 5

/*
 * This structure is data for the different types of floating point errors
 * there are.  It is used to map error bits to names (short codes) and
 * descriptive strings.
 */
struct err_type {
    char error_name[ERROR_NAME_MAX_LEN];        /* Short name for the error   */
    char error_string[ERROR_STRING_MAX_LEN];    /* The descriptive message    */
};

/**
 * @private
 * @var this is some kind of way to map an error number to a bit for bit
 *      math.  I believe it is exploiting how unions work to do this in
 *      a way so clever it breaks my brain a bit. (tanabi)
 *
 *      This seems to be legitimately read-only and probably safe to be
 *      global like this.
 */
static union error_mask err_bits[ERROR_NUM];

/**
 * @private
 * @var the different float error errors and messages, initialized
 */
static struct err_type err_defs[] = {
    {"DIV_ZERO", "Division by zero attempted."},
    {"NAN", "Result was not a number."},
    {"IMAGINARY", "Result was imaginary."},
    {"FBOUNDS", "Floating-point inputs were infinite or out of range."},
    {"IBOUNDS", "Calculation resulted in an integer overflow."},
    {"", ""}
};

/**
 * @private
 * @var boolean true if err_bits array has been initialized
 */
static int err_init = 0;

/**
 * Initialize the err_bits array
 *
 * This is run by functions that need to use the bit vector if err_init
 * is false.  err_init is not checked by this function, but it is set by it.
 *
 * It is probably safe to re-run this multiple times if that is done by error,
 * other than wasting CPU cycles.
 *
 * @private
 */
static void
init_err_val(void)
{
    for (int i = 0; i < ERROR_NUM; i++) {
        err_bits[i].is_flags = 0;
    }

    err_bits[0].error_flags.div_zero = 1;
    err_bits[1].error_flags.nan = 1;
    err_bits[2].error_flags.imaginary = 1;
    err_bits[3].error_flags.f_bounds = 1;
    err_bits[4].error_flags.i_bounds = 1;
    err_init = 1;
}

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
void
prim_clear(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    fr->error.is_flags = 0;
}

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
void
prim_error_num(PRIM_PROTOTYPE)
{
    int result;

    CHECKOP(0);
    CHECKOFLOW(1);
    result = ERROR_NUM;
    PushInt(result);
}

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
void
prim_clear_error(PRIM_PROTOTYPE)
{
    int result = 0;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER) {
        abort_interp("Invalid argument type. (1)");
    }

    if (!err_init) {
        init_err_val();
    }

    if (oper1->type == PROG_INTEGER) {
        if ((oper1->data.number >= 0) && (oper1->data.number < ERROR_NUM)) {
            fr->error.is_flags = fr->error.is_flags
                    & (~err_bits[oper1->data.number].is_flags);
            result = 1;
        }
    } else {
        char buf[BUFFER_LEN];
        snprintf(buf, sizeof(buf), "%s", DoNullInd(oper1->data.string));

        for (int i = 0; i < ERROR_NUM; i++) {
            if (!strcasecmp(buf, err_defs[i].error_name)) {
                result = 1;
                fr->error.is_flags = fr->error.is_flags
                        & (~err_bits[i].is_flags);
                break;
            }
        }
    }

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_set_error(PRIM_PROTOTYPE)
{
    int result = 0;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER) {
        abort_interp("Invalid argument type. (1)");
    }

    if (!err_init) {
        init_err_val();
    }

    if (oper1->type == PROG_INTEGER) {
        if ((oper1->data.number >= 0) && (oper1->data.number < ERROR_NUM)) {
            fr->error.is_flags = fr->error.is_flags
                    | err_bits[oper1->data.number].is_flags;
            result = 1;
        }
    } else {
        char buf[BUFFER_LEN];
        snprintf(buf, sizeof(buf), "%s", DoNullInd(oper1->data.string));

        for (int i = 0; i < ERROR_NUM; i++) {
            if (!strcasecmp(buf, err_defs[i].error_name)) {
                result = 1;
                fr->error.is_flags = fr->error.is_flags
                        | err_bits[i].is_flags;
                break;
            }
        }
    }

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_is_set(PRIM_PROTOTYPE)
{
    int result = 0;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER) {
        abort_interp("Invalid argument type. (1)");
    }

    if (!err_init) {
        init_err_val();
    }

    if (oper1->type == PROG_INTEGER) {
        if ((oper1->data.number >= 0) && (oper1->data.number < ERROR_NUM)) {
            result = ((fr->error.is_flags
                    & err_bits[oper1->data.number].is_flags) != 0);
        }
    } else {
        char buf[BUFFER_LEN];
        snprintf(buf, sizeof(buf), "%s", DoNullInd(oper1->data.string));

        for (int i = 0; i < ERROR_NUM; i++) {
            if (!strcasecmp(buf, err_defs[i].error_name)) {
                result = ((fr->error.is_flags
                        & err_bits[i].is_flags) != 0);
                break;
            }
        }
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF ERROR_STR
 *
 * Consumes either an integer (error number) or a string (error short name).
 *
 * Converts it to the error description.
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
void
prim_error_str(PRIM_PROTOTYPE)
{
    int result = -1;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER) {
        abort_interp("Invalid argument type. (1)");
    }

    if (!err_init) {
        init_err_val();
    }

    if (oper1->type == PROG_INTEGER) {
        if ((oper1->data.number >= 0) && (oper1->data.number < ERROR_NUM)) {
            result = oper1->data.number;
        }
    } else {
        char buf[BUFFER_LEN];
        snprintf(buf, sizeof(buf), "%s", DoNullInd(oper1->data.string));

        for (int i = 0; i < ERROR_NUM; i++) {
            if (!strcasecmp(buf, err_defs[i].error_name)) {
                result = i;
                break;
            }
        }
    }

    CLEAR(oper1);

    if (result == -1) {
        PushNullStr;
    } else {
        PushString(err_defs[result].error_string);
    }
}

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
void
prim_error_name(PRIM_PROTOTYPE)
{
    int result;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER) {
        abort_interp("Invalid argument type. (1)");
    }

    if (!err_init) {
        init_err_val();
    }

    if ((oper1->data.number < 0) || (oper1->data.number >= ERROR_NUM)) {
        result = -1;
    } else {
        result = oper1->data.number;
    }

    CLEAR(oper1);

    if (result == -1) {
        PushNullStr;
    } else {
        PushString(err_defs[result].error_name);
    }
}

/**
 * Implementation of MUF ERROR_BIT
 *
 * Consumes a string which is the short name of the error
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
void
prim_error_bit(PRIM_PROTOTYPE)
{
    int result = -1;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING) {
        abort_interp("Invalid argument type. (1)");
    }

    if (!err_init) {
        init_err_val();
    }

    if (oper1->data.string) {
        char buf[BUFFER_LEN];
        snprintf(buf, sizeof(buf), "%s", oper1->data.string->data);

        for (int i = 0; i < ERROR_NUM; i++) {
            if (!strcasecmp(buf, err_defs[i].error_name)) {
                result = i;
                break;
            }
        }
    }

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_is_error(PRIM_PROTOTYPE)
{
    int result;

    CHECKOP(0);
    CHECKOFLOW(1);
    result = ((fr->error.is_flags) != 0);
    PushInt(result);
}
