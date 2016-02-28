#include "config.h"
#include "match.h"
#include "mpi.h"

#include <sys/types.h>

#ifndef WIN32
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#endif

#include <fcntl.h>
#if defined (HAVE_ERRNO_H)
# include <errno.h>
#else
#if defined (HAVE_SYS_ERRNO_H)
# include <sys/errno.h>
#else
  extern int errno;
#endif
#endif
#include <ctype.h>

#ifndef WIN32
# define NEED_SOCKLEN_T
/* "do not include netinet6/in6.h directly, include netinet/in.h.  see RFC2553" */
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <arpa/inet.h>
#else
  typedef int socklen_t;
#endif

#ifdef AIX
# include <sys/select.h>
#endif

#ifdef USE_SSL
# ifdef HAVE_OPENSSL
#  include <openssl/ssl.h>
# else
#  include <ssl.h>
# endif
#endif

#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#ifdef MCP_SUPPORT
#include "mcp.h"
#include "mcpgui.h"
#endif
#include "externs.h"

int shutdown_flag = 0;
int restart_flag = 0;

static const char *connect_fail =
		"Either that player does not exist, or has a different password.\r\n";

static const char *create_fail =
		"Either there is already a player with that name, or that name is illegal.\r\n";

static const char *flushed_message = "<Output Flushed>\r\n";
static const char *shutdown_message = "\r\nGoing down - Bye\r\n";

int resolver_sock[2];

struct descriptor_data *descriptor_list = NULL;

#define MAX_LISTEN_SOCKS 16

/* Yes, both of these should start defaulted to disabled. */
/* If both are still disabled after arg parsing, we'll enable one or both. */
static int ipv4_enabled = 0;
static int ipv6_enabled = 0;

static int numports = 0;
static int numsocks = 0;
static int max_descriptor = 0;
static int listener_port[MAX_LISTEN_SOCKS];
static int sock[MAX_LISTEN_SOCKS];
#ifdef USE_IPV6
static int numsocks_v6 = 0;
static int sock_v6[MAX_LISTEN_SOCKS];
#endif
#ifdef USE_SSL
static int ssl_numports = 0;
static int ssl_numsocks = 0;
static int ssl_listener_port[MAX_LISTEN_SOCKS];
static int ssl_sock[MAX_LISTEN_SOCKS];
# ifdef USE_IPV6
static int ssl_numsocks_v6 = 0;
static int ssl_sock_v6[MAX_LISTEN_SOCKS];
# endif
SSL_CTX *ssl_ctx = NULL;
#endif

static int ndescriptors = 0;

void process_commands(void);
static void update_max_descriptor(int descr);
void shovechars();
void shutdownsock(struct descriptor_data *d);
struct descriptor_data *initializesock(int s, const char *hostname, int is_ssl);
void make_nonblocking(int s);
void freeqs(struct descriptor_data *d);
void welcome_user(struct descriptor_data *d);
void show_file(struct descriptor_data *d, const char *filename);
void check_connect(struct descriptor_data *d, const char *msg);
void close_sockets(const char *msg);

#ifdef USE_IPV6
const char *addrout_v6(int, struct in6_addr *, unsigned short);
int make_socket_v6(int);
struct descriptor_data *new_connection_v6(int port, int sock, int is_ssl);
#endif
const char *addrout(int, long, unsigned short);
int make_socket(int);
struct descriptor_data *new_connection(int port, int sock, int is_ssl);

void dump_users(struct descriptor_data *d, char *user);
void parse_connect(const char *msg, char *command, char *user, char *pass);
void set_userstring(char **userstring, const char *command);
int do_command(struct descriptor_data *d, char *command);
int is_interface_command(const char* cmd);
int send_keepalive(struct descriptor_data *d);
int queue_write(struct descriptor_data *, const char *, int);
int process_input(struct descriptor_data *d);
void announce_connect(int, dbref);
void announce_disconnect(struct descriptor_data *);
char *time_format_1(time_t);
void    init_descriptor_lookup();
void    init_descr_count_lookup();
void    remember_descriptor(struct descriptor_data *);
void    remember_player_descr(dbref player, int);
void    update_desc_count_table();
void    forget_player_descr(dbref player, int);
void    forget_descriptor(struct descriptor_data *);
struct descriptor_data *lookup_descriptor(int);
int online(dbref player);
int online_init(void);
dbref online_next(int *ptr);
#ifdef MUD_ID
void do_setuid(char *user);
#endif /* MUD_ID */
#ifdef MUD_GID
void do_setgid(char *group);
#endif /* MUD_GID */

#ifdef SPAWN_HOST_RESOLVER
void kill_resolver(void);
#endif

#ifdef USE_SSL /* SSL for NIX and WinFuzz */
ssize_t socket_read(struct descriptor_data *d, void *buf, size_t count);
ssize_t socket_write(struct descriptor_data *d, const void *buf, size_t count);
#else /* NOT SSL */
# ifndef WIN32 /* NOT WinFuzz */
#  define socket_write(d, buf, count) write(d->descriptor, buf, count)
#  define socket_read(d, buf, count) read(d->descriptor, buf, count)
# else /* WinFuzz */
#  define socket_write(d, buf, count) send(d->descriptor, buf, count,0)
#  define socket_read(d, buf, count) recv(d->descriptor, buf, count,0)
# endif
#endif

#ifdef USE_SSL
static SSL_CTX *configure_new_ssl_ctx(void);
int reconfigure_ssl(void);
#endif

void resolve_hostnames(void);

#define MALLOC(result, type, number) do {   \
                                       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
                                       panic("Out of memory");                             \
                                     } while (0)

#define FREE(x) (free((void *) x))

extern FILE *input_file;

short db_conversion_flag = 0;
short wizonly_mode = 0;
pid_t global_resolver_pid=0;
#ifndef DISKBASE
pid_t global_dumper_pid=0;
#endif
short global_dumpdone=0;


time_t sel_prof_start_time;
long sel_prof_idle_sec;
long sel_prof_idle_usec;
unsigned long sel_prof_idle_use;


void
show_program_usage(char *prog)
{
	fprintf(stderr, "Usage: %s [<options>] [infile [outfile [portnum [portnum ...]]]]\n", prog);
	fprintf(stderr, "    Arguments:\n");
	fprintf(stderr, "        infile           db file loaded at startup.  optional with -dbin.\n");
	fprintf(stderr, "        outfile          output db file to save to.  optional with -dbout.\n");
	fprintf(stderr, "        portnum          port num to listen for conns on. (16 ports max)\n");
	fprintf(stderr, "    Options:\n");
	fprintf(stderr, "        -dbin INFILE     use INFILE as the database to load at startup.\n");
	fprintf(stderr, "        -dbout OUTFILE   use OUTFILE as the output database to save to.\n");
	fprintf(stderr, "        -port NUMBER     sets the port number to listen for connections on.\n");
#ifdef USE_SSL
	fprintf(stderr, "        -sport NUMBER    sets the port number for secure connections\n");
#else
	fprintf(stderr, "        -sport NUMBER    Ignored.  SSL support isn't compiled in.\n");
#endif
	fprintf(stderr, "        -gamedir PATH    changes directory to PATH before starting up.\n");
	fprintf(stderr, "        -convert         load the db, then save and quit.\n");
	fprintf(stderr, "        -nosanity        don't do db sanity checks at startup time.\n");
	fprintf(stderr, "        -insanity        load db, then enter the interactive sanity editor.\n");
	fprintf(stderr, "        -sanfix          attempt to auto-fix a corrupt db after loading.\n");
	fprintf(stderr, "        -wizonly         only allow wizards to login.\n");
	fprintf(stderr, "        -godpasswd PASS  reset God(#1)'s password to PASS.  Implies -convert\n");
	fprintf(stderr, "        -ipv6            enable listening on ipv6 sockets.\n");
	fprintf(stderr, "        -version         display this server's version.\n");
	fprintf(stderr, "        -help            display this message.\n");
#ifdef WIN32
      fprintf(stderr, "        -freeconsole     free the console window and run in the background\n");
#endif
	exit(1);
}

/* NOTE: Will need to think about this more for unicode */
#define isinput( q ) isprint( (q) & 127 )

extern int sanity_violated;

int
main(int argc, char **argv)
{
	FILE *ffd;
	char *infile_name;
	char *outfile_name;
	char *num_one_new_passwd = NULL;
	int i, nomore_options;
	int sanity_skip;
	int sanity_interactive;
	int sanity_autofix;
	int val;
#ifdef WIN32
	int freeconsole = 0;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
#endif /* WIN32 */
#ifdef DEBUG
	/* This makes glibc's malloc abort() on heap errors. */
	if(strcmp(DoNull(getenv("MALLOC_CHECK_")),"2") != 0) {
		if(setenv("MALLOC_CHECK_","2",1) < 0) {
			fprintf(stderr, "[debug]setenv failed: out of memory");
			abort();
		}
		/* Make sure that the malloc checker really works by
			respawning it... though I'm not sure if C specifies
			that argv[argc] must be NULL? */
		{
			const char *argv2[argc+1];
			int i=0;
			for(i=0;i<argc;i++)
				argv2[i] = argv[i];
			argv2[argc] = NULL;
		execvp(argv[0],(char **)argv2);
		}
	}
#endif /* DEBUG */
	listener_port[0] = TINYPORT;

    init_descriptor_lookup();
    init_descr_count_lookup();

	nomore_options = 0;
	sanity_skip = 0;
	sanity_interactive = 0;
	sanity_autofix = 0;
	infile_name = NULL;
	outfile_name = NULL;

	for (i = 1; i < argc; i++) {
		if (!nomore_options && argv[i][0] == '-') {
			if (!strcmp(argv[i], "-convert")) {
				db_conversion_flag = 1;
			} else if (!strcmp(argv[i], "-nosanity")) {
				sanity_skip = 1;
			} else if (!strcmp(argv[i], "-insanity")) {
				sanity_interactive = 1;
			} else if (!strcmp(argv[i], "-wizonly")) {
				wizonly_mode = 1;
			} else if (!strcmp(argv[i], "-sanfix")) {
				sanity_autofix = 1;
#ifdef WIN32
                  } else if (!strcmp(argv[i], "-freeconsole")) {
                        freeconsole = 1;
#endif
			} else if (!strcmp(argv[i], "-version")) {
				printf("%s\n", VERSION);
				exit(0);
			} else if (!strcmp(argv[i], "-dbin")) {
				if (i + 1 >= argc) {
					show_program_usage(*argv);
				}
				infile_name = argv[++i];

			} else if (!strcmp(argv[i], "-dbout")) {
				if (i + 1 >= argc) {
					show_program_usage(*argv);
				}
				outfile_name = argv[++i];

			} else if (!strcmp(argv[i], "-ipv4")) {
				ipv4_enabled = 1;

			} else if (!strcmp(argv[i], "-ipv6")) {
#ifdef USE_IPV6
				ipv6_enabled = 1;
#else
				fprintf(stderr, "-ipv6: This server isn't configured to enable IPv6.  Sorry.\n");
				exit(1);
#endif

			} else if (!strcmp(argv[i], "-godpasswd")) {
				if (i + 1 >= argc) {
					show_program_usage(*argv);
				}
				num_one_new_passwd = argv[++i];
				if (!ok_password(num_one_new_passwd)) {
					fprintf(stderr, "Bad -godpasswd password.\n");
					exit(1);
				}
				db_conversion_flag = 1;

			} else if (!strcmp(argv[i], "-port")) {
				if (i + 1 >= argc) {
					show_program_usage(*argv);
				}
#ifdef USE_SSL
				if ( (ssl_numports + numports) < MAX_LISTEN_SOCKS) {
					listener_port[numports++] = atoi(argv[++i]);
				}
#else
				if (numports < MAX_LISTEN_SOCKS) {
					listener_port[numports++] = atoi(argv[++i]);
				}
#endif
			} else if (!strcmp(argv[i], "-sport")) {
				if (i + 1 >= argc) {
					show_program_usage(*argv);
				}
#ifdef USE_SSL
				if ( (ssl_numports + numports) < MAX_LISTEN_SOCKS) {
					ssl_listener_port[ssl_numports++] = atoi(argv[++i]);
				}
#else
				i++;
				fprintf(stderr, "-sport: This server isn't configured to enable SSL.  Sorry.\n");
				exit(1);
#endif
			} else if (!strcmp(argv[i], "-gamedir")) {
				if (i + 1 >= argc) {
					show_program_usage(*argv);
				}
				if (chdir(argv[++i])) {
					perror("cd to gamedir");
					exit(4);
				}

			} else if (!strcmp(argv[i], "--")) {
				nomore_options = 1;
			} else {
				show_program_usage(*argv);
			}
		} else {
			if (!infile_name) {
				infile_name = argv[i];
			} else if (!outfile_name) {
				outfile_name = argv[i];
			} else {
				val = atoi(argv[i]);
				if (val < 1 || val > 65535) {
					show_program_usage(*argv);
				}
#ifdef USE_SSL
				if ( (ssl_numports + numports) < MAX_LISTEN_SOCKS) {
					listener_port[numports++] = val;
				}
#else
				if (numports < MAX_LISTEN_SOCKS) {
					listener_port[numports++] = val;
				}
#endif
			}
		}
	}
	if (numports < 1) {
		numports = 1;
	}
	if (!infile_name || !outfile_name) {
		show_program_usage(*argv);
	}

#ifdef USE_IPV6
	if (!ipv4_enabled && !ipv6_enabled) {
	    /* No -ipv4 or -ipv6 flags given.  Default to enabling both. */
	    ipv4_enabled = 1;
	    ipv6_enabled = 1;
	}
#else
	/* If IPv6 isn't available always enable IPv4. */
	ipv4_enabled = 1;
	ipv6_enabled = 0;
#endif

#ifdef DISKBASE
	if (!strcmp(infile_name, outfile_name)) {
		fprintf(stderr, "Output file must be different from the input file.");
		exit(3);
	}
#endif

	if (!sanity_interactive) {

                log_status("INIT: TinyMUCK %s starting.", "version");

#ifdef DETACH
# ifndef WIN32
		/* Go into the background unless requested not to */
		if (!sanity_interactive && !db_conversion_flag) {
			fclose(stdin);
			fclose(stdout);
			fclose(stderr);
			if (fork() != 0)
				_exit(0);
		}
# endif
#endif

#ifdef WIN32
		if (!sanity_interactive && !db_conversion_flag && freeconsole) {
			fclose(stdin);
			fclose(stdout);
			fclose(stderr);
                  FreeConsole();
		} else {
               FreeConsole();
               AllocConsole();
            }
#endif

		/* save the PID for future use */
		if ((ffd = fopen(PID_FILE, "wb")) != NULL) {
			fprintf(ffd, "%d\n", getpid());
			fclose(ffd);
		}
		log_status("%s PID is: %d", argv[0], getpid());


#ifdef DETACH
# ifndef WIN32
		if (!sanity_interactive && !db_conversion_flag) {
			/* Detach from the TTY, log whatever output we have... */
			freopen(LOG_ERR_FILE, "a", stderr);
			setbuf(stderr, NULL);
			freopen(LOG_FILE, "a", stdout);
			setbuf(stdout, NULL);

			/* Disassociate from Process Group */
#  ifdef _POSIX_SOURCE
			setsid();
#  else
#   ifdef SYSV
			setpgrp();			/* The SysV way */
#   else
			setpgid(0, getpid());	/* The POSIX way. */
#   endif						/* SYSV */

#   ifdef  TIOCNOTTY				/* we can force this, POSIX / BSD */
			{
				int fd;
				if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
					ioctl(fd, TIOCNOTTY, (char *) 0);	/* lose controll TTY */
					close(fd);
				}
			}
#   endif						/* TIOCNOTTY */
#  endif							/* !_POSIX_SOURCE */
		}
# endif /* WIN32 */
#endif							/* DETACH */

#ifdef WIN32
		if (!sanity_interactive && !db_conversion_flag && freeconsole) {
			freopen(LOG_ERR_FILE, "a", stderr);
			setbuf(stderr, NULL);
			freopen(LOG_FILE, "a", stdout);
			setbuf(stdout, NULL);
            }
#endif
	}



