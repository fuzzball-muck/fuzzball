/** @file array.c
 *
 * Source for the different array types used in Fuzzball.  This is primarily
 * used for MUF primitives. 
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"

/* This keeps track of all active arrays but its not clear to me exactly
 * how (or why) this is used
 *
 * @TODO: Come back and document this better after documenting interp.c
 *        and/or p_misc.c
 */
stk_array_list *stk_array_active_list;

/* See the definition for full comment */
int array_tree_compare(array_iter * a, array_iter * b, int case_sens);

/* The buckets themselves */
typedef struct visited_set_bucket_t {
    stk_array *value;                       /* The value in the bucket      */
    struct visited_set_bucket_t *next;      /* Next bucket at this position */
} visited_set_bucket;

/*
 * A visited set is a basic hash set
 */
typedef struct visited_set_t {
    visited_set_bucket* buckets;    /* Buckets in the hash */
    int num_buckets;                /* Number of buckets   */
    int num_elements;               /* Number of elements  */
} visited_set;

/**
 * This is a simple hash function, for hashing memory addresses.
 *
 * @private
 * @param value to hash
 * @return the hash result
 */
static unsigned int visited_set_hash(stk_array *value) {
    return (unsigned long)value / 8;
}

/* See definition for full information */
static int visited_set_add_and_return_if_added(visited_set *set, stk_array *value);

/**
 * Initialize a visited set
 *
 * This is mostly used when a visited_set is a stack variable.  It is
 * actually pretty dangerous because if used on an already initialized
 * visited_set, it will cause a memory leak.  Thus, be careful with it.
 *
 * @private
 * @param set the set to empty
 */
static void visited_set_empty(visited_set *set) {
    set->buckets = 0;
    set->num_buckets = 0;
    set->num_elements = 0;
}

/**
 * Do a rehash on a visited_set hash
 *
 * This is done to grow the hash table
 *
 * @private
 * @param set the hash to rehash
 * @param new_size the new size to scale to
 */
static void visited_set_rehash(visited_set *set, int new_size) {
    visited_set new_set;
    visited_set_empty(&new_set);
    new_set.num_buckets = new_size;

    for (int i = 0; i < set->num_buckets; ++i) {
        visited_set_bucket *bucket = &set->buckets[i];

        if (bucket->value) {
            visited_set_add_and_return_if_added(&new_set, bucket->value);
        }

        if (bucket->next) {
            int is_first = 1;

            do {
                visited_set_bucket *next_bucket = bucket->next;

                if (!is_first)
                    free(bucket);

                is_first = 0;
                bucket = next_bucket;
                (void) visited_set_add_and_return_if_added(&new_set, bucket->value);
            } while (bucket->next);

            free(bucket);
        }
    }

    *set = new_set;
}

/**
 * Add an address to the visited set, return true if added, 0 if already in set
 *
 * Does exactly what it says it does.  Provides a short cut so that you
 * don't have to do a separate find -- just try to add and get the return
 * value of this.
 *
 * @private
 * @param set the set to add to
 * @param value the value to add to the set
 * @return 1 if added, 0 if value is already in the set.
 */
static int visited_set_add_and_return_if_added(visited_set* set, stk_array *value) {
    visited_set_bucket *bucket;

    if (!set->buckets) {
        /* @TODO this probably doesn't come up too much, but, 15 buckets
         *       as an initial size is pretty small.  We can actually make
         *       a really good guess as to how many buckets we need
         *       (array size + (array_size /3) should do since I think this
         *       re-hashes at /4.  The exact mechanism for passing in the
         *       array size could be at initialization time, such as with
         *       visited_set_empty
         *
         *       I mean, honestly, there's no reason this needs to ever
         *       rehash if we do this right.  We could even theoretically
         *       rip out the rehash feature.
         */
        set->num_buckets = 15;
        set->num_elements = 0;
        set->buckets = calloc(set->num_buckets, sizeof(*set->buckets));

        if (!set->buckets) {
            perror("check_and_add_to_visited: out of memory!");
            abort();
        }
    }

    bucket = &set->buckets[visited_set_hash(value) % set->num_buckets];

    while (bucket->value != value && bucket->next) {
        bucket = bucket->next;
    }

    if (bucket->value == value) {
        return 0;
    } else {
        bucket->next = malloc(sizeof(visited_set_bucket));

        if (!bucket->next) {
            perror("check_and_add_to_visited: out of memory!");
            abort();
        }

        bucket->next->value = value;
        bucket->next->next = NULL;

        if (++set->num_elements > set->num_buckets * 3 / 4) {
            visited_set_rehash(set, set->num_buckets * 2 + 1);
        }

        return 1;
    }
}

