
#include "copyright.h"
#include "config.h"

#ifdef SOLARIS
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE		/* Solaris needs this */
#  endif
#endif

#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>


#define NUM_THREADS     5

static const char *resolver_c_version = "$RCSfile: resolver.c,v $ $Revision: 1.12 $";
const char *get_resolver_c_version(void) { return resolver_c_version; }


/*
 * SunOS can't include signal.h and sys/signal.h, stupid broken OS.
 */
#if defined(HAVE_SYS_SIGNAL_H) && !defined(SUN_OS)
# include <sys/signal.h>
#endif



/* number of hostnames cached in an LRU queue */
#define HOST_CACHE_SIZE 8192

/* Time before retrying to resolve a previously unresolved IP address. */
/* 1800 seconds == 30 minutes */
#define EXPIRE_TIME 1800

/* Time before the resolver gives up on a identd lookup.  Prevents hangs. */
#define IDENTD_TIMEOUT 60


/*
 * Like strncpy, except it guarentees null termination of the result string.
 * It also has a more sensible argument ordering.
 */
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


int
notify(int player, const char* msg)
{
	return printf("%s\n", msg);
}

#ifdef USE_IPV6
const char *addrout_v6(struct in6_addr *, unsigned short, unsigned short);
#endif
const char *addrout(long, unsigned short, unsigned short);


#define MALLOC(result, type, number) do {   \
                                       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
                                       abort();                             \
                                     } while (0)

#define FREE(x) (free((void *) x))


struct hostcache {
#ifdef USE_IPV6
	struct in6_addr ipnum_v6;
#endif
	long ipnum;
	char name[128];
	time_t time;
	struct hostcache *next;
	struct hostcache **prev;
} *hostcache_list = 0;



#ifdef USE_IPV6
int ipcmp(struct in6_addr *a, struct in6_addr *b)
{
	int i = 128;
	char *A = (char *) a;
	char *B = (char *) b;

	while (i) {
		i--;
		if (*A++ != *B++)
			break;
	}
	return i;
}
#endif



#ifdef USE_IPV6
void hostdel_v6(struct in6_addr *ip)
{
	struct hostcache *ptr;

	for (ptr = hostcache_list; ptr; ptr = ptr->next) {
		if (!ipcmp(&(ptr->ipnum_v6), ip)) {
			if (ptr->next) {
				ptr->next->prev = ptr->prev;
			}
			*ptr->prev = ptr->next;
			FREE(ptr);
			return;
		}
	}
}
#endif



void hostdel(long ip)
{
	struct hostcache *ptr;

	for (ptr = hostcache_list; ptr; ptr = ptr->next) {
		if (ptr->ipnum == ip) {
			if (ptr->next) {
				ptr->next->prev = ptr->prev;
			}
			*ptr->prev = ptr->next;
			FREE(ptr);
			return;
		}
	}
}



#ifdef USE_IPV6
const char * hostfetch_v6(struct in6_addr *ip)
{
	struct hostcache *ptr;

	for (ptr = hostcache_list; ptr; ptr = ptr->next) {
		if (!ipcmp(&(ptr->ipnum_v6), ip)) {
			if (time(NULL) - ptr->time > EXPIRE_TIME) {
				hostdel_v6(ip);
				return NULL;
			}
			if (ptr != hostcache_list) {
				*ptr->prev = ptr->next;
				if (ptr->next) {
					ptr->next->prev = ptr->prev;
				}
				ptr->next = hostcache_list;
				if (ptr->next) {
					ptr->next->prev = &ptr->next;
				}
				ptr->prev = &hostcache_list;
				hostcache_list = ptr;
			}
			return (ptr->name);
		}
	}
	return NULL;
}
#endif



