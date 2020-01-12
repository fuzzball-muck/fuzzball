/** @file p_array.c
 *
 * Implementation of Array Operations for MUF
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "db.h"
#include "dbsearch.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

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
 * @var these are used all over ... not sure why these aren't local variables
 *      This makes things very not threadsafe.
 */
static struct inst temp1, temp2;

/**
 * @private
 * @var to store the result which is not a local variable for some reason
 *      This makes things very not threadsafe.
 */
static int result;

/**
 * @private
 * @var I don't really have any rationale for this one :)  Just makes things
 *      not threadsafe for fun.
 */
static dbref ref;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

/**
 * Implementtion of MUF ARRAY_MAKE
 *
 * Create a "packed" (list) array from a variable number of items on the
 * stack.  The topmost stack item is the count of items to add to the
 * array.
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
prim_array_make(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");

    result = oper1->data.number;

    if (result < 0)
        abort_interp("Item count must be non-negative.");

    CLEAR(oper1);
    nargs = 0;

    if (*top < result)
        abort_interp("Stack underflow.");

    temp1.type = PROG_INTEGER;
    temp1.line = 0;
    nu = new_array_packed(result, fr->pinning);

    for (int i = result; i-- > 0;) {
        temp1.data.number = i;
        CHECKOP(1);
        oper1 = POP();
        array_setitem(&nu, &temp1, oper1);
        CLEAR(oper1);
    }

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_MAKE_DICT
 *
 * Create a dictionary array from a variable number of key/values on the
 * stack.  The topmost item on the stack is the number of key/value pairs
 * (so the actual number of items that will be consumed from the stack is
 * 2 times this number).
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
prim_array_make_dict(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");

    result = oper1->data.number;

    if (result < 0)
        abort_interp("Item count must be positive.");

    CLEAR(oper1);
    nargs = 0;

    if (*top < (2 * result))
        abort_interp("Stack underflow.");

    nu = new_array_dictionary(fr->pinning);

    for (int i = result; i-- > 0;) {
        CHECKOP(2);
        oper1 = POP();  /* val */
        oper2 = POP();  /* key */

        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING) {
            array_free(nu);
            abort_interp("Keys must be integers or strings.");
        }

        array_setitem(&nu, oper2, oper1);
        CLEAR(oper1);
        CLEAR(oper2);
    }

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_EXPLODE
 *
 * Explodes the array into a "stack range", meaning, key/value pairs
 * followed by a count of pairs (which is half of the total number of
 * stack entries) on the top of the stack.
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
prim_array_explode(PRIM_PROTOTYPE)
{
    stk_array *arr;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    result = array_count(oper1->data.array);
    CHECKOFLOW(((2 * result) + 1));

    copyinst(oper1, &temp2);
    arr = temp2.data.array;
    CLEAR(oper1);

    if (array_first(arr, &temp1)) {
        do {
            copyinst(&temp1, &arg[((*top)++)]);
            oper2 = array_getitem(arr, &temp1);
            copyinst(oper2, &arg[((*top)++)]);
            oper2 = NULL;
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp2);
    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_VALS
 *
 * Extracts all the values of the array onto the stack, with a total count
 * of extracted items at the top of the stack.  Keys are ignored.
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
prim_array_vals(PRIM_PROTOTYPE)
{
    stk_array *arr;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    copyinst(oper1, &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW((result + 1));
    CLEAR(oper1);

    if (array_first(arr, &temp1)) {
        do {
            oper2 = array_getitem(arr, &temp1);
            copyinst(oper2, &arg[((*top)++)]);
            oper2 = NULL;
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp2);
    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_KEYS
 *
 * Extracts all the keys of the array onto the stack, with a total count
 * of extracted items at the top of the stack.  Values are ignored.
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
prim_array_keys(PRIM_PROTOTYPE)
{
    stk_array *arr;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    copyinst(oper1, &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW((result + 1));
    CLEAR(oper1);

    if (array_first(arr, &temp1)) {
        do {
            copyinst(&temp1, &arg[((*top)++)]);
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp2);
    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_COUNT
 *
 * Consumes an array and pushes its count onto the stack.
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
prim_array_count(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    result = array_count(oper1->data.array);

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_FIRST
 *
 * Consumes an array and pushes its first index onto the stack, and a boolean
 * which is false if there are no items in the array.
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
prim_array_first(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();  /* arr  Array */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    temp1.type = PROG_INTEGER;
    temp1.data.number = 0;
    result = array_first(oper1->data.array, &temp1);

    CLEAR(oper1);
    nargs = 0;
    CHECKOFLOW(2);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }

    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_LAST
 *
 * Consumes an array and pushes its last index onto the stack, and a boolean
 * which is false if there are no items in the array.
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
prim_array_last(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();  /* arr  Array */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    temp1.type = PROG_INTEGER;
    temp1.data.number = 0;
    result = array_last(oper1->data.array, &temp1);

    CLEAR(oper1);
    nargs = 0;
    CHECKOFLOW(2);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }

    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_PREV
 *
 * Consumes an array and index, and pushes the previous index onto the stack,
 * and a boolean which is false if there are no previous item.
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
prim_array_prev(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();  /* ???  previous index */
    oper2 = POP();  /* arr  Array */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array.(1)");

    copyinst(oper1, &temp1);
    result = array_prev(oper2->data.array, &temp1);

    CLEAR(oper1);
    CLEAR(oper2);

    CHECKOFLOW(2);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
        CLEAR(&temp1);
    } else {
        result = 0;
        PushInt(result);
    }

    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_NEXT
 *
 * Consumes an array and index, and pushes the next index onto the stack,
 * and a boolean which is false if there are no next item.
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
prim_array_next(PRIM_PROTOTYPE)
{

    CHECKOP(2);
    oper1 = POP();  /* ???  previous index */
    oper2 = POP();  /* arr  Array */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array.(1)");

    copyinst(oper1, &temp1);
    result = array_next(oper2->data.array, &temp1);

    CLEAR(oper1);
    CLEAR(oper2);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
        CLEAR(&temp1);
    } else {
        result = 0;
        PushInt(result);
    }

    PushInt(result);
}

/**
 * Implementtion of MUF ARRAY_GETITEM
 *
 * Consumes an array and index, and returns the item there or 0 if nothing
 * in the index.
 *
 * @see array_getitem
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
prim_array_getitem(PRIM_PROTOTYPE)
{
    struct inst *in;
    struct inst temp;

    CHECKOP(2);
    oper1 = POP();  /* ???  index */
    oper2 = POP();  /* arr  Array */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    in = array_getitem(oper2->data.array, oper1);

    /* copy data to a temp inst before clearing the containing array */
    if (in) {
        copyinst(in, &temp);
    } else {
        temp.type = PROG_INTEGER;
        temp.data.number = 0;
    }

    CLEAR(oper1);
    CLEAR(oper2);

    /* copy data to stack, then clear temp inst */
    copyinst(&temp, &arg[(*top)++]);
    CLEAR(&temp);
}

/**
 * Implementtion of MUF ARRAY_SETITEM
 *
 * Consumes any kind of stack data, an array, and an index and returns
 * an array with the stack data set to 'index', overwriting whatever may
 * already be there.
 *
 * @see array_setitem
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
prim_array_setitem(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(3);
    oper1 = POP();  /* ???  index to store at */
    oper2 = POP();  /* arr  Array to store in */
    oper3 = POP();  /* ???  item to store     */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");

    result = array_setitem(&oper2->data.array, oper1, oper3);

    if (result < 0)
        abort_interp("Index out of array bounds. (3)");

    nu = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_SETITEM
 *
 * Consumes any kind of stack item and a list array and returns the list
 * array with the item appended on it.
 *
 * @see array_appenditem
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
prim_array_appenditem(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(2);
    oper2 = POP();  /* arr  Array to store in */
    oper1 = POP();  /* ???  item to store     */

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");

    if (oper2->data.array && oper2->data.array->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (2)");

    result = array_appenditem(&oper2->data.array, oper1);

    if (result == -1)
        abort_interp("Internal Error!");

    nu = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_INSERTITEM
 *
 * Consumes any kind of stack data, an array, and an index and returns
 * an array with the stack data set to 'index'.
 *
 * There is functionally very little (no?) difference between this and
 * ARRAY_SETITEM
 *
 * @see array_insertitem
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
prim_array_insertitem(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(3);
    oper1 = POP();  /* ???  index to store at */
    oper2 = POP();  /* arr  Array to store in */
    oper3 = POP();  /* ???  item to store     */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");

    result = array_insertitem(&oper2->data.array, oper1, oper3);

    if (result < 0)
        abort_interp("Index out of array bounds. (3)");

    nu = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_GETRANGE
 *
 * Consumes an array and a start/end index, and puts an array on the stack
 * which is the range of items from start to end (inclusive)
 *
 * @see array_getrange
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
prim_array_getrange(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(3);
    oper1 = POP();  /* int  range end item */
    oper2 = POP();  /* int  range start item */
    oper3 = POP();  /* arr  Array   */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");

    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    nu = array_getrange(oper3->data.array, oper2, oper1, fr->pinning);

    if (!nu)
        nu = new_array_packed(0, fr->pinning);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_SETRANGE
 *
 * Consumes an array, an index, and a second array.  Sets items from the
 * second array into the first array starting with the given index.  Puts
 * the resulting array onto the stack.
 *
 * @see array_getrange
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
prim_array_setrange(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(3);
    oper1 = POP();  /* arr  array to insert */
    oper2 = POP();  /* ???  starting pos for lists */
    oper3 = POP();  /* arr  Array   */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (3)");

    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_setrange(&oper3->data.array, oper2, oper1->data.array);

    if (result < 0)
        abort_interp("Index out of array bounds. (2)");

    nu = oper3->data.array;
    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_INSERTRANGE
 *
 * Consumes an array, an index, and a second array.  Inserts items from the
 * second array into the first array starting with the given index.  Puts
 * the resulting array onto the stack.
 *
 * @see array_insertrange
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
prim_array_insertrange(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(3);
    oper1 = POP();  /* arr  array to insert */
    oper2 = POP();  /* ???  starting pos for lists */
    oper3 = POP();  /* arr  Array   */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (3)");

    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_insertrange(&oper3->data.array, oper2, oper1->data.array);

    if (result < 0)
        abort_interp("Index out of array bounds. (2)");

    nu = oper3->data.array;
    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_DELITEM
 *
 * Consumes an array and an index, and returns an array with that item
 * deleted
 *
 * @see array_delitem
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
prim_array_delitem(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(2);
    oper1 = POP();  /* int  item to delete */
    oper2 = POP();  /* arr  Array   */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_delitem(&oper2->data.array, oper1);

    if (result < 0)
        abort_interp("Bad array index specified.");

    nu = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_DELRANGE
 *
 * Consumes an array and two indexes, and deletes all items between the
 * two, inclusive.
 *
 * @see array_delrange
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
prim_array_delrange(PRIM_PROTOTYPE)
{
    stk_array *nu;

    CHECKOP(3);
    oper1 = POP();  /* int  range end item */
    oper2 = POP();  /* int  range start item */
    oper3 = POP();  /* arr  Array   */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");

    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_delrange(&oper3->data.array, oper2, oper1);

    if (result < 0)
        abort_interp("Bad array range specified.");

    nu = oper3->data.array;
    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_CUT
 *
 * Consumes an array and an index. Returns two arrays, which is the original
 * array cut at the given position.  The second array includes the index
 * indicated, and the first array has all the items before it.
 *
 * @see array_delrange
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
prim_array_cut(PRIM_PROTOTYPE)
{
    stk_array *nu1 = NULL;
    stk_array *nu2 = NULL;
    struct inst temps;
    struct inst tempc;
    struct inst tempe;

    CHECKOP(2);
    oper2 = POP();  /* int  position */
    oper1 = POP();  /* arr  Array   */

    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    temps.type = PROG_INTEGER;
    temps.data.number = 0;
    result = array_first(oper1->data.array, &temps);

    if (result) {
        copyinst(oper2, &tempc);
        result = array_prev(oper1->data.array, &tempc);

        if (result) {
            nu1 = array_getrange(oper1->data.array, &temps, &tempc, fr->pinning);
            CLEAR(&tempc);
        }

        result = array_last(oper1->data.array, &tempe);

        if (result) {
            nu2 = array_getrange(oper1->data.array, oper2, &tempe, fr->pinning);
            CLEAR(&tempe);
        }

        CLEAR(&temps);
    }

    if (!nu1)
        nu1 = new_array_packed(0, fr->pinning);

    if (!nu2)
        nu2 = new_array_packed(0, fr->pinning);

    CLEAR(oper1);
    CLEAR(oper2);

    PushArrayRaw(nu1);
    PushArrayRaw(nu2);
}

/**
 * Implementtion of MUF ARRAY_NUNION
 *
 * Returns a list array containing the union of values of all
 * the given arrays in a stackrange.  If a value is found in any of the
 * arrays, then it will be in the resultant list, though duplicates and
 * keys will be discarded.
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
prim_array_n_union(PRIM_PROTOTYPE)
{
    stk_array *new_union;
    stk_array *new_mash;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");

    result = oper1->data.number;

    if (result < 0)
        abort_interp("Item count must be positive.");

    CLEAR(oper1);
    nargs = 0;

    if (*top < result)
        abort_interp("Stack underflow.");

    if (result > 0) {
        new_mash = new_array_dictionary(fr->pinning);

        for (int num_arrays = 0; num_arrays < result; num_arrays++) {
            CHECKOP(1);
            oper1 = POP();

            if (oper1->type != PROG_ARRAY) {
                array_free(new_mash);
                abort_interp("Argument not an array.");
            }

            array_mash(oper1->data.array, &new_mash, 1);
            CLEAR(oper1);
        }

        new_union = array_demote_only(new_mash, 1, fr->pinning);
        array_free(new_mash);
    } else {
        new_union = new_array_packed(0, fr->pinning);
    }

    PushArrayRaw(new_union);
}

/**
 * Implementtion of MUF ARRAY_NINTERSECT
 *
 * Returns a list array containing the intersction of values of all
 * the given arrays in a stackrange.  If a value is found in all of the
 * arrays, then it will be in the resultant list, though duplicates and
 * keys will be discarded.
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
prim_array_n_intersection(PRIM_PROTOTYPE)
{
    stk_array *new_union;
    stk_array *new_mash;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");

    result = oper1->data.number;

    if (result < 0)
        abort_interp("Item count must be positive.");

    CLEAR(oper1);
    nargs = 0;

    EXPECT_POP_STACK(result);

    if (result > 0) {
        new_mash = new_array_dictionary(fr->pinning);

        for (int num_arrays = 0; num_arrays < result; num_arrays++) {
            oper1 = POP();

            if (oper1->type != PROG_ARRAY) {
                array_free(new_mash);
                CLEAR(oper1);
                abort_interp("Argument not an array.");
            }

            array_mash(oper1->data.array, &new_mash, 1);
            CLEAR(oper1);
        }

        new_union = array_demote_only(new_mash, result, fr->pinning);
        array_free(new_mash);
    } else {
        new_union = new_array_packed(0, fr->pinning);
    }

    PushArrayRaw(new_union);
}

/**
 * Implementtion of MUF ARRAY_NDIFF
 *
 * Returns a list array containing the difference of values of all
 * the given arrays in a stackrange.  It returns all values from the
 * topmost array that weren't found in any of the other arrays.
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
prim_array_n_difference(PRIM_PROTOTYPE)
{
    stk_array *new_union;
    stk_array *new_mash;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");

    result = oper1->data.number;

    if (result < 0)
        abort_interp("Item count must be positive.");

    CLEAR(oper1);
    nargs = 0;

    EXPECT_POP_STACK(result);

    if (result > 0) {
        new_mash = new_array_dictionary(fr->pinning);

        oper1 = POP();

        if (oper1->type != PROG_ARRAY) {
            array_free(new_mash);
            CLEAR(oper1);
            abort_interp("Argument not an array.");
        }

        array_mash(oper1->data.array, &new_mash, 1);
        CLEAR(oper1);

        for (int num_arrays = 1; num_arrays < result; num_arrays++) {
            oper1 = POP();

            if (oper1->type != PROG_ARRAY) {
                array_free(new_mash);
                abort_interp("Argument not an array.");
            }

            array_mash(oper1->data.array, &new_mash, -1);
            CLEAR(oper1);
        }

        new_union = array_demote_only(new_mash, 1, fr->pinning);
        array_free(new_mash);
    } else {
        new_union = new_array_packed(0, fr->pinning);
    }

    PushArrayRaw(new_union);
}

/**
 * Implementtion of MUF ARRAY_NOTIFY
 *
 * Consumes two arrays on the stack.  The lower array must be a list of
 * strings and the top array is the list of dbrefs.  All messages in the
 * first list are sent to all people in the second list.
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
prim_array_notify(PRIM_PROTOTYPE)
{
    stk_array *strarr;
    stk_array *refarr;
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL;
    struct inst temp1, temp2;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (1)");

    if (!array_is_homogenous(oper1->data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (1)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array of dbrefs. (2)");

    if (!array_is_homogenous(oper2->data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (2)");

    strarr = oper1->data.array;
    refarr = oper2->data.array;

    if (array_first(strarr, &temp2)) {
        do {
            oper4 = array_getitem(strarr, &temp2);

            if (tp_force_mlev1_name_notify && mlev < 2) {
                prefix_message(buf, DoNullInd(oper4->data.string), NAME(player),
                               BUFFER_LEN, 1);
            } else {
                strcpyn(buf, sizeof(buf), DoNullInd(oper4->data.string));
            }

            if (array_first(refarr, &temp1)) {
                do {
                    oper3 = array_getitem(refarr, &temp1);

                    notify_listeners(player, program, oper3->data.objref,
                                     LOCATION(oper3->data.objref), buf, 1);

                    oper3 = NULL;
                } while (array_next(refarr, &temp1));
            }

            oper4 = NULL;
        } while (array_next(strarr, &temp2));
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementtion of MUF ARRAY_REVERSE
 *
 * Takes a list array and reverses the order of the elements in it.
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
prim_array_reverse(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nu;

    CHECKOP(1);
    oper1 = POP();  /* arr  Array   */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    arr = oper1->data.array;

    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array.");

    result = array_count(arr);
    nu = new_array_packed(result, fr->pinning);

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_INTEGER;

    for (int i = 0; i < result; i++) {
        temp1.data.number = i;
        temp2.data.number = (result - i) - 1;
        array_setitem(&nu, &temp1, array_getitem(arr, &temp2));
    }

    CLEAR(oper1);
    PushArrayRaw(nu);
}

/**
 * @private
 * @var Variable to keep track of if we are sorting case insensitively.
 *      This is NOT threadsafe
 */
static int sortflag_caseinsens = 0;

/**
 * @private
 * @var Variable to keep track of if we are sorting in descending order
 *      This is NOT threadsafe
 */
static int sortflag_descending = 0;

/**
 * @private
 * @var Variable to keep track of if we are doing an indexed stort, this is
 *      the index we are using.
 *      This is NOT threadsafe
 */
static struct inst *sortflag_index = NULL;

/**
 * This is the shared guts of the sort routines to compare two stack items
 *
 * This is passed into qsort, which is why the calling signature is weird.
 * x and y are pointers to struct inst, and this returns -1, 0, or 1 based
 * on if x is less than y, equal to y, or greater than y respectively.
 *
 * This is mostly a thin wrapper around array_tree_compare with some extra
 * logic around it for the case of doing key-based comparisons (if
 * sortflag_index is set).  If sortflag_index is set, then we will fall
 * into that logic instead.
 *
 * @see array_tree_compare
 *
 * @private
 * @param x a pointer to a pointer to a struct inst
 * @param y a pointer to a pointer to a struct inst
 * @return integer either -1 if x < y, 0 if x == y, 1 if x > y
 */
static int
sortcomp_generic(const void *x, const void *y)
{
    struct inst *a;
    struct inst *b;

    if (!sortflag_descending) {
        a = *(struct inst **) x;
        b = *(struct inst **) y;
    } else {
        a = *(struct inst **) y;
        b = *(struct inst **) x;
    }

    if (sortflag_index) {
        /* This should only be set if comparators are all arrays. */
        a = array_getitem(a->data.array, sortflag_index);
        b = array_getitem(b->data.array, sortflag_index);

        if (!a && !b) {
            return 0;
        } else if (!a) {
            return -1;
        } else if (!b) {
            return 1;
        }
    }

    return (array_tree_compare(a, b, (sortflag_caseinsens ? 0 : 1)));
}

/**
 * This is to handle the case of a 'shuffle' sort which does randomization.
 *
 * This is passed into qsort, which is why the calling signature is weird.
 * x and y are pointers to struct inst, and this returns a random value that
 * is valid for sorting.
 *
 * @private
 * @param x a pointer to a pointer to a struct inst
 * @param y a pointer to a pointer to a struct inst
 * @return integer random sort result
 */
static int
sortcomp_shuffle(const void *x, const void *y)
{
    return (((RANDOM() >> 8) % 5) - 2);
}

/**
 * Implementtion of MUF ARRAY_SORT
 *
 * Takes an array to sort and an integer sort type, which can be one
 * of the following (bit OR'd together)
 *
 * 0 - case sensitive and ascending
 * 1 - SORTTYPE_CASEINSENS - Case insensitive sort
 * 2 - SORTTYPE_DESCENDING - Sort in reversed order
 * 4 - SORTTYPE_SHUFFLE - Sort randomly
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
prim_array_sort(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nu;
    size_t count;
    int (*comparator) (const void *, const void *);
    struct inst **tmparr = NULL;

    CHECKOP(2);
    oper2 = POP();  /* int  sort_type   */
    oper1 = POP();  /* arr  Array   */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    arr = oper1->data.array;

    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected integer argument to specify sort type. (2)");

    temp1.type = PROG_INTEGER;
    count = (size_t)array_count(arr);
    nu = new_array_packed(count, fr->pinning);
    tmparr = malloc(count * sizeof(struct inst *));

    for (unsigned int i = 0; i < count; i++) {
        temp1.data.number = i;
        tmparr[i] = array_getitem(arr, &temp1);
    }

    /*
     * There was a note here about mutexes -- that note was actually wrong.
     * Super wrong.  We'd be holding a mutex while the array sorts which is
     * really a bad idea.  The solution is to not use these global variables.
     *
     * The problem is we're using qsort, and so we are using globals to pass
     * into qsort.
     *
     * TODO: change this to use qsort_r -- this provides a third
     *       "data" argument that can be used to pass arbitrary arguments
     *       in.  We can pass in a struct that has these flags and get rid
     *       of the stupid globals.
     */
    sortflag_caseinsens = (oper2->data.number & SORTTYPE_CASEINSENS) ? 1 : 0;
    sortflag_descending = (oper2->data.number & SORTTYPE_DESCENDING) ? 1 : 0;
    sortflag_index = NULL;

    if ((oper2->data.number & SORTTYPE_SHUFFLE)) {
        comparator = sortcomp_shuffle;
    } else {
        comparator = sortcomp_generic;
    }

    qsort(tmparr, count, sizeof(struct inst *), comparator);

    for (unsigned int i = 0; i < count; i++) {
        temp1.data.number = i;
        array_setitem(&nu, &temp1, tmparr[i]);
    }

    free(tmparr);

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nu);
}

/**
 * Implementtion of MUF ARRAY_SORT_INDEXED
 *
 * Takes an array of arrays to sort, a sort type, and a key/index to sort
 * by.  Sorts arrays within the 'parent' array using the value of the key
 * or index provided.  Only works if that key contains an integer.
 *
 * 0 - case sensitive and ascending
 * 1 - SORTTYPE_CASEINSENS - Case insensitive sort
 * 2 - SORTTYPE_DESCENDING - Sort in reversed order
 * 4 - SORTTYPE_SHUFFLE - Sort randomly
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
prim_array_sort_indexed(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nu;
    size_t count;
    int (*comparator) (const void *, const void *);
    struct inst **tmparr = NULL;

    CHECKOP(3);
    oper3 = POP();  /* idx  index_key   */
    oper2 = POP();  /* int  sort_type   */
    oper1 = POP();  /* arr  Array   */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    arr = oper1->data.array;

    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (1)");

    if (!array_is_homogenous(arr, PROG_ARRAY))
        abort_interp("Argument must be a list array of arrays. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected integer argument to specify sort type. (2)");

    if (oper3->type != PROG_INTEGER && oper3->type != PROG_STRING)
        abort_interp("Index argument not an integer or string. (3)");

    temp1.type = PROG_INTEGER;
    count = (size_t)array_count(arr);
    nu = new_array_packed(count, fr->pinning);
    tmparr = malloc(count * sizeof(struct inst *));

    for (unsigned int i = 0; i < count; i++) {
        temp1.data.number = i;
        tmparr[i] = array_getitem(arr, &temp1);
    }

    /*
     * There was a note here about mutexes -- that note was actually wrong.
     * Super wrong.  We'd be holding a mutex while the array sorts which is
     * really a bad idea.  The solution is to not use these global variables.
     *
     * The problem is we're using qsort, and so we are using globals to pass
     * into qsort.
     *
     * TODO: change this to use qsort_r -- this provides a third
     *       "data" argument that can be used to pass arbitrary arguments
     *       in.  We can pass in a struct that has these flags and get rid
     *       of the stupid globals.
     */
    sortflag_caseinsens = (oper2->data.number & SORTTYPE_CASEINSENS) ? 1 : 0;
    sortflag_descending = (oper2->data.number & SORTTYPE_DESCENDING) ? 1 : 0;
    sortflag_index = oper3;

    if ((oper2->data.number & SORTTYPE_SHUFFLE)) {
        comparator = sortcomp_shuffle;
    } else {
        comparator = sortcomp_generic;
    }

    qsort(tmparr, count, sizeof(struct inst *), comparator);

    for (unsigned int i = 0; i < count; i++) {
        temp1.data.number = i;
        array_setitem(&nu, &temp1, tmparr[i]);
    }

    free(tmparr);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushArrayRaw(nu);
}

void
prim_array_get_propdirs(PRIM_PROTOTYPE)
{
    stk_array *nu;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PropPtr propadr, pptr;
    PropPtr prptr;
    int count = 0;
    int len;

    /* dbref strPropDir -- array */
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Permission denied.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");

    if (!valid_object(oper1))
        abort_interp("Invalid dbref. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("String required. (2)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), DoNullInd(oper2->data.string));

    if (!*dir)
        strcpyn(dir, sizeof(dir), (char[]){PROPDIR_DELIMITER,0});

    len = strlen(dir) - 1;

    if (len > 0 && dir[len] == PROPDIR_DELIMITER)
        dir[len] = '\0';

    nu = new_array_packed(0, fr->pinning);
    propadr = first_prop(ref, dir, &pptr, propname, sizeof(propname));

    while (propadr) {
        snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);

        if (prop_read_perms(ProgUID, ref, buf, mlev)) {
            prptr = get_property(ref, buf);

            if (prptr) {
#ifdef DISKBASE
                propfetch(ref, prptr);
#endif

                if (PropDir(prptr)) {
                    if (count >= 511) {
                        array_free(nu);
                        abort_interp("Too many propdirs to put in an array!");
                    }

                    array_set_intkey_strval(&nu, count++, propname);
                }
            }
        }

        propadr = next_prop(pptr, propadr, propname, sizeof(propname));
    }

    CLEAR(oper1);
    CLEAR(oper2);

    PushArrayRaw(nu);
}

void
prim_array_get_propvals(PRIM_PROTOTYPE)
{
    stk_array *nu;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PropPtr propadr, pptr;
    PropPtr prptr;
    int count = 0;

    /* dbref strPropDir -- array */
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Permission denied.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");

    if (!valid_object(oper1))
        abort_interp("Invalid dbref. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("String required. (2)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), DoNullInd(oper2->data.string));

    if (!*dir)
        strcpyn(dir, sizeof(dir), (char[]){PROPDIR_DELIMITER,0});

    nu = new_array_dictionary(fr->pinning);
    propadr = first_prop(ref, dir, &pptr, propname, sizeof(propname));

    while (propadr) {
        snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);

        if (prop_read_perms(ProgUID, ref, buf, mlev)) {
            prptr = get_property(ref, buf);

            if (prptr) {
                int goodflag = 1;

#ifdef DISKBASE
                propfetch(ref, prptr);
#endif
                switch (PropType(prptr)) {
                    case PROP_STRTYP:
                        temp2.type = PROG_STRING;
                        temp2.data.string = alloc_prog_string(PropDataStr(prptr));
                        break;

                    case PROP_LOKTYP:
                        temp2.type = PROG_LOCK;

                        if (PropFlags(prptr) & PROP_ISUNLOADED) {
                            temp2.data.lock = TRUE_BOOLEXP;
                        } else {
                            temp2.data.lock = PropDataLok(prptr);

                            if (temp2.data.lock != TRUE_BOOLEXP) {
                                temp2.data.lock = copy_bool(temp2.data.lock);
                            }
                        }

                        break;

                    case PROP_REFTYP:
                        temp2.type = PROG_OBJECT;
                        temp2.data.number = PropDataRef(prptr);
                        break;

                    case PROP_INTTYP:
                        temp2.type = PROG_INTEGER;
                        temp2.data.number = PropDataVal(prptr);
                        break;

                    case PROP_FLTTYP:
                        temp2.type = PROG_FLOAT;
                        temp2.data.fnumber = PropDataFVal(prptr);
                        break;

                    default:
                        goodflag = 0;
                        break;
                }

                if (goodflag) {
                    if (count++ >= 511) {
                        array_free(nu);
                        abort_interp("Too many properties to put in an array!");
                    }

                    temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string(propname);
                    array_setitem(&nu, &temp1, &temp2);
                    CLEAR(&temp1);
                    CLEAR(&temp2);
                }
            }
        }

        propadr = next_prop(pptr, propadr, propname, sizeof(propname));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nu);
}

void
prim_array_get_proplist(PRIM_PROTOTYPE)
{
    stk_array *nu;
    const char *strval;
    char dir[BUFFER_LEN];
    char propname[BUFFER_LEN];
    PropPtr prptr;
    int count = 1;
    int maxcount;

    /* dbref strPropDir -- array */
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");

    if (!valid_object(oper1))
        abort_interp("Invalid dbref. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("String required. (2)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), DoNullInd(oper2->data.string));

    if (!*dir)
        strcpyn(dir, sizeof(dir), (char[]){PROPDIR_DELIMITER,0});

    snprintf(propname, sizeof(propname), "%s#", dir);
    maxcount = get_property_value(ref, propname);

    if (!maxcount) {
        strval = get_property_class(ref, propname);

        if (strval) {
            if (strval && number(strval)) {
                maxcount = atoi(strval);
            }
        }

        if (!maxcount) {
            snprintf(propname, sizeof(propname), "%s%c#", dir,
                     PROPDIR_DELIMITER);
            maxcount = get_property_value(ref, propname);

            if (!maxcount) {
                strval = get_property_class(ref, propname);

                if (strval && number(strval)) {
                    maxcount = atoi(strval);
                }
            }
        }
    }

    nu = new_array_packed(0, fr->pinning);

    while (maxcount > 0) {
        snprintf(propname, sizeof(propname), "%s#%c%d", dir, PROPDIR_DELIMITER, count);
        prptr = get_property(ref, propname);

        if (!prptr) {
            snprintf(propname, sizeof(propname), "%s%c%d", dir, PROPDIR_DELIMITER, count);
            prptr = get_property(ref, propname);

            if (!prptr) {
                snprintf(propname, sizeof(propname), "%s%d", dir, count);
                prptr = get_property(ref, propname);
            }
        }

        if (maxcount > 1023) {
            maxcount = 1023;
        }

        if (maxcount) {
            if (count > maxcount)
                break;
        } else if (!prptr) {
            break;
        }

        if (prop_read_perms(ProgUID, ref, propname, mlev)) {
            if (!prptr) {
                temp2.type = PROG_INTEGER;
                temp2.data.number = 0;
            } else {
#ifdef DISKBASE
                propfetch(ref, prptr);
#endif
                switch (PropType(prptr)) {
                    case PROP_STRTYP:
                        temp2.type = PROG_STRING;
                        temp2.data.string = alloc_prog_string(PropDataStr(prptr));
                        break;

                    case PROP_LOKTYP:
                        temp2.type = PROG_LOCK;

                        if (PropFlags(prptr) & PROP_ISUNLOADED) {
                            temp2.data.lock = TRUE_BOOLEXP;
                        } else {
                            temp2.data.lock = PropDataLok(prptr);

                            if (temp2.data.lock != TRUE_BOOLEXP) {
                                temp2.data.lock = copy_bool(temp2.data.lock);
                            }
                        }

                        break;

                    case PROP_REFTYP:
                        temp2.type = PROG_OBJECT;
                        temp2.data.number = PropDataRef(prptr);
                        break;

                    case PROP_INTTYP:
                        temp2.type = PROG_INTEGER;
                        temp2.data.number = PropDataVal(prptr);
                        break;

                    case PROP_FLTTYP:
                        temp2.type = PROG_FLOAT;
                        temp2.data.fnumber = PropDataFVal(prptr);
                        break;

                    default:
                        temp2.type = PROG_INTEGER;
                        temp2.data.number = 0;
                        break;
                }
            }

            array_appenditem(&nu, &temp2);
            CLEAR(&temp2);
        } else {
            break;
        }

        count++;
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nu);
}

void
prim_array_put_propvals(PRIM_PROTOTYPE)
{
    stk_array *arr;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PData propdat;

    /* dbref strPropDir array -- */
    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");

    if (!valid_object(oper1))
        abort_interp("Invalid dbref. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("String required. (2)");

    if (oper3->type != PROG_ARRAY)
        abort_interp("Array required. (3)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), DoNullInd(oper2->data.string));
    arr = oper3->data.array;

    if (array_first(arr, &temp1)) {
        do {
            oper4 = array_getitem(arr, &temp1);

            switch (temp1.type) {
                case PROG_STRING:
                    snprintf(propname, sizeof(propname), "%s%c%s", dir,
                             PROPDIR_DELIMITER, DoNullInd(temp1.data.string));
                    break;

                case PROG_INTEGER:
                    snprintf(propname, sizeof(propname), "%s%c%d", dir,
                             PROPDIR_DELIMITER, temp1.data.number);
                    break;

                case PROG_FLOAT:
                    snprintf(propname, sizeof(propname), "%s%c%.15g", dir,
                             PROPDIR_DELIMITER, temp1.data.fnumber);

                    if (!strchr(propname, '.') && !strchr(propname, 'n')
                        && !strchr(propname, 'e')) {
                        strcatn(propname, sizeof(propname), ".0");
                    }

                    break;

                default:
                    *propname = '\0';
            }

            if (!prop_write_perms(ProgUID, ref, propname, mlev))
                abort_interp("Permission denied while trying to set "
                             "protected property.");

            switch (oper4->type) {
                case PROG_STRING:
                    propdat.flags = PROP_STRTYP;
                    propdat.data.str = oper4->data.string ? oper4->data.string->data : 0;
                    break;

                case PROG_INTEGER:
                    propdat.flags = PROP_INTTYP;
                    propdat.data.val = oper4->data.number;
                    break;

                case PROG_FLOAT:
                    propdat.flags = PROP_FLTTYP;
                    propdat.data.fval = oper4->data.fnumber;
                    break;

                case PROG_OBJECT:
                    propdat.flags = PROP_REFTYP;
                    propdat.data.ref = oper4->data.objref;
                    break;

                case PROG_LOCK:
                    propdat.flags = PROP_LOKTYP;
                    propdat.data.lok = copy_bool(oper4->data.lock);
                    break;

                default:
                    *propname = '\0';
            }

            if (*propname) {
                set_property(ref, propname, &propdat, 0);
            }
        } while (array_next(arr, &temp1));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_array_put_proplist(PRIM_PROTOTYPE)
{
    stk_array *arr;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PData propdat;
    const char *fmtin;
    char *fmtout;
    int dirlen;
    int count;

    /* dbref strPropDir array -- */
    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Dbref required. (1)");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("String required. (2)");
    if (oper3->type != PROG_ARRAY)
	abort_interp("Array required. (3)");
    if (oper3->data.array && oper3->data.array->type != ARRAY_PACKED)
	abort_interp("Argument must be a list type array. (3)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), DoNullInd(oper2->data.string));
    arr = oper3->data.array;

    dirlen = strlen(dir);
    fmtout = propname;
    fmtin = "P#";
    while (*fmtin) {
	if (*fmtin == 'P') {
	    if ((size_t) ((fmtout + dirlen) - propname) > sizeof(propname))
		break;
	    strcpyn(fmtout, sizeof(propname) - (size_t)(fmtout - propname), dir);
	    fmtout = &fmtout[strlen(fmtout)];
	} else {
	    *fmtout++ = *fmtin;
	}
	fmtin++;
    }
    *fmtout++ = '\0';

    if (!prop_write_perms(ProgUID, ref, propname, mlev))
	abort_interp("Permission denied while trying to set protected property.");

    if (array_is_homogenous(arr, PROG_STRING)) {
        propdat.flags = PROP_STRTYP;
        snprintf(buf, sizeof(buf), "%d", array_count(arr));
        propdat.data.str = buf;
    } else {
        propdat.flags = PROP_INTTYP;
        propdat.data.val = array_count(arr);
    }
    set_property(ref, propname, &propdat, 0);

    if (array_first(arr, &temp1)) {
	do {
	    oper4 = array_getitem(arr, &temp1);

	    fmtout = propname;
	    fmtin = "P#/N";
	    while (*fmtin) {
		if (*fmtin == 'N') {
		    if ((size_t) ((fmtout + 18) - propname) > sizeof(propname))
			break;
		    snprintf(fmtout, sizeof(propname) - (size_t)(fmtout - propname), "%d",
			     temp1.data.number + 1);
		    fmtout = &fmtout[strlen(fmtout)];
		} else if (*fmtin == 'P') {
		    if ((size_t) ((fmtout + dirlen) - propname) > sizeof(propname))
			break;
		    strcpyn(fmtout, sizeof(propname) - (size_t)(fmtout - propname), dir);
		    fmtout = &fmtout[strlen(fmtout)];
		} else {
		    *fmtout++ = *fmtin;
		}
		fmtin++;
	    }
	    *fmtout++ = '\0';

	    if (!prop_write_perms(ProgUID, ref, propname, mlev))
		abort_interp("Permission denied while trying to set protected property.");

	    switch (oper4->type) {
	    case PROG_STRING:
		propdat.flags = PROP_STRTYP;
		propdat.data.str = oper4->data.string ? oper4->data.string->data : 0;
		break;
	    case PROG_INTEGER:
		propdat.flags = PROP_INTTYP;
		propdat.data.val = oper4->data.number;
		break;
	    case PROG_FLOAT:
		propdat.flags = PROP_FLTTYP;
		propdat.data.fval = oper4->data.fnumber;
		break;
	    case PROG_OBJECT:
		propdat.flags = PROP_REFTYP;
		propdat.data.ref = oper4->data.objref;
		break;
	    case PROG_LOCK:
		propdat.flags = PROP_LOKTYP;
		propdat.data.lok = copy_bool(oper4->data.lock);
		break;
	    default:
		propdat.flags = PROP_INTTYP;
		propdat.data.val = 0;
	    }
	    set_property(ref, propname, &propdat, 0);
	} while (array_next(arr, &temp1));
    }

    count = temp1.data.number;
    for (;;) {
	count++;
	fmtout = propname;
	fmtin = "P#/N";
	while (*fmtin) {
	    if (*fmtin == 'N') {
		if ((size_t) ((fmtout + 18) - propname) > sizeof(propname))
		    break;
		snprintf(fmtout, sizeof(propname) - (size_t)(fmtout - propname), "%d", count + 1);
		fmtout = &fmtout[strlen(fmtout)];
	    } else if (*fmtin == 'P') {
		if ((size_t) ((fmtout + dirlen) - propname) > sizeof(propname))
		    break;
		strcpyn(fmtout, sizeof(propname) - (size_t)(fmtout - propname), dir);
		fmtout = &fmtout[strlen(fmtout)];
	    } else {
		*fmtout++ = *fmtin;
	    }
	    fmtin++;
	}
	*fmtout++ = '\0';
	if (get_property(ref, propname)) {
	    remove_property(ref, propname, 0);
	} else {
	    break;
	}
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_array_get_reflist(PRIM_PROTOTYPE)
{
    stk_array *nu;
    const char *rawstr;
    char dir[BUFFER_LEN];
    int count = 0;

    /* dbref strPropDir -- array */
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Dbref required. (1)");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("String required. (2)");
    if (!oper2->data.string)
	abort_interp("Non-null string required. (2)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), oper2->data.string->data);

    if (!prop_read_perms(ProgUID, ref, dir, mlev))
	abort_interp("Permission denied.");

    nu = new_array_packed(0, fr->pinning);
    rawstr = get_property_class(ref, dir);

    if (rawstr) {
	skip_whitespace(&rawstr);
	while (*rawstr) {
	    if (*rawstr == NUMBER_TOKEN)
		rawstr++;
	    if (!isdigit(*rawstr) && (*rawstr != '-'))
		break;
	    result = atoi(rawstr);
	    while (*rawstr && !isspace(*rawstr))
		rawstr++;
	    skip_whitespace(&rawstr);

	    temp1.type = PROG_INTEGER;
	    temp1.data.number = count;

	    temp2.type = PROG_OBJECT;
	    temp2.data.number = result;

	    array_setitem(&nu, &temp1, &temp2);
	    count++;

	    CLEAR(&temp1);
	    CLEAR(&temp2);
	}
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nu);
}

void
prim_array_put_reflist(PRIM_PROTOTYPE)
{
    stk_array *arr;
    char buf2[BUFFER_LEN];
    char dir[BUFFER_LEN];
    char *out;
    PData propdat;
    int len;

    /* dbref strPropDir array -- */
    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Dbref required. (1)");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("String required. (2)");
    if (!oper2->data.string)
	abort_interp("Non-null string required. (2)");
    if (oper3->type != PROG_ARRAY)
	abort_interp("Argument must be a list array of dbrefs. (3)");
    if (oper3->data.array && oper3->data.array->type != ARRAY_PACKED)
	abort_interp("Argument must be a list array of dbrefs. (3)");
    if (!array_is_homogenous(oper3->data.array, PROG_OBJECT))
	abort_interp("Argument must be a list array of dbrefs. (3)");

    ref = oper1->data.objref;
    strcpyn(dir, sizeof(dir), DoNullInd(oper2->data.string));
    arr = oper3->data.array;
    buf[0] = '\0';

    if (!prop_write_perms(ProgUID, ref, dir, mlev))
	abort_interp("Permission denied.");

    out = buf;
    if (array_first(arr, &temp1)) {
	do {
	    oper4 = array_getitem(arr, &temp1);
	    len = snprintf(buf2, sizeof(buf2), "#%d", oper4->data.objref);
	    if (len == -1) {
		buf2[sizeof(buf2) - 1] = '\0';
		len = sizeof(buf2) - 1;
	    }

	    if (out + len - buf >= BUFFER_LEN - 3)
		abort_interp("Operation would result in string length overflow.");

	    if (*buf)
		*out++ = ' ';
	    strcpyn(out, sizeof(buf) - (size_t)(out - buf), buf2);
	    out += len;
	} while (array_next(arr, &temp1));
    }

    remove_property(ref, dir, 0);
    propdat.flags = PROP_STRTYP;
    propdat.data.str = buf;
    set_property(ref, dir, &propdat, 0);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_array_findval(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nu;

    CHECKOP(2);
    oper2 = POP();		/* ???  index */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");

    nu = new_array_packed(0, fr->pinning);
    arr = oper1->data.array;
    if (array_first(arr, &temp1)) {
	do {
	    in = array_getitem(arr, &temp1);
	    if (!array_tree_compare(in, oper2, 0)) {
		array_appenditem(&nu, &temp1);
	    }
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu);
}

void
prim_array_compare(PRIM_PROTOTYPE)
{
    struct inst *val1;
    struct inst *val2;
    stk_array *arr1;
    stk_array *arr2;
    int res1, res2;

    CHECKOP(2);
    oper2 = POP();		/* arr  Array */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_ARRAY)
	abort_interp("Argument not an array. (2)");

    arr1 = oper1->data.array;
    arr2 = oper2->data.array;
    res1 = array_first(arr1, &temp1);
    res2 = array_first(arr2, &temp2);

    if (!res1 && !res2) {
	result = 0;
    } else if (!res1) {
	result = -1;
    } else if (!res2) {
	result = 1;
    } else {
	do {
	    result = array_tree_compare(&temp1, &temp2, 0);
	    if (result)
		break;

	    val1 = array_getitem(arr1, &temp1);
	    val2 = array_getitem(arr2, &temp2);
	    result = array_tree_compare(val1, val2, 0);
	    if (result)
		break;

	    res1 = array_next(arr1, &temp1);
	    res2 = array_next(arr2, &temp2);
	} while (res1 && res2);

	if (!res1 && !res2) {
	    result = 0;
	} else if (!res1) {
	    result = -1;
	} else if (!res2) {
	    result = 1;
	}
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushInt(result);
}

void
prim_array_matchkey(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nu;

    CHECKOP(2);
    oper2 = POP();		/* str  pattern */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not a string pattern. (2)");

    nu = new_array_dictionary(fr->pinning);
    arr = oper1->data.array;
    if (array_first(arr, &temp1)) {
	do {
	    if (temp1.type == PROG_STRING) {
		if (equalstr(DoNullInd(oper2->data.string), DoNullInd(temp1.data.string))) {
		    in = array_getitem(arr, &temp1);
		    array_setitem(&nu, &temp1, in);
		}
	    }
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu);
}

void
prim_array_matchval(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nu;

    CHECKOP(2);
    oper2 = POP();		/* str  pattern */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not a string pattern. (2)");

    nu = new_array_dictionary(fr->pinning);
    arr = oper1->data.array;
    if (array_first(arr, &temp1)) {
	do {
	    in = array_getitem(arr, &temp1);
	    if (in->type == PROG_STRING) {
		if (equalstr(DoNullInd(oper2->data.string), DoNullInd(in->data.string))) {
		    array_setitem(&nu, &temp1, in);
		}
	    }
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu);
}

void
prim_array_extract(PRIM_PROTOTYPE)
{
    struct inst *in;
    struct inst *key;
    stk_array *arr;
    stk_array *karr;
    stk_array *nu;

    CHECKOP(2);
    oper2 = POP();		/* arr  indexes */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_ARRAY)
	abort_interp("Argument not an array. (2)");

    nu = new_array_dictionary(fr->pinning);
    arr = oper1->data.array;
    karr = oper2->data.array;
    if (array_first(karr, &temp1)) {
	do {
	    key = array_getitem(karr, &temp1);
	    if (key) {
		in = array_getitem(arr, key);
		if (in) {
		    array_setitem(&nu, key, in);
		}
	    }
	} while (array_next(karr, &temp1));
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu);
}

void
prim_array_excludeval(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nu;

    CHECKOP(2);
    oper2 = POP();		/* ???  index */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");

    nu = new_array_packed(0, fr->pinning);
    arr = oper1->data.array;
    if (array_first(arr, &temp1)) {
	do {
	    in = array_getitem(arr, &temp1);
	    if (array_tree_compare(in, oper2, 0)) {
		array_appenditem(&nu, &temp1);
	    }
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu);
}

void
prim_array_join(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    char outbuf[BUFFER_LEN];
    char *ptr;
    const char *text;
    char *delim;
    int tmplen;
    int done;
    int first_item;

    CHECKOP(2);
    oper2 = POP();		/* str  joinstr */
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not a string pattern. (2)");

    arr = oper1->data.array;
    delim = DoNullInd(oper2->data.string);
    ptr = outbuf;
    *outbuf = '\0';
    first_item = 1;
    done = !array_first(arr, &temp1);
    while (!done) {
	in = array_getitem(arr, &temp1);
	switch (in->type) {
	case PROG_STRING:
	    text = DoNullInd(in->data.string);
	    break;
	case PROG_INTEGER:
	    snprintf(buf, sizeof(buf), "%d", in->data.number);
	    text = buf;
	    break;
	case PROG_OBJECT:
	    snprintf(buf, sizeof(buf), "#%d", in->data.number);
	    text = buf;
	    break;
	case PROG_FLOAT:
	    snprintf(buf, sizeof(buf), "%.15g", in->data.fnumber);
	    if (!strchr(buf, '.') && !strchr(buf, 'n') && !strchr(buf, 'e')) {
		strcatn(buf, sizeof(buf), ".0");
	    }
	    text = buf;
	    break;
	case PROG_LOCK:
	    text = unparse_boolexp(ProgUID, in->data.lock, 1);
	    break;
	default:
	    text = "<UNSUPPORTED>";
	    break;
	}
	if (first_item) {
	    first_item = 0;
	} else {
	    tmplen = strlen(delim);
	    if (tmplen > BUFFER_LEN - (ptr - outbuf) - 1)
		abort_interp("Operation would result in overflow.");
	    strcpyn(ptr, sizeof(outbuf) - (size_t)(outbuf - ptr), delim);
	    ptr += tmplen;
	}
	tmplen = strlen(text);
	if (tmplen > BUFFER_LEN - (ptr - outbuf) - 1)
	    abort_interp("Operation would result in overflow.");
	strcpyn(ptr, sizeof(outbuf) - (size_t)(outbuf - ptr), text);
	ptr += tmplen;
	done = !array_next(arr, &temp1);
    }

    CLEAR(oper2);
    CLEAR(oper1);

    PushString(outbuf);
}

void
prim_array_interpret(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    char outbuf[BUFFER_LEN];
    char *ptr;
    const char *text;

    /* char *delim; */
    int tmplen;
    int done;

    CHECKOP(1);
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");

    arr = oper1->data.array;
    ptr = outbuf;
    *outbuf = '\0';
    done = !array_first(arr, &temp1);
    while (!done) {
	in = array_getitem(arr, &temp1);
	switch (in->type) {
	case PROG_STRING:
	    text = DoNullInd(in->data.string);
	    break;
	case PROG_INTEGER:
	    snprintf(buf, sizeof(buf), "%d", in->data.number);
	    text = buf;
	    break;
	case PROG_OBJECT:
	    if (in->data.objref == NOTHING) {
		text = "*NOTHING*";
		break;
	    }
	    if (in->data.objref == AMBIGUOUS) {
		text = "*AMBIGUOUS*";
		break;
	    }
	    if (in->data.objref == HOME) {
		text = "*HOME*";
		break;
	    }
	    if (in->data.objref == NIL) {
		text = "*NIL*";
		break;
	    }
	    if (in->data.number < NIL) {
		text = "*INVALID*";
		break;
	    }
	    if (in->data.number >= db_top) {
		text = "*INVALID*";
		break;
	    }
	    snprintf(buf, sizeof(buf), "%s", NAME(in->data.number));
	    text = buf;
	    break;
	case PROG_FLOAT:
	    snprintf(buf, sizeof(buf), "%.15g", in->data.fnumber);
	    if (!strchr(buf, '.') && !strchr(buf, 'n') && !strchr(buf, 'e')) {
		strcatn(buf, sizeof(buf), ".0");
	    }
	    text = buf;
	    break;
	case PROG_LOCK:
	    text = unparse_boolexp(ProgUID, in->data.lock, 1);
	    break;
	default:
	    text = "<UNSUPPORTED>";
	    break;
	}
	tmplen = strlen(text);
	if (tmplen > BUFFER_LEN - (ptr - outbuf) - 1) {
	    strncpy(ptr, text, BUFFER_LEN - (ptr - outbuf) - 1);
	    outbuf[BUFFER_LEN - 1] = '\0';
	    break;
	} else {
	    strcpyn(ptr, sizeof(outbuf) - (size_t)(outbuf - ptr), text);
	    ptr += tmplen;
	}
	done = !array_next(arr, &temp1);
    }

    CLEAR(oper1);
    PushString(outbuf);
}

void
prim_array_pin(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nu;

    CHECKOP(1);
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array.");

    arr = oper1->data.array;
    if (!arr) {
	nu = new_array_packed(0, fr->pinning);
    } else if (arr->links > 1) {
	nu = array_decouple(arr);
    } else {
	arr->links++;
	nu = arr;
    }
    nu->pinned = 1;

    CLEAR(oper1);
    PushArrayRaw(nu);
}

void
prim_array_unpin(PRIM_PROTOTYPE)
{
    stk_array *arr;

    CHECKOP(1);
    oper1 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array.");

    arr = oper1->data.array;
    if (arr) {
	arr->links++;
	arr->pinned = 0;
    }

    CLEAR(oper1);
    PushArrayRaw(arr);
}

void
prim_array_default_pinning(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();		/* int  New default pinning boolean val */
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer.");
    fr->pinning = false_inst(oper1)? 0 : 1;
    CLEAR(oper1);
}

void
prim_array_get_ignorelist(PRIM_PROTOTYPE)
{
    stk_array *nu;
    const char *rawstr;
    int count = 0;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
	abort_interp("Dbref required.");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref.");
    if (mlev < 3)
	abort_interp("Permission denied.");

    ref = OWNER(oper1->data.objref);

    CLEAR(oper1);

    nu = new_array_packed(0, fr->pinning);

    if (tp_ignore_support) {
	rawstr = get_property_class(ref, IGNORE_PROP);

	if (rawstr) {
	    skip_whitespace(&rawstr);
	    while (*rawstr) {
		if (*rawstr == NUMBER_TOKEN)
		    rawstr++;
		if (!isdigit(*rawstr))
		    break;
		result = atoi(rawstr);
		while (*rawstr && !isspace(*rawstr))
		    rawstr++;
		skip_whitespace(&rawstr);

		temp1.type = PROG_INTEGER;
		temp1.data.number = count;

		temp2.type = PROG_OBJECT;
		temp2.data.number = result;

		array_setitem(&nu, &temp1, &temp2);
		count++;

		CLEAR(&temp1);
		CLEAR(&temp2);
	    }
	}
    }

    PushArrayRaw(nu);
}

void
prim_array_nested_get(PRIM_PROTOTYPE)
{
    struct inst *idx;
    struct inst *dat;
    struct inst temp;
    stk_array *idxarr;
    int idxcnt;

    CHECKOP(2);
    oper1 = POP();		/* arr  IndexList */
    oper2 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array of indexes. (2)");
    if (!oper1->data.array || oper1->data.array->type == ARRAY_DICTIONARY)
	abort_interp("Argument not an array of indexes. (2)");
    if (oper2->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");

    idxarr = oper1->data.array;
    idxcnt = array_count(idxarr);
    dat = oper2;
    for (int i = 0; dat && i < idxcnt; i++) {
	temp1.type = PROG_INTEGER;
	temp1.data.number = i;
	idx = array_getitem(idxarr, &temp1);
	if (idx->type != PROG_INTEGER && idx->type != PROG_STRING)
	    abort_interp("Argument not an array of indexes. (2)");
	if (dat->type != PROG_ARRAY)
	    abort_interp("Mid-level nested item was not an array. (1)");
	dat = array_getitem(dat->data.array, idx);
    }

    /* copy data to a temp inst before clearing the containing array */
    if (dat) {
	copyinst(dat, &temp);
    } else {
	temp.type = PROG_INTEGER;
	temp.data.number = 0;
    }

    CLEAR(oper1);
    CLEAR(oper2);

    /* copy data to stack, then clear temp inst */
    copyinst(&temp, &arg[(*top)++]);
    CLEAR(&temp);
}

void
prim_array_nested_set(PRIM_PROTOTYPE)
{
    struct inst nest[16];
    struct inst *idx = NULL;
    struct inst *dat;
    struct inst temp;
    stk_array *arr;
    stk_array *idxarr;
    size_t idxcnt;

    CHECKOP(3);
    oper1 = POP();		/* arr  IndexList */
    oper2 = POP();		/* arr  Array */
    oper3 = POP();		/* ???  Value */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array of indexes. (3)");
    if (!oper1->data.array || oper1->data.array->type == ARRAY_DICTIONARY)
	abort_interp("Argument not an array of indexes. (3)");
    if (oper2->type != PROG_ARRAY)
	abort_interp("Argument not an array. (2)");

    idxarr = oper1->data.array;
    idxcnt = (size_t)array_count(idxarr);
    if (idxcnt > sizeof(nest) / sizeof(struct inst))
	abort_interp("Nesting would be too deep. (3)");

    if (idxcnt == 0) {
	copyinst(oper2, &nest[0]);
    } else {
	dat = oper2;
	for (size_t i = 0; i < idxcnt; i++) {
	    copyinst(dat, &nest[i]);
	    temp.type = PROG_INTEGER;
	    temp.data.number = i;
	    idx = array_getitem(idxarr, &temp);
	    if (idx->type != PROG_INTEGER && idx->type != PROG_STRING)
		abort_interp("Argument not an array of indexes. (3)");
	    if (i < idxcnt - 1) {
		dat = array_getitem(nest[i].data.array, idx);
		if (dat) {
		    if (dat->type != PROG_ARRAY)
			abort_interp("Mid-level nested item was not an array. (2)");
		} else {
		    if (idx->type == PROG_INTEGER && idx->data.number == 0) {
			arr = new_array_packed(1, fr->pinning);
		    } else {
			arr = new_array_dictionary(fr->pinning);
		    }
		    arr->links = 0;
		    temp.type = PROG_ARRAY;
		    temp.data.array = arr;
		    dat = &temp;
		}
	    }
	}

	array_setitem(&nest[idxcnt - 1].data.array, idx, oper3);
	for (size_t i = idxcnt - 1; i-- > 0;) {
	    temp.type = PROG_INTEGER;
	    temp.data.number = i;
	    idx = array_getitem(idxarr, &temp);
	    array_setitem(&nest[i].data.array, idx, &nest[i + 1]);
	    CLEAR(&nest[i + 1]);
	}
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    /* copy data to stack, then clear temp inst */
    copyinst(&nest[0], &arg[(*top)++]);
    CLEAR(&nest[0]);
}

void
prim_array_nested_del(PRIM_PROTOTYPE)
{
    struct inst nest[32];
    struct inst *idx = NULL;
    struct inst *dat;
    struct inst temp;
    stk_array *idxarr;
    size_t idxcnt;

    CHECKOP(2);
    oper1 = POP();		/* arr  IndexList */
    oper2 = POP();		/* arr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array of indexes. (2)");
    if (!oper1->data.array || oper1->data.array->type == ARRAY_DICTIONARY)
	abort_interp("Argument not an array of indexes. (2)");
    if (oper2->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");

    idxarr = oper1->data.array;
    idxcnt = (size_t)array_count(idxarr);
    if (idxcnt > sizeof(nest) / sizeof(struct inst))
	abort_interp("Nesting would be too deep. (2)");

    if (idxcnt == 0) {
	copyinst(oper2, &nest[0]);
    } else {
	int doneearly = 0;
	dat = oper2;
	for (size_t i = 0; i < idxcnt; i++) {
	    copyinst(dat, &nest[i]);
	    temp.type = PROG_INTEGER;
	    temp.data.number = i;
	    idx = array_getitem(idxarr, &temp);
	    if (idx->type != PROG_INTEGER && idx->type != PROG_STRING)
		abort_interp("Argument not an array of indexes. (2)");
	    if (i < idxcnt - 1) {
		dat = array_getitem(nest[i].data.array, idx);
		if (dat) {
		    if (dat->type != PROG_ARRAY)
			abort_interp("Mid-level nested item was not an array. (1)");
		} else {
		    doneearly = 1;
		    break;
		}
	    }
	}
	if (!doneearly) {
	    array_delitem(&nest[idxcnt - 1].data.array, idx);
	    for (size_t i = idxcnt - 1; i-- > 0;) {
		temp.type = PROG_INTEGER;
		temp.data.number = i;
		idx = array_getitem(idxarr, &temp);
		array_setitem(&nest[i].data.array, idx, &nest[i + 1]);
		CLEAR(&nest[i + 1]);
	    }
	}
    }

    CLEAR(oper1);
    CLEAR(oper2);

    /* copy data to stack, then clear temp inst */
    copyinst(&nest[0], &arg[(*top)++]);
    CLEAR(&nest[0]);
}

void
prim_array_filter_flags(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    stk_array *nw, *arr;
    struct inst *in;

    CHECKOP(2);
    oper2 = POP();		/* str:flags */
    oper1 = POP();		/* arr:refs */

    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_OBJECT))
	abort_interp("Argument not an array of dbrefs. (1)");
    if (oper2->type != PROG_STRING || !oper2->data.string)
	abort_interp("Argument not a non-null string. (2)");

    arr = oper1->data.array;
    nw = new_array_packed(0, fr->pinning);

    init_checkflags(player, DoNullInd(oper2->data.string), &check);

    if (array_first(arr, &temp1)) {
	do {
	    in = array_getitem(arr, &temp1);
	    if (valid_object(in) && checkflags(in->data.objref, check))
		array_appenditem(&nw, in);
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nw);
}

void
prim_array_filter_lock(PRIM_PROTOTYPE)
{
    stk_array *nw, *arr;
    struct inst *in;

    CHECKOP(2);
    oper2 = POP();		/* lock */
    oper1 = POP();		/* arr:refs */

    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_OBJECT))
	abort_interp("Argument not an array of dbrefs. (1)");
    if (oper2->type != PROG_LOCK)
	abort_interp("Argument not a lock. (2)");

    arr = oper1->data.array;
    nw = new_array_packed(0, fr->pinning);

    if (array_first(arr, &temp1)) {
	do {
	    in = array_getitem(arr, &temp1);
	    if (valid_object(in) &&
		    eval_boolexp(fr->descr, in->data.objref, oper2->data.lock,
		    tp_consistent_lock_source ? fr->trig : player))
		array_appenditem(&nw, in);
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nw);
}

void
prim_array_notify_secure(PRIM_PROTOTYPE)
{
    stk_array *strarr, *strarr2, *refarr;
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL, *oper5 = NULL;
    struct inst temp2, temp3;

    int *darr;
    int dcount;

    if (mlev < 3)
	abort_interp("Mucker level 3 primitive.");

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array of dbrefs. (3)");
    if (!array_is_homogenous(oper1->data.array, PROG_OBJECT))
	abort_interp("Argument not an array of dbrefs. (3)");
    if (oper2->type != PROG_ARRAY)
	abort_interp("Argument not an array of strings. (2)");
    if (!array_is_homogenous(oper2->data.array, PROG_STRING))
	abort_interp("Argument not an array of strings. (2)");
    if (oper3->type != PROG_ARRAY)
	abort_interp("Argument not an array of strings. (1)");
    if (!array_is_homogenous(oper3->data.array, PROG_STRING))
	abort_interp("Argument not an array of strings. (1)");

    refarr = oper1->data.array;
    strarr = oper2->data.array;
    strarr2 = oper3->data.array;

    if (array_first(refarr, &temp3)) {
	do {
	    oper5 = array_getitem(refarr, &temp3);
	    ref = oper5->data.objref;

	    darr = get_player_descrs(ref, &dcount);

	    for (int di = 0; di < dcount; di++) {
		if (pdescrsecure(darr[di])) {
		    if (array_first(strarr, &temp2)) {
			do {
			    oper4 = array_getitem(strarr, &temp2);
			    pdescrnotify(darr[di], oper4->data.string->data);
			    oper4 = NULL;
			} while (array_next(strarr, &temp2));
		    }
		} else {
		    if (array_first(strarr2, &temp2)) {
			do {
			    oper4 = array_getitem(strarr2, &temp2);
			    pdescrnotify(darr[di], oper4->data.string->data);

			    listenqueue(-1, player, LOCATION(ref), ref, ref, program,
					LISTEN_PROPQUEUE, oper4->data.string->data, tp_listen_mlev, 1,
					0);
			    listenqueue(-1, player, LOCATION(ref), ref, ref, program,
					WLISTEN_PROPQUEUE, oper4->data.string->data, tp_listen_mlev, 1,
					1);
			    listenqueue(-1, player, LOCATION(ref), ref, ref, program,
					WOLISTEN_PROPQUEUE, oper4->data.string->data, tp_listen_mlev,
					0, 1);

			    oper4 = NULL;
			} while (array_next(strarr2, &temp2));
		    }
		}
	    }
	} while (array_next(refarr, &temp3));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}
