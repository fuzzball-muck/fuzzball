/** @file player.c
 *
 * Implementation of a variety of player-specific operations, such as
 * creating a player, setting password, etc.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "commands.h"
#include "db.h"
#include "edit.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "hashtab.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "move.h"
#include "player.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

static hash_tab player_list[PLAYER_HASH_SIZE];

dbref
lookup_player(const char *name)
{
    hash_data *hd;

    if ((hd = find_hash(name, player_list, PLAYER_HASH_SIZE)) == NULL) {
	return NOTHING;
    } else {
	return (hd->dbval);
    }
}

int
check_password(dbref player, const char *password)
{
    char md5buf[64];
    const char *processed = password;
    const char *pword = PLAYER_PASSWORD(player);

    if (password == NULL) {
	MD5base64(md5buf, "", 0);
	processed = md5buf;
    } else {
	if (*password) {
	    MD5base64(md5buf, password, strlen(password));
	    processed = md5buf;
	}
    }

    if (!pword || !*pword)
	return 1;

    if (!strcmp(pword, processed))
	return 1;

    return 0;
}

void
set_password_raw(dbref player, const char *password)
{
    PLAYER_SET_PASSWORD(player, password);
    DBDIRTY(player);
}

void
set_password(dbref player, const char *password)
{
    char md5buf[64];
    const char *processed = password;

    if (*password) {
	MD5base64(md5buf, password, strlen(password));
	processed = md5buf;
    }

    free((void *) PLAYER_PASSWORD(player));

    set_password_raw(player, alloc_string(processed));
}

dbref
create_player(const char *name, const char *password)
{
    dbref player;

    if (!ok_object_name(name, TYPE_PLAYER) || !ok_password(password))
	return NOTHING;

    /* else he doesn't already exist, create him */
    player = new_object();

    /* initialize everything */
    NAME(player) = alloc_string(name);
    add_property(player, PLAYER_CREATED_AS_PROP, name, 0);
    LOCATION(player) = tp_player_start;
    FLAGS(player) = TYPE_PLAYER;
    OWNER(player) = player;
    ALLOC_PLAYER_SP(player);
    PLAYER_SET_HOME(player, tp_player_start);
    EXITS(player) = NOTHING;

    SETVALUE(player, tp_start_pennies);
    set_password_raw(player, NULL);
    set_password(player, password);
    PLAYER_SET_CURR_PROG(player, NOTHING);
    PLAYER_SET_INSERT_MODE(player, 0);
    PLAYER_SET_IGNORE_CACHE(player, NULL);
    PLAYER_SET_IGNORE_COUNT(player, 0);
    PLAYER_SET_IGNORE_LAST(player, NOTHING);

    /* link him to tp_player_start */
    PUSH(player, CONTENTS(tp_player_start));
    add_player(player);
    DBDIRTY(player);
    DBDIRTY(tp_player_start);

    set_flags_from_tunestr(player, tp_pcreate_flags);

    return player;
}