const char *hostfetch(long ip)
{
	struct hostcache *ptr;

	for (ptr = hostcache_list; ptr; ptr = ptr->next) {
		if (ptr->ipnum == ip) {
			if (time(NULL) - ptr->time > EXPIRE_TIME) {
				hostdel(ip);
				return NULL;
			}
			if (ptr != hostcache_list) {
				*ptr->prev = ptr->next;
				if (ptr->next) {
					ptr->next->prev = ptr->prev;
				}
				ptr->next = hostcache_list;
				if (ptr->next) {
					ptr->next->prev = &ptr->next;
				}
				ptr->prev = &hostcache_list;
				hostcache_list = ptr;
			}
			return (ptr->name);
		}
	}
	return NULL;
}



void
hostprune(void)
{
	struct hostcache *ptr;
	struct hostcache *tmp;
	int i = HOST_CACHE_SIZE;

	ptr = hostcache_list;
	while (i-- && ptr) {
		ptr = ptr->next;
	}
	if (i < 0 && ptr) {
		*ptr->prev = NULL;
		while (ptr) {
			tmp = ptr->next;
			FREE(ptr);
			ptr = tmp;
		}
	}
}



#ifdef USE_IPV6
void hostadd_v6(struct in6_addr *ip, const char *name)
{
	struct hostcache *ptr;

	MALLOC(ptr, struct hostcache, 1);

	ptr->next = hostcache_list;
	if (ptr->next) {
		ptr->next->prev = &ptr->next;
	}
	ptr->prev = &hostcache_list;
	hostcache_list = ptr;
	memcpy(&(ptr->ipnum_v6), ip, sizeof(struct in6_addr));
	ptr->ipnum = 0;
	ptr->time = 0;
	strcpyn(ptr->name, sizeof(ptr->name), name);
	hostprune();
}
#endif


void hostadd(long ip, const char *name)
{
	struct hostcache *ptr;

	MALLOC(ptr, struct hostcache, 1);

	ptr->next = hostcache_list;
	if (ptr->next) {
		ptr->next->prev = &ptr->next;
	}
	ptr->prev = &hostcache_list;
	hostcache_list = ptr;
#ifdef USE_IPV6
	memset(&(ptr->ipnum_v6), 0, sizeof(struct in6_addr));
#endif
	ptr->ipnum = ip;
	ptr->time = 0;
	strcpyn(ptr->name, sizeof(ptr->name), name);
	hostprune();
}


#ifdef USE_IPV6
void hostadd_timestamp_v6(struct in6_addr *ip, const char *name)
{
	hostadd_v6(ip, name);
	hostcache_list->time = time(NULL);
}
#endif




void hostadd_timestamp(long ip, const char *name)
{
	hostadd(ip, name);
	hostcache_list->time = time(NULL);
}








void set_signals(void);

#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler) (int));
#else
# define our_signal(s,f) signal((s),(f))
#endif

/*
 * our_signal(signo, sighandler)
 *
 * signo      - Signal #, see defines in signal.h
 * sighandler - The handler function desired.
 *
 * Calls sigaction() to set a signal, if we are posix.
 */
#ifdef _POSIX_VERSION
void
our_signal(int signo, void (*sighandler) (int))
{
	struct sigaction act, oact;

	act.sa_handler = sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	/* Restart long system calls if a signal is caught. */
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	/* Make it so */
	sigaction(signo, &act, &oact);
}

#endif							/* _POSIX_VERSION */

/*
 * set_signals()
 * set_sigs_intern(bail)
 *
 * Traps a bunch of signals and reroutes them to various
 * handlers. Mostly bailout.
 *
 * If called from bailout, then reset all to default.
 *
 * Called from main() and bailout()
 */

void
set_signals(void)
{
	/* we don't care about SIGPIPE, we notice it in select() and write() */
	our_signal(SIGPIPE, SIG_IGN);

	/* didn't manage to lose that control tty, did we? Ignore it anyway. */
	our_signal(SIGHUP, SIG_IGN);
}






