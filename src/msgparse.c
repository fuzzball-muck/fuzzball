/** @file msgparse.c
 *
 * Implementation for handling MPI message processing.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "boolexp.h"
#include "db.h"
#include "fbstrings.h"
#include "game.h"
#include "hashtab.h"
#include "interface.h"
#include "match.h"
#include "mfun.h"
#include "mpi.h"
#include "props.h"
#include "tune.h"

/**
 * @var start time for MPI performance profiling
 *
 *      This is NOT threadsafe.
 */
time_t mpi_prof_start_time;

/**
 * Safely bless (or unbless) a property
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * @param obj the object to set bless on
 * @param buf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param set_p if true, set blessed.  If false, unbless.
 * @return boolean true if successful, false if a problem happened.
 */
int
safeblessprop(dbref obj, char *buf, int mesgtyp, int set_p)
{
    if (!buf)
        return 0;

    while (*buf == PROPDIR_DELIMITER)
        buf++;

    if (!*buf)
        return 0;

    if (!is_valid_propname(buf))
        return 0;

    if (!(mesgtyp & MPI_ISBLESSED)) {
        return 0;
    }

    if (set_p) {
        set_property_flags(obj, buf, PROP_BLESSED);
    } else {
        clear_property_flags(obj, buf, PROP_BLESSED);
    }

    return 1;
}

/**
 * Safely store a value to a property, or to clear it
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden, Prop_SeeOnly are true (@see Prop_Hidden  @see Prop_SeeOnly )
 * or if the prop name starts with MPI_MACROS_PROPDIR
 *
 * val, if NULL, will clear the property.
 *
 * @param obj the object to set bless on
 * @param buf the name of the property
 * @param val the value to set, or NULL to clear a property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @return boolean true if successful, false if a problem happened.
 */
int
safeputprop(dbref obj, char *buf, char *val, int mesgtyp)
{
    if (!buf)
        return 0;

    while (*buf == PROPDIR_DELIMITER)
        buf++;

    if (!*buf)
        return 0;

    if (!is_valid_propname(buf))
        return 0;

    /* disallow CR's and :'s in prop names. */
    for (char *ptr = buf; *ptr; ptr++)
        if (*ptr == '\r' || *ptr == PROP_DELIMITER)
            return 0;

    /* Can't mess with system properties */
    if (Prop_System(buf))
        return 0;

    if (!(mesgtyp & MPI_ISBLESSED)) {
        if (Prop_Hidden(buf))
            return 0;

        if (Prop_SeeOnly(buf))
            return 0;

        if (string_prefix(buf, MPI_MACROS_PROPDIR))
            return 0;
    }

    if (val == NULL) {
        remove_property(obj, buf, 0);
    } else {
        add_property(obj, buf, val, 0);
    }

    return 1;
}

/**
 * Safely fetch a property value without searching the entire environment
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * If there is a problem, the player is notified.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden is true, or if Prop_Private is true and the owner of the
 * object doesn't match the owner of what.  @see Prop_Hidden @see Prop_Private
 *
 * @param player the player looking up the prop
 * @param what the object to load the prop off of
 * @param perms the object we are basing permissions off of (trigger)
 * @param inbuf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param blessed whether or not the prop is blessed will be stored here
 * @return NULL if error, contents of property otherwise.
 */
const char *
safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf,
                   int mesgtyp, int *blessed)
{
    const char *ptr;
    char bbuf[BUFFER_LEN];
    static char vl[32];

    *blessed = 0;

    if (!inbuf) {
        notify_nolisten(player, "PropFetch: Propname required.", 1);
        return NULL;
    }

    while (*inbuf == PROPDIR_DELIMITER)
        inbuf++;

    if (!*inbuf) {
        notify_nolisten(player, "PropFetch: Propname required.", 1);
        return NULL;
    }

    strcpyn(bbuf, sizeof(bbuf), inbuf);

    if (Prop_System(bbuf)) {
        notify_nolisten(player, "PropFetch: Permission denied.", 1);
        return NULL;
    }

    if (!(mesgtyp & MPI_ISBLESSED)) {
        if (Prop_Hidden(bbuf)) {
            notify_nolisten(player, "PropFetch: Permission denied.", 1);
            return NULL;
        }

        if (Prop_Private(bbuf) && OWNER(perms) != OWNER(what)) {
            notify_nolisten(player, "PropFetch: Permission denied.", 1);
            return NULL;
        }
    }

    /* Load the property */
    ptr = get_property_class(what, bbuf);

    /*
     * If we didn't get a string, try to load dbref or integer.
     *
     * @TODO: Is this really the best way to do this?  Seems like we should
     *        have a common 'fetch any kind of property as a string' if
     *        we don't already have one.
     */
    if (!ptr) {
        int i;

        i = get_property_value(what, bbuf);

        if (!i) {
            dbref dd;

            dd = get_property_dbref(what, bbuf);

            if (dd == NOTHING) {
                *vl = '\0';
                ptr = vl;
                return ptr;
            } else {
                snprintf(vl, sizeof(vl), "#%d", dd);
                ptr = vl;
            }
        } else {
            snprintf(vl, sizeof(vl), "%d", i);
            ptr = vl;
        }
    }

    if (ptr) {
        if (Prop_Blessed(what, bbuf)) {
            *blessed = 1;
        }
    }

    return ptr;
}

/**
 * Safely fetch a property value, scanning the environment in a limited fashion
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * If there is a problem, the player is notified.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden is true, or if Prop_Private is true and the owner of the
 * object doesn't match the owner of what.  @see Prop_Hidden @see Prop_Private
 *
 * This scans the environment for the prop, but only searches for props on
 * objects that are owned by the owner of 'whom' unless 'blessed' points to
 * an integer containing a true value.
 *
 * @private
 * @param player the player looking up the prop
 * @param what the object to load the prop off of
 * @param perms the object we are basing permissions off of (trigger)
 * @param inbuf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param blessed whether or not the prop is blessed will be stored here
 * @return NULL if error, contents of property otherwise.
 */
static const char *
safegetprop_limited(dbref player, dbref what, dbref whom, dbref perms, const char *inbuf,
                    int mesgtyp, int *blessed)
{
    const char *ptr;

    while (what != NOTHING) {
        ptr = safegetprop_strict(player, what, perms, inbuf, mesgtyp, blessed);

        if (!ptr)
            return ptr;

        if (*ptr) {
            if (OWNER(what) == whom || *blessed) {
                return ptr;
            }
        }

        what = getparent(what);
    }

    return "";
}

/**
 * Safely fetch a property value, scanning the environment
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * If there is a problem, the player is notified.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden is true, or if Prop_Private is true and the owner of the
 * object doesn't match the owner of what.  @see Prop_Hidden @see Prop_Private
 *
 * This scans the environment for the prop as well.
 *
 * @param player the player looking up the prop
 * @param what the object to load the prop off of
 * @param perms the object we are basing permissions off of (trigger)
 * @param inbuf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param blessed whether or not the prop is blessed will be stored here
 * @return NULL if error, contents of property otherwise.
 */
const char *
safegetprop(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp,
            int *blessed)
{
    const char *ptr;

    while (what != NOTHING) {
        ptr = safegetprop_strict(player, what, perms, inbuf, mesgtyp, blessed);

        if (!ptr || *ptr)
            return ptr;

        what = getparent(what);
    }

    return "";
}

