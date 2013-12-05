/*
 *      announce - sits listening on a port, and whenever anyone connects
 *                 announces a message and disconnects them
 *
 *      Usage:  announce [port] < message_file
 *
 *      Author: Lawrence Brown <lpb@cs.adfa.oz.au>      Aug 90
 *
 *      Bits of code are adapted from the Berkeley telnetd sources
 */

#include "autoconf.h"

#define PORT    4201

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

/* "do not include netinet6/in6.h directly, include netinet/in.h.  see RFC2553" */

extern char **environ;
#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#else
extern int errno;
#endif
#endif
char *Name;						/* name of this program for error messages */
char msg[32768];


char*
strcpyn(char* buf, size_t bufsize, const char* src)
{
	int pos = 0;
	char* dest = buf;

	while (++pos < bufsize && *src) {
		*dest++ = *src++;
	}
	*dest = '\0';
	return buf;
}

char*
strcatn(char* buf, size_t bufsize, const char* src)
{
	int pos = strlen(buf);
	char* dest = &buf[pos];

	while (++pos < bufsize && *src) {
		*dest++ = *src++;
	}
	if (pos < bufsize + 1) {
		*dest = '\0';
	}
	return buf;
}

int
notify(int player, const char *msg)
{
	return printf("%s\n", msg);
}

int
main(int argc, char *argv[])
{
	int s, ns;
	socklen_t foo;

#ifdef USE_IPV6
	static struct sockaddr_in6 sin = { AF_INET6 };
	char host[128], *inet_ntoa();
#else
	static struct sockaddr_in sin = { AF_INET };
	char *host, *inet_ntoa();
#endif
	char tmp[32768];
	time_t ct;

	Name = argv[0];				/* save name of program for error messages  */

#ifdef USE_IPV6
	sin.sin6_port = htons((u_short) PORT);	/* Assume PORT */
	argc--, argv++;
	if (argc > 0) {				/* unless specified on command-line       */
		sin.sin6_port = atoi(*argv);
		sin.sin6_port = htons((u_short) sin.sin6_port);
	}
#else
	sin.sin_port = htons((u_short) PORT);	/* Assume PORT */
	argc--, argv++;
	if (argc > 0) {				/* unless specified on command-line       */
		sin.sin_port = atoi(*argv);
		sin.sin_port = htons((u_short) sin.sin_port);
	}
#endif

	strcpyn(msg, sizeof(msg), "");
	strcpyn(tmp, sizeof(tmp), "");
	while (1) {
		if (fgets(tmp,32766,stdin) == NULL)
			break;
		strcatn(tmp, sizeof(tmp), "\r\n");
		strcatn(msg, sizeof(msg), tmp);
	}
	msg[4095] = '\0';
	signal(SIGHUP, SIG_IGN);	/* get socket, bind port to it      */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("announce: socket");
		exit(1);
	}
	if (bind(s, (struct sockaddr *) &sin, sizeof sin) < 0) {
		perror("bind");
		exit(1);
	}
	if ((foo = fork()) != 0) {
#ifdef USE_IPV6
		fprintf(stderr, "announce: pid %d running on port %d\n", foo,
				ntohs((u_short) sin.sin6_port));
#else
		fprintf(stderr, "announce: pid %d running on port %d\n", foo,
				ntohs((u_short) sin.sin_port));
#endif
		_exit(0);
	} else {
#ifdef PRIO_PROCESS
		setpriority(PRIO_PROCESS, getpid(), 10);
#endif
	}
	if (listen(s, 1) < 0) {		/* start listening on port */
		perror("announce: listen");
		exit(1);
	}
	foo = sizeof sin;
	for (;;) {					/* loop forever, accepting requests & printing
								   * msg */
		ns = accept(s, (struct sockaddr *) &sin, &foo);
		if (ns < 0) {
			perror("announce: accept");
			exit(1);
		}
#ifdef USE_IPV6
		inet_ntop(AF_INET6, sin.sin6_addr, host, 128);
#else
		host = inet_ntoa(sin.sin_addr);
#endif

		ct = time((time_t *) NULL);
		fprintf(stderr, "CONNECTION made from %s at %s", host, ctime(&ct));
		write(ns, msg, strlen(msg));
		sleep(5);
		close(ns);
	}
}								/* main */
static const char *announce_c_version = "$RCSfile: announce.c,v $ $Revision: 1.17 $";
const char *get_announce_c_version(void) { return announce_c_version; }
