/** @file predicates.c
 *
 * Implementation of the "predicates" which are a set of calls that verify
 * if one is allowed to do certain actions or not.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#include "boolexp.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

/*
 * TODO: It would be useful if all these predicates were exposed as MUF
 *       primitives as often MUF primitives override built-ins and write
 *       their own versions of these predicates that are often inferior
 *       or inconsistent.
 */


/**
 * Checks if 'who' can link an object of type 'what_type' to 'where'
 *
 * Because this may be called prior to the object existing, it is
 * checked by type; however, 'Typeof(dbref)' is often used to see
 * if some existing objet's ref can link to where.
 *
 * @param who the person we are checking link permissions for
 * @param what_type the type of the object we wish to link to where
 * @param where the place we are linking to
 * @returns boolean true if who can link to what, false otherwise
 */
int
can_link_to(dbref who, object_flag_type what_type, dbref where)
{
    /* Can always link to HOME */
    if (where == HOME)
        return 1;

    /* Exits can be linked to NIL */
    if (what_type == TYPE_EXIT && where == NIL)
        return 1;

    /* Can't link to an invalid or recycled dbref */
    if (!OkObj(where))
        return 0;

    /* Players can only be linked to rooms */
    if (what_type == TYPE_PLAYER && Typeof(where) != TYPE_ROOM)
        return 0;

    /* Rooms can only be linked to things or other rooms */
    if (what_type == TYPE_ROOM
        && Typeof(where) != TYPE_THING && Typeof(where) != TYPE_ROOM)
        return 0;
 
    /* Things cannot be linked to exits or programs */
    if (what_type == TYPE_THING
        && (Typeof(where) == TYPE_EXIT || Typeof(where) == TYPE_PROGRAM))
        return 0;

    /* Programs cannot be linked */
    if (what_type == TYPE_PROGRAM)
        return 0;

    /* Target must be controlled or publicly linkable with its linklock passed */ 
    return controls(who, where) || (Linkable(where) && test_lock(NOTHING, who, where, MESGPROP_LINKLOCK));
}

/**
 * Checks if 'who' can teleport to 'where'
 *
 * This is used to see if a player can teleport to a place.  This does not
 * check if 'where' exists.  Tests link lock on the target, LINK_OK flag
 * on the target, if player controls target, and ABODE flag if target is
 * not a thing.
 *
 * @param who the player who wants to teleport
 * @param where the destination the player wishes to teleport to
 * @returns boolean true if who can teleport to where, false otherwise
 */
int
can_teleport_to(dbref who, dbref where)
{
    return (controls(who, where) ||
           (test_lock(NOTHING, who, where, MESGPROP_LINKLOCK) &&
           ((FLAGS(where) & LINK_OK) ||
            (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)))));
}

/**
 * Check to see if 'who' can see the flags of 'where'
 *
 * The rules are the same as can_teleport_to, for now, but that could
 * change so this is kept separate.
 *
 * @see can_teleport_to
 *
 * @param who the person to check to see if they can see the flags
 * @param where the object to check if flags can be seen
 */
int
can_see_flags(dbref who, dbref where)
{
    return can_teleport_to(who, where);
}

/**
 * Checks to see if what can be linked to something else by who.
 *
 * @param who the person who is doing the linking
 * @param what the object 'who' would like to link
 * @return boolean true if who can link what to something
 */
int
can_link(dbref who, dbref what)
{
    /* Anyone can link an exit that is currently unlinked. */
    return (controls(who, what) || ((Typeof(what) == TYPE_EXIT)
            && DBFETCH(what)->sp.exit.ndest == 0));
}

/**
 * Checks to see if player could actually do what is proposing to be done
 *
 * * If thing is an exit, this checks to see if the exit will
 *   perform a move that is allowed.
 * * Then, it checks the @lock on the thing, whether it's an exit or not.
 *
 * tp_secure_teleport note:
 *
 * You can only jump with an action from rooms that you own
 * or that are jump_ok, and you cannot jump to players that are !jump_ok.
 */
