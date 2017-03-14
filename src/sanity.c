#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "interface.h"
#include "log.h"
#include "player.h"
#include "props.h"
#include "tune.h"

#include <stdarg.h>

#define SanFixed(ref, fixed) san_fixed_log((fixed), 1, (ref), -1)
#define SanFixed2(ref, ref2, fixed) san_fixed_log((fixed), 1, (ref), (ref2))
#define SanFixedRef(ref, fixed) san_fixed_log((fixed), 0, (ref), -1)

int sanity_violated = 0;

static void
flush_user_output(dbref player)
{
    int *darr;
    int dcount;
    struct descriptor_data *d;

    darr = get_player_descrs(OWNER(player), &dcount);
    for (int di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d) {
            process_output(d);
        }
    }
}

static void
SanPrint(dbref player, const char *format, ...)
{
    va_list args;
    char buf[16384];
    static int san_linesprinted = 0;

    va_start(args, format);

    vsnprintf(buf, sizeof(buf), format, args);
    if (player == NOTHING) {
	fprintf(stdout, "%s\n", buf);
	fflush(stdout);
    } else if (player == AMBIGUOUS) {
	fprintf(stderr, "%s\n", buf);
    } else {
	notify_nolisten(player, buf, 1);
	if (san_linesprinted++ > 100) {
	    flush_user_output(player);
	    san_linesprinted = 0;
	}
    }

    va_end(args);
}

static void
SanPrintObject(dbref player, const char *prefix, dbref ref)
{
    char unparse_buf[16384];
    unparse_object(NOTHING, ref, unparse_buf, sizeof(unparse_buf));
    SanPrint(player, "%s%s\n", prefix, unparse_buf);
}


void
do_examine_sanity(dbref player, const char *arg)
{
    dbref d;
    int result;

    if (player > 0) {
	result = sscanf(arg, "#%i", &d);
    } else {
	result = sscanf(arg, "%i", &d);
    }

    if (!result || !ObjExists(d)) {
	SanPrint(player, "Invalid Object.");
	return;
    }

    if (Typeof(d) == TYPE_GARBAGE) {
	SanPrint(player, "Object:         *GARBAGE* #%d", d);
    } else {
        SanPrintObject(player, "Object:         ", d);
    }

    SanPrintObject(player, "  Owner:          ", OWNER(d));
    SanPrintObject(player, "  Location:       ", LOCATION(d));
    SanPrintObject(player, "  Contents Start: ", CONTENTS(d));
    SanPrintObject(player, "  Exits Start:    ", EXITS(d));
    SanPrintObject(player, "  Next:           ", NEXTOBJ(d));

    switch (Typeof(d)) {
    case TYPE_THING:
	SanPrintObject(player, "  Home:           ", THING_HOME(d));
	SanPrint(player, "  Value:          %d", GETVALUE(d));
	break;

    case TYPE_ROOM:
	SanPrintObject(player, "  Drop-to:        ", DBFETCH(d)->sp.room.dropto);
	break;

    case TYPE_PLAYER:
	SanPrintObject(player, "  Home:           ", PLAYER_HOME(d));
	SanPrint(player, "  Pennies:        %d", GETVALUE(d));
	if (player < 0) {
	    SanPrint(player, "  Password MD5:   %s", PLAYER_PASSWORD(d));
	}
	break;

    case TYPE_EXIT:
	SanPrint(player, "  Links:");
	for (int i = 0; i < DBFETCH(d)->sp.exit.ndest; i++)
	    SanPrintObject(player, "    ", DBFETCH(d)->sp.exit.dest[i]);
	break;

    case TYPE_PROGRAM:
    case TYPE_GARBAGE:
    default:
	break;
    }
    SanPrint(player, "Referring Objects:");
    for (dbref i = 0; i < db_top; i++) {
	if (CONTENTS(i) == d) {
	    SanPrintObject(player, "  By contents field: ", i);
	}
	if (EXITS(i) == d) {
	    SanPrintObject(player, "  By exits field:    ", i);
	}
	if (NEXTOBJ(i) == d) {
	    SanPrintObject(player, "  By next field:     ", i);
	}
    }

    SanPrint(player, "Done.");
}

static void
violate(dbref player, dbref i, const char *s)
{
    char unparse_buf[16384];
    unparse_object(NOTHING, i, unparse_buf, sizeof(unparse_buf));
    SanPrint(player, "Object \"%s\" %s!", unparse_buf, s);
    sanity_violated = 1;
}

static void
check_next_chain(dbref player, dbref obj)
{
    dbref orig;

    orig = obj;
    while (obj != NOTHING && OkRef(obj)) {
	for (dbref i = orig; i != NOTHING; i = NEXTOBJ(i)) {
	    if (i == NEXTOBJ(obj)) {
		violate(player, obj,
			"has a 'next' field that forms an illegal loop in an object chain");
		return;
	    }
	    if (i == obj) {
		break;
	    }
	}
	obj = NEXTOBJ(obj);
    }
    if (!OkRef(obj)) {
	violate(player, obj, "has an invalid object in its 'next' chain");
    }
}

