/** @file p_stack.c
 *
 * Implementation of stack related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "inst.h"
#include "interp.h"

/**
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
static struct inst temp1, temp2, temp3;
static struct inst *oper1, *oper2, *oper3, *oper4;

/**
 * @private
 * @var to store the result which is not a local variable for some reason
 *      This makes things very not threadsafe.
 */
static int tmp, result;
static dbref ref;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

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
void
prim_pop(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    CLEAR(oper1);
}

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
void
prim_dup(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    CHECKOFLOW(1);

    copyinst(&arg[*top - 1], &arg[*top]);
    (*top)++;
}

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
void
prim_shallow_copy(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    CHECKOFLOW(1);

    if (arg[*top - 1].type != PROG_ARRAY) {
        copyinst(&arg[*top - 1], &arg[*top]);
        (*top)++;
    } else {
        stk_array *nu;
        stk_array *arr = arg[*top - 1].data.array;

        if (!arr) {
            nu = new_array_packed(0, fr->pinning);
        } else {
            nu = array_decouple(arr);
        }

        PushArrayRaw(nu);
    }
}

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
void
prim_deep_copy(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    CHECKOFLOW(1);

    deep_copyinst(&arg[*top - 1], &arg[*top], fr->pinning);
    (*top)++;
}

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
void
prim_pdup(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);

    if (!false_inst(&arg[*top - 1])) {
        CHECKOFLOW(1);
        copyinst(&arg[*top - 1], &arg[*top]);
        (*top)++;
    }
}

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
void
prim_popn(PRIM_PROTOTYPE)
{
    int count;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Operand not an integer.");

    count = oper1->data.number;
    if (count < 0)
        abort_interp("Operand is negative.");

    EXPECT_POP_STACK(count);

    for (int i = count; i > 0; i--) {
        oper2 = POP();
        CLEAR(oper2);
    }

    CLEAR(oper1);
}

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
void
prim_dupn(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Operand is not an integer.");

    result = oper1->data.number;
    if (result < 0)
        abort_interp("Operand is negative.");

    EXPECT_READ_STACK(result);
    CLEAR(oper1);
    nargs = 0;
    CHECKOFLOW(result);

    for (int i = result; i > 0; i--) {
        copyinst(&arg[*top - result], &arg[*top]);
        (*top)++;
    }
}

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
void
prim_ldup(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);

    nargs = 0;

    if (arg[*top - 1].type != PROG_INTEGER)
        abort_interp("Operand is not an integer.");

    result = arg[*top - 1].data.number;

    if (result >= INT_MAX)
        abort_interp("Integer overflow.");

    if (result < 0)
        abort_interp("Operand is negative.");

    result++;

    EXPECT_READ_STACK(result);
    CHECKOFLOW(result);

    for (int i = result; i > 0; i--) {
        copyinst(&arg[*top - result], &arg[*top]);
        (*top)++;
    }
}

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
void
prim_nip(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    CLEAR(oper2);

    arg[(*top)++] = *oper1;
}

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
void
prim_tuck(PRIM_PROTOTYPE)
{
    CHECKOFLOW(1);
    CHECKOP(2);
    oper1 = POP();
    temp2 = *(oper2 = POP());

    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp2;
    copyinst(&arg[*top - 2], &arg[*top]);
    (*top)++;
}

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
void
prim_at(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    temp1 = *(oper1 = POP());

    if ((temp1.type != PROG_VAR) && (temp1.type != PROG_LVAR) && (temp1.type != PROG_SVAR))
        abort_interp("Non-variable argument.");

    if (temp1.data.number >= MAX_VAR || temp1.data.number < 0)
        abort_interp("Variable number out of range.");

    if (temp1.type == PROG_LVAR) {
        /* LOCALVAR */
        struct localvars *tmp = localvars_get(fr, program);
        copyinst(&(tmp->lvars[temp1.data.number]), &arg[(*top)++]);
    } else if (temp1.type == PROG_VAR) {
        /* GLOBALVAR */
        copyinst(&(fr->variables[temp1.data.number]), &arg[(*top)++]);
    } else {
        /* SCOPEDVAR */
        struct inst *tmp;

        tmp = scopedvar_get(fr, 0, temp1.data.number);
        if (!tmp)
            abort_interp("Scoped variable number out of range.");

        copyinst(tmp, &arg[(*top)++]);
    }

    CLEAR(&temp1);
}

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
void
prim_bang(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if ((oper1->type != PROG_VAR) && (oper1->type != PROG_LVAR) && (oper1->type != PROG_SVAR))
        abort_interp("Non-variable argument (2)");

    if (oper1->data.number >= MAX_VAR || oper1->data.number < 0)
        abort_interp("Variable number out of range. (2)");

    if (oper1->type == PROG_LVAR) {
        /* LOCALVAR */
        struct localvars *tmp = localvars_get(fr, program);
        CLEAR(&(tmp->lvars[oper1->data.number]));
        copyinst(oper2, &(tmp->lvars[oper1->data.number]));
    } else if (oper1->type == PROG_VAR) {
        /* GLOBALVAR */
        CLEAR(&(fr->variables[oper1->data.number]));
        copyinst(oper2, &(fr->variables[oper1->data.number]));
    } else {
        /* SCOPEDVAR */
        struct inst *tmp;

        tmp = scopedvar_get(fr, 0, oper1->data.number);
        if (!tmp)
            abort_interp("Scoped variable number out of range.");

        CLEAR(tmp);
        copyinst(oper2, tmp);
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

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
void
prim_variable(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument.");

    result = oper1->data.number;
    CLEAR(oper1);
    push(arg, top, PROG_VAR, &result);
}

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
void
prim_localvar(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument.");

    result = oper1->data.number;
    CLEAR(oper1);
    push(arg, top, PROG_LVAR, &result);
}

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
void
prim_swap(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();

    temp2 = *(oper2 = POP());
    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp2;
}

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
void
prim_over(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(2);
    CHECKOFLOW(1);

    copyinst(&arg[*top - 2], &arg[*top]);
    (*top)++;
}

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
void
prim_pick(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    temp1 = *(oper1 = POP());

    if (temp1.type != PROG_INTEGER || temp1.data.number <= 0)
        abort_interp("Operand not a positive integer.");

    CHECKOP_READONLY(temp1.data.number);

    copyinst(&arg[*top - temp1.data.number], &arg[*top]);
    (*top)++;
}

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
void
prim_put(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER || oper1->data.number <= 0)
        abort_interp("Operand not a positive integer.");

    tmp = oper1->data.number;
    EXPECT_WRITE_STACK(tmp);

    CLEAR(&arg[*top - tmp]);
    copyinst(oper2, &arg[*top - tmp]);
    CLEAR(oper1);
    CLEAR(oper2);
}

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
void
prim_rot(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    temp3 = *(oper3 = POP());

    arg[(*top)++] = *oper2;
    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp3;
}

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
void
prim_rrot(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    temp2 = *(oper2 = POP());
    temp3 = *(oper3 = POP());

    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp3;
    arg[(*top)++] = temp2;
}

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
void
prim_rotate(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid argument type.");

    int n = oper1->data.number; /* Depth on stack */
    if (n == INT_MIN)
        abort_interp("Stack underflow.");

    EXPECT_WRITE_STACK(abs(n));

    if (n > 0) {
        temp2 = arg[*top - n];

        for (tmp = n; tmp > 0; tmp--)
            arg[*top - tmp] = arg[*top - tmp + 1];

        arg[*top - 1] = temp2;
    } else if (n < 0) {
        temp2 = arg[*top - 1];

        for (tmp = -1; tmp > n; tmp--)
            arg[*top + tmp] = arg[*top + tmp - 1];

        arg[*top + tmp] = temp2;
    }

    CLEAR(oper1);
}

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
void
prim_dbtop(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    ref = (dbref) db_top;
    PushObject(ref);
}

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
void
prim_depth(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = fr->trys.top ? *top - fr->trys.st->depth : *top;
    PushInt(result);
}

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
void
prim_fulldepth(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = *top;
    PushInt(result);
}

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
void
prim_version(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    PushString(VERSION);
}

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
void
prim_prog(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    ref = (dbref) program;
    PushObject(ref);
}

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
void
prim_trig(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    ref = (dbref) fr->trig;
    PushObject(ref);
}

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
void
prim_caller(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    ref = (dbref) fr->caller.st[fr->caller.top - 1];
    PushObject(ref);
}

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
void
prim_intp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_INTEGER);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_floatp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_FLOAT);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_arrayp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_ARRAY);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_dictionaryp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_ARRAY && oper1->data.array &&
              oper1->data.array->type == ARRAY_DICTIONARY);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_stringp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_STRING);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_cmd(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    PushStrRaw(fr->cmd);

    if (fr->cmd)
        fr->cmd->links++;
}

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
void
prim_dbrefp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_OBJECT);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_addressp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_ADD);

    CLEAR(oper1);
    PushInt(result);
}

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
void
prim_lockp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (oper1->type == PROG_LOCK);

    CLEAR(oper1);
    PushInt(result);
}

