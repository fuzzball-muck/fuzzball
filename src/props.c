/** @file props.c
 * props.c
 *
 * This, along with property.c and propdir.c, is the implementation for the
 * props.h header.  The division between props.c and property.c seems
 * ... largely arbitrary.  I would say props.c appears to (mostly) have
 * "lower level" calls compared to property.c
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "boolexp.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbmath.h"
#include "fbstrings.h"
#include "interface.h"
#include "props.h"

/**
 * Returns the AVL 'height' of a given node
 *
 * @private
 * @param node The node to check
 *
 * @return the node height or 0 if node is NULL.
 */
static int
height_of(PropPtr node)
{
    if (node)
        return node->height;
    else
        return 0;
}

/**
 * Returns the AVL 'height' difference between a node's left and
 * right node.
 *
 * @private
 * @param the node to check
 *
 * @return the difference in height between left and right.
 *         Positive numbers mean right is 'heigher', negative
 *         numbers mean left is heigher, 0 is perfect balance.
 */
static int
height_diff(PropPtr node)
{
    if (node)
        return (height_of(node->right) - height_of(node->left));
    else
        return 0;
}

/**
 * Recompute the node's height as the larger of its two children + 1
 *
 * @private
 * @param node the node to fix the height of
 */
static void
fixup_height(PropPtr node)
{
    if (node)
        node->height = 1 + MAX(height_of(node->left), height_of(node->right));
}

/**
 * This does a single "left" rotation on a given node 'a'.  The rotated
 * node is returned.
 *
 * @private
 * @param a the node to rotate.
 * @return the rotated node
 */
static PropPtr
rotate_left_single(PropPtr a)
{
    PropPtr b = a->right;

    a->right = b->left;
    b->left = a;

    fixup_height(a);
    fixup_height(b);

    return b;
}

/**
 * This does a double "left" rotation on a given node 'a'.  The rotated
 * node is returned.
 *
 * @private
 * @param a the node to rotate.
 * @return the rotated node
 */
static PropPtr
rotate_left_double(PropPtr a)
{
    PropPtr b = a->right, c = b->left;

    a->right = c->left;
    b->left = c->right;
    c->left = a;
    c->right = b;

    fixup_height(a);
    fixup_height(b);
    fixup_height(c);

    return c;
}

/**
 * This does a single "right" rotation on a given node 'a'.  The rotated
 * node is returned.
 *
 * @private
 * @param a the node to rotate.
 * @return the rotated node
 */
static PropPtr
rotate_right_single(PropPtr a)
{
    PropPtr b = a->left;

    a->left = b->right;
    b->right = a;

    fixup_height(a);
    fixup_height(b);

    return (b);
}

/**
 * This does a double "right" rotation on a given node 'a'.  The rotated
 * node is returned.
 *
 * @private
 * @param a the node to rotate.
 * @return the rotated node
 */
static PropPtr
rotate_right_double(PropPtr a)
{
    PropPtr b = a->left, c = b->right;

    a->left = c->right;
    b->right = c->left;
    c->right = a;
    c->left = b;

    fixup_height(a);
    fixup_height(b);
    fixup_height(c);

    return c;
}

/**
 * This call balances the AVL node 'a' and returns the balanced node.
 *
 * @private
 * @param a the node to balance
 * @return the balanced node
 */
static PropPtr
balance_node(PropPtr a)
{
    int dh = height_diff(a);

    if (abs(dh) < 2) {
        fixup_height(a);
    } else {
        /* One might argue the brackets are not necessary here, but
         * this looked like a train wreck without them :)
         */
        if (dh == 2) {
            if (height_diff(a->right) >= 0) {
                a = rotate_left_single(a);
            } else {
                a = rotate_left_double(a);
            }
        } else if (height_diff(a->left) <= 0) {
            a = rotate_right_single(a);
        } else {
            a = rotate_right_double(a);
        }
    }

    return a;
}

/**
 * This allocates a property node for use in the property AVL tree.  The
 * note has the given name (memory is copied over) and is set dirty, but
 * otherwise is a blank slate ready to be added to the AVL.
 *
 * Chances are, you do not want to use this method.  It is exposed because
 * it is used in boolexp.c and props.c
 *
 * @internal
 * @param name String property name (memory will be copied)
 * @return allocated PropPtr AVL node.
 */
