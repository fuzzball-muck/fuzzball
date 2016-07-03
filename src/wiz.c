#include "config.h"

#include "db.h"
#include "externs.h"
#include "interface.h"
#include "msgparse.h"
#include "params.h"
#include "props.h"
#include "tune.h"

#include <stdio.h>

#ifndef WIN32
# include <sys/resource.h>
#endif

#ifndef MALLOC_PROFILING
#  ifndef HAVE_MALLOC_H
#    include <stdlib.h>
#  else
#    include <malloc.h>
#  endif
#endif

void
do_teleport(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref victim;
    dbref destination;
    const char *to;
    struct match_data md;

    /* get victim, destination */
    if (*arg2 == '\0') {
	victim = player;
	to = arg1;
    } else {
	init_match(descr, player, arg1, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_registered(&md);
	match_player(&md);

	if ((victim = noisy_match_result(&md)) == NOTHING) {
	    return;
	}
	to = arg2;
    }
#ifdef GOD_PRIV
    if (tp_strict_god_priv && !God(player) && God(OWNER(victim))) {
	notify(player, "God has already set that where God wants it to be.");
	return;
    }
#endif

    /* get destination */
    init_match(descr, player, to, TYPE_PLAYER, &md);
    match_possession(&md);
    match_me(&md);
    match_here(&md);
    match_home(&md);
    match_absolute(&md);
    match_registered(&md);
    if (Wizard(OWNER(player))) {
	match_neighbor(&md);
	match_player(&md);
    }
    switch (destination = match_result(&md)) {
    case NOTHING:
	notify(player, "Send it where?");
	break;
    case AMBIGUOUS:
	notify(player, "I don't know which destination you mean!");
	break;
    case HOME:
	switch (Typeof(victim)) {
	case TYPE_PLAYER:
	    destination = PLAYER_HOME(victim);
	    if (parent_loop_check(victim, destination))
		destination = PLAYER_HOME(OWNER(victim));
	    break;
	case TYPE_THING:
	    destination = THING_HOME(victim);
	    if (parent_loop_check(victim, destination)) {
		destination = PLAYER_HOME(OWNER(victim));
		if (parent_loop_check(victim, destination)) {
		    destination = GLOBAL_ENVIRONMENT;
		}
	    }
	    break;
	case TYPE_ROOM:
	    destination = GLOBAL_ENVIRONMENT;
	    break;
	case TYPE_PROGRAM:
	    destination = OWNER(victim);
	    break;
	default:
	    destination = tp_player_start;	/* caught in the next
						 * switch anyway */
	    break;
	}
    default:
	switch (Typeof(victim)) {
	case TYPE_PLAYER:
	    if (!controls(player, victim) ||
		!controls(player, destination) ||
		!controls(player, LOCATION(victim)) ||
		(Typeof(destination) == TYPE_THING
		 && !controls(player, LOCATION(destination)))) {
		notify(player,
		       "Permission denied. (must control victim, dest, victim's loc, and dest's loc)");
		break;
	    }
	    if (Typeof(destination) != TYPE_ROOM && Typeof(destination) != TYPE_THING) {
		notify(player, "Bad destination.");
		break;
	    }
	    if (!Wizard(victim) &&
		(Typeof(destination) == TYPE_THING && !(FLAGS(destination) & VEHICLE))) {
		notify(player, "Destination object is not a vehicle.");
		break;
	    }
	    if (parent_loop_check(victim, destination)) {
		notify(player, "Objects can't contain themselves.");
		break;
	    }
	    notify(victim, "You feel a wrenching sensation...");
	    enter_room(descr, victim, destination, LOCATION(victim));
	    notifyf(player, "%s teleported to %s.", unparse_object(player, victim),
		    unparse_object(player, destination));
	    break;
	case TYPE_THING:
	    if (parent_loop_check(victim, destination)) {
		notify(player, "You can't make a container contain itself!");
		break;
	    }
	case TYPE_PROGRAM:
	    if (Typeof(destination) != TYPE_ROOM
		&& Typeof(destination) != TYPE_PLAYER && Typeof(destination) != TYPE_THING) {
		notify(player, "Bad destination.");
		break;
	    }
	    if (!((controls(player, destination) ||
		   can_link_to(player, NOTYPE, destination)) &&
		  (controls(player, victim) || controls(player, LOCATION(victim))))) {
		notify(player,
		       "Permission denied. (must control dest and be able to link to it, or control dest's loc)");
		break;
	    }
	    /* check for non-sticky dropto */
	    if (Typeof(destination) == TYPE_ROOM
		&& DBFETCH(destination)->sp.room.dropto != NOTHING
		&& !(FLAGS(destination) & STICKY))
		destination = DBFETCH(destination)->sp.room.dropto;
	    if (tp_thing_movement && (Typeof(victim) == TYPE_THING)) {
		if (FLAGS(victim) & ZOMBIE) {
		    notify(victim, "You feel a wrenching sensation...");
		}
		enter_room(descr, victim, destination, LOCATION(victim));
	    } else {
		moveto(victim, destination);
	    }
	    notifyf(player, "%s teleported to %s.", unparse_object(player, victim),
		    unparse_object(player, destination));
	    break;
	case TYPE_ROOM:
	    if (Typeof(destination) != TYPE_ROOM) {
		notify(player, "Bad destination.");
		break;
	    }
	    if (!controls(player, victim)
		|| !can_link_to(player, NOTYPE, destination)
		|| victim == GLOBAL_ENVIRONMENT) {
		notify(player,
		       "Permission denied. (Can't move #0, dest must be linkable, and must control victim)");
		break;
	    }
	    if (parent_loop_check(victim, destination)) {
		notify(player, "Parent would create a loop.");
		break;
	    }
	    moveto(victim, destination);
	    notifyf(player, "Parent of %s set to %s.", unparse_object(player, victim),
		    unparse_object(player, destination));
	    break;
	case TYPE_GARBAGE:
	    notify(player, "That object is in a place where magic cannot reach it.");
	    break;
	default:
	    notify(player, "You can't teleport that.");
	    break;
	}
	break;
    }
    return;
}

int
blessprops_wildcard(dbref player, dbref thing, const char *dir, const char *wild, int blessp)
{
    char propname[BUFFER_LEN];
    char wld[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char *ptr, *wldcrd;
    PropPtr propadr, pptr;
    int i, cnt = 0;
    int recurse = 0;

#ifdef GOD_PRIV
    if (tp_strict_god_priv && !God(player) && God(OWNER(thing))) {
	notify(player, "Only God may touch what is God's.");
	return 0;
    }
#endif

    strcpyn(wld, sizeof(wld), wild);
    i = strlen(wld);
    if (i && wld[i - 1] == PROPDIR_DELIMITER)
	strcatn(wld, sizeof(wld), "*");
    for (wldcrd = wld; *wldcrd == PROPDIR_DELIMITER; wldcrd++) ;
    if (!strcmp(wldcrd, "**"))
	recurse = 1;

    for (ptr = wldcrd; *ptr && *ptr != PROPDIR_DELIMITER; ptr++) ;
    if (*ptr)
	*ptr++ = '\0';

    propadr = first_prop(thing, (char *) dir, &pptr, propname, sizeof(propname));
    while (propadr) {
	if (equalstr(wldcrd, propname)) {
	    snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);
	    if (!Prop_System(buf) && (!Prop_Hidden(buf) || Wizard(OWNER(player)))) {
		if (!*ptr || recurse) {
		    cnt++;
		    if (blessp) {
			set_property_flags(thing, buf, PROP_BLESSED);
			snprintf(buf2, sizeof(buf2), "Blessed %s", buf);
		    } else {
			clear_property_flags(thing, buf, PROP_BLESSED);
			snprintf(buf2, sizeof(buf2), "Unblessed %s", buf);
		    }
		    notify(player, buf2);
		}
		if (recurse)
		    ptr = "**";
		cnt += blessprops_wildcard(player, thing, buf, ptr, blessp);
	    }
	}
	propadr = next_prop(pptr, propadr, propname, sizeof(propname));
    }
    return cnt;
}


void
do_unbless(int descr, dbref player, const char *what, const char *propname)
{
    dbref victim;
    struct match_data md;
    int cnt;

    if (!propname || !*propname) {
	notify(player, "Usage is @unbless object=propname.");
	return;
    }

    /* get victim */
    init_match(descr, player, what, NOTYPE, &md);
    match_everything(&md);
    if ((victim = noisy_match_result(&md)) == NOTHING) {
	return;
    }

    cnt = blessprops_wildcard(player, victim, "", propname, 0);
    notifyf(player, "%d propert%s unblessed.", cnt, (cnt == 1) ? "y" : "ies");
}


void
do_bless(int descr, dbref player, const char *what, const char *propname)
{
    dbref victim;
    struct match_data md;
    int cnt;

    if (!propname || !*propname) {
	notify(player, "Usage is @bless object=propname.");
	return;
    }

    /* get victim */
    init_match(descr, player, what, NOTYPE, &md);
    match_everything(&md);
    if ((victim = noisy_match_result(&md)) == NOTHING) {
	return;
    }
#ifdef GOD_PRIV
    if (tp_strict_god_priv && !God(player) && God(OWNER(victim))) {
	notify(player, "Only God may touch God's stuff.");
	return;
    }
#endif

    cnt = blessprops_wildcard(player, victim, "", propname, 1);
    notifyf(player, "%d propert%s blessed.", cnt, (cnt == 1) ? "y" : "ies");
}

void
do_force(int descr, dbref player, const char *what, char *command)
{
    dbref victim, loc;
    struct match_data md;

    assert(what != NULL);
    assert(command != NULL);
    assert(player > 0);

    if (force_level > (tp_max_force_level - 1)) {
	notify(player, "Can't force recursively.");
	return;
    }

    if (!tp_zombies && (!Wizard(player) || Typeof(player) != TYPE_PLAYER)) {
	notify(player, "Zombies are not enabled here.");
	return;
#ifdef DEBUG
    } else {
	notify(player, "[debug] Zombies are not enabled for nonwizards -- force succeeded.");
#endif
    }

    /* get victim */
    init_match(descr, player, what, NOTYPE, &md);
    match_neighbor(&md);
    match_possession(&md);
    match_me(&md);
    match_here(&md);
    match_absolute(&md);
    match_registered(&md);
    match_player(&md);

    if ((victim = noisy_match_result(&md)) == NOTHING) {
#ifdef DEBUG
	notify(player, "[debug] do_force: unable to find your target!");
#endif				/* DEBUG */
	return;
    }

    if (Typeof(victim) != TYPE_PLAYER && Typeof(victim) != TYPE_THING) {
	notify(player, "Permission Denied -- Target not a player or thing.");
	return;
    }
#ifdef GOD_PRIV
    if (God(victim)) {
	notify(player, "You cannot force God to do anything.");
	return;
    }
#endif				/* GOD_PRIV */

/*    if (!controls(player, victim)) {
 *	notify(player, "Permission denied. (you're not a wizard!)");
 *	return;
 *    }
 */

    if (!Wizard(player) && !(FLAGS(victim) & XFORCIBLE)) {
	notify(player, "Permission denied: forced object not @set Xforcible.");
	return;
    }
    if (!Wizard(player) && !test_lock_false_default(descr, player, victim, MESGPROP_FLOCK)) {
	notify(player, "Permission denied: Object not force-locked to you.");
	return;
    }

    loc = LOCATION(victim);
    if (!Wizard(player) && Typeof(victim) == TYPE_THING && loc != NOTHING &&
	(FLAGS(loc) & ZOMBIE) && Typeof(loc) == TYPE_ROOM) {
	notify(player, "Sorry, but that's in a no-puppet zone.");
	return;
    }

    if (!Wizard(OWNER(player)) && Typeof(victim) == TYPE_THING) {
	const char *ptr = NAME(victim);
	char objname[BUFFER_LEN], *ptr2;

	if ((FLAGS(player) & ZOMBIE)) {
	    notify(player, "Permission denied -- you cannot use zombies.");
	    return;
	}
	if (FLAGS(victim) & DARK) {
	    notify(player, "Permission denied -- you cannot force dark zombies.");
	    return;
	}
	for (ptr2 = objname; *ptr && !isspace(*ptr);)
	    *(ptr2++) = *(ptr++);
	*ptr2 = '\0';
	if (lookup_player(objname) != NOTHING) {
	    notify(player, "Puppet cannot share the name of a player.");
	    return;
	}
    }

    log_status("FORCED: %s(%d) by %s(%d): %s", NAME(victim),
	       victim, NAME(player), player, command);
    /* force victim to do command */
    force_prog = NOTHING;
    force_level++;
    process_command(dbref_first_descr(victim), victim, command);
    force_level--;
    force_prog = NOTHING;
}

void
do_stats(dbref player, const char *name)
{
    int rooms;
    int exits;
    int things;
    int players;
    int programs;
    int garbage = 0;
    int total;
    int oldobjs = 0;
#ifdef DISKBASE
    int loaded = 0;
    int changed = 0;
#endif
    time_t currtime = time(NULL);
    dbref owner = NOTHING;

    if (!Wizard(OWNER(player)) && (!name || !*name)) {
	notifyf(player, "The universe contains %d objects.", db_top);
    } else {
	total = rooms = exits = things = players = programs = 0;
	if (name != NULL && *name != '\0') {
	    owner = lookup_player(name);
	    if (owner == NOTHING) {
		notify(player, "I can't find that player.");
		return;
	    }
	    if (!Wizard(OWNER(player)) && (OWNER(player) != owner)) {
		notify(player,
		       "Permission denied. (you must be a wizard to get someone else's stats)");
		return;
	    }
	    for (dbref i = 0; i < db_top; i++) {

#ifdef DISKBASE
		if ((OWNER(i) == owner) && DBFETCH(i)->propsmode != PROPS_UNLOADED)
		    loaded++;
		if ((OWNER(i) == owner) && DBFETCH(i)->propsmode == PROPS_CHANGED)
		    changed++;
#endif

		/* if unused for 90 days, inc oldobj count */
		if ((OWNER(i) == owner) &&
		    (currtime - DBFETCH(i)->ts.lastused) > tp_aging_time)
		    oldobjs++;

		switch (Typeof(i)) {
		case TYPE_ROOM:
		    if (OWNER(i) == owner)
			total++, rooms++;
		    break;

		case TYPE_EXIT:
		    if (OWNER(i) == owner)
			total++, exits++;
		    break;

		case TYPE_THING:
		    if (OWNER(i) == owner)
			total++, things++;
		    break;

		case TYPE_PLAYER:
		    if (i == owner)
			total++, players++;
		    break;

		case TYPE_PROGRAM:
		    if (OWNER(i) == owner)
			total++, programs++;
		    break;

		}
	    }
	} else {
	    for (dbref i = 0; i < db_top; i++) {

#ifdef DISKBASE
		if (DBFETCH(i)->propsmode != PROPS_UNLOADED)
		    loaded++;
		if (DBFETCH(i)->propsmode == PROPS_CHANGED)
		    changed++;
#endif

		/* if unused for 90 days, inc oldobj count */
		if ((currtime - DBFETCH(i)->ts.lastused) > tp_aging_time)
		    oldobjs++;

		switch (Typeof(i)) {
		case TYPE_ROOM:
		    total++;
		    rooms++;
		    break;
		case TYPE_EXIT:
		    total++;
		    exits++;
		    break;
		case TYPE_THING:
		    total++;
		    things++;
		    break;
		case TYPE_PLAYER:
		    total++;
		    players++;
		    break;
		case TYPE_PROGRAM:
		    total++;
		    programs++;
		    break;
		case TYPE_GARBAGE:
		    total++;
		    garbage++;
		    break;
		}
	    }
	}

	notifyf(player, "%7d room%s        %7d exit%s        %7d thing%s",
		rooms, (rooms == 1) ? " " : "s",
		exits, (exits == 1) ? " " : "s", things, (things == 1) ? " " : "s");

	notifyf(player, "%7d program%s     %7d player%s      %7d garbage",
		programs, (programs == 1) ? " " : "s",
		players, (players == 1) ? " " : "s", garbage);

	notifyf(player,
		"%7d total object%s                     %7d old & unused",
		total, (total == 1) ? " " : "s", oldobjs);

#ifdef DISKBASE
	if (Wizard(OWNER(player))) {
	    notifyf(player,
		    "%7d proploaded object%s                %7d propchanged object%s",
		    loaded, (loaded == 1) ? " " : "s", changed, (changed == 1) ? "" : "s");

	}
#endif
    }
}


void
do_boot(dbref player, const char *name)
{
    dbref victim;

    if ((victim = lookup_player(name)) == NOTHING) {
	notify(player, "That player does not exist.");
	return;
    }
    if (Typeof(victim) != TYPE_PLAYER) {
	notify(player, "You can only boot players!");
    }
#ifdef GOD_PRIV
    else if (God(victim)) {
	notify(player, "You can't boot God!");
    }
#endif				/* GOD_PRIV */

    else {
	notify(victim, "You have been booted off the game.");
	if (boot_off(victim)) {
	    log_status("BOOTED: %s(%d) by %s(%d)", NAME(victim), victim, NAME(player), player);
	    if (player != victim) {
		notifyf(player, "You booted %s off!", NAME(victim));
	    }
	} else {
	    notifyf(player, "%s is not connected.", NAME(victim));
	}
    }
}

void
do_toad(int descr, dbref player, const char *name, const char *recip)
{
    dbref victim;
    dbref recipient;
    char buf[BUFFER_LEN];
    struct tune_ref_entry *tref = tune_ref_list;

    if ((victim = lookup_player(name)) == NOTHING) {
	notify(player, "That player does not exist.");
	return;
    }
#ifdef GOD_PRIV
    if (God(victim)) {
	notify(player, "You cannot @toad God.");
	if (!God(player)) {
	    log_status("TOAD ATTEMPT: %s(#%d) tried to toad God.", NAME(player), player);
	}
	return;
    }
#endif
    if (player == victim) {
	/* If GOD_PRIV isn't defined, this could happen: we don't want the
	 * last wizard to be toaded, in any case, so only someone else can
	 * do it. */
	notify(player, "You cannot toad yourself.  Get someone else to do it for you.");
	return;
    }

    while (tref->name) {
	if (victim == *tref->ref) {
	    notify(player, "That player cannot currently be @toaded.");
	    return;
	}
	tref++;
    }

    if (!*recip) {
	recipient = tp_toad_default_recipient;
    } else {
	if ((recipient = lookup_player(recip)) == NOTHING || recipient == victim) {
	    notify(player, "That recipient does not exist.");
	    return;
	}
    }

    if (Typeof(victim) != TYPE_PLAYER) {
	notify(player, "You can only turn players into toads!");
#ifdef GOD_PRIV
    } else if (!(God(player)) && (TrueWizard(victim))) {
#else
    } else if (TrueWizard(victim)) {
#endif
	notify(player, "You can't turn a Wizard into a toad.");
    } else {
	/* we're ok */
	/* do it */
	send_contents(descr, victim, HOME);
	dequeue_prog(victim, 0);	/* Dequeue the programs that the player's running */
	for (dbref stuff = 0; stuff < db_top; stuff++) {
	    if (OWNER(stuff) == victim) {
		switch (Typeof(stuff)) {
		case TYPE_PROGRAM:
		    dequeue_prog(stuff, 0);	/* dequeue player's progs */
		    if (TrueWizard(recipient)) {
			FLAGS(stuff) &= ~(ABODE | WIZARD);
			SetMLevel(stuff, 1);
		    }
		case TYPE_ROOM:
		case TYPE_THING:
		case TYPE_EXIT:
		    OWNER(stuff) = recipient;
		    DBDIRTY(stuff);
		    break;
		}
	    }
	    if (Typeof(stuff) == TYPE_THING && THING_HOME(stuff) == victim) {
		THING_SET_HOME(stuff, tp_lost_and_found);
	    }
	}

	chown_macros(victim, recipient);

	if (PLAYER_PASSWORD(victim)) {
	    free((void *) PLAYER_PASSWORD(victim));
	    PLAYER_SET_PASSWORD(victim, 0);
	}

	/* notify people */
	notify(victim, "You have been turned into a toad.");
	notifyf(player, "You turned %s into a toad!", NAME(victim));
	log_status("TOADED: %s(%d) by %s(%d)", NAME(victim), victim, NAME(player), player);
	/* reset name */
	delete_player(victim);
	snprintf(buf, sizeof(buf), "A slimy toad named %s", NAME(victim));
	free((void *) NAME(victim));
	NAME(victim) = alloc_string(buf);
	DBDIRTY(victim);

	boot_player_off(victim);	/* Disconnect the toad */

	if (PLAYER_DESCRS(victim)) {
	    free(PLAYER_DESCRS(victim));
	    PLAYER_SET_DESCRS(victim, NULL);
	    PLAYER_SET_DESCRCOUNT(victim, 0);
	}

	ignore_remove_from_all_players(victim);
	ignore_flush_cache(victim);

	FREE_PLAYER_SP(victim);
	ALLOC_THING_SP(victim);
	THING_SET_HOME(victim, PLAYER_HOME(player));

	FLAGS(victim) = TYPE_THING;
	OWNER(victim) = player;	/* you get it */
	SETVALUE(victim, 1);	/* don't let him keep his immense wealth */
    }
}

void
do_newpassword(dbref player, const char *name, const char *password)
{
    dbref victim;

    if ((victim = lookup_player(name)) == NOTHING) {
	notify(player, "No such player.");
    } else if (*password != '\0' && !ok_password(password)) {
	/* Wiz can set null passwords, but not bad passwords */
	notify(player, "Bad password");

#ifdef GOD_PRIV
    } else if (God(victim)) {
	notify(player, "You can't change God's password!");
	return;
    } else {
	if (TrueWizard(victim) && !God(player)) {
	    notify(player, "Only God can change a wizard's password.");
	    return;
	}
#else				/* GOD_PRIV */
    } else {
#endif				/* GOD_PRIV */

	/* it's ok, do it */
	set_password(victim, password);
	DBDIRTY(victim);
	notify(player, "Password changed.");
	notifyf(victim, "Your password has been changed by %s.", NAME(player));
	log_status("NEWPASS'ED: %s(%d) by %s(%d)", NAME(victim), victim, NAME(player), player);
    }
}

void
do_pcreate(dbref player, const char *user, const char *password)
{
    dbref newguy;

    newguy = create_player(user, password);
    if (newguy == NOTHING) {
	notify(player, "Create failed.");
    } else {
	log_status("PCREATED %s(%d) by %s(%d)", NAME(newguy), newguy, NAME(player), player);
	notifyf(player, "Player %s created as object #%d.", user, newguy);
    }
}

#ifndef NO_USAGE_COMMAND
void
do_usage(dbref player)
{
    int psize;
#ifdef HAVE_GETRUSAGE
    struct rusage usage;
#endif

    notifyf(player, "Compiled on: %s", UNAME_VALUE);
    notifyf(player, "Process ID: %d", getpid());
    notifyf(player, "Max descriptors/process: %ld", max_open_files());

#ifdef HAVE_GETRUSAGE
    psize = getpagesize();
    getrusage(RUSAGE_SELF, &usage);

    notifyf(player, "Performed %d input servicings.", usage.ru_inblock);
    notifyf(player, "Performed %d output servicings.", usage.ru_oublock);
    notifyf(player, "Sent %d messages over a socket.", usage.ru_msgsnd);
    notifyf(player, "Received %d messages over a socket.", usage.ru_msgrcv);
    notifyf(player, "Received %d signals.", usage.ru_nsignals);
    notifyf(player, "Page faults NOT requiring physical I/O: %d", usage.ru_minflt);
    notifyf(player, "Page faults REQUIRING physical I/O: %d", usage.ru_majflt);
    notifyf(player, "Swapped out of main memory %d times.", usage.ru_nswap);
    notifyf(player, "Voluntarily context switched %d times.", usage.ru_nvcsw);
    notifyf(player, "Involuntarily context switched %d times.", usage.ru_nivcsw);
    notifyf(player, "User time used: %d sec.", usage.ru_utime.tv_sec);
    notifyf(player, "System time used: %d sec.", usage.ru_stime.tv_sec);
    notifyf(player, "Pagesize for this machine: %d", psize);
    notifyf(player, "Maximum resident memory: %ldk",
	    (long) (usage.ru_maxrss * (psize / 1024)));
    notifyf(player, "Integral resident memory: %ldk",
	    (long) (usage.ru_idrss * (psize / 1024)));
#endif				/* HAVE_GETRUSAGE */
}
#endif				/* NO_USAGE_COMMAND */



void
do_muf_topprofs(dbref player, char *arg1)
{
    struct profnode {
	struct profnode *next;
	dbref prog;
	double proftime;
	double pcnt;
	time_t comptime;
	long usecount;
    } *tops = NULL;

    struct profnode *curr = NULL;
    int nodecount = 0;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!string_compare(arg1, "reset")) {
	for (dbref i = db_top; i-- > 0;) {
	    if (Typeof(i) == TYPE_PROGRAM) {
		PROGRAM_SET_PROFTIME(i, 0, 0);
		PROGRAM_SET_PROFSTART(i, current_systime);
		PROGRAM_SET_PROF_USES(i, 0);
	    }
	}
	notify(player, "MUF profiling statistics cleared.");
	return;
    }
    if (count < 0) {
	notify(player, "Count has to be a positive number.");
	return;
    } else if (count == 0) {
	count = 10;
    }

    for (dbref i = db_top; i-- > 0;) {
	if (Typeof(i) == TYPE_PROGRAM && PROGRAM_CODE(i)) {
	    struct profnode *newnode = (struct profnode *) malloc(sizeof(struct profnode));
	    struct timeval tmpt = PROGRAM_PROFTIME(i);

	    newnode->next = NULL;
	    newnode->prog = i;
	    newnode->proftime = tmpt.tv_sec;
	    newnode->proftime += (tmpt.tv_usec / 1000000.0);
	    newnode->comptime = current_systime - PROGRAM_PROFSTART(i);
	    newnode->usecount = PROGRAM_PROF_USES(i);
	    if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
	    } else {
		newnode->pcnt = 0.0;
	    }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
		if (nodecount >= count) {
		    curr = tops;
		    tops = tops->next;
		    free(curr);
		} else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
	}
    }
    notify(player, "     %CPU   TotalTime  UseCount  Program");
    while (tops) {
	curr = tops;
	notifyf(player, "%10.3f %10.3f %9ld %s", curr->pcnt, curr->proftime, curr->usecount,
		unparse_object(player, curr->prog));
	tops = tops->next;
	free(curr);
    }
    notifyf(player, "Profile Length (sec): %5lld  %%idle: %5.2f%%  Total Cycles: %5lu",
	    (long long) (current_systime - sel_prof_start_time),
	    ((double) (sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) * 100.0) /
	    (double) ((current_systime - sel_prof_start_time) + 0.01), sel_prof_idle_use);
    notify(player, "*Done*");
}