static void
find_orphan_objects(dbref player)
{
    SanPrint(player, "Searching for orphan objects...");

    for (dbref i = 0; i < db_top; i++) {
	FLAGS(i) &= ~SANEBIT;
    }

    if (recyclable != NOTHING) {
	FLAGS(recyclable) |= SANEBIT;
    }
    FLAGS(GLOBAL_ENVIRONMENT) |= SANEBIT;

    for (dbref i = 0; i < db_top; i++) {
	if (EXITS(i) != NOTHING) {
	    if (FLAGS(EXITS(i)) & SANEBIT) {
		violate(player, EXITS(i),
			"is referred to by more than one object's Next, Contents, or Exits field");
	    } else {
		FLAGS(EXITS(i)) |= SANEBIT;
	    }
	}
	if (CONTENTS(i) != NOTHING) {
	    if (FLAGS(CONTENTS(i)) & SANEBIT) {
		violate(player, CONTENTS(i),
			"is referred to by more than one object's Next, Contents, or Exits field");
	    } else {
		FLAGS(CONTENTS(i)) |= SANEBIT;
	    }
	}
	if (NEXTOBJ(i) != NOTHING) {
	    if (FLAGS(NEXTOBJ(i)) & SANEBIT) {
		violate(player, NEXTOBJ(i),
			"is referred to by more than one object's Next, Contents, or Exits field");
	    } else {
		FLAGS(NEXTOBJ(i)) |= SANEBIT;
	    }
	}
    }

    for (dbref i = 0; i < db_top; i++) {
	if (!(FLAGS(i) & SANEBIT)) {
	    violate(player, i,
		    "appears to be an orphan object, that is not referred to by any other object");
	}
    }

    for (dbref i = 0; i < db_top; i++) {
	FLAGS(i) &= ~SANEBIT;
    }
}

static void
check_room(dbref player, dbref obj)
{
    dbref i;

    i = DBFETCH(obj)->sp.room.dropto;

    if (!OkRef(i) && i != HOME) {
	violate(player, obj, "has its dropto set to an invalid object");
    } else if (i >= 0 && Typeof(i) != TYPE_THING && Typeof(i) != TYPE_ROOM) {
	violate(player, obj, "has its dropto set to a non-room, non-thing object");
    }
}

static void
check_thing(dbref player, dbref obj)
{
    dbref i;

    i = THING_HOME(obj);

    if (!OkObj(i)) {
	violate(player, obj, "has its home set to an invalid object");
    } else if (Typeof(i) != TYPE_ROOM && Typeof(i) != TYPE_THING && Typeof(i) != TYPE_PLAYER) {
	violate(player, obj,
		"has its home set to an object that is not a room, thing, or player");
    }
}

static void
check_exit(dbref player, dbref obj)
{
    if (DBFETCH(obj)->sp.exit.ndest < 0)
	violate(player, obj, "has a negative link count.");
    for (int i = 0; i < DBFETCH(obj)->sp.exit.ndest; i++) {
	if (!OkRef((DBFETCH(obj)->sp.exit.dest)[i]) &&
	    (DBFETCH(obj)->sp.exit.dest)[i] != HOME &&
	    (DBFETCH(obj)->sp.exit.dest)[i] != NIL) {
	    violate(player, obj, "has an invalid object as one of its link destinations");
	}
    }
}

static void
check_player(dbref player, dbref obj)
{
    dbref i;

    i = PLAYER_HOME(obj);

    if (!OkObj(i)) {
	violate(player, obj, "has its home set to an invalid object");
    } else if (i >= 0 && Typeof(i) != TYPE_ROOM) {
	violate(player, obj, "has its home set to a non-room object");
    }
}

static void
check_garbage(dbref player, dbref obj)
{
    if (NEXTOBJ(obj) != NOTHING && Typeof(NEXTOBJ(obj)) != TYPE_GARBAGE) {
	violate(player, obj,
		"has a non-garbage object as the 'next' object in the garbage chain");
    }
}

static void
check_contents_list(dbref player, dbref obj)
{
    dbref i;
    int limit;

    if (Typeof(obj) != TYPE_PROGRAM && Typeof(obj) != TYPE_EXIT && Typeof(obj) != TYPE_GARBAGE) {
	for (i = CONTENTS(obj), limit = db_top;
	     OkObj(i) &&
	     --limit && LOCATION(i) == obj && Typeof(i) != TYPE_EXIT; i = NEXTOBJ(i)) ;
	if (i != NOTHING) {
	    if (!limit) {
		check_next_chain(player, CONTENTS(obj));
		violate(player, obj,
			"is the containing object, and has the loop in its contents chain");
	    } else {
		if (!OkObj(i)) {
		    violate(player, obj, "has an invalid object in its contents list");
		} else {
		    if (Typeof(i) == TYPE_EXIT) {
			violate(player, obj,
				"has an exit in its contents list (it shoudln't)");
		    }
		    if (LOCATION(i) != obj) {
			violate(player, obj,
				"has an object in its contents lists that thinks it is located elsewhere");
		    }
		}
	    }
	}
    } else {
	if (CONTENTS(obj) != NOTHING) {
	    if (Typeof(obj) == TYPE_EXIT) {
		violate(player, obj, "is an exit/action whose contents aren't #-1");
	    } else if (Typeof(obj) == TYPE_GARBAGE) {
		violate(player, obj, "is a garbage object whose contents aren't #-1");
	    } else {
		violate(player, obj, "is a program whose contents aren't #-1");
	    }
	}
    }
}

static void
check_exits_list(dbref player, dbref obj)
{
    dbref i;
    int limit;

    if (Typeof(obj) != TYPE_PROGRAM && Typeof(obj) != TYPE_EXIT && Typeof(obj) != TYPE_GARBAGE) {
	for (i = EXITS(obj), limit = db_top;
	     OkObj(i) &&
	     --limit && LOCATION(i) == obj && Typeof(i) == TYPE_EXIT; i = NEXTOBJ(i)) ;
	if (i != NOTHING) {
	    if (!limit) {
		check_next_chain(player, CONTENTS(obj));
		violate(player, obj,
			"is the containing object, and has the loop in its exits chain");
	    } else if (!OkObj(i)) {
		violate(player, obj, "has an invalid object in it's exits list");
	    } else {
		if (Typeof(i) != TYPE_EXIT) {
		    violate(player, obj, "has a non-exit in it's exits list");
		}
		if (LOCATION(i) != obj) {
		    violate(player, obj,
			    "has an exit in its exits lists that thinks it is located elsewhere");
		}
	    }
	}
    } else {
	if (EXITS(obj) != NOTHING) {
	    if (Typeof(obj) == TYPE_EXIT) {
		violate(player, obj, "is an exit/action whose exits list isn't #-1");
	    } else if (Typeof(obj) == TYPE_GARBAGE) {
		violate(player, obj, "is a garbage object whose exits list isn't #-1");
	    } else {
		violate(player, obj, "is a program whose exits list isn't #-1");
	    }
	}
    }
}

