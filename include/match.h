/** @file match.h
 *
 * Header for the object matching system used by Fuzzball.  It is surprisingly
 * complex and is something akin to Fuzzball's "SQL" in a vague sense, since
 * it provides a means to query objects in the DB in a few different ways.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef MATCH_H
#define MATCH_H

#include "config.h"

struct match_data {
    dbref exact_match;       /* holds result of exact match */
    int check_keys;          /* if non-zero, check for keys */
    dbref last_match;        /* holds result of last match */
    int match_count;         /* holds total number of inexact matches */
    dbref match_who;         /* player used for me, here, and messages */
    dbref match_from;        /* object which is being matched around */
    int match_descr;         /* descriptor initiating the match */
    const char *match_name;  /* name to match */
    int preferred_type;      /* preferred type */
    int longest_match;       /* longest matched string */
    int match_level;         /* the highest priority level so far */
    int block_equals;        /* block matching of same name exits */
    int partial_exits;       /* if non-zero, allow exits to match partially */
};


/**
 * Returns dbref if registered object found for name, else NOTHING
 *
 * This processes strings that start with $.  If the 'name' does not start
 * with $, it is ignored.  The search is done relative to 'player'.
 * Registration props are looked for in REGISTRATION_PROPDIR
 *
 * @param player the player where our registration search starts
 * @param name the name of the registration property to find
 * @return dbref that the registration resolves to or NOTHING
 */
dbref find_registered_obj(dbref player, const char *name);

/**
 * Initialize a match structure with initial values for doing an object search
 *
 * @param descr the descriptor of the person doing the search
 * @param player the player doing the search
 * @param name the name you're looking for
 * @param type the object type you are looking for (TYPE_*)
 * @param md the match_data structure to initialize
 */
void init_match(int descr, dbref player, const char *name, int type, struct match_data *md);

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
void init_match_check_keys(int descr, dbref player, const char *name, int type, struct match_data *md);

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
void init_match_remote(int descr, dbref player, dbref what, const char *name, int type, struct match_data *md);

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
dbref last_match_result(struct match_data *md);

/**
 * Matches match_names that are dbref strings that start with #
 *
 * If the dbref fails to parse, or the string doesn't start with #, or
 * if the dbref doesn't belong to an object in the DB, this will do nothing.
 * On successful match, it populates md->exact_match
 *
 * @param md the configured match_data struct
 */
void match_absolute(struct match_data *md);

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
void match_all_exits(struct match_data *md);

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
dbref match_controlled(int descr, dbref player, const char *name);

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
void match_everything(struct match_data *md);

/**
 * Matches 'here', case insensitive, and resolves to match_who's location
 *
 * If the match_name is not 'here', then this does nothing.  If match_who's
 * location is NOTHING, it will also do nothing.  Otherwise, it will set
 * md->exact_match
 *
 * @param md the configured match_data struct
 */
void match_here(struct match_data *md);

/**
 * Matches the word 'home', case insensitive, and resolves it to HOME
 *
 * If the match_name is not 'home', then this does nothing.  Otherwise, it
 * will set md->exact_match
 *
 * @param md the configured match_data struct
 */
void match_home(struct match_data *md);

/**
 * Matches the word 'nil', case insensitive, and resolves it to NIL
 *
 * If the match_name is not 'nil', then this does nothing.  Otherwise, it
 * will set md->exact_match
 *
 * @param md the configured match_data struct
 */
void match_nil(struct match_data *md);

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
char *match_msg_ambiguous(const char *s, unsigned short types);

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
char *match_msg_nomatch(const char *s, unsigned short types);

/**
 * Matches the word 'me', case insensitive, and resolves it to match_who
 *
 * If the match_name is not 'me', then this does nothing.  Otherwise, it
 * will set md->exact_match
 *
 * @param md the configured match_data struct
 */
void match_me(struct match_data *md);

/**
 * Match match_from's location's content list
 *
 * If match_from's location is NOTHING, this does nothing.
 * Uses the static function @match_contents under the hood.
 *
 * @param md the configured match structure
 */
void match_neighbor(struct match_data *md);

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
void match_player(struct match_data *md);

/**
 * Match match_from's contents list
 *
 * This is an obnoxiously tiny function, but it serves a purpose (for now).
 * Uses the static function @match_contents under the hood.
 *
 * @param md the configured match structure
 */
void match_possession(struct match_data *md);

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
void match_registered(struct match_data *md);

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
dbref match_result(struct match_data *md);

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
void match_rmatch(dbref, struct match_data *md);

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
dbref noisy_match_result(struct match_data *md);

#endif /* !MATCH_H */
