#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "match.h"
#include "params.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

void
do_name(int descr, dbref player, const char *name, char *newname)
{
    dbref thing;
    char *password;

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
		((Typeof(thing) != TYPE_THING) && !ok_ascii_other(newname))) {
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
		if (!quiet)
		    notify(player, "Unlinked.");
		if (MLevRaw(exit)) {
		    SetMLevel(exit, 0);
		    DBDIRTY(exit);
		    if (!quiet)
			notify(player, "Action priority Level reset to 0.");
		}
		break;
	    case TYPE_ROOM:
		ts_modifyobject(exit);
		DBSTORE(exit, sp.room.dropto, NOTHING);
		if (!quiet)
		    notify(player, "Dropto removed.");
		break;
	    case TYPE_THING:
		ts_modifyobject(exit);
		THING_SET_HOME(exit, OWNER(exit));
		DBDIRTY(exit);
		if (!quiet)
		    notify(player, "Thing's home reset to owner.");
		break;
	    case TYPE_PLAYER:
		ts_modifyobject(exit);
		PLAYER_SET_HOME(exit, tp_player_start);
		DBDIRTY(exit);
		if (!quiet)
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
	    if (!controls(player, thing)) {
		notify(player,
		       "Permission denied. (The exit is linked, and you don't control it)");
		return;
	    }
	} else {
	    if (!Wizard(OWNER(player)) && (GETVALUE(player) < (tp_link_cost + tp_exit_cost))) {
		notifyf(player, "It costs %d %s to link this exit.",
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
	if (ndest == 0) {
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
    case TYPE_ROOM:		/* room dropto's */
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
#endif				/* GOD_PRIV */
    if (!Wizard(OWNER(player))) {
	if (Typeof(thing) != TYPE_EXIT ||
	    (DBFETCH(thing)->sp.exit.ndest && !controls_link(player, thing))) {
	    if (!(FLAGS(thing) & CHOWN_OK) ||
		Typeof(thing) == TYPE_PROGRAM
		|| !test_lock(descr, player, thing, MESGPROP_CHLOCK)) {
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
	if (!Wizard(OWNER(player)) && LOCATION(player) != thing) {
	    notify(player, "You can only chown \"here\".");
	    return;
	}
	ts_modifyobject(thing);
	OWNER(thing) = OWNER(owner);
	break;
    case TYPE_THING:
	if (!Wizard(OWNER(player)) && LOCATION(thing) != player) {
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
	notifyf(player, "Owner changed to %s.", unparse_object(player, owner));
    }
    DBDIRTY(thing);
}

int
restricted(dbref player, dbref thing, object_flag_type flag)
{
    switch (flag) {
    case ABODE:
	/* Trying to set a program AUTOSTART requires TrueWizard */
	return (!TrueWizard(OWNER(player)) && (Typeof(thing) == TYPE_PROGRAM));
    case GUEST:
	/* Guest operations require a wizard */
	return (!(Wizard(OWNER(player))));
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
    case QUELL:
#ifdef GOD_PRIV
	/* Only God (or God's stuff) can quell or unquell another wizard. */
	return (God(OWNER(player)) || (TrueWizard(thing) && (thing != player) &&
				       Typeof(thing) == TYPE_PLAYER));
#else
	/* You cannot quell or unquell another wizard. */
	return (TrueWizard(thing) && (thing != player) && (Typeof(thing) == TYPE_PLAYER));
#endif
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
    case WIZARD:
	/* To do anything with a Wizard flag requires a Wizard. */
	if (Wizard(OWNER(player))) {
#ifdef GOD_PRIV
	    /* ...but only God can make a player a Wizard, or re-mort
	     * one. */
	    return ((Typeof(thing) == TYPE_PLAYER) && !God(player));
#else				/* !GOD_PRIV */
	    /* We don't want someone setting themselves !W, to prevent
	     * a case where there are no wizards at all */
	    return ((Typeof(thing) == TYPE_PLAYER && thing == OWNER(player)));
#endif				/* GOD_PRIV */
	} else
	    return 1;
    default:
	/* No other flags are restricted. */
	return 0;
    }
}

void
do_set(int descr, dbref player, const char *name, const char *flag)
{
    dbref thing;
    const char *p;
    object_flag_type f;

    /* find thing */
    if ((thing = match_controlled(descr, player, name)) == NOTHING)
	return;
#ifdef GOD_PRIV
    /* Only God can set anything on any of his stuff */
    if (tp_strict_god_priv && !God(player) && God(OWNER(thing))) {
	notify(player, "Only God may touch God's property.");
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
	char *x;		/* to preserve string location so we can free it */
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
		free((void *) x);
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

	pname++;		/* move to next character */
	/* while (isspace(*pname) && *pname) pname++; */
	if (*pname == '^' && number(pname + 1))
	    ival = atoi(++pname);

	if (Prop_System(type)
	    || (!Wizard(OWNER(player)) && (Prop_SeeOnly(type) || Prop_Hidden(type)))) {
	    notify(player, "Permission denied. (The property is restricted.)");
	    free((void *) x);
	    return;
	}

	if (!(*pname)) {
	    ts_modifyobject(thing);
	    remove_property(thing, type, 0);
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
	    dbref obj = CONTENTS(thing);

	    for (; obj != NOTHING; obj = NEXTOBJ(obj)) {
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
		notify(player,
		       "Permission denied. (Only a Wizard may set the M-level of an exit)");
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
    } else if (string_prefix("GUEST", p)) {
	f = GUEST;
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
	if (f == GUEST && Typeof(thing) == TYPE_PLAYER) {
	    remove_property(thing, LEGACY_GUEST_PROP, 0);
	}
	notify(player, "Flag reset.");
    } else {
	/* set the flag */
	ts_modifyobject(thing);
	FLAGS(thing) |= f;
	DBDIRTY(thing);
	if (f == GUEST && Typeof(thing) == TYPE_PLAYER) {
	    PData property;
	    property.flags = PROP_STRTYP;
	    property.data.str = "yes";
	    set_property(thing, LEGACY_GUEST_PROP, &property, 0);
	}
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

    if (Prop_System(pname)
	|| (!Wizard(OWNER(player)) && (Prop_SeeOnly(pname) || Prop_Hidden(pname)))) {
	notify(player, "Permission denied. (The property is restricted.)");
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
	set_property(thing, pname, &mydat, 0);
    } else if (string_prefix("dbref", type)) {
	init_match(descr, player, value, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);
	if ((ref = noisy_match_result(&md)) == NOTHING)
	    return;
	mydat.flags = PROP_REFTYP;
	mydat.data.ref = ref;
	set_property(thing, pname, &mydat, 0);
    } else if (string_prefix("lock", type)) {
	lok = parse_boolexp(descr, player, value, 0);
	if (lok == TRUE_BOOLEXP) {
	    notify(player, "I don't understand that lock.");
	    return;
	}
	mydat.flags = PROP_LOKTYP;
	mydat.data.lok = lok;
	set_property(thing, pname, &mydat, 0);
    } else if (string_prefix("erase", type)) {
	if (*value) {
	    notify(player, "Don't give a value when erasing a property.");
	    return;
	}
	remove_property(thing, pname, 0);
	notify(player, "Property erased.");
	return;
    } else {
	notify(player, "I don't know what type of property you want to set!");
	notify(player, "Valid types are string, integer, float, dbref, lock, and erase.");
	return;
    }
    notify(player, "Property set.");
}
