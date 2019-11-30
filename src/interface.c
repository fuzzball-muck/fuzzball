/** @file interface.c
 *
 * This source has a wide range of different calls in it which (mostly)
 * have to do with the interface between players and the game but has some
 * random other stuff too.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "config.h"

#include "autoconf.h"
#include "commands.h"
#include "db.h"
#include "defines.h"
#include "edit.h"
#include "events.h"
#include "fbsignal.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "mcp.h"
#ifdef MCPGUI_SUPPORT
#include "mcpgui.h"
#endif
#include "mpi.h"
#include "mufevent.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

#ifdef USE_SSL
# include "interface_ssl.h"
# ifdef HAVE_OPENSSL
#  include <openssl/ssl.h>
# else
#  include <ssl.h>
# endif
#endif

#ifdef WIN32
typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
#endif


/**
 * @private
 * @var Constant message for output flushed response.  Output flushed means
 *      the output buffer size has been exceeded, it usually happens when
 *      a program is set DEBUG and dumps a bunch of stuff all at once.
 */
static const char *flushed_message = "<Output Flushed>\r\n";

/**
 * @private
 * @var Constant message for shutting down
 */
static const char *shutdown_message = "\r\nGoing down - Bye\r\n";

/**
 * @private
 * @var Resolver socket pair for working with the resolver process.
 *      Resolver uses [0] and parent uses [1].
 */
static int resolver_sock[2];

/**
 * @var The list of descriptors being managed.
 */
struct descriptor_data *descriptor_list = NULL;

/**
 * @var a cache of the max connected, logged in, players.
 *      This was, apparently, one of Cynbe's good ideas.
 *      This is also stored in the SYS_MAX_CONNECTS_PROP on #0
 */
static int con_players_max = 0;

/**
 * @var the count of currently connected, logged in, players
 */
static int con_players_curr = 0;

/**
 * @var this is the maximum number of ports we will open
 *
 * @TODO This should probably be moved to some proper configuration spot
 *       rather than randomly injected here.  This could even be dynamic
 *       though I think the level of effort isn't worth it.  In the
 *       improbable case of a MUCK wanting more ports, they can recompile :P
 */
#define MAX_LISTEN_SOCKS 16

/**
 * @private
 * @var The IPv6 IP address to bind to.  This will default to in6addr_any,
 *      but may be set by the command line.
 */
static struct in6_addr bind_ipv6_address;

/**
 * @private
 * @var The IPv4 IP address to bind to.  This will default to INADDR_ANY,
 *      but may be set by the command line.
 */
static uint32_t bind_ipv4_address;

/**
 * @private
 * @var The number of NON SSL ports we are listening to
 */
static size_t numports = 0;

/**
 * @private
 * @var the number of NON SSL IPv4 sockets we are listening to
 *
 * @TODO Is this ever different from numports ?  Maybe if you allocate
 *       too many ports, numports might be 1 greater than the number of
 *       sockets?  I'm not sure why this distinction is made.  It is
 *       a minor nitpick but if someone's bored and wants to nose
 *       at something easy, this is a good candidate :)
 */
static int numsocks = 0;

/**
 * @private
 * @var the highest descriptor number - this is needed for select()/pselect()
 */
static int max_descriptor = 0;

/**
 * @private
 * @var the array of NON SSL listening port numbers.  This defaults to
 *      listener_port[0] = TINYPORT define.
 */
static int listener_port[MAX_LISTEN_SOCKS];

/**
 * @private
 * @var the array of NON SSL IPv4 listening sockets
 */
static int sock[MAX_LISTEN_SOCKS];

/**
 * @private
 * @var the number of NON SSL IPv6 sockets we are listening to.
 *
 * @TODO This has the same note as above -- is this ever different from
 *       numports ?
 */
static int numsocks_v6 = 0;

/**
 * @private
 * @var the array of NON SSL IPv6 listening sockets
 */
static int sock_v6[MAX_LISTEN_SOCKS];

#ifdef USE_SSL

/**
 * @private
 * @var the number of SSL ports we are listening to
 */
static size_t ssl_numports = 0;

/**
 * @private
 * @var the number of IPv4 SSL sockets we are listening to
 *
 * @TODO This has the same note as above -- is this ever different from
 *       ssl_numports?
 */
static int ssl_numsocks = 0;

/**
 * @private
 * @var the SSL port numbers
 */
static int ssl_listener_port[MAX_LISTEN_SOCKS];

/**
 * @private
 * @var the SSL IPv4 sockets we are listening to
 */
static int ssl_sock[MAX_LISTEN_SOCKS];

/**
 * @private
 * @var the number of IPv6 SSL sockets we are listening to
 *
 * @TODO This has the same note as above -- is this ever different from
 *       ssl_numports?
 */
static int ssl_numsocks_v6 = 0;

/**
 * @private
 * @var the SSL IPv6 sockets we are listening to
 */
static int ssl_sock_v6[MAX_LISTEN_SOCKS];

/**
 * @private
 * @var our SSL context object, needed for various SSL calls
 */
static SSL_CTX *ssl_ctx = NULL;
#endif

/**
 * @private
 * @var the number of connected descriptors.  This may be different from
 *      the con_players_curr count, because this includes connections
 *      that are sitting on the login screen as well as logged in folks.
 */
static int ndescriptors = 0;

/**
 * @private
 * @var a mapping of descriptor numbers to descriptor data objects
 *
 * @TODO This is actually a pretty fragile way to do it.  First off,
 *       we should consider moving to poll.  There's a nice example
 *       of converting select to poll here:
 *
 * https://stackoverflow.com/questions/7976388/increasing-limit-of-fd-setsize-and-select
 *
 *       (I think its the third answer)
 *
 *       This should be either a dynamically resized table or a hash.
 *       I think a dynamically resized table is probably more appropriate,
 *       and it can start with a default size of FD_SETSIZE (which is 1024)
 *       and grow if needed.  It is unlikely (though possible) for a MUCK
 *       to have more than 1024 descriptors + sockets, so this is unlikely
 *       to increase memory usage but would allow it to expand if needed.
 *       (tanabi)
 */
static struct descriptor_data *descr_lookup_table[FD_SETSIZE];

/**
 * Is 'q' a valid input character?
 *
 * Does NOT support Unicode.
 *
 * @TODO Fix when we want to support Unicode
 *
 * @param q the character to check
 * @return boolean true if q is valid input, false if not
 */
#define isinput( q ) isprint( (q) & 127 )

/**
 * @private
 * @var If true, do not detach the MUCK as a daemon.
 */
static int no_detach_flag = 0;

/**
 * @private
 * @var Path to resolver path -- may be empty string to hunt for it.
 *
 * @TODO This variable is assumed to be filled with '\0' initially, which
 *       I believe is a valid assumption for a static, but it still makes
 *       me cringe.  Recommend we instead make this a dynamic allocated
 *       variable, default to NULL, and point it to the argv[..] entry for
 *       the resolver if we need it.  This means we shouldn't need to
 *       free(...) the memory while we're also not wasting a random
 *       BUFFER_LEN block of memory for a rarely used feature.  It also
 *       takes care of my bad initialization concerns.  (tanabi)
 */
static char resolver_program[BUFFER_LEN];

/**
 * @var If true, we are doing a "DB conversion" which is a command line
 *      process.  If this is true, most of the MUCK code is skipped.
 *      If this is true, the MUCK will not be detached.
 */
short db_conversion_flag = 0;

/**
 * @var Boolean, true if has the forked dump process has completed.
 */
short global_dumpdone = 0;

#ifndef DISKBASE
/**
 * @var PID of the forked dump process - unused for DISKBASE - 0 if not running
 */
pid_t global_dumper_pid = 0;
#endif

/**
 * @var PID of the forked resolver process or 0 if not running
 */
pid_t global_resolver_pid = 0;

/**
 * @var If true, the MUCK will restart.
 */
int restart_flag = 0;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
time_t sel_prof_start_time;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
long sel_prof_idle_sec;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
long sel_prof_idle_usec;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
unsigned long sel_prof_idle_use;

/**
 * @var Boolean, if true, shut down the MUCK.
 */
int shutdown_flag = 0;

/**
 * @var boolean, if true, only wizards may log in ("maintenance mode")
 */
short wizonly_mode = 0;

/**
 * Display program usage information (command line help)
 *
 * This will exit() with status 1.
 *
 * @private
 * @param prog the string name of the program (argv[0] usually)
 */
static void
show_program_usage(char *prog)
{
    fprintf(stderr,
        "Usage: %s [<options>] [infile [outfile [portnum [portnum ...]]]]\n",
        prog);
    fprintf(stderr,
"    Arguments:\n"
"        infile           db file loaded at startup.  optional with -dbin.\n"
"        outfile          output db file to save to.  optional with -dbout.\n"
"        portnum          port num to listen for conns on. (16 ports max)\n"
"    Options:\n"
"        -dbin INFILE     use INFILE as the database to load at startup.\n"
"        -dbout OUTFILE   use OUTFILE as the output database to save to.\n"
"        -port NUMBER     sets the port number to listen for connections on.\n"
#ifdef USE_SSL
"        -sport NUMBER    sets the port number for secure connections\n"
#else
"        -sport NUMBER    Ignored.  SSL support isn't compiled in.\n"
#endif
"        -gamedir PATH    changes directory to PATH before starting up.\n"
"        -parmfile PATH   replace the system parameters with those in PATH.\n"
"        -convert         load the db, then save and quit.\n"
"        -nosanity        don't do db sanity checks at startup time.\n"
"        -insanity        load db, then enter the interactive sanity editor.\n"
"        -sanfix          attempt to auto-fix a corrupt db after loading.\n"
"        -wizonly         only allow wizards to login.\n"
"        -godpasswd PASS  reset God(#1)'s password to PASS.  Implies -convert\n"
"        -bindv4 ADDRESS  set listening IP address for IPv4 sockets (default: all)\n"
"        -bindv6 ADDRESS  set listening IP address for IPv6 sockets (default: all)\n"
"        -nodetach        do not detach server process\n"
"        -resolver PATH   path to fb-resolver program\n"
"        -version         display this server's version.\n"
"        -help            display this message.\n"
#ifdef WIN32
"        -freeconsole     free the console window and run in the background\n"
#endif
    );

    exit(1);
}

/**
 * Update max descriptor count
 *
 * This is actually done in a number of places so it makes sense to
 * centralize this logic.
 *
 * @private
 * @param descr the newly added descriptor to consider as the new maximum
 */
static inline void
update_max_descriptor(int descr)
{
    if (descr >= max_descriptor) {
        max_descriptor = descr + 1;
    }
}

/**
 * Update the command burst "quotas"
 *
 * The "quotas" refer to the number of commands a player can submit in
 * a given time period (tp_commands_per_time, tp_command_time_msec,
 * and tp_command_burst_size are the relevant tune variables).
 *
 * This does the book-keeping around keeping track of this quota and
 * is run at the top of each player interaction loop.
 *
 * Has a concept of a time slice; tp_command_time_msec is the time
 * slice interval.  This action only does something on the next time
 * slice after 'last'.
 *
 * @private
 * @param last the last time slice stamp
 * @param current the current time
 * @return the new time slice stamp
 */