void
do_mpi_topprofs(dbref player, char *arg1)
{
    struct profnode {
	struct profnode *next;
	dbref prog;
	double proftime;
	double pcnt;
	time_t comptime;
	long usecount;
    } *tops = NULL;

    struct profnode *curr = NULL;
    int nodecount = 0;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!string_compare(arg1, "reset")) {
	for (dbref i = db_top; i-- > 0;) {
	    if (DBFETCH(i)->mpi_prof_use) {
		DBFETCH(i)->mpi_prof_use = 0;
		DBFETCH(i)->mpi_proftime.tv_usec = 0;
		DBFETCH(i)->mpi_proftime.tv_sec = 0;
	    }
	}
	mpi_prof_start_time = current_systime;
	notify(player, "MPI profiling statistics cleared.");
	return;
    }
    if (count < 0) {
	notify(player, "Count has to be a positive number.");
	return;
    } else if (count == 0) {
	count = 10;
    }

    for (dbref i = db_top; i-- > 0;) {
	if (DBFETCH(i)->mpi_prof_use) {
	    struct profnode *newnode = (struct profnode *) malloc(sizeof(struct profnode));
	    newnode->next = NULL;
	    newnode->prog = i;
	    newnode->proftime = DBFETCH(i)->mpi_proftime.tv_sec;
	    newnode->proftime += (DBFETCH(i)->mpi_proftime.tv_usec / 1000000.0);
	    newnode->comptime = current_systime - mpi_prof_start_time;
	    newnode->usecount = DBFETCH(i)->mpi_prof_use;
	    if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
	    } else {
		newnode->pcnt = 0.0;
	    }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
		if (nodecount >= count) {
		    curr = tops;
		    tops = tops->next;
		    free(curr);
		} else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
	}
    }
    notify(player, "     %CPU   TotalTime  UseCount  Object");
    while (tops) {
	curr = tops;
	notifyf(player, "%10.3f %10.3f %9ld %s", curr->pcnt, curr->proftime, curr->usecount,
		unparse_object(player, curr->prog));
	tops = tops->next;
	free(curr);
    }
    notifyf(player, "Profile Length (sec): %5lld  %%idle: %5.2f%%  Total Cycles: %5lu",
	    (long long) (current_systime - sel_prof_start_time),
	    (((double) sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) * 100.0) /
	    (double) ((current_systime - sel_prof_start_time) + 0.01), sel_prof_idle_use);
    notify(player, "*Done*");
}


