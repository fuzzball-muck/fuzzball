
#include <windows.h>
#include <time.h>

void sync(void) { return; }


#define TOFFSET (((LONGLONG)27111902 << 32) + (LONGLONG)3577643008)

static void ftconvert(const FILETIME *ft, struct timeval *ts);

int gettimeofday(struct timeval *tv, struct timezone *tz) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ftconvert(&ft,tv);
	return 0;
}

static void ftconvert(const FILETIME *ft, struct timeval *ts) {
	ts->tv_sec = (int)((*(LONGLONG *)ft - TOFFSET) / 10000000);
	ts->tv_usec = (int)((*(LONGLONG *)ft - TOFFSET - ((LONGLONG)ts->tv_sec * (LONGLONG)10000000)) * 100) / 1000;
}

struct tm *uw32localtime(const time_t *t) {
	struct tm epoch2tm, *lt;
	time_t epoch2, p;
	if (*t < 0) {
		epoch2tm.tm_hour = 0;
		epoch2tm.tm_isdst = 0;
		epoch2tm.tm_mday = 1;
		epoch2tm.tm_min = 0;
		epoch2tm.tm_mon = 0;
		epoch2tm.tm_sec = 0;
		epoch2tm.tm_wday = 0;
		epoch2tm.tm_yday = 0;
		epoch2tm.tm_year = 2026 - 1900;

		epoch2 = mktime(&epoch2tm) - _timezone;

		p = *t + epoch2;
		lt = localtime(&p);
		lt->tm_year -= 56;
		return lt;
	} else {
		return localtime(t);
	}
}