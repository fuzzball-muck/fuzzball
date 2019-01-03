#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#include "dbsearch.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "match.h"
#include "mpi.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

/* prints owner of something */
static void
print_owner(dbref player, dbref thing)
{
    char buf[BUFFER_LEN];

    switch (Typeof(thing)) {
    case TYPE_PLAYER:
	snprintf(buf, sizeof(buf), "%s is a player.", NAME(thing));
	break;
    case TYPE_ROOM:
    case TYPE_THING:
    case TYPE_EXIT:
    case TYPE_PROGRAM:
	snprintf(buf, sizeof(buf), "Owner: %s", NAME(OWNER(thing)));
	break;
    case TYPE_GARBAGE:
	snprintf(buf, sizeof(buf), "%s is garbage.", NAME(thing));
	break;
    }
    notify(player, buf);
}

static int
can_see(dbref player, dbref thing, int can_see_loc)
{
    if (player == thing || Typeof(thing) == TYPE_EXIT || Typeof(thing) == TYPE_ROOM)
        return 0;

    if (can_see_loc) {
        switch (Typeof(thing)) {
        case TYPE_PROGRAM:
            return ((FLAGS(thing) & VEHICLE) || controls(player, thing));
        case TYPE_PLAYER:
            if (tp_dark_sleepers) {
                return (!Dark(thing) && online(thing));
            }
        default:
            return (!Dark(thing) || (controls(player, thing) && !(FLAGS(player) & STICKY)));
        }
    } else {
        /* can't see loc */
        return (controls(player, thing) && !(FLAGS(player) & STICKY));
    }
}

static void
look_contents(dbref player, dbref loc, const char *contents_name)
{
    dbref thing;
    dbref can_see_loc;
    char unparse_buf[BUFFER_LEN];

    /* check to see if he can see the location */
    can_see_loc = (!Dark(loc) || controls(player, loc));

    /* check to see if there is anything there */
    DOLIST(thing, CONTENTS(loc)) {
	if (can_see(player, thing, can_see_loc)) {
	    /* something exists!  show him everything */
	    notify(player, contents_name);
	    DOLIST(thing, CONTENTS(loc)) {
		if (can_see(player, thing, can_see_loc)) {
                    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
		    notify(player, unparse_buf);
		}
	    }
	    break;		/* we're done */
	}
    }
}

static void
look_simple(int descr, dbref player, dbref thing)
{
    if (GETDESC(thing)) {
	exec_or_notify(descr, player, thing, GETDESC(thing), "(@Desc)",
		       Prop_Blessed(thing, MESGPROP_DESC) ? MPI_ISBLESSED : 0);
    } else {
	notify(player, "You see nothing special.");
    }
}

