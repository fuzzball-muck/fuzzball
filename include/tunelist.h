#ifndef TUNELIST_H
#define TUNELIST_H

#include <stdbool.h>
#include <stdio.h>

#include "fbmuck.h"

extern const char *str_tunetype[];
const char *str_tunetype[] = {
    "(str)", "(time)", "(int)", "(ref)", "(bool)"
};

/* Specify the same default values in the pointer and in the lists of tune_*_entry.
   Default values here will be used if the tunable isn't found, default values in the lists of tune_*_entry
   are used when resetting to default via '%tunable_name'. */

bool        tp_7bit_other_names;
bool        tp_7bit_thing_names;
int         tp_addpennies_muf_mlev;
int         tp_aging_time;
bool        tp_allow_listeners;
bool        tp_allow_listeners_env;
bool        tp_allow_listeners_obj;
bool        tp_allow_zombies;
bool        tp_autolink_actions;
const char *tp_autolook_cmd;
int         tp_clean_interval;
int         tp_cmd_log_threshold_msec;
bool        tp_cmd_only_overrides;
int         tp_command_burst_size;
int         tp_command_time_msec;
int         tp_commands_per_time;
bool        tp_compatible_priorities;
const char *tp_connect_fail_mesg;
bool        tp_consistent_lock_source;
const char *tp_cpennies;
const char *tp_cpenny;
const char *tp_create_fail_mesg;
bool        tp_dark_sleepers;
bool        tp_dbdump_warning;
dbref       tp_default_room_parent;
const char *tp_description_default;
bool        tp_diskbase_propvals;
bool        tp_do_mpi_parsing;
int         tp_dump_interval;
int         tp_dump_warntime;
const char *tp_dumpdone_mesg;
bool        tp_dumpdone_warning;
const char *tp_dumping_mesg;
const char *tp_dumpwarn_mesg;
bool        tp_enable_home;
bool        tp_enable_prefix;
int         tp_exit_cost;
bool        tp_exit_darking;
bool        tp_expanded_debug_trace;
const char *tp_file_connection_help;
const char *tp_file_credits;
const char *tp_file_editor_help;
const char *tp_file_help;
const char *tp_file_help_dir;
const char *tp_file_info_dir;
const char *tp_file_log_cmd_times;
const char *tp_file_log_commands;
const char *tp_file_log_gripes;
const char *tp_file_log_malloc;
const char *tp_file_log_muf_errors;
const char *tp_file_log_programs;
const char *tp_file_log_sanfix;
const char *tp_file_log_sanity;
const char *tp_file_log_status;
const char *tp_file_log_stderr;
const char *tp_file_log_stdout;
const char *tp_file_log_user;
const char *tp_file_man;
const char *tp_file_man_dir;
const char *tp_file_motd;
const char *tp_file_mpihelp;
const char *tp_file_mpihelp_dir;
const char *tp_file_news;
const char *tp_file_news_dir;
const char *tp_file_welcome_screen;
bool        tp_force_mlev1_name_notify;
int         tp_free_frames_pool;
const char *tp_gender_prop;
const char *tp_huh_mesg;
const char *tp_idle_boot_mesg;
bool        tp_idle_ping_enable;
int         tp_idle_ping_time;
bool        tp_idleboot;
bool        tp_ignore_bidirectional;
bool        tp_ignore_support;
int         tp_instr_slice;
const char *tp_leave_mesg;
int         tp_link_cost;
int         tp_listen_mlev;
bool        tp_lock_envcheck;
bool        tp_log_commands;
bool        tp_log_failed_commands;
bool        tp_log_interactive;
bool        tp_log_programs;
int         tp_lookup_cost;
dbref       tp_lost_and_found;
bool        tp_m3_huh;
int         tp_max_force_level;
int         tp_max_instr_count;
int         tp_max_loaded_objs;
int         tp_max_ml4_nested_interp_loop_count;
int         tp_max_ml4_preempt_count;
int         tp_max_nested_interp_loop_count;
int         tp_max_object_endowment;
int         tp_max_output;
int         tp_max_pennies;
int         tp_max_plyr_processes;
int         tp_max_process_limit;
int         tp_maxidle;
int         tp_mcp_muf_mlev;
int         tp_movepennies_muf_mlev;
bool        tp_mpi_continue_after_logout;
int         tp_mpi_max_commands;
const char *tp_muckname;
bool        tp_muf_comments_strict;
const char *tp_new_program_flags;
int         tp_object_cost;
bool        tp_optimize_muf;
int         tp_pause_min;
const char *tp_pcreate_flags;
const char *tp_pennies;
int         tp_pennies_muf_mlev;
const char *tp_penny;
int         tp_penny_rate;
bool        tp_periodic_program_purge;
int         tp_player_name_limit;
dbref       tp_player_start;
bool        tp_playermax;
const char *tp_playermax_bootmesg;
int         tp_playermax_limit;
const char *tp_playermax_warnmesg;
bool        tp_pname_history_reporting;
int         tp_pname_history_threshold;
int         tp_process_timer_limit;
bool        tp_quiet_moves;
bool        tp_realms_control;
bool        tp_recognize_null_command;
const char *tp_register_mesg;
bool        tp_registration;
const char *tp_reserved_names;
const char *tp_reserved_player_names;
int         tp_room_cost;
bool        tp_secure_teleport;
bool        tp_secure_thing_movement;
bool        tp_secure_who;
bool        tp_server_cipher_preference;
bool        tp_show_legacy_props;
const char *tp_ssl_cert_file;
const char *tp_ssl_cipher_preference_list;
const char *tp_ssl_key_file;
const char *tp_ssl_keyfile_passwd;
const char *tp_ssl_min_protocol_version;
int         tp_start_pennies;
bool        tp_starttls_allow;
bool        tp_strict_god_priv;
bool        tp_tab_input_replaced_with_space;
bool        tp_teleport_to_player;
bool        tp_thing_darking;
dbref       tp_toad_default_recipient;
bool        tp_toad_recycle;
bool        tp_use_hostnames;
int         tp_userlog_mlev;
bool        tp_verbose_clone;
bool        tp_verbose_examine;
bool        tp_who_hides_dark;
bool        tp_wiz_vehicles;

