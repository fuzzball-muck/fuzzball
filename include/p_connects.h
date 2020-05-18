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
 * @see pdbref
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
 * @see pdbref
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
 * Implementation of MUF CONCOUNT
 *
 * Returns the number of connections to the server.
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
void prim_concount(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONDBREF
 *
 * Consumes a connection number and returns the associated player dbref.
 * This will be NOTHING if no player is connected using that number.
 *
 * @see pdbref
 * 
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_condbref(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONIDLE
 *
 * Consumes a connection number and returns how many seconds it has been idle.
 *
 * @see pidle
 * 
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_conidle(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONTIME
 *
 * Consumes a connection number and returns how many seconds it has been
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
void prim_contime(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONHOST
 *
 * Consumes a connection number and returns the hostname or IP associated with
 * the connection.
 *
 * @see phost
 * 
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_conhost(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONUSER
 *
 * Consumes a connection number and returns the username associated with
 * the connection.
 *
 * @see puser
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_conuser(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONBOOT
 *
 * Consumes a connection number and disconnects it from the server.
 *
 * @see pboot
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_conboot(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONNOTIFY
 *
 * Consumes a connection number and a string, and sends the string over
 * the connection.
 *
 * @see pnotify
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_connotify(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONDESCR
 * 
 * Consumes a connection number and returns the associated descriptor number.
 *
 * @see pdescr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_condescr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRCON
 *
 * Consumes a descriptor number and returns the associated connection number.
 *
 * @see pdescrcon
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_descrcon(PRIM_PROTOTYPE);

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
void prim_descr_notify(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DESCRDBREF
 *
 * Consumes a descriptor number and returns the associated player dbref.
 * This will be NOTHING if no player is connected using that descriptor.
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
 * Primitive callback functions
 */
#define PRIMS_CONNECTS_FUNCS prim_awakep, prim_online, prim_concount,      \
    prim_condbref, prim_conidle, prim_contime, prim_conhost, prim_conuser, \
    prim_conboot, prim_connotify, prim_condescr, prim_descrcon,            \
    prim_nextdescr, prim_descriptors, prim_descr_setuser, prim_descrflush, \
    prim_descr, prim_online_array, prim_descr_array, prim_firstdescr,      \
    prim_lastdescr, prim_descr_time, prim_descr_host, prim_descr_user,     \
    prim_descr_boot, prim_descr_idle, prim_descr_notify, prim_descr_dbref, \
    prim_descr_least_idle, prim_descr_most_idle, prim_descr_securep,       \
    prim_descr_bufsize

/**
 * Primitive names - must be in same order as the callback functions
 */
#define PRIMS_CONNECTS_NAMES "AWAKE?", "ONLINE", "CONCOUNT",  \
    "CONDBREF", "CONIDLE", "CONTIME", "CONHOST", "CONUSER",   \
    "CONBOOT", "CONNOTIFY", "CONDESCR", "DESCRCON",           \
    "NEXTDESCR", "DESCRIPTORS", "DESCR_SETUSER", "DESCRFLUSH",\
    "DESCR", "ONLINE_ARRAY", "DESCR_ARRAY", "FIRSTDESCR",     \
    "LASTDESCR", "DESCRTIME", "DESCRHOST", "DESCRUSER",       \
    "DESCRBOOT", "DESCRIDLE", "DESCRNOTIFY", "DESCRDBREF",    \
    "DESCRLEASTIDLE", "DESCRMOSTIDLE", "DESCRSECURE?",        \
    "DESCRBUFSIZE"

#endif /* !P_CONNECTS_H */
