/** @file array.h
 *
 * Header for the different array types used in Fuzzball.  This is primarily
 * used for MUF primitives. 
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>  /* for offsetof() */
#include "config.h"
#include "inst.h"

#define ARRAY_UNDEFINED     0   /* No type yet                            */
#define ARRAY_PACKED        1   /* A 'packed' array is sequential numbers */
#define ARRAY_DICTIONARY    2   /* This is a mapping of keys to values    */

typedef struct inst array_data;
typedef struct inst array_iter;

/* Arrays are implemented as AVL trees much like property directories */
typedef struct array_tree_t {
    struct array_tree_t *left;      /* The left child node  */
    struct array_tree_t *right;     /* The right child node */
    array_iter key;                 /* Key for this node    */
    array_data data;                /* Data for this node   */
    short height;                   /* Height of node       */
} array_tree;

/* Linked list node structure for stk_arrays. This is used to track what
   stk_arrays are allocated to a MUF program and cleanup any extras due to
   circular references.

   The linked list is circular --- a designated stk_array_list of these
   represents the head/tail of the list (next = head pointer, prev = tail
   pointer), and is not associated with any stk_array.

   The others elements on the list (iterated by following the next pointers
   until one reaches the head/tail again) are members of stk_array, and
   offsetof() is used to access the overall stk_array.
 */
typedef struct stk_array_list_t {
    struct stk_array_list_t *next, *prev;
} stk_array_list;

/* The currently active stk_array_list to which to assign allocations.
   If we were multithreaded, this would be thread-local */
extern stk_array_list *stk_array_active_list;

typedef struct stk_array_t {
    int links;          /* number of pointers  to array */
    int items;          /* number of items in array */
    short type;         /* type of array */
    int pinned;         /* if pinned, don't dup array on changes */
    union {
        array_data *packed; /* pointer to packed array */
        array_tree *dict;   /* pointer to dictionary AVL tree */
    } data;
    stk_array_list list_node; /* list array is on, typically of all allocated 
                               * to a MUF program
                               */
} stk_array;

#define STK_ARRAY_FROM_LIST_NODE(x) \
    ((stk_array*) ((unsigned char*) (x) - offsetof(stk_array, list_node)))

/**
 * Append an item to the end of an array
 *
 * This only works with ARRAY_PACKED arrays.  It is literally a shortcut
 * for this (note this is pseudocode so don't attempt it):
 *
 * array_setitem(arr, array_count(arr), item)
 *
 * @see array_setitem
 *
 * @param arr Pointer to a pointer, the array you wish to operate on.
 * @param item The item to add to the array.
 * @return -1 on error, number of items in array on success
 */
int array_appenditem(stk_array **arr, array_data *item);

/**
 * Get the number of items in the array.
 *
 * If 'arr' is NULL, returns 0.
 *
 * @param arr the array to get the count for
 * @return the number of items in the array or 0 if arr is NULL
 */
int array_count(stk_array *arr);

/**
 * Make a "decoupled" copy of an array object
 *
 * This function makes a copy of an array, copying all its data as well.
 * The nomenclature of "decouple" is because it is most often used to
 * copy an unpinned array so that a copy may diverge from the original.
 *
 * Most of the possible errors here are memory allocation related and will
 * result in an abort(...) call.
 *
 * @param arr array to copy
 * @return a new array or NULL on (some) errors.  Most errors call abort(...)
 */
stk_array *array_decouple(stk_array *arr);

/**
 * Delete an item from an array
 *
 * This just uses array_delrange under the hood.
 *
 * @see array_delrange
 *
 * @param harr the array to operate on
 * @param item the index to remove.
 * @return -1 on error, or number of items remaining in array
 */
int array_delitem(stk_array **harr, array_iter *item);

/**
 * Delete a range of items from an array
 *
 * @param harr the array to operate on
 * @param start the first item to delete
 * @param end the last item in the range to delete
 * @return integer number of items still in array or -1 on error
 */
int array_delrange(stk_array **harr, array_iter *start, array_iter *end);

