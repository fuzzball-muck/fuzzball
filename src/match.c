/** @file match.c
 *
 * Implementation for the object matching system used by Fuzzball.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "game.h"
#include "interface.h"
#include "match.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

char match_cmdname[BUFFER_LEN]; /* triggering command */
char match_args[BUFFER_LEN];    /* remaining text */

/**
 * Initialize a match structure with initial values for doing an object search
 *
 * @param descr the descriptor of the person doing the search
 * @param player the player doing the search
 * @param name the name you're looking for
 * @param type the object type you are looking for (TYPE_*)
 * @param md the match_data structure to initialize
 */
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

/**
 * Initializes a match object, with the check_keys flag set
 *
 * This means in case of a match where more than one item counts as an
 * exact match, we will use if the objects are locked against the player
 * as part of the selection process and only pick something that is not
 * locked against the player.
 *
 * This seems pretty edge-case-y, but it is useful in a very narrow
 * context -- it is used primarily by movement and exit selection.
 *
 * @param descr the descriptor of the person doing the search
 * @param player the player doing the search
 * @param name the name you're looking for
 * @param type the object type you are looking for (TYPE_*)
 * @param md the match_data structure to initialize
 */
void
init_match_check_keys(int descr, dbref player, const char *name, int type,
                      struct match_data *md)
{
    init_match(descr, player, name, type, md);
    md->check_keys = 1;
}

/**
 * Initializes a match object, setting match_from to 'what'
 *
 * This is used in exactly one place for a questionable purpose (in MPI parsing)
 * 'match_from' is the location where the match is 'run from' which effects
 * the precidence of different objects.  This call defaults match_from to
 * whatever is in 'what' instead of from 'player' which is the default.
 *
 * @param descr the descriptor of the person doing the search
 * @param player the player doing the search
 * @param what the location to search relative to
 * @param name the name you're looking for
 * @param type the object type you are looking for (TYPE_*)
 * @param md the match_data structure to initialize
 */
void
init_match_remote(int descr, dbref player, dbref what, const char *name,
                  int type, struct match_data *md)
{
    init_match(descr, player, name, type, md);
    md->match_from = what;
}

/**
 * Given two refs, try to choose the more appropriate one based on parameters.
 *
 * The parameters are defined in the struct match_data.  The following
 * aspects influence the selection in order:
 *
 * - If either object is NOTHING, the other object wins.
 * - If there is a preferred type (md->preferred_type), if one matches the
 *   preferred type and the other does not, the one that matches wins.
 * - If one of the objects is locked against the player, and md->check_keys
 *   is set, then the unlocked object wins.
 * - If one object has a shorter 'env_distance' than the other, then that
 *   one wins -- @see env_distance
 * - Otherwise, it is coin toss to see which wins.
 *
 * Whichever one "wins" is the one that is returned.
 *
 * @private
 * @param descr the descriptor of the person matching.
 * @param thing1 the first object to consider
 * @param thing2 the second object to consider
 * @param md the match_data match configuration
 * @return the dbref thing1 or thing2 based on above criteria
 */
static dbref
choose_thing(int descr, dbref thing1, dbref thing2, struct match_data *md)
{
    int has1;
    int has2;
    int preferred = md->preferred_type;

    /* If one or the other is unset, that makes it easy. */
    if (thing1 == NOTHING) {
        return thing2;
    } else if (thing2 == NOTHING) {
        return thing1;
    }

    /* Check for type preference */
    if (preferred != NOTYPE) {
        if (Typeof(thing1) == preferred) {
            if (Typeof(thing2) != preferred) {
                return thing1;
            }
        } else if (Typeof(thing2) == preferred) {
            return thing2;
        }
    }

    /* Do we want to check if the objects are locked against the player? */
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

    /* Try environment distance, pick the closer one if applicable. */
    int d1 = env_distance(md->match_from, thing1);
    int d2 = env_distance(md->match_from, thing2);

    if (d1 < d2) return thing1;
    else if (d2 < d1) return thing2;

    /* All else fails, toss a coin. */
    return (RANDOM() % 2 ? thing1 : thing2);
}

/*
 * @TODO The way matching happens right now is ridiculous.  You set up
 *       a structure, then you run a series of functions.  The order matters
 *       and could be easily messed up.
 *
 *       There are a lot of different ways we could sort this out.
 *       I'm thinking we could provide two ways to do it.  We could stack
 *       up the match_data structure like we do now ... and just have what
 *       forms of searches we want as like a bitvector so there's a single
 *       call to do the match instead of calling matches in a series.
 *
 *       We can also provide a simple 'standard match' function that covers
 *       the 99% case and doesn't require setting up match_data.
 */

