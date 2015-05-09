#ifndef _EXTERNS_AUTO_H
#define _EXTERNS_AUTO_H

int add_muf_delay_event(int delay, int descr, dbref player, dbref loc, dbref trig, dbref prog, struct frame *fr, const char *mode);
int add_muf_delayq_event(int delay, int descr, dbref player, dbref loc, dbref trig, dbref prog, const char *argstr, const char *cmdstr, int listen_p);
int add_muf_timer_event(int descr, dbref player, dbref prog, struct frame *fr, int delay, char *id);
int add_muf_tread_event(int descr, dbref player, dbref prog, struct frame *fr, int delay);
void add_property(dbref player, const char *pname, const char *strval, int value);
int array_delitem(stk_array ** harr, array_iter * item);
int array_delrange(stk_array ** harr, array_iter * start, array_iter * end);
int can_move(int descr, dbref player, const char *direction, int lev);
void clear_propnode(PropPtr p);
void copy_proplist(dbref obj, PropPtr * newer, PropPtr old);
int dequeue_prog_real(dbref program, int sleeponly, const char *, const int);
void diskbase_debug(dbref player);
char * displayprop(dbref player, dbref obj, const char *name, char *buf, size_t bufsiz);
void do_abort_silent(void);
void do_dequeue(int descr, dbref player, const char *arg1);
void do_doing(int descr, dbref player, const char *name, const char *mesg);
void do_edit(int descr, dbref player, const char *name);
void do_flock(int descr, dbref player, const char *name, const char *keyname);
void do_leave(int descr, dbref player);
void do_memory(dbref who);
void do_move(int descr, dbref player, const char *direction, int lev);
void do_newpassword(dbref player, const char *name, const char *password);
void do_oecho(int descr, dbref player, const char *name, const char *message);
void do_pecho(int descr, dbref player, const char *name, const char *message);
void do_pose(dbref player, const char *message);
void do_post_dlog(int descr, const char *text);
void do_propset(int descr, dbref player, const char *name, const char *prop);
void do_score(dbref player);
void do_sweep(int descr, dbref player, const char *name);
void do_tune(dbref player, char *parmname, char *parmval, int full_command_has_delimiter);
void do_whisper(int descr, dbref player, const char *arg1, const char *arg2);
void dump_status(void);
void envpropqueue(int descr, dbref player, dbref where, dbref trigger, dbref what, dbref xclude, const char *propname, const char *toparg, int mlev, int mt);
int exit_loop_check(dbref source, dbref dest);
const char * exit_prefix(register const char *string, register const char *prefix);
int fetch_propvals(dbref obj, const char *dir);
dbref find_mlev(dbref prog, struct frame * fr, int st);
void flush_user_output(dbref player);
int format_time(char *buf, int max_len, const char *fmt, struct tm *tmval);
long get_tz_offset(void);
dbref getparent(dbref obj);
int gui_dlog_freeall_descr(int descr);
void gui_initialize(void);
void init_match_remote(int descr, dbref player, dbref what, const char *name, int type, struct match_data *md);
void kill_resolver(void);
void list_proglines(dbref player, dbref program, struct frame *fr, int start, int end);
void listenqueue(int descr, dbref player, dbref where, dbref trigger, dbref what, dbref xclude, const char *propname, const char *toparg, int mlev, int mt, int mpi_p);
void log_command(char *format, ...);
void log_status(char *format, ...);
int mcpframe_to_descr(McpFrame * ptr);
int mcpframe_to_user(McpFrame * ptr);
void muf_backtrace(dbref player, dbref program, int count, struct frame *fr);
int muf_debugger(int descr, dbref player, dbref program, const char *text, struct frame *fr);
void muf_dlog_purge(struct frame *fr);
void muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive);
int muf_event_count(struct frame* fr);
long next_muckevent_time(void);
void next_muckevent(void);
int notify_nolisten(dbref player, const char *msg, int isprivate);
int notify_from_echo(dbref from, dbref player, const char *msg, int isprivate);
int notify_from(dbref from, dbref player, const char *msg);
int notify(dbref player, const char *msg);
void notify_fmt(dbref player, char *format, ...);
void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg, int isprivate);
void notify_except(dbref first, dbref exception, const char *msg, dbref who);
int ok_password(const char *password);
int ok_player_name(const char *name);
void parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg, const char *prefix, const char *whatcalled, int mpiflags);
void prog_clean(struct frame *fr);
int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);
int prop_write_perms(dbref player, dbref obj, const char *name, int mlev);
int pset_user(int c, dbref who);
void san_main(void);
void sane_dump_object(dbref player, const char *arg);
void sanechange(dbref player, const char *command);
void sanfix(dbref player);
void sanity(dbref player);
int scopedvar_poplevel(struct frame *fr);
void send_contents(int descr, dbref loc, dbref dest);
void set_signals(void);
void set_source(dbref player, dbref action, dbref source);
void show_mcp_error(McpFrame * mfr, char *topic, char *text);
long size_boolexp(struct boolexp *b);
long size_prog(dbref prog);
long size_properties(dbref player, int load);
long size_proplist(PropPtr avl);
void spit_file_segment(dbref player, const char *filename, const char *seg);
void spit_file(dbref player, const char *filename);
int string_compare(const char *s1, const char *s2);
int string_prefix(const char *string, const char *prefix);
char * strip_ansi(char *buf, const char *input);
char * strip_bad_ansi(char *buf, const char *input);
int test_lock(int descr, dbref player, dbref thing, const char *lockprop);
int test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop);
char * time_format_2(long dt);
void ts_lastuseobject(dbref thing);
void ts_useobject(dbref thing);
void tune_save_parmsfile(void);
int tune_setparm(const char *parmname, const char *val, int security);
void tune_freeparms(void);
const char * unparse_flags(dbref thing);
void untouchprops_incremental(int limit);

#endif /* _EXTERNS_AUTO_H */

#ifdef DEFINE_HEADER_VERSIONS

#ifndef externs_autoh_version
#define externs_autoh_version
const char *externs_auto_h_version = "$RCSfile: externs-auto.h,v $ $Revision: 1.15 $";
#endif
#else
extern const char *externs_auto_h_version;
#endif