static struct timeval
update_quotas(struct timeval last, struct timeval current)
{
    int nslices;
    int cmds_per_time;

    nslices = msec_diff(current, last) / tp_command_time_msec;

    /* Only perform actions on a new time slice */
    if (nslices > 0) {
        for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
            if (d->connected) {
                cmds_per_time = ((FLAGS(d->player) & INTERACTIVE)
                                ? (tp_commands_per_time * 8)
                                : tp_commands_per_time);
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

/**
 * Does a given descriptor have output to write?
 *
 * @private
 * @param d the decsriptor to check for output.
 * @return boolean true if d has output to write, false otherwise.
 */
static int
has_output(struct descriptor_data *d)
{
#ifdef USE_SSL
    if (d->pending_ssl_write.head)
        return 1;
#endif

    if (d->priority_output.head)
        return 1;

    if (!d->block_writes && d->output.head)
        return 1;

    return 0;
}

/**
 * Free a text block
 *
 * Text blocks are used to track output to players (primarily).
 * This frees a block and its associated text buffer.
 *
 * @private
 * @param t the block to free
 */
static inline void
free_text_block(struct text_block *t)
{
    free(t->buf);
    free(t);
}

/**
 * Allocate a new text block for 'n' bytes coming from 's'
 *
 * Note that no \0 provision is made here.  Exactly 'n' bytes is
 * allocated, and exactly 'n' bytes is copied, so if null termination
 * is required it is up to the caller.  Consumers be aware that string
 * functions cannot rely on a null terminator for text_block strings.
 *
 * Text blocks could technically be arbitrary bytes, such as telnet
 * control characters, so this situation does make sense.
 *
 * @private
 * @param s the source bytes
 * @param n the number of bytes to allocate, then copy from s
 * @return a new struct text_block
 */
static struct text_block *
make_text_block(const char *s, size_t n)
{
    struct text_block *p;

    if (!(p = malloc(sizeof(struct text_block))))
        panic("make_text_block: Out of memory");

    if (!(p->buf = malloc(n * sizeof(char))))
        panic("make_text_block: Out of memory");

    memmove(p->buf, s, n);
    p->nchars = n;
    p->start = p->buf;
    p->nxt = 0;
    return p;
}

/**
 * Add a new message to the given queue
 *
 * @private
 * @param q the queue to add the message to
 * @param b the message
 * @param n the number of bytes from message to allocate and copy
 */
static void
add_to_queue(struct text_queue *q, const char *b, size_t n)
{
    struct text_block *p;

    p = make_text_block(b, n);
    p->nxt = 0;
    *q->tail = p;
    q->tail = &p->nxt;
    q->lines++;
}

/**
 * For a given descriptor 'd' flush all output above size_limit
 *
 * This attempts to ensure only size_limit number of text blocks are in the
 * output queue.
 *
 * @private
 * @param d the descriptor_data to process queue data for
 * @param size_limit the number of blocks to limit to
 */
static void
flush_output_queue(struct descriptor_data *d, int size_limit)
{
    int space = d->output_size - size_limit;

    if (space > 0) {
        struct text_block *p;
        struct text_queue *q = &d->output;
        int n = space + strlen(flushed_message);
        int really_flushed = 0;

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

        d->output_size -= really_flushed;
    }
}

/**
 * Write a message to the output queue, respecting the maximum queue size
 *
 * tp_max_output determines the maximum number of items that can be in
 * the output queue at a time.  Practically speaking, this problem usually
 * only comes up when programs are set DEBUG and the output floods the user.
 *
 * The queue is flushed, then the new message added.
 *
 * @see flush_output_queue
 * @see add_to_queue
 *
 * If 'n' is 0, this returns immediately.
 *
 * @param d the descriptor_data to queue output to
 * @param b pointer to some bytes to queue up
 * @param n the number of bytes to allocate then copy from 'b'
 */
void
queue_write(struct descriptor_data *d, const char *b, size_t n)
{
    if (n == 0)
        return;

    flush_output_queue(d, (size_t)tp_max_output - n);
    add_to_queue(&d->output, b, n);
    d->output_size += n;
    return;
}

/**
 * Queue ANSI-enabled (color, etc.) text to the given descriptor
 *
 * This handles checking to see if the player is CHOWN_OK.  If not, the
 * ANSI is stripped out.  If the player is not connected, ANSI is stripped.
 *
 * If the player is set CHOWN_OK, then "bad" ANSI is still stripped.
 *
 * @see strip_bad_ansi
 * @see strip_ansi
 *
 * If MCP is enabled, this will filter things through the MCP system to
 * see if it is MCP output that needs escaping or other special handling.
 *
 * @see mcp_frame_output_inband
 *
 * @private
 * @param d the descriptor_data to queue output to
 * @param msg the string message to queue up
 * @return the number of bytes queued (may be less than 'msg' due to stripping)
 */
static int
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

    mcp_frame_output_inband(&d->mcpframe, buf);

    return strlen(buf);
}

#ifdef IP_FORWARDING
/**
 * Detect if 'd' is a connection coming from localhost
 *
 * @private
 * @param d the descriptor to check
 * @return boolean true if 'd' is coming from localhost
 */
static int
is_local_connection(struct descriptor_data *d)
{
    /*
     * @TODO First off this is probably a bad way to do this; I think
     *       we would be better off flagging the 'd' as local in a
     *       variable in descriptor_data's struct because it looks like we
     *       potentially hit this a lot and it is doing strcmps which is
     *       gross.
     *
     *       Further, we can detect if a connection is coming from localhost
     *       when we accept the connection by looking at the addr structure,
     *       which both removes the need for the strcmp, fixes the ipv6 issue,
     *       and generally removes the purpose of this function as you can
     *       just lookup d->is_local (or whatever) instead.
     */
    /* Treat anything coming from 127.0.0.1 or localhost as local */    
    if (!strcmp(d->hostname, "127.0.0.1") ||
        !strcmp(d->hostname, "localhost"))
        return 1;
    else
        return 0;
}
#endif

/**
 * Dump user information to a given descriptor_data
 *
 * This is the implementation of the system WHO command.  The somewhat
 * awkward use of the descriptor struct instead of the player ref or struct
 * is because this also supports typing WHO from the connect screen, which
 * would be a descriptor with no player yet ("not connected" by MUCK lingo).
 *
 * User must be provided.  It will either be "" to list all users, or
 * a prefix to filter names by.
 *
 * @see string_prefix
 *
 * If 'user' starts with the '*' character, and the descriptor 'e' belongs
 * to a connected WIZARD player, then the WHO listing will be in wizard
 * mode and will show host names.
 *
 * @private
 * @param e the descriptor to send the WHO data to.
 * @param user user name prefix, "", or "*.."
 */
static void
dump_users(struct descriptor_data *e, char *user)
{
    struct descriptor_data *d;
    int wizard, players;
    time_t now;
    char buf[2048];
    char pbuf[64];
    char secchar = ' '; /* Security symbol -- @ or blank space */

    wizard = 0;

    /*
     * Figure out if we are in wizard mode, advance past leading *s
     */
    while (*user && (isspace(*user) || *user == '*')) {
        if (*user == '*' && e->connected && Wizard(e->player))
            wizard = 1;

        user++;
    }

    if (wizard) {
        char *log_name = whowhere(e->player);

        /* S/he is connected and not quelled. Okay; log it. */
        log_command("%s: %s", log_name, WHO_COMMAND);
        free(log_name);
    }

    if (!*user)
        user = NULL;

    (void) time(&now);

    if (wizard) {
        queue_ansi(e,
            "Player Name                Location     On For Idle   Host\r\n"
        );
    } else {
        queue_ansi(e,
            "Player Name           On For Idle   Doing...\r\n"
        );
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
            /*
             * Secure means using SSL or coming from localhost or being
             * forwarded.
             */
            if (d->ssl_session
#ifdef IP_FORWARDING
               || d->forwarding_enabled || is_local_connection(d)
#endif
            )
                secchar = '@';
            else
                secchar = ' ';
#else
            secchar = ' ';
#endif

            if (wizard) {
                /* don't print flags, to save space */
                snprintf(pbuf, sizeof(pbuf), "%.*s(#%d)",
                         tp_player_name_limit + 1, NAME(d->player),
                         (int) d->player);


                /*
                 * @TODO This ifdef is pretty ugly.  I don't like the
                 *       dangling 'else' on the end, it strikes me as
                 *       fragile -- if the line after the #endif ever
                 *       changes or starts relying on more than 1 line
                 *       then this could break in a hard to find way.
                 *
                 *       I don't have a really good suggestion here,
                 *       I just have several bad suggestions :)
                 *
                 *       Bad suggestion one: make a static constant
                 *       variable that is '1' if GOD_PRIV is defined
                 *       and false otherwise.  Then use that in a proper
                 *       if statement here.
                 *
                 *       On seond thought, I don't think my other suggestions
                 *       are worth providing.  Feel free to come up with your
                 *       own (tanabi)
                 */
#ifdef GOD_PRIV
                if (!God(e->player))
                    snprintf(buf, sizeof(buf),
                             "%-*s [%6d] %10s %4s%c%c %s\r\n",
                             tp_player_name_limit + 10, pbuf,
                             (int) LOCATION(d->player),
                             time_format_1(now - d->connected_at),
                             time_format_2(now - d->last_time),
                             ((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
                             secchar, d->hostname);
                else
#endif
                snprintf(buf, sizeof(buf),
                         "%-*s [%6d] %10s %4s%c%c %s(%s)\r\n",
                         tp_player_name_limit + 10, pbuf,
                         (int) LOCATION(d->player),
                         time_format_1(now - d->connected_at),
                         time_format_2(now - d->last_time),
                         ((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
                         secchar, d->hostname, d->username);
            } else {
                /* Modified to take into account tp_player_name_limit changes */
                snprintf(buf, sizeof(buf), "%-*s %10s %4s%c%c %.*s\r\n",
                         tp_player_name_limit + 1,
                         NAME(d->player),
                         time_format_1(now - d->connected_at),
                         time_format_2(now - d->last_time),
                         ((FLAGS(d->player) & INTERACTIVE) ? '*' : ' '),
                         secchar, (int) (79 - (tp_player_name_limit + 20)),
                         GETDOING(d->player) ? GETDOING(d->player) : "");
            }

            queue_ansi(e, buf);
        }

        d = d->next;
    }

    /*
     * This is an odd place to update this variable.  It doesn't hurt,
     * but there's another place this also gets updated which happens
     * more frequently than this.  This code, on a MUCK with a replacement
     * WHO program, might never get called (or rarely, only when users on
     * the connect screen type WHO when that feature is enabled).
     */
    if (players > con_players_max)
        con_players_max = players;

    snprintf(buf, sizeof(buf), "%d player%s %s connected.  (Max was %d)\r\n",
             players, (players == 1) ? "" : "s", (players == 1) ? "is" : "are",
             con_players_max);

    queue_ansi(e, buf);
}

/**
 * Add a command to a descriptor's input queue
 *
 * A command is a null terminated string.
 *
 * @private
 * @param d the descriptor to add the command to
 * @param command the command to add
 */
static void
save_command(struct descriptor_data *d, const char *command)
{
    add_to_queue(&d->input, command, strlen(command) + 1);
}

/**
 * Process a connect screen message (msg) into command components.
 *
 * This takes a command string 'msg' that will be something typed
 * in to the welcome screen and breaks it out nto 'command', 'user',
 * and 'pass' strings.
 *
 * 'command', 'user', and 'pass' should all be allocated buffers
 * that will have their component of the 'msg' string copied into
 * them (or they will be set to "").  They should probably all be
 * BUFFER_LEN in size even if that is kind of wasteful -- just for
 * safety sake.  To be more efficient, you could dynamically allocate
 * them all to 'strlen(msg)+1' in size.
 *
 * @private
 * @param msg the message to parse
 * @param command the destination buffer for the command portion
 * @param user the destination buffer for the user portion
 * @param pass the destination buffer for the password portion.
 */
static void
parse_connect(const char *msg, char *command, char *user, char *pass)
{
    char *p;

    skip_whitespace(&msg);
    p = command;

    while (*msg && isinput(*msg) && !isspace(*msg))
        *p++ = tolower(*msg++);

    *p = '\0';
    skip_whitespace(&msg);
    p = user;

    while (*msg && isinput(*msg) && !isspace(*msg))
        *p++ = *msg++;

    *p = '\0';
    skip_whitespace(&msg);
    p = pass;

    while (*msg && isinput(*msg) && !isspace(*msg))
        *p++ = *msg++;

    *p = '\0';
}

/**
 * Dump a text file to the given descriptor
 *
 * This is extremely similar to spit_file_segment, except spit_file_degment
 * does a lot of "extra stuff" and takes a player parameter instead of a descriptor.
 *
 * This code seems very duplicate as a result, but the calling patterns
 * are very different -- this is more for "unconnected" or rather folks
 * that aren't logged in, vs. people who are.
 *
 * @private
 * @param d the descriptor to transmit to
 * @param filename the file to read and send to 'd'
 */
static void
show_file(struct descriptor_data *d, const char *filename)
{
    FILE *f;
    char buf[BUFFER_LEN];

    if ((f = fopen(filename, "r")) == NULL) {
        queue_ansi(d, "This content is missing - management has been notified.\r\n");
        fputs("show_file:", stderr);
        perror(filename);
    } else {
        while (fgets(buf, sizeof(buf), f)) {
            queue_ansi(d, buf);
        }

        fclose(f);
    }
}

/**
 * Broadcasts a message via all of player's puppets (ZOMBIE objects)
 *
 * This is used for the "has woke up" and "has fallen asleep" message
 * that ZOMBIE objects emit when their owner connects or disconnects.
 *
 * 'msg' should be a message with no name and no leading space.  Puppet's
 * name and a space will automatically be prefixed. "prop" contains the
 * prop name of a property who's value can override 'msg'.
 *
 * Messages are not emitted for zombies in dark locations, owned by dark
 * players, or for zombies that are set dark.
 *
 * @private
 * @param player the player connecting
 * @param msg the default message
 * @param prop the property that, if set, will have a value overriding msg
 */
static void
announce_puppets(dbref player, const char *msg, const char *prop)
{
    dbref where;
    const char *ptr, *msg2;
    char buf[BUFFER_LEN];

    /*
     * @TODO This is brutal.  Scan the entire DB every time someone
     *       logs in or disconnects?  Yuck.  This is a great reason
     *       to use a relational database because there really isn't
     *       a better way to do this, short of maintaining a linked
     *       list of owned items for each player which would be nasty
     *       in other ways.
     */
    for (dbref what = 0; what < db_top; what++) {
        if (Typeof(what) == TYPE_THING && (FLAGS(what) & ZOMBIE)) {
            if (OWNER(what) == player) {
                where = LOCATION(what);

                if ((!Dark(where)) && (!Dark(player)) && (!Dark(what))) {
                    msg2 = msg;

                    if ((ptr = (char *) get_property_class(what, prop)) && *ptr)
                        msg2 = ptr;

                    snprintf(buf, sizeof(buf), "%.512s %.3000s",
                             NAME(what), msg2);
                    notify_except(CONTENTS(where), what, buf, what);
                }
            }
        }
    }
}

/**
 * Announces a player connection, with associated 'business logic'
 *
 * If the player is dark, or in a dark location, no announcement is made.
 * If the player is connected exactly once (i.e. first connection, not a
 * reconnect with an existing connection in place), then the 'connect'
 * action is run.  Connect propqueues are run regardless.
 *
 * This triggers either the tp_autolook_cmd if set, or look_room is called
 * if not.  @see look_room
 *
 * Also it triggers zombie notification and planet has connected messages.
 *
 * @private
 * @param descr the descriptor to output to for to-player messages
 * @param player the player connection
 */
static void
announce_connect(int descr, dbref player)
{
    dbref loc = LOCATION(player);
    char buf[BUFFER_LEN];
    struct match_data md;
    dbref exit;

    if ((!Dark(player)) && (!Dark(loc))) {
        snprintf(buf, sizeof(buf), "%s has connected.", NAME(player));
        notify_except(CONTENTS(loc), player, buf, player);
    }

    exit = NOTHING;

    if (PLAYER_DESCRCOUNT(player) == 1) {
        /* match for connect */
        init_match(descr, player, "connect", TYPE_EXIT, &md);
        md.match_level = 1;
        match_all_exits(&md);
        exit = match_result(&md);

        if (exit == AMBIGUOUS)
            exit = NOTHING;
    }

    if (can_move(descr, player, tp_autolook_cmd, 1)) {
        do_move(descr, player, tp_autolook_cmd, 1);
    } else {
        look_room(descr, player, LOCATION(player));
    }

    /*
     * See if there's a connect action.  If so, and the player is the first to
     * connect, send the player through it.
     */

    if (exit != NOTHING)
        do_move(descr, player, "connect", 1);

    if (PLAYER_DESCRCOUNT(player) == 1) {
        announce_puppets(player, "wakes up.", MESGPROP_PCON);
    }

    /* 
     * Queue up all _connect programs referred to by properties
     *
     * @TODO Something odd -- propqueue's are run on every connect but
     *       'connect' action is only run if the player is connecting the
     *       first time (i.e. not on the reconnects).  Personally, I would
     *       expect both of these things to run every connect.  I'm not sure
     *       we want to fix it because it has been this way forever and maybe
     *       something MUCKs depend on?
     *
     *       I would actually bet the 'connect' action is rare enough that
     *       nobody ever noticed it worked differently.  We may want to
     *       consider make it consistent.
     */
    envpropqueue(descr, player, LOCATION(player), NOTHING, player, NOTHING,
                 CONNECT_PROPQUEUE, "Connect", 1, 1);
    envpropqueue(descr, player, LOCATION(player), NOTHING, player, NOTHING,
                 OCONNECT_PROPQUEUE, "Oconnect", 1, 0);

    ts_useobject(player);
    return;
}

/**
 * Warn if the user if connecting into an INTERACTIVE state
 *
 * Lets the player know what state they are in.
 *
 * - INTERACTIVE + READMODE is in a program
 * - INTERACTIVE + PLAYER_INSERT_MODE is player inserting MUF text
 * - INTERACTIVE by itself is player in MUF editor, but not inserting
 *
 * No message is displayed if not applicable.
 *
 * @private
 * @param player the player to check
 */
static void
interact_warn(dbref player)
{
    if (FLAGS(player) & INTERACTIVE) {
        char buf[BUFFER_LEN];

        if (FLAGS(player) & READMODE) {
            snprintf(buf, sizeof(buf),
                     "***  You are currently using a program.  Use \"%s\" to return to a more reasonable state of control.  ***",
                     BREAK_COMMAND);
        } else if (PLAYER_INSERT_MODE(player)) {
            snprintf(buf, sizeof(buf),
                     "***  You are currently inserting MUF program text.  Use \"%s\" to return to the editor, then \"%c\" if you wish to return to your regularly scheduled MUCK universe.  ***",
                     EXIT_INSERT, QUIT_EDIT_COMMAND);
        } else {
            snprintf(buf, sizeof(buf),
                     "***  You are currently using the MUF program editor.  ***");
        }

        notify(player, buf);
    }
}

/**
 * Display welcome banner to the user
 *
 * Also displays maintenance mode warning if applicable, or the
 * maximum player limit warning.
 *
 * Welcome message can come from the WELCOME_PROPLIST on #0, or
 * from the tp_file_welcome_screen.
 *
 * @private
 * @param d the descriptor to send the welcome screen to
 */
static void
welcome_user(struct descriptor_data *d)
{
    FILE *f;
    char *ptr;
    char buf[BUFFER_LEN];

    int welcome_proplist = 0;

    /*
     * @TODO This is kind of a weird way to read a list.  If I were doing
     *       it, I would iterate over the value of WELCOME_PROPLIST#
     *       which is the number of lines rather than ... just iterate
     *       til props stop loading.  I think that is more technically
     *       correct / expected.  If you do it this way, 'welcome_proplist'
     *       can become the integer value of WELCOME_PROPLIST's length
     *       prop instead of a boolean set every iteration of the loop.
     */
    for (int i = 1; ; i++) {
        const char *line;
        snprintf(buf, sizeof(buf), "%s#/%d", WELCOME_PROPLIST, i);

        if ((line = get_property_class(GLOBAL_ENVIRONMENT, buf))) {
            welcome_proplist = 1;
            queue_ansi(d, line);
            queue_write(d, "\r\n", 2);
        } else {
            break;
        }
    }

    if (!welcome_proplist) {
        /*
         * @TODO Use show_file instead?
         *       Could add a return value to show_file which returns
         *       boolean if show_file was able to load, and show
         *       the default message if it returns false.  Removes
         *       duplicate code and makes this function more tidy.
         */

        if ((f = fopen(tp_file_welcome_screen, "rb")) == NULL) {
            queue_ansi(d, DEFAULT_WELCOME_MESSAGE);
            perror("spit_file: welcome.txt");
        } else {
            while (fgets(buf, sizeof(buf) - 3, f)) {
                ptr = strchr(buf, '\n');
                if (ptr && ptr > buf && *(ptr - 1) != '\r') {
                    *ptr++ = '\r';
                    *ptr++ = '\n';
                    *ptr++ = '\0';
                }
                queue_ansi(d, buf);
            }

            fclose(f);
        }
    }

    if (wizonly_mode) {
        queue_ansi(d,
                   "## The game is currently in maintenance mode, and only "
                   "wizards will be able to connect.\r\n");
    } else if (tp_playermax && con_players_curr >= tp_playermax_limit) {
        if (tp_playermax_warnmesg && *tp_playermax_warnmesg) {
            queue_ansi(d, tp_playermax_warnmesg);
            queue_write(d, "\r\n", 2);
        }
    }
}

/**
 * Add a descriptor to a given player
 *
 * @private
 * @param player the player to add the descriptor to
 * @param descr the descriptor to add to the player
 */
static void
remember_player_descr(dbref player, int descr)
{
    size_t count = 0;
    int *arr = NULL;

    if (Typeof(player) != TYPE_PLAYER)
        return;

    count = (size_t)PLAYER_DESCRCOUNT(player);
    arr = PLAYER_DESCRS(player);

    if (!arr) {
        arr = malloc(sizeof(int));
        arr[0] = descr;
        count = 1;
    } else {
        arr = realloc(arr, sizeof(int) * (count + 1));
        arr[count] = descr;
        count++;
    }

    PLAYER_SET_DESCRCOUNT(player, count);
    PLAYER_SET_DESCRS(player, arr);
}

/**
 * Remove a given descriptor from a given player
 *
 * @private
 * @param player the player to remove the descriptor from
 * @param descr the descriptor to remove
 */
static void
forget_player_descr(dbref player, int descr)
{
    size_t count = 0;
    int *arr = NULL;

    if (Typeof(player) != TYPE_PLAYER)
        return;

    count = (size_t)PLAYER_DESCRCOUNT(player);
    arr = PLAYER_DESCRS(player);

    if (!arr) {
        count = 0;
    } else if (count > 1) {
        size_t dest = 0;

        /*
         * This is a somewhat clunky way to do this, but, it would
         * be a pretty rare case for this descriptor array to be
         * larger than one or two entries, so it doesn't matter too
         * much.
         */
        for (unsigned int src = 0; src < count; src++) {
            if (arr[src] != descr) {
                if (src != dest) {
                    arr[dest] = arr[src];
                }

                dest++;
            }
        }

        if (dest != count) {
            count = dest;
            arr = realloc(arr, sizeof(int) * count);
        }
    } else {
        free(arr);
        arr = NULL;
        count = 0;
    }

    PLAYER_SET_DESCRCOUNT(player, count);
    PLAYER_SET_DESCRS(player, arr);
}

/***** O(1) Connection Optimizations *****/

/**
 * @private
 * @var mapping of "connection numbers" to descriptors
 *
 *      This, on the surface, looks a lot like descr_lookup_table, but it
 *      is slightly different.  This is a linear array of descriptors that
 *      are in no particular order.  Meaning, slots in this table are re-used
 *      over time, so the index in this table has no correspondence to
 *      descriptor numbers.  This is for what the MUCK calls "connection
 *      numbers" in one of the more confusing bits of MUCKery.
 *
 *      Really, why do we have descriptors and connection numbers?  Getting
 *      rid of this would complicated MUFs that rely on DESCRCON/CONDESCR,
 *      so I am not suggesting we get rid of it ... but I do think it is
 *      something stupid that we are now married to.
 *
 * @TODO Actually, now that I think about it, would there be any drawback
 *       to making descriptors and connection numbers synonomymous?
 *       'descrcon' and 'condescr' prims would then just become no-ops.
 *       Anything that uses connections could use descriptors instead.
 *       There must be some reason for this connection vs. descriptor thing
 *       that I don't get.
 *
 * @TODO Make this table more dynamic -- if you make descr_lookup_table more
 *       dynamic to support poll, then this needs to change and grow/shrink
 *       dynamically as well.  Or if you follow the first todo, then this goes
 *       away entirely.
 */
static struct descriptor_data *descr_count_table[FD_SETSIZE];

/**
 * @private
 * @var the current descriptor count - which would be the size of the
 *      descr_count_table
 */
static int current_descr_count = 0;

/**
 * Initialize the descriptor count lookup table to all NULLs
 *
 * @private
 */
static void
init_descr_count_lookup()
{
    for (int i = 0; i < FD_SETSIZE; i++) {
        descr_count_table[i] = NULL;
    }
}

/**
 * Compact the descriptor count table, removing all disconnected descriptors
 *
 * Note that old entries are not NULLed out, so one must always check
 * current_descr_count and make sure not to overrun it, as you will
 * probably segfault.  You cannot iterate over descr_count_table until you
 * find a NULL entry.
 *
 * @private
 */
static void
update_desc_count_table()
{
    int c = 0;

    current_descr_count = 0;

    for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
        if (d->connected) {
            d->con_number = c + 1;
            descr_count_table[c++] = d;
            current_descr_count++;
        }
    }
}

/**
 * Get a descriptor data structure from its connection count number
 *
 * @private
 * @param c the connection count number
 * @return the descriptor data structure
 */
static struct descriptor_data *
descrdata_by_count(int c)
{
    c--;

    if (c >= current_descr_count || c < 0) {
        return NULL;
    }

    return descr_count_table[c];
}

#ifdef WIN32
/**
 * @var Windows doesn't handle descriptors the same way that UNIX does,
 *      and so the descriptors have to be handled differently.
 *
 *      It isn't really a hash; it is a flat array that is iterated over
 *      to find the descriptor number.
 *
 * @TODO This system will need to change to make this 'hash' table dynamically
 *       sized if we make this support poll.
 */
int descr_hash_table[FD_SETSIZE];

/**
 * Add a descriptor to the hash table
 *
 * @private
 * @param d the descriptor to add to the table
 * @return the index into the hash table, or -1 if the table is full
 */
static int
sethash_descr(int d)
{
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (descr_hash_table[i] == -1) {
            descr_hash_table[i] = d;
            return i;
        }
    }

    fprintf(stderr, "descr hash table full !");
    return -1;
}

/**
 * Resolve a descriptor hash index into a descriptor
 *
 * @private
 * @param d the has index
 * @return the descriptor corresponding to 'd' or -1 if not found
 */
static int
gethash_descr(int d)
{
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (descr_hash_table[i] == d)
            return i;
    }

    fprintf(stderr, "descr hash value missing !"); /* Should NEVER happen */
    return -1;

}

/**
 * Remove a descriptor from the descriptor "hash" table.
 *
 * @private
 * @param d the descriptor to remove from the "hash" table
 */
static void
unsethash_descr(int d)
{
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (descr_hash_table[i] == d) {
            descr_hash_table[i] = -1;
            return;
        }
    }

    fprintf(stderr, "descr hash value missing !");
}
#endif

/**
 * Initialize the descriptor lookup table
 *
 * @private
 */
static void
init_descriptor_lookup()
{
#ifdef WIN32
    for (int i = 0; i < FD_SETSIZE; i++) {
        descr_hash_table[i] = -1;
    }
#endif
    for (int i = 0; i < FD_SETSIZE; i++) {
        descr_lookup_table[i] = NULL;
    }
}

/**
 * Fetch a descriptor number from an index in the descriptor table.
 *
 * Note that for UNIX, 'index' is always going to be the same as the
 * returned descriptor.  This is more for the benefit of Windows,
 * which has sort of a complicated descriptor abstraction going on
 * (or at least that's how it seems to work).
 *
 * @param index the index in the descriptor table to fetch
 * @return the descriptor associated with that index
 */
int
index_descr(int index)
{
    if ((index < 0) || (index >= FD_SETSIZE))
        return -1;

    if (descr_lookup_table[index] == NULL)
        return -1;

    return descr_lookup_table[index]->descriptor;
}

/**
 * Add a descriptor_data entry to the descriptor lookup table
 *
 * For windows, this uses the abstraction of the hash table,
 * and the index into remember_descriptor will be the return
 * value of sethash_descr
 *
 * @see sethash_descr
 *
 * @private
 * @param d the descriptor_data to add to the descr_lookup_table
 */
static void
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

/**
 * Remove a descriptor_data structure from the descriptor lookup table
 *
 * Note that this does not free memory, but it does set the slot in the
 * table NULL.
 *
 * @private
 * @param d the descriptor_data structure to remove
 */
static void
forget_descriptor(struct descriptor_data *d)
{
    if (d) {
#ifdef WIN32
        unsethash_descr(d->descriptor);

        /*
         * @TODO Bug: if win32, don't we need to remove the HASHED
         *       d->descriptor rather than d->descriptor?  This seems
         *       like it will just set some poor random soul NULL in
         *       the descr_lookup_table :)  Could be that Windows
         *       descriptors such high numbers vs. the hashed numbers
         *       that we've been lucky and never over-written active
         *       entries with NULL :P
         *
         *       Why do we do this stupid hash thing anyway?  What does
         *       it matter if descriptors are sequential and relatively
         *       low numbered?  This system just seems weird and flawed
         *       to me, but it could be my lack of understanding of
         *       WinSock. (tanabi)
         */
#else
        descr_lookup_table[d->descriptor] = NULL;
#endif
    }
}

/**
 * Get the descriptor_data associated with a given descriptor number
 *
 * For UNIX, 'c' is the descriptor and is just an index in the table.
 * For Windows, 'c' gets hashed with gethash_descr then that index's
 * value is returned.
 *
 * @see gethash_descr
 *
 * @param c the descriptor to lookup
 * @return the descriptor_data corresponding to c or NULL if not found
 */
struct descriptor_data *
descrdata_by_descr(int c)
{
#ifdef WIN32
    if (c < 0)
        return NULL;

    return descr_lookup_table[gethash_descr(c)];
#else
    if (c >= FD_SETSIZE || c < 0) {
        return NULL;
    }

    return descr_lookup_table[c];
#endif
}

/**
 * Take the initial steps towards connecting a player
 *
 * This looks up player name 'name' (which, oddly enough, can be a dbref
 * ref number -- I never knew that! -tanabi) and checks the password.
 *
 * @see check_password
 *
 * If the player is found and the password matches, then the player's
 * ref is returned.  Otherwise, NOTHING is returned.
 *
 * This doesn't "really" connect a player though, so the name is a bit
 * of a misnomer.  It's more of a "validate" or "authenticate", because
 * it doesn't do any of the book keeping needed to log the player into
 * the game.
 *
 * @private
 * @param name the player name to look up
 * @param password the player's password
 * @return the dbref of the player or NOTHING if authentication fails.
 */
static dbref
connect_player(const char *name, const char *password)
{
    dbref player;

    if (*name == NUMBER_TOKEN && number(name + 1) && atoi(name + 1)) {
        player = (dbref) atoi(name + 1);
        if (!ObjExists(player) || Typeof(player) != TYPE_PLAYER)
            player = NOTHING;
    } else {
        player = lookup_player(name);
    }

    if (player == NOTHING)
        return NOTHING;

    if (!check_password(player, password))
        return NOTHING;

    return player;
}

/**
 * Input processing for the welcome screen / pre-connect screen
 *
 * 'check_connect' is another misnomer.  This is more like
 * 'process_welcome_input' or just 'check_welcome' or 'welcome_input' or
 * something.
 *
 * Anyway, it handles the commands understood by the welcome screen:
 * create, connect, and help.  For create and connect, only the first
 * two characters are checked.
 *
 * If the player successfully creates or connects, the descriptor's state
 * will be changed to connected which will take them past the login
 * screen, and all the stuff that happens on connect -- motd, connect
 * announce, etc. -- is all done.
 *
 * @private
 * @param d the descriptor structure of the user
 * @param msg the unprocessed message typed in from the user
 */
static void
check_connect(struct descriptor_data *d, const char *msg)
{
    char command[MAX_COMMAND_LEN];
    char user[MAX_COMMAND_LEN];
    char password[MAX_COMMAND_LEN];
    dbref player;

    parse_connect(msg, command, user, password);

    /*
     * @TODO Create and connect have a lot of overlapping code.
     *       Worse, create seems to be missing some checks that connect
     *       does.
     *
     *       Instead, I would suggest something like this:
     *
     *  if ( command is help ) {
     *      display help
     *  } else if ( command is create ) {
     *      do_create = 1
     *  } else if ( command is connect ) {
     *      do_connect = 1
     *  } else if ( !*command ) {
     *      return
     *  } else {
     *      do_welcome and return
     *  }
     *
     *  put the 'connect' code here, with a conditional to create vs.
     *  connect_player.  Make sure all conditions are met for both
     *  cases.
     */

    if (!strncmp(command, "co", 2)) {
        player = connect_player(user, password);

        if (player == NOTHING) {
            queue_ansi(d, tp_connect_fail_mesg);
            queue_write(d, "\r\n", 2);
            log_status("FAILED CONNECT %s on descriptor %d", user,
                       d->descriptor);
        } else {
            if ((wizonly_mode ||
                 (tp_playermax && con_players_curr >= tp_playermax_limit)) &&
                 !TrueWizard(player)) {
                if (wizonly_mode) {
                    queue_ansi(d,
                               "Sorry, but the game is in maintenance mode "
                               "currently, and only wizards are allowed to "
                               "connect.  Try again later.");
                } else {
                    queue_ansi(d, tp_playermax_bootmesg);
                }

                queue_write(d, "\r\n", 2);
                d->booted = 1;
            } else {
                log_status("CONNECTED: %s(%d) on descriptor %d",
                           NAME(player), player, d->descriptor);
#ifdef USE_SSL

                if (d->ssl_session) {
                    log_status("Connected via %s",
                               SSL_get_cipher_name(d->ssl_session));
                }
#endif
                d->connected = 1;
                d->connected_at = time(NULL);
                d->player = player;
                update_desc_count_table();
                remember_player_descr(player, d->descriptor);

                /* cks: someone has to initialize this somewhere. */
                PLAYER_SET_BLOCK(d->player, 0);
                show_file(d, tp_file_motd);
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
            if (wizonly_mode
                || (tp_playermax && con_players_curr >= tp_playermax_limit)) {
                if (wizonly_mode) {
                    queue_ansi(d,
                               "Sorry, but the game is in maintenance mode "
                               "currently, and only wizards are allowed to "
                               "connect.  Try again later.");
                } else {
                    queue_ansi(d, tp_playermax_bootmesg);
                }

                queue_write(d, "\r\n", 2);
                d->booted = 1;
            } else {
                player = create_player(user, password);

                if (player == NOTHING) {
                    queue_ansi(d, tp_create_fail_mesg);
                    queue_write(d, "\r\n", 2);
                    log_status("FAILED CREATE %s on descriptor %d", user,
                               d->descriptor);
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
                    spit_file_segment(player, tp_file_motd, "");
                    announce_connect(d->descriptor, player);
                    con_players_curr++;
                }
            }
        } else {
            queue_ansi(d, tp_register_mesg);
            queue_write(d, "\r\n", 2);
            log_status("FAILED CREATE %s on descriptor %d", user, d->descriptor);
        }
    } else if (!strncmp(command, "help", 4)) {
        show_file(d, tp_file_connection_help);
    } else if (!*command) {
        /* do nothing */
    } else {
        welcome_user(d);
    }
}

/**
 * Process command input from the given descriptor
 *
 * This is where the common input processing for a descriptor is done.
 * If MCP is enabled, it is run through mcp_frame_process_input first.
 * Then it updates the player's last use time if connected.
 *
 * @see mcp_frame_process_input
 *
 * This processes certain built-in 'special' commands; @Q (BREAK_COMMAND),
 * QUIT, and WHO.  Then it hands off to process_command or check_connect
 * based on if you're connected or not.
 *
 * @see process_command
 * @see check_connect
 *
 * @private
 * @param d the descriptor data structure
 * @param command the entered command to process
 * @return boolean true if successfully processed, false to disconnect user
 */
static int
do_command(struct descriptor_data *d, char *command)
{
    char buf[BUFFER_LEN];
    char cmdbuf[BUFFER_LEN];

    if (!mcp_frame_process_input(&d->mcpframe, command, cmdbuf, sizeof(cmdbuf))) {
        d->quota++;
        return 1;
    }

    command = cmdbuf;

    if (d->connected)
        ts_lastuseobject(d->player);

    if (!strcasecmp(command, BREAK_COMMAND)) {
        if (!d->connected)
            return 0;

        if (dequeue_prog(d->player, 2)) {
            queue_ansi(d, "Foreground program aborted.\r\n");

            if ((FLAGS(d->player) & INTERACTIVE))
                if ((FLAGS(d->player) & READMODE))
                    process_command(d->descriptor, d->player, command);
        }

        PLAYER_SET_BLOCK(d->player, 0);
        return 1;
    } else if (!strcmp(command, QUIT_COMMAND)) {
        return 0;
    } else if (tp_recognize_null_command && !strcasecmp(command, NULL_COMMAND)) {
        return 1;
    } else if ((!tp_cmd_only_overrides &&
               (!strncmp(command, WHO_COMMAND, sizeof(WHO_COMMAND) - 1))) ||
               (*command == OVERRIDE_TOKEN &&
               (!strncmp(command + 1, WHO_COMMAND, sizeof(WHO_COMMAND) - 1))
              )) {
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
                (*command == OVERRIDE_TOKEN))) &&
                can_move(d->descriptor, d->player, buf, 2)) {
                do_move(d->descriptor, d->player, buf, 2);
            } else {
                dump_users(d, command + sizeof(WHO_COMMAND) -
                           ((*command == OVERRIDE_TOKEN) ? 0 : 1));
            }
        }
    } else {
        if (d->connected) {
            process_command(d->descriptor, d->player, command);
        } else {
            check_connect(d, command);
        }
    }

    return 1;
}