/**
 * Free the memory for a visited set hash
 *
 * @private
 * @param set the set to free up
 */
static void visited_set_free(visited_set* set) {
    for (int i = 0; i < set->num_buckets; ++i) {
        visited_set_bucket *bucket = &set->buckets[i];

        if (bucket->next) {
            int is_first = 1;

            do {
                visited_set_bucket *next_bucket = bucket->next;

                if (!is_first)
                    free(bucket);

                bucket = next_bucket;
            } while (bucket->next);

            free(bucket);
        }
    }

    free(set->buckets);
}

/* See the definition for details about this function */
static int
array_tree_compare_internal(array_iter * a, array_iter * b, int case_sens, visited_set *visited_a, visited_set *visited_b);

/**
 * Compares two arrays
 *
 * The arrays are compared in order until the first difference.
 * If the key is the difference, the comparison result is based on the key.
 * If the value is the difference, the comparison result is based on the value.
 * Comparison of keys and values is done by array_tree_compare_internal().
 *
 * @see array_tree_compare_internal
 *
 * @private
 * @param a the first array
 * @param b the second array
 * @param case_sens case sensitive compares for strings?
 * @param visited_a visited set for a
 * @param visited_b visited set for b
 * @return similar to strcmp; 0 is equal, negative is A < B, positive is A > B
 */