struct tune_entry tune_list[] = {
    { "7bit_other_names", "Limit exit/room/muf names to 7-bit characters", "Charset", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_7bit_other_names, MLEV_WIZARD, MLEV_WIZARD, true },
    { "7bit_thing_names", "Limit thing names to 7-bit characters", "Charset", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_7bit_thing_names, MLEV_WIZARD, MLEV_WIZARD, true },
    { "addpennies_muf_mlev", "Mucker Level required to create/destroy pennies", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=2,
        .currentval.n=&tp_addpennies_muf_mlev, 0, MLEV_WIZARD, true },
    { "aging_time", "When to considered an object old and unused", "Database", "", TP_TYPE_TIMESPAN,
        .defaultval.t=7776000,
        .currentval.t=&tp_aging_time, 0, MLEV_WIZARD, true },
    { "allow_listeners", "Allow programs to listen to player output", "Listeners", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_allow_listeners, 0, MLEV_WIZARD, true },
    { "allow_listeners_env", "Allow listeners down environment", "Listeners", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_allow_listeners_env, 0, MLEV_WIZARD, true },
    { "allow_listeners_obj", "Allow objects to be listeners", "Listeners", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_allow_listeners_obj, 0, MLEV_WIZARD, true },
    { "allow_zombies", "Enable Zombie things to relay what they hear", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_allow_zombies, 0, MLEV_WIZARD, true },
    { "autolink_actions", "Automatically link @actions to NIL", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_autolink_actions, 0, MLEV_WIZARD, true },
    { "autolook_cmd", "Room entry look command", "Commands", "", TP_TYPE_STRING,
        .defaultval.s="look",
        .currentval.s=&tp_autolook_cmd, 0, MLEV_WIZARD, true },
    { "clean_interval", "Interval between memory/object cleanups", "Tuning", "", TP_TYPE_TIMESPAN,
        .defaultval.t=900,
        .currentval.t=&tp_clean_interval, 0, MLEV_WIZARD, true },
    { "cmd_log_threshold_msec", "Log commands that take longer than X millisecs", "Logging", "", TP_TYPE_INTEGER,
        .defaultval.n=1000,
        .currentval.n=&tp_cmd_log_threshold_msec, 0, MLEV_WIZARD, true },
    { "cmd_only_overrides", "Disable all built-in commands except wizard !overrides", "Commands", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_cmd_only_overrides, 0, MLEV_WIZARD, true },
    { "command_burst_size", "Max. commands per burst before limiter engages", "Spam Limits", "", TP_TYPE_INTEGER,
        .defaultval.n=500,
        .currentval.n=&tp_command_burst_size, 0, MLEV_WIZARD, true },
    { "command_time_msec", "Millisecs per spam limiter time period", "Spam Limits", "", TP_TYPE_INTEGER,
        .defaultval.n=1000,
        .currentval.n=&tp_command_time_msec, 0, MLEV_WIZARD, true },
    { "commands_per_time", "Commands allowed per time period during limit", "Spam Limits", "", TP_TYPE_INTEGER,
        .defaultval.n=2,
        .currentval.n=&tp_commands_per_time, 0, MLEV_WIZARD, true },
    { "compatible_priorities", "Use legacy exit priority levels on things", "Database", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_compatible_priorities, 0, MLEV_WIZARD, true },
    { "connect_fail_mesg", "Failed player connect message", "Connecting", "", TP_TYPE_STRING,
        .defaultval.s="Either that player does not exist, or has a different password.",
        .currentval.s=&tp_connect_fail_mesg, 0, MLEV_WIZARD, true, true },
    { "consistent_lock_source", "Maintain trigger as lock source in TESTLOCK", "MUF", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_consistent_lock_source, 0, MLEV_WIZARD, true },
    { "cpennies", "Currency name, capitalized, plural", "Currency", "", TP_TYPE_STRING,
        .defaultval.s="Pennies",
        .currentval.s=&tp_cpennies, 0, MLEV_WIZARD, true },
    { "cpenny", "Currency name, capitalized", "Currency", "", TP_TYPE_STRING,
        .defaultval.s="Penny",
        .currentval.s=&tp_cpenny, 0, MLEV_WIZARD, true },
    { "create_fail_mesg", "Failed player create message", "Connecting", "", TP_TYPE_STRING,
        .defaultval.s="Either there is already a player with that name, or that name is illegal.",
        .currentval.s=&tp_create_fail_mesg, 0, MLEV_WIZARD, true, true },
    { "dark_sleepers", "Make sleeping players dark", "Dark", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_dark_sleepers, 0, MLEV_WIZARD, true },
    { "dbdump_warning", "Enable warnings for upcoming database dumps", "DB Dumps", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_dbdump_warning, 0, MLEV_WIZARD, true },
    { "default_room_parent", "Place to parent new rooms to", "Database", "", TP_TYPE_DBREF,
        .defaultval.d=GLOBAL_ENVIRONMENT,
        .currentval.d=&tp_default_room_parent, 0, MLEV_WIZARD, true, false, TYPE_ROOM },
    { "description_default", "Default description", "Misc", "", TP_TYPE_STRING,
        .defaultval.s="You see nothing special.",
        .currentval.s=&tp_description_default, 0, MLEV_WIZARD, true },
    { "diskbase_propvals", "Enable property value diskbasing (req. restart)", "DB Dumps", "DISKBASE", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_diskbase_propvals, 0, MLEV_WIZARD, true },
    { "do_mpi_parsing", "Parse MPI strings in messages", "MPI", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_do_mpi_parsing, 0, MLEV_WIZARD, true },
    { "dump_interval", "Interval between dumps", "DB Dumps", "", TP_TYPE_TIMESPAN,
        .defaultval.t=14400,
        .currentval.t=&tp_dump_interval, 0, MLEV_WIZARD, true },
    { "dump_warntime", "Interval between warning and dump", "DB Dumps", "", TP_TYPE_TIMESPAN,
        .defaultval.t=120,
        .currentval.t=&tp_dump_warntime, 0, MLEV_WIZARD, true },
    { "dumpdone_mesg", "Database dump finished message", "DB Dumps", "", TP_TYPE_STRING,
        .defaultval.s="## Save complete. ##",
        .currentval.s=&tp_dumpdone_mesg, 0, MLEV_WIZARD, true, true },
    { "dumpdone_warning", "Notify when database dump complete", "DB Dumps", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_dumpdone_warning, 0, MLEV_WIZARD, true },
    { "dumping_mesg", "Database dump started message", "DB Dumps", "", TP_TYPE_STRING,
        .defaultval.s="## Pausing to save database. This may take a while. ##",
        .currentval.s=&tp_dumping_mesg, 0, MLEV_WIZARD, true, true },
    { "dumpwarn_mesg", "Database dump finished message", "DB Dumps", "", TP_TYPE_STRING,
        .defaultval.s="## Game will pause to save the database in a few minutes. ##",
        .currentval.s=&tp_dumpwarn_mesg, 0, MLEV_WIZARD, true, true },
    { "enable_home", "Enable 'home' command", "Commands", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_enable_home, MLEV_WIZARD, MLEV_WIZARD, true },
    { "enable_prefix", "Enable prefix actions", "Commands", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_enable_prefix, MLEV_WIZARD, MLEV_WIZARD, true },
    { "exit_cost", "Cost to create an exit", "Costs", "", TP_TYPE_INTEGER,
        .defaultval.n=1,
        .currentval.n=&tp_exit_cost, 0, MLEV_WIZARD, true },
    { "exit_darking", "Allow players to set exits dark", "Dark", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_exit_darking, 0, MLEV_WIZARD, true },
    { "expanded_debug_trace", "MUF debug trace shows array contents", "MUF", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_expanded_debug_trace, 0, MLEV_WIZARD, true },
    { "file_connection_help", "'help' before login", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/connect-help.txt",
        .currentval.s=&tp_file_connection_help, MLEV_WIZARD, MLEV_GOD, true },
    { "file_credits", "Acknowledgements", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/credits.txt",
        .currentval.s=&tp_file_credits, MLEV_WIZARD, MLEV_GOD, true },
    { "file_editor_help", "Editor help", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/edit-help.txt",
        .currentval.s=&tp_file_editor_help, MLEV_WIZARD, MLEV_GOD, true },
    { "file_help", "'help' main content", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/help.txt",
        .currentval.s=&tp_file_help, MLEV_WIZARD, MLEV_GOD, true },
    { "file_help_dir", "'help' topic directory", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/help",
        .currentval.s=&tp_file_help_dir, MLEV_WIZARD, MLEV_GOD, true },
    { "file_info_dir", "'info' topic directory", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/info/",
        .currentval.s=&tp_file_info_dir, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_cmd_times", "Command times", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/cmd-times",
        .currentval.s=&tp_file_log_cmd_times, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_commands", "Player commands", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/commands",
        .currentval.s=&tp_file_log_commands, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_gripes", "Player gripes", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/gripes",
        .currentval.s=&tp_file_log_gripes, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_malloc", "Memory allocations", "Files", "MEMPROF", TP_TYPE_STRING,
        .defaultval.s="logs/malloc",
        .currentval.s=&tp_file_log_malloc, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_muf_errors", "MUF compile errors and warnings", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/muf-errors",
        .currentval.s=&tp_file_log_muf_errors, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_programs", "Text of changed programs", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/programs",
        .currentval.s=&tp_file_log_programs, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_sanfix", "Database fixes", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/sanfixed",
        .currentval.s=&tp_file_log_sanfix, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_sanity", "Database corruption and errors", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/sanity",
        .currentval.s=&tp_file_log_sanity, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_status", "System errors and stats", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/status",
        .currentval.s=&tp_file_log_status, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_stderr", "Server error redirect", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/fbmuck.err",
        .currentval.s=&tp_file_log_stderr, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_stdout", "Server output redirect", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/fbmuck",
        .currentval.s=&tp_file_log_stdout, MLEV_WIZARD, MLEV_GOD, true },
    { "file_log_user", "MUF-writable messages", "Files", "", TP_TYPE_STRING,
        .defaultval.s="logs/user",
        .currentval.s=&tp_file_log_user, MLEV_WIZARD, MLEV_GOD, true },
    { "file_man", "'man' main content", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/man.txt",
        .currentval.s=&tp_file_man, MLEV_WIZARD, MLEV_GOD, true },
    { "file_man_dir", "'man' topic directory", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/man",
        .currentval.s=&tp_file_man_dir, MLEV_WIZARD, MLEV_GOD, true },
    { "file_motd", "Message of the day", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/motd.txt",
        .currentval.s=&tp_file_motd, MLEV_WIZARD, MLEV_GOD, true },
    { "file_mpihelp", "'mpi' main content", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/mpihelp.txt",
        .currentval.s=&tp_file_mpihelp, MLEV_WIZARD, MLEV_GOD, true },
    { "file_mpihelp_dir", "'mpi' topic directory", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/mpihelp",
        .currentval.s=&tp_file_mpihelp_dir, MLEV_WIZARD, MLEV_GOD, true },
    { "file_news", "'news' main content", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/news.txt",
        .currentval.s=&tp_file_news, MLEV_WIZARD, MLEV_GOD, true },
    { "file_news_dir", "'news' topic directory", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/news",
        .currentval.s=&tp_file_news_dir, MLEV_WIZARD, MLEV_GOD, true },
    { "file_welcome_screen", "Opening screen", "Files", "", TP_TYPE_STRING,
        .defaultval.s="data/welcome.txt",
        .currentval.s=&tp_file_welcome_screen, MLEV_WIZARD, MLEV_GOD, true },
    { "force_mlev1_name_notify", "MUF notify prepends username for ML1 programs", "MUF", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_force_mlev1_name_notify, 0, MLEV_WIZARD, true },
    { "free_frames_pool", "Size of allocated MUF process frame pool", "Tuning", "", TP_TYPE_INTEGER,
        .defaultval.n=8,
        .currentval.n=&tp_free_frames_pool, 0, MLEV_WIZARD, true },
    { "gender_prop", "Property name used for pronoun substitutions", "Properties", "", TP_TYPE_STRING,
        .defaultval.s="sex",
        .currentval.s=&tp_gender_prop, 0, MLEV_WIZARD, true },
    { "huh_mesg", "Unrecognized command warning", "Misc", "", TP_TYPE_STRING,
        .defaultval.s="Huh?  (Type \"help\" for help.)",
        .currentval.s=&tp_huh_mesg, 0, MLEV_WIZARD, true },
    { "idle_boot_mesg", "Boot message given to users idling out", "Idle Boot", "", TP_TYPE_STRING,
        .defaultval.s="Autodisconnecting for inactivity.",
        .currentval.s=&tp_idle_boot_mesg, 0, MLEV_WIZARD, true },
    { "idle_ping_enable", "Enable server side keepalive pings", "Idle Boot", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_idle_ping_enable, 0, MLEV_WIZARD, true },
    { "idle_ping_time", "Server side keepalive time in seconds", "Idle Boot", "", TP_TYPE_TIMESPAN,
        .defaultval.t=55,
        .currentval.t=&tp_idle_ping_time, 0, MLEV_WIZARD, true },
    { "idleboot", "Enable booting of idle players", "Idle Boot", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_idleboot, 0, MLEV_WIZARD, true },
    { "ignore_bidirectional", "Enable bidirectional ignore", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_ignore_bidirectional, MLEV_MASTER, MLEV_WIZARD, true },
    { "ignore_support", "Enable support for @ignoring players", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_ignore_support, MLEV_MASTER, MLEV_WIZARD, true },
    { "instr_slice", "Max. uninterrupted instructions per timeslice", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=2000,
        .currentval.n=&tp_instr_slice, 0, MLEV_WIZARD, true },
    { "leave_mesg", "Logoff message for QUIT", "Misc", "", TP_TYPE_STRING,
        .defaultval.s="Come back later!",
        .currentval.s=&tp_leave_mesg, 0, MLEV_WIZARD, true },
    { "link_cost", "Cost to link an exit", "Costs", "", TP_TYPE_INTEGER,
        .defaultval.n=1,
        .currentval.n=&tp_link_cost, 0, MLEV_WIZARD, true },
    { "listen_mlev", "Mucker Level required for Listener programs", "Listeners", "", TP_TYPE_INTEGER,
        .defaultval.n=3,
        .currentval.n=&tp_listen_mlev, 0, MLEV_WIZARD, true },
    { "lock_envcheck", "Locks check environment for properties", "Properties", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_lock_envcheck, 0, MLEV_WIZARD, true },
    { "log_commands", "Log player commands", "Logging", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_log_commands, MLEV_WIZARD, MLEV_WIZARD, true },
    { "log_failed_commands", "Log unrecognized commands", "Logging", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_log_failed_commands, MLEV_WIZARD, MLEV_WIZARD, true },
    { "log_interactive", "Log text sent to MUF", "Logging", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_log_interactive, MLEV_WIZARD, MLEV_WIZARD, true },
    { "log_programs", "Log programs every time they are saved", "Logging", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_log_programs, MLEV_WIZARD, MLEV_WIZARD, true },
    { "lookup_cost", "Cost to lookup a player name", "Costs", "", TP_TYPE_INTEGER,
        .defaultval.n=0,
        .currentval.n=&tp_lookup_cost, 0, MLEV_WIZARD, true },
    { "lost_and_found", "Place for things without a home", "Database", "", TP_TYPE_DBREF,
        .defaultval.d=GLOBAL_ENVIRONMENT,
        .currentval.d=&tp_lost_and_found, 0, MLEV_WIZARD, true, false, TYPE_ROOM },
    { "m3_huh", "Enable huh? to call an exit named \"huh?\" and set M3, with full command string", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_m3_huh, MLEV_MASTER, MLEV_WIZARD, true },
    { "max_force_level", "Max. number of forces processed within a command", "Misc", "", TP_TYPE_INTEGER,
        .defaultval.n=1,
        .currentval.n=&tp_max_force_level, MLEV_GOD, MLEV_GOD, true },
    { "max_instr_count", "Max. MUF instruction run length for ML1", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=20000,
        .currentval.n=&tp_max_instr_count, 0, MLEV_WIZARD, true, true },
    { "max_loaded_objs", "Max. percent of proploaded database objects", "Tuning", "DISKBASE", TP_TYPE_INTEGER,
        .defaultval.n=5,
        .currentval.n=&tp_max_loaded_objs, 0, MLEV_WIZARD, true },
    { "max_ml4_nested_interp_loop_count", "Max. MUF preempt interp loop nesting level for ML4 (0 = no limit)", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=0,
        .currentval.n=&tp_max_ml4_nested_interp_loop_count, 0, MLEV_WIZARD, true },
    { "max_ml4_preempt_count", "Max. MUF preempt instruction run length for ML4, (0 = no limit)", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=0,
        .currentval.n=&tp_max_ml4_preempt_count, 0, MLEV_WIZARD, true },
    { "max_nested_interp_loop_count", "Max. MUF preempt interp loop nesting level", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=16,
        .currentval.n=&tp_max_nested_interp_loop_count, 0, MLEV_WIZARD, true },
    { "max_object_endowment", "Max. value of an object", "Costs", "", TP_TYPE_INTEGER,
        .defaultval.n=100,
        .currentval.n=&tp_max_object_endowment, 0, MLEV_WIZARD, true },
    { "max_output", "Max. output buffer size", "Spam Limits", "", TP_TYPE_INTEGER,
        .defaultval.n=131071,
        .currentval.n=&tp_max_output, 0, MLEV_WIZARD, true },
    { "max_pennies", "Max. pennies a player can own", "Currency", "", TP_TYPE_INTEGER,
        .defaultval.n=10000,
        .currentval.n=&tp_max_pennies, 0, MLEV_WIZARD, true },
    { "max_plyr_processes", "Concurrent processes allowed per player", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=32,
        .currentval.n=&tp_max_plyr_processes, 0, MLEV_WIZARD, true },
    { "max_process_limit", "Total concurrent processes allowed on system", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=400,
        .currentval.n=&tp_max_process_limit, 0, MLEV_WIZARD, true },
    { "maxidle", "Maximum idle time before booting", "Idle Boot", "", TP_TYPE_TIMESPAN,
        .defaultval.t=7200,
        .currentval.t=&tp_maxidle, 0, MLEV_WIZARD, true },
    { "mcp_muf_mlev", "Mucker Level required to use MCP", "MUF", "MCP", TP_TYPE_INTEGER,
        .defaultval.n=3,
        .currentval.n=&tp_mcp_muf_mlev, 0, MLEV_WIZARD, true },
    { "movepennies_muf_mlev", "Mucker Level required to move pennies non-destructively", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=2,
        .currentval.n=&tp_movepennies_muf_mlev, 0, MLEV_WIZARD, true },
    { "mpi_continue_after_logout", "Continue executing MPI after logout", "MPI", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_mpi_continue_after_logout, 0, MLEV_WIZARD, true },
    { "mpi_max_commands", "Max. number of uninterruptable MPI commands", "MPI", "", TP_TYPE_INTEGER,
        .defaultval.n=2048,
        .currentval.n=&tp_mpi_max_commands, 0, MLEV_WIZARD, true },
    { "muckname", "Name of the MUCK", "Misc", "", TP_TYPE_STRING,
        .defaultval.s="TygryssMUCK",
        .currentval.s=&tp_muckname, 0, MLEV_WIZARD, true },
    { "muf_comments_strict", "MUF comments are strict and not recursive", "MUF", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_muf_comments_strict, 0, MLEV_WIZARD, true },
    { "new_program_flags", "Initial flags for newly created programs", "Database", "", TP_TYPE_STRING,
        .defaultval.s="",
        .currentval.s=&tp_new_program_flags, 0, MLEV_WIZARD, true, true },
    { "object_cost", "Cost to create an object", "Costs", "", TP_TYPE_INTEGER,
        .defaultval.n=10,
        .currentval.n=&tp_object_cost, 0, MLEV_WIZARD, true },
    { "optimize_muf", "Enable MUF bytecode optimizer", "MUF", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_optimize_muf, 0, MLEV_WIZARD, true },
    { "pause_min", "Min. millisecs between MUF input/output timeslices", "Tuning", "", TP_TYPE_INTEGER,
        .defaultval.n=0,
        .currentval.n=&tp_pause_min, 0, MLEV_WIZARD, true },
    { "pcreate_flags", "Initial flags for newly created players", "Database", "", TP_TYPE_STRING,
        .defaultval.s="B",
        .currentval.s=&tp_pcreate_flags, 0, MLEV_WIZARD, true, true },
    { "pennies", "Currency name, plural", "Currency", "", TP_TYPE_STRING,
        .defaultval.s="pennies",
        .currentval.s=&tp_pennies, 0, MLEV_WIZARD, true },
    { "pennies_muf_mlev", "Mucker Level required to read the value of pennies, settings above 1 disable {money}", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=1,
        .currentval.n=&tp_pennies_muf_mlev, 0, MLEV_WIZARD, true },
    { "penny", "Currency name", "Currency", "", TP_TYPE_STRING,
        .defaultval.s="penny",
        .currentval.s=&tp_penny, 0, MLEV_WIZARD, true },
    { "penny_rate", "Avg. moves between finding currency", "Currency", "", TP_TYPE_INTEGER,
        .defaultval.n=8,
        .currentval.n=&tp_penny_rate, 0, MLEV_WIZARD, true },
    { "periodic_program_purge", "Periodically free unused MUF programs", "Tuning", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_periodic_program_purge, 0, MLEV_WIZARD, true },
    { "player_name_limit", "Limit on player name length", "Commands", "", TP_TYPE_INTEGER,
        .defaultval.n=16,
        .currentval.n=&tp_player_name_limit, 0, MLEV_WIZARD, true },
    { "player_start", "Home where new players start", "Database", "", TP_TYPE_DBREF,
        .defaultval.d=GLOBAL_ENVIRONMENT,
        .currentval.d=&tp_player_start, 0, MLEV_WIZARD, true, false, TYPE_ROOM },
    { "playermax", "Limit number of concurrent players allowed", "Connecting", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_playermax, 0, MLEV_WIZARD, true },
    { "playermax_bootmesg", "Max. players connection error message", "Connecting", "", TP_TYPE_STRING,
        .defaultval.s="Sorry, but there are too many players online.  Please try reconnecting in a few minutes.",
        .currentval.s=&tp_playermax_bootmesg, 0, MLEV_WIZARD, true },
    { "playermax_limit", "Max. player connections allowed", "Connecting", "", TP_TYPE_INTEGER,
        .defaultval.n=56,
        .currentval.n=&tp_playermax_limit, 0, MLEV_WIZARD, true },
    { "playermax_warnmesg", "Max. players connection login warning", "Connecting", "", TP_TYPE_STRING,
        .defaultval.s="You likely won't be able to connect right now, since too many players are online.",
        .currentval.s=&tp_playermax_warnmesg, 0, MLEV_WIZARD, true },
    { "pname_history_reporting", "Report player name change history", "Tuning", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_pname_history_reporting, 0, MLEV_WIZARD, true },
    { "pname_history_threshold", "Length of player name change history", "Tuning", "", TP_TYPE_TIMESPAN,
        .defaultval.t=2592000,
        .currentval.t=&tp_pname_history_threshold, 0, MLEV_WIZARD, true },
    { "process_timer_limit", "Max. timers per process", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=4,
        .currentval.n=&tp_process_timer_limit, 0, MLEV_WIZARD, true },
    { "quiet_moves", "Suppress basic arrive and depart notifications", "Movement", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_quiet_moves, 0, MLEV_WIZARD, true },
    { "realms_control", "Enable support for realm wizzes", "Database", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_realms_control, 0, MLEV_WIZARD, true },
    { "recognize_null_command", "Recognize null command", "Commands", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_recognize_null_command, MLEV_WIZARD, MLEV_WIZARD, true },
    { "register_mesg", "Login registration denied message", "Connecting", "", TP_TYPE_STRING,
        .defaultval.s="Sorry, you can get a character by e-mailing XXXX@machine.net.address with a charname and password.",
        .currentval.s=&tp_register_mesg, 0, MLEV_WIZARD, true },
    { "registration", "Require new players to register manually", "Connecting", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_registration, 0, MLEV_WIZARD, true },
    { "reserved_names", "String-match list of reserved names", "Database", "", TP_TYPE_STRING,
        .defaultval.s="",
        .currentval.s=&tp_reserved_names, 0, MLEV_WIZARD, true, true },
    { "reserved_player_names", "String-match list of reserved player names", "Database", "", TP_TYPE_STRING,
        .defaultval.s="",
        .currentval.s=&tp_reserved_player_names, 0, MLEV_WIZARD, true, true },
    { "room_cost", "Cost to create an room", "Costs", "", TP_TYPE_INTEGER,
        .defaultval.n=10,
        .currentval.n=&tp_room_cost, 0, MLEV_WIZARD, true },
    { "secure_teleport", "Restrict actions to Jump_OK or controlled rooms", "Movement", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_secure_teleport, 0, MLEV_WIZARD, true },
    { "secure_thing_movement", "Moving things act like player", "Movement", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_secure_thing_movement, MLEV_WIZARD, MLEV_WIZARD, true },
    { "secure_who", "Disallow WHO command from login screen and programs", "WHO", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_secure_who, 0, MLEV_WIZARD, true },
    { "server_cipher_preference", "Honor server cipher preference order over client's", "SSL", "SSL", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_server_cipher_preference, MLEV_GOD, MLEV_GOD, true },
    { "show_legacy_props", "Examining objects lists legacy props", "Properties", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_show_legacy_props, 0, MLEV_WIZARD, true },
    { "ssl_cert_file", "Path to SSL certificate .pem", "SSL", "SSL", TP_TYPE_STRING,
        .defaultval.s="data/server.pem",
        .currentval.s=&tp_ssl_cert_file, MLEV_GOD, MLEV_GOD, true },
    { "ssl_cipher_preference_list", "Allowed OpenSSL cipher list", "SSL", "SSL", TP_TYPE_STRING,
        .defaultval.s="ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA",
        .currentval.s=&tp_ssl_cipher_preference_list, MLEV_GOD, MLEV_GOD, true },
    { "ssl_key_file", "Path to SSL private key .pem", "SSL", "SSL", TP_TYPE_STRING,
        .defaultval.s="data/server.pem",
        .currentval.s=&tp_ssl_key_file, MLEV_GOD, MLEV_GOD, true },
    { "ssl_keyfile_passwd", "Password for SSL private key file", "SSL", "SSL", TP_TYPE_STRING,
        .defaultval.s="",
        .currentval.s=&tp_ssl_keyfile_passwd, MLEV_GOD, MLEV_GOD, true, true },
    { "ssl_min_protocol_version", "Min. allowed SSL protocol version for clients", "SSL", "SSL", TP_TYPE_STRING,
        .defaultval.s="None",
        .currentval.s=&tp_ssl_min_protocol_version, MLEV_GOD, MLEV_GOD, true },
    { "start_pennies", "Player starting currency count", "Currency", "", TP_TYPE_INTEGER,
        .defaultval.n=50,
        .currentval.n=&tp_start_pennies, 0, MLEV_WIZARD, true },
    { "starttls_allow", "Enable TELNET STARTTLS encryption on plaintext port", "Encryption", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_starttls_allow, MLEV_MASTER, MLEV_WIZARD, true },
    { "strict_god_priv", "Only God can touch God's objects", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_strict_god_priv, MLEV_GOD, MLEV_GOD, true },
    { "tab_input_replaced_with_space", "Change tab to space when processing input", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_tab_input_replaced_with_space, 0, MLEV_WIZARD, true },
    { "teleport_to_player", "Allow using exits linked to players", "Movement", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_teleport_to_player, 0, MLEV_WIZARD, true },
    { "thing_darking", "Allow players to set things dark", "Dark", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_thing_darking, 0, MLEV_WIZARD, true },
    { "toad_default_recipient", "Default owner for @toaded player's things", "Database", "", TP_TYPE_DBREF,
        .defaultval.d=GOD,
        .currentval.d=&tp_toad_default_recipient, 0, MLEV_WIZARD, true, false, TYPE_PLAYER },
    { "toad_recycle", "Recycle newly-created toads", "Database", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_toad_recycle, 0, MLEV_WIZARD, true },
    { "use_hostnames", "Resolve IP addresses into hostnames", "WHO", "RESOLVER", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_use_hostnames, 0, MLEV_WIZARD, true },
    { "userlog_mlev", "Mucker Level required to write to userlog", "MUF", "", TP_TYPE_INTEGER,
        .defaultval.n=3,
        .currentval.n=&tp_userlog_mlev, 0, MLEV_WIZARD, true },
    { "verbose_clone", "Show more information when using @clone command", "Commands", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_verbose_clone, MLEV_WIZARD, MLEV_WIZARD, true },
    { "verbose_examine", "Show more information when using examine command", "Commands", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_verbose_examine, MLEV_WIZARD, MLEV_WIZARD, true },
    { "who_hides_dark", "Hide dark players from WHO list", "Dark", "", TP_TYPE_BOOLEAN,
        .defaultval.b=true,
        .currentval.b=&tp_who_hides_dark, MLEV_WIZARD, MLEV_WIZARD, true },
    { "wiz_vehicles", "Only let Wizards set vehicle bits", "Misc", "", TP_TYPE_BOOLEAN,
        .defaultval.b=false,
        .currentval.b=&tp_wiz_vehicles, 0, MLEV_WIZARD, true },
    { 0 }
};

#endif /* !TUNELIST_H */
