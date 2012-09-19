/* $Header: /cvsroot/fbmuck/fbmuck/src/match.c,v 1.12 2009/10/11 05:19:18 points Exp $ */


#include "copyright.h"
#include "config.h"

/* Routines for parsing arguments */
#include <ctype.h>

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "match.h"
#include "interface.h"
#include "externs.h"


#define DOWNCASE(x) (tolower(x))

char match_cmdname[BUFFER_LEN];	/* triggering command */
char match_args[BUFFER_LEN];	/* remaining text */

void
init_match(int descr, dbref player, const char *name, int type, struct match_data *md)
{
	md->exact_match = md->last_match = NOTHING;
	md->match_count = 0;
	md->match_who = player;
	md->match_from = player;
	md->match_descr = descr;
	md->match_name = name;
	md->check_keys = 0;
	md->preferred_type = type;
	md->longest_match = 0;
	md->match_level = 0;
	md->block_equals = 0;
	md->partial_exits = (TYPE_EXIT == type);
}

void
init_match_check_keys(int descr, dbref player, const char *name, int type,
					  struct match_data *md)
{
	init_match(descr, player, name, type, md);
	md->check_keys = 1;
}

void
init_match_remote(int descr, dbref player, dbref what, const char *name, int type,
				  struct match_data *md)
{
	init_match(descr, player, name, type, md);
	md->match_from = what;
}

static dbref
choose_thing(int descr, dbref thing1, dbref thing2, struct match_data *md)
{
	int has1;
	int has2;
	int preferred = md->preferred_type;

	if (thing1 == NOTHING) {
		return thing2;
	} else if (thing2 == NOTHING) {
		return thing1;
	}
	if (preferred != NOTYPE) {
		if (Typeof(thing1) == preferred) {
			if (Typeof(thing2) != preferred) {
				return thing1;
			}
		} else if (Typeof(thing2) == preferred) {
			return thing2;
		}
	}
	if (md->check_keys) {
		has1 = could_doit(descr, md->match_who, thing1);
		has2 = could_doit(descr, md->match_who, thing2);

		if (has1 && !has2) {
			return thing1;
		} else if (has2 && !has1) {
			return thing2;
		}
		/* else fall through */
	}
	return (RANDOM() % 2 ? thing1 : thing2);
}

void
match_player(struct match_data *md)
{
	dbref match;
	const char *p;

	if (*(md->match_name) == LOOKUP_TOKEN && payfor(OWNER(md->match_from), tp_lookup_cost)) {
		for (p = (md->match_name) + 1; isspace(*p); p++) ;
		if ((match = lookup_player(p)) != NOTHING) {
			md->exact_match = match;
		}
	}
}

/* returns dbref if registered object found for name, else NOTHING */
dbref
find_registered_obj(dbref player, const char *name)
{
	dbref match;
	const char *p;
	char buf[BUFFER_LEN];
	PropPtr ptr;

	if (*name != REGISTERED_TOKEN)
		return (NOTHING);
	for (p = name + 1; *p && isspace(*p); p++) ;
	if (!*p)
		return (NOTHING);
	snprintf(buf, sizeof(buf), "_reg/%s", p);
	ptr = envprop(&player, buf, 0);
	if (!ptr)
		return NOTHING;
#ifdef DISKBASE
	propfetch(player, ptr);		/* DISKBASE PROPVALS */
#endif
	switch (PropType(ptr)) {
	case PROP_STRTYP:
		p = PropDataStr(ptr);
		if (*p == NUMBER_TOKEN)
			p++;
		if (number(p)) {
			match = (dbref) atoi(p);
			if ((match >= 0) && (match < db_top) && (Typeof(match) != TYPE_GARBAGE))
				return (match);
		}
		break;
	case PROP_REFTYP:
		match = PropDataRef(ptr);
		if ((match >= 0) && (match < db_top) && (Typeof(match) != TYPE_GARBAGE))
			return (match);
		break;
	case PROP_INTTYP:
		match = (dbref) PropDataVal(ptr);
		if ((match > 0) && (match < db_top) && (Typeof(match) != TYPE_GARBAGE))
			return (match);
		break;
	case PROP_LOKTYP:
		return (NOTHING);
		break;
	}
	return (NOTHING);
}


