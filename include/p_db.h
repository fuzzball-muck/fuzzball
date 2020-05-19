/** @file p_db.h
 *
 * Declaration of DB related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_DB_H
#define P_DB_H

#include "interp.h"

/**
 * Implementation of MUF ADDPENNIES
 *
 * Consumes a dbref and an integer, and adds that value to the player or
 * thing dbref.  Requires MUCKER level tp_addpennies_muf_mlev for players
 * and 4 for things.
 *
 * Needs MUCKER level 4 to bypass range checks for players.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_addpennies(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MOVETO
 *
 * Consumes two dbrefs, and moves the first object to the non-exit second.
 *
 * Needs MUCKER level 3 to bypass basic ownership and JUMP_OK checks, and
 * also to move an exit.
 *
 * This code should be further reviewed, and restructured to be easier to
 * understand and maintain - and match other movement logic when applicable.
 * It *seems* like we may find some unexpected consequences of using
 * fallthrough logic in the main switch.
 *
 * Here we go --
 *
 * Moving a player will fail if the destination is not a room or basic vehicle.
 *
 * Moving a player/thing will fail if it would create a parent loop.
 *
 * Moving a player/thing with less than MUCKER level 3 will fail if:
 * - source location or destination is not JUMP_OK
 * - source location or destination does not pass basic ownership checks
 * - destinaton is a basic vehicle and source is not in the same room
 * - source is a guest and the destination room does not accept guests
 *
 * Moving a player/thing/program will fail if the destination is not a player,
 * thing, or room.
 *
 * Moving a player/thing/program with less than MUCKER level three will fail if
 * the source location or destination is not JUMP_OK or does not pass basic
 * ownership checks.
 *
 * Moving an exit will fail if the destination is not a player, thing, or
 * room (as above!) - but also if the destination is HOME.
 *
 * Moving an exit with less that MUCKER level 3 will fail if neither source
 * nor destination pass basic ownership checks.
 *
 * Moving a room will fail if:
 * - the desintation is also not a room and tp_secure_thing_movement is off
 * - the desitnation is the GLOBAL_ENVIRONMENT
 *
 * Moving a room with less than MUCKER level 3 will fail if the destination
 * is HOME and the source and source location do not pass basic ownership
 * checks.  Otherwise, the source must pass basic ownership and teleport
 * checks and the movement must not create a parent loop.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_moveto(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PENNIES
 *
 * Consumes a player or thing dbref and returns its value.  Requires
 * MUCKER level tp_pennies_muf_mlev and remote-read permissions when
 * applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pennies(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DBREF
 *
 * Consumes an integer, and returns it as a dbref - which is not guaranteed
 * to be valid.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dbref(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONTENTS
 *
 * Consumes a dbref, and returns the first dbref in its contents chain -
 * which for MUCKER level 1 only returns controlled non-DARK objects.
 * Requires remote-read permissions when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_contents(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXITS
 *
 * Consumes a dbref, and returns the first dbref in its exits chain.
 * Requires remote-read privileges when applicable.
 *
 * Needs MUCKER level 3 to bypass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_exits(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEXT
 *
 * Consumes a dbref, and returns the next dbref in its contents/exits
 * chain - which for MUCKER level 1 only traverses controlled non-DARK exits.
 * Requires remote-read permissions when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_next(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NAME
 *
 * Consumes a dbref, and returns the associated name.  Requires remote-read
 * privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_name(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETNAME
 *
 * Consumes a dbref and a string, and sets the name associated with the
 * object.  Setting a player name requires MUCKER level 4 and the player's
 * password inside the given string (delimited by a space).  Also required
 * to change the name of objects that don't pass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setname(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PMATCH
 *
 * Consumes a string, and returns the player dbref with a name that matches
 * the ANSI-stripped string, or NOTHING if no match.  Recognizes 'me'.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pmatch(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MATCH
 *
 * Consumes a string, and returns the dbref that matches the ANSI-stripped
 * string.  Recognizes registered objects, local matches, 'me', 'here',
 * 'home', and 'nil'.  Wizard programs and users can also match dbref
 * strings and player lookups.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_match(PRIM_PROTOTYPE);

/**
 * Implementation of MUF RMATCH
 *
 * Consumes a dbref and a string, and returns the dbref that matches one of
 * the object's contents or exits.  Requires remote-read privileges when
 * applicable.  Aborts when given an exit or program dbref.
 *
 * Does not ANSI-strip the string before matching.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rmatch(PRIM_PROTOTYPE);

/**
 * Implementation of MUF COPYOBJ
 *
 * Consumes a thing dbref, copies the object's name and properties, and
 * returns the new dbref.  Requires remote-read privilges when applicable.
 *
 * Programs below MUCKER level 3 are restricted to creating one object per
 * frame.  That object must pass basic ownership checks and its copy has
 * its value reset to 0.
 *
 * Currently calls copyobj.  @see copyobj
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_copyobj(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SET
 *
 * Consumes a dbref and a string, and sets or resets the flag on the object
 * accoringly.  Reognizes flag aliases and Requires remote-read privileges
 * when applicable.
 *
 * Programs below MUCKER level 4 cannot act on:
 *    - objects that don't pass basic ownership checks
 *    - ABODE flags on programs
 *    - BUIDLER flags
 *    - DARK flags on exits with exit_darking sysparm = no
 *    - DARK flags on players
 *    - DARK flags on things with thing_darking sysparm = no
 *    - GUEST flags
 *    - INTERACTIVE, MUCKER, or SMUCKER ("nucker") internal flags
 *    - OVERT flags
 *    - QUELL flags
 *    - WIZARD flags
 *    - XFORCIBLE flags
 *    - YIELD flags
 *    - ZOMBIE flags on players
 *    - ZOMBIE flags on things if effective user is maked ZOMBIE.
 *
 * No program is able to:
 *    - reset the VEHICLE flag of a basic vehicle with polayers in it
 *    - interact with the OVERT flag on objects that aren't a thing or room
 *    - interact with the YIELD flag on objects that aren't a thing or room
 *
 *
 * "truewizard" is merely an alias for the WIZARD flag here.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_set(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MLEVEL
 *
 * Consumes a dbref, and returns its raw MUCKER level.  Objects with the
 * WIZARD flag and a MUCKER level are returned as 4.  Requires remote-read
 * privileges when applicable.
 *
 * If the given dbref is NOTHING, returns the effective MUCKER level of
 * the running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mlevel(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FLAG?
 *
 * Consumes a dbref and a string, and returns a boolean that represents if
 * the object's flag is set accordingly.  Requires remote-read priviliges
 * when applicable.
 *
 * Checking "truewizard" is the same as checking "wizard" and "!quell".
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_flagp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PLAYER?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a player.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_playerp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF THING?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a thing.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_thingp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ROOM?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a room.  Requires remote-read privilges when applicable.
 *
 * Any recycled object, NOTHING, AMBIGUOUS, or NIL returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_roomp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PROGRAM?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a program.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_programp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXIT?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is an exit/action.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_exitp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF OK?
 *
 * Consumes a stack item, and returns a boolean that represents if it is an
 * object that is not currently recycled.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_okp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LOCATION
 *
 * Consumes a dbref, and returns the dbref of its location.  Requires
 * remote-read privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_location(PRIM_PROTOTYPE);

/**
 * Implementation of MUF OWNER
 *
 * Consumes a dbref, and returns the dbref of its owner.  Requires
 * remote-read privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_owner(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONTROLS
 *
 * Consumes two dbrefs, and returns a boolean representing if the first
 * controls the second.  Requires remote-read privileges for the second
 * object when applicable.
 *
 * @see controls
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_controls(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETLINK
 *
 * Consumes a dbref, and returns the dbref of its (first) link.  Requires
 * remote-read privileges when applicable.
 *
 * Currently does not work on programs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getlink(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETLINKS
 *
 * Consumes a dbref, and returns a stackrange of dbrefs representing the
 * object's links.  Requires remote-read privileges when applicable.
 *
 * Currently does not work on programs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getlinks(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETLINK
 *
 * Consumes two dbrefs, and sets the link of the first non-program object to
 * the second object.  Cannot link if it would create an exit/parent loop, or
 * the source is unable to be linked to the target.  @see prog_can_link_to
 *
 * Needs MUCKER level 4 to bypass basic ownership checks on the source.
 *
 * A target of NOTHING unlinks the given exit or room source.  AMBIGUOUS
 * is not a valid target.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setlink(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETOWN
 *
 * Consumes two dbrefs, and sets the owner of the first non-player object
 * to the second player.
 *
 * Programs with a MUCKER level less than 4 cannot change the owner:
 * - to other players
 * - of non-chownable objects to players don't pass the chown-lock
 * - of rooms that the player is not located in
 * - of things that the player is not carrying
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setown(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEWOBJECT
 *
 * Consumes a string and a dbref, creates a thing with the given name and
 * player/room object as its home, and returns the new dbref.  Requires
 * remote-read privilges when applicable.
 *
 * Programs below MUCKER level 3 are restricted to creating one object per
 * frame.  The home object must pass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_newobject(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEWROOM
 *
 * Consumes a string and a dbref, creates a room with the given name and
 * room object as its dropto, and returns the new dbref.  Requires
 * remote-read privilges when applicable.
 *
 * Programs below MUCKER level 3 are restricted to creating one object per
 * frame.  The dropto object must pass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_newroom(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEWEXIT
 *
 * Consumes a string and a dbref, creates an exit with the given name and
 * object as its location, and returns the new dbref.
 *
 * Requires MUCKER level 3.  Level 4 is needed to bypass basic ownership
 * checks on the location object.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_newexit(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LOCKED?
 *
 * Consumes two dbrefs and returns a boolean representing if the first
 * player/thing object is locked against the second object.  Requires
 * remote-read privileges for both objects when applicable.
 *
 * @see could_do_it
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_lockedp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF RECYCLE
 *
 * Consumes a non-player dbref, and recyles it.
 *
 * Requires MUCKER level 3.  Level 4 is needed to bypass basic ownership
 * checks.
 *
 * In addition, cannot use this to recycle:
 * - the global environment
 * - objects that are referred to by sysparm values
 * - the currently-running program or those on the active call stack
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_recycle(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETLOCKSTR
 *
 * Consumes a dbref and a string, sets the lock accordingly, and returns
 * a boolean representing success or failure.  Needs MUCKER level 4 to
 * bypass basic ownership checks.
 *
 * @see setlockstr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setlockstr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETLOCKSTR
 *
 * Consumes a dbref, and returns its lock string or PROP_UNLOCKED_VAL if
 * unlocked.  Requires remote-read privileges when applicable.
 *
 * Needs MUCKER level 3 to bypass basic ownership chekcs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getlockstr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PART_PMATCH
 *
 * Consumes a string, and returns the dbref of an online player that has
 * a name that begins with the string.  Requires MUCKER level 3.
 *
 * Can return NOTHING or AMBIGUOUS.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_part_pmatch(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CHECKPASSWORD
 *
 * Consumes a player dbref and a string, and returns a boolean representing
 * if the given password is correct for that player.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_checkpassword(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEXTOWNED
 *
 * Consumes a dbref, and returns the next dbref in the database that is
 * owned by the same player.  If a player dbref is consumed, the entire
 * database is searched.  Requires MUCKER level 2.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_nextowned(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MOVEPENNIES
 *
 * Consumes two dbrefs and a non-negative integer, and moves that value from
 * the first player/thing to the second.  Requires MUCKER level
 * tp_movepennies_muf_mlev for players and 4 for things.
 *
 * Needs MUCKER level 4 to bypass range checks for players.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_movepennies(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FINDNEXT
 *
 * Consumes two dbrefs (current object and an owner object) and two strings
 * (name pattern and flag pattern), and returns the next object in the
 * database that matches both patterns and has the specified owner.
 *
 * Requires MUCKER level 2.  Needs MUCKER level 3 to search for objects
 * with an owner other than the player (including specifying NOTHING to
 * bypass the owner check.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_findnext(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETLINKS_ARRAY
 *
 * Consumes a dbref and a sequential array of dbrefs, and sets the links of
 * the single dbref to the contents of the array.  The array may contain
 * more than one item (up to MAX_LINKS) if the source is an exit.
 *
 * Needs MUCKER level 4 to bypass basic ownership checks on the source.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setlinks_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEXTENTRANCE
 *
 * Consumes two dbrefs, and returns the next object in the database after
 * the second that is linked to the first.  Requires MUCKER level 3.
 *
 * This primitive does resolve HOME to the player's home.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_nextentrance(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEWPLAYER
 *
 * Consumes two strings (name and password), and returns the dbref of
 * the new player created if successful.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_newplayer(PRIM_PROTOTYPE);

/**
 * Implementation of MUF COPYPLAYER
 *
 * Consumes a dbref and two strings (name and password), and returns the
 * dbref of new player created from the template object if successful.
 * Requires MUCKER level 4.
 *
 * The new player has the flags, properties, home, and value of the template
 * object - and is relocated to its home.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_copyplayer(PRIM_PROTOTYPE);

/**
 * Implementation of MUF OBJMEM
 *
 * Consumes a dbref, and returns the number of bytes of memory it currently
 * uses.  This works on recycled objects.
 *
 * @see size_object
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_objmem(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INSTANCES
 *
 * Consumes a program dbref, and returns the number of active processes
 * that are running its code.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_instances(PRIM_PROTOTYPE);

/**
 * Implementation of MUF COMPILED?
 *
 * Consumes a program dbref, and returns the number of compiled instructions
 * it has, or 0 if it is not compiled.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_compiledp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEWPROGRAM
 *
 * Consumes a string, creates a program with that name, and returns its
 * dbref.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_newprogram(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONTENTS_ARRAY
 *
 * Consumes a dbref, and returns an array of its contents.  Requires
 * remote-read permissions when applicable.
 *
 * Unlike prim_contents, this has no MUCKER level 1 restrictions.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_contents_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXITS_ARRAY
 *
 * Consumes a dbref, and returns an array of its exits.  Requires
 * remote-read permissions when applicable.
 *
 * Needs MUCKER level 3 to byp[ass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_exits_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETLINKS_ARRAY
 *
 * Consumes a dbref, and returns an array of its links.  Requires remote-read
 * privileges when applicable.
 *
 * Unlike prim_getlinks, does not abort on program objects.
 *
 * @see array_getlinks
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getlinks_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ENTRANCES_ARRAY
 *
 * Consumes a dbref, and returns an array of objects that link to it.
 * Requires remote-read privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_entrances_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEWPASSWORD
 *
 * Consumes a player dbref and a string, and sets the player's password.
 * Requires MUCKER level 4.
 *
 * With GOD_PRIV defined, only God can use this primitive and cannot be the
 * target of it.
 *
 * @see set_password
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_newpassword(PRIM_PROTOTYPE);

/**
 * Implementation of MUF COMPILE
 *
 * Consumes a program dbref and a boolean, compiles the program, and returns
 * the number of compiled instructions if successful, otherwise 0.  Requires
 * MUCKER level 4.
 *
 * If the boolean is true, errors and warnings are shown to the player.
 *
 * Cannot recompile a running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_compile(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DUMP
 *
 * Takes no parameters and returns boolean; true if the dump was triggered,
 * false if it was not.  A dump may not be triggered in the case where a
 * dump is already in progress.
 *
 * On completion of the dump, it generates an event with eventID DUMP
 * which can be waited for / caught as desired.  It will have the integer
 * value of 1.
 *
 * Wizards only.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dump(PRIM_PROTOTYPE);

/**
 * Implementation of MUF UNCOMPILE
 *
 * Consumes a program dbref, and uncompiles the program. Requires
 * MUCKER level 4.
 *
 * Cannot uncompile a running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_uncompile(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETPIDS
 *
 * Consumes a dbref, and returns an array of associated process IDs.
 * Requires MUCKER level 3.
 *
 * @see get_pids
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getpids(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GETPIDINFO
 *
 * Consumes an integer, and returns a dictionary with information related
 * to the process ID.
 *
 * Needs MUCKER level 3 to view informtion on another process.
 *
 * @see get_pidinfo
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_getpidinfo(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SUPPLICANT
 *
 * Consumes a player dbref, and returns an array containing the name changes
 * that have been recorded for the player.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pname_history(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PROGRAM_GETLINES
 *
 * Consumes a program dbref and two non-negative integers, and returns
 * an array of code lines within that range.
 *
 * Needs MUCKER level 4 to bypass advanced controls checks or read
 * non-VIEWABLE programs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_program_getlines(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PROGRAM_SETLINES
 *
 * Consumes a program dbref and a sequential array of strings, and sets the
 * program code lines to the contents of the array.  Requires MUCKER level 4.
 *
 * Effective user must control the program, and it must not be being edited.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_program_setlines(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SUPPLICANT
 *
 * Returns the dbref of the object being tested against a lock.  If the
 * program is no being run from a lock, returns NOTHING.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_supplicant(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TOADPLAYER
 *
 * Consumes two different player dbrefs, and toads the first.  Its possessions
 * and macros are chowned to the second player.  Requires MUCKER level 4.
 *
 * This primitive cannot be forced.
 *
 * In addition, cannot use this to toad players that are:
 * - protected (with NO_RECYCLE_PROP - usually '@/precious')
 * - wizards (or God with GOD_PRIV)
 * - referred to by sysparm values
 *
 * @see toad_player
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_toadplayer(PRIM_PROTOTYPE);

/**
 * Primitive callbacks
 */
