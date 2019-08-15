/** @file fbtime.c
 *
 * Implenentation of fuzzball's time handling system, providing a number of
 * time helpers.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

/* To suppress compiler warning re strptime */
#define _XOPEN_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "config.h"

#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"

/**
 * Set initial timestamps and use count for a new object.
 *
 * @param thing the dbref of the thing to initialize
 */
void
ts_newobject(dbref thing)
{
    time_t now = time(NULL);

    DBFETCH(thing)->ts_created = now;
    DBFETCH(thing)->ts_modified = now;
    DBFETCH(thing)->ts_lastused = now;
    DBFETCH(thing)->ts_usecount = 0;
}

/**
 * Update timestamp and usecount after an object is 'used'
 *
 * Room parent rooms will be 'used' if their child rooms are 'used'.
 *
 * @param thing the dbref of the thing to touch
 */
void
ts_useobject(dbref thing)
{
    if (thing == NOTHING)
        return;

    DBFETCH(thing)->ts_lastused = time(NULL);
    DBFETCH(thing)->ts_usecount++;
    DBDIRTY(thing);

    if (Typeof(thing) == TYPE_ROOM)
        ts_useobject(LOCATION(thing));
}

/**
 * This updates an object's last use count timestamp but not its use count.
 *
 * Room parent rooms will be updated as well, but not their use count.
 * Cause sometimes its fun to just update the timestamp and not the
 * use count.  Seems a little arbitrary which used where.
 *
 * @param thing the object to update
 */
void
ts_lastuseobject(dbref thing)
{
    if (thing == NOTHING)
        return;

    DBSTORE(thing, ts_lastused, time(NULL));

    if (Typeof(thing) == TYPE_ROOM)
        ts_lastuseobject(LOCATION(thing));
}

/**
 * Set the modified time to now for the given object.
 *
 * @param thing the object to update
 */
void
ts_modifyobject(dbref thing)
{
    DBSTORE(thing, ts_modified, time(NULL));
}

/**
 * Get the machine's offset from Greenwich Mean Time in seconds.
 *
 * @return the machine's offset from Greenwich Mean Time in seconds.
 */
long
get_tz_offset(void)
{
    time_t now;

    time(&now);

    if (localtime(&now)->tm_wday == gmtime(&now)->tm_wday)
        return (localtime(&now)->tm_hour - gmtime(&now)->tm_hour)
               * 3600
               + (localtime(&now)->tm_min - gmtime(&now)->tm_min)
               * 60
               + localtime(&now)->tm_sec
               - gmtime(&now)->tm_sec;

    if ((localtime(&now)->tm_wday - gmtime(&now)->tm_wday == -1) || (localtime(&now)->tm_wday - gmtime(&now)->tm_wday == 1))
        return (localtime(&now)->tm_wday - gmtime(&now)->tm_wday)
               * 86400
               + (localtime(&now)->tm_hour - gmtime(&now)->tm_hour)
               * 3600
               + (localtime(&now)->tm_min - gmtime(&now)->tm_min)
               * 60
               + localtime(&now)->tm_sec
               - gmtime(&now)->tm_sec;

    if ((localtime(&now)->tm_wday - gmtime(&now)->tm_wday == 6) || (localtime(&now)->tm_wday - gmtime(&now)->tm_wday == -6))
        return (localtime(&now)->tm_wday - gmtime(&now)->tm_wday)
               * -14400
               + (localtime(&now)->tm_hour - gmtime(&now)->tm_hour)
               * 3600
               + (localtime(&now)->tm_min - gmtime(&now)->tm_min)
               * 60
               + localtime(&now)->tm_sec
               - gmtime(&now)->tm_sec;

    return 0;
}

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
char *
time_format_1(time_t dt)
{
    register struct tm *delta;
    static char buf[64];

    delta = gmtime((time_t *) & dt);
    if (delta->tm_yday > 0)
        snprintf(buf, sizeof(buf), "%dd %02d:%02d", delta->tm_yday, delta->tm_hour,
                 delta->tm_min);
    else
        snprintf(buf, sizeof(buf), "%02d:%02d", delta->tm_hour, delta->tm_min);
    return buf;
}

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
char *
time_format_2(time_t dt)
{
    register struct tm *delta;
    static char buf[64];

    delta = gmtime((time_t *) & dt);
    if (delta->tm_yday > 0)
        snprintf(buf, sizeof(buf), "%dd", delta->tm_yday);
    else if (delta->tm_hour > 0)
        snprintf(buf, sizeof(buf), "%dh", delta->tm_hour);
    else if (delta->tm_min > 0)
        snprintf(buf, sizeof(buf), "%dm", delta->tm_min);
    else
        snprintf(buf, sizeof(buf), "%ds", delta->tm_sec);
    return buf;
}

