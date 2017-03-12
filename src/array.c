#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "interp.h"

static int array_tree_compare(array_iter * a, array_iter * b, int case_sens);

/*
** This function compares two arrays in struct insts (array_iter's).
** The arrays are compared in order until the first difference.
** If the key is the difference, the comparison result is based on the key.
** If the value is the difference, the comparison result is based on the value.
** Comparison of keys and values is done by array_tree_compare().
*/
static int
array_tree_compare_arrays(array_iter * a, array_iter * b, int case_sens)
{
    int more1, more2, res;
    array_iter idx1;
    array_iter idx2;
    array_data *val1;
    array_data *val2;

    assert(a != NULL);
    assert(b != NULL);

    if (a->type != PROG_ARRAY || b->type != PROG_ARRAY) {
	return array_tree_compare(a, b, case_sens);
    }

    if (a->data.array == b->data.array) {
	return 0;
    }

    more1 = array_first(a->data.array, &idx1);
    more2 = array_first(b->data.array, &idx2);
    for (;;) {
	if (more1 && more2) {
	    val1 = array_getitem(a->data.array, &idx1);
	    val2 = array_getitem(b->data.array, &idx2);
	    res = array_tree_compare(&idx1, &idx2, case_sens);
	    if (res != 0) {
		return res;
	    }
	    res = array_tree_compare(val1, val2, case_sens);
	    if (res != 0) {
		return res;
	    }
	} else if (more1) {
	    return 1;
	} else if (more2) {
	    return -1;
	} else {
	    return 0;
	}
	more1 = array_next(a->data.array, &idx1);
	more2 = array_next(b->data.array, &idx2);
    }
}

/*
** Compares two array_iter's (struct insts)
** If they are both either floats or ints, compare to see which is greater.
** If they are both strings, compare string values with given case sensitivity.
** If not, but they are both the same type, compare their values logicly.
** If not, then compare based on an arbitrary ordering of types.
** Returns -1 is a < b.  Returns 1 is a > b.  Returns 0 if a == b.
*/
static int
array_tree_compare(array_iter * a, array_iter * b, int case_sens)
{
    assert(a != NULL);
    assert(b != NULL);
    if (a->type != b->type) {
	if (a->type == PROG_INTEGER && b->type == PROG_FLOAT) {
	    if (fabs(((double) a->data.number - b->data.fnumber) / (double) a->data.number) <
		DBL_EPSILON) {
		return 0;
	    } else if (a->data.number > b->data.fnumber) {
		return 1;
	    } else {
		return -1;
	    }
	} else if (a->type == PROG_FLOAT && b->type == PROG_INTEGER) {
	    if (fabs((a->data.fnumber - b->data.number) / a->data.fnumber) < DBL_EPSILON) {
		return 0;
	    } else if (a->data.fnumber > b->data.number) {
		return 1;
	    } else {
		return -1;
	    }
	}
	return (a->type - b->type);
    }
    /* Indexes are of same type if we reached here. */
    if (a->type == PROG_FLOAT) {
	if (a->data.fnumber == b->data.fnumber) {
	    return 0;
	} else if (fabs((a->data.fnumber - b->data.fnumber) / a->data.fnumber) < DBL_EPSILON) {
	    return 0;
	} else if (a->data.fnumber > b->data.fnumber) {
	    return 1;
	} else {
	    return -1;
	}
    } else if (a->type == PROG_STRING) {
	char *astr = DoNullInd(a->data.string);
	char *bstr = DoNullInd(b->data.string);

	if (0 != case_sens) {
	    return strcmp(astr, bstr);
	} else {
	    return strcasecmp(astr, bstr);
	}
    } else if (a->type == PROG_ARRAY) {
	return array_tree_compare_arrays(a, b, case_sens);
    } else if (a->type == PROG_LOCK) {
	/*
	 * In a perfect world, we'd compare the locks by element,
	 * instead of unparsing them into strings for strcmp()s.
	 */
	char *la;
	const char *lb;
	int retval = 0;

	la = strdup(unparse_boolexp((dbref) 1, a->data.lock, 0));
	lb = unparse_boolexp((dbref) 1, b->data.lock, 0);
	retval = strcmp(la, lb);
	free(la);
	return retval;
    } else if (a->type == PROG_ADD) {
	int result = (a->data.addr->progref - b->data.addr->progref);

	if (0 != result) {
	    return result;
	}
	return (a->data.addr->data - b->data.addr->data);
    } else {
	return (a->data.number - b->data.number);
    }
}

static array_tree *
array_tree_find(array_tree * avl, array_iter * key)
{
    int cmpval;

    assert(key != NULL);

    while (avl) {
	cmpval = array_tree_compare(key, &(avl->key), 0);
	if (cmpval > 0) {
	    avl = avl->right;
	} else if (cmpval < 0) {
	    avl = avl->left;
	} else {
	    break;
	}
    }
    return avl;
}

