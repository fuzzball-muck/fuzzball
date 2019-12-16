/** @file wiz.c
 *
 * Source defining primarily wizard related commands
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "autoconf.h"
#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "match.h"
#include "move.h"
#include "mpi.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

/**
 * Implementation of the @teleport command
 *
 * This is the MUCK's @teleport command.  It does do permission checking
 * and sometimes that gets pretty detailed -- the rules for teleporting
 * different things vary slightly.  However, in general, the player must
 * control both the object to be teleported and the destination -- or
 * the destination must be LINK_OK (things) or ABODE (rooms) based on what
 * is being teleported.
 *
 * The nuances beyond that are largely uninteresting as they have to do
 * with data integrity more than "business logic".  See the can_link_to
 * call as that is used to determine who can go where for THINGS, ROOMS,
 * and PROGRAMS.
 *
 * @see can_link_to
 *
 * Players cannot teleport other players if they are not a wizard.
 *
 * Garbage cannot be teleported.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param arg1 The person being teleported, or the destination to teleport
 *             'me' to if arg2 is empty stirng
 * @param arg2 the destination to deleport to or "" as noted for arg1
 */
void
do_teleport(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref victim;
    dbref destination;
    const char *to;
    char victim_name_buf[BUFFER_LEN];
    char destination_name_buf[BUFFER_LEN];
    struct match_data md;

    /* get victim, destination */
    if (*arg2 == '\0') {
        victim = player;
        to = arg1;
    } else {
        /* @TODO: I don't know the match code very well yet, but I do
         *        know this block is basically copy/pasted down a few
         *        lines below with a few variations based on wizard or
         *        not.
         *
         *        It seems like should either be offering a set of
         *        different types of matches which are aggregations of these,
         *        more granular match types or maybe a call where the
         *        match_* calls are flags instead of a series of longform
         *        calls.  This just seems repetitive and not very user
         *        friendly
         *
         *        If we do a change like that, these styles of call
         *        live elsewhere and should change as well; though
         *        in most places they are not quite so blatantly repetitive.
         */
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

    if ((destination = noisy_match_result(&md)) == NOTHING) {
        return;
    }

    unparse_object(player, victim, victim_name_buf, sizeof(victim_name_buf));

    /* If we're sending the thing home, let's figure out where that is. */
    if (destination == HOME) {
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
            default: /* GARBAGE, for instance */
                destination = tp_player_start;  /* caught in the next
                                                 * switch anyway
                                                 */
                break;
        }
    }

    /* Sanity checking varies based on type
     *
     * @TODO: There's a lot of overlap here.  For instance, everything
     *        checks parent_loop_check except programs and garbage.
     *        Everything checks controls.  There's a few other checks
     *        that are very similar.
     *
     *        The "rub" is many of these checks have subtle variations
     *        based on type.  We could combine the logic, but it may
     *        be some really gnarly if statements.  The task here is
     *        to map it out a bit and see if we can condense this into
     *        more common checks.
     *
     *        The answer here may be to do nothing, but this seems like
     *        its a lot of flailing around with similar checks.
     */
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

            if (Typeof(destination) != TYPE_ROOM && 
                Typeof(destination) != TYPE_THING) {
                notify(player, "Bad destination.");
                break;
            }

            if (!Wizard(victim) &&
                (Typeof(destination) == TYPE_THING && 
                 !(FLAGS(destination) & VEHICLE))) {
                notify(player, "Destination object is not a vehicle.");
                break;
            }

            if (parent_loop_check(victim, destination)) {
                notify(player, "Objects can't contain themselves.");
                break;
            }

            notify(victim, "You feel a wrenching sensation...");
            unparse_object(player, destination, destination_name_buf,
                           sizeof(destination_name_buf));
            enter_room(descr, victim, destination, LOCATION(victim));
            notifyf(player, "%s teleported to %s.", victim_name_buf,
                    destination_name_buf);
            break;
        case TYPE_THING:
            if (parent_loop_check(victim, destination)) {
                notify(player, "You can't make a container contain itself!");
                break;
            }
        case TYPE_PROGRAM:
            if (Typeof(destination) != TYPE_ROOM
                && Typeof(destination) != TYPE_PLAYER
                && Typeof(destination) != TYPE_THING) {
                notify(player, "Bad destination.");
                break;
            }

            if (!((controls(player, destination) ||
                can_link_to(player, NOTYPE, destination)) &&
                (controls(player, victim) ||
                 controls(player, LOCATION(victim))))) {
                notify(player,
                       "Permission denied. (must control dest and be able to link to it, or control dest's loc)");
                break;
            }

            /* check for non-sticky dropto */
            if (Typeof(destination) == TYPE_ROOM
                && DBFETCH(destination)->sp.room.dropto != NOTHING
                && !(FLAGS(destination) & STICKY))
                destination = DBFETCH(destination)->sp.room.dropto;

            if (tp_secure_thing_movement && (Typeof(victim) == TYPE_THING)) {
                if (FLAGS(victim) & ZOMBIE) {
                    notify(victim, "You feel a wrenching sensation...");
                }

                enter_room(descr, victim, destination, LOCATION(victim));
            } else {
                moveto(victim, destination);
            }

            unparse_object(player, destination, destination_name_buf, sizeof(destination_name_buf));
            notifyf(player, "%s teleported to %s.", victim_name_buf,
                    destination_name_buf);
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
            unparse_object(player, destination, destination_name_buf, sizeof(destination_name_buf));
            notifyf(player, "Parent of %s set to %s.", victim_name_buf,
                    destination_name_buf);
            break;
        case TYPE_GARBAGE:
            notify(player, "That object is in a place where magic cannot reach it.");
            break;
        default:
            notify(player, "You can't teleport that.");
            break;
    }

    return;
}