/**
 * Match player based on name.
 *
 * This also deducts the tp_lookup_cost if applicable.
 *
 * If the md->match_name does not start with a '*' this does absolutely
 * nothing.
 *
 * Always does an exact match, and if a player is found, it will be
 * put in md->exact_match
 *
 * @param md the match_data struct initialized with match settings
 */
void
match_player(struct match_data *md)
{
    dbref match;
    const char *p;

    if (*(md->match_name) == LOOKUP_TOKEN
        && payfor(OWNER(md->match_from), tp_lookup_cost)) {
        p = md->match_name + 1;

        if ((match = lookup_player(p)) != NOTHING) {
            md->exact_match = match;
        }
    }
}

/**
 * Returns dbref if registered object found for name, else NOTHING
 *
 * This processes strings that start with $.  If the 'name' does not start
 * with $, it is ignored.  The search is done relative to 'player'.
 * Registration props are looked for in REGISTRATION_PROPDIR
 *
 * Strings, refs, and integer type props are all converted to dbref if
 * possible.
 *
 * @param player the player where our registration search starts
 * @param name the name of the registration property to find
 * @return dbref that the registration resolves to or NOTHING
 */
dbref
find_registered_obj(dbref player, const char *name)
{
    dbref match;
    const char *p;
    char buf[BUFFER_LEN];
    PropPtr ptr;

    if (*name != REGISTERED_TOKEN)
        return (NOTHING);

    p = name + 1;

    if (!*p)
        return (NOTHING);

    snprintf(buf, sizeof(buf), "%s/%s", REGISTRATION_PROPDIR, p);
    ptr = envprop(&player, buf, 0);

    if (!ptr)
        return NOTHING;

#ifdef DISKBASE
    propfetch(player, ptr); /* DISKBASE PROPVALS */
#endif

    switch (PropType(ptr)) {
        case PROP_STRTYP:
            p = PropDataStr(ptr);

            if (*p == NUMBER_TOKEN)
                p++;

            if (number(p)) {
                match = (dbref) atoi(p);

                if (OkObj(match))
                    return match;
            }

            break;

        case PROP_REFTYP:
            match = PropDataRef(ptr);

            if (match == HOME || match == NIL || OkObj(match))
                return match;

            break;

        case PROP_INTTYP:
            match = (dbref) PropDataVal(ptr);

            if (OkObj(match))
                return match;

            break;

        case PROP_LOKTYP:
            return (NOTHING);
    }

    return (NOTHING);
}


/**
 * Check md->match_name to see if it is a $registration match
 *
 * If md->match_name doesn't start with $, this does nothing.  If it
 * finds something, it stores the result in md->exact_match.  Otherwise
 * it does nothing.  Uses find_registered_obj under the hood.
 *
 * @see find_registered_obj
 *
 * @param md the match_data configuration structure
 */
void
match_registered(struct match_data *md)
{
    dbref match;

    match = find_registered_obj(md->match_from, md->match_name);

    if (match != NOTHING)
        md->exact_match = match;
}

/**
 * Parse a string into a dbref or NOTHING if it can't resolve it.
 *
 * Does not handle leading #; this just wraps strtol and does the error
 * check.  atoi cannot be used because there is no difference between 0 and
 * an error.
 *
 * @TODO This is literally used in only one place.  Relocate this into
 *       absolute_name which is a way more useful function anyway.
 *
 * @private
 * @param s the string to try and convert
 * @return a dbref or NOTHING if the conversion failed.
 */
static dbref
parse_dbref(const char *s)
{
    char *p;
    long x = strtol(s, &p, 10);
    return (p == s) ? NOTHING : x;
}

/**
 * Returns a dbref of the match_name is in the format of #12345
 *
 * Returns NOTHING if the string isn't in the right format, or if ths
 * parsed reference doesn't resolve to an actual object.
 *
 * @private
 * @param md the configured match_data struct
 * @return either the dbref if the string resolves to a proper #dbref or
 *         NOTHING
 */
static dbref
absolute_name(struct match_data *md)
{
    dbref match;

    if (*(md->match_name) == NUMBER_TOKEN && !isspace(*(md->match_name + 1))) {
        match = parse_dbref((md->match_name) + 1);

        if (!ObjExists(match)) {
            return NOTHING;
        } else {
            return match;
        }
    } else {
        return NOTHING;
    }
}