static short
array_tree_height_of(array_tree * node)
{
    if (node != NULL)
	return node->height;
    else
	return 0;
}

static int
array_tree_height_diff(array_tree * node)
{
    if (node != NULL)
	return (array_tree_height_of(node->right) - array_tree_height_of(node->left));
    else
	return 0;
}

static void
array_tree_fixup_height(array_tree * node)
{
    if (node != NULL)
	node->height = (short) (1 +
				MAX(array_tree_height_of(node->left),
				    array_tree_height_of(node->right)));
}

static array_tree *
array_tree_rotate_left_single(array_tree * a)
{
    array_tree *b;
    assert(a != NULL);
    b = a->right;

    a->right = b->left;
    b->left = a;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);

    return b;
}

static array_tree *
array_tree_rotate_left_double(array_tree * a)
{
    array_tree *b, *c;
    assert(a != NULL);
    b = a->right;
    c = b->left;

    assert(b != NULL);
    assert(c != NULL);

    a->right = c->left;
    b->left = c->right;
    c->left = a;
    c->right = b;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);
    array_tree_fixup_height(c);

    return c;
}

static array_tree *
array_tree_rotate_right_single(array_tree * a)
{
    array_tree *b;
    assert(a != NULL);
    b = a->left;

    a->left = b->right;
    b->right = a;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);

    return (b);
}

static array_tree *
array_tree_rotate_right_double(array_tree * a)
{
    array_tree *b, *c;
    assert(a != NULL);
    b = a->left;
    c = b->right;

    assert(b != NULL);
    assert(c != NULL);

    a->left = c->right;
    b->right = c->left;
    c->right = a;
    c->left = b;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);
    array_tree_fixup_height(c);

    return c;
}

static array_tree *
array_tree_balance_node(array_tree * a)
{
    int dh;
    assert(a != NULL);
    dh = array_tree_height_diff(a);

    if (abs(dh) < 2) {
	array_tree_fixup_height(a);
    } else {
	if (dh == 2) {
	    if (array_tree_height_diff(a->right) >= 0) {
		a = array_tree_rotate_left_single(a);
	    } else {
		a = array_tree_rotate_left_double(a);
	    }
	} else if (array_tree_height_diff(a->left) <= 0) {
	    a = array_tree_rotate_right_single(a);
	} else {
	    a = array_tree_rotate_right_double(a);
	}
    }
    return a;
}

static array_tree *
array_tree_alloc_node(array_iter * key)
{
    array_tree *new_node;
    assert(key != NULL);

    new_node = (array_tree *) calloc(1, sizeof(array_tree));
    if (!new_node) {
	fprintf(stderr, "array_tree_alloc_node(): Out of Memory!\n");
	abort();
    }

    new_node->left = NULL;
    new_node->right = NULL;
    new_node->height = 1;

    copyinst(key, &(new_node->key));
    new_node->data.type = PROG_INTEGER;
    new_node->data.data.number = 0;
    return new_node;
}

static void
array_tree_free_node(array_tree * p)
{
    assert(p->left == NULL);
    assert(p->right == NULL);
    CLEAR(&(p->key));
    CLEAR(&(p->data));
    free(p);
}

static array_tree *
array_tree_insert(array_tree ** avl, array_iter * key)
{
    array_tree *ret;
    register array_tree *p = *avl;
    register int cmp;
    static short balancep;

    assert(avl != NULL);
    assert(key != NULL);

    if (p) {
	cmp = array_tree_compare(key, &(p->key), 0);
	if (cmp > 0) {
	    ret = array_tree_insert(&(p->right), key);
	} else if (cmp < 0) {
	    ret = array_tree_insert(&(p->left), key);
	} else {
	    balancep = 0;
	    return (p);
	}
	if (0 != balancep) {
	    *avl = array_tree_balance_node(p);
	}
	return ret;
    } else {
	p = *avl = array_tree_alloc_node(key);
	balancep = 1;
	return (p);
    }
}

static array_tree *
array_tree_getmax(array_tree * avl)
{
    assert(avl != NULL);
    if (avl && avl->right)
	return array_tree_getmax(avl->right);
    return avl;
}

