#ifndef _TUNELIST_H
#define _TUNELIST_H

#include "defaults.h"
#include "fbtime.h"
#include "game.h"

/* Specify the same default values in the pointer and in the lists of tune_*_entry.
   Default values here will be used if the tunable isn't found, default values in the lists of tune_*_entry
   are used when resetting to default via '%tunable_name'. */

const char *tp_autolook_cmd;
const char *tp_cpennies;
const char *tp_cpenny;
const char *tp_dumpdone_mesg;
const char *tp_dumping_mesg;
const char *tp_dumpwarn_mesg;
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
const char *tp_file_mpihelp;
const char *tp_file_mpihelp_dir;
const char *tp_file_motd;
const char *tp_file_news;
const char *tp_file_news_dir;
const char *tp_file_sysparms;
const char *tp_file_welcome_screen;
const char *tp_gender_prop;
const char *tp_huh_mesg;
const char *tp_idle_mesg;
const char *tp_leave_mesg;
const char *tp_muckname;
const char *tp_pcreate_flags;
const char *tp_pennies;
const char *tp_penny;
const char *tp_playermax_bootmesg;
const char *tp_playermax_warnmesg;
const char *tp_register_mesg;
const char *tp_reserved_names;
const char *tp_reserved_player_names;
const char *tp_ssl_cert_file;
const char *tp_ssl_key_file;
const char *tp_ssl_keyfile_passwd;
const char *tp_ssl_cipher_preference_list;
const char *tp_ssl_min_protocol_version;