void
toad_player(int descr, dbref player, dbref victim, dbref recipient)
{
    char buf[BUFFER_LEN];

    send_contents(descr, victim, HOME);
    dequeue_prog(victim, 0);

    for (dbref stuff = 0; stuff < db_top; stuff++) {
        if (OWNER(stuff) == victim) {
            switch (Typeof(stuff)) {
            case TYPE_PROGRAM:
                dequeue_prog(stuff, 0);
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

    chown_macros(macrotop, victim, recipient);

    free((void *) PLAYER_PASSWORD(victim));
    PLAYER_SET_PASSWORD(victim, 0);

    delete_player(victim);
    snprintf(buf, sizeof(buf), "A slimy toad named %s", NAME(victim));
    free((void *) NAME(victim));
    NAME(victim) = alloc_string(buf);
    DBDIRTY(victim);

    boot_player_off(victim);

    free(PLAYER_DESCRS(victim));
    PLAYER_SET_DESCRS(victim, NULL);
    PLAYER_SET_DESCRCOUNT(victim, 0);

    ignore_remove_from_all_players(victim);
    ignore_flush_cache(victim);

    FREE_PLAYER_SP(victim);
    ALLOC_THING_SP(victim);
    THING_SET_HOME(victim, PLAYER_HOME(player));

    FLAGS(victim) = TYPE_THING;
    OWNER(victim) = player;
    if (tp_toad_recycle) {
        recycle(descr, player, victim);
    }
    SETVALUE(victim, 1);
}

/**
 * Implementation of @password command
 *
 * This changes the player's password after doing some validation checks.
 * Old password must be valid and new password must not have spaces in it.
 *
 * @param player the player doing the password change
 * @param old the player's old password
 * @param newobj the player's new password.
 */
void
do_password(dbref player, const char *old, const char *newobj)
{
    if (!PLAYER_PASSWORD(player) || !check_password(player, old)) {
        notify(player, "Sorry, old password did not match current password.");
    } else if (!ok_password(newobj)) {
        notify(player, "Bad new password (no spaces allowed).");
    } else {
        set_password(player, newobj);
        DBDIRTY(player);
        notify(player, "Password changed.");
    }
}

void
clear_players(void)
{
    kill_hash(player_list, PLAYER_HASH_SIZE, 0);
    return;
}

void
add_player(dbref who)
{
    hash_data hd;

    hd.dbval = who;
    if (add_hash(NAME(who), hd, player_list, PLAYER_HASH_SIZE) == NULL) {
	panic("Out of memory");
    } else {
	return;
    }
}

void
delete_player(dbref who)
{
    int result;
    char buf[BUFFER_LEN];
    char namebuf[BUFFER_LEN];
    dbref found, ren;
    int j;


    result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);

    if (result) {
	wall_wizards
		("## WARNING: Playername hashtable is inconsistent.  Rebuilding it.  Don't panic.");
	clear_players();
	for (dbref i = 0; i < db_top; i++) {
	    if (Typeof(i) == TYPE_PLAYER) {
		found = lookup_player(NAME(i));
		if (found != NOTHING) {
		    ren = (i == who) ? found : i;
		    j = 0;
		    do {
			snprintf(namebuf, sizeof(namebuf), "%s%d", NAME(ren), ++j);
		    } while (lookup_player(namebuf) != NOTHING);

		    snprintf(buf, sizeof(buf),
			     "## Renaming %s(#%d) to %s to prevent name collision.",
			     NAME(ren), ren, namebuf);
		    wall_wizards(buf);

		    log_status("SANITY NAME CHANGE: %s(#%d) to %s", NAME(ren), ren, namebuf);

		    if (ren == found) {
			free_hash(NAME(ren), player_list, PLAYER_HASH_SIZE);
		    }
		    change_player_name(ren, namebuf);
		    add_player(ren);
		} else {
		    add_player(i);
		}
	    }
	}
	result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);
	if (result) {
	    wall_wizards
		    ("## WARNING: Playername hashtable still inconsistent.  Now you can panic.");
	}
    }

    return;
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
ok_password(const char *password)
{
    /* Password cannot be blank */
    if (*password == '\0')
        return 0;

    /* Password also cannot contain any nonprintable or space-type
     * characters */
    for (const char *scan = password; *scan; scan++) {
        if (!(isprint(*scan) && !isspace(*scan))) {
            return 0;
        }
    }

    /* Anything else is fair game */
    return 1;
}

void
change_player_name(dbref player, const char *name)
{
    char buf[BUFFER_LEN];
    PropPtr propadr, pptr;
    char propname[BUFFER_LEN];
    time_t t, now = time(NULL), cutoff = now - tp_pname_history_threshold;

    if (tp_pname_history_threshold > 0) {
	propadr = first_prop(player, PNAME_HISTORY_PROPDIR, &pptr, propname, sizeof(propname));
	while (propadr) {
	    t = atoi(propname);
	    if (t > cutoff) break;
	    snprintf(buf, sizeof(buf), "%s/%d", PNAME_HISTORY_PROPDIR, (int)t);
            propadr = next_prop(pptr, propadr, propname, sizeof(propname));
	    remove_property(player, buf, 0);
	}
    }

    snprintf(buf, sizeof(buf), "%s/%d", PNAME_HISTORY_PROPDIR, (int)now);
    add_property(player, buf, name, 0);

    free((void *) NAME(player));

    NAME(player) = alloc_string(name);
    ts_modifyobject(player);
}