/*
 * You have to change gid first, since setgid() relies on root permissions
 * if you don't call initgroups() -- and since initgroups() isn't standard,
 * I'd rather assume the user knows what he's doing.
*/

#ifdef MUD_GID
	if (!sanity_interactive) {
		do_setgid(MUD_GID);
	}
#endif							/* MUD_GID */
#ifdef MUD_ID
	if (!sanity_interactive) {
		do_setuid(MUD_ID);
	}
#endif							/* MUD_ID */

	/* Have to do this after setting GID/UID, else we can't kill
	 * the bloody thing... */
#ifdef SPAWN_HOST_RESOLVER
	if (!db_conversion_flag) {
		spawn_resolver();
	}
#endif

#ifdef MCP_SUPPORT
	/* Initialize MCP and some packages. */
	mcp_initialize();
	gui_initialize();
#endif
    sel_prof_start_time = time(NULL); /* Set useful starting time */
    sel_prof_idle_sec = 0;
    sel_prof_idle_usec = 0;
    sel_prof_idle_use = 0;

	if (init_game(infile_name, outfile_name) < 0) {
		fprintf(stderr, "Couldn't load %s!\n", infile_name);
		exit(2);
	}

	if (num_one_new_passwd != NULL) {
		set_password(GOD, num_one_new_passwd);
	}

	if (!sanity_interactive && !db_conversion_flag) {
		set_signals();

		if (!sanity_skip) {
			sanity(AMBIGUOUS);
			if (sanity_violated) {
				wizonly_mode = 1;
				if (sanity_autofix) {
					sanfix(AMBIGUOUS);
				}
			}
		}


#ifdef WIN32
	wVersionRequested = MAKEWORD( 2, 0 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		perror("Unable to start socket layer");
		return 1;
	}
 
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 0 ) {
		perror("Winsock 2.0 or later is required to run this application.");
		WSACleanup( );
		return 1; 
	}

	set_console(); 
#endif

		/* go do it */
		shovechars();

		if (restart_flag) {
			close_sockets("\r\nServer restarting.  Try logging back on in a few minutes.\r\n");
		} else {
			close_sockets("\r\nServer shutting down normally.\r\n");
		}

		do_dequeue(-1, (dbref) 1, "all");

#ifdef WIN32
		WSACleanup();
#endif

	}

	if (sanity_interactive) {
		san_main();
	} else {
		dump_database();

#ifdef SPAWN_HOST_RESOLVER
		kill_resolver();
#endif

#ifdef MALLOC_PROFILING
		db_free();
		free_old_macros();
		purge_all_free_frames();
		purge_timenode_free_pool();
		purge_for_pool();
		purge_for_pool(); /* have to do this a second time to purge all */
		purge_try_pool();
		purge_try_pool(); /* have to do this a second time to purge all */
		purge_mfns();
		cleanup_game();
		tune_freeparms();
#endif

#ifdef DISKBASE
		fclose(input_file);
#endif

#ifdef MALLOC_PROFILING
		CrT_summarize_to_file("malloc_log", "Shutdown");
#endif

		if (restart_flag) {
#ifndef WIN32
			char **argslist;
			char numbuf[32];
			int argcnt = numports + 2;
			int argnum = 1;

#ifdef USE_SSL
			argcnt += ssl_numports;
#endif

			argslist = (char**)calloc(argcnt, sizeof(char*));

			for (i = 0; i < numports; i++) {
				int alen = strlen(numbuf)+1;
				snprintf(numbuf, sizeof(numbuf), "%d", listener_port[i]);
				argslist[argnum] = (char*)malloc(alen);
				strcpyn(argslist[argnum++], alen, numbuf);
			}

#ifdef USE_SSL
			for (i = 0; i < ssl_numports; i++) {
				int alen = strlen(numbuf)+1;
				snprintf(numbuf, sizeof(numbuf), "-sport %d", ssl_listener_port[i]);
				argslist[argnum] = (char*)malloc(alen);
				strcpyn(argslist[argnum++], alen, numbuf);
			}
#endif

			if (!fork()) {
				argslist[0] = "./restart";
				execv(argslist[0], argslist);

				argslist[0] = "restart";
				execv(argslist[0], argslist);

				fprintf(stderr, "Could not find restart script!\n");
			}
#else	/* WIN32 */
			char* argbuf[1];
			argbuf[0] = NULL;
			execv("restart", argbuf);
#endif	/* WIN32 */
		}
	}

	exit(0);
	return 0;
}

int
queue_ansi(struct descriptor_data *d, const char *msg)
{
	char buf[BUFFER_LEN + 8];

	if (d->connected) {
		if (FLAGS(d->player) & CHOWN_OK) {
			strip_bad_ansi(buf, msg);
		} else {
			strip_ansi(buf, msg);
		}
	} else {
		strip_ansi(buf, msg);
	}
#ifdef MCP_SUPPORT
	mcp_frame_output_inband(&d->mcpframe, buf);
#else
	queue_string(d, buf);
#endif
	return strlen(buf);
	/* return queue_string(d, buf); */
}


int notify_nolisten_level = 0;

int
notify_nolisten(dbref player, const char *msg, int isprivate)
{
	int retval = 0;
	char buf[BUFFER_LEN + 2];
	char buf2[BUFFER_LEN + 2];
	int firstpass = 1;
	char *ptr1;
	const char *ptr2;
	dbref ref;
    int di;
    int* darr;
    int dcount;

	ptr2 = msg;
	while (ptr2 && *ptr2) {
		ptr1 = buf;
		while (ptr2 && *ptr2 && *ptr2 != '\r')
			*(ptr1++) = *(ptr2++);
		*(ptr1++) = '\r';
		*(ptr1++) = '\n';
		*(ptr1++) = '\0';
		if (*ptr2 == '\r')
			ptr2++;

		darr = get_player_descrs(player, &dcount);
        for (di = 0; di < dcount; di++) {
            queue_ansi(descrdata_by_descr(darr[di]), buf);
            if (firstpass) retval++;
        }

		if (tp_zombies) {
			if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
				!(FLAGS(OWNER(player)) & ZOMBIE) &&
				(!(FLAGS(player) & DARK) || Wizard(OWNER(player)))) {
				ref = LOCATION(player);
				if (Wizard(OWNER(player)) || ref == NOTHING ||
					Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {
					if (isprivate || LOCATION(player) != LOCATION(OWNER(player))) {
						char pbuf[BUFFER_LEN];
						const char *prefix;
						char ch = *match_args;

						*match_args = '\0';

						if (notify_nolisten_level <= 0)
						{
							notify_nolisten_level++;

							prefix = do_parse_prop(-1, player, player, MESGPROP_PECHO, "(@Pecho)", pbuf, sizeof(pbuf), MPI_ISPRIVATE);

							notify_nolisten_level--;
						}
						else
							prefix = 0;

						*match_args = ch;

						if (!prefix || !*prefix) {
							prefix = NAME(player);
							snprintf(buf2, sizeof(buf2), "%s> %.*s", prefix,
									(int)(BUFFER_LEN - (strlen(prefix) + 3)), buf);
						} else {
							snprintf(buf2, sizeof(buf2), "%s %.*s", prefix,
									(int)(BUFFER_LEN - (strlen(prefix) + 2)), buf);
						}

						darr = get_player_descrs(OWNER(player), &dcount);
                        for (di = 0; di < dcount; di++) {
                            queue_ansi(descrdata_by_descr(darr[di]), buf2);
                            if (firstpass) retval++;
                        }
					}
				}
			}
		}
		firstpass = 0;
	}

	return retval;
}

int
notify_filtered(dbref from, dbref player, const char *msg, int isprivate)
{
	if ((msg == 0) || ignore_is_ignoring(player, from))
		return 0;
	return notify_nolisten(player, msg, isprivate);
}

int
notify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
	const char *ptr=msg;

	if (tp_listeners) {
		if (tp_listeners_obj || Typeof(player) == TYPE_ROOM) {
			listenqueue(-1, from, LOCATION(from), player, player, NOTHING,
						"_listen", ptr, tp_listen_mlev, 1, 0);
			listenqueue(-1, from, LOCATION(from), player, player, NOTHING,
						"~listen", ptr, tp_listen_mlev, 1, 1);
			listenqueue(-1, from, LOCATION(from), player, player, NOTHING,
						"~olisten", ptr, tp_listen_mlev, 0, 1);
		}
	}

	if (Typeof(player) == TYPE_THING && (FLAGS(player) & VEHICLE) &&
		(!(FLAGS(player) & DARK) || Wizard(OWNER(player)))
			) {
		dbref ref;

		ref = LOCATION(player);
		if (Wizard(OWNER(player)) || ref == NOTHING ||
			Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & VEHICLE)
				) {
			if (!isprivate && LOCATION(from) == LOCATION(player)) {
				char buf[BUFFER_LEN];
				char pbuf[BUFFER_LEN];
				const char *prefix;
				char ch = *match_args;

				*match_args = '\0';
				prefix = do_parse_prop(-1, from, player, MESGPROP_OECHO, "(@Oecho)", pbuf, sizeof(pbuf), MPI_ISPRIVATE);
				*match_args = ch;

				if (!prefix || !*prefix)
					prefix = "Outside>";
				snprintf(buf, sizeof(buf), "%s %.*s", prefix, (int)(BUFFER_LEN - (strlen(prefix) + 2)), msg);
				ref = CONTENTS(player);
				while (ref != NOTHING) {
					notify_filtered(from, ref, buf, isprivate);
					ref = NEXTOBJ(ref);
				}
			}
		}
	}

	return notify_filtered(from, player, msg, isprivate);
}

int
notify_from(dbref from, dbref player, const char *msg)
{
	return notify_from_echo(from, player, msg, 1);
}

int
notify(dbref player, const char *msg)
{
	return notify_from_echo(player, player, msg, 1);
}


struct timeval
timeval_sub(struct timeval now, struct timeval then)
{
	now.tv_sec -= then.tv_sec;
	now.tv_usec -= then.tv_usec;
	if (now.tv_usec < 0) {
		now.tv_usec += 1000000;
		now.tv_sec--;
	}
	return now;
}

int
msec_diff(struct timeval now, struct timeval then)
{
	return ((now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec) / 1000);
}

struct timeval
msec_add(struct timeval t, int x)
{
	t.tv_sec += x / 1000;
	t.tv_usec += (x % 1000) * 1000;
	if (t.tv_usec >= 1000000) {
		t.tv_sec += t.tv_usec / 1000000;
		t.tv_usec = t.tv_usec % 1000000;
	}
	return t;
}

struct timeval
update_quotas(struct timeval last, struct timeval current)
{
	int nslices;
	int cmds_per_time;
	struct descriptor_data *d;

	nslices = msec_diff(current, last) / tp_command_time_msec;

	if (nslices > 0) {
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected) {
				cmds_per_time = ((FLAGS(d->player) & INTERACTIVE)
								 ? (tp_commands_per_time * 8) : tp_commands_per_time);
			} else {
				cmds_per_time = tp_commands_per_time;
			}
			d->quota += cmds_per_time * nslices;
			if (d->quota > tp_command_burst_size)
				d->quota = tp_command_burst_size;
		}
	}
	return msec_add(last, nslices * tp_command_time_msec);
}

/*
 * long max_open_files()
 *
 * This returns the max number of files you may have open
 * as a long, and if it can use setrlimit() to increase it,
 * it will do so.
 *
 * Becuse there is no way to just "know" if get/setrlimit is
 * around, since its defs are in <sys/resource.h>, you need to
 * define USE_RLIMIT in config.h to attempt it.
 *
 * Otherwise it trys to use sysconf() (POSIX.1) or getdtablesize()
 * to get what is avalible to you.
 */
#ifdef HAVE_RESOURCE_H
# include <sys/resource.h>
#endif

#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
# define USE_RLIMIT
#endif

long
max_open_files(void)
{
#if defined(_SC_OPEN_MAX) && !defined(USE_RLIMIT)	/* Use POSIX.1 method, sysconf() */
/*
 * POSIX.1 code.
 */
	return sysconf(_SC_OPEN_MAX);
#else							/* !POSIX */
# if defined(USE_RLIMIT) && (defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE))
#  ifndef RLIMIT_NOFILE
#   define RLIMIT_NOFILE RLIMIT_OFILE	/* We Be BSD! */
#  endif						/* !RLIMIT_NOFILE */
/*
 * get/setrlimit() code.
 */
	struct rlimit file_limit;

	getrlimit(RLIMIT_NOFILE, &file_limit);	/* Whats the limit? */

	if (file_limit.rlim_cur < file_limit.rlim_max) {	/* if not at max... */
		file_limit.rlim_cur = file_limit.rlim_max;	/* ...set to max. */
		setrlimit(RLIMIT_NOFILE, &file_limit);

		getrlimit(RLIMIT_NOFILE, &file_limit);	/* See what we got. */
	}

	return (long) file_limit.rlim_cur;

# elif WIN32					/* !RLIMIT && WIN32 */
	return FD_SETSIZE;
# else							/* !RLIMIT && !WIN32 */
/*
 * Don't know what else to do, try getdtablesize().
 * email other bright ideas to me. :) (whitefire)
 */
	return (long) getdtablesize();
# endif							/* !RLIMIT */
#endif							/* !POSIX */
}

int
queue_immediate(struct descriptor_data *d, const char *msg)
{
	char buf[BUFFER_LEN + 8];
	int quote_len = 0;

	if (d->connected) {
		if (FLAGS(d->player) & CHOWN_OK) {
			strip_bad_ansi(buf, msg);
		} else {
			strip_ansi(buf, msg);
		}
	} else {
		strip_ansi(buf, msg);
	}

#ifdef MCP_SUPPORT
	if (d->mcpframe.enabled && !(strncmp(buf, MCP_MESG_PREFIX, 3) && strncmp(buf, MCP_QUOTE_PREFIX, 3)))
	{
		quote_len = strlen(MCP_QUOTE_PREFIX);
		socket_write(d, MCP_QUOTE_PREFIX, quote_len);
	}
#endif
	return socket_write(d, buf, strlen(buf)) + quote_len;
}

void
goodbye_user(struct descriptor_data *d)
{
	queue_immediate(d, "\r\n");
	queue_immediate(d, tp_leave_mesg);
	queue_immediate(d, "\r\n\r\n");
}

void
idleboot_user(struct descriptor_data *d)
{
	queue_immediate(d, "\r\n");
	queue_immediate(d, tp_idle_mesg);
	queue_immediate(d, "\r\n\r\n");
	d->booted = 1;
}

#ifdef USE_SSL
int pem_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
	const char *pw = (const char*)userdata;
	int pwlen = strlen(pw);
	strncpy(buf, pw, size);
	return ((pwlen > size)? size : pwlen);
}
#endif

static int con_players_max = 0;	/* one of Cynbe's good ideas. */
static int con_players_curr = 0;	/* for playermax checks. */
extern void purge_free_frames(void);

