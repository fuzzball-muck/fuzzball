#ifndef INTERFACE_H
#define INTERFACE_H

#include <stddef.h>
#include <time.h>

#include "autoconf.h"
#include "config.h"
#include "fbmuck.h"

#ifdef MCP_SUPPORT
#include "mcp.h"
#endif

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

typedef enum {
    TELNET_STATE_NORMAL,
    TELNET_STATE_IAC,
    TELNET_STATE_WILL,
    TELNET_STATE_DO,
    TELNET_STATE_WONT,
    TELNET_STATE_DONT,
    TELNET_STATE_SB,
    TELNET_STATE_FORWARDING  // Non-standard telnet extension for gateway support.
} telnet_states_t;

#define TELNET_IAC	255
#define TELNET_DONT	254
#define TELNET_DO	253
#define TELNET_WONT	252
#define TELNET_WILL	251
#define TELNET_SB	250
#define TELNET_GA	249
#define TELNET_EL	248
#define TELNET_EC	247
#define TELNET_AYT	246
#define TELNET_AO	245
#define TELNET_IP	244
#define TELNET_BRK	243
#define TELNET_DM	242
#define TELNET_NOP	241
#define TELNET_SE	240

#define TELOPT_STARTTLS	46
#define TELOPT_FORWARDED  113 /* non-standard; for gateways */

struct text_block {
    size_t nchars;
    struct text_block *nxt;
    char *start;
    char *buf;
};

struct text_queue {
    int lines;
    struct text_block *head;
    struct text_block **tail;
};

struct descriptor_data {
    int descriptor;
    int connected;
    int con_number;
    int booted; /* 0 = do not boot; 1 = boot without message; 2 = boot with goodbye */
    int block_writes; /* if true, block non-priority writes. Used for STARTTLS. */
    int is_starttls;
#ifdef USE_SSL
    SSL *ssl_session;
    /* incomplete SSL_write() because OpenSSL does not allow us to switch from a partial
       write of something from output to writing something else from priority_output. */
    struct text_queue pending_ssl_write;
#endif
    dbref player;
    int output_size;
    struct text_queue output;
    struct text_queue priority_output; /* used for telnet messages */
    struct text_queue input;
    char *raw_input;
    char *raw_input_at;
    int telnet_enabled;
    telnet_states_t telnet_state;
    int telnet_sb_opt;
    int short_reads;

    /* Fields related to ip-forwarding, to support websocket gateways  */
    int forwarding_enabled;
    char *forwarded_buffer;
    int forwarded_size;

    time_t last_time;
    time_t connected_at;
    time_t last_pinged_at;
    const char *hostname;
    const char *username;
    int quota;
    struct descriptor_data *next;
    struct descriptor_data **prev;
#ifdef MCP_SUPPORT
    McpFrame mcpframe;
#endif
};

extern short db_conversion_flag;
extern struct descriptor_data *descriptor_list;
extern short global_dumpdone;
#ifndef DISKBASE
extern pid_t global_dumper_pid;
#endif
extern pid_t global_resolver_pid;
extern int restart_flag;
extern int sanity_violated;
extern long sel_prof_idle_sec;
extern unsigned long sel_prof_idle_use;
extern long sel_prof_idle_usec;
extern time_t sel_prof_start_time;
extern int shutdown_flag;
extern short wizonly_mode;

int boot_off(dbref player);
void boot_player_off(dbref player);
int dbref_first_descr(dbref c);
struct descriptor_data *descrdata_by_descr(int i);

void dump_database(void);
void dump_status(void);
void emergency_shutdown(void);
int *get_player_descrs(dbref player, int *count);
void ignore_add_player(dbref Player, dbref Who);
void ignore_flush_cache(dbref Player);
int ignore_is_ignoring(dbref Player, dbref Who);
void ignore_remove_from_all_players(dbref Player);
void ignore_remove_player(dbref Player, dbref Who);
int index_descr(int c);
int least_idle_player_descr(dbref who);
void look_room(int descr, dbref player, dbref loc);
long max_open_files(void);
int most_idle_player_descr(dbref who);
int notify(dbref player, const char *msg);
void notify_except(dbref first, dbref exception, const char *msg, dbref who);
int notify_filtered(dbref from, dbref player, const char *msg, int ispriv);
int notify_from(dbref from, dbref player, const char *msg);
int notify_from_echo(dbref from, dbref player, const char *msg, int isprivate);
void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg,
                             int isprivate);
int notify_nolisten(dbref player, const char *msg, int isprivate);
void notifyf(dbref player, char *format, ...);
void notifyf_nolisten(dbref player, char *format, ...);
int online(dbref player);
dbref partial_pmatch(const char *name);
void pboot(int c);
int pcount(void);
int pdbref(int c);
int pdescr(int c);
int pdescrboot(int c);
int pdescrbufsize(int c);
int pdescrcon(int c);
int pdescrdbref(int c);
int pdescrflush(int c);
char *pdescrhost(int c);
int pdescridle(int c);
int pdescrnotify(int c, char *outstr);
int pdescrontime(int c);
int pdescrsecure(int c);
char *pdescruser(int c);
int pfirstdescr(void);
char *phost(int c);
int plastdescr(void);
int pidle(int c);
int pnextdescr(int c);
void pnotify(int c, char *outstr);
int pontime(int c);
void process_output(struct descriptor_data *d);
int pset_user(int c, dbref who);
char *puser(int c);
#ifdef USE_SSL
int reconfigure_ssl(void);
#endif
void queue_write(struct descriptor_data *, const char *, size_t);
void san_main(void);
int show_subfile(dbref player, const char *dir, const char *topic, const char *seg, int partial);
#ifdef SPAWN_HOST_RESOLVER
void spawn_resolver(void);
#endif
void spit_file(dbref player, const char *filename);
void wall_and_flush(const char *msg);
void wall_wizards(const char *msg);

#endif /* !INTERFACE_H */
