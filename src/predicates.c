#include "config.h"

#include "boolexp.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

int
can_link_to(dbref who, object_flag_type what_type, dbref where)
{
    /* Can always link to HOME */
    if (where == HOME)
	return 1;

    /* Exits can be linked to NIL */
    if (where == NIL && what_type == TYPE_EXIT)
	return 1;

    /* Can't link to an invalid dbref */
    if (!ObjExists(where))
	return 0;

    switch (what_type) {
    case TYPE_EXIT:
	/* If the target is LINK_OK, then any exit may be linked
	 * there.  Otherwise, only someone who controls the
	 * target may link there. */
	return (controls(who, where) || ((FLAGS(where) & LINK_OK) && test_lock(NOTHING, who, where, MESGPROP_LINKLOCK)));
    case TYPE_PLAYER:
	/* Players may only be linked to rooms, that are either
	 * controlled by the player or set either L or A. */
	return (Typeof(where) == TYPE_ROOM && (controls(who, where) || (Linkable(where) && test_lock(NOTHING, who, where, MESGPROP_LINKLOCK))));
    case TYPE_ROOM:
	/* Rooms may be linked to rooms or things (this sets their
	 * dropto location).  Target must be controlled, or be L or A. */
	return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_THING)
		&& (controls(who, where) || (Linkable(where) && test_lock(NOTHING, who, where, MESGPROP_LINKLOCK))));
    case TYPE_THING:
	/* Things may be linked to rooms, players, or other things (this
	 * sets the thing's home).  Target must be controlled, or be L or A. */
	return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER ||
		 Typeof(where) == TYPE_THING) && (controls(who, where) || (Linkable(where) && test_lock(NOTHING, who, where, MESGPROP_LINKLOCK))));
    case NOTYPE:
	return (controls(who, where) || (test_lock(NOTHING, who, where, MESGPROP_LINKLOCK) &&
		((FLAGS(where) & LINK_OK) || (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)))));
    default:
	return 0;
    }
}

/* This checks to see if what can be linked to something else by who. */
int
can_link(dbref who, dbref what)
{
    /* Anyone can link an exit that is currently unlinked. */
    return (controls(who, what) || ((Typeof(what) == TYPE_EXIT)
				    && DBFETCH(what)->sp.exit.ndest == 0));
}

/*
 * Revision 1.2 -- SECURE_TELEPORT
 * you can only jump with an action from rooms that you own
 * or that are jump_ok, and you cannot jump to players that are !jump_ok.
 */

/*
 * could_doit: Checks to see if player could actually do what is proposing
 * to be done: if thing is an exit, this checks to see if the exit will
 * perform a move that is allowed. Then, it checks the @lock on the thing,
 * whether it's an exit or not.
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
	    return (eval_boolexp(descr, player, GETLOCK(thing), thing));

	if (Typeof(dest) == TYPE_PLAYER) {
	    /* Check for additional restrictions related to player dests */
	    dbref destplayer = dest;

	    dest = LOCATION(dest);
	    /* If the dest player isn't JUMP_OK, or if the dest player's loc
	     * is set BOUND, can't do it. */
	    if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
		return 0;
	    }
	}

	if (dest != HOME && Typeof(dest) == TYPE_ROOM && (FLAGS(dest) & GUEST) && ISGUEST(player)) {
	    return 0;
	}

	/* for actions */
	if ((LOCATION(thing) != NOTHING) && (Typeof(LOCATION(thing)) != TYPE_ROOM)) {

	    /* If this is an exit on a Thing or a Player... */

	    /* If the destination is a room or player, and the current
	     * location is set BOUND (note: if the player is in a vehicle
	     * set BUILDER this will also return failure) */
	    if ((Typeof(dest) == TYPE_ROOM || Typeof(dest) == TYPE_PLAYER) &&
		(FLAGS(source) & BUILDER))
		return 0;

	    /* If secure_teleport is true, and if the destination is a room */
	    if (tp_secure_teleport && Typeof(dest) == TYPE_ROOM) {
		/* if player doesn't control the source and the source isn't
		 * set Jump_OK, then if the destination isn't HOME,
		 * can't do it.  (Should this include getlink(owner)?  Not
		 * everyone knows that 'home' or '#-3' can be linked to and
		 * be treated specially. -winged) */
		if ((dest != HOME) && (!controls(owner, source))
		    && ((FLAGS(source) & JUMP_OK) == 0)) {
		    return 0;
		}
		/* FIXME: Add support for in-server banishment from rooms
		 * and environments here. */
	    }
	}
    }

    /* Check the @lock on the thing, as a final test. */
    return (eval_boolexp(descr, player, GETLOCK(thing), thing));
}

int
can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg)
{
    dbref loc;

    if ((loc = LOCATION(player)) == NOTHING)
	return 0;

    if (!Wizard(OWNER(player)) && Typeof(player) == TYPE_THING && (FLAGS(thing) & ZOMBIE)) {
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
	    parse_oprop(descr, player, loc, thing, MESGPROP_OFAIL, NAME(player), "(@Ofail)");
	}
	return 0;
    } else {
	/* can do it */
	if (GETSUCC(thing)) {
	    exec_or_notify_prop(descr, player, thing, MESGPROP_SUCC, "(@Succ)");
	}
	if (GETOSUCC(thing) && !Dark(player)) {
	    parse_oprop(descr, player, loc, thing, MESGPROP_OSUCC, NAME(player), "(@Osucc)");
	}
	return 1;
    }
}

/* exit_loop_check()
 *
 * Recursive check for loops in destinations of exits.  Checks to see
 * if any circular references are present in the destination chain.
 * Returns 1 if circular reference found, 0 if not.
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

/* What are we doing here?  Quick explanation - we want to prevent
   environment loops from happening.  Any item should always be able
   to 'find' its way to room #0.  Since the loop check is recursive,
   we also put in a max iteration check, to keep people from creating
   huge envchains in order to bring the server down.  We have a loop
   if we:
   a) Try to parent to ourselves.
   b) Parent to nothing (not really a loop, but won't get you to #0).
   c) Parent to our own home (not a valid destination).
   d) Find our source room down the environment chain.
   Note: This system will only work if every step _up_ to this point has
   resulted in a consistent (ie: no loops) environment.
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

    while (level < MAX_PARENT_DEPTH) {
        /* if (Typeof(dest) == TYPE_THING) {
           dest = THING_HOME(dest);
           } */
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