static array_tree *
array_tree_remove_node(array_iter * key, array_tree ** root)
{
    array_tree *save;
    array_tree *tmp;
    array_tree *avl;
    int cmpval;
    assert(root != NULL);
    assert(*root != NULL);
    assert(key != NULL);
    avl = *root;

    save = avl;
    if (avl) {
	cmpval = array_tree_compare(key, &(avl->key), 0);
	if (cmpval < 0) {
	    save = array_tree_remove_node(key, &(avl->left));
	} else if (cmpval > 0) {
	    save = array_tree_remove_node(key, &(avl->right));
	} else if (!(avl->left)) {
	    avl = avl->right;
	} else if (!(avl->right)) {
	    avl = avl->left;
	} else {
	    tmp = array_tree_remove_node(&((array_tree_getmax(avl->left))->key),
					 &(avl->left));
	    if (!tmp) {		/* this shouldn't be possible. */
		panic("array_tree_remove_node() returned NULL !");
	    }
	    tmp->left = avl->left;
	    tmp->right = avl->right;
	    avl = tmp;
	}
	if (save) {
	    save->left = NULL;
	    save->right = NULL;
	}
	*root = array_tree_balance_node(avl);
    }
    return save;
}

static array_tree *
array_tree_delete(array_iter * key, array_tree * avl)
{
    array_tree *save;
    assert(key != NULL);
    assert(avl != NULL);

    save = array_tree_remove_node(key, &avl);
    if (save)
	array_tree_free_node(save);
    return avl;
}

static void
array_tree_delete_all(array_tree * p)
{
    if (p == NULL)
	return;
    array_tree_delete_all(p->left);
    p->left = NULL;
    array_tree_delete_all(p->right);
    p->right = NULL;
    array_tree_free_node(p);
}

static array_tree *
array_tree_first_node(array_tree * list)
{
    if (list == NULL)
	return ((array_tree *) NULL);

    while (list->left)
	list = list->left;

    return (list);
}

static array_tree *
array_tree_last_node(array_tree * list)
{
    if (!list)
	return NULL;

    while (list->right)
	list = list->right;

    return (list);
}

static array_tree *
array_tree_prev_node(array_tree * ptr, array_iter * key)
{
    array_tree *from;
    int cmpval;

    if (ptr == NULL)
	return NULL;
    if (key == NULL)
	return NULL;
    cmpval = array_tree_compare(key, &(ptr->key), 0);
    if (cmpval > 0) {
	from = array_tree_prev_node(ptr->right, key);
	if (from)
	    return from;
	return ptr;
    } else if (cmpval < 0) {
	return array_tree_prev_node(ptr->left, key);
    } else if (ptr->left) {
	from = ptr->left;
	while (from->right)
	    from = from->right;
	return from;
    } else {
	return NULL;
    }
}

static array_tree *
array_tree_next_node(array_tree * ptr, array_iter * key)
{
    array_tree *from;
    int cmpval;

    if (ptr == NULL)
	return NULL;
    if (key == NULL)
	return NULL;
    cmpval = array_tree_compare(key, &(ptr->key), 0);
    if (cmpval < 0) {
	from = array_tree_next_node(ptr->left, key);
	if (from)
	    return from;
	return ptr;
    } else if (cmpval > 0) {
	return array_tree_next_node(ptr->right, key);
    } else if (ptr->right) {
	from = ptr->right;
	while (from->left)
	    from = from->left;
	return from;
    } else {
	return NULL;
    }
}

/*****************************************************************
 *  Stack Array Handling Routines
 *****************************************************************/

stk_array *
new_array(void)
{
    stk_array *nu;

    nu = (stk_array *) malloc(sizeof(stk_array));
    assert(nu != NULL);
    if (nu == NULL) {
	fprintf(stderr, "new_array(): Out of Memory!");
	abort();
    }
    nu->links = 1;
    nu->type = ARRAY_UNDEFINED;
    nu->items = 0;
    nu->pinned = 0;
    nu->data.packed = NULL;

    return nu;
}

stk_array *
new_array_packed(int size)
{
    stk_array *nu;

    if (size < 0) {
	return NULL;
    }

    nu = new_array();
    assert(nu != NULL);		/* Redundant, but I'm coding defensively */
    nu->items = size;
    nu->type = ARRAY_PACKED;
    if (size < 1)
	size = 1;
    nu->data.packed = (array_data *) malloc(sizeof(array_data) * size);
    if (nu->data.packed == NULL) {
	fprintf(stderr, "new_array_packed(): Out of Memory!");
	abort();
    }
    for (int i = size; i-- > 0;) {
	nu->data.packed[i].type = PROG_INTEGER;
	nu->data.packed[i].line = 0;
	nu->data.packed[i].data.number = 0;
    }
    return nu;
}

stk_array *
new_array_dictionary(void)
{
    stk_array *nu;

    nu = new_array();
    assert(nu != NULL);		/* Redundant, but I'm coding defensively */
    nu->type = ARRAY_DICTIONARY;
    return nu;
}

void
array_set_pinned(stk_array * arr, int pinned)
{
    /* Since this is a no-op if it's not actually an array,
     * should it ever be called with a NULL arr? */
    assert(arr != NULL);
    if (arr != NULL) {
	arr->pinned = pinned;
    }
}