static void
check_object(dbref player, dbref obj)
{
    /*
     * Do we have a name?
     */
    if (!NAME(obj))
	violate(player, obj, "doesn't have a name");

    /*
     * Check the ownership
     */
    if (Typeof(obj) != TYPE_GARBAGE) {
	if (!OkObj(OWNER(obj))) {
	    violate(player, obj, "has an invalid object as its owner.");
	} else if (Typeof(OWNER(obj)) != TYPE_PLAYER) {
	    violate(player, obj, "has a non-player object as its owner.");
	}

	/* 
	 * check location 
	 */
	if (!OkObj(LOCATION(obj)) && !(obj == GLOBAL_ENVIRONMENT && LOCATION(obj) == NOTHING)) {
	    violate(player, obj, "has an invalid object as it's location");
	}
    }

    if (LOCATION(obj) != NOTHING &&
	(Typeof(LOCATION(obj)) == TYPE_GARBAGE ||
	 Typeof(LOCATION(obj)) == TYPE_EXIT || Typeof(LOCATION(obj)) == TYPE_PROGRAM))
	violate(player, obj, "thinks it is located in a non-container object");

    if ((Typeof(obj) == TYPE_GARBAGE) && (LOCATION(obj) != NOTHING))
	violate(player, obj, "is a garbage object with a location that isn't #-1");

    check_contents_list(player, obj);
    check_exits_list(player, obj);

    switch (Typeof(obj)) {
    case TYPE_ROOM:
	check_room(player, obj);
	break;
    case TYPE_THING:
	check_thing(player, obj);
	break;
    case TYPE_PLAYER:
	check_player(player, obj);
	break;
    case TYPE_EXIT:
	check_exit(player, obj);
	break;
    case TYPE_PROGRAM:
	break;
    case TYPE_GARBAGE:
	check_garbage(player, obj);
	break;
    default:
	violate(player, obj, "has an unknown object type, and its flags may also be corrupt");
	break;
    }
}

void
do_sanity(dbref player)
{
    const int increp = 10000;
    int j;

    sanity_violated = 0;

    for (dbref i = 0; i < db_top; i++) {
	if (!(i % increp)) {
	    j = i + increp - 1;
	    j = (j >= db_top) ? (db_top - 1) : j;
	    SanPrint(player, "Checking objects %d to %d...", i, j);
	    if (player >= 0) {
		flush_user_output(player);
	    }
	}
	check_object(player, i);
    }

    find_orphan_objects(player);

    SanPrint(player, "Done.");
}

static void
san_fixed_log(char *format, int unparse, dbref ref1, dbref ref2)
{
    char buf1[4096];
    char buf2[4096];

    if (unparse) {
	if (ref1 >= 0) {
            unparse_object(NOTHING, ref1, buf1, sizeof(buf1));
	}
	if (ref2 >= 0) {
            unparse_object(NOTHING, ref2, buf2, sizeof(buf2));
	}
	log_sanfix(format, buf1, buf2);
    } else {
	log_sanfix(format, ref1, ref2);
    }
}

static void
cut_all_chains(dbref obj)
{
    if (CONTENTS(obj) != NOTHING) {
	SanFixed(obj, "Cleared contents of %s");
	CONTENTS(obj) = NOTHING;
	DBDIRTY(obj);
    }
    if (EXITS(obj) != NOTHING) {
	SanFixed(obj, "Cleared exits of %s");
	EXITS(obj) = NOTHING;
	DBDIRTY(obj);
    }
}

static void
cut_bad_recyclable(void)
{
    dbref loop, prev;

    loop = recyclable;
    prev = NOTHING;
    while (loop != NOTHING) {
	if (!OkRef(loop) || Typeof(loop) != TYPE_GARBAGE || FLAGS(loop) & SANEBIT) {
	    SanFixed(loop, "Recyclable object %s is not TYPE_GARBAGE");
	    if (prev != NOTHING) {
		NEXTOBJ(prev) = NOTHING;
		DBDIRTY(prev);
	    } else {
		recyclable = NOTHING;
	    }
	    return;
	}
	FLAGS(loop) |= SANEBIT;
	prev = loop;
	loop = NEXTOBJ(loop);
    }
}

static void
cut_bad_contents(dbref obj)
{
    dbref loop, prev;

    loop = CONTENTS(obj);
    prev = NOTHING;
    while (loop != NOTHING) {
	if (!OkObj(loop) || FLAGS(loop) & SANEBIT ||
	    Typeof(loop) == TYPE_EXIT || LOCATION(loop) != obj || loop == obj) {
	    if (!OkObj(loop)) {
		SanFixed(obj, "Contents chain for %s cut at invalid dbref");
	    } else if (Typeof(loop) == TYPE_EXIT) {
		SanFixed2(obj, loop, "Contents chain for %s cut at exit %s");
	    } else if (loop == obj) {
		SanFixed(obj, "Contents chain for %s cut at self-reference");
	    } else if (LOCATION(loop) != obj) {
		SanFixed2(obj, loop, "Contents chain for %s cut at misplaced object %s");
	    } else if (FLAGS(loop) & SANEBIT) {
		SanFixed2(obj, loop, "Contents chain for %s cut at already chained object %s");
	    } else {
		SanFixed2(obj, loop, "Contents chain for %s cut at %s");
	    }
	    if (prev != NOTHING) {
		NEXTOBJ(prev) = NOTHING;
		DBDIRTY(prev);
	    } else {
		CONTENTS(obj) = NOTHING;
		DBDIRTY(obj);
	    }
	    return;
	}
	FLAGS(loop) |= SANEBIT;
	prev = loop;
	loop = NEXTOBJ(loop);
    }
}

