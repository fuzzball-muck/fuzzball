/** @file create.c
 *
 * Source defining primarily the creation commands (@action, etc.)
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "edit.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "game.h"
#include "log.h"
#include "move.h"
#include "interface.h"
#include "match.h"
#include "predicates.h"
#include "player.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

/**
 * Implementation of @open command
 *
 * This creates an exit in the current room and optionally links it
 * in one step.
 *
 * Supports registering if linkto has a = in it.  Exits are checked so
 * that they are only 7-bit ASCII.  Does permission and money checking,
 * but not builder bit checking.
 *
 * @param descr descriptor of person running command
 * @param player the player running the command
 * @param direction the exit name to make
 * @param linkto what to link to
 */
void
do_open(int descr, dbref player, const char *direction, const char *linkto)
{
    dbref loc, exit;
    dbref good_dest[MAX_LINKS];
    char buf2[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    char *qname, *rname;
    int ndest;

    strcpyn(buf2, sizeof(buf2), linkto);

    for (rname = buf2; (*rname && (*rname != ARG_DELIMITER)); rname++) ;

    qname = rname;

    if (*rname)
        rname++;

    *qname = '\0';

    qname = buf2;
    remove_ending_whitespace(&qname);

    skip_whitespace((const char **)&rname);

    loc = LOCATION(player);

    if (!ok_object_name(direction, TYPE_EXIT)) {
        notify(player, "Please specify a valid name for this exit.");
        return;
    }

    if (!controls(player, loc)) {
        notify(player, "Permission denied. (you don't control the location)");
        return;
    } else if (!payfor(player, tp_exit_cost)) {
        notifyf(player, "Sorry, you don't have enough %s to open an exit.", tp_pennies);
        return;
    } else {
        exit = create_action(player, direction, loc);
        unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "Exit %s opened.", unparse_buf);

        /* check second arg to see if we should do a link */
        if (*qname != '\0') {
            notify(player, "Trying to link...");

            if (!payfor(player, tp_link_cost)) {
                notifyf(player, "You don't have enough %s to link.", tp_pennies);
            } else {
                ndest = link_exit(descr, player, exit, (char *) qname, good_dest);
                DBFETCH(exit)->sp.exit.ndest = ndest;
                DBFETCH(exit)->sp.exit.dest = malloc(sizeof(dbref) * (size_t)ndest);

                for (int i = 0; i < ndest; i++) {
                    (DBFETCH(exit)->sp.exit.dest)[i] = good_dest[i];
                }

                DBDIRTY(exit);
            }
        }
    }

    if (*rname) {
        register_object(player, player, REGISTRATION_PROPDIR, rname, exit);
    }
}

/**
 * Implementation of @link command.
 *
 * Use this to link to a room that you own.  It also sets home for objects and
 * things, and drop-to's for rooms.  It seizes ownership of an unlinked exit,
 * and costs 1 penny plus a penny transferred to the exit owner if they aren't
 * you
 *
 * All destinations must either be owned by you, or be LINK_OK.  This does
 * check all permissions and, in fact, even the BUILDER bit.
 *
 * @param descr the descriptor of the calling user
 * @param player the calling player
 * @param thing_name the thing to link
 * @param dest_name the destination to link to.
 */
