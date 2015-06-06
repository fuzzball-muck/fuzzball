#include "config.h"

/* Predicates for testing various conditions */

#include <ctype.h>

#include "db.h"
#include "props.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "externs.h"

int
OkObj(dbref obj)
{
	return(!(obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE));
}


int
can_link_to(dbref who, object_flag_type what_type, dbref where)
{
		/* Can always link to HOME */
	if (where == HOME)
		return 1;
		/* Can't link to an invalid dbref */
	if (where < 0 || where >= db_top)
		return 0;
	switch (what_type) {
	case TYPE_EXIT:
		/* If the target is LINK_OK, then any exit may be linked
		 * there.  Otherwise, only someone who controls the
		 * target may link there. */
		return (controls(who, where) || (FLAGS(where) & LINK_OK));
		/* NOTREACHED */
		break;
	case TYPE_PLAYER:
		/* Players may only be linked to rooms, that are either
		 * controlled by the player or set either L or A. */
		return (Typeof(where) == TYPE_ROOM && (controls(who, where)
											   || Linkable(where)));
		/* NOTREACHED */
		break;
	case TYPE_ROOM:
		/* Rooms may be linked to rooms or things (this sets their
		 * dropto location).  Target must be controlled, or be L or A. */
		return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_THING)
				&& (controls(who, where) || Linkable(where)));
		/* NOTREACHED */
		break;
	case TYPE_THING:
		/* Things may be linked to rooms, players, or other things (this
		 * sets the thing's home).  Target must be controlled, or be L or A. */
		return (
				(Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER ||
				 Typeof(where) == TYPE_THING) && 
				 (controls(who, where) || Linkable(where)));
		/* NOTREACHED */
		break;
	case NOTYPE:
		/* Why is this here? -winged */
		return (controls(who, where) || (FLAGS(where) & LINK_OK) ||
				(Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
		/* NOTREACHED */
		break;
	default:
		/* Programs can't be linked anywhere */
		return 0;
		/* NOTREACHED */
		break;
	}
	/* NOTREACHED */
	return 0;
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
		source = DBFETCH(player)->location;
		dest = *(DBFETCH(thing)->sp.exit.dest);

		if (Typeof(dest) == TYPE_PLAYER) {
			/* Check for additional restrictions related to player dests */
			dbref destplayer = dest;

			dest = DBFETCH(dest)->location;
			/* If the dest player isn't JUMP_OK, or if the dest player's loc
			 * is set BOUND, can't do it. */
			if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
				return 0;
			}
		}

		/* for actions */
		if ((DBFETCH(thing)->location != NOTHING) &&
			(Typeof(DBFETCH(thing)->location) != TYPE_ROOM)) {

			/* If this is an exit on a Thing or a Player... */

			/* If the destination is a room or player, and the current
			 * location is set BOUND (note: if the player is in a vehicle
			 * set BUILDER this will also return failure) */
			if ((Typeof(dest) == TYPE_ROOM || Typeof(dest) == TYPE_PLAYER) &&
				(FLAGS(source) & BUILDER)) return 0;

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
test_lock(int descr, dbref player, dbref thing, const char *lockprop)
{
	struct boolexp *lokptr;

	lokptr = get_property_lock(thing, lockprop);
	return (eval_boolexp(descr, player, lokptr, thing));
}


int
test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop)
{
	struct boolexp *lok;

	lok = get_property_lock(thing, lockprop);

	if (lok == TRUE_BOOLEXP)
		return 0;
	return (eval_boolexp(descr, player, lok, thing));
}


