#ifndef _EXTERNS_H
#define _EXTERNS_H

/* Definition of pid_t */
#include <sys/types.h>
/* Definition of 'dbref' */
#include "db.h"
/* Definition of match_data */
#include "match.h"
/* Definition of PropPtr, among other things */
#include "props.h"

/* array.c */
extern int array_delitem(stk_array ** harr, array_iter * item);
extern int array_delrange(stk_array ** harr, array_iter * start, array_iter * end);

/* boolexp.c */
extern struct boolexp *copy_bool(struct boolexp *old);
extern int eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing);
extern void free_boolexp(struct boolexp *b);
extern struct boolexp *parse_boolexp(int descr, dbref player, const char *string, int dbloadp);
extern long size_boolexp(struct boolexp *b);

/* compile.c */
extern void clear_primitives(void);
extern void do_compile(int descr, dbref in_player, dbref in_program, int force_err_disp);
extern void do_uncompile(dbref player);
extern void free_unused_programs(void);
extern int get_primitive(const char *);
extern void init_primitives(void);
extern long size_prog(dbref prog);
extern void uncompile_program(dbref i);

/* create.c */
extern void do_action(int descr, dbref player, const char *action_name,
		      const char *source_name);
extern void do_attach(int descr, dbref player, const char *action_name,
		      const char *source_name);
extern void do_clone(int descr, dbref player, char *name);
extern void do_create(dbref player, char *name, char *cost);
extern void do_edit(int descr, dbref player, const char *name);
extern void do_dig(int descr, dbref player, const char *name, const char *pname);
extern void do_link(int descr, dbref player, const char *name, const char *room_name);
extern void do_open(int descr, dbref player, const char *direction, const char *linkto);
extern void do_prog(int descr, dbref player, const char *name);
extern int exit_loop_check(dbref source, dbref dest);
extern int link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list);
extern int link_exit_dry(int descr, dbref player, dbref exit, char *dest_name,
			 dbref * dest_list);
extern void set_source(dbref player, dbref action, dbref source);
extern int unset_source(dbref player, dbref action);

/* crt_malloc.c */
#ifdef MALLOC_PROFILING
extern void CrT_summarize(dbref player);
extern void CrT_summarize_to_file(const char *file, const char *comment);
#endif

/* db.c */
extern void db_clear_object(dbref i);
extern void db_free_object(dbref i);

/* debugger.c */
extern void list_proglines(dbref player, dbref program, struct frame *fr, int start, int end);
extern void muf_backtrace(dbref player, dbref program, int count, struct frame *fr);
extern int muf_debugger(int descr, dbref player, dbref program, const char *text,
			struct frame *fr);
extern char *show_line_prims(dbref program, struct inst *pc, int maxprims, int markpc);

/* disassem.c */
extern void disassemble(dbref player, dbref program);

/* edit.c */
extern void chown_macros(dbref from, dbref to);
extern void do_list(dbref player, dbref program, int *oarg, int argc);
extern void free_old_macros();
extern void free_prog_text(struct line *l);
extern struct line *get_new_line(void);
extern void interactive(int descr, dbref player, const char *command);
extern char *macro_expansion(struct macrotable *node, const char *match);
extern void match_and_list(int descr, dbref player, const char *name, char *linespec);
extern struct macrotable *new_macro(const char *name, const char *definition, dbref player);
extern struct line *read_program(dbref i);
extern void write_program(struct line *first, dbref i);

/* events.c */
extern void dump_db_now(void);
extern time_t next_event_time(void);
extern void next_muckevent(void);
extern time_t next_muckevent_time(void);

/* game.c */
extern void cleanup_game();
extern const char *compile_options;
extern short db_conversion_flag;
extern void do_dump(dbref player, const char *newfile);
extern void do_shutdown(dbref player);
extern void dump_warning(void);
extern int force_level;
extern dbref force_prog;
extern void fork_and_dump(void);
extern FILE *input_file;

/* hashtab.c */
extern hash_entry *add_hash(const char *name, hash_data data, hash_tab * table, unsigned size);
extern hash_data *find_hash(const char *s, hash_tab * table, unsigned size);
extern int free_hash(const char *name, hash_tab * table, unsigned size);
extern unsigned int hash(const char *s, unsigned int hash_size);
extern void kill_hash(hash_tab * table, unsigned size, int freeptrs);

/* help.c */
extern void do_help(dbref player, char *topic, char *seg);
extern void do_info(dbref player, const char *topic, const char *seg);
extern void do_man(dbref player, char *topic, char *seg);
extern void do_motd(dbref player, char *text);
extern void do_mpihelp(dbref player, char *topic, char *seg);
extern void do_news(dbref player, char *topic, char *seg);
extern void spit_file(dbref player, const char *filename);