stk_array *
array_decouple(stk_array * arr)
{
    stk_array *nu;

    if (arr == NULL) {
	return NULL;
    }

    nu = new_array();
    assert(nu != NULL);		/* Redundant, but I'm coding defensively */
    nu->pinned = arr->pinned;
    nu->type = arr->type;
    switch (arr->type) {
    case ARRAY_PACKED:{
	    nu->items = arr->items;
	    nu->data.packed = (array_data *) malloc(sizeof(array_data) * arr->items);
	    if (nu->data.packed == NULL) {
		fprintf(stderr, "array_decouple(): Out of Memory!");
		abort();
	    }
	    for (int i = arr->items; i-- > 0;) {
		copyinst(&arr->data.packed[i], &nu->data.packed[i]);
	    }
	    return nu;
	}

    case ARRAY_DICTIONARY:{
	    array_iter idx;
	    array_data *val;

	    if (array_first(arr, &idx)) {
		do {
		    val = array_getitem(arr, &idx);
		    assert(val != NULL);
		    array_setitem(&nu, &idx, val);
		} while (array_next(arr, &idx));
	    }
	    return nu;
	}

    default:
	break;
    }
    return NULL;
}

void
array_free(stk_array * arr)
{
    if (arr == NULL) {
	return;
    }
    arr->links--;
    if (arr->links > 0) {
	return;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    for (int i = arr->items; i-- > 0;) {
		CLEAR(&arr->data.packed[i]);
	    }
	    free(arr->data.packed);
	    break;
	}
    case ARRAY_DICTIONARY:
	array_tree_delete_all(arr->data.dict);
	break;
    default:{
	    assert(0);		/* should never get here */
	    break;
	}
    }
    arr->items = 0;
    arr->data.packed = NULL;
    arr->type = ARRAY_UNDEFINED;
    free(arr);
}

int
array_count(stk_array * arr)
{
    if (arr == NULL) {
	return 0;
    }
    return arr->items;
}

int
array_idxcmp_case(array_iter * a, array_iter * b, int case_sens)
{
    assert(a != NULL);
    assert(b != NULL);
    return array_tree_compare(a, b, case_sens);
}

int
array_first(stk_array * arr, array_iter * item)
{
    assert(item != NULL);
    if ((arr == NULL) || (arr->items == 0)) {
	return 0;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    item->type = PROG_INTEGER;
	    item->data.number = 0;
	    return 1;
	}
    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    p = array_tree_first_node(arr->data.dict);
	    if (p == NULL)
		return 0;
	    copyinst(&p->key, item);
	    return 1;
	}
    default:{
	    break;
	}
    }
    return 0;
}

int
array_last(stk_array * arr, array_iter * item)
{
    assert(item != NULL);
    if ((arr == NULL) || (arr->items == 0)) {
	return 0;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    item->type = PROG_INTEGER;
	    item->data.number = arr->items - 1;
	    return 1;
	}
    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    p = array_tree_last_node(arr->data.dict);
	    if (p == NULL)
		return 0;
	    copyinst(&p->key, item);
	    return 1;
	}
    default:{
	    break;
	}
    }
    return 0;
}

int
array_prev(stk_array * arr, array_iter * item)
{
    assert(item != NULL);
    if ((arr == NULL) || (arr->items == 0)) {
	return 0;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    int idx;

	    if (item->type == PROG_STRING) {
		CLEAR(item);
		return 0;
	    } else if (item->type == PROG_FLOAT) {
		if (item->data.fnumber >= arr->items) {
		    idx = arr->items - 1;
		} else {
		    idx = (int)item->data.fnumber - 1;
		}
	    } else {
		idx = item->data.number - 1;
	    }
	    CLEAR(item);
	    if (idx >= arr->items) {
		idx = arr->items - 1;
	    } else if (idx < 0) {
		return 0;
	    }
	    item->type = PROG_INTEGER;
	    item->data.number = idx;
	    return 1;
	}
    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    p = array_tree_prev_node(arr->data.dict, item);
	    CLEAR(item);
	    if (!p)
		return 0;
	    copyinst(&p->key, item);
	    return 1;
	}
    default:{
	    break;
	}
    }
    return 0;
}

int
array_next(stk_array * arr, array_iter * item)
{
    assert(item != NULL);
    if ((arr == NULL) || (arr->items == 0)) {
	return 0;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    int idx;

	    if (item->type == PROG_STRING) {
		CLEAR(item);
		return 0;
	    } else if (item->type == PROG_FLOAT) {
		if (item->data.fnumber < 0.0) {
		    idx = 0;
		} else {
		    idx = (int)item->data.fnumber + 1;
		}
	    } else {
		idx = item->data.number + 1;
	    }
	    CLEAR(item);
	    if (idx >= arr->items) {
		return 0;
	    } else if (idx < 0) {
		idx = 0;
	    }
	    item->type = PROG_INTEGER;
	    item->data.number = idx;
	    return 1;
	}
    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    p = array_tree_next_node(arr->data.dict, item);
	    CLEAR(item);
	    if (!p)
		return 0;
	    copyinst(&p->key, item);
	    return 1;
	}
    default:{
	    break;
	}
    }
    return 0;
}