/**
 * Check if 'cmd' is one of the special commands handled in interface.c
 *
 * This checks for certain built-in 'special' commands; @Q (BREAK_COMMAND),
 * QUIT (QUIT_COMMAND), and WHO (WHO_COMMAND).  Also checks for
 * MCP message prefix.
 *
 * @private
 * @param cmd the command to check
 * @return boolean true if any of the above match the command, false otherwise
 */
static int
is_interface_command(const char *cmd)
{
    const char *tmp = cmd;

    if (!strncmp(tmp, MCP_QUOTE_PREFIX, 3)) {
        /* dequote MCP quoting. */
        tmp += 3;
    }

    if (!strncmp(cmd, MCP_MESG_PREFIX, 3)) /* MCP mesg. */
        return 1;

    if (!strcasecmp(tmp, BREAK_COMMAND))
        return 1;

    if (!strcmp(tmp, QUIT_COMMAND))
        return 1;

    if (!strncmp(tmp, WHO_COMMAND, strlen(WHO_COMMAND)))
        return 1;

    if (tp_recognize_null_command && !strcasecmp(tmp, NULL_COMMAND))
        return 1;

    return 0;
}

/**
 * Iterate over all the descriptors and process 1 block off each input queue
 *
 * For each descriptor, we grab what is on d->input.head, process it as
 * needed, and advance the queue pointers.  So at most one command is
 * processed per descriptor per call.
 *
 * @private
 */
static void
process_commands(void)
{
    int nprocessed;
    struct descriptor_data *dnext;
    struct text_block *t;

    do {
        nprocessed = 0;

        for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
            dnext = d->next;

            if (d->quota > 0 && (t = d->input.head)) {
                if (d->connected && PLAYER_BLOCK(d->player)
                    && !is_interface_command(t->start)) {
                    char *tmp = t->start;

                    if (!strncmp(tmp, MCP_QUOTE_PREFIX, 3)) {
                        /* Un-escape MCP escaped lines */
                        tmp += 3;
                    }

                    /*
                     * WORK: send player's foreground/preempt programs an
                     *       exclusive READ mufevent
                     */
                    
                    /*
                     * @TODO If this returns true, then you've got a problem,
                     *       because nothing will advance the queue other
                     *       than output being flushed.  Is that a problem?
                     *       We should probably dig into what can happen
                     *       here.
                     */
                    if (!read_event_notify(d->descriptor, d->player, tmp)
                        && !*tmp) {
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
                    if (strncmp(t->start, MCP_MESG_PREFIX, 3)) {
                        /* Not an MCP mesg, so count this against quota. */
                        d->quota--;
                    }

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

/**
 * Do an SSL-agnostic socket read for descriptor 'd'
 *
 * Reads 'count' bytes into 'buf.  Returns the number of bytes read
 * or -1 on error.  'errno' will be set to either EWOULDBLOCK if
 * reading would cause a block condition.  Otherwise it is probably
 * a legitimate error.
 *
 * @private
 * @param d the descriptor to read from
 * @param buf the buffer to write to
 * @param count the number of bytes to transfer
 * @return the number of bytes read, or -1 on failure.
 */
static ssize_t
socket_read(struct descriptor_data *d, void *buf, size_t count)
{
    int i;

#ifdef USE_SSL
    if (!d->ssl_session) {
#endif
        /* If no SSL, this is the only line in this function */
        return read(d->descriptor, buf, count);
#ifdef USE_SSL
    } else {
        i = SSL_read(d->ssl_session, buf, count);

        if (i < 0) {
            i = ssl_check_error(d, i, ssl_logging_stream);

            if (i == SSL_ERROR_WANT_READ || i == SSL_ERROR_WANT_WRITE) {
                // "read 0" error situation
#ifdef WIN32
                WSASetLastError(WSAEWOULDBLOCK);
#else
                errno = EWOULDBLOCK;
#endif
                return -1;
            } else if (d->is_starttls
                       && (i == SSL_ERROR_ZERO_RETURN
                       || i == SSL_ERROR_SSL)) {
                // "read 1" error situation
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
                // "read 1" error situation
#ifndef WIN32
                errno = EBADF;
#endif
                return -1;
            }
        }

        return i;
    }
#endif // USE_SSL
}

/**
 * Do an SSL agnostic socket write
 *
 * Writes up to 'count' bytes of 'buf' to descriptor d.  There's
 * no promise that all those bytes will be written.  Will be -1
 * on error with 'errno' having a proper error such as EWOULDBLOCK
 * or something nastier.
 *
 * @private
 * @param d the descriptor structure to write to
 * @param buf the buffer to write from
 * @param count the number of bytes to write
 * @return the number of bytes actually written
 */
static ssize_t
socket_write(struct descriptor_data * d, const void *buf, size_t count)
{
    int i;

    d->last_pinged_at = time(NULL);

#ifdef USE_SSL
    if (!d->ssl_session) {
#endif
        /* If no SSL, this is the only line in this function */
        return write(d->descriptor, buf, count);
#ifdef USE_SSL
    } else {
        i = SSL_write(d->ssl_session, buf, count);

        if (i < 0) {
            i = ssl_check_error(d, i, ssl_logging_stream);

            if (i == SSL_ERROR_WANT_WRITE || i == SSL_ERROR_WANT_READ) {
#ifdef WIN32
                WSASetLastError(WSAEWOULDBLOCK);
#else
                errno = EWOULDBLOCK;
#endif
                return -1;
            } else if (d->is_starttls
                       && (i == SSL_ERROR_ZERO_RETURN || i == SSL_ERROR_SSL)) {
                // "write 1" error situation
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
                // "write 2" error situation
#ifndef WIN32
                errno = EBADF;
#endif
                return -1;
            }
        }

        return i;
    }
#endif // USE_SSL
}

/**
 * Add a message to a given descriptor's priority output queue
 *
 * Priority output is processed ahead of regular output.  This is done
 * mostly for telnet protocol items.
 *
 * @private
 * @param d the descriptor data to queue the message to
 * @param msg the message to queue - must be a NULL terminated string.
 */
static void
queue_immediate_raw(struct descriptor_data *d, const char *msg)
{
    /*
     * @TODO: This function is actually pretty stupid, and kind of a
     *        misnomer because 'msg' is not really "raw" ... it must
     *        be a string.  Maybe just copy/paste this one line
     *        everywhere queue_immediate_raw is used instead?
     *        Or not if I'm just being stupid nitpicky. -tanabi
     */
    add_to_queue(&d->priority_output, msg, strlen(msg));
}

/**
 * Flush the output queue queue msg in the immediate queue, and process output
 *
 * @private
 * @param d the descriptor to queue and flush
 * @param msg the message to queue -- must be a null terminated string!
 */
static void
queue_immediate_and_flush(struct descriptor_data *d, const char *msg)
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

    flush_output_queue(d, 0);

    if (d->mcpframe.enabled
        && !(strncmp(buf, MCP_MESG_PREFIX, 3) && strncmp(buf, MCP_QUOTE_PREFIX, 3))) {
        queue_immediate_raw(d, MCP_QUOTE_PREFIX);
    }

    queue_immediate_raw(d, (const char *) buf);
    process_output(d);
}

/**
 * Display the leave message, flushing whatever other output is in the queue
 *
 * @private
 * @param d the descriptor to be well.
 */
static void
goodbye_user(struct descriptor_data *d)
{
    char buf[BUFFER_LEN];
    snprintf(buf, sizeof(buf), "\r\n%s\r\n\r\n", tp_leave_mesg);
    queue_immediate_and_flush(d, buf);
}

/**
 * Display the idle boot message, flushing the output queue first.
 *
 * @private
 * @param d the descriptor to send the message to.
 */
static void
idleboot_user(struct descriptor_data *d)
{
    char buf[BUFFER_LEN];
    snprintf(buf, sizeof(buf), "\r\n%s\r\n\r\n", tp_idle_boot_mesg);
    queue_immediate_and_flush(d, buf);
    d->booted = 1;
}

/**
 * Free a text queue and all the nodes on it.
 *
 * This frees all associated memory.
 *
 * @private
 * @param q the queue to free
 */
static void
free_queue(struct text_queue *q)
{
    struct text_block *cur, *next;

    cur = q->head;

    while (cur) {
        next = cur->nxt;
        free_text_block(cur);
        cur = next;
    }

    q->lines = 0;
    q->head = NULL;
    q->tail = &q->head;
}

/**
 * Free all queues on a descriptor_data structure
 *
 * @private
 * @param d the descriptor to clear queues from.
 */
static void
freeqs(struct descriptor_data *d)
{
#ifdef USE_SSL
    free_queue(&d->pending_ssl_write);
#endif
    free_queue(&d->priority_output);
    free_queue(&d->output);
    free_queue(&d->input);
    free(d->raw_input);
    d->raw_input = 0;
    d->raw_input_at = 0;
}

/**
 * Handle disconnect messaging and other related items
 *
 * This is the disconnect equivalent of announce_connect
 *
 * @see announce_connect
 *
 * If the player is in a foreground program, it will be dequeued and an
 * abort message shown.  Then the player has disconnected message will show
 * up if the player isn't dark or in a dark location.
 *
 * If the player is only connected once, then it will run the 'disconnect'
 * action if there and uses announce_puppets to put all the puppets to sleep.
 *
 * @see announce_puppets
 *
 * Then it does MCP cleanup, runs forget_player_descr, and runs the
 * disconnect propqueues.
 *
 * @see forget_player_descr
 *
 * @private
 * @param d the descriptor that is disconnecting
 */
static void
announce_disconnect(struct descriptor_data *d)
{
    dbref player = d->player;
    dbref loc = LOCATION(player);
    char buf[BUFFER_LEN];
    int dcount;

    get_player_descrs(d->player, &dcount);

    if (dcount < 2 && dequeue_prog(player, 2))
        notify(player, "Foreground program aborted.");

    if ((!Dark(player)) && (!Dark(loc))) {
        snprintf(buf, sizeof(buf), "%s has disconnected.", NAME(player));
        notify_except(CONTENTS(loc), player, buf, player);
    }

    /* trigger local disconnect action */
    if (PLAYER_DESCRCOUNT(player) == 1) {
        if (can_move(d->descriptor, player, "disconnect", 1)) {
            do_move(d->descriptor, player, "disconnect", 1);
        }

        announce_puppets(player, "falls asleep.", MESGPROP_PDCON);
    }

#ifdef MCPGUI_SUPPORT
    gui_dlog_closeall_descr(d->descriptor);
#endif

    d->connected = 0;
    d->player = NOTHING;

    forget_player_descr(player, d->descriptor);
    update_desc_count_table();

    /*
     * Queue up all _disconnect programs referred to by properties
     *
     * @TODO Something odd -- propqueue's are run on every disconnect but
     *       'disconnect' action is only run if the player is disconnecting the
     *       last connection.  Personally, I would expect both of these things
     *       to run every disconnect.  I'm not sure we want to fix it because
     *       it has been this way forever and maybe something MUCKs depend on?
     *
     *       I would actually bet the 'disconnect' action is rare enough that
     *       nobody ever noticed it worked differently.  We may want to
     *       consider make it consistent.
     */
    envpropqueue(d->descriptor, player,
                 LOCATION(player), NOTHING, player, NOTHING,
                 DISCONNECT_PROPQUEUE, "Disconnect", 1, 1);
    envpropqueue(d->descriptor, player,
                 LOCATION(player), NOTHING, player, NOTHING,
                 ODISCONNECT_PROPQUEUE, "Odisconnect", 1, 0);

    ts_lastuseobject(player);
    DBDIRTY(player);
}

/**
 * Shut down a descriptor, free 'd's memory, and remove it from queues
 *
 * This will run announce_disconnect if the player is connected.
 *
 * @see announce_disconnect
 *
 * Cleans up all memory used by d, including strings and queues.
 * Then free's d itself after removing it from descriptor queue's
 *
 * @private
 * @param d the descriptor to shut down
 */
static void
shutdownsock(struct descriptor_data *d)
{
    if (d->connected) {
        log_status("DISCONNECT: descriptor %d player %s(%d) from %s(%s)",
                   d->descriptor, NAME(d->player), d->player, d->hostname,
                   d->username);
        announce_disconnect(d);
    } else {
        log_status("DISCONNECT: descriptor %d from %s(%s) never connected.",
                   d->descriptor, d->hostname, d->username);
    }

    shutdown(d->descriptor, 2);
    close(d->descriptor);
    forget_descriptor(d);
    freeqs(d);
    *d->prev = d->next;

    if (d->next)
        d->next->prev = d->prev;

    free((void *) d->hostname);
    free((void *) d->username);

#ifdef IP_FORWARDING
    free(d->forwarded_buffer);
#endif

    mcp_frame_clear(&d->mcpframe);

#ifdef USE_SSL
    if (d->ssl_session)
        SSL_free(d->ssl_session);
#endif

    free(d);
    ndescriptors--;
    log_status("CONCOUNT: There are now %d open connections.", ndescriptors);
}

/*
 * The idea here is to make sure O_NONBLOCK is set no matter which operating
 * system we are on.
 *
 * This seems somewhat out of place, just randomly injected here.  Might be
 * more of a config thing?
 *
 * @TODO Why do we include WIN32 in this block?  In make_nonblocking,
 *       we use a #ifdef for WIN32 ... why not just *not* use O_NONBLOCK
 *       in that block?  It will reduce some of the complexity here.
 */
#ifdef WIN32
# define O_NONBLOCK 1
#else
# if !defined(O_NONBLOCK) || defined(ULTRIX)
#  ifdef FNDELAY /* SUN OS */
#   define O_NONBLOCK FNDELAY
#  else
#   ifdef O_NDELAY /* SyseVil */
#    define O_NONBLOCK O_NDELAY
#   endif
#  endif
# endif
#endif

/**
 * Make a socket non-blocking
 *
 * @private
 * @param s the socket to set non-blocking
 */
static void
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

/**
 * Make a socket 'close on exec'
 *
 * FD_CLOEXEC makes it so a socket closes when an exec*() call is done.
 * For instance, when forking the resolver.
 *
 * @private
 * @param s the socket to set the close on exc flag on
 */
static
void make_cloexec(int s)
{
#ifdef FD_CLOEXEC
    int flags;

    if ((flags = fcntl(s, F_GETFD, 0)) == -1) {
        perror("make_cloexec: fcntl F_GETFD");
        panic("fcntl F_GETFD failed");
    }

    if (fcntl(s, F_SETFD, flags | FD_CLOEXEC) == -1) {
        perror("make_cloexec: fcntl F_SETFD");
        panic("fcntl F_SETFD failed");
    }
#endif
}

/**
 * Initialize and return a descriptor_data structure.
 *
 * This adds the descriptor data to the list, registers it in the tables,
 * and initializes everything nicely so the returned descriptor is ready
 * to be used.
 *
 * This also sends over the welcome message and initializes MCP negotiation,
 * among a lot of other little minor (but important) book-keeping items.
 *
 * @see welcome_user
 *
 * @private
 * @param s the descriptor socket
 * @param hostname the descriptor host name
 * @param is_ssl boolean, is SSL connection or not?
 * @return the initialized descriptor struct.
 */
static struct descriptor_data *
initializesock(int s, const char *hostname, int is_ssl)
{
    struct descriptor_data *d;
    char buf[128], *ptr;

    if (!(d = malloc(sizeof(struct descriptor_data))))
        panic("initializesock: Out of memory");

    memset(d, 0, sizeof(struct descriptor_data));

    d->descriptor = s;

#ifdef USE_SSL
    d->pending_ssl_write.tail = &d->pending_ssl_write.head;
#endif

    d->player = NOTHING;
    d->connected_at = time(NULL);

    make_nonblocking(s);
    make_cloexec(s);

    d->priority_output.tail = &d->priority_output.head;
    d->output.tail = &d->output.head;
    d->input.tail = &d->input.head;
    
#ifdef IP_FORWARDING
    if (!(d->forwarded_buffer = malloc(128 * sizeof(char))))
        panic("initializesock: Out of memory");
#endif

    d->quota = tp_command_burst_size;
    d->last_time = d->connected_at;
    d->last_pinged_at = d->connected_at;

    mcp_frame_init(&d->mcpframe, d);

    strcpyn(buf, sizeof(buf), hostname);
    ptr = strchr(buf, ')');

    if (ptr)
        *ptr = '\0';

    ptr = strchr(buf, '(');
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
            TELNET_IAC, TELNET_DO, TELOPT_STARTTLS, '\0'
        };

        queue_immediate_raw(d, (const char *)telnet_do_starttls);
        queue_write(d, "\r\n", 2);
    }
#endif

    mcp_negotiation_start(&d->mcpframe);

    welcome_user(d);
    return d;
}

/**
 * Create an IPv6 socket and bind it to the given port
 *
 * Uses the global 'bind_ipv6_address' for the address binding.
 * Does not 'listen' on the socket so that this function can be called
 * by a privileged user, then we can listen after we've dropped
 * privileges.
 *
 * @private
 * @param port the port number
 * @return the socket
 */
static int
make_socket_v6(int port)
{
    int s;
    int opt;
    struct sockaddr_in6 server;

    /* Create the socket */
    s = socket(AF_INET6, SOCK_STREAM, 0);

    if (s < 0) {
        perror("creating stream socket");
        exit(3);
    }

    /* Set all the different socket options */
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

    /* Blank out the server structure */
    memset((char *) &server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_addr = bind_ipv6_address;
    server.sin6_port = htons(port);

    /* Bind it */
    if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
        perror("binding stream socket");
        close(s);
        exit(4);
    }

    make_cloexec(s);

    /*
     * We separate the binding of the socket and the listening on the socket
     * to support binding to privileged ports, then dropping privileges
     * before listening.  Listening now happens in listen_bound_sockets(),
     * called during shovechars().  This function is now called before the
     * checks for MUD_GID and MUD_ID in main().
     */
    return s;
}

/**
 * Translate IPV6 address 'a' from addr struct to text.
 *
 * This resolves the IPv6 address 'a' to either an IP string or a hostname
 * based on if we can resolve it, and how we're resolving it.
 *
 * If the resolver is spawned, we will temporarily use the IPv6 address
 * until the resolver responds.
 *
 * Uses a static buffer, so be careful with it; copy the string out if
 * needed.
 *
 * @private
 * @param lport the port this user connected to
 * @param a the address to resolve
 * @param prt boolean true if SSL, false if not
 * @return pointer to static buffer with host name or address in string form
 */
static const char *
addrout_v6(in_port_t lport, struct in6_addr *a, in_port_t prt)
{
    static char buf[128];
    char ip6addr[128];

    struct in6_addr addr;
    memcpy(&addr.s6_addr, a, sizeof(struct in6_addr));

    prt = ntohs(prt);

#ifndef SPAWN_HOST_RESOLVER
    if (tp_use_hostnames) {
        /*
         * One day the nameserver Qwest uses decided to start
         * doing halfminute lags, locking up the entire muck
         * that long on every connect.  This is intended to
         * prevent that, reduces average lag due to nameserver
         * to 1 sec/call, simply by not calling nameserver if
         * it's in a slow mood *grin*. If the nameserver lags
         * consistently, a hostname cache ala OJ's tinymuck2.3
         * would make more sense:
         */
        static time_t secs_lost = 0;

        if (secs_lost) {
            secs_lost--;
        } else {
            time_t gethost_start = time(NULL);

            struct hostent *he = gethostbyaddr(((char *) &addr),
                                               sizeof(addr), AF_INET6);
            time_t gethost_stop = time(NULL);
            time_t lag = gethost_stop - gethost_start;

            if (lag > 10) {
                secs_lost = lag;
            }

            if (he) {
                snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", he->h_name, prt);
                return buf;
            }
        }
    }
#endif /* !SPAWN_HOST_RESOLVER */

    inet_ntop(AF_INET6, a, ip6addr, 128);

#ifdef SPAWN_HOST_RESOLVER
    snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")%" PRIu16 "\n", ip6addr, prt,
             lport);

    if (tp_use_hostnames) {
        write(resolver_sock[1], buf, strlen(buf));
    }
#endif
    snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")\n", ip6addr, prt);

    return buf;
}

