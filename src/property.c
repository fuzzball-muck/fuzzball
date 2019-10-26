/** @file property.c
 * property.c
 *
 * This, along with props.c and propdir.c, is the implementation for the
 * props.h header.  The division between props.c and property.c seems
 * ... largely arbitrary.  I would say props.c appears to (mostly) have
 * "lower level" calls compared to property.c
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "boolexp.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbmath.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "mpi.h"
#include "props.h"
#include "tune.h"


/**
 * Set a property on an object (the 'player'), with name pname and
 * with property data 'dat'.  'sync' has to do with syncing gender
 * props -- more on that in a moment.
 *
 * This method (unlike some others) does full property name validation.
 * Leading / are trimmed off, and if there is a ':' the prop name will
 * be terminated at that point (the : becomes a \0).  The : is defined
 * as PROP_DELIMITER.  
 * 
 * If there is no property name after cleanup, this just returns with nothing
 * done.
 *
 * As for 'dat' ... its flags field is used to initialize the flags on the
 * newly made property.  If the flags field in 'dat' has PROP_ISUNLOADED, then
 * it is set on the prop as-is.  In this way, a diskbase'd property can
 * be set to its diskbase'd value.
 *
 * Otherwise, type is derived from the flags field in dat.  If it is a
 * string, a *copy* of the string is made for the new prop so you should
 * clean up your own string as necessary.
 *
 * Float, int, and dbref are all trivial and just are copied over.  If
 * you create a PROP_DIRTYP (propdir) it must already have props in it.
 * For a LOCK -- and this is very important -- the boolexp lock structure
 * is NOT COPIED and the property 'owns' the lock memory from this point
 * forward.  Do not free it!
 *
 * If you set the property to what is considered an empty value
 * ("" for string, 0 for int, 0.0 for float, or a PROP_DIRTYP with no
 * properties in it), it will be deleted instead.
 *
 * And if this comment wasn't long enough -- we've also got "sync"!
 * "sync" is for some funky logic around the gender property.  If the MUCK's
 * @tune'd gender property is not the same as LEGACY_GENDER_PROP
 * (usually "sex"), and sync is 0, then LEGACY_GENDER_PROP and the tune'd
 * gender property receive the same fate.
 *
 * Thus, if sync is 0, and you set the tune'd gender prop, it will
 * also set LEGACY_GENDER_PROP to the same or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  The original purpose
 * around sync is to avoid a recursion problem.
 *
 * This version of set_property DOES NOT do diskbase -- you probably want
 * to use set_property instead.
 *
 * @see set_property
 *
 * @internal
 * @param player The ref of the object to set the property on.
 * @param pname The property name to set.
 * @param dat a PData structure, loaded with the right flags and prop data.
 * @param sync the weird gender sync option.
 */
void
set_property_nofetch(dbref player, const char *pname, PData * dat, int sync)
{
    PropPtr p;
    char buf[BUFFER_LEN];
    char *n, *w;

    /* Make sure that we are passed a valid property name */
    if (!pname)
        return;

    /* Clear out the leading /'s */
    while (*pname == PROPDIR_DELIMITER)
        pname++;

    /* If it is a listen prop, mark the player as a listener.
     *
     * I don't know the exact purpose of this, 'LISTENER' is not
     * well documented from what I can tell.  Even after grep-ing around
     * I'm not sure what this is for.
     *
     * @TODO: Document the purpose of this.
     */
    if ((!(FLAGS(player) & LISTENER)) &&
        (string_prefix(pname, LISTEN_PROPQUEUE) ||
         string_prefix(pname, WLISTEN_PROPQUEUE) ||
         string_prefix(pname, WOLISTEN_PROPQUEUE))) {
        FLAGS(player) |= LISTENER;
    }

    w = strcpyn(buf, sizeof(buf), pname);

    /* truncate propnames with a ':' in them at the ':' */
    n = strchr(buf, PROP_DELIMITER);
    if (n)
        *n = '\0';

    /* Take no action if there is no property name after it has been
     * cleaned up.
     */
    if (!*buf)
        return;

    /* Create a new element for our new property, or get an existing
     * property object if it already exists.
     */
    p = propdir_new_elem(&(DBFETCH(player)->properties), w);

    /* free up any old values, just in case this is an existing property */
    clear_propnode(p);

    SetPFlagsRaw(p, dat->flags);

    if (PropFlags(p) & PROP_ISUNLOADED) {
        /* If the prop is unloaded, we want to just copy the data over
         * raw -- this ensures the position in the diskbase is preserved and
         * no further action is required.
         */
        SetPDataUnion(p, dat->data);
        return;
    }

    /* Since we're not dealing with an unloaded prop, we need to set
     * the property data in the data union based on type.
     */
    switch (PropType(p)) {
        case PROP_STRTYP:
            /* If you try to set an empty string property value, we will
             * instead delete the prop unless it is a propdir.
             */
            if (!dat->data.str || !*(dat->data.str)) {
                SetPType(p, PROP_DIRTYP);
                SetPDataStr(p, NULL);

                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname, 0);
                }
            } else {
                SetPDataStr(p, alloc_string(dat->data.str));
            }

            break;
        case PROP_INTTYP:
            SetPDataVal(p, dat->data.val);

            if (!dat->data.val) {
                SetPType(p, PROP_DIRTYP);

                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname, 0);
                }
            }

            break;
        case PROP_FLTTYP:
            SetPDataFVal(p, dat->data.fval);

            if (dat->data.fval == 0.0) {
                SetPType(p, PROP_DIRTYP);

                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname, 0);
                }
            }

            break;
        case PROP_REFTYP:
            SetPDataRef(p, dat->data.ref);

            if (dat->data.ref == NOTHING) {
                SetPType(p, PROP_DIRTYP);
                SetPDataRef(p, 0);

                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname, 0);
                }
            }

            break;
        case PROP_LOKTYP:
            SetPDataLok(p, dat->data.lok);
            break;
        case PROP_DIRTYP:
            SetPDataVal(p, 0);

            if (!PropDir(p)) {
                remove_property_nofetch(player, pname, 0);
            }

            break;
    }

    if (Typeof(player) == TYPE_PLAYER) {
        if (!strcasecmp(pname, LEGACY_GUEST_PROP)) {
            /* Removing the legacy guest prop also removes the guest
             * flag.
             */
            FLAGS(player) |= GUEST;
        } else if (!sync && strcasecmp(tp_gender_prop, LEGACY_GENDER_PROP)) {
            /* This code block sync's the tune'd gender property
             * with the legacy gender property if indicated to do so
             * by the parameters.
             */

            const char *current;
            const char *legacy;
            current = tp_gender_prop;
            legacy = LEGACY_GENDER_PROP;

            while (*current == PROPDIR_DELIMITER)
                current++;

            while (*legacy == PROPDIR_DELIMITER)
                legacy++;

            if (!strcasecmp(pname, current)) {
                set_property(player, (char *) legacy, dat, 1);
            } else if (!strcasecmp(pname, legacy)) {
                set_property(player, current, dat, 1);
            }
        }
    }
}

/**
 * Set a property on an object (the 'player'), with name pname and
 * with property data 'dat'.  'sync' has to do with syncing gender
 * props -- more on that in a moment.
 *
 * This method (unlike some others) does full property name validation.
 * Leading / are trimmed off, and if there is a ':' the prop name will
 * be terminated at that point (the : becomes a \0).  The : is defined
 * as PROP_DELIMITER.  
 * 
 * If there is no property name after cleanup, this just returns with nothing
 * done.
 *
 * As for 'dat' ... its flags field is used to initialize the flags on the
 * newly made property.  If the flags field in 'dat' has PROP_ISUNLOADED, then
 * it is set on the prop as-is.  In this way, a diskbase'd property can
 * be set to its diskbase'd value.
 *
 * Otherwise, type is derived from the flags field in dat.  If it is a
 * string, a *copy* of the string is made for the new prop so you should
 * clean up your own string as necessary.
 *
 * Float, int, and dbref are all trivial and just are copied over.  If
 * you create a PROP_DIRTYP (propdir) it must already have props in it.
 * For a LOCK -- and this is very important -- the boolexp lock structure
 * is NOT COPIED and the property 'owns' the lock memory from this point
 * forward.  Do not free it!
 *
 * If you set the property to what is considered an empty value
 * ("" for string, 0 for int, 0.0 for float, or a PROP_DIRTYP with no
 * properties in it), it will be deleted instead.
 *
 * And if this comment wasn't long enough -- we've also got "sync"!
 * "sync" is for some funky logic around the gender property.  If the MUCK's
 * @tune'd gender property is not the same as LEGACY_GENDER_PROP
 * (usually "sex"), and sync is 0, then LEGACY_GENDER_PROP and the tune'd
 * gender property receive the same fate.
 *
 * Thus, if sync is 0, and you set the tune'd gender prop, it will
 * also set LEGACY_GENDER_PROP to the same or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  The original purpose
 * around sync is to avoid a recursion problem.
 *
 * This version of set_property handles all the diskbase stuff.
 *
 * @param player The ref of the object to set the property on.
 * @param pname The property name to set.
 * @param dat a PData structure, loaded with the right flags and prop data.
 * @param sync the weird gender sync option.
 */
void
set_property(dbref player, const char *name, PData * dat, int sync)
{
#ifdef DISKBASE
    fetchprops(player, propdir_name(name));
#endif

    set_property_nofetch(player, name, dat, sync);

#ifdef DISKBASE
    dirtyprops(player);
#endif

    DBDIRTY(player);
}