struct tune_str_entry tune_str_list[] = {
    {"Commands", "autolook_cmd", &tp_autolook_cmd, 0, MLEV_WIZARD, "Room entry look command",
     "", 0, 1, AUTOLOOK_CMD},
    {"Currency", "cpennies", &tp_cpennies, 0, MLEV_WIZARD,
     "Currency name, capitalized, plural", "", 0, 1, CPENNIES},
    {"Currency", "cpenny", &tp_cpenny, 0, MLEV_WIZARD, "Currency name, capitalized", "", 0, 1,
     CPENNY},
    {"Currency", "pennies", &tp_pennies, 0, MLEV_WIZARD, "Currency name, plural", "", 0, 1,
     PENNIES},
    {"Currency", "penny", &tp_penny, 0, MLEV_WIZARD, "Currency name", "", 0, 1, PENNY},
    {"DB Dumps", "dumpdone_mesg", &tp_dumpdone_mesg, 0, MLEV_WIZARD,
     "Database dump finished message", "", 1, 1, DUMPDONE_MESG},
    {"DB Dumps", "dumping_mesg", &tp_dumping_mesg, 0, MLEV_WIZARD,
     "Database dump started message", "", 1, 1, DUMPING_MESG},
    {"DB Dumps", "dumpwarn_mesg", &tp_dumpwarn_mesg, 0, MLEV_WIZARD,
     "Database dump warning message", "", 1, 1, DUMPWARN_MESG},
    {"Files", "file_connection_help", &tp_file_connection_help, MLEV_WIZARD, MLEV_GOD,
     "'help' before login", "", 0, 1, CONHELP_FILE},
    {"Files", "file_credits", &tp_file_credits, MLEV_WIZARD, MLEV_GOD,
     "Acknowledgements", "", 0, 1, CREDITS_FILE},
    {"Files", "file_editor_help", &tp_file_editor_help, MLEV_WIZARD, MLEV_GOD,
     "Editor help", "", 0, 1, EDITOR_HELP_FILE},
    {"Files", "file_help", &tp_file_help, MLEV_WIZARD, MLEV_GOD,
     "'help' main content", "", 0, 1, HELP_FILE},
    {"Files", "file_help_dir", &tp_file_help_dir, MLEV_WIZARD, MLEV_GOD,
     "'help' topic directory", "", 0, 1, HELP_DIR},
    {"Files", "file_info_dir", &tp_file_info_dir, MLEV_WIZARD, MLEV_GOD,
     "'info' topic directory", "", 0, 1, INFO_DIR},
    {"Files", "file_log_cmd_times", &tp_file_log_cmd_times, MLEV_WIZARD, MLEV_GOD,
     "Command times", "", 0, 1, LOG_CMD_TIMES},
    {"Files", "file_log_commands", &tp_file_log_commands, MLEV_WIZARD, MLEV_GOD,
     "Player commands", "", 0, 1, COMMAND_LOG},
    {"Files", "file_log_gripes", &tp_file_log_gripes, MLEV_WIZARD, MLEV_GOD,
     "Player gripes", "", 0, 1, LOG_GRIPE},
    {"Files", "file_log_malloc", &tp_file_log_malloc, MLEV_WIZARD, MLEV_GOD,
     "Memory allocations", "", 0, 1, LOG_MALLOC},
    {"Files", "file_log_muf_errors", &tp_file_log_muf_errors, MLEV_WIZARD, MLEV_GOD,
     "MUF compile errors and warnings", "", 0, 1, LOG_MUF},
    {"Files", "file_log_programs", &tp_file_log_programs, MLEV_WIZARD, MLEV_GOD,
     "Text of changed programs", "", 0, 1, PROGRAM_LOG},
    {"Files", "file_log_sanfix", &tp_file_log_sanfix, MLEV_WIZARD, MLEV_GOD,
     "Database fixes", "", 0, 1, LOG_SANFIX},
    {"Files", "file_log_sanity", &tp_file_log_sanity, MLEV_WIZARD, MLEV_GOD,
     "Database corruption and errors", "", 0, 1, LOG_SANITY},
    {"Files", "file_log_status", &tp_file_log_status, MLEV_WIZARD, MLEV_GOD,
     "System errors and stats", "", 0, 1, LOG_STATUS},
    {"Files", "file_log_stderr", &tp_file_log_stderr, MLEV_WIZARD, MLEV_GOD,
     "Server error redirect", "", 0, 1, LOG_ERR_FILE},
    {"Files", "file_log_stdout", &tp_file_log_stdout, MLEV_WIZARD, MLEV_GOD,
     "Server output redirect", "", 0, 1, LOG_FILE},
    {"Files", "file_log_user", &tp_file_log_user, MLEV_WIZARD, MLEV_GOD,
     "MUF-writable messages", "", 0, 1, USER_LOG},
    {"Files", "file_man", &tp_file_man, MLEV_WIZARD, MLEV_GOD,
     "'man' main content", "", 0, 1, MAN_FILE},
    {"Files", "file_man_dir", &tp_file_man_dir, MLEV_WIZARD, MLEV_GOD,
     "'man' topic directory", "", 0, 1, MAN_DIR},
    {"Files", "file_motd", &tp_file_motd, MLEV_WIZARD, MLEV_GOD,
     "Message of the day", "", 0, 1, MOTD_FILE},
    {"Files", "file_mpihelp", &tp_file_mpihelp, MLEV_WIZARD, MLEV_GOD,
     "'mpi' main content", "", 0, 1, MPI_FILE},
    {"Files", "file_mpihelp_dir", &tp_file_mpihelp_dir, MLEV_WIZARD, MLEV_GOD,
     "'mpi' topic directory", "", 0, 1, MPI_DIR},
    {"Files", "file_news", &tp_file_news, MLEV_WIZARD, MLEV_GOD,
     "'news' main content", "", 0, 1, NEWS_FILE},
    {"Files", "file_news_dir", &tp_file_news_dir, MLEV_WIZARD, MLEV_GOD,
     "'news' topic directory", "", 0, 1, NEWS_DIR},
    {"Files", "file_sysparms", &tp_file_sysparms, MLEV_WIZARD, MLEV_GOD,
     "System parameters", "", 0, 1, PARM_FILE},
    {"Files", "file_welcome_screen", &tp_file_welcome_screen, MLEV_WIZARD, MLEV_GOD,
     "Opening screen", "", 0, 1, WELC_FILE},
    {"Idle Boot", "idle_boot_mesg", &tp_idle_mesg, 0, MLEV_WIZARD,
     "Boot message given to users idling out", "", 0, 1, IDLEBOOT_MESSAGE},
    {"Misc", "huh_mesg", &tp_huh_mesg, 0, MLEV_WIZARD, "Unrecognized command warning", "", 0,
     1, HUH_MESSAGE},
    {"Misc", "leave_mesg", &tp_leave_mesg, 0, MLEV_WIZARD, "Logoff message for QUIT", "", 0, 1,
     LEAVE_MESSAGE},
    {"Misc", "muckname", &tp_muckname, 0, MLEV_WIZARD, "Name of the MUCK", "", 0, 1, MUCKNAME},
    {"Player Max", "playermax_bootmesg", &tp_playermax_bootmesg, 0, MLEV_WIZARD,
     "Max. players connection error message", "", 0, 1, PLAYERMAX_BOOTMESG},
    {"Player Max", "playermax_warnmesg", &tp_playermax_warnmesg, 0, MLEV_WIZARD,
     "Max. players connection login warning", "", 0, 1, PLAYERMAX_WARNMESG},
    {"Properties", "gender_prop", &tp_gender_prop, 0, MLEV_WIZARD,
     "Property name used for pronoun substitutions", "", 0, 1, LEGACY_GENDER_PROP},
    {"Registration", "register_mesg", &tp_register_mesg, 0, MLEV_WIZARD,
     "Login registration denied message", "", 0, 1, REG_MSG},
    {"SSL", "ssl_cert_file", &tp_ssl_cert_file, MLEV_GOD, MLEV_GOD,
     "Path to SSL certificate .pem", "SSL", 0, 1, SSL_CERT_FILE},
    {"SSL", "ssl_key_file", &tp_ssl_key_file, MLEV_GOD, MLEV_GOD,
     "Path to SSL private key .pem", "SSL", 0, 1, SSL_KEY_FILE},
    {"SSL", "ssl_keyfile_passwd", &tp_ssl_keyfile_passwd, MLEV_GOD, MLEV_GOD,
     "Password for SSL private key file", "SSL", 1, 1, SSL_KEYFILE_PASSWD},
    {"SSL", "ssl_cipher_preference_list", &tp_ssl_cipher_preference_list, MLEV_GOD, MLEV_GOD,
     "Allowed OpenSSL cipher list", "SSL", 0, 1, SSL_CIPHER_PREFERENCE_LIST},
    {"SSL", "ssl_min_protocol_version", &tp_ssl_min_protocol_version, MLEV_GOD, MLEV_GOD,
     "Min. allowed SSL protocol version for clients", "SSL", 0, 1, SSL_MIN_PROTOCOL_VERSION},
    {"Database", "pcreate_flags", &tp_pcreate_flags, 0, MLEV_WIZARD,
     "Initial flags for newly created players", "", 1, 1, PCREATE_FLAGS},
    {"Database", "reserved_names", &tp_reserved_names, 0, MLEV_WIZARD,
     "String-match list of reserved names", "", 1, 1, RESERVED_NAMES},
    {"Database", "reserved_player_names", &tp_reserved_player_names, 0, MLEV_WIZARD,
     "String-match list of reserved player names", "", 1, 1, RESERVED_PLAYER_NAMES},
    {NULL, NULL, NULL, 0, 0, NULL, NULL, 0, 0, NULL}
};