static void
look_room(int descr, dbref player, dbref loc)
{
    char obj_num[20];
    char unparse_buf[BUFFER_LEN];

    if (loc == NOTHING) return;

    /* tell him the name, and the number if he can link to it */
    unparse_object(player, loc, unparse_buf, sizeof(unparse_buf));
    notify(player, unparse_buf);

    /* tell him the description */
    if (Typeof(loc) == TYPE_ROOM) {
	if (GETDESC(loc)) {
	    exec_or_notify(descr, player, loc, GETDESC(loc), "(@Desc)",
			   Prop_Blessed(loc, MESGPROP_DESC) ? MPI_ISBLESSED : 0);
	}
	/* tell him the appropriate messages if he has the key */
	can_doit(descr, player, loc, 0);
    } else {
	if (GETIDESC(loc)) {
	    exec_or_notify(descr, player, loc, GETIDESC(loc), "(@Idesc)",
			   Prop_Blessed(loc, MESGPROP_IDESC) ? MPI_ISBLESSED : 0);
	}
    }
    ts_useobject(loc);

    /* tell him the contents */
    look_contents(player, loc, "Contents:");
    snprintf(obj_num, sizeof(obj_num), "#%d", loc);
    envpropqueue(descr, player, loc, player, loc, NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
}

void
do_look_around(int descr, dbref player)
{
    look_room(descr, player, LOCATION(player));
}

void
do_look_at(int descr, dbref player, const char *name, const char *detail)
{
    dbref thing;
    struct match_data md;
    char buf[BUFFER_LEN];
    char obj_num[20];

    if (*name == '\0' || !strcasecmp(name, "here")) {
	look_room(descr, player, LOCATION(player));
    } else {
	/* look at a thing here */
	init_match(descr, player, name, NOTYPE, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	/* match_registered(&md); */
	if (Wizard(OWNER(player))) {
	    match_absolute(&md);
	    match_player(&md);
	}
	match_here(&md);
	match_me(&md);

	thing = match_result(&md);
	if (thing != NOTHING && thing != AMBIGUOUS && !*detail) {
	    switch (Typeof(thing)) {
	    case TYPE_ROOM:
		if (LOCATION(player) != thing && !can_link_to(player, TYPE_ROOM, thing)) {
		    notify(player,
			   "Permission denied. (you're not where you want to look, and can't link to it)");
		} else {
		    look_room(descr, player, thing);
		}
		break;
	    case TYPE_PLAYER:
		if (LOCATION(player) != LOCATION(thing)
		    && !controls(player, thing)) {
		    notify(player,
			   "Permission denied. (Your location isn't the same as what you're looking at)");
		} else {
		    look_simple(descr, player, thing);
		    look_contents(player, thing, "Carrying:");
		    snprintf(obj_num, sizeof(obj_num), "#%d", thing);
		    envpropqueue(descr, player, thing, player, thing,
			    NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
		}
		break;
	    case TYPE_THING:
		if (LOCATION(player) != LOCATION(thing)
		    && LOCATION(thing) != player && !controls(player, thing)) {
		    notify(player,
			   "Permission denied. (You're not in the same room as or carrying the object)");
		} else {
		    look_simple(descr, player, thing);
		    if (!(FLAGS(thing) & HAVEN)) {
			look_contents(player, thing, "Contains:");
			ts_useobject(thing);
		    }
		    snprintf(obj_num, sizeof(obj_num), "#%d", thing);
		    envpropqueue(descr, player, thing, player, thing,
			    NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
		}
		break;
	    default:
		look_simple(descr, player, thing);
		if (Typeof(thing) != TYPE_PROGRAM)
		    ts_useobject(thing);
		snprintf(obj_num, sizeof(obj_num), "#%d", thing);
		envpropqueue(descr, player, thing, player, thing,
		        NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
		break;
	    }
	} else if (thing == NOTHING || (*detail && thing != AMBIGUOUS)) {
	    int ambig_flag = 0;
	    char propname[BUFFER_LEN];
	    PropPtr propadr, pptr, lastmatch = NULL;

	    if (thing == NOTHING) {
		thing = LOCATION(player);
		snprintf(buf, sizeof(buf), "%s", name);
	    } else {
		snprintf(buf, sizeof(buf), "%s", detail);
	    }

#ifdef DISKBASE
	    fetchprops(thing, DETAILS_PROPDIR);
#endif

	    lastmatch = NULL;
	    propadr = first_prop(thing, DETAILS_PROPDIR, &pptr, propname, sizeof(propname));
	    while (propadr) {
		if (exit_prefix(propname, buf)) {
		    if (lastmatch) {
			lastmatch = NULL;
			ambig_flag = 1;
			break;
		    } else {
			lastmatch = propadr;
		    }
		}
		propadr = next_prop(pptr, propadr, propname, sizeof(propname));
	    }
	    if (lastmatch && PropType(lastmatch) == PROP_STRTYP) {
#ifdef DISKBASE
		propfetch(thing, lastmatch);	/* DISKBASE PROPVALS */
#endif
		exec_or_notify(descr, player, thing, PropDataStr(lastmatch), "(@detail)",
			       (PropFlags(lastmatch) & PROP_BLESSED) ? MPI_ISBLESSED : 0);
	    } else if (ambig_flag) {
		notifyf_nolisten(player, match_msg_ambiguous(buf, 0));
	    } else if (*detail) {
		notify(player, "You see nothing special.");
	    } else {
		notifyf_nolisten(player, match_msg_nomatch(buf, 0));
	    }
	} else {
	    notifyf_nolisten(player, match_msg_ambiguous(detail, 0));
	}
    }
}

static const char *
flag_description(dbref thing)
{
    static char buf[BUFFER_LEN];

    strcpyn(buf, sizeof(buf), "Type: ");
    switch (Typeof(thing)) {
    case TYPE_ROOM:
	strcatn(buf, sizeof(buf), "ROOM");
	break;
    case TYPE_EXIT:
	strcatn(buf, sizeof(buf), "EXIT/ACTION");
	break;
    case TYPE_THING:
	strcatn(buf, sizeof(buf), "THING");
	break;
    case TYPE_PLAYER:
	strcatn(buf, sizeof(buf), "PLAYER");
	break;
    case TYPE_PROGRAM:
	strcatn(buf, sizeof(buf), "PROGRAM");
	break;
    case TYPE_GARBAGE:
	strcatn(buf, sizeof(buf), "GARBAGE");
	break;
    default:
	strcatn(buf, sizeof(buf), "***UNKNOWN TYPE***");
	break;
    }

    if (FLAGS(thing) & ~TYPE_MASK & ~DUMP_MASK) {
	/* print flags */
	strcatn(buf, sizeof(buf), "  Flags:");
	if (FLAGS(thing) & WIZARD)
	    strcatn(buf, sizeof(buf), " WIZARD");
	if (FLAGS(thing) & QUELL)
	    strcatn(buf, sizeof(buf), " QUELL");
	if (FLAGS(thing) & STICKY)
	    strcatn(buf, sizeof(buf), (Typeof(thing) == TYPE_PROGRAM) ? " SETUID" :
		    (Typeof(thing) == TYPE_PLAYER) ? " SILENT" : " STICKY");
	if (FLAGS(thing) & DARK)
	    strcatn(buf, sizeof(buf),
		    (Typeof(thing) != TYPE_PROGRAM) ? " DARK" : " DEBUGGING");
	if (FLAGS(thing) & LINK_OK)
	    strcatn(buf, sizeof(buf), " LINK_OK");

	if (FLAGS(thing) & KILL_OK)
	    strcatn(buf, sizeof(buf), " KILL_OK");

	if (MLevRaw(thing)) {
	    strcatn(buf, sizeof(buf), " MUCKER");
	    switch (MLevRaw(thing)) {
	    case 1:
		strcatn(buf, sizeof(buf), "1");
		break;
	    case 2:
		strcatn(buf, sizeof(buf), "2");
		break;
	    case 3:
		strcatn(buf, sizeof(buf), "3");
		break;
	    }
	}
	if (FLAGS(thing) & BUILDER)
	    strcatn(buf, sizeof(buf), (Typeof(thing) == TYPE_PROGRAM) ? " BOUND" : " BUILDER");
	if (FLAGS(thing) & CHOWN_OK)
	    strcatn(buf, sizeof(buf), (Typeof(thing) == TYPE_PLAYER) ? " COLOR" : " CHOWN_OK");
	if (FLAGS(thing) & JUMP_OK)
	    strcatn(buf, sizeof(buf), " JUMP_OK");
	if (FLAGS(thing) & VEHICLE)
	    strcatn(buf, sizeof(buf),
		    (Typeof(thing) == TYPE_PROGRAM) ? " VIEWABLE" : " VEHICLE");
	if (tp_enable_match_yield && FLAGS(thing) & YIELD)
	    strcatn(buf, sizeof(buf), " YIELD");
	if (tp_enable_match_yield && FLAGS(thing) & OVERT)
	    strcatn(buf, sizeof(buf), " OVERT");
	if (FLAGS(thing) & XFORCIBLE) {
	    if (Typeof(thing) == TYPE_EXIT) {
		strcatn(buf, sizeof(buf), " XPRESS");
	    } else {
		strcatn(buf, sizeof(buf), " XFORCIBLE");
	    }
	}
	if (FLAGS(thing) & ZOMBIE)
	    strcatn(buf, sizeof(buf), " ZOMBIE");
	if (FLAGS(thing) & GUEST)
	    strcatn(buf, sizeof(buf),
                    (Typeof(thing) == TYPE_PROGRAM || Typeof(thing) == TYPE_EXIT || Typeof(thing) == TYPE_ROOM) ?
				       " NOGUEST" : " GUEST");
	if (FLAGS(thing) & HAVEN)
	    strcatn(buf, sizeof(buf),
		    (Typeof(thing) !=
		     TYPE_PROGRAM) ? ((Typeof(thing) ==
				       TYPE_THING) ? " HIDE" : " HAVEN") : " HARDUID");
	if (FLAGS(thing) & ABODE)
	    strcatn(buf, sizeof(buf),
		    (Typeof(thing) != TYPE_PROGRAM) ? (Typeof(thing) !=
						       TYPE_EXIT ? " ABODE" : " ABATE") :
		    " AUTOSTART");
    }
    return buf;
}

static int
listprops_wildcard(dbref player, dbref thing, const char *dir, const char *wild)
{
    char propname[BUFFER_LEN];
    char wld[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char *ptr, *wldcrd;
    PropPtr propadr, pptr;
    int i, cnt = 0;
    int recurse = 0;

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
	    const char *current, *tmpname;
	    static const char *legacy_gender;
	    static const char *legacy_guest;
	    current = tp_gender_prop;
	    legacy_gender = LEGACY_GENDER_PROP;
	    legacy_guest = LEGACY_GUEST_PROP;

	    snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);
	    tmpname = buf;

	    while (*current == PROPDIR_DELIMITER)
		current++;
	    while (*legacy_gender == PROPDIR_DELIMITER)
		legacy_gender++;
	    while (*legacy_guest == PROPDIR_DELIMITER)
		legacy_guest++;
	    while (*tmpname == PROPDIR_DELIMITER)
		tmpname++;

	    if (!tp_show_legacy_props &&
	            (!strcasecmp(tmpname, legacy_guest) || (!strcasecmp(tmpname, legacy_gender) && strcasecmp(current, legacy_gender)))) {
		propadr = next_prop(pptr, propadr, propname, sizeof(propname));
		continue;
	    }
	    if (!Prop_System(buf) && (!Prop_Hidden(buf) || Wizard(OWNER(player)))) {
		if (!*ptr || recurse) {
		    cnt++;
		    displayprop(player, thing, buf, buf2, sizeof(buf2));
		    notify(player, buf2);
		}
		if (recurse)
		    ptr = "**";
		cnt += listprops_wildcard(player, thing, buf, ptr);
	    }
	}
	propadr = next_prop(pptr, propadr, propname, sizeof(propname));
    }
    return cnt;
}

void
do_examine(int descr, dbref player, const char *name, const char *dir)
{
    dbref thing;
    char unparse_buf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    const char *temp;
    dbref content;
    dbref exit;
    int cnt;
    struct match_data md;
    struct tm *time_tm;		/* used for timestamps */

    if (*name == '\0' || !strcasecmp(name, "here")) {
	thing = LOCATION(player);
    } else {
	/* look it up */
	init_match(descr, player, name, NOTYPE, &md);
	match_everything(&md);

	/* get result */
	if ((thing = noisy_match_result(&md)) == NOTHING)
	    return;
    }

    if (!can_link(player, thing) && !test_lock_false_default(descr, player, thing, MESGPROP_READLOCK)) {
	print_owner(player, thing);
	return;
    }
    if (*dir) {
	/* show him the properties */
	cnt = listprops_wildcard(player, thing, "", dir);
	notifyf(player, "%d propert%s listed.", cnt, (cnt == 1 ? "y" : "ies"));
	return;
    }
    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
    switch (Typeof(thing)) {
    case TYPE_ROOM:
	snprintf(buf, sizeof(buf), "%.*s  Owner: %s  Parent: ",
		 (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
		 unparse_buf, NAME(OWNER(thing)));
        unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
	strcatn(buf, sizeof(buf), unparse_buf);
	break;
    case TYPE_THING:
	snprintf(buf, sizeof(buf), "%.*s  Owner: %s  Value: %d",
		 (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
		 unparse_buf, NAME(OWNER(thing)), GETVALUE(thing));
	break;
    case TYPE_PLAYER:
	snprintf(buf, sizeof(buf), "%.*s  %s: %d  ",
		 (int) (BUFFER_LEN - strlen(tp_cpennies) - 35),
		 unparse_buf, tp_cpennies, GETVALUE(thing));
	break;
    case TYPE_EXIT:
    case TYPE_PROGRAM:
	snprintf(buf, sizeof(buf), "%.*s  Owner: %s",
		 (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
		 unparse_buf, NAME(OWNER(thing)));
	break;
    case TYPE_GARBAGE:
	strcpyn(buf, sizeof(buf), unparse_buf);
	break;
    }
    notify(player, buf);

    if (tp_verbose_examine)
	notify(player, flag_description(thing));

    if (GETDESC(thing))
	notify(player, GETDESC(thing));

    if (strcmp(temp = unparse_boolexp(player, GETLOCK(thing), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Key: %s", temp);
    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_LINKLOCK), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Link_OK Key: %s", temp);
    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_CHLOCK), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Chown_OK Key: %s", temp);
    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_CONLOCK), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Container Key: %s", temp);
    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_FLOCK), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Force Key: %s", temp);
    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_READLOCK), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Read Key: %s", temp);
    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_OWNLOCK), 1), PROP_UNLOCKED_VAL))
	notifyf(player, "Ownership Key: %s", temp);

    if (GETSUCC(thing)) {
	notifyf(player, "Success: %s", GETSUCC(thing));
    }
    if (GETFAIL(thing)) {
	notifyf(player, "Fail: %s", GETFAIL(thing));
    }
    if (GETDROP(thing)) {
	notifyf(player, "Drop: %s", GETDROP(thing));
    }
    if (GETOSUCC(thing)) {
	notifyf(player, "Osuccess: %s", GETOSUCC(thing));
    }
    if (GETOFAIL(thing)) {
	notifyf(player, "Ofail: %s", GETOFAIL(thing));
    }
    if (GETODROP(thing)) {
	notifyf(player, "Odrop: %s", GETODROP(thing));
    }

    if (GETDOING(thing)) {
	notifyf(player, "Doing: %s", GETDOING(thing));
    }
    if (GETOECHO(thing)) {
	notifyf(player, "Oecho: %s", GETOECHO(thing));
    }
    if (GETPECHO(thing)) {
	notifyf(player, "Pecho: %s", GETPECHO(thing));
    }
    if (GETIDESC(thing)) {
	notifyf(player, "Idesc: %s", GETIDESC(thing));
    }

    /* Timestamps */

    time_tm = localtime(&(DBFETCH(thing)->ts_created));
    strftime(buf, BUFFER_LEN, "%c %Z", time_tm);
    notifyf(player, "Created:  %s", buf);

    time_tm = localtime(&(DBFETCH(thing)->ts_modified));
    strftime(buf, BUFFER_LEN, "%c %Z", time_tm);
    notifyf(player, "Modified: %s", buf);

    time_tm = localtime(&(DBFETCH(thing)->ts_lastused));
    strftime(buf, BUFFER_LEN, "%c %Z", time_tm);
    notifyf(player, "Lastused: %s", buf);

    if (Typeof(thing) == TYPE_PROGRAM) {
	snprintf(buf, sizeof(buf), "Usecount: %d     Instances: %d",
		 DBFETCH(thing)->ts_usecount, PROGRAM_INSTANCES(thing));
    } else {
	snprintf(buf, sizeof(buf), "Usecount: %d", DBFETCH(thing)->ts_usecount);
    }
    notify(player, buf);

    notify(player, "[ Use 'examine <object>=/' to list root properties. ]");

    notifyf(player, "Memory used: %ld bytes", size_object(thing, 1));

    /* show him the contents */
    if (CONTENTS(thing) != NOTHING) {
	if (Typeof(thing) == TYPE_PLAYER)
	    notify(player, "Carrying:");
	else
	    notify(player, "Contents:");
	DOLIST(content, CONTENTS(thing)) {
            unparse_object(player, content, unparse_buf, sizeof(unparse_buf));
	    notify(player, unparse_buf);
	}
    }
    switch (Typeof(thing)) {
    case TYPE_ROOM:
	/* tell him about exits */
	if (EXITS(thing) != NOTHING) {
	    notify(player, "Exits:");
	    DOLIST(exit, EXITS(thing)) {
                unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
		notify(player, unparse_buf);
	    }
	} else {
	    notify(player, "No exits.");
	}

	/* print dropto if present */
	if (DBFETCH(thing)->sp.room.dropto != NOTHING) {
            unparse_object(player, DBFETCH(thing)->sp.room.dropto, unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "Dropped objects go to: %s", unparse_buf);
	}
	break;
    case TYPE_THING:
	/* print home */
        unparse_object(player, THING_HOME(thing), unparse_buf, sizeof(unparse_buf));
	notifyf(player, "Home: %s", unparse_buf);
	/* print location if player can link to it */
	if (LOCATION(thing) != NOTHING && (controls(player, LOCATION(thing))
					   || can_link_to(player, NOTYPE, LOCATION(thing)))) {
            unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "Location: %s", unparse_buf);
	}
	/* print thing's actions, if any */
	if (EXITS(thing) != NOTHING) {
	    notify(player, "Actions/exits:");
	    DOLIST(exit, EXITS(thing)) {
                unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
		notify(player, unparse_buf);
	    }
	} else {
	    notify(player, "No actions attached.");
	}
	break;
    case TYPE_PLAYER:

	/* print home */
        unparse_object(player, PLAYER_HOME(thing), unparse_buf, sizeof(unparse_buf));
	notifyf(player, "Home: %s", unparse_buf);	/* home */

	/* print location if player can link to it */
	if (LOCATION(thing) != NOTHING && (controls(player, LOCATION(thing))
					   || can_link_to(player, NOTYPE, LOCATION(thing)))) {
            unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "Location: %s", unparse_buf);
	}
	/* print player's actions, if any */
	if (EXITS(thing) != NOTHING) {
	    notify(player, "Actions/exits:");
	    DOLIST(exit, EXITS(thing)) {
                unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
		notify(player, unparse_buf);
	    }
	} else {
	    notify(player, "No actions attached.");
	}
	break;
    case TYPE_EXIT:
	if (LOCATION(thing) != NOTHING) {
            unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "Source: %s", unparse_buf);
	}
	/* print destinations */
	if (DBFETCH(thing)->sp.exit.ndest == 0)
	    break;
	for (int i = 0; i < DBFETCH(thing)->sp.exit.ndest; i++) {
	    switch ((DBFETCH(thing)->sp.exit.dest)[i]) {
	    case NOTHING:
		break;
	    case HOME:
		notify(player, "Destination: *HOME*");
		break;
	    default:
                unparse_object(player, (DBFETCH(thing)->sp.exit.dest)[i], unparse_buf, sizeof(unparse_buf));
		notifyf(player, "Destination: %s", unparse_buf);
		break;
	    }
	}
	break;
    case TYPE_PROGRAM:
	if (PROGRAM_SIZ(thing)) {
	    struct timeval tv = PROGRAM_PROFTIME(thing);
	    notifyf(player, "Program compiled size: %d instructions", PROGRAM_SIZ(thing));
	    notifyf(player, "Cumulative runtime: %d.%06d seconds ", (int) tv.tv_sec,
		    (int) tv.tv_usec);
	} else {
	    notify(player, "Program not compiled.");
	}

	/* print location if player can link to it */
	if (LOCATION(thing) != NOTHING && (controls(player, LOCATION(thing))
					   || can_link_to(player, NOTYPE, LOCATION(thing)))) {
            unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "Location: %s", unparse_buf);
	}
	break;
    default:
	/* do nothing */
	break;
    }
}

