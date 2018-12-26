#include <time.h>
#include <windows.h>

void sync(void) { return; }

#define TOFFSET (((LONGLONG)27111902 << 32) + (LONGLONG)3577643008)

static void ftconvert(const FILETIME *ft, struct timeval *ts)
{
	ts->tv_sec = (int)((*(LONGLONG *)ft - TOFFSET) / 10000000);
	ts->tv_usec = (int)((*(LONGLONG *)ft - TOFFSET - ((LONGLONG)ts->tv_sec * (LONGLONG)10000000)) * 100) / 1000;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ftconvert(&ft,tv);
	return 0;
}
