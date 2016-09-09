#ifndef _FBTIME_H
#define _FBTIME_H

#include "config.h"

#ifndef WIN32
# define MUCK_LOCALTIME(t)              localtime(&t)
#else
# define MUCK_LOCALTIME(t)              uw32localtime(&t)
#endif

int format_time(char *buf, int max_len, const char *fmt, struct tm *tmval);
long get_tz_offset(void);
struct timeval msec_add(struct timeval t, int x);
int msec_diff(struct timeval now, struct timeval then);
char *time_format_1(time_t dt);
char *time_format_2(time_t dt);
const char *timestr_full(long dtime);
struct timeval timeval_sub(struct timeval now, struct timeval then);
void ts_lastuseobject(dbref thing);
void ts_modifyobject(dbref thing);
void ts_newobject(dbref thing);
void ts_useobject(dbref thing);

#endif				/* _FBTIME_H */