/**
 * Accept an IPv6 connection
 *
 * Returns either a newly allocated struct descriptor_data if we were
 * able to accept() from sock_ or NULL if there was nothing to accept.
 * (Or theoretically on error -- errno would be set).
 *
 * @private
 * @param port the port number we accepted this connection from
 * @param sock_ the socket to accept the connection from
 * @param is_ssl boolean, if true, sock_ is an SSL socket
 * @return either a descriptor_data structure or NULL if nothing to accept
 */
static struct descriptor_data *
new_connection_v6(in_port_t port, int sock_, int is_ssl)
{
    int newsock;

    struct sockaddr_in6 addr;
    socklen_t addr_len;
    char hostname[128];

    addr_len = (socklen_t) sizeof(addr);
    newsock = accept(sock_, (struct sockaddr *) &addr, &addr_len);

    if (newsock < 0) {
        return 0;
    } else {
#ifdef F_SETFD
        fcntl(newsock, F_SETFD, FD_CLOEXEC);
#endif
        strcpyn(hostname, sizeof(hostname), addrout_v6(port, &(addr.sin6_addr), addr.sin6_port));
        log_status("ACCEPT: %s on descriptor %d", hostname, newsock);
        log_status("CONCOUNT: There are now %d open connections.", ++ndescriptors);
        return initializesock(newsock, hostname, is_ssl);
    }
}

/*
 * @TODO addrout and make_socket are almost identical to their IPv6
 *       counterparts.  Should we make just 1 addrout and 1 make_socket,
 *       and have ipv6 vs ipv4 as a boolean, to reduce duplicate code?
 *       Also new_connection / new_connection_v6
 */

/**
 * Translate IPv4 address 'a' from addr struct to text.
 *
 * This resolves the IPv4 address 'a' to either an IP string or a hostname
 * based on if we can resolve it, and how we're resolving it.
 *
 * If the resolver is spawned, we will temporarily use the IPv4 address
 * until the resolver responds.
 *
 * Uses a static buffer, so be careful with it; copy the string out if
 * needed.
 *
 * @private
 * @param lport the port this user connected to
 * @param a the address to resolve
 * @param prt boolean true if SSL, false if not
 * @return pointer to static buffer with host name or address in string form
 */
static const char *
addrout(in_port_t lport, in_addr_t a, in_port_t prt)
{
    static char buf[128];
    struct in_addr addr;

    memset(&addr, 0, sizeof(addr));
    memcpy(&addr.s_addr, &a, sizeof(struct in_addr));

    prt = ntohs(prt);

#ifndef SPAWN_HOST_RESOLVER
    if (tp_use_hostnames) {
        /*
         * One day the nameserver Qwest uses decided to start
         * doing halfminute lags, locking up the entire muck
         * that long on every connect.  This is intended to
         * prevent that, reduces average lag due to nameserver
         * to 1 sec/call, simply by not calling nameserver if
         * it's in a slow mood *grin*. If the nameserver lags
         * consistently, a hostname cache ala OJ's tinymuck2.3
         * would make more sense:
         */
        static time_t secs_lost = 0;

        if (secs_lost) {
            secs_lost--;
        } else {
            time_t gethost_start = time(NULL);

            struct hostent *he = gethostbyaddr(((char *) &addr), sizeof(addr),
                                               AF_INET);
            time_t gethost_stop = time(NULL);
            time_t lag = gethost_stop - gethost_start;

            if (lag > 10) {
                secs_lost = lag;
            }

            if (he) {
                snprintf(buf, sizeof(buf), "%s(%" PRIu16 ")", he->h_name, prt);
                return buf;
            }
        }
    }
#endif /* !SPAWN_HOST_RESOLVER */

    a = ntohl(a);

#ifdef SPAWN_HOST_RESOLVER
    snprintf(buf, sizeof(buf), "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32
             "(%" PRIu16 ")%" PRIu16 "\n", (a >> 24) & 0xff, (a >> 16) & 0xff,
             (a >> 8) & 0xff, a & 0xff, prt, lport);

    if (tp_use_hostnames) {
        write(resolver_sock[1], buf, strlen(buf));
    }
#endif

    snprintf(buf, sizeof(buf), "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32
             "(%" PRIu16 ")", (a >> 24) & 0xff, (a >> 16) & 0xff,
             (a >> 8) & 0xff, a & 0xff, prt);
    return buf;
}

/**
 * Create an IPv4 socket and bind it to the given port
 *
 * Uses the global 'bind_ipv4_address' for the address binding.
 * Does not 'listen' on the socket so that this function can be called
 * by a privileged user, then we can listen after we've dropped
 * privileges.
 *
 * @private
 * @param port the port number
 * @return the socket
 */
static int
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

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = bind_ipv4_address;
    server.sin_port = htons(port);

    if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
        perror("binding stream socket");
        close(s);
        exit(4);
    }

    make_cloexec(s);

    /*
     * We separate the binding of the socket and the listening on the socket
     * to support binding to privileged ports, then dropping privileges
     * before listening.  Listening now happens in listen_bound_sockets(),
     * called during shovechars().  This function is now called before the
     * checks for MUD_GID and MUD_ID in main().
     */
    return s;
}

/**
 * Listen to all bound sockets
 *
 * This just does the listen(...) call for all bound sockets, with a queue
 * length of 5.
 *
 * @private
 */
static void listen_bound_sockets()
{
    for (int i = 0; i < numsocks; i++) {
        listen(sock[i], 5);
    }

    for (int i = 0; i< numsocks_v6; i++) {
        listen(sock_v6[i],5);
    }

#ifdef USE_SSL
    for (int i = 0; i < ssl_numsocks; i++) {
        listen(ssl_sock[i], 5);
    }

    for (int i = 0; i < ssl_numsocks_v6; i++) {
        listen(ssl_sock_v6[i], 5);
    }
#endif
}

/**
 * Accept an IPv4 connection
 *
 * Returns either a newly allocated struct descriptor_data if we were
 * able to accept() from sock_ or NULL if there was nothing to accept.
 * (Or theoretically on error -- errno would be set).
 *
 * @private
 * @param port the port number we accepted this connection from
 * @param sock_ the socket to accept the connection from
 * @param is_ssl boolean, if true, sock_ is an SSL socket
 * @return either a descriptor_data structure or NULL if nothing to accept
 */
static struct descriptor_data *
new_connection(in_port_t port, int sock_, int is_ssl)
{
    int newsock;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char hostname[128];

    addr_len = (socklen_t) sizeof(addr);
    newsock = accept(sock_, (struct sockaddr *) &addr, &addr_len);

    if (newsock < 0) {
        return 0;
    } else {
#ifdef F_SETFD
        fcntl(newsock, F_SETFD, 1);
#endif
        strcpyn(hostname, sizeof(hostname),
                addrout(port, addr.sin_addr.s_addr, addr.sin_port));
        log_status("ACCEPT: %s on descriptor %d", hostname, newsock);
        log_status("CONCOUNT: There are now %d open connections.",
                   ++ndescriptors);
        return initializesock(newsock, hostname, is_ssl);
    }
}

/**
 * Sends keepalive in the form of a TELNET_NOP or empty string.
 *
 * Depends on process_output to set booted if connection is dead.  This
 * uses telnet control protocols if they are supported, sends an empty
 * string otherwise.
 *
 * @private
 * @param d the descriptor_data to send a keepalive to.
 */
static void 
send_keepalive(struct descriptor_data *d)
{
    unsigned char telnet_nop[] = {
        TELNET_IAC, TELNET_NOP, '\0'
    };

    /* drastic, but this may give us crash test data */
    if (!d || !d->descriptor) {
        panic("send_keepalive(): bad descriptor or connect struct !");
    }

    if (d->telnet_enabled) {
        queue_immediate_raw(d, (const char *)telnet_nop);
    } else {
        queue_immediate_raw(d, "");
    }

    process_output(d);
}

/**
 * Receive input and process it for descriptor_data d
 *
 * If this returns false, then the descriptor should be disconnected.
 * Handles all the telnet protocol stuff, and queues the commands into
 * the descriptor_data.
 *
 * @private
 * @param d the descriptor to process input for
 * @return boolean, on false you should disconnect the descriptor_data
 */
