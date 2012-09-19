/* $Header: /cvsroot/fbmuck/fbmuck/src/predicates.c,v 1.20 2009/10/11 05:30:43 points Exp $ */

/*
 * $Log: predicates.c,v $
 * Revision 1.20  2009/10/11 05:30:43  points
 * Fixed compile issue if GOD_PRIV not set.  Added missing tune external.
 *
 * Revision 1.19  2009/10/11 05:19:18  points
 * Enable 'yielding' of matches up the environment tree when finding possible exits.
 *
 * Revision 1.18  2007/05/08 09:38:09  winged
 * Get rid of spoofing vulnerability with single quote (possessive) and comma in player @name
 *
 * Revision 1.17  2007/03/08 17:13:49  winged
 * adding tunes for 8-bit object names per tracker 1676289
 *
 * Revision 1.16  2005/08/18 15:46:11  wog
 * Fix bug introduced in r1.15 by which all non-wizards controlled everything (!)
 * if GOD_PRIV was defined.
 *
 * Revision 1.15  2005/07/14 12:16:23  winged
 * Adding a bunch of GOD_PRIV God's objects can't be screwed with code
 *
 * Revision 1.14  2005/07/04 12:04:24  winged
 * Initial revisions for everything.
 *
 * Revision 1.13  2005/06/27 19:30:35  winged
 * Prevent wizards from setting themselves \!W with GOD_PRIV undefined, to prevent a case where there are no wizards at all
 *
 * Revision 1.12  2005/06/21 17:25:57  winged
 * GOD_PRIV now allows God and God's things to set other wizards QUELL.
 * Documented a LOT of things herein.
 * Simplified OkObj().
 *
 * Revision 1.11  2004/01/14 16:57:11  wolfwings
 * Further cleanup of #defines across codebase.
 * Replaced numerous #defines relating to Pennies with SETVALUE/GETVALUE.
 * Removed replaced #defines from db.h.
 * Simplified processing of prim_movepennies to support newly-extended penny-prop system.
 * Replaced SETVALUE with LOADVALUE in db.c database-loading functions.
 * Moved ANTILOCK to OBSOLETE_ANTILOCK, limited processing to pre-Foxen8 databases.
 *
 * Revision 1.10  2004/01/12 09:42:49  wolfwings
 * Removed remaining Anonymity code.
 *
 * Revision 1.9  2003/11/04 19:26:19  revar
 * Fixed a number of off-by-one crasher bugs, due to bad comparisons with db_top.
 *
 * Revision 1.8  2003/10/07 07:24:28  revar
 * Fixed possible crasher bug with autostart programs.
 *
 * Revision 1.7  2002/09/08 23:07:19  sombre
 * Fixed memory leak when toading online players.
 * Fixed remove_prop bug so it will remove props ending in /. (bug #537744)
 * Fixed potential buffer overrun with the CHECKRETURN and ABORT_MPI macros.
 * Fixed @omessage bug where player names would not be prefixed on additional
 *   newlines. (bug #562370)
 * Added IGNORING? ( d1 d2 -- i ) returns true if d1 is ignoring d2.
 * Added IGNORE_ADD ( d1 d2 -- ) adds d2 to d1's ignore list.
 * Added IGNORE_DEL ( d1 d2 -- ) removes d2 from d1's ignore list.
 * Added ARRAY_GET_IGNORELIST ( d -- a ) returns an array of d's ignores.
 * Added support for ignoring (gagging) players, ignores are mutual in that if
 *   player A ignores player B, A will not hear B, and B will not hear A.
 * Added ignore_prop @tune to specify the directory the ignore list is held under,
 *   if set blank ignore support is disabled, defaults to "@ignore/def".
 * Added max_ml4_preempt_count @tune to specify the maximum number of instructions
 *   an mlevel4 (wizbitted) program may run before it is aborted, if set to 0
 *   no limit is imposed.  Defaults to 0.
 * Added reserved_names @tune which when set to a smatch pattern will refuse any
 *   object creations or renames which match said pattern.  Defaults to "".
 * Added reserved_player_names @tune which when set to a smatch pattern will refuse
 *   any player creations or renames which match said pattern.  Defaults to "".
 *
 * Revision 1.6  2001/05/16 22:23:11  wog
 * Made ( and ) restricted to only player names.
 *
 * Revision 1.5  2001/05/16 22:14:34  wog
 * Prevented player names with ( or ) in them.
 *
 * Revision 1.4  2000/11/22 10:01:58  revar
 * Changed MPI from using Wizbit objects to give special permissions, to using
 * 'Blessed' properties.  Blessed props have few permissions restrictions.
 * Added @bless and @unbless wizard commands.
 * Added BLESSPROP and UNBLESSPROP muf primitives.
 * Added {bless} {unbless} and {revoke} MPI commands.
 * Fixed {listprops} crasher bug.
 *
 * Revision 1.3  2000/06/15 18:35:11  revar
 * Prettified some code formatting slightly.
 *
 * Revision 1.2  2000/03/29 12:21:02  revar
 * Reformatted all code into consistent format.
 * 	Tabs are 4 spaces.
 * 	Indents are one tab.
 * 	Braces are generally K&R style.
 * Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
 * Rewrote restart script as a bourne shell script.
 *
 * Revision 1.1.1.1  1999/12/16 03:23:29  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.1.1.1  1999/12/12 07:27:43  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/12 02:47:32  foxen
 * Initial revision
 *
 * Revision 5.13  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.12  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  91/08/20  15:05:30  jearls
 * Initial revision
 *
 * Revision 1.2  91/03/24  01:19:30  lynx
 * added check for jump_ok or owned by owner of actions
 *
 * Revision 1.1  91/01/24  00:44:27  cks
 * changes for QUELL.
 *
 * Revision 1.0  91/01/22  20:54:34  cks
 * Initial revision
 *
 * Revision 1.8  90/09/18  08:01:39  rearl
 * Fixed a few broken things.
 *
 * Revision 1.7  90/09/16  04:42:42  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.6  90/09/10  02:22:30  rearl
 * Fixed some calls to pronoun_substitute.
 *
 * Revision 1.5  90/08/27  03:32:49  rearl
 * Major changes on several predicates, usage...
 *
 * Revision 1.4  90/08/15  03:07:34  rearl
 * Macroified some things.  Took out #ifdef GENDER.
 *
 * Revision 1.3  90/08/06  02:46:30  rearl
 * Put can_link() back in.  Added restricted() for flags.
 * With #define GOD_PRIV, sub-wizards can no longer set other
 * players to Wizard.
 *
 * Revision 1.2  90/08/02  18:54:04  rearl
 * Fixed controls() to return TRUE if exit is unlinked.  Everyone
 * controls an unlinked exit.  Got rid of can_link().
 *
 * Revision 1.1  90/07/19  23:03:58  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
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
		if(God(OWNER(what)) && !God(who))
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

static inline int
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
static const char *predicates_c_version = "$RCSfile: predicates.c,v $ $Revision: 1.20 $";
const char *get_predicates_c_version(void) { return predicates_c_version; }
