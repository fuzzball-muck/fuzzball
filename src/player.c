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

/**
 * @var player_list
 *
 * Hash table for quick player lookups.  This is set up when the DB is
 * loaded and maintained as players are created and deleted.
 *
 * This is not threadsafe but could easily be so with a mutex.
 */
static hash_tab player_list[PLAYER_HASH_SIZE];

/**
 * Look up a player by name
 *
 * Takes a name and searches the player list for that plaer.  Returns
 * the dbref of the player or NOTHING if no player is found.  The
 * search is case insensitive.
 *
 * @see find_hash
 *
 * @param name the name to look for
 * @return dbref of player
 */
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

/**
 * Check a player's password
 *
 * Given a password, this will return true if it matches the player's
 * password in the DB or false if it does not.  It will hash 'password'
 * to compare it.
 *
 * @param player the player ref to check a password for
 * @param password the raw password that we will hash and compare
 * @return boolean true if the password is correct, false otherwise
 */
int
check_password(dbref player, const char *password)
{
    char md5buf[128];
    int len_hash = 0;
    int len_salt = 0;

    const char* salt;

    const char *processed = password;
    const char *pword = PLAYER_PASSWORD(player);

    if (!pword || !*pword) {
        return 1;
    }

    /*
     * Get the hash length
     */
    len_hash = strlen(pword);

    /*
     * Is it a seeded hash?  If so, let's extract the seed.
     */
    if ((len_hash > 4) && (!strncmp(pword, "$1$", 3))) {
        /* Figure out the seed */
        salt = pword + 3;

        for ( ; (salt[len_salt] != '$') && ((len_salt+3) < len_hash);
             len_salt++) { }

        pbkdf2_hash(password, strlen(password), salt, len_salt, md5buf,
                    sizeof(md5buf));

        processed = md5buf;
    } else {
        if (password == NULL) {
            MD5base64(md5buf, "", 0);
            processed = md5buf;
        } else {
            if (*password) {
                MD5base64(md5buf, password, strlen(password));
                processed = md5buf;
            }
        }
    }

    /*
     * There was a bug where the password hash was causing a buffer
     * overflow.  Some compilers apparently cover this up or smooth
     * this over in some fashion which means it is an inconsistent
     * overflow.
     *
     * By matching by the length of 'processed', we'll be able to
     * support any old "too long" hashes that may have slipped into
     * the system.
     */
    if (!strncmp(pword, processed, strlen(processed)))
        return 1;

    return 0;
}

/**
 * Set a player's password without hashing it
 *
 * You usually don't want to use this call.
 *
 * @param player the player ref to set a password for
 * @param password the password that will be set as-as unhashed
 */
void
set_password_raw(dbref player, const char *password)
{
    PLAYER_SET_PASSWORD(player, password);
    DBDIRTY(player);
}

/**
 * Set a player's password
 *
 * This takes a 'raw' unhashed password and hashes it before setting it.
 *
 * @param player the player ref to set a password for
 * @param password the password that will be hashed and set
 */
void
set_password(dbref player, const char *password)
{
    char md5buf[128];
    const char *processed = password;

    if (*password) {
        if (tp_legacy_password_hash) {
            MD5base64(md5buf, password, strlen(password));
        } else {
            pbkdf2_hash(password, strlen(password), NULL, 0, md5buf,
                        sizeof(md5buf));
        }

        processed = md5buf;
    }

    free((void *) PLAYER_PASSWORD(player));

    set_password_raw(player, alloc_string(processed));
}

/**
 * Create a player
 *
 * Takes a name and a password, and creates a player.  Returns the
 * created player ref or NOTHING if the player could not be created
 * because either ok_object_name or ok_password failed.
 *
 * tp_player_start, tp_start_pennies and tp_pcreate_flags influence player
 * creation in what should be obvious ways.
 *
 * Uses an error parameter to communicate the reason for failure. This
 * should be at least SMALL_BUFFER_LEN in size.
 *
 * @see ok_object_name
 * @see ok_password
 *
 * @param name the player name to create
 * @param password the password that will be hashed and set
 * @param[out] error why the create failed
 * @return the dbref of the new program, or NOTHING if it failed.
 */
