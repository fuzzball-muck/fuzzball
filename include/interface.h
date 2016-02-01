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
extern struct descriptor_data *descrdata_by_descr(int i);
extern int notify_nolisten(dbref player, const char *msg, int ispriv);
extern int notify_filtered(dbref from, dbref player, const char *msg, int ispriv);
extern void wall_and_flush(const char *msg);
extern void wall_wizards(const char *msg);
extern int shutdown_flag;		/* if non-zero, interface should shut down */
extern int restart_flag;		/* if non-zero, should restart after shut down */
extern void emergency_shutdown(void);
extern int boot_off(dbref player);
extern void boot_player_off(dbref player);
extern int online(dbref player);
extern int index_descr(int c);
extern int* get_player_descrs(dbref player, int*count);
extern int least_idle_player_descr(dbref who);
extern int most_idle_player_descr(dbref who);
extern int pcount(void);
extern int pdescrcount(void);
extern int pidle(int c);
extern int pdescridle(int c);
extern int pdbref(int c);
extern int pdescrdbref(int c);
extern int pontime(int c);
extern int pdescrontime(int c);
extern char *phost(int c);
extern char *pdescrhost(int c);
extern char *puser(int c);
extern char *pdescruser(int c);
extern int pfirstdescr(void);
extern int plastdescr(void);
extern void pboot(int c);
extern int pdescrboot(int c);
extern void pnotify(int c, char *outstr);
extern int pdescrnotify(int c, char *outstr);
extern int dbref_first_descr(dbref c);
extern int pdescr(int c);
extern int pdescrcon(int c);
#ifdef MCP_SUPPORT
extern McpFrame *descr_mcpframe(int c);
#endif
extern int pnextdescr(int c);
extern int pdescrflush(int c);
extern int pdescrbufsize(int c);
extern dbref partial_pmatch(const char *name);
extern int queue_string(struct descriptor_data *, const char *);
extern int process_output(struct descriptor_data *d);
extern int ignore_is_ignoring(dbref Player, dbref Who);
extern int ignore_prime_cache(dbref Player);
extern void ignore_flush_cache(dbref Player);
extern void ignore_flush_all_cache(void);
extern void ignore_add_player(dbref Player, dbref Who);
extern void ignore_remove_player(dbref Player, dbref Who);
extern void ignore_remove_from_all_players(dbref Player);

/* the following symbols are provided by game.c */

extern void process_command(int descr, dbref player, char *command);

extern dbref create_player(const char *name, const char *password);
extern dbref connect_player(const char *name, const char *password);
extern void do_look_around(int descr, dbref player);

extern int init_game(const char *infile, const char *outfile);
extern void dump_database(void);
extern void panic(const char *);

#endif /* _INTERFACE_H */