/**
 * Get an item from a proplist
 *
 * 'itemnum' should be a number from 1 to the size of the list, but it isn't
 * validated at all and can be out of bounds (it should result in an empty
 * string being returned).
 *
 * The list is searched for in the environment using safegetprop under the
 * hood.  @see safegetprop
 *
 * @param player the player who triggered the action
 * @param what the object to load the prop off of
 * @param perms the object that is the source of permissions
 * @param listname the name of the list without the # mark
 * @param itemnum the line number to fetch from the list
 * @param mesgtyp the mesgtyp from the MPI call
 * @param blessed pointer to integer containing blessed boolean
 * @return string value or empty string if nothing found
 */
const char *
get_list_item(dbref player, dbref what, dbref perms, char *listname,
              int itemnum, int mesgtyp, int *blessed)
{
    char buf[BUFFER_LEN];
    int len = strlen(listname);

    if (len > 0 && listname[len - 1] == NUMBER_TOKEN)
        listname[len - 1] = 0;

    snprintf(buf, sizeof(buf), "%.512s#/%d", listname, itemnum);
    return (safegetprop(player, what, perms, buf, mesgtyp, blessed));
}

/**
 * Get the item count from a proplist
 *
 * This is not as simple as it seems at first blush because it supports
 * multiple list formats.  It supports listname#, listname/#, and if all
 * else fails, it iterates over the whole list up to MAX_MFUN_LIST_LEN
 * stopping when it finds an empty line.
 *
 * The list is searched for in the environment using safegetprop under the
 * hood.  @see safegetprop   When doing an iterative count, it uses
 * @see get_list_item
 *
 * @param player the player who triggered the action
 * @param obj the object to load the prop off of
 * @param perms the object that is the source of permissions
 * @param listname the name of the list without the # mark
 * @param itemnum the line number to fetch from the list
 * @param mesgtyp the mesgtyp from the MPI call
 * @param blessed pointer to integer containing blessed boolean
 * @return string value or empty string if nothing found
 */
int
get_list_count(dbref player, dbref obj, dbref perms, char *listname,
               int mesgtyp, int *blessed)
{
    char buf[BUFFER_LEN];
    const char *ptr;
    int i, len = strlen(listname);

    if (len > 0 && listname[len - 1] == NUMBER_TOKEN)
        listname[len - 1] = 0;

    snprintf(buf, sizeof(buf), "%.512s#", listname);
    ptr = safegetprop(player, obj, perms, buf, mesgtyp, blessed);

    if (ptr && *ptr)
        return (atoi(ptr));

    snprintf(buf, sizeof(buf), "%.512s/#", listname);
    ptr = safegetprop(player, obj, perms, buf, mesgtyp, blessed);

    if (ptr && *ptr)
        return (atoi(ptr));

    for (i = 1; i < MAX_MFUN_LIST_LEN; i++) {
        ptr = get_list_item(player, obj, perms, listname, i, mesgtyp, blessed);

        if (!ptr)
            return 0;

        if (!*ptr)
            break;
    }

    if (i-- < MAX_MFUN_LIST_LEN)
        return i;

    return MAX_MFUN_LIST_LEN;
}

/**
 * Concatinate a list into a delimited string.
 *
 * How the string is concatinated is controlled by 'mode'.  If mode is
 * 0, then the list is delimited by \r characters such as with the {list}
 * macro.
 * 
 * For mode 1, most lines are separated by a single space.  If a line ends in '.', '?',
 * or '!' then a second space is added as well to the delimiter.
 *
 * For mode 2, there is no delimiter.
 *
 * Modes 1 and 2 will remove leading / trailing spaces.
 *
 * Uses get_list_count to get the list count.  @see get_list_count
 *
 * @param what the thing that triggered the action
 * @param perms the object that is the source of the permissions
 * @param obj the object to fetch the list off of
 * @param listname the name of the list prop
 * @param buf the buffer to write to
 * @param maxchars the size of the buffer
 * @param mode the mode which is 0, 1, or 2 as described above.
 * @param mesgtyp the mesgtyp variable from the MPI call
 * @param blessed a pointer to an integer which is a boolean for blessed
 * @return the string result which may be NULL if there was a problem loading
 */
char *
get_concat_list(dbref what, dbref perms, dbref obj, char *listname,
                char *buf, int maxchars, int mode, int mesgtyp, int *blessed)
{
    int line_limit = MAX_MFUN_LIST_LEN;
    int cnt, len;
    const char *ptr;
    char *pos = buf;
    int tmpbless = 0;

    len = strlen(listname);

    if (len > 0 && listname[len - 1] == NUMBER_TOKEN)
        listname[len - 1] = 0;

    /* Get the list size */
    cnt = get_list_count(what, obj, perms, listname, mesgtyp, &tmpbless);

    *blessed = 1;

    if (!tmpbless) {
        *blessed = 0;
    }

    if (cnt == 0) {
        return NULL;
    }

    maxchars -= 2;
    *buf = '\0';

    /* Iterate over the list and assemble the string */
    for (int i = 1; ((pos - buf) < (maxchars - 1)) && i <= cnt && line_limit--;
         i++) {
        ptr = get_list_item(what, obj, perms, listname, i, mesgtyp, &tmpbless);

        if (ptr) {
            if (!tmpbless) {
                *blessed = 0;
            }

            /* Remove leading space for modes 1 and 2 */
            while (mode && isspace(*ptr))
                ptr++;

            if (pos > buf) {
                if (!mode) { /* Mode 0 -- \r delimited */
                    *(pos++) = '\r';
                    *pos = '\0';
                } else if (mode == 1) { /* Mode 1 - space delimited */
                    char ch = *(pos - 1);

                    if ((pos - buf) >= (maxchars - 2))
                        break;

                    if (ch == '.' || ch == '?' || ch == '!')
                        *(pos++) = ' ';

                    *(pos++) = ' ';
                    *pos = '\0';
                } else { /* Mode 2 -- no delimiter */
                    *pos = '\0';
                }
            }

            while (((pos - buf) < (maxchars - 1)) && *ptr)
                *(pos++) = *(ptr++);

            /* Remove trailing spaces */
            if (mode) {
                while (pos > buf && *(pos - 1) == ' ')
                    pos--;
            }

            *pos = '\0';

            if ((pos - buf) >= (maxchars - 1))
                break;
        }
    }

    return (buf);
}

/**
 * Check for read permissions on a given object relative to another object
 *
 * Read permissions are given if the object is 0, the object is the calling
 * player, the object is the 'perms' object.  Or if the owner of the perms
 * object matches the owner of the object, and to make sure the object is
 * not locked. Or, if mesgtyp includes MPI_ISBLESSED
 *
 * @private
 * @param descr the descriptor for the calling player
 * @param player the player calling the MPI
 * @param perms the object to base relative permissions off of
 * @param obj the object we wish to access
 * @param mesgtyp the flags from the MPI call
 * @return boolean true if allowed, false if not
 */
static int
mesg_read_perms(int descr, dbref player, dbref perms, dbref obj, int mesgtyp)
{
    if ((obj == 0) || (obj == player) || (obj == perms))
        return 1;

    if (OWNER(perms) == OWNER(obj))
        return 1;

    if (test_lock_false_default(descr, OWNER(perms), obj, MESGPROP_READLOCK))
        return 1;

    if ((mesgtyp & MPI_ISBLESSED))
        return 1;

    return 0;
}

/**
 * Check if d1 is a neighbor of d2
 *
 * A neighbor is defined as:
 *
 * * d1 is d2
 * * d1 and d2 is not a room and they are in the same room
 * * d1 or d2 is a room and one is inside the other.
 *
 * @param d1 one object to check
 * @param d2 the other object to check
 * @return boolean true if d1 and d2 are neighbors
 */
