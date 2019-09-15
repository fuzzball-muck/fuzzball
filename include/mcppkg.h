/** @file mcppkg.h
 *
 * Header for built-in MCP packages.
 * Specification is here: https://www.moo.mud.org/mcp/
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef MCPPKG_H
#define MCPPKG_H

#include "mcp.h"

/**
 * Implementation of the simpleedit MCP command
 *
 * reference is in the format objnum.category.misc where objnum is the
 * object reference, and category can be one of the following:
 *    prop     to set a property named by misc.
 *    proplist to store a string proplist named by misc.
 *    prog     to set the program text of the given object.  Ignores misc.
 *    sysparm  to set an @tune value.  Ignores objnum.
 *    user     to return data to a muf program.
 *
 * If the category is prop, then it accepts the following types:
 *    string        to set the property to a string value.
 *    string-list   to set the property to a multi-line string value.
 *    integer       to set the property to an integer value.
 *
 * Any other values are ignored.
 *
 * @param mfr the frame
 * @param msg the message to process
 * @param ver the MCP version at play
 * @param context this is always NULL for this call
 */
void mcppkg_simpleedit(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);

/**
 * Implementation of the org-fuzzball-languages MCP package
 *
 * Currently only mentions MUF.  Uses the message 'request' to list
 * supported languages.
 *
 * @param mfr the frame
 * @param msg the mcp message
 * @param ver the version being requested
 * @param context this is always NULL for this call
 */
void mcppkg_languages(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);

/**
 * Implementation of the org-fuzzball-help MCP package
 *
 * This lets you request different help files.  'topic' is the help item
 * requested and 'type' is either mpi, help, news, or man.
 *
 * Returns 'error' message if not found.
 *
 * @param mfr the frame
 * @param msg the mcp message
 * @param ver the version being requested
 * @param context this is always NULL for this call
 */
void mcppkg_help_request(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);

/**
 * Send an MCP error message using the org-fuzzball-notify package
 *
 * Sends it to the given frame, with arguments 'topic' and 'text' sent
 * and using the -error message.
 *
 * @param mfr the frame
 * @param topic the topic to use
 * @param text the text to send
 */
void show_mcp_error(McpFrame * mfr, char *topic, char *text);

#endif /* !MCPPKG_H */
