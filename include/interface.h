#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "config.h"

#include "db.h"

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
# define DIR_AVALIBLE
#endif

typedef enum {
    TELNET_STATE_NORMAL,
    TELNET_STATE_IAC,
    TELNET_STATE_WILL,
    TELNET_STATE_DO,
    TELNET_STATE_WONT,
    TELNET_STATE_DONT,
    TELNET_STATE_SB
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

struct text_block {
    int nchars;
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
    int booted;
    int block_writes;
    int is_starttls;
#ifdef USE_SSL
    SSL *ssl_session;
#endif
    dbref player;
    char *output_prefix;
    char *output_suffix;
    int output_size;
    struct text_queue output;
    struct text_queue input;
    char *raw_input;
    char *raw_input_at;
    int telnet_enabled;
    telnet_states_t telnet_state;
    int telnet_sb_opt;
    int short_reads;
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

/* these symbols must be defined by the interface */
struct descriptor_data *descrdata_by_descr(int i);
int notify_nolisten(dbref player, const char *msg, int ispriv);
int notify_filtered(dbref from, dbref player, const char *msg, int ispriv);
void wall_and_flush(const char *msg);
void wall_wizards(const char *msg);
extern int shutdown_flag;	/* if non-zero, interface should shut down */
extern int restart_flag;	/* if non-zero, should restart after shut down */
void emergency_shutdown(void);
int boot_off(dbref player);
void boot_player_off(dbref player);
int online(dbref player);
int index_descr(int c);
int *get_player_descrs(dbref player, int *count);
int least_idle_player_descr(dbref who);
int most_idle_player_descr(dbref who);
int pcount(void);
int pdescrcount(void);
int pidle(int c);
int pdescridle(int c);
int pdbref(int c);
int pdescrdbref(int c);
int pontime(int c);
int pdescrontime(int c);
char *phost(int c);
char *pdescrhost(int c);
char *puser(int c);
char *pdescruser(int c);
int pfirstdescr(void);
int plastdescr(void);
void pboot(int c);
int pdescrboot(int c);
void pnotify(int c, char *outstr);
int pdescrnotify(int c, char *outstr);
int dbref_first_descr(dbref c);
int pdescr(int c);
int pdescrcon(int c);
#ifdef MCP_SUPPORT
McpFrame *descr_mcpframe(int c);
#endif
int pnextdescr(int c);
int pdescrflush(int c);
int pdescrbufsize(int c);
dbref partial_pmatch(const char *name);
int queue_string(struct descriptor_data *, const char *);
int process_output(struct descriptor_data *d);
int ignore_is_ignoring(dbref Player, dbref Who);
int ignore_prime_cache(dbref Player);
void ignore_flush_cache(dbref Player);
void ignore_flush_all_cache(void);
void ignore_add_player(dbref Player, dbref Who);
void ignore_remove_player(dbref Player, dbref Who);
void ignore_remove_from_all_players(dbref Player);

#ifdef USE_SSL
int reconfigure_ssl(void);
#endif

/* the following symbols are provided by game.c */

void process_command(int descr, dbref player, char *command);

dbref create_player(const char *name, const char *password);
dbref connect_player(const char *name, const char *password);
void do_look_around(int descr, dbref player);

int init_game(const char *infile, const char *outfile);
void dump_database(void);
void panic(const char *);

void dump_status(void);
void flush_user_output(dbref player);
extern short global_dumpdone;
#ifndef DISKBASE
extern pid_t global_dumper_pid;
#endif
extern pid_t global_resolver_pid;
long max_open_files(void);
int notify(dbref player, const char *msg);
void notify_except(dbref first, dbref exception, const char *msg, dbref who);
int notify_from(dbref from, dbref player, const char *msg);
int notify_from_echo(dbref from, dbref player, const char *msg, int isprivate);
void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg,
                             int isprivate);
int notify_nolisten(dbref player, const char *msg, int isprivate);
void notifyf(dbref player, char *format, ...);
void notifyf_nolisten(dbref player, char *format, ...);
int pdescrsecure(int c);
int pset_user(int c, dbref who);
extern long sel_prof_idle_sec;
extern unsigned long sel_prof_idle_use;
extern long sel_prof_idle_usec;
extern time_t sel_prof_start_time;
#ifdef SPAWN_HOST_RESOLVER
void spawn_resolver(void);
#endif
char *time_format_2(time_t dt);
extern short wizonly_mode;

int show_subfile(dbref player, const char *dir, const char *topic, const char *seg, int partial);
void spit_file(dbref player, const char *filename);
void spit_file_segment(dbref player, const char *filename, const char *seg);

void san_main(void);
extern int sanity_violated;

#endif				/* _INTERFACE_H */
