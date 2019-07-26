/** @file speech.c
 *
 * Implementation of various speech and player communication features.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>

#include "config.h"

#include "commands.h"
#include "db.h"
#include "fbstrings.h"
#include "interface.h"
#include "log.h"
#include "match.h"
#include "player.h"
#include "tune.h"

/**
 * Implementation of the say command.
 *
 * There is absolutely nothing fancy here.  The player and the room get
 * slightly different variants of the same message.
 *
 * @param player the player talking
 * @param message the message being said
 */
void
do_say(dbref player, const char *message)
{
    char buf[BUFFER_LEN];

    notifyf(player, "You say, \"%s\"", message);
    snprintf(buf, sizeof(buf), "%s says, \"%s\"", NAME(player), message);
    notify_except(CONTENTS(LOCATION(player)), player, buf, player);
}

/**
 * Implementation of whisper
 *
 * The built-in whisper is incredibly basic.  Wizard players are able
 * to remote whisper with by prefixing player name with a *
 *
 * You can only whisper a single player at a time with this command,
 * unlike most MUF based whispers.
 *
 * @param descr the descriptor of the player whispering
 * @param player the player whispering
 * @param arg1 the target to whisper to
 * @param arg2 the whisper message to send
 */
void
do_whisper(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref who;
    char buf[BUFFER_LEN];
    struct match_data md;

    init_match(descr, player, arg1, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);

    if (Wizard(player) && Typeof(player) == TYPE_PLAYER) {
        match_absolute(&md);
        match_player(&md);
    }

    if ((who = noisy_match_result(&md)) == NOTHING) {
        return;
    }

    snprintf(buf, sizeof(buf), "%s whispers, \"%s\"", NAME(player), arg2);

    if (!notify_from_echo(player, who, buf, 1)) {
        notifyf(player, "%s is not connected.", NAME(who));
        return;
    }

    notifyf(player, "You whisper, \"%s\" to %s.", arg2, NAME(who));
}

/**
 * Implementation of pose command
 *
 * The built-in pose command is somewhat basic, but it does smartly
 * determine if there should be a space after the player's name or
 * not based on is_valid_pose_separator
 *
 * @see is_valid_pose_separator
 *
 * @param player the player doing the pose
 * @param message the message being posed.
 */
void
do_pose(dbref player, const char *message)
{
    char buf[BUFFER_LEN];

    snprintf(buf, sizeof(buf), "%s%s%s", NAME(player),
             is_valid_pose_separator(*message) ? "" : " ", message);
    notify_except(CONTENTS(LOCATION(player)), NOTHING, buf, player);
}

/**
 * Implementation of @wall command
 *
 * This broadcasts a message to everyone on the MUCK, as a "shout"
 * using a format similar to "say".  This does NOT do permission
 * checking.
 *
 * @param player the player shouting
 * @param message the message being sent
 */
void
do_wall(dbref player, const char *message)
{
    struct descriptor_data *dnext;
    char buf[BUFFER_LEN];

    log_status("WALL from %s(%d): %s", NAME(player), player, message);
    snprintf(buf, sizeof(buf), "%s shouts, \"%s\"", NAME(player), message);

    for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
        dnext = d->next;

        if (d->connected) {
            notify_from_echo(player, d->player, buf, 1);
        }
    }
}

/**
 * Implementation of the gripe command
 *
 * If the message is empty or NULL, if the player is a wizard, it will
 * display the gripe file.  Otherwise it will show an error message.
 *
 * If a message is provided, it will be logged to the gripe file.
 *
 * @param player the player running the gripe
 * @param message the message to gripe, or ""
 */
void
do_gripe(dbref player, const char *message)
{
    dbref loc;
    char buf[BUFFER_LEN];

    if (!message || !*message) {
        if (Wizard(player)) {
            spit_file_segment(player, tp_file_log_gripes, "");
        } else {
            notify(player, "If you wish to gripe, use 'gripe <message>'.");
        }

        return;
    }

    loc = LOCATION(player);
    log_gripe("GRIPE from %s(%d) in %s(%d): %s",
              NAME(player), player, NAME(loc), loc, message);

    notify(player, "Your complaint has been duly noted.");

    snprintf(buf, sizeof(buf), "## GRIPE from %s: %s", NAME(player), message);
    wall_wizards(buf);
}

/**
 * Implementation of page command
 *
 * This is the very basic page that comes with the MUCK.  First off,
 * it costs the 'tp_lookup_cost' to page.
 *
 * Supports HAVEN check on target.  Does not support multiple page targets.
 * Also supports a blank arg2 (blank message) which results in page sending
 * player's location instead of a message.
 *
 * @param player the player doing the page
 * @param arg1 the target player to page
 * @param arg2 the message to send to that player
 */
void
do_page(dbref player, const char *arg1, const char *arg2)
{
    char buf[BUFFER_LEN];
    dbref target;

    if ((target = lookup_player(arg1)) == NOTHING) {
        notify(player, "I don't recognize that name.");
        return;
    }

    if (FLAGS(target) & HAVEN) {
        notify(player, "That player does not wish to be disturbed.");
        return;
    }

    if (!payfor(player, tp_lookup_cost)) {
        notifyf(player, "You don't have enough %s.", tp_pennies);
        return;
    }

    if (blank(arg2))
        snprintf(buf, sizeof(buf), "You sense that %s is paging you from %s.",
                 NAME(player), NAME(LOCATION(player)));
    else
        snprintf(buf, sizeof(buf), "%s pages from %s: \"%s\"", NAME(player),
                 NAME(LOCATION(player)), arg2);

    if (notify_from_echo(player, target, buf, 1))
        notify(player, "Your message has been sent.");
    else {
        notifyf(player, "%s is not connected.", NAME(target));
    }
}