static void update_max_descriptor(int descr) {
    if (descr >= max_descriptor) {
        max_descriptor = descr + 1;
    }
}

void
shovechars()
{
	fd_set input_set, output_set;
	time_t now;
	long tmptq;
	struct timeval last_slice, current_time;
	struct timeval next_slice;
	struct timeval timeout, slice_timeout;
	int cnt;
	struct descriptor_data *d, *dnext;
	struct descriptor_data *newd;
	struct timeval sel_in, sel_out;
	int avail_descriptors;
	int i;


	if (ipv4_enabled) {
		for (i = 0; i < numports; i++) {
			sock[i] = make_socket(listener_port[i]);
                        update_max_descriptor(sock[i]);
			numsocks++;
		}
	}
#ifdef USE_IPV6
	if (ipv6_enabled) {
		for (i = 0; i < numports; i++) {
			sock_v6[i] = make_socket_v6(listener_port[i]);
                        update_max_descriptor(sock_v6[i]);
			numsocks_v6++;
		}
	}
#endif

#ifdef USE_SSL
	SSL_load_error_strings ();
 	OpenSSL_add_ssl_algorithms (); 

        (void) reconfigure_ssl();
#endif
 
	gettimeofday(&last_slice, (struct timezone *) 0);

	avail_descriptors = max_open_files() - 5;

	(void) time(&now);

/* And here, we do the actual player-interaction loop */

	while (shutdown_flag == 0) {
		gettimeofday(&current_time, (struct timezone *) 0);
		last_slice = update_quotas(last_slice, current_time);

		next_muckevent();
		process_commands();
		muf_event_process();
#ifdef WIN32
/*		check_console();*/ /* Handle possible CTRL+C */
#endif

		for (d = descriptor_list; d; d = dnext) {
			dnext = d->next;
			if (d->booted) {
				process_output(d);
				if (d->booted == 2) {
					goodbye_user(d);
				}
				d->booted = 0;
				process_output(d);
				shutdownsock(d);
			}
		}
		if (global_dumpdone != 0) {
			if (tp_dumpdone_warning) {
				wall_and_flush(tp_dumpdone_mesg);
			}
			global_dumpdone = 0;
		}
		purge_free_frames();
		untouchprops_incremental(1);

		if (shutdown_flag)
			break;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		next_slice = msec_add(last_slice, tp_command_time_msec);
		slice_timeout = timeval_sub(next_slice, current_time);

		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		if (ndescriptors < avail_descriptors) {
			for (i = 0; i < numsocks; i++) {
				FD_SET(sock[i], &input_set);
			}
#ifdef USE_IPV6
			for (i = 0; i < numsocks_v6; i++) {
				FD_SET(sock_v6[i], &input_set);
			}
#endif

#ifdef USE_SSL
			for (i = 0; i < ssl_numsocks; i++) {
				FD_SET(ssl_sock[i], &input_set);
			}
# ifdef USE_IPV6
			for (i = 0; i < ssl_numsocks_v6; i++) {
				FD_SET(ssl_sock_v6[i], &input_set);
			}
# endif
#endif
		}
		for (d = descriptor_list; d; d = d->next) {
			if (d->input.lines > 100)
				timeout = slice_timeout;
			else
				FD_SET(d->descriptor, &input_set);

#ifdef USE_SSL
			if (d->output.head && !d->block_writes) {
				/*
				 * If SSL isn't already in place, give TELNET STARTTLS
				 * handshaking a couple seconds to respond, to start it.
				 */
				const time_t welcome_pause = 2; /* seconds */
				time_t timeon = now - d->connected_at;

				if (d->ssl_session || !tp_starttls_allow) {
					FD_SET(d->descriptor, &output_set);
				} else if (timeon >= welcome_pause) {
					FD_SET(d->descriptor, &output_set);
				} else {
					if (timeout.tv_sec > welcome_pause - timeon) {
						timeout.tv_sec = welcome_pause - timeon;
						timeout.tv_usec = 10;  /* 10 msecs min.  Arbitrary. */
					}
				}
			}

			if (d->ssl_session) {
				/* SSL may want to write even if the output queue is empty */
				if ( ! SSL_is_init_finished(d->ssl_session) ) {
					/* log_status("SSL : Init not finished.\n", "version"); */
					FD_CLR(d->descriptor, &output_set);
					FD_SET(d->descriptor, &input_set);
				} 
				if ( SSL_want_write(d->ssl_session) ) {
					/* log_status("SSL : Need write.\n", "version"); */
					FD_SET(d->descriptor, &output_set);
				}
			}
#else
			if (d->output.head && !d->block_writes) {
				FD_SET(d->descriptor, &output_set);
			}
#endif

		}
#ifdef SPAWN_HOST_RESOLVER
		FD_SET(resolver_sock[1], &input_set);
#endif

		tmptq = next_muckevent_time();
		if ((tmptq >= 0L) && (timeout.tv_sec > tmptq)) {
			timeout.tv_sec = tmptq + (tp_pause_min / 1000);
			timeout.tv_usec = (tp_pause_min % 1000) * 1000L;
		}
		gettimeofday(&sel_in,NULL);
#ifndef WIN32
		if (select(max_descriptor, &input_set, &output_set, (fd_set *) 0, &timeout) < 0) {
			if (errno != EINTR) {
				perror("select");
				return;
			}
#else
		if (select(max_descriptor, &input_set, &output_set, (fd_set *) 0, &timeout) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEINTR) {
				perror("select");
				return;
			}
#endif
		} else {
			gettimeofday(&sel_out,NULL);
			if (sel_out.tv_usec < sel_in.tv_usec) {
				sel_out.tv_usec += 1000000;
				sel_out.tv_sec -= 1;
			}
			sel_out.tv_usec -= sel_in.tv_usec;
			sel_out.tv_sec -= sel_in.tv_sec;
			sel_prof_idle_sec += sel_out.tv_sec;
			sel_prof_idle_usec += sel_out.tv_usec;
			if (sel_prof_idle_usec >= 1000000) {
				sel_prof_idle_usec -= 1000000;
				sel_prof_idle_sec += 1;
			}
			sel_prof_idle_use++;
			(void) time(&now);
			for (i = 0; i < numsocks; i++) {
				if (FD_ISSET(sock[i], &input_set)) {
					if (!(newd = new_connection(listener_port[i], sock[i], 0))) {
#ifndef WIN32
						if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
							perror("new_connection");
							/* return; */
						}
#else	/* WIN32 */
						if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
							perror("new_connection");
							/* return; */
						}
#endif	/* WIN32 */
					} else {
                                                update_max_descriptor(newd->descriptor);
					}
				}
			}
#ifdef USE_IPV6
			for (i = 0; i < numsocks_v6; i++) {
				if (FD_ISSET(sock_v6[i], &input_set)) {
					if (!(newd = new_connection_v6(listener_port[i], sock_v6[i], 0))) {
# ifndef WIN32
						if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
							perror("new_connection");
							/* return; */
						}
# else	/* WIN32 */
						if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
							perror("new_connection");
							/* return; */
						}
# endif	/* WIN32 */
					} else {
                                                update_max_descriptor(newd->descriptor);
					}
				}
			}
#endif
#ifdef USE_SSL
			for (i = 0; i < ssl_numsocks; i++) {
				if (FD_ISSET(ssl_sock[i], &input_set)) {
					if (!(newd = new_connection(ssl_listener_port[i], ssl_sock[i], 1))) {
# ifndef WIN32
						if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
							perror("new_connection");
							/* return; */
						}
# else
						if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
							perror("new_connection");
							/* return; */
						}
# endif
					} else {
                                                update_max_descriptor(newd->descriptor);
						newd->ssl_session = SSL_new(ssl_ctx);
						SSL_set_fd(newd->ssl_session, newd->descriptor);
						cnt = SSL_accept(newd->ssl_session);
						/* log_status("SSL accept1: %i\n", cnt ); */
					}
				}
			}
# ifdef USE_IPV6
			for (i = 0; i < ssl_numsocks_v6; i++) {
				if (FD_ISSET(ssl_sock_v6[i], &input_set)) {
					if (!(newd = new_connection_v6(ssl_listener_port[i], ssl_sock_v6[i], 1))) {
#  ifndef WIN32
						if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
							perror("new_connection");
							/* return; */
						}
#  else
						if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
							perror("new_connection");
							/* return; */
						}
#  endif
					} else {
                                                update_max_descriptor(newd->descriptor);
						newd->ssl_session = SSL_new(ssl_ctx);
						SSL_set_fd(newd->ssl_session, newd->descriptor);
						cnt = SSL_accept(newd->ssl_session);
						/* log_status("SSL accept1: %i\n", cnt ); */
					}
				}
			}
# endif
#endif
#ifdef SPAWN_HOST_RESOLVER
			if (FD_ISSET(resolver_sock[1], &input_set)) {
				resolve_hostnames();
			}
#endif
			for (cnt = 0, d = descriptor_list; d; d = dnext) {
				dnext = d->next;
				if (FD_ISSET(d->descriptor, &input_set)
#ifdef USE_SSL
				 || (d->ssl_session && SSL_pending(d->ssl_session))
#endif
				) {
					if (!process_input(d)) {
						d->booted = 1;
					}
				}
				if (FD_ISSET(d->descriptor, &output_set)) {
					if (!process_output(d)) {
						d->booted = 1;
					}
				}
				if (d->connected) {
					cnt++;
					if (tp_idleboot && ((now - d->last_time) > tp_maxidle) &&
						!Wizard(d->player)) {
						idleboot_user(d);
					}
				} else {
					/* Hardcode 300 secs -- 5 mins -- at the login screen */
					if ((now - d->connected_at) > 300) {
						log_status("connection screen: connection timeout 300 secs");
						d->booted = 1;
					}
				}
				if ( d->connected && tp_idle_ping_enable && (tp_idle_ping_time > 0) && ((now - d->last_pinged_at) > tp_idle_ping_time) ) {
					const char *tmpptr = get_property_class( d->player, NO_IDLE_PING_PROP);
					if( !tmpptr && !send_keepalive(d)) {
						d->booted = 1;
					}
				}
			}
			if (cnt > con_players_max) {
				add_property((dbref) 0, "_sys/max_connects", NULL, cnt);
				con_players_max = cnt;
			}
			con_players_curr = cnt;
		}
	}

	/* End of the player processing loop */

	(void) time(&now);
	add_property((dbref) 0, "_sys/lastdumptime", NULL, (int) now);
	add_property((dbref) 0, "_sys/shutdowntime", NULL, (int) now);
}


void
wall_and_flush(const char *msg)
{
	struct descriptor_data *d, *dnext;
	char buf[BUFFER_LEN + 2];

	if (!msg || !*msg)
		return;
	strcpyn(buf, sizeof(buf), msg);
	strcatn(buf, sizeof(buf), "\r\n");

	for (d = descriptor_list; d; d = dnext) {
		dnext = d->next;
		queue_ansi(d, buf);
		/* queue_write(d, "\r\n", 2); */
		if (!process_output(d)) {
			d->booted = 1;
		}
	}
}


void
flush_user_output(dbref player)
{
    int di;
    int* darr;
    int dcount;
	struct descriptor_data *d;

	darr = get_player_descrs(OWNER(player), &dcount);
    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d && !process_output(d)) {
            d->booted = 1;
        }
    }
}


void
wall_wizards(const char *msg)
{
	struct descriptor_data *d, *dnext;
	char buf[BUFFER_LEN + 2];

	strcpyn(buf, sizeof(buf), msg);
	strcatn(buf, sizeof(buf), "\r\n");

	for (d = descriptor_list; d; d = dnext) {
		dnext = d->next;
		if (d->connected && Wizard(d->player)) {
			queue_ansi(d, buf);
			if (!process_output(d)) {
				d->booted = 1;
			}
		}
	}
}


#ifdef USE_IPV6
struct descriptor_data *
new_connection_v6(int port, int sock, int is_ssl)
{
	int newsock;

	struct sockaddr_in6 addr;
	socklen_t addr_len;
	char hostname[128];

	addr_len = (socklen_t)sizeof(addr);
	newsock = accept(sock, (struct sockaddr *) &addr, &addr_len);
	if (newsock < 0) {
		return 0;
	} else {
# ifdef F_SETFD
		fcntl(newsock, F_SETFD, 1);
# endif
		strcpyn(hostname, sizeof(hostname), addrout_v6(port, &(addr.sin6_addr), addr.sin6_port));
		log_status("ACCEPT: %s on descriptor %d", hostname, newsock);
		log_status("CONCOUNT: There are now %d open connections.", ++ndescriptors);
		return initializesock(newsock, hostname, is_ssl);
	}
}
#endif


struct descriptor_data *
new_connection(int port, int sock, int is_ssl)
{
	int newsock;
	struct sockaddr_in addr;
	socklen_t addr_len;
	char hostname[128];

	addr_len = (socklen_t)sizeof(addr);
	newsock = accept(sock, (struct sockaddr *) &addr, &addr_len);
	if (newsock < 0) {
		return 0;
	} else {
#ifdef F_SETFD
		fcntl(newsock, F_SETFD, 1);
#endif
		strcpyn(hostname, sizeof(hostname), addrout(port, addr.sin_addr.s_addr, addr.sin_port));
		log_status("ACCEPT: %s on descriptor %d", hostname, newsock);
		log_status("CONCOUNT: There are now %d open connections.", ++ndescriptors);
		return initializesock(newsock, hostname, is_ssl);
	}
}


#ifdef SPAWN_HOST_RESOLVER

void
kill_resolver(void)
{
	int i;

	write(resolver_sock[1], "QUIT\n", 5);
	wait(&i);
}



static time_t resolver_spawn_time = 0;

void
spawn_resolver(void)
{
	if (time(NULL) - resolver_spawn_time < 5) {
		return;
	}
	resolver_spawn_time = time(NULL);

	socketpair(AF_UNIX, SOCK_STREAM, 0, resolver_sock);
	make_nonblocking(resolver_sock[1]);
	if ((global_resolver_pid=fork())==0) {
		close(0);
		close(1);
		dup(resolver_sock[0]);
		dup(resolver_sock[0]);
#ifdef BINDIR
		{
			char resolverpath[BUFFER_LEN];
			snprintf(resolverpath, sizeof(resolverpath), "%s/fb-resolver", BINDIR);
			execl(resolverpath, "fb-resolver", NULL);
			snprintf(resolverpath, sizeof(resolverpath), "%s/resolver", BINDIR);
			execl(resolverpath, "resolver", NULL);
		}
#endif
		execl("/usr/local/bin/fb-resolver", "resolver", NULL);
		execl("/usr/local/bin/resolver", "resolver", NULL);
		execl("/usr/bin/fb-resolver", "resolver", NULL);
		execl("/usr/bin/resolver", "resolver", NULL);
		execl("./fb-resolver", "resolver", NULL);
		execl("./resolver", "resolver", NULL);
#if 0
		execl("@bindir@/resolver", "resolver", NULL);
		execl("./bin/resolver", "resolver", NULL);
		execl("/usr/lib/fbmuck/resolver", "resolver", NULL);
		execl("/usr/local/fbmuck/bin/resolver", "resolver", NULL);
		execl("/usr/local/bin/resolver", "resolver", NULL);
		execl("../src/resolver", "resolver", NULL);
		execl("./resolver", "resolver", NULL);
		execl("resolver", "resolver", NULL);
#endif
		log_status("%s","Unable to spawn host resolver!");
		_exit(1);
	}
}


