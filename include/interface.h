/** @file interface.h
 *
 * This header has a wide range of different calls in it which (mostly)
 * have to do with the interface between players and the game but has some
 * random other stuff too.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stddef.h>
#include <time.h>

#include "autoconf.h"
#include "config.h"
#include "fbmuck.h"
#include "mcp.h"

/* For the SSL* type. */
#ifdef USE_SSL
# ifdef HAVE_OPENSSL
#  include <openssl/ssl.h>
# else
#  include <ssl.h>
# endif
#endif

/*
 * Ok, directory stuff IS a bit ugly.
 */
#if defined(HAVE_DIRENT_H) || defined(_POSIX_VERSION)
# include <dirent.h>
#else                           /* not (HAVE_DIRENT_H or _POSIX_VERSION) */
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif                         /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif                         /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif                         /* HAVE_NDIR_H */
#endif                          /* not (HAVE_DIRENT_H or _POSIX_VERSION) */

#if defined(HAVE_DIRENT_H) || defined(_POSIX_VERSION) || defined(HAVE_SYS_NDIR_H) || defined(HAVE_SYS_DIR_H) || defined(HAVE_NDIR_H)
# define DIR_AVAILABLE
#endif

/*
 * Telnet uses a state machine to negotiate what it supports.
 */
typedef enum {
    TELNET_STATE_NORMAL,    /* Default state                                 */
    TELNET_STATE_IAC,       /* Interpret as command -- allows telnet control */
    TELNET_STATE_WILL,      /* Indicates the desire to perform an operation  */
    TELNET_STATE_DO,        /* Indicates the other party WILL                */
    TELNET_STATE_WONT,      /* Opposite of WILL                              */
    TELNET_STATE_DONT,      /* Opposite of DO                                */
    TELNET_STATE_SB,        /* Subnegotiation                                */
    TELNET_STATE_FORWARDING /* Non-standard extension for gateway support    */
} telnet_states_t;

/*
 * These are telnet commands.  If you're curious about how this works, the
 * RFC is here:
 *
 * https://imgur.com/gallery/UE0HIH6
 */
#define TELNET_IAC  255 /* Interpret as command -- next byte is command    */
#define TELNET_DONT 254 /* Don't - tell other party to not do an operation */
#define TELNET_DO   253 /* Do - tell other party to do an operation        */
#define TELNET_WONT 252 /* Won't - indicate not doing an operation         */
#define TELNET_WILL 251 /* Will - indicate that we will do an operation    */
#define TELNET_SB   250 /* Perform a sub negotiation                       */
#define TELNET_GA   249 /* Go ahead acknowledgment                         */
#define TELNET_EL   248 /* Erase line function                             */
#define TELNET_EC   247 /* Erase character function                        */
#define TELNET_AYT  246 /* Are you there?  Ping function.                  */
#define TELNET_AO   245 /* Abort output                                    */
#define TELNET_IP   244 /* Interrupt process                               */
#define TELNET_BRK  243 /* Break                                           */
#define TELNET_DM   242 /* Data mark - part of a sync operation            */
#define TELNET_NOP  241 /* No Op                                           */
#define TELNET_SE   240 /* End of subnegotiation                           */

#define TELOPT_STARTTLS     46  /* Start TLS                  */
#define TELOPT_FORWARDED    113 /* non-standard; for gateways */

struct text_block {
    size_t nchars;          /* Number of characters in the text block   */
    struct text_block *nxt; /* Next block in the queue                  */
    char *start;            /* Pointer into buf, advanced during writes */
    char *buf;              /* The whole buffer                         */
};

struct text_queue {
    int lines;                  /* Lines in the queue */
    struct text_block *head;    /* Head of the queue  */
    struct text_block **tail;   /* End of the queue   */
};