int
isneighbor(dbref d1, dbref d2)
{
    if (d1 == d2)
        return 1;

    if (Typeof(d1) != TYPE_ROOM)
        if (LOCATION(d1) == d2)
            return 1;

    if (Typeof(d2) != TYPE_ROOM)
        if (LOCATION(d2) == d1)
            return 1;

    if (Typeof(d1) != TYPE_ROOM && Typeof(d2) != TYPE_ROOM)
        if (LOCATION(d1) == LOCATION(d2))
            return 1;

    return 0;
}

/**
 * Checks to see if we have "local permissions" to a given object
 *
 * Local permissions are to see if you can access props on a given
 * object.  It makes sure the object is proximal to the player in
 * some sensible fashion.  perms is used as an object to determine
 * relative permissions.
 *
 * * If 'obj' is owned by the same person as perm, then permission granted
 * * If 'perms' is a neighbor of obj, then permission granted
 * * If 'player' is a neighbor of obj, then permission granted
 * * If the object isn't locked against the player
 * * And finally if mesg_read_perms passes
 *
 * @see isneighbor
 * @see mesg_read_perms
 *
 * @private
 * @param descr the descriptor for the calling player
 * @param player the player calling the MPI
 * @param perms the object to base relative permissions off of
 * @param obj the object we wish to access
 * @param mesgtyp the flags from the MPI call
 * @return boolean true if allowed, false if not
 */
static int
mesg_local_perms(int descr, dbref player, dbref perms, dbref obj, int mesgtyp)
{
    if (LOCATION(obj) != NOTHING && OWNER(perms) == OWNER(LOCATION(obj)))
        return 1;

    if (isneighbor(perms, obj))
        return 1;

    if (isneighbor(player, obj))
        return 1;

    if (LOCATION(obj) != NOTHING
        && test_lock_false_default(descr, OWNER(perms), OWNER(LOCATION(obj)),
                                   MESGPROP_READLOCK))
        return 1;

    if (mesg_read_perms(descr, player, perms, obj, mesgtyp))
        return 1;

    return 0;
}

/**
 * Handle the MPI way of getting a dbref from a string
 *
 * First checks "this", "me", "here", and "home".  Then uses
 * a match relative to the player, and finally does a remote
 * match.
 *
 * @param descr the descriptor of the calling player
 * @param player the player doing the search
 * @param what the object the search is relative to
 * @param buf the string to search for
 * @return either a dbref or UNNOWN if nothing found.
 */
dbref
mesg_dbref_raw(int descr, dbref player, dbref what, const char *buf)
{
    struct match_data md;
    dbref obj = UNKNOWN;

    if (buf && *buf) {
        /*
         * TODO: This is somewhat redudant and I'm not sure why.
         *
         *       Matching "this" is unique to MPI.  However, me/here/home
         *       can be handled by the match library.  Why are we duplicating
         *       this logic?  Not sure.
         */
        if (!strcasecmp(buf, "this")) {
            obj = what;
        } else if (!strcasecmp(buf, "me")) {
            obj = player;
        } else if (!strcasecmp(buf, "here")) {
            obj = LOCATION(player);
        } else if (!strcasecmp(buf, "home")) {
            obj = HOME;
        } else {
            init_match(descr, player, buf, NOTYPE, &md);
            match_absolute(&md);
            match_all_exits(&md);
            match_neighbor(&md);
            match_possession(&md);
            match_registered(&md);
            obj = match_result(&md);

            if (obj == NOTHING) {
                init_match_remote(descr, player, what, buf, NOTYPE, &md);
                match_player(&md);
                match_all_exits(&md);
                match_neighbor(&md);
                match_possession(&md);
                match_registered(&md);
                obj = match_result(&md);
            }
        }
    }

    if (!OkObj(obj))
        obj = UNKNOWN;

    return obj;
}

/**
 * This is a thin wrapper around mesg_dbref_raw that verifies read permissions
 *
 * First matches 'buf' using @see mesg_dbref_raw
 *
 * Then verifies user can read the object using @see mesg_read_perms
 *
 * @param descr the descriptor of the person doing the match
 * @param player the player doing the matching
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param buf the string to match dbref
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return matched ref, UNKNOWN, or PERMDENIED as appropriate
 */
dbref
mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf,
           int mesgtyp)
{
    dbref obj = mesg_dbref_raw(descr, player, what, buf);

    if (obj == UNKNOWN)
        return obj;

    if (!mesg_read_perms(descr, player, perms, obj, mesgtyp)) {
        obj = PERMDENIED;
    }

    return obj;
}

/**
 * Do a strict permissions database string match
 *
 * First, the matching is done with @see mesg_dbref_raw
 *
 * Then, we check to mesgtyp to see if MPI_ISBLESSED is set, or if
 * the result is owned by the same owner as 'perms'.  If neither is true,
 * we will return PERMDENIED
 *
 * @param descr the descriptor of the player running the MPI
 * @param player hte player doing the action
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param buf the string to match dbref
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return matched ref, UNKNOWN, or PERMDENIED as appropriate
 */ 
dbref
mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf,
                  int mesgtyp)
{
    dbref obj = mesg_dbref_raw(descr, player, what, buf);

    if (obj == UNKNOWN)
        return obj;

    if (!(mesgtyp & MPI_ISBLESSED) && OWNER(perms) != OWNER(obj)) {
        obj = PERMDENIED;
    }

    return obj;
}

/**
 * Do a database string match and include a local perms match.
 *
 * This first matches using @see mesg_dbref_raw
 *
 * Then it runs @see mesg_local_perms
 *
 * If the mesg_local_perms call returns false, this will return
 * PERMDENIED
 *
 * @param descr the descriptor of the player running the MPI
 * @param player hte player doing the action
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param buf the string to match dbref
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return matched ref, UNKNOWN, or PERMDENIED as appropriate
 */ 
dbref
mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf,
                 int mesgtyp)
{
    dbref obj = mesg_dbref_raw(descr, player, what, buf);

    if (obj == UNKNOWN)
        return obj;

    if (!mesg_local_perms(descr, player, perms, obj, mesgtyp)) {
        obj = PERMDENIED;
    }

    return obj;
}

/******** MPI Variable handling ********/

/**
 * @private
 * @var our currently loaded MPI variables
 *
 * This is very much not threadsafe as it expects only one person to be
 * running MPI at a given moment.
 */
static struct mpivar varv[MPI_MAX_VARIABLES];

/**
 * @var count of instanced variables
 *
 * This is very much not threadsafe as it expects only one person to be
 * running MPI at a given moment.
 */
int varc = 0;

/**
 * Check to see if adding 'count' more variables would be too many
 *
 * This will return true if 'count' more variables will be more than
 * MPI_MAX_VARIABLES in total considering already allocated variables.
 *
 * @param count the number of variables you may wish to allocate
 * @return boolean true if you should NOT add more variables, false if you can
 */
int
check_mvar_overflow(int count)
{
    return (varc + count) > MPI_MAX_VARIABLES;
}

/**
 * Allocate a new MPI variable if possible
 *
 * This will return 0 on success, 1 if the variable name is too long, or
 * 2 if there are already too many variables allocated.
 *
 * varname is copied, but 'buf' is used as-is.
 *
 * @param varnam the variable name to allocate
 * @param buf the initial value of the variable
 * @return 0 on success, or potentially 1 or 2 on error as described above
 */
