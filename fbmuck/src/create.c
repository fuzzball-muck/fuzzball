#include "config.h"

/* Commands that create new objects */

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"
#include "match.h"
#include <ctype.h>

struct line *read_program(dbref i);

/* parse_linkable_dest()
 *
 * A utility for open and link which checks whether a given destination
 * string is valid.  It returns a parsed dbref on success, and NOTHING
 * on failure.
 */

static dbref
parse_linkable_dest(int descr, dbref player, dbref exit, const char *dest_name)
{
	dbref dobj;					/* destination room/player/thing/link */
	static char buf[BUFFER_LEN];
	struct match_data md;

	init_match(descr, player, dest_name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);
	match_home(&md);

	if ((dobj = match_result(&md)) == NOTHING || dobj == AMBIGUOUS) {
		snprintf(buf, sizeof(buf), "I couldn't find '%s'.", dest_name);
		notify(player, buf);
		return NOTHING;

	}

	if (!tp_teleport_to_player && Typeof(dobj) == TYPE_PLAYER) {
		snprintf(buf, sizeof(buf), "You can't link to players.  Destination %s ignored.",
				unparse_object(player, dobj));
		notify(player, buf);
		return NOTHING;
	}

	if (!can_link(player, exit)) {
		notify(player, "You can't link that.");
		return NOTHING;
	}
	if (!can_link_to(player, Typeof(exit), dobj)) {
		snprintf(buf, sizeof(buf), "You can't link to %s.", unparse_object(player, dobj));
		notify(player, buf);
		return NOTHING;
	} else {
		return dobj;
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

	int i;

	if (source == dest)
		return 1;				/* That's an easy one! */
	if (Typeof(dest) != TYPE_EXIT)
		return 0;
	for (i = 0; i < DBFETCH(dest)->sp.exit.ndest; i++) {
		if ((DBFETCH(dest)->sp.exit.dest)[i] == source) {
			return 1;			/* Found a loop! */
		}
		if (Typeof((DBFETCH(dest)->sp.exit.dest)[i]) == TYPE_EXIT) {
			if (exit_loop_check(source, (DBFETCH(dest)->sp.exit.dest)[i])) {
				return 1;		/* Found one recursively */
			}
		}
	}
	return 0;					/* No loops found */
}

/* use this to create an exit */
void
do_open(int descr, dbref player, const char *direction, const char *linkto)
{
	dbref loc, exit;
	dbref good_dest[MAX_LINKS];
	char buf[BUFFER_LEN];
	char buf2[BUFFER_LEN];
	char *rname, *qname;
	int i, ndest;

	strcpyn(buf2, sizeof(buf2), linkto);
	for (rname = buf2; (*rname && (*rname != '=')); rname++) ;
	qname = rname;
	if (*rname)
		rname++;
	*qname = '\0';

	while (((qname--) > buf2) && (isspace(*qname)))
		*qname = '\0';
	qname = buf2;
	for (; *rname && isspace(*rname); rname++) ;

	if ((loc = getloc(player)) == NOTHING)
		return;
	if (!*direction) {
		notify(player, "You must specify a direction or action name to open.");
		return;
	}
	if(!ok_ascii_other(direction)) {
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
		notify_fmt(player, "Sorry, you don't have enough %s to open an exit.", tp_pennies);
		return;
	} else {
		/* create the exit */
		exit = new_object();

		/* initialize everything */
		NAME(exit) = alloc_string(direction);
		DBFETCH(exit)->location = loc;
		OWNER(exit) = OWNER(player);
		FLAGS(exit) = TYPE_EXIT;
		DBFETCH(exit)->sp.exit.ndest = 0;
		DBFETCH(exit)->sp.exit.dest = NULL;

		/* link it in */
		PUSH(exit, DBFETCH(loc)->exits);
		DBDIRTY(loc);

		/* and we're done */
		snprintf(buf, sizeof(buf), "Exit opened with number %d.", exit);
		notify(player, buf);

		/* check second arg to see if we should do a link */
		if (*qname != '\0') {
			notify(player, "Trying to link...");
			if (!payfor(player, tp_link_cost)) {
				notify_fmt(player, "You don't have enough %s to link.", tp_pennies);
			} else {
				ndest = link_exit(descr, player, exit, (char *) qname, good_dest);
				DBFETCH(exit)->sp.exit.ndest = ndest;
				DBFETCH(exit)->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * ndest);
				for (i = 0; i < ndest; i++) {
					(DBFETCH(exit)->sp.exit.dest)[i] = good_dest[i];
				}
				DBDIRTY(exit);
			}
		}
	}

	if (*rname) {
		PData mydat;

		snprintf(buf, sizeof(buf), "Registered as $%s", rname);
		notify(player, buf);
		snprintf(buf, sizeof(buf), "_reg/%s", rname);
		mydat.flags = PROP_REFTYP;
		mydat.data.ref = exit;
		set_property(player, buf, &mydat);
	}
}