static int
array_tree_compare_arrays(array_iter * a, array_iter * b, int case_sens, visited_set *visited_a, visited_set *visited_b)
{
    int more1, more2, res;
    array_iter idx1;
    array_iter idx2;
    array_data *val1;
    array_data *val2;

    assert(a != NULL);
    assert(b != NULL);

    if (a->type != PROG_ARRAY || b->type != PROG_ARRAY) {
        return array_tree_compare_internal(a, b, case_sens, visited_a, visited_b);
    }

    if (a->data.array == b->data.array) {
        return 0;
    }

    /* I believe this is to avoid accidental recursion; this way, we make
     * sure we only check each array once because the visited_set is 
     * a hash of memory addresses.
     */
    if (!visited_set_add_and_return_if_added(visited_a, a->data.array) ||
        !visited_set_add_and_return_if_added(visited_b, b->data.array)) {
        if (a->data.array > b->data.array) {
            return 1;
        } else {
            return -1;
        }
    }

    /* Get the first node of each array */
    more1 = array_first(a->data.array, &idx1);
    more2 = array_first(b->data.array, &idx2);

    for (;;) {
        if (more1 && more2) {
            /* Compare keys */
            val1 = array_getitem(a->data.array, &idx1);
            val2 = array_getitem(b->data.array, &idx2);
            res = array_tree_compare_internal(&idx1, &idx2, case_sens, visited_a, visited_b);

            if (res != 0) {
                return res;
            }

            /* Then compare values */
            res = array_tree_compare_internal(val1, val2, case_sens, visited_a, visited_b);

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

        /* Iterate over the entire structure until one of them proves
         * to be different, or they turn out to be the same.
         */
        more1 = array_next(a->data.array, &idx1);
        more2 = array_next(b->data.array, &idx2);
    }
}

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
int
array_tree_compare(array_iter * a, array_iter * b, int case_sens) {
    visited_set visited_a, visited_b;
    visited_set_empty(&visited_a);
    visited_set_empty(&visited_b);
    int result = array_tree_compare_internal(a, b, case_sens, &visited_a, &visited_b);
    visited_set_free(&visited_a);
    visited_set_free(&visited_b);
    return result;
}

/**
 * This is the internal implementation of the comparison between two nodes.
 *
 * In particular, this call handles the passing of visited_sets which are
 * used in array comparisons.  You probably want array_tree_compare
 * instead of this method if you're looking for a comparison call.
 *
 * @see array_tree_compare
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
 * @param visited_a visited nodes set A
 * @param visited_b visited nodes set B
 * @return similar to strcmp; 0 is equal, negative is A < B, positive is A > B
 */
static int
array_tree_compare_internal(array_iter * a, array_iter * b, int case_sens, visited_set *visited_a, visited_set *visited_b)
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
        return array_tree_compare_arrays(a, b, case_sens, visited_a, visited_b);
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

/**
 * Find a given key in the tree avl
 *
 * @private
 * @param avl the avl tree
 * @param key the key to find
 * @return the found node
 */
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

/**
 * Get the height of the given node
 *
 * This has 'friendly' node handling, returning 0 on a NULL
 *
 * @private
 * @param the node to be computed
 * @return the height of the given node
 */
static short
array_tree_height_of(array_tree * node)
{
    if (node != NULL)
        return node->height;
    else
        return 0;
}

/**
 * Compute the height difference between the left and right nodes on a node
 *
 * @private
 * @param node the node to compute the height difference for
 * @return the height difference.  Negative means left is 'taller'.
 */
static int
array_tree_height_diff(array_tree * node)
{
    if (node != NULL)
        return (array_tree_height_of(node->right) - array_tree_height_of(node->left));
    else
        return 0;
}

/**
 * Adjusts the height of node 'node'
 *
 * @private
 * @param node the node to do the height correction on.
 */
static void
array_tree_fixup_height(array_tree * node)
{
    if (node != NULL)
        node->height = (short) (1 +
                        MAX(array_tree_height_of(node->left),
                        array_tree_height_of(node->right)));
}

/**
 * Does a 'single left' rotation on node a.  Used in balancing.
 *
 * @private
 * @param a the node to rotate
 * @return the rotated node
 */
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

/**
 * Does a 'double left' rotation on node a.  Used in balancing.
 *
 * @private
 * @param a the node to rotate
 * @return the rotated node
 */
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

/**
 * Does a 'single right' rotation on node a.  Used in balancing.
 *
 * @private
 * @param a the node to rotate
 * @return the rotated node
 */
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

/**
 * Does a 'double right' rotation on node a.  Used in balancing.
 *
 * @private
 * @param a the node to rotate
 * @return the rotated node
 */
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

/**
 * Rebalance the AVL tree
 *
 * This is an essential part of AVL that, honestly, its been too many
 * years since college for me to remember exactly how it works.  But
 * basically it reorganizes the structure to make sure the nodes are
 * in the right order, and is done after an insert or delete.
 *
 * @private
 * @param a the tree to rebalance
 * @return the re-balanced tree.
 */
static array_tree *
array_tree_balance_node(array_tree * a)
{
    int dh;
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

/**
 * Alocate a new node with the given key.
 *
 * It initializes the data with an integer 0
 *
 * @private
 * @param key to initialize - its data is copied.
 * @return the allocated node
 */
static array_tree *
array_tree_alloc_node(array_iter * key)
{
    array_tree *new_node;
    assert(key != NULL);

    new_node = calloc(1, sizeof(array_tree));
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

/**
 * Insert a new node into the AVL tree
 *
 * Note that this uses a static variable to keep track if we need to balance
 * the tree.  That makes this call not threadsafe; however how Fuzzball
 * is constructed means there is little risk of one call of this
 * inappropriately interfering with another.
 *
 * Note that if the pointer that avl points to is NULL, it will be replaced
 * with a valid node.
 *
 * @private
 * @param avl the AVL tree to work on.
 * @param key the key to add
 * @return the created node, or the existing node if the key is already there
 */
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

/**
 * Get the last node in the tree.
 *
 * The last node is the rightmost one.
 *
 * @private
 * @param list the array to scan
 * @return pointer to 'last node'
 */
static array_tree *
array_tree_last_node(array_tree * list)
{
    if (!list)
        return NULL;

    while (list->right)
        list = list->right;

    return (list);
}

/**
 * Remove a given key from an AVL tree structure
 *
 * This does the 'low level' work, you probably want array_tree_delete
 * instead.
 *
 * @private
 * @param key the key to delete
 * @param root the root node to start from.
 * @return the node to delete (it's removed from the tree)
 */
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
        /* Traverse the tree based on comparison */
        cmpval = array_tree_compare(key, &(avl->key), 0);

        if (cmpval < 0) {
            /* Move left if less */
            save = array_tree_remove_node(key, &(avl->left));
        } else if (cmpval > 0) {
            /* Move right if more */
            save = array_tree_remove_node(key, &(avl->right));
        } else if (!(avl->left)) { /* we found the node - if it has no left,
                                    * switch this node to become the right.
                                    */
            avl = avl->right;
        } else if (!(avl->right)) { /* If it has no right, switch it to become
                                     * the left.
                                     */
            avl = avl->left;
        } else {
            /* Otherwise, recurse into the left to find the node to
             * bring up to the current position.
             */
            tmp = array_tree_remove_node(&((array_tree_last_node(avl->left))->key),
                                         &(avl->left));
            if (!tmp) {    /* this shouldn't be possible. */
                panic("array_tree_remove_node() returned NULL !");
            }

            tmp->left = avl->left;
            tmp->right = avl->right;
            avl = tmp;
        }

        /* If we found the node, then blank out the left/right */
        if (save) {
            save->left = NULL;
            save->right = NULL;
        }

        /* Re-balance the tree */
        *root = array_tree_balance_node(avl);
    }

    return save;
}

/**
 * Remove a given key from an AVL tree structure
 *
 * This is the higher level wrapper around array_tree_remove_node
 *
 * @see array_tree_remove_node
 *
 * @private
 * @param key the key to delete
 * @param root the root node to start from.
 * @return the node to delete (it's removed from the tree)
 */
static array_tree *
array_tree_delete(array_iter * key, array_tree * avl)
{
    array_tree *save;
    assert(key != NULL);
    assert(avl != NULL);

    if ((save = array_tree_remove_node(key, &avl))) {
        CLEAR(&(save->key));
        CLEAR(&(save->data));
        free(save);
    }

    return avl;
}

/**
 * Delete all the nodes in a tree, recursively.
 *
 * @private
 * @param p the tree to clear out
 */
static void
array_tree_delete_all(array_tree * p)
{
    if (p == NULL)
        return;

    array_tree_delete_all(p->left);
    p->left = NULL;
    array_tree_delete_all(p->right);
    p->right = NULL;
    CLEAR(&(p->key));
    CLEAR(&(p->data));
    free(p);
}

/**
 * Get the first node in the tree.
 *
 * The first node is the leftmost one.
 *
 * @private
 * @param list the list to scan.
 * @return a pointer to the first node in the tree.
 */
static array_tree *
array_tree_first_node(array_tree * list)
{
    if (list == NULL)
        return ((array_tree *) NULL);

    while (list->left)
        list = list->left;

    return (list);
}

/**
 * Get the next previous in the AVL tree
 *
 * @TODO: This has a lot of overlap with prop AVL trees.  Can we merge
 *        the implementations?
 *
 * @private
 * @param ptr the source node
 * @param key the next key
 *
 * @return the previous node in the AVL or NULL if there is no previous.
 */
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

    /* If cmpval is greater than our current node, then we need to go right.
     * If we have a right node, then return it, otherwise the current node
     * is the previous.
     */
    if (cmpval > 0) {
        from = array_tree_prev_node(ptr->right, key);

        if (from)
            return from;

        return ptr;
    } else if (cmpval < 0) {
        /* If the cmpval is less than our current node, search left */
        return array_tree_prev_node(ptr->left, key);
    } else if (ptr->left) {
        /* Othewise, we match, and we want the rightmost node of oru
         * left node.
         */
        from = ptr->left;

        while (from->right)
            from = from->right;

        return from;
    } else {
        /* Otherwise not found */
        return NULL;
    }
}