int
new_mvar(const char *varname, char *buf)
{
    if (strlen(varname) > MAX_MFUN_NAME_LEN)
        return 1;

    if (varc >= MPI_MAX_VARIABLES)
        return 2;

    strcpyn(varv[varc].name, sizeof(varv[varc].name), varname);
    varv[varc++].buf = buf;
    return 0;
}

/**
 * Fetch the value of an MPI variable by name
 *
 * Returns the value based on name, or NULL if the name does not exist.
 *
 * @param varname the name to look up
 * @return the value set to varname or NULL if varname does not exist
 */
char *
get_mvar(const char *varname)
{
    int i = 0;

    /*
     * @TODO This is really super inefficient.
     *
     *       I would recommend keeping track of the last accessed variable
     *       and checking that before looping ... or moving this to a real
     *       data structure.  The latter would be useful for multithreading
     *       because we really need to change how this works to support
     *       multithread.
     *
     *       This would change how free_top_mvar works though, so I dunno,
     *       maybe more effort than it is worth.
     */
    for (i = varc - 1; i >= 0 && strcasecmp(varname, varv[i].name); i--) ;

    if (i < 0)
        return NULL;

    return varv[i].buf;
}

/**
 * Deallocate the oldest variable on the variable list
 *
 * @return 1 if there are no variables left, 0 if the variable was deallocated
 */
int
free_top_mvar(void)
{
    if (varc < 1)
        return 1;

    varc--;
    return 0;
}

/***** MPI function handling *****/

/**
 * @private
 * @var array to keep track of allocated MPI functions
 *
 * Warning: this is NOT Threadsafe!  This assumes just one person is running
 * MPI at any moment.
 */
static struct mpifunc funcv[MPI_MAX_FUNCTIONS];

/**
 * @private
 * @var count of number of MPI functions allocated
 *
 * Warning: this is also not threadsafe
 */
static int funcc = 0;

/**
 * Allocate a new MPI function with the given name and code
 *
 * This will return 0 on success.  1 if the function name is too long, and
 * 2 if we don't have enough space for another function in the function array.
 *
 * Memory is copied for both funcname and buf.
 *
 * @param funcname the name of the function
 * @param buf the code associated with the name
 * @return integer return status code which may be 0 (success), or 1, or 2
 */
int
new_mfunc(const char *funcname, const char *buf)
{
    if (strlen(funcname) > MAX_MFUN_NAME_LEN)
        return 1;

    if (funcc >= MPI_MAX_FUNCTIONS)
        return 2;

    strcpyn(funcv[funcc].name, sizeof(funcv[funcc].name), funcname);
    funcv[funcc++].buf = strdup(buf);
    return 0;
}

/**
 * Fetch the code associated with the given function name, or NULL if not found
 *
 * @private
 * @param funcname the function name
 * @return either a string with the function code or NULL if function not found
 */
static const char *
get_mfunc(const char *funcname)
{
    int i = 0;

    for (i = funcc - 1; i >= 0 && strcasecmp(funcname, funcv[i].name); i--) ;

    if (i < 0)
        return NULL;

    return funcv[i].buf;
}

/**
 * Delete functions until only 'downto' are left.
 *
 * This deallocates the allocated memory for the code blocks.
 *
 * Returns true if there are no functions to delete (or downto is less than 0)
 *
 * @private
 * @param downto number of functions that should be left
 * @return 0 if there are still functions to delete, 1 if they are all gone
 */
static int
free_mfuncs(int downto)
{
    if (funcc < 1)
        return 1;

    if (downto < 0)
        return 1;

    while (funcc > downto)
        free(funcv[--funcc].buf);

    return 0;
}

/**
 * Returns true if 'name' is an MPI macro
 *
 * This first tries to find a function - @see get_mfunc
 *
 * Then this searches in the MPI_MACROS_PROPDIR on the environment
 * using a series of calls to:
 *
 * @see safegetprop_strict
 * @see safegetprop_limited
 *
 * @private
 * @param player hte player doing the action
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param name the macro to find
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return const char * the macro's value if it exists, otherwise NULL
 */
static const char *
msg_macro_val(dbref player, dbref what, dbref perms, const char *name,
             int mesgtyp)
{
    const char *ptr;
    char buf2[BUFFER_LEN];
    dbref obj;
    int blessed;

    if (!*name)
        return NULL;

    snprintf(buf2, sizeof(buf2), "%s/%s", MPI_MACROS_PROPDIR, name);
    obj = what;
    ptr = get_mfunc(name);

    if (!ptr || !*ptr)
        ptr = safegetprop_strict(player, OWNER(obj), perms, buf2, mesgtyp,
                                 &blessed);

    if (!ptr || !*ptr)
        ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2,
                                  mesgtyp, &blessed);

    if (!ptr || !*ptr)
        ptr = safegetprop_strict(player, 0, perms, buf2, mesgtyp, &blessed);

    if (!ptr || !*ptr)
        return NULL;

    return ptr;
}

/**
 * Unpack a macro into actual MPI code
 *
 * This is a difficult function to understand.  It seems to load the
 * function code and then inject the arguments into it.  Basically a clunky
 * MPI way of implementing a function without a real stack or anything.
 *
 * @private
 * @param player hte player doing the action
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param name the macro to find
 * @param argc the number of arguments
 * @param argv the array of argument strings
 * @param rest we will inject the unparsed function here
 * @param maxchars the maximum size that can be put in 'rest'
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 */
static void
msg_unparse_macro(dbref player, dbref what, dbref perms, char *name, int argc,
                  char **argv, char *rest, int maxchars, int mesgtyp)
{
    const char *ptr;
    char *ptr2;
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    dbref obj;
    int i, p = 0;
    int blessed;

    strcpyn(buf, sizeof(buf), rest);

    ptr = msg_macro_val(player, what, perms, name, mesgtyp);

    while (ptr && *ptr && p < (maxchars - 1)) {
        if (*ptr == '\\') {
           if (*(ptr + 1) == 'r') {
                rest[p++] = '\r';
                ptr++;
                ptr++;
            } else if (*(ptr + 1) == '[') {
                rest[p++] = ESCAPE_CHAR;
                ptr++;
                ptr++;
            } else {
                rest[p++] = *(ptr++);
                rest[p++] = *(ptr++);
            }
        } else if (*ptr == MFUN_LEADCHAR) {
            if (*(ptr + 1) == MFUN_ARGSTART && isdigit(*(ptr + 2)) &&
                *(ptr + 3) == MFUN_ARGEND) {
                ptr++;
                ptr++;
                i = *(ptr++) - '1';
                ptr++;

                if (i >= argc || i < 0) {
                    ptr2 = NULL;
                } else {
                    ptr2 = argv[i];
                }

                while (ptr2 && *ptr2 && p < (maxchars - 1)) {
                    rest[p++] = *(ptr2++);
                }
            } else {
                rest[p++] = *(ptr++);
            }
        } else {
            rest[p++] = *(ptr++);
        }
    }

    ptr2 = buf;

    while (ptr2 && *ptr2 && p < (maxchars - 1)) {
        rest[p++] = *(ptr2++);
    }

    rest[p] = '\0';
}

#ifndef MSGHASHSIZE
#define MSGHASHSIZE 256
#endif

/**
 * @private
 * @var hashtable for mapping function names to their index in the mfun_list
 *      Note that you must subtract 1 from the return value, because '0' is
 *      a table miss but the index of mfun_list starts with 0.
 *
 * Note: this probably IS threadsafe because this is just for MPI built-ins.
 */
static hash_tab msghash[MSGHASHSIZE];