void
resolve_hostnames()
{
	char buf[BUFFER_LEN];
	char *ptr, *ptr2, *ptr3, *hostip, *port, *hostname, *username, *tempptr;
	struct descriptor_data *d;
	int got, dc;

	got = read(resolver_sock[1], buf, sizeof(buf));
	if (got < 0)
		return;
	if (got == sizeof(buf)) {
		got--;
		while (got > 0 && buf[got] != '\n')
			buf[got--] = '\0';
	}
	ptr = buf;
	dc = 0;
	do {
		for (ptr2 = ptr; *ptr && *ptr != '\n' && dc < got; ptr++, dc++) ;
		if (*ptr) {
			*ptr++ = '\0';
			dc++;
		}
		if (*ptr2) {
			ptr3 = index(ptr2, '|');
			if (!ptr3)
				return;
			hostip = ptr2;
			port = index(ptr2, '(');
			if (!port)
				return;
			tempptr = index(port, ')');
			if (!tempptr)
				return;
			*tempptr = '\0';
			hostname = ptr3;
			username = index(ptr3, '(');
			if (!username)
				return;
			tempptr = index(username, ')');
			if (!tempptr)
				return;
			*tempptr = '\0';
			if (*port && *hostname && *username) {
				*port++ = '\0';
				*hostname++ = '\0';
				*username++ = '\0';
				for (d = descriptor_list; d; d = d->next) {
					if (!strcmp(d->hostname, hostip) && !strcmp(d->username, port)) {
						FREE(d->hostname);
						FREE(d->username);
						d->hostname = string_dup(hostname);
						d->username = string_dup(username);
					}
				}
			}
		}
	} while (dc < got && *ptr);
}

#endif


/*  addrout_v6 -- Translate IPV6 address 'a' from addr struct to text.		*/
#ifdef USE_IPV6
const char *
addrout_v6(int lport, struct in6_addr *a, unsigned short prt)
{
	static char buf[128];
	char ip6addr[128];

	struct in6_addr addr;
	memcpy(&addr.s6_addr, a, sizeof(struct in6_addr));

	prt = ntohs(prt);

# ifndef SPAWN_HOST_RESOLVER
	if (tp_hostnames) {
		/* One day the nameserver Qwest uses decided to start */
		/* doing halfminute lags, locking up the entire muck  */
		/* that long on every connect.  This is intended to   */
		/* prevent that, reduces average lag due to nameserver */
		/* to 1 sec/call, simply by not calling nameserver if */
		/* it's in a slow mood *grin*. If the nameserver lags */
		/* consistently, a hostname cache ala OJ's tinymuck2.3 */
		/* would make more sense:                             */
		static int secs_lost = 0;

		if (secs_lost) {
			secs_lost--;
		} else {
			time_t gethost_start = time(NULL);

			struct hostent *he = gethostbyaddr(((char *) &addr), sizeof(addr), AF_INET6);
			time_t gethost_stop = time(NULL);
			time_t lag = gethost_stop - gethost_start;

			if (lag > 10) {
				secs_lost = lag;

# if MIN_SECS_TO_LOG
				if (lag >= CFG_MIN_SECS_TO_LOG) {
					log_status("GETHOSTBYNAME-RAN: secs %3d", lag);
				}
# endif

			}
			if (he) {
				snprintf(buf, sizeof(buf), "%s(%u)", he->h_name, prt);
				return buf;
			}
		}
	}
# endif							/* SPAWN_HOST_RESOLVER */

	inet_ntop(AF_INET6, a, ip6addr, 128);
# ifdef SPAWN_HOST_RESOLVER
	snprintf(buf, sizeof(buf), "%s(%u)%u\n", ip6addr, prt, lport);
	if (tp_hostnames) {
		write(resolver_sock[1], buf, strlen(buf));
	}
# endif
	snprintf(buf, sizeof(buf), "%s(%u)\n", ip6addr, prt);

	return buf;
}
#endif


/*  addrout -- Translate address 'a' from addr struct to text.		*/

const char *
addrout(int lport, long a, unsigned short prt)
{
	static char buf[128];
	struct in_addr addr;

	bzero(&addr, sizeof(addr));
	memcpy(&addr.s_addr, &a, sizeof(struct in_addr));

	prt = ntohs(prt);

#ifndef SPAWN_HOST_RESOLVER
	if (tp_hostnames) {
		/* One day the nameserver Qwest uses decided to start */
		/* doing halfminute lags, locking up the entire muck  */
		/* that long on every connect.  This is intended to   */
		/* prevent that, reduces average lag due to nameserver */
		/* to 1 sec/call, simply by not calling nameserver if */
		/* it's in a slow mood *grin*. If the nameserver lags */
		/* consistently, a hostname cache ala OJ's tinymuck2.3 */
		/* would make more sense:                             */
		static int secs_lost = 0;

		if (secs_lost) {
			secs_lost--;
		} else {
			time_t gethost_start = time(NULL);

			struct hostent *he = gethostbyaddr(((char *) &addr),
											   sizeof(addr), AF_INET);
			time_t gethost_stop = time(NULL);
			time_t lag = gethost_stop - gethost_start;

			if (lag > 10) {
				secs_lost = lag;

#if MIN_SECS_TO_LOG
				if (lag >= CFG_MIN_SECS_TO_LOG) {
					log_status("GETHOSTBYNAME-RAN: secs %3d", lag);
				}
#endif

			}
			if (he) {
				snprintf(buf, sizeof(buf), "%s(%u)", he->h_name, prt);
				return buf;
			}
		}
	}
#endif							/* SPAWN_HOST_RESOLVER */

	a = ntohl(a);

#ifdef SPAWN_HOST_RESOLVER
	snprintf(buf, sizeof(buf), "%ld.%ld.%ld.%ld(%u)%u\n",
			(a >> 24) & 0xff,
			(a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff, prt, lport);
	if (tp_hostnames) {
		write(resolver_sock[1], buf, strlen(buf));
	}
#endif

	snprintf(buf, sizeof(buf), "%ld.%ld.%ld.%ld(%u)",
			(a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff, prt);
	return buf;
}



void
clearstrings(struct descriptor_data *d)
{
	if (d->output_prefix) {
		FREE(d->output_prefix);
		d->output_prefix = 0;
	}
	if (d->output_suffix) {
		FREE(d->output_suffix);
		d->output_suffix = 0;
	}
}



void
shutdownsock(struct descriptor_data *d)
{
	if (d->connected) {
		log_status("DISCONNECT: descriptor %d player %s(%d) from %s(%s)",
				   d->descriptor, NAME(d->player), d->player, d->hostname, d->username);
		announce_disconnect(d);
	} else {
		log_status("DISCONNECT: descriptor %d from %s(%s) never connected.",
				   d->descriptor, d->hostname, d->username);
	}
	clearstrings(d);
	shutdown(d->descriptor, 2);
	close(d->descriptor);
    forget_descriptor(d);
	freeqs(d);
	*d->prev = d->next;
	if (d->next)
		d->next->prev = d->prev;
	if (d->hostname)
		free((void *) d->hostname);
	if (d->username)
		free((void *) d->username);
#ifdef MCP_SUPPORT
	mcp_frame_clear(&d->mcpframe);
#endif
	FREE(d);
	ndescriptors--;
	log_status("CONCOUNT: There are now %d open connections.", ndescriptors);
}

struct descriptor_data *
initializesock(int s, const char *hostname, int is_ssl)
{
	struct descriptor_data *d;
	char buf[128], *ptr;

	MALLOC(d, struct descriptor_data, 1);

	d->descriptor = s;
#ifdef USE_SSL
	d->ssl_session = NULL;
#endif
	d->connected = 0;
	d->booted = 0;
	d->block_writes = 0;
	d->is_starttls = 0;
	d->player = -1;
	d->con_number = 0;
	d->connected_at = time(NULL);
	make_nonblocking(s);
	d->output_prefix = 0;
	d->output_suffix = 0;
	d->output_size = 0;
	d->output.lines = 0;
	d->output.head = 0;
	d->output.tail = &d->output.head;
	d->input.lines = 0;
	d->input.head = 0;
	d->input.tail = &d->input.head;
	d->raw_input = 0;
	d->raw_input_at = 0;
	d->telnet_enabled = 0;
	d->telnet_state = TELNET_STATE_NORMAL;
	d->telnet_sb_opt = 0;
	d->short_reads = 0;
	d->quota = tp_command_burst_size;
	d->last_time = d->connected_at;
	d->last_pinged_at = d->connected_at;

#ifdef MCP_SUPPORT
	mcp_frame_init(&d->mcpframe, d);
#endif

	strcpyn(buf, sizeof(buf), hostname);
	ptr = index(buf, ')');
	if (ptr)
		*ptr = '\0';
	ptr = index(buf, '(');
	*ptr++ = '\0';
	d->hostname = alloc_string(buf);
	d->username = alloc_string(ptr);
	if (descriptor_list)
		descriptor_list->prev = &d->next;
	d->next = descriptor_list;
	d->prev = &descriptor_list;
	descriptor_list = d;
	remember_descriptor(d);

#ifdef USE_SSL
	if (!is_ssl && tp_starttls_allow) {
		unsigned char telnet_do_starttls[] = {
			TELNET_IAC, TELNET_DO, TELOPT_STARTTLS,'\0'
		};
		socket_write(d, telnet_do_starttls, 3);
		queue_string(d, "\r\n");
	}
#endif

#ifdef MCP_SUPPORT
	mcp_negotiation_start(&d->mcpframe);
#endif
	welcome_user(d);
	return d;
}



#ifdef USE_IPV6
int
make_socket_v6(int port)
{
	int s;
	int opt;
	struct sockaddr_in6 server;

	s = socket(AF_INET6, SOCK_STREAM, 0);

	if (s < 0) {
		perror("creating stream socket");
		exit(3);
	}

	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt(SO_REUSEADDR)");
		exit(1);
	}

	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt(SO_KEEPALIVE)");
		exit(1);
	}

	opt = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt(IPV6_V6ONLY");
		exit(1);
	}

	/*
	opt = 240;
	if (setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	*/

	memset((char*)&server, 0, sizeof(server));
	/* server.sin6_len = sizeof(server); */
	server.sin6_family = AF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = htons(port);

	if (bind(s, (struct sockaddr *)&server, sizeof(server))) {
		perror("binding stream socket");
		close(s);
		exit(4);
	}
	listen(s, 5);
	return s;
}
#endif



int
make_socket(int port)
{
	int s;
	int opt;
	struct sockaddr_in server;

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror("creating stream socket");
		exit(3);
	}

	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	/*
	opt = 240;
	if (setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	*/

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
		perror("binding stream socket");
		close(s);
		exit(4);
	}
	listen(s, 5);
	return s;
}



struct text_block *
make_text_block(const char *s, int n)
{
	struct text_block *p;

	MALLOC(p, struct text_block, 1);
	MALLOC(p->buf, char, n);

	bcopy(s, p->buf, n);
	p->nchars = n;
	p->start = p->buf;
	p->nxt = 0;
	return p;
}

void
free_text_block(struct text_block *t)
{
	FREE(t->buf);
	FREE((char *) t);
}

void
add_to_queue(struct text_queue *q, const char *b, int n)
{
	struct text_block *p;

	if (n == 0)
		return;

	p = make_text_block(b, n);
	p->nxt = 0;
	*q->tail = p;
	q->tail = &p->nxt;
	q->lines++;
}

int
flush_queue(struct text_queue *q, int n)
{
	struct text_block *p;
	int really_flushed = 0;

	n += strlen(flushed_message);

	while (n > 0 && (p = q->head)) {
		n -= p->nchars;
		really_flushed += p->nchars;
		q->head = p->nxt;
		q->lines--;
		free_text_block(p);
	}
	p = make_text_block(flushed_message, strlen(flushed_message));
	p->nxt = q->head;
	q->head = p;
	q->lines++;
	if (!p->nxt)
		q->tail = &p->nxt;
	really_flushed -= p->nchars;
	return really_flushed;
}

int
queue_write(struct descriptor_data *d, const char *b, int n)
{
	int space;

	space = tp_max_output - d->output_size - n;
	if (space < 0)
		d->output_size -= flush_queue(&d->output, -space);
	add_to_queue(&d->output, b, n);
	d->output_size += n;
	return n;
}

int
queue_string(struct descriptor_data *d, const char *s)
{
	return queue_write(d, s, strlen(s));
}


int
send_keepalive(struct descriptor_data *d)
{
	int cnt;
	unsigned char telnet_nop[] = {
		TELNET_IAC, TELNET_NOP, '\0'
	};

	/* drastic, but this may give us crash test data */
	if (!d || !d->descriptor) {
		panic("send_keepalive(): bad descriptor or connect struct !");
	}

	if (d->telnet_enabled) {
		cnt = socket_write(d, telnet_nop, 2);
	} else {
		cnt = socket_write(d, "", 0);
	}
#ifdef WIN32
	/* We expect a 0 return */
	if (cnt < 0 || cnt == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 1;
		return 0;
	}
#else
	/* We expect a 0 return */
	if (cnt < 0) {
		if (errno == EWOULDBLOCK)
			return 1;
		if (errno == 0)
			return 1;
		log_status("keepalive socket write descr=%i, errno=%i", d->descriptor, errno);
		return 0;
	}
#endif
	return 1;
}


int
process_output(struct descriptor_data *d)
{
	struct text_block **qp, *cur;
	int cnt;

	/* drastic, but this may give us crash test data */
	if (!d || !d->descriptor) {
		panic("process_output(): bad descriptor or connect struct !");
	}

	if (d->output.lines == 0) {
		return 1;
	}

	if (d->block_writes) {
		return 1;
	}

	for (qp = &d->output.head; (cur = *qp);) {
		cnt = socket_write(d, cur->start, cur->nchars);

#ifdef WIN32
		if (cnt <= 0 || cnt == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				return 1;
			return 0;
		}
#else
		if (cnt <= 0) {
			if (errno == EWOULDBLOCK)
				return 1;
			return 0;
		}
#endif
		d->output_size -= cnt;
		if (cnt == cur->nchars) {
			d->output.lines--;
			if (!cur->nxt) {
				d->output.tail = qp;
				d->output.lines = 0;
			}
			*qp = cur->nxt;
			free_text_block(cur);
			continue;			/* do not adv ptr */
		}
		cur->nchars -= cnt;
		cur->start += cnt;
		break;
	}
	return 1;
}

#ifdef WIN32
# define O_NONBLOCK 1
#else
# if !defined(O_NONBLOCK) || defined(ULTRIX)	/* POSIX ME HARDER */
#  ifdef FNDELAY					/* SUN OS */
#   define O_NONBLOCK FNDELAY
#  else
#   ifdef O_NDELAY				/* SyseVil */
#    define O_NONBLOCK O_NDELAY
#   endif
#  endif
# endif
#endif

void
make_nonblocking(int s)
{
#ifdef WIN32
	unsigned long val = O_NONBLOCK;
	if (ioctlsocket(s, FIONBIO, &val) == SOCKET_ERROR) {
		perror("make_nonblocking: ioctlsocket");
		panic("O_NONBLOCK ioctlsocket failed");
	}
#else
	if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
		perror("make_nonblocking: fcntl");
		panic("O_NONBLOCK fcntl failed");
	}
#endif
}

void
freeqs(struct descriptor_data *d)
{
	struct text_block *cur, *next;

	cur = d->output.head;
	while (cur) {
		next = cur->nxt;
		free_text_block(cur);
		cur = next;
	}
	d->output.lines = 0;
	d->output.head = 0;
	d->output.tail = &d->output.head;

	cur = d->input.head;
	while (cur) {
		next = cur->nxt;
		free_text_block(cur);
		cur = next;
	}
	d->input.lines = 0;
	d->input.head = 0;
	d->input.tail = &d->input.head;

	if (d->raw_input)
		FREE(d->raw_input);
	d->raw_input = 0;
	d->raw_input_at = 0;
}

