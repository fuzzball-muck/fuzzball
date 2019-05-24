#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_THREADS 5

/* number of hostnames cached in an LRU queue */
#define HOST_CACHE_SIZE 8192

/* Time before retrying to resolve a previously unresolved IP address.  1800 seconds == 30 minutes */
#define EXPIRE_TIME 1800

/* Time before the resolver gives up on a identd lookup.  Prevents hangs. */
#define IDENTD_TIMEOUT 60

/*
 * Like strncpy, except it guarentees null termination of the result string.
 * It also has a more sensible argument ordering.
 */
static char *strcpyn(char *buf, size_t bufsize, const char *src) {
    size_t pos = 0;
    char *dest = buf;

    while (++pos < bufsize && *src) {
	*dest++ = *src++;
    }
    *dest = '\0';
    return buf;
}

int notify(int player, const char *msg);
int notify(int player, const char *msg) {
    (void)player;
    return printf("%s\n", msg);		
}

static struct hostcache {
    struct in6_addr ipnum_v6;
    long ipnum;
    char name[128];
    time_t time;
    struct hostcache *next;
    struct hostcache **prev;
} *hostcache_list = 0;

static void hostprune(void) {
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
	    free(ptr);
	    ptr = tmp;
	}
    }
}

static void make_nonblocking(int s) {
#if !defined(O_NONBLOCK) || defined(ULTRIX)
# ifdef FNDELAY			/* SUN OS */
#  define O_NONBLOCK FNDELAY
# else
#  ifdef O_NDELAY		/* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif			/* O_NDELAY */
# endif				/* FNDELAY */ 
#endif

    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
	perror("make_nonblocking: fcntl");
	abort();
    }
}