void
do_all_topprofs(dbref player, char *arg1)
{
    struct profnode {
	struct profnode *next;
	dbref prog;
	double proftime;
	double pcnt;
	time_t comptime;
	long usecount;
	short type;
    } *tops = NULL;

    struct profnode *curr = NULL;
    int nodecount = 0;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!string_compare(arg1, "reset")) {
	for (dbref i = db_top; i-- > 0;) {
	    if (DBFETCH(i)->mpi_prof_use) {
		DBFETCH(i)->mpi_prof_use = 0;
		DBFETCH(i)->mpi_proftime.tv_usec = 0;
		DBFETCH(i)->mpi_proftime.tv_sec = 0;
	    }
	    if (Typeof(i) == TYPE_PROGRAM) {
		PROGRAM_SET_PROFTIME(i, 0, 0);
		PROGRAM_SET_PROFSTART(i, current_systime);
		PROGRAM_SET_PROF_USES(i, 0);
	    }
	}
	sel_prof_idle_sec = 0;
	sel_prof_idle_usec = 0;
	sel_prof_start_time = current_systime;
	sel_prof_idle_use = 0;
	mpi_prof_start_time = current_systime;
	notify(player, "All profiling statistics cleared.");
	return;
    }
    if (count < 0) {
	notify(player, "Count has to be a positive number.");
	return;
    } else if (count == 0) {
	count = 10;
    }

    for (dbref i = db_top; i-- > 0;) {
	if (DBFETCH(i)->mpi_prof_use) {
	    struct profnode *newnode = (struct profnode *) malloc(sizeof(struct profnode));
	    newnode->next = NULL;
	    newnode->prog = i;
	    newnode->proftime = DBFETCH(i)->mpi_proftime.tv_sec;
	    newnode->proftime += (DBFETCH(i)->mpi_proftime.tv_usec / 1000000.0);
	    newnode->comptime = current_systime - mpi_prof_start_time;
	    newnode->usecount = DBFETCH(i)->mpi_prof_use;
	    newnode->type = 0;
	    if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
	    } else {
		newnode->pcnt = 0.0;
	    }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
		if (nodecount >= count) {
		    curr = tops;
		    tops = tops->next;
		    free(curr);
		} else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
	}
	if (Typeof(i) == TYPE_PROGRAM && PROGRAM_CODE(i)) {
	    struct profnode *newnode = (struct profnode *) malloc(sizeof(struct profnode));
	    struct timeval tmpt = PROGRAM_PROFTIME(i);

	    newnode->next = NULL;
	    newnode->prog = i;
	    newnode->proftime = tmpt.tv_sec;
	    newnode->proftime += (tmpt.tv_usec / 1000000.0);
	    newnode->comptime = current_systime - PROGRAM_PROFSTART(i);
	    newnode->usecount = PROGRAM_PROF_USES(i);
	    newnode->type = 1;
	    if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
	    } else {
		newnode->pcnt = 0.0;
	    }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
		if (nodecount >= count) {
		    curr = tops;
		    tops = tops->next;
		    free(curr);
		} else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
	}
    }
    notify(player, "     %CPU   TotalTime  UseCount  Type  Object");
    while (tops) {
	curr = tops;
	notifyf(player, "%10.3f %10.3f %9ld%5s   %s", curr->pcnt, curr->proftime,
		curr->usecount, curr->type ? "MUF" : "MPI", unparse_object(player,
									   curr->prog));
	tops = tops->next;
	free(curr);
    }
    notifyf(player, "Profile Length (sec): %5lld  %%idle: %5.2f%%  Total Cycles: %5lu",
	    (long long) (current_systime - sel_prof_start_time),
	    ((double) (sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) * 100.0) /
	    (double) ((current_systime - sel_prof_start_time) + 0.01), sel_prof_idle_use);
    notify(player, "*Done*");
}