/**
 * Matches match_names that are dbref strings that start with #
 *
 * If the dbref fails to parse, or the string doesn't start with #, or
 * if the dbref doesn't belong to an object in the DB, this will do nothing.
 * On successful match, it populates md->exact_match
 *
 * @param md the configured match_data struct
 */
void
match_absolute(struct match_data *md)
{
    dbref match;

    if ((match = absolute_name(md)) != NOTHING) {
        md->exact_match = match;
    }
}

/**
 * Matches the word 'me', case insensitive, and resolves it to match_who
 *
 * If the match_name is not 'me', then this does nothing.  Otherwise, it
 * will set md->exact_match
 *
 * @param md the configured match_data struct
 */
void
match_me(struct match_data *md)
{
    if (!strcasecmp(md->match_name, "me")) {
        md->exact_match = md->match_who;
    }
}

/**
 * Matches 'here', case insensitive, and resolves to match_who's location
 *
 * If the match_name is not 'here', then this does nothing.  If match_who's
 * location is NOTHING, it will also do nothing.  Otherwise, it will set
 * md->exact_match
 *
 * @param md the configured match_data struct
 */
void
match_here(struct match_data *md)
{
    if (!strcasecmp(md->match_name, "here")
        && LOCATION(md->match_who) != NOTHING) {
        md->exact_match = LOCATION(md->match_who);
    }
}

/**
 * Matches the word 'home', case insensitive, and resolves it to HOME
 *
 * If the match_name is not 'home', then this does nothing.  Otherwise, it
 * will set md->exact_match
 *
 * @param md the configured match_data struct
 */
void
match_home(struct match_data *md)
{
    if (!strcasecmp(md->match_name, "home"))
        md->exact_match = HOME;
}

/**
 * Matches the word 'nil', case insensitive, and resolves it to NIL
 *
 * If the match_name is not 'nil', then this does nothing.  Otherwise, it
 * will set md->exact_match
 *
 * @param md the configured match_data struct
 */
void
match_nil(struct match_data *md)
{
    if (!strcasecmp(md->match_name, "nil"))
        md->exact_match = NIL;
}

/**
 * Use the contents of 'obj' as a basis for matching what is set up in 'md'
 *
 * It will only match things in 'obj', even if match_from is a #dbref.
 *
 * @private
 * @param obj the object to scan contents for
 * @param md the configured match_data structure.
 */
static void
match_contents(dbref obj, struct match_data *md)
{
    dbref absolute, first;

    absolute = absolute_name(md);

    if (!controls(OWNER(md->match_from), absolute))
        absolute = NOTHING;

    first = CONTENTS(obj);

    DOLIST(first, first) {
        if (first == absolute) {
            md->exact_match = first;
            return;
        } else if (!strcasecmp(NAME(first), md->match_name)) {
            /*
             * If there are multiple exact matches, randomly choose one.
             * This works becaus md->exact_match will have been populated
             * (potentially) by past iterations of this loop.
             */
            md->exact_match = choose_thing(md->match_descr, md->exact_match,
                                           first, md);
        } else if (string_match(NAME(first), md->match_name)) {
            md->last_match = first; /* Partial match case */
            (md->match_count)++;
        }
    }
}

/**
 * Match match_from's contents list
 *
 * This is an obnoxiously tiny function, but it serves a purpose (for now).
 * Uses the static function @match_contents under the hood.
 *
 * @param md the configured match structure
 */
void
match_possession(struct match_data *md)
{
    match_contents(md->match_from, md);
}

/**
 * Match match_from's location's content list
 *
 * If match_from's location is NOTHING, this does nothing.
 * Uses the static function @match_contents under the hood.
 *
 * @param md the configured match structure
 */
void
match_neighbor(struct match_data *md)
{
    dbref loc;

    if ((loc = LOCATION(md->match_from)) != NOTHING) {
        match_contents(loc, md);
    }
}

/**
 * Match a list of exits, starting with the first entry of EXITS(obj)
 *
 * It will match exits of players, rooms, or things using the criteria
 * loaded in 'md'.  It understands how to parse exit aliases and works
 * pretty smartly.  Only the object 'obj' is searched by this, environmental
 * style searches are not done.
 *
 * @private
 * @param obj the object to search for exits on
 * @param md the matching criteria
 */
