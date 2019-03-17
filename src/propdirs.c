/*
 * propdirs.c
 *
 * Implementation of various propdir "low level" calls declared in props.h
 *
 * This source file is part of the Fuzzball MUCK Project and is released
 * under the GNU Public License version 3 with certain exceptions.  Please
 * see LICENSE.md for full details.
 */

#include <string.h>

#include "config.h"

#include "db.h"
#include "props.h"

/**
 * This creates a new property node at the given path on the given root
 * propdir object.  If there is already a property at that path location,
 * the existing property is returned; otherwise a new node is returned.
 *
 * According to the original documentation, if the property name is "bad"
 * this can return NULL.  However, practically speaking, this isn't actually
 * implemented.  Note that creating a property with a ':' in the name is
 * a particularly bad idea but is currently not checked for.  This *will*
 * return NULL if you put an empty string or just /'s.
 *
 * This is a "low level" call and probably not what you want; you
 * will probably want to use add_property or set_property.  set_property in
 * particular offers the same flexibility as this call but operates with
 * dbrefs and lets you set a value in one call.
 *
 * @see add_property
 * @see set_property
 *
 * @param root The root of the property directory structure.
 * @param path The path to create.
 *
 * @return the newly created node, or the existing node at the given path,
 *         or NULL on error
 */
PropPtr
propdir_new_elem(PropPtr * root, char *path)
{
    PropPtr p;
    char *n;

    /* @TODO: The original comments on this function say that this
     *        returns NULL "if the name given is bad".  The only check
     *        currently done is for empty or just / named paths.  A better
     *        check would be for : as well (PROP_DELIMITER I think it is)
     */

    while (*path && *path == PROPDIR_DELIMITER)
        path++;

    if (!*path)
        return (NULL);

    n = strchr(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER)
        *(n++) = '\0';

    if (n && *n) {
        /* just another propdir in the path */
        p = new_prop(root, path);
        return (propdir_new_elem(&PropDir(p), n));
    } else {
        /* aha, we are finally to the property itself. */
        p = new_prop(root, path);
        return (p);
    }
}

/**
 * This deletes an element (path) from a propdir.  Note that you cannot
 * delete the root property (path "/") with this call; that actually does
 * nothing.  If you delete a propdir, it will delete all child properties
 * within the propdir.
 *
 * This is something of a low-level call; you may rather use delete_prop
 *
 * @see delete_prop
 *
 * @param root The root property node to start your search
 * @param path the path you are searching for to delete.
 *
 * @return the updated root node with the property removed.  Because this
 *         mutates the passed structure, this is equivalent to the 'root'
 *         parameter.
 */
PropPtr
propdir_delete_elem(PropPtr root, char *path)
{
    PropPtr p;
    char *n;

    if (!root)
        return (NULL);

    /* Trim off leading / */
    while (*path && *path == PROPDIR_DELIMITER)
        path++;

    if (!*path)
        return (root);

    /* Find our / */
    n = strchr(path, PROPDIR_DELIMITER);

    /* Erase extra /'s */
    while (n && *n == PROPDIR_DELIMITER)
        *(n++) = '\0';

    if (n && *n) {
        /* just another propdir in the path */
        p = locate_prop(root, path);

        /* If the property we want to delete is in a "subpropdir" then we
         * need to run propdir_delete_element on the "subdir"
         */
        if (p && PropDir(p)) {
            /* yup, found the propdir */
            SetPDir(p, propdir_delete_elem(PropDir(p), n));

            if (!PropDir(p) && PropType(p) == PROP_DIRTYP) {
                root = delete_prop(&root, PropName(p));
            }
        }

        /* return the updated root pntr */
        return (root);
    } else {
        /* aha, we are finally to the property itself. */
        p = locate_prop(root, path);

        if (p && PropDir(p)) {
            delete_proplist(PropDir(p));
        }

        (void) delete_prop(&root, path);

        return (root);
    }
}

/**
 * Fetches a given property from the property path structure 'root'.
 * As this is something of a low level call you may prefer to use 
 * get_property
 *
 * @see get_property
 * @see get_property_class
 * @see get_property_dbref
 * @see get_property_fvalue
 * @see get_property_lock
 * @see get_property_value
 *
 * @param root The root of the property directory tree
 * @param path the property you wish to return
 *
 * @return the found property or NULL if not found.
 */