/**
 * Adds a new property to an object without diskbase handling.  It is
 * safer to use the add_property method, however this is exposed because
 * it used in both db.c and property.c
 *
 * This is a wrapper around set_property_nofetch, so if you need to
 * set values more complex than string or integer, use set_property_nofetch
 * instead.
 *
 * @param player Object to set property on
 * @param pname Property name
 * @param strval String value - pass NULL if you want an integer value
 * @param value Integer value - ignored if String value is used.
 */
void
add_prop_nofetch(dbref player, const char *pname, const char *strval, int value)
{
    PData mydat;

    if (strval && *strval) {
        mydat.flags = PROP_STRTYP;
        mydat.data.str = (char *) strval;
    } else if (value) {
        mydat.flags = PROP_INTTYP;
        mydat.data.val = value;
    } else {
        mydat.flags = PROP_DIRTYP;
        mydat.data.str = NULL;
    }

    set_property_nofetch(player, pname, &mydat, 0);
}

/**
 * Adds a new property to an object WITH diskbase handling.  You should
 * use this one if you want to add a property.
 *
 * This is a wrapper around set_property, so if you need to
 * set values more complex than string or integer, use set_property_nofetch
 * instead.
 *
 * @param player Object to set property on
 * @param pname Property name
 * @param strval String value - pass NULL if you want an integer value
 * @param value Integer value - ignored if String value is used.
 */
void
add_property(dbref player, const char *pname, const char *strval, int value)
{

#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
    add_prop_nofetch(player, pname, strval, value);
    dirtyprops(player);
#else
    add_prop_nofetch(player, pname, strval, value);
#endif
    DBDIRTY(player);
}

/**
 * This static method supports remove_property_list and is called in
 * a loop to help clear all the properties on an object.
 *
 * If 'allp' is 0, "wizard" properties (@ and ~ properties, or more
 * specifically, PROP_HIDDEN and PROP_SEEONLY properties) and the "_/"
 * propdir are left alone.
 *
 * If 'allp' is 1, then everything is blown away regardless.
 *
 * @private
 * @param player The object to clear properties on.
 * @param p The propdir / property structure we are currently working
 *        on.
 * @param all Clear all properties? boolean -- see description above.
 */
static void
remove_proplist_item(dbref player, PropPtr p, int allp)
{
    const char *ptr;

    if (!p)
        return;

    ptr = PropName(p);

    if (Prop_System(ptr))
        return;

    if (!allp) {
        if (*ptr == PROP_HIDDEN)
            return;

        if (*ptr == PROP_SEEONLY)
            return;

        if (ptr[0] == '_' && ptr[1] == '\0')
            return;
    }

    remove_property(player, ptr, 0);
}

/**
 * This call is to remove ALL properties on an object; it is used
 * for @set whatever=:clear exclusively at the time of this writing.
 *
 * If 'all' is 0, "wizard" properties (@ and ~ properties, or more
 * specifically, PROP_HIDDEN and PROP_SEEONLY properties) and the "_/"
 * propdir are left alone.
 *
 * If 'all' is 1, then everything is blown away regardless.
 *
 * @param player The object to clear properties on.
 * @param all Clear all properties? boolean -- see description above.
 */
void
remove_property_list(dbref player, int all)
{
    PropPtr l;
    PropPtr p;
    PropPtr n;

#ifdef DISKBASE
    fetchprops(player, NULL);
#endif

    if ((l = DBFETCH(player)->properties) != NULL) {
        p = first_node(l);

        while (p) {
            n = next_node(l, PropName(p));
            remove_proplist_item(player, p, all);
            l = DBFETCH(player)->properties;
            p = n;
        }
    }

#ifdef DISKBASE
    dirtyprops(player);
#endif

    DBDIRTY(player);
}

/**
 * This removes a property from a given object.  "sync" is for some funky
 * logic around the gender property.  If the MUCK's @tune'd gender property
 * is not the same as LEGACY_GENDER_PROP (usually "sex"), and sync is 0,
 * then LEGACY_GENDER_PROP and the tune'd gender property receive the
 * same fate.
 *
 * Thus, if sync is 0, and you delete the tune'd gender prop, it will
 * also delete LEGACY_GENDER_PROP or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  This call DOES NOT
 * handle the diskbase stuff and therefore you probably want remove_property
 * instead.
 *
 * @see remove_property
 *
 * @internal
 * @param player The object to operate on.
 * @param pname the property name to delete
 * @param sync Do not sync gender props?  See description above.
 */
void
remove_property_nofetch(dbref player, const char *pname, int sync)
{
    PropPtr l;
    char buf[BUFFER_LEN];
    char *w;

    w = strcpyn(buf, sizeof(buf), pname);

    l = DBFETCH(player)->properties;
    l = propdir_delete_elem(l, w);
    DBFETCH(player)->properties = l;

    if (Typeof(player) == TYPE_PLAYER) {
        if (!strcasecmp(buf, LEGACY_GUEST_PROP)) {
            /* Removing the legacy guest prop also removes the guest
             * flag.
             */
            FLAGS(player) &= ~GUEST;
            DBDIRTY(player);
        } else if (!sync && strcasecmp(tp_gender_prop, LEGACY_GENDER_PROP)) {
            /* This code block sync's the tune'd gender property
             * with the legacy gender property if indicated to do so
             * by the parameters.
             */

            const char *current;
            const char *legacy;
            current = tp_gender_prop;
            legacy = LEGACY_GENDER_PROP;

            while (*current == PROPDIR_DELIMITER)
                current++;

            while (*legacy == PROPDIR_DELIMITER)
                legacy++;

            if (!strcasecmp(pname, current)) {
                remove_property(player, (char *) legacy, 1);
            } else if (!strcasecmp(pname, legacy)) {
                remove_property(player, current, 1);
            }
        }
    }
}

/**
 * This removes a property from a given object.  "sync" is for some funky
 * logic around the gender property.  If the MUCK's @tune'd gender property
 * is not the same as LEGACY_GENDER_PROP (usually "sex"), and sync is 0,
 * then LEGACY_GENDER_PROP and the tune'd gender property receive the
 * same fate.
 *
 * Thus, if sync is 0, and you delete the tune'd gender prop, it will
 * also delete LEGACY_GENDER_PROP or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  This call handles
 * all the diskbase stuff.
 *
 * @param player The object to operate on.
 * @param pname the property name to delete
 * @param sync Do not sync gender props?  See description above.
 */
void
remove_property(dbref player, const char *pname, int sync)
{
#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
#endif

    remove_property_nofetch(player, pname, sync);

#ifdef DISKBASE
    dirtyprops(player);
#endif

    DBDIRTY(player);
}

/**
 * Get a property and return its PropPtr, for a given object.
 *
 * This version handles diskbase and is the one you should normally use.
 * Returns NULL if the property was not found.
 *
 * @param player The object to search for the property on.
 * @param pname The property name
 *
 * @return a PropPtr object with the property, or NULL if not found.
 */
PropPtr
get_property(dbref player, const char *pname)
{
    PropPtr p;
    char buf[BUFFER_LEN];
    char *w;

#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
#endif

    w = strcpyn(buf, sizeof(buf), pname);

    p = propdir_get_elem(DBFETCH(player)->properties, w);
    return (p);
}

/**
 * has_property scans an object and all its contents to see if it has
 * a given property name, and checks to see if it matches the values
 * provided.
 *
 * If the found property type is string, it matches against the 'strval'
 * and will (potentially) MPI parse the property to see if it resolves
 * into 'strval' (more below).
 *
 * If the found property is an integer, it is compared against 'value'.
 * If the found property is a floating point, it is also compared against
 * integer after the float has been cast to an integer.  'Ref' and 'lock'
 * type props will always return not found under this call.
 *
 * The fact that the property type rather than the type of parameter
 * passed into this call determines the type of comparison is very odd;
 * it could have unexpected behavior if, for instance, you want a string
 * comparison but the property is set as an integer.  It could accidentally
 * match the integer instead of the string, which may be a way to bypass
 * locks (as this call seems to be used for locking primarily).
 *
 * @TODO: Code review - would a call such as :
 *        has_property(x, db, db2, "whatever/prop", "PassString", 0)
 *        successfully pass the lock check if the property is an
 *        integer propval '0' instead of 'PassString' which would be
 *        the expected value?  Potential security hole.
 *
 * MPI in string properties is parsed based on a has_prop_recursion_limit
 * static variable.  It looks like a measure to prevent the MPI from
 * causing a loop where has_property calls has_property on the same property
 * downstream.  If the recursion limit is reached, the string property is
 * just treated as a regular string without MPI parsing.
 *
 * If the tune parameter 'lock_envcheck' is set to yes, then this call
 * will also check the environment for the property and value.
 *
 * @see has_property_strict which is used under the hood.
 *
 * @param descr Integer descriptor of the caller.
 * @param player the DBREF of the player that has triggered the has_property
 *        check
 * @param what the object that triggered the check
 * @param pname The property name to check
 * @param strval the string value to check
 * @param value the integer value to check
 *
 * @return boolean - true if property exists with value, false otherwise.
 */
int
has_property(int descr, dbref player, dbref what, const char *pname, const char *strval,
             int value)
{
    dbref things;

    /* Does the object itself have the property? */
    if (has_property_strict(descr, player, what, pname, strval, value))
        return 1;

    /* Do any of the object's contents have the property? */
    for (things = CONTENTS(what); things != NOTHING; things = NEXTOBJ(things)) {
        if (has_property(descr, player, things, pname, strval, value))
            return 1;
    }

    /* Is the lock_envcheck tune parameter on?  If so, check environment
     * rooms for property.
     */
    if (tp_lock_envcheck) {
        things = getparent(what);

        while (things != NOTHING) {
            if (has_property_strict(descr, player, things, pname, strval, value))
                return 1;

            things = getparent(things);
        }
    }

    return 0;
}