int
_link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list, int dryrun)
{
	char *p, *q;
	int prdest;
	dbref dest;
	int ndest, error;
	char buf[BUFFER_LEN], qbuf[BUFFER_LEN];

	prdest = 0;
	ndest = 0;
	error = 0;

	while (*dest_name) {
		while (isspace(*dest_name))
			dest_name++;		/* skip white space */
		p = dest_name;
		while (*dest_name && (*dest_name != EXIT_DELIMITER))
			dest_name++;
		q = (char *) strncpy(qbuf, p, BUFFER_LEN);	/* copy word */
		q[(dest_name - p)] = '\0';	/* terminate it */
		if (*dest_name)
			for (dest_name++; *dest_name && isspace(*dest_name); dest_name++) ;

		if ((dest = parse_linkable_dest(descr, player, exit, q)) == NOTHING)
			continue;

		switch (Typeof(dest)) {
		case TYPE_PLAYER:
		case TYPE_ROOM:
		case TYPE_PROGRAM:
			if (prdest) {
				snprintf(buf, sizeof(buf),
						"Only one player, room, or program destination allowed. Destination %s ignored.",
						unparse_object(player, dest));
				notify(player, buf);

				if(dryrun)
					error = 1;

				continue;
			}
			dest_list[ndest++] = dest;
			prdest = 1;
			break;
		case TYPE_THING:
			dest_list[ndest++] = dest;
			break;
		case TYPE_EXIT:
			if (exit_loop_check(exit, dest)) {
				snprintf(buf, sizeof(buf),
						"Destination %s would create a loop, ignored.",
						unparse_object(player, dest));
				notify(player, buf);
				
				if(dryrun)
					error = 1;

				continue;
			}
			dest_list[ndest++] = dest;
			break;
		default:
			notify(player, "Internal error: weird object type.");
			log_status("PANIC: weird object: Typeof(%d) = %d", dest, Typeof(dest));

			if(dryrun)
				error = 1;
				
			break;
		}
		if(!dryrun) {
			if (dest == HOME) {
				notify(player, "Linked to HOME.");
			} else {
				snprintf(buf, sizeof(buf), "Linked to %s.", unparse_object(player, dest));
				notify(player, buf);
			}
		}
		
		if (ndest >= MAX_LINKS) {
			notify(player, "Too many destinations, rest ignored.");

			if(dryrun)
				error = 1;

			break;
		}
	}
	
	if(dryrun && error)
		return 0;
		
	return ndest;
}

/*
 * link_exit()
 *
 * This routine connects an exit to a bunch of destinations.
 *
 * 'player' contains the player's name.
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of exits.
 *
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.
 *
 */

int
link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list)
{
	return _link_exit(descr, player, exit, dest_name, dest_list, 0);
}

/*
 * link_exit_dry()
 *
 * like link_exit(), but only checks whether the link would be ok or not.
 * error messages are still output.
 */
