/** @file log.c
 *
 * Implementation of the MUCK's logging system.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "db.h"
#include "fbtime.h"
#include "fbstrings.h"
#include "inst.h"
#include "log.h"
#include "tune.h"

/**
 * sprintf-style variable argument function for logging to a given file name
 *
 * Used as the underpinning of other log calls.  The file written to will be
 * opened in binary/append mode, then closed after the write.
 *
 * Prints to standard error if the file cannot be opened.
 *
 * @private
 * @param prepend_time boolean, if true, prepend a timestamp to the log string
 * @param filename the filename to write to
 * @param format the log string which can have sprinf style replacements
 * @param args variable number of arguments, which are sprintf replacements
 */
static void
vlog2file(int prepend_time, const char *filename, char *format, va_list args)
{
    FILE *fp;
    time_t lt;
    char buf[40];
    lt = time(NULL);
    *buf = '\0';

    if ((fp = fopen(filename, "ab")) == NULL) {
        fprintf(stderr, "Unable to open %s!\n", filename);

        if (prepend_time)
            strftime(buf, 32, "%Y-%m-%dT%H:%M:%S", localtime(&lt));

        fprintf(fp, "%.32s: ", buf);
        vfprintf(stderr, format, args);
    } else {
        if (prepend_time) {
            strftime(buf, 32, "%Y-%m-%dT%H:%M:%S", localtime(&lt));
            fprintf(fp, "%.32s: ", buf);
        }

        vfprintf(fp, format, args);
        fprintf(fp, "\n");

        fclose(fp);
    }
}

/**
 * Log a string to a file, with sprinf-style replacements
 *
 * The string is raw, no timestamp is prefixed.
 *
 * @param filename the file to write to
 * @param format the format string with sprinf-style replacements
 * @param ... whatever sprintf replacement variables
 */
void
log2file(const char *filename, char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog2file(0, filename, format, args);
    va_end(args);
}

/**
 * This define provides a simple wrapper used by most of the log_ calls
 *
 * It has the prefix turned on, and is designed to be the body of a function
 * with start/end braces and everything.  It assumes the calling signature
 * looks like this:
 *
 * function_name(char* format, ...)
 *
 * The 'format' will be the string to write to file.
 *
 * @private
 * @param FILENAME the file to log to
 */
#define log_function(FILENAME) \
{ \
    va_list args; \
    va_start(args, format); \
    vlog2file(1, FILENAME, format, args); \
    va_end(args); \
}

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
void
log_sanity(char *format, ...)
log_function(tp_file_log_sanity)

/**
 * Log to the sanity fix file
 *
 * The sanity fix file is configured with tp_file_log_sanfix.
 *
 * This attaches a timestamp to 'format' and does sprintf-style replacements
 * with a variable argument list.
 *
 * @param format the log line to submit
 * @param ... the arguments
 */
void
log_sanfix(char *format, ...)
log_function(tp_file_log_sanfix)

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
void
log_status(char *format, ...)
log_function(tp_file_log_status)

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
void
log_muf(char *format, ...)
log_function(tp_file_log_muf_errors)

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
void
log_gripe(char *format, ...)
log_function(tp_file_log_gripes)

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
void
log_command(char *format, ...)
log_function(tp_file_log_commands)

/**
 * Colorfully named method to strip characters inappropriate for logging
 *
 * This strips "high" value ASCII characters and non-printable characters.
 *
 * @TODO We have several functions that do more or less the same thing, just
 *       with minor variations (like the strip ansi functions).  Can we
 *       consolidate them?
 *
 * @private
 * @param badstring the string to filter.  Modifications are made in-place.
 */
static void
strip_evil_characters(char *badstring)
{
    unsigned char *s = (unsigned char *) badstring;

    if (!s)
        return;

    for (; *s; ++s) {
        *s &= '\177'; /* No high ascii */

        if (*s == '\033')
            *s = '['; /* No escape (Aieeee!) */

        if (!isprint(*s))
            *s = '_';
    }

    return;
}

/**
 * Log something to the user log file used by USERLOG primitive
 *
 * The message is formatted thusly:
 *
 * Winged(#4023) [newaction.muf(#666)] 2000-01-01T03:09:31: <string>
 *
 * Messages are stripped of all non-printable characters.  The file this
 * logs to is controlled by tp_file_log_user
 *
 * @param player the player triggering the log message
 * @param program the program triggering the log message
 * @param logmessage the message to log
 */
void
log_user(dbref player, dbref program, char *logmessage)
{
    char logformat[BUFFER_LEN];
    char buf[40];
    time_t lt = 0;
    int len = 0;

    *buf = '\0';
    *logformat = '\0';

    lt = time(NULL);
    strftime(buf, 32, "%Y-%m-%dT%H:%M:%S", localtime(&lt));

    snprintf(logformat, BUFFER_LEN, "%s: %s(#%d) [%s(#%d)]: ",
             buf, NAME(player), player, NAME(program), program);

    len = BUFFER_LEN - strlen(logformat) - 1;
    strncat(logformat, logmessage, len);
    strip_evil_characters(logformat);
    log2file(tp_file_log_user, "%s", logformat);
}

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
void
log_program_text(struct line *first, dbref player, dbref i)
{
    FILE *f;
    char fname[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    char tbuf[24];
    time_t lt = time(NULL);

    strcpyn(fname, sizeof(fname), tp_file_log_programs);
    f = fopen(fname, "ab");
    if (!f) {
        log_status("Couldn't open file %s!", fname);
        return;
    }

    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", localtime(&lt));
    fputs("#######################################", f);
    fputs("#######################################\n", f);
    unparse_object(player, i, unparse_buf, sizeof(unparse_buf));
    fprintf(f, "%s: %s SAVED BY %s(#%d)\n",
            tbuf, unparse_buf, NAME(player), player);
    fputs("#######################################", f);
    fputs("#######################################\n\n", f);

    while (first) {
        if (!first->this_line)
            continue;

        fputs(first->this_line, f);
        fputc('\n', f);
        first = first->next;
    }

    fputs("\n\n\n", f);
    fclose(f);
}

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
char *
whowhere(dbref who)
{
    char buf[BUFFER_LEN];

    snprintf(buf, sizeof(buf), "%s%s%s%s(#%d) in %s(#%d)",
             Wizard(OWNER(who)) ? "WIZ: " : "",
             (Typeof(who) != TYPE_PLAYER) ? NAME(who) : "",
             (Typeof(who) != TYPE_PLAYER) ? " owned by " : "",
             NAME(OWNER(who)), who, NAME(LOCATION(who)), LOCATION(who));

    return strdup(buf);
}