struct descriptor_data {
    int descriptor;         /* Descriptor                          */
    int connected;          /* Is playing? (has gotten past login) */
    int con_number;         /* The connection number               */
    int booted;             /* 0 = do not boot;
                             * 1 = boot without message;
                             * 2 = boot with goodbye
                             */
    int block_writes;       /* If true, block non-priority writes.
                             * Used for STARTTLS.
                             */
    int is_starttls;        /* Has TLS started?                    */
#ifdef USE_SSL
    SSL *ssl_session;       /* SSL Session structure for TLS       */
    /*
     * incomplete SSL_write() because OpenSSL does not allow us to switch
     * from a partial write of something from output to writing something
     * else from priority_output.
     */
    struct text_queue pending_ssl_write;
#endif
    dbref player;                   /* The player for this descriptor      */
    int output_size;                /* Size of output queue in bytes       */
    struct text_queue output;       /* Output queue                        */
    struct text_queue priority_output; /* used for telnet messages         */
    struct text_queue input;        /* Input queue                         */
    char *raw_input;                /* Raw input from the connection       */
    char *raw_input_at;             /* Pointer into that raw input buffer  */
    int telnet_enabled;             /* Descriptor supports telnet protocol */
    telnet_states_t telnet_state;   /* Current telnet state                */
    int telnet_sb_opt;              /* Subnegotiation option               */
    int short_reads;                /* If true, read byte at a time        */

#ifdef IP_FORWARDING
    /* Fields related to ip-forwarding, to support websocket gateways  */
    int forwarding_enabled;         /* IP forwarding enabled?              */
    char *forwarded_buffer;         /* Buffer for forwarding               */
    int forwarded_size;             /* Amount in buffer, must be < 128     */
#endif

    time_t last_time;               /* Last time they did something        */
    time_t connected_at;            /* Connection timestamp                */
    time_t last_pinged_at;          /* Last time we sent data to them      */
    const char *hostname;           /* Descriptor host name                */
    const char *username;           /* Ident username if available         */
    int quota;                      /* Command burst quota                 */
    struct descriptor_data *next;   /* Linked list of descriptors          */
    struct descriptor_data **prev;  /* Double linked list                  */
    McpFrame mcpframe;              /* MCP Frame information               */
};

/**
 * @var If true, we are doing a "DB conversion" which is a command line
 *      process.  If this is true, most of the MUCK code is skipped.
 *      If this is true, the MUCK will not be detached.
 */
extern short db_conversion_flag;

/**
 * @var The list of descriptors being managed.
 */
extern struct descriptor_data *descriptor_list;

/**
 * @var Boolean, true if has the forked dump process has completed.
 */
extern short global_dumpdone;

#ifndef DISKBASE
/**
 * @var PID of the forked dump process - unused for DISKBASE - 0 if not running
 */
extern pid_t global_dumper_pid;
#endif

/**
 * @var PID of the forked resolver process or 0 if not running
 */
extern pid_t global_resolver_pid;

/**
 * @var If true, the MUCK will restart.
 */
extern int restart_flag;
extern int sanity_violated;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
extern long sel_prof_idle_sec;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
extern unsigned long sel_prof_idle_use;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
extern long sel_prof_idle_usec;

/**
 * @var Global used for profiling -- @see do_topprofs and relatives
 */
extern time_t sel_prof_start_time;

/**
 * @var Boolean, if true, shut down the MUCK.
 */
extern int shutdown_flag;

/**
 * @var boolean, if true, only wizards may log in ("maintenance mode")
 */
extern short wizonly_mode;

/**
 * Boot a player's oldest descriptor
 *
 * Processes the output on that last descriptor, then sets the booted
 * flag to '1'.
 *
 * @param player the player whom's descriptor we will boot.
 * @return boolean true if we marked someone to be booted, false otherwise
 */
int boot_off(dbref player);

/**
 * Completely boot a player off, kicking off all their descriptors.
 *
 * Iterates over all a player's descriptors and sets the ->booted flag
 * to 1.  This won't immediately boot people, it just flags them for
 * booting.
 *
 * @param player the player whom's descriptor we will boot.
 */
void boot_player_off(dbref player);

/**
 * Get the first descriptor for the given player dbref
 *
 * @param c the player dbref to get the descriptor for
 * @return the first descriptor for the given player or -1 if not connected.
 */
int dbref_first_descr(dbref c);

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
struct descriptor_data *descrdata_by_descr(int i);

/**
 * Output status of connections to the log file via log_status
 *
 * This iterates over all the descriptors (both "connected" and logged in
 * and folks sitting on the welcome screen) and dumps information about
 * them into the log file.
 *
 * @see log_status
 */
void dump_status(void);

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
void panic(const char *message);

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
int *get_player_descrs(dbref player, int *count);