/**
 * The wildcard implementation that underpins @bless (do_bless) and @unbless
 *
 * @see do_bless
 * @see do_unbless
 *
 * This is a recursive call.  'dir' should normally be "" when calling;
 * this is the base directory to scan for the passed wildcard, and when
 * entering this call you want to start your search at root (empty string).
 *
 * Two kinds of wildcards are understood; * and **.  ** is a recursive
 * wildcard, * is just a 'local' match.
 *
 * @private
 * @param player the player doing the operation
 * @param thing the thing we are operating on
 * @param dir the base prop dir -- should always be "" unless recursing.
 * @param wild the pattern we're searching for
 * @param blessp 1 if blessing, 0 if unblessing
 * @return number of props blessed/unblessed.
 */
static int
blessprops_wildcard(dbref player, dbref thing, const char *dir,
                    const char *wild, int blessp)
{
    char propname[BUFFER_LEN];
    char wld[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char *ptr, *wldcrd;
    PropPtr propadr, pptr;
    int i, cnt = 0;
    int recurse = 0;

    /* @TODO: This check is done by the caller methods so this is always
     *        redundant.  Worse: this is a recursive call, so we're punching
     *        this check over and over for no reason.  Granted, blessing
     *        doesn't come up that much :)  But still!  Remove this check
     *        and add a note to the docblock that the caller is responsible
     *        for permission checks.
     */
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

    /* Load the root propdir we're going to start iterating over, and then
     * scan it for the property(ies) we're looking for.
     */
    propadr = first_prop(thing, (char *) dir, &pptr, propname, sizeof(propname));

    while (propadr) {
        if (equalstr(wldcrd, propname)) {
            snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);

            /* @TODO: Is this check really necessary?  Perhaps
             *        the Prop_System one is (not sure), but the Prop_Hidden
             *        portion of this call will *always* pass because to get
             *        to this point in code you are *always* a wizard.
             *
             *        This strikes me as an inappropriate copy/paste because
             *        I've seen this exact if structure elsewhere.
             */
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

/**
 * Implementation of the @unbless command
 *
 * This does not handle basic permission checking.  The propname can have
 * wildcards in it.
 *
 * Two kinds of wildcards are understood; * and **.  ** is a recursive
 * wildcard, * is just a 'local' match.
 *
 * @param descr the descriptor of the user doing the command
 * @param player the player doing the command
 * @param what the object being operated on
 * @param propname the name of the prop to bless.
 */
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

/**
 * Implementation of the @bless command
 *
 * This does not handle basic permission checking.  The propname can have
 * wildcards in it.
 *
 * Two kinds of wildcards are understood; * and **.  ** is a recursive
 * wildcard, * is just a 'local' match.
 *
 * @param descr the descriptor of the user doing the command
 * @param player the player doing the command
 * @param what the object being operated on
 * @param propname the name of the prop to bless.
 */
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

    /* @TODO: Shouldn't this check be *first* rather than doing it last? */
#ifdef GOD_PRIV
    if (tp_strict_god_priv && !God(player) && God(OWNER(victim))) {
        notify(player, "Only God may touch God's stuff.");
        return;
    }
#endif

    cnt = blessprops_wildcard(player, victim, "", propname, 1);
    notifyf(player, "%d propert%s blessed.", cnt, (cnt == 1) ? "y" : "ies");
}

/**
 * The implementation of the @force command
 *
 * This uses the global 'force_level' to keep track of how many
 * layers of force calls are in play.  That makes this not threadsafe,
 * and its very critical force_level is maintained correctly through the
 * call.
 *
 * Does a number of permission checks -- checks to see if zombies are
 * allowed if trying to force a THING.  Permissions are checked, and
 * F-Lock is checked as well.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param what the target to be forced
 * @param command the command to run.
 */
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

    if (!tp_allow_zombies && (!Wizard(player) || Typeof(player) != TYPE_PLAYER)) {
        notify(player, "Zombies are not enabled here.");
        return;

#ifdef DEBUG
    } else {
        notify(player, "[debug] Zombies are not enabled for nonwizards -- force succeeded.");
#endif
    }

    /* @TODO: Another weird match block that could be refactored
     *        See do_teleport's todo
     */

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
#endif
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
#endif

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
    objnode_push(&forcelist, player);

    /* Keep track of force level */
    force_level++;
    process_command(dbref_first_descr(victim), victim, command);
    force_level--;
    objnode_pop(&forcelist);
}

/**
 * Implementation of the @stats command
 *
 * This displays stats on what is in the database, optionally showing
 * stats on what a given player owns.
 *
 * The stats returned are basic counts -- numbeer of rooms, objects, etc.
 * It loops over the entire DB to get this information.
 *
 * This does do permission checking
 *
 * @param player the player doing the call
 * @param name the name of the person to get stats for, or "" for global stats
 */
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

    /* @TODO: This if structure sucks.  The if !Wizard check should
     *        return rather than make this giant branch.
     */

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
                if ((OWNER(i) == owner) && 
                    DBFETCH(i)->propsmode != PROPS_UNLOADED)
                    loaded++;

                if ((OWNER(i) == owner) && 
                    DBFETCH(i)->propsmode == PROPS_CHANGED)
                    changed++;
#endif

                /* if unused for 90 days, inc oldobj count */
                if ((OWNER(i) == owner) &&
                    (currtime - DBFETCH(i)->ts_lastused) > tp_aging_time)
                    oldobjs++;

                switch (Typeof(i)) {
                    case TYPE_ROOM:
                        if (OWNER(i) == owner) {
                            total++;
                            rooms++;
                        }
                        break;

                    case TYPE_EXIT:
                        if (OWNER(i) == owner) {
                            total++;
                            exits++;
                        }
                        break;

                    case TYPE_THING:
                        if (OWNER(i) == owner) {
                            total++;
                            things++;
                        }
                        break;

                    case TYPE_PLAYER:
                        if (i == owner) {
                            total++;
                            players++;
                        }
                        break;

                    case TYPE_PROGRAM:
                        if (OWNER(i) == owner) {
                            total++;
                            programs++;
                        }
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
                if ((currtime - DBFETCH(i)->ts_lastused) > tp_aging_time)
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

/**
 * Implementation of the @boot command
 *
 * WARNING: This call does NOT check to see if 'player' is a wizard.
 *
 * @param player the player doing the @boot command
 * @param name the string name that will be matched for the player to boot.
 */ 
void
do_boot(dbref player, const char *name)
{
    dbref victim;

    /* @TODO: So most wizard commands like @armageddon and @bless check
     *        to see if the caller is a wizard.  This relies on a check
     *        in the giant switch statement in game.c -- @armageddon and
     *        @bless also have that same check.
     *
     *        This should be done consistently.  I think it should probably
     *        be in the functions themselves like do_bless or do_armageddon
     *
     *        The only problem with that approach is it makes for a lot
     *        of duplicate code.  So perhaps we should use the WIZARDONLY
     *        define *in* the functions, so the logic isn't baked into
     *        the monster switch.
     *
     *        If you fix this, remember to change the docblock.
     */

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
#endif  /* GOD_PRIV */
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

/**
 * Implementation of the @toad command
 *
 * Toading is the deletion of a player.  This is a wrapper around
 * toad_player, so the mechanics of toading are in that call.
 *
 * @see toad_player
 *
 * This does all the permission checking around toading and verifies
 * the target player is valid for toading.  This will not allow you to
 * toad yourself which is a protection against wiping all players out of
 * a DB.
 *
 * @param descr the descriptor of the person doing the toading
 * @param player the player doing the toading
 * @param name the name of the person being toaded
 * @param recip the person to receive the toaded player's owned things or ""
 *
 * If no toad recipient is provided, then it goes to the system default.
 */
void
do_toad(int descr, dbref player, const char *name, const char *recip)
{
    dbref victim;
    dbref recipient;

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
         * do it.
         */
        notify(player, "You cannot toad yourself.  Get someone else to do it for you.");
        return;
    }

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->type == TP_TYPE_DBREF && victim == *tent->currentval.d) {
            notify(player, "That player cannot currently be @toaded.");
            return;
        }
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
        notify(victim, "You have been turned into a toad.");
        notifyf(player, "You turned %s into a toad!", NAME(victim));
        log_status("TOADED: %s(%d) by %s(%d)", NAME(victim), victim, NAME(player), player);

        toad_player(descr, player, victim, recipient);
    }
}

