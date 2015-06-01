#include "config.h"

#include "db.h"
#include "mpi.h"
#include "interface.h"
#include "match.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "externs.h"
#include "speech.h"
#include <ctype.h>

/* Commands which involve speaking */

void
do_say(dbref player, const char *message)
{
	dbref loc;
	char buf[BUFFER_LEN];

	if ((loc = getloc(player)) == NOTHING)
		return;

	/* notify everybody */
	snprintf(buf, sizeof(buf), "You say, \"%s\"", message);
	notify(player, buf);
	snprintf(buf, sizeof(buf), "%s says, \"%s\"", NAME(player), message);
	notify_except(DBFETCH(loc)->contents, player, buf, player);
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
	switch (who = match_result(&md)) {
	case NOTHING:
		notify(player, "Whisper to whom?");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		snprintf(buf, sizeof(buf), "%s whispers, \"%s\"", NAME(player), arg2);
		if (!notify_from(player, who, buf)) {
			snprintf(buf, sizeof(buf), "%s is not connected.", NAME(who));
			notify(player, buf);
			break;
		}
		snprintf(buf, sizeof(buf), "You whisper, \"%s\" to %s.", arg2, NAME(who));
		notify(player, buf);
		break;
	}
}

void
do_pose(dbref player, const char *message)
{
	dbref loc;
	char buf[BUFFER_LEN];

	if ((loc = getloc(player)) == NOTHING)
		return;

	/* notify everybody */
	snprintf(buf, sizeof(buf), "%s %s", NAME(player), message);
	notify_except(DBFETCH(loc)->contents, NOTHING, buf, player);
}

void
do_wall(dbref player, const char *message)
{
	dbref i;
	char buf[BUFFER_LEN];

	if (Wizard(player) && Typeof(player) == TYPE_PLAYER) {
		log_status("WALL from %s(%d): %s", NAME(player), player, message);
		snprintf(buf, sizeof(buf), "%s shouts, \"%s\"", NAME(player), message);
		for (i = 0; i < db_top; i++) {
			if (Typeof(i) == TYPE_PLAYER) {
				notify_from(player, i, buf);
			}
		}
	} else {
		notify(player, "But what do you want to do with the wall?");
	}
}

void
do_gripe(dbref player, const char *message)
{
	dbref loc;
	char buf[BUFFER_LEN];

	if (!message || !*message) {
		if (Wizard(player)) {
			spit_file(player, LOG_GRIPE);
		} else {
			notify(player, "If you wish to gripe, use 'gripe <message>'.");
		}
		return;
	}

	loc = DBFETCH(player)->location;
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
		notify_fmt(player, "You don't have enough %s.", tp_pennies);
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
		snprintf(buf, sizeof(buf), "You sense that %s is looking for you in %s.",
				NAME(player), NAME(DBFETCH(player)->location));
	else
		snprintf(buf, sizeof(buf), "%s pages from %s: \"%s\"", NAME(player),
				NAME(DBFETCH(player)->location), arg2);
	if (notify_from(player, target, buf))
		notify(player, "Your message has been sent.");
	else {
		snprintf(buf, sizeof(buf), "%s is not connected.", NAME(target));
		notify(player, buf);
	}
}

void
notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg, int isprivate)
{
	char buf[BUFFER_LEN];
	dbref ref;

	if (obj == NOTHING)
		return;

	if (tp_listeners && (tp_listeners_obj || Typeof(obj) == TYPE_ROOM)) {
		listenqueue(-1, who, room, obj, obj, xprog, "_listen", msg, tp_listen_mlev, 1, 0);
		listenqueue(-1, who, room, obj, obj, xprog, "~listen", msg, tp_listen_mlev, 1, 1);
		listenqueue(-1, who, room, obj, obj, xprog, "~olisten", msg, tp_listen_mlev, 0, 1);
	}

	if (tp_zombies && Typeof(obj) == TYPE_THING && !isprivate) {
		if (FLAGS(obj) & VEHICLE) {
			if (getloc(who) == getloc(obj)) {
				char pbuf[BUFFER_LEN];
				const char *prefix;

				memset(buf,0,BUFFER_LEN); /* Make sure the buffer is zeroed */

				prefix = do_parse_prop(-1, who, obj, MESGPROP_OECHO, "(@Oecho)", pbuf, sizeof(pbuf), MPI_ISPRIVATE);
				if (!prefix || !*prefix)
					prefix = "Outside>";
				snprintf(buf, sizeof(buf), "%s %.*s", prefix, (int)(BUFFER_LEN - 2 - strlen(prefix)), msg);
				ref = DBFETCH(obj)->contents;
				while (ref != NOTHING) {
					notify_filtered(who, ref, buf, isprivate);
					ref = DBFETCH(ref)->next;
				}
			}
		}
	}

	if (Typeof(obj) == TYPE_PLAYER || Typeof(obj) == TYPE_THING)
		notify_filtered(who, obj, msg, isprivate);
}

void
notify_except(dbref first, dbref exception, const char *msg, dbref who)
{
	dbref room, srch;

	if (first != NOTHING) {

		srch = room = DBFETCH(first)->location;

		if (tp_listeners) {
			notify_from_echo(who, srch, msg, 0);

			if (tp_listeners_env) {
				srch = DBFETCH(srch)->location;
				while (srch != NOTHING) {
					notify_from_echo(who, srch, msg, 0);
					srch = getparent(srch);
				}
			}
		}

		DOLIST(first, first) {
			if ((Typeof(first) != TYPE_ROOM) && (first != exception)) {
				/* don't want excepted player or child rooms to hear */
				notify_from_echo(who, first, msg, 0);
			}
		}
	}
}


void
parse_oprop(int descr, dbref player, dbref dest, dbref exit, const char *propname,
			   const char *prefix, const char *whatcalled)
{
	const char* msg = get_property_class(exit, propname);
	int ival = 0;
	if (Prop_Blessed(exit, propname))
		ival |= MPI_ISBLESSED;

	if (msg)
		parse_omessage(descr, player, dest, exit, msg, prefix, whatcalled, ival);
}

void
parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg,
			   const char *prefix, const char *whatcalled, int mpiflags)
{
	char buf[BUFFER_LEN];
	char *ptr;

	do_parse_mesg(descr, player, exit, msg, whatcalled, buf, sizeof(buf), MPI_ISPUBLIC | mpiflags);
	ptr = pronoun_substitute(descr, player, buf);
	if (!*ptr)
		return;

	/*
		TODO: Find out if this should be prefixing with NAME(player), or if
		it should use the prefix argument...  The original code just ignored
		the prefix argument...
	*/

	prefix_message(buf, ptr, prefix, BUFFER_LEN, 1);

	notify_except(DBFETCH(dest)->contents, player, buf, player);
}


int
blank(const char *s)
{
	while (*s && isspace(*s))
		s++;

	return !(*s);
}