#define PRIMS_DB_FUNCS prim_addpennies, prim_moveto, prim_pennies,     	\
    prim_dbref, prim_contents, prim_exits, prim_next, prim_name,	\
    prim_setname, prim_match, prim_rmatch, prim_copyobj, prim_set,	\
    prim_mlevel, prim_flagp, prim_playerp, prim_thingp, prim_roomp,	\
    prim_programp, prim_exitp, prim_okp, prim_location, prim_owner,	\
    prim_getlink, prim_setlink, prim_setown, prim_newobject,		\
    prim_newroom, prim_newexit, prim_lockedp, prim_recycle,             \
    prim_setlockstr, prim_getlockstr, prim_part_pmatch, prim_controls,  \
    prim_checkpassword, prim_nextowned, prim_getlinks,                  \
    prim_pmatch, prim_movepennies, prim_findnext, prim_nextentrance,    \
    prim_newplayer, prim_copyplayer, prim_objmem, prim_instances,       \
    prim_compiledp, prim_newprogram, prim_contents_array,               \
    prim_exits_array, prim_getlinks_array, prim_entrances_array,        \
    prim_compile, prim_uncompile, prim_newpassword, prim_getpids,       \
    prim_program_getlines, prim_getpidinfo, prim_program_setlines,	\
    prim_setlinks_array, prim_toadplayer, prim_supplicant,		\
    prim_pname_history, prim_dump

