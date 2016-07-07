#ifndef _FBTIME_H
#define _FBTIME_H

#include "config.h"

#include "db.h"

#ifndef WIN32
# define MUCK_LOCALTIME(t)              localtime(&t)
#else
# define MUCK_LOCALTIME(t)              uw32localtime(&t)
#endif

int format_time(char *buf, int max_len, const char *fmt, struct tm *tmval);
long get_tz_offset(void);
void ts_lastuseobject(dbref thing);
void ts_modifyobject(dbref thing);
void ts_newobject(struct object *thing);
void ts_useobject(dbref thing);

#endif				/* _FBTIME_H */