static int
process_input(struct descriptor_data *d)
{
    char buf[MAX_COMMAND_LEN * 2];
    size_t maxget = sizeof(buf);
    int got;
    char *p, *pend, *q, *qend;

    /* Short reads means fetch one byte at a time */
    if (d->short_reads) {
        maxget = 1;
    }

    /*
     * Fetch from the socket; this will return number of bytes read or
     * -1 on an error.
     */
    got = socket_read(d, buf, maxget);

    /*
     * The way the error works varies based on Windows or not.  Basically,
     * if the error is EWOULDBLOCK (or the windows equivalent) then that
     * isn't really an error -- just means no data.  Otherwise, it is
     * a socket error and we must return false.
     *
     * Or better yet, got > 0 and we have bytes to process.
     */
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
#else /* Not Windows */

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

    /*
     * If there's no place to put raw input, then allocate a buffer
     * MAX_COMMAND_LEN in size.
     */
    if (!d->raw_input) {
        if (!(d->raw_input = malloc(MAX_COMMAND_LEN * sizeof(char))))
            panic("process_input: Out of memory");

        d->raw_input_at = d->raw_input;
    }

    p = d->raw_input_at;
    pend = d->raw_input + MAX_COMMAND_LEN - 1;

    /*
     * Iterate over the 'buf' we just collected and perform appropriate
     * actions.
     *
     * If we find a '\n' character, then we will either queue the command
     * or nudge the d->last_time if the command is a null string and we
     * aren't accepting null commands (!tp_recognize_null_command)
     *
     * Otherwise, we will check for telnet commands, and then last but not
     * least, copy the character into raw_input.
     */
    for (q = buf, qend = buf + got; q < qend; q++) {
        if (*q == '\n') {
            if (!tp_recognize_null_command
                || !(!strncasecmp(d->raw_input, NULL_COMMAND, got-2)
                && strlen(NULL_COMMAND) == got-2)) {
                d->last_time = time(NULL);
            }

            *p = '\0';

            if (p >= d->raw_input)
                save_command(d, d->raw_input);

            p = d->raw_input;
        } else if (d->telnet_state == TELNET_STATE_IAC) {
            /*
             * If we're in the IAC state, then we are expecting the next
             * byte to be a telnet command.
             */
            switch (*((unsigned char *) q)) {
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
                case TELNET_AYT: /* Are you there? */
                    {
                        char sendbuf[] = "[Yes]\r\n";
                        queue_immediate_raw(d, (const char *)sendbuf);
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
                case TELNET_SE: /* Send of subnegotiation parameters */
                    /*
                     * @TODO Note this is probably where the screen size
                     *       code would be put.  This, and we'd have to
                     *       do some WILL/DO's to ask for it.
                     *
                     *       See: https://tools.ietf.org/html/rfc1073
                     */
#ifdef USE_SSL
                    /*
                     * If we're asking for TLS, let's hand it off to SSL
                     * library to negotiate it.
                     */
                    if (d->telnet_sb_opt == TELOPT_STARTTLS && !d->ssl_session) {
                        d->block_writes = 0;
                        d->short_reads = 0;

                        if (tp_starttls_allow) {
                            d->is_starttls = 1;
                            d->ssl_session = SSL_new(ssl_ctx);
                            SSL_set_fd(d->ssl_session, d->descriptor);

                            int ssl_ret_value = SSL_accept(d->ssl_session);
                            ssl_check_error(d, ssl_ret_value,
                                            ssl_logging_connect);

                            /*
                             * Eventually it might be nice to use the return
                             * value to close the connection if SSL fails.
                             * Unfortunately, OpenSSL makes this a proper
                             * hassle, requiring inspecting a changing list of
                             * possible SSL_ERROR defines.
                             *
                             * See:
                             * https://www.openssl.org/docs/man1.1.0/ssl/SSL_accept.html#RETURN-VALUES
                             */
                            log_status("STARTTLS: %i", d->descriptor);
                        }
                    }
#endif

#ifdef IP_FORWARDING
                    /*
                     * Implementation of telnet forwarding
                     *
                     * Copy over the hostname.
                     */
                    if (d->telnet_sb_opt == TELOPT_FORWARDED) {
                        if (d->forwarded_size && d->forwarding_enabled) {
                            d->forwarded_buffer[d->forwarded_size++] = '\0';
                            free((void *) d->hostname);
                            d->hostname = alloc_string(d->forwarded_buffer);
                        }

                        d->forwarded_size = 0;
                    }

                    d->telnet_sb_opt = 0;
#endif

                    /* Reset the telnet state to normal after processing */
                    d->telnet_state = TELNET_STATE_NORMAL;
                    break;
                case TELNET_IAC: /* IAC a second time */
#if 0
                    /*
                     * Editor's note: Normally I clean out sections like this
                     * that are commented out, but this seems relevant for
                     * if we ever wanted to implement UTF8, so I am keeping
                     * it as a note. (tanabi)
                     */

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
            /* Client is suggesting supporting one thing or
             * another.
             *
             * By default, we will respond with a DON'T.  But some
             * stuff, like TLS and (eventually) NAWS -- the screen size
             * stuff -- we'll want to support.
             */
            unsigned char sendbuf[8];

#ifdef USE_SSL
            /*
             * @TODO The dangling 'else' on this ifdef block is
             *       kind of gross IMO.  I think I'd rather do
             *       something like this:
             *
             * #ifdef IP_FORWARDING
             *        if (*q ....
             * #else
             *        if (0)
             * #endif
             *
             * just to make it consistent and then you always have
             * the else.  There may be a better way to do this...
             * (tanabi)
             *
             * Also another problem with the dangling 'else' is it
             * contains a huge block of stuff, so it gets really
             * confusing.  Note the second block has the same problem.
             */

            /* If we get a STARTTLS reply, negotiate SSL startup */
            if (*q == TELOPT_STARTTLS && !d->ssl_session && tp_starttls_allow) {
                sendbuf[0] = TELNET_IAC;
                sendbuf[1] = TELNET_SB;
                sendbuf[2] = TELOPT_STARTTLS;
                sendbuf[3] = 1; /* TLS FOLLOWS */
                sendbuf[4] = TELNET_IAC;
                sendbuf[5] = TELNET_SE;
                sendbuf[6] = '\0';

                queue_immediate_raw(d, (const char *)sendbuf);

                d->block_writes = 1;
                d->short_reads = 1;
                process_output(d);
            } else
#endif
#ifdef IP_FORWARDING
                /*
                 * @TODO The dangling 'else' on this ifdef block is
                 *       kind of gross IMO.  I think I'd rather do
                 *       something like this:
                 *
                 * #ifdef IP_FORWARDING
                 *        if (*q ....
                 * #else
                 *        if (0)
                 * #endif
                 *
                 * just to make it consistent and then you always have
                 * the else.  There may be a better way to do this...
                 * (tanabi)
                 */

                /*
                 * If we get a request to do IP forwarding agree to it, 
                 * but only for localhost.
                 */
                if (*q == TELOPT_FORWARDED) {
                    /* We support TELOPT_FORWARDED */
                    sendbuf[0] = TELNET_IAC;
                    sendbuf[1] = TELNET_DO;
                    sendbuf[2] = *q;
                    sendbuf[3] = '\0';
                    socket_write(d, sendbuf, 3);

                    /* Only support forwarding from the localhost */
                    if (is_local_connection(d))
                        d->forwarding_enabled = 1;
                } else
#endif
                    /* Otherwise, we don't negotiate: send back DONT
                     * option
                     */
                {
                    sendbuf[0] = TELNET_IAC;
                    sendbuf[1] = TELNET_DONT;
                    sendbuf[2] = (unsigned char)*q;
                    sendbuf[3] = '\0';
                    queue_immediate_raw(d, (const char *)sendbuf);
                }

            d->telnet_state = TELNET_STATE_NORMAL;
            d->telnet_enabled = 1;
        } else if (d->telnet_state == TELNET_STATE_DO) {
            /* We don't negotiate: send back WONT option */
            
            /*
             * @TODO This little block of code to send a single telnet
             *       command is repeated all over the place, maybe
             *       it should be a function.
             */
            unsigned char sendbuf[4];
            sendbuf[0] = TELNET_IAC;
            sendbuf[1] = TELNET_WONT;
            sendbuf[2] = (unsigned char)*q;
            sendbuf[3] = '\0';

            queue_immediate_raw(d, (const char *)sendbuf);

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
            sendbuf[2] = (unsigned char)*q;
            sendbuf[3] = '\0';

            queue_immediate_raw(d, (const char *)sendbuf);

            d->telnet_state = TELNET_STATE_NORMAL;
            d->telnet_enabled = 1;
        } else if (d->telnet_state == TELNET_STATE_SB) {
            d->telnet_sb_opt = *((unsigned char *) q);

#ifdef IP_FORWARDING
            /* Check if this is a forwarded hostname from a gateway */
            if (d->telnet_sb_opt == TELOPT_FORWARDED && d->forwarding_enabled) {
                d->telnet_state = TELNET_STATE_FORWARDING;
            } else {
#endif
            /* TODO: Start remembering other subnegotiation data. */
            d->telnet_state = TELNET_STATE_NORMAL;

#ifdef IP_FORWARDING
            }

        } else if (d->telnet_state == TELNET_STATE_FORWARDING) {
            if (*((unsigned char *)q) == TELNET_IAC) {
                d->telnet_state = TELNET_STATE_IAC;
            } else {
                if (d->forwarded_size < 127)
                    d->forwarded_buffer[d->forwarded_size++] = *q;
            }
#endif
        } else if (*((unsigned char *) q) == TELNET_IAC) {
            /* Got TELNET IAC, store for next byte */
            d->telnet_state = TELNET_STATE_IAC;
        } else if (p < pend) {
            /* In this case, we push the byte into our raw_input buffer.
             *
             * NOTE: This will need rethinking for unicode
             */
            if (isinput(*q)) {
                *p++ = *q;
            } else if (*q == '\t') {
                *p++ = tp_tab_input_replaced_with_space ? ' ' : *q;
            } else if (*q == 8 || *q == 127) {
                /* if BS or DEL, delete last character */
                if (p > d->raw_input)
                    p--;
            }

            d->telnet_state = TELNET_STATE_NORMAL;
        }
    }

    if (p > d->raw_input) {
        /* Move raw_input_at so we know where to pull in the next bytes */
        d->raw_input_at = p;
    } else {
        /* Clear the buffer if it is fully handled */
        free(d->raw_input);
        d->raw_input = 0;
        d->raw_input_at = 0;
    }

    return 1;
}

#ifdef USE_SSL

/**
 * Passwod callback for SSL PEM.
 *
 * Just uses 'userdata' as the password.  Userdata should be a null terminated
 * string.
 *
 * @private
 * @param buf the buffer to copy the password into
 * @param size the size of the buffer
 * @param rwflag not used by callback
 * @param userdata the password should be a null terminated string here
 * @return length of password (or 'size' if password is longer than 'size')
 */
static int
pem_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
    const char *pw = (const char *) userdata;
    int pwlen = strlen(pw);

    strncpy(buf, pw, size);
    return ((pwlen > size) ? size : pwlen);
}

/**
 * Create a new SSl_CTX object.
 *
 * We do this rather than reconfiguring an existing one to allow us to
 * recover gracefully if we can't reload a new certificate file, etc.
 *
 * This returns NULL on failure.  Uses tune parameters for certificate
 * files and other SSL based config options.
 *
 * Relevant tune parameters: tp_ssl_cipher_preference_list
 *                      and: tp_ssl_cert_file
 *                      and: tp_ssl_keyfile_passwd
 *                      and: tp_ssl_key_file
 *                      and: tp_ssl_min_protocol_version
 *
 * Errors will be logged via log_status and stderr.  @see log_status
 *
 * @private
 * @return the new SSL_CTX structure or NULL on failure.
 */
static SSL_CTX *
configure_new_ssl_ctx(void)
{
    EC_KEY *eckey;
    int ssl_status_ok = 1;

    SSL_CTX *new_ssl_ctx = SSL_CTX_new(SSLv23_server_method());

#ifdef SSL_OP_SINGLE_ECDH_USE
    /*
     * As a default "optimization", OpenSSL shares ephemeral keys between
     * sessions. Disable this to improve forward secrecy.
     */
    SSL_CTX_set_options(new_ssl_ctx, SSL_OP_SINGLE_ECDH_USE);
#endif

#ifdef SSL_OP_NO_TICKET
    /*
     * OpenSSL supports session tickets by default but never rotates the
     * keys by default.  Since session resumption isn't important for MUCK
     * performance and since this breaks forward secrecy, just disable
     * tickets rather than trying to implement key rotation.
     */
    SSL_CTX_set_options(new_ssl_ctx, SSL_OP_NO_TICKET);
#endif

    /*
     * Disable the session cache on the assumption that session resumption
     * is not worthwhile given our long-lived connections. This also avoids
     * any concern about session secret keys in memory for a long time.
     * (By default, OpenSSL only clears timed out sessions every 256
     * connections.)
     */
    SSL_CTX_set_session_cache_mode(new_ssl_ctx, SSL_SESS_CACHE_OFF);

#if defined(SSL_CTX_set_dh_auto)
    /*
     * This is to support old SSL libraries; either you use this call,
     * or you use set_ecdh_auto (I think).  Not really sure; there isn't
     * a lot of remaining documentation for this call.
     *
     * @TODO Do we want to stop supporting old-ass SSL?  That could break
     *       builds for some users, but probably not anyone using FB7.
     */
    SSL_CTX_set_dh_auto(new_ssl_ctx, 1);
#endif

#if defined(SSL_CTX_set_ecdh_auto)
    /*
     * In OpenSSL >= 1.0.2, this exists; otherwise, fallback to the older
     * API where we have to name a curve.
     */
    eckey = NULL;
    SSL_CTX_set_ecdh_auto(new_ssl_ctx, 1);
#else
    eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    SSL_CTX_set_tmp_ecdh(new_ssl_ctx, eckey);
#endif

    SSL_CTX_set_cipher_list(new_ssl_ctx, tp_ssl_cipher_preference_list);

    if (tp_server_cipher_preference) {
        SSL_CTX_set_options(new_ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
    }

    if (!SSL_CTX_use_certificate_chain_file(new_ssl_ctx,
                                            (void *) tp_ssl_cert_file)) {
        log_status("Could not load certificate file %s",
                   (void *) tp_ssl_cert_file);

        /* Use (char*) to avoid a -Wformat= warning */
        fprintf(stderr, "Could not load certificate file %s\n",
                (char *) tp_ssl_cert_file);
        ssl_status_ok = 0;
    }

    if (ssl_status_ok) {
        SSL_CTX_set_default_passwd_cb(new_ssl_ctx, pem_passwd_cb);
        SSL_CTX_set_default_passwd_cb_userdata(new_ssl_ctx,
                                               (void *) tp_ssl_keyfile_passwd);

        if (!SSL_CTX_use_PrivateKey_file
            (new_ssl_ctx, (void *) tp_ssl_key_file, SSL_FILETYPE_PEM)) {
            log_status("Could not load private key file %s", (void *) tp_ssl_key_file);

            /* Use (char*) to avoid a -Wformat= warning */
            fprintf(stderr, "Could not load private key file %s\n",
                    (char *) tp_ssl_key_file);
            ssl_status_ok = 0;
        }
    }

    /*
     * @TODO These two if's (and possibly others) can be reduced to a
     *       single if that is if(ssl_status_ok && ...) rather than
     *       doing this funny nesting.
     */

    if (ssl_status_ok) {
        if (!SSL_CTX_check_private_key(new_ssl_ctx)) {
            log_status(
                "Private key does not check out and appears to be invalid."
            );
            fprintf(stderr,
                    "Private key does not check out and appears to be "
                    "invalid.\n");
            ssl_status_ok = 0;
        }
    }

    if (ssl_status_ok) {
        if (!set_ssl_ctx_versions(new_ssl_ctx, tp_ssl_min_protocol_version)) {
            log_status("Could not set minimum SSL protocol version.");
            fprintf(stderr, "Could not set minimum SSL protocol version.\n");
            ssl_status_ok = 0;
        }
    }

    /*
     * Set ssl_ctx to automatically retry conditions that would
     * otherwise return SSL_ERROR_WANT_(READ|WRITE)
     */
    if (ssl_status_ok) {
        SSL_CTX_set_mode(new_ssl_ctx, SSL_MODE_AUTO_RETRY);
    } else {
        SSL_CTX_free(new_ssl_ctx);
        new_ssl_ctx = NULL;
    }

    return new_ssl_ctx;
}

/**
 * Reinitialize the SSL context, reloading certificate fils.
 *
 * Reinitialize or configure the SSL_CTX, allowing new connections to get any
 * changed settings, most notably new certificate files. Then, if we haven't
 * already, create the listening sockets for the SSL ports.
 *
 * Does not do one-time library OpenSSL library initailization.  Errors are
 * logged via log_status and to stderr.  @see log_status
 *
 * @return boolean true on successful SSL load, false on failure.
 */
int
reconfigure_ssl(void)
{
    SSL_CTX *new_ssl_ctx;

    new_ssl_ctx = configure_new_ssl_ctx();

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

#ifdef SPAWN_HOST_RESOLVER

/**
 * Shutdown the host resolver
 *
 * Note that the way this works is very trusting.  If the resolver
 * doesn't quit when receiving the QUIT command, then this will hang
 * forever because it is waiting to reap a child.
 *
 * Also, this doesn't wait for the resolver's pid.... it waits for any
 * process to exit.  Theoretically a forked dump could make this return.
 */
void
static kill_resolver(void)
{
    int i;

    write(resolver_sock[1], "QUIT\n", 5);
    shutdown(resolver_sock[1], 2);

    /*
     * @TODO Use waitpid to wait for the resolver pid.  waitpid can
     *       use WNOHANG and return immediately; that would allow us
     *       to loop on the wait, and if it takes too long, kill the
     *       resolver with 'kill' instead.  Would be a lot more robust,
     *       though honestly, this hasn't ever been a problem as far
     *       as I know so pretty low priority (tanabi)
     */
    wait(&i);
}


/**
 * @var the timestamp of when the resolver was spawned.
 */
static time_t resolver_spawn_time = 0;

/**
 * Spawn the host resolver.
 *
 * This won't spawn the resolver if the last spawn time is under 5
 * seconds ago.
 *
 * Resolver PID is stored in global 'global_resolver_pid'.
 */
void
spawn_resolver(void)
{
    if (time(NULL) - resolver_spawn_time < 5) {
        return;
    }

    resolver_spawn_time = time(NULL);

    socketpair(AF_UNIX, SOCK_STREAM, 0, resolver_sock);
    make_nonblocking(resolver_sock[1]);
    make_cloexec(resolver_sock[1]);

    if ((global_resolver_pid = fork()) == 0) {
        close(0);
        close(1);
        dup(resolver_sock[0]);
        dup(resolver_sock[0]);

        /*
         * The way this works -- the first execl that actually loads
         * the resolver will "win" and not call the rest.  This is why
         * we call a bunch of them and don't check errors.
         *
         * If none of the execl's work out, we'll exit with code 1.
         */

        if (resolver_program[0])
            execl(resolver_program, "fb-resolver", NULL);

#ifdef BINDIR
        {
            char resolverpath[BUFFER_LEN];
            snprintf(resolverpath, sizeof(resolverpath), "%s/fb-resolver",
                     BINDIR);
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
        log_status("%s", "Unable to spawn host resolver!");
        _exit(1);
    }
}

/**
 * Collect hostnames from the host resolver socket
 *
 * The resolver returns host information one per line.  The host information
 * looks like:
 *
 * hostip(port)|hostname(user)\n
 *
 * Once we get a line from the resolver, we iterate over the descriptor
 * list and use hostip + port to set the hostname + user on the descriptor
 * when we find it.
 *
 * @private
 */
static void
resolve_hostnames()
{
    char buf[BUFFER_LEN];
    char *ptr, *ptr2, *ptr3, *hostip, *port, *hostname, *username, *tempptr;
    int got, dc;

    /*
     * @TODO The way this is written ... if I'm reading it right, can have
     *       a number of theoretical problems.  For instance:
     *
     *       - If there is more than BUFFER_LEN bytes coming back from the
     *         resolver (unlikely), we lose anything after the last \n
     *         found in the buffer.
     *
     *       - It relies on complete lines being in the socket.  This is
     *         likely going to be the case, but theoretically it doesn't
     *         have to be so.
     *
     *       Probably the easiest way to handle this would be to use
     *       fdopen() to turn resolver_sock[1] into a FILE* instead and
     *       use that to read/write.  It may not be worth the effort as
     *       this "seems to work".  Could also read this byte at a time
     *       to fetch up to the \n, though you'd have to make provision
     *       for if the buffer doesn't have a \n in it yet.
     */
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
            ptr3 = strchr(ptr2, '|');

            if (!ptr3)
                return;

            hostip = ptr2;
            port = strchr(ptr2, '(');

            if (!port)
                return;

            tempptr = strchr(port, ')');

            if (!tempptr)
                return;

            *tempptr = '\0';
            hostname = ptr3;
            username = strchr(ptr3, '(');

            if (!username)
                return;

            tempptr = strchr(username, ')');

            if (!tempptr)
                return;

            *tempptr = '\0';

            /* If we got all the components, iterate over the descriptors */
            if (*port && *hostname && *username) {
                *port++ = '\0';
                *hostname++ = '\0';
                *username++ = '\0';

                for (struct descriptor_data *d = descriptor_list; d;
                     d = d->next) {
                    if (!strcmp(d->hostname, hostip)
                        && !strcmp(d->username, port)) {
                        free((void *)d->hostname);
                        free((void *)d->username);
                        d->hostname = strdup(hostname);
                        d->username = strdup(username);
                    }
                }
            }
        }
    } while (dc < got && *ptr);
}

#endif

/**
 * This is the game's main loop with a very weird name.
 *
 * The loop continues until shutdown_flag is set boolean true.
 * The overall process is roughly:
 *
 * - listen to bound sockets (@see listen_bound_sockets)
 * - initialize timeslice
 * - enter loop
 *   - update timeslice
 *   - process time-based MUCK events (@see next_muckevent)
 *   - process commands (@see process_commands)
 *   - MUF events (@see muf_event_process)
 *   - process output to descriptors (@see process_output) and shutdown
 *     descriptors marked asa3 ->booted == 2.
 *   - Do DB dump warning and processing if applicable. @see wall_and_flush
 *   - set up for pselect/select then run pselect/select.
 *   - process input -- this is a **lot** of code.
 * - set shutdown properties on #0 and return.
 */
static void
shovechars()
{
    fd_set input_set, output_set;
    time_t now;
    long tmptq;
    struct timeval last_slice, current_time;
    struct timeval next_slice;
    struct timeval timeout, slice_timeout;
#ifdef HAVE_PSELECT
    struct timespec timeout_for_pselect;
#endif
    int cnt;
    struct descriptor_data *dnext;
    struct descriptor_data *newd;
    struct timeval sel_in, sel_out;
    int avail_descriptors;

    listen_bound_sockets();

    gettimeofday(&last_slice, (struct timezone *) 0);

    avail_descriptors = max_open_files() - 5;

    (void) time(&now);

    /* And here, we do the actual player-interaction loop */
    while (shutdown_flag == 0) {
        gettimeofday(&current_time, (struct timezone *) 0);
        last_slice = update_quotas(last_slice, current_time);

        /* Process timed events, commands, and MUF stuff. */
        next_muckevent();
        process_commands();
        muf_event_process();

        /* Send output, and be-well any users that need to get canned. */
        for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
            dnext = d->next;

            if (d->booted) {
                process_output(d);

                if (d->booted == 2) {
                    goodbye_user(d);
                }

                process_output(d);
                shutdownsock(d);
            }
        }

        /* Process dump stuff */
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

        /* Set up FDSET's for select/pselect */
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        next_slice = msec_add(last_slice, tp_command_time_msec);
        slice_timeout = timeval_sub(next_slice, current_time);

        FD_ZERO(&input_set);
        FD_ZERO(&output_set);

        /* Add all the descriptors to the FDSETs */
        if (ndescriptors < avail_descriptors) {
            for (int i = 0; i < numsocks; i++) {
                FD_SET(sock[i], &input_set);
            }

            for (int i = 0; i < numsocks_v6; i++) {
                FD_SET(sock_v6[i], &input_set);
            }

#ifdef USE_SSL
            for (int i = 0; i < ssl_numsocks; i++) {
                FD_SET(ssl_sock[i], &input_set);
            }

            for (int i = 0; i < ssl_numsocks_v6; i++) {
                FD_SET(ssl_sock_v6[i], &input_set);
            }
#endif
        }

        /* Iterate over the descriptors and add to input_set */
        for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
            if (d->input.lines > 0)
                timeout = slice_timeout;

            if (d->input.lines < 100)
                FD_SET(d->descriptor, &input_set);

#ifdef USE_SSL
            if (has_output(d)) {
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
                        timeout.tv_sec = (long)(welcome_pause - timeon);
                        timeout.tv_usec = 10; /* 10 msecs min.  Arbitrary. */
                    }
                }
            }

            if (d->ssl_session) {
                /* SSL may want to write even if the output queue is empty */
                if (!SSL_is_init_finished(d->ssl_session)) {
                    /* log_status("SSL : Init not finished.\n", "version"); */
                    FD_CLR(d->descriptor, &output_set);
                    FD_SET(d->descriptor, &input_set);
                }

                if (SSL_want_write(d->ssl_session)) {
                    /* log_status("SSL : Need write.\n", "version"); */
                    FD_SET(d->descriptor, &output_set);
                }
            }
#else
            if (has_output(d)) {
                FD_SET(d->descriptor, &output_set);
            }
#endif

        }

#ifdef SPAWN_HOST_RESOLVER
        /* Add the host resolver socket if we're using spawned resolver. */
        FD_SET(resolver_sock[1], &input_set);
#endif

        /* Set up timer for select */
        tmptq = (long)next_muckevent_time();

        if ((tmptq >= 0L) && (timeout.tv_sec > tmptq)) {
            timeout.tv_sec = tmptq + (tp_pause_min / 1000);
            timeout.tv_usec = (tp_pause_min % 1000) * 1000L;
        }

        gettimeofday(&sel_in, NULL);

        /* Use the right select call for our system */
#ifdef HAVE_PSELECT
        timeout_for_pselect.tv_sec = timeout.tv_sec;
        timeout_for_pselect.tv_nsec = timeout.tv_usec * 1000L;

        if (pselect(max_descriptor, &input_set, &output_set, (fd_set *) 0,
                    &timeout_for_pselect, &pselect_signal_mask) < 0) {
            if (errno != EINTR) {
                perror("select");
                return;
            }

#elif !defined(WIN32)
        if (select(max_descriptor, &input_set, &output_set, (fd_set *) 0, &timeout) < 0) {
            if (errno != EINTR) {
                perror("select");
                return;
            }
#else
        if (select(max_descriptor, &input_set, &output_set, (fd_set *) 0, &timeout) ==
            SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEINTR) {
                perror("select");
                return;
            }
#endif
        } else {
            /* Do some time based book-keeping */
            gettimeofday(&sel_out, NULL);

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

            /* Iterate over sockets and handle new connections */
            for (int i = 0; i < numsocks; i++) {
                if (FD_ISSET(sock[i], &input_set)) {
                    if (!(newd = new_connection(listener_port[i], sock[i], 0))) {
#ifndef WIN32
                        if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
                            perror("new_connection");
                            /* return; */
                        }
#else /* WIN32 */
                        if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
                            perror("new_connection");
                            /* return; */
                        }
#endif /* WIN32 */
                    } else {
                        update_max_descriptor(newd->descriptor);
                    }
                }
            }

            /* Iterate over sockets and handle new connections */
            for (int i = 0; i < numsocks_v6; i++) {
                if (FD_ISSET(sock_v6[i], &input_set)) {
                    if (!(newd = new_connection_v6(listener_port[i], sock_v6[i], 0))) {
#ifndef WIN32
                        if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
                            perror("new_connection");
                            /* return; */
                        }
#else
                        if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
                            perror("new_connection");
                            /* return; */
                        }
#endif
                    } else {
                        update_max_descriptor(newd->descriptor);
                    }
                }
            }

#ifdef USE_SSL
            /* Iterate over sockets and handle new connections */
            for (int i = 0; i < ssl_numsocks; i++) {
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
                        ssl_check_error(newd, cnt, ssl_logging_connect);
                        /*
                         * Eventually it might be nice to use the return
                         * value to close the connection if SSL fails.
                         * Unfortunately, OpenSSL makes this a proper
                         * hassle, requiring inspecting a changing list of
                         * possible SSL_ERROR defines.
                         *
                         * See:
                         * https://www.openssl.org/docs/man1.1.0/ssl/SSL_accept.html#RETURN-VALUES
                         */
                    }
                }
            }

            /* Iterate over sockets and handle new connections */
            for (int i = 0; i < ssl_numsocks_v6; i++) {
                if (FD_ISSET(ssl_sock_v6[i], &input_set)) {
                    if (!(newd = new_connection_v6(ssl_listener_port[i], ssl_sock_v6[i], 1))) {
# ifndef WIN32
                        if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
                            perror("new_connection");
                        }
# else
                        if (WSAGetLastError() != WSAEINTR && WSAGetLastError() != EMFILE) {
                            perror("new_connection");
                        }
# endif
                    } else {
                        update_max_descriptor(newd->descriptor);
                        newd->ssl_session = SSL_new(ssl_ctx);
                        SSL_set_fd(newd->ssl_session, newd->descriptor);
                        cnt = SSL_accept(newd->ssl_session);
                        ssl_check_error(newd, cnt, ssl_logging_connect);
                        /*
                         * Eventually it might be nice to use the return value
                         * to close the connection if SSL fails.
                         * Unfortunately, OpenSSL makes this a proper hassle,
                         * requiring inspecting a changing list of possible
                         * SSL_ERROR defines.
                         *
                         * See:
                         * https://www.openssl.org/docs/man1.1.0/ssl/SSL_accept.html#RETURN-VALUES
                         */
                    }
                }
            }
#endif
#ifdef SPAWN_HOST_RESOLVER
            if (FD_ISSET(resolver_sock[1], &input_set)) {
                resolve_hostnames();
            }
#endif
            cnt = 0;

            /* Iterate over descriptors and handle I/O */
            for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
                dnext = d->next;

#ifdef USE_SSL
                if (FD_ISSET(d->descriptor, &input_set)
                    || (d->ssl_session && SSL_pending(d->ssl_session))) {
#else
                if (FD_ISSET(d->descriptor, &input_set)) {
#endif
                    if (!process_input(d)) {
                        d->booted = 1;
                    }
                }

                if (FD_ISSET(d->descriptor, &output_set)) {
                    process_output(d);
                }

                if (d->connected) {
                    cnt++;

                    /* Do idle boots if configured */
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

                if (d->connected && tp_idle_ping_enable && (tp_idle_ping_time > 0)
                    && ((now - d->last_pinged_at) > tp_idle_ping_time)) {
                    const char *tmpptr = get_property_class(d->player, NO_IDLE_PING_PROP);
                    if (!tmpptr) {
                        send_keepalive(d);
                    }
                }
            }

            if (cnt > con_players_max) {
                add_property((dbref) 0, SYS_MAX_CONNECTS_PROP, NULL, cnt);
                con_players_max = cnt;
            }

            con_players_curr = cnt;
        }
    }

    /*
     * End of the player processing loop.
     *
     * Set shutdown props on #0.
     */
    (void) time(&now);
    add_property((dbref) 0, SYS_LASTDUMPTIME_PROP, NULL, (int) now);
    add_property((dbref) 0, SYS_SHUTDOWNTIME_PROP, NULL, (int) now);
}

