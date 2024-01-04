/** @file player.h
 *
 * Declaration of a variety of player-specific operations, such as
 * creating a player, setting password, etc.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "config.h"

/**
 * Adds a player to the player lookup cache: WARNING poorly named function
 *
 * This does NOT actually create a player -- @see create_player
 *
 * @todo: Rename this to player_hash_add or something similar to that
 *        to make it more clear.
 *
 * @param who the player ref to add to the hash
 */
void add_player(dbref who);

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
int check_password(dbref player, const char *password);

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
void change_player_name(dbref player, const char *name);

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
void clear_players(void);

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
dbref create_player(const char *name, const char *password, char *error);

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
 * @todo: Rename this to player_hash_delete or something similar to that
 *        to make it more clear.
 *
 * @param who the player to remove from the cache
 */
void delete_player(dbref who);

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
dbref lookup_player(const char *name);

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
int ok_password(const char *password);

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
int payfor(dbref who, int cost);

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
void toad_player(int descr, dbref player, dbref victim, dbref recipient);

/**
 * Set a player's password
 *
 * This takes a 'raw' unhashed password and hashes it before setting it.
 *
 * @param player the player ref to set a password for
 * @param password the password that will be hashed and set
 */
void set_password(dbref player, const char *password);

/**
 * Set a player's password without hashing it
 *
 * You usually don't want to use this call.
 *
 * @param player the player ref to set a password for
 * @param password the password that will be set as-as unhashed
 */
void set_password_raw(dbref player, const char *password);

#endif /* !PLAYER_H */
