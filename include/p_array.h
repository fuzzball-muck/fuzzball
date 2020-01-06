/** @file p_array.h
 *
 * Declaration of Array Operations for MUF
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
#ifndef P_ARRAY_H
#define P_ARRAY_H

#include "interp.h"

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
void prim_array_make(PRIM_PROTOTYPE);

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
void prim_array_make_dict(PRIM_PROTOTYPE);

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
void prim_array_explode(PRIM_PROTOTYPE);

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
void prim_array_vals(PRIM_PROTOTYPE);

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
void prim_array_keys(PRIM_PROTOTYPE);

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
void prim_array_count(PRIM_PROTOTYPE);

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
void prim_array_first(PRIM_PROTOTYPE);

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
void prim_array_last(PRIM_PROTOTYPE);

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
void prim_array_next(PRIM_PROTOTYPE);

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
void prim_array_prev(PRIM_PROTOTYPE);

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
void prim_array_getitem(PRIM_PROTOTYPE);

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
void prim_array_setitem(PRIM_PROTOTYPE);

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
void prim_array_appenditem(PRIM_PROTOTYPE);

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
void prim_array_insertitem(PRIM_PROTOTYPE);

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
void prim_array_getrange(PRIM_PROTOTYPE);

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
void prim_array_setrange(PRIM_PROTOTYPE);

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
void prim_array_insertrange(PRIM_PROTOTYPE);

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
void prim_array_delitem(PRIM_PROTOTYPE);

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
void prim_array_delrange(PRIM_PROTOTYPE);
void prim_array_compare(PRIM_PROTOTYPE);
void prim_array_findval(PRIM_PROTOTYPE);
void prim_array_matchval(PRIM_PROTOTYPE);
void prim_array_matchkey(PRIM_PROTOTYPE);
void prim_array_extract(PRIM_PROTOTYPE);
void prim_array_excludeval(PRIM_PROTOTYPE);
void prim_array_sort(PRIM_PROTOTYPE);
void prim_array_sort_indexed(PRIM_PROTOTYPE);
void prim_array_join(PRIM_PROTOTYPE);
void prim_array_interpret(PRIM_PROTOTYPE);

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
void prim_array_cut(PRIM_PROTOTYPE);

void prim_array_n_union(PRIM_PROTOTYPE);
void prim_array_n_intersection(PRIM_PROTOTYPE);
void prim_array_n_difference(PRIM_PROTOTYPE);

void prim_array_notify(PRIM_PROTOTYPE);
void prim_array_reverse(PRIM_PROTOTYPE);

void prim_array_get_propdirs(PRIM_PROTOTYPE);
void prim_array_get_propvals(PRIM_PROTOTYPE);
void prim_array_get_proplist(PRIM_PROTOTYPE);
void prim_array_put_propvals(PRIM_PROTOTYPE);
void prim_array_put_proplist(PRIM_PROTOTYPE);

void prim_array_get_reflist(PRIM_PROTOTYPE);
void prim_array_put_reflist(PRIM_PROTOTYPE);

void prim_array_pin(PRIM_PROTOTYPE);
void prim_array_unpin(PRIM_PROTOTYPE);
void prim_array_default_pinning(PRIM_PROTOTYPE);

void prim_array_get_ignorelist(PRIM_PROTOTYPE);

void prim_array_nested_get(PRIM_PROTOTYPE);
void prim_array_nested_set(PRIM_PROTOTYPE);
void prim_array_nested_del(PRIM_PROTOTYPE);

void prim_array_filter_flags(PRIM_PROTOTYPE);
void prim_array_filter_lock(PRIM_PROTOTYPE);
void prim_array_notify_secure(PRIM_PROTOTYPE);

#define PRIMS_ARRAY_FUNCS prim_array_make, prim_array_make_dict, \
        prim_array_explode, prim_array_vals, prim_array_keys, \
        prim_array_first, prim_array_last, prim_array_next, prim_array_prev, \
        prim_array_count, prim_array_getitem, prim_array_setitem, \
        prim_array_insertitem, prim_array_getrange, prim_array_setrange, \
        prim_array_insertrange, prim_array_delitem, prim_array_delrange, \
        prim_array_n_union, prim_array_n_intersection, \
        prim_array_n_difference, prim_array_notify, prim_array_reverse, \
        prim_array_get_propvals, prim_array_get_propdirs, \
        prim_array_get_proplist, prim_array_put_propvals, \
        prim_array_put_proplist, prim_array_get_reflist, \
        prim_array_put_reflist, prim_array_appenditem, prim_array_findval, \
        prim_array_excludeval, prim_array_sort, prim_array_matchval, \
        prim_array_matchkey, prim_array_extract, prim_array_join, \
        prim_array_cut, prim_array_compare, prim_array_sort_indexed, \
        prim_array_pin, prim_array_unpin, prim_array_get_ignorelist, \
        prim_array_nested_get, prim_array_nested_set, prim_array_nested_del, \
        prim_array_filter_flags, prim_array_interpret, prim_array_notify_secure, \
        prim_array_default_pinning, prim_array_filter_lock

#define PRIMS_ARRAY_NAMES "ARRAY_MAKE", "ARRAY_MAKE_DICT", \
        "ARRAY_EXPLODE", "ARRAY_VALS", "ARRAY_KEYS", \
        "ARRAY_FIRST", "ARRAY_LAST", "ARRAY_NEXT", "ARRAY_PREV", \
        "ARRAY_COUNT", "ARRAY_GETITEM", "ARRAY_SETITEM", \
        "ARRAY_INSERTITEM", "ARRAY_GETRANGE", "ARRAY_SETRANGE", \
        "ARRAY_INSERTRANGE", "ARRAY_DELITEM", "ARRAY_DELRANGE", \
        "ARRAY_NUNION", "ARRAY_NINTERSECT", \
        "ARRAY_NDIFF", "ARRAY_NOTIFY", "ARRAY_REVERSE", \
        "ARRAY_GET_PROPVALS", "ARRAY_GET_PROPDIRS", \
        "ARRAY_GET_PROPLIST", "ARRAY_PUT_PROPVALS", \
        "ARRAY_PUT_PROPLIST", "ARRAY_GET_REFLIST", \
        "ARRAY_PUT_REFLIST", "ARRAY_APPENDITEM", "ARRAY_FINDVAL", \
        "ARRAY_EXCLUDEVAL", "ARRAY_SORT", "ARRAY_MATCHVAL", \
        "ARRAY_MATCHKEY", "ARRAY_EXTRACT", "ARRAY_JOIN", \
        "ARRAY_CUT", "ARRAY_COMPARE", "ARRAY_SORT_INDEXED", \
        "ARRAY_PIN", "ARRAY_UNPIN", "ARRAY_GET_IGNORELIST", \
        "ARRAY_NESTED_GET", "ARRAY_NESTED_SET", "ARRAY_NESTED_DEL", \
        "ARRAY_FILTER_FLAGS", "ARRAY_INTERPRET", "ARRAY_NOTIFY_SECURE", \
        "ARRAY_DEFAULT_PINNING", "ARRAY_FILTER_LOCK"

#endif /* !P_ARRAY_H */