/**
 * has_property_strict is the call that underpins has_property, and
 * unlike has_property, it checks property values for a *single*
 * object and not the object, all its contents, etc.
 *
 * Otherwise, all the notes for has_property apply here and you should
 * read the information there for more details as that call goes into
 * great detail about how this works.
 *
 * @see has_property
 * @param descr Integer descriptor of the caller.
 * @param player the DBREF of the player that has triggered the has_property
 *        check
 * @param what the object that triggered teh check
 * @param pname The property name to check
 * @param strval the string value to check
 * @param value the integer value to check
 *
 * @return boolean - true if property exists with value, false otherwise.
 */
static int has_prop_recursion_limit = 2;
int
has_property_strict(int descr, dbref player, dbref what, const char *pname, const char *strval,
                    int value)
{
    PropPtr p;
    const char *str;
    char *ptr;
    char buf[BUFFER_LEN];

    p = get_property(what, pname);

    if (p) {
#ifdef DISKBASE
        propfetch(what, p);
#endif

        switch (PropType(p)) {
            case PROP_STRTYP:
                str = DoNull(PropDataStr(p));

                if (has_prop_recursion_limit-- > 0) {
                    ptr =
                        do_parse_mesg(
                            descr, player, what, str, "(Lock)", buf, sizeof(buf),
                            (MPI_ISPRIVATE | MPI_ISLOCK |
                            ((PropFlags(p) & PROP_BLESSED) ? MPI_ISBLESSED : 0))
                        );
                } else {
                    strcpyn(buf, sizeof(buf), str);
                    ptr = buf;
                }

                has_prop_recursion_limit++;

                return (equalstr((char *) strval, ptr));

            case PROP_INTTYP:
                return (value == PropDataVal(p));

            case PROP_FLTTYP:
                return (value == (int) PropDataFVal(p));

            default:
                /* assume other types don't match */
                return 0;
        }
    }

    return 0;
}

/**
 * The name of this call is a little bit of a misnomer; this actually
 * returns the STRING property value of a given property.  If the property
 * is not a string property (i.e. if it is an integer or ref), this will
 * return NULL.  If the property does not exist, it will also return NULL.
 *
 * @param player The object to look up the ref on
 * @param pname The property name to look up.
 *
 * @return either the string value of the property or NULL as described above.
 */
const char *
get_property_class(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_STRTYP)
            return (char *) NULL;

        return (PropDataStr(p));
    } else {
        return (char *) NULL;
    }
}

/**
 * This is another misnomer; get_property_value actually gets the
 * INTEGER value of a property.  If it is a STRING or other property
 * type, it will return 0 instead.  It will also return 0 if the
 * property does not exist.
 *
 * @param player DBREF of object to look up
 * @param pname the property name to look up.
 *
 * @return integer property value or 0 as described above.
 */
int
get_property_value(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_INTTYP)
            return 0;

        return (PropDataVal(p));
    } else {
        return 0;
    }
}

/**
 * Get the floating point value for a given property name.  If the property
 * is not a floating point (like if it is a string or integer), this will
 * return 0.0.  If the property does not exist, it will also return 0.0.
 *
 * @param player DBREF of the object to look up.
 * @param pname The property name to look up
 *
 * @return Floating point value of property, or 0.0 as described above.
 */
double
get_property_fvalue(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_FLTTYP)
            return 0.0;

        return (PropDataFVal(p));
    } else {
        return 0.0;
    }
}

/**
 * This gets the DBREF property value of a given property name.  If the
 * property is not a dbref property (like if its a string or integer), this
 * will return NULL.  If the property does not exist, it will also return
 * NULL.
 *
 * @param player the object to look up the ref on.
 * @param pname The property name to look up.
 *
 * @return either the dbref value of the property or NULL as described above.
 */
dbref
get_property_dbref(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (!p)
        return NOTHING;

#ifdef DISKBASE
    propfetch(player, p);
#endif

    if (PropType(p) != PROP_REFTYP)
        return NOTHING;

    return PropDataRef(p);
}

/**
 * Get the boolexp (lock) value for a given property name.  If the
 * property is not a lock prop, if it does not exist, or if it won't
 * load from the diskbase (... not sure under what condition that would
 * happen!) then this returns TRUE_BOOLEXP.
 *
 * @param player DBREF of object to loop up.
 * @param pname the property name to look up
 *
 * @return boolexp structure if a lock property, otherwise TRUE_BOOLEXP
 *         as described above.
 */
struct boolexp *
get_property_lock(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (!p)
        return TRUE_BOOLEXP;

#ifdef DISKBASE
    propfetch(player, p);

    /* I'm not sure why this check is necessary; it isn't done anywhere
     * else that I have seen.  Is it possible for locks to not load?
     */
    if (PropFlags(p) & PROP_ISUNLOADED)
        return TRUE_BOOLEXP;
#endif
    if (PropType(p) != PROP_LOKTYP)
        return TRUE_BOOLEXP;

    return PropDataLok(p);
}

/**
 * Get the flags for a given property name.  At the time of this writing,
 * the flags are mostly used for BLESSED properties.  This doesn't include
 * the property type flags.
 *
 * @param player DBREF of the object to look up.
 * @param pname The property name to look up
 *
 * @return integer flags of property - or 0 if not found.
 */
int
get_property_flags(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
        return (PropFlags(p));
    } else {
        return 0;
    }
}

/**
 * This clears the indicated flags off the property.  It does not clear
 * type flags.  This is primarily used to remove the PROP_BLESSED flag.
 *
 * If the property does not exist, this function does nothing, but will
 * not cause an error of any sort.
 *
 * @param player The object to clear flags off of.
 * @param pname The name of the property
 * @param flags The flags to clear.
 */
void
clear_property_flags(dbref player, const char *pname, int flags)
{
    PropPtr p;

    flags &= ~PROP_TYPMASK;
    p = get_property(player, pname);
    if (p) {
        SetPFlags(p, (PropFlags(p) & ~flags));
#ifdef DISKBASE
        dirtyprops(player);
#endif
    }
}

/**
 * Sets the provided flags on a property named 'type' on object 'player'.
 *
 * Does not allow setting of property type flag; this is primarily used
 * to set BLESSED.
 *
 * @param player The object to operate on
 * @param pname The property name
 * @param flags The flags to set.
 */
void
set_property_flags(dbref player, const char *pname, int flags)
{
    PropPtr p;

    flags &= ~PROP_TYPMASK;
    p = get_property(player, pname);
    if (p) {
        SetPFlags(p, (PropFlags(p) | flags));
#ifdef DISKBASE
        dirtyprops(player);
#endif
    }
}

/**
 * This copies all the properties on an object and returns the root node.
 * It is used, for example, by the COPYOBJ primitive to copy all the
 * properties on an object.
 *
 * @param old DBREF of original object.
 * @return a struct plist that is a copy of all properties on 'old'.
 */
PropPtr
copy_prop(dbref old)
{
    PropPtr p, n = NULL;

#ifdef DISKBASE
    fetchprops(old, NULL);
#endif

    p = DBFETCH(old)->properties;
    copy_proplist(old, &n, p);
    return (n);
}

/**
 * This copies the properties from 'from' onto 'to'.  It does not blow
 * away 'to's existing properties but merges in 'from' into 'to'.
 *
 * This is essentially like copy_prop, except copy_prop would be used
 * for an object in the process of being creaetd, and this will work
 * better for an existing object.
 *
 * @param from Source DBREF
 * @param to Destination DBREF
 */
void
copy_properties_onto(dbref from, dbref to)
{
    PropPtr from_props;
#ifdef DISKBASE
    fetchprops(from, NULL);
    fetchprops(to, NULL);
#endif

    from_props = DBFETCH(from)->properties;

    copy_proplist(from, &DBFETCH(to)->properties, from_props);
}

/**
 * Returns a pointer to the first property on an object for the given
 * propdir.  This is useful for iterating over properties, such as with
 * the NEXTPROP primitive.
 *
 * This version does not fetch from the diskbase and is mostly for internal
 * use; chances are you want first_prop.
 *
 * dir may be NULL to indicate the / root property.
 *
 * @internal
 * @param player the DBREF of the object to get the properties from.
 * @param dir The string name of the propdir
 * @param list pointer to a proplist.  We will use this field to return
 *        the root node to you.
 * @param name pointer to a string buffer - this will be used to return
 *        the property name to you.
 * @param maxlen the size of the buffer.
 *
 * @return a pointer to the first property in a propdir, or NULL if
 *         the property list is empty.  If there is no property, then
 *         name will be an empty string.
 */
PropPtr
first_prop_nofetch(dbref player, const char *dir, PropPtr * list, char *name, size_t maxlen)
{
    char buf[BUFFER_LEN];
    PropPtr p;

    /* Trim off leading delimiters if there are extra */
    if (dir) {
        while (*dir && *dir == PROPDIR_DELIMITER) {
            dir++;
        }
    }

    /* If we have trimmed off all the lead delimiters (just / was passed),
     * or dir was simply not passed, then default to the root property.
     */
    if (!dir || !*dir) {
        *list = DBFETCH(player)->properties;

        p = first_node(*list);

        if (p) {
            strcpyn(name, maxlen, PropName(p));
        } else {
            *name = '\0';
        }

        return (p);
    }

    /* Try to fetch our propdir element. */
    strcpyn(buf, sizeof(buf), dir);
    *list = p = propdir_get_elem(DBFETCH(player)->properties, buf);

    if (!p) { /* Not found */
        *name = '\0';
        return NULL;
    }

    /* Find the first node in our propdir */
    *list = PropDir(p);
    p = first_node(*list);

    if (p) {
        strcpyn(name, maxlen, PropName(p));
    } else {
        *name = '\0';
    }

    return (p);
}