void
do_uptime(dbref player)
{
    char buf[BUFFER_LEN];
    time_t startup = get_property_value(0, SYS_STARTUPTIME_PROP);
    strftime(buf, sizeof(buf), "%c %Z", localtime(&startup));
    notifyf(player, "Up %s since %s", timestr_long((long)(time(NULL) - startup)), buf);
}

void
do_score(dbref player)
{
    notifyf(player, "You have %d %s.", GETVALUE(player),
	    GETVALUE(player) == 1 ? tp_penny : tp_pennies);
}

void
do_inventory(dbref player)
{
    dbref thing;
    char unparse_buf[BUFFER_LEN];

    if ((thing = CONTENTS(player)) == NOTHING) {
	notify(player, "You aren't carrying anything.");
    } else {
	notify(player, "You are carrying:");
	DOLIST(thing, thing) {
            unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
	    notify(player, unparse_buf);
	}
    }

    do_score(player);
}

int
init_checkflags(dbref player, const char *flags, struct flgchkdat *check)
{
    char buf[BUFFER_LEN];
    char *cptr;
    int output_type = 0;
    int mode = 0;

    strcpyn(buf, sizeof(buf), flags);
    for (cptr = buf; *cptr && (*cptr != ARG_DELIMITER); cptr++) ;
    if (*cptr == ARG_DELIMITER)
	*(cptr++) = '\0';
    flags = buf;
    skip_whitespace((const char **)&cptr);

    if (!*cptr) {
	output_type = 0;
    } else if (string_prefix("owners", cptr)) {
	output_type = 1;
    } else if (string_prefix("locations", cptr)) {
	output_type = 3;
    } else if (string_prefix("links", cptr)) {
	output_type = 2;
    } else if (string_prefix("count", cptr)) {
	output_type = 4;
    } else if (string_prefix("size", cptr)) {
	output_type = 5;
    } else {
	output_type = 0;
    }

    check->fortype = 0;
    check->istype = 0;
    check->isnotroom = 0;
    check->isnotexit = 0;
    check->isnotthing = 0;
    check->isnotplayer = 0;
    check->isnotprog = 0;
    check->setflags = 0;
    check->clearflags = 0;

    check->forlevel = 0;
    check->islevel = 0;
    check->isnotzero = 0;
    check->isnotone = 0;
    check->isnottwo = 0;
    check->isnotthree = 0;

    check->forlink = 0;
    check->islinked = 0;
    check->forold = 0;
    check->isold = 0;

    check->loadedsize = 0;
    check->issize = 0;
    check->size = 0;

    while (*flags) {
	switch (toupper(*flags)) {
	case '!':
	    if (mode)
		mode = 0;
	    else
		mode = 2;
	    break;
	case 'R':
	    if (mode) {
		check->isnotroom = 1;
	    } else {
		check->fortype = 1;
		check->istype = TYPE_ROOM;
	    }
	    break;
	case 'T':
	    if (mode) {
		check->isnotthing = 1;
	    } else {
		check->fortype = 1;
		check->istype = TYPE_THING;
	    }
	    break;
	case 'E':
	    if (mode) {
		check->isnotexit = 1;
	    } else {
		check->fortype = 1;
		check->istype = TYPE_EXIT;
	    }
	    break;
	case 'P':
	    if (mode) {
		check->isnotplayer = 1;
	    } else {
		check->fortype = 1;
		check->istype = TYPE_PLAYER;
	    }
	    break;
	case 'F':
	    if (mode) {
		check->isnotprog = 1;
	    } else {
		check->fortype = 1;
		check->istype = TYPE_PROGRAM;
	    }
	    break;
	case '~':
	case '^':
	    check->loadedsize = (Wizard(player) && *flags == '^');
	    check->size = atoi(flags + 1);
	    check->issize = !mode;
	    while (isdigit(flags[1]))
		flags++;
	    break;
	case 'U':
	    check->forlink = 1;
	    if (mode) {
		check->islinked = 1;
	    } else {
		check->islinked = 0;
	    }
	    break;
	case '@':
	    check->forold = 1;
	    if (mode) {
		check->isold = 0;
	    } else {
		check->isold = 1;
	    }
	    break;
	case '0':
	    if (mode) {
		check->isnotzero = 1;
	    } else {
		check->forlevel = 1;
		check->islevel = 0;
	    }
	    break;
	case '1':
	    if (mode) {
		check->isnotone = 1;
	    } else {
		check->forlevel = 1;
		check->islevel = 1;
	    }
	    break;
	case '2':
	    if (mode) {
		check->isnottwo = 1;
	    } else {
		check->forlevel = 1;
		check->islevel = 2;
	    }
	    break;
	case '3':
	    if (mode) {
		check->isnotthree = 1;
	    } else {
		check->forlevel = 1;
		check->islevel = 3;
	    }
	    break;
	case 'M':
	    if (mode) {
		check->forlevel = 1;
		check->islevel = 0;
	    } else {
		check->isnotzero = 1;
	    }
	    break;
	case 'A':
	    if (mode)
		check->clearflags |= ABODE;
	    else
		check->setflags |= ABODE;
	    break;
	case 'B':
	    if (mode)
		check->clearflags |= BUILDER;
	    else
		check->setflags |= BUILDER;
	    break;
	case 'C':
	    if (mode)
		check->clearflags |= CHOWN_OK;
	    else
		check->setflags |= CHOWN_OK;
	    break;
	case 'D':
	    if (mode)
		check->clearflags |= DARK;
	    else
		check->setflags |= DARK;
	    break;
	case 'G':
	    if (mode)
		check->clearflags |= GUEST;
	    else
		check->setflags |= GUEST;
	    break;
	case 'H':
	    if (mode)
		check->clearflags |= HAVEN;
	    else
		check->setflags |= HAVEN;
	    break;
	case 'J':
	    if (mode)
		check->clearflags |= JUMP_OK;
	    else
		check->setflags |= JUMP_OK;
	    break;
	case 'K':
	    if (mode)
		check->clearflags |= KILL_OK;
	    else
		check->setflags |= KILL_OK;
	    break;
	case 'L':
	    if (mode)
		check->clearflags |= LINK_OK;
	    else
		check->setflags |= LINK_OK;
	    break;
	case 'O':
	    if (tp_enable_match_yield) {
		if (mode)
		    check->clearflags |= OVERT;
		else
		    check->setflags |= OVERT;
	    }
	    break;

	case 'Q':
	    if (mode)
		check->clearflags |= QUELL;
	    else
		check->setflags |= QUELL;
	    break;
	case 'S':
	    if (mode)
		check->clearflags |= STICKY;
	    else
		check->setflags |= STICKY;
	    break;
	case 'V':
	    if (mode)
		check->clearflags |= VEHICLE;
	    else
		check->setflags |= VEHICLE;
	    break;
	case 'Y':
	    if (tp_enable_match_yield) {
		if (mode)
		    check->clearflags |= YIELD;
		else
		    check->setflags |= YIELD;
	    }
	    break;
	case 'Z':
	    if (mode)
		check->clearflags |= ZOMBIE;
	    else
		check->setflags |= ZOMBIE;
	    break;
	case 'W':
	    if (mode)
		check->clearflags |= WIZARD;
	    else
		check->setflags |= WIZARD;
	    break;
	case 'X':
	    if (mode)
		check->clearflags |= XFORCIBLE;
	    else
		check->setflags |= XFORCIBLE;
	    break;
	case ' ':
	    if (mode)
		mode = 2;
	    break;
	}
	if (mode)
	    mode--;
	flags++;
    }
    return output_type;
}