/* interface.c */
extern void do_armageddon(dbref player, const char *msg);
extern void dump_status(void);
extern void flush_user_output(dbref player);
extern short global_dumpdone;
#ifndef DISKBASE
extern pid_t global_dumper_pid;
#endif
extern pid_t global_resolver_pid;
extern long max_open_files(void);
extern int notify(dbref player, const char *msg);
extern int notify_from(dbref from, dbref player, const char *msg);
extern int notify_from_echo(dbref from, dbref player, const char *msg, int isprivate);
extern int notify_nolisten(dbref player, const char *msg, int isprivate);
extern void notifyf(dbref player, char *format, ...);
extern void notifyf_nolisten(dbref player, char *format, ...);
extern int pdescrsecure(int c);
extern int pset_user(int c, dbref who);
extern long sel_prof_idle_sec;
extern unsigned long sel_prof_idle_use;
extern long sel_prof_idle_usec;
extern time_t sel_prof_start_time;
#ifdef SPAWN_HOST_RESOLVER
extern void spawn_resolver(void);
#endif
extern char *time_format_2(time_t dt);
extern short wizonly_mode;

/* interp.c */
extern void do_abort_silent(void);
extern dbref find_mlev(dbref prog, struct frame *fr, int st);
extern struct frame *interp(int descr, dbref player, dbref location, dbref program,
			    dbref source, int nosleeping, int whichperms, int forced_pid);
extern struct inst *interp_loop(dbref player, dbref program, struct frame *fr, int rettyp);
extern void prog_clean(struct frame *fr);
extern void purge_all_free_frames();
extern void purge_for_pool(void);
extern void purge_try_pool(void);

/* log.c */
extern void log2file(char *myfilename, char *format, ...);
extern void log_command(char *format, ...);
extern void log_gripe(char *format, ...);
extern void log_muf(char *format, ...);
extern void log_program_text(struct line *first, dbref player, dbref i);
extern void log_sanity(char *format, ...);
extern void log_status(char *format, ...);
extern void log_user(dbref player, dbref program, char *logmessage);

/* look.c */
extern void do_contents(int descr, dbref player, const char *name, const char *flags);
extern void do_entrances(int descr, dbref player, const char *name, const char *flags);
extern void do_examine(int descr, dbref player, const char *name, const char *dir);
extern void do_find(dbref player, const char *name, const char *flags);
extern void do_inventory(dbref player);
extern void do_look_at(int descr, dbref player, const char *name, const char *detail);
extern void do_owned(dbref player, const char *name, const char *flags);
extern void do_score(dbref player);
extern void do_sweep(int descr, dbref player, const char *name);
extern void do_trace(int descr, dbref player, const char *name, int depth);
extern void exec_or_notify_prop(int descr, dbref player, dbref thing, const char *propname,
				const char *whatcalled);
extern long size_object(dbref i, int load);

/* match.c */
extern void init_match_remote(int descr, dbref player, dbref what, const char *name, int type,
			      struct match_data *md);

/* move.c */
extern int can_move(int descr, dbref player, const char *direction, int lev);
extern void do_drop(int descr, dbref player, const char *name, const char *obj);
extern void do_get(int descr, dbref player, const char *what, const char *obj);
extern void do_leave(int descr, dbref player);
extern void do_move(int descr, dbref player, const char *direction, int lev);
extern void do_recycle(int descr, dbref player, const char *name);
extern void enter_room(int descr, dbref player, dbref loc, dbref exit);
extern void moveto(dbref what, dbref where);
extern int parent_loop_check(dbref source, dbref dest);
extern void send_contents(int descr, dbref loc, dbref dest);
extern void send_home(int descr, dbref thing, int homepuppet);
extern void recycle(int descr, dbref player, dbref thing);

/* msgparse.c */
extern void mesg_init(void);
extern time_t mpi_prof_start_time;
extern void purge_mfns();

/* mufevent.c */
extern void muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive);
extern int muf_event_count(struct frame *fr);
extern int muf_event_exists(struct frame *fr, const char *eventid);

/* p_props.c */
extern int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);
extern int prop_write_perms(dbref player, dbref obj, const char *name, int mlev);

/* player.c */
extern void add_player(dbref who);
extern int check_password(dbref player, const char *password);
extern void clear_players(void);
extern void delete_player(dbref who);
extern void do_password(dbref player, const char *old, const char *newobj);
extern dbref lookup_player(const char *name);
extern void set_password(dbref player, const char *password);
extern void set_password_raw(dbref player, const char *password);
extern char *whowhere(dbref player);