void
save_command(struct descriptor_data *d, const char *command)
{
	add_to_queue(&d->input, command, strlen(command) + 1);
}


int
process_input(struct descriptor_data *d)
{
	char buf[MAX_COMMAND_LEN * 2];
	int maxget = sizeof(buf);
	int got;
	char *p, *pend, *q, *qend;

	if (d->short_reads) {
	    maxget = 1;
	}
	got = socket_read(d, buf, maxget);

#ifdef WIN32
# ifdef USE_SSL
	if (got <= 0 || got == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			return 0;
		}
	}
# else
	if (got <= 0 || got == SOCKET_ERROR) {
		return 0;
	}
# endif
#else

# ifdef USE_SSL
	if (got <= 0 && errno != EWOULDBLOCK) {
		return 0;
	}
# else
	if (got <= 0) {
		return 0;
	}
# endif
#endif

	if (!d->raw_input) {
		MALLOC(d->raw_input, char, MAX_COMMAND_LEN);
		d->raw_input_at = d->raw_input;
	}
	p = d->raw_input_at;
	pend = d->raw_input + MAX_COMMAND_LEN - 1;
	for (q = buf, qend = buf + got; q < qend; q++) {
		if (*q == '\n') {
			d->last_time = time(NULL);
			*p = '\0';
			if (p >= d->raw_input)
				save_command(d, d->raw_input);
			p = d->raw_input;
		} else if (d->telnet_state == TELNET_STATE_IAC) {
			switch (*((unsigned char *)q)) {
				case TELNET_NOP: /* NOP */
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_BRK: /* Break */
				case TELNET_IP: /* Interrupt Process */
					save_command(d, BREAK_COMMAND);
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_AO: /* Abort Output */
					/* could be handy, but for now leave unimplemented */
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_AYT: /* AYT */
					{
						char sendbuf[] = "[Yes]\r\n";
						socket_write(d, sendbuf, strlen(sendbuf));
						d->telnet_state = TELNET_STATE_NORMAL;
						break;
					}
				case TELNET_EC: /* Erase character */
					if (p > d->raw_input)
						p--;
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_EL: /* Erase line */
					p = d->raw_input;
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_GA: /* Go Ahead */
					/* treat as a NOP (?) */
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_WILL: /* WILL (option offer) */
					d->telnet_state = TELNET_STATE_WILL;
					break;
				case TELNET_WONT: /* WONT (option offer) */
					d->telnet_state = TELNET_STATE_WONT;
					break;
				case TELNET_DO: /* DO (option request) */
					d->telnet_state = TELNET_STATE_DO;
					break;
				case TELNET_DONT: /* DONT (option request) */
					d->telnet_state = TELNET_STATE_DONT;
					break;
				case TELNET_SB: /* SB (option subnegotiation) */
					d->telnet_state = TELNET_STATE_SB;
					break;
				case TELNET_SE: /* Go Ahead */
#ifdef USE_SSL
					if (d->telnet_sb_opt == TELOPT_STARTTLS) {
						d->block_writes = 0;
						d->short_reads = 0;
						if (tp_starttls_allow) {
						    d->is_starttls = 1;
						    d->ssl_session = SSL_new(ssl_ctx);
						    SSL_set_fd(d->ssl_session, d->descriptor);
						    SSL_accept(d->ssl_session);
						    log_status("STARTTLS: %i", d->descriptor);
					    }
					}
#endif
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				case TELNET_IAC: /* IAC a second time */
#if 0
					/* If we were 8 bit clean, we'd pass this along */
					*p++ = *q;
#endif
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
				default:
					/* just ignore */
					d->telnet_state = TELNET_STATE_NORMAL;
					break;
			}
		} else if (d->telnet_state == TELNET_STATE_WILL) {
			/* We don't negotiate: send back DONT option */
			unsigned char sendbuf[8];
#ifdef USE_SSL
            /* If we get a STARTTLS reply, negotiate SSL startup */
			if (*q == TELOPT_STARTTLS && !d->ssl_session && tp_starttls_allow) {
				sendbuf[0] = TELNET_IAC;
				sendbuf[1] = TELNET_SB;
				sendbuf[2] = TELOPT_STARTTLS;
				sendbuf[3] = 1;  /* TLS FOLLOWS */
				sendbuf[4] = TELNET_IAC;
				sendbuf[5] = TELNET_SE;
				sendbuf[6] = '\0';
				socket_write(d, sendbuf, 6);
				d->block_writes = 1;
				d->short_reads = 1;
			} else
#endif
			/* Otherwise, we don't negotiate: send back DONT option */
			{
				sendbuf[0] = TELNET_IAC;
				sendbuf[1] = TELNET_DONT;
				sendbuf[2] = *q;
				sendbuf[3] = '\0';
				socket_write(d, sendbuf, 3);
			}
			d->telnet_state = TELNET_STATE_NORMAL;
			d->telnet_enabled = 1;
		} else if (d->telnet_state == TELNET_STATE_DO) {
			/* We don't negotiate: send back WONT option */
			unsigned char sendbuf[4];
			sendbuf[0] = TELNET_IAC;
			sendbuf[1] = TELNET_WONT;
			sendbuf[2] = *q;
			sendbuf[3] = '\0';
			socket_write(d, sendbuf, 3);
			d->telnet_state = TELNET_STATE_NORMAL;
			d->telnet_enabled = 1;
		} else if (d->telnet_state == TELNET_STATE_WONT) {
			/* Ignore WONT option. */
			d->telnet_state = TELNET_STATE_NORMAL;
			d->telnet_enabled = 1;
		} else if (d->telnet_state == TELNET_STATE_DONT) {
			/* We don't negotiate: send back WONT option */
			unsigned char sendbuf[4];
			sendbuf[0] = TELNET_IAC;
			sendbuf[1] = TELNET_WONT;
			sendbuf[2] = *q;
			sendbuf[3] = '\0';
			socket_write(d, sendbuf, 3);
			d->telnet_state = TELNET_STATE_NORMAL;
			d->telnet_enabled = 1;
		} else if (d->telnet_state == TELNET_STATE_SB) {
			d->telnet_sb_opt = *((unsigned char*)q);
			/* TODO: Start remembering subnegotiation data. */
			d->telnet_state = TELNET_STATE_NORMAL;
		} else if (*((unsigned char *)q) == TELNET_IAC) {
			/* Got TELNET IAC, store for next byte */	
			d->telnet_state = TELNET_STATE_IAC;
		} else if (p < pend) {
			/* NOTE: This will need rethinking for unicode */
			if ( isinput( *q ) ) {
				*p++ = *q;
			} else if (*q == '\t') {
				*p++ = ' ';
			} else if (*q == 8 || *q == 127) {
				/* if BS or DEL, delete last character */
				if (p > d->raw_input)
					p--;
			}
			d->telnet_state = TELNET_STATE_NORMAL;
		}
	}
	if (p > d->raw_input) {
		d->raw_input_at = p;
	} else {
		FREE(d->raw_input);
		d->raw_input = 0;
		d->raw_input_at = 0;
	}
	return 1;
}

void
set_userstring(char **userstring, const char *command)
{
	if (*userstring) {
		FREE(*userstring);
		*userstring = 0;
	}
	while (*command && isinput(*command) && isspace(*command))
		command++;
	if (*command)
		*userstring = string_dup(command);
}

void
process_commands(void)
{
	int nprocessed;
	struct descriptor_data *d, *dnext;
	struct text_block *t;

	do {
		nprocessed = 0;
		for (d = descriptor_list; d; d = dnext) {
			dnext = d->next;
			if (d->quota > 0 && (t = d->input.head)) {
				if (d->connected && PLAYER_BLOCK(d->player) && !is_interface_command(t->start)) {
					char *tmp = t->start;
#ifdef MCP_SUPPORT
					if (!strncmp(tmp, "#$\"", 3)) {
						/* Un-escape MCP escaped lines */
						tmp += 3;
					}
#endif
					/* WORK: send player's foreground/preempt programs an exclusive READ mufevent */
					if (!read_event_notify(d->descriptor, d->player, tmp) && !*tmp) {
						/* Didn't send blank line.  Eat it.  */
						nprocessed++;
						d->input.head = t->nxt;
						d->input.lines--;
						if (!d->input.head) {
							d->input.tail = &d->input.head;
							d->input.lines = 0;
						}
						free_text_block(t);
					}
				} else {
#ifdef MCP_SUPPORT
					if (strncmp(t->start, "#$#", 3)) {
						/* Not an MCP mesg, so count this against quota. */
						d->quota--;
					}
#endif
					nprocessed++;
					if (!do_command(d, t->start)) {
						d->booted = 2;
						/* Disconnect player next pass through main event loop. */
					}
					d->input.head = t->nxt;
					d->input.lines--;
					if (!d->input.head) {
						d->input.tail = &d->input.head;
						d->input.lines = 0;
					}
					free_text_block(t);
				}
			}
		}
	} while (nprocessed > 0);
}

int
is_interface_command(const char* cmd)
{
	const char* tmp = cmd;
#ifdef MCP_SUPPORT
	if (!strncmp(tmp, "#$\"", 3)) {
		/* dequote MCP quoting. */
		tmp += 3;
	}
	if (!strncmp(cmd, "#$#", 3)) /* MCP mesg. */
		return 1;
#endif
	if (!string_compare(tmp, BREAK_COMMAND))
		return 1;
	if (!strcmp(tmp, QUIT_COMMAND))
		return 1;
	if (!strncmp(tmp, WHO_COMMAND, strlen(WHO_COMMAND)))
		return 1;
	if (!strncmp(tmp, PREFIX_COMMAND, strlen(PREFIX_COMMAND)))
		return 1;
	if (!strncmp(tmp, SUFFIX_COMMAND, strlen(SUFFIX_COMMAND)))
		return 1;
	if (tp_recognize_null_command && !string_compare(tmp, NULL_COMMAND))
		return 1;
	return 0;
}

int
do_command(struct descriptor_data *d, char *command)
{
	char buf[BUFFER_LEN];
	char cmdbuf[BUFFER_LEN];
#ifdef MCP_SUPPORT
	if (!mcp_frame_process_input(&d->mcpframe, command, cmdbuf, sizeof(cmdbuf))) {
		d->quota++;
		return 1;
	}
#else
	strcpy(cmdbuf, command);
#endif
	command = cmdbuf;

	if (d->connected)
		ts_lastuseobject(d->player);

	if (!string_compare(command, BREAK_COMMAND)) {
	        if (!d->connected)
		        return 0;
		if (dequeue_prog(d->player, 2)) {
			if (d->output_prefix) {
				queue_ansi(d, d->output_prefix);
				queue_write(d, "\r\n", 2);
			}
			queue_ansi(d, "Foreground program aborted.\r\n");
			if ((FLAGS(d->player) & INTERACTIVE))
				if ((FLAGS(d->player) & READMODE))
					process_command(d->descriptor, d->player, command);
			if (d->output_suffix) {
				queue_ansi(d, d->output_suffix);
				queue_write(d, "\r\n", 2);
			}
		}
		PLAYER_SET_BLOCK(d->player, 0);
		return 1;
	} else if (!strcmp(command, QUIT_COMMAND)) {
		return 0;
	} else if (tp_recognize_null_command && !string_compare(command, NULL_COMMAND)) {
		return 1;
	} else if ((!strncmp(command, WHO_COMMAND, sizeof(WHO_COMMAND) - 1)) ||
                   (*command == OVERIDE_TOKEN &&
                    (!strncmp(command+1, WHO_COMMAND, sizeof(WHO_COMMAND) - 1))
                   )) {
		if (d->output_prefix) {
			queue_ansi(d, d->output_prefix);
			queue_write(d, "\r\n", 2);
		}
		strcpyn(buf, sizeof(buf), "@");
		strcatn(buf, sizeof(buf), WHO_COMMAND);
		strcatn(buf, sizeof(buf), " ");
		strcatn(buf, sizeof(buf), command + sizeof(WHO_COMMAND) - 1);
		if (!d->connected || (FLAGS(d->player) & INTERACTIVE)) {
			if (tp_secure_who) {
				queue_ansi(d, "Sorry, WHO is unavailable at this point.\r\n");
			} else {
				dump_users(d, command + sizeof(WHO_COMMAND) - 1);
			}
		} else {
			if ((!(TrueWizard(OWNER(d->player)) &&
                              (*command == OVERIDE_TOKEN))) &&
                            can_move(d->descriptor, d->player, buf, 2)) {
				do_move(d->descriptor, d->player, buf, 2);
			} else {
				dump_users(d, command + sizeof(WHO_COMMAND) - 
                                           ((*command == OVERIDE_TOKEN)?0:1));
			}
		}
		if (d->output_suffix) {
			queue_ansi(d, d->output_suffix);
			queue_write(d, "\r\n", 2);
		}
	} else if (!strncmp(command, PREFIX_COMMAND, sizeof(PREFIX_COMMAND) - 1)) {
		set_userstring(&d->output_prefix, command + sizeof(PREFIX_COMMAND) - 1);
	} else if (!strncmp(command, SUFFIX_COMMAND, sizeof(SUFFIX_COMMAND) - 1)) {
		set_userstring(&d->output_suffix, command + sizeof(SUFFIX_COMMAND) - 1);
	} else {
		if (d->connected) {
			if (d->output_prefix) {
				queue_ansi(d, d->output_prefix);
				queue_write(d, "\r\n", 2);
			}
			process_command(d->descriptor, d->player, command);
			if (d->output_suffix) {
				queue_ansi(d, d->output_suffix);
				queue_write(d, "\r\n", 2);
			}
		} else {
			check_connect(d, command);
		}
	}
	return 1;
}

void
interact_warn(dbref player)
{
	if (FLAGS(player) & INTERACTIVE) {
		char buf[BUFFER_LEN];

		if (FLAGS(player) & READMODE) {
			snprintf(buf, sizeof(buf), "***  You are currently using a program.  Use \"%s\" to return to a more reasonable state of control.  ***", BREAK_COMMAND);
		} else if (PLAYER_INSERT_MODE(player)) {
			snprintf(buf, sizeof(buf), "***  You are currently inserting MUF program text.  Use \"%s\" to return to the editor, then \"%c\" if you wish to return to your regularly scheduled MUCK universe.  ***", EXIT_INSERT, QUIT_EDIT_COMMAND);
		} else {
			snprintf(buf, sizeof(buf), "***  You are currently using the MUF program editor.  ***");
		}
		notify(player, buf);
	}
}