/**
 * Get the index of mfun_list for a given function name
 *
 * NOTE: You must subtract 1 from this number to use it, since 0 is both
 * a valid index and a 'not found' return value.
 *
 * @private
 * @param name the function name to look up
 * @return 0 if not found, otherwise the index
 */
static int
find_mfn(const char *name)
{
    hash_data *exp = find_hash(name, msghash, MSGHASHSIZE);

    if (exp)
        return (exp->ival);

    return (0);
}

/**
 * Insert a function name and index into the function hash
 *
 * NOTE: 'i' cannot be 0 -- so you should add 1 to i before saving it,
 *       since 0 is a valid index.  Whomever uses find_mfn will subtract
 *       1 from the result.  @see find_mfn
 *
 * @private
 * @param name the name of the function you are adding
 * @param i the index to store
 */
static void
insert_mfn(const char *name, int i)
{
    hash_data hd;

    (void) free_hash(name, msghash, MSGHASHSIZE);
    hd.ival = i;
    (void) add_hash(name, hd, msghash, MSGHASHSIZE);
}

/**
 * Purge all the hash entries for MPI functions.
 *
 * This is used to clean up MPI memory.  It should only be used on server
 * shutdown.
 */
void
purge_mfns(void)
{
    kill_hash(msghash, MSGHASHSIZE, 0);
}

/**
 * Initialize the hash table with whatever is in mfun_list
 *
 * This must be run before MPI will work, but only has to be run once as
 * mfun_list doesn't change through the course of the server running.
 */
void
mesg_init(void)
{
    for (int i = 0; mfun_list[i].name; i++)
        insert_mfn(mfun_list[i].name, i + 1);

    mpi_prof_start_time = time(NULL);
}

/**
 * Process MPI arguments into an argument vector (argv) style arrangement
 *
 * This takes an incoming buffer (wbuf) and scans it for arguments taking
 * into account the lead character (ulv), the argument separator character
 * (sep), the argument ending character (dlv), and the character used for
 * quoting (quot).  Finally, it also needs the maximum number of arguments
 * allowed in argv.
 *
 * Any argument that is already in argv for a given position will be free'd
 * and removed before being replaced.  Memory is freshly allocated and the
 * string copied, however how I believe this is supposed to work is the the
 * 'argv' vector is reused repeatedly without cleaning up from call to call,
 * so this call kind of cleans itself up, but there will still be some
 * lingering strings when you're completely done.
 *
 * So, when you are totally done with your argv, you should iterate over it and
 * free the memory; there doesn't seem to be a separate call for this.
 *
 * @private
 * @param wbuf the input string
 * @param maxlen the maximum length of an argument
 * @param argv the argument vector to populate
 * @param ulv the lead character, usually MFUN_LEADCHAR
 * @param sep the separator character, usually MFUN_ARGSEP
 * @param dlv the end character, usually MFUN_ARGEND
 * @param quot the quote character, usually MFUN_LITCHAR
 * @param maxargs the maximum number of slots in argv
 * @return return the "argc" value or count of discovered arguments or -1 on
 *         error.
 */
static int
mesg_args(char *wbuf, size_t maxlen, char **argv, char ulv, char sep, char dlv,
          char quot, int maxargs)
{
    int argc = 0;
    char buf[BUFFER_LEN];
    char *ptr;
    int litflag = 0;

    /* Copy into our working buffer, so that we can edit 'buf' along the way */
    strcpyn(buf, sizeof(buf), wbuf);
    ptr = buf;

    /* Iteratve over the string */
    for (int lev = 0, r = 0; (r < (BUFFER_LEN - 2)); r++) {
        if (buf[r] == '\0') { /* End of string */
            for (int i = 0; i < argc; ++i) {
                free(argv[i]); /*
                                * Clear any args we found .. Not sure we
                                * really need to do this, but sure.
                                */
            }

            return (-1);
        } else if (buf[r] == '\\') { /* Escape, skip next character */
            r++;
            continue;
        } else if (buf[r] == quot) { /* Toggle quote mode */
            litflag = (!litflag);
        } else if (!litflag && buf[r] == ulv) { /* Enter nested MPI call */
            lev++;
            continue;
        } else if (!litflag && buf[r] == dlv) { /* Leave nested MPI call */
            lev--;

            if (lev < 0) { /* We found all the arguments, we're done */
                buf[r] = '\0';
                free(argv[argc]);
                argv[argc] = NULL;
                argv[argc] = malloc((size_t)((buf + r) - ptr) + 1);
                strcpyn(argv[argc++], (size_t)((buf + r) - ptr) + 1, ptr);
                ptr = buf + r + 1;
                break;
            }

            continue;
        } else if (!litflag && lev < 1 && buf[r] == sep) {
            if (argc < maxargs - 1) { /* Found an argument! */
                buf[r] = '\0';
                free(argv[argc]);
                argv[argc] = NULL;
                argv[argc] = malloc((size_t)((buf + r) - ptr) + 1);
                strcpyn(argv[argc++], (size_t)((buf + r) - ptr) + 1, ptr);
                ptr = buf + r + 1;
            }
        }
    }

    buf[BUFFER_LEN - 1] = '\0';
    strcpyn(wbuf, maxlen, ptr);
    return argc;
}

/**
 * @private
 * @var keep track of MPI recursion level.  This is NOT threadsafe
 */
static int mesg_rec_cnt = 0;

/**
 * @private
 * @var keep track of MPI instruction count.  This is NOT threadsafe
 */
static int mesg_instr_cnt = 0;

/**
 * Parse an MPI message
 *
 * MPI is limited to recursing fewer than MPI_RECURSION_LIMIT times.
 * Only BUFFER_LEN of 'inbuf' will be processed even if inbuf is longer
 * than BUFFER_LEN.
 *
 * @param descr the descriptor of the user running the parser
 * @param player the player running the parser
 * @param what the triggering object
 * @param perms the object that dictates the permission
 * @param inbuf the string to process
 * @param outbuf the output buffer
 * @param maxchars the maximum size of the output buffer
 * @param mesgtyp permission bitvector
 * @return NULL on failure, outbuf on success
 */
