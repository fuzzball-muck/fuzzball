/** @file fbtime.h
 *
 * Header for fuzzball's time handling system, providing a number of time
 * helpers.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef FBTIME_H
#define FBTIME_H

#include <time.h>

#include "config.h"

/**
 * Get the machine's offset from Greenwich Mean Time in seconds.
 *
 * @return the machine's offset from Greenwich Mean Time in seconds.
 */
long get_tz_offset(void);

/**
 * Add x milliseconds to timeval 't' and return an updated struct timeval
 *
 * @param t the timeval with the starting time value
 * @param x the number of milliseconds to add
 * @return an updated struct timeval
 */
struct timeval msec_add(struct timeval t, int x);

/**
 * Calculate the millisecond difference between two struct timevals.
 *
 * @param now the more future timestruct
 * @param then the more past timestruct
 * @return the millisecond difference of 'now - then'
 */
int msec_diff(struct timeval now, struct timeval then);

/**
 * Creates a time string from a UNIX timestamp delta in "format 1" layout.
 *
 * Calculates the delta between the given time and GMT time.  Then returns
 * a string in the format "DAYSd HH:MM" where Days is the number of days
 * between the two points with a 'd' suffix.
 *
 * If the delta is less than a day, then just the format "HH:MM" is used.
 *
 * This is used by WHO's "on for" listing.
 *
 * Uses a static buffer, so this is not threadsafe -- memory must be
 * copied for multiple calls, etc.
 *
 * @param dt the UNIX timestamp to generate a time string for
 * @return a timestamp string as described above, residing in a static buffer
 */
char *time_format_1(time_t dt);

/**
 * Creates a time string from a UNIX timestamp delta in "format 2" layout.
 *
 * It computes the delta between the given timestamp and GMT time, then
 * goes through a logic process to figure out what "resolution" of 
 * the timestamp should be.  If the delta is greater than a day, then
 * days is used.  If greater than an hour, then hours is used.  If greater
 * than a minute, then minutes are used.  Otherwise, seconds.
 *
 * The time is suffixed with a "d", "h", "m" or "s" as appropriate.
 *
 * Uses a static buffer, so this is not threadsafe -- memory must be
 * copied for multiple calls, etc.
 *
 * @param dt the UNIX timestamp to generate a time string for
 * @return a timestamp string as described above, residing in a static buffer
 */
char *time_format_2(time_t dt);

/**
 * Generate a "full" time string from a count of seconds
 *
 * The full time string is 'Dd H:MM:SS' in format.  'D' is the day count,
 * suffixed with a lower case d, up to 3 digits.  Hour may be 1 or 2 digits,
 * and MM and SS will both be two digits.
 *
 * 'dtime' should be a count of seconds, not a raw unix timestamp.
 * A timestamp would give you some ridiculous result.  This is for taking
 * something like a number of idle seconds and turning it into a time string.
 *
 * A static buffer is used, so this is not thread safe and the usual
 * precautions should be used as this will mutate over multiple calls.
 *
 * @param dtime the number of seconds to convert into a time string
 * @return the time string as described above, in a static buffer
 */
const char *timestr_full(long dtime);

/**
 * Generate a "full" time string from a count of seconds
 *
 * The full time string is dynamically constructed using long form
 * words such as "year", "month", "week", "day", "hour", "minute", and "second".
 *
 * The most readily visible place this is used is the 'uptime' command.
 * The produced string looks something like this:
 *
 * 3 days, 13 hours, 7 minutes, 12 seconds
 *
 * It correctly plurals, so "1 day" vs "2 days" is right.
 *
 * 'dtime' should be a count of seconds, not a raw unix timestamp.
 * A timestamp would give you some ridiculous result.  This is for taking
 * something like a number of idle seconds and turning it into a time string.
 *
 * A static buffer is used, so this is not thread safe and the usual
 * precautions should be used as this will mutate over multiple calls.
 *
 * @param dtime the number of seconds to convert into a time string
 * @return the time string as described above, in a static buffer
 */
const char *timestr_long(long dtime);

/**
 * Subtract one struct timeval from another.
 *
 * @param now the timeval to subtract from
 * @param then the timeval the value to subtract from 'now'
 * @return a timeval that is now - then
 */
struct timeval timeval_sub(struct timeval now, struct timeval then);

/**
 * This updates an object's last use count timestamp but not its use count.
 *
 * Room parent rooms will be updated as well, but not their use count.
 * Cause sometimes its fun to just update the timestamp and not the
 * use count.  Seems a little arbitrary which used where.
 *
 * @param thing the object to update
 */
void ts_lastuseobject(dbref thing);

/**
 * Set the modified time to now for the given object.
 *
 * @param thing the object to update
 */
void ts_modifyobject(dbref thing);

/**
 * Set initial timestamps and use count for a new object.
 *
 * @param thing the dbref of the thing to initialize
 */
void ts_newobject(dbref thing);

/**
 * Update timestamp and usecount after an object is 'used'
 *
 * Room parent rooms will be 'used' if their child rooms are 'used'.
 *
 * @param thing the dbref of the thing to touch
 */
void ts_useobject(dbref thing);

/**
 * Convert time string with given format to a number of seconds.
 *
 * @param string the string to convert
 * @param format the string's format
 * @parm[out] error the error message, if any
 * @return the time in seconds
 */
#ifndef WIN32
time_t time_string_to_seconds(char *string, char *format, char **error);
#endif
#endif /* !FBTIME_H */
