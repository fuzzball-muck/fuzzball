#ifndef _WIN32_H_
#define _WIN32_H_

#include <direct.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define CURRENTLY_UNAVAILABLE "If you see this message, visit us at https://github.com/fuzzball-muck/fuzzball and open an issue. We'd be happy to fix it, but we would like more detail on how it is being used."

#define pid_t		int
#define socklen_t	int
#define ssize_t		long

#define close(x)	closesocket(x)
#define chdir		_chdir
#define execv		_execv
#define getpid		_getpid
#define inet_pton	InetPton
#define pclose		_pclose
#define popen		_popen
#define read(fd, buf, count) \
			recv(fd, (char *)buf, count, 0)
#define strcasecmp	_stricmp
#define strdup		_strdup
#define strncasecmp	_strnicmp
#define strtok_r	strtok_s
#define tzname		_tzname
#define unlink		_unlink
#define write(fd, buf, count) \
			send(fd, (char *)buf, count, 0)

int gettimeofday(struct timeval *tv, struct timezone *tz);
void set_console();
void sync();
struct tm *uw32localtime(const time_t *t);

#endif