int
could_doit(int descr, dbref player, dbref thing)
{
    dbref source, dest, owner;

    if (Typeof(thing) == TYPE_EXIT) {
        /* If exit is unlinked, can't do it. */
        if (DBFETCH(thing)->sp.exit.ndest == 0) {
            return 0;
        }

        owner = OWNER(thing);
        source = LOCATION(player);
        dest = *(DBFETCH(thing)->sp.exit.dest);

        if (dest == NIL)
            return (eval_boolexp(descr, player, GETLOCK(thing, MESGPROP_LOCK), thing));

        if (Typeof(dest) == TYPE_PLAYER) {
            /* Check for additional restrictions related to player dests */
            dbref destplayer = dest;

            dest = LOCATION(dest);
            /* If the dest player isn't JUMP_OK, or if the dest player's loc
             * is set BOUND, can't do it.
             */
            if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
                return 0;
            }
        }

        if (dest != HOME && Typeof(dest) == TYPE_ROOM &&
            (FLAGS(dest) & GUEST) && ISGUEST(player)) {
            return 0;
        }

        /* for actions */
        if ((LOCATION(thing) != NOTHING) &&
            (Typeof(LOCATION(thing)) != TYPE_ROOM)) {
            /* If this is an exit on a Thing or a Player... */

            /* If the destination is a room or player, and the current
             * location is set BOUND (note: if the player is in a vehicle
             * set BUILDER this will also return failure)
             */
            if ((Typeof(dest) == TYPE_ROOM || Typeof(dest) == TYPE_PLAYER) &&
                (FLAGS(source) & BUILDER))
                return 0;

            /* If secure_teleport is true, and if the destination is a room */
            if (tp_secure_teleport && Typeof(dest) == TYPE_ROOM) {
                /* if player doesn't control the source and the source isn't
                 * set Jump_OK, then if the destination isn't HOME,
                 * can't do it.  (Should this include getlink(owner)?  Not
                 * everyone knows that 'home' or '#-3' can be linked to and
                 * be treated specially. -winged)
                 */
                if ((dest != HOME) && (!controls(owner, source))
                    && ((FLAGS(source) & JUMP_OK) == 0)) {
                    return 0;
                }

                /* FIXME: Add support for in-server banishment from rooms
                 * and environments here.
                 */
            }
        }
    }

    /* Check the @lock on the thing, as a final test. */
    return (eval_boolexp(descr, player, GETLOCK(thing, MESGPROP_LOCK), thing));
}

/**
 * Checks if a player can perform an action, emitting messages
 *
 * This does zombie checking, and if tp_allow_zombies is false it will
 * emit an appropriate error message.
 *
 * could_doit is used to see if the 'thing' is actionable by 'player'.
 * @see could_doit
 *
 * If could_doit returns true, success messages will be displayed
 * (succ and osucc) and this call will return true.  Otherwise, fail/ofail
 * messages will be displayed and this call will return false.
 *
 * @param descr the descriptor of the actor
 * @param player the player actor
 * @param thing the thing the player is trying to work with
 * @param default_fail_message a fail message to use if no 'fail' prop set
 * @returns boolean true if player can do it, false if not
 */
int
can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg)
{
    dbref loc;

    if ((loc = LOCATION(player)) == NOTHING)
        return 0;

    if (!Wizard(OWNER(player)) && tp_allow_zombies && Typeof(player) == TYPE_THING && (FLAGS(thing) & ZOMBIE)) {
        notify(player, "Sorry, but zombies can't do that.");
        return 0;
    }

    if (!could_doit(descr, player, thing)) {
        /* can't do it */
        if (GETFAIL(thing)) {
            exec_or_notify_prop(descr, player, thing, MESGPROP_FAIL, "(@Fail)");
        } else if (default_fail_msg) {
            notify(player, default_fail_msg);
        }

        if (GETOFAIL(thing) && !Dark(player)) {
            parse_oprop(descr, player, loc, thing, MESGPROP_OFAIL,
                        NAME(player), "(@Ofail)");
        }

        return 0;
    } else {
        /* can do it */
        if (GETSUCC(thing)) {
            exec_or_notify_prop(descr, player, thing, MESGPROP_SUCC, "(@Succ)");
        }

        if (GETOSUCC(thing) && !Dark(player)) {
            parse_oprop(descr, player, loc, thing, MESGPROP_OSUCC,
                        NAME(player), "(@Osucc)");
        }

        return 1;
    }
}

/**
 * Recursive check for loops in destinations of exits.
 *
 * Checks to see if any circular references are present in the destination
 * chain.  Returns 1 if circular reference found, 0 if not.
 *
 * So, you want to make sure this returns false prior to allowing an exit
 * link.
 *
 * @param source the exit to check
 * @param dest the potential destination to verify no loops
 * @return boolean true if an exit loop exists, false otherwise
 */
int
exit_loop_check(dbref source, dbref dest)
{
    if (source == dest)
        return 1;               /* That's an easy one! */

    if (!ObjExists(dest) || Typeof(dest) != TYPE_EXIT)
        return 0;

    for (int i = 0; i < DBFETCH(dest)->sp.exit.ndest; i++) {
        dbref current = (DBFETCH(dest)->sp.exit.dest)[i];

        if (!ObjExists(current))
            continue;

        if (current == source) {
            return 1;           /* Found a loop! */
        }

        if (Typeof(current) == TYPE_EXIT) {
            if (exit_loop_check(source, current)) {
                return 1;       /* Found one recursively */
            }
        }
    }

    return 0;                   /* No loops found */
}