/**
 * Primitive callback function names - must be in same order as callbacks.
 */
#define PRIMS_DB_NAMES "ADDPENNIES", "MOVETO", "PENNIES",  \
    "DBREF", "CONTENTS", "EXITS", "NEXT", "NAME",	   \
    "SETNAME", "MATCH", "RMATCH", "COPYOBJ", "SET",	   \
    "MLEVEL", "FLAG?", "PLAYER?", "THING?", "ROOM?",	   \
    "PROGRAM?", "EXIT?", "OK?", "LOCATION", "OWNER",	   \
    "GETLINK", "SETLINK", "SETOWN", "NEWOBJECT", "NEWROOM",\
    "NEWEXIT", "LOCKED?", "RECYCLE", "SETLOCKSTR",	   \
    "GETLOCKSTR", "PART_PMATCH", "CONTROLS",		   \
    "CHECKPASSWORD", "NEXTOWNED", "GETLINKS",              \
    "PMATCH", "MOVEPENNIES", "FINDNEXT", "NEXTENTRANCE",   \
    "NEWPLAYER", "COPYPLAYER", "OBJMEM", "INSTANCES",      \
    "COMPILED?", "NEWPROGRAM", "CONTENTS_ARRAY",           \
    "EXITS_ARRAY", "GETLINKS_ARRAY", "ENTRANCES_ARRAY",    \
    "COMPILE", "UNCOMPILE", "NEWPASSWORD", "GETPIDS",      \
    "PROGRAM_GETLINES", "GETPIDINFO", "PROGRAM_SETLINES",  \
    "SETLINKS_ARRAY", "TOADPLAYER", "SUPPLICANT",	   \
    "PNAME_HISTORY", "DUMP"

#endif /* !P_DB_H */