static void
cut_bad_exits(dbref obj)
{
    dbref loop, prev;

    loop = EXITS(obj);
    prev = NOTHING;
    while (loop != NOTHING) {
	if (!OkObj(loop) || FLAGS(loop) & SANEBIT ||
	    Typeof(loop) != TYPE_EXIT || LOCATION(loop) != obj) {
	    if (!OkObj(loop)) {
		SanFixed(obj, "Exits chain for %s cut at invalid dbref");
	    } else if (Typeof(loop) != TYPE_EXIT) {
		SanFixed2(obj, loop, "Exits chain for %s cut at non-exit %s");
	    } else if (LOCATION(loop) != obj) {
		SanFixed2(obj, loop, "Exits chain for %s cut at misplaced exit %s");
	    } else if (FLAGS(loop) & SANEBIT) {
		SanFixed2(obj, loop, "Exits chain for %s cut at already chained exit %s");
	    } else {
		SanFixed2(obj, loop, "Exits chain for %s cut at %s");
	    }
	    if (prev != NOTHING) {
		NEXTOBJ(prev) = NOTHING;
		DBDIRTY(prev);
	    } else {
		EXITS(obj) = NOTHING;
		DBDIRTY(obj);
	    }
	    return;
	}
	FLAGS(loop) |= SANEBIT;
	prev = loop;
	loop = NEXTOBJ(loop);
    }
}

static void
hacksaw_bad_chains(void)
{
    cut_bad_recyclable();
    for (dbref loop = 0; loop < db_top; loop++) {
	if (Typeof(loop) != TYPE_ROOM && Typeof(loop) != TYPE_THING &&
	    Typeof(loop) != TYPE_PLAYER) {
	    cut_all_chains(loop);
	} else {
	    cut_bad_contents(loop);
	    cut_bad_exits(loop);
	}
    }
}

static char *
rand_password(void)
{
    char pwdchars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char password[17];
    int charslen = strlen(pwdchars);

    for (int loop = 0; loop < 16; loop++) {
	password[loop] = pwdchars[(RANDOM() >> 8) % charslen];
    }
    password[16] = 0;
    return alloc_string(password);
}

static void
create_lostandfound(dbref * player, dbref * room)
{
    char *player_name = "lost+found";
    char unparse_buf[16384];
    int temp = 0;

    *room = new_object();
    NAME(*room) = alloc_string("lost+found");
    LOCATION(*room) = GLOBAL_ENVIRONMENT;
    EXITS(*room) = NOTHING;
    DBFETCH(*room)->sp.room.dropto = NOTHING;
    FLAGS(*room) = TYPE_ROOM | SANEBIT;
    PUSH(*room, CONTENTS(GLOBAL_ENVIRONMENT));
    SanFixed(*room, "Using %s to resolve unknown location");

    while (lookup_player(player_name) != NOTHING && strlen(player_name) < (unsigned int)tp_player_name_limit) {
	snprintf(player_name, sizeof(player_name), "lost+found%d", ++temp);
    }
    if (strlen(player_name) >= (unsigned int)tp_player_name_limit) {
        unparse_object(NOTHING, GOD, unparse_buf, sizeof(unparse_buf));
	log_sanfix("WARNING: Unable to get lost+found player, using %s", unparse_buf);
	*player = GOD;
    } else {
	const char *rpass;
	*player = new_object();
	NAME(*player) = alloc_string(player_name);
	LOCATION(*player) = *room;
	FLAGS(*player) = TYPE_PLAYER | SANEBIT;
	OWNER(*player) = *player;
	ALLOC_PLAYER_SP(*player);
	PLAYER_SET_HOME(*player, *room);
	EXITS(*player) = NOTHING;
	SETVALUE(*player, tp_start_pennies);
	rpass = rand_password();
	PLAYER_SET_PASSWORD(*player, NULL);
	set_password(*player, rpass);
	PLAYER_SET_CURR_PROG(*player, NOTHING);
	PLAYER_SET_INSERT_MODE(*player, 0);
	PUSH(*player, CONTENTS(*room));
	DBDIRTY(*player);
	add_player(*player);
        unparse_object(NOTHING, *player, unparse_buf, sizeof(unparse_buf));
	log_sanfix("Using %s (with password %s) to resolve unknown owner",
		unparse_buf, rpass);
    }
    OWNER(*room) = *player;
    DBDIRTY(*room);
    DBDIRTY(*player);
    DBDIRTY(GLOBAL_ENVIRONMENT);
}

static void
fix_room(dbref obj)
{
    dbref i;

    i = DBFETCH(obj)->sp.room.dropto;

    if (!OkRef(i) && i != HOME) {
	SanFixed(obj, "Removing invalid drop-to from %s");
	DBFETCH(obj)->sp.room.dropto = NOTHING;
	DBDIRTY(obj);
    } else if (i >= 0 && Typeof(i) != TYPE_THING && Typeof(i) != TYPE_ROOM) {
	SanFixed2(obj, i, "Removing drop-to on %s to %s");
	DBFETCH(obj)->sp.room.dropto = NOTHING;
	DBDIRTY(obj);
    }
}

