
/* $Header: /cvsroot/fbmuck/fbmuck/src/set.c,v 1.31 2009/10/11 05:19:18 points Exp $ */


#include "copyright.h"
#include "config.h"

/* commands which set parameters */
#include <stdio.h>
#include <ctype.h>
#include "fbstrings.h"

#include "db.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "match.h"
#include "interface.h"
#include "externs.h"


static dbref
match_controlled(int descr, dbref player, const char *name)
{
	dbref match;
	struct match_data md;

	init_match(descr, player, name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);

	match = noisy_match_result(&md);
	if (match != NOTHING && !controls(player, match)) {
		notify(player, "Permission denied. (You don't control what was matched)");
		return NOTHING;
	} else {
		return match;
	}
}

void
do_name(int descr, dbref player, const char *name, char *newname)
{
	dbref thing;
	char *password;

	NOGUEST("@name",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		/* check for bad name */
		if (*newname == '\0') {
			notify(player, "Give it what new name?");
			return;
		}
		/* check for renaming a player */
		if (Typeof(thing) == TYPE_PLAYER) {
			/* split off password */
			for (password = newname; *password && !isspace(*password); password++) ;
			/* eat whitespace */
			if (*password) {
				*password++ = '\0';	/* terminate name */
				while (*password && isspace(*password))
					password++;
			}
			/* check for null password */
			if (!*password) {
				notify(player, "You must specify a password to change a player name.");
				notify(player, "E.g.: name player = newname password");
				return;
			} else if (!check_password(thing, password)) {
				notify(player, "Incorrect password.");
				return;
			} else if (string_compare(newname, NAME(thing))
					   && !ok_player_name(newname)) {
				notify(player, "You can't give a player that name.");
				return;
			}
			/* everything ok, notify */
			log_status("NAME CHANGE: %s(#%d) to %s", NAME(thing), thing, newname);
			delete_player(thing);
			if (NAME(thing)) {
				free((void *) NAME(thing));
			}
			ts_modifyobject(thing);
			NAME(thing) = alloc_string(newname);
			add_player(thing);
			notify(player, "Name set.");
			return;
		} else {
			if (((Typeof(thing) == TYPE_THING) && !ok_ascii_thing(newname)) ||
			    ((Typeof(thing) != TYPE_THING) && !ok_ascii_other(newname)) ) {
				notify(player, "Invalid 8-bit name.");
				return;
			}
			if (!ok_name(newname)) {
				notify(player, "That is not a reasonable name.");
				return;
			}
		}

		/* everything ok, change the name */
		if (NAME(thing)) {
			free((void *) NAME(thing));
		}
		ts_modifyobject(thing);
		NAME(thing) = alloc_string(newname);
		notify(player, "Name set.");
		DBDIRTY(thing);
		if (Typeof(thing) == TYPE_EXIT && MLevRaw(thing)) {
			SetMLevel(thing, 0);
			notify(player, "Action priority Level reset to zero.");
		}
	}
}

void
do_describe(int descr, dbref player, const char *name, const char *description)
{
	dbref thing;

	NOGUEST("@describe",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETDESC(thing, description);
		if(description && *description) {
			notify(player, "Description set.");
		} else {
			notify(player, "Description cleared.");
		}
	}
}

void
do_idescribe(int descr, dbref player, const char *name, const char *description)
{
	dbref thing;

	NOGUEST("@idescribe",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETIDESC(thing, description);
		if(description && *description) {
			notify(player, "Description set.");
		} else {
			notify(player, "Description cleared.");
		}
	}
}

void
do_doing(int descr, dbref player, const char *name, const char *mesg)
{
	dbref thing;

	NOGUEST("@doing",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETDOING(thing, mesg);
		if(mesg && *mesg) {
			notify(player, "Doing set.");
		} else {
			notify(player, "Doing cleared.");
		}
	}
}