/**
 * Collapse a dictionary into a "packed" array
 *
 * The purpose of this is to basically use a dictionary as a unique
 * set, where the keys matter but the values do not.  The values must be
 * integers above threshold in order for the keys to show up in the
 * final array.
 *
 * @param arr the dictionary to 'demote'
 * @param threshold values must be integers larger than this number
 * @param pin is pinning turned on?
 * @return the packed array of keys that meet the criteria above.
 */
stk_array *array_demote_only(stk_array *arr, int threshold, int pin);

/**
 * Gets the first key of the given array
 *
 * For ARRAY_PACKED arrays, this is always 0.  For ARRAY_DICTIONARY,
 * this will be whatever the first entry's key is.
 *
 * @param arr The array to get the node for
 * @param item The key will be packed into this item.
 * @return 1 if item was populated with a key, 0 if it was not (empty array?)
 */
int array_first(stk_array *arr, array_iter *item);

/**
 * Free the memory used by the given array, respecting reference count
 *
 * Arrays have a reference count ("links").  Memory will not actually be
 * free'd until the reference count has reached 0.
 *
 * @param arr the array to free
 */
void array_free(stk_array *arr);

/**
 * Get a string value from an array assuming an integer key.
 *
 * If the value is not a string, it will return NULL.  Otherwise, it will
 * return the string.  NULL will also be returned if not found.
 *
 * @param arr the array you are fetching from
 * @param key the key to fetch
 * @return string value or NULL if not a string value (or not found)
 */
char *array_get_intkey_strval(stk_array *arr, int key);

/**
 * Fetch an item from the given array.
 *
 * Returns NULL in a variety of cases:
 *
 * - If a non-integer idx is used with ARRAY_PACKED.
 * - If an out of bounds idx is used with ARRAY_PACKED.
 * - If the key is not found in a dictionary.
 *
 * @param arr the array to fetch from
 * @param idx the index
 * @return the array item's data portion
 */
array_data *array_getitem(stk_array *arr, array_iter *idx);

/**
 * Return a range of values from an array, and returns it as a new array.
 *
 * This will return NULL on any sort of error, which would usually be
 * invalid start end.  You are responsible for freeing the memory of
 * the returned new array.
 *
 * For PACKED arrays, the usage is obvious, since keys are in numerical
 * order.  For DICTIONARY arrays, it returns key-values in alphabetical
 * order.
 *
 * For PACKED arrays, if start is below 0 it is changed to 0.  If
 * end is higher than the number of items in the array, it is smooshed to
 * the end of the array.
 *
 * For DICTIONARY arrays, it tries to find the next valid (for start) or
 * previous valid (for end) key if there aren't exact matches.
 *
 * @param arr the array to return values from
 * @param start the starting index
 * @param end the ending index
 * @param pin pin on or off?
 *
 * @return array containing the range, or NULL on any error.
 */
stk_array *array_getrange(stk_array *arr, array_iter *start, array_iter *end, int pin);

/**
 * Perform a comparison between two nodes.
 *
 * The nuances of the comparison are a little complicated:
 *
 * If they are both either floats or ints, compare to see which is greater.
 * If they are both strings, compare string values with given case sensitivity.
 * If not, but they are both the same type, compare their values logicly.
 * If not, then compare based on an arbitrary ordering of types.
 *
 * @private
 * @param a The first node to look at
 * @param b the node to compare it to
 * @param case_sens Case sensitive compare for strings?
 * @return similar to strcmp; 0 is equal, negative is A < B, positive is A > B
 */
int array_tree_compare(array_iter * a, array_iter * b, int case_sens);

/**
 * Insert an item into the given array at the given index.
 *
 * Returns size of array after addition, or -1 on failure.  This can
 * insert an item at any point in the array and works for both PACKED
 * and DICTIONARY.
 *
 * @param arr the array to operate on
 * @param idx the index to insert into
 * @param item the data to put in that node
 * @return the size of the array after the addition or -1 on failure.
 */
int array_insertitem(stk_array **arr, array_iter *idx, array_data *item);