int
link_exit_dry(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list)
{
	return _link_exit(descr, player, exit, dest_name, dest_list, 1);
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

	int ndest, i;

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
				notify(player, "That exit is already linked.");
				return;
			} else {
				notify(player, "Permission denied. (you don't control the exit to relink)");
				return;
			}
		}
		/* handle costs */
		if (OWNER(thing) == OWNER(player)) {
			if (!payfor(player, tp_link_cost)) {
				notify_fmt(player, "It costs %d %s to link this exit.",
						   tp_link_cost, (tp_link_cost == 1) ? tp_penny : tp_pennies);
				return;
			}
		} else {
			if (!payfor(player, tp_link_cost + tp_exit_cost)) {
				notify_fmt(player, "It costs %d %s to link this exit.",
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
		for (i = 0; i < ndest; i++) {
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
			notify(player, "Permission denied. (you don't control the thing, or you can't link to dest)");
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
	case TYPE_ROOM:			/* room dropto's */
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
			notify(player, "Permission denied. (you don't control the room, or can't link to the dropto)");
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
	if(!ok_ascii_other(name)) {
		notify(player, "Room names are limited to 7-bit ASCII.");
		return;
	}
	if (!ok_name(name)) {
		notify(player, "That's a strange name for a room!");
		return;
	}
	if (!payfor(player, tp_room_cost)) {
		notify_fmt(player, "Sorry, you don't have enough %s to dig a room.", tp_pennies);
		return;
	}
	room = new_object();

	/* Initialize everything */
	newparent = DBFETCH(DBFETCH(player)->location)->location;
	while ((newparent != NOTHING) && !(FLAGS(newparent) & ABODE))
		newparent = DBFETCH(newparent)->location;
	if (newparent == NOTHING)
		newparent = tp_default_room_parent;

	NAME(room) = alloc_string(name);
	DBFETCH(room)->location = newparent;
	OWNER(room) = OWNER(player);
	DBFETCH(room)->exits = NOTHING;
	DBFETCH(room)->sp.room.dropto = NOTHING;
	FLAGS(room) = TYPE_ROOM | (FLAGS(player) & JUMP_OK);
	PUSH(room, DBFETCH(newparent)->contents);
	DBDIRTY(room);
	DBDIRTY(newparent);

	snprintf(buf, sizeof(buf), "%s created with room number %d.", name, room);
	notify(player, buf);

	strcpyn(buf, sizeof(buf), pname);
	for (rname = buf; (*rname && (*rname != '=')); rname++) ;
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
				moveto(room, parent);
				snprintf(buf, sizeof(buf), "Parent set to %s.", unparse_object(player, parent));
				notify(player, buf);
			}
		}
	}

	if (*rname) {
		PData mydat;

		snprintf(buf, sizeof(buf), "_reg/%s", rname);
		mydat.flags = PROP_REFTYP;
		mydat.data.ref = room;
		set_property(player, buf, &mydat);
		snprintf(buf, sizeof(buf), "Room registered as $%s", rname);
		notify(player, buf);
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
	int jj;
	dbref newprog;
	char buf[BUFFER_LEN];
	struct match_data md;

	if (!*name) {
		notify(player, "No program name given.");
		return;
	}
	if(!ok_ascii_other(name)) {
		notify(player, "Program names are limited to 7-bit ASCII.");
		return;
	}
	if (!ok_name(name)) {
		notify(player, "That's a strange name for a program!");
		return;
	}
	init_match(descr, player, name, TYPE_PROGRAM, &md);

	match_possession(&md);
	match_neighbor(&md);
	match_registered(&md);
	match_absolute(&md);

	if ((i = match_result(&md)) == NOTHING) {
		newprog = new_object();

		NAME(newprog) = alloc_string(name);
		snprintf(buf, sizeof(buf), "A scroll containing a spell called %s", name);
		SETDESC(newprog, buf);
		DBFETCH(newprog)->location = player;
		FLAGS(newprog) = TYPE_PROGRAM;
		jj = MLevel(player);
		if (jj < 1)
			jj = 2;
		if (jj > 3)
			jj = 3;
		SetMLevel(newprog, jj);

		OWNER(newprog) = OWNER(player);
		ALLOC_PROGRAM_SP(newprog);
		PROGRAM_SET_FIRST(newprog, NULL);
		PROGRAM_SET_CURR_LINE(newprog, 0);
		PROGRAM_SET_SIZ(newprog, 0);
		PROGRAM_SET_CODE(newprog, NULL);
		PROGRAM_SET_START(newprog, NULL);
		PROGRAM_SET_PUBS(newprog, NULL);
		PROGRAM_SET_MCPBINDS(newprog, NULL);
		PROGRAM_SET_PROFTIME(newprog, 0, 0);
		PROGRAM_SET_PROFSTART(newprog, 0);
		PROGRAM_SET_PROF_USES(newprog, 0);
		PROGRAM_SET_INSTANCES(newprog, 0);

		PLAYER_SET_CURR_PROG(player, newprog);

		PUSH(newprog, DBFETCH(player)->contents);
		DBDIRTY(newprog);
		DBDIRTY(player);
		snprintf(buf, sizeof(buf), "Program %s created with number %d.", name, newprog);
		notify(player, buf);
		snprintf(buf, sizeof(buf), "Entering editor.");
		notify(player, buf);
	} else if (i == AMBIGUOUS) {
		notify(player, "I don't know which one you mean!");
		return;
	} else {
		if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
			notify(player, "Permission denied!");
			return;
		}
		if (FLAGS(i) & INTERNAL) {
			notify(player, "Sorry, this program is currently being edited by someone else.  Try again later.");
			return;
		}
		PROGRAM_SET_FIRST(i, read_program(i));
		FLAGS(i) |= INTERNAL;
		PLAYER_SET_CURR_PROG(player, i);
		notify(player, "Entering editor.");
		/* list current line */
		do_list(player, i, NULL, 0);
		DBDIRTY(i);
	}
	FLAGS(player) |= INTERACTIVE;
	DBDIRTY(player);
}

