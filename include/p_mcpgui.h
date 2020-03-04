/** @file p_mcp.h
 *
 * Declaration of MCP GUI calls for MUF.  These are iimplemented in p_mcp.c
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#ifdef MCPGUI_SUPPORT
#ifndef P_MCPGUI_H
#define P_MCPGUI_H

#include "interp.h"

/*
 * WARNING: These functions are of dubious utility and as such not a lot of
 *          effort is being put into documenting them, as they may well be
 *          removed in a future release.
 */

/**
 * Implementation of MUF GUI_AVAILABLE
 *
 * Consumes a descriptor integer and provides a floating point number that
 * is the version of the MCP GUI protocol supported.  0.0 means no GUI
 * support.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_available(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_DLOG_CREATE
 *
 * Consumes a descriptor integer, a string type, a string title, and
 * a dictionary of arguments that won't be documented here for a function
 * that will probably be deprecated.  See the MAN page for the function if
 * you are curious about how it works.  Pushes a string dialog ID on the
 * stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_dlog_create(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_DLOG_SHOW
 *
 * Consumes a string dialog ID and pushes a message to show the given
 * dialog to the user.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_dlog_show(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_DLOG_CLOSE
 *
 * Consumes a string dialog ID and pushes a message to close the given
 * dialog to the user.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_dlog_close(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_VALUE_SET
 *
 * Takes a dialog ID, a control ID, and a value and sets that control to
 * whatever value string is provided.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_value_set(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_VALUE_GET
 *
 * Takes a dialog ID and a control ID, and pushes the current value onto
 * the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_value_get(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_VALUES_GET
 *
 * Takes a dialog ID and returns a dictionary of control IDs to set value
 * strings.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_values_get(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_CTRL_CREATE
 *
 * Consumes a string dialog ID, a string type, a string control ID, and
 * a dictionary of arguments that will not be documented here.  See
 * the MAN page for gui_ctrl_create if you are curious.  Creates a new
 * control with the given ID in the given dialog.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_ctrl_create(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GUI_CTRL_COMMAND
 *
 * This is the "generic primitive for executing a GUI control specific
 * command".  It consumes a string dialog ID, a string control ID, a
 * string command, and a dictionary of arguments that will not be
 * documented here.  See MAN gui_ctrl_command if you're curious.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gui_ctrl_command(PRIM_PROTOTYPE);


#define PRIMS_MCPGUI_FUNCS prim_gui_available, prim_gui_dlog_show, \
        prim_gui_dlog_close, prim_gui_values_get, prim_gui_value_get, \
        prim_gui_value_set, prim_gui_ctrl_create, prim_gui_ctrl_command, \
        prim_gui_dlog_create

#define PRIMS_MCPGUI_NAMES "GUI_AVAILABLE", "GUI_DLOG_SHOW", \
        "GUI_DLOG_CLOSE", "GUI_VALUES_GET", "GUI_VALUE_GET", \
        "GUI_VALUE_SET", "GUI_CTRL_CREATE", "GUI_CTRL_COMMAND", \
        "GUI_DLOG_CREATE"

#endif /* !P_MCPGUI_H */
#endif /* MCPGUI_SUPPORT */
