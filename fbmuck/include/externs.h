#ifndef _EXTERNS_H
#define _EXTERNS_H

/* Definition of pid_t */
#include <sys/types.h>

/* Definition of 'dbref' */
#include "db.h"
/* Definition of 'McpFrame' */
#include "mcp.h"
/* Definition of PropPtr, among other things */
#include "props.h"
/* Definition of match_data */
#include "match.h"
/* Auto-generated list of extern functions */
#include "externs-auto.h"

/* Prototypes for externs not defined elsewhere */

extern char match_args[];
extern char match_cmdname[];

/* from event.c */
extern long next_muckevent_time(void);
extern void next_muckevent(void);

/* from timequeue.c */
extern void handle_read_event(int descr, dbref player, const char* command);
extern int read_event_notify(int descr, dbref player, const char* cmd);
extern int add_muf_read_event(int descr, dbref player, dbref prog, struct frame *fr);
extern int add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig, dbref prog,
						   const char *argstr, const char *cmdstr, int listen_p);
extern int add_event(int event_type, int subtyp, int dtime, int descr, dbref player,
					 dbref loc, dbref trig, dbref program, struct frame *fr,
					 const char *strdata, const char *strcmd, const char *str3);
extern void next_timequeue_event(void);
extern int in_timequeue(int pid);
extern struct frame* timequeue_pid_frame(int pid);
extern long next_event_time(void);
extern void list_events(dbref program);
extern int dequeue_prog_real(dbref, int, const char *, const int);
extern int dequeue_process(int procnum);
extern int dequeue_timers(int procnum, char* timerid);
extern void purge_timenode_free_pool(void);
extern int control_process(dbref player, int procnum);
extern void do_dequeue(int descr, dbref player, const char *arg1);
extern void propqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
					  dbref xclude, const char *propname, const char *toparg,

					  int mlev, int mt);
extern void envpropqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
						 dbref xclude, const char *propname, const char *toparg,

						 int mlev, int mt);
extern int scan_instances(dbref program);
extern int program_active(dbref program);

/* from db.c */
extern int number(const char *s);
extern int ifloat(const char *s);
extern void putproperties(FILE * f, int obj);
extern void getproperties(FILE * f, int obj, const char *pdir);
extern void free_line(struct line *l);
extern void db_free_object(dbref i);
extern void db_clear_object(dbref i);
extern void macrodump(struct macrotable *node, FILE * f);
extern void macroload(FILE * f);
extern void free_prog_text(struct line *l);
extern struct line *read_program(dbref i);
extern void write_program(struct line *first, dbref i);
extern char *show_line_prims(struct frame *fr, dbref program, struct inst *pc, int maxprims, int markpc);
extern dbref db_write_deltas(FILE * f);

/* From create.c */
extern void do_open(int descr, dbref player, const char *direction, const char *linkto);
extern void do_link(int descr, dbref player, const char *name, const char *room_name);
extern void do_dig(int descr, dbref player, const char *name, const char *pname);
extern void do_create(dbref player, char *name, char *cost);
extern void do_clone(int descr, dbref player, char *name);
extern void do_prog(int descr, dbref player, const char *name);
extern void do_mcpedit(int descr, dbref player, const char *name);
extern void do_mcpprogram(int descr, dbref player, const char* name);
extern void mcpedit_program(int descr, dbref player, dbref prog, const char* name);
extern void do_attach(int descr, dbref player, const char *action_name, const char *source_name);
extern int unset_source(dbref player, dbref loc, dbref action);
extern int link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list);
extern int link_exit_dry(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list);
extern void do_action(int descr, dbref player, const char *action_name, const char *source_name);

/* from edit.c */
extern struct macrotable
*new_macro(const char *name, const char *definition, dbref player);
extern char *macro_expansion(struct macrotable *node, const char *match);
extern void match_and_list(int descr, dbref player, const char *name, char *linespec);
extern void do_list(dbref player, dbref program, int *oarg, int argc);

/* From game.c */
extern void do_dump(dbref player, const char *newfile);
extern void do_shutdown(dbref player);
extern void dump_warning(void);
extern void dump_deltas(void);
extern void fork_and_dump(void);