/**
 * Implements the @newpassword command
 *
 * Changes the password to 'password' on a given player 'name'.
 *
 * This does NOT do permission checks -- it does a limited set around
 * changing wizard passwords, but it doesn't check permissions for
 * regular players.
 *
 * @param player the player doing the command
 * @param name the player who's password we are changing
 * @param password the new password
 */
void
do_newpassword(dbref player, const char *name, const char *password)
{
    dbref victim;

    if ((victim = lookup_player(name)) == NOTHING) {
        notify(player, "No such player.");
    } else if (!ok_password(password)) {
        notify(player, "Invalid password.");

#ifdef GOD_PRIV
    } else if (God(victim) && !God(player)) {
        notify(player, "You can't change God's password!");
        return;
    } else {
        if (TrueWizard(victim) && !God(player)) {
            notify(player, "Only God can change a wizard's password.");
            return;
        }
#else       /* GOD_PRIV */
    } else {
#endif      /* GOD_PRIV */

        /* it's ok, do it */
        set_password(victim, password);
        DBDIRTY(victim);
        notify(player, "Password changed.");
        notifyf(victim, "Your password has been changed by %s.", NAME(player));
        log_status("NEWPASS'ED: %s(%d) by %s(%d)", NAME(victim), victim, NAME(player), player);
    }
}