/**
 * Insert a range of items into an array
 *
 * Inserts a range of items starting at index 'start' into array 'arr'
 * from 'inarr'.  For arrays, start says which part of the array to start
 * inserting at.  For dictionaries, 'start' is ignored but still must
 * be set.  I think, practically speaking, this call is not used for
 * dictionaries but they are supported.
 *
 * @param arr the array to insert into
 * @param start the starting index
 * @param inarr the array to insert into arr
 * @return size of array after inserts, or -1 on error
 */
int array_insertrange(stk_array **arr, array_iter *start, stk_array *inarr);

/**
 * Checks to see if an array only contains the given type
 *
 * @param arr the array to check
 * @param typ the type to check for
 * @return 1 if array only contains type 'typ', 0 otherwise.
 */
int array_is_homogenous(stk_array *arr, int typ);

/**
 * Get the last item in the array
 *
 * For PACKED, this will be array_count - 1.  For DICTIONARY, this will
 * be the item with the "highest" value.  The key value is put in
 * the provided 'item', and this returns boolean if item was populated.
 *
 * @param arr the array to get the last item from
 * @param item the struct we will copy the found key into
 * @return 0 if item was not populated, 1 if it was.
 */
int array_last(stk_array *arr, array_iter *item);

/**
 * Takes an array of keys, and a value, then manipulates 'mash' accordingly
 *
 * What this specifically does is, each value in 'arr_in' is used as a
 * key in 'mash'.  If the key already exists in 'mash' and is an integer,
 * 'value' is added to that key.  If the key already exists in 'mash' and
 * is any other value type, it is left alone.  If the key is unset, it
 * is set with the value 'value'.
 *
 * The original notes (which were somewhat innacurate) included a few
 * comments that probably are accurate:
 *
 * "This will be the core of the different/union/intersection code.
 *
 * Of course, this is going to absolutely blow chunks when passed an array
 * with value types that can't be used as key values in an array.  Blast.
 * That may be infeasible regardless, though."
 *
 * I'm not exactly sure what the potential problem here is.
 * @TODO: Figure out the potential problem here and determine if we need
 *        to fix it.
 *
 * @param arr_in the array of keys
 * @param mash the array to modify
 * @param value the value to set/add to values in mash
 */
void array_mash(stk_array *arr_in, stk_array **mash, int value);

/**
 * Get the next item in a given array with 'item' being the previous item.
 *
 * Given the previous item in the array, 'item', this method will clear
 * out whatever is in 'item' and replace it with the next item in the
 * array.  Please note that 'item' always gets cleared regardless of outcome.
 *
 * Returns 1 if 'item' was populated with something, 0 if it was not
 * (such as because you're at the end of the array)
 *
 * @param arr the array to check
 * @param item the previous item
 * @return 1 if 'item' was populated, 0 if it was not (end of array usually)
 */
int array_next(stk_array *arr, array_iter *item);

/**
 * Get the previous item in a given array with 'item' being the next item.
 *
 * Given the next item in the array, 'item', this method will clear
 * out whatever is in 'item' and replace it with the previous item in the
 * array.  Please note that 'item' always gets cleared regardless of outcome.
 *
 * Returns 1 if 'item' was populated with something, 0 if it was not
 * (such as because you're at the end of the array)
 *
 * @param arr the array to check
 * @param item the next item
 * @return 1 if 'item' was populated, 0 if it was not (end of array usually)
 */
int array_prev(stk_array *arr, array_iter *item);