int tp_aging_time;
int tp_clean_interval;
int tp_dump_interval;
int tp_dump_warntime;
int tp_idle_ping_time;
int tp_maxidle;
int tp_pname_history_threshold;

struct tune_time_entry tune_time_list[] = {
    {"Database", "aging_time", &tp_aging_time, 0, MLEV_WIZARD,
     "When to considered an object old and unused", "", 1, AGING_TIME},
    {"DB Dumps", "dump_interval", &tp_dump_interval, 0, MLEV_WIZARD, "Interval between dumps",
     "", 1, DUMP_INTERVAL},
    {"DB Dumps", "dump_warntime", &tp_dump_warntime, 0, MLEV_WIZARD,
     "Interval between warning and dump", "", 1, DUMP_WARNTIME},
    {"Idle Boot", "idle_ping_time", &tp_idle_ping_time, 0, MLEV_WIZARD,
     "Server side keepalive time in seconds", "", 1, IDLE_PING_TIME},
    {"Idle Boot", "maxidle", &tp_maxidle, 0, MLEV_WIZARD, "Maximum idle time before booting",
     "", 1, MAXIDLE},
    {"Tuning", "clean_interval", &tp_clean_interval, 0, MLEV_WIZARD,
     "Interval between memory/object cleanups", "", 1, CLEAN_INTERVAL},
    {"Tuning", "pname_history_threshold", &tp_pname_history_threshold, 0, MLEV_WIZARD,
     "Length of player name change history", "", 1, PNAME_HISTORY_THRESHOLD},
    {NULL, NULL, NULL, 0, 0, NULL, NULL, 0, 0}
};

