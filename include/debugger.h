/** @file debugger.h
 *
 * This is the header file that declares the different functions that
 * support the MUF debugger.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "config.h"

#include "inst.h"
#include "interp.h"

/**
 * List program lines to a player
 *
 * This is used by the debugger for listing lines and for backtraces.
 * Uses the editor's list_program under the hood.
 *
 * @see list_program
 *
 * @param player the player to send the listing to
 * @param program the program to list
 * @param fr the running program frame
 * @param start the start line
 * @param end the end line
 */
void list_proglines(dbref player, dbref program, struct frame *fr, int start, int end);

/**
 * Displays a MUF Backtrace message.
 *
 * This is used when a MUF program is aborted, if someone who has 'controls'
 * permissions was running it, or if you type 'where' in the debugger.
 * Shows a fairly straight forward backtrace, going 'count' deep.  If
 * 'count' is 0, it will display up to STACK_SIZE depth.
 *
 * @param player the player getting the backtrace
 * @param program the program being backtraced
 * @param count the depth of the trace, or 0 for STACK_SIZE
 * @param fr the frame pointer
 */
void muf_backtrace(dbref player, dbref program, int count, struct frame *fr);

/**
 * Implementation of the MUF debugger
 *
 * This implements the command parsing for the MUF debugger.  It also clears
 * temporary bookmarks if this was triggered from a temporary one.
 *
 * This relies on some static globals, so it is not threadsafe.  If the
 * 'prim' debugger command is ever used to trigger the MUF debugger somehow
 * in a recursive fashion, and then you call 'prim' again, it will probably
 * cause havock.
 *
 * @param descr the descriptor of the debugging player
 * @param player the debugging player
 * @param program the program we are debugging
 * @param text the input text from the user
 * @param fr the current frame pointer
 * @return boolean true if the program should exit, false if not
 */
int muf_debugger(int descr, dbref player, dbref program, const char *text, struct frame *fr);

/**
 * This is for showing line listings in "instruction" format.
 *
 * This returns a single line of code's primitives.  It uses a static buffer,
 * so be careful with it.  Not multi-threaded.
 *
 * @param program the program to show listings for
 * @param pc the program counter where the listing starts
 * @param maxprims the maximum primitives
 * @param markpc do a special mark when listing the 'pc' line
 * @return pointer to static buffer containing primitives
 */
char *show_line_prims(dbref program, struct inst *pc, int maxprims, int markpc);

#endif /* !DEBUGGER_H */