array_data *
array_getitem(stk_array * arr, array_iter * idx)
{
    if (!arr || !idx) {
	return NULL;
    }
    switch (arr->type) {
    case ARRAY_PACKED:
	if (idx->type != PROG_INTEGER) {
	    return NULL;
	}
	if (idx->data.number < 0 || idx->data.number >= arr->items) {
	    return NULL;
	}
	return &arr->data.packed[idx->data.number];

    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    p = array_tree_find(arr->data.dict, idx);
	    if (!p) {
		return NULL;
	    }
	    return &p->data;
	}

    default:
	break;
    }
    return NULL;
}

int
array_setitem(stk_array ** harr, array_iter * idx, array_data * item)
{
    stk_array *arr;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(idx != NULL);
    if (!harr || !*harr || !idx) {
	return -1;
    }
    arr = *harr;
    switch (arr->type) {
    case ARRAY_PACKED:{
	    if (idx->type != PROG_INTEGER) {
		return -1;
	    }
	    if (idx->data.number >= 0 && idx->data.number < arr->items) {
		if (arr->links > 1 && !arr->pinned) {
		    arr->links--;
		    arr = *harr = array_decouple(arr);
		}
		CLEAR(&arr->data.packed[idx->data.number]);
		copyinst(item, &arr->data.packed[idx->data.number]);
		return arr->items;
	    } else if (idx->data.number == arr->items) {
		if (arr->links > 1 && !arr->pinned) {
		    arr->links--;
		    arr = *harr = array_decouple(arr);
		}
		arr->data.packed = (array_data *)
			realloc(arr->data.packed, sizeof(array_data) * (arr->items + 1));
		if (arr->data.packed == NULL) {
		    fprintf(stderr, "array_setitem(): Out of Memory!");
		    abort();
		}
		copyinst(item, &arr->data.packed[arr->items]);
		return (++arr->items);
	    } else {
		return -1;
	    }
	}

    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    p = array_tree_find(arr->data.dict, idx);
	    if (p) {
		CLEAR(&p->data);
	    } else {
		arr->items++;
		p = array_tree_insert(&arr->data.dict, idx);
	    }
	    copyinst(item, &p->data);
	    return arr->items;
	}

    default:
	break;
    }
    return -1;
}

int
array_insertitem(stk_array ** harr, array_iter * idx, array_data * item)
{
    stk_array *arr;
    int i;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(idx != NULL);
    assert(item != NULL);

    if (!harr || !*harr || !idx) {
	return -1;
    }
    arr = *harr;
    switch (arr->type) {
    case ARRAY_PACKED:{
	    if (idx->type != PROG_INTEGER) {
		return -1;
	    }
	    if (idx->data.number < 0 || idx->data.number > arr->items) {
		return -1;
	    }
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    arr->data.packed = (array_data *)
		    realloc(arr->data.packed, sizeof(array_data) * (arr->items + 1));
	    if (arr->data.packed == NULL) {
		fprintf(stderr, "array_insertitem(): Out of Memory!");
		abort();
	    }
	    for (i = arr->items++; i > idx->data.number; i--) {
		copyinst(&arr->data.packed[i - 1], &arr->data.packed[i]);
		CLEAR(&arr->data.packed[i - 1]);
	    }
	    copyinst(item, &arr->data.packed[i]);
	    return arr->items;
	}

    case ARRAY_DICTIONARY:{
	    array_tree *p;

	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    p = array_tree_find(arr->data.dict, idx);
	    if (p) {
		CLEAR(&p->data);
	    } else {
		arr->items++;
		p = array_tree_insert(&arr->data.dict, idx);
	    }
	    copyinst(item, &p->data);
	    return arr->items;
	}

    default:
	break;
    }
    return -1;
}

int
array_appenditem(stk_array ** harr, array_data * item)
{
    struct inst key;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(item != NULL);

    if (!harr || !*harr)
	return -1;
    if ((*harr)->type != ARRAY_PACKED)
	return -1;
    key.type = PROG_INTEGER;
    key.data.number = array_count(*harr);

    return array_setitem(harr, &key, item);
}