void
do_link(int descr, dbref player, const char *thing_name, const char *dest_name)
{
    dbref thing;
    dbref dest;
    dbref good_dest[MAX_LINKS];
    struct match_data md;

    int ndest;

    init_match(descr, player, thing_name, TYPE_EXIT, &md);
    match_everything(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING)
        return;

    if (Typeof(thing) != TYPE_EXIT && strchr(dest_name, EXIT_DELIMITER)) {
        notify(player, "Only actions and exits can be linked to multiple destinations.");
        return;
    }

    switch (Typeof(thing)) {
        case TYPE_EXIT:
            /* we're ok, check the usual stuff */
            if (DBFETCH(thing)->sp.exit.ndest != 0) {
                if (controls(player, thing)) {
                    if ((DBFETCH(thing)->sp.exit.dest)[0] != NIL) {
                        notify(player, "That exit is already linked.");
                        return;
                    }
                } else {
                    notify(player, "Permission denied. (you don't control the exit to relink)");
                    return;
                }
            }

            /* handle costs */
            if (OWNER(thing) == OWNER(player)) {
                if (!payfor(player, tp_link_cost)) {
                    notifyf(player, "It costs %d %s to link this exit.",
                            tp_link_cost, (tp_link_cost == 1) ? tp_penny : tp_pennies);
                    return;
                }
            } else {
                if (!Builder(player)) {
                    notify(player, "Only authorized builders may seize exits.");
                    return;
                }
                if (!payfor(player, tp_link_cost + tp_exit_cost)) {
                    notifyf(player, "It costs %d %s to link this exit.",
                            (tp_link_cost + tp_exit_cost),
                            (tp_link_cost + tp_exit_cost == 1) ? tp_penny : tp_pennies);
                    return;
                }
                /* pay the owner for his loss */
                dbref owner = OWNER(thing);

                SETVALUE(owner, GETVALUE(owner) + tp_exit_cost);
                DBDIRTY(owner);

                notifyf_nolisten(player, "Claiming unlinked exits: %s", DEPRECATED_FEATURE);
            }

            /* link has been validated and paid for; do it */
            OWNER(thing) = OWNER(player);
            ndest = link_exit(descr, player, thing, (char *) dest_name, good_dest);

            if (ndest == 0) {
                notify(player, "No destinations linked.");
                if(!Wizard(OWNER(thing)))
                    SETVALUE(player, GETVALUE(player) + tp_link_cost);
                DBDIRTY(player);
                break;
            }

            DBFETCH(thing)->sp.exit.ndest = ndest;

            free(DBFETCH(thing)->sp.exit.dest);
            DBFETCH(thing)->sp.exit.dest = malloc(sizeof(dbref) * (size_t)ndest);

            for (int i = 0; i < ndest; i++) {
                (DBFETCH(thing)->sp.exit.dest)[i] = good_dest[i];
            }

            break;
        case TYPE_THING:
        case TYPE_PLAYER:
            init_match(descr, player, dest_name, TYPE_ROOM, &md);
            match_neighbor(&md);
            match_absolute(&md);
            match_registered(&md);
            match_me(&md);
            match_here(&md);

            if (Typeof(thing) == TYPE_THING)
                match_possession(&md);

            if ((dest = noisy_match_result(&md)) == NOTHING)
                return;

            if (!controls(player, thing)
                || !can_link_to(player, Typeof(thing), dest)) {
                notify(player,
                    "Permission denied. (you don't control the thing, or you can't link to dest)");
                return;
            }

            if (parent_loop_check(thing, dest)) {
                notify(player, "That would cause a parent paradox.");
                return;
            }

            /* do the link */
            if (Typeof(thing) == TYPE_THING) {
                THING_SET_HOME(thing, dest);
            } else {
                PLAYER_SET_HOME(thing, dest);
            }

            notify(player, "Home set.");
            break;
        case TYPE_ROOM:     /* room dropto's */
            init_match(descr, player, dest_name, TYPE_ROOM, &md);
            match_neighbor(&md);
            match_possession(&md);
            match_registered(&md);
            match_absolute(&md);
            match_home(&md);

            if ((dest = noisy_match_result(&md)) == NOTHING)
                break;

            if (!controls(player, thing) || !can_link_to(player, Typeof(thing), dest)
                || (thing == dest)) {
                notify(player,
                    "Permission denied. (you don't control the room, or can't link to the dropto)");
            } else {
                DBFETCH(thing)->sp.room.dropto = dest;  /* dropto */
                notify(player, "Dropto set.");
            }

            break;
        case TYPE_PROGRAM:
            notify(player, "You can't link programs to things!");
            break;
        default:
            notify(player, "Internal error: weird object type.");
            log_status("PANIC: weird object: Typeof(%d) = %d", thing, Typeof(thing));
            break;
    }

    DBDIRTY(thing);
    return;
}

/**
 * Implementation of @dig
 *
 * Use this to create a room.  This does a lot of checks; rooms must pass
 * ok_object_name.  It handles finding the parent, if no parent is
 * provided.  Otherwise, it tries to link the parent and does appropriate
 * permission checks.
 *
 * This does NOT check builder bit.
 *
 * It supports registrtration if pname has an = in it.
 *
 * @param descr the descriptor of the person making the call
 * @param player the player doing the call
 * @param name the name of the room to make
 * @param pname the parent room and possibly registration info
 */
