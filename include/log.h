/** @file log.h
 *
 * Header that supports the MUCK's logging system.  Anything that wants to
 * log (which is pretty much everything) will include this.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef LOG_H
#define LOG_H

#include "config.h"
#include "inst.h"

/**
 * Log a string to a file, with sprinf-style replacements
 *
 * @param filename the file to write to
 * @param format the format string with sprinf-style replacements
 * @param ... whatever sprintf replacement variables
 */
void log2file(const char *myfilename, char *format, ...);

/**
 * Log to the commands file
 *
 * The commands file is configured with tp_file_log_commands.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void log_command(char *format, ...);

/**
 * Log to the gripes file
 *
 * The gripes file is configured with tp_file_log_gripes.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void log_gripe(char *format, ...);

/**
 * Log to the muf errors file
 *
 * The muf errors file is configured with tp_file_log_muf_errors.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void log_muf(char *format, ...);

/**
 * Log a program's text to the program log file
 *
 * The program log file is controlled by the tp_file_log_programs variable.
 * All the text is dumped with a header that notes who saved the program
 * file.  This way, naughty programmers can't write naughty programs then
 * delete them to hide from the admins.
 *
 * @param first the first 'struct line' of a program's text
 * @param player the player who saved the MUF program text
 * @param i the program dbref
 */
void log_program_text(struct line *first, dbref player, dbref i);

/**
 * Log to the sanity fix file
 *
 * The sanity file is configured with tp_file_log_sanfix.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void log_sanfix(char *format, ...);

/**
 * Log to the sanity file
 *
 * The sanity file is configured with tp_file_log_sanity.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void log_sanity(char *format, ...);

/**
 * Log to the status file
 *
 * The status file is configured with tp_file_log_status.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void log_status(char *format, ...);

/**
 * Log something to the user log file used by USERLOG primitive
 *
 * The message is formatted thusly:
 *
 * Winged(#4023) [newaction.muf(#666)] 2000-01-01T03:09:31: <string>
 *
 * Messages are stripped of all non-printable characters.
 *
 * @param player the player triggering the log message
 * @param program the program triggering the log message
 * @param logmessage the message to log
 */
void log_user(dbref player, dbref program, char *logmessage);

/**
 * From a given 'who', generate a standard log 'name' entry.
 *
 * This is used in a number of places, such as command logging, where
 * we want a user's name, location, and if they are a wizard.
 *
 * The string produced will look something like this.  Items in {curly}
 * braces would be empty if they don't apply:
 *
 * {WIZ}{Name owned by <-- if not player}Name of who's owner(#who) in
 * location name(#location ref)
 *
 * For instance:
 *
 * WIZ Tanabi(#1234) in Some Place(#4321)
 *
 * or
 *
 * Bleh owned by Tanabi (#1234) in Some Place(#4321)
 *
 * You should free the memory returned by this call as you 'own' it.
 *
 * @param who the person to generate a log entry string for.
 * @return a newly allocated string as described above
 */
char *whowhere(dbref player);

#endif /* !LOG_H */
