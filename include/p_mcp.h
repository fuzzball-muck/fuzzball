/** @file p_mcp.h
 *
 * Declaration of MCP calls for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_MCP_H
#define P_MCP_H

#include "interp.h"

/**
 * Implementation of MUF MCP_REGISTER
 *
 * Consumes a string package name, a float min version, and a float max
 * version.  This is a thin wrapper around mcp_package_register
 *
 * @see mcp_package_register
 *
 * So when you run this function, it registers the CURRENT PROGRAM with
 * the given package name.  When the MCP is triggered, the current
 * program will run with the descriptor and argument array on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mcp_register(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MCP_REGISTER_EVENT
 *
 * Consumes a string package name, a float min version, and a float max
 * version.  This is a thin wrapper around mcp_package_register
 *
 * @see mcp_package_register
 *
 * Instead of instancing a new MUF call (which MCP_REGISTER does), this
 * uses MUF's event system instead.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mcp_register_event(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MCP_BIND
 *
 * Consumes a string package name, a string message name, and a callback
 * function reference from the stack.  This registers a function to
 * service a particular package/message combination.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mcp_bind(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MCP_SUPPORTS
 *
 * Consumes a descriptor and a string package name, and pushes a float
 * version number on the stack or 0.0 if the package version is not
 * supported.  This is to check versions/support of negotiated packages
 * for a given descriptor.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mcp_supports(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MCP_SEND
 *
 * Consumes a descriptor integer, a string package name, a string message name,
 * and a dictionary mapping string keys to values which are either strings,
 * floats, dbrefs, integers, or array lists consisting only of strings.
 *
 * This "sends" an MCP message which is to say it displays a formatted string
 * with the correct structure and prefixing.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_mcp_send(PRIM_PROTOTYPE);

#define PRIMS_MCP_FUNCS prim_mcp_register, prim_mcp_bind, prim_mcp_supports, \
        prim_mcp_send, prim_mcp_register_event

#define PRIMS_MCP_NAMES "MCP_REGISTER", "MCP_BIND", "MCP_SUPPORTS", \
        "MCP_SEND", "MCP_REGISTER_EVENT"

#endif /* !P_MCP_H */
