#include "config.h"

#include "boolexp.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "edit.h"
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

static void
register_object(dbref location, char *propdir, char *name, dbref object)
{
    PData mydat;
    char buf[BUFFER_LEN];

    snprintf(buf, sizeof(buf), "%s/%s", propdir, name);
    mydat.flags = PROP_REFTYP;
    mydat.data.ref = object;
    set_property(location, buf, &mydat, 0);
}

/* use this to create an exit */
void
do_open(int descr, dbref player, const char *direction, const char *linkto)
{
    dbref loc, exit;
    dbref good_dest[MAX_LINKS];
    char buf2[BUFFER_LEN];
    char *rname, *qname;
    int ndest;

    strcpyn(buf2, sizeof(buf2), linkto);
    for (rname = buf2; (*rname && (*rname != ARG_DELIMITER)); rname++) ;
    qname = rname;
    if (*rname)
	rname++;
    *qname = '\0';

    while (((qname--) > buf2) && (isspace(*qname)))
	*qname = '\0';
    qname = buf2;
    for (; *rname && isspace(*rname); rname++) ;

    if ((loc = LOCATION(player)) == NOTHING)
	return;
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
	/* create the exit */
	exit = new_object();

	/* initialize everything */
	NAME(exit) = alloc_string(direction);
	LOCATION(exit) = loc;
	OWNER(exit) = OWNER(player);
	FLAGS(exit) = TYPE_EXIT;
	DBFETCH(exit)->sp.exit.ndest = 0;
	DBFETCH(exit)->sp.exit.dest = NULL;

	/* link it in */
	PUSH(exit, EXITS(loc));
	DBDIRTY(loc);

	/* and we're done */
	notifyf(player, "Exit %s opened as #%d.", NAME(exit), exit);

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
        register_object(player, REGISTRATION_PROPDIR, rname, exit);
        notifyf(player, "Registered as $%s", rname);
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
    room = new_object();

    /* Initialize everything */
    newparent = LOCATION(LOCATION(player));
    while ((newparent != NOTHING) && !(FLAGS(newparent) & ABODE))
	newparent = LOCATION(newparent);
    if (newparent == NOTHING)
	newparent = tp_default_room_parent;

    NAME(room) = alloc_string(name);
    LOCATION(room) = newparent;
    OWNER(room) = OWNER(player);
    EXITS(room) = NOTHING;
    DBFETCH(room)->sp.room.dropto = NOTHING;
    FLAGS(room) = TYPE_ROOM | (FLAGS(player) & JUMP_OK);
    PUSH(room, CONTENTS(newparent));
    DBDIRTY(room);
    DBDIRTY(newparent);

    notifyf(player, "Room %s created as #%d.", name, room);

    strcpyn(buf, sizeof(buf), pname);
    for (rname = buf; (*rname && (*rname != ARG_DELIMITER)); rname++) ;
    qname = rname;
    if (*rname)
	*(rname++) = '\0';
    while ((qname > buf) && (isspace(*qname)))
	*(qname--) = '\0';
    qname = buf;
    for (; *rname && isspace(*rname); rname++) ;
    rname = strcpyn(rbuf, sizeof(rbuf), rname);
    qname = strcpyn(qbuf, sizeof(qbuf), qname);

    if (*qname) {
	notify(player, "Trying to set parent...");
	init_match(descr, player, qname, TYPE_ROOM, &md);
	match_absolute(&md);
	match_registered(&md);
	match_here(&md);
	if ((parent = noisy_match_result(&md)) == NOTHING || parent == AMBIGUOUS) {
	    notify(player, "Parent set to default.");
	} else {
	    if (!can_link_to(player, Typeof(room), parent) || room == parent) {
		notify(player, "Permission denied.  Parent set to default.");
	    } else {
                char unparse_buf[BUFFER_LEN];
		moveto(room, parent);
                unparse_object(player, parent, unparse_buf, sizeof(unparse_buf));
		notifyf(player, "Parent set to %s.", unparse_buf);
	    }
	}
    }

    if (*rname) {
        register_object(player, REGISTRATION_PROPDIR, rname, room);
        notifyf(player, "Registered as $%s", rname);
    }
}

/*
  Use this to create a program.
  First, find a program that matches that name.  If there's one,
  then we put him into edit mode and do it.
  Otherwise, we create a new object for him, and call it a program.
  */
void
do_prog(int descr, dbref player, const char *name)
{
    dbref i;
    dbref newprog;
    char unparse_buf[BUFFER_LEN];
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

    if ((i = match_result(&md)) == NOTHING) {
	if (!ok_ascii_other(name)) {
	    notify(player, "Program names are limited to 7-bit ASCII.");
	    return;
	}
	if (!ok_name(name)) {
	    notify(player, "That's a strange name for a program!");
	    return;
	}
	newprog = create_program(player, name);
        unparse_object(player, newprog, unparse_buf, sizeof(unparse_buf));
	notifyf(player, "Entering editor for new program %s.",
                unparse_buf);
    } else if (i == AMBIGUOUS) {
	notify(player, "I don't know which one you mean!");
	return;
    } else {
	if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
	    notify(player, "Permission denied!");
	    return;
	}
	if (FLAGS(i) & INTERNAL) {
	    notify(player,
		   "Sorry, this program is currently being edited by someone else.  Try again later.");
	    return;
	}
	PROGRAM_SET_FIRST(i, read_program(i));
	FLAGS(i) |= INTERNAL;
	PLAYER_SET_CURR_PROG(player, i);
        unparse_object(player, i, unparse_buf, sizeof(unparse_buf));
	notifyf(player, "Entering editor for %s.", unparse_buf);
	/* list current line */
	list_program(player, i, NULL, 0);
	DBDIRTY(i);
    }
    FLAGS(player) |= INTERACTIVE;
    DBDIRTY(player);
}