void
check_connect(struct descriptor_data *d, const char *msg)
{
	char command[MAX_COMMAND_LEN];
	char user[MAX_COMMAND_LEN];
	char password[MAX_COMMAND_LEN];
	dbref player;

	parse_connect(msg, command, user, password);

	if (!strncmp(command, "co", 2)) {
		player = connect_player(user, password);
		if (player == NOTHING) {
			queue_ansi(d, connect_fail);
			log_status("FAILED CONNECT %s on descriptor %d", user, d->descriptor);
		} else {
			if ((wizonly_mode ||
				 (tp_playermax && con_players_curr >= tp_playermax_limit)) &&
				!TrueWizard(player)) {
				if (wizonly_mode) {
					queue_ansi(d, "Sorry, but the game is in maintenance mode currently, and only wizards are allowed to connect.  Try again later.");
				} else {
					queue_ansi(d, tp_playermax_bootmesg);
				}
				queue_string(d, "\r\n");
				d->booted = 1;
			} else {
				log_status("CONNECTED: %s(%d) on descriptor %d",
						   NAME(player), player, d->descriptor);
				d->connected = 1;
				d->connected_at = time(NULL);
				d->player = player;
				update_desc_count_table();
				remember_player_descr(player, d->descriptor);
				/* cks: someone has to initialize this somewhere. */
				PLAYER_SET_BLOCK(d->player, 0);
				show_file(d, MOTD_FILE);
				announce_connect(d->descriptor, player);
				interact_warn(player);
				if (sanity_violated && Wizard(player)) {
					notify(player,
						   "#########################################################################");
					notify(player,
						   "## WARNING!  The DB appears to be corrupt!  Please repair immediately! ##");
					notify(player,
						   "#########################################################################");
				}
				con_players_curr++;
			}
		}
	} else if (!strncmp(command, "cr", 2)) {
		if (!tp_registration) {
			if (wizonly_mode || (tp_playermax && con_players_curr >= tp_playermax_limit)) {
				if (wizonly_mode) {
					queue_ansi(d, "Sorry, but the game is in maintenance mode currently, and only wizards are allowed to connect.  Try again later.");
				} else {
					queue_ansi(d, tp_playermax_bootmesg);
				}
				queue_string(d, "\r\n");
				d->booted = 1;
			} else {
				player = create_player(user, password);
				if (player == NOTHING) {
					queue_ansi(d, create_fail);
					log_status("FAILED CREATE %s on descriptor %d", user, d->descriptor);
				} else {
					log_status("CREATED %s(%d) on descriptor %d",
							   NAME(player), player, d->descriptor);
					d->connected = 1;
					d->connected_at = time(NULL);
					d->player = player;
					update_desc_count_table();
					remember_player_descr(player, d->descriptor);
					/* cks: someone has to initialize this somewhere. */
					PLAYER_SET_BLOCK(d->player, 0);
					spit_file(player, MOTD_FILE);
					announce_connect(d->descriptor, player);
					con_players_curr++;
				}
			}
		} else {
			queue_ansi(d, tp_register_mesg);
			queue_string(d, "\r\n");
			log_status("FAILED CREATE %s on descriptor %d", user, d->descriptor);
		}
	} else if (!strncmp(command, "help", 4)) {
		show_file(d, CONHELP_FILE);
	} else if (!*command) {
		/* do nothing */
	} else {
		welcome_user(d);
	}
}

void
show_file(struct descriptor_data *d, const char *filename)
{
	FILE *f;
	char buf[BUFFER_LEN];

	if ((f = fopen(filename, "r")) == NULL) {
		queue_ansi(d, "This content is missing - management has been notified.\r\n");
                fputs("show_file:", stderr);
                perror(filename);
	} else {
		while(fgets(buf, sizeof(buf), f)) {
			queue_ansi(d, buf);
		}
		fclose(f);
	}
}

