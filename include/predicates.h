/** @file predicates.h
 *
 * Declaration of the "predicates" which are a set of calls that verify
 * if one is allowed to do certain actions or not.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef PREDICATES_H
#define PREDICATES_H

#include "config.h"
#include "db.h"

/**
 * Checks if a player can perform an action, emitting messages
 *
 * This does zombie checking, and if tp_allow_zombies is false it will
 * emit an appropriate error message.
 *
 * could_doit is used to see if the 'thing' is actionable by 'player'.
 * @see could_doit
 *
 * If could_doit returns true, success messages will be displayed
 * (succ and osucc) and this call will return true.  Otherwise, fail/ofail
 * messages will be displayed and this call will return false.
 *
 * @param descr the descriptor of the actor
 * @param player the player actor
 * @param thing the thing the player is trying to work with
 * @param default_fail_msg a fail message to use if no 'fail' prop set
 * @returns boolean true if player can do it, false if not
 */
int can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg);

/**
 * Checks to see if what can be linked to something else by who.
 *
 * @param who the person who is doing the linking
 * @param what the object 'who' would like to link
 * @return boolean true if who can link what to something
 */
int can_link(dbref who, dbref what);

/**
 * Checks if 'who' can link an object of type 'what_type' to 'where'
 *
 * Because this may be called prior to the object existing, it is
 * checked by type; however, 'Typeof(dbref)' is often used to see
 * if some existing objet's ref can link to where.
 *
 * @param who the person we are checking link permissions for
 * @param what_type the type of the object we wish to link to where
 * @param where the place we are linking to
 * @returns boolean true if who can link to what, false otherwise
 */
int can_link_to(dbref who, object_flag_type what_type, dbref where);

/**
 * Can 'player' move in direction 'direction'
 *
 * If direction is 'home' and tp_enable_home is true, then this will
 * always return true.
 *
 * Otherwise, it will match 'direction' to exits with match_level set
 * to 'lev'.  @see init_match
 *
 * If an appropriate exit is found, returns true, otherwise returns false.
 *
 * @param descr the descriptor for the player
 * @param player the player trying to move
 * @param direction the direction they want to move in
 * @param lev the match_level to use -- @see init_match
 * @returns true if the player can move in that direction, false otherwise
 */
int can_move(int descr, dbref player, const char *direction, int lev);

/**
 * Check to see if 'who' can see the flags of 'where'
 *
 * The rules are the same as can_teleport_to, for now, but that could
 * change so this is kept separate.
 *
 * @see can_teleport_to
 *
 * @param who the person to check to see if they can see the flags
 * @param where the object to check if flags can be seen
 */
int can_see_flags(dbref who, dbref where);

/**
 * Checks if 'who' can teleport to 'where'
 *
 * This is used to see if a player can teleport to a place.  This does not
 * check if 'where' exists.  Tests link lock on the target, LINK_OK flag
 * on the target, if player controls target, and ABODE flag if target is
 * not a thing.
 *
 * @param who the player who wants to teleport
 * @param where the destination the player wishes to teleport to
 * @returns boolean true if who can teleport to where, false otherwise
 */
int can_teleport_to(dbref who, dbref where);

/**
 * Checks to see if player could actually do what is proposing to be done
 *
 * * If thing is an exit, this checks to see if the exit will
 *   perform a move that is allowed.
 * * Then, it checks the \@lock on the thing, whether it's an exit or not.
 *
 * tp_secure_teleport note:
 *
 * You can only jump with an action from rooms that you own
 * or that are jump_ok, and you cannot jump to players that are !jump_ok.
 */
int could_doit(int descr, dbref player, dbref thing);

/**
 * Recursive check for loops in destinations of exits.
 *
 * Checks to see if any circular references are present in the destination
 * chain.  Returns 1 if circular reference found, 0 if not.
 *
 * So, you want to make sure this returns false prior to allowing an exit
 * link.
 *
 * @param source the exit to check
 * @param dest the potential destination to verify no loops
 * @return boolean true if an exit loop exists, false otherwise
 */
int exit_loop_check(dbref source, dbref dest);

/**
 * Checks for a parent loop, resolving HOME to the appropriate real dbref
 *
 * This makes sure that 'source' can use 'dest' as a parent without causing
 * an environment loop.  If 'dest' is HOME, then it will be translated to
 * the correct actual ref -- PLAYER_HOME, THING_HOME, GLOBAL_ENVIRONMENT
 * (for rooms), or the owner for programs.
 *
 * This returns true if a loop is created or if HOME could not be translated
 * to something meaningful, so you only want to allow source to be moved
 * to dest IF this returns *false*.
 *
 * @param source the source ref
 * @param dest the place we wish to parent source to
 * @returns true if a loop would be created, false if not
 */
int parent_loop_check(dbref source, dbref dest);

#endif /* !PREDICATES_H */