/* From hashtab.c */
extern unsigned int hash(const char *s, unsigned int hash_size);
extern hash_data *find_hash(const char *s, hash_tab * table, unsigned size);
extern hash_entry *add_hash(const char *name, hash_data data, hash_tab * table, unsigned size);
extern int free_hash(const char *name, hash_tab * table, unsigned size);
extern void kill_hash(hash_tab * table, unsigned size, int freeptrs);

/* From help.c */
extern void spit_file(dbref player, const char *filename);
extern void do_help(dbref player, char *topic, char *seg);
extern void do_mpihelp(dbref player, char *topic, char *seg);
extern void do_news(dbref player, char *topic, char *seg);
extern void do_man(dbref player, char *topic, char *seg);
extern void do_motd(dbref player, char *text);
extern void do_info(dbref player, const char *topic, const char *seg);

/* From look.c */
extern void look_room(int descr, dbref player, dbref room, int verbose);
extern long size_object(dbref i, int load);


/* extern void look_room_simple(dbref player, dbref room); */
extern void do_look_around(int descr, dbref player);
extern void do_look_at(int descr, dbref player, const char *name, const char *detail);
extern void do_examine(int descr, dbref player, const char *name, const char *dir);
extern void do_inventory(dbref player);
extern void do_find(dbref player, const char *name, const char *flags);
extern void do_owned(dbref player, const char *name, const char *flags);
extern void do_trace(int descr, dbref player, const char *name, int depth);
extern void do_entrances(int descr, dbref player, const char *name, const char *flags);
extern void do_contents(int descr, dbref player, const char *name, const char *flags);
extern void exec_or_notify_prop(int descr, dbref player, dbref thing,
						   const char *propname, const char *whatcalled);
extern void exec_or_notify(int descr, dbref player, dbref thing,
						   const char *message, const char *whatcalled,
						   int mpiflags);

/* From move.c */
extern void moveto(dbref what, dbref where);
extern void enter_room(int descr, dbref player, dbref loc, dbref exit);
extern void send_home(int descr, dbref thing, int homepuppet);
extern int parent_loop_check(dbref source, dbref dest);
extern int can_move(int descr, dbref player, const char *direction, int lev);
extern void do_move(int descr, dbref player, const char *direction, int lev);
extern void do_get(int descr, dbref player, const char *what, const char *obj);
extern void do_drop(int descr, dbref player, const char *name, const char *obj);
extern void do_recycle(int descr, dbref player, const char *name);
extern void recycle(int descr, dbref player, dbref thing);

/* From player.c */
extern dbref lookup_player(const char *name);
extern void do_password(dbref player, const char *old, const char *newobj);
extern void add_player(dbref who);
extern void delete_player(dbref who);
extern void clear_players(void);
extern void set_password_raw(dbref player, const char*password);
extern void set_password(dbref player, const char*password);
extern int check_password(dbref player, const char*password);

/* From predicates.c */
extern int OkObj(dbref obj);
extern int can_link_to(dbref who, object_flag_type what_type, dbref where);
extern int can_link(dbref who, dbref what);
extern int could_doit(int descr, dbref player, dbref thing);
extern int can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg);
extern int can_see(dbref player, dbref thing, int can_see_location);
extern int controls(dbref who, dbref what);
extern int controls_link(dbref who, dbref what);
extern int restricted(dbref player, dbref thing, object_flag_type flag);
extern int payfor(dbref who, int cost);
extern int ok_ascii_thing(const char *name);
extern int ok_ascii_other(const char *name);
extern int ok_name(const char *name);
extern int isancestor(dbref parent, dbref child);

/* From rob.c */
extern void do_kill(int descr, dbref player, const char *what, int cost);
extern void do_give(int descr, dbref player, const char *recipient, int amount);
extern void do_rob(int descr, dbref player, const char *what);