void
parse_connect(const char *msg, char *command, char *user, char *pass)
{
	char *p;

	while (*msg && isinput(*msg) && isspace(*msg))
		msg++;
	p = command;
	while (*msg && isinput(*msg) && !isspace(*msg))
		*p++ = tolower(*msg++);
	*p = '\0';
	while (*msg && isinput(*msg) && isspace(*msg))
		msg++;
	p = user;
	while (*msg && isinput(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
	while (*msg && isinput(*msg) && isspace(*msg))
		msg++;
	p = pass;
	while (*msg && isinput(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
}


int
boot_off(dbref player)
{
    int* darr;
    int dcount;
	struct descriptor_data *last = NULL;

	darr = get_player_descrs(player, &dcount);
	if (darr) {
        last = descrdata_by_descr(darr[0]);
	}

	if (last) {
		process_output(last);
		last->booted = 1;
		/* shutdownsock(last); */
		return 1;
	}
	return 0;
}

void
boot_player_off(dbref player)
{
    int di;
    int* darr;
    int dcount;
    struct descriptor_data *d;
 
	darr = get_player_descrs(player, &dcount);
    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d) {
            d->booted = 1;
        }
    }
}


void
close_sockets(const char *msg)
{
	struct descriptor_data *d, *dnext;
	int i;

	for (d = descriptor_list; d; d = dnext) {
		dnext = d->next;
		if (d->connected) {
			forget_player_descr(d->player, d->descriptor);
		}
		socket_write(d, msg, strlen(msg));
		socket_write(d, shutdown_message, strlen(shutdown_message));
		clearstrings(d);
		if (shutdown(d->descriptor, 2) < 0)
			perror("shutdown");
		close(d->descriptor);
		freeqs(d);
		*d->prev = d->next;
		if (d->next)
			d->next->prev = d->prev;
		if (d->hostname)
			free((void *) d->hostname);
		if (d->username)
			free((void *) d->username);
#ifdef MCP_SUPPORT
		mcp_frame_clear(&d->mcpframe);
#endif
		FREE(d);
		ndescriptors--;
	}
	update_desc_count_table();
	for (i = 0; i < numsocks; i++) {
		close(sock[i]);
	}
#ifdef USE_IPV6
	for (i = 0; i < numsocks_v6; i++) {
		close(sock_v6[i]);
	}
#endif
#ifdef USE_SSL
	for (i = 0; i < ssl_numsocks; i++) {
		close(ssl_sock[i]);
	}
# ifdef USE_IPV6
	for (i = 0; i < ssl_numsocks_v6; i++) {
		close(ssl_sock_v6[i]);
	}
# endif
#endif
}


void
do_armageddon(dbref player, const char *msg)
{
	char buf[BUFFER_LEN];

	if (!Wizard(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, "Sorry, but you don't look like the god of War to me.");
		log_status("ILLEGAL ARMAGEDDON: tried by %s", unparse_object(player, player));
		return;
	}
	snprintf(buf, sizeof(buf), "\r\nImmediate shutdown initiated by %s.\r\n", NAME(player));
	if (msg || *msg)
		strcatn(buf, sizeof(buf), msg);
	log_status("ARMAGEDDON initiated by %s(%d): %s", NAME(player), player, msg);
	fprintf(stderr, "ARMAGEDDON initiated by %s(%d): %s\n", NAME(player), player, msg);
	close_sockets(buf);

#ifdef SPAWN_HOST_RESOLVER
	kill_resolver();
#endif

	exit(1);
}


void
emergency_shutdown(void)
{
	close_sockets("\r\nEmergency shutdown due to server crash.");

#ifdef SPAWN_HOST_RESOLVER
	kill_resolver();
#endif

}


void
dump_users(struct descriptor_data *e, char *user)
{
	struct descriptor_data *d;
	int wizard, players;
	time_t now;
	char buf[2048];
	char pbuf[64];
	char secchar = ' ';


/* #ifdef GOD_PRIV */
/* -- Wizard should always override tp_who_doing JES
	if (tp_who_doing) {
		wizard = e->connected && God(e->player);
	} else {
		wizard = e->connected && Wizard(e->player);
	}
*/
/* #else */
	wizard = e->connected && Wizard(e->player) && !tp_who_doing;
/* #endif */

	while (*user && (isspace(*user) || *user == '*')) {
		if (tp_who_doing && *user == '*' && e->connected && Wizard(e->player))
			wizard = 1;
		user++;
	}

	if (wizard)
		/* S/he is connected and not quelled. Okay; log it. */
		log_command("%s: %s", whowhere(e->player), "WHO");

	if (!*user)
		user = NULL;

	(void) time(&now);
	if (wizard) {
		queue_ansi(e, "Player Name                Location     On For Idle   Host\r\n");
	} else {
		if (tp_who_doing) {
			queue_ansi(e, "Player Name           On For Idle   Doing...\r\n");
		} else {
			queue_ansi(e, "Player Name           On For Idle\r\n");
		}
	}

	d = descriptor_list;
	players = 0;
	while (d) {
		if (d->connected &&
			(!tp_who_hides_dark ||
			 (wizard || !(FLAGS(d->player) & DARK))) &&
			++players && (!user || string_prefix(NAME(d->player), user))
				) {

#ifdef USE_SSL
			secchar = (d->ssl_session ? '@' : ' ');
#else
			secchar = ' ';
#endif

			if (wizard) {
				/* don't print flags, to save space */
				snprintf(pbuf, sizeof(pbuf), "%.*s(#%d)", PLAYER_NAME_LIMIT + 1,
						NAME(d->player), (int) d->player);
#ifdef GOD_PRIV
				if (!God(e->player))
					snprintf(buf, sizeof(buf),
							"%-*s [%6d] %10s %4s%c%c %s\r\n",
							PLAYER_NAME_LIMIT + 10, pbuf,
							(int) LOCATION(d->player),
							time_format_1(now - d->connected_at),
							time_format_2(now - d->last_time),
							((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
							secchar, d->hostname);
				else
#endif
					snprintf(buf, sizeof(buf),
							"%-*s [%6d] %10s %4s%c%c %s(%s)\r\n",
							PLAYER_NAME_LIMIT + 10, pbuf,
							(int) LOCATION(d->player),
							time_format_1(now - d->connected_at),
							time_format_2(now - d->last_time),
							((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
							secchar, d->hostname, d->username);
			} else {
				if (tp_who_doing) {
					/* Modified to take into account PLAYER_NAME_LIMIT changes */
					snprintf(buf, sizeof(buf), "%-*s %10s %4s%c%c %.*s\r\n",
							PLAYER_NAME_LIMIT + 1,
							NAME(d->player),
							time_format_1(now - d->connected_at),
							time_format_2(now - d->last_time),
							((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
							secchar,
							/* Things must end on column 79. The required cols
							 * (not counting player name, but counting forced
							 * space after it) use up 20 columns.
							 *
							 * !! Don't forget to update this if that changes
							 */
							(int) (79 - (PLAYER_NAME_LIMIT + 20)),
							GETDOING(d->player) ?
								GETDOING(d->player) : ""
							);
				} else {
					snprintf(buf, sizeof(buf), "%-*s %10s %4s%c%c\r\n",
							(int)(PLAYER_NAME_LIMIT + 1),
							NAME(d->player),
							time_format_1(now - d->connected_at),
							time_format_2(now - d->last_time),
							((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
							secchar);
				}
			}
			queue_ansi(e, buf);
		}
		d = d->next;
	}
	if (players > con_players_max)
		con_players_max = players;
	snprintf(buf, sizeof(buf), "%d player%s %s connected.  (Max was %d)\r\n", players,
			(players == 1) ? "" : "s", (players == 1) ? "is" : "are", con_players_max);
	queue_ansi(e, buf);
}

char *
time_format_1(time_t dt)
{
	register struct tm *delta;
	static char buf[64];

	delta = gmtime((time_t *) &dt);
	if (delta->tm_yday > 0)
		snprintf(buf, sizeof(buf), "%dd %02d:%02d", delta->tm_yday, delta->tm_hour, delta->tm_min);
	else
		snprintf(buf, sizeof(buf), "%02d:%02d", delta->tm_hour, delta->tm_min);
	return buf;
}

char *
time_format_2(time_t dt)
{
	register struct tm *delta;
	static char buf[64];

	delta = gmtime((time_t *) &dt);
	if (delta->tm_yday > 0)
		snprintf(buf, sizeof(buf), "%dd", delta->tm_yday);
	else if (delta->tm_hour > 0)
		snprintf(buf, sizeof(buf), "%dh", delta->tm_hour);
	else if (delta->tm_min > 0)
		snprintf(buf, sizeof(buf), "%dm", delta->tm_min);
	else
		snprintf(buf, sizeof(buf), "%ds", delta->tm_sec);
	return buf;
}


void
announce_puppets(dbref player, const char *msg, const char *prop)
{
	dbref what, where;
	const char *ptr, *msg2;
	char buf[BUFFER_LEN];

	for (what = 0; what < db_top; what++) {
		if (Typeof(what) == TYPE_THING && (FLAGS(what) & ZOMBIE)) {
			if (OWNER(what) == player) {
				where = LOCATION(what);
				if ((!Dark(where)) && (!Dark(player)) && (!Dark(what))) {
					msg2 = msg;
					if ((ptr = (char *) get_property_class(what, prop)) && *ptr)
						msg2 = ptr;
					snprintf(buf, sizeof(buf), "%.512s %.3000s", NAME(what), msg2);
					notify_except(CONTENTS(where), what, buf, what);
				}
			}
		}
	}
}

void
announce_connect(int descr, dbref player)
{
	dbref loc;
	char buf[BUFFER_LEN];
	struct match_data md;
	dbref exit;

	if ((loc = LOCATION(player)) == NOTHING)
		return;

	if ((!Dark(player)) && (!Dark(loc))) {
		snprintf(buf, sizeof(buf), "%s has connected.", NAME(player));
		notify_except(CONTENTS(loc), player, buf, player);
	}

	exit = NOTHING;
	if (online(player) == 1) {
		init_match(descr, player, "connect", TYPE_EXIT, &md);	/* match for connect */
		md.match_level = 1;
		match_all_exits(&md);
		exit = match_result(&md);
		if (exit == AMBIGUOUS)
			exit = NOTHING;
	}

	if (exit == NOTHING || !(FLAGS(exit) & STICKY)) {
		if (can_move(descr, player, tp_autolook_cmd, 1)) {
			do_move(descr, player, tp_autolook_cmd, 1);
		} else {
			do_look_around(descr, player);
		}
	}


	/*
	 * See if there's a connect action.  If so, and the player is the first to
	 * connect, send the player through it.  If the connect action is set
	 * sticky, then suppress the normal look-around.
	 */

	if (exit != NOTHING)
		do_move(descr, player, "connect", 1);

	if (online(player) == 1) {
		announce_puppets(player, "wakes up.", MESGPROP_PCON);
	}

	/* queue up all _connect programs referred to by properties */
	envpropqueue(descr, player, LOCATION(player), NOTHING, player, NOTHING,
				 "_connect", "Connect", 1, 1);
	envpropqueue(descr, player, LOCATION(player), NOTHING, player, NOTHING,
				 "_oconnect", "Oconnect", 1, 0);

	ts_useobject(player);
	return;
}

void
announce_disconnect(struct descriptor_data *d)
{
	dbref player = d->player;
	dbref loc;
	char buf[BUFFER_LEN];
	int dcount;

	if ((loc = LOCATION(player)) == NOTHING)
		return;

	get_player_descrs(d->player, &dcount);
	if (dcount < 2 && dequeue_prog(player, 2))
		notify(player, "Foreground program aborted.");

	if ((!Dark(player)) && (!Dark(loc))) {
		snprintf(buf, sizeof(buf), "%s has disconnected.", NAME(player));
		notify_except(CONTENTS(loc), player, buf, player);
	}

	/* trigger local disconnect action */
	if (online(player) == 1) {
		if (can_move(d->descriptor, player, "disconnect", 1)) {
			do_move(d->descriptor, player, "disconnect", 1);
		}
		announce_puppets(player, "falls asleep.", MESGPROP_PDCON);
	}
#ifdef MCP_SUPPORT
	gui_dlog_closeall_descr(d->descriptor);
#endif
	d->connected = 0;
	d->player = NOTHING;

    forget_player_descr(player, d->descriptor);
    update_desc_count_table();

	/* queue up all _connect programs referred to by properties */
	envpropqueue(d->descriptor, player, LOCATION(player), NOTHING, player, NOTHING,
				 "_disconnect", "Disconnect", 1, 1);
	envpropqueue(d->descriptor, player, LOCATION(player), NOTHING, player, NOTHING,
				 "_odisconnect", "Odisconnect", 1, 0);

	ts_lastuseobject(player);
	DBDIRTY(player);
}

#ifdef MUD_ID
#include <pwd.h>
void
do_setuid(char *name)
{
	struct passwd *pw;

	if ((pw = getpwnam(name)) == NULL) {
		log_status("can't get pwent for %s", name);
		exit(1);
	}
	if (setuid(pw->pw_uid) == -1) {
		log_status("can't setuid(%d): ", pw->pw_uid);
		perror("setuid");
		exit(1);
	}
}
#endif							/* MUD_ID */


#ifdef MUD_GID
#include <grp.h>
void
do_setgid(char *name)
{
	struct group *gr;

	if ((gr = getgrnam(name)) == NULL) {
		log_status("can't get grent for group %s", name);
		exit(1);
	}
	if (setgid(gr->gr_gid) == -1) {
		log_status("can't setgid(%d): ",gr->gr_gid);
		perror("setgid");
		exit(1);
	}
}
#endif							/* MUD_GID */


/***** O(1) Connection Optimizations *****/
struct descriptor_data *descr_count_table[FD_SETSIZE];
int current_descr_count = 0;

void
init_descr_count_lookup()
{
	int i;
	for (i = 0; i < FD_SETSIZE; i++) {
		descr_count_table[i] = NULL;
	}
}

void
update_desc_count_table()
{
	int c;
	struct descriptor_data *d;

	current_descr_count = 0;
	for (c = 0, d = descriptor_list; d; d = d->next)
	{
		if (d->connected)
		{
			d->con_number = c + 1;
			descr_count_table[c++] = d;
			current_descr_count++;
		}
	}
}

struct descriptor_data *
descrdata_by_count(int c)
{
	c--;
	if (c >= current_descr_count || c < 0) {
		return NULL;
	}
	return descr_count_table[c];
}

struct descriptor_data *descr_lookup_table[FD_SETSIZE];

#ifdef WIN32
int descr_hash_table[FD_SETSIZE];

int sethash_descr(int d) {
   for (int i = 0; i < FD_SETSIZE; i++) {
      if (descr_hash_table[i] == -1) { descr_hash_table[i] = d; return i; }
   }
   fprintf(stderr, "descr hash table full !"); /* Should NEVER happen */
   return -1;
}

int gethash_descr(int d) {
   for (int i = 0; i < FD_SETSIZE; i++) {
      if (descr_hash_table[i] == d) return i;
   }
   fprintf(stderr, "descr hash value missing !"); /* Should NEVER happen */
   return -1;
 
}

void unsethash_descr(int d) {
   for (int i = 0; i < FD_SETSIZE; i++) {
      if (descr_hash_table[i] == d) {
         descr_hash_table[i] = -1;
         return;
      }
   }
   fprintf(stderr, "descr hash value missing !"); /* Should NEVER happen */
}
#endif

void
init_descriptor_lookup()
{
	int i;
#ifdef WIN32
	for (i = 0; i < FD_SETSIZE; i++) {
		descr_hash_table[i] = -1;
	}
#endif
	for (i = 0; i < FD_SETSIZE; i++) {
		descr_lookup_table[i] = NULL;
	}
}


int
index_descr(int index)
{
    if((index < 0) || (index >= FD_SETSIZE))
		return -1;
	if(descr_lookup_table[index] == NULL)
		return -1;
	return descr_lookup_table[index]->descriptor;
}


int*
get_player_descrs(dbref player, int* count)
{
	int* darr;

	if (Typeof(player) == TYPE_PLAYER) {
		*count = PLAYER_DESCRCOUNT(player);
	    darr = PLAYER_DESCRS(player);
		if (!darr) {
			*count = 0;
		}
		return darr;
	} else {
		*count = 0;
		return NULL;
	}
}

void
remember_player_descr(dbref player, int descr)
{
	int  count = 0;
	int* arr   = NULL;

	if (Typeof(player) != TYPE_PLAYER)
		return;

	count = PLAYER_DESCRCOUNT(player);
	arr = PLAYER_DESCRS(player);

	if (!arr) {
		arr = (int*)malloc(sizeof(int));
		arr[0] = descr;
		count = 1;
	} else {
		arr = (int*)realloc(arr,sizeof(int) * (count+1));
		arr[count] = descr;
		count++;
	}
	PLAYER_SET_DESCRCOUNT(player, count);
	PLAYER_SET_DESCRS(player, arr);
}

void
forget_player_descr(dbref player, int descr)
{
	int  count = 0;
	int* arr   = NULL;

	if (Typeof(player) != TYPE_PLAYER)
		return;

	count = PLAYER_DESCRCOUNT(player);
	arr = PLAYER_DESCRS(player);

	if (!arr) {
		count = 0;
	} else if (count > 1) {
		int src, dest;
		for (src = dest = 0; src < count; src++) {
			if (arr[src] != descr) {
				if (src != dest) {
					arr[dest] = arr[src];
				}
				dest++;
			}
		}
		if (dest != count) {
			count = dest;
			arr = (int*)realloc(arr,sizeof(int) * count);
		}
	} else {
		free((void*)arr);
		arr = NULL;
		count = 0;
	}
	PLAYER_SET_DESCRCOUNT(player, count);
	PLAYER_SET_DESCRS(player, arr);
}

void
remember_descriptor(struct descriptor_data *d)
{
	if (d) {
#ifdef WIN32
		descr_lookup_table[sethash_descr(d->descriptor)] = d;
#else
		descr_lookup_table[d->descriptor] = d;
#endif
	}
}

void
forget_descriptor(struct descriptor_data *d)
{
	if (d) {
#ifdef WIN32
		unsethash_descr(d->descriptor);
#else
		descr_lookup_table[d->descriptor] = NULL;
#endif
	}
}

struct descriptor_data *
lookup_descriptor(int c)
{
#ifdef WIN32
	if ( c < 0 ) return NULL;
	return descr_lookup_table[gethash_descr(c)];
#else 
	if (c >= FD_SETSIZE || c < 0) {
		return NULL;
	}
	return descr_lookup_table[c];
#endif
}

struct descriptor_data *
descrdata_by_descr(int i)
{
	return lookup_descriptor(i);
}


/*** JME ***/
int
online(dbref player)
{
	return PLAYER_DESCRCOUNT(player);
}

int
pcount(void)
{
    return current_descr_count;
}

int
pidle(int c)
{
	struct descriptor_data *d;
	time_t now;

	d = descrdata_by_count(c);

	(void) time(&now);
	if (d) {
		return (now - d->last_time);
	}

	return -1;
}

int
pdescridle(int c)
{
	struct descriptor_data *d;
	time_t now;

	d = descrdata_by_descr(c);

	(void) time(&now);
	if (d) {
		return (now - d->last_time);
	}

	return -1;
}

dbref
pdbref(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_count(c);

	if (d) {
		return (d->player);
	}

	return NOTHING;
}

dbref
pdescrdbref(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_descr(c);

	if (d) {
		return (d->player);
	}

	return NOTHING;
}

int
pontime(int c)
{
	struct descriptor_data *d;
	time_t now;

	d = descrdata_by_count(c);

	(void) time(&now);
	if (d) {
		return (now - d->connected_at);
	}

	return -1;
}

int
pdescrontime(int c)
{
	struct descriptor_data *d;
	time_t now;

	d = descrdata_by_descr(c);

	(void) time(&now);
	if (d) {
		return (now - d->connected_at);
	}
	return -1;
}

char *
phost(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_count(c);

	if (d) {
		return ((char *) d->hostname);
	}

	return (char *) NULL;
}

char *
pdescrhost(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_descr(c);

	if (d) {
		return ((char *) d->hostname);
	}

	return (char *) NULL;
}

char *
puser(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_count(c);

	if (d) {
		return ((char *) d->username);
	}

	return (char *) NULL;
}

char *
pdescruser(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_descr(c);

	if (d) {
		return ((char *) d->username);
	}

	return (char *) NULL;
}

/*** Foxen ***/
int
least_idle_player_descr(dbref who)
{
	struct descriptor_data *d;
	struct descriptor_data *best_d = NULL;
	int dcount, di;
	int* darr;
	time_t best_time = 0;

	darr = get_player_descrs(who, &dcount);
	for (di = 0; di < dcount; di++) {
		d = descrdata_by_descr(darr[di]);
		if (d && (!best_time || d->last_time > best_time)) {
			best_d = d;
			best_time = d->last_time;
		}
	}
	if (best_d) {
		return best_d->con_number;
	}
	return 0;
}


int
most_idle_player_descr(dbref who)
{
	struct descriptor_data *d;
	struct descriptor_data *best_d = NULL;
	int dcount, di;
	int* darr;
	time_t best_time = 0;

	darr = get_player_descrs(who, &dcount);
	for (di = 0; di < dcount; di++) {
		d = descrdata_by_descr(darr[di]);
		if (d && (!best_time || d->last_time < best_time)) {
			best_d = d;
			best_time = d->last_time;
		}
	}
	if (best_d) {
		return best_d->con_number;
	}
	return 0;
}


void
pboot(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_count(c);

	if (d) {
		process_output(d);
		d->booted = 1;
		/* shutdownsock(d); */
	}
}

int 
pdescrboot(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);

    if (d) {
		process_output(d);
		d->booted = 1;
		/* shutdownsock(d); */
		return 1;
    }
	return 0;
}


void
pnotify(int c, char *outstr)
{
	struct descriptor_data *d;

	d = descrdata_by_count(c);

	if (d) {
		queue_ansi(d, outstr);
		queue_write(d, "\r\n", 2);
	}
}


int
pdescrnotify(int c, char *outstr)
{
	struct descriptor_data *d;

	d = descrdata_by_descr(c);

	if (d) {
		queue_ansi(d, outstr);
		queue_write(d, "\r\n", 2);
		return 1;
	}
	return 0;
}


int
pdescr(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_count(c);

	if (d) {
		return (d->descriptor);
	}

	return -1;
}


int 
pdescrcount(void)
{
    return current_descr_count;
}


int 
pfirstdescr(void)
{
    struct descriptor_data *d;

	d = descrdata_by_count(1);
    if (d) {
		return d->descriptor;
	}

	return 0;
}


int 
plastdescr(void)
{
    struct descriptor_data *d;

	d = descrdata_by_count(current_descr_count);
	if (d) {
		return d->descriptor;
	}
	return 0;
}


int
pnextdescr(int c)
{
	struct descriptor_data *d;

    d = descrdata_by_descr(c);
	if (d) {
		d = d->next;
	}
	while (d && (!d->connected))
		d = d->next;
	if (d) {
		return (d->descriptor);
	}
	return (0);
}


int
pdescrcon(int c)
{
	struct descriptor_data *d;

    d = descrdata_by_descr(c);
	if (d) {
		return d->con_number;
	} else {
		return 0;
	}
}


int
pset_user(int c, dbref who)
{
	struct descriptor_data *d;
	static int setuser_depth = 0;

	if (++setuser_depth > 8) {
		/* Prevent infinite loops */
		setuser_depth--;
		return 0;
	}

    d = descrdata_by_descr(c);
	if (d && d->connected) {
		announce_disconnect(d);
		if (who != NOTHING) {
			d->player = who;
			d->connected = 1;
			update_desc_count_table();
            remember_player_descr(who, d->descriptor);
			announce_connect(d->descriptor, who);
		}
		setuser_depth--;
		return 1;
	}
	setuser_depth--;
	return 0;
}


int
dbref_first_descr(dbref c)
{
	int dcount;
	int* darr;

	darr = get_player_descrs(c, &dcount);
	if (dcount > 0) {
		return darr[dcount - 1];
	} else {
		return -1;
	}
}

int
pdescrflush(int c)
{
	struct descriptor_data *d;
	int i = 0;

	if (c != -1) {
		d = descrdata_by_descr(c);
		if (d) {
			if (!process_output(d)) {
				d->booted = 1;
			}
			i++;
		}
	} else {
		for (d = descriptor_list; d; d = d->next) {
			if (!process_output(d)) {
				d->booted = 1;
			}
			i++;
		}
	}
	return i;
}

int
pdescrsecure(int c)
{
#ifdef USE_SSL
	struct descriptor_data *d;

	d = descrdata_by_descr(c);

	if (d && d->ssl_session)
		return 1;
	else
		return 0;
#else
	return 0;
#endif
}

int
pdescrbufsize(int c)
{
	struct descriptor_data *d;

	d = descrdata_by_descr(c);

	if (d) {
		return (tp_max_output - d->output_size);
	}

	return -1;
}

dbref
partial_pmatch(const char *name)
{
	struct descriptor_data *d;
	dbref last = NOTHING;

	d = descriptor_list;
	while (d) {
		if (d->connected && (last != d->player) && string_prefix(NAME(d->player), name)) {
			if (last != NOTHING) {
				last = AMBIGUOUS;
				break;
			}
			last = d->player;
		}
		d = d->next;
	}
	return (last);
}


void
welcome_user(struct descriptor_data *d)
{
	FILE *f;
	char *ptr;
	char buf[BUFFER_LEN];

	if ((f = fopen(WELC_FILE, "rb")) == NULL) {
		queue_ansi(d, DEFAULT_WELCOME_MESSAGE);
		perror("spit_file: welcome.txt");
	} else {
		while (fgets(buf, sizeof(buf) - 3, f)) {
			ptr = index(buf, '\n');
			if (ptr && ptr > buf && *(ptr - 1) != '\r') {
				*ptr++ = '\r';
				*ptr++ = '\n';
				*ptr++ = '\0';
			}
			queue_ansi(d, buf);
		}
		fclose(f);
	}
	if (wizonly_mode) {
		queue_ansi(d, "## The game is currently in maintenance mode, and only wizards will be able to connect.\r\n");
	} else if (tp_playermax && con_players_curr >= tp_playermax_limit) {
		if (tp_playermax_warnmesg && *tp_playermax_warnmesg) {
			queue_ansi(d, tp_playermax_warnmesg);
			queue_string(d, "\r\n");
		}
	}
}

void
dump_status(void)
{
	struct descriptor_data *d;
	time_t now;
	char buf[BUFFER_LEN];

	(void) time(&now);
	log_status("STATUS REPORT:");
	for (d = descriptor_list; d; d = d->next) {
		if (d->connected) {
			snprintf(buf, sizeof(buf), "PLAYING descriptor %d player %s(%d) from host %s(%s), %s.\n",
					d->descriptor, NAME(d->player), d->player, d->hostname, d->username,
					(d->last_time) ? "idle %d seconds" : "never used");
		} else {
			snprintf(buf, sizeof(buf), "CONNECTING descriptor %d from host %s(%s), %s.\n",
					d->descriptor, d->hostname, d->username,
					(d->last_time) ? "idle %d seconds" : "never used");
		}
		log_status(buf, now - d->last_time);
	}
}

#ifdef USE_SSL

/* SSL_ERROR_WANT_ACCEPT is not defined in OpenSSL v0.9.6i and before. This
   fix allows to compile fbmuck on systems with an 'old' OpenSSL library, and
   yet have the server recognize the WANT_ACCEPT error when ran on systems with
   a newer OpenSSL version installed... Of course, should the value of the
   define change in ssl.h in the future (unlikely but not impossible), the
   define below would have to be changed too...
 */
#ifndef SSL_ERROR_WANT_ACCEPT
#define SSL_ERROR_WANT_ACCEPT 8
#endif

void
log_ssl_error(const char* text, int descr, int errnum)
{
	switch (errnum) {
		case SSL_ERROR_SSL:
			log_status("SSL %s: sock %d, Error SSL_ERROR_SSL", text, descr);
			break;
		case SSL_ERROR_WANT_READ:
			log_status("SSL %s: sock %d, Error SSL_ERROR_WANT_READ", text, descr);
			break;
		case SSL_ERROR_WANT_WRITE:
			log_status("SSL %s: sock %d, Error SSL_ERROR_WANT_WRITE", text, descr);
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			log_status("SSL %s: sock %d, Error SSL_ERROR_WANT_X509_LOOKUP", text, descr);
			break;
		case SSL_ERROR_SYSCALL:
			log_status("SSL %s: sock %d, Error SSL_ERROR_SYSCALL: %s", text, descr, strerror(errno));
			break;
		case SSL_ERROR_ZERO_RETURN:
			log_status("SSL %s: sock %d, Error SSL_ERROR_ZERO_RETURN", text, descr);
			break;
		case SSL_ERROR_WANT_CONNECT:
			log_status("SSL %s: sock %d, Error SSL_ERROR_WANT_CONNECT", text, descr);
			break;
		case SSL_ERROR_WANT_ACCEPT:
			log_status("SSL %s: sock %d, Error SSL_ERROR_WANT_ACCEPT", text, descr);
			break;
	}
}


ssize_t socket_read(struct descriptor_data *d, void *buf, size_t count) {
	int i;
 
	if (! d->ssl_session) {
#ifdef WIN32
		return recv(d->descriptor, (char *) buf, count, 0);
#else
		return read(d->descriptor, buf, count);
#endif
	} else {
		i = SSL_read(d->ssl_session, buf, count);
		if (i < 0) {
			i = SSL_get_error(d->ssl_session, i);
			if (i == SSL_ERROR_WANT_READ || i == SSL_ERROR_WANT_WRITE) {
				/* log_ssl_error("read 0", d->descriptor, i); */
#ifdef WIN32
				WSASetLastError(WSAEWOULDBLOCK);
#else
 				errno = EWOULDBLOCK;
#endif
				return -1;
			} else if (d->is_starttls && (i == SSL_ERROR_ZERO_RETURN || i == SSL_ERROR_SSL)) {
				/* log_ssl_error("read 1", d->descriptor, i); */
				d->is_starttls = 0;
				SSL_free(d->ssl_session);
				d->ssl_session = NULL;
#ifdef WIN32
				WSASetLastError(WSAEWOULDBLOCK);
#else
				errno = EWOULDBLOCK;
#endif
				return -1;
			} else {
				/* log_ssl_error("read 1", d->descriptor, i); */
#ifndef WIN32
				errno = EBADF;
#endif
				return -1;
			}
		}
		return i;
	}
}
 
ssize_t socket_write(struct descriptor_data *d, const void *buf, size_t count) {
	int i;

	d->last_pinged_at = time(NULL);
	if (! d->ssl_session) {
#ifdef WIN32
		return send(d->descriptor, (char *) buf, count, 0);
#else
		return write(d->descriptor, buf, count);
#endif
	} else {
		i = SSL_write(d->ssl_session, buf, count);
		if (i < 0) {
			i = SSL_get_error(d->ssl_session, i);
			if (i == SSL_ERROR_WANT_READ || i == SSL_ERROR_WANT_WRITE) {
				/* log_ssl_error("write 0", d->descriptor, i); */
#ifdef WIN32
				WSASetLastError(WSAEWOULDBLOCK);
#else
 				errno = EWOULDBLOCK;
#endif
				return -1;
			} else if (d->is_starttls && (i == SSL_ERROR_ZERO_RETURN || i == SSL_ERROR_SSL)) {
				/* log_ssl_error("write 1", d->descriptor, i); */
			    d->is_starttls = 0;
				SSL_free(d->ssl_session);
				d->ssl_session = NULL;
#ifdef WIN32
				WSASetLastError(WSAEWOULDBLOCK);
#else
 				errno = EWOULDBLOCK;
#endif
				return -1;
			} else { 
				/* log_ssl_error("write 2", d->descriptor, i); */
#ifndef WIN32
				errno = EBADF;
#endif
				return -1;
			}
		}
		return i;
	}
}
#endif

/* Ignore support -- Could do with moving into its own file */

static int ignore_is_ignoring_sub(dbref Player, dbref Who)
{
	int Top, Bottom;
	dbref* List;

	if (!tp_ignore_support)
		return 0;

	if ((Player < 0) || (Player >= db_top) || (Typeof(Player) == TYPE_GARBAGE))
		return 0;

	if ((Who < 0) || (Who >= db_top) || (Typeof(Who) == TYPE_GARBAGE))
		return 0;

	Player	= OWNER(Player);
	Who		= OWNER(Who);

	/* You can't ignore yourself, or an unquelled wizard, */
	/* and unquelled wizards can ignore no one. */
	if ((Player == Who) || (Wizard(Player)) || (Wizard(Who))) 
		return 0;

	if (PLAYER_IGNORE_LAST(Player) == AMBIGUOUS)
		return 0;

	/* Ignore the last player ignored without bothering to look them up */
	if (PLAYER_IGNORE_LAST(Player) == Who)
		return 1;

	if ((PLAYER_IGNORE_CACHE(Player) == NULL) && !ignore_prime_cache(Player))
		return 0;

	Top		= 0;
	Bottom	= PLAYER_IGNORE_COUNT(Player);
	List	= PLAYER_IGNORE_CACHE(Player);

	while(Top < Bottom)
	{
		int Middle = Top + (Bottom - Top) / 2;

		if (List[Middle] == Who)
			break;

		if (List[Middle] < Who)
			Top		= Middle + 1;
		else
			Bottom	= Middle;
	}

	if (Top >= Bottom)
		return 0;

	PLAYER_SET_IGNORE_LAST(Player, Who);
	
	return 1;
}

int ignore_is_ignoring(dbref Player, dbref Who)
{
	return ignore_is_ignoring_sub(Player, Who) || (tp_ignore_bidirectional && ignore_is_ignoring_sub(Who, Player));
}

static int ignore_dbref_compare(const void* Lhs, const void* Rhs)
{
	return *(dbref*)Lhs - *(dbref*)Rhs;
}

int ignore_prime_cache(dbref Player)
{
	const char* Txt = 0;
	const char* Ptr = 0;
	dbref* List = 0;
	int Count = 0;
	int i;

	if (!tp_ignore_support)
		return 0;

	if ((Player < 0) || (Player >= db_top) || (Typeof(Player) != TYPE_PLAYER))
		return 0;

	if ((Txt = get_property_class(Player, IGNORE_PROP)) == NULL)
	{
		PLAYER_SET_IGNORE_LAST(Player, AMBIGUOUS);
		return 0;
	}

	while(*Txt && isspace(*Txt))
		Txt++;

	if (*Txt == '\0')
	{
		PLAYER_SET_IGNORE_LAST(Player, AMBIGUOUS);
		return 0;
	}

	for(Ptr = Txt; *Ptr; )
	{
		Count++;

		if (*Ptr == NUMBER_TOKEN)
			Ptr++;

		while(*Ptr && !isspace(*Ptr))
			Ptr++;

		while(*Ptr && isspace(*Ptr))
			Ptr++;
	}

	if ((List = (dbref*)malloc(sizeof(dbref) * Count)) == 0)
		return 0;

	for(Ptr = Txt, i = 0; *Ptr; )
	{
		if (*Ptr == NUMBER_TOKEN)
			Ptr++;

		if (isdigit(*Ptr))
			List[i++] = atoi(Ptr);
		else
			List[i++] = NOTHING;

		while(*Ptr && !isspace(*Ptr))
			Ptr++;

		while(*Ptr && isspace(*Ptr))
			Ptr++;
	}

	qsort(List, Count, sizeof(dbref), ignore_dbref_compare);

	PLAYER_SET_IGNORE_CACHE(Player, List);
	PLAYER_SET_IGNORE_COUNT(Player, Count);

	return 1;
}

void ignore_flush_cache(dbref Player)
{
	if ((Player < 0) || (Player >= db_top) || (Typeof(Player) != TYPE_PLAYER))
		return;

	if (PLAYER_IGNORE_CACHE(Player))
	{
		free(PLAYER_IGNORE_CACHE(Player));
		PLAYER_SET_IGNORE_CACHE(Player, NULL);
		PLAYER_SET_IGNORE_COUNT(Player, 0);
	}

	PLAYER_SET_IGNORE_LAST(Player, NOTHING);
}

void ignore_flush_all_cache(void)
{
	int i;

	/* Don't touch the database if it's not been loaded yet... */
	if (db == 0)
		return;
	
	for(i = 0; i < db_top; i++)
	{
		if (Typeof(i) == TYPE_PLAYER)
		{
			if (PLAYER_IGNORE_CACHE(i))
			{
				free(PLAYER_IGNORE_CACHE(i));
				PLAYER_SET_IGNORE_CACHE(i, NULL);
				PLAYER_SET_IGNORE_COUNT(i, 0);
			}

			PLAYER_SET_IGNORE_LAST(i, NOTHING);
		}
	}
}

void ignore_add_player(dbref Player, dbref Who)
{
	if (!tp_ignore_support)
		return;

	if ((Player < 0) || (Player >= db_top) || (Typeof(Player) == TYPE_GARBAGE))
		return;

	if ((Who < 0) || (Who >= db_top) || (Typeof(Who) == TYPE_GARBAGE))
		return;

	reflist_add(OWNER(Player), IGNORE_PROP, OWNER(Who));

	ignore_flush_cache(OWNER(Player));
}

void ignore_remove_player(dbref Player, dbref Who)
{
	if (!tp_ignore_support)
		return;

	if ((Player < 0) || (Player >= db_top) || (Typeof(Player) == TYPE_GARBAGE))
		return;

	if ((Who < 0) || (Who >= db_top) || (Typeof(Who) == TYPE_GARBAGE))
		return;

	reflist_del(OWNER(Player), IGNORE_PROP, OWNER(Who));

	ignore_flush_cache(OWNER(Player));
}

void ignore_remove_from_all_players(dbref Player)
{
	int i;

	if (!tp_ignore_support)
		return;

	for(i = 0; i < db_top; i++)
		if (Typeof(i) == TYPE_PLAYER)
			reflist_del(i, IGNORE_PROP, Player);

	ignore_flush_all_cache();
}

#ifdef USE_SSL

/* Create a new SSl_CTX object. We do this rather than reconfiguring
   an existing one to allow us to recover gracefully if we can't reload
   a new certificate file, etc. */
static SSL_CTX *configure_new_ssl_ctx(void) {
        EC_KEY *eckey;
        int ssl_status_ok = 1;

	SSL_CTX* new_ssl_ctx = SSL_CTX_new (SSLv23_server_method ());

#ifdef SSL_OP_SINGLE_ECDH_USE
        /* As a default "optimization", OpenSSL shares ephemeral keys between sessions.
           Disable this to improve forward secrecy. */
        SSL_CTX_set_options(new_ssl_ctx, SSL_OP_SINGLE_ECDH_USE);
#endif

#ifdef SSL_OP_NO_TICKET
        /* OpenSSL supports session tickets by default but never rotates the keys by default.
           Since session resumption isn't important for MUCK performance and since this
           breaks forward secrecy, just disable tickets rather than trying to implement
           key rotation. */
        SSL_CTX_set_options(new_ssl_ctx, SSL_OP_NO_TICKET);
#endif

        /* Disable the session cache on the assumption that session resumption is not
           worthwhile given our long-lived connections. This also avoids any concern
           about session secret keys in memory for a long time. (By default, OpenSSL
           only clears timed out sessions every 256 connections.) */
        SSL_CTX_set_session_cache_mode(new_ssl_ctx, SSL_SESS_CACHE_OFF);

#if defined(SSL_CTX_set_ecdh_auto)
        /* In OpenSSL >= 1.0.2, this exists; otherwise, fallback to the older
           API where we have to name a curve. */
        SSL_CTX_set_ecdh_auto(new_ssl_ctx, 1);
#else
        eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        SSL_CTX_set_tmp_ecdh(new_ssl_ctx, eckey);
#endif

        SSL_CTX_set_cipher_list(new_ssl_ctx, tp_ssl_cipher_preference_list);

        if (tp_cipher_server_preference) {
            SSL_CTX_set_options(new_ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
        }

	if (!SSL_CTX_use_certificate_chain_file (new_ssl_ctx, SSL_CERT_FILE)) {
		log_status("Could not load certificate file %s", SSL_CERT_FILE);
		fprintf(stderr, "Could not load certificate file %s\n", SSL_CERT_FILE);
		ssl_status_ok = 0;
	}

	if (ssl_status_ok) {
		SSL_CTX_set_default_passwd_cb(new_ssl_ctx, pem_passwd_cb);
		SSL_CTX_set_default_passwd_cb_userdata(new_ssl_ctx, (void*)tp_ssl_keyfile_passwd);

		if (!SSL_CTX_use_PrivateKey_file (new_ssl_ctx, SSL_KEY_FILE, SSL_FILETYPE_PEM)) {
			log_status("Could not load private key file %s", SSL_KEY_FILE);
			fprintf(stderr, "Could not load private key file %s\n", SSL_KEY_FILE);
			ssl_status_ok = 0;
		}
	}

	if (ssl_status_ok) {
		if (!SSL_CTX_check_private_key (new_ssl_ctx)) {
			log_status("Private key does not check out and appears to be invalid.");
			fprintf(stderr, "Private key does not check out and appears to be invalid.\n");
			ssl_status_ok = 0;
		}
	}

	/* Set ssl_ctx to automatically retry conditions that would
	   otherwise return SSL_ERROR_WANT_(READ|WRITE) */
	if (ssl_status_ok) {
		SSL_CTX_set_mode(new_ssl_ctx,SSL_MODE_AUTO_RETRY);
	}

        if (!ssl_status_ok) {
                SSL_CTX_free(new_ssl_ctx);
                new_ssl_ctx = NULL;
        }

        return new_ssl_ctx;
}

static void bind_ssl_sockets(void) {
        int i = 0;
        if (ipv4_enabled) {
                for (i = 0; i < ssl_numports; i++) {
                        ssl_sock[i] = make_socket(ssl_listener_port[i]);
                        update_max_descriptor(ssl_sock[i]);
                        ssl_numsocks++;
                }
        }
# ifdef USE_IPV6
        if (ipv6_enabled) {
                for (i = 0; i < ssl_numports; i++) {
                        ssl_sock_v6[i] = make_socket_v6(ssl_listener_port[i]);
                        update_max_descriptor(ssl_sock_v6[i]);
                        ssl_numsocks_v6++;
                }
        }
# endif
}

/* Reinitialize or configure the SSL_CTX, allowing new connections to get any changed settings, most notably
   new certificate files. Then, if we haven't already, create the listening sockets for the SSL ports.
   
   Does not do one-time library OpenSSL library initailization.
*/
int reconfigure_ssl(void) {
        SSL_CTX *new_ssl_ctx;

        new_ssl_ctx = configure_new_ssl_ctx();

        if (new_ssl_ctx != NULL && ssl_numsocks == 0
#ifdef USE_IPV6
            && ssl_numsocks_v6 == 0
#endif
        ) {
            bind_ssl_sockets();
        }

        if (new_ssl_ctx != NULL) {
            if (ssl_ctx) {
                SSL_CTX_free(ssl_ctx);
            }
            ssl_ctx = new_ssl_ctx;
            return 1;
        } else {
            return 0;
        }
}
#endif