int tp_addpennies_muf_mlev;
int tp_cmd_log_threshold_msec;
int tp_command_burst_size;
int tp_command_time_msec;
int tp_commands_per_time;
int tp_exit_cost;
int tp_free_frames_pool;
int tp_instr_slice;
int tp_kill_base_cost;
int tp_kill_bonus;
int tp_kill_min_cost;
int tp_link_cost;
int tp_listen_mlev;
int tp_lookup_cost;
int tp_max_force_level;
int tp_max_instr_count;
int tp_max_loaded_objs;
int tp_max_ml4_preempt_count;
int tp_max_ml4_nested_interp_loop_count;
int tp_max_nested_interp_loop_count;
int tp_max_object_endowment;
int tp_max_output;
int tp_max_pennies;
int tp_max_plyr_processes;
int tp_max_process_limit;
int tp_mcp_muf_mlev;
int tp_mpi_max_commands;
int tp_movepennies_muf_mlev;
int tp_object_cost;
int tp_pause_min;
int tp_pennies_muf_mlev;
int tp_penny_rate;
int tp_playermax_limit;
int tp_player_name_limit;
int tp_process_timer_limit;
int tp_room_cost;
int tp_start_pennies;
int tp_userlog_mlev;

struct tune_val_entry tune_val_list[] = {
    {"Commands", "player_name_limit", &tp_player_name_limit, 0, MLEV_WIZARD, "Limit on player name length", "", 1,
     PLAYER_NAME_LIMIT},
    {"Costs", "exit_cost", &tp_exit_cost, 0, MLEV_WIZARD, "Cost to create an exit", "", 1,		 
     EXIT_COST},
    {"Costs", "link_cost", &tp_link_cost, 0, MLEV_WIZARD, "Cost to link an exit", "", 1,
     LINK_COST},
    {"Costs", "lookup_cost", &tp_lookup_cost, 0, MLEV_WIZARD, "Cost to lookup a player name",
     "", 1, LOOKUP_COST},
    {"Costs", "max_object_endowment", &tp_max_object_endowment, 0, MLEV_WIZARD,
     "Max. value of an object", "", 1, MAX_OBJECT_ENDOWMENT},
    {"Costs", "object_cost", &tp_object_cost, 0, MLEV_WIZARD, "Cost to create an object", "",
     1, OBJECT_COST},
    {"Costs", "room_cost", &tp_room_cost, 0, MLEV_WIZARD, "Cost to create a room", "", 1,
     ROOM_COST},
    {"Currency", "max_pennies", &tp_max_pennies, 0, MLEV_WIZARD,
     "Max. pennies a player can own", "", 1, MAX_PENNIES},
    {"Currency", "penny_rate", &tp_penny_rate, 0, MLEV_WIZARD,
     "Avg. moves between finding currency", "", 1, PENNY_RATE},
    {"Currency", "start_pennies", &tp_start_pennies, 0, MLEV_WIZARD,
     "Player starting currency count", "", 1, START_PENNIES},
    {"Killing", "kill_base_cost", &tp_kill_base_cost, 0, MLEV_WIZARD, "Cost to guarantee kill",
     "", 1, KILL_BASE_COST},
    {"Killing", "kill_bonus", &tp_kill_bonus, 0, MLEV_WIZARD, "Bonus given to a killed player",
     "", 1, KILL_BONUS},
    {"Killing", "kill_min_cost", &tp_kill_min_cost, 0, MLEV_WIZARD, "Min. cost to do a kill",
     "", 1, KILL_MIN_COST},
    {"Listeners", "listen_mlev", &tp_listen_mlev, 0, MLEV_WIZARD,
     "Mucker Level required for Listener programs", "", 1, LISTEN_MLEV},
    {"Logging", "cmd_log_threshold_msec", &tp_cmd_log_threshold_msec, 0, MLEV_WIZARD,
     "Log commands that take longer than X millisecs", "", 1, CMD_LOG_THRESHOLD_MSEC},
    {"Misc", "max_force_level", &tp_max_force_level, MLEV_GOD, MLEV_GOD,
     "Max. number of forces processed within a command", "", 1, MAX_FORCE_LEVEL},
    {"MPI", "mpi_max_commands", &tp_mpi_max_commands, 0, MLEV_WIZARD,
     "Max. number of uninterruptable MPI commands", "", 1, MPI_MAX_COMMANDS},
    {"MUF", "addpennies_muf_mlev", &tp_addpennies_muf_mlev, 0, MLEV_WIZARD,
     "Mucker Level required to create/destroy pennies", "", 1, ADDPENNIES_MUF_MLEV},
    {"MUF", "instr_slice", &tp_instr_slice, 0, MLEV_WIZARD,
     "Max. uninterrupted instructions per timeslice", "", 1, INSTR_SLICE},
    {"MUF", "max_instr_count", &tp_max_instr_count, 0, MLEV_WIZARD,
     "Max. MUF instruction run length for ML1", "", 1, MAX_INSTR_COUNT},
    {"MUF", "max_ml4_preempt_count", &tp_max_ml4_preempt_count, 0, MLEV_WIZARD,
     "Max. MUF preempt instruction run length for ML4, (0 = no limit)", "", 1,
     MAX_ML4_PREEMPT_COUNT},
    {"MUF", "max_ml4_nested_interp_loop_count", &tp_max_ml4_nested_interp_loop_count, 0, MLEV_WIZARD,
     "Max. MUF preempt interp loop nesting level for ML4 (0 = no limit)", "", 1,
     MAX_ML4_NESTED_INTERP_LOOP_COUNT},
    {"MUF", "max_nested_interp_loop_count", &tp_max_nested_interp_loop_count, 0, MLEV_WIZARD,
     "Max. MUF preempt interp loop nesting level", "", 1,
     MAX_NESTED_INTERP_LOOP_COUNT},
    {"MUF", "max_plyr_processes", &tp_max_plyr_processes, 0, MLEV_WIZARD,
     "Concurrent processes allowed per player", "", 1, MAX_PLYR_PROCESSES},
    {"MUF", "max_process_limit", &tp_max_process_limit, 0, MLEV_WIZARD,
     "Total concurrent processes allowed on system", "", 1, MAX_PROCESS_LIMIT},
    {"MUF", "mcp_muf_mlev", &tp_mcp_muf_mlev, 0, MLEV_WIZARD,
     "Mucker Level required to use MCP", "MCP", 1, MCP_MUF_MLEV},
    {"MUF", "movepennies_muf_mlev", &tp_movepennies_muf_mlev, 0, MLEV_WIZARD,
     "Mucker Level required to move pennies non-destructively", "", 1, MOVEPENNIES_MUF_MLEV},
    {"MUF", "pennies_muf_mlev", &tp_pennies_muf_mlev, 0, MLEV_WIZARD,
     "Mucker Level required to read the value of pennies, settings above 1 disable {money}",
     "", 1, PENNIES_MUF_MLEV},
    {"MUF", "process_timer_limit", &tp_process_timer_limit, 0, MLEV_WIZARD,
     "Max. timers per process", "", 1, PROCESS_TIMER_LIMIT},
    {"MUF", "userlog_mlev", &tp_userlog_mlev, 0, MLEV_WIZARD,
     "Mucker Level required to write to userlog", "", 1, USERLOG_MLEV},
    {"Player Max", "playermax_limit", &tp_playermax_limit, 0, MLEV_WIZARD,
     "Max. player connections allowed", "", 1, PLAYERMAX_LIMIT},
    {"Spam Limits", "command_burst_size", &tp_command_burst_size, 0, MLEV_WIZARD,
     "Max. commands per burst before limiter engages", "", 1, COMMAND_BURST_SIZE},
    {"Spam Limits", "command_time_msec", &tp_command_time_msec, 0, MLEV_WIZARD,
     "Millisecs per spam limiter time period", "", 1, COMMAND_TIME_MSEC},
    {"Spam Limits", "commands_per_time", &tp_commands_per_time, 0, MLEV_WIZARD,
     "Commands allowed per time period during limit", "", 1, COMMANDS_PER_TIME},
    {"Spam Limits", "max_output", &tp_max_output, 0, MLEV_WIZARD, "Max. output buffer size",
     "", 1, MAX_OUTPUT},
    {"Tuning", "free_frames_pool", &tp_free_frames_pool, 0, MLEV_WIZARD,
     "Size of allocated MUF process frame pool", "", 1, FREE_FRAMES_POOL},
    {"Tuning", "max_loaded_objs", &tp_max_loaded_objs, 0, MLEV_WIZARD,
     "Max. percent of proploaded database objects", "DISKBASE", 1, MAX_LOADED_OBJS},
    {"Tuning", "pause_min", &tp_pause_min, 0, MLEV_WIZARD,
     "Min. millisecs between MUF input/output timeslices", "", 1, PAUSE_MIN},
    {NULL, NULL, NULL, 0, 0, NULL, NULL, 0, 0}
};

