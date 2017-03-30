#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "config.h"

void do_action(int descr, dbref player, const char *action_name, const char *source_name);
void do_armageddon(dbref player, const char *msg);
void do_attach(int descr, dbref player, const char *action_name, const char *source_name);

void do_bless(int descr, dbref player, const char *target, const char *propname);
void do_boot(dbref player, const char *name);

void do_chown(int descr, dbref player, const char *name, const char *newobj);
void do_clone(int descr, dbref player, const char *name, const char *rname);
void do_contents(int descr, dbref player, const char *name, const char *flags);
void do_create(dbref player, char *name, char *cost);
void do_credits(dbref player);

void do_debug(dbref player, const char *args);
void do_dig(int descr, dbref player, const char *name, const char *pname);
void do_dump(dbref player, const char *newfile);

void do_edit(int descr, dbref player, const char *name);
void do_entrances(int descr, dbref player, const char *name, const char *flags);
void do_examine_sanity(dbref player, const char *arg);

void do_find(dbref player, const char *name, const char *flags);
void do_force(int descr, dbref player, const char *what, char *command);

void do_hashes(dbref player, char *args);

void do_kill_process(int descr, dbref player, const char *arg1);

void do_link(int descr, dbref player, const char *name, const char *room_name);
void do_list(int descr, dbref player, const char *name, char *linespec);

#ifdef MCP_SUPPORT
void do_mcpedit(int descr, dbref player, const char *name);
void do_mcpprogram(int descr, dbref player, const char *name, const char *rname);
#endif
#ifndef NO_MEMORY_COMMAND
void do_memory(dbref who);
#endif
void do_mpi_topprofs(dbref player, char *arg1);
void do_muf_topprofs(dbref player, char *arg1);

void do_name(int descr, dbref player, const char *name, char *newname);
void do_newpassword(dbref player, const char *name, const char *password);

void do_open(int descr, dbref player, const char *direction, const char *linkto);
void do_owned(dbref player, const char *name, const char *flags);

void do_password(dbref player, const char *old, const char *newobj);
void do_pcreate(dbref player, const char *arg1, const char *arg2);
void do_process_status(dbref player);
void do_program(int descr, dbref player, const char *name, const char *rname);
void do_propset(int descr, dbref player, const char *name, const char *prop);

#ifdef USE_SSL
void do_reconfigure_ssl(dbref player);
#endif
void do_recycle(int descr, dbref player, const char *name);
void do_relink(int descr, dbref player, const char *thing_name, const char *dest_name);
void do_restart(dbref player);
void do_restrict(dbref player, const char *arg);

void do_sanchange(dbref player, const char *command);
void do_sanfix(dbref player);
void do_sanity(dbref player);

void do_set(int descr, dbref player, const char *name, const char *flag);
void do_shutdown(dbref player);
void do_stats(dbref player, const char *name);
void do_sweep(int descr, dbref player, const char *name);

void do_teleport(int descr, dbref player, const char *arg1, const char *arg2);
void do_toad(int descr, dbref player, const char *name, const char *recip);
void do_topprofs(dbref player, char *arg1);
void do_trace(int descr, dbref player, const char *name, int depth);
void do_tune(dbref player, char *parmname, char *parmval, int full_command_has_delimiter);

void do_unbless(int descr, dbref player, const char *target, const char *propname);
void do_unlink(int descr, dbref player, const char *name);
void do_uncompile(dbref player);
#ifndef NO_USAGE_COMMAND
void do_usage(dbref player);
#endif

void do_version(dbref player);

void do_wall(dbref player, const char *message);

void set_standard_lock(int descr, dbref player, const char *objname,
                              const char *propname, const char *proplabel,
                              const char *keyvalue);
void set_standard_property(int descr, dbref player, const char *objname,
                                  const char *propname, const char *proplabel,
                                  const char *propvalue);


void do_drop(int descr, dbref player, const char *name, const char *obj);
void do_examine(int descr, dbref player, const char *name, const char *dir);
void do_get(int descr, dbref player, const char *what, const char *obj);
void do_give(int descr, dbref player, const char *recipient, int amount);
void do_gripe(dbref player, const char *message);
void do_help(dbref player, char *topic, char *seg);
void do_info(dbref player, const char *topic, const char *seg);
void do_inventory(dbref player);
void do_kill(int descr, dbref player, const char *what, int cost);
void do_leave(int descr, dbref player);
void do_look_at(int descr, dbref player, const char *name, const char *detail);
void do_man(dbref player, char *topic, char *seg);
void do_motd(dbref player, char *text);
void do_move(int descr, dbref player, const char *direction, int lev);
void do_mpihelp(dbref player, char *topic, char *seg);
void do_news(dbref player, char *topic, char *seg);
void do_page(dbref player, const char *arg1, const char *arg2);
void do_pose(dbref player, const char *message);
void do_rob(int descr, dbref player, const char *what);
void do_say(dbref player, const char *message);
void do_score(dbref player);
void do_whisper(int descr, dbref player, const char *arg1, const char *arg2);

#endif				/* _COMMANDS_H */