static void
match_exits(dbref obj, struct match_data *md)
{
    dbref exit, absolute, first;
    const char *exitname, *p;
    int exitprog, lev, partial;

    first = EXITS(obj);

    if (first == NOTHING)
        return; /* Easy fail match */

    if ((LOCATION(md->match_from)) == NOTHING)
        return;

    absolute = absolute_name(md); /* parse #nnn entries */

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
            for (int i = 0; i < DBFETCH(exit)->sp.exit.ndest; i++)
                if ((DBFETCH(exit)->sp.exit.dest)[i] == NIL
                    || Typeof((DBFETCH(exit)->sp.exit.dest)[i]) == TYPE_PROGRAM)
                    exitprog = 1;
        }

        if (tp_enable_prefix && exitprog && md->partial_exits &&
            (FLAGS(exit) & XFORCIBLE) && FLAGS(OWNER(exit)) & WIZARD) {
            partial = 1;
        } else {
            partial = 0;
        }

        exitname = NAME(exit);

        while (*exitname) { /* for all exit aliases */
            int notnull = 0;

            for (p = md->match_name; /* check out 1 alias */
                 *p &&
                 tolower(*p) == tolower(*exitname) &&
                 *exitname != EXIT_DELIMITER; p++, exitname++) {
                if (!isspace(*p)) {
                    notnull = 1;
                }
            }

            /* did we get a match on this alias? */
            if ((partial && notnull) || ((*p == '\0')
                || (*p == ' ' && exitprog))) {
                skip_whitespace(&exitname);
                lev = PLevel(exit);

                if (tp_compatible_priorities && (lev == 1) &&
                    (LOCATION(exit) == NOTHING ||
                     Typeof(LOCATION(exit)) != TYPE_THING ||
                     controls(OWNER(exit), LOCATION(md->match_from))))
                    lev = 2;

                if (*exitname == '\0' || *exitname == EXIT_DELIMITER) {
                    /* we got a match on this alias */
                    if (lev >= md->match_level) {
                        if (strlen(md->match_name) - strlen(p)
                            > (size_t) md->longest_match) {
                            if (lev > md->match_level) {
                                md->match_level = lev;
                                md->block_equals = 0;
                            }

                            md->exact_match = exit;
                            md->longest_match =
                                        strlen(md->match_name) - strlen(p);

                            if ((*p == ' ') || (partial && notnull)) {
                                strcpyn(match_args, sizeof(match_args),
                                        (partial && notnull) ? p : (p + 1));
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
                                strcpyn(match_cmdname, sizeof(match_cmdname),
                                        (char *) md->match_name);
                            }
                        } else if ((strlen(md->match_name) - strlen(p) ==
                                   (size_t) md->longest_match) 
                                   && !((lev == md->match_level)
                                   && (md->block_equals))) {
                            if (lev > md->match_level) {
                                md->exact_match = exit;
                                md->match_level = lev;
                                md->block_equals = 0;
                            } else {
                                md->exact_match =
                                    choose_thing(md->match_descr,
                                                 md->exact_match, exit,
                                                 md);
                            }

                            if (md->exact_match == exit) {
                                if ((*p == ' ') || (partial && notnull)) {
                                    strcpyn(match_args, sizeof(match_args),
                                            (partial && notnull) ? p : (p + 1));
                                    {
                                        char *pp;
                                        int ip;

                                        for (ip = 0,
                                             pp = (char *) md->match_name;
                                             *pp && (pp != p); pp++)
                                            match_cmdname[ip++] = *pp;

                                        match_cmdname[ip] = '\0';
                                    }
                                } else {
                                    *match_args = '\0';
                                    strcpyn(match_cmdname, sizeof(match_cmdname),
                                            (char *) md->match_name);
                                }
                            }
                        }
                    }

                    goto next_exit;
                }
            }

            /* we didn't get it, go on to next alias */
            while (*exitname && *exitname++ != EXIT_DELIMITER) ;
            skip_whitespace(&exitname);
        } /* end of while alias string matches */

      next_exit:
        ;
    }
}

/**
 * Matches actions attached to objects in inventory
 *
 * Thie object who's inventory is searched is md->match_from
 *
 * @private
 * @param md the configured match object
 */
static void
match_invobj_actions(struct match_data *md)
{
    dbref thing;

    if (CONTENTS(md->match_from) == NOTHING)
        return;

    DOLIST(thing, CONTENTS(md->match_from)) {
        if (Typeof(thing) == TYPE_THING && EXITS(thing) != NOTHING) {
            match_exits(thing, md);
        }
    }
}

/**
 * Matches actions attached to objects in the room.
 *
 * Thie object who's location is searched is md->match_from
 *
 * @private
 * @param md the configured match object
 */
