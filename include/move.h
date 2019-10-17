/** @file move.h
 *
 * Header for handling movement on the MUCK, such as moving a player from
 * room to room.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef MOVE_H
#define MOVE_H

#include "config.h"

/**
 * Have a given player (or player-like thing) enter a room and do all processing
 *
 * Note: This call has some partial support for rooms and programs
 *       to be moved with it, however that does not appear to be the
 *       primary purpose and using this call with rooms, players, or
 *       non-zombie THINGS may have odd results and is not recommended.
 *
 * This does the following things:
 *
 * - Checks for parent loops of 'player' and 'loc'; the logic of what happens
 *   when a parent loop is detected is somewhat complicated but in most
 *   cases the player or thing is sent home with a few edge cases that
 *   probably don't happen much.
 *
 * - If the starting location of 'player' is not the same as 'loc', then
 *   'player' is moved to 'loc' ( @see moveto )
 *
 *   Then, if 'player' didn't come from NOTHING, depart propqueues are run.
 *   A "X has left." message is sent from the source room based on
 *   player flags, room flags, exit flags, and tp_quiet_moves (basically
 *   this is where the DARK flag checking is done)
 *
 *   Then, If the previous location of 'player' wasn't NOTHING, and was set
 *   STICKY and has a dropto, dropto processing is done (objects sent to
 *   DROPTO only if there are no players or zombies left in the room).
 *
 *   Finally, the "X has arrived" notification is sent to the target room.
 *   To make propqueues run after autolook, this message is disconnected from
 *   the arrive propqueue processing which is done further down. 
 *   This notification follows the same rules as the "has left"  message
 *
 * - If 'player' is not a thing, or if 'player' is a ZOMBIE or VEHICLE, then
 *   look is triggered.  Note that this is where this function falls down for
 *   rooms and programs.  Look action loops are detected and if they are found
 *   to go 8 deep an error is displayed.  This is NOT threadsafe because it
 *   uses a global static 'donelook'.  This supports tp_autolook_cmd if
 *   set, otherwise @see look_room
 *
 * - If tp_penny_rate != 0, player doesn't control loc, player has less
 *   than tp_max_pennies, and a random number % tp_penny_rate == 0, then
 *   the owner of the player gets a penny.
 *
 * - Finally, arrive propqueue is processed if loc != old
 *
 * There's a lot going on here :)
 *
 * @param descr the descriptor of the person doing the action
 * @param player the object moving
 * @param loc the place the object is moving to
 * @param exit the exit that triggered the move
 */
void enter_room(int descr, dbref player, dbref loc, dbref exit);

/**
 * Do a move of 'what' to 'where', handling special cases of 'where'
 *
 * This is used to move rooms and non-zombie things; exits are moved
 * in a different fashion, and players/zombies should be moved using
 * enter_room instead.
 *
 * @see enter_room
 *
 * Technically it wouldn't be the end of the world if a player or zombie
 * were moved with this call, but, it wouldn't handle the special processing,
 * messaging, and whatnot so it would move them in a weird fashion.
 *
 * @param what the thing to move
 * @param where the destination to move it to
 */
void moveto(dbref what, dbref where);

/**
 * Send all the contents of 'loc' to 'dest'
 *
 * Loops through all the contents of 'loc' and send it over to 'dest'.
 *
 * @param descr the descriptor of the person who triggered the move
 * @param loc the location to to move contents from
 * @param dest the location to send contents to
 */
void send_contents(int descr, dbref loc, dbref dest);

/**
 * Sweep 'thing' home
 *
 * For players or things where puppethome is true, their contents are
 * swept home as well.  Programs are swept to their owner.  Rooms and
 * exits cannot be swept.
 *
 * @param descr the descriptor associated with the person doing the action
 * @param thing the thing to return home
 * @param puppethome if true, and thing is a THING, sweep its contents
 */
void send_home(int descr, dbref thing, int homepuppet);

/**
 * Implementation of recycling an object
 *
 * Does all the sanity checking necessary.  For instance, if something is
 * being forced to recycle, we make sure that it won't create an unstable
 * situation.
 *
 * Rooms and things have all their exits deleted.  Programs have their
 * filesystem file deleted.
 *
 * Then the fields are all nulled out as necessary and the object converted
 * to garbage.
 *
 * @param descr the descriptor of the player trigging the recycle
 * @param player the player doing recycling
 * @param thing the thing to recycle
 */
void recycle(int descr, dbref player, dbref thing);

#endif /* !MOVE_H */