int
checkflags(dbref what, struct flgchkdat check)
{
    if (check.fortype && (Typeof(what) != check.istype))
	return (0);
    if (check.isnotroom && (Typeof(what) == TYPE_ROOM))
	return (0);
    if (check.isnotexit && (Typeof(what) == TYPE_EXIT))
	return (0);
    if (check.isnotthing && (Typeof(what) == TYPE_THING))
	return (0);
    if (check.isnotplayer && (Typeof(what) == TYPE_PLAYER))
	return (0);
    if (check.isnotprog && (Typeof(what) == TYPE_PROGRAM))
	return (0);

    if (check.forlevel && (MLevRaw(what) != check.islevel))
	return (0);
    if (check.isnotzero && (MLevRaw(what) == 0))
	return (0);
    if (check.isnotone && (MLevRaw(what) == 1))
	return (0);
    if (check.isnottwo && (MLevRaw(what) == 2))
	return (0);
    if (check.isnotthree && (MLevRaw(what) == 3))
	return (0);

    if (FLAGS(what) & check.clearflags)
	return (0);
    if ((~FLAGS(what)) & check.setflags)
	return (0);

    if (check.forlink) {
	switch (Typeof(what)) {
	case TYPE_ROOM:
	    if ((DBFETCH(what)->sp.room.dropto == NOTHING) != (!check.islinked))
		return (0);
	    break;
	case TYPE_EXIT:
	    if ((!DBFETCH(what)->sp.exit.ndest) != (!check.islinked))
		return (0);
	    break;
	case TYPE_PLAYER:
	case TYPE_THING:
	    if (!check.islinked)
		return (0);
	    break;
	default:
	    if (check.islinked)
		return (0);
	}
    }
    if (check.forold) {
	if (((((time(NULL)) - DBFETCH(what)->ts_lastused) < tp_aging_time) ||
	     (((time(NULL)) - DBFETCH(what)->ts_modified) < tp_aging_time))
	    != (!check.isold))
	    return (0);
    }
    if (check.size) {
	if ((size_object(what, check.loadedsize) < check.size)
	    != (!check.issize)) {
	    return 0;
	}
    }
    return (1);
}