static int ipcmp(struct in6_addr *a, struct in6_addr *b) {
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

static void hostdel_v6(struct in6_addr *ip) {
    for (struct hostcache *ptr = hostcache_list; ptr; ptr = ptr->next) {
	if (!ipcmp(&(ptr->ipnum_v6), ip)) {
	    if (ptr->next) {
		ptr->next->prev = ptr->prev;
	    }
	    *ptr->prev = ptr->next;
	    free(ptr);
	    return;
	}
    }
}

static const char * hostfetch_v6(struct in6_addr *ip) {
    for (struct hostcache *ptr = hostcache_list; ptr; ptr = ptr->next) {
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

static void hostadd_v6(struct in6_addr *ip, const char *name) {
    struct hostcache *ptr;

    if (!(ptr = malloc(sizeof(struct hostcache))))
        abort();

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

static void hostadd_timestamp_v6(struct in6_addr *ip, const char *name) {
    hostadd_v6(ip, name);
    hostcache_list->time = time(NULL);
}

static const char * get_username_v6(struct in6_addr *a, in_port_t prt, in_port_t myprt) {
	int fd, result;
	socklen_t len;
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
	addr.sin6_port = htons(113);

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

	snprintf(buf, sizeof(buf), "%" PRIu16 ",%" PRIu16 "\n", prt, myprt);
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

	ptr = strchr(buf, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	if (*ptr)
		ptr++;
	if (strncmp(ptr, "USERID", 6))
		goto bad2;

	ptr = strchr(ptr, ':');
	if (!ptr)
		goto bad2;
	ptr = strchr(ptr + 1, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	/* if (*ptr) ptr++; */

	shutdown(fd, 2);
	close(fd);
	if ((ptr2 = strchr(ptr, '\r')))
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

/* addrout_v6 -- Translate address 'a' to text.  */
static const char * addrout_v6(struct in6_addr *a, in_port_t prt, in_port_t myprt) {
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
			snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", ptr, prt);
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
			snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", tmpbuf, prt);
		}
		return buf;
	}
	inet_ntop(AF_INET6, a, tmpbuf, 128);
	hostadd_timestamp_v6(a, tmpbuf);
	ptr = get_username_v6(a, prt, myprt);

	if (ptr) {
		snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
	} else {
		snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", tmpbuf, prt);
	}
	return buf;
}

static void hostdel(long ip) {
	for (struct hostcache *ptr = hostcache_list; ptr; ptr = ptr->next) {
		if (ptr->ipnum == ip) {
			if (ptr->next) {
				ptr->next->prev = ptr->prev;
			}
			*ptr->prev = ptr->next;
			free(ptr);
			return;
		}
	}
}

static const char * hostfetch(long ip) {
    for (struct hostcache *ptr = hostcache_list; ptr; ptr = ptr->next) {
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

static void hostadd(long ip, const char *name) {
    struct hostcache *ptr;

    if (!(ptr = malloc(sizeof(struct hostcache))))
        abort();

    ptr->next = hostcache_list;
    if (ptr->next) {
	ptr->next->prev = &ptr->next;
    }
    ptr->prev = &hostcache_list;
    hostcache_list = ptr;
    memset(&(ptr->ipnum_v6), 0, sizeof(struct in6_addr));
    ptr->ipnum = ip;
    ptr->time = 0;
    strcpyn(ptr->name, sizeof(ptr->name), name);
    hostprune();
}

static void hostadd_timestamp(long ip, const char *name) {
    hostadd(ip, name);
    hostcache_list->time = time(NULL);
}

void set_signals(void);

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

void set_signals(void) {
    /* we don't care about SIGPIPE, we notice it in select() and write() */
    signal(SIGPIPE, SIG_IGN);

    /* didn't manage to lose that control tty, did we? Ignore it anyway. */
    signal(SIGHUP, SIG_IGN);
}

static const char * get_username(in_addr_t a, in_port_t prt, in_port_t myprt) {
	int fd, result;
	socklen_t len;
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
	addr.sin_port = htons(113);

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

	snprintf(buf, sizeof(buf), "%" PRIu16 ",%" PRIu16 "\n", prt, myprt);
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

	ptr = strchr(buf, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	if (*ptr)
		ptr++;
	if (strncmp(ptr, "USERID", 6))
		goto bad2;

	ptr = strchr(ptr, ':');
	if (!ptr)
		goto bad2;
	ptr = strchr(ptr + 1, ':');
	if (!ptr)
		goto bad2;
	ptr++;
	/* if (*ptr) ptr++; */

	shutdown(fd, 2);
	close(fd);
	if ((ptr2 = strchr(ptr, '\r')))
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

/* addrout -- Translate address 'a' to text.  */
static const char * addrout(in_addr_t a, in_port_t prt, in_port_t myprt) {
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
			snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", ptr, prt);
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
			snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", tmpbuf, prt);
		}
		return buf;
	}

	a = ntohl(a);
	snprintf(tmpbuf, sizeof(tmpbuf), "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32, (a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff);
	hostadd_timestamp(a, tmpbuf);
	ptr = get_username(htonl(a), prt, myprt);

	if (ptr) {
		snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
	} else {
		snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", tmpbuf, prt);
	}
	return buf;
}

static volatile short shutdown_was_requested = 0;
static pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

static int do_resolve(void) {
	int ip1, ip2, ip3, ip4;
	in_port_t prt, myprt;
	int doagain;
	char *result;
	const char *ptr;
	char buf[1024];
	char outbuf[1024];
	char *bufptr = NULL;
	in_addr_t fullip;

	struct in6_addr fullip_v6;

	for (;;) {
		ip1 = ip2 = ip3 = ip4 = prt = myprt = -1;
		do {
			doagain = 0;
			*buf = '\0';

			/* lock input here. */
			if (pthread_mutex_lock(&input_mutex)) {
				return 0;
			}
			if (shutdown_was_requested) {
				/* unlock input here. */
				pthread_mutex_unlock(&input_mutex);
				return 0;
			}

			result = fgets(buf, sizeof(buf), stdin);

			/* detect if QUIT was requested before letting another thread start reading */
			if (!result) {
				if (errno == EAGAIN) {
					doagain = 1;
				} else {
					if (!feof(stdin)) {
						perror("fgets");
					}
					shutdown_was_requested = 1;
				}
			} else if (!strncmp("QUIT", buf, 4)) {
				shutdown_was_requested = 1;
				fclose(stdin);
			}

			/* unlock input here. */
			pthread_mutex_unlock(&input_mutex);

			if (shutdown_was_requested) {
				return 0;
			} else if (doagain) {
				sleep(1);
			}
		} while (doagain || !strcmp(buf, "\n"));

		bufptr = strchr(buf, ':');
		if (bufptr) {
			/* Is an IPv6 addr. */
			bufptr = strchr(buf, '(');
			if (!bufptr) {
				continue;
			}
			sscanf(bufptr, "(%" SCNu16 ")%" SCNu16, &prt, &myprt);
			*bufptr = '\0';
			if (myprt > 65535 || myprt < 0) {
				continue;
			}
			if (inet_pton(AF_INET6, buf, &fullip_v6) <= 0) {
				continue;
			}
			ptr = addrout_v6(&fullip_v6, prt, myprt);
			snprintf(outbuf, sizeof(outbuf), "%s(%" PRIu16 ")|%s", buf, prt, ptr);
		}

		if (!bufptr) {
			/* Is an IPv4 addr. */
			sscanf(buf, "%d.%d.%d.%d(%" SCNu16 ")%" SCNu16, &ip1, &ip2, &ip3, &ip4, &prt, &myprt);
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
			snprintf(outbuf, sizeof(outbuf), "%d.%d.%d.%d(%" PRIu16 ")|%s", ip1, ip2, ip3, ip4, prt, ptr);
		}

		/* lock output here. */
		if (pthread_mutex_lock(&output_mutex)) {
			return 0;
		}

		fprintf(stdout, "%s\n", outbuf);
		fflush(stdout);

		/* unlock output here. */
		pthread_mutex_unlock(&output_mutex);
	}
}

static void * resolver_thread_root(void *threadid) {
	do_resolve();
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	pthread_t threads[NUM_THREADS];

	if (argc > 1) {
		fprintf(stderr, "Usage: %s\n", *argv);
		exit(1);
	}

	/* remember to ignore certain signals */
	set_signals();

	/* go do it */
	for (long i = 0; i < NUM_THREADS; i++) {
		int rc = pthread_create(&threads[i], NULL, resolver_thread_root, (void *) i);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(2);
		}
	}
	for (long i = 0; i < NUM_THREADS; i++) {
		void *retval;
		pthread_join(threads[i], &retval);
	}

	fprintf(stderr, "Resolver exited.\n");

	exit(0);
}