/**
 * @private
 * @var lock to avoid recursive listening due to @pecho's
 */
static int notify_nolisten_level = 0;

/**
 * Notify the given player (or zombie) without triggering listen propqueue.
 *
 * This can trigger MPI via pecho if the 'player' is a zombie and zombies
 * are supported.  The MPI will not be able to run recursively; a recursive
 * run will ignore the pecho setting.
 *
 * Multi-line content (separated by \r's) are queued up line at a time in
 * a loop.
 *
 * Returns true if the message went somewhere, false otherwise.
 *
 * "isprivate" is something of an oddity.  It only applies to zombies,
 * and when true, the message will go to the zombie regardless of if their
 * owner is in the same room.  Otherwise, if it is false, the message will
 * be ignored if the zombie's owner is in the same room.
 *
 * @param player the player (or zombie) to deliver the message to
 * @param msg the message to send as a character string
 * @param isprivate
 * @return boolean true if message received, false if nothing was queued.
 */
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
    int *darr;
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

        /*
         * @TODO The way this is written is kind of a mess; it was probably
         *       written for the single line case and then retrofitted for
         *       the multiline case.
         *
         *       If I'm not mistaken, we can do get_player_descrs once
         *       outside the loop because I do not think the player
         *       descriptor list can change during the run of this call.
         *
         *       Secondly, if the player is a zombie and tp_allow_zombies is
         *       false, then nothing will happen -- and we just looped
         *       over all this stuff for fun.
         *
         *       Recommendation: move get_player_descrs out of the loop.
         *       Do a single check to see if the player is a zombie:
         *
         *       isZombie = Typeof(player) == TYPE_THING, FLAGS & ZOMBIE,
         *       and all that other good stuff that's in the second if
         *       statement after if(tp_allow_zombies)
         *
         *       Then, if darr == 0 && !isZombie || isZombie && !tp_allow_zombies,
         *       then return -- these cases we will do nothing.
         *
         *       Otherwise, loop.  if isZombie, run the stuff within the
         *       if statement after if (tp_allow_zombies).  Else, loop over the
         *       descriptors and queue it up as-written.
         *
         *       That cleans it up and makes this way more efficient.
         */
        darr = get_player_descrs(player, &dcount);

        for (int di = 0; di < dcount; di++) {
            queue_ansi(descrdata_by_descr(darr[di]), buf);

            if (firstpass)
                retval++;
        }

        if (tp_allow_zombies) {
            if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
                !(FLAGS(OWNER(player)) & ZOMBIE) &&
                (!(FLAGS(player) & DARK) || Wizard(OWNER(player)))) {
                ref = LOCATION(player);

                /* Make sure the location is allowed */
                if (Wizard(OWNER(player)) || ref == NOTHING ||
                    Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {

                    /*
                     * Make sure the message is either private or the
                     * owner isn't here.
                     */
                    if (isprivate
                        || LOCATION(player) != LOCATION(OWNER(player))) {
                        char pbuf[BUFFER_LEN];
                        const char *prefix;
                        char ch = *match_args;

                        *match_args = '\0';

                        if (notify_nolisten_level <= 0) {
                            notify_nolisten_level++;

                            prefix = do_parse_prop(-1, player, player, MESGPROP_PECHO,
                                                   "(@Pecho)", pbuf, sizeof(pbuf),
                                                   MPI_ISPRIVATE);

                            notify_nolisten_level--;
                        } else
                            prefix = 0;

                        *match_args = ch;

                        if (!prefix || !*prefix) {
                            prefix = NAME(player);
                            snprintf(buf2, sizeof(buf2), "%s> %.*s", prefix,
                                     (int) (BUFFER_LEN - (strlen(prefix) + 3)),
                                     buf);
                        } else {
                            snprintf(buf2, sizeof(buf2), "%s %.*s", prefix,
                                     (int) (BUFFER_LEN - (strlen(prefix) + 2)),
                                     buf);
                        }

                        darr = get_player_descrs(OWNER(player), &dcount);

                        for (int di = 0; di < dcount; di++) {
                            queue_ansi(descrdata_by_descr(darr[di]), buf2);

                            if (firstpass)
                                retval++;
                        }
                    }
                }
            }
        }

        firstpass = 0;
    }

    return retval;
}

/**
 * Filter the message through player's ignore list, and filter NULL messages.
 *
 * If the 'from' player is being ignored by 'player' or the 'msg' is NULL,
 * then this returns false and nothing happens.
 *
 * Otherwise, hands off to notify_nolisten and returns the return value of
 * that call.  @see notify_nolisten
 *
 * @param from the player sending the message
 * @param player the player receiving the message
 * @param msg the message to send, which may be \r delimited for multi-line.
 * @param isprivate boolean see description under notify_nolisten
 * @return as described above, or return value of notify_nolisten
 */
int
notify_filtered(dbref from, dbref player, const char *msg, int isprivate)
{
    if ((msg == 0) || ignore_is_ignoring(player, from))
        return 0;

    return notify_nolisten(player, msg, isprivate);
}

/**
 * This is used by basically all the different notification routines.
 *
 * Sends a message to a player or thing, handing vehicle oecho, listen
 * propqueues, ignore filters, and zombie stuff.  This hands the message
 * off to notify_filtered after dealing with queues and oecho.
 *
 * oecho is a prefix for vehicle messages.
 *
 * @see notify_filtered
 *
 * isprivate is an oddity; if true, zombies will receive the message
 * regardless of where their owner is.  If false, the message will not
 * go to the zombie if the owner is in the same room as me.
 *
 * @param from the player that initiated this message
 * @param player the target of the message -- player or thing
 * @param msg the message to send
 * @param isprivate zombie message filter -- see the notes above.
 */
int
notify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
    const char *ptr = msg;

    if (tp_allow_listeners) {
        if (tp_allow_listeners_obj || Typeof(player) == TYPE_ROOM) {
            listenqueue(-1, from, LOCATION(from), player, player, NOTHING,
                        LISTEN_PROPQUEUE, ptr, tp_listen_mlev, 1, 0);
            listenqueue(-1, from, LOCATION(from), player, player, NOTHING,
                        WLISTEN_PROPQUEUE, ptr, tp_listen_mlev, 1, 1);
            listenqueue(-1, from, LOCATION(from), player, player, NOTHING,
                        WOLISTEN_PROPQUEUE, ptr, tp_listen_mlev, 0, 1);
        }
    }

    /* Handle vehicle prefixing */
    if (Typeof(player) == TYPE_THING && (FLAGS(player) & VEHICLE) &&
        (!(FLAGS(player) & DARK) || Wizard(OWNER(player)))) {
        dbref ref;

        ref = LOCATION(player);

        if (Wizard(OWNER(player)) || ref == NOTHING ||
            Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & VEHICLE)) {
            if (!isprivate && LOCATION(from) == LOCATION(player)) {
                char buf[BUFFER_LEN];
                char pbuf[BUFFER_LEN];
                const char *prefix;
                char ch = *match_args;

                *match_args = '\0';
                prefix = do_parse_prop(-1, from, player, MESGPROP_OECHO,
                                       "(@Oecho)", pbuf, sizeof(pbuf),
                                       MPI_ISPRIVATE);
                *match_args = ch;

                if (!prefix || !*prefix)
                    prefix = "Outside>";

                snprintf(buf, sizeof(buf), "%s %.*s", prefix,
                         (int) (BUFFER_LEN - (strlen(prefix) + 2)), msg);
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

/**
 * Send a notification to a player, from the player.
 *
 * This is for system messages in response to a player's actions for the
 * most part.  It wraps notify_from_echo, with the message being both
 * to and from the same player.
 *
 * @see notify_from_echo
 *
 * It is used all over the place so it isn't practical to replace this,
 * despite it being a silly one line function.
 *
 * @param player the player to send the message to
 * @param msg the message to send to the player
 * @return the return value from notify_from_echo
 */
inline int
notify(dbref player, const char *msg)
{
    return notify_from_echo(player, player, msg, 1);
}

/**
 * Send a notification to a player, from the player, with sprintf substitutions
 *
 * This wraps notify, but it works like sprintf and understands sprintf's
 * format string formats.
 *
 * @see notify
 *
 * @param player the player to send a message to
 * @param format the format string
 * @param ... as many arguments as necessary
 */
void
notifyf(dbref player, char *format, ...)
{
    va_list args;
    char bufr[BUFFER_LEN];

    va_start(args, format);
    vsnprintf(bufr, sizeof(bufr), format, args);
    bufr[sizeof(bufr) - 1] = '\0';
    notify(player, bufr);
    va_end(args);
}

/**
 * Notify the given player (or zombie) without triggering listen propqueue.
 *
 * This version supports sprintf-style string formatting.  isprivate is
 * always true in this call so it is not exposed as an argument.  See the
 * original call for all the details, as there are many details.
 *
 * @see notify_nolisten
 *
 * @param player the player to notify
 * @param format the format string
 * @param ... as many arguments as necessary.
 */
void
notifyf_nolisten(dbref player, char *format, ...)
{
    va_list args;
    char bufr[BUFFER_LEN];

    va_start(args, format);
    vsnprintf(bufr, sizeof(bufr), format, args);
    bufr[sizeof(bufr) - 1] = '\0';
    notify_nolisten(player, bufr, 1);
    va_end(args);
}

/**
 * This is used by MUF programs to send notifications that process listeners
 *
 * Most (all?) MUF notifications use this call.  However, it is not typically
 * used for non-MUF notifications.
 *
 * isprivate only impacts zombies.  If true, then the zombie will get the
 * message regardless of the location of the zombie's owner.  If false, the
 * message will not be sent to the zombie if the owner is in the same room
 * as the zombie.
 *
 * @param who the person sending the message
 * @param xprog the program that originated the message
 * @param obj the target of the message
 * @param room the room that the message is sourced in
 * @param msg the message to send
 * @param isprivate boolean, usually false.  See explanation above
 */
void
notify_listeners(dbref who, dbref xprog, dbref obj, dbref room,
                 const char *msg, int isprivate)
{
    char buf[BUFFER_LEN];
    dbref ref;

    if (obj == NOTHING)
        return;

    if (tp_allow_listeners && (tp_allow_listeners_obj || Typeof(obj) == TYPE_ROOM)) {
        listenqueue(-1, who, room, obj, obj, xprog, LISTEN_PROPQUEUE, msg,
                    tp_listen_mlev, 1, 0);
        listenqueue(-1, who, room, obj, obj, xprog, WLISTEN_PROPQUEUE, msg,
                    tp_listen_mlev, 1, 1);
        listenqueue(-1, who, room, obj, obj, xprog, WOLISTEN_PROPQUEUE, msg,
                    tp_listen_mlev, 0, 1);
    }

    /*
     * @TODO I think this is wrong.  tp_allow_zombies shouldn't block vehicles.
     *       isprivate is probably used improperly as well here.  I'd just
     *       take this outer if off and add a TYPE_THING check to the inner
     *       if.  I think notify_filtered (or one of its children) handles
     *       tp_allow_zombies + isprivate so none of that is of concern to us here.
     *       (tanabi)
     */
    if (tp_allow_zombies && Typeof(obj) == TYPE_THING && !isprivate) {
        if (FLAGS(obj) & VEHICLE) {
            if (LOCATION(who) == LOCATION(obj)) {
                char pbuf[BUFFER_LEN];
                const char *prefix;

                memset(buf, 0, BUFFER_LEN); /* Make sure the buffer is zeroed */

                prefix = do_parse_prop(-1, who, obj, MESGPROP_OECHO,
                                       "(@Oecho)", pbuf, sizeof(pbuf),
                                       MPI_ISPRIVATE);

                if (!prefix || !*prefix)
                    prefix = "Outside>";

                snprintf(buf, sizeof(buf), "%s %.*s", prefix,
                         (int) (BUFFER_LEN - 2 - strlen(prefix)), msg);
                ref = CONTENTS(obj);

                while (ref != NOTHING) {
                    notify_filtered(who, ref, buf, isprivate);
                    ref = NEXTOBJ(ref);
                }
            }
        }
    }

    if (Typeof(obj) == TYPE_PLAYER || Typeof(obj) == TYPE_THING)
        notify_filtered(who, obj, msg, isprivate);
}

/**
 * Send notification to a DB list starting with 'first' except for 'exception'
 *
 * 'who' is the person sending the message.  This will notify listeners if
 * they are set, including environmental listeners, then iterates over the
 *
 * @param first the first ref on the DB list
 * @param exception the ref to not notify
 * @param msg the message to send
 * @param who the ref of the person sending it
 */
void
notify_except(dbref first, dbref exception, const char *msg, dbref who)
{
    dbref room, srch;

    if (first == NOTHING)
        return;

    srch = room = LOCATION(first);

    if (tp_allow_listeners) {
        notify_from_echo(who, srch, msg, 0);

        if (tp_allow_listeners_env) {
            srch = LOCATION(srch);
            while (srch != NOTHING) {
                notify_from_echo(who, srch, msg, 0);
                srch = getparent(srch);
            }
        }
    }

    DOLIST(first, first) {
        if ((Typeof(first) != TYPE_ROOM) && (first != exception)) {
            /* don't want excepted player or child rooms to hear */
            notify_from_echo(who, first, msg, 0);
        }
    }
}

/*
 * @TODO This should probably be in config.h or something, or set by
 *       autoconfig ... not just randomly injected in the middle of this
 *       file.
 */
#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
# define USE_RLIMIT
#endif

/**
 * Return the maximum number of files that you may have open.
 *
 * This returns the max number of files you may have open as a long, and if
 * it can use setrlimit() to increase it, it will do so.
 *
 * Becuse there is no way to just "know" if get/setrlimit is around, since
 * its defs are in <sys/resource.h>, you need to define USE_RLIMIT in
 * config.h to attempt it.
 *
 * Otherwise it trys to use sysconf() (POSIX.1) or getdtablesize() to get what
 * is available to you.
 *
 * @return maximum number of files that can be open
 */
long
max_open_files(void)
{
    /*
     * @TODO This if / else block is full of stuff that should probably
     *       be abstracted into config.h or something.  Instead of checking
     *       for system defines here, we should boil this down into
     *       something like #if USE_SYSCONF ... then do the RLIMIT_NOFILE
     *       vs. RLIMIT_OFILE "we be BSD" stuff config.h rather than inline
     *       here.
     */

    /* Use POSIX.1 method, sysconf() */
#if defined(_SC_OPEN_MAX) && !defined(USE_RLIMIT)
    /*
     * POSIX.1 code.
     */
    return sysconf(_SC_OPEN_MAX);
#else/* !POSIX */
# if defined(USE_RLIMIT) && (defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE))
#  ifndef RLIMIT_NOFILE
#   define RLIMIT_NOFILE RLIMIT_OFILE /* We Be BSD! */
#  endif /* !RLIMIT_NOFILE */
    /*
     * get/setrlimit() code.
     */
    struct rlimit file_limit;

    getrlimit(RLIMIT_NOFILE, &file_limit); /* Whats the limit? */

    if (file_limit.rlim_cur < file_limit.rlim_max) { /* if not at max... */
        file_limit.rlim_cur = file_limit.rlim_max; /* ...set to max. */
        setrlimit(RLIMIT_NOFILE, &file_limit);
        getrlimit(RLIMIT_NOFILE, &file_limit); /* See what we got. */
    }

    return (long) file_limit.rlim_cur;

# elif WIN32 /* !RLIMIT && WIN32 */
    return FD_SETSIZE;
# else /* !RLIMIT && !WIN32 */
    /*
     * Don't know what else to do, try getdtablesize().
     * email other bright ideas to me. :) (whitefire)
     */
    return (long) getdtablesize();
# endif /* !RLIMIT */
#endif /* !POSIX */
}

/**
 * Send a message to everyone online and flush the output immediately.
 *
 * \r\n will be added to the end of the message.
 *
 * @param msg the message to send to all the descriptors
 */ 
void
wall_and_flush(const char *msg)
{
    struct descriptor_data *dnext;
    char buf[BUFFER_LEN + 2];

    if (!msg || !*msg)
        return;

    strcpyn(buf, sizeof(buf), msg);
    strcatn(buf, sizeof(buf), "\r\n");

    for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
        dnext = d->next;
        queue_ansi(d, buf);
        process_output(d);
    }
}

/**
 * Send a message to all online WIZARD players and flush output immediately
 *
 * \r\n will be added to the end of the message.
 *
 * This is just like wall_and_flush, just for wizards only.
 * @see wall_and_flush
 *
 * @param msg the message to send
 */
void
wall_wizards(const char *msg)
{
    struct descriptor_data *dnext;
    char buf[BUFFER_LEN + 2];

    strcpyn(buf, sizeof(buf), msg);
    strcatn(buf, sizeof(buf), "\r\n");

    for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
        dnext = d->next;

        if (d->connected && Wizard(d->player)) {
            queue_ansi(d, buf);
            process_output(d);
        }
    }
}

/**
 * Write a text block from a queue, advancing the queue if needed.
 *
 * This also adds the block to d->pending_ssl_write if needed.
 *
 * Returns 1 if something incomplete written, 0 if write was successful, 
 * and -1 if I/O error
 *
 * @private
 * @param d the descriptor to send the text to
 * @param qp pointer to queue pointer; the queue to process.
 * @return integer, 0 for success, 1 for incomplete write, and -1 on error
 */
static int
write_text_block(struct descriptor_data *d, struct text_block **qp)
{
    int count;
    struct text_block *cur = *qp;
    int was_wouldblock = 0, was_error = 0;

    if (!cur)
        return 0;

    count = socket_write(d, cur->start, (size_t)cur->nchars);

#ifdef WIN32
    if (count < 0 || count == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            was_wouldblock = 1;
        else
            was_error = 1;
    }
#else
    if (count < 0) {
        if (errno == EWOULDBLOCK)
            was_wouldblock = 1;
        else
            was_error = 1;
    }
#endif

    if (was_error) {
        return -1;
    } else if (was_wouldblock) {
        count = 0;
    }

    d->output_size -= count;

    if (count == cur->nchars) {
        *qp = cur->nxt;
        free_text_block(cur);
        return 0;
    }

    cur->nchars -= (size_t)count;
    cur->start += count;

#ifdef USE_SSL
    if (cur != d->pending_ssl_write.head) {
        assert(!d->pending_ssl_write.head);
        d->pending_ssl_write.head = cur;
        d->pending_ssl_write.tail = &cur->nxt;
        d->pending_ssl_write.lines = 1;
        *qp = cur->nxt;
        cur->nxt = 0;
    }
#endif
    return 1;
}

/**
 * Write all messages in queue
 *
 * Returns 0 if completed successfuly, 1 if write blocked or failed, so we
 * are done handling writing this connection for this cycle of the main loop.
 *
 * @private
 * @param d the descriptor structure to write to
 * @param queue the queue to write to the descriptor
 * @return integer 0 if successful, 1 if write blocked or fail.
 */
static int
write_queue(struct descriptor_data * d, struct text_queue *queue)
{
    int result = 0;
    struct text_block **qp = &queue->head;

    /* Iterate over the text queue and write each block */
    while (*qp && result == 0) {
        result = write_text_block(d, qp);
        if (result == 0) {
            queue->lines--;
        }
    }

    if (!*qp) {
        queue->tail = &queue->head;
    }

    /* Boot them if we need to */
    if (result == -1) {
        /*
         * Booted can be 2, so, we actually do need this if statement.
         */
        if (!d->booted) {
            d->booted = 1;
        }

        result = 1;
    }

    return result;
}

/**
 * Process output, writing out all the queues for a given descriptor.
 *
 * If d is NULL or d->descriptor not set, it will panic the server.
 * Pending SSL writes and priority output is always written.  If
 * d->block_writes is true, then d->output is not written.
 *
 * @param d the descriptor structure who's queues we should write out
 */
void
process_output(struct descriptor_data *d)
{
    /* drastic, but this may give us crash test data */
    if (!d || !d->descriptor) {
        panic("process_output(): bad descriptor or connect struct !");
    }

#ifdef USE_SSL
    if (write_queue(d, &d->pending_ssl_write))
        return;
#endif
    if (write_queue(d, &d->priority_output))
        return;

    if (d->block_writes)
        return;

    if (write_queue(d, &d->output))
        return;
}

/**
 * Boot a player's oldest descriptor
 *
 * Processes the output on that last descriptor, then sets the booted
 * flag to '1'.  Booting is therefore not instant.
 *
 * @param player the player whom's descriptor we will boot.
 * @return boolean true if we marked someone to be booted, false otherwise
 */
int
boot_off(dbref player)
{
    int *darr;
    int dcount;
    struct descriptor_data *last = NULL;

    darr = get_player_descrs(player, &dcount);

    if (darr) {
        last = descrdata_by_descr(darr[0]);
    }

    if (last) {
        process_output(last);
        last->booted = 1;
        return 1;
    }

    return 0;
}

/**
 * Completely boot a player off, kicking off all their descriptors.
 *
 * Iterates over all a player's descriptors and sets the ->booted flag
 * to 1.  This won't immediately boot people, it just flags them for
 * booting.
 *
 * @param player the player whom's descriptor we will boot.
 */
void
boot_player_off(dbref player)
{
    int *darr;
    int dcount;
    struct descriptor_data *d;

    darr = get_player_descrs(player, &dcount);

    for (int di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);

        if (d) {
            d->booted = 1;
        }
    }
}

/**
 * Close all descriptors, sockets, and clean up the SSL context if applicable
 *
 * This is used as part of shutting down the MUCK.  A message is provided
 * that is written to all the sockets before they are closed.  Frees
 * up associated memory as well.
 *
 * @private
 * @param msg the message to send on shutdown
 */