static void
match_roomobj_actions(struct match_data *md)
{
    dbref thing, loc;

    if ((loc = LOCATION(md->match_from)) == NOTHING)
        return;

    if (CONTENTS(loc) == NOTHING)
        return;

    DOLIST(thing, CONTENTS(loc)) {
        if (Typeof(thing) == TYPE_THING && EXITS(thing) != NOTHING) {
            match_exits(thing, md);
        }
    }
}

/**
 * Matches actions attached to player or maybe more generally "caller"
 *
 * The object who's actions are scanned is md->match_from.  This only
 * does something for TYPE_PLAYER, TYPE_ROOM, and TYPE_THING objects.
 * Other types are ignored.
 *
 * @private
 * @param md the configured match object
 */
static void
match_player_actions(struct match_data *md)
{
    switch (Typeof(md->match_from)) {
        case TYPE_PLAYER:
        case TYPE_ROOM:
        case TYPE_THING:
            match_exits(md->match_from, md);

        default:
            break;
    }
}

/**
 * Matches actions attached to the location 'loc'
 *
 * Only does matching if loc is TYPE_PLAYER, TYPE_ROOM, or TYPE_THING.
 * This is a very thin wrapper around match_exits.
 *
 * @see match_exits
 *
 * @private
 * @param md the configured match object
 */
static void
match_room_exits(dbref loc, struct match_data *md)
{
    switch (Typeof(loc)) {
        case TYPE_PLAYER:
        case TYPE_ROOM:
        case TYPE_THING:
            match_exits(loc, md);

        default:
            break;
    }
}

/**
 * Matches actions on player, objects in room, objects in inventory, and room
 *
 * This happens in reverse order of priority order.  This is the heart-and-soul
 * of what powers finding exits on the MUCK.
 *
 * This will scan the entire environment (up to a hard coded depth of 88
 * environment rooms) until it finds a match or stops.  This respects
 * the YIELD and OVERT flags.  If YIELD is set on a room in the
 * environment, it is considered "blocking" and other rooms in the
 * environment will be ignored unless they are set OVERT.
 *
 * The matching starts from md->match_from
 *
 * @param md the configured match_data structure
 */
void
match_all_exits(struct match_data *md)
{
    dbref loc;
    int limit = 88;
    int blocking = 0;

    strcpyn(match_args, sizeof(match_args), "\0");
    strcpyn(match_cmdname, sizeof(match_cmdname), "\0");

    if ((loc = LOCATION(md->match_from)) != NOTHING)
    {
        if (FLAGS(loc) & YIELD)
            blocking = 1;
        match_room_exits(loc, md);
    }

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

    /*
     * Walk the environment chain to #0, or until depth chain limit
     * has been hit, looking for a match.
     */
    while ((loc = LOCATION(loc)) != NOTHING) {
        /*
         * If we're blocking (because of a yield), only match a room if
         * and only if it has overt set on it.
         */
        if ((blocking && FLAGS(loc) & OVERT) || !blocking) {
            if (md->exact_match != NOTHING)
                md->block_equals = 1;

            match_room_exits(loc, md);
        }

        if (!limit--)
            break;

        /* Does this room have env-chain exit blocking enabled? */
        if (!blocking && FLAGS(loc) & YIELD) {
            blocking = 1;
        }
    }
}

/**
 * Shortcut to run all matches in a logical order.
 *
 * The order done is:
 *
 * - match_all_exits
 * - match_neighbor
 * - match_posession
 * - match_me
 * - match_here
 * - match_registered
 * - match_absolute
 * - match_player (if owner md->match_from or md->match_who is wizard)
 *
 * @param md a configured match_data structure.
 */
void
match_everything(struct match_data *md)
{
    match_all_exits(md);
    match_neighbor(md);
    match_possession(md);
    match_me(md);
    match_here(md);
    match_registered(md);
    match_absolute(md);

    if (Wizard(OWNER(md->match_from)) || Wizard(md->match_who)) {
        match_player(md);
    }
}

/**
 * Extracts the final results from a match_data structure.
 *
 * md->exact_match wins if set.  Otherwise, if there was only one match,
 * md->last_match wins.  If there were no matches, returns NOTHING, otherwise
 * it will return AMBIGUOUS for too many matches.
 *
 * If you don't care about ambiguous results, @see last_match_result
 *
 * @param md the configured match_data object to unpack results from.
 * @return a matched dbref, NOTHING, or AMBIGUOUS as described above.
 */
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