void
do_edit(int descr, dbref player, const char *name)
{
	dbref i;
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
		notify(player, "Sorry, this program is currently being edited by someone else.  Try again later.");
		return;
	}
	FLAGS(i) |= INTERNAL;
	PROGRAM_SET_FIRST(i, read_program(i));
	PLAYER_SET_CURR_PROG(player, i);
	notify(player, "Entering editor.");
	/* list current line */
	do_list(player, i, NULL, 0);
	FLAGS(player) |= INTERACTIVE;
	DBDIRTY(i);
	DBDIRTY(player);
}

#ifdef MCP_SUPPORT
void
mcpedit_program(int descr, dbref player, dbref prog, const char* name)
{
	char namestr[BUFFER_LEN];
	char refstr[BUFFER_LEN];
	struct line *curr;
	McpMesg msg;
	McpFrame *mfr;
	McpVer supp;

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		do_edit(descr, player, name);
		return;
	}

	supp = mcp_frame_package_supported(mfr, "dns-org-mud-moo-simpleedit");

	if (supp.verminor == 0 && supp.vermajor == 0) {
		do_edit(descr, player, name);
		return;
	}

	if ((Typeof(prog) != TYPE_PROGRAM) || !controls(player, prog)) {
		show_mcp_error(mfr, "@mcpedit", "Permission denied!");
		return;
	}
	if (FLAGS(prog) & INTERNAL) {
		show_mcp_error(mfr, "@mcpedit", "Sorry, this program is currently being edited by someone else.  Try again later.");
		return;
	}
	PROGRAM_SET_FIRST(prog, read_program(prog));
	PLAYER_SET_CURR_PROG(player, prog);

	snprintf(refstr, sizeof(refstr), "%d.prog.", prog);
	snprintf(namestr, sizeof(namestr), "a program named %s(%d)", NAME(prog), prog);
	mcp_mesg_init(&msg, "dns-org-mud-moo-simpleedit", "content");
	mcp_mesg_arg_append(&msg, "reference", refstr);
	mcp_mesg_arg_append(&msg, "type", "muf-code");
	mcp_mesg_arg_append(&msg, "name", namestr);
	for (curr = PROGRAM_FIRST(prog); curr; curr = curr->next) {
		mcp_mesg_arg_append(&msg, "content", DoNull(curr->this_line));
	}
	mcp_frame_output_mesg(mfr, &msg);
	mcp_mesg_clear(&msg);

	free_prog_text(PROGRAM_FIRST(prog));
	PROGRAM_SET_FIRST(prog, NULL);
}