char *
mesg_parse(int descr, dbref player, dbref what, dbref perms, const char *inbuf,
           char *outbuf, int maxchars, int mesgtyp)
{
    char wbuf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char dbuf[BUFFER_LEN];
    char ebuf[BUFFER_LEN];
    char cmdbuf[MAX_MFUN_NAME_LEN + 2];
    const char *ptr;
    char *dptr;
    int q = 0, s;
    int i;
    char *argv[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    int argc = 0;
    int showtextflag = 0;
    int literalflag = 0;

    mesg_rec_cnt++;

    if (mesg_rec_cnt > MPI_RECURSION_LIMIT) {
        char *zptr = get_mvar("how");
        notifyf_nolisten(player, "%s Recursion limit exceeded.", zptr);
        mesg_rec_cnt--;
        outbuf[0] = '\0';
        return NULL;
    }

    /* Sanity check player */
    if (Typeof(player) == TYPE_GARBAGE) {
        mesg_rec_cnt--;
        outbuf[0] = '\0';
        return NULL;
    }

    /* Sanity check trigger */
    if (Typeof(what) == TYPE_GARBAGE) {
        notify_nolisten(player, "MPI Error: Garbage trigger.", 1);
        mesg_rec_cnt--;
        outbuf[0] = '\0';
        return NULL;
    }

    /*
     * Make a copy of the MPI string to work with, so we don't modify the
     * original.
     */
    strcpyn(wbuf, sizeof(wbuf), inbuf);

    /* Iterate over the string and process it */
    for (int p = 0; wbuf[p] && (p < maxchars - 1) && q < (maxchars - 1); p++) {
        if (wbuf[p] == '\\') {  /* Escape character */
            p++;
            showtextflag = 1;

            if (wbuf[p] == 'r') {
                outbuf[q++] = '\r';
            } else if (wbuf[p] == '[') {
                outbuf[q++] = ESCAPE_CHAR;
            } else {
                outbuf[q++] = wbuf[p];
            }
        } else if (wbuf[p] == MFUN_LITCHAR) { /* Toggle literalness */
            literalflag = (!literalflag);
        } else if (!literalflag && wbuf[p] == MFUN_LEADCHAR) {
            /* Are we entering a function? */
            if (wbuf[p + 1] == MFUN_LEADCHAR) {
                /* This is an escaped entry brace */
                showtextflag = 1;
                outbuf[q++] = wbuf[p++];
            } else {
                /* Figure out what function and arguments */
                ptr = wbuf + (++p);
                s = 0;

                /* Advance these indexes past the function name */
                while (wbuf[p] && wbuf[p] != MFUN_LEADCHAR &&
                       !isspace(wbuf[p]) && wbuf[p] != MFUN_ARGSTART &&
                       wbuf[p] != MFUN_ARGEND && s <= MAX_MFUN_NAME_LEN) {
                    p++;
                    s++;
                }

                /* If 's' is a valid function name, let's process it */
                if ( ( s <= MAX_MFUN_NAME_LEN || 
                      ( s <= MAX_MFUN_NAME_LEN + 1 && *ptr == '&' ) ) &&
                    (wbuf[p] == MFUN_ARGSTART || wbuf[p] == MFUN_ARGEND)) {
                    int varflag;

                    strncpy(cmdbuf, ptr, s);
                    cmdbuf[s] = '\0';

                    varflag = 0;

                    if (*cmdbuf == '&') { /* Functions start with & */
                        s = find_mfn("sublist");
                        varflag = 1;
                    } else if (*cmdbuf) { /* Try to look up function name */
                        s = find_mfn(cmdbuf);
                    } else {
                        s = 0; /* No function name */
                    }

                    if (s) { /* If we found a function, process arguments */
                        s--;

                        /*
                         * Clear out old arguments if argv already has
                         * stuff in it.
                         *
                         * This argv stuff is handled really awkwardly,
                         * and when we load args we will also check for and
                         * free already populated items in argv ... so things
                         * are kinda redundant but kinda not.
                         */
                        for (i = 0; i < argc; i++) {
                            free(argv[i]);
                            argv[i] = NULL;
                        }

                        /* Have we run out of instructions? */
                        if (++mesg_instr_cnt > tp_mpi_max_commands) {
                            char *zptr = get_mvar("how");

                            notifyf_nolisten(player,
                                             "%s %c%s%c: Instruction limit "
                                             "exceeded.", zptr,
                                             MFUN_LEADCHAR,
                                             (varflag ? cmdbuf :
                                                        mfun_list[s].name),
                                             MFUN_ARGEND);
                            mesg_rec_cnt--;
                            outbuf[0] = '\0';
                            return NULL;
                        }

                        if (wbuf[p] == MFUN_ARGEND) { /* No arguments */
                            argc = 0;
                        } else { /* Otherwise, add up argv / argc */
                            argc = mfun_list[s].maxargs;

                            /*
                             * So if the maxargs is negative, that seems
                             * to be a hard limit on the number of parameters.
                             * However, if maxargs is positive, it seems like
                             * it is ignored.  We'll throw an error further
                             * down.
                             */
                            if (argc < 0) {
                                argc = mesg_args((wbuf + p + 1),
                                                 ((sizeof(wbuf) - 
                                                  (size_t)p) - 1),
                                                 &argv[(varflag ? 1 : 0)],
                                                 MFUN_LEADCHAR, MFUN_ARGSEP,
                                                 MFUN_ARGEND, MFUN_LITCHAR,
                                                 (-argc) + (varflag ? 1 : 0));
                            } else {
                                argc = mesg_args((wbuf + p + 1),
                                                 ((sizeof(wbuf) - 
                                                  (size_t)p) - 1),
                                                 &argv[(varflag ? 1 : 0)],
                                                 MFUN_LEADCHAR, MFUN_ARGSEP,
                                                 MFUN_ARGEND, MFUN_LITCHAR,
                                                 (varflag ? 8 : 9));
                            }

                            /* We have an error -- no end brace found */
                            if (argc == -1) {
                                char *zptr = get_mvar("how");

                                notifyf_nolisten(player,
                                                 "%s %c%s%c: End brace not "
                                                 "found.", zptr,
                                                 MFUN_LEADCHAR, cmdbuf,
                                                 MFUN_ARGEND);

                                for (i = 0; i < argc; i++) {
                                    free(argv[i + (varflag ? 1 : 0)]);
                                }

                                mesg_rec_cnt--;
                                outbuf[0] = '\0';

                                return NULL;
                            }
                        }

                        /*
                         * If varflag is set, we started with & which is
                         * the variable accessor.
                         */
                        if (varflag) {
                            char *zptr;

                            zptr = get_mvar(cmdbuf + 1);
                            free(argv[0]);
                            argv[0] = NULL;

                            /* Couldn't find the variable */
                            if (!zptr) {
                                zptr = get_mvar("how");
                                notifyf_nolisten(player,
                                                 "%s %c%s%c: Unrecognized "
                                                 "variable.", zptr,
                                                 MFUN_LEADCHAR, cmdbuf,
                                                 MFUN_ARGEND);

                                for (i = 0; i < argc; i++) {
                                    free(argv[i + (varflag ? 1 : 0)]);
                                }

                                mesg_rec_cnt--;
                                outbuf[0] = '\0';
                                return NULL;
                            }

                            argv[0] = strdup(zptr);
                            argc++;
                        }

                        /* Debug flagged? */
                        if (mesgtyp & MPI_ISDEBUG) {
                            char *zptr = get_mvar("how");

                            /* make a message for debugging */
                            snprintf(dbuf, sizeof(dbuf), "%s %*s%c%s%c", zptr,
                                     (mesg_rec_cnt * 2 - 4), "", MFUN_LEADCHAR,
                                     (varflag ? cmdbuf : mfun_list[s].name),
                                      MFUN_ARGSTART);

                            /* Appending all the items to the debug messages */
                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                if (i) {
                                    const char tbuf[] = { MFUN_ARGSEP, '\0' };
                                    strcatn(dbuf, sizeof(dbuf), tbuf);
                                }

                                cr2slash(ebuf, sizeof(ebuf) / 8, argv[i]);
                                strcatn(dbuf, sizeof(dbuf), "`");
                                strcatn(dbuf, sizeof(dbuf), ebuf);

                                if (strlen(ebuf) >= (sizeof(ebuf) / 8) - 2) {
                                    strcatn(dbuf, sizeof(dbuf), "...");
                                }

                                strcatn(dbuf, sizeof(dbuf), "`");
                            }
                            {
                                const char tbuf[] = { MFUN_ARGEND, '\0' };
                                strcatn(dbuf, sizeof(dbuf), tbuf);
                            }

                            notify_nolisten(player, dbuf, 1);
                        }

                        /*
                         * Strip spaces off arguments if the function requires
                         * it.
                         */
                        if (mfun_list[s].stripp) {
                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                strcpyn(buf, sizeof(buf), stripspaces(argv[i]));
                                strcpyn(argv[i], strlen(buf) + 1, buf);
                            }
                        }

                        /*
                         * Parse the arguments if we are asked to do so
                         * by the function.
                         */
                        if (mfun_list[s].parsep) {
                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                ptr = MesgParse(argv[i], buf, sizeof(buf));

                                if (!ptr) {
                                    char *zptr = get_mvar("how");

                                    notifyf_nolisten(player,
                                                     "%s %c%s%c (arg %d)", zptr,
                                                     MFUN_LEADCHAR,
                                                     (varflag ? cmdbuf :
                                                      mfun_list[s].name),
                                                     MFUN_ARGEND, i + 1);

                                    for (i = 0; i < argc; i++) {
                                        free(argv[i]);
                                    }

                                    mesg_rec_cnt--;
                                    outbuf[0] = '\0';
                                    return NULL;
                                }

                                argv[i] = realloc(argv[i], strlen(buf) + 1);
                                strcpyn(argv[i], strlen(buf) + 1, buf);
                            }
                        }

                        /* Are we debugging?  If so, output */
                        if (mesgtyp & MPI_ISDEBUG) {
                            char *zptr = get_mvar("how");

                            snprintf(dbuf, sizeof(dbuf), "%.512s %*s%c%.512s%c",
                                     zptr,
                                     (mesg_rec_cnt * 2 - 4), "", MFUN_LEADCHAR,
                                     (varflag ? cmdbuf : mfun_list[s].name),
                                     MFUN_ARGSTART);

                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                if (i) {
                                    const char tbuf[] = { MFUN_ARGSEP, '\0' };
                                    strcatn(dbuf, sizeof(dbuf), tbuf);
                                }

                                cr2slash(ebuf, sizeof(ebuf) / 8, argv[i]);
                                strcatn(dbuf, sizeof(dbuf), "`");
                                strcatn(dbuf, sizeof(dbuf), ebuf);

                                if (strlen(ebuf) >= (sizeof(ebuf) / 8) - 2) {
                                    strcatn(dbuf, sizeof(dbuf), "...");
                                }

                                strcatn(dbuf, sizeof(dbuf), "`");
                            }
                            {
                                const char tbuf[] = { MFUN_ARGEND, '\0' };
                                strcatn(dbuf, sizeof(dbuf), tbuf);
                            }
                        }

                        /* Too few arguments? */
                        if (argc < mfun_list[s].minargs) {
                            char *zptr = get_mvar("how");

                            notifyf_nolisten(player,
                                             "%s %c%s%c: Too few arguments.",
                                             zptr, MFUN_LEADCHAR,
                                             (varflag ? cmdbuf :
                                              mfun_list[s].name), MFUN_ARGEND);

                            for (i = 0; i < argc; i++) {
                                free(argv[i]);
                            }

                            mesg_rec_cnt--;
                            outbuf[0] = '\0';
                            return NULL;
                        } else if (mfun_list[s].maxargs > 0 && argc > mfun_list[s].maxargs) {
                            /* Too many arguments */
                            char *zptr = get_mvar("how");

                            notifyf_nolisten(player,
                                             "%s %c%s%c: Too many arguments.",
                                             zptr, MFUN_LEADCHAR,
                                             (varflag ? cmdbuf :
                                              mfun_list[s].name), MFUN_ARGEND);

                            for (i = 0; i < argc; i++) {
                                free(argv[i]);
                            }

                            mesg_rec_cnt--;
                            outbuf[0] = '\0';
                            return NULL;
                        } else {
                            /* Good to go! */
                            ptr = mfun_list[s].mfn(descr, player, what, perms,
                                                   argc, argv, buf, sizeof(buf),
                                                   mesgtyp);

                            if (!ptr) {
                                outbuf[q] = '\0';

                                for (i = 0; i < argc; i++) {
                                    free(argv[i]);
                                }

                                mesg_rec_cnt--;
                                outbuf[0] = '\0';
                                return NULL;
                            }

                            /* Parse the output, if requested by function */
                            if (mfun_list[s].postp) {
                                dptr = MesgParse(ptr, buf, sizeof(buf));

                                if (!dptr) {
                                    char *zptr = get_mvar("how");

                                    notifyf_nolisten(player,
                                                     "%s %c%s%c (returned "
                                                     "string)", zptr,
                                                     MFUN_LEADCHAR,
                                                     (varflag ? cmdbuf :
                                                      mfun_list[s].name),
                                                      MFUN_ARGEND);

                                    for (i = 0; i < argc; i++) {
                                        free(argv[i]);
                                    }

                                    mesg_rec_cnt--;
                                    outbuf[0] = '\0';
                                    return NULL;
                                }

                                ptr = dptr;
                            }
                        }

                        /* Print debug output, if we are debugging */
                        if (mesgtyp & MPI_ISDEBUG) {
                            strcatn(dbuf, sizeof(dbuf), " = `");
                            cr2slash(ebuf, sizeof(ebuf) / 8, ptr);
                            strcatn(dbuf, sizeof(dbuf), ebuf);

                            if (strlen(ebuf) >= (sizeof(ebuf) / 8) - 2) {
                                strcatn(dbuf, sizeof(dbuf), "...");
                            }

                            strcatn(dbuf, sizeof(dbuf), "`");
                            notify_nolisten(player, dbuf, 1);
                        }
                    } else if (msg_macro_val(player, what, perms, cmdbuf,
                                            mesgtyp)) {
                        /* Is it a macro?  If so, let's run it */
                        for (i = 0; i < argc; i++) {
                            free(argv[i]);
                            argv[i] = NULL;
                        }

                        if (wbuf[p] == MFUN_ARGEND) {
                            argc = 0;
                            p++;
                        } else {
                            p++;
                            argc = mesg_args(wbuf + p,
                                             sizeof(wbuf) - (size_t)p, argv,
                                             MFUN_LEADCHAR, MFUN_ARGSEP,
                                             MFUN_ARGEND, MFUN_LITCHAR, 9);

                            if (argc == -1) {
                                char *zptr = get_mvar("how");

                                notifyf_nolisten(player,
                                                 "%s %c%s%c: End brace not "
                                                 "found.", zptr,
                                                 MFUN_LEADCHAR, cmdbuf,
                                                 MFUN_ARGEND);

                                for (i = 0; i < argc; i++) {
                                    free(argv[i]);
                                }

                                mesg_rec_cnt--;
                                outbuf[0] = '\0';
                                return NULL;
                            }
                        }

                        msg_unparse_macro(player, what, perms, cmdbuf, argc,
                                          argv, (wbuf + p), (BUFFER_LEN - p),
                                          mesgtyp);
                        p--;
                        ptr = NULL;
                    } else {
                        /* unknown function */
                        char *zptr = get_mvar("how");

                        notifyf_nolisten(player,
                                         "%s %c%s%c: Unrecognized function.",
                                         zptr, MFUN_LEADCHAR, cmdbuf,
                                         MFUN_ARGEND);

                        for (i = 0; i < argc; i++) {
                            free(argv[i]);
                        }

                        mesg_rec_cnt--;
                        outbuf[0] = '\0';
                        return NULL;
                    }
                } else {
                    showtextflag = 1;
                    ptr--;
                    i = s + 1;

                    while (ptr && *ptr && i-- && q < (maxchars - 1)) {
                        outbuf[q++] = *(ptr++);
                    }

                    outbuf[q] = '\0';
                    p = (int) (ptr - wbuf) - 1;
                    ptr = "";   /* unknown substitution type */
                }

                while (ptr && *ptr && q < (maxchars - 1)) {
                    outbuf[q++] = *(ptr++);
                }
            }
        } else {
            outbuf[q++] = wbuf[p];
            showtextflag = 1;
        }
    }

    outbuf[q] = '\0';

    if ((mesgtyp & MPI_ISDEBUG) && showtextflag) {
        char *zptr = get_mvar("how");

        notifyf_nolisten(player, "%s %*s`%.512s`", zptr,
                         (mesg_rec_cnt * 2 - 4), "",
                         cr2slash(buf2, sizeof(buf2), outbuf));
    }

    for (i = 0; i < argc; i++) {
        free(argv[i]);
    }

    mesg_rec_cnt--;
    outbuf[maxchars - 1] = '\0';

    return (outbuf);
}

