#ifndef _TUNE_H
#define _TUNE_H

#include "array.h"

#define TUNESET_SUCCESS         0	/* success                  */
#define TUNESET_SUCCESS_DEFAULT 5	/* success, set to default  */
#define TUNESET_UNKNOWN         1	/* unrecognized sysparm     */
#define TUNESET_SYNTAX          2	/* bad value syntax         */
#define TUNESET_BADVAL          3	/* bad value                */
#define TUNESET_DENIED          4	/* mucker level too low     */

/* Tunable defaults management */

/* Tunable parameter names must not start with TP_FLAG_DEFAULT or they won't be saved. */
#define TP_FLAG_DEFAULT '%'
/* True if parameter starts with default flag, otherwise false */
#define TP_HAS_FLAG_DEFAULT(param) (param != NULL && param[0] == TP_FLAG_DEFAULT)
/* If specified, remove the reset to default flag by incrementing past it */
#define TP_CLEAR_FLAG_DEFAULT(param) if (TP_HAS_FLAG_DEFAULT(param)) { param++; }

extern const char *tp_autolook_cmd;
extern const char *tp_cpennies;
extern const char *tp_cpenny;
extern const char *tp_dumping_mesg;
extern const char *tp_dumpdone_mesg;
extern const char *tp_dumpwarn_mesg;
extern const char *tp_file_connection_help;
extern const char *tp_file_credits;
extern const char *tp_file_editor_help;
extern const char *tp_file_help;
extern const char *tp_file_help_dir;
extern const char *tp_file_info_dir;
extern const char *tp_file_log_cmd_times;
extern const char *tp_file_log_commands;
extern const char *tp_file_log_gripes;
extern const char *tp_file_log_malloc;
extern const char *tp_file_log_muf_errors;
extern const char *tp_file_log_programs;
extern const char *tp_file_log_sanfix;
extern const char *tp_file_log_sanity;
extern const char *tp_file_log_status;
extern const char *tp_file_log_stderr;
extern const char *tp_file_log_stdout;
extern const char *tp_file_log_user;
extern const char *tp_file_man;
extern const char *tp_file_man_dir;
extern const char *tp_file_mpihelp;
extern const char *tp_file_mpihelp_dir;
extern const char *tp_file_motd;
extern const char *tp_file_news;
extern const char *tp_file_news_dir;
extern const char *tp_file_sysparms;
extern const char *tp_file_parameters;
extern const char *tp_file_welcome_screen;
extern const char *tp_gender_prop;
extern const char *tp_huh_mesg;
extern const char *tp_idle_mesg;
extern const char *tp_leave_mesg;
extern const char *tp_muckname;
extern const char *tp_pcreate_flags;
extern const char *tp_pennies;
extern const char *tp_penny;
extern const char *tp_playermax_bootmesg;
extern const char *tp_playermax_warnmesg;
extern const char *tp_register_mesg;
extern const char *tp_reserved_names;
extern const char *tp_reserved_player_names;
extern const char *tp_ssl_cert_file;
extern const char *tp_ssl_key_file;
extern const char *tp_ssl_keyfile_passwd;
extern const char *tp_ssl_cipher_preference_list;
extern const char *tp_ssl_min_protocol_version;

struct tune_str_entry {
    const char *group;
    const char *name;
    const char **str;
    int readmlev;
    int writemlev;
    const char *label;
    char *module;
    int isnullable;
    int isdefault;
    const char *defaultstr;
};

extern struct tune_str_entry tune_str_list[];

extern int tp_aging_time;
extern int tp_clean_interval;
extern int tp_dump_interval;
extern int tp_dump_warntime;
extern int tp_idle_ping_time;
extern int tp_maxidle;
extern int tp_pname_history_threshold;

struct tune_time_entry {
    const char *group;
    const char *name;
    int *tim;
    int readmlev;
    int writemlev;
    const char *label;
    char *module;
    int isdefault;
    const int defaulttim;
};

extern struct tune_time_entry tune_time_list[];