/* From set.c */
extern void do_name(int descr, dbref player, const char *name, char *newname);
extern void do_describe(int descr, dbref player, const char *name, const char *description);
extern void do_idescribe(int descr, dbref player, const char *name, const char *description);
extern void do_fail(int descr, dbref player, const char *name, const char *message);
extern void do_success(int descr, dbref player, const char *name, const char *message);
extern void do_drop_message(int descr, dbref player, const char *name, const char *message);
extern void do_osuccess(int descr, dbref player, const char *name, const char *message);
extern void do_ofail(int descr, dbref player, const char *name, const char *message);
extern void do_odrop(int descr, dbref player, const char *name, const char *message);
extern int setlockstr(int descr, dbref player, dbref thing, const char *keyname);
extern void do_lock(int descr, dbref player, const char *name, const char *keyname);
extern void do_unlock(int descr, dbref player, const char *name);
extern void do_relink(int descr, dbref player, const char *thing_name, const char *dest_name);
extern void do_unlink(int descr, dbref player, const char *name);
extern void do_unlink_quiet(int descr, dbref player, const char *name);
extern void do_chown(int descr, dbref player, const char *name, const char *newobj);
extern void do_set(int descr, dbref player, const char *name, const char *flag);
extern void do_chlock(int descr, dbref player, const char *name, const char *keyname);
extern void do_conlock(int descr, dbref player, const char *name, const char *keyname);
extern void set_flags_from_tunestr(dbref obj, const char* flags);

/* From speech.c */
extern void do_wall(dbref player, const char *message);
extern void do_gripe(dbref player, const char *message);
extern void do_say(dbref player, const char *message);
extern void do_page(dbref player, const char *arg1, const char *arg2);
extern void notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg, int isprivate);
extern void notify_except(dbref first, dbref exception, const char *msg, dbref who);
extern void parse_oprop(int descr, dbref player, dbref dest, dbref exit, const char *propname, const char *prefix, const char *whatcalled);
extern void parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg, const char *prefix, const char *whatcalled, int mpiflags) ;

/* From stringutil.c */
extern int alphanum_compare(const char *s1, const char *s2);
extern int string_compare(const char *s1, const char *s2);
extern int string_prefix(const char *string, const char *prefix);
extern const char *string_match(const char *src, const char *sub);
extern char *pronoun_substitute(int descr, dbref player, const char *str);
extern char *intostr(int i);
extern int prepend_string(char** before, char* start, const char* what);
extern void prefix_message(char* Dest, const char* Src, const char* Prefix, int BufferLength, int SuppressIfPresent);
extern int is_prop_prefix(const char* Property, const char* Prefix);
extern int has_suffix(const char* text, const char* suffix);
extern int has_suffix_char(const char* text, char suffix);
extern char* strcatn(char* buf, size_t bufsize, const char* src);
extern char* strcpyn(char* buf, size_t bufsize, const char* src);


#if !defined(MALLOC_PROFILING)
extern char *string_dup(const char *s);
#endif


/* From utils.c */
extern int member(dbref thing, dbref list);
extern dbref remove_first(dbref first, dbref what);

/* From wiz.c */
extern void do_teleport(int descr, dbref player, const char *arg1, const char *arg2);
extern void do_force(int descr, dbref player, const char *what, char *command);
extern void do_stats(dbref player, const char *name);
extern void do_toad(int descr, dbref player, const char *name, const char *recip);
extern void do_boot(dbref player, const char *name);
extern void do_pcreate(dbref player, const char *arg1, const char *arg2);
extern void do_usage(dbref player);
extern void do_serverdebug(int descr, dbref player, const char *arg1, const char *arg2);
extern void do_muf_topprofs(dbref player, char *arg1);
extern void do_mpi_topprofs(dbref player, char *arg1);
extern void do_all_topprofs(dbref player, char *arg1);
extern void do_bless(int descr, dbref player, const char *target, const char *propname);
extern void do_unbless(int descr, dbref player, const char *target, const char *propname);

extern time_t mpi_prof_start_time;
extern time_t sel_prof_start_time;
extern long sel_prof_idle_sec;
extern long sel_prof_idle_usec;
extern unsigned long sel_prof_idle_use;



/* From boolexp.c */
extern int eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing);
extern struct boolexp *parse_boolexp(int descr, dbref player, const char *string, int dbloadp);
extern struct boolexp *copy_bool(struct boolexp *old);
extern struct boolexp *getboolexp(FILE * f);
extern struct boolexp *negate_boolexp(struct boolexp *b);
extern void free_boolexp(struct boolexp *b);