void
make_nonblocking(int s)
{
#if !defined(O_NONBLOCK) || defined(ULTRIX)	/* POSIX ME HARDER */
# ifdef FNDELAY					/* SUN OS */
#  define O_NONBLOCK FNDELAY
# else
#  ifdef O_NDELAY				/* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif						/* O_NDELAY */
# endif							/* FNDELAY */
#endif

	if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
		perror("make_nonblocking: fcntl");
		abort();
	}
}


#ifdef USE_IPV6
const char *get_username_v6(struct in6_addr *a, int prt, int myprt)
{
	int fd, len, result;
	char *ptr, *ptr2;
	static char buf[1024];
	int lasterr;
	int timeout = IDENTD_TIMEOUT;

	struct sockaddr_in6 addr;

	if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("resolver ident socket");
		return (0);
	}

	make_nonblocking(fd);

	len = sizeof(addr);
	addr.sin6_family = AF_INET6;
	memcpy(&addr.sin6_addr.s6_addr, a, sizeof(struct in6_addr));
	addr.sin6_port = htons((short) 113);

	do {
		result = connect(fd, (struct sockaddr *) &addr, len);
		lasterr = errno;
		if (result < 0) {
			if (!timeout--)
				break;
			sleep(1);
		}
	} while (result < 0 && lasterr == EINPROGRESS);
	if (result < 0 && lasterr != EISCONN) {
		goto bad;
	}

	snprintf(buf, sizeof(buf), "%d,%d\n", prt, myprt);
	do {
		result = write(fd, buf, strlen(buf));
		lasterr = errno;
		if (result < 0) {
			if (!timeout--)
				break;
			sleep(1);
		}
	} while (result < 0 && lasterr == EAGAIN);
	if (result < 0)
		goto bad2;

	do {
		result = read(fd, buf, sizeof(buf));
		lasterr = errno;
		if (result < 0) {
			if (!timeout--)
				break;
			sleep(1);
		}
	} while (result < 0 && lasterr == EAGAIN);
	if (result < 0)
		goto bad2;

	ptr = index(buf, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	if (*ptr)
		ptr++;
	if (strncmp(ptr, "USERID", 6))
		goto bad2;

	ptr = index(ptr, ':');
	if (!ptr)
		goto bad2;
	ptr = index(ptr + 1, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	/* if (*ptr) ptr++; */

	shutdown(fd, 2);
	close(fd);
	if ((ptr2 = index(ptr, '\r')))
		*ptr2 = '\0';
	if (!*ptr)
		return (0);
	return ptr;

  bad2:
	shutdown(fd, 2);

  bad:
	close(fd);
	return (0);
}
#endif


const char *get_username(long a, int prt, int myprt)
{
	int fd, len, result;
	char *ptr, *ptr2;
	static char buf[1024];
	int lasterr;
	int timeout = IDENTD_TIMEOUT;

	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("resolver ident socket");
		return (0);
	}

	make_nonblocking(fd);

	len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = a;
	addr.sin_port = htons((short) 113);

	do {
		result = connect(fd, (struct sockaddr *) &addr, len);
		lasterr = errno;
		if (result < 0) {
			if (!timeout--)
				break;
			sleep(1);
		}
	} while (result < 0 && lasterr == EINPROGRESS);
	if (result < 0 && lasterr != EISCONN) {
		goto bad;
	}

	snprintf(buf, sizeof(buf), "%d,%d\n", prt, myprt);
	do {
		result = write(fd, buf, strlen(buf));
		lasterr = errno;
		if (result < 0) {
			if (!timeout--)
				break;
			sleep(1);
		}
	} while (result < 0 && lasterr == EAGAIN);
	if (result < 0)
		goto bad2;

	do {
		result = read(fd, buf, sizeof(buf));
		lasterr = errno;
		if (result < 0) {
			if (!timeout--)
				break;
			sleep(1);
		}
	} while (result < 0 && lasterr == EAGAIN);
	if (result < 0)
		goto bad2;

	ptr = index(buf, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	if (*ptr)
		ptr++;
	if (strncmp(ptr, "USERID", 6))
		goto bad2;

	ptr = index(ptr, ':');
	if (!ptr)
		goto bad2;
	ptr = index(ptr + 1, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	/* if (*ptr) ptr++; */

	shutdown(fd, 2);
	close(fd);
	if ((ptr2 = index(ptr, '\r')))
		*ptr2 = '\0';
	if (!*ptr)
		return (0);
	return ptr;

  bad2:
	shutdown(fd, 2);

  bad:
	close(fd);
	return (0);
}



/*  addrout_v6 -- Translate address 'a' to text.          */
#ifdef USE_IPV6
const char *addrout_v6(struct in6_addr *a, unsigned short prt, unsigned short myprt)
{
	static char buf[128];
	char tmpbuf[128];
	const char *ptr, *ptr2;
	struct hostent *he;

	struct in6_addr addr;
	memcpy(&addr.s6_addr, a, sizeof(struct in6_addr));

	ptr = hostfetch_v6(a);

	if (ptr) {
		ptr2 = get_username_v6(a, prt, myprt);
		if (ptr2) {
			snprintf(buf, sizeof(buf), "%s(%s)", ptr, ptr2);
		} else {
			snprintf(buf, sizeof(buf), "%s(%d)", ptr, prt);
		}
		return buf;
	}
	he = gethostbyaddr(((char *) &addr), sizeof(addr), AF_INET6);

	if (he) {
		strcpyn(tmpbuf, sizeof(tmpbuf), he->h_name);
		hostadd_v6(a, tmpbuf);
		ptr = get_username_v6(a, prt, myprt);
		if (ptr) {
			snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
		} else {
			snprintf(buf, sizeof(buf), "%s(%d)", tmpbuf, prt);
		}
		return buf;
	}
	inet_ntop(AF_INET6, a, tmpbuf, 128);
	hostadd_timestamp_v6(a, tmpbuf);
	ptr = get_username_v6(a, prt, myprt);

	if (ptr) {
		snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
	} else {
		snprintf(buf, sizeof(buf), "%s(%d)", tmpbuf, prt);
	}
	return buf;
}
#endif


/*  addrout -- Translate address 'a' to text.          */
const char *addrout(long a, unsigned short prt, unsigned short myprt)
{
	static char buf[128];
	char tmpbuf[128];
	const char *ptr, *ptr2;
	struct hostent *he;
	struct in_addr addr;

	addr.s_addr = a;
	ptr = hostfetch(ntohl(a));

	if (ptr) {
		ptr2 = get_username(a, prt, myprt);
		if (ptr2) {
			snprintf(buf, sizeof(buf), "%s(%s)", ptr, ptr2);
		} else {
			snprintf(buf, sizeof(buf), "%s(%d)", ptr, prt);
		}
		return buf;
	}
	he = gethostbyaddr(((char *) &addr), sizeof(addr), AF_INET);

	if (he) {
		strcpyn(tmpbuf, sizeof(tmpbuf), he->h_name);
		hostadd(ntohl(a), tmpbuf);
		ptr = get_username(a, prt, myprt);
		if (ptr) {
			snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
		} else {
			snprintf(buf, sizeof(buf), "%s(%d)", tmpbuf, prt);
		}
		return buf;
	}

	a = ntohl(a);
	snprintf(tmpbuf, sizeof(tmpbuf), "%ld.%ld.%ld.%ld",
			(a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff);
	hostadd_timestamp(a, tmpbuf);
	ptr = get_username(htonl(a), prt, myprt);

	if (ptr) {
		snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
	} else {
		snprintf(buf, sizeof(buf), "%s(%d)", tmpbuf, prt);
	}
	return buf;
}



volatile short shutdown_was_requested = 0;
pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

int
do_resolve(void)
{
	int ip1, ip2, ip3, ip4;
	int prt, myprt;
	int doagain;
	char *result;
	const char *ptr;
	char buf[1024];
	char outbuf[1024];
	char *bufptr = NULL;
	long fullip;

#ifdef USE_IPV6
	char ipv6addr[128];
	struct in6_addr fullip_v6;
#endif

	for (;;) {
		ip1 = ip2 = ip3 = ip4 = prt = myprt = -1;
		do {
			doagain = 0;
			*buf = '\0';

			// lock input here.
			if (pthread_mutex_lock(&input_mutex)) {
				return 0;
			}
			if (shutdown_was_requested) {
				// unlock input here.
				pthread_mutex_unlock(&input_mutex);
				return 0;
			}

			result = fgets(buf, sizeof(buf), stdin);

			// unlock input here.
			pthread_mutex_unlock(&input_mutex);

			if (shutdown_was_requested) {
				return 0;
			}
			if (!result) {
				if (errno == EAGAIN) {
					doagain = 1;
					sleep(1);
				} else {
					if (feof(stdin)) {
						shutdown_was_requested = 1;
						return 0;
					}
					perror("fgets");
					shutdown_was_requested = 1;
					return 0;
				}
			}
		} while (doagain || !strcmp(buf, "\n"));
		if (!strncmp("QUIT", buf, 4)) {
			shutdown_was_requested = 1;
			fclose(stdin);
			return 0;
		}

		bufptr = NULL;
	#ifdef USE_IPV6
		bufptr = strchr(buf, ':');
		if (bufptr) {
			// Is an IPv6 addr.
			bufptr = strchr(buf, '(');
			if (!bufptr) {
				continue;
			}
			sscanf(bufptr, "(%d)%d", &prt, &myprt);
			*bufptr = '\0';
			if (myprt > 65535 || myprt < 0) {
				continue;
			}
			if (inet_pton(AF_INET6, buf, &fullip_v6) <= 0) {
				continue;
			}
			ptr = addrout_v6(&fullip_v6, prt, myprt);
			snprintf(outbuf, sizeof(outbuf), "%s(%d)|%s", buf, prt, ptr);
		}
	#endif
		if (!bufptr) {
			// Is an IPv4 addr.
			sscanf(buf, "%d.%d.%d.%d(%d)%d", &ip1, &ip2, &ip3, &ip4, &prt, &myprt);
			if (ip1 < 0 || ip2 < 0 || ip3 < 0 || ip4 < 0 || prt < 0) {
				continue;
			}
			if (ip1 > 255 || ip2 > 255 || ip3 > 255 || ip4 > 255 || prt > 65535) {
				continue;
			}
			if (myprt > 65535 || myprt < 0) {
				continue;
			}

			fullip = (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
			fullip = htonl(fullip);

			ptr = addrout(fullip, prt, myprt);
			snprintf(outbuf, sizeof(outbuf), "%d.%d.%d.%d(%d)|%s", ip1, ip2, ip3, ip4, prt, ptr);
		}

		// lock output here.
		if (pthread_mutex_lock(&output_mutex)) {
			return 0;
		}

		fprintf(stdout, "%s\n", outbuf);
		fflush(stdout);

		// unlock output here.
		pthread_mutex_unlock(&output_mutex);
	}

	return 1;
}




void *resolver_thread_root(void* threadid)
{
	do_resolve();
    pthread_exit(NULL);
}


int main(int argc, char **argv)
{
	pthread_t threads[NUM_THREADS];
	long i;

	if (argc > 1) {
		fprintf(stderr, "Usage: %s\n", *argv);
		exit(1);
		return 1;
	}

	/* remember to ignore certain signals */
	set_signals();

	/* go do it */
	for(i = 0; i < NUM_THREADS; i++){
		int rc = pthread_create(&threads[i], NULL, resolver_thread_root, (void *)i);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	for(i = 0; i < NUM_THREADS; i++){
		void* retval;
	    pthread_join(threads[i], &retval);
	}

	fprintf(stderr, "Resolver exited.\n");

	exit(0);
	return 0;
}

