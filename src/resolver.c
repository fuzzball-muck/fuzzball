/** @file resolver.c
 *
 * This is the detachable host resolver process used by Fuzzball.  This is
 * because Fuzzball predates threads being commonplace, and non-blocking
 * host resolution.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

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

/*
 * TODO: These config parameters should probably be in config.h
 */

#define NUM_THREADS 5

/* Number of hostnames cached in an LRU queue */
#define HOST_CACHE_SIZE 8192

/*
 * Time before retrying to resolve a previously unresolved IP address.  1800
 * seconds == 30 minutes
 */
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

/*
 * TODO: I don't think this declaration is needed since the definition is
 *       right after it, and this is local to the file.
 *
 *       If this is being linked to another file that defines notify, then
 *       it would make since to declare ... but to declare + define?
 *       I think we can delete it.
 *
 *       In fact, 'notify' is not used in this file at all -- maybe it is
 *       needed for a define?  Maybe we should just remove it altogether?
 */
int notify(int player, const char *msg);

/**
 * This is a local version of the 'notify' method used by fuzzball
 *
 * It just is a wrapper around printf, probably to satisfy some dependency.
 *
 * @param player ignored
 * @param msg the message to display
 * @return the return value of printf
 */
int notify(int player, const char *msg) {
    (void)player;
    return printf("%s\n", msg);		
}

/*
 * An entry for the host cache.  This is a double linked list.
 */
static struct hostcache {
    struct in6_addr ipnum_v6;    /* IPv6 IP           */
    long ipnum;                  /* IPv4 IP           */
    char name[SMALL_BUFFER_LEN]; /* Hostname          */
    time_t time;                 /* Time we cached it */
    struct hostcache *next;      /* Next entry        */
    struct hostcache **prev;     /* Previous entry    */
} *hostcache_list = 0;

/**
 * Prune entries that exceed HOST_CACHE_SIZE in the host cache list
 *
 * This will expire the oldest items from the list, trimming off entries
 * until the list is HOST_CACHE_SIZE long.
 *
 * @private
 */
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

/**
 * Make a socket nonblocking
 *
 * This is a duplicate of make_nonblocking in interface.c in terms of
 * functionality, but not in terms of code.
 *
 * TODO: Can we share the code somehow rather than have this duplicate?
 *       The includes may be too complex because resolver.c might not link
 *       right with interface.c.  Let's take a look though, I hate to see
 *       a copy of a function like this.
 *
 * @private
 * @param s the socket to set nonblocking
 */
static void make_nonblocking(int s) {
#if !defined(O_NONBLOCK) || defined(ULTRIX)
# ifdef FNDELAY     /* SUN OS */
#  define O_NONBLOCK FNDELAY
# else
#  ifdef O_NDELAY   /* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif            /* O_NDELAY */
# endif             /* FNDELAY */ 
#endif

    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
        perror("make_nonblocking: fcntl");
        abort();
    }
}

/**
 * Compare two IPv6 IP addresses
 *
 * Returns 0 on a match.
 *
 * @private
 * @param a the first address to compare
 * @param b the second address to compare
 * @return 0 on a match, non-0 on failure
 */
static int ipcmp(struct in6_addr *a, struct in6_addr *b) {
    int i = SMALL_BUFFER_LEN;
    char *A = (char *) a;
    char *B = (char *) b;

    while (i) {
        i--;

        if (*A++ != *B++)
            break;
    }

    return i;
}

/**
 * Delete a host cache entry for the given IPv6 address
 *
 * @private
 * @param ip the IP to delete from the cache
 */
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

/**
 * Resolve an IPv6 address to a host name
 *
 * This will re-resolve a host if the cache entry is older than
 * EXPIRE_TIME.  Returns either a host name string, or NULL if
 * no host name was found.
 *
 * @private
 * @param ip the IPv6 address to look up
 * @return either a host name string or NULL on no match
 */
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

/**
 * Add an IPv6 and name pair to the cache
 *
 * @private
 * @param ip the IP to add to the cache
 * @param name the host name to add to the cache
 */
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

/**
 * Add an IPv6 and name pair to the cache, and set the timestamp
 *
 * Why this is separate, I'm not entirely sure; hostadd_v6 is called
 * seperately from this call for some reason, though it seems like
 * this and hostadd_v6 could be merged and the timestamp always set.
 *
 * TODO: Investiage if this and hostadd_v6 can be merged and timestamp
 *       always set -- I'm really not sure why we would ever want the
 *       timestamp to not be set.
 *
 * @see hostadd_v6
 *
 * @private
 * @param ip the IP to set
 * @param name the host name paired to the IP.
 */