static void
display_objinfo(dbref player, dbref obj, int output_type)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char buf3[BUFFER_LEN];

    unparse_object(player, obj, buf2, sizeof(buf2));

    switch (output_type) {
    case 1:			/* owners */
        unparse_object(player, OWNER(obj), buf3, sizeof(buf3));
	snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
	break;
    case 2:			/* links */
	switch (Typeof(obj)) {
	case TYPE_ROOM:
            unparse_object(player, DBFETCH(obj)->sp.room.dropto, buf3, sizeof(buf3));
	    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
	    break;
	case TYPE_EXIT:
	    if (DBFETCH(obj)->sp.exit.ndest == 0) {
		snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, "*UNLINKED*");
		break;
	    }
	    if (DBFETCH(obj)->sp.exit.ndest > 1) {
		snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, "*METALINKED*");
		break;
	    }
            unparse_object(player, DBFETCH(obj)->sp.exit.dest[0], buf3, sizeof(buf3));
	    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
	    break;
	case TYPE_PLAYER:
            unparse_object(player, PLAYER_HOME(obj), buf3, sizeof(buf3));
	    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
	    break;
	case TYPE_THING:
            unparse_object(player, THING_HOME(obj), buf3, sizeof(buf3));
	    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
	    break;
	default:
	    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, "N/A");
	    break;
	}
	break;
    case 3:			/* locations */
        unparse_object(player, LOCATION(obj), buf3, sizeof(buf3));
	snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
	break;
    case 4:
	return;
    case 5:
	snprintf(buf, sizeof(buf), "%-38.512s  %ld bytes.", buf2, size_object(obj, 0));
	break;
    case 0:
    default:
	strcpyn(buf, sizeof(buf), buf2);
	break;
    }
    notify(player, buf);
}