void
match_registered(struct match_data *md)
{
	dbref match;

	match = find_registered_obj(md->match_from, md->match_name);
	if (match != NOTHING)
		md->exact_match = match;
}


/* returns nnn if name = #nnn, else NOTHING */
static dbref
absolute_name(struct match_data *md)
{
	dbref match;

	if (*(md->match_name) == NUMBER_TOKEN) {
		match = parse_dbref((md->match_name) + 1);
		if (match < 0 || match >= db_top) {
			return NOTHING;
		} else {
			return match;
		}
	} else {
		return NOTHING;
	}
}

void
match_absolute(struct match_data *md)
{
	dbref match;

	if ((match = absolute_name(md)) != NOTHING) {
		md->exact_match = match;
	}
}

void
match_me(struct match_data *md)
{
	if (!string_compare(md->match_name, "me")) {
		md->exact_match = md->match_who;
	}
}

void
match_here(struct match_data *md)
{
	if (!string_compare(md->match_name, "here")
		&& DBFETCH(md->match_who)->location != NOTHING) {
		md->exact_match = DBFETCH(md->match_who)->location;
	}
}

void
match_home(struct match_data *md)
{
	if (!string_compare(md->match_name, "home"))
		md->exact_match = HOME;
}


static void
match_list(dbref first, struct match_data *md)
{
	dbref absolute;

	absolute = absolute_name(md);
	if (!controls(OWNER(md->match_from), absolute))
		absolute = NOTHING;

	DOLIST(first, first) {
		if (first == absolute) {
			md->exact_match = first;
			return;
		} else if (!string_compare(NAME(first), md->match_name)) {
			/* if there are multiple exact matches, randomly choose one */
			md->exact_match = choose_thing(md->match_descr, md->exact_match, first, md);
		} else if (string_match(NAME(first), md->match_name)) {
			md->last_match = first;
			(md->match_count)++;
		}
	}
}

void
match_possession(struct match_data *md)
{
	match_list(DBFETCH(md->match_from)->contents, md);
}

void
match_neighbor(struct match_data *md)
{
	dbref loc;

	if ((loc = DBFETCH(md->match_from)->location) != NOTHING) {
		match_list(DBFETCH(loc)->contents, md);
	}
}

/*
 * match_exits matches a list of exits, starting with 'first'.
 * It will match exits of players, rooms, or things.
 */