stk_array *
array_getrange(stk_array * arr, array_iter * start, array_iter * end)
{
    stk_array *nu;
    array_data *tmp;
    int sidx, eidx;

    assert(arr != NULL);
    assert(start != NULL);
    assert(end != NULL);

    if (!arr) {
	return NULL;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    array_iter idx;
	    array_iter didx;

	    if (start->type != PROG_INTEGER) {
		return NULL;
	    }
	    if (end->type != PROG_INTEGER) {
		return NULL;
	    }
	    sidx = start->data.number;
	    eidx = end->data.number;
	    if (sidx < 0) {
		sidx = 0;
	    } else if (sidx >= arr->items) {
		return NULL;
	    }
	    if (eidx >= arr->items) {
		eidx = arr->items - 1;
	    } else if (eidx < 0) {
		return NULL;
	    }
	    if (sidx > eidx) {
		return NULL;
	    }
	    idx.type = PROG_INTEGER;
	    idx.data.number = sidx;
	    didx.type = PROG_INTEGER;
	    didx.data.number = 0;
	    nu = new_array_packed(eidx - sidx + 1);
	    while (idx.data.number <= eidx) {
		tmp = array_getitem(arr, &idx);
		if (!tmp)
		    break;
		array_setitem(&nu, &didx, tmp);
		didx.data.number++;
		idx.data.number++;
	    }
	    return nu;
	}

    case ARRAY_DICTIONARY:{
	    array_tree *s;
	    array_tree *e;

	    nu = new_array_dictionary();
	    s = array_tree_find(arr->data.dict, start);
	    if (!s) {
		s = array_tree_next_node(arr->data.dict, start);
		if (!s) {
		    return nu;
		}
	    }
	    e = array_tree_find(arr->data.dict, end);
	    if (!e) {
		e = array_tree_prev_node(arr->data.dict, end);
		if (!e) {
		    return nu;
		}
	    }
	    if (array_tree_compare(&s->key, &e->key, 0) > 0) {
		return nu;
	    }
	    while (s) {
		array_setitem(&nu, &s->key, &s->data);
		if (s == e)
		    break;
		s = array_tree_next_node(arr->data.dict, &s->key);
	    }
	    return nu;
	}
    default:
	break;
    }
    return NULL;
}

int
array_setrange(stk_array ** harr, array_iter * start, stk_array * inarr)
{
    stk_array *arr;
    array_iter idx;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(start != NULL);
    assert(inarr != NULL);

    if (!harr || !*harr) {
	return -1;
    }
    arr = *harr;
    if (!inarr || !inarr->items) {
	return arr->items;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    if (!start) {
		return -1;
	    }
	    if (start->type != PROG_INTEGER) {
		return -1;
	    }
	    if (start->data.number < 0 || start->data.number > arr->items) {
		return -1;
	    }
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    if (array_first(inarr, &idx)) {
		do {
		    array_setitem(&arr, start, array_getitem(inarr, &idx));
		    start->data.number++;
		} while (array_next(inarr, &idx));
	    }
	    return arr->items;
	}

    case ARRAY_DICTIONARY:{
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    if (array_first(inarr, &idx)) {
		do {
		    array_setitem(&arr, &idx, array_getitem(inarr, &idx));
		} while (array_next(inarr, &idx));
	    }
	    return arr->items;
	}

    default:
	break;
    }
    return -1;
}

int
array_insertrange(stk_array ** harr, array_iter * start, stk_array * inarr)
{
    stk_array *arr;
    array_data *itm;
    array_iter idx;
    array_iter didx;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(start != NULL);
    assert(inarr != NULL);

    if (!harr || !*harr) {
	return -1;
    }
    arr = *harr;
    if (!inarr || !inarr->items) {
	return arr->items;
    }
    switch (arr->type) {
    case ARRAY_PACKED:{
	    if (!start) {
		return -1;
	    }
	    if (start->type != PROG_INTEGER) {
		return -1;
	    }
	    if (start->data.number < 0 || start->data.number > arr->items) {
		return -1;
	    }
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    arr->data.packed = (struct inst *)
		    realloc(arr->data.packed,
			    sizeof(array_data) * (arr->items + inarr->items));
	    if (arr->data.packed == NULL) {
		fprintf(stderr, "array_insertrange: Out of Memory!");
		abort();
	    }

	    copyinst(start, &idx);
	    copyinst(start, &didx);
	    idx.data.number = arr->items - 1;
	    didx.data.number = arr->items + inarr->items - 1;
	    while (idx.data.number >= start->data.number) {
		itm = array_getitem(arr, &idx);
		copyinst(itm, &arr->data.packed[didx.data.number]);
		CLEAR(itm);
		idx.data.number--;
		didx.data.number--;
	    }
	    if (array_first(inarr, &idx)) {
		do {
		    itm = array_getitem(inarr, &idx);
		    copyinst(itm, &arr->data.packed[start->data.number]);
		    start->data.number++;
		} while (array_next(inarr, &idx));
	    }
	    arr->items += inarr->items;
	    return arr->items;
	}

    case ARRAY_DICTIONARY:{
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    if (array_first(inarr, &idx)) {
		do {
		    array_setitem(&arr, &idx, array_getitem(inarr, &idx));
		} while (array_next(inarr, &idx));
	    }
	    return arr->items;
	}

    default:
	break;
    }
    return -1;
}