PropPtr
alloc_propnode(const char *name)
{
    PropPtr new_node;
    size_t nlen = strlen(name);

    new_node = malloc(sizeof(struct plist) + nlen);

    if (!new_node) {
        fprintf(stderr, "alloc_propnode(): Out of Memory!\n");
        abort();
    }

    new_node->left = NULL;
    new_node->right = NULL;
    new_node->height = 1;

    strcpyn(PropName(new_node), nlen + 1, name);
    SetPFlagsRaw(new_node, PROP_DIRTYP);
    SetPDataVal(new_node, 0);
    SetPDir(new_node, NULL);
    return new_node;
}

/**
 * This is the opposite of alloc_propnode, and is used to free the
 * PropPtr datastructure.  It will free whatever data is associated
 * with the prop as well as the PropPtr as well.
 *
 * @param node the prop to free
 */
void
free_propnode(PropPtr p)
{
    if (!(PropFlags(p) & PROP_ISUNLOADED)) {
        if (PropType(p) == PROP_STRTYP)
            free(PropDataStr(p));

        if (PropType(p) == PROP_LOKTYP)
            free_boolexp(PropDataLok(p));
    }

    /* @TODO: The PropPtr object has a 'key' field which is
     *        set up like char key[1];  In alloc_propnode, we use
     *        strncpy to copy the name of the prop to the key field.
     *
     *        I should think we would need to free the key or we would
     *        have a leak.  However, I should have also thought the key
     *        should be a char* ... I don't actually understand how this
     *        works.  It seems like it should be segfaulting and/or
     *        leaking memory.  I want to review this in greater depth
     *        later.
     */
    free(p);
}

/**
 * This clears the data out of a property and sets it as unloaded.
 * This will free a string or boolexp (lock) from memory if applicable,
 * and clears out the data.  It does not clear the name out, so it is
 * not the opposite of alloc_propnode.
 *
 * @internal
 * @param p PropPtr to clear
 */
void
clear_propnode(PropPtr p)
{
    if (!(PropFlags(p) & PROP_ISUNLOADED)) {
        if (PropType(p) == PROP_STRTYP) {
            free(PropDataStr(p));
            PropDataStr(p) = NULL;
        }

        if (PropType(p) == PROP_LOKTYP)
            free_boolexp(PropDataLok(p));
    }

    SetPDataVal(p, 0);
    SetPFlags(p, (PropFlags(p) & ~PROP_ISUNLOADED));
    SetPType(p, PROP_DIRTYP);
}

/**
 * Get the 'maximum' AVL node, with the search starting at 'avl'.  This
 * basically finds the right-mode node.
 *
 * @private
 * @param avl the node to search
 * @return the rightmode ("max") node.
 */
static PropPtr
getmax(PropPtr avl)
{
    if (avl && avl->right)
        return getmax(avl->right);

    return avl;
}

/**
 * Remove a propnode named 'key' from the AVL root 'root'.  This function
 * is recursive.
 *
 * The node (removed from the AVL structure) is returned, or NULL if it
 * was not found.
 *
 * @private
 * @param key the prop name to remove
 * @param root the root node to start the removal process from
 *
 * @return the removed node - you should probably free it with free_propnode
 */
static PropPtr
remove_propnode(char *key, PropPtr * root)
{
    PropPtr save;
    PropPtr tmp;
    PropPtr avl = *root;
    int cmpval;

    save = avl;

    if (avl) {
        cmpval = strcasecmp(key, PropName(avl));

        if (cmpval < 0) {
            save = remove_propnode(key, &(avl->left));
        } else if (cmpval > 0) {
            save = remove_propnode(key, &(avl->right));
        } else if (!(avl->left)) {
            avl = avl->right;
        } else if (!(avl->right)) {
            avl = avl->left;
        } else {
            tmp = remove_propnode(PropName(getmax(avl->left)), &(avl->left));

            if (!tmp) { /* this shouldn't be possible. */
                panic("remove_propnode() returned NULL !");
            }

            tmp->left = avl->left;
            tmp->right = avl->right;
            avl = tmp;
        }

        if (save) {
            save->left = NULL;
            save->right = NULL;
        }

        *root = balance_node(avl);
    }

    return save;
}