#ifndef NO_MEMORY_COMMAND
void
do_memory(dbref who)
{
#ifdef HAVE_MALLINFO
    {
	struct mallinfo mi;

	mi = mallinfo();
	notifyf(who, "Total allocated from system:   %6dk", (mi.arena / 1024));
	notifyf(who, "Number of non-inuse chunks:    %6d", mi.ordblks);
	notifyf(who, "Small block count:             %6d", mi.smblks);
	notifyf(who, "Small block mem alloced:       %6dk", (mi.usmblks / 1024));
	notifyf(who, "Small block memory free:       %6dk", (mi.fsmblks / 1024));
#ifdef HAVE_STRUCT_MALLINFO_HBLKS
	notifyf(who, "Number of mmapped regions:     %6d", mi.hblks);
#endif
	notifyf(who, "Total memory mmapped:          %6dk", (mi.hblkhd / 1024));
	notifyf(who, "Total memory malloced:         %6dk", (mi.uordblks / 1024));
	notifyf(who, "Mem allocated overhead:        %6dk", ((mi.arena - mi.uordblks) / 1024));
	notifyf(who, "Memory free:                   %6dk", (mi.fordblks / 1024));
#ifdef HAVE_STRUCT_MALLINFO_KEEPCOST
	notifyf(who, "Top-most releasable chunk size:%6dk", (mi.keepcost / 1024));
#endif
#ifdef HAVE_STRUCT_MALLINFO_TREEOVERHEAD
	notifyf(who, "Memory free overhead:    %6dk", (mi.treeoverhead / 1024));
#endif
#ifdef HAVE_STRUCT_MALLINFO_GRAIN
	notifyf(who, "Small block grain: %6d", mi.grain);
#endif
#ifdef HAVE_STRUCT_MALLINFO_ALLOCATED
	notifyf(who, "Mem chunks alloced:%6d", mi.allocated);
#endif
    }
#endif				/* HAVE_MALLINFO */

#ifdef MALLOC_PROFILING
    notify(who, "  ");
    CrT_summarize(who);
    CrT_summarize_to_file("malloc_log", "Manual Checkpoint");
#endif

    notify(who, "Done.");
}
#endif				/* NO_MEMORY_COMMAND */