void
do_fail(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@fail",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETFAIL(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

void
do_success(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@success",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETSUCC(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

/* sets the drop message for player */
void
do_drop_message(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@drop",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETDROP(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

void
do_osuccess(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@osuccess",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETOSUCC(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

void
do_ofail(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@ofail",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETOFAIL(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

void
do_odrop(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@odrop",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETODROP(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

void
do_oecho(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@oecho",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETOECHO(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

void
do_pecho(int descr, dbref player, const char *name, const char *message)
{
	dbref thing;

	NOGUEST("@pecho",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		SETPECHO(thing, message);
		if(message && *message) {
			notify(player, "Message set.");
		} else {
			notify(player, "Message cleared.");
		}
	}
}

/* sets a lock on an object to the lockstring passed to it.
   If the lockstring is null, then it unlocks the object.
   this returns a 1 or a 0 to represent success. */
int
setlockstr(int descr, dbref player, dbref thing, const char *keyname)
{
	struct boolexp *key;

	if (*keyname != '\0') {
		key = parse_boolexp(descr, player, keyname, 0);
		if (key == TRUE_BOOLEXP) {
			return 0;
		} else {
			/* everything ok, do it */
			ts_modifyobject(thing);
			SETLOCK(thing, key);
			return 1;
		}
	} else {
		ts_modifyobject(thing);
		CLEARLOCK(thing);
		return 1;
	}
}

void
do_conlock(int descr, dbref player, const char *name, const char *keyname)
{
	dbref thing;
	struct boolexp *key;
	struct match_data md;
	PData mydat;

	NOGUEST("@conlock",player);

	init_match(descr, player, name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);

	switch (thing = match_result(&md)) {
	case NOTHING:
		notify(player, "I don't see what you want to set the container-lock on!");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know which one you want to set the container-lock on!");
		return;
	default:
		if (!controls(player, thing)) {
			notify(player, "You can't set the container-lock on that!");
			return;
		}
		break;
	}

	if (!*keyname) {
		mydat.flags = PROP_LOKTYP;
		mydat.data.lok = TRUE_BOOLEXP;
		set_property(thing, "_/clk", &mydat);
		ts_modifyobject(thing);
		notify(player, "Container lock cleared.");
	} else {
		key = parse_boolexp(descr, player, keyname, 0);
		if (key == TRUE_BOOLEXP) {
			notify(player, "I don't understand that key.");
		} else {
			/* everything ok, do it */
			mydat.flags = PROP_LOKTYP;
			mydat.data.lok = key;
			set_property(thing, "_/clk", &mydat);
			ts_modifyobject(thing);
			notify(player, "Container lock set.");
		}
	}
}

void
do_flock(int descr, dbref player, const char *name, const char *keyname)
{
	dbref thing;
	struct boolexp *key;
	struct match_data md;
	PData mydat;

	NOGUEST("@force_lock",player);

	init_match(descr, player, name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);

	switch (thing = match_result(&md)) {
	case NOTHING:
		notify(player, "I don't see what you want to set the force-lock on!");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know which one you want to set the force-lock on!");
		return;
	default:
		if (!controls(player, thing)) {
			notify(player, "You can't set the force-lock on that!");
			return;
		}
		break;
	}

	if (force_level) {
		notify(player, "You can't use @flock from an @force or {force}.");
		return;
	}

	if (!*keyname) {
		mydat.flags = PROP_LOKTYP;
		mydat.data.lok = TRUE_BOOLEXP;
		set_property(thing, "@/flk", &mydat);
		ts_modifyobject(thing);
		notify(player, "Force lock cleared.");
	} else {
		key = parse_boolexp(descr, player, keyname, 0);
		if (key == TRUE_BOOLEXP) {
			notify(player, "I don't understand that key.");
		} else {
			/* everything ok, do it */
			mydat.flags = PROP_LOKTYP;
			mydat.data.lok = key;
			set_property(thing, "@/flk", &mydat);
			ts_modifyobject(thing);
			notify(player, "Force lock set.");
		}
	}
}

void
do_chlock(int descr, dbref player, const char *name, const char *keyname)
{
	dbref thing;
	struct boolexp *key;
	struct match_data md;
	PData mydat;

	NOGUEST("@chown_lock",player);

	init_match(descr, player, name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);

	switch (thing = match_result(&md)) {
	case NOTHING:
		notify(player, "I don't see what you want to set the chown-lock on!");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know which one you want to set the chown-lock on!");
		return;
	default:
		if (!controls(player, thing)) {
			notify(player, "You can't set the chown-lock on that!");
			return;
		}
		break;
	}

	if (!*keyname) {
		mydat.flags = PROP_LOKTYP;
		mydat.data.lok = TRUE_BOOLEXP;
		set_property(thing, "_/chlk", &mydat);
		ts_modifyobject(thing);
		notify(player, "Chown lock cleared.");
	} else {
		key = parse_boolexp(descr, player, keyname, 0);
		if (key == TRUE_BOOLEXP) {
			notify(player, "I don't understand that key.");
		} else {
			/* everything ok, do it */
			mydat.flags = PROP_LOKTYP;
			mydat.data.lok = key;
			set_property(thing, "_/chlk", &mydat);
			ts_modifyobject(thing);
			notify(player, "Chown lock set.");
		}
	}
}

void
do_lock(int descr, dbref player, const char *name, const char *keyname)
{
	dbref thing;
	struct boolexp *key;
	struct match_data md;

	NOGUEST("@lock",player);

	init_match(descr, player, name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);

	switch (thing = match_result(&md)) {
	case NOTHING:
		notify(player, "I don't see what you want to lock!");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know which one you want to lock!");
		return;
	default:
		if (!controls(player, thing)) {
			notify(player, "You can't lock that!");
			return;
		}
		break;
	}

	if(keyname && *keyname) {
		key = parse_boolexp(descr, player, keyname, 0);
		if (key == TRUE_BOOLEXP) {
			notify(player, "I don't understand that key.");
		} else {
			/* everything ok, do it */
			SETLOCK(thing, key);
			ts_modifyobject(thing);
			notify(player, "Locked.");
		}
	} else
		do_unlock(descr, player, name);

}

void
do_unlock(int descr, dbref player, const char *name)
{
	dbref thing;

	NOGUEST("@unlock",player);

	if ((thing = match_controlled(descr, player, name)) != NOTHING) {
		ts_modifyobject(thing);
		CLEARLOCK(thing);
		notify(player, "Unlocked.");
	}
}

int
controls_link(dbref who, dbref what)
{
	switch (Typeof(what)) {
	case TYPE_EXIT:
		{
			int i = DBFETCH(what)->sp.exit.ndest;

			while (i > 0) {
				if (controls(who, DBFETCH(what)->sp.exit.dest[--i]))
					return 1;
			}
			if (who == OWNER(DBFETCH(what)->location))
				return 1;
			return 0;
		}

	case TYPE_ROOM:
		{
			if (controls(who, DBFETCH(what)->sp.room.dropto))
				return 1;
			return 0;
		}

	case TYPE_PLAYER:
		{
			if (controls(who, PLAYER_HOME(what)))
				return 1;
			return 0;
		}

	case TYPE_THING:
		{
			if (controls(who, THING_HOME(what)))
				return 1;
			return 0;
		}

	case TYPE_PROGRAM:
	default:
		return 0;
	}
}

/* like do_unlink, but if quiet is true, then only error messages are
   printed. */
void
_do_unlink(int descr, dbref player, const char *name, int quiet)
{
	dbref exit;
	struct match_data md;

	init_match(descr, player, name, TYPE_EXIT, &md);
	match_absolute(&md);
	match_player(&md);
	match_everything(&md);
	switch (exit = match_result(&md)) {
	case NOTHING:
		notify(player, "Unlink what?");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know which one you mean!");
		break;
	default:
		if (!controls(player, exit) && !controls_link(player, exit)) {
			notify(player, "Permission denied. (You don't control the exit or its link)");
		} else {
			switch (Typeof(exit)) {
			case TYPE_EXIT:
				if (DBFETCH(exit)->sp.exit.ndest != 0) {
					SETVALUE(OWNER(exit), GETVALUE(OWNER(exit)) + tp_link_cost);
					DBDIRTY(OWNER(exit));
				}
				ts_modifyobject(exit);
				DBSTORE(exit, sp.exit.ndest, 0);
				if (DBFETCH(exit)->sp.exit.dest) {
					free((void *) DBFETCH(exit)->sp.exit.dest);
					DBSTORE(exit, sp.exit.dest, NULL);
				}
				if(!quiet)
					notify(player, "Unlinked.");
				if (MLevRaw(exit)) {
					SetMLevel(exit, 0);
					DBDIRTY(exit);
					if(!quiet)
						notify(player, "Action priority Level reset to 0.");
				}
				break;
			case TYPE_ROOM:
				ts_modifyobject(exit);
				DBSTORE(exit, sp.room.dropto, NOTHING);
				if(!quiet)
					notify(player, "Dropto removed.");
				break;
			case TYPE_THING:
				ts_modifyobject(exit);
				THING_SET_HOME(exit, OWNER(exit));
				DBDIRTY(exit);
				if(!quiet)
					notify(player, "Thing's home reset to owner.");
				break;
			case TYPE_PLAYER:
				ts_modifyobject(exit);
				PLAYER_SET_HOME(exit, tp_player_start);
				DBDIRTY(exit);
				if(!quiet)
					notify(player, "Player's home reset to default player start room.");
				break;
			default:
				notify(player, "You can't unlink that!");
				break;
			}
		}
	}
}

void
do_unlink(int descr, dbref player, const char *name)
{
	NOGUEST("@unlink",player);
	/* do a regular, non-quiet unlink. */
	_do_unlink(descr, player, name, 0);
}

void
do_unlink_quiet(int descr, dbref player, const char *name)
{
	_do_unlink(descr, player, name, 1);
}

/*
 * do_relink()
 *
 * re-link an exit object. FIXME: this shares some code with do_link() which
 * should probably be moved into a separate function (is_link_ok() or
 * something like that).
 *
 */
void
do_relink(int descr, dbref player, const char *thing_name, const char *dest_name)
{
	dbref thing;
	dbref dest;
	dbref good_dest[MAX_LINKS];
	struct match_data md;
	int ndest;

	NOGUEST("@relink",player);

	init_match(descr, player, thing_name, TYPE_EXIT, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_registered(&md);
	if (Wizard(OWNER(player))) {
		match_player(&md);
	}
	if ((thing = noisy_match_result(&md)) == NOTHING)
		return;

	/* first of all, check if the new target would be valid, so we can
	   avoid breaking the old link if it isn't. */
	   
	switch (Typeof(thing)) {
		case TYPE_EXIT:
			/* we're ok, check the usual stuff */
			if (DBFETCH(thing)->sp.exit.ndest != 0) {
				if(!controls(player, thing)) {
					notify(player, "Permission denied. (The exit is linked, and you don't control it)");
					return;
				}
			} else {
				if(!Wizard(OWNER(player)) && (GETVALUE(player) < (tp_link_cost + tp_exit_cost))) {
					notify_fmt(player, "It costs %d %s to link this exit.",
							   (tp_link_cost + tp_exit_cost),
							   (tp_link_cost + tp_exit_cost == 1) ? tp_penny : tp_pennies);
					return;
				} else if (!Builder(player)) {
					notify(player, "Only authorized builders may seize exits.");
					return;
				}					
			}
			
			/* be anal: each and every new links destination has
			   to be ok. Detailed error messages are given by
			   link_exit_dry(). */
			   
			ndest = link_exit_dry(descr, player, thing, (char *) dest_name, good_dest);
			if(ndest == 0) {
				notify(player, "Invalid target.");
				return;
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
				notify(player, "Permission denied. (You can't link to where you want to.");
				return;
			}
			if (parent_loop_check(thing, dest)) {
				notify(player, "That would cause a parent paradox.");
				return;
			}
			break;
		case TYPE_ROOM:			/* room dropto's */
			init_match(descr, player, dest_name, TYPE_ROOM, &md);
			match_neighbor(&md);
			match_possession(&md);
			match_registered(&md);
			match_absolute(&md);
			match_home(&md);
			if ((dest = noisy_match_result(&md)) == NOTHING)
				return;
			if (!controls(player, thing) || !can_link_to(player, Typeof(thing), dest)
				|| (thing == dest)) {
				notify(player, "Permission denied. (You can't link to the dropto like that)");
				return;
			}
			break;
		case TYPE_PROGRAM:
			notify(player, "You can't link programs to things!");
			return;
		default:
			notify(player, "Internal error: weird object type.");
			log_status("PANIC: weird object: Typeof(%d) = %d", thing, Typeof(thing));
			return;
	}

	do_unlink_quiet(descr, player, thing_name);
	notify(player, "Attempting to relink...");
	do_link(descr, player, thing_name, dest_name);

}

void
do_chown(int descr, dbref player, const char *name, const char *newowner)
{
	dbref thing;
	dbref owner;
	struct match_data md;

	if (!*name) {
		notify(player, "You must specify what you want to take ownership of.");
		return;
	}
	init_match(descr, player, name, NOTYPE, &md);
	match_everything(&md);
	match_absolute(&md);
	if ((thing = noisy_match_result(&md)) == NOTHING)
		return;

	if (*newowner && string_compare(newowner, "me")) {
		if ((owner = lookup_player(newowner)) == NOTHING) {
			notify(player, "I couldn't find that player.");
			return;
		}
	} else {
		owner = OWNER(player);
	}
	if (!Wizard(OWNER(player)) && OWNER(player) != owner) {
		notify(player, "Only wizards can transfer ownership to others.");
		return;
	}
#ifdef GOD_PRIV
	if (Wizard(OWNER(player)) && !God(player) && God(owner)) {
		notify(player, "God doesn't need an offering or sacrifice.");
		return;
	}
#endif /* GOD_PRIV */
	if (!Wizard(OWNER(player))) {
		if (Typeof(thing) != TYPE_EXIT ||
			(DBFETCH(thing)->sp.exit.ndest && !controls_link(player, thing))) {
			if (!(FLAGS(thing) & CHOWN_OK) ||
				Typeof(thing) == TYPE_PROGRAM || !test_lock(descr, player, thing, "_/chlk")) {
				notify(player, "You can't take possession of that.");
				return;
			}
		}
	}

	if (tp_realms_control && !Wizard(OWNER(player)) && TrueWizard(thing) &&
		Typeof(thing) == TYPE_ROOM) {
		notify(player, "You can't take possession of that.");
		return;
	}

	switch (Typeof(thing)) {
	case TYPE_ROOM:
		if (!Wizard(OWNER(player)) && DBFETCH(player)->location != thing) {
			notify(player, "You can only chown \"here\".");
			return;
		}
		ts_modifyobject(thing);
		OWNER(thing) = OWNER(owner);
		break;
	case TYPE_THING:
		if (!Wizard(OWNER(player)) && DBFETCH(thing)->location != player) {
			notify(player, "You aren't carrying that.");
			return;
		}
		ts_modifyobject(thing);
		OWNER(thing) = OWNER(owner);
		break;
	case TYPE_PLAYER:
		notify(player, "Players always own themselves.");
		return;
	case TYPE_EXIT:
	case TYPE_PROGRAM:
		ts_modifyobject(thing);
		OWNER(thing) = OWNER(owner);
		break;
	case TYPE_GARBAGE:
		notify(player, "No one wants to own garbage.");
		return;
	}
	if (owner == player)
		notify(player, "Owner changed to you.");
	else {
		char buf[BUFFER_LEN];

		snprintf(buf, sizeof(buf), "Owner changed to %s.", unparse_object(player, owner));
		notify(player, buf);
	}
	DBDIRTY(thing);
}


/* Note: Gender code taken out.  All gender references are now to be handled
   by property lists...
   Setting of flags and property code done here.  Note that the PROP_DELIMITER
   identifies when you're setting a property.
   A @set <thing>= :clear
   will clear all properties.
   A @set <thing>= type:
   will remove that property.
   A @set <thing>= propname:string
   will add that string property or replace it.
   A @set <thing>= propname:^value
   will add that integer property or replace it.
 */

void
do_set(int descr, dbref player, const char *name, const char *flag)
{
	dbref thing;
	const char *p;
	object_flag_type f;

	NOGUEST("@set",player);

	/* find thing */
	if ((thing = match_controlled(descr, player, name)) == NOTHING)
		return;
#ifdef GOD_PRIV
	/* Only God can set anything on any of his stuff */
	if(!God(player) && God(OWNER(thing))) {
		notify(player,"Only God may touch God's property.");
		return;
	}
#endif

	/* move p past NOT_TOKEN if present */
	for (p = flag; *p && (*p == NOT_TOKEN || isspace(*p)); p++) ;

	/* Now we check to see if it's a property reference */
	/* if this gets changed, please also modify boolexp.c */
	if (index(flag, PROP_DELIMITER)) {
		/* copy the string so we can muck with it */
		char *type = alloc_string(flag);	/* type */
		char *pname = (char *) index(type, PROP_DELIMITER);	/* propname */
		char *x;				/* to preserve string location so we can free it */
		char *temp;
		int ival = 0;

		x = type;
		while (isspace(*type) && (*type != PROP_DELIMITER))
			type++;
		if (*type == PROP_DELIMITER) {
			/* clear all properties */
			for (type++; isspace(*type); type++) ;
			if (string_compare(type, "clear")) {
				notify(player, "Use '@set <obj>=:clear' to clear all props on an object");
				free((void *)x);
				return;
			}
			remove_property_list(thing, Wizard(OWNER(player)));
			ts_modifyobject(thing);
			notify(player, "All user-owned properties removed.");
			free((void *) x);
			return;
		}
		/* get rid of trailing spaces and slashes */
		for (temp = pname - 1; temp >= type && isspace(*temp); temp--) ;
		while (temp >= type && *temp == '/')
			temp--;
		*(++temp) = '\0';

		pname++;				/* move to next character */
		/* while (isspace(*pname) && *pname) pname++; */
		if (*pname == '^' && number(pname + 1))
			ival = atoi(++pname);

		if (Prop_System(type) || (!Wizard(OWNER(player)) && (Prop_SeeOnly(type) || Prop_Hidden(type)))) {
			notify(player, "Permission denied. (The property is hidden from you.)");
			free((void *)x);
			return;
		}

		if (!(*pname)) {
			ts_modifyobject(thing);
			remove_property(thing, type);
			notify(player, "Property removed.");
		} else {
			ts_modifyobject(thing);
			if (ival) {
				add_property(thing, type, NULL, ival);
			} else {
				add_property(thing, type, pname, 0);
			}
			notify(player, "Property set.");
		}
		free((void *) x);
		return;
	}
	/* identify flag */
	if (*p == '\0') {
		notify(player, "You must specify a flag to set.");
		return;
	} else if ((!string_compare("0", p) || !string_compare("M0", p)) ||
			   ((string_prefix("MUCKER", p)) && (*flag == NOT_TOKEN))) {
		if (!Wizard(OWNER(player))) {
			if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)) {
				notify(player, "Permission denied. (You can't clear that mucker flag)");
				return;
			}
		}
		if (force_level) {
			notify(player, "Can't set this flag from an @force or {force}.");
			return;
		}
		SetMLevel(thing, 0);
		notify(player, "Mucker level set.");
		return;
	} else if (!string_compare("1", p) || !string_compare("M1", p)) {
		if (!Wizard(OWNER(player))) {
			if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)
				|| (MLevRaw(player) < 1)) {
				notify(player, "Permission denied. (You may not set that M1)");
				return;
			}
		}
		if (force_level) {
			notify(player, "Can't set this flag from an @force or {force}.");
			return;
		}
		SetMLevel(thing, 1);
		notify(player, "Mucker level set.");
		return;
	} else if ((!string_compare("2", p) || !string_compare("M2", p)) ||
			   ((string_prefix("MUCKER", p)) && (*flag != NOT_TOKEN))) {
		if (!Wizard(OWNER(player))) {
			if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)
				|| (MLevRaw(player) < 2)) {
				notify(player, "Permission denied. (You may not set that M2)");
				return;
			}
		}
		if (force_level) {
			notify(player, "Can't set this flag from an @force or {force}.");
			return;
		}
		SetMLevel(thing, 2);
		notify(player, "Mucker level set.");
		return;
	} else if (!string_compare("3", p) || !string_compare("M3", p)) {
		if (!Wizard(OWNER(player))) {
			if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)
				|| (MLevRaw(player) < 3)) {
				notify(player, "Permission denied. (You may not set that M3)");
				return;
			}
		}
		if (force_level) {
			notify(player, "Can't set this flag from an @force or {force}.");
			return;
		}
		SetMLevel(thing, 3);
		notify(player, "Mucker level set.");
		return;
	} else if (!string_compare("4", p) || !string_compare("M4", p)) {
		notify(player, "To set Mucker Level 4, set the Wizard bit and another Mucker bit.");
		return;
	} else if (string_prefix("WIZARD", p)) {
		if (force_level) {
			notify(player, "Can't set this flag from an @force or {force}.");
			return;
		}
		f = WIZARD;
	} else if (string_prefix("ZOMBIE", p)) {
		f = ZOMBIE;
	} else if (string_prefix("VEHICLE", p)
		|| string_prefix("VIEWABLE", p)) {
		if (*flag == NOT_TOKEN && Typeof(thing) == TYPE_THING) {
			dbref obj = DBFETCH(thing)->contents;

			for (; obj != NOTHING; obj = DBFETCH(obj)->next) {
				if (Typeof(obj) == TYPE_PLAYER) {
					notify(player, "That vehicle still has players in it!");
					return;
				}
			}
		}
		f = VEHICLE;
	} else if (string_prefix("LINK_OK", p)) {
		f = LINK_OK;

	} else if (string_prefix("XFORCIBLE", p) || string_prefix("XPRESS", p)) {
		if (force_level) {
			notify(player, "Can't set this flag from an @force or {force}.");
			return;
		}
		if (Typeof(thing) == TYPE_EXIT) {
			if (!Wizard(OWNER(player))) {
				notify(player, "Permission denied. (Only a Wizard may set the M-level of an exit)");
				return;
			}
		}
		f = XFORCIBLE;

	} else if (string_prefix("KILL_OK", p)) {
		f = KILL_OK;
	} else if ((string_prefix("DARK", p)) || (string_prefix("DEBUG", p))) {
		f = DARK;
	} else if ((string_prefix("STICKY", p)) || (string_prefix("SETUID", p)) ||
			   (string_prefix("SILENT", p))) {
		f = STICKY;
	} else if (string_prefix("QUELL", p)) {
		f = QUELL;
	} else if (string_prefix("BUILDER", p) || string_prefix("BOUND", p)) {
		f = BUILDER;
	} else if (string_prefix("CHOWN_OK", p) || string_prefix("COLOR", p)) {
		f = CHOWN_OK;
	} else if (string_prefix("JUMP_OK", p)) {
		f = JUMP_OK;
	} else if (string_prefix("HAVEN", p) || string_prefix("HARDUID", p)) {
		f = HAVEN;
	} else if ((string_prefix("ABODE", p)) ||
			   (string_prefix("AUTOSTART", p)) || (string_prefix("ABATE", p))) {
		f = ABODE;
        } else if (string_prefix("YIELD", p) && tp_enable_match_yield &&
                   (Typeof(thing) == TYPE_ROOM || Typeof(thing) == TYPE_THING)) {
                f = YIELD;
        } else if (string_prefix("OVERT", p) && tp_enable_match_yield &&
                   (Typeof(thing) == TYPE_ROOM || Typeof(thing) == TYPE_THING)) {
                f = OVERT;
	} else {
		notify(player, "I don't recognize that flag.");
		return;
	}

	/* check for restricted flag */
	if (restricted(player, thing, f)) {
		notify(player, "Permission denied. (restricted flag)");
		return;
	}
	/* check for stupid wizard */
	if (f == WIZARD && *flag == NOT_TOKEN && thing == player) {
		notify(player, "You cannot make yourself mortal.");
		return;
	}
	/* else everything is ok, do the set */
	if (*flag == NOT_TOKEN) {
		/* reset the flag */
		ts_modifyobject(thing);
		FLAGS(thing) &= ~f;
		DBDIRTY(thing);
		notify(player, "Flag reset.");
	} else {
		/* set the flag */
		ts_modifyobject(thing);
		FLAGS(thing) |= f;
		DBDIRTY(thing);
		notify(player, "Flag set.");
	}
}