int
can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg)
{
	dbref loc;

	if ((loc = getloc(player)) == NOTHING)
		return 0;

	if (!Wizard(OWNER(player)) && Typeof(player) == TYPE_THING &&
			(FLAGS(thing) & ZOMBIE)) {
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

int
can_see(dbref player, dbref thing, int can_see_loc)
{
	if (player == thing || Typeof(thing) == TYPE_EXIT ||
			Typeof(thing) == TYPE_ROOM)
		return 0;

	if (can_see_loc) {
		switch (Typeof(thing)) {
		case TYPE_PROGRAM:
			return ((FLAGS(thing) & LINK_OK) || controls(player, thing));
		case TYPE_PLAYER:
			if (tp_dark_sleepers) {
				return (!Dark(thing) && online(thing));
			}
		default:
			return (!Dark(thing) || (controls(player, thing) && 
						!(FLAGS(player) & STICKY)));
		}
	} else {
		/* can't see loc */
		return (controls(player, thing) && !(FLAGS(player) & STICKY));
	}
}

int
controls(dbref who, dbref what)
{
	dbref index;

	/* No one controls invalid objects */
	if (what < 0 || what >= db_top)
		return 0;

	/* No one controls garbage */
	if (Typeof(what) == TYPE_GARBAGE)
		return 0;

	/* Zombies and puppets use the permissions of their owner */
	if (Typeof(who) != TYPE_PLAYER)
		who = OWNER(who);

	/* Wizard controls everything */
	if (Wizard(who)) {
#ifdef GOD_PRIV
		if(tp_strict_god_priv && God(OWNER(what)) && !God(who))
			/* Only God controls God's objects */
			return 0;
		else
#endif
		return 1;
	}

	if (tp_realms_control) {
		/* Realm Owner controls everything under his environment. */
		/* To set up a Realm, a Wizard sets the W flag on a room.  The
		 * owner of that room controls every Room object contained within
		 * that room, all the way to the leaves of the tree.
		 * -winged */
		for (index = what; index != NOTHING; index = getloc(index)) {
			if ((OWNER(index) == who) && (Typeof(index) == TYPE_ROOM)
					&& Wizard(index)) {
				/* Realm Owner doesn't control other Player objects */
				if(Typeof(what) == TYPE_PLAYER) {
					return 0;
				} else {
					return 1;
				}
			}
		}
	}

	/* exits are also controlled by the owners of the source and destination */
	/* ACTUALLY, THEY AREN'T.  IT OPENS A BAD MPI SECURITY HOLE. */
	/* any MPI on an exit's @succ or @fail would be run in the context
	 * of the owner, which would allow the owner of the src or dest to
	 * write malicious code for the owner of the exit to run.  Allowing them
	 * control would allow them to modify _ properties, thus enabling the
	 * security hole. -winged */
	/*
	 * if (Typeof(what) == TYPE_EXIT) {
	 *    int     i = DBFETCH(what)->sp.exit.ndest;
	 *
	 *    while (i > 0) {
	 *        if (who == OWNER(DBFETCH(what)->sp.exit.dest[--i]))
	 *            return 1;
	 *    }
	 *    if (who == OWNER(DBFETCH(what)->location))
	 *        return 1;
	 * }
	 */

	/* owners control their own stuff */
	return (who == OWNER(what));
}

int
restricted(dbref player, dbref thing, object_flag_type flag)
{
	switch (flag) {
	case ABODE:
			/* Trying to set a program AUTOSTART requires TrueWizard */
		return (!TrueWizard(OWNER(player)) && (Typeof(thing) == TYPE_PROGRAM));
		/* NOTREACHED */
		break;
        case YIELD:
                        /* Mucking with the env-chain matching requires TrueWizard */
                return (!(Wizard(OWNER(player))));
        case OVERT:
                        /* Mucking with the env-chain matching requires TrueWizard */
                return (!(Wizard(OWNER(player))));
	case ZOMBIE:
			/* Restricting a player from using zombies requires a wizard. */
		if (Typeof(thing) == TYPE_PLAYER)
			return (!(Wizard(OWNER(player))));
			/* If a player's set Zombie, he's restricted from using them...
			 * unless he's a wizard, in which case he can do whatever. */
		if ((Typeof(thing) == TYPE_THING) && (FLAGS(OWNER(player)) & ZOMBIE))
			return (!(Wizard(OWNER(player))));
		return (0);
	case VEHICLE:
			/* Restricting a player from using vehicles requires a wizard. */
		if (Typeof(thing) == TYPE_PLAYER)
			return (!(Wizard(OWNER(player))));
			/* If only wizards can create vehicles... */
		if (tp_wiz_vehicles) {
			/* then only a wizard can create a vehicle. :) */
			if (Typeof(thing) == TYPE_THING)
				return (!(Wizard(OWNER(player))));
		} else {
			/* But, if vehicles aren't restricted to wizards, then
			 * players who have not been restricted can do so */
			if ((Typeof(thing) == TYPE_THING) && (FLAGS(player) & VEHICLE))
				return (!(Wizard(OWNER(player))));
		}
		return (0);
	case DARK:
		/* Dark can be set on a Program or Room by anyone. */
		if (!Wizard(OWNER(player))) {
				/* Setting a player dark requires a wizard. */
			if (Typeof(thing) == TYPE_PLAYER)
				return (1);
				/* If exit darking is restricted, it requires a wizard. */
			if (!tp_exit_darking && Typeof(thing) == TYPE_EXIT)
				return (1);
				/* If thing darking is restricted, it requires a wizard. */
			if (!tp_thing_darking && Typeof(thing) == TYPE_THING)
				return (1);
		}
		return (0);

		/* NOTREACHED */
		break;
	case QUELL:
#ifdef GOD_PRIV
		/* Only God (or God's stuff) can quell or unquell another wizard. */
		return (God(OWNER(player)) || (TrueWizard(thing) && (thing != player) &&
					Typeof(thing) == TYPE_PLAYER));
#else
		/* You cannot quell or unquell another wizard. */
		return (TrueWizard(thing) && (thing != player) && (Typeof(thing) == TYPE_PLAYER));
#endif
		/* NOTREACHED */
		break;
	case MUCKER:
	case SMUCKER:
	case (SMUCKER | MUCKER):
	case BUILDER:
		/* Would someone tell me why setting a program SMUCKER|MUCKER doesn't
		 * go through here? -winged */
		/* Setting a program Bound causes any function called therein to be
		 * put in preempt mode, regardless of the mode it had before.
		 * Since this is just a convenience for atomic-functionwriters,
		 * why is it limited to only a Wizard? -winged */
		/* Setting a player Builder is limited to a Wizard. */
		return (!Wizard(OWNER(player)));
		/* NOTREACHED */
		break;
	case WIZARD:
			/* To do anything with a Wizard flag requires a Wizard. */
		if (Wizard(OWNER(player))) {
#ifdef GOD_PRIV
			/* ...but only God can make a player a Wizard, or re-mort
			 * one. */
			return ((Typeof(thing) == TYPE_PLAYER) && !God(player));
#else							/* !GOD_PRIV */
			/* We don't want someone setting themselves !W, to prevent
			 * a case where there are no wizards at all */
			return ((Typeof(thing) == TYPE_PLAYER && thing == OWNER(player)));
#endif							/* GOD_PRIV */
		} else
			return 1;
		/* NOTREACHED */
		break;
	default:
			/* No other flags are restricted. */
		return 0;
		/* NOTREACHED */
		break;
	}
	/* NOTREACHED */
}

/* Removes 'cost' value from 'who', and returns 1 if the act has been
 * paid for, else returns 0. */
int
payfor(dbref who, int cost)
{
	who = OWNER(who);
		/* Wizards don't have to pay for anything. */
	if (Wizard(who)) {
		return 1;
	} else if (GETVALUE(who) >= cost) {
		SETVALUE(who, GETVALUE(who) - cost);
		DBDIRTY(who);
		return 1;
	} else {
		return 0;
	}
}

int
word_start(const char *str, const char let)
{
	int chk;

	for (chk = 1; *str; str++) {
		if (chk && (*str == let))
			return 1;
		chk = (*str == ' ');
	}
	return 0;
}

#ifdef WIN32
static __inline int
#else
static inline int
#endif
ok_ascii_any(const char *name)
{
	const unsigned char *scan;
	for( scan=(const unsigned char*) name; *scan; ++scan ) {
		if( *scan>127 )
			return 0;
	}
	return 1;
}

int 
ok_ascii_thing(const char *name)
{
	return !tp_7bit_thing_names || ok_ascii_any(name);
}

int
ok_ascii_other(const char *name)
{
	return !tp_7bit_other_names || ok_ascii_any(name);
}
	
int
ok_name(const char *name)
{
	return (name
			&& *name
			&& *name != LOOKUP_TOKEN
			&& *name != REGISTERED_TOKEN
			&& *name != NUMBER_TOKEN
			&& !index(name, ARG_DELIMITER)
			&& !index(name, AND_TOKEN)
			&& !index(name, OR_TOKEN)
			&& !index(name, '\r')
			&& !index(name, ESCAPE_CHAR)
			&& !word_start(name, NOT_TOKEN)
			&& string_compare(name, "me")
			&& string_compare(name, "home")
			&& string_compare(name, "here")
			&& (
				!*tp_reserved_names ||
				!equalstr((char*)tp_reserved_names, (char*)name)
			));
}

int
ok_player_name(const char *name)
{
	const char *scan;

	if (!ok_name(name) || strlen(name) > PLAYER_NAME_LIMIT)
		return 0;
	

	for (scan = name; *scan; scan++) {
		if (!(isprint(*scan)
			 && !isspace(*scan))
			 && *scan != '('
			 && *scan != ')'
			 && *scan != '\''
			 && *scan != ',') {	
		    /* was isgraph(*scan) */
			return 0;
		}
	}

	/* Check the name isn't reserved */
	if (*tp_reserved_player_names && equalstr((char*)tp_reserved_player_names, (char*)name))
		return 0;

	/* lookup name to avoid conflicts */
	return (lookup_player(name) == NOTHING);
}

int
ok_password(const char *password)
{
	const char *scan;

	/* Password cannot be blank */
	if (*password == '\0')
		return 0;

	/* Password also cannot contain any nonprintable or space-type
	 * characters */
	for (scan = password; *scan; scan++) {
		if (!(isprint(*scan) && !isspace(*scan))) {
			return 0;
		}
	}

	/* Anything else is fair game */
	return 1;
}

/* If only paternity checks were this easy in real life... 
 * Returns 1 if the given 'child' is contained by the 'parent'.*/
int
isancestor(dbref parent, dbref child)
{
	while (child != NOTHING && child != parent) {
		child = getparent(child);
	}
	return child == parent;
}
