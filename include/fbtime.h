#ifndef FBTIME_H
#define FBTIME_H

#include <time.h>

#include "config.h"

#define TIME_MINUTE(x)  (60 * (x))			/* 60 seconds */
#define TIME_HOUR(x)    ((x) * (TIME_MINUTE(60)))	/* 60 minutes */
#define TIME_DAY(x)     ((x) * (TIME_HOUR(24)))		/* 24 hours   */

long get_tz_offset(void);
struct timeval msec_add(struct timeval t, int x);
int msec_diff(struct timeval now, struct timeval then);
char *time_format_1(time_t dt);
char *time_format_2(time_t dt);
const char *timestr_full(long dtime);
const char *timestr_long(long dtime);
struct timeval timeval_sub(struct timeval now, struct timeval then);
void ts_lastuseobject(dbref thing);
void ts_modifyobject(dbref thing);
void ts_newobject(dbref thing);
void ts_useobject(dbref thing);

#endif /* !FBTIME_H */
