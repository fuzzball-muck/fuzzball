#ifndef _WIN32_H_
#define _WIN32_H_

#include <winsock2.h>
#include <process.h>
#include <direct.h>
#include <time.h>
#pragma warning( disable : 4244 4101 4018 )

extern int gettimeofday(struct timeval *tv, struct timezone *tz);
extern struct tm *uw32localtime(const time_t *t);
extern void sync(void);
extern void set_console();
#define close(x) closesocket(x)
#define chdir _chdir
#define getpid _getpid
#define pid_t int
#define ssize_t long

#ifndef tzname
#define tzname _tzname
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif

#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef popen
#define popen _popen
#endif

#ifndef pclose
#define pclose _pclose
#endif

#endif