void
match_exits(dbref first, struct match_data *md)
{
	dbref exit, absolute;
	const char *exitname, *p;
	int i, exitprog, lev, partial;

	if (first == NOTHING)
		return;					/* Easy fail match */
	if ((DBFETCH(md->match_from)->location) == NOTHING)
		return;

	absolute = absolute_name(md);	/* parse #nnn entries */
	if (!controls(OWNER(md->match_from), absolute))
		absolute = NOTHING;

	DOLIST(exit, first) {
		if (exit == absolute) {
			md->exact_match = exit;
			continue;
		}
		exitprog = 0;
		if (FLAGS(exit) & HAVEN) {
			exitprog = 1;
		} else if (DBFETCH(exit)->sp.exit.dest) {
			for (i = 0; i < DBFETCH(exit)->sp.exit.ndest; i++)
				if (Typeof((DBFETCH(exit)->sp.exit.dest)[i]) == TYPE_PROGRAM)
					exitprog = 1;
		}
		if (tp_enable_prefix && exitprog && md->partial_exits &&
			(FLAGS(exit) & XFORCIBLE) && FLAGS(OWNER(exit)) & WIZARD) {
			partial = 1;
		} else {
			partial = 0;
		}
		exitname = NAME(exit);
		while (*exitname) {		/* for all exit aliases */
			int notnull = 0;
			for (p = md->match_name;	/* check out 1 alias */
				 *p &&
				 DOWNCASE(*p) == DOWNCASE(*exitname) &&
				 *exitname != EXIT_DELIMITER;
				 p++, exitname++)
			{
				if (!isspace(*p)) {
					notnull = 1;
				}
			}
			/* did we get a match on this alias? */
			if ((partial && notnull) || ((*p == '\0') || (*p == ' ' && exitprog))) {
				/* make sure there's nothing afterwards */
				while (isspace(*exitname))
					exitname++;
				lev = PLevel(exit);
				if (tp_compatible_priorities && (lev == 1) &&
					(DBFETCH(exit)->location == NOTHING ||
					 Typeof(DBFETCH(exit)->location) != TYPE_THING ||
					 controls(OWNER(exit), getloc(md->match_from))))
					lev = 2;
				if (*exitname == '\0' || *exitname == EXIT_DELIMITER) {
					/* we got a match on this alias */
					if (lev >= md->match_level) {
						if (strlen(md->match_name) - strlen(p) > md->longest_match) {
							if (lev > md->match_level) {
								md->match_level = lev;
								md->block_equals = 0;
							}
							md->exact_match = exit;
							md->longest_match = strlen(md->match_name) - strlen(p);
							if ((*p == ' ') || (partial && notnull)) {
								strcpyn(match_args, sizeof(match_args), (partial && notnull)? p : (p + 1));
								{
									char *pp;
									int ip;

									for (ip = 0, pp = (char *) md->match_name;
										 *pp && (pp != p); pp++)
										match_cmdname[ip++] = *pp;
									match_cmdname[ip] = '\0';
								}
							} else {
								*match_args = '\0';
								strcpyn(match_cmdname, sizeof(match_cmdname), (char *) md->match_name);
							}
						} else if ((strlen(md->match_name) - strlen(p) ==
									md->longest_match) && !((lev == md->match_level) &&
															(md->block_equals))) {
							if (lev > md->match_level) {
								md->exact_match = exit;
								md->match_level = lev;
								md->block_equals = 0;
							} else {
								md->exact_match =
										choose_thing(md->match_descr, md->exact_match, exit,
													 md);
							}
							if (md->exact_match == exit) {
								if ((*p == ' ') || (partial && notnull)) {
									strcpyn(match_args, sizeof(match_args), (partial && notnull) ? p : (p + 1));
									{
										char *pp;
										int ip;

										for (ip = 0, pp = (char *) md->match_name;
											 *pp && (pp != p); pp++)
											match_cmdname[ip++] = *pp;
										match_cmdname[ip] = '\0';
									}
								} else {
									*match_args = '\0';
									strcpyn(match_cmdname, sizeof(match_cmdname), (char *) md->match_name);
								}
							}
						}
					}
					goto next_exit;
				}
			}
			/* we didn't get it, go on to next alias */
			while (*exitname && *exitname++ != EXIT_DELIMITER) ;
			while (isspace(*exitname))
				exitname++;
		}						/* end of while alias string matches */
	  next_exit:
		;
	}
}

/*
 * match_invobj_actions
 * matches actions attached to objects in inventory
 */
void
match_invobj_actions(struct match_data *md)
{
	dbref thing;

	if (DBFETCH(md->match_from)->contents == NOTHING)
		return;
	DOLIST(thing, DBFETCH(md->match_from)->contents) {
		if (Typeof(thing) == TYPE_THING && DBFETCH(thing)->exits != NOTHING) {
			match_exits(DBFETCH(thing)->exits, md);
		}
	}
}

/*
 * match_roomobj_actions
 * matches actions attached to objects in the room
 */
void
match_roomobj_actions(struct match_data *md)
{
	dbref thing, loc;

	if ((loc = DBFETCH(md->match_from)->location) == NOTHING)
		return;
	if (DBFETCH(loc)->contents == NOTHING)
		return;
	DOLIST(thing, DBFETCH(loc)->contents) {
		if (Typeof(thing) == TYPE_THING && DBFETCH(thing)->exits != NOTHING) {
			match_exits(DBFETCH(thing)->exits, md);
		}
	}
}

/*
 * match_player_actions
 * matches actions attached to player
 */
void
match_player_actions(struct match_data *md)
{
	dbref obj;

	switch (Typeof(md->match_from)) {
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_THING:
		obj = DBFETCH(md->match_from)->exits;
		break;
	default:
		obj = NOTHING;
		break;
	}
	if (obj == NOTHING)
		return;
	match_exits(obj, md);
}

