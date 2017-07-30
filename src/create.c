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
#include "tune.h"

/* use this to create an exit */
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

    if (!*direction) {
	notify(player, "You must specify a direction or action name to open.");
	return;
    }
    if (!ok_ascii_other(direction)) {
	notify(player, "Exit names are limited to 7-bit ASCII.");
	return;
    }
    if (!ok_name(direction)) {
	notify(player, "That's a strange name for an exit!");
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
		DBFETCH(exit)->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * ndest);
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

/* do_link
 *
 * Use this to link to a room that you own.  It also sets home for
 * objects and things, and drop-to's for rooms.
 * It seizes ownership of an unlinked exit, and costs 1 penny
 * plus a penny transferred to the exit owner if they aren't you
 *
 * All destinations must either be owned by you, or be LINK_OK.
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

    if (Typeof(thing) != TYPE_EXIT && index(dest_name, EXIT_DELIMITER)) {
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
	    if (!payfor(player, tp_link_cost + tp_exit_cost)) {
		notifyf(player, "It costs %d %s to link this exit.",
			(tp_link_cost + tp_exit_cost),
			(tp_link_cost + tp_exit_cost == 1) ? tp_penny : tp_pennies);
		return;
	    } else if (!Builder(player)) {
		notify(player, "Only authorized builders may seize exits.");
		return;
	    } else {
		/* pay the owner for his loss */
		dbref owner = OWNER(thing);

		SETVALUE(owner, GETVALUE(owner) + tp_exit_cost);
		DBDIRTY(owner);
	    }
	}

	/* link has been validated and paid for; do it */
	OWNER(thing) = OWNER(player);
	ndest = link_exit(descr, player, thing, (char *) dest_name, good_dest);
	if (ndest == 0) {
	    notify(player, "No destinations linked.");
	    SETVALUE(player, GETVALUE(player) + tp_link_cost);
	    DBDIRTY(player);
	    break;
	}
	DBFETCH(thing)->sp.exit.ndest = ndest;
	if (DBFETCH(thing)->sp.exit.dest) {
	    free(DBFETCH(thing)->sp.exit.dest);
	}
	DBFETCH(thing)->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * ndest);
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
    case TYPE_ROOM:		/* room dropto's */
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
	    DBFETCH(thing)->sp.room.dropto = dest;	/* dropto */
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

/*
 * do_dig
 *
 * Use this to create a room.
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

    if (*name == '\0') {
	notify(player, "You must specify a name for the room.");
	return;
    }
    if (!ok_ascii_other(name)) {
	notify(player, "Room names are limited to 7-bit ASCII.");
	return;
    }
    if (!ok_name(name)) {
	notify(player, "That's a strange name for a room!");
	return;
    }
    if (!payfor(player, tp_room_cost)) {
	notifyf(player, "Sorry, you don't have enough %s to dig a room.", tp_pennies);
	return;
    }

    newparent = LOCATION(LOCATION(player));
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

/*
  Use this to create a program.
  First, find a program that matches that name.  If there's one,
  then we put him into edit mode and do it.
  Otherwise, we create a new object for him, and call it a program.
  */
void
do_program(int descr, dbref player, const char *name, const char *rname)
{
    dbref program;
    struct match_data md;
    char unparse_buf[BUFFER_LEN];

    if (!*name) {
	notify(player, "No program name given.");
	return;
    }

    init_match(descr, player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if (*rname || (program = match_result(&md)) == NOTHING) {
	if (!ok_ascii_other(name)) {
	    notify(player, "Program names are limited to 7-bit ASCII.");
	    return;
	}

	if (!ok_name(name)) {
	    notify(player, "That's a strange name for a program!");
	    return;
	}

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

/*
 * copy a single property, identified by its name, from one object to
 * another. helper routine for copy_props (below).
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

/*
 * copy a property (sub)tree from one object to another one. this is a
 * helper routine used by do_clone, based loosely on listprops_wildcard from
 * look.c.
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

/*
 * do_clone
 *
 * Use this to clone an object.
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

    /* check the name again, just in case reserved name patterns have
       changed since the original object was created. */
    if (!ok_name(NAME(thing))) {
	notify(player, "You cannot clone something with such a weird name!");
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

    SETVALUE(clonedthing, MIN(GETVALUE(thing), tp_max_object_endowment));

    /* FIXME: should we clone attached actions? */
    EXITS(clonedthing) = NOTHING;

    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
    unparse_object(player, clonedthing, unparse_buf2, sizeof(unparse_buf2));
    notifyf(player, "Object %s cloned as %s.", unparse_buf, unparse_buf2);

    if (*rname) {
	register_object(player, player, REGISTRATION_PROPDIR, (char *)rname, clonedthing);
    }
}

/*
 * do_create
 *
 * Use this to create an object.
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
	*(rname++) = '\0';

    skip_whitespace((const char **)&rname);

    cost = atoi(qname);
    if (*name == '\0') {
	notify(player, "Create what?");
	return;
    } else if (!ok_ascii_thing(name)) {
	notify(player, "Thing names are limited to 7-bit ASCII.");
	return;
    } else if (!ok_name(name)) {
	notify(player, "That's a strange name for a thing!");
	return;
    } else if (cost < 0) {
	notify(player, "You can't create an object for less than nothing!");
	return;
    } else if (cost < tp_object_cost) {
	cost = tp_object_cost;
    }
    if (!payfor(player, cost)) {
	notifyf(player, "Sorry, you don't have enough %s.", tp_pennies);
	return;
    }

    thing = create_thing(player, name, player); 
    SETVALUE(thing, MIN(OBJECT_ENDOWMENT(cost), tp_max_object_endowment));

    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Object %s created.", unparse_buf);

    if (*rname) {
        register_object(player, player, REGISTRATION_PROPDIR, rname, thing);
    }
}

/*
 * parse_source()
 *
 * This is a utility used by do_action and do_attach.  It parses
 * the source string into a dbref, and checks to see that it
 * exists.
 *
 * The return value is the dbref of the source, or NOTHING if an
 * error occurs.
 *
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

/*
 * do_action()
 *
 * This routine attaches a new existing action to a source object,
 * where possible.
 * The action will not do anything until it is LINKed.
 *
 */
void
do_action(int descr, dbref player, const char *action_name, const char *source_name)
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

    if (!*action_name || !*qname) {
	notify(player, "You must specify an action name and a source object.");
	return;
    } else if (!ok_ascii_other(action_name)) {
	notify(player, "Action names are limited to 7-bit ASCII.");
	return;
    } else if (!ok_name(action_name)) {
	notify(player, "That's a strange name for an action!");
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

/*
 * do_attach()
 *
 * This routine attaches a previously existing action to a source object.
 * The action will not do anything unless it is LINKed.
 *
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

    unset_source(player, action);
    set_source(player, action, source);

    notify(player, "Action re-attached.");

    if (MLevRaw(action)) {
	SetMLevel(action, 0);
	notify(player, "Action priority Level reset to zero.");
    }
}