/**
 * Add 'Who' to 'Player's ignore list
 *
 * Does nothing if tp_ignore_support is false or Player/Who is invald.
 * As a quirk, as it turns out if 'Who' is any object of any kind, Who's
 * owner will be ignored... so you can ignore someone's exit or room with
 * this function, and wind up ignoring that exit or room's owner.  Weird,
 * huh?
 *
 * Flushes Player's ignore cache once done.  @see ignore_flush_cache
 *
 * @param Player the player who's ignore list we are manipulating
 * @param Who the target to add to the ignore list
 */
void ignore_add_player(dbref Player, dbref Who);

/**
 * Clear a player's ignore cache
 *
 * This will free the memory associated with the cache.  If Player is
 * not a valid TYPE_PLAYER object, then this does nothing.
 *
 * @param Player the player whom's cache we will be deleting
 */
void ignore_flush_cache(dbref Player);

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
int ignore_is_ignoring(dbref Player, dbref Who);

/**
 * Remove a given player from all ignore lists
 *
 * This is just used by player toading.  It iterates over the entire DB,
 * and once done, flushes *all* ignore caches.
 *
 * @param Player the player to remove from all ignore lists
 */
void ignore_remove_from_all_players(dbref Player);

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
void ignore_remove_player(dbref Player, dbref Who);

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
int index_descr(int index);

/**
 * Get the least idle descriptor for the given player dbref
 *
 * @param who the dbref of the player to find a descriptor for.
 * @return the least idle descriptor or 0 if not connected.
 */
int least_idle_player_descr(dbref who);

/*
 * @TODO Why is look_room here?  It is defined in look.c
 */

/**
 * Look at a room
 *
 * This handles all the nuances of looking at a room; displaying room name,
 * description, title, and success prop processing.
 *
 * @private
 * @param descr the descriptor of the looking person
 * @param player the looking person
 * @param loc the location to look at.
 */
void look_room(int descr, dbref player, dbref loc);

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
long max_open_files(void);

/**
 * Get the least most descriptor for the given player dbref
 *
 * @param who the dbref of the player to find a descriptor for.
 * @return the most idle descriptor or 0 if not connected.
 */
int most_idle_player_descr(dbref who);

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
int notify(dbref player, const char *msg);

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
void notify_except(dbref first, dbref exception, const char *msg, dbref who);

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
int notify_filtered(dbref from, dbref player, const char *msg, int ispriv);

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
int notify_from_echo(dbref from, dbref player, const char *msg, int isprivate);

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
void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg,
                             int isprivate);

/**
 * Notify the given player (or zombie) without triggering listen propqueue.
 *
 * This can trigger MPI via pecho if the 'player' is a zombie and zombies
 * are supported.  The MPI will not be able to run recursively; a recursive
 * run will ignore the pecho setting.
 *
 * Multi-line cntent (separated by \r's) are queued up line at a time in
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
int notify_nolisten(dbref player, const char *msg, int isprivate);

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
void notifyf(dbref player, char *format, ...);

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
void notifyf_nolisten(dbref player, char *format, ...);

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
dbref partial_pmatch(const char *name);

/**
 * Boot a given connection number.
 *
 * Does nothing if connection is not found.
 *
 * @param c the connection number
 */
void pboot(int c);

/**
 * Return the current number of descriptors handled by the MUCK.
 *
 * @return the current number of descriptors handled by the MUCK.
 */
int pcount(void);

/**
 * Get the player dbref for a given connection count number.
 *
 * Returns NOTHING if 'c' doesn't match.
 *
 * @param c the connection count number
 * @return the associated player dbref.
 */
int pdbref(int c);

/**
 * Get the descriptor for a given connection number
 *
 * @param c the connection number
 * @return the associated descriptor
 */
int pdescr(int c);

/**
 * Boot a given descriptor.
 *
 * Returns true if booted, false if not found.
 *
 * @param c the descriptor number
 * @return true if booted, false if not found.
 */
int pdescrboot(int c);

/**
 * Return the remaining output buffer size for the given descriptor
 *
 * Calculated using tp_max_output tune parameter.
 *
 * @param c the descriptor to get output buffer size for.
 * @return the number of bytes remaining in the output buffer.
 */
int pdescrbufsize(int c);

/**
 * Get the connection number associated with the given descriptor
 *
 * @param c the descriptor
 * @return the associated connection number
 */
int pdescrcon(int c);

/**
 * Get the player dbref for a given descriptor.
 *
 * Returns NOTHING if 'c' doesn't match.
 *
 * @param c the descriptor
 * @return the associated player dbref.
 */