/**
 * Get the next node in the AVL tree
 *
 * @TODO: This has a lot of overlap with prop AVL trees.  Can we merge
 *        the implementations?
 *
 * @private
 * @param ptr the source node
 * @param key the previous key
 *
 * @return the next node in the AVL or NULL if there is no next.
 */
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

    /* If the node value is less than the key, go left.
     * Otherwise, go right
     */
    if (cmpval < 0) {
        from = array_tree_next_node(ptr->left, key);

        if (from)
            return from;

        return ptr;
    } else if (cmpval > 0) {
        return array_tree_next_node(ptr->right, key);
    } else if (ptr->right) {
        /* We found the key -- the next key will be the leftmost node
         * of our right node.
         */
        from = ptr->right;

        while (from->left)
            from = from->left;

        return from;
    } else {
        /* If we have no right node, there's no next.  Return NULL */
        return NULL;
    }
}

/*****************************************************************
 *  Stack Array Handling Routines
 *****************************************************************/

/**
 * Create a new array structure
 *
 * This initializes the array, sets it as an unknown array type (so
 * the caller must fix that right away), and adds it to the active
 * array list if the active array list is not NULL.
 *
 * Pin is a boolean.  If true, any changes made to one copy of the
 * array will be applied to all dup'd versions of that array.  Basically,
 * turning pinning on makes the arrays share a reference, unpinning them
 * (setting it to 0) will cause the array to copy-on-change.
 *
 * @param pin boolean set pinning - see array_set_pinned for definition
 * @return newly allocated and initialized array structure
 */
