#include "config.h"

#include "commands.h"
#include "db.h"
#include "fbstrings.h"
#include "interface.h"
#include "log.h"
#include "match.h"
#include "player.h"
#include "tune.h"

void
do_say(dbref player, const char *message)
{
    char buf[BUFFER_LEN];

    notifyf(player, "You say, \"%s\"", message);
    snprintf(buf, sizeof(buf), "%s says, \"%s\"", NAME(player), message);
    notify_except(CONTENTS(LOCATION(player)), player, buf, player);
}

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
    if (!notify_from(player, who, buf)) {
	notifyf(player, "%s is not connected.", NAME(who));
	return;
    }
    notifyf(player, "You whisper, \"%s\" to %s.", arg2, NAME(who));
}

void
do_pose(dbref player, const char *message)
{
    char buf[BUFFER_LEN];

    snprintf(buf, sizeof(buf), "%s%s%s", NAME(player),
	    is_valid_pose_separator(*message) ? "" : " ", message);
    notify_except(CONTENTS(LOCATION(player)), NOTHING, buf, player);
}

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
	    notify_from(player, d->player, buf);
	}
    }
}

void
do_gripe(dbref player, const char *message)
{
    dbref loc;
    char buf[BUFFER_LEN];

    if (!message || !*message) {
	if (Wizard(player)) {
	    spit_file(player, tp_file_log_gripes);
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

/* doesn't really belong here, but I couldn't figure out where else */
void
do_page(dbref player, const char *arg1, const char *arg2)
{
    char buf[BUFFER_LEN];
    dbref target;

    if (!payfor(player, tp_lookup_cost)) {
	notifyf(player, "You don't have enough %s.", tp_pennies);
	return;
    }
    if ((target = lookup_player(arg1)) == NOTHING) {
	notify(player, "I don't recognize that name.");
	return;
    }
    if (FLAGS(target) & HAVEN) {
	notify(player, "That player does not wish to be disturbed.");
	return;
    }
    if (blank(arg2))
	snprintf(buf, sizeof(buf), "You sense that %s is paging you from %s.",
		 NAME(player), NAME(LOCATION(player)));
    else
	snprintf(buf, sizeof(buf), "%s pages from %s: \"%s\"", NAME(player),
		 NAME(LOCATION(player)), arg2);
    if (notify_from(player, target, buf))
	notify(player, "Your message has been sent.");
    else {
	notifyf(player, "%s is not connected.", NAME(target));
    }
}
