/* Array/Dictionary Package */

#ifndef _ARRAY_H
#define _ARRAY_H

#define ARRAY_UNDEFINED		0
#define ARRAY_PACKED		1
#define ARRAY_DICTIONARY	2

typedef struct inst array_iter;
typedef struct inst array_data;

typedef struct array_tree_t {
	struct array_tree_t *left;
	struct array_tree_t *right;
	array_iter key;
	array_data data;
	short height;
} array_tree;

typedef struct stk_array_t {
	int links;					/* number of pointers  to array */
	int items;					/* number of items in array */
	short type;					/* type of array */
	int pinned;				/* if pinned, don't dup array on changes */
	union {
		array_data *packed;		/* pointer to packed array */
		array_tree *dict;		/* pointer to dictionary AVL tree */
	} data;
} stk_array;

stk_array *new_array_dictionary(void);
stk_array *new_array_packed(int size);
void array_free(stk_array * arr);
stk_array *array_promote(stk_array * arr);
stk_array *array_decouple(stk_array * arr);
void array_set_pinned(stk_array * arr, int pinned);

int array_count(stk_array * arr);
int array_idxcmp(array_iter * a, array_iter * b);
int array_idxcmp_case(array_iter * a, array_iter * b, int case_sens);

 /**/ int array_keys_homogenous(stk_array * arr);
 /**/ int array_vals_homogenous(stk_array * arr);
int array_contains_key(stk_array * arr, array_iter * item);
int array_contains_value(stk_array * arr, array_data * item);

int array_first(stk_array * arr, array_iter * item);
int array_last(stk_array * arr, array_iter * item);
int array_prev(stk_array * arr, array_iter * item);
int array_next(stk_array * arr, array_iter * item);

array_data *array_getitem(stk_array * arr, array_iter * idx);
int array_setitem(stk_array ** arr, array_iter * idx, array_data * item);
int array_insertitem(stk_array ** arr, array_iter * idx, array_data * item);
int array_appenditem(stk_array ** arr, array_data * item);

stk_array *array_getrange(stk_array * arr, array_iter * start, array_iter * end);
int array_setrange(stk_array ** arr, array_iter * start, stk_array * inarr);
int array_insertrange(stk_array ** arr, array_iter * start, stk_array * inarr);

stk_array *array_demote_only(stk_array * arr, int threshold);
void array_mash(stk_array * arr_in, stk_array ** mash, int value);

int array_is_homogenous(stk_array * arr, int typ);

int array_set_strkey(stk_array ** harr, const char *key, struct inst *val);

int array_set_strkey_intval(stk_array ** arr, const char *key, int val);
int array_set_strkey_fltval(stk_array ** arr, const char *key, double val);
int array_set_strkey_refval(stk_array ** harr, const char *key, dbref val);
int array_set_strkey_strval(stk_array ** harr, const char *key, const char *val);
int array_set_strkey_arrval(stk_array ** harr, const char *key, stk_array* val);

int array_set_intkey(stk_array ** harr, int key, struct inst *val);

int array_set_intkey_intval(stk_array ** harr, int key, int val);
int array_set_intkey_fltval(stk_array ** harr, int key, double val);
int array_set_intkey_refval(stk_array ** harr, int key, dbref val);
int array_set_intkey_strval(stk_array ** harr, int key, const char *val);
int array_set_intkey_arrval(stk_array ** harr, int key, stk_array* val);

char* array_get_intkey_strval(stk_array * arr, int key);

int array_set_intval(stk_array ** harr, struct inst* key, int val);
int array_set_fltval(stk_array ** harr, struct inst* key, double val);
int array_set_refval(stk_array ** harr, struct inst* key, dbref val);
int array_set_strval(stk_array ** harr, struct inst* key, const char *val);
int array_set_arrval(stk_array ** harr, struct inst* key, stk_array* val);

#endif /* _ARRAY_H */