void
do_propset(int descr, dbref player, const char *name, const char *prop)
{
	dbref thing, ref;
	char *p, *q;
	char buf[BUFFER_LEN];
	char *type, *pname, *value;
	struct match_data md;
	struct boolexp *lok;
	PData mydat;

	NOGUEST("@propset",player);

	/* find thing */
	if ((thing = match_controlled(descr, player, name)) == NOTHING)
		return;

	while (isspace(*prop))
		prop++;
	strcpyn(buf, sizeof(buf), prop);

	type = p = buf;
	while (*p && *p != PROP_DELIMITER)
		p++;
	if (*p)
		*p++ = '\0';

	if (*type) {
		q = type + strlen(type) - 1;
		while (q >= type && isspace(*q))
			*(q--) = '\0';
	}

	pname = p;
	while (*p && *p != PROP_DELIMITER)
		p++;
	if (*p)
		*p++ = '\0';
	value = p;

	while (*pname == PROPDIR_DELIMITER || isspace(*pname))
		pname++;
	if (*pname) {
		q = pname + strlen(pname) - 1;
		while (q >= pname && isspace(*q))
			*(q--) = '\0';
	}

	if (!*pname) {
		notify(player, "I don't know which property you want to set!");
		return;
	}

	if (Prop_System(pname) || (!Wizard(OWNER(player)) && (Prop_SeeOnly(pname) || Prop_Hidden(pname)))) {
		notify(player, "Permission denied. (can't set a property that's restricted against you)");
		return;
	}

	if (!*type || string_prefix("string", type)) {
		add_property(thing, pname, value, 0);
	} else if (string_prefix("integer", type)) {
		if (!number(value)) {
			notify(player, "That's not an integer!");
			return;
		}
		add_property(thing, pname, NULL, atoi(value));
	} else if (string_prefix("float", type)) {
		if (!ifloat(value)) {
			notify(player, "That's not a floating point number!");
			return;
		}
		mydat.flags = PROP_FLTTYP;
		mydat.data.fval = strtod(value, NULL);
		set_property(thing, pname, &mydat);
	} else if (string_prefix("dbref", type)) {
		init_match(descr, player, value, NOTYPE, &md);
		match_absolute(&md);
		match_everything(&md);
		if ((ref = noisy_match_result(&md)) == NOTHING)
			return;
		mydat.flags = PROP_REFTYP;
		mydat.data.ref = ref;
		set_property(thing, pname, &mydat);
	} else if (string_prefix("lock", type)) {
		lok = parse_boolexp(descr, player, value, 0);
		if (lok == TRUE_BOOLEXP) {
			notify(player, "I don't understand that lock.");
			return;
		}
		mydat.flags = PROP_LOKTYP;
		mydat.data.lok = lok;
		set_property(thing, pname, &mydat);
	} else if (string_prefix("erase", type)) {
		if (*value) {
			notify(player, "Don't give a value when erasing a property.");
			return;
		}
		remove_property(thing, pname);
		notify(player, "Property erased.");
		return;
	} else {
		notify(player, "I don't know what type of property you want to set!");
		notify(player, "Valid types are string, integer, float, dbref, lock, and erase.");
		return;
	}
	notify(player, "Property set.");
}