static stk_array *
new_array(int pin)
{
    stk_array *nu;

    nu = malloc(sizeof(stk_array));
    assert(nu != NULL);

    if (nu == NULL) {
        fprintf(stderr, "new_array(): Out of Memory!");
        abort();
    }

    nu->links = 1;
    nu->type = ARRAY_UNDEFINED;
    nu->items = 0;
    nu->pinned = pin;
    nu->data.packed = NULL;
    nu->list_node.prev = nu->list_node.next = NULL;

    if (stk_array_active_list) {
        array_maybe_place_on_list(stk_array_active_list, nu);
    }

    return nu;
}

/**
 * Allocate a new packed (sequential numeric index) array
 *
 * @param pin Boolean - if true, array will be pinned.
 * @return a new packed array
 */
stk_array *
new_array_packed(int size, int pin)
{
    stk_array *nu;

    if (size < 0) {
        return NULL;
    }

    nu = new_array(pin);
    assert(nu != NULL); /* Redundant, but I'm coding defensively */
    nu->items = size;
    nu->type = ARRAY_PACKED;

    if (size < 1)
        size = 1;

    nu->data.packed = malloc(sizeof(array_data) * (size_t)size);

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

/**
 * Allocate a new dictionary array
 *
 * @param pin Boolean - if true, array will be pinned.
 * @return a new dictionary array
 */
stk_array *
new_array_dictionary(int pin)
{
    stk_array *nu;

    nu = new_array(pin);
    assert(nu != NULL);     /* Redundant, but I'm coding defensively */
    nu->type = ARRAY_DICTIONARY;
    return nu;
}

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
stk_array *
array_decouple(stk_array * arr)
{
    stk_array *nu;

    if (arr == NULL) {
        return NULL;
    }

    nu = new_array(arr->pinned);
    assert(nu != NULL);     /* Redundant, but I'm coding defensively */
    nu->type = arr->type;

    switch (arr->type) {
        case ARRAY_PACKED:{
            /* PACKED arrays are sequential, so the copy process is fairly
             * simple.  Allocate a new memory block then copy it.
             */
            nu->items = arr->items;
            nu->data.packed = malloc(sizeof(array_data) * (size_t)arr->items);

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

            /* When iterating over the dictionary, 'array_setitem' will
             * make a copy of the item provided.
             */
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

/**
 * Free the memory used by the given array, respecting reference count
 *
 * Arrays have a reference count ("links").  Memory will not actually be
 * free'd until the reference count has reached 0.
 *
 * @param arr the array to free
 */
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
            assert(0);      /* should never get here */
            break;
        }
    }

    arr->items = 0;
    arr->data.packed = NULL;
    arr->type = ARRAY_UNDEFINED;
    array_remove_from_list(arr);
    free(arr);
}

/**
 * Get the number of items in the array.
 *
 * If 'arr' is NULL, returns 0.
 *
 * @param arr the array to get the count for
 * @return the number of items in the array or 0 if arr is NULL
 */
int
array_count(stk_array * arr)
{
    if (arr == NULL) {
        return 0;
    }

    return arr->items;
}

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
        default:
            break;
    }

    return 0;
}

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
        default:
            break;
    }

    return 0;
}

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
        default:
            break;
    }

    return 0;
}

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
        default:
            break;
    }

    return 0;
}

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

                /* @TODO : This code looks pretty similar to array_insertitem
                 *         We should probably converge the two.
                 */
                arr->data.packed = (array_data *)
                realloc(arr->data.packed, sizeof(array_data) * (size_t)(arr->items + 1));

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
            /* @TODO: This is a copy/paste from array_insertitem.  This
             *        should definitely not be duplicated.
             */
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
            realloc(arr->data.packed, sizeof(array_data) * (size_t)(arr->items + 1));

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
stk_array *
array_getrange(stk_array * arr, array_iter * start, array_iter * end, int pin)
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

            /* Do some sanity checking */
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
            nu = new_array_packed(eidx - sidx + 1, pin);

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

            nu = new_array_dictionary(pin);
            s = array_tree_find(arr->data.dict, start);

            /* Do sanity checks */
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
int
array_setrange(stk_array ** harr, array_iter * start, stk_array * inarr)
{
    stk_array *arr;
    array_iter idx;
    int copied_inarr = 0;

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

    if (inarr == arr && arr->pinned) {
        /* avoid iterating over arr == inarr while modifying arr */
        inarr = array_decouple(inarr);
        copied_inarr = 1;
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

            /* @TODO: Doing array_setitem in a loop like this probably
             *        isn't very efficient.  Can we converge this code with
             *        array_insertset?
             */

            if (array_first(inarr, &idx)) {
                do {
                    array_setitem(&arr, start, array_getitem(inarr, &idx));
                    start->data.number++;
                } while (array_next(inarr, &idx));
            }

            if (copied_inarr) {
                array_free(inarr);
            }

            return arr->items;
        }

        case ARRAY_DICTIONARY:{
            /* @TODO: This is copy/paste code from array_insertrange ...
             *        this should be converged and not duplicated.
             */
            if (arr->links > 1 && !arr->pinned) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }

            if (array_first(inarr, &idx)) {
                do {
                    array_setitem(&arr, &idx, array_getitem(inarr, &idx));
                } while (array_next(inarr, &idx));
            }
            if (copied_inarr) {
                array_free(inarr);
            }

            return arr->items;
        }

        default:
            break;
    }

    return -1;
}

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
int
array_insertrange(stk_array ** harr, array_iter * start, stk_array * inarr)
{
    stk_array *arr;
    array_data *itm;
    array_iter idx;
    array_iter didx;
    int copied_inarr = 0;

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

    if (inarr == arr && arr->pinned) {
        /* @TODO: what if they are the same and !arr->pinned?  Can that
         *        happen?  Seem like a potential (and easy to happen) issue.
         */
        /* avoid iterating over arr == inarr while modifying arr */
        inarr = array_decouple(inarr);
        copied_inarr = 1;
    }

    /* @TODO: All these calls have a huge switch statement for dict vs. packed
     *        Is there a cleaner way to do this?  If it was C++, the solution
     *        would be easy.  In C, its a little harder.  I don't have a great
     *        idea off the top of my head -- maybe there is no other way -- but
     *        this just seems like an ugly way to do it.
     */
    switch (arr->type) {
        case ARRAY_PACKED:{
            /* Do sanity checking */
            if (!start) {
                return -1;
            }

            if (start->type != PROG_INTEGER) {
                return -1;
            }

            if (start->data.number < 0 || start->data.number > arr->items) {
                return -1;
            }

            /* Decouple the array if we need to */
            if (arr->links > 1 && !arr->pinned) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }

            arr->data.packed =
                (struct inst *)realloc(arr->data.packed,
                    sizeof(array_data) * (size_t)(arr->items + inarr->items));

            if (arr->data.packed == NULL) {
                fprintf(stderr, "array_insertrange: Out of Memory!");
                abort();
            }

            copyinst(start, &idx);
            copyinst(start, &didx);
            idx.data.number = arr->items - 1;
            didx.data.number = arr->items + inarr->items - 1;

            /* Move existing items to make space */
            while (idx.data.number >= start->data.number) {
                itm = array_getitem(arr, &idx);
                copyinst(itm, &arr->data.packed[didx.data.number]);
                CLEAR(itm);
                idx.data.number--;
                didx.data.number--;
            }

            /* Copy inarr into the space made. */
            idx.data.number = 0;
            do {
                    itm = array_getitem(inarr, &idx);
                    copyinst(itm, &arr->data.packed[start->data.number]);
                    start->data.number++;
            } while (array_next(inarr, &idx));

            arr->items += inarr->items;

            if (copied_inarr) {
                array_free(inarr);
            }

            return arr->items;
        }

        case ARRAY_DICTIONARY:{
            /* This is way easier to do on a dictionary -- its just a
             * multiple-insert.
             */
            if (arr->links > 1 && !arr->pinned) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }

            if (array_first(inarr, &idx)) {
                do {
                    array_setitem(&arr, &idx, array_getitem(inarr, &idx));
                } while (array_next(inarr, &idx));
            }

            if (copied_inarr) {
                array_free(inarr);
            }

            return arr->items;
        }

        default:
            break;
    }

    return -1;
}