/**
 * Returns a pointer to the first property on an object for the given
 * propdir.  This is useful for iterating over properties, such as with
 * the NEXTPROP primitive.
 *
 * This version has 'diskbase' wrappers; under the hood, it calls
 * first_prop_nofetch.  You should probably use this version.
 *
 * dir may be NULL to indicate the / root property.
 *
 * @param player the DBREF of the object to get the properties from.
 * @param dir The string name of the propdir
 * @param list pointer to a proplist.  We will use this field to return
 *        the root node to you.
 * @param name pointer to a string buffer - this will be used to return
 *        the property name to you.
 * @param maxlen the size of the buffer.
 *
 * @return a pointer to the first property in a propdir, or NULL if
 *         the property list is empty.  If there is no property, then
 *         name will be an empty string.
 */
PropPtr
first_prop(dbref player, const char *dir, PropPtr * list, char *name, size_t maxlen)
{

#ifdef DISKBASE
    fetchprops(player, (char *) dir);
#endif

    return (first_prop_nofetch(player, dir, list, name, maxlen));
}

/**
 * next_prop is a wrapper around next_node to provide a slightly different
 * way to get the next property.
 *
 * Given the root of an AVL 'list' and a property 'prop', this function
 * returns the next property after 'prop' or NULL if there is no next
 * property.
 *
 * 'name' is a buffer used to store the name of the returned property;
 * it is not read from, it is strictly used to return the name.  As
 * you can easily get the name from the property with PropName, this
 * has sort of questionable utility but is available if useful to you
 *
 * maxlen is the length of your buffer.
 *
 * @param list the AVL prop list root
 * @param prop the 'previous' node - we will get the next node after this one
 * @param name a buffer to copy the property name into
 * @param maxlen the length of that buffer.
 *
 * @return the next property node or NULL if no more.
 */
PropPtr
next_prop(PropPtr list, PropPtr prop, char *name, size_t maxlen)
{
    PropPtr p = prop;

    if (!p || !(p = next_node(list, PropName(p))))
        return ((PropPtr) 0);

    strcpyn(name, maxlen, PropName(p));
    return (p);
}

/**
 * next_prop_name returns the string name of the next property on a
 * given object (player) with the "previous proprty" being "name".
 * outbuf is used to store the name of the property after the property "name"
 *
 * This will return 'outbuf' if there is a next property, or NULL if there
 * is not.  If there is no next property, outbuf will be an empty string.
 *
 * Call this with name "" to get the first property.
 *
 * @param player the object to fetch properties from
 * @param outbuf the output buffer to use for storing the next prop name
 * @param outbuflen The size of the output buffer.
 * @param name the name of the previous property
 *
 * @return either 'outbuf' or NULL as described above.
 */
char *
next_prop_name(dbref player, char *outbuf, size_t outbuflen, char *name)
{
    char *ptr;
    char buf[BUFFER_LEN];
    PropPtr p, l;

#ifdef DISKBASE
    fetchprops(player, propdir_name(name));
#endif

    strcpyn(buf, sizeof(buf), name);

    /* If passed an empty string, or if the last character is a /, then
     * we are loading a propdir
     */
    if (!*name || name[strlen(name) - 1] == PROPDIR_DELIMITER) {
        l = DBFETCH(player)->properties;
        p = propdir_first_elem(l, buf); /* Get the first property in this
                                         * prop directory.
                                         */
        if (!p) {
            *outbuf = '\0';
            return NULL;
        }

        if (!*name) {
            outbuf[0] = PROPDIR_DELIMITER;
            outbuf[1] = '\0';
        } else {
            strcpyn(outbuf, outbuflen, name);
        }

        /* Assemble a name from the rest of the path */
        strcatn(outbuf, outbuflen, PropName(p));
    } else {
        l = DBFETCH(player)->properties;
        p = propdir_next_elem(l, buf); /* Get the next prop element */

        if (!p) {
            *outbuf = '\0';
            return NULL;
        }

        strcpyn(outbuf, outbuflen, name);
        ptr = strrchr(outbuf, PROPDIR_DELIMITER);

        if (!ptr)
            ptr = outbuf;

        *(ptr++) = PROPDIR_DELIMITER;

        /* Assemble the full path to the next prop element */
        strcpyn(ptr, outbuflen - (size_t)(ptr - outbuf), PropName(p));
    }

    return outbuf;
}


/**
 * Get the size in bytes of the properties on an object.  If load is 1,
 * we will actually load all properties on the object before calculating
 * size; this will give an accurate depiction of the total size, though
 * load = 0 will tell you the size of what is currently loaded in memory.
 *
 * @param player The object to check
 * @param load a boolean who's use is described above.
 *
 * @return the size in bytes consumed by the object in memory.
 */
size_t
size_properties(dbref player, int load)
{
#ifdef DISKBASE
    if (load) {
        fetchprops(player, NULL);
        fetch_propvals(player, (char[]){PROPDIR_DELIMITER,0});
    }
#endif

    return size_proplist(DBFETCH(player)->properties);
}

/**
 * Checks to see if the property 'dir' is a propdir or not on the object
 * 'player'
 *
 * @param player DB of object to check
 * @param dir The property path to check.
 *
 * @return boolean - 1 if is propdir, otherwise 0 (including if prop does
 *         not exist.
 */
int
is_propdir(dbref player, const char *pname)
{

#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
#endif

    PropPtr p;
    char w[BUFFER_LEN];

    strcpyn(w, sizeof(w), pname);
    p = propdir_get_elem(DBFETCH(player)->properties, w);

    if (!p)
        return 0;

    return (PropDir(p) != (PropPtr) NULL);
}

/**
 * This scans the environment for a given propname, starting with
 * the DBREF 'where' and crawling up the environment.  Please note
 * that this modifies 'where'; 'where' will ultimately contain the
 * object that the prop was found on.
 *
 * Will return NULL if the property is not found, and 'where' will
 * become NOTHING at that point.
 *
 * @param where A pointer to dbref object which contains the start object
 * @param propname The name of the property to search for.
 * @param typ The property type to look for or 0 for any type.
 *
 * @return Either the prop structure we found, or NULL if not found.
 *         Note that 'where' is mutated as well.
 */
PropPtr
envprop(dbref * where, const char *propname, int typ)
{
    PropPtr temp;

    while (*where != NOTHING) {
        temp = get_property(*where, propname);

#ifdef DISKBASE
        if (temp)
            propfetch(*where, temp);
#endif

        if (temp && (!typ || PropType(temp) == typ))
            return temp;

        *where = getparent(*where);
    }

    return NULL;
}

/**
 * This scans the environment for a given propname and returns the
 * string contents of the property.  The property MUST be a string
 * or this will fail.  Returns NULL if property is not found.
 *
 * 'where' will be mutated to be the dbref of the object where the
 * property was found or NOTHING if not found.
 *
 * @param where A pointer to dbref object which contains the start object
 * @param propname The name of the property to search for.
 *
 * @return Either the prop structure we found, or NULL if not found.
 *         Note that 'where' is mutated as well.
 */
const char *
envpropstr(dbref * where, const char *propname)
{
    PropPtr temp;

    temp = envprop(where, propname, PROP_STRTYP);
    if (!temp)
        return NULL;

    return (PropDataStr(temp));
}

/**
 * This function takes a property and generates a line akin to what
 * you would see on a prop listing (e.g. ex me=/)
 *
 * So a line that looks like this:
 *
 * - str /whatever:value
 *
 * The first character is either - or B for blessed.  Then the type.
 * Then the name, and finally the value.
 *
 * Any errors (no such property) will be written to the buffer.
 *
 * @param player is the player doing the call, and is used to determine
 *        permissions for unparse_object ('ref' type props)
 * @param obj is the object who's property is being examined.
 * @param name is the property name
 * @param buf is a buffer that you provide for property information to
 *        be loaded into.
 * @param bufsiz is the size of the buffer you are providing.
 *
 * @return the buffer (though using the return value is not necessary
 *         since the buffer is modified).
 */
char *
displayprop(dbref player, dbref obj, const char *name, char *buf, size_t bufsiz)
{
    char mybuf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    int pdflag;
    char blesschar = '-';
    PropPtr p = get_property(obj, name);

    if (!p) {
        snprintf(buf, bufsiz, "%s: No such property.", name);
        return buf;
    }

#ifdef DISKBASE
    propfetch(obj, p);
#endif

    if (PropFlags(p) & PROP_BLESSED)
        blesschar = 'B';

    pdflag = (PropDir(p) != NULL);
    snprintf(mybuf, bufsiz, "%.*s%c", (BUFFER_LEN / 4), name,
            (pdflag) ? PROPDIR_DELIMITER : '\0');

    switch (PropType(p)) {
        case PROP_STRTYP:
            snprintf(buf, bufsiz, "%c str %s:%.*s", blesschar, mybuf, (BUFFER_LEN / 2),
                     PropDataStr(p));
            break;
        case PROP_REFTYP:
            unparse_object(player, PropDataRef(p), unparse_buf, sizeof(unparse_buf));
            snprintf(buf, bufsiz, "%c ref %s:%s", blesschar, mybuf, unparse_buf);
            break;
        case PROP_INTTYP:
            snprintf(buf, bufsiz, "%c int %s:%d", blesschar, mybuf, PropDataVal(p));
            break;
        case PROP_FLTTYP:
            snprintf(buf, bufsiz, "%c flt %s:%.17g", blesschar, mybuf, PropDataFVal(p));
            break;
        case PROP_LOKTYP:
            if (PropFlags(p) & PROP_ISUNLOADED) {
                snprintf(buf, bufsiz, "%c lok %s:%s", blesschar, mybuf, PROP_UNLOCKED_VAL);
            } else {
                snprintf(buf, bufsiz, "%c lok %s:%.*s", blesschar, mybuf, (BUFFER_LEN / 2),
                unparse_boolexp(player, PropDataLok(p), 1));
            }

            break;
        case PROP_DIRTYP:
            snprintf(buf, bufsiz, "%c dir %s:(no value)", blesschar, mybuf);
            break;
    }

    return buf;
}