void
set_flags_from_tunestr(dbref obj, const char* tunestr)
{
	const char *p = tunestr;
	object_flag_type f = 0;

	for (;;) {
		char pcc = toupper(*p);
		if (pcc == '\0' || pcc == '\n' || pcc == '\r') {
			break;
		} else if (pcc == '0') {
			SetMLevel(obj, 0);
		} else if (pcc == '1') {
			SetMLevel(obj, 1);
		} else if (pcc == '2') {
			SetMLevel(obj, 2);
		} else if (pcc == '3') {
			SetMLevel(obj, 3);
		} else if (pcc == 'A') {
			f = ABODE;
		} else if (pcc == 'B') {
			f = BUILDER;
		} else if (pcc == 'C') {
			f = CHOWN_OK;
		} else if (pcc == 'D') {
			f = DARK;
		} else if (pcc == 'H') {
			f = HAVEN;
		} else if (pcc == 'J') {
			f = JUMP_OK;
		} else if (pcc == 'K') {
			f = KILL_OK;
		} else if (pcc == 'L') {
			f = LINK_OK;
		} else if (pcc == 'M') {
			SetMLevel(obj, 2);
		} else if (pcc == 'Q') {
			f = QUELL;
		} else if (pcc == 'S') {
			f = STICKY;
		} else if (pcc == 'V') {
			f = VEHICLE;
		} else if (pcc == 'W') {
			/* f = WIZARD;     This is very bad to auto-set. */
		} else if (pcc == 'X') {
			f = XFORCIBLE;
                } else if (pcc == 'Y') {
                        f = YIELD;
                } else if (pcc == 'O') {
                        f = OVERT;
		} else if (pcc == 'Z') {
			f = ZOMBIE;
		}
		FLAGS(obj) |= f;
		p++;
	}
	ts_modifyobject(obj);
	DBDIRTY(obj);
}


static const char *set_c_version = "$RCSfile: set.c,v $ $Revision: 1.31 $";
const char *get_set_c_version(void) { return set_c_version; }