/* predicates.c */
extern int can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg);
extern int can_link(dbref who, dbref what);
extern int can_link_to(dbref who, object_flag_type what_type, dbref where);
extern int can_see(dbref player, dbref thing, int can_see_location);
extern int controls(dbref who, dbref what);
extern int controls_link(dbref who, dbref what);
extern int could_doit(int descr, dbref player, dbref thing);
extern dbref getparent(dbref obj);
extern int isancestor(dbref parent, dbref child);
extern int ok_ascii_other(const char *name);
extern int ok_ascii_thing(const char *name);
extern int ok_name(const char *name);
extern int ok_password(const char *password);
extern int ok_player_name(const char *name);
extern int payfor(dbref who, int cost);
extern int restricted(dbref player, dbref thing, object_flag_type flag);
extern int test_lock(int descr, dbref player, dbref thing, const char *lockprop);
extern int test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop);

/* property.c */
extern char *displayprop(dbref player, dbref obj, const char *name, char *buf, size_t bufsiz);
extern long size_properties(dbref player, int load);
extern void untouchprops_incremental(int limit);

/* props.c */
extern void clear_propnode(PropPtr p);
extern void copy_proplist(dbref obj, PropPtr * newer, PropPtr old);
extern long size_proplist(PropPtr avl);

/* random.c */
extern void delete_seed(void *buffer);
extern void *init_seed(char *seed);
extern void MD5base64(char *dest, const void *orig, int len);
extern unsigned long rnd(void *buffer);

/* rob.c */
extern void do_give(int descr, dbref player, const char *recipient, int amount);
extern void do_kill(int descr, dbref player, const char *what, int cost);
extern void do_rob(int descr, dbref player, const char *what);

/* sanity.c */
extern void san_main(void);
extern void sane_dump_object(dbref player, const char *arg);
extern void sanechange(dbref player, const char *command);
extern void sanfix(dbref player);
extern void sanity(dbref player);
extern int sanity_violated;

/* set.c */
extern void do_chown(int descr, dbref player, const char *name, const char *newobj);
extern void do_name(int descr, dbref player, const char *name, char *newname);
extern void do_propset(int descr, dbref player, const char *name, const char *prop);
extern void do_relink(int descr, dbref player, const char *thing_name, const char *dest_name);
extern void do_set(int descr, dbref player, const char *name, const char *flag);
extern void do_unlink(int descr, dbref player, const char *name);
extern void do_unlink_quiet(int descr, dbref player, const char *name);
extern void set_flags_from_tunestr(dbref obj, const char *flags);
extern void set_standard_lock(int descr, dbref player, const char *objname,
			      const char *propname, const char *proplabel,
			      const char *keyvalue);
extern void set_standard_property(int descr, dbref player, const char *objname,
				  const char *propname, const char *proplabel,
				  const char *propvalue);
extern int setlockstr(int descr, dbref player, dbref thing, const char *keyname);

/* signal.c */
#ifdef WIN32
extern void set_console(void);
#endif
extern void set_dumper_signals(void);
extern void set_signals(void);

/* smatch.c */
extern int equalstr(char *s, char *t);

/* speech.c */
extern void do_gripe(dbref player, const char *message);
extern void do_page(dbref player, const char *arg1, const char *arg2);
extern void do_pose(dbref player, const char *message);
extern void do_say(dbref player, const char *message);
extern void do_wall(dbref player, const char *message);
extern void do_whisper(int descr, dbref player, const char *arg1, const char *arg2);
extern void notify_except(dbref first, dbref exception, const char *msg, dbref who);
extern void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg,
			     int isprivate);
extern void parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg,
			   const char *prefix, const char *whatcalled, int mpiflags);
extern void parse_oprop(int descr, dbref player, dbref dest, dbref exit, const char *propname,
			const char *prefix, const char *whatcalled);

/* strftime.c */
extern int format_time(char *buf, int max_len, const char *fmt, struct tm *tmval);
extern long get_tz_offset(void);

/* stringutil.c */
extern int alphanum_compare(const char *s1, const char *s2);
extern int blank(const char *s);
extern const char *exit_prefix(register const char *string, register const char *prefix);
extern char *intostr(int i);
extern int ifloat(const char *s);
extern int is_prop_prefix(const char *Property, const char *Prefix);
extern int number(const char *s);
extern void prefix_message(char *Dest, const char *Src, const char *Prefix, int BufferLength,
			   int SuppressIfPresent);