/**
 * For a given array and integer key, set value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_intkey(stk_array **harr, int key, struct inst *val);

/**
 * For a given array and integer key, set integer value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_intkey_intval(stk_array **harr, int key, int val);

/**
 * For a given array and integer key, set dbref value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_intkey_refval(stk_array **harr, int key, dbref val);

/**
 * For a given array and integer key, set string value 'val'.
 *
 * @see array_setitem
 *
 * The string is copied, so there is no memory management concerns.
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_intkey_strval(stk_array **harr, int key, const char *val);

/**
 * For a given array and string key, set value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_strkey(stk_array **harr, const char *key, struct inst *val);

/**
 * For a given array and string key, set integer value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_strkey_intval(stk_array **arr, const char *key, int val);

/**
 * For a given array and string key, set double value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_strkey_fltval(stk_array **arr, const char *key, double val);

/**
 * For a given array and string key, set dbref value 'val'.
 *
 * @see array_setitem
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_strkey_refval(stk_array **harr, const char *key, dbref val);

/**
 * For a given array and string key, set string value 'val'.
 *
 * @see array_setitem
 *
 * The string is copied so there are no memory management issues.
 *
 * @param harr the array to operate n
 * @param key the key to set in the array
 * @param val the value to set at that key position
 * @return return value of array_setitem, which is -1 on failure or number of
 *         items in array after value set.
 */
int array_set_strkey_strval(stk_array **harr, const char *key, const char *val);

/**
 * Set value 'item' at key/index 'idx' for array 'arr'
 *
 * For PACKED arrays, you can set any index in the array that exists already
 * to overwrite, -or- you can set the array_count(arr) value (which is to
 * say, the value after the last value) to add a new element.  If you go
 * further out of bounds, it will return -1 as an error.
 *
 * For dictionaries, this is exactly the same as array_insertitem.
 *
 * @see array_insertitem
 *
 * @param arr the array to insert into
 * @param idx the key or index to set
 * @param item the value to set at the given index or key.
 * @return size of array after item is set, or -1 on failure.
 */
int array_setitem(stk_array **arr, array_iter *idx, array_data *item);

/**
 * Set a range of items on an array
 *
 * Sets / inserts a range of items starting at index 'start' into array 'arr'
 * from 'inarr'.  For arrays, start says which part of the array to start
 * inserting at.  For dictionaries, 'start' is ignored but still must
 * be set.  I think, practically speaking, this call is not used for
 * dictionaries but they are supported.
 *
 * This is functionally very similar to array_insertrange
 *
 * @see array_insertrange
 *
 * @param arr the array to insert into
 * @param start the starting index
 * @param inarr the array to insert into arr
 * @return size of array after inserts, or -1 on error
 */
int array_setrange(stk_array **arr, array_iter *start, stk_array *inarr);

/**
 * Allocate a new dictionary array
 *
 * @see array_set_pinned
 *
 * @param pin Boolean - if true, array will be pinned.  See array_set_pinned
 * @return a new dictionary array
 */
stk_array *new_array_dictionary(int pin);

/**
 * Allocate a new packed (sequential numeric index) array
 *
 * The array is initialized with integer '0' values if a size of greater
 * than 0 is provided.
 *
 * @see array_set_pinned
 *
 * @param size initial array size - must be at least 0
 * @param pin Boolean - if true, array will be pinned.  See array_set_pinned
 * @return a new packed array
 */
stk_array *new_array_packed(int size, int pin);

/**
 * Place the array on a list of arrays of tracking allocations unless it is
 * already on one.
 *
 * This is used exclusively by new_array to add a newly allocated array
 * to the array list.  In this case, the array is *always* added to
 * the list, which makes the "maybe" part of this call a little silly.
 *
 * However, theoretically, this could be used elsewhere, and this will
 * validate that an array does not wind up on the active list twice
 * by accident.
 *
 * @param list the active array list
 * @param array the array to add to the active list
 */
void array_maybe_place_on_list(stk_array_list *list, stk_array *array);

/**
 * Remove the array from any lists trackings its allocations
 *
 * This does validate if the array has already been removed, so a double
 * removal will not cause any harm.
 *
 * @param the array to remove
 */
void array_remove_from_list(stk_array *array);

/**
 * Initialize an active array list
 *
 * @param list the list to initialize
 */
void array_init_active_list(stk_array_list *list);

/**
 * Free all the arrays on an array list
 *
 * This is important for MUF cleanup, among other potential uses.
 */
void array_free_all_on_list(stk_array_list *list);

#endif /* !ARRAY_H */