/**
 * This gets a single property from the database.  It is usually used
 * by a higher level method such as db_getprops or diskbase's propfetch
 * You should probably not use it yourself.
 *
 * A point of interest is "pos", this is the initial position to load
 * the prop from or 0.  In the case of diskbase props, if a prop is in
 * the UNLOADED state, the value stored in the prop is an integer which
 * is the file position of the prop in teh database file.
 *
 * If pos is non 0, we will load the prop from that position, which is
 * how diskbase properties are loaded.  If pos is 0, we will actually
 * load the next prop in the database and use the current database
 * position for the diskbase if needed.
 *
 * @internal
 * @param f DB File handle.
 * @param obj DBREF of object who's properties we are loading.
 * @param pos Position to load the property from, or 0 to load in squence.
 * @param pnode If we have an existing property AVL node to load into.
 *              This may be NULL if you do not have it.
 * @param pdir This is used exclusively for error display.  This is
 *             usually a propdir but can be NULL.
 *
 * @return -1 on failure, 0 if end of prop list in DB, 1 if a prop was loaded,
 *         2 if the prop is empty and skipped.
 */
int
db_get_single_prop(FILE * f, dbref obj, long pos, PropPtr pnode, const char *pdir)
{
    char getprop_buf[BUFFER_LEN * 3];
    char *name, *flags, *value, *p;
    int flg;
    long tpos = 0L;
    struct boolexp *lok;
    short do_diskbase_propvals;
    PData mydat;

#ifdef DISKBASE
    do_diskbase_propvals = tp_diskbase_propvals;
#else
    do_diskbase_propvals = 0;
#endif

    /* If pos is provided, we are loading from the diskbase; seek to
     * the correct position to load the property.
     *
     * Otherwise, we're assuming that we are loading props sequentially,
     * and we will use the current position as the diskbase position for
     * this given property.
     */
    if (pos) {
        fseek(f, pos, SEEK_SET);
    } else if (do_diskbase_propvals) {
        tpos = ftell(f);
    }

    /* Props are stored one per line, with the list of props terminated
     * by the string *End*
     *
     * The maximum property size is BUFFER_LEN * 3
     *
     * @TODO Verify that props written can never be larger than
     *       BUFFER_LEN*3 ... props that violate that max size would
     *       cause chaos in this system.
     */
    name = fgets(getprop_buf, sizeof(getprop_buf), f);

    /* This would be either due to a bad diskbase property file position,
     * or a truncated DB most likely.
     */
    if (!name) {
        wall_wizards
        ("## WARNING! A corrupt property was found while trying to read it from disk.");
        wall_wizards
        ("##   This property has been skipped and will not be loaded.  See the sanity");
        wall_wizards("##   logfile for technical details.");
        log_sanity
        ("Failed to read property from disk: Failed disk read.  obj = #%d, pos = %ld, pdir = %s",
         obj, pos, pdir);
        return -1;
    }

    /* If the first character is an astrix, it may either be *End*,
     * the marker for end of the list of props, or just an astrix in
     * which case we're skipping the line.
     */
    if (*name == '*') {
        if (!strcmp(name, "*End*\n")) {
            return 0;
        }

        if (name[1] == '\n') {
            return 2;
        }
    }

    /* Properties are delimited in the DB file by the prop delimiter
     * character (colon - : ) The format is:
     *
     * name:flags:value
     *
     * The 'flags' is a string value which must be atoi'd.
     *
     * If anything is missing here, we will throw a critical error,
     * but all these errors are super unlikely unless something has
     * gone horribly wrong with the file.
     */
    flags = strchr(name, PROP_DELIMITER);
    if (!flags) {
        wall_wizards
        ("## WARNING! A corrupt property was found while trying to read it from disk.");
        wall_wizards
        ("##   This property has been skipped and will not be loaded.  See the sanity");
        wall_wizards("##   logfile for technical details.");
        log_sanity
        ("Failed to read property from disk: Corrupt property, flag delimiter not found.  obj = #%d, pos = %ld, pdir = %s, data = %s",
         obj, pos, pdir, name);
        return -1;
    }

    *flags++ = '\0';

    value = strchr(flags, PROP_DELIMITER);

    if (!value) {
        wall_wizards
        ("## WARNING! A corrupt property was found while trying to read it from disk.");
        wall_wizards
        ("##   This property has been skipped and will not be loaded.  See the sanity");
        wall_wizards("##   logfile for technical details.");
        log_sanity
        ("Failed to read property from disk: Corrupt property, value delimiter not found.  obj = #%d, pos = %ld, pdir = %s, data = %s:%s",
         obj, pos, pdir, name, flags);
        return -1;
    }

    *value++ = '\0';

    p = strchr(value, '\n');
    if (p) {
        *p = '\0';
    }

    if (!number(flags)) {
        wall_wizards
        ("## WARNING! A corrupt property was found while trying to read it from disk.");
        wall_wizards
        ("##   This property has been skipped and will not be loaded.  See the sanity");
        wall_wizards("##   logfile for technical details.");
        log_sanity
        ("Failed to read property from disk: Corrupt property flags.  obj = #%d, pos = %ld, pdir = %s, data = %s:%s:%s",
         obj, pos, pdir, name, flags, value);
        return -1;
    }

    /* Get the flag number, then process the prop based on type. */
    flg = atoi(flags);

    /* Note that for each type, we do a branch based on disk base.
     *
     * If we have provided a pos, we want to load the prop off the
     * disk -- or if we are not using diskbase.
     *
     * If pos is 0 and we are using diskbase, then we will load the
     * 'tpos' into the property and set it PROP_ISUNLOADED.
     */
    switch (flg & PROP_TYPMASK) {
        case PROP_STRTYP:
            if (!do_diskbase_propvals || pos) {
                flg &= ~PROP_ISUNLOADED;

                if (pnode) {
                    SetPDataStr(pnode, alloc_string(value));
                    SetPFlagsRaw(pnode, flg);
                } else {
                    mydat.flags = flg;
                    mydat.data.str = value;
                    set_property_nofetch(obj, name, &mydat, 0);
                }
            } else {
                flg |= PROP_ISUNLOADED;
                mydat.flags = flg;
                mydat.data.val = tpos;
                set_property_nofetch(obj, name, &mydat, 0);
            }
            break;
        case PROP_LOKTYP:
            if (!do_diskbase_propvals || pos) {
                lok = parse_boolexp(-1, (dbref) 1, value, 32767);
                flg &= ~PROP_ISUNLOADED;

                if (pnode) {
                    SetPDataLok(pnode, lok);
                    SetPFlagsRaw(pnode, flg);
                } else {
                    mydat.flags = flg;
                    mydat.data.lok = lok;
                    set_property_nofetch(obj, name, &mydat, 0);
                }
            } else {
                flg |= PROP_ISUNLOADED;
                mydat.flags = flg;
                mydat.data.val = tpos;
                set_property_nofetch(obj, name, &mydat, 0);
            }
            break;
        case PROP_INTTYP:
            if (!number(value)) {
                wall_wizards
                ("## WARNING! A corrupt property was found while trying to read it from disk.");
                wall_wizards
                ("##   This property has been skipped and will not be loaded.  See the sanity");
                wall_wizards("##   logfile for technical details.");
                log_sanity
                ("Failed to read property from disk: Corrupt integer value.  obj = #%d, pos = %ld, pdir = %s, data = %s:%s:%s",
                 obj, pos, pdir, name, flags, value);
                return -1;
            }

            mydat.flags = flg;
            mydat.data.val = atoi(value);
            set_property_nofetch(obj, name, &mydat, 0);
            break;
        case PROP_FLTTYP:
            mydat.flags = flg;

            if (!number(value) && !ifloat(value)) {
                char *tpnt = value;
                int dtemp = 0;

                if ((*tpnt == '+') || (*tpnt == '-')) {
                    if (*tpnt == '+') {
                        dtemp = 0;
                    } else {
                        dtemp = 1;
                    }

                    tpnt++;
                }

                tpnt[0] = toupper(tpnt[0]);
                tpnt[1] = toupper(tpnt[1]);
                tpnt[2] = toupper(tpnt[2]);
                if (!strncmp(tpnt, "INF", 3)) {
                    if (!dtemp) {
                        mydat.data.fval = INF;
                    } else {
                        mydat.data.fval = NINF;
                    }
                } else {
                    if (!strncmp(tpnt, "NAN", 3)) {
                        /* FIXME: This should be NaN. */
                        mydat.data.fval = INF;
                    }
                }
            } else {
                sscanf(value, "%lg", &mydat.data.fval);
            }

            set_property_nofetch(obj, name, &mydat, 0);
            break;
        case PROP_REFTYP:
            if (!number(value)) {
                wall_wizards
                ("## WARNING! A corrupt property was found while trying to read it from disk.");
                wall_wizards
                ("##   This property has been skipped and will not be loaded.  See the sanity");
                wall_wizards("##   logfile for technical details.");
                log_sanity
                ("Failed to read property from disk: Corrupt dbref value.  obj = #%d, pos = %ld, pdir = %s, data = %s:%s:%s",
                 obj, pos, pdir, name, flags, value);
                return -1;
            }

            mydat.flags = flg;
            mydat.data.ref = atoi(value);
            set_property_nofetch(obj, name, &mydat, 0);
            break;
        case PROP_DIRTYP:
            break;
    }

    return 1;
}

