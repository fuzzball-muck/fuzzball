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

int
format_time(char *buf, int max_len, const char *fmt, struct tm *tmval)
{

#ifdef USE_STRFTIME
    return (strftime(buf, max_len, fmt, tmval));
#else
    int pos, ret;
    char tmp[256];

    /* struct timezone tz; */

    pos = 0;
    max_len--;
    while ((*fmt) && (pos < max_len))
	if ((buf[pos++] = *(fmt++)) == '%')
	    if (*fmt) {
		pos--;
		*tmp = '\0';
		switch (*(fmt++)) {
		case 'a':
		    switch (tmval->tm_wday) {
		    case 0:
			(void) strcpyn(tmp, sizeof(tmp), "Sun");
			break;
		    case 1:
			(void) strcpyn(tmp, sizeof(tmp), "Mon");
			break;
		    case 2:
			(void) strcpyn(tmp, sizeof(tmp), "Tue");
			break;
		    case 3:
			(void) strcpyn(tmp, sizeof(tmp), "Wed");
			break;
		    case 4:
			(void) strcpyn(tmp, sizeof(tmp), "Thu");
			break;
		    case 5:
			(void) strcpyn(tmp, sizeof(tmp), "Fri");
			break;
		    case 6:
			(void) strcpyn(tmp, sizeof(tmp), "Sat");
			break;
		    }
		    break;
		case 'A':
		    switch (tmval->tm_wday) {
		    case 0:
			(void) strcpyn(tmp, sizeof(tmp), "Sunday");
			break;
		    case 1:
			(void) strcpyn(tmp, sizeof(tmp), "Monday");
			break;
		    case 2:
			(void) strcpyn(tmp, sizeof(tmp), "Tuesday");
			break;
		    case 3:
			(void) strcpyn(tmp, sizeof(tmp), "Wednesday");
			break;
		    case 4:
			(void) strcpyn(tmp, sizeof(tmp), "Thursday");
			break;
		    case 5:
			(void) strcpyn(tmp, sizeof(tmp), "Friday");
			break;
		    case 6:
			(void) strcpyn(tmp, sizeof(tmp), "Saturday");
			break;
		    }
		    break;
		case 'h':
		case 'b':
		    switch (tmval->tm_mon) {
		    case 0:
			(void) strcpyn(tmp, sizeof(tmp), "Jan");
			break;
		    case 1:
			(void) strcpyn(tmp, sizeof(tmp), "Feb");
			break;
		    case 2:
			(void) strcpyn(tmp, sizeof(tmp), "Mar");
			break;
		    case 3:
			(void) strcpyn(tmp, sizeof(tmp), "Apr");
			break;
		    case 4:
			(void) strcpyn(tmp, sizeof(tmp), "May");
			break;
		    case 5:
			(void) strcpyn(tmp, sizeof(tmp), "Jun");
			break;
		    case 6:
			(void) strcpyn(tmp, sizeof(tmp), "Jul");
			break;
		    case 7:
			(void) strcpyn(tmp, sizeof(tmp), "Aug");
			break;
		    case 8:
			(void) strcpyn(tmp, sizeof(tmp), "Sep");
			break;
		    case 9:
			(void) strcpyn(tmp, sizeof(tmp), "Oct");
			break;
		    case 10:
			(void) strcpyn(tmp, sizeof(tmp), "Nov");
			break;
		    case 11:
			(void) strcpyn(tmp, sizeof(tmp), "Dec");
			break;
		    }
		    break;
		case 'B':
		    switch (tmval->tm_mon) {
		    case 0:
			(void) strcpyn(tmp, sizeof(tmp), "January");
			break;
		    case 1:
			(void) strcpyn(tmp, sizeof(tmp), "February");
			break;
		    case 2:
			(void) strcpyn(tmp, sizeof(tmp), "March");
			break;
		    case 3:
			(void) strcpyn(tmp, sizeof(tmp), "April");
			break;
		    case 4:
			(void) strcpyn(tmp, sizeof(tmp), "May");
			break;
		    case 5:
			(void) strcpyn(tmp, sizeof(tmp), "June");
			break;
		    case 6:
			(void) strcpyn(tmp, sizeof(tmp), "July");
			break;
		    case 7:
			(void) strcpyn(tmp, sizeof(tmp), "August");
			break;
		    case 8:
			(void) strcpyn(tmp, sizeof(tmp), "September");
			break;
		    case 9:
			(void) strcpyn(tmp, sizeof(tmp), "October");
			break;
		    case 10:
			(void) strcpyn(tmp, sizeof(tmp), "November");
			break;
		    case 11:
			(void) strcpyn(tmp, sizeof(tmp), "December");
			break;
		    }
		    break;
		case 'c':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%x %X", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'C':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%A %B %e, %Y", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'd':
		    int2str(tmp, tmval->tm_mday, 2, '0');
		    break;
		case 'D':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%m/%d/%y", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'e':
		    int2str(tmp, tmval->tm_mday, 2, ' ');
		    break;
		case 'H':
		    int2str(tmp, tmval->tm_hour, 2, '0');
		    break;
		case 'I':
		    int2str(tmp, (tmval->tm_hour + 11) % 12 + 1, 2, '0');
		    break;
		case 'j':
		    int2str(tmp, tmval->tm_yday + 1, 3, '0');
		    break;
		case 'k':
		    int2str(tmp, tmval->tm_hour, 2, ' ');
		    break;
		case 'l':
		    int2str(tmp, (tmval->tm_hour + 11) % 12 + 1, 2, ' ');
		    break;
		case 'm':
		    int2str(tmp, tmval->tm_mon + 1, 2, '0');
		    break;
		case 'M':
		    int2str(tmp, tmval->tm_min, 2, '0');
		    break;
		case 'p':
		    tmp[0] = ((tmval->tm_hour < 12) ? 'A' : 'P');
		    tmp[1] = 'M';
		    tmp[2] = '\0';
		    break;
		case 'r':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%I:%M:%S %p", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'R':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%H:%M", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'S':
		    int2str(tmp, tmval->tm_sec, 2, '0');
		    break;
		case 'T':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%H:%M:%S", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'U':
		    int2str(tmp,
			    (tmval->tm_yday +
			     ((7 + tmval->tm_wday - (tmval->tm_yday % 7)) % 7)) / 7, 2, '0');
		    break;
		case 'w':
		    tmp[0] = tmval->tm_wday + '0';
		    tmp[1] = '\0';
		    break;
		case 'W':
		    int2str(tmp,
			    (tmval->tm_yday +
			     ((13 + tmval->tm_wday - (tmval->tm_yday % 7)) % 7)) / 7, 2, '0');
		    break;
		case 'x':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%m/%d/%y", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'X':
		    if (!(ret = format_time(buf + pos, max_len - pos,
					    (char *) "%H:%M:%S", tmval)))
			return (0);
		    pos += ret;
		    break;
		case 'y':
		    int2str(tmp, (tmval->tm_year % 100), 2, '0');
		    break;
		case 'Y':
		    int2str(tmp, tmval->tm_year + 1900, 4, '0');
		    break;
		case 'Z':
#ifdef HAVE_STRUCT_TM_TM_ZONE
		    strcpyn(tmp, sizeof(tmp), tmval->tm_zone);
#else				/* !HAVE_STRUCT_TM_TM_ZONE */
# ifdef HAVE_TZNAME
		    strcpyn(tmp, sizeof(tmp), tzname[tmval->tm_isdst]);
# endif
#endif				/* !HAVE_STRUCT_TM_TM_ZONE */
		    break;
		case '%':
		    tmp[0] = '%';
		    tmp[1] = '\0';
		    break;
		default:
		    tmp[0] = '%';
		    tmp[1] = *(fmt - 1);
		    tmp[2] = '\0';
		    break;
		}
		if (pos + strlen(tmp) > (size_t) max_len)
		    return (0);
		(void) strcpyn(buf + pos, max_len - pos, tmp);
		pos += strlen(tmp);
	    }
    buf[pos] = '\0';
    return (pos);
#endif				/* USE_STRFTIME */
}

long
get_tz_offset(void)
{
/*
 * SunOS don't seem to have timezone as a "extern long", but as
 * a structure. This makes it very hard (at best) to check for,
 * therefor I'm checking for tm_gmtoff. --WF
 */
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    time_t now;

    time(&now);
    return (MUCK_LOCALTIME(now)->tm_gmtoff);
#elif defined(HAVE_DECL__TIMEZONE)
    /* CygWin uses _timezone instead of timezone. */
    return _timezone;
#else
    /* extern long timezone; */
    return timezone;
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