/**
 * Delete a range of items from an array
 *
 * @param harr the array to operate on
 * @param start the first item to delete
 * @param end the last item in the range to delete
 * @return integer number of items still in array or -1 on error
 */
int
array_delrange(stk_array ** harr, array_iter * start, array_iter * end)
{
    stk_array *arr;
    array_data *itm;
    int sidx, eidx;
    size_t totsize;
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
            /* Packed arrays only support integer keys */
            if (start->type != PROG_INTEGER) {
                return -1;
            }

            if (end->type != PROG_INTEGER) {
                return -1;
            }

            if (arr->items == 0) {  /* nothing to do here */
                return 0;
            }

            sidx = start->data.number;
            eidx = end->data.number;

            /* Basic sanity checking */
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

            /* If the array isn't pinned and has multiple references, we
             * will need to make a copy to "decouple" it.
             */
            if (arr->links > 1 && !arr->pinned) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }

            start->data.number = sidx;
            end->data.number = eidx;
            copyinst(end, &idx);
            copyinst(start, &didx);
            idx.data.number += 1;

            /* Shift items down in the array */
            while (idx.data.number < arr->items) {
                itm = array_getitem(arr, &idx);
                copyinst(itm, &arr->data.packed[didx.data.number]);
                CLEAR(itm);
                idx.data.number++;
                didx.data.number++;
            }

            arr->items -= (eidx - sidx + 1);
            totsize = (size_t)((arr->items) ? arr->items : 1);

            /* Shrink the packed data area */
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

            /* Find the keys in the range and do some sanity checking.
             * Start must be 'before' end.
             */
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

            /* If this array has multiple references and its not
             * pinned, we need to make a copy to make changes on.
             */
            if (arr->links > 1 && !arr->pinned) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }

            copyinst(&s->key, &idx);

            /* Iterate over the structure and delete nodes as we go along */
            while (s && array_tree_compare(&s->key, &e->key, 0) <= 0) {
                arr->data.dict = array_tree_delete(&s->key, arr->data.dict);
                arr->items--;

                if (s == e) break;

                s = array_tree_next_node(arr->data.dict, &idx);
            }

            CLEAR(&idx);
            return arr->items;
        }

        default:
            break;
    }

    /* If we got to this point, then we don't know how to handle this array
     * type.
     */
    return -1;
}

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
 *
 * Editor's note: Its a shame to get rid of the penis-shaped comment
 * block that was here before. :)  I'm sure that wasn't the original
 * author's intention, but it was kind of funny.
 */