void
do_edit(int descr, dbref player, const char *name)
{
    dbref i;
    char unparse_buf[BUFFER_LEN];
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

    if ((i = noisy_match_result(&md)) == NOTHING || i == AMBIGUOUS)
	return;

    if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
	notify(player, "Permission denied!");
	return;
    }
    if (FLAGS(i) & INTERNAL) {
	notify(player,
	       "Sorry, this program is currently being edited by someone else.  Try again later.");
	return;
    }
    FLAGS(i) |= INTERNAL;
    PROGRAM_SET_FIRST(i, read_program(i));
    PLAYER_SET_CURR_PROG(player, i);
    unparse_object(player, i, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Entering editor for %s.", unparse_buf);
    /* list current line */
    list_program(player, i, NULL, 0);
    FLAGS(player) |= INTERACTIVE;
    DBDIRTY(i);
    DBDIRTY(player);
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

	/* data, however, must be cloned in case it's a string or a
	   lock. */
	switch (PropType(currprop)) {
	case PROP_STRTYP:
	    newprop.data.str = alloc_string((currprop->data).str);
	    break;
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
do_clone(int descr, dbref player, char *name)
{
    dbref thing, clonedthing;
    int cost;
    struct match_data md;

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

    if (thing == AMBIGUOUS) {
	notify(player, "I don't know which one you mean!");
	return;
    }

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
    } else {
	if (tp_verbose_clone) {
            char unparse_buf[BUFFER_LEN];
            unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "Now cloning %s...", unparse_buf);
	}

	/* create the object */
	clonedthing = new_object();

	/* initialize everything */
	NAME(clonedthing) = alloc_string(NAME(thing));
	ALLOC_THING_SP(clonedthing);
	LOCATION(clonedthing) = player;
	OWNER(clonedthing) = OWNER(player);
	SETVALUE(clonedthing, GETVALUE(thing));
/* FIXME: should we clone attached actions? */
	EXITS(clonedthing) = NOTHING;
	FLAGS(clonedthing) = FLAGS(thing);

	/* copy all properties */
	copy_props(player, thing, clonedthing, "");

	/* endow the object */
	if (GETVALUE(thing) > tp_max_object_endowment) {
	    SETVALUE(thing, tp_max_object_endowment);
	}

	/* Home, sweet home */
	THING_SET_HOME(clonedthing, THING_HOME(thing));

	/* link it in */
	PUSH(clonedthing, CONTENTS(player));
	DBDIRTY(player);

	/* and we're done */
	notifyf(player, "Object %s cloned as #%d.", NAME(thing), clonedthing);
	DBDIRTY(clonedthing);
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
    dbref loc;
    dbref thing;
    int cost;

    char buf2[BUFFER_LEN];
    char *rname, *qname;

    strcpyn(buf2, sizeof(buf2), acost);
    for (rname = buf2; (*rname && (*rname != ARG_DELIMITER)); rname++) ;
    qname = rname;
    if (*rname)
	*(rname++) = '\0';
    while ((qname > buf2) && (isspace(*qname)))
	*(qname--) = '\0';
    qname = buf2;
    for (; *rname && isspace(*rname); rname++) ;

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
    } else {
	/* create the object */
	thing = new_object();

	/* initialize everything */
	NAME(thing) = alloc_string(name);
	ALLOC_THING_SP(thing);
	LOCATION(thing) = player;
	OWNER(thing) = OWNER(player);
	SETVALUE(thing, OBJECT_ENDOWMENT(cost));
	EXITS(thing) = NOTHING;
	FLAGS(thing) = TYPE_THING;

	/* endow the object */
	if (GETVALUE(thing) > tp_max_object_endowment) {
	    SETVALUE(thing, tp_max_object_endowment);
	}
	if ((loc = LOCATION(player)) != NOTHING && controls(player, loc)) {
	    THING_SET_HOME(thing, loc);	/* home */
	} else {
	    THING_SET_HOME(thing, player);	/* home */
	    /* set thing's home to player instead */
	}

	/* link it in */
	PUSH(thing, CONTENTS(player));
	DBDIRTY(player);

	/* and we're done */
	notifyf(player, "Object %s created as #%d.", name, thing);
	DBDIRTY(thing);
    }

    if (*rname) {
        register_object(player, REGISTRATION_PROPDIR, rname, thing);
        notifyf(player, "Registered as $%s", rname);
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
    char *rname, *qname;

    strcpyn(buf2, sizeof(buf2), source_name);
    for (rname = buf2; (*rname && (*rname != ARG_DELIMITER)); rname++) ;
    qname = rname;
    if (*rname)
	*(rname++) = '\0';
    while ((qname > buf2) && (isspace(*qname)))
	*(qname--) = '\0';
    qname = buf2;
    for (; *rname && isspace(*rname); rname++) ;

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

    notifyf(player, "Action %s created as #%d and attached.", NAME(action), action);

    if (tp_autolink_actions) {
	notify(player, "Linked to NIL.");
    }

    if (*rname) {
        register_object(player, REGISTRATION_PROPDIR, rname, action);
        notifyf(player, "Registered as $%s", rname);
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

    if (LOCATION(player) == NOTHING)
	return;

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

    if (!unset_source(player, action)) {
	return;
    }
    set_source(player, action, source);
    notify(player, "Action re-attached.");
    if (MLevRaw(action)) {
	SetMLevel(action, 0);
	notify(player, "Action priority Level reset to zero.");
    }
}
