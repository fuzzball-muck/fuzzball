/** @file p_connects.h
 *
 * Header for MUF connection primitives - enabling query of timing
 * and machine information, translation between dbrefs and
 * connection/descriptor numbers, and output processing.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_CONNECTS_H
#define P_CONNECTS_H

#include "interp.h"

/**
 * Implementation of MUF AWAKE?
 *
 * Consumes a dbref, and returns the number of connections the player
 * (or the zombie's owner) has to the server.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_awakep(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ONLINE
 *
 * Returns a stackrange of dbrefs representing the players associated with
 * each connection to the server.
 *
 * @see pcount
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_online(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ONLINE_ARRAY
 *
 * Returns an array of dbrefs representing the players associated with
 * each connection to the server.
 *
 * @see pcount
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_online_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRCOUNT
 *
 * Returns the number of connections to the server.
 *
 * @see pdescrcount
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descrcount(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NEXTDESCR
 *
 * Consumes a descriptor number and returns the next descriptor number that
 * is connected to the game beyond the welcome screen. This value will be 0
 * if there are no more descriptors or the input is invalid.
 *
 * @see pnextdescr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_nextdescr(PRIM_PROTOTYPE);
/**
 * Implementation of MUF DESCRIPTORS
 *
 * Consumes a dbref and returns a stackrange of descriptor numbers associated
 * with the player.  If the dbref is NOTHING, all players' descriptors are
 * included.
 *
 * @see pcount
 * @see pdescr
 * @see get_player_descrs
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descriptors(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCR_ARRAY
 *
 * Consumes a dbref and returns an array of descriptor numbers associated
 * with the player.  If the dbref is NOTHING, all players' descriptors are
 * included.
 *
 * @see pcount
 * @see pdescr
 * @see get_player_descrs
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCR_SETUSER
 *
 * Consumes a dbref and returns an array of descriptor numbers associated
 * with the player.  If the dbref is NOTHING, all players' descriptors are
 * included.
 *
 * @see pset_user
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_setuser(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRFLUSH
 *
 * Consumes a descriptor number and processes its output text.  If the input
 * is -1, output text for all descriptors is flushed.
 *
 * @see pdescrflush
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descrflush(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCR
 *
 * Returns the descriptor number that started the program. This will be -1
 * if it was invoked during a listener event or when the MUCK started up.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FIRSTDESCR
 *
 * Consumes a dbref and returns the first descriptor number associated with
 * that player, or all players if the input is NOTHING. The return value will
 * be 0 if there are no descriptors available in the context.
 *
 * @see pfirstdescr
 * @see get_player_descrs
 * @see index_descr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_firstdescr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF LASTDESCR
 *
 * Consumes a dbref and returns the last descriptor number associated with
 * that player, or all players if the input is NOTHING. The return value will
 * be 0 if there are no descriptors available in the context.
 *
 * @see plastdescr
 * @see get_player_descrs
 * @see index_descr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_lastdescr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRTIME
 *
 * Consumes a descriptor number and returns how many seconds it has been
 * connected to the server.
 *
 * @see pontime
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_time(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRBOOT
 *
 * Consumes a descriptor number and disconnects it from the server.
 *
 * @see pdescrboot
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_boot(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRHOST
 *
 * Consumes a descriptor number and returns the hostname or IP associated with
 * the connection.
 *
 * @see pdescrhost
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_host(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRUSER
 *
 * Consumes a descriptor number and returns the username associated with
 * the connection.
 *
 * @see pdescruser
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_user(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRIDLE
 *
 * Consumes a descriptor number and returns how many seconds it has been idle.
 * 
 * @see pdescridle
 * 
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_idle(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRLEASTIDLE
 *
 * Consumes a dbref and returns the least idle descriptor number, or -1 if
 * that cannot be determined.
 *
 * @see least_idle_player_descr
 * 
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_notify(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRDBREF
 *
 * Consumes a dbref and returns the most idle descriptor number, or -1 if
 * that cannot be determined.
 * 
 * @see pdescrdbref
 * 
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_dbref(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRLEASTIDLE
 *
 * Consumes a dbref and returns the least idle descriptor number.
 *
 * @see least_idle_player_descr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_least_idle(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRMOSTIDLE
 *
 * Consumes a descriptor number and returns how many seconds it has been idle.
 *
 * @see most_idle_player_descr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_most_idle(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRSECURE?
 *
 * Consumes a descriptor number and returns 1 if it is secure, otherwise 0.
 *
 * @see pdescrsecure
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_securep(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRBUFSIZE 
 *
 * Consumes a descriptor number and returns the number of bytes available.
 *
 * @see pdescrbufsize
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descr_bufsize(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETWIDTH
 *
 * Consumes a descriptor number and screensize.  Returns nothing.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setwidth(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETHEIGHT
 *
 * Consumes a descriptor number and screensize.  Returns nothing.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setheight(PRIM_PROTOTYPE);

/**
 * Implementation of MUF WIDTH
 *
 * Consumes a descriptor.  Puts the detected width on the stack.  This width
 * maybe 0 if it is unknown.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_width(PRIM_PROTOTYPE);

/**
 * Implementation of MUF HEIGHT
 *
 * Consumes a descriptor.  Puts the detected height on the stack.  This height
 * maybe 0 if it is unknown.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_height(PRIM_PROTOTYPE);


/**
 * Primitive callback functions
 */
#define PRIMS_CONNECTS_FUNCS prim_awakep, prim_online, prim_descrcount,    \
    prim_nextdescr, prim_descriptors, prim_descr_setuser, prim_descrflush, \
    prim_descr, prim_online_array, prim_descr_array, prim_firstdescr,      \
    prim_lastdescr, prim_descr_time, prim_descr_host, prim_descr_user,     \
    prim_descr_boot, prim_descr_idle, prim_descr_notify, prim_descr_dbref, \
    prim_descr_least_idle, prim_descr_most_idle, prim_descr_securep,       \
    prim_descr_bufsize, prim_setwidth, prim_setheight, prim_width, prim_height

/**
 * Primitive names - must be in same order as the callback functions
 */
#define PRIMS_CONNECTS_NAMES "AWAKE?", "ONLINE", "DESCRCOUNT",\
    "NEXTDESCR", "DESCRIPTORS", "DESCR_SETUSER", "DESCRFLUSH",\
    "DESCR", "ONLINE_ARRAY", "DESCR_ARRAY", "FIRSTDESCR",     \
    "LASTDESCR", "DESCRTIME", "DESCRHOST", "DESCRUSER",       \
    "DESCRBOOT", "DESCRIDLE", "DESCRNOTIFY", "DESCRDBREF",    \
    "DESCRLEASTIDLE", "DESCRMOSTIDLE", "DESCRSECURE?",        \
    "DESCRBUFSIZE", "SETWIDTH", "SETHEIGHT", "WIDTH", "HEIGHT"

#endif /* !P_CONNECTS_H */