/**
 * Implements the @pcreate command
 *
 * Creates a player.  This is an incredibly thin wrapper around create_player;
 * in fact, this does no permission checks or any other kind of check.
 *
 * @see create_player
 *
 * @param player the player running the command
 * @param user the user to create
 * @param password the password to set on the new user.
 */
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
/**
 * Implementation of the @usage command
 *
 * This gives a bunch of information about the FuzzBall MUCK process.
 * Back in "the day" a lot of this information was not available on
 * all operating systems, so the NO_USAGE_COMMAND and HAVE_RUSAGE
 * defines were born to disable this on non-participating systems.
 *
 * There are no permissions checks done; this is usually wizard only though.
 *
 * @param player the player doing the call.
 */
void
do_usage(dbref player)
{
#ifdef HAVE_GETRUSAGE
    long psize;
    struct rusage usage;
#endif

    notifyf(player, "Process ID: %d", getpid());
    notifyf(player, "Max descriptors/process: %ld", max_open_files());

#ifdef HAVE_GETRUSAGE
    psize = sysconf(_SC_PAGESIZE);
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
    notifyf(player, "User time used: %ld sec and %ld usec.", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
    notifyf(player, "System time used: %ld sec and %ld usec.", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    notifyf(player, "Pagesize for this machine: %ld", psize);
    notifyf(player, "Maximum resident memory: %ldk",
            (long) (usage.ru_maxrss * (psize / 1024)));
    notifyf(player, "Integral resident memory: %ldk",
            (long) (usage.ru_idrss * (psize / 1024)));
#endif /* HAVE_GETRUSAGE */
}
#endif /* !NO_USAGE_COMMAND */

/**
 * @TODO: These "topprofs" calls are almost all copy/paste.  I've noted in
 *        other TODO's such as under do_muf_topprofs that there is a lot
 *        of duplicate code ... but these calls are almost 100% duplicated,
 *        so maybe we can refactor them to use a common core function.
 */

/**
 * Implementation of the @muftops command
 *
 * This shows statistics about programs that have been running.  These
 * statistics are shown for top 'arg1' number of programs.  The default
 * is '10'.  'reset' can also be passed to reset the statistic numbers.
 *
 * This iterates over the entire DB and puts the programs in a
 * 'struct profnode' linked list.  For large numbers of 'arg1', this is
 * really inefficient since its sorting a linked list.
 *
 * This does not do any permission checking.
 *
 * @param player the player doing the call
 * @param arg1 Either a string containing a number, the word "reset", or ""
 */
void
do_muf_topprofs(dbref player, char *arg1)
{
    struct profnode *tops = NULL, *curr = NULL;
    int nodecount = 0;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!strcasecmp(arg1, "reset")) {
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

            /* @TODO: For all these topprofs calls, the following lines
             *        of code are copy/pasted.  This is nasty and unnecessary.
             *
             *        The linked list implementation is kind of awful for
             *        large datasets.  For the dataset of 10 (default)
             *        it isn't bad.  In a perfect world, we'd use our
             *        AVL generic implementation, but for now, we should
             *        abstract this into a function that manages adding
             *        nodes to get rid of this duplicate logic.
             */
            struct profnode *newnode = malloc(sizeof(struct profnode));
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

    notify(player, "     %CPU   TotalTime  UseCount  Program");

    /* Output the produced result */
    while (tops) {
        char unparse_buf[BUFFER_LEN];
        curr = tops;
        unparse_object(player, curr->prog, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "%10.3f %10.3f %9ld %s", curr->pcnt, curr->proftime, curr->usecount,
                unparse_buf);
        tops = tops->next;
        free(curr);
    }

    notifyf(player, "Profile Length (sec): %5lld  %%idle: %5.2f%%  Total Cycles: %5lu",
            (long long) (current_systime - sel_prof_start_time),
            ((double) (sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) * 100.0) /
            (double) ((current_systime - sel_prof_start_time) + 0.01), sel_prof_idle_use);
    notify(player, "*Done*");
}

/**
 * Implementation of the @mpitops command
 *
 * This shows statistics about programs that have been running.  These
 * statistics are shown for top 'arg1' number of programs.  The default
 * is '10'.  'reset' can also be passed to reset the statistic numbers.
 *
 * This iterates over the entire DB and puts the programs in a
 * 'struct profnode' linked list.  For large numbers of 'arg1', this is
 * really inefficient since its sorting a linked list.
 *
 * This does not do any permission checking.
 *
 * @param player the player doing the call
 * @param arg1 Either a string containing a number, the word "reset", or ""
 */
void
do_mpi_topprofs(dbref player, char *arg1)
{
    struct profnode *tops = NULL, *curr = NULL;
    int nodecount = 0;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!strcasecmp(arg1, "reset")) {
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
            /* @TODO: See other note -- this code is duplicate */
            struct profnode *newnode = malloc(sizeof(struct profnode));
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
    }

    notify(player, "     %CPU   TotalTime  UseCount  Object");

    /* Output the results */
    while (tops) {
        char unparse_buf[BUFFER_LEN];
        curr = tops;
        unparse_object(player, curr->prog, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "%10.3f %10.3f %9ld %s", curr->pcnt, curr->proftime, curr->usecount,
                unparse_buf);
        tops = tops->next;
        free(curr);
    }

    notifyf(player, "Profile Length (sec): %5lld  %%idle: %5.2f%%  Total Cycles: %5lu",
            (long long) (current_systime - sel_prof_start_time),
            (((double) sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) * 100.0) /
            (double) ((current_systime - sel_prof_start_time) + 0.01), sel_prof_idle_use);

    notify(player, "*Done*");
}

/**
 * Implementation of the @tops command
 *
 * This shows statistics about programs that have been running.  These
 * statistics are shown for top 'arg1' number of programs.  The default
 * is '10'.  'reset' can also be passed to reset the statistic numbers.
 *
 * This iterates over the entire DB and puts the programs in a
 * 'struct profnode' linked list.  For large numbers of 'arg1', this is
 * really inefficient since its sorting a linked list.
 *
 * This does not do any permission checking.
 *
 * @param player the player doing the call
 * @param arg1 Either a string containing a number, the word "reset", or ""
 */
void
do_topprofs(dbref player, char *arg1)
{
    /* @TODO: This function is terrible.  It's like the previous two topprofs
     *        copy/pasted together into one function.
     */
    struct profnode *tops = NULL, *curr = NULL;
    int nodecount = 0;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!strcasecmp(arg1, "reset")) {
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
            /* @TODO: The same duplicate linked list code as noted above */
            struct profnode *newnode = malloc(sizeof(struct profnode));
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
            /* @TODO: Wow, this is duplicated even in the same function */
            struct profnode *newnode = malloc(sizeof(struct profnode));
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

    /* Output results */
    while (tops) {
        char unparse_buf[BUFFER_LEN];
        curr = tops;
        unparse_object(player, curr->prog, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "%10.3f %10.3f %9ld%5s   %s", curr->pcnt, curr->proftime,
                curr->usecount, curr->type ? "MUF" : "MPI", unparse_buf);
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
/**
 * Implementation of @memory command
 *
 * This displays memory information to the calling user.  Note that it
 * does not do any permission checkings.
 *
 * Much like @usage, this uses a set of #define's to determine what
 * is actually displayed due to different features that operating
 * systems support.
 *
 * @param who the person running the command
 */
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
#endif      /* HAVE_MALLINFO */

#ifdef MALLOC_PROFILING
    notify(who, "  ");
    CrT_summarize(who);
    CrT_summarize_to_file(tp_file_log_malloc, "Manual Checkpoint");
#endif

    notify(who, "Done.");
}
#endif      /* NO_MEMORY_COMMAND */

/**
 * Implementation of @debug command
 *
 * This only applies to DISKBASE (at the moment), and only works if
 * the argument "display propcache" is displayed.  Under the hood,
 * it just calls display_propcache.  Clearly the idea is that this could
 * do more in the future
 *
 * This does NO permission checking.
 *
 * @see display_propcache
 *
 * @param player the player doing the call
 * @param args the arguments provided.
 */
void
do_debug(dbref player, const char *args)
{
    /* for future expansion */

    if (MOD_ENABLED("DISKBASE") && !strcasecmp(args, "display propcache")) {
#ifdef DISKBASE
        display_propcache(player);
#endif
    } else {
        notify(player, "Unrecognized option.");
    }
}

/**
 * @var constant used for base64 encoding - this is the character set
 */
static const unsigned char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Implementation of a base64 encoder for the purpose of dumping DBs to a user
 *
 * This is taken from:
 *
 * https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/
 *
 * and is Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * It is used under the terms of the BSD license.
 *
 * @private
 * @param src the data to encode
 * @param len the length of the data to be encoded
 * @param out the buffer to use
 * @param out_size the size of the buffer
 * @return size of the generated b64 string (not including null)
 */
static size_t
base64_encode(const unsigned char *src, size_t len, 
              unsigned char *out, size_t out_size)
{
    unsigned char *pos;
    const unsigned char *end, *in;
    size_t olen;
    int line_len;

    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72; /* line feeds */
    olen++; /* nul termination */

    if ((olen < len) || (olen > out_size))
        return 0; /* integer overflow */

    end = src + len;
    in = src;
    pos = out;
    line_len = 0;

    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
        line_len += 4;

        if (line_len >= 72) {
            *pos++ = '\n';
            line_len = 0;
        }
    }

    if (end - in) {
        *pos++ = base64_table[in[0] >> 2];

        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) |
                                  (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }

        *pos++ = '=';
        line_len += 4;
    }

    if (line_len)
        *pos++ = '\n';

    *pos = '\0';

    return (pos - out);
}