/**
 * Calculate the millisecond difference between two struct timevals.
 *
 * @param now the more future timestruct
 * @param then the more past timestruct
 * @return the millisecond difference of 'now - then'
 */
int
msec_diff(struct timeval now, struct timeval then)
{
    return ((now.tv_sec - then.tv_sec)
            * 1000
            + (now.tv_usec - then.tv_usec) / 1000);
}

/**
 * Add x milliseconds to timeval 't' and return an updated struct timeval
 *
 * @param t the timeval with the starting time value
 * @param x the number of milliseconds to add
 * @return an updated struct timeval
 */
struct timeval
msec_add(struct timeval t, int x)
{
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;

    if (t.tv_usec >= 1000000) {
        t.tv_sec += t.tv_usec / 1000000;
        t.tv_usec = t.tv_usec % 1000000;
    }

    return t;
}

/**
 * Subtract one struct timeval from another.
 *
 * @param now the timeval to subtract from
 * @param then the timeval the value to subtract from 'now'
 * @return a timeval that is now - then
 */
struct timeval
timeval_sub(struct timeval now, struct timeval then)
{
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;

    if (now.tv_usec < 0) {
        now.tv_usec += 1000000;
        now.tv_sec--;
    }

    return now;
}

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
const char *
timestr_full(long dtime)
{
    static char buf[32];
    int days, hours, minutes, seconds;

    days = dtime / 86400;
    dtime %= 86400;
    hours = dtime / 3600;
    dtime %= 3600;
    minutes = dtime / 60;
    seconds = dtime % 60;

    snprintf(buf, sizeof(buf), "%3dd %2d:%02d:%02d", days, hours, minutes, seconds);

    return buf;
}

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
const char *
timestr_long(long dtime)
{
    static char buf[100], value[100];
    const char *str[7] = { "year", "month", "week", "day", "hour", "minute", "second" };
    int div[7] = { 31556736, 2621376, 604800, 86400, 3600, 60, 1 };

    *buf = *value = 0;

    for (int i = 0; i < 7; i++) {
        if (dtime >= div[i]) {
            long temp = dtime / div[i];
            snprintf(value, sizeof(value), "%s%s%ld %s%s", buf, (*buf ? ", " : ""), temp, str[i], temp != 1 ? "s" : "");
            strncpy(buf, value, sizeof(buf));
            dtime %= div[i];
        }
    }

    return buf;
}

/**
 * Convert time string with given format to a number of seconds.
 *
 * If the 'format' string is "%T%t%D" then we will do some hackery to force
 * %D to handle 4 digit or 2 digit years.
 *
 * @param string the string to convert
 * @param format the string's format
 * @parm[out] error the error message, if any
 * @return the time in seconds
 */
#ifndef WIN32
time_t
time_string_to_seconds(char *string, char *format, char **error)
{
    struct tm otm;

    if (!strcmp(format, "%T%t%D")) {
        char *finder;
        int  year_digits = 0;

        /* Find the space block */
        for (finder = string; *finder != '\0' && !isspace(*finder); finder++);

        /* If it's null, this is an invalid date. */
        if (*finder == '\0') {
            goto timeformat_err;
        }

        /* Skip spaces */
        for ( ; *finder != '\0' && isspace(*finder); finder++);

        if (*finder == '\0') {
            goto timeformat_err;
        }

        /* Find year */
        for ( ; *finder != '\0' && *finder != '/'; finder++); /* month */

        if (*finder == '\0') {
            goto timeformat_err;
        }

        finder++; /* Advance it off the / */

        for ( ; *finder != '\0' && *finder != '/'; finder++); /* day */

        if (*finder == '\0') {
            goto timeformat_err;
        }

        finder++; /* Advance it off the / */

        for ( ; *finder != '\0' && isdigit(*finder); finder++, year_digits++);

        if(year_digits == 4) {
            /* do 4 digit year */
            format = "%T%t%m/%d/%Y";
        }
    }

    if (!strptime(string, format, &otm))
        goto timeformat_err;

    return mktime(&otm);

    /*
     * I don't like using goto ... but this is the classic example of when it
     * is acceptable.  It's basically a faux try-catch block as all the errors
     * will be the same here.
     */
timeformat_err:
    *error = "Time string does not match expected format.";
    return 0;
}
#endif