static void
fix_thing(dbref obj)
{
    dbref i;

    i = THING_HOME(obj);

    if (!OkObj(i) || (Typeof(i) != TYPE_ROOM && Typeof(i) != TYPE_THING &&
		      Typeof(i) != TYPE_PLAYER)) {
	SanFixed2(obj, OWNER(obj), "Setting the home on %s to %s, it's owner");
	THING_SET_HOME(obj, OWNER(obj));
	DBDIRTY(obj);
    }
}

static void
fix_exit(dbref obj)
{
    for (int i = 0; i < DBFETCH(obj)->sp.exit.ndest;) {
	if (!OkObj((DBFETCH(obj)->sp.exit.dest)[i]) && (DBFETCH(obj)->sp.exit.dest)[i] != HOME && (DBFETCH(obj)->sp.exit.dest)[i] != NIL) {
	    SanFixed(obj, "Removing invalid destination from %s");
	    DBFETCH(obj)->sp.exit.ndest--;
	    DBDIRTY(obj);
	    for (int j = i; j < DBFETCH(obj)->sp.exit.ndest; j++) {
		(DBFETCH(obj)->sp.exit.dest)[i] = (DBFETCH(obj)->sp.exit.dest)[i + 1];
	    }
	} else {
	    i++;
	}
    }
}

static void
fix_player(dbref obj)
{
    dbref i;

    i = PLAYER_HOME(obj);

    if (!OkObj(i) || Typeof(i) != TYPE_ROOM) {
	SanFixed2(obj, tp_player_start, "Setting the home on %s to %s");
	PLAYER_SET_HOME(obj, tp_player_start);
	DBDIRTY(obj);
    }
}

static void
find_misplaced_objects(void)
{
    dbref player = NOTHING, room = NOTHING;

    for (dbref loop = 0; loop < db_top; loop++) {
	if (Typeof(loop) != TYPE_ROOM &&
	    Typeof(loop) != TYPE_THING &&
	    Typeof(loop) != TYPE_PLAYER &&
	    Typeof(loop) != TYPE_EXIT &&
	    Typeof(loop) != TYPE_PROGRAM && Typeof(loop) != TYPE_GARBAGE) {
	    SanFixedRef(loop, "Object #%d is of unknown type");
	    sanity_violated = 1;
	    continue;
	}
	if (!NAME(loop) || !(*NAME(loop))) {
	    switch Typeof
		(loop) {
	    case TYPE_GARBAGE:
		NAME(loop) = "<garbage>";
		break;
	    case TYPE_PLAYER:
		{
		    char *name = "Unnamed";
		    int temp = 0;

		    while (lookup_player(name) != NOTHING && strlen(name) < (unsigned int)tp_player_name_limit) {
			snprintf(name, sizeof(name), "Unnamed%d", ++temp);
		    }
		    NAME(loop) = alloc_string(name);
		    add_player(loop);
		}
		break;
	    default:
		NAME(loop) = alloc_string("Unnamed");
		}
	    SanFixed(loop, "Gave a name to %s");
	    DBDIRTY(loop);
	}
	if (Typeof(loop) != TYPE_GARBAGE) {
	    if (!OkObj(OWNER(loop)) || Typeof(OWNER(loop)) != TYPE_PLAYER) {
		if (player == NOTHING) {
		    create_lostandfound(&player, &room);
		}
		SanFixed2(loop, player, "Set owner of %s to %s");
		OWNER(loop) = player;
		DBDIRTY(loop);
	    }
	    if (loop != GLOBAL_ENVIRONMENT && (!OkObj(LOCATION(loop)) ||
					       Typeof(LOCATION(loop)) == TYPE_GARBAGE ||
					       Typeof(LOCATION(loop)) == TYPE_EXIT ||
					       Typeof(LOCATION(loop)) == TYPE_PROGRAM ||
					       (Typeof(loop) == TYPE_PLAYER &&
						Typeof(LOCATION(loop)) == TYPE_PLAYER))) {
		if (Typeof(loop) == TYPE_PLAYER) {
		    if (OkObj(LOCATION(loop)) && Typeof(LOCATION(loop)) == TYPE_PLAYER) {
			dbref loop1;

			loop1 = LOCATION(loop);
			if (CONTENTS(loop1) == loop) {
			    CONTENTS(loop1) = NEXTOBJ(loop);
			    DBDIRTY(loop1);
			} else
			    for (loop1 = CONTENTS(loop1);
				 loop1 != NOTHING; loop1 = NEXTOBJ(loop1)) {
				if (NEXTOBJ(loop1) == loop) {
				    NEXTOBJ(loop1) = NEXTOBJ(loop);
				    DBDIRTY(loop1);
				    break;
				}
			    }
		    }
		    LOCATION(loop) = tp_player_start;
		} else {
		    if (player == NOTHING) {
			create_lostandfound(&player, &room);
		    }
		    LOCATION(loop) = room;
		}
		DBDIRTY(loop);
		DBDIRTY(LOCATION(loop));
		if (Typeof(loop) == TYPE_EXIT) {
		    PUSH(loop, EXITS(LOCATION(loop)));
		} else {
		    PUSH(loop, CONTENTS(LOCATION(loop)));
		}
		FLAGS(loop) |= SANEBIT;
		SanFixed2(loop, LOCATION(loop), "Set location of %s to %s");
	    }
	} else {
	    if (OWNER(loop) != NOTHING) {
		SanFixedRef(loop, "Set owner of recycled object #%d to NOTHING");
		OWNER(loop) = NOTHING;
		DBDIRTY(loop);
	    }
	    if (LOCATION(loop) != NOTHING) {
		SanFixedRef(loop, "Set location of recycled object #%d to NOTHING");
		LOCATION(loop) = NOTHING;
		DBDIRTY(loop);
	    }
	}
	switch (Typeof(loop)) {
	case TYPE_ROOM:
	    fix_room(loop);
	    break;
	case TYPE_THING:
	    fix_thing(loop);
	    break;
	case TYPE_PLAYER:
	    fix_player(loop);
	    break;
	case TYPE_EXIT:
	    fix_exit(loop);
	    break;
	case TYPE_PROGRAM:
	case TYPE_GARBAGE:
	    break;
	}
    }
}