void
do_mcpedit(int descr, dbref player, const char *name)
{
	dbref prog;
	struct match_data md;
	McpFrame *mfr;
	McpVer supp;

	if (!*name) {
		notify(player, "No program name given.");
		return;
	}

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		do_edit(descr, player, name);
		return;
	}

	supp = mcp_frame_package_supported(mfr, "dns-org-mud-moo-simpleedit");

	if (supp.verminor == 0 && supp.vermajor == 0) {
		do_edit(descr, player, name);
		return;
	}

	init_match(descr, player, name, TYPE_PROGRAM, &md);

	match_possession(&md);
	match_neighbor(&md);
	match_registered(&md);
	match_absolute(&md);

	prog = match_result(&md);
	if (prog == NOTHING) {
		/* FIXME: must arrange this to query user. */
		notify(player, "I don't see that here!");
		return;
	}

	if (prog == AMBIGUOUS) {
		notify(player, "I don't know which one you mean!");
		return;
	}

	mcpedit_program(descr, player, prog, name);
}


void
do_mcpprogram(int descr, dbref player, const char* name)
{
	dbref prog;
	char buf[BUFFER_LEN];
	struct match_data md;
	int jj;

	if (!*name) {
		notify(player, "No program name given.");
		return;
	}
	if(!ok_ascii_other(name)) {
		notify(player, "Program names are limited to 7-bit ASCII.");
		return;
	}
	if (!ok_name(name)) {
		notify(player, "That's a strange name for a program!");
		return;
	}
	init_match(descr, player, name, TYPE_PROGRAM, &md);

	match_possession(&md);
	match_neighbor(&md);
	match_registered(&md);
	match_absolute(&md);

	prog = match_result(&md);

	if (prog == NOTHING) {
		prog = new_object();

		NAME(prog) = alloc_string(name);
		snprintf(buf, sizeof(buf), "A scroll containing a spell called %s", name);
		SETDESC(prog, buf);
		DBFETCH(prog)->location = player;
		FLAGS(prog) = TYPE_PROGRAM;
		jj = MLevel(player);
		if (jj < 1)
			jj = 2;
		if (jj > 3)
			jj = 3;
		SetMLevel(prog, jj);

		OWNER(prog) = OWNER(player);
		ALLOC_PROGRAM_SP(prog);
		PROGRAM_SET_FIRST(prog, NULL);
		PROGRAM_SET_CURR_LINE(prog, 0);
		PROGRAM_SET_SIZ(prog, 0);
		PROGRAM_SET_CODE(prog, NULL);
		PROGRAM_SET_START(prog, NULL);
		PROGRAM_SET_PUBS(prog, NULL);
		PROGRAM_SET_MCPBINDS(prog, NULL);
		PROGRAM_SET_PROFTIME(prog, 0, 0);
		PROGRAM_SET_PROFSTART(prog, 0);
		PROGRAM_SET_PROF_USES(prog, 0);
		PROGRAM_SET_INSTANCES(prog, 0);

		PLAYER_SET_CURR_PROG(player, prog);

		PUSH(prog, DBFETCH(player)->contents);
		DBDIRTY(prog);
		DBDIRTY(player);
		snprintf(buf, sizeof(buf), "Program %s created with number %d.", name, prog);
		notify(player, buf);

	} else if (prog == AMBIGUOUS) {
		notify(player, "I don't know which one you mean!");
		return;
	}

	mcpedit_program(descr, player, prog, name);
}
#endif
/*
 * copy a single property, identified by its name, from one object to
 * another. helper routine for copy_props (below).
 */