int
array_delrange(stk_array ** harr, array_iter * start, array_iter * end)
{
    stk_array *arr;
    array_data *itm;
    int sidx, eidx, totsize;
    array_iter idx;
    array_iter didx;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(start != NULL);
    assert(end != NULL);

    if (!harr || !*harr) {
	return -1;
    }
    arr = *harr;
    switch (arr->type) {
    case ARRAY_PACKED:{
	    if (start->type != PROG_INTEGER) {
		return -1;
	    }
	    if (end->type != PROG_INTEGER) {
		return -1;
	    }
	    if (arr->items == 0) {	/* nothing to do here */
		return 0;
	    }
	    sidx = start->data.number;
	    eidx = end->data.number;
	    if (sidx < 0) {
		sidx = 0;
	    } else if (sidx >= arr->items) {
		return -1;
	    }
	    if (eidx >= arr->items) {
		eidx = arr->items - 1;
	    } else if (eidx < 0) {
		return -1;
	    }
	    if (sidx > eidx) {
		return -1;
	    }
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    start->data.number = sidx;
	    end->data.number = eidx;
	    copyinst(end, &idx);
	    copyinst(start, &didx);
	    idx.data.number += 1;
	    while (idx.data.number < arr->items) {
		itm = array_getitem(arr, &idx);
		copyinst(itm, &arr->data.packed[didx.data.number]);
		CLEAR(itm);
		idx.data.number++;
		didx.data.number++;
	    }
	    arr->items -= (eidx - sidx + 1);
	    totsize = (arr->items) ? arr->items : 1;
	    arr->data.packed = (array_data *)
		    realloc(arr->data.packed, sizeof(array_data) * totsize);
	    if (arr->data.packed == NULL) {
		fprintf(stderr, "array_delrange(): Out of Memory!");
		abort();
	    }
	    return arr->items;
	}

    case ARRAY_DICTIONARY:{
	    array_tree *s;
	    array_tree *e;

	    s = array_tree_find(arr->data.dict, start);
	    if (!s) {
		s = array_tree_next_node(arr->data.dict, start);
		if (!s) {
		    return arr->items;
		}
	    }
	    e = array_tree_find(arr->data.dict, end);
	    if (!e) {
		e = array_tree_prev_node(arr->data.dict, end);
		if (!e) {
		    return arr->items;
		}
	    }
	    if (array_tree_compare(&s->key, &e->key, 0) > 0) {
		return arr->items;
	    }
	    if (arr->links > 1 && !arr->pinned) {
		arr->links--;
		arr = *harr = array_decouple(arr);
	    }
	    copyinst(&s->key, &idx);
	    while (s && array_tree_compare(&s->key, &e->key, 0) <= 0) {
		arr->data.dict = array_tree_delete(&s->key, arr->data.dict);
		arr->items--;
		s = array_tree_next_node(arr->data.dict, &idx);
	    }
	    CLEAR(&idx);
	    return arr->items;
	}

    default:
	break;
    }
    return -1;
}

int
array_delitem(stk_array ** harr, array_iter * item)
{
    array_iter idx;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(item != NULL);

    copyinst(item, &idx);
    result = array_delrange(harr, item, &idx);
    CLEAR(&idx);
    return result;
}

/*\
|*| array_demote_only
|*| array demote discards the values of a dictionary, and
|*| returns a packed list of the keys.
|*| (Useful because keys are ordered and unique, presumably.)
|*| (This allows the keys to be abused as sets.)
\*/
stk_array *
array_demote_only(stk_array * arr, int threshold)
{
    stk_array *demoted_array = NULL;
    int items_left = 0;
    array_iter current_key;
    array_iter *current_value;
    array_iter new_index;

    assert(arr != NULL);

    if (!arr || ARRAY_DICTIONARY != arr->type)
	return NULL;

    new_index.type = PROG_INTEGER;
    new_index.data.number = 0;
    demoted_array = new_array_packed(0);
    items_left = array_first(arr, &current_key);
    while (items_left) {
	current_value = array_getitem(arr, &current_key);
	if (PROG_INTEGER == current_value->type && current_value->data.number >= threshold) {
	    array_insertitem(&demoted_array, &new_index, &current_key);
	    new_index.data.number++;
	}
	items_left = array_next(arr, &current_key);
    }
    return demoted_array;
}