dbref tp_default_room_parent;
dbref tp_lost_and_found;
dbref tp_player_start;
dbref tp_toad_default_recipient;

struct tune_ref_entry tune_ref_list[] = {
    {"Database", "default_room_parent", TYPE_ROOM, &tp_default_room_parent, 0, MLEV_WIZARD,
     "Place to parent new rooms to", "", 1, GLOBAL_ENVIRONMENT},
    {"Database", "lost_and_found", TYPE_ROOM, &tp_lost_and_found, 0, MLEV_WIZARD,
     "Place for things without a home", "", 1, LOST_AND_FOUND},
    {"Database", "player_start", TYPE_ROOM, &tp_player_start, 0, MLEV_WIZARD,
     "Home where new players start", "", 1, PLAYER_START},
    {"Database", "toad_default_recipient", TYPE_PLAYER, &tp_toad_default_recipient, 0,
     MLEV_WIZARD, "Default owner for @toaded player's things", "", 1, TOAD_DEFAULT_RECIPIENT},
    {NULL, NULL, 0, NULL, 0, 0, NULL, NULL, 0, NOTHING}
};

int tp_7bit_other_names;
int tp_7bit_thing_names;
int tp_allow_home;
int tp_autolink_actions;
int tp_cipher_server_preference;
int tp_compatible_priorities;
int tp_consistent_lock_source;
int tp_dark_sleepers;
int tp_dbdump_warning;
int tp_diskbase_propvals;
int tp_do_mpi_parsing;
int tp_dumpdone_warning;
int tp_enable_match_yield;
int tp_enable_prefix;
int tp_exit_darking;
int tp_expanded_debug;
int tp_force_mlev1_name_notify;
int tp_hostnames;
int tp_idleboot;
int tp_idle_ping_enable;
int tp_ignore_bidirectional;
int tp_ignore_support;
int tp_listeners;
int tp_listeners_env;
int tp_listeners_obj;
int tp_lock_envcheck;
int tp_log_commands;
int tp_log_failed_commands;
int tp_log_interactive;
int tp_log_programs;
int tp_m3_huh;
int tp_muf_comments_strict;
int tp_muf_string_math;
int tp_optimize_muf;
int tp_periodic_program_purge;
int tp_playermax;
int tp_pname_history_reporting;
int tp_quiet_moves;
int tp_realms_control;
int tp_recognize_null_command;
int tp_registration;
int tp_restrict_kill;
int tp_secure_who;
int tp_secure_teleport;
int tp_show_legacy_props;
int tp_starttls_allow;
int tp_strict_god_priv;
int tp_teleport_to_player;
int tp_thing_darking;
int tp_thing_movement;
int tp_toad_recycle;
int tp_who_hides_dark;
int tp_wiz_vehicles;
int tp_verbose_clone;
int tp_verbose_examine;
int tp_zombies;