/**
 * The guts of do_parse_mesg, which does not include stat accounting items
 *
 * @see do_parse_mesg
 *
 * There is absolutely no reason for this to be its own function other
 * than to keep do_parse_mesg trim.  I guess there's a slight stack
 * performance increase for the rare case where tp_do_mpi_parsing is
 * false in exchange for a slight stack performance decrease for the
 * more common case.
 *
 * One could argue that adding this code to do_parse_mesg would make
 * do_parse_mesg too long, but one could also argue splitting this up
 * just adds an obfuscation / complication that doesn't do much.
 *
 * Because do_parse_mesg is the public facing function, all relevant
 * commentary will be put there rather than burried here.
 *
 * @TODO: Consider moving this code into do_parse_mesg
 *
 * @private
 * @param descr the descriptor of the calling player
 * @param player the player doing the MPI parsing call
 * @param what the object that triggered the MPI parse call
 * @param perms the object from which permissions are sourced
 * @param inbuf the input string to process
 * @param abuf identifier string for error messages
 * @param outbuf the buffer to store output results
 * @param outbuflen the length of outbuf
 * @param mesgtyp the bitvector of permissions
 * @return a pointer to outbuf
 */
static char *
do_parse_mesg_2(int descr, dbref player, dbref what, dbref perms,
                const char *inbuf, const char *abuf, char *outbuf,
                size_t outbuflen, int mesgtyp)
{

    char howvar[BUFFER_LEN];
    char cmdvar[BUFFER_LEN];
    char argvar[BUFFER_LEN];
    char tmparg[BUFFER_LEN];
    char tmpcmd[BUFFER_LEN];
    char *dptr;
    int mvarcnt = varc;
    int mfunccnt = funcc;
    int tmprec_cnt = mesg_rec_cnt;
    int tmpinst_cnt = mesg_instr_cnt;

    *outbuf = '\0';

    /* Can't allocate 'how' and HOW is required */
    if ((mesgtyp & MPI_NOHOW) == 0) {
        if (new_mvar("how", howvar)) {
            notifyf_nolisten(player, "%s Out of MPI variables.", abuf);
            varc = mvarcnt;
            return outbuf;
        }

        strcpyn(howvar, sizeof(howvar), abuf);
    }

    /* Can't allocate cmd */
    if (new_mvar("cmd", cmdvar)) {
        notifyf_nolisten(player, "%s Out of MPI variables.", abuf);
        varc = mvarcnt;
        return outbuf;
    }

    strcpyn(cmdvar, sizeof(cmdvar), match_cmdname);
    strcpyn(tmpcmd, sizeof(tmpcmd), match_cmdname);

    /* Can't allocate arg */
    if (new_mvar("arg", argvar)) {
        notifyf_nolisten(player, "%s Out of MPI variables.", abuf);
        varc = mvarcnt;
        return outbuf;
    }

    strcpyn(argvar, sizeof(argvar), match_args);
    strcpyn(tmparg, sizeof(tmparg), match_args);

    /* Run the parser */
    dptr = MesgParse(inbuf, outbuf, outbuflen);

    if (!dptr) {
        *outbuf = '\0';
    }

    /* Clean up */
    varc = mvarcnt;
    free_mfuncs(mfunccnt);
    mesg_rec_cnt = tmprec_cnt;
    mesg_instr_cnt = tmpinst_cnt;

    strcpyn(match_cmdname, sizeof(match_cmdname), tmpcmd);
    strcpyn(match_args, sizeof(match_args), tmparg);

    return outbuf;
}