/**
 * This fetches all props for a given object.  It assumes the file pointer
 * 'f' is in position to start loading properties.  It will load all
 * properties (or, if diskbase is turned on, prop stubs) from the database.
 *
 * @param f The File pointer of the database.
 * @param obj The DBREF of the object we are loading props for.
 * @param pdir The propdir we are loading -- this should always be "/"
 *             but it really doesn't matter since it is just used in
 *             error messages.
 */
void
db_getprops(FILE * f, dbref obj, const char *pdir)
{
    while (db_get_single_prop(f, obj, 0L, (PropPtr) NULL, pdir)) ;
}

/**
 * Writes a single property to the disk.
 *
 * This is exclusively used in property.c and could be refactored so as
 * not to be exposed in this include file.
 *
 * @param f The database file
 * @param dir The prop dir this prop is in.  This *is* actually used by
 *            this method, but I am unsure if it is strictly necessary.
 * @param p The prop to save to the file.
 */
static void
db_putprop(FILE * f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN * 2];
    const char *ptr2;
    char tbuf[50];
    int outflags = (PropFlagsRaw(p) & ~(PROP_TOUCHED | PROP_ISUNLOADED | PROP_DIRUNLOADED));

    if (PropType(p) == PROP_DIRTYP)
        return;

    ptr2 = "";
    switch (PropType(p)) {
        case PROP_INTTYP:
            if (!PropDataVal(p))
                return;
            ptr2 = intostr(PropDataVal(p));
            break;
        case PROP_FLTTYP:
            if (!PropDataFVal(p))
                return;
            snprintf(tbuf, sizeof(tbuf), "%.17g", PropDataFVal(p));
            ptr2 = tbuf;
            break;
        case PROP_REFTYP:
            if (PropDataRef(p) == NOTHING)
                return;
            ptr2 = intostr(PropDataRef(p));
            break;
        case PROP_STRTYP:
            if (!*PropDataStr(p))
                return;
            ptr2 = PropDataStr(p);
            break;
        case PROP_LOKTYP:
            if (PropFlags(p) & PROP_ISUNLOADED)
                return;

            if (PropDataLok(p) == TRUE_BOOLEXP)
                return;
            ptr2 = unparse_boolexp((dbref) 1, PropDataLok(p), 0);
            break;
    }

    snprintf(buf, sizeof(buf), "%s%s%c%d%c%s\n",
         dir + 1, PropName(p), PROP_DELIMITER, outflags, PROP_DELIMITER, ptr2);

    if (fputs(buf, f) == EOF) {
        log_sanity("Failed to write out property db_putprop(dir = %s)", dir);
        abort();
    }
}

/**
 * Recursively dumps properties on object 'obj' to file handle 'f'
 * statring with directory 'dir' that has propdir object 'p'.
 *
 * You would normally kick this off by passing "/" to dir.
 *
 * @private
 * @param obj the DB object ref
 * @param f The file handle to write to.
 * @param dir the path that belongs to p
 * @param p The proptr that belongs to path.
 *
 * @return integer number of properties dumped.
 */
static int
db_dump_props_rec(dbref obj, FILE * f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN];
#ifdef DISKBASE
    long tpos = 0L;
    int flg;
    short wastouched = 0;
#endif
    int count = 0;
    int pdcount;

    if (!p)
        return 0;

    count += db_dump_props_rec(obj, f, dir, p->left);

#ifdef DISKBASE
    wastouched = (PropFlags(p) & PROP_TOUCHED);

    if (tp_diskbase_propvals) {
        tpos = ftell(f);
    }

    if (wastouched) {
        count++;
    }

    if (propfetch(obj, p)) {
        fseek(f, 0L, SEEK_END);
    }
#endif

    db_putprop(f, dir, p);

#ifdef DISKBASE
    if (tp_diskbase_propvals && !wastouched) {
        if (PropType(p) == PROP_STRTYP || PropType(p) == PROP_LOKTYP) {
            flg = PropFlagsRaw(p) | PROP_ISUNLOADED;
            clear_propnode(p);
            SetPFlagsRaw(p, flg);
            SetPDataVal(p, tpos);
        }
    }
#endif

    if (PropDir(p)) {
        const char *iptr;
        char *optr;

        for (iptr = dir, optr = buf; *iptr;)
            *optr++ = *iptr++;

        for (iptr = PropName(p); *iptr;)
            *optr++ = *iptr++;

        *optr++ = PROPDIR_DELIMITER;
        *optr++ = '\0';

        pdcount = db_dump_props_rec(obj, f, buf, PropDir(p));
        count += pdcount;
    }

    count += db_dump_props_rec(obj, f, dir, p->right);

    return count;
}

/**
 * Starts a recursive dump of props to the given file handle for the
 * given object.
 *
 * Under the hood, this uses db_dump_props_rec
 *
 * @param f File handle to write properties to.
 * @param obj DBREF of object to dump
 */
void
db_dump_props(FILE * f, dbref obj)
{
    db_dump_props_rec(obj, f, (char[]){PROPDIR_DELIMITER,0}, DBFETCH(obj)->properties);
}

/**
 * Recursively "untouch" props, or in other words, remove the PROP_TOUCHED
 * flag.  This is the underpinning of untouchprops_incremental
 *
 * @see untouchprops_incremental
 *
 * @private
 * @param p the propdir AVL to work on.
 */
static void
untouchprop_rec(PropPtr p)
{
    if (!p)
        return;

    SetPFlags(p, (PropFlags(p) & ~PROP_TOUCHED));
    untouchprop_rec(p->left);
    untouchprop_rec(p->right);
    untouchprop_rec(PropDir(p));
}


/**
 * This function is a progressive iteration over the entire database,
 * keeping track of its last position with a static variable.  It will
 * iterate over "limit" number of database refs, marking all properties
 * on that database object as untouched (in other words, removing the
 * PROP_TOUCHED property).
 *
 * Once it reaches the end of the database, it starts over again.
 *
 * This is called once per game loop.  I am honestly not sure
 * what the purpose of this is, but it would likely be very disruptive
 * to mess with it.  Recommend not touching this call.
 *
 * @internal
 * @param limit The number of database objects to process before returning.
 */
static dbref untouch_lastdone = 0;
void
untouchprops_incremental(int limit)
{
    PropPtr p;

    while (untouch_lastdone < db_top) {
        /* clear the touch flags */
        p = DBFETCH(untouch_lastdone)->properties;

        if (p) {
            if (!limit--)
                return;

            untouchprop_rec(p);
        }

        untouch_lastdone++;
    }

    untouch_lastdone = 0;
}


/**
 * A reflist is a space-delimited set of DBREFs in a string, each
 * ref starting with a hash mark, such as:
 *
 * #123 #456 #789
 *
 * This is a convienance method to add a dbref to a ref list.  If
 * the propname given already exists and is a 'ref' type prop, it
 * will be converted to a string with the old and new refs in it.
 * If the property is empty, this ref will be added to it as a
 * string prop to start a new reflist.
 *
 * If the toadd ref is already in the reflist, it will 'migrate' to
 * the end of the reflist.  This method does not allow refs to be
 * duplicate.
 *
 * @param obj The object to operate on
 * @param propname the property name for our reflist
 * @param toadd the ref to add to the reflist
 */
void
reflist_add(dbref obj, const char *propname, dbref toadd)
{
    PropPtr ptr;
    const char *temp;
    const char *list;
    int count = 0;
    size_t charcount = 0;
    char buf[BUFFER_LEN];
    char outbuf[BUFFER_LEN];

    ptr = get_property(obj, propname);

    /* If the property does not exist, the work here is trivial.
     * Otherwise, things get a little more complex.
     */
    if (ptr) {
        const char *pat = NULL;

#ifdef DISKBASE
        propfetch(obj, ptr);
#endif
        switch (PropType(ptr)) {
            /* If it is a string, it may already be a reflist. */
            case PROP_STRTYP:
                *outbuf = '\0';
                list = temp = PropDataStr(ptr);
                snprintf(buf, sizeof(buf), "%d", toadd);

                /* Iterate over the existing string
                 *
                 * I'll be honest, this logic makes my head hurt.  However,
                 * what it is doing is checking to see if the ref is already
                 * in the reflist by doing a 'tandem iteration' process.
                 * I describe this a lot better in reflist_del.
                 *
                 * If the ref is already in the string, it is first *removed*
                 * and then re-added onto the end.
                 *
                 * @TODO: This is a lot of copypasta code from reflist_del.
                 * Perhaps instead of all the copypasta, we should reflist_del
                 * before trying to add.  The performance difference would
                 * be marginal but it would confine this nightmare to one
                 * function.  Its also copypasta'd on reflist find ... so
                 * this is in 3 places.  We can do better!
                 */
                while (*temp) {
                    if (*temp == NUMBER_TOKEN) {
                        pat = buf; /* buf contains our new ref */
                        count++;
                        charcount = (size_t)(temp - list);
                    } else if (pat) {
                        if (!*pat) {
                            if (!*temp || *temp == ' ') {
                                /* This seems to mean we found our ref,
                                 * because we are at the end of pat (our
                                 * new ref) *and* we're either at the
                                 * end of the overall string or we've
                                 * found a space.
                                 */
                                break;
                            }

                            pat = NULL;
                        } else if (*pat != *temp) {
                            /* This seems to mean if we did not find our
                             * ref
                             */
                            pat = NULL;
                        } else {
                            /* 'buf' and the number we've found (at this
                             * juncture) match.
                             */
                            pat++;
                        }
                    }

                    temp++;
                }

                if (pat && !*pat) {
                    /* We found the number *and* we got to the end of
                     * the number still matching -- so basically our
                     * ref is already on the reflist.
                     */

                    if (charcount > 0) {
                        /* Copy whatever was before the ref I think ? */
                        strcpyn(outbuf, charcount, list);
                    }

                    /* Copy what is after the ref */
                    strcatn(outbuf, sizeof(outbuf), temp);
                } else {
                    /* Just copy the whole reflist into our buffer because
                     * we did not find our ref on the list.
                     */
                    strcpyn(outbuf, sizeof(outbuf), list);
                }

                /* Set up our string for concatenation */
                snprintf(buf, sizeof(buf), " #%d", toadd);

                if (strlen(outbuf) + strlen(buf) < BUFFER_LEN) {
                    /* If we have room, do the string concat and then
                     * clean out the white spaces.  Finally, set the prop.
                     */
                    strcatn(outbuf, sizeof(outbuf), buf);
                    temp = outbuf;
                    skip_whitespace(&temp);
                    add_property(obj, propname, temp, 0);
                }

                break;
            case PROP_REFTYP:
                /* If the prop was a ref prop, then concatinate the existing
                 * ref and our new ref (assuming what is set isn't the same
                 * as our current ref) then set it as a string prop.
                 */
                if (PropDataRef(ptr) != toadd) {
                    snprintf(outbuf, sizeof(outbuf), "#%d #%d", PropDataRef(ptr), toadd);
                    add_property(obj, propname, outbuf, 0);
                }

                break;
            default:
                /* If it isn't a string or a ref, just clobber it and
                 * treat it like an unset prop.
                 */
                snprintf(outbuf, sizeof(outbuf), "#%d", toadd);
                add_property(obj, propname, outbuf, 0);
                break;
        }
    } else {
        /* The prop isn't set -- the super simple case */
        snprintf(outbuf, sizeof(outbuf), "#%d", toadd);
        add_property(obj, propname, outbuf, 0);
    }
}

