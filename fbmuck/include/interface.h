#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "db.h"
#ifdef MCP_SUPPORT
#include "mcp.h"
#endif
/* these symbols must be defined by the interface */
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
