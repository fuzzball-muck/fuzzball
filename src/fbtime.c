#include "config.h"

#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"

void
ts_newobject(dbref thing)
{
    time_t now = time(NULL);

    DBFETCH(thing)->ts_created = now;
    DBFETCH(thing)->ts_modified = now;
    DBFETCH(thing)->ts_lastused = now;
    DBFETCH(thing)->ts_usecount = 0;
}

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

void
ts_lastuseobject(dbref thing)
{
    if (thing == NOTHING)
	return;
    DBSTORE(thing, ts_lastused, time(NULL));
    if (Typeof(thing) == TYPE_ROOM)
	ts_lastuseobject(LOCATION(thing));
}

void
ts_modifyobject(dbref thing)
{
    DBSTORE(thing, ts_modified, time(NULL));
}

long
get_tz_offset(void)
{
#ifdef HAVE_DECL__TIMEZONE
	/* CygWin uses _timezone instead of timezone. */
	return _timezone;
#else
	tzset();
	/* extern long timezone; */
	return timezone * -1;
#endif
}

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

int
msec_diff(struct timeval now, struct timeval then)
{
    return ((now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec) / 1000);
}

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