/* From unparse.c */
extern const char *unparse_object(dbref player, dbref object);
extern const char *unparse_boolexp(dbref player, struct boolexp *b, int fullname);

/* From edit.c */
extern void interactive(int descr, dbref player, const char *command);

/* From compile.c */
extern void uncompile_program(dbref i);
extern void do_uncompile(dbref player);
extern void free_unused_programs(void);
extern int get_primitive(const char *);
extern void do_compile(int descr, dbref in_player, dbref in_program, int force_err_disp);
extern void clear_primitives(void);
extern void init_primitives(void);

/* From interp.c */
extern struct inst *interp_loop(dbref player, dbref program, struct frame *fr, int rettyp);
extern struct frame *interp(int descr, dbref player, dbref location, dbref program,
							dbref source, int nosleeping, int whichperms, int forced_pid);
extern void purge_for_pool(void);
extern void purge_try_pool(void);

/* From mufevent.c */
extern int muf_event_exists(struct frame* fr, const char* eventid);
extern int muf_event_process_unregister(struct frame *fr);

/* from signal.h */
extern void set_dumper_signals(void);
#ifdef WIN32
extern void set_console(void);
extern void check_cosole(void);
#endif

/* declared in log.c */
extern void log2file(char *myfilename, char *format, ...);
extern void log_error(char *format, ...);
extern void log_gripe(char *format, ...);
extern void log_muf(char *format, ...);
extern void log_sanity(char *format, ...);
extern void log_status(char *format, ...);
extern void log_other(char *format, ...);
extern void notify_fmt(dbref player, char *format, ...);
extern void log_program_text(struct line *first, dbref player, dbref i);
extern void log_command(char *format, ...);
extern void log_user(dbref player, dbref program, char *logmessage);

/* From timestamp.c */
extern void ts_newobject(struct object *thing);
extern void ts_useobject(dbref thing);
extern void ts_lastuseobject(dbref thing);
extern void ts_modifyobject(dbref thing);

/* From smatch.c */
extern int equalstr(char *s, char *t);

extern void CrT_summarize(dbref player);

extern int force_level;
extern dbref force_prog;

extern void do_credits(dbref player);
extern void do_version(dbref player);
extern void do_hashes(dbref player, char *args);

extern void disassemble(dbref player, dbref program);

/* from random.c */
void *init_seed(char *seed);
void delete_seed(void *buffer);
unsigned long rnd(void *buffer);

    /* dest buffer MUST be at least 24 chars long. */
void MD5base64(char* dest, const void* orig, int len);

    /* outbuf MUST be at least (((inlen+2)/3)*4)+1 chars long. */
void Base64Encode(char* outbuf, const void* inbuf, size_t inlen);
    /* outbuf MUST be at least (((strlen(inbuf)+3)/4)*3)+1 chars long to read
     * the full set of base64 encoded data in the string. */
size_t Base64Decode(void* outbuf, size_t outbuflen, const char* inbuf);

/* from mcppkgs.c */
extern void show_mcp_error(McpFrame * mfr, char *topic, char *text);

/* from diskprop.c */
extern void dispose_all_oldprops(void);

/* from interface.c */
extern void do_armageddon(dbref player, const char *msg);
extern int pdescrsecure(int c);
extern pid_t global_dumper_pid;
extern pid_t global_resolver_pid;
extern short global_dumpdone;

#ifdef SPAWN_HOST_RESOLVER
extern void spawn_resolver(void);
#endif


/* from events.c */
extern void dump_db_now(void);
extern void delta_dump_now(void);

/* from mesgparse.c */
extern void mesg_init(void);

/* from tune.c */
extern void tune_load_parmsfile(dbref player);

void dump_status(void);
void log_status(char *format, ...);
void kill_resolver(void);

int add_mpi_event(int delay, int descr, dbref player, dbref loc, dbref trig, const char *mpi, const char *cmdstr, const char *argstr, int listen_p, int omesg_p, int bless_p);
stk_array *get_pids(dbref ref);
stk_array *get_pidinfo(int pid);

/* from mcpgui.c */
int gui_dlog_closeall_descr(int descr);

#endif /* _EXTERNS_H */