stk_array *
array_demote_only(stk_array * arr, int threshold, int pin)
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
    demoted_array = new_array_packed(0, pin);
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

/**
 * Checks to see if an array only contains the given type
 *
 * @param arr the array to check
 * @param typ the type to check for
 * @return 1 if array only contains type 'typ', 0 otherwise.
 */
int
array_is_homogenous(stk_array * arr, int typ)
{
    array_iter idx;
    array_data *dat;

    assert(arr != NULL);

    if (array_first(arr, &idx)) {
        do {
            dat = array_getitem(arr, &idx);

            if (dat->type != typ) {
                return 0;
            }
        } while (array_next(arr, &idx));
    }

    return 1;
}

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
int
array_set_intkey_intval(stk_array ** harr, int key, int val)
{
    struct inst value;
    int result;

    assert(harr != NULL);
    assert(*harr != NULL);

    value.type = PROG_INTEGER;
    value.data.number = val;

    result = array_set_intkey(harr, key, &value);

    CLEAR(&value);

    return result;
}

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
void
array_maybe_place_on_list(stk_array_list *list, stk_array *arr) {
    if (list && !arr->list_node.prev) {
        arr->list_node.next = list->next;
        arr->list_node.prev = list;
        list->next->prev = &arr->list_node;
        list->next = &arr->list_node;
    }
}