extern int prepend_string(char **before, char *start, const char *what);
extern char *pronoun_substitute(int descr, dbref player, const char *str);
extern char *strcatn(char *buf, size_t bufsize, const char *src);
extern char *strcpyn(char *buf, size_t bufsize, const char *src);
extern const char *strencrypt(const char *, const char *);
extern const char *strdecrypt(const char *, const char *);
extern int string_compare(const char *s1, const char *s2);
#if !defined(MALLOC_PROFILING)
extern char *string_dup(const char *s);
#endif
extern const char *string_match(const char *src, const char *sub);
extern int string_prefix(const char *string, const char *prefix);
extern char *strip_ansi(char *buf, const char *input);
extern char *strip_bad_ansi(char *buf, const char *input);

/* time.c */
extern void ts_lastuseobject(dbref thing);
extern void ts_modifyobject(dbref thing);
extern void ts_newobject(struct object *thing);
extern void ts_useobject(dbref thing);

/* timequeue.c */
extern int add_mpi_event(int delay, int descr, dbref player, dbref loc, dbref trig,
			 const char *mpi, const char *cmdstr, const char *argstr, int listen_p,
			 int omesg_p, int bless_p);
extern int add_muf_read_event(int descr, dbref player, dbref prog, struct frame *fr);
extern int add_muf_delay_event(int delay, int descr, dbref player, dbref loc, dbref trig,
			       dbref prog, struct frame *fr, const char *mode);
extern int add_muf_delayq_event(int delay, int descr, dbref player, dbref loc, dbref trig,
				dbref prog, const char *argstr, const char *cmdstr,
				int listen_p);
extern int add_muf_timer_event(int descr, dbref player, dbref prog, struct frame *fr,
			       int delay, char *id);
extern int add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig, dbref prog,
			       const char *argstr, const char *cmdstr, int listen_p);
extern int control_process(dbref player, int procnum);
extern int dequeue_process(int procnum);
extern int dequeue_prog_real(dbref, int, const char *, const int);
extern int dequeue_timers(int procnum, char *timerid);
extern void do_dequeue(int descr, dbref player, const char *arg1);
extern void envpropqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
			 dbref xclude, const char *propname, const char *toparg, int mlev,
			 int mt);
extern stk_array *get_pidinfo(int pid);
extern stk_array *get_pids(dbref ref);
extern void handle_read_event(int descr, dbref player, const char *command);
extern void listenqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
			dbref xclude, const char *propname, const char *toparg, int mlev,
			int mt, int mpi_p);
extern void list_events(dbref program);
extern int in_timequeue(int pid);
extern void next_timequeue_event(void);
extern void propqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
		      dbref xclude, const char *propname, const char *toparg, int mlev,
		      int mt);
extern void purge_timenode_free_pool(void);
extern int read_event_notify(int descr, dbref player, const char *cmd);
extern int scan_instances(dbref program);
extern struct frame *timequeue_pid_frame(int pid);

/* tune.c */
extern void do_tune(dbref player, char *parmname, char *parmval,
		    int full_command_has_delimiter);
extern void tune_freeparms(void);

/* unparse.c */
extern const char *unparse_boolexp(dbref player, struct boolexp *b, int fullname);
extern const char *unparse_flags(dbref thing);
extern const char *unparse_object(dbref player, dbref object);

/* utils.c */
extern int member(dbref thing, dbref list);
extern dbref remove_first(dbref first, dbref what);

/* version.c */
extern void do_credits(dbref player);
extern void do_version(dbref player);
extern void do_hashes(dbref player, char *args);

/* wiz.c */
extern void do_all_topprofs(dbref player, char *arg1);
extern void do_bless(int descr, dbref player, const char *target, const char *propname);
extern void do_boot(dbref player, const char *name);
extern void do_force(int descr, dbref player, const char *what, char *command);
extern void do_memory(dbref who);
extern void do_mpi_topprofs(dbref player, char *arg1);
extern void do_muf_topprofs(dbref player, char *arg1);
extern void do_newpassword(dbref player, const char *name, const char *password);
extern void do_pcreate(dbref player, const char *arg1, const char *arg2);
extern void do_serverdebug(int descr, dbref player, const char *arg1, const char *arg2);
extern void do_stats(dbref player, const char *name);
extern void do_teleport(int descr, dbref player, const char *arg1, const char *arg2);
extern void do_toad(int descr, dbref player, const char *name, const char *recip);
extern void do_unbless(int descr, dbref player, const char *target, const char *propname);
extern void do_usage(dbref player);

#endif				/* _EXTERNS_H */