/**
 * Removes a ref from a reflist.  See the description of reflist_add
 * for a description of what a reflist is
 *
 * @see reflist_add
 *
 * @param obj the object to work on
 * @param propname the property name of the reflist
 * @param todel the ref to delete.
 */
void
reflist_del(dbref obj, const char *propname, dbref todel)
{
    PropPtr ptr;
    const char *temp;
    const char *list;
    int count = 0;
    size_t charcount = 0;
    char buf[BUFFER_LEN];
    char outbuf[BUFFER_LEN];

    ptr = get_property(obj, propname);
    if (ptr) {
        const char *pat = NULL;

#ifdef DISKBASE
        propfetch(obj, ptr);
#endif
        switch (PropType(ptr)) {
            /* This is largely copy/pasted from reflist_add */
            case PROP_STRTYP:
                *outbuf = '\0';
                list = temp = PropDataStr(ptr);
                snprintf(buf, sizeof(buf), "%d", todel);

                /* Iterate over the property */
                while (*temp) {
                    if (*temp == NUMBER_TOKEN) {
                        pat = buf; /* We found a number token -- let's see
                                    * if it is the number we want.
                                    *
                                    * Remember 'buf' has our ref in string
                                    * format.
                                    */
                        count++;
                        charcount = (size_t)(temp - list);
                    } else if (pat) {
                        /* If 'pat' is not NULL, then we are iterating
                         * over 'pat' and 'temp in tandem, comparing along
                         * the way to see if the numbers match.
                         */
                        if (!*pat) {
                            /* If we have reached the end of 'pat', and
                             * we have either reached the end of temp
                             * or we have reached a space, then we have
                             * found our number.
                             */
                            if (!*temp || *temp == ' ') {
                                break;
                            }

                            /* Otherwise, this isn't our number */
                            pat = NULL;
                        } else if (*pat != *temp) {
                            /* Pat and temp are no longer in sync -- this
                             * isn't our number.
                             */
                            pat = NULL;
                        } else {
                            /* Still in sync with temp -- step pat forward */
                            pat++;
                        }
                    }

                    temp++;
                }

                /* If pat is not NULL and we reached the end of pat, then
                 * we have a number match.
                 */
                if (pat && !*pat) {
                    if (charcount > 0) {
                        /* Copy whatever was in the reflist before our
                         * number to our output buffer, if applicable.
                         */
                        strcpyn(outbuf, charcount, list);
                    }

                    /* Concatinate in whatever was after our number
                     * in the reflist to our output buffer, clean up
                     * whitespace, and set.
                     */
                    strcatn(outbuf, sizeof(outbuf), temp);
                    skip_whitespace(&temp);
                    add_property(obj, propname, outbuf, 0);
                }

                break;
            case PROP_REFTYP:
                /* If the prop is a ref prop, and it matches, then turn
                 * it into an empty string.
                 */
                if (PropDataRef(ptr) == todel) {
                    add_property(obj, propname, "", 0);
                }
                break;
            default:
                break;
        }
    }
}

/**
 * This finds a ref in the reflist.  The number returned is either '0'
 * if not found, or it is the "position" of the ref in the reflist.
 *
 * So ... if you have a reflist:
 *
 * #123 #345 #678
 *
 * #123 is position 1, #345 is position 2, and #678 is position 3.
 *
 * This method is in support of the REFLIST_FIND primitive which is why
 * the odd return value.
 *
 * @param obj The object to work on
 * @param propname The reflist property name
 * @param tofind the ref to find in the list
 *
 * @return an integer describing the position of the ref as described above,
 *         or 0 if not found.
 */
int
reflist_find(dbref obj, const char *propname, dbref tofind)
{
    PropPtr ptr;
    const char *temp;
    int pos = 0;
    int count = 0;
    char buf[BUFFER_LEN];

    ptr = get_property(obj, propname);
    if (ptr) {
        const char *pat = NULL;

#ifdef DISKBASE
        propfetch(obj, ptr);
#endif
        switch (PropType(ptr)) {
            /* This is copy/pasta'd in reflist_add and reflist_del */
            case PROP_STRTYP:
                temp = PropDataStr(ptr);
                snprintf(buf, sizeof(buf), "%d", tofind);

                while (*temp) {
                    if (*temp == NUMBER_TOKEN) {
                        pat = buf; /* We found a number token -- let's see
                                    * if it is the number we want.
                                    *
                                    * Remember 'buf' has our ref in string
                                    * format.
                                    */
                        count++;
                    } else if (pat) {
                        /* If 'pat' is not NULL, then we are iterating
                         * over 'pat' and 'temp in tandem, comparing along
                         * the way to see if the numbers match.
                         */
                        if (!*pat) {
                            /* If we have reached the end of 'pat', and
                             * we have either reached the end of temp
                             * or we have reached a space, then we have
                             * found our number.
                             */
                            if (!*temp || *temp == ' ') {
                                break;
                            }

                            /* Otherwise, this isn't our number */
                            pat = NULL;
                        } else if (*pat != *temp) {
                            /* Pat and temp are no longer in sync -- this
                             * isn't our number.
                             */
                            pat = NULL;
                        } else {
                            /* Still in sync with temp -- step pat forward */
                            pat++;
                        }
                    }

                    temp++;
                }

                /* If pat is not NULL and we reached the end of pat, then
                 * we have a number match.
                 */
                if (pat && !*pat) {
                    pos = count;
                }

                break;
            case PROP_REFTYP:
                /* If its a ref, and the props match, then our ref
                 * is the first element.
                 */
                if (PropDataRef(ptr) == tofind)
                    pos = 1;

                break;
            default:
                break;
        }
    }

    return pos;
}

/**
 * This command is the underpinning of all of the @ commands that just
 * set a property under _/ .... for instance, @odrop, @describe, etc.
 * As such, this is not really an API call and it relies on the
 * global 'match_args' as well as emits messages to the user.
 *
 * If objname is an empty string, 'player' will be used as the target
 * object.  Otherwise, the name will be matched.  If the match fails,
 * a message will be emitted to the user.
 *
 * If there is no '=' mark in match_args, then the 'propname' is cleared.
 *
 * Appropriate messages based on proplabel are emitted.  Proplabel
 * should be somethinglike "Object Description", "Drop Message", etc.
 * and is used to emit messages.
 *
 * @param descr Descriptor
 * @param player The player doing the action
 * @param objname The object name to match, or "" for the player.
 * @param propname The property we are going to set, like "_/de"
 * @param proplabel Used for messaging -- see description above.
 * @param propvalue The value to set.
 */
void
set_standard_property(int descr, dbref player, const char *objname,
                      const char *propname, const char *proplabel,
                      const char *propvalue)
{
    dbref object;

    int set = !!strchr(match_args, ARG_DELIMITER);

    if (!*objname) {
        object = player;
    } else {
        object = match_controlled(descr, player, objname);
    }

    if (object != NOTHING) {
        if (!set) {
            notifyf_nolisten(player, "%s: %s", proplabel,
                             GETMESG(object, propname));
            return;
        }

        SETMESG(object, propname, propvalue);
        notifyf_nolisten(player, "%s %s.", proplabel,
                         propvalue && *propvalue ? "set" : "cleared");
    }
}