static void hostadd_timestamp_v6(struct in6_addr *ip, const char *name) {
    hostadd_v6(ip, name);
    hostcache_list->time = time(NULL);
}

/**
 * Use the IDENT protocol to get username for IPv6 Address
 *
 * @TODO: This looks like it's a copy/paste of get_username just
 *        changed for IPv6.  We can probably unify this code easily enough.
 *        OR ... maybe we ditch the ident protocol?  I don't think it is
 *        in common use anymore.  Actually, that's my vote, get rid of
 *        ident.
 *
 * @private
 * @param a the address to connect to
 * @param prt the client port
 * @param myprt the local server port
 * @return user name string or NULL
 */
static const char * get_username_v6(struct in6_addr *a, in_port_t prt,
                                    in_port_t myprt) {
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

/**
 * Translate address 'a' to text.
 *
 * It will be a host name if possible, or an IP address if not.
 *
 * The format will be host(username) or host(client-side port)
 *
 * @private
 * @param a the address to convert
 * @param prt the client port
 * @param myprt the server-side local port
 * @return the host string
 */
static const char * addrout_v6(struct in6_addr *a, in_port_t prt,
                               in_port_t myprt) {
    static char buf[SMALL_BUFFER_LEN+7];
    char tmpbuf[SMALL_BUFFER_LEN+7];
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

    he = gethostbyaddr(&addr, sizeof(addr), AF_INET6);

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

    inet_ntop(AF_INET6, a, tmpbuf, SMALL_BUFFER_LEN);
    hostadd_timestamp_v6(a, tmpbuf);
    ptr = get_username_v6(a, prt, myprt);

    if (ptr) {
        snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
    } else {
        snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", tmpbuf, prt);
    }

    return buf;
}

/*
 * TODO: The IPv4 and IPv6 code has almost 100% overlap.
 *       We could make the cache code here share the same code and
 *       use wrapper functions to get the correct parameter type -- or
 *       do some other genericization.
 *
 *       There's a lot of other functions that are almost identical
 *       for both 4 and 6, more than is easy to note in a little blurb.
 */

/**
 * Delete a host associated with an IPv4 address from the cache
 *
 * @private
 * @param ip the IP to delete
 */
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

/**
 * Fetch a host from the cache for an IPv4
 *
 * @private
 * @param ip the IP to try and fetch from the cache
 * @return either a string host name or a NULL if not in the cache
 */
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

/**
 * Add an ipv4 host to the cache
 *
 * @private
 * @param ip the IP to add
 * @param name the host to map the IP to
 */
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

/**
 * Add an IPv4 host to the cache and set the current time
 *
 * TODO: Investiage if this and hostadd can be merged and timestamp
 *       always set -- I'm really not sure why we would ever want the
 *       timestamp to not be set.
 *
 * @private
 * @param ip the IP to add
 * @param name the hostname that belongs to that IP
 */
static void hostadd_timestamp(long ip, const char *name) {
    hostadd(ip, name);
    hostcache_list->time = time(NULL);
}

/*
 * TODO: I'm pretty sure we don't need this declaration
 */
void set_signals(void);

/**
 * Traps certain signals that we don't care about
 *
 * Specifically SIGPIPE which will happen all the time, and SIGHUP because
 * this is a detached process.
 *
 * @private
 */
void set_signals(void) {
    /* we don't care about SIGPIPE, we notice it in select() and write() */
    signal(SIGPIPE, SIG_IGN);

    /* didn't manage to lose that control tty, did we? Ignore it anyway. */
    signal(SIGHUP, SIG_IGN);
}

/*
 * The IP v4 version of get_username
 *
 * TODO: per the v6 documentation above, recommend we just trash this.
 *       IDENT isn't used anymore to any significant degree, or if it is,
 *       it is blocked by firewall.
 */
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

/**
 * Translate address 'a' to text.
 *
 * It will be a host name if possible, or an IP address if not.
 *
 * The format will be host(username) or host(client-side port)
 *
 * @private
 * @param a the address to convert
 * @param prt the client port
 * @param myprt the server-side local port
 * @return the host string
 */
static const char * addrout(in_addr_t a, in_port_t prt, in_port_t myprt) {
    static char buf[SMALL_BUFFER_LEN+7];
    char tmpbuf[SMALL_BUFFER_LEN+7];
    const char *ptr, *ptr2;
    struct hostent *he;
    struct in_addr addr;

    /*
     * TODO: This overlaps damn near 100% with ipv6 except for a handful
     *       of system calls that could be easly wrapped in if statements.
     */

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

    he = gethostbyaddr(&addr, sizeof(addr), AF_INET);

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
    snprintf(tmpbuf, sizeof(tmpbuf),
             "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32, (a >> 24) & 0xff,
             (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff);
    hostadd_timestamp(a, tmpbuf);
    ptr = get_username(htonl(a), prt, myprt);

    if (ptr) {
        snprintf(buf, sizeof(buf), "%s(%s)", tmpbuf, ptr);
    } else {
        snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", tmpbuf, prt);
    }

    return buf;
}

/**
 * @private
 * @var boolean true if we need to shut down the program, false otherwise
 */
static volatile short shutdown_was_requested = 0;

/**
 * @private
 * @var mutex for locking input from stdin (resolution requests)
 */
static pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @private
 * @var mutex for locking output to stdout (resolution responses)
 */
static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * The main loop for accepting resolution requests and performing DNS lookups
 *
 * This is basically the loop that powers the resolution threads.  The
 * resolver_thread_root function is a thin layer above this one.
 *
 * @see resolver_thread_root
 *
 * @private
 * @return int -- not really used for anything, does not always return
 */
static int do_resolve(void) {
    /*
     * TODO: change this to return void and just have the 'return 0's
     *       below just 'return'.  If you get to the bottom of this function,
     *       it actually returns NOTHING which is weird -- thankfully nothing
     *       pays attention to the return value of this function.
     */
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

        /* Read lines in from stdin */
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

            /* 
             * This will hang the thread while we wait on input, and will
             * deadlock the other threads.  This, however, is on purpose
             * so it essentially forces the threads to queue.
             */
            result = fgets(buf, sizeof(buf), stdin);

            /*
             * Detect if QUIT was requested before letting another thread
             * start reading.
             */
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

        /*
         * Input will be an IP address that is either IP v6 (and has : in
         * it which is why we use strchr : above) or IPv4 followed by
         * a port number in parens and then another port after the parens.
         *
         * The first port is the remote client port, the second port is
         * the local server port.
         */
        if (bufptr) {
            /* Is an IPv6 addr. */
            bufptr = strchr(buf, '(');

            if (!bufptr) {
                continue;
            }

            sscanf(bufptr, "(%" SCNu16 ")%" SCNu16, &prt, &myprt);
            *bufptr = '\0';

            if (inet_pton(AF_INET6, buf, &fullip_v6) <= 0) {
                continue;
            }

            ptr = addrout_v6(&fullip_v6, prt, myprt);
            snprintf(outbuf, sizeof(outbuf), "%s(%" PRIu16 ")|%s", buf, prt,
                     ptr);
        }

        /*
         * TODO: This should just be an else I think -- because 
         *       we will never want to fall into this block if the IPv6
         *       block runs.  There's no reason to do if bufptr then if !bufptr
         */
        if (!bufptr) {
            /* Is an IPv4 addr. */
            sscanf(buf, "%d.%d.%d.%d(%" SCNu16 ")%" SCNu16, &ip1, &ip2, &ip3,
                   &ip4, &prt, &myprt);

            if (ip1 < 0 || ip2 < 0 || ip3 < 0 || ip4 < 0) {
                continue;
            }

            if (ip1 > 255 || ip2 > 255 || ip3 > 255 || ip4 > 255) {
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

/**
 * Basic wrapper function that probably doesn't need to exist
 *
 * There's no particular reason to do it this way; do_resolve's calling
 * signature could be altered to match this and avoid the need for this
 * function.
 *
 * @TODO: refactor to remove the need for this function.
 *
 * @private
 * @param threadid unused but required by pthread library
 * @return broken - this should probably return NULL at least, but returns
 *         nothing
 */
static void * resolver_thread_root(void *threadid) {
    do_resolve();
    pthread_exit(NULL); /* You can just return, don't need this exit */
}

/**
 * Main function
 *
 * @param argc argument count
 * @param argv argument list
 * @return integer this function actually returns nothing
 */
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
