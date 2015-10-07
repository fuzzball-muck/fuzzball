#ifndef _TUNE_H
#define _TUNE_H

/* Ensure that 'dbref' type gets properly typedef'd */
#include "db.h"

#define TUNESET_SUCCESS 0 /* success			*/
#define TUNESET_UNKNOWN 1 /* unrecognized sysparm	*/
#define TUNESET_SYNTAX  2 /* bad value syntax		*/
#define TUNESET_BADVAL  3 /* bad value			*/
#define TUNESET_DENIED	4 /* mucker level too low    	*/

/* strings */
extern const char *tp_dumpwarn_mesg;
extern const char *tp_deltawarn_mesg;
extern const char *tp_dumpdeltas_mesg;
extern const char *tp_dumping_mesg;
extern const char *tp_dumpdone_mesg;

extern const char *tp_penny;
extern const char *tp_pennies;
extern const char *tp_cpenny;
extern const char *tp_cpennies;

extern const char *tp_muckname;

extern const char *tp_huh_mesg;
extern const char *tp_leave_mesg;
extern const char *tp_idle_mesg;
extern const char *tp_register_mesg;
extern const char *tp_playermax_warnmesg;
extern const char *tp_playermax_bootmesg;
extern const char *tp_autolook_cmd;

extern const char *tp_proplist_counter_fmt;
extern const char *tp_proplist_entry_fmt;
extern const char *tp_ssl_keyfile_passwd;
extern const char *tp_ssl_cipher_preference_list;

extern const char *tp_pcreate_flags;
extern const char *tp_reserved_names;
extern const char *tp_reserved_player_names;



/* times */
extern int tp_dump_interval;
extern int tp_dump_warntime;
extern int tp_monolithic_interval;
extern int tp_clean_interval;
extern int tp_aging_time;
extern int tp_maxidle;
extern int tp_idle_ping_time;


/* integers */
extern int tp_max_object_endowment;
extern int tp_object_cost;
extern int tp_exit_cost;
extern int tp_link_cost;
extern int tp_room_cost;
extern int tp_lookup_cost;
extern int tp_max_pennies;
extern int tp_penny_rate;
extern int tp_start_pennies;

extern int tp_kill_base_cost;
extern int tp_kill_min_cost;
extern int tp_kill_bonus;

extern int tp_command_burst_size;
extern int tp_commands_per_time;
extern int tp_command_time_msec;
extern int tp_max_output;

extern int tp_max_delta_objs;
extern int tp_max_loaded_objs;
extern int tp_max_process_limit;
extern int tp_max_plyr_processes;
extern int tp_max_instr_count;
extern int tp_max_ml4_preempt_count;
extern int tp_instr_slice;
extern int tp_mpi_max_commands;
extern int tp_pause_min;
extern int tp_free_frames_pool;
extern int tp_listen_mlev;
extern int tp_playermax_limit;
extern int tp_process_timer_limit;
extern int tp_cmd_log_threshold_msec;
extern int tp_mcp_muf_mlev;
extern int tp_movepennies_muf_mlev;
extern int tp_addpennies_muf_mlev;
extern int tp_pennies_muf_mlev;
extern int tp_userlog_mlev;
extern int tp_max_force_level;


/* dbrefs */
extern dbref tp_player_start;
extern dbref tp_default_room_parent;
extern dbref tp_toad_default_recipient;
extern dbref tp_lost_and_found;


/* booleans */
extern int tp_hostnames;
extern int tp_log_commands;
extern int tp_log_failed_commands;
extern int tp_log_programs;
extern int tp_log_interactive;
extern int tp_dbdump_warning;
extern int tp_dumpdone_warning;
extern int tp_deltadump_warning;
extern int tp_periodic_program_purge;
extern int tp_secure_who;
extern int tp_who_doing;
extern int tp_realms_control;
extern int tp_listeners;
extern int tp_listeners_obj;
extern int tp_listeners_env;
extern int tp_zombies;
extern int tp_wiz_vehicles;
extern int tp_force_mlev1_name_notify;
extern int tp_restrict_kill;
extern int tp_registration;
extern int tp_teleport_to_player;
extern int tp_secure_teleport;
extern int tp_exit_darking;
extern int tp_thing_darking;
extern int tp_dark_sleepers;
extern int tp_who_hides_dark;
extern int tp_compatible_priorities;
extern int tp_do_mpi_parsing;
extern int tp_look_propqueues;
extern int tp_lock_envcheck;
extern int tp_diskbase_propvals;
extern int tp_idleboot;
extern int tp_playermax;
extern int tp_allow_home;
extern int tp_enable_prefix;
extern int tp_enable_match_yield;
extern int tp_idle_ping_enable;
extern int tp_thing_movement;
extern int tp_expanded_debug;
extern int tp_proplist_int_counter;
extern int tp_lazy_mpi_istype_perm;
extern int tp_optimize_muf;
extern int tp_ignore_support;
extern int tp_ignore_bidirectional;
extern int tp_verbose_clone;
extern int tp_muf_comments_strict;
extern int tp_m3_huh;
extern int tp_starttls_allow;
extern int tp_cipher_server_preference;
extern int tp_7bit_thing_names;
extern int tp_7bit_other_names;
extern int tp_idle_ping_enable;
extern int tp_recognize_null_command;
extern int tp_strict_god_priv;

extern int tune_count_parms(void);
extern void tune_load_parms_from_file(FILE * f, dbref player, int cnt);
extern void tune_save_parms_to_file(FILE * f);
extern int tune_setparm(const char *parmname, const char *val, int security);
extern stk_array *tune_parms_array(const char* pattern, int mlev);
const char *tune_get_parmstring(const char *name, int mlev);

#endif /* _TUNE_H */