#define ABORT_CHECKARGS(msg) { if (*top == stackpos+1) snprintf(zbuf, sizeof(zbuf), "%s (top)", msg); else snprintf(zbuf, sizeof(zbuf), "%s (top-%d)", msg, ((*top)-stackpos-1));  abort_interp(zbuf); }

/**
 * @TODO This should be with other configuration options.
 */
#define MaxComplexity 18

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
void
prim_checkargs(PRIM_PROTOTYPE)
{
    int currpos, stackpos;
    int rngstktop = 0;
    enum {
        itsarange, itsarepeat
    } rngstktyp[MaxComplexity];
    int rngstkpos[MaxComplexity];
    int rngstkcnt[MaxComplexity];
    char zbuf[BUFFER_LEN];

    CHECKOP(1);
    oper1 = POP();              /* string argument */

    if (oper1->type != PROG_STRING)
        abort_interp("Non string argument.");

    if (!oper1->data.string) {
        /* if null string, then no args expected. */
        CLEAR(oper1);
        return;
    }

    strcpyn(buf, sizeof(buf), oper1->data.string->data);
    currpos = strlen(buf) - 1;
    stackpos = *top - 1;

    /**
     * @TODO More analysis is needed on this loop.  Refactoring the "underflow"
     *       logic is recommended, though we could also introduce a "stack type"
     *       structure that could eliminate more redundancy.
     */
    while (currpos >= 0) {
        if (isdigit(buf[currpos])) {
            if (rngstktop >= MaxComplexity)
                abort_interp("Argument expression ridiculously complex.");

            tmp = 1;
            result = 0;

            while ((currpos >= 0) && isdigit(buf[currpos])) {
                result = result + (tmp * (buf[currpos] - '0'));
                tmp = tmp * 10;
                currpos--;
            }

            if (result == 0)
                abort_interp("Bad multiplier '0' in argument expression.");

            if (result >= STACK_SIZE)
                abort_interp("Multiplier too large in argument expression.");

            rngstktyp[rngstktop] = itsarepeat;
            rngstkcnt[rngstktop] = result;
            rngstkpos[rngstktop] = currpos;
            rngstktop++;
        } else if (buf[currpos] == '}') {
            if (rngstktop >= MaxComplexity)
                abort_interp("Argument expression ridiculously complex.");

            if (stackpos < 0)
                ABORT_CHECKARGS("Stack underflow.");

            if (arg[stackpos].type != PROG_INTEGER)
                ABORT_CHECKARGS("Expected an integer range counter.");

            result = arg[stackpos].data.number;
            if (result < 0)
                ABORT_CHECKARGS("Range counter should be non-negative.");

            rngstkpos[rngstktop] = currpos - 1;
            rngstkcnt[rngstktop] = result;
            rngstktyp[rngstktop] = itsarange;
            rngstktop++;
            currpos--;

            if (result == 0) {
                while ((currpos > 0) && (buf[currpos] != '{'))
                    currpos--;
            }

            stackpos--;
        } else if (buf[currpos] == '{') {
            if (rngstktop <= 0)
                abort_interp("Mismatched { in argument expression");

            if (rngstktyp[rngstktop - 1] != itsarange)
                abort_interp("Misformed argument expression.");

            if (--rngstkcnt[rngstktop - 1] > 0) {
                currpos = rngstkpos[rngstktop - 1];
            } else {
                rngstktop--;
                currpos--;

                if (rngstktop && (rngstktyp[rngstktop - 1] == itsarepeat)) {
                    if (--rngstkcnt[rngstktop - 1] > 0) {
                        currpos = rngstkpos[rngstktop - 1];
                    } else {
                        rngstktop--;
                    }
                }
            }
        } else {
            switch (buf[currpos]) {
                case 'i':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_INTEGER)
                        ABORT_CHECKARGS("Expected an integer.");

                    break;

                case 'n':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_FLOAT)
                        ABORT_CHECKARGS("Expected a float.");

                    break;

                case 's':
                case 'S':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_STRING)
                        ABORT_CHECKARGS("Expected a string.");

                    if (buf[currpos] == 'S' && !arg[stackpos].data.string)
                        ABORT_CHECKARGS("Expected a non-null string.");

                    break;

                case 'd':
                case 'p':
                case 'r':
                case 't':
                case 'e':
                case 'f':
                case 'D':
                case 'P':
                case 'R':
                case 'T':
                case 'E':
                case 'F':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_OBJECT)
                        ABORT_CHECKARGS("Expected a dbref.");

                    ref = arg[stackpos].data.objref;
                    if (ref >= db_top || ref < HOME)
                        ABORT_CHECKARGS("Invalid dbref.");

                    switch (buf[currpos]) {
                        case 'D':
                            if ((ref < 0) && (ref != HOME))
                                ABORT_CHECKARGS("Invalid dbref.");

                            if (Typeof(ref) == TYPE_GARBAGE)
                                ABORT_CHECKARGS("Invalid dbref.");

                        case 'd':
                            if (ref < HOME)
                                ABORT_CHECKARGS("Invalid dbref.");

                            break;

                        case 'P':
                            if (ref < 0)
                                ABORT_CHECKARGS("Expected player dbref.");

                        case 'p':
                            if ((ref >= 0) && (Typeof(ref) != TYPE_PLAYER))
                                ABORT_CHECKARGS("Expected player dbref.");

                            if (ref == HOME)
                                ABORT_CHECKARGS("Expected player dbref.");

                            break;

                        case 'R':
                            if ((ref < 0) && (ref != HOME))
                                ABORT_CHECKARGS("Expected room dbref.");

                        case 'r':
                            if ((ref >= 0) && (Typeof(ref) != TYPE_ROOM))
                                ABORT_CHECKARGS("Expected room dbref.");

                            break;

                        case 'T':
                            if (ref < 0)
                                ABORT_CHECKARGS("Expected thing dbref.");

                        case 't':
                            if ((ref >= 0) && (Typeof(ref) != TYPE_THING))
                                ABORT_CHECKARGS("Expected thing dbref.");

                            if (ref == HOME)
                                ABORT_CHECKARGS("Expected thing dbref.");

                            break;

                        case 'E':
                            if (ref < 0)
                                ABORT_CHECKARGS("Expected exit dbref.");

                        case 'e':
                            if ((ref >= 0) && (Typeof(ref) != TYPE_EXIT))
                                ABORT_CHECKARGS("Expected exit dbref.");

                            if (ref == HOME)
                                ABORT_CHECKARGS("Expected exit dbref.");

                            break;

                        case 'F':
                            if (ref < 0)
                                ABORT_CHECKARGS("Expected program dbref.");

                        case 'f':
                            if ((ref >= 0) && (Typeof(ref) != TYPE_PROGRAM))
                                ABORT_CHECKARGS("Expected program dbref.");

                            if (ref == HOME)
                                ABORT_CHECKARGS("Expected program dbref.");

                            break;
                    }

                    break;

                case '?':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    break;

                case 'l':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_LOCK)
                        ABORT_CHECKARGS("Expected a lock boolean expression.");

                    break;

                case 'v':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_VAR &&
                            arg[stackpos].type != PROG_LVAR &&
                            arg[stackpos].type != PROG_SVAR) {
                        ABORT_CHECKARGS("Expected a variable.");
                    }

                    break;

                case 'a':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_ADD)
                        ABORT_CHECKARGS("Expected a function address.");

                    break;

                case 'x':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_ARRAY ||
                            !arg[stackpos].data.array ||
                            arg[stackpos].data.array->type != ARRAY_DICTIONARY)
                        ABORT_CHECKARGS("Expected a dictionary array.");

                    break;

                case 'y':
                case 'Y':
                    if (stackpos < 0)
                        ABORT_CHECKARGS("Stack underflow.");

                    if (arg[stackpos].type != PROG_ARRAY)
                        ABORT_CHECKARGS("Expected an array.");

                    if (buf[currpos] == 'Y' && arg[stackpos].data.array &&
                            arg[stackpos].data.array->type == ARRAY_DICTIONARY)
                        ABORT_CHECKARGS("Expected a non-dictionary array.");

                    break;

                case ' ':
                    /* this is meaningless space.  Ignore it. */
                    stackpos++;

                    break;

                default:
                    abort_interp("Unkown argument type in expression.");
            }

            currpos--;          /* decrement string index */
            stackpos--;         /* move on to next stack item down */

            /* are we expecting a repeat of the last argument or range? */
            if ((rngstktop > 0) && (rngstktyp[rngstktop - 1] == itsarepeat)) {
                /* is the repeat is done yet? */
                if (--rngstkcnt[rngstktop - 1] > 0) {
                    /* no, repeat last argument or range */
                    currpos = rngstkpos[rngstktop - 1];
                } else {
                    /* yes, we're done with this repeat */
                    rngstktop--;
                }
            }
        }
    }                           /* while loop */

    /* Oops. still haven't finished a range or repeat expression. */
    if (rngstktop > 0)
        abort_interp("Badly formed argument expression.");

    CLEAR(oper1);
}

