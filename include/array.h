#ifndef _ARRAY_H
#define _ARRAY_H

#include <stddef.h>  /* for offsetof() */
#include "inst.h"

#define ARRAY_UNDEFINED		0
#define ARRAY_PACKED		1
#define ARRAY_DICTIONARY	2

typedef struct inst array_data;
typedef struct inst array_iter;

typedef struct array_tree_t {
    struct array_tree_t *left;
    struct array_tree_t *right;
    array_iter key;
    array_data data;
    short height;
} array_tree;

/* Linked list node structure for stk_arrays. This is used to track what stk_arrays are allocated
   to a MUF program and cleanup any extras due to circular references.

   The linked list is circular --- a designated stk_array_list of these represents the head/tail of
   the list (next = head pointer, prev = tail pointer), and is not associated with any stk_array.

   The others elements on the list (iterated by following the next pointers until one reaches the
   head/tail again) are members of stk_array, and offsetof() is used to access the overall stk_array.
 */
typedef struct stk_array_list_t {
    struct stk_array_list_t *next, *prev;
} stk_array_list;

/* The currently active stk_array_list to which to assign allocations.
   If we were multithreaded, this would be thread-local */
extern stk_array_list *stk_array_active_list;

typedef struct stk_array_t {
    int links;			/* number of pointers  to array */
    int items;			/* number of items in array */
    short type;			/* type of array */
    int pinned;			/* if pinned, don't dup array on changes */
    union {
	array_data *packed;	/* pointer to packed array */
	array_tree *dict;	/* pointer to dictionary AVL tree */
    } data;
    stk_array_list list_node;   /* list array is on, typically of all allocated to a MUF program */
} stk_array;

#define STK_ARRAY_FROM_LIST_NODE(x) \
    ((stk_array*) ((unsigned char*) (x) - offsetof(stk_array, list_node)))

int array_appenditem(stk_array **arr, array_data *item);
int array_count(stk_array *arr);
stk_array *array_decouple(stk_array *arr);
int array_delitem(stk_array **harr, array_iter *item);
int array_delrange(stk_array **harr, array_iter *start, array_iter *end);
stk_array *array_demote_only(stk_array *arr, int threshold, int pin);
int array_first(stk_array *arr, array_iter *item);
void array_free(stk_array *arr);
char *array_get_intkey_strval(stk_array *arr, int key);
array_data *array_getitem(stk_array *arr, array_iter *idx);
stk_array *array_getrange(stk_array *arr, array_iter *start, array_iter *end, int pin);
int array_idxcmp_case(array_iter *a, array_iter *b, int case_sens);
int array_insertitem(stk_array **arr, array_iter *idx, array_data *item);
int array_insertrange(stk_array **arr, array_iter *start, stk_array *inarr);
int array_is_homogenous(stk_array *arr, int typ);
int array_last(stk_array *arr, array_iter *item);
void array_mash(stk_array *arr_in, stk_array **mash, int value);
int array_next(stk_array *arr, array_iter *item);
int array_prev(stk_array *arr, array_iter *item);
int array_set_intkey(stk_array **harr, int key, struct inst *val);
int array_set_intkey_refval(stk_array **harr, int key, dbref val);
int array_set_intkey_strval(stk_array **harr, int key, const char *val);
void array_set_pinned(stk_array *arr, int pinned);
int array_set_strkey(stk_array **harr, const char *key, struct inst *val);
int array_set_strkey_intval(stk_array **arr, const char *key, int val);
int array_set_strkey_fltval(stk_array **arr, const char *key, double val);
int array_set_strkey_refval(stk_array **harr, const char *key, dbref val);
int array_set_strkey_strval(stk_array **harr, const char *key, const char *val);
int array_setitem(stk_array **arr, array_iter *idx, array_data *item);
int array_setrange(stk_array **arr, array_iter *start, stk_array *inarr);
stk_array *new_array_dictionary(int pin);
stk_array *new_array_packed(int size, int pin);

/* place the array on a list of arrays of tracking allocations
   unless it is already on one */
void array_maybe_place_on_list(stk_array_list *list, stk_array *array);
/* remove the array from any lists trackings its allocations */
void array_remove_from_list(stk_array *array);

void array_init_active_list(stk_array_list *list);

void array_free_all_on_list(stk_array_list *list);

#endif				/* _ARRAY_H */