extern int tp_addpennies_muf_mlev;
extern int tp_cmd_log_threshold_msec;
extern int tp_commands_per_time;
extern int tp_command_burst_size;
extern int tp_command_time_msec;
extern int tp_exit_cost;
extern int tp_free_frames_pool;
extern int tp_instr_slice;
extern int tp_kill_base_cost;
extern int tp_kill_bonus;
extern int tp_kill_min_cost;
extern int tp_link_cost;
extern int tp_listen_mlev;
extern int tp_lookup_cost;
extern int tp_max_force_level;
extern int tp_max_instr_count;
extern int tp_max_loaded_objs;
extern int tp_max_ml4_preempt_count;
extern int tp_max_ml4_nested_interp_loop_count;
extern int tp_max_nested_interp_loop_count;
extern int tp_max_plyr_processes;
extern int tp_max_process_limit;
extern int tp_max_object_endowment;
extern int tp_max_output;
extern int tp_max_pennies;
extern int tp_mcp_muf_mlev;
extern int tp_movepennies_muf_mlev;
extern int tp_mpi_max_commands;
extern int tp_object_cost;
extern int tp_pause_min;
extern int tp_pennies_muf_mlev;
extern int tp_penny_rate;
extern int tp_player_name_limit;
extern int tp_playermax_limit;
extern int tp_process_timer_limit;
extern int tp_room_cost;
extern int tp_start_pennies;
extern int tp_userlog_mlev;

struct tune_val_entry {
    const char *group;
    const char *name;
    int *val;
    int readmlev;
    int writemlev;
    const char *label;
    char *module;
    int isdefault;
    const int defaultval;
};

extern struct tune_val_entry tune_val_list[];

extern dbref tp_default_room_parent;
extern dbref tp_lost_and_found;
extern dbref tp_player_start;
extern dbref tp_toad_default_recipient;

struct tune_ref_entry {
    const char *group;
    const char *name;
    int typ;
    dbref *ref;
    int readmlev;
    int writemlev;
    const char *label;
    char *module;
    int isdefault;
    const dbref defaultref;
};

extern struct tune_ref_entry tune_ref_list[];

extern int tp_7bit_other_names;
extern int tp_7bit_thing_names;
extern int tp_allow_home;
extern int tp_autolink_actions;
extern int tp_cipher_server_preference;
extern int tp_compatible_priorities;
extern int tp_consistent_lock_source;
extern int tp_dark_sleepers;
extern int tp_dbdump_warning;
extern int tp_diskbase_propvals;
extern int tp_do_mpi_parsing;
extern int tp_dumpdone_warning;
extern int tp_enable_match_yield;
extern int tp_enable_prefix;
extern int tp_exit_darking;
extern int tp_expanded_debug;
extern int tp_force_mlev1_name_notify;
extern int tp_hostnames;
extern int tp_idleboot;
extern int tp_idle_ping_enable;
extern int tp_ignore_bidirectional;
extern int tp_ignore_support;
extern int tp_listeners;
extern int tp_listeners_env;
extern int tp_listeners_obj;
extern int tp_lock_envcheck;
extern int tp_log_commands;
extern int tp_log_failed_commands;
extern int tp_log_interactive;
extern int tp_log_programs;
extern int tp_m3_huh;
extern int tp_muf_comments_strict;
extern int tp_muf_string_math;
extern int tp_optimize_muf;
extern int tp_periodic_program_purge;
extern int tp_playermax;
extern int tp_pname_history_reporting;
extern int tp_quiet_moves;
extern int tp_realms_control;
extern int tp_recognize_null_command;
extern int tp_registration;
extern int tp_restrict_kill;
extern int tp_secure_teleport;
extern int tp_secure_who;
extern int tp_show_legacy_props;
extern int tp_starttls_allow;
extern int tp_strict_god_priv;
extern int tp_teleport_to_player;
extern int tp_thing_darking;
extern int tp_thing_movement;
extern int tp_toad_recycle;
extern int tp_verbose_clone;
extern int tp_verbose_examine;
extern int tp_who_hides_dark;
extern int tp_wiz_vehicles;
extern int tp_zombies;

struct tune_bool_entry {
    const char *group;
    const char *name;
    int *boolval;
    int readmlev;
    int writemlev;
    const char *label;
    char *module;
    int isdefault;
    const int defaultbool;
};

extern struct tune_bool_entry tune_bool_list[];

void set_flags_from_tunestr(dbref obj, const char *flags);
int tune_count_parms(void);
void tune_freeparms(void);
const char *tune_get_parmstring(const char *name, int mlev);
void tune_load_parms_defaults(void);
void tune_load_parms_from_file(FILE * f, dbref player, int cnt);
stk_array *tune_parms_array(const char *pattern, int mlev, int pinned);
void tune_save_parms_to_file(FILE * f);
int tune_setparm(dbref player, const char *parmname, const char *val, int security);

#endif				/* _TUNE_H */