/**
 * This command is the underpinning of @lock, @flock, @linklock and @chlock.
 * Please note that part of this function's functionality relies on the
 * global 'match_args', so this is not really an API call.  This is
 * more of a command implementation.
 *
 * If objname is an empty string, 'player' will be used as the target
 * object.  Otherwise, the name will be matched.  If the match fails,
 * a message will be emitted to the user.
 *
 * If there is no '=' mark in match_args, it will emit a message
 * describing the lock on the object and does not set anything.
 *
 * If there is an = but no string after the equals (i.e. '@lock x='),
 * then it clears the lock
 *
 * Otherwise, the lock is 'compiled' and, if valid, set on the property
 * propname.  Otherwise, an error is emitted to the user.
 *
 * @param descr Descriptor
 * @param player The player doing the command
 * @param objname The name of the object we are locking or ""
 * @param propname The name of the prop to set
 * @param proplabel This is used for messaging and is the type of lock.
 *        The first letter should be capitalized.  For instance, "Lock",
 *        "Ownership Lock", etc.
 * @param keyvalue The lock itself or "" to clear the lock.
 */
void
set_standard_lock(int descr, dbref player, const char *objname,
                  const char *propname, const char *proplabel,
                  const char *keyvalue)
{
    dbref object;
    PData property;
    struct boolexp *key;

    /* The presence of '=' (ARG_DELIMITER) in the arguments indicates if
     * this is a result of a lock set command.
     */
    int set = !!strchr(match_args, ARG_DELIMITER);

    /* If object name was provided, then we will try to find it.
     * If it was not, then we default to the player.
     */
    if (!*objname) {
        object = player;
    } else {
        /* I believe this emits the error message if there is no match.
         */
        object = match_controlled(descr, player, objname);
    }

    if (object != NOTHING) { /* If we found an object */
        if (!set) { /* If there was no =, then display lock status.
                     * You can see this message if you just type '@lock'
                     * by itself.
                     */
            notifyf_nolisten(
                player, "%s: %s", proplabel,
                unparse_boolexp(player, get_property_lock(object, propname), 1)
            );

            return;
        }

        /* If there is no key value, clear the lock. You can see this
         * if you type: @lock whatver=
         */
        if (!*keyvalue) {
            remove_property(object, propname, 0);
            ts_modifyobject(object);
            notifyf_nolisten(player, "%s cleared.", proplabel);
            return;
        }

        /* Parse the lock */
        key = parse_boolexp(descr, player, keyvalue, 0);

        if (key == TRUE_BOOLEXP) {
            notifyf_nolisten(player, "I don't understand that key.");
            return;
        }

        /* If it all works out, set the property */
        property.flags = PROP_LOKTYP;
        property.data.lok = key;
        set_property(object, propname, &property, 0);
        ts_modifyobject(object);
        notifyf_nolisten(player, "%s set.", proplabel);
    }
}

/**
 * exec_or_notify is the thing that is used to process various "message"
 * props such as the props for @success, @odrop, etc.  It understands:
 *
 * - Strings that start with '@' symbol and run a MUF program
 * - Any other string.  MPI will be parsed.
 *
 * MUFs are run with PREEMPT and HARDUID.  Error conditions are notify'd
 * to the player.
 *
 * @param descr - integer descriptor to notify.
 * @param player - The DBREF of the calling player.
 * @param thing - The DBREF of the thing emitting the message.
 * @param message - The message to emit
 * @param whatcalled - This is the caller context -- its the
 *                     MPI &how or the MUF command.  Such as "(@Desc)"
 * @param mpiflags - What MPI flags to use - MPI_ISPRIVATE is added.
 */
void
exec_or_notify(int descr, dbref player, dbref thing,
               const char *message, const char *whatcalled, int mpiflags)
{
    const char *p;
    const char *p2;
    char *p3;
    char buf[BUFFER_LEN];
    char tmpcmd[BUFFER_LEN];
    char tmparg[BUFFER_LEN];

    p = message;

    if (*p == EXEC_SIGNAL) { /* This might be a MUF. */
        int i;

        if (*(++p) == REGISTERED_TOKEN) { /* @$whatever handling */
            strcpyn(buf, sizeof(buf), p);

            for (p2 = buf; *p && !isspace(*p); p++) ;

            if (*p)
                p++;

            for (p3 = buf; *p3 && !isspace(*p3); p3++) ;

            if (*p3)
                *p3 = '\0';

            if (*p2) {
                i = (dbref) find_registered_obj(thing, p2);
            } else {
                i = 0;
            }
        } else {
            i = atoi(p);

            for (; *p && !isspace(*p); p++) ;

            if (*p)
                p++;
        }

        /* If it wasn't a program, default to standard non-MPI parsed
         * notification or display the nothing-special message.
         *
         * @TODO: Shouldn't this parse MPI?  Granted, this is a crazy
         *        edge case, and maybe its useful to have a route where
         *        MPI is not parsed.
         */
        if (!ObjExists(i) || (Typeof(i) != TYPE_PROGRAM)) {
            if (*p) {
                notify(player, p);
            } else {
                notify(player, tp_description_default);
            }
        } else {
            struct frame *tmpfr;

            /* In this case, we are setting up a MUF to run.
             *
             * Before we run the MUF, we will first parse any MPI on
             * the line and feed it into the MUF as arguments.
             *
             * Then we will run the MUF in PREEMPT.
             *
             * The MUF is responsible for all notifications; this
             * function does not add any additional text.
             */
            strcpyn(tmparg, sizeof(tmparg), match_args);
            strcpyn(tmpcmd, sizeof(tmpcmd), match_cmdname);
            p = do_parse_mesg(descr, player, thing, p, whatcalled, buf,
                              sizeof(buf), MPI_ISPRIVATE | mpiflags);
            strcpyn(match_args, sizeof(match_args), p);
            strcpyn(match_cmdname, sizeof(match_cmdname), whatcalled);
            tmpfr = interp(descr, player, LOCATION(player), i, thing, PREEMPT,
                           STD_HARDUID, 0);

            if (tmpfr) {
                interp_loop(player, i, tmpfr, 0);
            }

            strcpyn(match_args, sizeof(match_args), tmparg);
            strcpyn(match_cmdname, sizeof(match_cmdname), tmpcmd);
        }
    } else {
        /* The default behavior is to parse MPI and return the message to
         * the player.
         */
        p = do_parse_mesg(descr, player, thing, p, whatcalled, buf, sizeof(buf),
                          MPI_ISPRIVATE | mpiflags);
        notify(player, p);
    }
}

/**
 * @see exec_or_notify
 *
 * This does the same thing as exec_or_notify except it does the prop load
 * for you and also handles the MPI flags (handling if a property is blessed
 * or not).
 *
 * If the prop is not set, there is no notification at all.  Any errors are
 * notified to the user.
 *
 * @param descr - integer descriptor to notify.
 * @param player - The DBREF of the calling player.
 * @param thing - The DBREF of the thing emitting the message.
 * @param propname - The property to load off 'thing'.
 * @param whatcalled - This is the caller context -- its the
 *                     MPI &how or the MUF command.  Such as "(@Desc)"
 */
void
exec_or_notify_prop(int descr, dbref player, dbref thing,
                    const char *propname, const char *whatcalled)
{
    const char *message = get_property_class(thing, propname);
    int mpiflags = Prop_Blessed(thing, propname) ? MPI_ISBLESSED : 0;

    if (message)
        exec_or_notify(descr, player, thing, message, whatcalled, mpiflags);
}

/**
 * An "oprop" is something like an @OSuccess or @ODrop ... a message that
 * is prefixed with the player's name.  This loads the property, parses
 * pronouns and MPI, then emits the message to the room the triggering
 * player is in.
 *
 * This is equivalent to exec_or_notify which works for @Success, etc.
 * messages.
 *
 * @param descr The descriptor of the person triggering
 * @param player The player DBREF that triggered the action.
 * @param dest The destination DBREF.
 * @param exit The exit DBREF that triggered the action.
 * @param propname The name of the property to load.
 * @param prefix What will be prefixed to this message before broadcast.
 *        This is pretty much always the player's name.  You do not need
 *        to include a trailing space.
 * @param whatcalled The &how / command verb, such as (@OSucc)
 */
void
parse_oprop(int descr, dbref player, dbref dest, dbref exit, const char *propname,
            const char *prefix, const char *whatcalled)
{
    const char *msg = get_property_class(exit, propname);
    int ival = 0;

    if (Prop_Blessed(exit, propname))
        ival |= MPI_ISBLESSED;

    if (msg) {
        char buf[BUFFER_LEN];
        char *ptr;

        do_parse_mesg(descr, player, exit, msg, whatcalled, buf, sizeof(buf),
                      MPI_ISPUBLIC | ival);
        ptr = pronoun_substitute(descr, player, buf);
        if (!*ptr)
            return;

        prefix_message(buf, ptr, prefix, BUFFER_LEN, 1);
        notify_except(CONTENTS(dest), player, buf, player);
    }
}

/**
 * Check to see if a property name is valid.  Which, at present, means
 * the name does not contain a '\r' or PROP_DELIMITER (':') in it.
 *
 * Note - This function is used by MUF primitives but not by the rest of
 * the property 'library' here.  The preference with the underlying library
 * seems to be to treat the PROP_DELIMITER as a null terminator rather than
 * doing some more proper validation / error handling.
 *
 * This is a rather odd design choice, but perhaps a dangerous one to
 * reverse at this time.  getprop / remove prop don't validate the prop
 * name string at all and just assume setprop won't let something bad
 * get past the goalie.
 *
 * ext-name-ok and other such string checks also check ok_ascii_other;
 * props do no such checking, so theoretically you could get some weird
 * characters in the propname.  Do we care?  I assume not!
 *
 * @param pname The property name to validate
 * @return boolean 1 if valid, 0 if not
 */
int
is_valid_propname(const char* pname)
{
    if (!*pname)
        return 0;

    while (*pname && *pname != '\r' && *pname != PROP_DELIMITER)
        pname++;

    return (*pname) == 0;
}