void
do_find(dbref player, const char *name, const char *flags)
{
    struct flgchkdat check;
    char buf[BUFFER_LEN + 2];
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    strcpyn(buf, sizeof(buf), "*");
    strcatn(buf, sizeof(buf), name);
    strcatn(buf, sizeof(buf), "*");

    if (!payfor(player, tp_lookup_cost)) {
	notifyf(player, "You don't have enough %s.", tp_pennies);
    } else {
	for (dbref i = 0; i < db_top; i++) {
	    if ((Wizard(OWNER(player)) || OWNER(i) == OWNER(player)) &&
		checkflags(i, check) && NAME(i) && (!*name
						    || equalstr(buf, (char *) NAME(i)))) {
		display_objinfo(player, i, output_type);
		total++;
	    }
	}
	notify(player, "***End of List***");
	notifyf(player, "%d objects found.", total);
    }
}


void
do_owned(dbref player, const char *name, const char *flags)
{
    dbref victim;
    struct flgchkdat check;
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    if (!payfor(player, tp_lookup_cost)) {
	notifyf(player, "You don't have enough %s.", tp_pennies);
	return;
    }
    if (Wizard(OWNER(player)) && *name) {
	if ((victim = lookup_player(name)) == NOTHING) {
	    notify(player, "I couldn't find that player.");
	    return;
	}
    } else
	victim = player;

    for (dbref i = 0; i < db_top; i++) {
	if ((OWNER(i) == OWNER(victim)) && checkflags(i, check)) {
	    display_objinfo(player, i, output_type);
	    total++;
	}
    }
    notify(player, "***End of List***");
    notifyf(player, "%d objects found.", total);
}