/**
 * Base 64 encode the contents of a file and send it over a descriptor in
 * such a way that it won't get truncated by the MUCK.
 *
 * The output buffer will be flushed before the file is sent and then
 * after.
 *
 * Note that, basically for buffer simplicity sake, we base 64 encode each
 * line of the files we send individually.  As such, we need a dividing
 * character to split these lines up so python can base 64 decode each line
 * at a time.  * is the delimiter character.  Read the teledump-extract.py
 * file for a better understanding of how this works.
 *
 * @private
 * @param descr the descriptor data structure
 * @param in the file handled to read from
 */
static void
base64_send(struct descriptor_data* descr, FILE* in)
{
    int                     sent = 0, limit = 0, b64_len = 0;
    char                    buf[BUFFER_LEN+1];

    /*
     * This is more than base64 needs, but is a simple calculation and
     * better to have too much headroom than too little.
     */
    char                    base64_buf[BUFFER_LEN*2];

    /*
     * Calculate the maximum we can have in queue, so we know when to flush.
     * Make sure we have some headroom here, too, so no accidents.
     */
    limit = tp_max_output - BUFFER_LEN;

    /* Make sure there's nothing in our buffer */
    process_output(descr);

    while (fgets(buf, BUFFER_LEN, in)) {
        b64_len = base64_encode(buf, strlen(buf), base64_buf,
                                sizeof(base64_buf));

        /* Add a * as a separator so we know each line */
        base64_buf[b64_len] = '*';
        b64_len++;
        base64_buf[b64_len] = '\0';


        if (sent + b64_len > limit) {
            process_output(descr);
            sent = 0;
        }        

        queue_write(descr, base64_buf, b64_len);
        sent += b64_len;
    }

    process_output(descr);
}