dbref
create_player(const char *name, const char *password, char *error)
{
    dbref player;

    if (!ok_object_name(name, TYPE_PLAYER)) {
        snprintf(error, SMALL_BUFFER_LEN, "You cannot use that name for a player.");
        return NOTHING;
    }

    if (!ok_password(password)) {
        snprintf(error, SMALL_BUFFER_LEN, "You cannot use that password.");
        return NOTHING;
    }

    player = new_object(true);

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

/**
 * Toads (deletes) a player
 *
 * This transfers the player's posessions to 'recipient' and clears
 * the player's various special fields.  The player is converted to a regular
 * object which is named a "toad", which is subsequently automatically
 * recycled if tp_toad_recycle is set.
 *
 * The player is kicked off if they are online.
 *
 * @param descr the descriptor of the player DOING the deletion
 * @param player the player DOING the deletion
 * @param victim the player being deleted
 * @param recipient the person to receive victim's owned objects
 */
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
                    /* fall through */

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
 * Implementation of \@password command
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

/**
 * Clears the player lookup hash
 *
 * This is used to clean up memory on shutdown; it is also used if the
 * player hash gets inconsistent and needs to be reloaded which is a case
 * that should never happens.
 *
 * The MUCK will break pretty severely if you call this but do not reload
 * the cache afterwards.
 */
void
clear_players(void)
{
    kill_hash(player_list, PLAYER_HASH_SIZE, 0);
    return;
}

/**
 * Adds a player to the player lookup cache: WARNING poorly named function
 *
 * This does NOT actually create a player -- @see create_player
 *
 * @TODO: Rename this to player_hash_add or something similar to that
 *        to make it more clear.
 *
 * @param who the player ref to add to the hash
 */
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

/**
 * Deletes a player from the lookup cache: WARNING poorly named function
 *
 * This does not actually delete a player -- @see toad_player
 *
 * This will attempt to fix the player hash if it is determined to be
 * inconsistent (i.e. if free_hash returns non-0.  @see free_hash)
 *
 * The wizards will be notified if there is a failure here.  @see wall_wizards
 *
 * @TODO: Rename this to player_hash_delete or something similar to that
 *        to make it more clear.
 *
 * @param who the player to remove from the cache
 */
void
delete_player(dbref who)
{
    int result;
    /*
     * TODO: I increased the buffer size here to stifle a warning.
     *       The underlying error is namebuf is actually too large.
     *       The snprinft below for "Renaming %s(#%d)..." was throwing
     *       a warning.
     */
    char buf[BUFFER_LEN+BUFFER_LEN];
    char namebuf[BUFFER_LEN];
    dbref found, ren;
    int j;

    result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);

    if (result) {
        wall_wizards(
            "## WARNING: Playername hashtable is inconsistent.  "
            "Rebuilding it.  Don't panic."
        );

        clear_players();

        for (dbref i = 0; i < db_top; i++) {
            if (Typeof(i) == TYPE_PLAYER) {
                found = lookup_player(NAME(i));

                if (found != NOTHING) {
                    ren = (i == who) ? found : i;
                    j = 0;

                    /*
                     * TODO: This, technically, can enable player names
                     *       to exceed the max name length.  I'm not sure
                     *       we care since this should be a super rare
                     *       occasion?  But exceeding the max name length
                     *       could cause chaos in other areas.
                     *
                     *       I'm not even sure how to test this, it seems
                     *       like it would be from a catastrophic DB failure.
                     */

                    do {
                        snprintf(namebuf, sizeof(namebuf), "%s%d", NAME(ren),
                                 ++j);
                    } while (lookup_player(namebuf) != NOTHING);

                    snprintf(buf, sizeof(buf),
                             "## Renaming %s(#%d) to %s to prevent name "
                             "collision.",
                             NAME(ren), ren, namebuf);

                    wall_wizards(buf);

                    log_status("SANITY NAME CHANGE: %s(#%d) to %s",
                               NAME(ren), ren, namebuf);

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
            wall_wizards(
                "## WARNING: Playername hashtable still inconsistent.  "
                "Now you can panic."
            );
        }
    }

    return;
}

/**
 * Causes 'who' to pay 'cost' in pennies.
 *
 * Removes 'cost' value from 'who', and returns 1 if the act has been
 * paid for, else returns 0.  Wizards never pay.
 *
 * @param who the person or object that is paying
 * @param cost the amount to deduct
 * @return boolean true on success, false if who could not afford it
 */
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

/**
 * Check to see if the given password is valid
 *
 * * Passwords cannot be blank
 * * They cannot contain non-printable characters
 * * And also no space characters 
 *
 * @param password the password to check
 * @return boolean true if it is valid, false otherwise
 */
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

/**
 * Change a player's name
 *
 * This implements the logic around tp_pname_history_threshold
 * Name history is stored regardless of the tp_pname_history_reporting
 * option -- that option toggles the functionality of the PNAME_HISTORY
 * muf primitive.  @see prim_pname_history
 *
 * Names are expired if they are older than the tp_pname_history_threshhold.
 * If that variable is 0, names never expire from the history.
 *
 * @param player the player to change the name of
 * @param name the name to set on the player
 */
void
change_player_name(dbref player, const char *name)
{
    char buf[BUFFER_LEN];
    PropPtr propadr, pptr;
    char propname[BUFFER_LEN];
    time_t t, now = time(NULL), cutoff = now - tp_pname_history_threshold;

    if (tp_pname_history_threshold > 0) {
        propadr = first_prop(player, PNAME_HISTORY_PROPDIR, &pptr, propname,
                             sizeof(propname));
        while (propadr) {
            t = atoi(propname);

            if (t > cutoff)
                break;

            snprintf(buf, sizeof(buf), "%s/%d", PNAME_HISTORY_PROPDIR, (int)t);
            propadr = next_prop(pptr, propadr, propname, sizeof(propname));
            remove_property(player, buf);
        }
    }

    snprintf(buf, sizeof(buf), "%s/%d", PNAME_HISTORY_PROPDIR, (int)now);
    add_property(player, buf, name, 0);

    free((void *) NAME(player));

    NAME(player) = alloc_string(name);
    ts_modifyobject(player);
}