int pdescrdbref(int c);

/**
 * Process output for a given descriptor number, or for all descriptors if -1
 *
 * This wraps process_output -- @see process_output
 *
 * @param c descriptor number or -1 for all descriptors
 * @return number of descriptors that had their output processed
 */
int pdescrflush(int c);

/**
 * Get the host information for a given descriptor.
 *
 * Returns -1 if 'c' doesn't match a descriptor.
 *
 * @param c the descriptor
 * @return the host information (could be a name or an IP string)
 */
char *pdescrhost(int c);

/**
 * Get the idle time in seconds of a given descriptor.
 *
 * Returns -1 if 'c' doesn't match anything.
 *
 * @param c the descriptor
 * @return the idle time in seconds
 */
int pdescridle(int c);

/**
 * Send a string to a given descriptor.
 *
 * Returns boolean true if the descriptor was found, false otherwise.
 *
 * @param c the descriptor
 * @param outstr the string to send.
 * @return boolean true if the descriptor was found, false otherwise.
 */
int pdescrnotify(int c, char *outstr);

/**
 * Get the online time for a given descriptor.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the descriptor number
 * @return the time connected in seconds.
 */
int pdescrontime(int c);

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
int pdescrsecure(int c);

/**
 * Get the user information for a given descriptor.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the descriptor number
 * @return the user information
 */
char *pdescruser(int c);

/**
 * Get the first descriptor currently connected (connection number 1)
 *
 * @return the first descriptor currently connected.
 */
int pfirstdescr(void);

/**
 * Get the host information for a given connection number.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the connection number
 * @return the host information (could be a name or an IP string)
 */
char *phost(int c);

/**
 * Get the last descriptor currently connected (connection number 1)
 *
 * @return the last descriptor currently connected.
 */
int plastdescr(void);

/**
 * Get the idle time in seconds of a given connection count number.
 *
 * Returns -1 if 'c' doesn't match anything.
 *
 * @param c the connection number
 * @return the idle time in seconds
 */
int pidle(int c);

/**
 * Get the next connected descriptor in the linked list
 *
 * @param c the descriptor to get the next descriptor of
 * @return the next connected descriptor in the linked list or NULL if no more
 */
int pnextdescr(int c);

/**
 * Send a string to a given connection number.
 *
 * Does nothing if connection is not found.
 *
 * @param c the connection number
 * @param outstr the string to send.
 */
void pnotify(int c, char *outstr);

/**
 * Get the online time for a given connection number.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the connection number
 * @return the time connected in seconds.
 */
int pontime(int c);

/**
 * Process output, writing out all the queues for a given descriptor.
 *
 * If d is NULL or d->descriptor not set, it will panic the server.
 * Pending SSL writes and priority output is always written.  If
 * d->block_writes is true, then d->output is not written.
 *
 * @param d the descriptor structure who's queues we should write out
 */
void process_output(struct descriptor_data *d);

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
int pset_user(int c, dbref who);

/**
 * Get the user information for a given connection number.
 *
 * Returns -1 if 'c' doesn't match a connection.
 *
 * @param c the connection number
 * @return the user information
 */
char *puser(int c);

#ifdef USE_SSL
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
int reconfigure_ssl(void);
#endif

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
void queue_write(struct descriptor_data *, const char *, size_t);

/**
 * This is the main function for the sanity interactive editor.
 *
 * When the MUCK is running, you won't be able to access this.
 *
 * This is defined in sanity.c, but there is no sanity.h, so this lives
 * here.
 */
void san_main(void);

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
int show_subfile(dbref player, const char *dir, const char *topic, const char *seg, int partial);

#ifdef SPAWN_HOST_RESOLVER
/**
 * Spawn the host resolver.
 *
 * This won't spawn the resolver if the last spawn time is under 5
 * seconds ago.
 *
 * Resolver PId is stored in global 'global_resolver_pid'.
 */
void spawn_resolver(void);
#endif

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
void spit_file_segment(dbref player, const char *filename, const char *seg);

/**
 * Send a message to everyone online and flush the output immediately.
 *
 * \r\n will be added to the end of the message.
 *
 * @param msg the message to send to all the descriptors
 */
void wall_and_flush(const char *msg);

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
void wall_wizards(const char *msg);

#endif /* !INTERFACE_H */