struct tune_bool_entry tune_bool_list[] = {
    {"Charset", "7bit_thing_names", &tp_7bit_thing_names, MLEV_WIZARD, MLEV_WIZARD,
     "Limit thing names to 7-bit characters", "", 1, ASCII_THING_NAMES},
    {"Charset", "7bit_other_names", &tp_7bit_other_names, MLEV_WIZARD, MLEV_WIZARD,
     "Limit exit/room/muf names to 7-bit characters", "", 1, ASCII_OTHER_NAMES},
    {"Commands", "enable_home", &tp_allow_home, MLEV_WIZARD, MLEV_WIZARD,
     "Enable 'home' command", "", 1, ALLOW_HOME},
    {"Commands", "enable_match_yield", &tp_enable_match_yield, MLEV_WIZARD, MLEV_WIZARD,
     "Enable yield/overt flags on rooms and things", "", 1, ENABLE_MATCH_YIELD},
    {"Commands", "enable_prefix", &tp_enable_prefix, MLEV_WIZARD, MLEV_WIZARD,
     "Enable prefix actions", "", 1, ENABLE_PREFIX},
    {"Commands", "recognize_null_command", &tp_recognize_null_command, MLEV_WIZARD,
     MLEV_WIZARD, "Recognize null command", "", 1, RECOGNIZE_NULL_COMMAND},
    {"Commands", "verbose_clone", &tp_verbose_clone, MLEV_WIZARD, MLEV_WIZARD,
     "Show more information when using @clone command", "", 1, VERBOSE_CLONE},
    {"Commands", "verbose_examine", &tp_verbose_examine, MLEV_WIZARD, MLEV_WIZARD,
     "Show more information when using examine command", "", 1, VERBOSE_EXAMINE},
    {"Dark", "dark_sleepers", &tp_dark_sleepers, 0, MLEV_WIZARD, "Make sleeping players dark",
     "", 1, DARK_SLEEPERS},
    {"Dark", "exit_darking", &tp_exit_darking, 0, MLEV_WIZARD,
     "Allow players to set exits dark", "", 1, EXIT_DARKING},
    {"Dark", "thing_darking", &tp_thing_darking, 0, MLEV_WIZARD,
     "Allow players to set things dark", "", 1, THING_DARKING},
    {"Dark", "who_hides_dark", &tp_who_hides_dark, MLEV_WIZARD, MLEV_WIZARD,
     "Hide dark players from WHO list", "", 1, WHO_HIDES_DARK},
    {"Database", "compatible_priorities", &tp_compatible_priorities, 0, MLEV_WIZARD,
     "Use legacy exit priority levels on things", "", 1, COMPATIBLE_PRIORITIES},
    {"Database", "realms_control", &tp_realms_control, 0, MLEV_WIZARD,
     "Enable support for realm wizzes", "", 1, REALMS_CONTROL},
    {"Database", "toad_recycle", &tp_toad_recycle, 0, MLEV_WIZARD,
     "Recycle newly-created toads", "", 1, TOAD_RECYCLE},
    {"DB Dumps", "diskbase_propvals", &tp_diskbase_propvals, 0, MLEV_WIZARD,
     "Enable property value diskbasing (req. restart)", "DISKBASE", 1, DISKBASE_PROPVALS},
    {"DB Dumps", "dbdump_warning", &tp_dbdump_warning, 0, MLEV_WIZARD,
     "Enable warnings for upcoming database dumps", "", 1, DBDUMP_WARNING},
    {"DB Dumps", "dumpdone_warning", &tp_dumpdone_warning, 0, MLEV_WIZARD,
     "Notify when database dump complete", "", 1, DUMPDONE_WARNING},
    {"Encryption", "starttls_allow", &tp_starttls_allow, MLEV_MASTER, MLEV_WIZARD,
     "Enable TELNET STARTTLS encryption on plaintext port", "", 1, STARTTLS_ALLOW},
    {"Idle Boot", "idleboot", &tp_idleboot, 0, MLEV_WIZARD, "Enable booting of idle players",
     "", 1, IDLEBOOT},
    {"Idle Boot", "idle_ping_enable", &tp_idle_ping_enable, 0, MLEV_WIZARD,
     "Enable server side keepalive pings", "", 1, IDLE_PING_ENABLE},
    {"Killing", "restrict_kill", &tp_restrict_kill, 0, MLEV_WIZARD,
     "Restrict kill command to players set Kill_OK", "", 1, RESTRICT_KILL},
    {"Listeners", "allow_listeners", &tp_listeners, 0, MLEV_WIZARD,
     "Allow programs to listen to player output", "", 1, LISTENERS},
    {"Listeners", "allow_listeners_env", &tp_listeners_env, 0, MLEV_WIZARD,
     "Allow listeners down environment", "", 1, LISTENERS_ENV},
    {"Listeners", "allow_listeners_obj", &tp_listeners_obj, 0, MLEV_WIZARD,
     "Allow objects to be listeners", "", 1, LISTENERS_OBJ},
    {"Logging", "log_commands", &tp_log_commands, MLEV_WIZARD, MLEV_WIZARD,
     "Log player commands", "", 1, LOG_COMMANDS},
    {"Logging", "log_failed_commands", &tp_log_failed_commands, MLEV_WIZARD, MLEV_WIZARD,
     "Log unrecognized commands", "", 1, LOG_FAILED_COMMANDS},
    {"Logging", "log_interactive", &tp_log_interactive, MLEV_WIZARD, MLEV_WIZARD,
     "Log text sent to MUF", "", 1, LOG_INTERACTIVE},
    {"Logging", "log_programs", &tp_log_programs, MLEV_WIZARD, MLEV_WIZARD,
     "Log programs every time they are saved", "", 1, LOG_PROGRAMS},
    {"Misc", "allow_zombies", &tp_zombies, 0, MLEV_WIZARD,
     "Enable Zombie things to relay what they hear", "", 1, ZOMBIES},
    {"Misc", "wiz_vehicles", &tp_wiz_vehicles, 0, MLEV_WIZARD,
     "Only let Wizards set vehicle bits", "", 1, WIZ_VEHICLES},
    {"Misc", "ignore_support", &tp_ignore_support, MLEV_MASTER, MLEV_WIZARD,
     "Enable support for @ignoring players", "", 1, IGNORE_SUPPORT},
    {"Misc", "ignore_bidirectional", &tp_ignore_bidirectional, MLEV_MASTER, MLEV_WIZARD,
     "Enable bidirectional @ignore", "", 1, IGNORE_BIDIRECTIONAL},
    {"Misc", "m3_huh", &tp_m3_huh, MLEV_MASTER, MLEV_WIZARD,
     "Enable huh? to call an exit named \"huh?\" and set M3, with full command string", "", 1,
     M3_HUH},
    {"Misc", "strict_god_priv", &tp_strict_god_priv, MLEV_GOD, MLEV_GOD,
     "Only God can touch God's objects", "GODPRIV", 1, STRICT_GOD_PRIV},
    {"Misc", "autolink_actions", &tp_autolink_actions, 0, MLEV_WIZARD,
     "Automatically link @actions to NIL", "", 1, AUTOLINK_ACTIONS},
    {"Movement", "teleport_to_player", &tp_teleport_to_player, 0, MLEV_WIZARD,
     "Allow using exits linked to players", "", 1, TELEPORT_TO_PLAYER},
    {"Movement", "secure_teleport", &tp_secure_teleport, 0, MLEV_WIZARD,
     "Restrict actions to Jump_OK or controlled rooms", "", 1, SECURE_TELEPORT},
    {"Movement", "secure_thing_movement", &tp_thing_movement, MLEV_WIZARD, MLEV_WIZARD,
     "Moving things act like player", "", 1, SECURE_THING_MOVEMENT},
    {"MPI", "do_mpi_parsing", &tp_do_mpi_parsing, 0, MLEV_WIZARD,
     "Parse MPI strings in messages", "", 1, DO_MPI_PARSING},
    {"MUF", "consistent_lock_source", &tp_consistent_lock_source, 0, MLEV_WIZARD,
     "Maintain trigger as lock source in TESTLOCK", "", 1, CONSISTENT_LOCK_SOURCE},
    {"MUF", "expanded_debug_trace", &tp_expanded_debug, 0, MLEV_WIZARD,
     "MUF debug trace shows array contents", "", 1, EXPANDED_DEBUG_TRACE},
    {"MUF", "force_mlev1_name_notify", &tp_force_mlev1_name_notify, 0, MLEV_WIZARD,
     "MUF notify prepends username for ML1 programs", "", 1, FORCE_MLEV1_NAME_NOTIFY},
    {"MUF", "muf_comments_strict", &tp_muf_comments_strict, 0, MLEV_WIZARD,
     "MUF comments are strict and not recursive", "", 1, MUF_COMMENTS_STRICT},
    {"MUF", "muf_string_math", &tp_muf_string_math, 0, MLEV_WIZARD,
     "Allow some math operators to work with string operands", "", 1, MUF_STRING_MATH},
    {"MUF", "optimize_muf", &tp_optimize_muf, 0, MLEV_WIZARD, "Enable MUF bytecode optimizer",
     "", 1, OPTIMIZE_MUF},
    {"Player Max", "playermax", &tp_playermax, 0, MLEV_WIZARD,
     "Limit number of concurrent players allowed", "", 1, PLAYERMAX},
    {"Movement", "quiet_moves", &tp_quiet_moves, 0, MLEV_WIZARD,
     "Suppress basic arrive and depart notifications", "", 1, QUIET_MOVES},
    {"Properties", "lock_envcheck", &tp_lock_envcheck, 0, MLEV_WIZARD,
     "Locks check environment for properties", "", 1, LOCK_ENVCHECK},
    {"Properties", "show_legacy_props", &tp_show_legacy_props, 0, MLEV_WIZARD,
     "Examining objects lists legacy props", "", 1, SHOW_LEGACY_PROPS},
    {"Registration", "registration", &tp_registration, 0, MLEV_WIZARD,
     "Require new players to register manually", "", 1, REGISTRATION},
    {"SSL", "server_cipher_preference", &tp_cipher_server_preference, MLEV_WIZARD, MLEV_WIZARD,
     "Honor server cipher preference order over client's", "SSL", 1, SERVER_CIPHER_PREFERENCE},
    {"Tuning", "periodic_program_purge", &tp_periodic_program_purge, 0, MLEV_WIZARD,
     "Periodically free unused MUF programs", "", 1, PERIODIC_PROGRAM_PURGE},
    {"Tuning", "pname_history_reporting", &tp_pname_history_reporting, 0, MLEV_WIZARD,
     "Report player name change history", "", 1, PNAME_HISTORY_REPORTING},
    {"WHO", "secure_who", &tp_secure_who, 0, MLEV_WIZARD,
     "Disallow WHO command from login screen and programs", "", 1, SECURE_WHO},
    {"WHO", "use_hostnames", &tp_hostnames, 0, MLEV_WIZARD,
     "Resolve IP addresses into hostnames", "", 1, HOSTNAMES},
    {NULL, NULL, NULL, 0, 0, NULL, 0, 0, 0}
};

#endif				/* _TUNELIST_H */