static void
adopt_orphans(void)
{
    for (dbref loop = 0; loop < db_top; loop++) {
	if (!(FLAGS(loop) & SANEBIT)) {
	    DBDIRTY(loop);
	    switch (Typeof(loop)) {
	    case TYPE_ROOM:
	    case TYPE_THING:
	    case TYPE_PLAYER:
	    case TYPE_PROGRAM:
		NEXTOBJ(loop) = CONTENTS(LOCATION(loop));
		CONTENTS(LOCATION(loop)) = loop;
		SanFixed2(loop, LOCATION(loop), "Orphaned object %s added to contents of %s");
		break;
	    case TYPE_EXIT:
		NEXTOBJ(loop) = EXITS(LOCATION(loop));
		EXITS(LOCATION(loop)) = loop;
		SanFixed2(loop, LOCATION(loop), "Orphaned exit %s added to exits of %s");
		break;
	    case TYPE_GARBAGE:
		NEXTOBJ(loop) = recyclable;
		recyclable = loop;
		SanFixedRef(loop, "Litter object %d moved to recycle bin");
		break;
	    default:
		sanity_violated = 1;
		break;
	    }
	}
    }
}

static void
clean_global_environment(void)
{
    if (NEXTOBJ(GLOBAL_ENVIRONMENT) != NOTHING) {
	SanFixed(GLOBAL_ENVIRONMENT, "Removed the global environment %s from a chain");
	NEXTOBJ(GLOBAL_ENVIRONMENT) = NOTHING;
	DBDIRTY(GLOBAL_ENVIRONMENT);
    }
    if (LOCATION(GLOBAL_ENVIRONMENT) != NOTHING) {
	SanFixed2(GLOBAL_ENVIRONMENT, LOCATION(GLOBAL_ENVIRONMENT),
		  "Removed the global environment %s from %s");
	LOCATION(GLOBAL_ENVIRONMENT) = NOTHING;
	DBDIRTY(GLOBAL_ENVIRONMENT);
    }
}

void
do_sanfix(dbref player)
{
    sanity_violated = 0;

    for (dbref loop = 0; loop < db_top; loop++) {
	FLAGS(loop) &= ~SANEBIT;
    }
    FLAGS(GLOBAL_ENVIRONMENT) |= SANEBIT;

    if (!OkObj(tp_player_start) || Typeof(tp_player_start) != TYPE_ROOM) {
	SanFixed(GLOBAL_ENVIRONMENT, "Reset invalid player_start to %s");
	tp_player_start = GLOBAL_ENVIRONMENT;
    }

    hacksaw_bad_chains();
    find_misplaced_objects();
    adopt_orphans();
    clean_global_environment();

    for (dbref loop = 0; loop < db_top; loop++) {
	FLAGS(loop) &= ~SANEBIT;
    }

    if (player > NOTHING) {
	if (!sanity_violated) {
	    notifyf_nolisten(player, "Database repair complete, please re-run"
			    " @sanity.  For details of repairs, check %s.", tp_file_log_sanfix);
	} else {
	    notify_nolisten(player, "Database repair complete, however the "
			    "database is still corrupt.  Please re-run @sanity.", 1);
	}
    } else {
	fprintf(stderr, "Database repair complete, ");
	if (!sanity_violated)
	    fprintf(stderr, "please re-run sanity check.\n");
	else
	    fprintf(stderr, "however the database is still corrupt.\n"
		    "Please re-run sanity check for details and fix it by hand.\n");
	fprintf(stderr, "For details of repairs made, check %s.\n", tp_file_log_sanfix);
    }
    if (sanity_violated) {
	log_sanfix("WARNING: The database is still corrupted, please repair by hand");
    }
}

static char cbuf[1000];
static char buf2[1000];