void
do_trace(int descr, dbref player, const char *name, int depth)
{
    dbref thing;
    struct match_data md;
    char unparse_buf[BUFFER_LEN];

    if (*name == '\0' || !strcasecmp(name, "here")) {
	thing = LOCATION(player);
    } else {
	init_match(descr, player, name, NOTYPE, &md);
	match_everything(&md);

	if ((thing = noisy_match_result(&md)) == NOTHING)
	    return;
    }

    for (int i = 0; (!depth || i < depth) && thing != NOTHING; i++) {
	unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
	notify(player, unparse_buf);
	thing = LOCATION(thing);
    }
    notify(player, "***End of List***");
}

void
do_entrances(int descr, dbref player, const char *name, const char *flags)
{
    dbref thing;
    struct match_data md;
    struct flgchkdat check;
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    if (*name == '\0' || !strcasecmp(name, "here")) {
	thing = LOCATION(player);
    } else {
	init_match(descr, player, name, NOTYPE, &md);
	match_everything(&md);

	if ((thing = noisy_match_result(&md)) == NOTHING) {
	    return;
	}
    }

    if (!controls(OWNER(player), thing)) {
	notify(player,
	       "Permission denied. (You can't list entrances of objects you don't control)");
	return;
    }
    init_checkflags(player, flags, &check);
    for (dbref i = 0; i < db_top; i++) {
	if (checkflags(i, check)) {
	    switch (Typeof(i)) {
	    case TYPE_EXIT:
		for (int j = DBFETCH(i)->sp.exit.ndest; j--;) {
		    if (DBFETCH(i)->sp.exit.dest[j] == thing) {
			display_objinfo(player, i, output_type);
			total++;
		    }
		}
		break;
	    case TYPE_PLAYER:
		if (PLAYER_HOME(i) == thing) {
		    display_objinfo(player, i, output_type);
		    total++;
		}
		break;
	    case TYPE_THING:
		if (THING_HOME(i) == thing) {
		    display_objinfo(player, i, output_type);
		    total++;
		}
		break;
	    case TYPE_ROOM:
		if (DBFETCH(i)->sp.room.dropto == thing) {
		    display_objinfo(player, i, output_type);
		    total++;
		}
		break;
	    case TYPE_PROGRAM:
	    case TYPE_GARBAGE:
		break;
	    }
	}
    }
    notify(player, "***End of List***");
    notifyf(player, "%d objects found.", total);
}