/**
 * This is a method to delete a particular key from the AVL list.  It
 * frees the memory of the deleted node.
 *
 * @private
 * @param key the key to delete
 * @param avl the AVL prop structure to delete the key from.
 * @return the updated prop structure ... which is probably not
 *         a necessary return since the 'avl' itself is mutated by
 *         this process.
 */
static PropPtr
delnode(char *key, PropPtr avl)
{
    PropPtr save;

    save = remove_propnode(key, &avl);

    if (save)
        free_propnode(save);
    return avl;
}

/**
 * Recursively deletes an entire proplist AVL with 'p' as the root,
 * and frees 'p' itself.
 *
 * @param p The proplist to delete.
 */
void
delete_proplist(PropPtr p)
{
    if (!p)
        return;

    delete_proplist(p->left);
    delete_proplist(PropDir(p));
    delete_proplist(p->right);
    free_propnode(p);
}


/**
 * This finds a prop named 'key' in the AVL proplist 'avl'.  It is basically
 * a primitive for looking up items in the AVLs.
 *
 * @param avl the AVL to search
 * @param key the key to look up
 *
 * @return the found node, or NULL if not found.
 */
PropPtr
locate_prop(PropPtr avl, char *key)
{
    int cmpval;

    while (avl) {
        cmpval = strcasecmp(key, PropName(avl));

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
 * This creates a new node in the AVL then returns the created node
 * so that you might populate it with data.  If the key already
 * exists, then the existing node is returned.
 *
 * @param avl the AVL to add a property to.
 * @param key the key to add to the AVL.
 *
 * @return the newly created AVL node.
 */
PropPtr
new_prop(PropPtr *avl, char *key)
{
    PropPtr ret;
    register PropPtr p = *avl;
    register int cmp;
    static short balancep;

    if (p) {
        cmp = strcasecmp(key, PropName(p));

        if (cmp > 0) {
            ret = new_prop(&(p->right), key);
        } else if (cmp < 0) {
            ret = new_prop(&(p->left), key);
        } else {
            balancep = 0;
            return (p);
        }

        if (balancep) {
            *avl = balance_node(p);
        }

        return ret;
    } else {
        p = *avl = alloc_propnode(key);
        balancep = 1;
        return (p);
    }
}

/**
 * Delete a property from the given prop set, with the given property
 * name.  It removes it from the AVL of 'list'.  Does not save it to
 * the database right away.
 *
 * @param list Pointer to a PropPtr which is (usually) the root of the
 *             property AVL tree.
 * @param name The name of the property to delete
 * @return Returns the pointer that 'list' is pointing to.  Because 'list'
 *         is modified, there is probably no reason to use the return
 *         value.
 */
PropPtr
delete_prop(PropPtr * list, char *name)
{
    *list = delnode(name, *list);
    return (*list);
}

/**
 * Finds the first node ("leftmost") on the property AVL list 'p' or
 * returns NULL if p has no nodes on it.
 *
 * @param p PropPtr of the root of the AVL property list you want to scan.
 *
 * @return First/leftmost node on proplist.
 */
PropPtr
first_node(PropPtr list)
{
    if (!list)
        return ((PropPtr) NULL);

    while (list->left)
        list = list->left;

    return (list);
}

/**
 * next_node locates and returns the next node in the AVL (prop directory)
 * or NULL if there is no more.  It is used for traversing a prop directory.
 *
 * @param ptr the AVL to navigate
 * @param name The "previous path" ... what is returned is the next path
 *        after this one
 * @return the property we found, or NULL
 */
PropPtr
next_node(PropPtr ptr, char *name)
{
    PropPtr from;
    int cmpval;

    if (!ptr)
        return NULL;

    if (!name || !*name)
        return (PropPtr) NULL;

    cmpval = strcasecmp(name, PropName(ptr));

    if (cmpval < 0) {
        from = next_node(ptr->left, name);

        if (from)
            return from;

        return ptr;
    } else if (cmpval > 0) {
        return next_node(ptr->right, name);
    } else if (ptr->right) {
        from = ptr->right;

        while (from->left)
            from = from->left;

        return from;
    } else {
        return NULL;
    }
}

/**
 * This is the underpinning for both copy_prop and copy_properties_onto
 *
 * It recursively copies properties from obj (the "old" PropPtr should
 * be the root properties from "obj") into a structure "newer".
 *
 * newer may be either NULL or an existing prop tree.  The 'obj' dbref is
 * needed for diskbase reasons, however it looks like all consumers of
 * this call do the diskbase load so that could probably be refactored out
 * pretty easily.
 *
 * As this is an internal call, it doesn't matter too much; you should use
 * either copy_prop or copy_properties_onto.  Were we to move this call to
 * property.c, we could avoid having it in this header.
 *
 * @TODO Consider refactoring this so it lives in property.c and remove
 *       the need for 'obj'
 * @internal
 * @param obj DBREF object that 'old' props belong to.
 * @param newer Essentially a pointer to a pointer; the target structure
 * @param old The source property list
 */
void
copy_proplist(dbref obj, PropPtr * nu, PropPtr old)
{
    PropPtr p;

    if (old) {
#ifdef DISKBASE
        propfetch(obj, old);
#endif
        p = new_prop(nu, PropName(old));
        clear_propnode(p);
        SetPFlagsRaw(p, PropFlagsRaw(old));

        switch (PropType(old)) {
            case PROP_STRTYP:
                SetPDataStr(p, alloc_string(PropDataStr(old)));
                break;
            case PROP_LOKTYP:
                if (PropFlags(old) & PROP_ISUNLOADED) {
                    SetPDataLok(p, TRUE_BOOLEXP);
                    SetPFlags(p, (PropFlags(p) & ~PROP_ISUNLOADED));
                } else {
                    SetPDataLok(p, copy_bool(PropDataLok(old)));
                }
                break;
            case PROP_DIRTYP:
                SetPDataVal(p, 0);
                break;
            case PROP_FLTTYP:
                SetPDataFVal(p, PropDataFVal(old));
                break;
            default:
                SetPDataVal(p, PropDataVal(old));
                break;
        }

        copy_proplist(obj, &PropDir(p), PropDir(old));
        copy_proplist(obj, &(p->left), old->left);
        copy_proplist(obj, &(p->right), old->right);
    }
}

/**
 * Calculates the size of the given property directory AVL list.  This
 * will iterate over the entire structure to give the entire size.  It
 * is the low level equivalent of size_properties
 *
 * @see size_properties
 *
 * @param avl the Property directory AVL to check
 * @return the size of the loaded properties in memory -- this does NOT
 *         do any diskbase loading.
 */
size_t
size_proplist(PropPtr avl)
{
    size_t bytes = 0;

    if (!avl)
        return 0;

    bytes += sizeof(struct plist);

    bytes += strlen(PropName(avl));

    if (!(PropFlags(avl) & PROP_ISUNLOADED)) {
        switch (PropType(avl)) {
            case PROP_STRTYP:
                bytes += strlen(PropDataStr(avl)) + 1;
                break;
            case PROP_LOKTYP:
                bytes += size_boolexp(PropDataLok(avl));
                break;
            default:
                break;
        }
    }

    bytes += size_proplist(avl->left);
    bytes += size_proplist(avl->right);
    bytes += size_proplist(PropDir(avl));
    return bytes;
}

/**
 * Check each segment of the property path and see if 'what' is the first
 * character of the segment.
 *
 * It is recommended you use the property check macro's instead:
 * Prop_ReadOnly, Prop_Private, Prop_SeeOnly, Prop_Hidden, Prop_System
 *
 * @param name Property name string
 * @param what Single character
 * @return true if 'what' appears in any path segment.
 */
int
Prop_Check(const char *name, const char what)
{
    if (*name == what)
        return (1);

    while ((name = strchr(name, PROPDIR_DELIMITER))) {
        if (name[1] == what)
            return (1);
        name++;
    }

    return (0);
}