PropPtr
propdir_get_elem(PropPtr root, char *path)
{
    PropPtr p;
    char *n;

    if (!root)
        return (NULL);

    /* Standard issue / cleanup
     *
     * @TODO: make this a common call or maybe just a macro so its not
     * quite so all over the place.  This is very copy-pasta.
     */
    while (*path && *path == PROPDIR_DELIMITER)
        path++;

    if (!*path)
        return (NULL);

    n = strchr(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER)
        *(n++) = '\0';

    if (n && *n) {
        /* just another propdir in the path */
        p = locate_prop(root, path);

        if (p && PropDir(p)) {
            /* yup, found the propdir */
            return (propdir_get_elem(PropDir(p), n));
        }

        return (NULL);
    } else {
        /* aha, we are finally to the property subname itself. */
        if ((p = locate_prop(root, path))) {
            /* found the property! */
            return (p);
        }

        return (NULL);        /* nope, doesn't exist */
    }
}

/**
 * This gets the first element of a propdir given a certain path.
 * As this is something of a low level call, you may prefer to use
 * first_prop instead.
 *
 * @see first_prop
 *
 * @param root The root of the property directory tree.
 * @param path The path you want to get the first element of; "" or "/" both
 *        count as root.
 *
 * @return the first element of the given path or NULL if not found.
 */
PropPtr
propdir_first_elem(PropPtr root, char *path)
{
    PropPtr p;

    /* Clear leading / */
    while (*path && *path == PROPDIR_DELIMITER)
        path++;

    if (!*path)
        return (first_node(root));

    p = propdir_get_elem(root, path);

    if (p && PropDir(p)) {
        return (first_node(PropDir(p)));    /* found the property! */
    }

    return (NULL);        /* nope, doesn't exist */
}

/**
 * Returns pointer to the next property after the given one in the given
 * given structure (root).  Please note that this is a "low level call"
 * and you may prefer to use next_prop_name, next_prop, or next_node.
 *
 * @see next_prop_name
 * @see next_prop
 * @see next_node
 *
 * @param root The root of the property directory structure
 * @param path The 'previous' property path.
 *
 * @return the next property in the propdir or NULL if no more.
 */
PropPtr
propdir_next_elem(PropPtr root, char *path)
{
    PropPtr p;
    char *n;

    if (!root)
        return (NULL);

    while (*path && *path == PROPDIR_DELIMITER)
        path++;

    if (!*path)
        return (NULL);

    n = strchr(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER)
        *(n++) = '\0';

    if (n && *n) {
        /* just another propdir in the path */
        p = locate_prop(root, path);

        if (p && PropDir(p)) {
            /* yup, found the propdir */
            return (propdir_next_elem(PropDir(p), n));
        }

        return (NULL);
    } else {
        /* aha, we are finally to the property subname itself. */
        return (next_node(root, path));
    }
}

/**
 * This is basically the equivalent of the POSIX "dirname", which retrieves
 * the property directory path portion of a given propname.  Its primary
 * purpose is for the diskbase; in order to load a propdir's properties,
 * you must first fetch the propdir itself from the diskbase.
 *
 * This call uses a static buffer so it is not threadsafe (though none of
 * the MUCK is) but also will mutate over subsequent calls.  In the
 * core usecase of this method, this does not matter, but if you are
 * doing something more creative you may wish to copy the directory out
 * of its buffer.  Do not try to free this buffer.
 *
 * @param name The property name to get a propdir name for.
 * @return the property's propdir
 */
const char *
propdir_name(const char *name)
{
    static char pnbuf[BUFFER_LEN];
    const char *i;
    char *o;

    i = name;
    o = pnbuf;
    while (*i) {
        while (*i == PROPDIR_DELIMITER)
            i++;

        *o++ = PROPDIR_DELIMITER;
        while (*i && *i != PROPDIR_DELIMITER)
            *o++ = *i++;
    }

    *o = '\0';
    while (*o != PROPDIR_DELIMITER)
        *o-- = '\0';

    return pnbuf;
}

/**
 * Returns the path of the first unloaded propdir in a given path,
 * or NULL if all the propdirs to the path are loaded.
 *
 * @param root The root propdir node
 * @param path The path to operate on.
 *
 * @return path name as described above, or NULL.
 */
const char *
propdir_unloaded(PropPtr root, const char *path)
{
    PropPtr p;
    const char *n;
    char *o, *l;
    static char buf[BUFFER_LEN];

    if (!root)
        return NULL;

    n = path;
    o = buf;
    while (*n == PROPDIR_DELIMITER)
        n++;

    if (!*n)
        return NULL;

    while (*n) {
        *o++ = PROPDIR_DELIMITER;
        l = o;

        while (*n && *n != PROPDIR_DELIMITER)
            *o++ = *n++;

        while (*n == PROPDIR_DELIMITER)
            n++;

        *o = '\0';
        p = locate_prop(root, l);

        if (!p) {
            return NULL;
        }

        if (PropFlags(p) & PROP_DIRUNLOADED) {
            SetPFlags(p, (PropFlags(p) & ~PROP_DIRUNLOADED));
            return buf + 1;
        }

        root = PropDir(p);
    }

    return NULL;
}