/*
 * match_room_exits
 * Matches exits and actions attached to player's current room.
 * Formerly 'match_exit'.
 */
void
match_room_exits(dbref loc, struct match_data *md)
{
	dbref obj;

	switch (Typeof(loc)) {
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_THING:
		obj = DBFETCH(loc)->exits;
		break;
	default:
		obj = NOTHING;
		break;
	}
	if (obj == NOTHING)
		return;
	match_exits(obj, md);
}

/*
 * match_all_exits
 * Matches actions on player, objects in room, objects in inventory,
 * and room actions/exits (in reverse order of priority order).
 */
void
match_all_exits(struct match_data *md)
{
	dbref loc;
	int limit = 88;
        int blocking = 0;

	strcpyn(match_args, sizeof(match_args), "\0");
	strcpyn(match_cmdname, sizeof(match_cmdname), "\0");
	if ((loc = DBFETCH(md->match_from)->location) != NOTHING)
		match_room_exits(loc, md);

	if (md->exact_match != NOTHING)
		md->block_equals = 1;
	match_invobj_actions(md);

	if (md->exact_match != NOTHING)
		md->block_equals = 1;
	match_roomobj_actions(md);

	if (md->exact_match != NOTHING)
		md->block_equals = 1;
	match_player_actions(md);

	if (loc == NOTHING)
		return;

	/* if player is in a vehicle, use environment of vehicle's home */
	if (Typeof(loc) == TYPE_THING) {
		loc = THING_HOME(loc);
		if (loc == NOTHING)
			return;
		if (md->exact_match != NOTHING)
			md->block_equals = 1;
		match_room_exits(loc, md);
	}

        /* Walk the environment chain to #0, or until depth chain limit
           has been hit, looking for a match. */
        while ((loc = DBFETCH(loc)->location) != NOTHING) {
                /* If we're blocking (because of a yield), only match a room if
                   and only if it has overt set on it. */
                if ((blocking && FLAGS(loc) & OVERT) || !blocking) {
                  if (md->exact_match != NOTHING)
                    md->block_equals = 1;
                  match_room_exits(loc, md);
                }
                if (!limit--)
                        break;
                /* Does this room have env-chain exit blocking enabled? */
                if (!blocking && tp_enable_match_yield && FLAGS(loc) & YIELD) {
                  blocking = 1;
                }
        }
}

void
match_everything(struct match_data *md)
{
	match_all_exits(md);
	match_neighbor(md);
	match_possession(md);
	match_me(md);
	match_here(md);
	match_registered(md);
	if (Wizard(OWNER(md->match_from)) || Wizard(md->match_who)) {
		match_absolute(md);
		match_player(md);
	}
}

dbref
match_result(struct match_data *md)
{
	if (md->exact_match != NOTHING) {
		return (md->exact_match);
	} else {
		switch (md->match_count) {
		case 0:
			return NOTHING;
		case 1:
			return (md->last_match);
		default:
			return AMBIGUOUS;
		}
	}
}

/* use this if you don't care about ambiguity */
dbref
last_match_result(struct match_data * md)
{
	if (md->exact_match != NOTHING) {
		return (md->exact_match);
	} else {
		return (md->last_match);
	}
}

dbref
noisy_match_result(struct match_data * md)
{
	dbref match;

	switch (match = match_result(md)) {
	case NOTHING:
		notify(md->match_who, NOMATCH_MESSAGE);
		return NOTHING;
	case AMBIGUOUS:
		notify(md->match_who, AMBIGUOUS_MESSAGE);
		return NOTHING;
	default:
		return match;
	}
}

void
match_rmatch(dbref arg1, struct match_data *md)
{
	if (arg1 == NOTHING)
		return;
	switch (Typeof(arg1)) {
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_THING:
		match_list(DBFETCH(arg1)->contents, md);
		match_exits(DBFETCH(arg1)->exits, md);
		break;
	}
}
static const char *match_c_version = "$RCSfile: match.c,v $ $Revision: 1.12 $";
const char *get_match_c_version(void) { return match_c_version; }