void
do_dig(int descr, dbref player, const char *name, const char *pname)
{
    dbref room;
    dbref parent;
    dbref newparent;
    char buf[BUFFER_LEN];
    char rbuf[BUFFER_LEN];
    char qbuf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    char *rname;
    char *qname;
    struct match_data md;

    if (!ok_object_name(name, TYPE_ROOM)) {
        notify(player, "Please specify a valid name for this room.");
        return;
    }

    if (!payfor(player, tp_room_cost)) {
        notifyf(player, "Sorry, you don't have enough %s to dig a room.", tp_pennies);
        return;
    }

    newparent = LOCATION(LOCATION(player));

    /**
     * @TODO: Okay, so, this isn't the right place to put this TODO,
     *        but I don't want to forget it.  This logic should be
     *        put in the newroom MUF.  Maybe if you indicate a parent room
     *        of #-1 it will use this logic?  An *extremely* common problem
     *        is people who make @dig replacements, editrooms, and other such
     *        tools screw up this logic.
     */
    while ((newparent != NOTHING) && !(FLAGS(newparent) & ABODE))
        newparent = LOCATION(newparent);

    if (newparent == NOTHING)
        newparent = tp_default_room_parent;

    room = create_room(player, name, newparent);
    unparse_object(player, room, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Room %s created.", unparse_buf);

    strcpyn(buf, sizeof(buf), pname);

    for (rname = buf; (*rname && (*rname != ARG_DELIMITER)); rname++) ;

    qname = rname;

    if (*rname)
        *(rname++) = '\0';

    qname = buf;
    remove_ending_whitespace(&qname);

    skip_whitespace((const char **)&rname);
    rname = strcpyn(rbuf, sizeof(rbuf), rname);
    qname = strcpyn(qbuf, sizeof(qbuf), qname);

    if (*qname) {
        notify(player, "Trying to set parent...");
        init_match(descr, player, qname, TYPE_ROOM, &md);
        match_absolute(&md);
        match_registered(&md);
        match_here(&md);

        if ((parent = noisy_match_result(&md)) == NOTHING) {
            notify(player, "Parent set to default.");
        } else {
            if (!can_link_to(player, Typeof(room), parent) || room == parent) {
                notify(player, "Permission denied.  Parent set to default.");
            } else {
                moveto(room, parent);
                unparse_object(player, parent, unparse_buf, sizeof(unparse_buf));
                notifyf(player, "Parent set to %s.", unparse_buf);
            }
        }
    }

    if (*rname) {
        register_object(player, player, REGISTRATION_PROPDIR, rname, room);
    }
}

/**
 * Implementation of @program
 *
 * First, find a program that matches that name.  If there's one, then we 
 * put the player into edit mode and do it.  Otherwise, we create a new object
 * for the player, and call it a program.
 *
 * The name must pass ok_object_name, but no permissions checks are done.
 *
 * @param descr the descriptor of the programmer
 * @param player the player trying to create the program
 * @param name the name of the program
 * @param rname the registration name, which may be "" if not registering.
 */
void
do_program(int descr, dbref player, const char *name, const char *rname)
{
    dbref program;
    struct match_data md;
    char unparse_buf[BUFFER_LEN];

    /* @TODO: So if we decide to normalize how permissions are handled,
     *        this definitely needs some normalization.  It has no checks
     *        and relies entirely on the caller (the big switch statement)
     */

    if (!ok_object_name(name, TYPE_PROGRAM)) {
        notify(player, "Please specify a valid name for this program.");
        return;
    }

    init_match(descr, player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if (*rname || (program = match_result(&md)) == NOTHING) {
        program = create_program(player, name);
        unparse_object(player, program, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "Program %s created.", unparse_buf);

        if (*rname) {
            register_object(player, player, REGISTRATION_PROPDIR, (char *)rname, program);
        }
    } else if (program == AMBIGUOUS) {
        notifyf_nolisten(player, match_msg_ambiguous(name, 0));
        return;
    }

    edit_program(player, program);
}


/**
 * This is the implementation of @edit
 *
 * This does not do any permission checking, however the underlying call
 * (edit_program) does do some minimal checking to make sure the player
 * controls the program being edited.
 *
 * This wrapper handles the matching of the program name then hands off
 * to edit_program.
 *
 * @see edit_program
 *
 * @param descr this is descriptor of the player entering edit
 * @param player the player doing the editing
 * @param name the name of the program to edit.
 */
void
do_edit(int descr, dbref player, const char *name)
{
    dbref program;
    struct match_data md;

    if (!*name) {
        notify(player, "No program name given.");
        return;
    }

    init_match(descr, player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((program = noisy_match_result(&md)) == NOTHING)
        return;

    edit_program(player, program);
}

/**
 * Copy a single property, identified by its name, from one object to
 * another. helper routine for copy_props (below).
 *
 * @TODO: Remove this call entirely.  See my notes for copy_props.  I'm
 *        not going to bother fully documenting this call.
 */
static void
copy_one_prop(dbref source, dbref destination, char *propname)
{
    PropPtr currprop;
    PData newprop;

    /* read property from old object */
    currprop = get_property(source, propname);

#ifdef DISKBASE
    /* make sure the property value is there */
    propfetch(source, currprop);
#endif

    if (currprop) {
        /* flags can be copied. */
        newprop.flags = currprop->flags;

        /* data, however, must be cloned if it's a lock. */
        switch (PropType(currprop)) {
            case PROP_STRTYP:
            case PROP_INTTYP:
            case PROP_FLTTYP:
            case PROP_REFTYP:
                newprop.data = currprop->data;
                break;
            case PROP_LOKTYP:
                newprop.data.lok = copy_bool((currprop->data).lok);
            case PROP_DIRTYP:
                break;
        }

        /* now hook the new property into the destination object. */
        set_property(destination, propname, &newprop, 0);
    }

    return;
}

/**
 * Copy a property (sub)tree from one object to another one.
 *
 * This is a helper routine used by do_clone, based loosely on
 * listprops_wildcard from look.c.
 *
 * @TODO: This is probably 100% redundant with the copy_prop call in
 *        props.h ... the difference is this takes a 'dir' parameter for
 *        recursion purposes.  However, the only consumer of this call always
 *        uses "" which makes this identical to copy_prop
 *
 *        I'm not going to bother documenting this further because it
 *        really should be removed.  Also remove copy_one_prop which exists
 *        just to service this call and be redundant.
 */
static void
copy_props(dbref player, dbref source, dbref destination, const char *dir)
{
    char propname[BUFFER_LEN];
    char buf[BUFFER_LEN];
    PropPtr propadr, pptr;

    /* loop through all properties in the current propdir */
    propadr = first_prop(source, (char *) dir, &pptr, propname, sizeof(propname));

    while (propadr) {
        /* generate name for current property */
        snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);

        /* notify player */
        if (tp_verbose_clone && Wizard(OWNER(player))) {
            notifyf(player, "copying property %s", buf);
        }

        /* copy this property */
        copy_one_prop(source, destination, buf);

        /* recursively copy this property directory */
        copy_props(player, source, destination, buf);

        /* find next property in current dir */
        propadr = next_prop(pptr, propadr, propname, sizeof(propname));
    }

    /* chaos and disorder - our work here is done. */
    return;
}

/**
 * Use this to clone an object.  Implementation for the @clone command.
 *
 * This does most of the permission checks, but does not check for player
 * BUILDER bit.  Supports prop registration if rname is provided.  You
 * have no control over the cloned object's name, it uses the name of
 * the original object.
 *
 * @param descr the cloner's descriptor
 * @param player the player doing the cloning
 * @param name the object name to clone
 * @param rname the registration name, or "" if not registering.
 */
void
do_clone(int descr, dbref player, const char *name, const char *rname)
{
    dbref thing, clonedthing;
    int cost;
    struct match_data md;
    char unparse_buf[BUFFER_LEN];
    char unparse_buf2[BUFFER_LEN];

    /* Perform sanity checks */

    if (*name == '\0') {
        notify(player, "Clone what?");
        return;
    }

    /* All OK so far, so try to find the thing that should be cloned. We
       do not allow rooms, exits, etc. to be cloned for now. */

    init_match(descr, player, name, TYPE_THING, &md);
    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING)
        return;

    /* Further sanity checks */

    /* things only. */
    if (Typeof(thing) != TYPE_THING) {
        notify(player, "That is not a cloneable object.");
        return;
    }

    /* check the name again, just in case rules have changed since the
       original object was created. */
    if (!ok_object_name(NAME(thing), TYPE_THING)) {
        notify(player, "You cannot clone an object with this name.");
        return;
    }

    /* no stealing stuff. */
    if (!controls(player, thing)) {
        notify(player, "Permission denied. (you can't clone this)");
        return;
    }

    /* there ain't no such lunch as a free thing. */
    cost = OBJECT_GETCOST(GETVALUE(thing));
    if (cost < tp_object_cost) {
        cost = tp_object_cost;
    }

    if (!payfor(player, cost)) {
        notifyf(player, "Sorry, you don't have enough %s.", tp_pennies);
        return;
    }

    if (tp_verbose_clone) {
        unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "Now cloning %s...", unparse_buf);
    }

    clonedthing = create_thing(player, NAME(thing), player);

    /* copy all properties */
    copy_props(player, thing, clonedthing, "");

    SETVALUE(clonedthing, MAX(0,MIN(GETVALUE(thing), tp_max_object_endowment)));

    /* FIXME: should we clone attached actions? */
    EXITS(clonedthing) = NOTHING;

    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
    unparse_object(player, clonedthing, unparse_buf2, sizeof(unparse_buf2));
    notifyf(player, "Object %s cloned as %s.", unparse_buf, unparse_buf2);

    if (*rname) {
        register_object(player, player, REGISTRATION_PROPDIR, (char *)rname, clonedthing);
    }
}