/*\
|*| array_mash
|*| Takes the lists of values from the first array and
|*| uses each value as a key in the second array.  For
|*| each key, the passed "change_by" value is applied to
|*| any existing value.  If the value does not exist, it
|*| is set.
|*| This will be the core of the different/union/intersection
|*| code.
|*| Of course, this is going to absolutely blow chunks when
|*| passed an array with value types that can't be used as
|*| key values in an array.  Blast.  That may be infeasible
|*| regardless, though.
\*/
void
array_mash(stk_array * arr_in, stk_array ** mash, int value)
{

    int still_values = 0;
    array_iter current_key;
    array_iter *current_keyval;
    array_iter *current_value;
    array_data temp_value;

    assert(arr_in != NULL);
    assert(mash != NULL);
    assert(*mash != NULL);

    if (NULL == arr_in || NULL == mash || NULL == *mash)
	return;

    still_values = array_first(arr_in, &current_key);
    while (still_values) {
	current_keyval = array_getitem(arr_in, &current_key);
	current_value = array_getitem(*mash, current_keyval);
	if (NULL != current_value) {
	    if (PROG_INTEGER == current_value->type) {
		copyinst(current_value, &temp_value);
		temp_value.data.number += value;
		array_setitem(mash, current_keyval, &temp_value);
	    }
	} else {
	    temp_value.type = PROG_INTEGER;
	    temp_value.data.number = value;
	    array_insertitem(mash, current_keyval, &temp_value);
	}
	still_values = array_next(arr_in, &current_key);
    }
}

int
array_is_homogenous(stk_array * arr, int typ)
{
    array_iter idx;
    array_data *dat;
    int failedflag = 0;

    assert(arr != NULL);

    if (array_first(arr, &idx)) {
	do {
	    dat = array_getitem(arr, &idx);
	    if (dat->type != typ) {
		failedflag = 1;
	    }
	} while (array_next(arr, &idx));
    }
    return (!failedflag);
}

int
array_set_strkey(stk_array ** harr, const char *key, struct inst *val)
{
    struct inst name;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(key != NULL);
    assert(val != NULL);

    name.type = PROG_STRING;
    name.data.string = alloc_prog_string(key);

    result = array_setitem(harr, &name, val);

    CLEAR(&name);

    return result;
}

int
array_set_strkey_intval(stk_array ** arr, const char *key, int val)
{
    struct inst value;
    int result;

    assert(arr != NULL);
    assert(*arr != NULL);
    assert(key != NULL);

    value.type = PROG_INTEGER;
    value.data.number = val;

    result = array_set_strkey(arr, key, &value);

    CLEAR(&value);

    return result;
}

int
array_set_strkey_fltval(stk_array ** arr, const char *key, double val)
{
    struct inst value;
    int result;

    assert(arr != NULL);
    assert(*arr != NULL);
    assert(key != NULL);

    value.type = PROG_FLOAT;
    value.data.fnumber = val;

    result = array_set_strkey(arr, key, &value);

    CLEAR(&value);

    return result;
}

int
array_set_strkey_strval(stk_array ** harr, const char *key, const char *val)
{
    struct inst value;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(key != NULL);
    assert(val != NULL);

    value.type = PROG_STRING;
    value.data.string = alloc_prog_string(val);

    result = array_set_strkey(harr, key, &value);

    CLEAR(&value);

    return result;
}

int
array_set_strkey_refval(stk_array ** harr, const char *key, dbref val)
{
    struct inst value;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(key != NULL);
    assert((int) val >= -4);

    value.type = PROG_OBJECT;
    value.data.objref = val;

    result = array_set_strkey(harr, key, &value);

    CLEAR(&value);

    return result;
}

int
array_set_intkey(stk_array ** harr, int key, struct inst *val)
{
    struct inst name;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(val != NULL);

    name.type = PROG_INTEGER;
    name.data.number = key;

    result = array_setitem(harr, &name, val);

    CLEAR(&name);

    return result;
}

int
array_set_intkey_refval(stk_array ** harr, int key, dbref val)
{
    struct inst value;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert((int) val >= -4);

    value.type = PROG_OBJECT;
    value.data.objref = val;

    result = array_set_intkey(harr, key, &value);

    CLEAR(&value);

    return result;
}

int
array_set_intkey_strval(stk_array ** harr, int key, const char *val)
{
    struct inst value;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);
    assert(val != NULL);

    value.type = PROG_STRING;
    value.data.string = alloc_prog_string(val);

    result = array_set_intkey(harr, key, &value);

    CLEAR(&value);

    return result;
}

char *
array_get_intkey_strval(stk_array * arr, int key)
{
    struct inst ikey;
    array_data *value;

    assert(arr != NULL);

    ikey.type = PROG_INTEGER;
    ikey.data.number = key;

    value = array_getitem(arr, &ikey);

    CLEAR(&ikey);

    if (!value || value->type != PROG_STRING) {
	return NULL;
    } else if (!value->data.string) {
	return "";
    } else {
	return value->data.string->data;
    }
}