/**
 * Process an MPI message, handling variable allocations and cleanup
 *
 * The importance of this function is that it allocates certain standard
 * MPI variables: how (if MPI_NOHOW is not in mesgtyp), cmd, and arg
 *
 * Then it runs MesgParse to process the message after those variables
 * are set up.  And finally, it does cleanup.
 *
 * Statistics are kept as part of this call, to keep track of how much
 * time MPI is taking up to run.  This updates all the profile numbers.
 *
 * @param descr the descriptor of the calling player
 * @param player the player doing the MPI parsing call
 * @param what the object that triggered the MPI parse call
 * @param inbuf the input string to process
 * @param abuf identifier string for error messages
 * @param outbuf the buffer to store output results
 * @param outbuflen the length of outbuf
 * @param mesgtyp the bitvector of permissions
 * @return a pointer to outbuf
 */
char *
do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf,
              const char *abuf, char *outbuf, int outbuflen, int mesgtyp)
{
    if (tp_do_mpi_parsing) {
        char *tmp = NULL;
        struct timeval st, et;

        /* Quickie additions to do rough per-object MPI profiling */
        gettimeofday(&st, NULL);
        tmp = do_parse_mesg_2(descr, player, what, what, inbuf, abuf, outbuf,
                              (size_t)outbuflen, mesgtyp);
        gettimeofday(&et, NULL);

        if (strcmp(tmp, inbuf)) {
            if (st.tv_usec > et.tv_usec) {
                et.tv_usec += 1000000;
                et.tv_sec -= 1;
            }

            et.tv_usec -= st.tv_usec;
            et.tv_sec -= st.tv_sec;
            DBFETCH(what)->mpi_proftime.tv_sec += et.tv_sec;
            DBFETCH(what)->mpi_proftime.tv_usec += et.tv_usec;

            if (DBFETCH(what)->mpi_proftime.tv_usec >= 1000000) {
                DBFETCH(what)->mpi_proftime.tv_usec -= 1000000;
                DBFETCH(what)->mpi_proftime.tv_sec += 1;
            }

            DBFETCH(what)->mpi_prof_use++;
        }

        return (tmp);
    } else {
        strcpyn(outbuf, (size_t)outbuflen, inbuf);
    }

    return outbuf;
}

/**
 * This is a wrapper around do_parse_mesg to automate a prop load
 *
 * @see do_parse_mesg
 *
 * This loads the prop 'propname' and processes MPI contained in it.
 * It will set the MPI_ISBLESSED flag if Prop_Blessed returns true.
 *
 * @see Prop_Blessed
 *
 * @param descr the descriptor of the calling player
 * @param player the player doing the MPI parsing call
 * @param what the triggering object, and also where we load the prop from
 * @param propname the name of the property to load
 * @param abuf identifier string for error messages
 * @param outbuf the buffer to store output results
 * @param outbuflen the length of outbuf
 * @param mesgtyp the bitvector of permissions
 * @return a pointer to outbuf
 */
char *
do_parse_prop(int descr, dbref player, dbref what, const char *propname,
              const char *abuf, char *outbuf, int outbuflen, int mesgtyp)
{
    const char *propval = get_property_class(what, propname);

    if (!propval)
        return NULL;

    if (Prop_Blessed(what, propname))
        mesgtyp |= MPI_ISBLESSED;

    return do_parse_mesg(descr, player, what, propval, abuf, outbuf, (size_t)outbuflen, mesgtyp);
}