/**
 * Remove the array from any lists trackings its allocations
 *
 * This does validate if the array has already been removed, so a double
 * removal will not cause any harm.
 *
 * @param the array to remove
 */
void
array_remove_from_list(stk_array *arr) {
    if (arr->list_node.prev) {
        arr->list_node.prev->next = arr->list_node.next;
        arr->list_node.next->prev = arr->list_node.prev;
        arr->list_node.prev = arr->list_node.next = 0;
    }
}

/**
 * Initialize an active array list
 *
 * @param list the list to initialize
 */
void
array_init_active_list(stk_array_list *list) {
    list->next = list->prev = list;
}

/**
 * Free all the arrays on an array list
 *
 * This is important for MUF cleanup, among other potential uses.
 */
void
array_free_all_on_list(stk_array_list *list) {
    stk_array_list *cur_node;

    cur_node = list->next;
    while (cur_node != list) {
        stk_array *arr = STK_ARRAY_FROM_LIST_NODE(cur_node);
        /* CLEARing the contents of this array might cause it to be free'd
           if it has a circular reference. We want to prevent this from
           happening until we have used the next pointer, so we increment
           the link count here. */
        arr->links++;

        switch (arr->type) {
            case ARRAY_PACKED:
                for (int i = arr->items; i-- > 0;) {
                    CLEAR(&arr->data.packed[i]);
                }

                arr->items = 1;
                arr->data.packed[0].type = PROG_INTEGER;
                arr->data.packed[0].line = 0;
                arr->data.packed[0].data.number = 0;
                break;
            case ARRAY_DICTIONARY:
                array_tree_delete_all(arr->data.dict);
                arr->data.dict = NULL;
                break;
            default:
                assert(0 /* unknown array type */);
        }

        cur_node = cur_node->next;

        /* Call array_free() to decrement the link count and free this array
           if it has no more links. */
        array_free(arr);
    }
}