void
do_contents(int descr, dbref player, const char *name, const char *flags)
{
    dbref i;
    dbref thing;
    struct match_data md;
    struct flgchkdat check;
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    if (*name == '\0' || !strcasecmp(name, "here")) {
	thing = LOCATION(player);
    } else {
	init_match(descr, player, name, NOTYPE, &md);
	match_everything(&md);

	if ((thing = noisy_match_result(&md)) == NOTHING) {
	    return;
	}
    }

    if (!controls(OWNER(player), thing)) {
	notify(player,
	       "Permission denied. (You can't get the contents of something you don't control)");
	return;
    }
    init_checkflags(player, flags, &check);
    DOLIST(i, CONTENTS(thing)) {
	if (checkflags(i, check)) {
	    display_objinfo(player, i, output_type);
	    total++;
	}
    }
    switch (Typeof(thing)) {
    case TYPE_EXIT:
    case TYPE_PROGRAM:
    case TYPE_GARBAGE:
	i = NOTHING;
	break;
    case TYPE_ROOM:
    case TYPE_THING:
    case TYPE_PLAYER:
	i = EXITS(thing);
	break;
    }
    DOLIST(i, i) {
	if (checkflags(i, check)) {
	    display_objinfo(player, i, output_type);
	    total++;
	}
    }
    notify(player, "***End of List***");
    notifyf(player, "%d objects found.", total);
}

static int
exit_matches_name(dbref exit, const char *name, int exactMatch)
{
    char buf[BUFFER_LEN];
    char *ptr;

    strcpyn(buf, sizeof(buf), NAME(exit));
    for (char *ptr2 = ptr = buf; *ptr; ptr = ptr2) {
	while (*ptr2 && *ptr2 != EXIT_DELIMITER)
	    ptr2++;
	if (*ptr2)
	    *ptr2++ = '\0';
	while (*ptr2 == EXIT_DELIMITER)
	    ptr2++;
	if ((exactMatch ? !strcasecmp(name, ptr) : string_prefix(name, ptr)) &&
	    DBFETCH(exit)->sp.exit.ndest)
	    return 1;
    }
    return 0;
}

static int
exit_match_exists(dbref player, dbref obj, const char *name, int exactMatch)
{
    dbref exit;

    exit = EXITS(obj);
    while (exit != NOTHING) {
	if (exit_matches_name(exit, name, exactMatch)) {
            char unparse_buf[BUFFER_LEN];
            unparse_object(player, obj, unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "  %ss are trapped on %.2048s", name, unparse_buf);
	    return 1;
	}
	exit = NEXTOBJ(exit);
    }
    return 0;
}

void
do_sweep(int descr, dbref player, const char *name)
{
    dbref thing, ref, loc;
    int flag, tellflag;
    struct match_data md;
    char buf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];

    if (*name == '\0' || !strcasecmp(name, "here")) {
	thing = LOCATION(player);
    } else {
	init_match(descr, player, name, NOTYPE, &md);
	match_everything(&md);

	if ((thing = noisy_match_result(&md)) == NOTHING) {
	    return;
	}
    }

    if (*name && !controls(OWNER(player), thing)) {
	notify(player,
	       "Permission denied. (You can't perform a security sweep in a room you don't own)");
	return;
    }
    
    unparse_object(player, thing, unparse_buf, sizeof unparse_buf);
    notifyf(player, "Listeners in %s:", unparse_buf);

    ref = CONTENTS(thing);
    for (; ref != NOTHING; ref = NEXTOBJ(ref)) {
	switch (Typeof(ref)) {
	case TYPE_PLAYER:
	    if (!Dark(thing) || online(ref)) {
                unparse_object(player, ref, unparse_buf, sizeof unparse_buf);
		notifyf(player, "  %s is a %splayer.", unparse_buf,
			online(ref) ? "" : "sleeping ");
	    }
	    break;
	case TYPE_THING:
	    if (FLAGS(ref) & (ZOMBIE | LISTENER)) {
		tellflag = 0;
                unparse_object(player, ref, unparse_buf, sizeof unparse_buf);
		snprintf(buf, sizeof(buf), "  %.255s is a", unparse_buf);
		if (FLAGS(ref) & ZOMBIE) {
		    tellflag = 1;
		    if (!online(OWNER(ref))) {
			tellflag = 0;
			strcatn(buf, sizeof(buf), " sleeping");
		    }
		    strcatn(buf, sizeof(buf), " zombie");
		}
		if ((FLAGS(ref) & LISTENER) &&
		    (get_property(ref, LISTEN_PROPQUEUE) ||
		     get_property(ref, WLISTEN_PROPQUEUE) || get_property(ref, WOLISTEN_PROPQUEUE))) {
		    strcatn(buf, sizeof(buf), " listener");
		    tellflag = 1;
		}
		strcatn(buf, sizeof(buf), " object owned by ");
                unparse_object(player, OWNER(ref), unparse_buf, sizeof unparse_buf);
		strcatn(buf, sizeof(buf), unparse_buf);
		strcatn(buf, sizeof(buf), ".");
		if (tellflag)
		    notify(player, buf);
	    }
	    exit_match_exists(player, ref, "page", 0);
	    exit_match_exists(player, ref, "whisper", 0);
	    if (!exit_match_exists(player, ref, "pose", 1))
		if (!exit_match_exists(player, ref, "pos", 1))
		    exit_match_exists(player, ref, "po", 1);
	    exit_match_exists(player, ref, "say", 0);
	    break;
	}
    }
    flag = 0;
    loc = thing;
    while (loc != NOTHING) {
	if (!flag) {
	    notify(player, "Listening rooms down the environment:");
	    flag = 1;
	}

	if ((FLAGS(loc) & LISTENER) &&
	    (get_property(loc, LISTEN_PROPQUEUE) ||
	     get_property(loc, WLISTEN_PROPQUEUE) || get_property(loc, WOLISTEN_PROPQUEUE))) {
            unparse_object(player, loc, unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "  %s is a listening room.", unparse_buf);
	}

	exit_match_exists(player, loc, "page", 0);
	exit_match_exists(player, loc, "whisper", 0);
	if (!exit_match_exists(player, loc, "pose", 1))
	    if (!exit_match_exists(player, loc, "pos", 1))
		exit_match_exists(player, loc, "po", 1);
	exit_match_exists(player, loc, "say", 0);

	loc = getparent(loc);
    }
    notify(player, "**End of list**");
}