/**
 * Check for environment loops
 *
 * Any item should always be able to 'find' its way to room #0.  Since the
 * loop check is recursive, we also put in a max iteration check, to keep
 * people from creating huge envchains in order to bring the server down.
 * We have a loop if we:
 *
 * a) Try to parent to ourselves.
 * b) Parent to nothing (not really a loop, but won't get you to #0).
 * c) Parent to our own *HOME* (not a valid destination).
 * d) Find our source room down the environment chain.
 *
 * Note: This system will only work if every step _up_ to this point has
 * resulted in a consistent (ie: no loops) environment.
 *
 * The desired outcome for this function is a return of FALSE in order to
 * permit linking.
 *
 * @private
 * @param source the source object to check
 * @param dest the potential new home for 'source'
 * @return boolean true if an environment loop would result, false if no loop
 */
static int
location_loop_check(dbref source, dbref dest)
{
    unsigned int level = 0;
    dbref pstack[MAX_PARENT_DEPTH + 2];

    if (source == dest) {
        return 1;
    }

    pstack[0] = source;
    pstack[1] = dest;

    while (level < MAX_PARENT_DEPTH) {
        dest = LOCATION(dest);

        if (dest == NOTHING) {
            return 0;
        }

        if (dest == HOME) {     /* We should never get this, either. */
            return 1;
        }

        if (dest == (dbref) 0) {        /* Reached the top of the chain. */
            return 0;
        }

        /*
         * TODO : This seems like we could be iterating over pstack a lot.
         *        Making this a recursive call that keeps track of its
         *        depth as a third parameter would probably be more optimal.
         *
         *        Look lower to parent_loop_check's TODO for more design ideas
         */

        /* Check to see if we've found this item before.. */
        for (unsigned int place = 0; place < (level + 2); place++) {
            if (pstack[place] == dest) {
                return 1;
            }
        }

        pstack[level + 2] = dest;
        level++;
    }

    return 1;
}

/**
 * Checks for a parent loop, resolving HOME to the appropriate real dbref
 *
 * This makes sure that 'source' can use 'dest' as a parent without causing
 * an environment loop.  If 'dest' is HOME, then it will be translated to
 * the correct actual ref -- PLAYER_HOME, THING_HOME, GLOBAL_ENVIRONMENT
 * (for rooms), or the owner for programs.
 *
 * This returns true if a loop is created or if HOME could not be translated
 * to something meaningful, so you only want to allow source to be moved
 * to dest IF this returns *false*.
 *
 * @param source the source ref
 * @param dest the place we wish to parent source to
 * @returns true if a loop would be created, false if not
 */
int
parent_loop_check(dbref source, dbref dest)
{
    unsigned int level = 0;
    dbref pstack[MAX_PARENT_DEPTH + 2];

    if (dest == HOME) {
        switch (Typeof(source)) {
            case TYPE_PLAYER:
                dest = PLAYER_HOME(source);
                break;

            case TYPE_THING:
                dest = THING_HOME(source);
                break;

            case TYPE_ROOM:
                dest = GLOBAL_ENVIRONMENT;
                break;

            case TYPE_PROGRAM:
                dest = OWNER(source);
                break;

            default:
                return 1;
        }
    }

    if (location_loop_check(source, dest)) {
        return 1;
    }

    if (source == dest) {
        return 1;
    }

    pstack[0] = source;
    pstack[1] = dest;

    /* TODO This seems redundant with location_loop_check, but getparent
     *      is a really complex call so maybe not.
     *
     *      This is almost identical to location_loop_check, though, maybe
     *      we can consolidate the code...
     *
     *      loop_check(source, dest, type, 0)
     *
     *      loop_check can be recursive, with '0' being the depth counter,
     *      and type distinguishing between 'getparent' and GET_LOCATION
     */
    while (level < MAX_PARENT_DEPTH) {
        dest = getparent(dest);

        if (dest == NOTHING) {
            return 0;
        }

        if (dest == HOME) {     /* We should never get this, either. */
            return 1;
        }

        if (dest == (dbref) 0) {        /* Reached the top of the chain. */
            return 0;
        }

        /* Check to see if we've found this item before.. */
        for (unsigned int place = 0; place < (level + 2); place++) {
            if (pstack[place] == dest) {
                return 1;
            }
        }

        pstack[level + 2] = dest;
        level++;
    }

    return 1;
}

/**
 * Can 'player' move in direction 'direction'
 *
 * If direction is 'home' and tp_enable_home is true, then this will
 * always return true.
 *
 * Otherwise, it will match 'direction' to exits with match_level set
 * to 'lev'.  @see init_match
 *
 * If an appropriate exit is found, returns true, otherwise returns false.
 *
 * @param descr the descriptor for the player
 * @param player the player trying to move
 * @param direction the direction they want to move in
 * @param lev the match_level to use -- @see init_match
 * @returns true if the player can move in that direction, false otherwise
 */
int
can_move(int descr, dbref player, const char *direction, int lev)
{
    struct match_data md;

    if (tp_enable_home && !strcasecmp(direction, "home"))
        return 1;

    /* otherwise match on exits */
    init_match(descr, player, direction, TYPE_EXIT, &md);
    md.match_level = lev;
    match_all_exits(&md);
    return (last_match_result(&md) != NOTHING);
}