void
do_sanchange(dbref player, const char *command)
{
    dbref d, v;
    char field[50];
    char which[1000];
    char value[1000];
    char unparse_buf[1000];
    int *ip;
    int results;

    if (player > NOTHING) {
	results = sscanf(command, "%s %s %s", which, field, value);
	sscanf(which, "#%d", &d);
	sscanf(value, "#%d", &v);
    } else {
	results = sscanf(command, "%s %s %s", which, field, value);
	sscanf(which, "%d", &d);
	sscanf(value, "%d", &v);
    }

    if (results != 3) {
	d = v = 0;
	strcpyn(field, sizeof(field), "help");
    }

    *buf2 = 0;

    if (!OkRef(d) || d < 0) {
	SanPrint(player, "## %d is an invalid dbref.", d);
	return;
    }

    unparse_object(NOTHING, v, unparse_buf, sizeof(unparse_buf));

    if (!strcasecmp(field, "next")) {
        unparse_object(NOTHING, NEXTOBJ(d), buf2, sizeof(buf2));
	NEXTOBJ(d) = v;
	DBDIRTY(d);
	SanPrint(player, "## Setting #%d's next field to %s", d, unparse_buf);

    } else if (!strcasecmp(field, "exits")) {
        unparse_object(NOTHING, EXITS(d), buf2, sizeof(buf2));
	EXITS(d) = v;
	DBDIRTY(d);
	SanPrint(player, "## Setting #%d's next field to %s", d, unparse_buf);

    } else if (!strcasecmp(field, "contents")) {
        unparse_object(NOTHING, CONTENTS(d), buf2, sizeof(buf2));
	CONTENTS(d) = v;
	DBDIRTY(d);
	SanPrint(player, "## Setting #%d's Contents list start to %s", d, unparse_buf);

    } else if (!strcasecmp(field, "location")) {
        unparse_object(NOTHING, LOCATION(d), buf2, sizeof(buf2));
	LOCATION(d) = v;
	DBDIRTY(d);
	SanPrint(player, "## Setting #%d's location to %s", d, unparse_buf);

    } else if (!strcasecmp(field, "owner")) {
        unparse_object(NOTHING, OWNER(d), buf2, sizeof(buf2));
	OWNER(d) = v;
	DBDIRTY(d);
	SanPrint(player, "## Setting #%d's owner to %s", d, unparse_buf);

    } else if (!strcasecmp(field, "home")) {
	switch (Typeof(d)) {
	case TYPE_PLAYER:
	    ip = &(PLAYER_HOME(d));
	    break;

	case TYPE_THING:
	    ip = &(THING_HOME(d));
	    break;

	default:
	    printf("%s has no home to set.\n", unparse_buf);
	    return;
	}

        unparse_object(NOTHING, *ip, buf2, sizeof(buf2));
	*ip = v;
	DBDIRTY(d);
	SanPrint(player, "## Setting #%d's home to: %s\n", d, unparse_buf);

    } else {
	if (player > NOTHING) {
	    notify(player, "@sanchange <dbref> <field> <object>");
	} else {
	    SanPrint(player, "change command help:");
	    SanPrint(player, "c <dbref> <field> <object>");
	}
	SanPrint(player, "Fields are:     exits       Start of Exits list.");
	SanPrint(player, "                contents    Start of Contents list.");
	SanPrint(player, "                next        Next object in list.");
	SanPrint(player, "                location    Object's Location.");
	SanPrint(player, "                home        Object's Home.");
	SanPrint(player, "                owner       Object's Owner.");
	return;
    }

    if (*buf2) {
	SanPrint(player, "## Old value was %s", buf2);
    }
}

static void
extract_prop(FILE * f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN * 2];
    char *ptr;
    const char *ptr2;
    char tbuf[50];

    if (PropType(p) == PROP_DIRTYP)
	return;

    for (ptr = buf, ptr2 = dir + 1; *ptr2;)
	*ptr++ = *ptr2++;
    for (ptr2 = PropName(p); *ptr2;)
	*ptr++ = *ptr2++;
    *ptr++ = PROP_DELIMITER;
    ptr2 = intostr(PropFlagsRaw(p) & ~(PROP_TOUCHED | PROP_ISUNLOADED));
    while (*ptr2)
	*ptr++ = *ptr2++;
    *ptr++ = PROP_DELIMITER;

    ptr2 = "";
    switch (PropType(p)) {
    case PROP_INTTYP:
	if (!PropDataVal(p))
	    return;
	ptr2 = intostr(PropDataVal(p));
	break;
    case PROP_FLTTYP:
	if (PropDataFVal(p) == 0.0)
	    return;
	snprintf(tbuf, sizeof(tbuf), "%.17g", PropDataFVal(p));
	ptr2 = tbuf;
	break;
    case PROP_REFTYP:
	if (PropDataRef(p) == NOTHING)
	    return;
	ptr2 = intostr(PropDataRef(p));
	break;
    case PROP_STRTYP:
	if (!*PropDataStr(p))
	    return;
	ptr2 = PropDataStr(p);
	break;
    case PROP_LOKTYP:
	if (PropFlags(p) & PROP_ISUNLOADED)
	    return;
	if (PropDataLok(p) == TRUE_BOOLEXP)
	    return;
	ptr2 = unparse_boolexp((dbref) 1, PropDataLok(p), 0);
	break;
    }
    while (*ptr2)
	*ptr++ = *ptr2++;
    *ptr++ = '\n';
    *ptr++ = '\0';
    if (fputs(buf, f) == EOF) {
	fprintf(stderr, "extract_prop(): failed write!\n");
	abort();
    }
}

static void
extract_props_rec(FILE * f, dbref obj, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN];

    if (!p)
	return;

    extract_props_rec(f, obj, dir, p->left);
    extract_prop(f, dir, p);
    if (PropDir(p)) {
	snprintf(buf, sizeof(buf), "%s%s%c", dir, PropName(p), PROPDIR_DELIMITER);
	extract_props_rec(f, obj, buf, PropDir(p));
    }
    extract_props_rec(f, obj, dir, p->right);
}


static void
extract_props(FILE * f, dbref obj)
{
    extract_props_rec(f, obj, "/", DBFETCH(obj)->properties);
}

static void
extract_program(FILE * f, dbref obj)
{
    char buf[BUFFER_LEN];
    FILE *pf;
    int c = 0;

    snprintf(buf, sizeof(buf), "muf/%d.m", obj);
    pf = fopen(buf, "rb");
    if (!pf) {
	fprintf(f, "  (No listing found)\n");
	return;
    }
    while (fgets(buf, BUFFER_LEN, pf)) {
	c++;
	fprintf(f, "%s", buf);	/* newlines automatically included */
    }
    fclose(pf);
    fprintf(f, "  End of program listing (%d lines)\n", c);
}

