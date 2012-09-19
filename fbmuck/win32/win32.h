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
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define vsnprintf _vsnprintf
#define snprintf _snprintf