static void
close_sockets(const char *msg)
{
    struct descriptor_data *dnext;

    /*
     * Iterate over all the descriptors to send the message and shut
     * them all down.
     */
    for (struct descriptor_data *d = descriptor_list; d; d = dnext) {
        dnext = d->next;

        if (d->connected) {
            forget_player_descr(d->player, d->descriptor);
        }

#ifdef USE_SSL
        if (d->pending_ssl_write.head)
            socket_write(d, d->pending_ssl_write.head->start, d->pending_ssl_write.head->nchars);
#endif

        socket_write(d, msg, strlen(msg));
        socket_write(d, shutdown_message, strlen(shutdown_message));

        if (shutdown(d->descriptor, 2) < 0)
            perror("shutdown");

        close(d->descriptor);
        freeqs(d);

#ifdef USE_SSL
        if (d->ssl_session)
            SSL_free(d->ssl_session);
#endif

        *d->prev = d->next;

        if (d->next)
            d->next->prev = d->prev;

        free((void *) d->hostname);
        free((void *) d->username);

        mcp_frame_clear(&d->mcpframe);

        free(d);
        ndescriptors--;
    }

    update_desc_count_table();

    for (int i = 0; i < numsocks; i++) {
        close(sock[i]);
    }

    for (int i = 0; i < numsocks_v6; i++) {
        close(sock_v6[i]);
    }

#ifdef USE_SSL
    for (int i = 0; i < ssl_numsocks; i++) {
        close(ssl_sock[i]);
    }

    for (int i = 0; i < ssl_numsocks_v6; i++) {
        close(ssl_sock_v6[i]);
    }

    SSL_CTX_free(ssl_ctx);
    ssl_ctx = NULL;
#endif
}

/**
 * Handles the @armageddon command, which hopefully you'll never have to use
 *
 * This closes all player connections, kills the host resolver if applicable,
 * deletes the PID file, then exits with the ARMAGEDDON_EXIT_CODE.
 *
 * The intention of this is to not save the database to avoid corruption.
 *
 * It will always broadcast a message saying:
 *
 * \r\nImmediate shutdown initiated by {name}\r\n
 *
 * 'msg', if provided, will be appended after this message.
 *
 * Only wizards can use this command and player permissions are checked.
 *
 * @param player the player attempting to do the armageddon call.
 * @param msg the message to append to the default message, or NULL if you wish
 */
void
do_armageddon(dbref player, const char *msg)
{
    char buf[BUFFER_LEN];

    if (!Wizard(player) || Typeof(player) != TYPE_PLAYER) {
        char unparse_buf[BUFFER_LEN];
        unparse_object(player, player, unparse_buf, sizeof(unparse_buf));

        notify(player, "Sorry, but you don't look like the god of War to me.");
        log_status("ILLEGAL ARMAGEDDON: tried by %s", unparse_buf);
        return;
    }

    snprintf(buf, sizeof(buf), "\r\nImmediate shutdown initiated by %s.\r\n", NAME(player));

    if (msg && *msg)
        strcatn(buf, sizeof(buf), msg);

    log_status("ARMAGEDDON initiated by %s(%d): %s", NAME(player), player, msg);
    fprintf(stderr, "ARMAGEDDON initiated by %s(%d): %s\n", NAME(player), player, msg);
    close_sockets(buf);

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

    unlink(PID_FILE);
    exit(ARMAGEDDON_EXIT_CODE);
}


/**
 * "Panic" the MUCK, which shuts it down with a message.
 *
 * The database is dumped to 'dumpfile' with a '.PANIC' suffix, unless
 * we can't write it.  Macros are similarly dumped with a '.PANIC' suffix
 * unless it cannot be written.
 *
 * If NOCOREDUMP is defined, we will exit with code 135.  Otherwise, we
 * will call abort() which should produce a core dump.
 *
 * @param message the message to show in the log
 */
void
panic(const char *message)
{
    char panicfile[2048];
    FILE *f;

    log_status("PANIC: %s", message);
    fprintf(stderr, "PANIC: %s\n", message);

    /* shut down interface */
    if (!forked_dump_process_flag) {
        close_sockets("\r\nEmergency shutdown due to server crash.");

#ifdef SPAWN_HOST_RESOLVER
        kill_resolver();
#endif
    }

#ifdef SPAWN_HOST_RESOLVER
    if (global_resolver_pid != 0) {
        (void) kill(global_resolver_pid, SIGKILL);
    }
#endif

    /* dump panic file */
    snprintf(panicfile, sizeof(panicfile), "%s.PANIC", dumpfile);

    if ((f = fopen(panicfile, "wb")) != NULL) {
        log_status("DUMPING: %s", panicfile);
        fprintf(stderr, "DUMPING: %s\n", panicfile);
        db_write(f);
        fclose(f);
        log_status("DUMPING: %s (done)", panicfile);
        fprintf(stderr, "DUMPING: %s (done)\n", panicfile);
    } else {
        perror("CANNOT OPEN PANIC FILE, YOU LOSE");
    }

    /* Write out the macros */
    snprintf(panicfile, sizeof(panicfile), "%s.PANIC", MACRO_FILE);

    if ((f = fopen(panicfile, "wb")) != NULL) {
        macrodump(macrotop, f);
        fclose(f);
    } else {
        perror("CANNOT OPEN MACRO PANIC FILE, YOU LOSE");
    }


    sync();
#ifdef NOCOREDUMP
    exit(135);
#else /* !NOCOREDUMP */
#ifdef SIGIOT
    signal(SIGIOT, SIG_DFL);
#endif
    abort();
#endif /* NOCOREDUMP */
}

#ifdef MUD_ID
#include <pwd.h>

/**
 * Do a set UID to the given user name string.
 *
 * Exits with code 1 if there is an error here.  Only defined if MUD_ID is
 * defined.
 *
 * @private
 * @param name the user to setuid to.
 */
static void
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
#endif /* MUD_ID */

#ifdef MUD_GID
#include <grp.h>

/**
 * Do a set GID to the given group name string.
 *
 * Exits with code 1 if there is an error here.  Only defined if MUD_GID is
 * defined.
 *
 * @private
 * @param name the group name to setgid to.
 */
static void
do_setgid(char *name)
{
    struct group *gr;

    if ((gr = getgrnam(name)) == NULL) {
        log_status("can't get grent for group %s", name);
        exit(1);
    }

    if (setgid(gr->gr_gid) == -1) {
        log_status("can't setgid(%d): ", gr->gr_gid);
        perror("setgid");
        exit(1);
    }
}
#endif /* MUD_GID */

/**
 * Get an array of all the given player dbref's descriptors.
 *
 * The count will be stored in 'count'.  The returned value should not
 * be free'd because it would totally screw things up.  If the player
 * is not connected (no descriptors), this will return NULL and count
 * set to 0.
 *
 * @param player the player to get descriptors for
 * @param count the count will be stored to this pointer's location
 * @return the array of descriptor IDs
 */