/**
 * Extracts the final results from a match_data, ignoring ambiguity.
 *
 * What this specifically means, is if there is not an exact match,
 * then the last match (whatever it was) is picked regardless of if
 * multiple potentials matched equally well (which would match as
 * AMBIGUOUS if you used match_result).
 *
 * @see match_result
 *
 * This may, of course, still return NOTHING if nothing matched.
 *
 * @param md the configured match_data object to unpack results from
 * @return a matched dbref or NOTHING.
 */
dbref
last_match_result(struct match_data * md)
{
    if (md->exact_match != NOTHING) {
        return (md->exact_match);
    } else {
        return (md->last_match);
    }
}

/**
 * Like match_result, except it emits a message on failure to match.
 *
 * @see match_result
 *
 * The message is sent to md->match_who with an appropriate message
 * for NOTHING or AMBIGUOUS results.  This function will either return
 * a matched ref or NOTHING.
 *
 * @param md the configured match_data object to unpack results from
 * @return a matched dbref or NOTHING
 */
dbref
noisy_match_result(struct match_data * md)
{
    dbref match;

    switch (match = match_result(md)) {
        case NOTHING:
            notifyf_nolisten(md->match_who,
                             match_msg_nomatch(md->match_name, 0));
            return NOTHING;

        case AMBIGUOUS:
            notifyf_nolisten(md->match_who,
                             match_msg_ambiguous(md->match_name, 0));
            return NOTHING;

        default:
            return match;
    }
}

/**
 * Match contents and exits of 'arg1'
 *
 * MUF describes this as checking "all objects and actions associated"
 * with object 'arg1'.  This is used in a very limited context; mostly
 * around fetching things from container objects it looks like.
 *
 * @param arg1 the object we're basing our search around
 * @param md the configured match_data object with search string
 */
void
match_rmatch(dbref arg1, struct match_data *md)
{
    if (arg1 == NOTHING)
        return;

    switch (Typeof(arg1)) {
        case TYPE_PLAYER:
        case TYPE_ROOM:
        case TYPE_THING:
            match_contents(arg1, md);
            match_exits(arg1, md);
            break;
    }
}

/**
 * Does a match, but if player doesn't control what is matched, it will fail.
 *
 * Does a 'match_everything' on 'name', and makes sure 'player' controls
 * the result.
 *
 * @see match_everything
 * @see noisy_match_result
 * @see controls
 *
 * This emits output to player if the object is not found, or is ambiguous,
 * or if the player doesn't control the matched object.
 *
 * @param descr the descriptor of the player matching
 * @param player the player starting the match
 * @param name the string to match
 * @return either a dbref of an object controlled by player, or NOTHING
 */
dbref
match_controlled(int descr, dbref player, const char *name)
{
    dbref match;
    struct match_data md;

    init_match(descr, player, name, NOTYPE, &md);
    match_everything(&md);

    match = noisy_match_result(&md);

    if (match != NOTHING && !controls(player, match)) {
        notify(player,
               "Permission denied. (You don't control what was matched)");
        return NOTHING;
    } else {
        return match;
    }
}

/**
 * Generate a standard 'couldn't find object' message.
 *
 * This returns a pointer to a static buffer; DO NOT free it, and please
 * note that this is not threadsafe and can mutate out from under you
 * if you aren't careful.
 *
 * @param s the search string that failed
 * @param types the unused parameter that Wyld warned me about (tanabi)
 * @return pointer to static buffer containing 'I don't understand' string.
 */
char *
match_msg_nomatch(const char *s, unsigned short types)
{
    static char buf[BUFFER_LEN];

    /*
     * @TODO Unused variable removal!
     */
    (void)types;
    snprintf(buf, sizeof(buf), "I don't understand '%s'.", s);
    return buf;
}

/**
 * Generate a standard 'object is ambiguous' message.
 *
 * This returns a pointer to a static buffer; DO NOT free it, and please
 * note that this is not threadsafe and can mutate out from under you
 * if you aren't careful.
 *
 * @param s the search string that failed
 * @param types the unused parameter that Wyld warned me about (tanabi)
 * @return pointer to static buffer containing 'which do you mean?' string.
 */
char *
match_msg_ambiguous(const char *s, unsigned short types)
{
    static char buf[BUFFER_LEN];
    /*
     * @TODO Unused variable removal!
     */
    (void)types;
    snprintf(buf, sizeof(buf), "I don't know which '%s' you mean!", s);
    return buf;
}