void
copy_one_prop(dbref player, dbref source, dbref destination, char *propname)
{
	PropPtr currprop;
	PData newprop;

	/* read property from old object */
	currprop = get_property(source, propname);

#ifdef DISKBASE
	/* make sure the property value is there */
	propfetch(source, currprop);
#endif

	if(currprop) {

		/* flags can be copied. */		
		newprop.flags = currprop->flags;

		/* data, however, must be cloned in case it's a string or a
		   lock. */
		switch(PropType(currprop)) {
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
		set_property(destination, propname, &newprop);
	}
	
	return;
}

/*
 * copy a property (sub)tree from one object to another one. this is a
 * helper routine used by do_clone, based loosely on listprops_wildcard from
 * look.c.
 */
void
copy_props(dbref player, dbref source, dbref destination, const char *dir)
{
	char propname[BUFFER_LEN];
	char buf[BUFFER_LEN];
	char buf2[BUFFER_LEN];
	PropPtr propadr, pptr;

	/* loop through all properties in the current propdir */
	propadr = first_prop(source, (char *) dir, &pptr, propname, sizeof(propname));
	while (propadr) {
	
		/* generate name for current property */
		snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);

		/* notify player */
		if(tp_verbose_clone && Wizard(OWNER(player))) {
			snprintf(buf2, sizeof(buf2), "copying property %s", buf);
			notify(player, buf2);
		}

		/* copy this property */
		copy_one_prop(player, source, destination, buf);
		
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
	static char buf[BUFFER_LEN];
	dbref  thing, clonedthing;
	int    cost;
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
	if(Typeof(thing) != TYPE_THING) {
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
	if(!controls(player, thing)) {
		notify(player, "Permission denied. (you can't clone this)");
		return;
	}

	/* there ain't no such lunch as a free thing. */
	cost = OBJECT_GETCOST(GETVALUE(thing));
	if (cost < tp_object_cost) {
		cost = tp_object_cost;
	}
	
	if (!payfor(player, cost)) {
		notify_fmt(player, "Sorry, you don't have enough %s.", tp_pennies);
		return;
	} else {
		if(tp_verbose_clone) {
			snprintf(buf, sizeof(buf), "Now cloning %s...", unparse_object(player, thing));
			notify(player, buf);
		}
		
		/* create the object */
		clonedthing = new_object();

		/* initialize everything */
		NAME(clonedthing) = alloc_string(NAME(thing));
		ALLOC_THING_SP(clonedthing);
		DBFETCH(clonedthing)->location = player;
		OWNER(clonedthing) = OWNER(player);
		SETVALUE(clonedthing, GETVALUE(thing));
/* FIXME: should we clone attached actions? */
		DBFETCH(clonedthing)->exits = NOTHING;
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
		PUSH(clonedthing, DBFETCH(player)->contents);
		DBDIRTY(player);

		/* and we're done */
		snprintf(buf, sizeof(buf), "%s created with number %d.", NAME(thing), clonedthing);
		notify(player, buf);
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

	static char buf[BUFFER_LEN];
	char buf2[BUFFER_LEN];
	char *rname, *qname;

	strcpyn(buf2, sizeof(buf2), acost);
	for (rname = buf2; (*rname && (*rname != '=')); rname++) ;
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
	} else if(!ok_ascii_thing(name)) {
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
		notify_fmt(player, "Sorry, you don't have enough %s.", tp_pennies);
		return;
	} else {
		/* create the object */
		thing = new_object();

		/* initialize everything */
		NAME(thing) = alloc_string(name);
		ALLOC_THING_SP(thing);
		DBFETCH(thing)->location = player;
		OWNER(thing) = OWNER(player);
		SETVALUE(thing, OBJECT_ENDOWMENT(cost));
		DBFETCH(thing)->exits = NOTHING;
		FLAGS(thing) = TYPE_THING;

		/* endow the object */
		if (GETVALUE(thing) > tp_max_object_endowment) {
			SETVALUE(thing, tp_max_object_endowment);
		}
		if ((loc = DBFETCH(player)->location) != NOTHING && controls(player, loc)) {
			THING_SET_HOME(thing, loc);	/* home */
		} else {
			THING_SET_HOME(thing, player);	/* home */
			/* set thing's home to player instead */
		}

		/* link it in */
		PUSH(thing, DBFETCH(player)->contents);
		DBDIRTY(player);

		/* and we're done */
		snprintf(buf, sizeof(buf), "%s created with number %d.", name, thing);
		notify(player, buf);
		DBDIRTY(thing);
	}
	if (*rname) {
		PData mydat;

		snprintf(buf, sizeof(buf), "Registered as $%s", rname);
		notify(player, buf);
		snprintf(buf, sizeof(buf), "_reg/%s", rname);
		mydat.flags = PROP_REFTYP;
		mydat.data.ref = thing;
		set_property(player, buf, &mydat);
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
dbref
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
 * set_source()
 *
 * This routine sets the source of an action to the specified source.
 * It is called by do_action and do_attach.
 *
 */
void
set_source(dbref player, dbref action, dbref source)
{
	switch (Typeof(source)) {
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_PLAYER:
		PUSH(action, DBFETCH(source)->exits);
		break;
	default:
		notify(player, "Internal error: weird object type.");
		log_status("PANIC: tried to source %d to %d: type: %d",
				   action, source, Typeof(source));
		return;
		break;
	}
	DBDIRTY(source);
	DBSTORE(action, location, source);
	return;
}

int
unset_source(dbref player, dbref loc, dbref action)
{

	dbref oldsrc;

	if ((oldsrc = DBFETCH(action)->location) == NOTHING) {
		/* old-style, sourceless exit */
		if (!member(action, DBFETCH(loc)->exits)) {
			return 0;
		}
		DBSTORE(DBFETCH(player)->location, exits,
				remove_first(DBFETCH(DBFETCH(player)->location)->exits, action));
	} else {
		switch (Typeof(oldsrc)) {
		case TYPE_PLAYER:
		case TYPE_ROOM:
		case TYPE_THING:
			DBSTORE(oldsrc, exits, remove_first(DBFETCH(oldsrc)->exits, action));
			break;
		default:
			log_status("PANIC: source of action #%d was type: %d.", action, Typeof(oldsrc));
			return 0;
			/* NOTREACHED */
			break;
		}
	}
	return 1;
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
	static char buf[BUFFER_LEN];
	char buf2[BUFFER_LEN];
	char *rname, *qname;

	strcpyn(buf2, sizeof(buf2), source_name);
	for (rname = buf2; (*rname && (*rname != '=')); rname++) ;
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
	} else if(!ok_ascii_other(action_name)) {
		notify(player, "Action names are limited to 7-bit ASCII.");
		return;
	} else if (!ok_name(action_name)) {
		notify(player, "That's a strange name for an action!");
		return;
	}
	if (((source = parse_source(descr, player, qname)) == NOTHING))
		return;
        if (!payfor(player, tp_exit_cost)) {
                notify_fmt(player, "Sorry, you don't have enough %s to make an action.", tp_pennies);
                return;
        }

	action = new_object();

	NAME(action) = alloc_string(action_name);
	DBFETCH(action)->location = NOTHING;
	OWNER(action) = OWNER(player);
	DBFETCH(action)->sp.exit.ndest = 0;
	DBFETCH(action)->sp.exit.dest = NULL;
	FLAGS(action) = TYPE_EXIT;

	set_source(player, action, source);
	snprintf(buf, sizeof(buf), "Action created with number %d and attached.", action);
	notify(player, buf);
	DBDIRTY(action);

	if (*rname) {
		PData mydat;

		snprintf(buf, sizeof(buf), "Registered as $%s", rname);
		notify(player, buf);
		snprintf(buf, sizeof(buf), "_reg/%s", rname);
		mydat.flags = PROP_REFTYP;
		mydat.data.ref = action;
		set_property(player, buf, &mydat);
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
	dbref loc;				/* player's current location */
	struct match_data md;

	if ((loc = DBFETCH(player)->location) == NOTHING)
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
		notify(player, "Permission denied. (you don't control the action you're trying to reattach)");
		return;
	}
	if (((source = parse_source(descr, player, source_name)) == NOTHING)
		|| Typeof(source) == TYPE_PROGRAM)
		return;

	if (!unset_source(player, loc, action)) {
		return;
	}
	set_source(player, action, source);
	notify(player, "Action re-attached.");
	if (MLevRaw(action)) {
		SetMLevel(action, 0);
		notify(player, "Action priority Level reset to zero.");
	}
}