/**
 * Implementation of @teledump command
 *
 * @teledump does a base64 encoded dump of the entire database and the
 * associated macros and MUF in a fashion that can be consumed by an unpacker
 * script (a python unpacker will be provided as part of this).
 *
 * It is a way to preserve the vital data of a MUCK but it does not
 * send over irrelevant files.
 *
 * For simplicity sake, it dumps the DB on disk, so @dump before running
 * this command to get the latest.
 *
 * @param descr the player's descriptor
 * @param player the player doing the teledump
 */
void
do_teledump(int descr, dbref player)
{
    struct descriptor_data* d;
    FILE*                   in;
    char                    buf[BUFFER_LEN];

    /*
     * This is needed so we can run process_output and make sure we don't
     * truncate output as we work.
     */
    d = descrdata_by_descr(descr);

    /* Clear out anything in the player's descriptor */
    process_output(d);

    in = fopen(dumpfile, "r");

    /* This file won't exist if there hasn't been a dump yet. */
    if (!in) {
        notify(player, "The database hasn't been dumped yet.  Please use "
                       "@dump first.");
        return;
    }

    /* Give a notification banner -- send DB first */
    queue_write(d, "*** DB DUMP ***\r\n", 17);

    base64_send(d, in);
    fclose(in);

    /* DB dump end marker */
    queue_write(d, "*** DB DUMP END ***\r\n", 21);

    /* Macros start marker */
    queue_write(d, "*** MACRO START ***\r\n", 21);

    in = fopen(MACRO_FILE, "r");

    /* I'm not sure if MACRO_FILE always exists */
    if (in) {
        base64_send(d, in);
        fclose(in);
    }

    /* Macros end marker */
    queue_write(d, "*** MACRO END ***\r\n", 19);

    /*
     * Send over the MUFs
     *
     * I died a little inside writing this loop, but I think this is the
     * only way to find all the programs on the MUCK.
     */
    for (dbref i = 0; i < db_top; i++) {
        if (Typeof(i) != TYPE_PROGRAM) {
            continue;
        }

        /* Start message */
        snprintf(buf, sizeof(buf), "*** MUF %d ***\r\n", i);
        queue_write(d, buf, strlen(buf));

        /* Find the file */
        snprintf(buf, sizeof(buf), "muf/%d.m", i);
        in = fopen(buf, "r");

        if (in) {
            base64_send(d, in);
            fclose(in);
        }

        /* End message */
        queue_write(d, "*** MUF END ***\r\n", 17);
    }

    queue_write(d, "*** FINISHED ***\r\n", 18);
}