int *
get_player_descrs(dbref player, int *count)
{
    int *darr;

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

/**
 * Return the current number of descriptors handled by the MUCK.
 *
 * @return the current number of descriptors handled by the MUCK.
 */
int
pcount(void)
{
    return current_descr_count;
}

/**
 * Get the idle time in seconds of a given connection count number.
 *
 * Returns -1 if 'c' doesn't match anything.
 *
 * @param c the connection number
 * @return the idle time in seconds
 */
int
pidle(int c)
{
    /*
     * @TODO This is identical to pdescridle ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescridle.
     */
    struct descriptor_data *d;
    time_t now;

    d = descrdata_by_count(c);

    (void) time(&now);

    if (d) {
        return (int)(now - d->last_time);
    }

    return -1;
}

/**
 * Get the idle time in seconds of a given descriptor.
 *
 * Returns -1 if 'c' doesn't match anything.
 *
 * @param c the descriptor
 * @return the idle time in seconds
 */
int
pdescridle(int c)
{
    struct descriptor_data *d;
    time_t now;

    d = descrdata_by_descr(c);

    (void) time(&now);

    if (d) {
        return (int)(now - d->last_time);
    }

    return -1;
}

/**
 * Get the player dbref for a given connection count number.
 *
 * Returns NOTHING if 'c' doesn't match.
 *
 * @param c the connection count number
 * @return the associated player dbref.
 */
dbref
pdbref(int c)
{
    /*
     * @TODO This is identical to pdescrdbref ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescrdbref.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
        return (d->player);
    }

    return NOTHING;
}

/**
 * Get the player dbref for a given descriptor.
 *
 * Returns NOTHING if 'c' doesn't match.
 *
 * @param c the descriptor
 * @return the associated player dbref.
 */
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

/**
 * Get the online time for a given connection number.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the connection number
 * @return the time connected in seconds.
 */
int
pontime(int c)
{
    /*
     * @TODO This is identical to pdescrontime ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescrontime.
     */
    struct descriptor_data *d;
    time_t now;

    d = descrdata_by_count(c);

    (void) time(&now);

    if (d) {
        return (int)(now - d->connected_at);
    }

    return -1;
}

/**
 * Get the online time for a given descriptor.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the descriptor number
 * @return the time connected in seconds.
 */
int
pdescrontime(int c)
{
    struct descriptor_data *d;
    time_t now;

    d = descrdata_by_descr(c);

    (void) time(&now);

    if (d) {
        return (int)(now - d->connected_at);
    }

    return -1;
}

/**
 * Get the host information for a given connection number.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the connection number
 * @return the host information (could be a name or an IP string)
 */
char *
phost(int c)
{
    /*
     * @TODO This is identical to pdescrhost ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescrhost.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
        return ((char *) d->hostname);
    }

    return (char *) NULL;
}

/**
 * Get the host information for a given descriptor.
 *
 * Returns -1 if 'c' doesn't match a descriptor.
 *
 * @param c the descriptor
 * @return the host information (could be a name or an IP string)
 */
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

/**
 * Get the user information for a given connection number.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the connection number
 * @return the user information
 */
char *
puser(int c)
{
    /*
     * @TODO This is identical to pdescruser ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescruser.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
        return ((char *) d->username);
    }

    return (char *) NULL;
}

/**
 * Get the user information for a given descriptor.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the descriptor number
 * @return the user information
 */
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

/**
 * Get the least idle descriptor for the given player dbref
 *
 * @param who the dbref of the player to find a descriptor for.
 * @return the least idle descriptor or 0 if not connected.
 */
int
least_idle_player_descr(dbref who)
{
    struct descriptor_data *d;
    struct descriptor_data *best_d = NULL;
    int dcount;
    int *darr;
    time_t best_time = 0;

    darr = get_player_descrs(who, &dcount);

    for (int di = 0; di < dcount; di++) {
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

/**
 * Get the least most descriptor for the given player dbref
 *
 * @param who the dbref of the player to find a descriptor for.
 * @return the most idle descriptor or 0 if not connected.
 */
int
most_idle_player_descr(dbref who)
{
    struct descriptor_data *d;
    struct descriptor_data *best_d = NULL;
    int dcount;
    int *darr;
    time_t best_time = 0;

    darr = get_player_descrs(who, &dcount);

    for (int di = 0; di < dcount; di++) {
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

/**
 * Boot a given connection number.
 *
 * Does nothing if connection is not found.
 *
 * @param c the connection number
 */
void
pboot(int c)
{
    /*
     * @TODO This is identical to pdescrboot ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescrboot.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
        process_output(d);
        d->booted = 1;
    }
}

/**
 * Boot a given descriptor.
 *
 * Returns true if booted, false if not found.
 *
 * @param c the descriptor number
 * @return true if booted, false if not found.
 */
int
pdescrboot(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);

    if (d) {
        process_output(d);
        d->booted = 1;
        return 1;
    }

    return 0;
}

/**
 * Send a string to a given connection number.
 *
 * Does nothing if connection is not found.
 *
 * @param c the connection number
 * @param outstr the string to send.
 */
void
pnotify(int c, char *outstr)
{
    /*
     * @TODO This is identical to pdescrnotify ... If we get rid of the
     *       use of connection count numbers, we can get rid of ths call
     *       or alias it to pdescrnotify.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
        queue_ansi(d, outstr);
        queue_write(d, "\r\n", 2);
    }
}

/**
 * Send a string to a given descriptor.
 *
 * Returns boolean true if the descriptor was found, false otherwise.
 *
 * @param c the descriptor
 * @param outstr the string to send.
 * @return boolean true if the descriptor was found, false otherwise.
 */
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

/**
 * Get the descriptor for a given connection number
 *
 * @param c the connection number
 * @return the associated descriptor
 */
int
pdescr(int c)
{
    /*
     * @TODO In a world without connection numbers, this can just return
     *       the descriptor or be replaced with a #define
     */
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
        return (d->descriptor);
    }

    return -1;
}

/**
 * Get the first descriptor currently connected (connection number 1)
 *
 * @return the first descriptor currently connected.
 */
int
pfirstdescr(void)
{
    /*
     * @TODO This is used by prim_firstdescr so this would need to be
     *       reimplemented somehow.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(1);

    if (d) {
        return d->descriptor;
    }

    return 0;
}

/**
 * Get the last descriptor currently connected (connection number 1)
 *
 * @return the last descriptor currently connected.
 */
int
plastdescr(void)
{
    /*
     * @TODO This is used by prim_lastdescr so this would need to be
     *       reimplemented somehow.
     */
    struct descriptor_data *d;

    d = descrdata_by_count(current_descr_count);

    if (d) {
        return d->descriptor;
    }

    return 0;
}

/**
 * Get the next connected descriptor in the linked list
 *
 * @param c the descriptor to get the next descriptor of
 * @return the next connected descriptor in the linked list or NULL if no more
 */
int
pnextdescr(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);

    if (d) {
        d = d->next;
    }

    /*
     * Descriptors may be sitting on the welcome screen -- we want to skip
     * those.
     */
    while (d && (!d->connected))
        d = d->next;

    if (d) {
        return (d->descriptor);
    }

    return (0);
}

/**
 * Get the connection number associated with the given descriptor
 *
 * @param c the descriptor
 * @return the associated connection number
 */
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

/**
 * Set descriptor 'c' to become user 'who'
 *
 * This has some kind of "infinite loop" prevention.  I guess this is
 * to avoid breakages in case descr_setuser is done in a connect propqueue?
 * The infinite loop detection uses a static int variable and to be
 * honest I'm not sure it works right.
 *
 * @param c the descriptor to change users on
 * @param who the player dbref to switch into
 * @return boolean true if successful, false if not
 */
int
pset_user(int c, dbref who)
{
    struct descriptor_data *d;
    static int setuser_depth = 0;

    /*
     * @TODO How does this know the difference between an infinite loop
     *       and a program that makes several setuser calls?  Because
     *       this isn't actually recursive, I don't think there's any
     *       promise that setuser_depth will become 0 again?
     *
     *       Maybe setuser_depth should be a boolean; if true, then
     *       pset_user returns 0.
     *
     *       Set it to 1 before announce_disconnect below and set it to
     *       0 before the return 1.  Should protect it properly, right?
     *       (tanabi)
     */
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

/**
 * Get the first descriptor for the given player dbref
 *
 * @param c the player dbref to get the descriptor for
 * @return the first descriptor for the given player or -1 if not connected.
 */
int
dbref_first_descr(dbref c)
{
    int dcount;
    int *darr;

    darr = get_player_descrs(c, &dcount);

    if (dcount > 0) {
        return darr[dcount - 1];
    } else {
        return -1;
    }
}

/**
 * Process output for a given descriptor number, or for all descriptors if -1
 *
 * This wraps process_output -- @see process_output
 *
 * @param c descriptor number or -1 for all descriptors
 * @return number of descriptors that had their output processed
 */
int
pdescrflush(int c)
{
    struct descriptor_data *d;
    int i = 0;

    if (c != -1) {
        d = descrdata_by_descr(c);

        if (d) {
            process_output(d);
            i++;
        }
    } else {
        for (d = descriptor_list; d; d = d->next) {
            process_output(d);
            i++;
        }
    }

    return i;
}

/**
 * Is the given descriptor secure?
 *
 * Secure descriptors are either SSL, forwarding enbabled, or
 * is_local_connection TRUE.
 *
 * @see is_local_connection
 *
 * @param c the descriptor to chec
 * @return boolean true if c is a secure descriptor.
 */
int
pdescrsecure(int c)
{
#ifdef USE_SSL
    struct descriptor_data *d;

    d = descrdata_by_descr(c);
# ifdef IP_FORWARDING
    /*
     * Consider local connections to always be secure.  This code assumes
     * forwarded is only supported from the localhost.
     */
    if (d && (d->forwarding_enabled || is_local_connection(d)))
        return 1;
# endif

    if (d && d->ssl_session)
        return 1;
    else
        return 0;
#else
    return 0;
#endif
}

/**
 * Return the remaining output buffer size for the given descriptor
 *
 * Calculated using tp_max_output tune parameter.
 *
 * @param c the descriptor to get output buffer size for.
 * @return the number of bytes remaining in the output buffer.
 */
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

/**
 * Partial pmatch searches for 'name' amongst the currently online players
 *
 * It is a "partial" player match because it only matches online players
 * and not players in the database.  It uses string_prefix to do the string
 * match.
 *
 * @see string_prefix
 *
 * This can return NOTHING if there is no match, or AMBIGUOUS if the
 * string matches more than one player.
 *
 * @param name the name to search for
 * @return either the dbref of the player, NOTHING, or AMBIGUOUS
 */
dbref
partial_pmatch(const char *name)
{
    struct descriptor_data *d;
    dbref last = NOTHING;

    if (!name || !*name)
        return AMBIGUOUS;

    d = descriptor_list;
    while (d) {
        if (d->connected && (last != d->player)
            && string_prefix(NAME(d->player), name)) {
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

/**
 * Output status of connections to the log file via log_status
 *
 * This iterates over all the descriptors (both "connected" and logged in
 * and folks sitting on the welcome screen) and dumps information about
 * them into the log file.
 *
 * @see log_status
 */
void
dump_status(void)
{
    time_t now;
    char buf[BUFFER_LEN];

    (void) time(&now);
    log_status("STATUS REPORT:");

    for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
        if (d->connected) {
            snprintf(buf, sizeof(buf),
                     "PLAYING descriptor %d player %s(%d) from host %s(%s), %s.",
                     d->descriptor, NAME(d->player), d->player, d->hostname,
                     d->username,
                     (d->last_time) ? "idle %d seconds" : "never used");
        } else {
            snprintf(buf, sizeof(buf), "CONNECTING descriptor %d from host %s(%s), %s.",
                     d->descriptor, d->hostname, d->username,
                     (d->last_time) ? "idle %d seconds" : "never used");
        }

        log_status(buf, now - d->last_time);
    }
}

/* Ignore support -- Could do with moving into its own file */

/**
 * This is a callback function for qsort
 *
 * It implements an ascending sort order.  Both operands are pointers to
 * dbref's.
 *
 * @private
 * @param Lhs the left hand operand
 * @param Rhs the right hand operand
 * @return Negative if Lhs < Rhs, 0 if they are equal, positive if Lhs > Rhs
 */
static int
ignore_dbref_compare(const void *Lhs, const void *Rhs)
{
    return *(dbref *) Lhs - *(dbref *) Rhs;
}

/**
 * Set up a player's ignore list cache
 *
 * This does nothing if ignore support (tp_ignore_support) is off, or
 * if Player is not TYPE_PLAYER.
 *
 * Loads the player's ignore list from IGNORE_PROP, then puts them
 * in an array of dbrefs (sorted ascending) on the player's struct.
 *
 * Note that if the ignore list is empty, that counts as a failure to
 * load.
 *
 * @private
 * @param Player the player to set up the ignore cache for
 * @return 0 on failure to load, 1 on success.
 */
static int
ignore_prime_cache(dbref Player)
{
    const char *Txt = 0;
    dbref *List = 0;
    size_t Count = 0;
    int i = 0;

    /* Basic checks */
    if (!tp_ignore_support)
        return 0;

    if (!ObjExists(Player) || Typeof(Player) != TYPE_PLAYER)
        return 0;

    if ((Txt = get_property_class(Player, IGNORE_PROP)) == NULL) {
        PLAYER_SET_IGNORE_LAST(Player, AMBIGUOUS);
        return 0;
    }

    skip_whitespace(&Txt);

    /* Empty ignore list */
    if (*Txt == '\0') {
        PLAYER_SET_IGNORE_LAST(Player, AMBIGUOUS);
        return 0;
    }

    /*
     * Count up the number of entries.  It is a space delimited list
     * of dbrefs.
     */
    for (const char *Ptr = Txt; *Ptr;) {
        Count++;

        if (*Ptr == NUMBER_TOKEN)
            Ptr++;

        while (*Ptr && !isspace(*Ptr))
            Ptr++;

        skip_whitespace(&Ptr);
    }

    /*
     * Allocate our array
     */
    if ((List = malloc(sizeof(dbref) * Count)) == 0)
        return 0;

    /*
     * Load values into the array
     */
    for (const char *Ptr = Txt; *Ptr;) {
        if (*Ptr == NUMBER_TOKEN)
            Ptr++;

        if (isdigit(*Ptr))
            List[i++] = atoi(Ptr);
        else
            List[i++] = NOTHING;

        while (*Ptr && !isspace(*Ptr))
            Ptr++;

        skip_whitespace(&Ptr);
    }

    /*
     * Sort it, and store it in our cache
     */
    qsort(List, Count, sizeof(dbref), ignore_dbref_compare);

    PLAYER_SET_IGNORE_CACHE(Player, List);
    PLAYER_SET_IGNORE_COUNT(Player, Count);

    return 1;
}

/**
 * Check to see if 'Who' is being ignored by 'Player'
 *
 * This does a number of techniques to speed up the check.  First, it returns
 * false if Player is ineligible for ignoring (i.e. ignore support is off,
 * Player or Who isn't a valid object, player isn't an unquelled wizard or
 * Player == Who), player has nobody on their ignore list)
 *
 * The last 'successful' run of ignore_is_ignoring_sub is cached, so if
 * the same 'Who' pokes 'Player' twice, the second run doesn't scan the
 * list of ignored folks.
 *
 * Otherwise, the ignore list is searched.  It is an ordered array of
 * refs; the list is searched by finding the midpoint of the array,
 * seeing if 'Who' is > or < than that midpoint, then dividing in half
 * again between the midpoint and the boundary and repeating til we find it.
 *
 * It is slightly more complicated than that, but, that is more or less
 * how it works.  It's faster than a linear one by one search at least.
 *
 * Player and Who may be zombies, that will check the zombie owner's
 * ignore list.
 *
 * @private
 * @param Player the player to check the ignore list on
 * @param Who the person to see if we are ignoring
 * @return boolean true if ignoring, false if not.
 */
static int
ignore_is_ignoring_sub(dbref Player, dbref Who)
{
    int Top, Bottom;
    dbref *List;

    if (!tp_ignore_support)
        return 0;

    if (!OkObj(Player) || !OkObj(Who))
        return 0;

    Player = OWNER(Player);
    Who = OWNER(Who);

    /*
     * You can't ignore yourself, or an unquelled wizard, and unquelled
     * wizards can ignore no one.
     */
    if ((Player == Who) || (Wizard(Player)) || (Wizard(Who)))
        return 0;

    if (PLAYER_IGNORE_LAST(Player) == AMBIGUOUS)
        return 0;

    /* Ignore the last player ignored without bothering to look them up */
    if (PLAYER_IGNORE_LAST(Player) == Who)
        return 1;

    if ((PLAYER_IGNORE_CACHE(Player) == NULL) && !ignore_prime_cache(Player))
        return 0;

    Top = 0;
    Bottom = PLAYER_IGNORE_COUNT(Player);
    List = PLAYER_IGNORE_CACHE(Player);

    /* Try to efficiently scan the ignore list, knowing that it is sorted. */
    while (Top < Bottom) {
        int Middle = Top + (Bottom - Top) / 2;

        if (List[Middle] == Who)
            break;

        if (List[Middle] < Who)
            Top = Middle + 1;
        else
            Bottom = Middle;
    }

    if (Top >= Bottom)
        return 0;

    PLAYER_SET_IGNORE_LAST(Player, Who);

    return 1;
}

/**
 * Check to see if 'Who' is being ignored by 'Player'
 *
 * While usually these single line functions are dumb, this one is kind
 * of necessary because of the tp_ignore_bidirectional flag.  If true,
 * we will also return True if 'Who' is ignoring 'Player'.
 *
 * Player and Who may be zombies, that will check the zombie owner's
 * ignore list.
 *
 * @param Player the player to check the ignore list on
 * @param Who the player to check to see if being ignored.
 * @return boolean true if Player is ignoring Who
 */
inline int
ignore_is_ignoring(dbref Player, dbref Who)
{
    return ignore_is_ignoring_sub(Player, Who) || (tp_ignore_bidirectional
                                  && ignore_is_ignoring_sub(Who, Player));
}

/**
 * Clear a player's ignore cache
 *
 * This will free the memory associated with the cache.  If Player is
 * not a valid TYPE_PLAYER object, then this does nothing.
 *
 * @param Player the player whom's cache we will be deleting
 */
void
ignore_flush_cache(dbref Player)
{
    if (!ObjExists(Player) || Typeof(Player) != TYPE_PLAYER)
        return;

    free(PLAYER_IGNORE_CACHE(Player));
    PLAYER_SET_IGNORE_CACHE(Player, NULL);
    PLAYER_SET_IGNORE_COUNT(Player, 0);
    PLAYER_SET_IGNORE_LAST(Player, NOTHING);
}

/**
 * Add 'Who' to 'Player's ignore list
 *
 * Does nothing if tp_ignore_support is false or Player/Who is invald.
 * As a quirk, as it turns out if 'Who' is any object of any kind, Who's
 * owner will be ignored... so you can ignore someone's exit or room with
 * this function, and wind up ignoring that exit or room's owner.  Weird,
 * huh?
 *
 * Player is the same way; Player can be any kind of object, and its owner's
 * ignore list will be added to.
 *
 * Flushes Player's ignore cache once done.  @see ignore_flush_cache
 *
 * @param Player the player who's ignore list we are manipulating
 * @param Who the target to add to the ignore list
 */
void
ignore_add_player(dbref Player, dbref Who)
{
    if (!tp_ignore_support)
        return;

    if (!OkObj(Player) || !OkObj(Who))
        return;

    reflist_add(OWNER(Player), IGNORE_PROP, OWNER(Who));

    ignore_flush_cache(OWNER(Player));
}

/**
 * Remove 'Who' from Player's ignore list
 *
 * The same notes for ignore_add_player apply here as the two functions
 * are basically identical, just one adds and one removes.  A single line
 * difference in the code.
 *
 * @see ignore_add_player
 *
 * @param Player the player who's ignore list we are manipulating
 * @param Who the person to add to the ignore list
 */
void
ignore_remove_player(dbref Player, dbref Who)
{
    if (!tp_ignore_support)
        return;

    if (!OkObj(Player) || !OkObj(Who))
        return;

    reflist_del(OWNER(Player), IGNORE_PROP, OWNER(Who));

    ignore_flush_cache(OWNER(Player));
}

/**
 * Remove a given player from all ignore lists
 *
 * This is just used by player toading.  It iterates over the entire DB,
 * and once done, flushes *all* ignore caches.
 *
 * @param Player the player to remove from all ignore lists
 */
void
ignore_remove_from_all_players(dbref Player)
{
    if (!tp_ignore_support)
        return;

    for (dbref i = 0; i < db_top; i++)
        if (Typeof(i) == TYPE_PLAYER)
            reflist_del(i, IGNORE_PROP, Player);

    /* Don't touch the database if it's not been loaded yet... */
    if (db == 0)
        return;

    for (dbref i = 0; i < db_top; i++) {
        /*
         * @TODO while it would be very slightly less efficient (an
         *       ObjExists check added to each object), this little if
         *       statement could be replaced with a call to ignore_flush_cache
         *       to centralize the memory free-ing logic.
         *
         *       This only gets called when a player gets toaded, so I
         *       think the cleanliness is worth a very slight performance
         *       impact.
         */
        if (Typeof(i) == TYPE_PLAYER) {
            free(PLAYER_IGNORE_CACHE(i));
            PLAYER_SET_IGNORE_CACHE(i, NULL);
            PLAYER_SET_IGNORE_COUNT(i, 0);
            PLAYER_SET_IGNORE_LAST(i, NOTHING);
        }
    }
}

/**
 * Output a file to a player, optionally using a segment.
 *
 * Segments are line numbers.  Either a single line number or
 * a range of line numbers.
 *
 * If the file is missing, it outputs an error and outputs the message
 * to stderr.  There's no restriction on file paths.
 *
 * @param player the player to send the file to
 * @param filename the file to send to the player
 * @param seg this can be an empty string or a line number range
 */
void
spit_file_segment(dbref player, const char *filename, const char *seg)
{
    FILE *f;
    char buf[BUFFER_LEN];
    char segbuf[BUFFER_LEN];
    char *p;
    int startline, endline, currline;

    startline = endline = currline = 0;

    /* Figure out the start and endline from the segment. */
    if (seg && *seg) {
        strcpyn(segbuf, sizeof(segbuf), seg);
        for (p = segbuf; isdigit(*p); p++) ;

        if (*p) {
            *p++ = '\0';
            startline = atoi(segbuf);

            while (*p && !isdigit(*p))
                p++;

            if (*p)
                endline = atoi(p);
        } else {
            endline = startline = atoi(segbuf);
        }
    }

    if ((f = fopen(filename, "rb")) == NULL) {
        /*
         * @TODO perhaps this should go to the log as well
         */
        notifyf(player, "Sorry, %s is missing.  Management has been notified.", filename);
        fputs("spit_file_segment:", stderr);
        perror(filename);
    } else {
        while (fgets(buf, sizeof buf, f)) {
            for (p = buf; *p; p++) {
                if (*p == '\n' || *p == '\r') {
                    *p = '\0';
                    break;
                }
            }

            currline++;

            if ((!startline || (currline >= startline)) && (!endline || (currline <= endline))) {
                if (*buf) {
                    notify(player, buf);
                } else {
                    notify(player, "  ");
                }
            }
        }

        fclose(f);
    }
}

/**
 * Read a file from a directory with support for partial match
 *
 * Outputs the file to a player.  'topic' is a file name in
 * directory 'd'.  If partial is true, then a partial match also
 * works.  If the file is found we hand off to spit_file_segment.
 *
 * @see spit_file_segment
 *
 * @param player the player to output the file to
 * @param dir the directory to look for topics in
 * @param topic the file or partial file to search for
 * @param seg the segment -- @see spit_file_segment
 * @param partial boolean if true, allow partial file name match
 * @return boolean true if we handed off to spit_file_segment, false otherwise
 */
int
show_subfile(dbref player, const char *dir, const char *topic, const char *seg,
             int partial)
{
    char buf[256];
    struct stat st;

#ifdef DIR_AVAILABLE
    DIR *df;
    struct dirent *dp;
#endif

#ifdef WIN32
    char *dirname;
    int dirnamelen = 0;
    HANDLE hFind;
    BOOL bMore;
    WIN32_FIND_DATA finddata;
#endif

    if (!topic || !*topic)
        return 0;

    if ((*topic == '.') || (*topic == '~') || (strchr(topic, '/'))) {
        return 0;
    }

    if (strlen(topic) > 63)
        return 0;

#ifdef DIR_AVAILABLE
    /* Method of operation: (1) exact match, or (2) partial match, but unique */
    *buf = 0;

    if ((df = (DIR *) opendir(dir))) {
        while ((dp = readdir(df))) {
            if ((partial && string_prefix(dp->d_name, topic)) ||
                (!partial && !strcasecmp(dp->d_name, topic))) {
                snprintf(buf, sizeof(buf), "%s/%s", dir, dp->d_name);
                break;
            }
        }

        closedir(df);
    }

    if (!*buf) {
        return 0; /* no such file or directory */
    }

#elif WIN32
    /* Method of operation: (1) exact match, or (2) partial match, but unique */
    *buf = 0;

    dirnamelen = strlen(dir) + 5;
    dirname = malloc(dirnamelen);
    strcpyn(dirname, dirnamelen, dir);
    strcatn(dirname, dirnamelen, "/*.*");
    hFind = FindFirstFile(dirname, &finddata);
    bMore = (hFind != (HANDLE) - 1);

    free(dirname);

    while (bMore) {
        if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if ((partial && string_prefix(finddata.cFileName, topic)) ||
                (!partial && !strcasecmp(finddata.cFileName, topic))) {
                snprintf(buf, sizeof(buf), "%s/%s", dir, finddata.cFileName);
                break;
            }
        }

        bMore = FindNextFile(hFind, &finddata);
    }
#else /* !DIR_AVAILABLE && !WIN32 */
    snprintf(buf, sizeof(buf), "%s/%s", dir, topic);
#endif

    if (stat(buf, &st)) {
        return 0;
    } else {
        spit_file_segment(player, buf, seg);
        return 1;
    }
}

/**
 * Main function
 *
 * @param argc number of arguments
 * @param argv the array of argument strings
 * @return return code of the program (though usually we exit via exit(...))
 */
int
main(int argc, char **argv)
{
    FILE *ffd, *parmfile = NULL;
    char *infile_name;
    char *outfile_name;
    char *num_one_new_passwd = NULL;
    int nomore_options;
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
    if (strcmp(DoNull(getenv("MALLOC_CHECK_")), "2") != 0) {
        if (setenv("MALLOC_CHECK_", "2", 1) < 0) {
            fprintf(stderr, "[debug]setenv failed: out of memory");
            abort();
        }

        /*
         * Make sure that the malloc checker really works by respawning it...
         * though I'm not sure if C specifies that argv[argc] must be NULL?
         */
        {
            const char *argv2[argc + 1];

            for (int i = 0; i < argc; i++)
                argv2[i] = argv[i];

            argv2[argc] = NULL;
            execvp(argv[0], (char **) argv2);
        }
    }
#endif /* DEBUG */

    /* Set up some global values */
    listener_port[0] = TINYPORT;

    bind_ipv6_address = in6addr_any;
    bind_ipv4_address = INADDR_ANY;

    /* Initialize some lookup tables */
    init_descriptor_lookup();
    init_descr_count_lookup();

    /* More global initalizations */
    nomore_options = 0;
    sanity_skip = 0;
    sanity_interactive = 0;
    sanity_autofix = 0;
    infile_name = NULL;
    outfile_name = NULL;

    /* Process command line arguments */
    for (int i = 1; i < argc; i++) {
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
                if ((ssl_numports + numports) < MAX_LISTEN_SOCKS) {
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
                if ((ssl_numports + numports) < MAX_LISTEN_SOCKS) {
                    ssl_listener_port[ssl_numports++] = atoi(argv[++i]);
                }
#else
                i++;

                fprintf(stderr,
                        "-sport: This server isn't configured to enable SSL.  "
                        "Sorry.\n");
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
            } else if (!strcmp(argv[i], "-parmfile")) {
                if (i + 1 >= argc) {
                    show_program_usage(*argv);
                }

                if ((parmfile = fopen(argv[++i], "r")) == NULL) {
                    perror("read parmfile");
                    exit(1);
                }
            } else if (!strcmp(argv[i], "-bindv4")) {
                if (1 != inet_pton(AF_INET, argv[++i], &bind_ipv4_address)) {
                    fprintf(stderr, "invalid address to bind to: %s\n", argv[i]);
                }
            } else if (!strcmp(argv[i], "-bindv6")) {
                if (1 != inet_pton(AF_INET6, argv[++i], &bind_ipv6_address)) {
                    fprintf(stderr, "invalid address to bind to: %s\n", argv[i]);
                }
            } else if (!strcmp(argv[i], "-nodetach")) {
                no_detach_flag = 1;
            } else if (!strcmp(argv[i], "-resolver")) {
                strcpyn(resolver_program, sizeof(resolver_program), argv[++i]);
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
                if ((ssl_numports + numports) < MAX_LISTEN_SOCKS) {
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

#ifdef DISKBASE
    if (!strcmp(infile_name, outfile_name)) {
        fprintf(stderr, "Output file must be different from the input file.");
        exit(3);
    }
#endif

    tune_load_parms_defaults();

    if (!sanity_interactive) {
        log_status("INIT: TinyMUCK %s starting.", VERSION);

# ifndef WIN32
        /* Go into the background unless requested not to */
        if (!no_detach_flag && !sanity_interactive && !db_conversion_flag) {
            fclose(stdin);
            fclose(stdout);
            fclose(stderr);

            if (fork() != 0)
                _exit(0);
        }
# endif

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

# ifndef WIN32
        if (!sanity_interactive && !db_conversion_flag && !no_detach_flag) {
            /* Detach from the TTY, log whatever output we have... */
            freopen(tp_file_log_stderr, "a", stderr);
            setbuf(stderr, NULL);
            freopen(tp_file_log_stdout, "a", stdout);
            setbuf(stdout, NULL);

            /* Disassociate from Process Group */
#  ifdef _POSIX_SOURCE
            setsid();
#  else
#   ifdef SYSV
            setpgrp(); /* The SysV way */
#   else
            setpgid(0, getpid()); /* The POSIX way. */
#   endif /* SYSV */

#   ifdef  TIOCNOTTY /* we can force this, POSIX / BSD */
            {
                int fd;

                if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
                    ioctl(fd, TIOCNOTTY, (char *) 0); /* lose controll TTY */
                    close(fd);
                }
            }
#   endif /* TIOCNOTTY */
#  endif /* !_POSIX_SOURCE */
        }
#endif /* WIN32 */

#ifdef WIN32
        if (!sanity_interactive && !db_conversion_flag && freeconsole) {
            freopen(tp_file_log_stderr, "a", stderr);
            setbuf(stderr, NULL);
            freopen(tp_file_log_stdout, "a", stdout);
            setbuf(stdout, NULL);
        }
#endif
    }

    /*
     * Make sure that this block of code happens BEFORE any attempt to
     * initialize the sockets. Otherwise, calls to socket() will fail with
     * INVALID_SOCKET. This manifests as the process exiting with an error
     * code of 3.
     */
#ifdef WIN32
    wVersionRequested = MAKEWORD(2, 0);

    err = WSAStartup(wVersionRequested, &wsaData);

    if (err != 0) {
        perror("Unable to start socket layer");
        return 1;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) {
        perror("Winsock 2.0 or later is required to run this application.");
        WSACleanup();
        return 1;
    }

    set_console();
#endif


    /*
     * It is necessary to create the sockets at this point, because the main
     * reason to use MUD_ID is if you want to bind to a privileged port, thus
     * (usually) requiring root permissions.  setuid() requires root
     * permissions anyway (unless obsolete, draft privileges are available on
     * the platform) so MUD_ID is usually only useful in this circumstance.
     */

    for (unsigned int i = 0; i < numports; i++) {
        sock[i] = make_socket(listener_port[i]);
        update_max_descriptor(sock[i]);
        numsocks++;
    }

    for (unsigned int i = 0; i < numports; i++) {
        sock_v6[i] = make_socket_v6(listener_port[i]);
        update_max_descriptor(sock_v6[i]);
        numsocks_v6++;
    }

#ifdef USE_SSL
    for (unsigned int i = 0; i < ssl_numports; i++) {
        ssl_sock[i] = make_socket(ssl_listener_port[i]);
        update_max_descriptor(ssl_sock[i]);
        ssl_numsocks++;
    }

    for (unsigned int i = 0; i < ssl_numports; i++) {
        ssl_sock_v6[i] = make_socket_v6(ssl_listener_port[i]);
        update_max_descriptor(ssl_sock_v6[i]);
        ssl_numsocks_v6++;
    }
#endif

    /*
     * Reset the GID before resetting the UID.  setgid() requires root
     * permissions, which no longer exist after we drop them.
     */

#ifdef MUD_GID
    if (!sanity_interactive) {
        do_setgid(MUD_GID);
    }
#endif /* MUD_GID */

#ifdef MUD_ID
    if (!sanity_interactive) {
        do_setuid(MUD_ID);
    }
#endif /* MUD_ID */

    /* Initialize MCP and some packages. */
    mcp_initialize();
#ifdef MCPGUI_SUPPORT
    gui_initialize();
#endif

    sel_prof_start_time = time(NULL); /* Set useful starting time */
    sel_prof_idle_sec = 0;
    sel_prof_idle_usec = 0;
    sel_prof_idle_use = 0;

    if (init_game(infile_name, outfile_name) < 0) {
        fprintf(stderr, "Couldn't load %s!\n", infile_name);
        unlink(PID_FILE);
        exit(2);
    }

    if (num_one_new_passwd != NULL) {
        set_password(GOD, num_one_new_passwd);
    }

    if (parmfile) {
        tune_load_parms_from_file(parmfile, NOTHING, -1);
    }

#ifdef USE_SSL
    /*
     * This should be done after loading parms (from extra file or DB)
     * in order to ensure that SSL settings are honored.
     */
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    if (!reconfigure_ssl()) {
        for (int i = 0; i < ssl_numsocks; i++) {
            close(ssl_sock[i]);
        }

        for (int i = 0; i < ssl_numsocks_v6; i++) {
            close(ssl_sock_v6[i]);
        }
    }
#endif

    if (!sanity_interactive && !db_conversion_flag) {
        set_signals();

        if (!sanity_skip) {
            do_sanity(AMBIGUOUS);

            if (sanity_violated) {
                wizonly_mode = 1;

                if (sanity_autofix) {
                    do_sanfix(AMBIGUOUS);
                }
            }
        }

        /*
         * Have to do this after setting GID/UID, else we can't kill
         * the bloody thing...
         *
         * Should also wait until after set_signals() is called, so
         * we handle if it immediately crashes.
         */
#ifdef SPAWN_HOST_RESOLVER
        spawn_resolver();
#endif

        /* go do it */
        shovechars();

        if (restart_flag) {
            close_sockets(
                "\r\nServer restarting.  Try logging back on in a few "
                "minutes.\r\n"
            );
        } else {
            close_sockets("\r\nServer shutting down normally.\r\n");
        }

        do_kill_process(-1, (dbref) 1, "all");

#ifdef WIN32
        WSACleanup();
#endif
    }

    if (sanity_interactive) {
        san_main();
    } else {
        dump_database();

#ifdef SPAWN_HOST_RESOLVER
        if (!db_conversion_flag) {
            kill_resolver();
        }
#endif

#ifdef MEMORY_CLEANUP
        db_free();
        purge_macro_tree(macrotop);
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
        CrT_summarize_to_file(tp_file_log_malloc, "Shutdown");
#endif

        if (restart_flag) {
#ifndef WIN32
            char **argslist;
            char numbuf[32];
            size_t argcnt = numports + 2;
            int argnum = 1;

#ifdef USE_SSL
            argcnt += ssl_numports;
#endif

            argslist = calloc(argcnt, sizeof(char *));

            for (int i = 0; i < numports; i++) {
                size_t alen = strlen(numbuf) + 1;
                snprintf(numbuf, sizeof(numbuf), "%d", listener_port[i]);
                argslist[argnum] = malloc(alen);
                strcpyn(argslist[argnum++], alen, numbuf);
            }

#ifdef USE_SSL
            for (int i = 0; i < ssl_numports; i++) {
                size_t alen = strlen(numbuf) + 1;
                snprintf(numbuf, sizeof(numbuf), "-sport %d",
                         ssl_listener_port[i]);
                argslist[argnum] = malloc(alen);
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
#else /* WIN32 */
            char *argbuf[1];
            argbuf[0] = NULL;
            execv("restart", argbuf);
#endif /* WIN32 */
        }
    }

    if (restart_flag) {
        exit(RESTART_EXIT_CODE);
    } else {
        exit(0);
    }
}