static void
extract_object(FILE * f, dbref d)
{
    char unparse_buf[1024];
#define unparse(x) (unparse_object(NOTHING, (x), unparse_buf, sizeof(unparse_buf)), unparse_buf)
    fprintf(f, "  #%d\n", d);
    fprintf(f, "  Object:         %s\n", unparse(d));
    fprintf(f, "  Owner:          %s\n", unparse(OWNER(d)));
    fprintf(f, "  Location:       %s\n", unparse(LOCATION(d)));
    fprintf(f, "  Contents Start: %s\n", unparse(CONTENTS(d)));
    fprintf(f, "  Exits Start:    %s\n", unparse(EXITS(d)));
    fprintf(f, "  Next:           %s\n", unparse(NEXTOBJ(d)));

    switch (Typeof(d)) {
    case TYPE_THING:
	fprintf(f, "  Home:           %s\n", unparse(THING_HOME(d)));
	fprintf(f, "  Value:          %d\n", GETVALUE(d));
	break;

    case TYPE_ROOM:
	fprintf(f, "  Drop-to:        %s\n", unparse(DBFETCH(d)->sp.room.dropto));
	break;

    case TYPE_PLAYER:
	fprintf(f, "  Home:           %s\n", unparse(PLAYER_HOME(d)));
	fprintf(f, "  Pennies:        %d\n", GETVALUE(d));
	break;

    case TYPE_EXIT:
	fprintf(f, "  Links:         ");
	for (int i = 0; i < DBFETCH(d)->sp.exit.ndest; i++)
	    fprintf(f, " %s;", unparse(DBFETCH(d)->sp.exit.dest[i]));
	fprintf(f, "\n");
	break;

    case TYPE_PROGRAM:
	fprintf(f, "  Listing:\n");
	extract_program(f, d);
	break;

    case TYPE_GARBAGE:
    default:
	break;
    }
#undef unparse

#ifdef DISKBASE
    fetchprops(d, NULL);
#endif

    if (DBFETCH(d)->properties) {
	fprintf(f, "  Properties:\n");
	extract_props(f, d);
    } else {
	fprintf(f, "  No properties\n");
    }
    fprintf(f, "\n");
}

static void
extract(void)
{
    dbref d;
    int i;
    char filename[80];
    FILE *f;

    i = sscanf(cbuf, "%*s %d %s", &d, filename);

    if (!OkObj(d)) {
	printf("%d is an invalid dbref.\n", d);
	return;
    }

    if (i == 2) {
	f = fopen(filename, "wb");
	if (!f) {
	    printf("Could not open file %s\n", filename);
	    return;
	}
	printf("Writing to file %s\n", filename);
    } else {
	f = stdout;
    }

    for (int i = 0; i < db_top; i++) {
	if ((OWNER(i) == d) && (Typeof(i) != TYPE_GARBAGE)) {
	    extract_object(f, i);
	}			/* extract only objects owned by this player */
    }				/* loop through db */

    if (f != stdout)
	fclose(f);

    printf("\nDone.\n");
}

static void
extract_single(void)
{
    dbref d;
    int i;
    char filename[80];
    FILE *f;

    i = sscanf(cbuf, "%*s %d %s", &d, filename);

    if (!OkObj(d)) {
	printf("%d is an invalid dbref.\n", d);
	return;
    }

    if (i == 2) {
	f = fopen(filename, "wb");
	if (!f) {
	    printf("Could not open file %s\n", filename);
	    return;
	}
	printf("Writing to file %s\n", filename);
    } else {
	f = stdout;
    }

    if (Typeof(d) != TYPE_GARBAGE) {
	extract_object(f, d);
    }
    /* extract only objects owned by this player */
    if (f != stdout)
	fclose(f);

    printf("\nDone.\n");
}

static void
hack_it_up(void)
{
    char *ptr;

    do {
	printf("\nCommand: (? for help)\n");
	fgets(cbuf, sizeof(cbuf), stdin);

	switch (tolower(cbuf[0])) {
	case 's':
	    printf("Running Sanity...\n");
	    do_sanity(NOTHING);
	    break;

	case 'f':
	    printf("Running Sanfix...\n");
	    do_sanfix(NOTHING);
	    break;

	case 'p':
	    for (ptr = cbuf; *ptr && !isspace(*ptr); ptr++) ;
	    if (*ptr)
		ptr++;
	    do_examine_sanity(NOTHING, ptr);
	    break;

	case 'w':
	    *buf2 = '\0';
	    sscanf(cbuf, "%*s %s", buf2);
	    if (*buf2) {
		printf("Writing database to %s...\n", buf2);
	    } else {
		printf("Writing database...\n");
	    }
	    do_dump(GOD, buf2);
	    printf("Done.\n");
	    break;

	case 'c':
	    for (ptr = cbuf; *ptr && !isspace(*ptr); ptr++) ;
	    if (*ptr)
		ptr++;
	    do_sanchange(NOTHING, ptr);
	    break;

	case 'x':
	    extract();
	    break;

	case 'y':
	    extract_single();
	    break;

	case 'h':
	case '?':
	    printf("\n");
	    printf("s                           Run Sanity checks on database\n");
	    printf("f                           Automatically fix the database\n");
	    printf("p <dbref>                   Print an object\n");
	    printf("q                           Quit\n");
	    printf("w <file>                    Write database to file.\n");
	    printf("c <dbref> <field> <value>   Change a field on an object.\n");
	    printf("                              (\"c ? ?\" for list)\n");
	    printf("x <dbref> [<filename>]      Extract all objects belonging to <dbref>\n");
	    printf("y <dbref> [<filename>]      Extract the single object <dbref>\n");
	    printf("?                           Help! (Displays this screen.\n");
	    break;
	}
    }

    while (cbuf[0] != 'q');

    printf("Quitting.\n\n");
}

void
san_main(void)
{
    char unparse_buf[1024];
    printf("\nEntering the Interactive Sanity DB editor.\n");
    printf("Good luck!\n\n");

    printf("Number of objects in DB is: %d\n", db_top - 1);
    unparse_object(NOTHING, GLOBAL_ENVIRONMENT, unparse_buf, sizeof(unparse_buf));
    printf("Global Environment is: %s\n", unparse_buf);

#ifdef GOD_PRIV
    unparse_object(NOTHING, GOD, unparse_buf, sizeof(unparse_buf));
    printf("God is: %s\n", unparse_buf);
    printf("\n");
#endif

    hack_it_up();

    printf("Exiting sanity editor...\n\n");
}