#undef ABORT_CHECKARGS

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
void
prim_mode(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = fr->multitask;
    PushInt(result);
}

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
void
prim_mark(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    PushMark();
}

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
void
prim_findmark(PRIM_PROTOTYPE)
{
    int depth, height, count;

    CHECKOP(0);

    depth = 1;
    height = *top - 1;

    while (height >= 0 && arg[height].type != PROG_MARK) {
        height--;
        depth++;
    }

    count = depth - 1;

    if (height < 0)
        abort_interp("No matching mark on stack!");

    if (depth > 1) {
        temp2 = arg[*top - depth];

        for (; depth > 1; depth--)
            arg[*top - depth] = arg[*top - depth + 1];

        arg[*top - 1] = temp2;
    }

    EXPECT_POP_STACK(1);

    oper1 = POP();
    CLEAR(oper1);

    PushInt(count);
}

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
void
prim_preempt(PRIM_PROTOTYPE)
{
    fr->multitask = PREEMPT;
}

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
void
prim_foreground(PRIM_PROTOTYPE)
{
    if (fr->been_background)
        abort_interp("Cannot FOREGROUND a BACKGROUNDed program.");

    fr->multitask = FOREGROUND;
}

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
void
prim_background(PRIM_PROTOTYPE)
{
    fr->multitask = BACKGROUND;
    fr->been_background = 1;
    fr->writeonly = 1;
}

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
void
prim_setmode(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid argument type.");

    result = oper1->data.number;

    switch (result) {
        case BACKGROUND:
            fr->been_background = 1;
            fr->writeonly = 1;
            break;

        case FOREGROUND:
            if (fr->been_background)
                abort_interp("Cannot FOREGROUND a BACKGROUNDed program.");

            break;

        case PREEMPT:
            break;

        default:
            abort_interp("Invalid mode.");
    }

    fr->multitask = result;

    CLEAR(oper1);
}

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
void
prim_interp(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper3 = NULL;  /* prevents re-entrancy issues! */

    struct inst *rv = NULL;
    char buf[BUFFER_LEN];
    struct frame *tmpfr;

    CHECKOP(3);
    oper3 = POP();              /* string -- top stack argument */
    oper2 = POP();              /* dbref  --  trigger */
    oper1 = POP();              /* dbref  --  Program to run */

    if (!valid_object(oper1) || Typeof(oper1->data.objref) != TYPE_PROGRAM)
        abort_interp("Bad program reference. (1)");

    if (!valid_object(oper2))
        abort_interp("Bad object. (2)");

    if (oper3->type != PROG_STRING)
        abort_interp("Expected a string. (3)");

    if ((mlev < 3) && !permissions(ProgUID, oper2->data.objref))
        abort_interp("Permission denied.");

    if (fr->level > 8)
        abort_interp("Interp call loops not allowed.");

    CHECKREMOTE(oper2->data.objref);

    strcpyn(buf, sizeof(buf), match_args);
    strcpyn(match_args, sizeof(match_args), DoNullInd(oper3->data.string));
    tmpfr = interp(fr->descr, player, LOCATION(player), oper1->data.objref,
                   oper2->data.objref, PREEMPT, STD_HARDUID, 0);
    if (tmpfr) {
        rv = interp_loop(player, oper1->data.objref, tmpfr, 1);
    }

    strcpyn(match_args, sizeof(match_args), buf);

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    if (rv) {
        if (rv->type < PROG_STRING) {
            push(arg, top, rv->type, &rv->data.number);
        } else {
            push(arg, top, rv->type, rv->data.string);
        }
    } else {
        PushNullStr;
    }
}

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
void
prim_for(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();              /* step */
    oper2 = POP();              /* end */
    oper1 = POP();              /* start */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Starting count expected. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Ending count expected. (2)");

    if (oper3->type != PROG_INTEGER)
        abort_interp("Step count expected. (3)");

    if (fr->fors.top >= STACK_SIZE)
        abort_interp("Too many nested FOR loops.");

    fr->fors.top++;
    fr->fors.st = push_for(fr->fors.st);

    copyinst(oper1, &fr->fors.st->cur);
    copyinst(oper2, &fr->fors.st->end);

    fr->fors.st->step = oper3->data.number;
    fr->fors.st->didfirst = 0;

    if (fr->trys.st)
        fr->trys.st->for_count++;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

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
void
prim_foreach(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
        abort_interp("Array argument expected. (1)");

    if (fr->fors.top >= STACK_SIZE)
        abort_interp("Too many nested FOR loops.");

    fr->fors.top++;
    fr->fors.st = push_for(fr->fors.st);
    fr->fors.st->cur.type = PROG_INTEGER;
    fr->fors.st->cur.line = 0;
    fr->fors.st->cur.data.number = 0;

    if (fr->trys.st)
        fr->trys.st->for_count++;

    copyinst(oper1, &fr->fors.st->end);

    fr->fors.st->step = 0;
    fr->fors.st->didfirst = 0;

    CLEAR(oper1);
}

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
void
prim_foriter(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    if (!fr->fors.st)
        abort_interp("Internal error; FOR stack underflow.");

    if (fr->fors.st->end.type == PROG_ARRAY) {
        stk_array *arr = fr->fors.st->end.data.array;

        if (fr->fors.st->didfirst) {
            result = array_next(arr, &fr->fors.st->cur);
        } else {
            result = array_first(arr, &fr->fors.st->cur);
            fr->fors.st->didfirst = 1;
        }

        if (result) {
            array_data *val = array_getitem(arr, &fr->fors.st->cur);

            if (val) {
                CHECKOFLOW(2);
                PushInst(&fr->fors.st->cur);    /* push key onto stack */
                PushInst(val);  /* push value onto stack */
                tmp = 1;        /* tell following IF to NOT branch out of loop */
            } else {
                tmp = 0;        /* tell following IF to branch out of loop */
            }
        } else {
            fr->fors.st->cur.type = PROG_INTEGER;
            fr->fors.st->cur.line = 0;
            fr->fors.st->cur.data.number = 0;

            tmp = 0;            /* tell following IF to branch out of loop */
        }
    } else {
        int cur = fr->fors.st->cur.data.number;
        int end = fr->fors.st->end.data.number;

        tmp = fr->fors.st->step > 0;

        if (tmp)
            tmp = !(cur > end);
        else
            tmp = !(cur < end);

        if (tmp) {
            CHECKOFLOW(1);

            result = cur;
            fr->fors.st->cur.data.number += fr->fors.st->step;
            PushInt(result);
        }
    }

    CHECKOFLOW(1);
    PushInt(tmp);
}

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
void
prim_forpop(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    if (!(fr->fors.top))
        abort_interp("Internal error; FOR stack underflow.");

    CLEAR(&fr->fors.st->cur);
    CLEAR(&fr->fors.st->end);

    if (fr->trys.st)
        fr->trys.st->for_count--;

    fr->fors.top--;
    fr->fors.st = pop_for(fr->fors.st);
}

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
void
prim_trypop(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    if (!(fr->trys.top))
        abort_interp("Internal error; TRY stack underflow.");

    fr->trys.top--;
    fr->trys.st = pop_try(fr->trys.st);
}

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
void
prim_reverse(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid argument type.");

    tmp = oper1->data.number;   /* Depth on stack */

    if (tmp < 0)
        abort_interp("Argument must be positive.");

    EXPECT_WRITE_STACK(tmp);

    if (tmp > 0) {
        for (int i = 0; i < (tmp / 2); i++) {
            temp2 = arg[*top - (tmp - i)];
            arg[*top - (tmp - i)] = arg[*top - (i + 1)];
            arg[*top - (i + 1)] = temp2;
        }
    }

    CLEAR(oper1);
}

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
void
prim_lreverse(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid argument type.");

    tmp = oper1->data.number;   /* Depth on stack */
    if (tmp < 0)
        abort_interp("Argument must be positive.");

    EXPECT_WRITE_STACK(tmp);

    if (tmp > 0) {
        for (int i = 0; i < (tmp / 2); i++) {
            temp2 = arg[*top - (tmp - i)];
            arg[*top - (tmp - i)] = arg[*top - (i + 1)];
            arg[*top - (i + 1)] = temp2;
        }
    }

    CLEAR(oper1);
    PushInt(tmp);
}

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
void
prim_secure_sysvars(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    CLEAR(&(fr->variables[0]));
    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = player;

    CLEAR(&(fr->variables[1]));
    fr->variables[1].type = PROG_OBJECT;
    fr->variables[1].data.objref = LOCATION(player);

    CLEAR(&(fr->variables[2]));
    fr->variables[2].type = PROG_OBJECT;
    fr->variables[2].data.objref = fr->trig;

    CLEAR(&(fr->variables[3]));
    fr->variables[3].type = PROG_STRING;
    fr->variables[3].data.string = fr->cmd;

    if (fr->cmd)
        fr->cmd->links++;
}