/**
 * Implementation of @create
 *
 * Use this to create an object.  This does some checking; for instance,
 * the name must pass ok_object_name.  Does NOT check builder bit.
 * Validates that the cost you want to set on the object is a valid amount.
 * Note that if acost has an = in it, it will have a registration done.
 *
 * @param player the player doing the create
 * @param name the name of the object
 * @param acost the cost you want to assign, or "" for default.
 */
void
do_create(dbref player, char *name, char *acost)
{
    dbref thing;
    int cost;

    char buf2[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    char *rname, *qname;

    strcpyn(buf2, sizeof(buf2), acost);
    for (rname = buf2; (*rname && (*rname != ARG_DELIMITER)); rname++) ;
    qname = rname;

    if (*rname)
        rname++;

    *qname = '\0';

    qname = buf2;
    remove_ending_whitespace(&qname);

    skip_whitespace((const char **)&rname);

    cost = atoi(qname);

    if (!ok_object_name(name, TYPE_THING)) {
        notify(player, "Please specify a valid name for this thing.");
        return;
    }

    if (cost < 0) {
        notify(player, "You can't create an object for less than nothing!");
        return;
    }

    if (cost < tp_object_cost) {
        cost = tp_object_cost;
    }

    if (!payfor(player, cost)) {
        notifyf(player, "Sorry, you don't have enough %s.", tp_pennies);
        return;
    }

    thing = create_thing(player, name, player); 
    SETVALUE(thing, MAX(0,MIN(OBJECT_ENDOWMENT(cost), tp_max_object_endowment)));

    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Object %s created.", unparse_buf);

    if (*rname) {
        register_object(player, player, REGISTRATION_PROPDIR, rname, thing);
    }
}

/**
 * Parse the source string into a dbref and checks that it exists.
 *
 * This is a utility used by do_action and do_attach.
 *
 * @see do_action
 * @see do_attach
 *
 * The return value is the dbref of the source, or NOTHING if an
 * error occurs.
 *
 * @private
 * @param descr the descriptor of the caller
 * @param player the dbref of the caller
 * @param source_name the string to parse
 * @return dbref of source, or NOTHING on error.
 */
static dbref
parse_source(int descr, dbref player, const char *source_name)
{
    dbref source;
    struct match_data md;

    init_match(descr, player, source_name, NOTYPE, &md);

    /* source type can be any */
    match_neighbor(&md);
    match_me(&md);
    match_here(&md);
    match_possession(&md);
    match_registered(&md);
    match_absolute(&md);
    source = noisy_match_result(&md);

    if (source == NOTHING)
        return NOTHING;

    /* You can only attach actions to things you control */
    if (!controls(player, source)) {
        notify(player, "Permission denied. (you don't control the attachment point)");
        return NOTHING;
    }

    if (Typeof(source) == TYPE_EXIT) {
        notify(player, "You can't attach an action to an action.");
        return NOTHING;
    }

    if (Typeof(source) == TYPE_PROGRAM) {
        notify(player, "You can't attach an action to a program.");
        return NOTHING;
    }

    return source;
}

/**
 * This routine attaches a new existing action to a source object, if possible.
 *
 * The action will not do anything until it is LINKed.  This is the 
 * implementation of @action.
 *
 * Action names must pass ok_object_name.
 *
 * Calls create_action under the hood.  @see create_action
 *
 * Deducts money from player based on tuned configuration.  Supports
 * registration name if source_name contains an = character in it.
 * Supports autolink action if configured in tune.
 *
 * This does a lot of permission checking, but does not check for BUILDER.
 *
 * @param descr the descriptor
 * @param player the player creating the action
 * @param action_name the action name to make
 * @param source_name the name of the thing to put the action on.
 */
void
do_action(int descr, dbref player, const char *action_name,
          const char *source_name)
{
    dbref action, source;
    char buf2[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    char *rname, *qname;

    strcpyn(buf2, sizeof(buf2), source_name);

    for (rname = buf2; (*rname && (*rname != ARG_DELIMITER)); rname++) ;
    qname = rname;

    if (*rname)
        *(rname++) = '\0';

    qname = buf2;
    remove_ending_whitespace(&qname);

    skip_whitespace((const char **)&rname);

    if (!ok_object_name(action_name, TYPE_EXIT)) {
        notify(player, "Please specify a valid name for this action.");
        return;
    }

    if (!*qname) {
        notify(player, "You must specify a source object.");
        return;
    }

    if (((source = parse_source(descr, player, qname)) == NOTHING))
        return;

    if (!payfor(player, tp_exit_cost)) {
        notifyf(player, "Sorry, you don't have enough %s to make an action.", tp_pennies);
        return;
    }

    action = create_action(player, action_name, source);
    unparse_object(player, action, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Action %s created and attached.", unparse_buf);

    if (tp_autolink_actions) {
        notify(player, "Linked to NIL.");
    }

    if (*rname) {
        register_object(player, player, REGISTRATION_PROPDIR, rname, action);
    }
}

/**
 * This routine attaches a previously existing action to a source object.
 * The action will not do anything unless it is LINKed.  This is the
 * implementation of @attach
 *
 * Does all the permission checking associated except for the builder bit.
 * Will reset the priority level if there is is a priority set.
 *
 * Won't let you attach to a program or something that doesn't exist.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player doing the attaching.
 * @param action_name the action we are operating on
 * @param source_name what we are attaching to.
 */
void
do_attach(int descr, dbref player, const char *action_name, const char *source_name)
{
    dbref action, source;
    struct match_data md;

    if (!*action_name || !*source_name) {
        notify(player, "You must specify an action name and a source object.");
        return;
    }

    init_match(descr, player, action_name, TYPE_EXIT, &md);
    match_all_exits(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((action = noisy_match_result(&md)) == NOTHING)
        return;

    if (Typeof(action) != TYPE_EXIT) {
        notify(player, "That's not an action!");
        return;
    } else if (!controls(player, action)) {
        notify(player,
               "Permission denied. (you don't control the action you're trying to reattach)");
        return;
    }

    if (((source = parse_source(descr, player, source_name)) == NOTHING)
        || Typeof(source) == TYPE_PROGRAM)
        return;

    unset_source(action);
    set_source(action, source);

    notify(player, "Action re-attached.");

    if (MLevRaw(action)) {
        SetMLevel(action, 0);
        notify(player, "Action priority Level reset to zero.");
    }
}

/**
 * Implementation of @recycle command
 *
 * This is a wrapper around 'recycle', but it does a lot of additional
 * permission checks.  For instance, in order to recycle an object or
 * room, you must actually own it even if you a are a wizard.  This
 * makes the requirement for a two-step '@chown' then '@recycle' if you
 * wish to delete something as a wizard that you do not own.
 *
 * This is on purpose, I would imagine, to prevent accidents.
 *
 * @see recycle
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param name the object to recycle
 */
void
do_recycle(int descr, dbref player, const char *name)
{
    dbref thing;
    char buf[BUFFER_LEN];
    struct match_data md;

    init_match(descr, player, name, TYPE_THING, &md);
    match_everything(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING)
        return;

#ifdef GOD_PRIV
    if (tp_strict_god_priv && !God(player) && God(OWNER(thing))) {
        notify(player, "Only God may reclaim God's property.");
        return;
    }
#endif
    if (!controls(player, thing)) {
        if (Wizard(OWNER(player)) && (Typeof(thing) == TYPE_GARBAGE))
            notify(player, "That's already garbage!");
        else
            notify(player,
                   "Permission denied. (You don't control what you want to recycle)");
    } else {
        for (struct tune_entry *tent = tune_list; tent->name; tent++) {
            if (tent->type == TP_TYPE_DBREF && thing == *tent->currentval.d) {
                notify(player, "That object cannot currently be @recycled.");
                return;
            }
        }

        switch (Typeof(thing)) {
            case TYPE_ROOM:
                if (OWNER(thing) != OWNER(player)) {
                    notify(player,
                           "Permission denied. (You don't control the room you want to recycle)");
                    return;
                }

                if (thing == GLOBAL_ENVIRONMENT) {
                    notify(player,
                           "If you want to do that, why don't you just delete the database instead?  Room #0 contains everything, and is needed for database sanity.");
                    return;
                }

                break;
            case TYPE_THING:
                if (OWNER(thing) != OWNER(player)) {
                    notify(player,
                           "Permission denied. (You can't recycle a thing you don't control)");
                    return;
                }

                /* player may be a zombie or puppet */
                if (thing == player) {
                    snprintf(buf, sizeof(buf),
                             "%.512s's owner commands it to kill itself.  It blinks a few times in shock, and says, \"But.. but.. WHY?\"  It suddenly clutches it's heart, grimacing with pain..  Staggers a few steps before falling to it's knees, then plops down on it's face.  *thud*  It kicks its legs a few times, with weakening force, as it suffers a seizure.  It's color slowly starts changing to purple, before it explodes with a fatal *POOF*!",
                             NAME(thing));
                    notify_except(CONTENTS(LOCATION(thing)), thing, buf, player);
                    notify(OWNER(player), buf);
                    notify(OWNER(player), "Now don't you feel guilty?");
                }

                break;
            case TYPE_EXIT:
                if (OWNER(thing) != OWNER(player)) {
                    notify(player,
                           "Permission denied. (You may not recycle an exit you don't own)");
                    return;
                }

                unset_source(thing);
                break;
            case TYPE_PLAYER:
                notify(player, "You can't recycle a player!");
                return;
            case TYPE_PROGRAM:
                if (OWNER(thing) != OWNER(player)) {
                    notify(player,
                           "Permission denied. (You can't recycle a program you don't own)");
                    return;
                }

                SetMLevel(thing, 0);

                if (PROGRAM_INSTANCES(thing)) {
                        dequeue_prog(thing, 0);
                }

                /*
                 * @TODO This is a workaround for bug #201633
                 *
                 * This bug predates sourceforge and is from a system
                 * long gone: hence, we don't know what the bug is.
                 * Modern tracking of this bug is here:
                 * https://github.com/fuzzball-muck/fuzzball/issues/364
                 */
                if (PROGRAM_INSTANCES(thing)) {
                    assert(0);  /* getting here is a bug - we already dequeued it. */
                    notify(player, "Recycle failed: Program is still running.");
                    return;
                }

                break;
            case TYPE_GARBAGE:
                notify(player, "That's already garbage!");
                return;
        }

        snprintf(buf, sizeof(buf), "Thank you for recycling %.512s (#%d).", NAME(thing),
                 thing);
        recycle(descr, player, thing);
        notify(player, buf);
    }
}
