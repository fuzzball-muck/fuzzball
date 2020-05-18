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
 * Implementation of MUF ARRAY_MAKE
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
 * Implementation of MUF ARRAY_MAKE_DICT
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
 * Implementation of MUF ARRAY_EXPLODE
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
 * Implementation of MUF ARRAY_VALS
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
 * Implementation of MUF ARRAY_KEYS
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
 * Implementation of MUF ARRAY_COUNT
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
 * Implementation of MUF ARRAY_FIRST
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
 * Implementation of MUF ARRAY_LAST
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
 * Implementation of MUF ARRAY_NEXT
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
 * Implementation of MUF ARRAY_PREV
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
 * Implementation of MUF ARRAY_GETITEM
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
 * Implementation of MUF ARRAY_SETITEM
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
 * Implementation of MUF ARRAY_SETITEM
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
 * Implementation of MUF ARRAY_INSERTITEM
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
 * Implementation of MUF ARRAY_GETRANGE
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
 * Implementation of MUF ARRAY_SETRANGE
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
 * Implementation of MUF ARRAY_INSERTRANGE
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
 * Implementation of MUF ARRAY_DELITEM
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
 * Implementation of MUF ARRAY_DELRANGE
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

/**
 * Implementation of MUF ARRAY_COMPARE
 *
 * Consumes two arrays and puts an integer on the stack.  The integer
 * is positive if the first is considered greater than the second,
 * negative if the first is less than the second, or 0 if they are the
 * same.
 *
 * The greater than/less than comparison is of arguable value, according to
 * the man page.
 *
 * @see array_tree_compare
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_compare(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_FINDVAL
 *
 * Consumes an array and a value to search for.  Returns an array that
 * contains the keys of every element in the array that matches.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_findval(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_MATCHVAL
 *
 * Consumes an array of strings and a string, and then returns an array of
 * strings.
 *
 * The returned array with contain all they key-value pairs where the value
 * from the source array matched the pattern given.  The pattern is processed
 * using "smatch" rules.
 *
 * @see equalstr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_matchval(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_MATCHKEY
 *
 * Consumes an array of strings and a string, and then returns an array of
 * strings.
 *
 * The returned array with contain all they key-value pairs where the key
 * from the source array matched the pattern given.  The pattern is processed
 * using "smatch" rules.
 *
 * @see equalstr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_matchkey(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_EXTRACT
 *
 * Consumes two arrays; a source array and an array of indexes.
 *
 * Returns a dictionary array with the key-value pairs belonging to items
 * in the index array.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_extract(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_EXCLUDEVAL
 *
 * Consumes a list array and a value of any type.  Puts a list array on
 * the stack that includes all items in the list that did NOT match the
 * provided value.
 *
 * Ordering and duplicates will be preserved (wheras ARRAY_DIFF will remove
 * duplicates and the items will be sorted)
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_excludeval(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_SORT
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
void prim_array_sort(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_SORT_INDEXED
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
void prim_array_sort_indexed(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_JOIN
 *
 * Consumes a list array and a delimiter string, and returns a string that
 * is all the elements of the array concatenated by the delimiter.  Non
 * string items will be converted to strings in a reasonably smart way.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_join(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_INTERPRET
 *
 * Consumes a list array and concatenates all the items together as a string,
 * returning it -- it is a bit like ARRAY_JOIN with an empty string separator.
 *
 * But it's got a twist -- any DBREFs (TYPE_OBJECT) items in the list will
 * be converted to their object name.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_interpret(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_CUT
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

/**
 * Implementation of MUF ARRAY_NUNION
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
void prim_array_n_union(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NINTERSECT
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
void prim_array_n_intersection(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NDIFF
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
void prim_array_n_difference(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NOTIFY
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
void prim_array_notify(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_REVERSE
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
void prim_array_reverse(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_GET_PROPDIRS
 *
 * Consomes a dbref and a string containing a propdir, and puts a list array
 * containing all the sub-propdirs contained within that directory.  Any
 * prop-dirs not visible due to permission will not be seen.  MUCKER level 3
 * required.
 *
 * @see prop_read_perms
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_get_propdirs(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_GET_PROPVALS
 *
 * Consomes a dbref and a string containing a propdir, and puts a dictionary
 * containing a ma of prop names to prop values.  Subpropdirs with no value
 * associated with them are left out.  Any prop-dirs not visible due to
 * permission will not be seen.  MUCKER level 3 required.
 *
 * Only up to 511 properties can go into an array before this aborts.
 *
 * @see prop_read_perms
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_get_propvals(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_GET_PROPLIST
 *
 * Consumes a dbref and a property name
 *
 * This reads a "prop list" which is a propdir of the format:
 *
 * "propname#/X", "propname/X", or "propnameX"
 *
 * Where X is some number and the propdir itself contains the number of
 * elements in the array.  The prop list is loaded into an array and put on
 * the stack.
 *
 * If the number of props reported is greater than 1023, then this is
 * (silently, without error) limited to 1023 list items.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_get_proplist(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_PUT_PROPVALS
 *
 * Consumes a dbref and a dictionary.  Uses the dictionary keys as prop names
 * and sets those props to the corresponding values.  Permissions are respected.
 *
 * @see prop_write_perms
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_put_propvals(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_PUT_PROPLIST
 *
 * Consumes a dbref and a property name, and a list array
 *
 * This writes a "prop list" which is a propdir of the format:
 *
 * "propname#/X"
 *
 * Where X is some number and the propdir itself contains the number of
 * elements in the array.  The prop list is populated with the contents
 * of the provided array.
 *
 * Note that while ARRAY_GET_PROPLIST can read multiple list formats, we
 * can only write back one of them.  The '#' is added so you better not
 * put it on there or you'll get double #'s and then various things won't
 * work right.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_put_proplist(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_GET_REFLIST
 *
 * Consumes a dbref and a property name, and returns a list of dbrefs.
 *
 * This works on a "ref list" which is a space delimited string of dbrefs
 * with the leading hash sign.  For example, "#1 #2 #3"
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_get_reflist(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_PUT_REFLIST
 *
 * Consumes a dbref, a property name, and a list array of dbrefs.
 *
 * This stores the dbrefs in a reflist which is space delimited string of
 * refs that start with \#.  It is functionally equivalent to:
 *
 * @verbatim
 * #target_db "target/prop" { #1 #2 #3 }list " " array_join setprop
 * @endverbatim
 *
 * In fact, if we were inclined to do so, this could easily be converted
 * to a compiler macro, though there are probably more downsides to that
 * than upsides so it is not recommended.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_put_reflist(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_PIN
 *
 * Takes an array and sets its pinned bit, putting it back/leaving it on the
 * stack.
 *
 * If the array is already pinned, this makes a *copy* that will be pinned.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_pin(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_UNPIN
 *
 * Takes an array and removes its pinned bit, putting it back/leaving it on the
 * stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_unpin(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_DEFAULT_PINNING
 *
 * Consumes an integer (treated as boolean), returns nothing.
 *
 * Sets whether or not future arrays or dictionaries will be set pinned
 * by default.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_default_pinning(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_GET_IGNORELIST
 *
 * Consumes a DBREF of a player and returns a list-array of dbrefs
 *
 * The array contains the refs of players that the given player is
 * ignoring.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_get_ignorelist(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NESTED_GET
 *
 * Consumes two arrays, and puts a result value on the stack.
 *
 * This is the MUF way of doing "multi dimensional" arrays.  The first
 * parameter is the array of arrays.  The second parameter is a list array
 * of indexes to traverse.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_nested_get(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NESTED_SET
 *
 * Consumes a value and two arrays, and puts the altered array on the stack.
 *
 * This is the MUF way of doing "multi dimensional" arrays.  The first
 * parameter is the value to set.  The second parameter is the array
 * of arrays to set the value on.  The third parameter is a list array
 * of indexes to traverse.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_nested_set(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NESTED_DEL
 *
 * Consumes two arrays, and then puts the resulting modified array on the
 * stack.
 *
 * This is the MUF way of doing "multi dimensional" arrays.  The first
 * parameter is the array of arrays.  The second parameter is a list array
 * of indexes to traverse.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_nested_del(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_FILTER_FLAGS
 *
 * Consumes a list of dbrefs (PROG_OBJECT) and a string of flags, and
 * pushes a list array of dbrefs back onto the stack.
 *
 * Every object on the list is filtered by the flags.  @see checkflags
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_filter_flags(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_FILTER_LOCK
 *
 * Consumes a list of dbrefs (PROG_OBJECT) and a parsed lock, and
 * pushes a list array of dbrefs back onto the stack.
 *
 * Every object on the list is filtered by the lock.  @see eval_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_filter_lock(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_NOTIFY_SECURE
 *
 * Consumes 3 list arrays.  The first array is list of messages to send
 * over insecure descriptors.  The second array is a list of messages to
 * send over secure descriptors.  The third array is a list of dbrefs
 * (PROJ_OBJECT) to send these messages to.
 *
 * Listen props are only run with the secure message.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_notify_secure(PRIM_PROTOTYPE);

/**
 * Array primitive function handlers
 */
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

/**
 * Array primitive function names
 */
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
