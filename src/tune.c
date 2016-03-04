#include "config.h"
#include "params.h"
#include "db.h"
#include "defaults.h"
#include "externs.h"
#include "array.h"
#include "interp.h"
#include "tune.h"

#define MOD_ENABLED(module) (module[0] == 0 || strstr(compile_options, module) != NULL)

const char *tp_autolook_cmd = AUTOLOOK_CMD;
const char *tp_cpennies = CPENNIES;
const char *tp_cpenny = CPENNY;
const char *tp_dumpdone_mesg = DUMPDONE_MESG;
const char *tp_dumping_mesg = DUMPING_MESG;
const char *tp_dumpwarn_mesg = DUMPWARN_MESG;
const char *tp_gender_prop = LEGACY_GENDER_PROP;
const char *tp_huh_mesg = HUH_MESSAGE;
const char *tp_idle_mesg = IDLEBOOT_MESSAGE;
const char *tp_leave_mesg = LEAVE_MESSAGE;
const char *tp_muckname = MUCKNAME;
const char *tp_pcreate_flags = PCREATE_FLAGS;
const char *tp_pennies = PENNIES;
const char *tp_penny = PENNY;
const char *tp_playermax_bootmesg = PLAYERMAX_BOOTMESG;
const char *tp_playermax_warnmesg = PLAYERMAX_WARNMESG;
const char *tp_register_mesg = REG_MSG;
const char *tp_reserved_names = RESERVED_NAMES;
const char *tp_reserved_player_names = RESERVED_PLAYER_NAMES;
const char *tp_ssl_cert_file = SSL_CERT_FILE;
const char *tp_ssl_key_file = SSL_KEY_FILE;
const char *tp_ssl_keyfile_passwd = SSL_KEYFILE_PASSWD;
const char *tp_ssl_cipher_preference_list = SSL_CIPHER_PREFERENCE_LIST;
const char *tp_ssl_min_protocol_version = SSL_MIN_PROTOCOL_VERSION;

struct tune_str_entry tune_str_list[] = {
	{"Commands",	"autolook_cmd", &tp_autolook_cmd, 0, MLEV_WIZARD, "Room entry look command", "", 0, 1},
	{"Currency",	"cpennies", &tp_cpennies, 0, MLEV_WIZARD, "Currency name, capitalized, plural", "", 0, 1},
	{"Currency",	"cpenny", &tp_cpenny, 0, MLEV_WIZARD, "Currency name, capitalized", "", 0, 1},
	{"Currency",	"pennies", &tp_pennies, 0, MLEV_WIZARD, "Currency name, plural", "", 0, 1},
	{"Currency",	"penny", &tp_penny, 0, MLEV_WIZARD, "Currency name", "", 0, 1},
	{"DB Dumps",	"dumpdone_mesg", &tp_dumpdone_mesg, 0, MLEV_WIZARD, "Dump completion message", "", 1, 1},
	{"DB Dumps",	"dumping_mesg", &tp_dumping_mesg, 0, MLEV_WIZARD, "Full dump start mesg", "", 1, 1},
	{"DB Dumps",	"dumpwarn_mesg", &tp_dumpwarn_mesg, 0, MLEV_WIZARD, "Full dump warning mesg", "", 1, 1},
	{"Idle Boot",	"idle_boot_mesg", &tp_idle_mesg, 0, MLEV_WIZARD, "Boot message for idling out", "", 0, 1},
	{"Misc",	"huh_mesg", &tp_huh_mesg, 0, MLEV_WIZARD, "Command unrecognized warning", "", 0, 1},
	{"Misc",	"leave_mesg", &tp_leave_mesg, 0, MLEV_WIZARD, "Logoff message", "", 0, 1},
	{"Misc",	"muckname", &tp_muckname, 0, MLEV_WIZARD, "Muck name", "", 0, 1},
	{"Player Max",	"playermax_bootmesg", &tp_playermax_bootmesg, 0, MLEV_WIZARD, "Max. players boot message", "", 0, 1},
	{"Player Max",	"playermax_warnmesg", &tp_playermax_warnmesg, 0, MLEV_WIZARD, "Max. players login warning", "", 0, 1},
	{"Properties",  "gender_prop", &tp_gender_prop, 0, MLEV_WIZARD, "Property name used for pronoun substitutions", "", 0, 1},
	{"Registration","register_mesg", &tp_register_mesg, 0, MLEV_WIZARD, "Login registration mesg", "", 0, 1},
	{"SSL",		"ssl_cert_file", &tp_ssl_cert_file, MLEV_GOD, MLEV_GOD, "Path to SSL certificate .pem", "SSL", 0, 1},
	{"SSL",		"ssl_key_file", &tp_ssl_key_file, MLEV_GOD, MLEV_GOD, "Path to SSL private key .pem", "SSL", 0, 1},
	{"SSL",		"ssl_keyfile_passwd", &tp_ssl_keyfile_passwd, MLEV_GOD, MLEV_GOD, "Password for SSL keyfile", "SSL", 1, 1},
	{"SSL",		"ssl_cipher_preference_list", &tp_ssl_cipher_preference_list, MLEV_GOD, MLEV_GOD, "OpenSSL cipher list", "SSL", 0, 1},
	{"SSL",		"ssl_min_protocol_version", &tp_ssl_min_protocol_version, MLEV_GOD, MLEV_GOD, "Minimum allowed SSL version for clients", "SSL", 0, 1},
	{"Database",	"pcreate_flags", &tp_pcreate_flags, 0, MLEV_WIZARD, "Initial Player Flags", "", 1, 1},
	{"Database",	"reserved_names", &tp_reserved_names, 0, MLEV_WIZARD, "Reserved names smatch", "", 1, 1},
	{"Database",	"reserved_player_names", &tp_reserved_player_names, 0, MLEV_WIZARD, "Reserved player names smatch", "", 1, 1},
	{NULL, NULL, NULL, 0, 0, NULL, NULL, 0, 0}
};

int tp_aging_time = AGING_TIME;
int tp_clean_interval = CLEAN_INTERVAL;
int tp_dump_interval = DUMP_INTERVAL;
int tp_dump_warntime = DUMP_WARNTIME;
int tp_idle_ping_time = IDLE_PING_TIME;
int tp_maxidle = MAXIDLE;

struct tune_time_entry tune_time_list[] = {
	{"Database",	"aging_time", &tp_aging_time, 0, MLEV_WIZARD, "When to considered an object old and unused", ""},
	{"DB Dumps",	"dump_interval", &tp_dump_interval, 0, MLEV_WIZARD, "Interval between dumps", ""},
	{"DB Dumps",	"dump_warntime", &tp_dump_warntime, 0, MLEV_WIZARD, "Interval between warning and dump", ""},
	{"Idle Boot",	"idle_ping_time", &tp_idle_ping_time, 0, MLEV_WIZARD, "Server side keepalive time in seconds", ""},
	{"Idle Boot",	"maxidle", &tp_maxidle, 0, MLEV_WIZARD, "Maximum idle time before booting", ""},
	{"Tuning",	"clean_interval", &tp_clean_interval, 0, MLEV_WIZARD, "Interval between memory cleanups.", ""},
	{NULL, NULL, NULL, 0, 0, NULL, NULL}
};

int tp_addpennies_muf_mlev = ADDPENNIES_MUF_MLEV;
int tp_cmd_log_threshold_msec = CMD_LOG_THRESHOLD_MSEC;
int tp_command_burst_size = COMMAND_BURST_SIZE;
int tp_command_time_msec = COMMAND_TIME_MSEC;
int tp_commands_per_time = COMMANDS_PER_TIME;
int tp_exit_cost = EXIT_COST;
int tp_free_frames_pool = FREE_FRAMES_POOL;
int tp_instr_slice = INSTR_SLICE;
int tp_kill_base_cost = KILL_BASE_COST;
int tp_kill_bonus = KILL_BONUS;
int tp_kill_min_cost = KILL_MIN_COST;
int tp_link_cost = LINK_COST;
int tp_listen_mlev = LISTEN_MLEV;
int tp_lookup_cost = LOOKUP_COST;
int tp_max_force_level = MAX_FORCE_LEVEL;
int tp_max_instr_count = MAX_INSTR_COUNT;
int tp_max_loaded_objs = MAX_LOADED_OBJS;
int tp_max_ml4_preempt_count = MAX_ML4_PREEMPT_COUNT;
int tp_max_object_endowment = MAX_OBJECT_ENDOWMENT;
int tp_max_output = MAX_OUTPUT;
int tp_max_pennies = MAX_PENNIES;
int tp_max_plyr_processes = MAX_PLYR_PROCESSES;
int tp_max_process_limit = MAX_PROCESS_LIMIT;
int tp_mcp_muf_mlev = MCP_MUF_MLEV;
int tp_mpi_max_commands = MPI_MAX_COMMANDS;
int tp_movepennies_muf_mlev = MOVEPENNIES_MUF_MLEV;
int tp_object_cost = OBJECT_COST;
int tp_pause_min = PAUSE_MIN;
int tp_pennies_muf_mlev = PENNIES_MUF_MLEV;
int tp_penny_rate = PENNY_RATE;
int tp_room_cost = ROOM_COST;
int tp_start_pennies = START_PENNIES;
int tp_playermax_limit = PLAYERMAX_LIMIT;
int tp_process_timer_limit = PROCESS_TIMER_LIMIT;
int tp_userlog_mlev = USERLOG_MLEV;

struct tune_val_entry tune_val_list[] = {
	{"Costs",	"exit_cost", &tp_exit_cost, 0, MLEV_WIZARD, "Cost to create exit", ""},
	{"Costs",	"link_cost", &tp_link_cost, 0, MLEV_WIZARD, "Cost to link exit", ""},
	{"Costs",	"lookup_cost", &tp_lookup_cost, 0, MLEV_WIZARD, "Cost to lookup playername", ""},
	{"Costs",	"max_object_endowment", &tp_max_object_endowment, 0, MLEV_WIZARD, "Max value of object", ""},
	{"Costs",	"object_cost", &tp_object_cost, 0, MLEV_WIZARD, "Cost to create thing", ""},
	{"Costs",	"room_cost", &tp_room_cost, 0, MLEV_WIZARD, "Cost to create room", ""},
	{"Currency",	"max_pennies", &tp_max_pennies, 0, MLEV_WIZARD, "Player currency cap", ""},
	{"Currency",	"penny_rate", &tp_penny_rate, 0, MLEV_WIZARD, "Moves between finding currency, avg", ""},
	{"Currency",	"start_pennies", &tp_start_pennies, 0, MLEV_WIZARD, "Player starting currency count", ""},
	{"Killing",	"kill_base_cost", &tp_kill_base_cost, 0, MLEV_WIZARD, "Cost to guarentee kill", ""},
	{"Killing",	"kill_bonus", &tp_kill_bonus, 0, MLEV_WIZARD, "Bonus to killed player", ""},
	{"Killing",	"kill_min_cost", &tp_kill_min_cost, 0, MLEV_WIZARD, "Min cost to kill", ""},
	{"Listeners",	"listen_mlev", &tp_listen_mlev, 0, MLEV_WIZARD, "Mucker Level required for Listener progs", ""},
	{"Logging",	"cmd_log_threshold_msec", &tp_cmd_log_threshold_msec, 0, MLEV_WIZARD, "Log commands that take longer than X millisecs", ""},
	{"Misc",	"max_force_level", &tp_max_force_level, MLEV_GOD, MLEV_GOD, "Maximum number of forces processed within a command", ""},
	{"MPI",		"mpi_max_commands", &tp_mpi_max_commands, 0, MLEV_WIZARD, "Max MPI instruction run length", ""},
	{"MUF",		"addpennies_muf_mlev", &tp_addpennies_muf_mlev, 0, MLEV_WIZARD, "Mucker Level required to create/destroy pennies", ""},
	{"MUF",		"instr_slice", &tp_instr_slice, 0, MLEV_WIZARD, "Instructions run per timeslice", ""},
	{"MUF",		"max_instr_count", &tp_max_instr_count, 0, MLEV_WIZARD, "Max MUF instruction run length for ML1", ""},
	{"MUF",		"max_ml4_preempt_count", &tp_max_ml4_preempt_count, 0, MLEV_WIZARD, "Max MUF preempt instruction run length for ML4, (0 = no limit)", ""},
	{"MUF",		"max_plyr_processes", &tp_max_plyr_processes, 0, MLEV_WIZARD, "Max concurrent processes per player", ""},
	{"MUF",		"max_process_limit", &tp_max_process_limit, 0, MLEV_WIZARD, "Max concurrent processes on system", ""},
	{"MUF",		"mcp_muf_mlev", &tp_mcp_muf_mlev, 0, MLEV_WIZARD, "Mucker Level required to use MCP", "MCP"},
	{"MUF",		"movepennies_muf_mlev", &tp_movepennies_muf_mlev, 0, MLEV_WIZARD, "Mucker Level required to move pennies non-destructively", ""},
	{"MUF",		"pennies_muf_mlev", &tp_pennies_muf_mlev, 0, MLEV_WIZARD, "Mucker Level required to read the value of pennies, settings above 1 disable {money}", ""},
	{"MUF",		"process_timer_limit", &tp_process_timer_limit, 0, MLEV_WIZARD, "Max timers per process", ""},
	{"MUF",		"userlog_mlev", &tp_userlog_mlev, 0, MLEV_WIZARD, "Mucker Level required to write to userlog", ""},
	{"Player Max",	"playermax_limit", &tp_playermax_limit, 0, MLEV_WIZARD, "Max player connections allowed", ""},
	{"Spam Limits",	"command_burst_size", &tp_command_burst_size, 0, MLEV_WIZARD, "Commands before limiter engages", ""},
	{"Spam Limits",	"command_time_msec", &tp_command_time_msec, 0, MLEV_WIZARD, "Millisecs per spam limiter time period", ""},
	{"Spam Limits",	"commands_per_time", &tp_commands_per_time, 0, MLEV_WIZARD, "Commands allowed per time period", ""},
	{"Spam Limits",	"max_output", &tp_max_output, 0, MLEV_WIZARD, "Max output buffer size", ""},
	{"Tuning",	"free_frames_pool", &tp_free_frames_pool, 0, MLEV_WIZARD, "Size of MUF process frame pool", ""},
	{"Tuning",	"max_loaded_objs", &tp_max_loaded_objs, 0, MLEV_WIZARD, "Max proploaded object percentage", "DISKBASE"},
	{"Tuning",	"pause_min", &tp_pause_min, 0, MLEV_WIZARD, "Min ms to pause between MUF timeslices", ""},
	{NULL, NULL, NULL, 0, 0, NULL, NULL}
};

dbref tp_default_room_parent = GLOBAL_ENVIRONMENT;
dbref tp_lost_and_found = LOST_AND_FOUND;
dbref tp_player_start = PLAYER_START;
dbref tp_toad_default_recipient = TOAD_DEFAULT_RECIPIENT;

struct tune_ref_entry tune_ref_list[] = {
	{"Database",	"default_room_parent", TYPE_ROOM, &tp_default_room_parent, 0, MLEV_WIZARD, "Place to parent new rooms to", ""},
	{"Database",	"lost_and_found", TYPE_ROOM, &tp_lost_and_found, 0, MLEV_WIZARD, "Place for things without a home", ""},
	{"Database",	"player_start", TYPE_ROOM, &tp_player_start, 0, MLEV_WIZARD, "Place where new players start", ""},
	{"Database",	"toad_default_recipient", TYPE_PLAYER, &tp_toad_default_recipient, 0, MLEV_WIZARD, "Default owner for @toaded player's things", ""},
	{NULL, NULL, 0, NULL, 0, 0, NULL, NULL}
};

int tp_7bit_other_names = ASCII_OTHER_NAMES;
int tp_7bit_thing_names = ASCII_THING_NAMES;
int tp_allow_home = ALLOW_HOME;
int tp_autolink_actions = AUTOLINK_ACTIONS;
int tp_cipher_server_preference = SERVER_CIPHER_PREFERENCE;
int tp_compatible_priorities = COMPATIBLE_PRIORITIES;
int tp_dark_sleepers = DARK_SLEEPERS;
int tp_dbdump_warning = DBDUMP_WARNING;
int tp_diskbase_propvals = DISKBASE_PROPVALS;
int tp_do_mpi_parsing = DO_MPI_PARSING;
int tp_dumpdone_warning = DUMPDONE_WARNING;
int tp_enable_match_yield = ENABLE_MATCH_YIELD;
int tp_enable_prefix = ENABLE_PREFIX;
int tp_exit_darking = EXIT_DARKING;
int tp_expanded_debug = EXPANDED_DEBUG_TRACE;
int tp_force_mlev1_name_notify = FORCE_MLEV1_NAME_NOTIFY;
int tp_hostnames = HOSTNAMES;
int tp_idleboot = IDLEBOOT;
int tp_idle_ping_enable = IDLE_PING_ENABLE;
int tp_ignore_bidirectional = IGNORE_BIDIRECTIONAL;
int tp_ignore_support = IGNORE_SUPPORT;
int tp_lazy_mpi_istype_perm = LAZY_MPI_ISTYPE_PERM;
int tp_listeners = LISTENERS;
int tp_listeners_env = LISTENERS_ENV;
int tp_listeners_obj = LISTENERS_OBJ;
int tp_lock_envcheck = LOCK_ENVCHECK;
int tp_log_commands = LOG_COMMANDS;
int tp_log_failed_commands = LOG_FAILED_COMMANDS;
int tp_log_interactive = LOG_INTERACTIVE;
int tp_log_programs = LOG_PROGRAMS;
int tp_look_propqueues = LOOK_PROPQUEUES;
int tp_m3_huh = M3_HUH;
int tp_muf_comments_strict = MUF_COMMENTS_STRICT;
int tp_optimize_muf = OPTIMIZE_MUF;
int tp_periodic_program_purge = PERIODIC_PROGRAM_PURGE;
int tp_playermax = PLAYERMAX;
int tp_realms_control = REALMS_CONTROL;
int tp_recognize_null_command = RECOGNIZE_NULL_COMMAND;
int tp_registration = REGISTRATION;
int tp_restrict_kill = RESTRICT_KILL;
int tp_secure_who = SECURE_WHO;
int tp_secure_teleport = SECURE_TELEPORT;
int tp_show_legacy_props = SHOW_LEGACY_PROPS;
int tp_sync_legacy_props = SYNC_LEGACY_PROPS;
int tp_starttls_allow = STARTTLS_ALLOW;
int tp_strict_god_priv = STRICT_GOD_PRIV;
int tp_teleport_to_player = TELEPORT_TO_PLAYER;
int tp_thing_darking = THING_DARKING;
int tp_thing_movement = SECURE_THING_MOVEMENT;
int tp_who_doing = WHO_DOING;
int tp_who_hides_dark = WHO_HIDES_DARK;
int tp_wiz_vehicles = WIZ_VEHICLES;
int tp_verbose_clone = VERBOSE_CLONE;
int tp_zombies = ZOMBIES;

struct tune_bool_entry tune_bool_list[] = {
	{"Charset",	"7bit_thing_names", &tp_7bit_thing_names, MLEV_WIZARD, MLEV_WIZARD, "Thing names may contain only 7-bit characters", ""},
	{"Charset",	"7bit_other_names", &tp_7bit_other_names, MLEV_WIZARD, MLEV_WIZARD, "Exit/room/muf names may contain only 7-bit characters", ""},
	{"Commands",	"enable_home", &tp_allow_home, MLEV_WIZARD, MLEV_WIZARD, "Enable 'home' command", ""},
        {"Commands",	"enable_match_yield", &tp_enable_match_yield, MLEV_WIZARD, MLEV_WIZARD, "Enable yield/overt flags on rooms and things", ""},
	{"Commands",	"enable_prefix", &tp_enable_prefix, MLEV_WIZARD, MLEV_WIZARD, "Enable prefix actions", ""},
	{"Commands",	"recognize_null_command", &tp_recognize_null_command, MLEV_WIZARD, MLEV_WIZARD, "Recognize null command", ""},
	{"Commands",	"verbose_clone", &tp_verbose_clone, MLEV_WIZARD, MLEV_WIZARD, "Verbose @clone command", ""},
	{"Dark",	"dark_sleepers", &tp_dark_sleepers, 0, MLEV_WIZARD, "Make sleeping players dark", ""},
	{"Dark",	"exit_darking", &tp_exit_darking, 0, MLEV_WIZARD, "Allow setting exits dark", ""},
	{"Dark",	"thing_darking", &tp_thing_darking, 0, MLEV_WIZARD, "Allow setting things dark", ""},
	{"Dark",	"who_hides_dark", &tp_who_hides_dark, MLEV_WIZARD, MLEV_WIZARD, "Hide dark players from WHO list", ""},
	{"Database",	"compatible_priorities", &tp_compatible_priorities, 0, MLEV_WIZARD, "Use legacy exit priority levels on things", ""},
	{"Database",	"realms_control", &tp_realms_control, 0, MLEV_WIZARD, "Enable Realms control", ""},
	{"DB Dumps",	"diskbase_propvals", &tp_diskbase_propvals, 0, MLEV_WIZARD, "Enable property value diskbasing (req. restart)", "DISKBASE"},
	{"DB Dumps",	"dbdump_warning", &tp_dbdump_warning, 0, MLEV_WIZARD, "Enable warning messages for full DB dumps", ""},
	{"DB Dumps",	"dumpdone_warning", &tp_dumpdone_warning, 0, MLEV_WIZARD, "Enable notification of DB dump completion", ""},
	{"Encryption",  "starttls_allow", &tp_starttls_allow, MLEV_MASTER, MLEV_WIZARD, "Enable TELNET STARTTLS encryption on plaintext port", ""},
	{"Idle Boot",	"idleboot", &tp_idleboot, 0, MLEV_WIZARD, "Enable booting of idle players", ""},
	{"Idle Boot",	"idle_ping_enable", &tp_idle_ping_enable, 0, MLEV_WIZARD, "Enable server side keepalive", ""},
	{"Killing",	"restrict_kill", &tp_restrict_kill, 0, MLEV_WIZARD, "Restrict kill command to players set Kill_OK", ""},
	{"Listeners",	"allow_listeners", &tp_listeners, 0, MLEV_WIZARD, "Enable programs to listen to player output", ""},
	{"Listeners",	"allow_listeners_env", &tp_listeners_env, 0, MLEV_WIZARD, "Allow listeners down environment", ""},
	{"Listeners",	"allow_listeners_obj", &tp_listeners_obj, 0, MLEV_WIZARD, "Allow listeners on things", ""},
	{"Logging",	"log_commands", &tp_log_commands, MLEV_WIZARD, MLEV_WIZARD, "Enable logging of player commands", ""},
	{"Logging",	"log_failed_commands", &tp_log_failed_commands, MLEV_WIZARD, MLEV_WIZARD, "Enable logging of unrecognized commands", ""},
	{"Logging",	"log_interactive", &tp_log_interactive, MLEV_WIZARD, MLEV_WIZARD, "Enable logging of text sent to MUF", ""},
	{"Logging",	"log_programs", &tp_log_programs, MLEV_WIZARD, MLEV_WIZARD, "Log programs every time they are saved", ""},
	{"Misc",	"allow_zombies", &tp_zombies, 0, MLEV_WIZARD, "Enable Zombie things to relay what they hear", ""},
	{"Misc",	"wiz_vehicles", &tp_wiz_vehicles, 0, MLEV_WIZARD, "Only let Wizards set vehicle bits", ""},
	{"Misc",	"ignore_support", &tp_ignore_support, MLEV_MASTER, MLEV_WIZARD, "Enable support for @ignoring players", ""},
	{"Misc",	"ignore_bidirectional", &tp_ignore_bidirectional, MLEV_MASTER, MLEV_WIZARD, "Enable bidirectional @ignore", ""},
	{"Misc",	"m3_huh", &tp_m3_huh, MLEV_MASTER, MLEV_WIZARD, "Enable huh? to call an exit named \"huh?\" and set M3, with full command string", ""},
	{"Misc",	"strict_god_priv", &tp_strict_god_priv, MLEV_GOD, MLEV_GOD, "Only God can touch God's objects", "GODPRIV"},
	{"Misc",	"autolink_actions", &tp_autolink_actions, 0, MLEV_WIZARD, "Automatically link @actions to NIL", ""},
	{"Movement",	"teleport_to_player", &tp_teleport_to_player, 0, MLEV_WIZARD, "Allow teleporting to a player", ""},
	{"Movement",	"secure_teleport", &tp_secure_teleport, 0, MLEV_WIZARD, "Restrict actions to Jump_OK or controlled rooms", ""},
	{"Movement",	"secure_thing_movement", &tp_thing_movement, MLEV_WIZARD, MLEV_WIZARD, "Moving things act like player", ""},
	{"MPI",		"do_mpi_parsing", &tp_do_mpi_parsing, 0, MLEV_WIZARD, "Enable parsing of mesgs for MPI", ""},
	{"MPI",		"lazy_mpi_istype_perm", &tp_lazy_mpi_istype_perm, 0, MLEV_WIZARD, "Enable looser legacy perms for MPI {istype}", ""},
	{"MUF",		"expanded_debug_trace", &tp_expanded_debug, 0, MLEV_WIZARD, "MUF debug trace shows array contents", ""},
	{"MUF",		"force_mlev1_name_notify", &tp_force_mlev1_name_notify, 0, MLEV_WIZARD, "MUF notify prepends username at ML1", ""},
	{"MUF",		"muf_comments_strict", &tp_muf_comments_strict, 0, MLEV_WIZARD, "MUF comments are strict and not recursive", ""},
	{"MUF",		"optimize_muf", &tp_optimize_muf, 0, MLEV_WIZARD, "Enable MUF bytecode optimizer", ""},
	{"Player Max",	"playermax", &tp_playermax, 0, MLEV_WIZARD, "Limit number of concurrent players allowed", ""},
	{"Properties",	"lock_envcheck", &tp_lock_envcheck, 0, MLEV_WIZARD, "Locks check environment for properties", ""},
	{"Properties",	"look_propqueues", &tp_look_propqueues, 0, MLEV_WIZARD, "When a player looks, trigger _look/ propqueues", ""},
	{"Properties",	"show_legacy_props", &tp_show_legacy_props, 0, MLEV_WIZARD, "Examining objects lists legacy props", ""},
	{"Properties",	"sync_legacy_props", &tp_sync_legacy_props, 0, MLEV_WIZARD, "Setting properties also sets associated legacy props", ""},
	{"Registration","registration", &tp_registration, 0, MLEV_WIZARD, "Require new players to register manually", ""},
        {"SSL",		"server_cipher_preference", &tp_cipher_server_preference, MLEV_WIZARD, MLEV_WIZARD, "Honor server cipher preference order over client's", "SSL"},
	{"Tuning",	"periodic_program_purge", &tp_periodic_program_purge, 0, MLEV_WIZARD, "Periodically free unused MUF programs", ""},
	{"WHO",		"secure_who", &tp_secure_who, 0, MLEV_WIZARD, "Disallow WHO command from login screen and programs", ""},
	{"WHO",		"use_hostnames", &tp_hostnames, 0, MLEV_WIZARD, "Resolve IP addresses into hostnames", ""},
	{"WHO",		"who_doing", &tp_who_doing, 0, MLEV_WIZARD, "Show '_/do' property value in WHO lists", ""},
	{NULL, NULL, NULL, 0, 0, NULL, 0}
};

static const char *
timestr_full(long dtime)
{
	static char buf[32];
	int days, hours, minutes, seconds;

	days = dtime / 86400;
	dtime %= 86400;
	hours = dtime / 3600;
	dtime %= 3600;
	minutes = dtime / 60;
	seconds = dtime % 60;

	snprintf(buf, sizeof(buf), "%3dd %2d:%02d:%02d", days, hours, minutes, seconds);

	return buf;
}

int
tune_count_parms(void)
{
	int total = 0;
	struct tune_str_entry *tstr = tune_str_list;
	struct tune_time_entry *ttim = tune_time_list;
	struct tune_val_entry *tval = tune_val_list;
	struct tune_ref_entry *tref = tune_ref_list;
	struct tune_bool_entry *tbool = tune_bool_list;

	while ((tstr++)->name)
		total++;
	while ((ttim++)->name)
		total++;
	while ((tval++)->name)
		total++;
	while ((tref++)->name)
		total++;
	while ((tbool++)->name)
		total++;

	return total;
}

void
tune_display_parms(dbref player, char *name, int mlev)
{
	char buf[BUFFER_LEN + 50];
	struct tune_str_entry *tstr = tune_str_list;
	struct tune_time_entry *ttim = tune_time_list;
	struct tune_val_entry *tval = tune_val_list;
	struct tune_ref_entry *tref = tune_ref_list;
	struct tune_bool_entry *tbool = tune_bool_list;

	while (tstr->name) {
		if (tstr->readmlev > mlev) { tstr++; continue; }
		strcpyn(buf, sizeof(buf), tstr->name);
		if (!*name || equalstr(name, buf)) {
			snprintf(buf, sizeof(buf), "(str)  %-20s = %.4096s%s", tstr->name, *tstr->str, MOD_ENABLED(tstr->module) ? "" : " [inactive]");
			notify(player, buf);
		}
		tstr++;
	}

	while (ttim->name) {
		if (ttim->readmlev > mlev) { ttim++; continue; }
		strcpyn(buf, sizeof(buf), ttim->name);
		if (!*name || equalstr(name, buf)) {
			snprintf(buf, sizeof(buf), "(time) %-20s = %s%s", ttim->name, timestr_full(*ttim->tim), MOD_ENABLED(ttim->module) ? "" : " [inactive]");
			notify(player, buf);
		}
		ttim++;
	}

	while (tval->name) {
		if (tval->readmlev > mlev) { tval++; continue; }
		strcpyn(buf, sizeof(buf), tval->name);
		if (!*name || equalstr(name, buf)) {
			snprintf(buf, sizeof(buf), "(int)  %-20s = %d%s", tval->name, *tval->val, MOD_ENABLED(tval->module) ? "" : " [inactive]");
			notify(player, buf);
		}
		tval++;
	}

	while (tref->name) {
		if (tref->readmlev > mlev) { tref++; continue; }
		strcpyn(buf, sizeof(buf), tref->name);
		if (!*name || equalstr(name, buf)) {
			snprintf(buf, sizeof(buf), "(ref)  %-20s = %s%s", tref->name, unparse_object(player, *tref->ref), MOD_ENABLED(tref->module) ? "" : " [inactive]");
			notify(player, buf);
		}
		tref++;
	}

	while (tbool->name) {
		if (tbool->readmlev > mlev) {tbool++; continue; }
		strcpyn(buf, sizeof(buf), tbool->name);
		if (!*name || equalstr(name, buf)) {
			snprintf(buf, sizeof(buf), "(bool) %-20s = %s%s", tbool->name, ((*tbool->boolval) ? "yes" : "no"), MOD_ENABLED(tbool->module) ? "" : " [inactive]");
			notify(player, buf);
		}
		tbool++;
	}
	notify(player, "*done*");
}

void
tune_save_parms_to_file(FILE * f)
{
	struct tune_str_entry *tstr = tune_str_list;
	struct tune_time_entry *ttim = tune_time_list;
	struct tune_val_entry *tval = tune_val_list;
	struct tune_ref_entry *tref = tune_ref_list;
	struct tune_bool_entry *tbool = tune_bool_list;

	while (tstr->name) {
		fprintf(f, "%s=%.4096s\n", tstr->name, *tstr->str);
		tstr++;
	}

	while (ttim->name) {
		fprintf(f, "%s=%s\n", ttim->name, timestr_full(*ttim->tim));
		ttim++;
	}

	while (tval->name) {
		fprintf(f, "%s=%d\n", tval->name, *tval->val);
		tval++;
	}

	while (tref->name) {
		fprintf(f, "%s=#%d\n", tref->name, *tref->ref);
		tref++;
	}

	while (tbool->name) {
		fprintf(f, "%s=%s\n", tbool->name, (*tbool->boolval) ? "yes" : "no");
		tbool++;
	}
}

stk_array*
tune_parms_array(const char* pattern, int mlev)
{
	struct tune_str_entry *tstr = tune_str_list;
	struct tune_time_entry *ttim = tune_time_list;
	struct tune_val_entry *tval = tune_val_list;
	struct tune_ref_entry *tref = tune_ref_list;
	struct tune_bool_entry *tbool = tune_bool_list;
	stk_array *nu = new_array_packed(0);
	struct inst temp1;
	char pat[BUFFER_LEN];
	char buf[BUFFER_LEN];
	int i = 0;

	strcpyn(pat, sizeof(pat), pattern);
	while (tbool->name) {
		if (tbool->readmlev <= mlev) {
			strcpyn(buf, sizeof(buf), tbool->name);
			if (!*pattern || equalstr(pat, buf)) {
				stk_array *item = new_array_dictionary();
				array_set_strkey_strval(&item, "type", "boolean");
				array_set_strkey_strval(&item, "group", tbool->group);
				array_set_strkey_strval(&item, "name",  tbool->name);
				array_set_strkey_intval(&item, "value", *tbool->boolval? 1 : 0);
				array_set_strkey_intval(&item, "readmlev",  tbool->readmlev);
				array_set_strkey_intval(&item, "writemlev",  tbool->writemlev);
				array_set_strkey_strval(&item, "label", tbool->label);
				array_set_strkey_intval(&item, "active", MOD_ENABLED(tbool->module));

				temp1.type = PROG_ARRAY;
				temp1.data.array = item;
				array_set_intkey(&nu, i++, &temp1);
				CLEAR(&temp1);
			}
		}
		tbool++;
	}

	while (ttim->name) {
		if (ttim->readmlev <= mlev) {
			strcpyn(buf, sizeof(buf), ttim->name);
			if (!*pattern || equalstr(pat, buf)) {
				stk_array *item = new_array_dictionary();
				array_set_strkey_strval(&item, "type", "timespan");
				array_set_strkey_strval(&item, "group", ttim->group);
				array_set_strkey_strval(&item, "name",  ttim->name);
				array_set_strkey_intval(&item, "value", *ttim->tim);
				array_set_strkey_intval(&item, "readmlev",  ttim->readmlev);
				array_set_strkey_intval(&item, "writemlev",  ttim->writemlev);
				array_set_strkey_strval(&item, "label", ttim->label);
				array_set_strkey_intval(&item, "active", MOD_ENABLED(ttim->module));

				temp1.type = PROG_ARRAY;
				temp1.data.array = item;
				array_set_intkey(&nu, i++, &temp1);
				CLEAR(&temp1);
			}
		}
		ttim++;
	}

	while (tval->name) {
		if (tval->readmlev <= mlev) {
			strcpyn(buf, sizeof(buf), tval->name);
			if (!*pattern || equalstr(pat, buf)) {
				stk_array *item = new_array_dictionary();
				array_set_strkey_strval(&item, "type", "integer");
				array_set_strkey_strval(&item, "group", tval->group);
				array_set_strkey_strval(&item, "name",  tval->name);
				array_set_strkey_intval(&item, "value", *tval->val);
				array_set_strkey_intval(&item, "readmlev",  tval->readmlev);
				array_set_strkey_intval(&item, "writemlev",  tval->writemlev);
				array_set_strkey_strval(&item, "label", tval->label);
				array_set_strkey_intval(&item, "active", MOD_ENABLED(tval->module));

				temp1.type = PROG_ARRAY;
				temp1.data.array = item;
				array_set_intkey(&nu, i++, &temp1);
				CLEAR(&temp1);
			}
		}
		tval++;
	}

	while (tref->name) {
		if (tref->readmlev <= mlev) {
			strcpyn(buf, sizeof(buf), tref->name);
			if (!*pattern || equalstr(pat, buf)) {
				stk_array *item = new_array_dictionary();
				array_set_strkey_strval(&item, "type", "dbref");
				array_set_strkey_strval(&item, "group", tref->group);
				array_set_strkey_strval(&item, "name",  tref->name);
				array_set_strkey_refval(&item, "value", *tref->ref);
				array_set_strkey_intval(&item, "readmlev",  tref->readmlev);
				array_set_strkey_intval(&item, "writemlev",  tref->writemlev);
				array_set_strkey_strval(&item, "label", tref->label);
				array_set_strkey_intval(&item, "active", MOD_ENABLED(tref->module));

				switch (tref->typ) {
					case NOTYPE:
						array_set_strkey_strval(&item, "objtype",  "any");
						break;
					case TYPE_PLAYER:
						array_set_strkey_strval(&item, "objtype",  "player");
						break;
					case TYPE_THING:
						array_set_strkey_strval(&item, "objtype",  "thing");
						break;
					case TYPE_ROOM:
						array_set_strkey_strval(&item, "objtype",  "room");
						break;
					case TYPE_EXIT:
						array_set_strkey_strval(&item, "objtype",  "exit");
						break;
					case TYPE_PROGRAM:
						array_set_strkey_strval(&item, "objtype",  "program");
						break;
					case TYPE_GARBAGE:
						array_set_strkey_strval(&item, "objtype",  "garbage");
						break;
					default:
						array_set_strkey_strval(&item, "objtype",  "unknown");
						break;
				}

				temp1.type = PROG_ARRAY;
				temp1.data.array = item;
				array_set_intkey(&nu, i++, &temp1);
				CLEAR(&temp1);
			}
		}
		tref++;
	}

	while (tstr->name) {
		if (tstr->readmlev <= mlev) {
			strcpyn(buf, sizeof(buf), tstr->name);
			if (!*pattern || equalstr(pat, buf)) {
				stk_array *item = new_array_dictionary();
				array_set_strkey_strval(&item, "type", "string");
				array_set_strkey_strval(&item, "group", tstr->group);
				array_set_strkey_strval(&item, "name",  tstr->name);
				array_set_strkey_strval(&item, "value", *tstr->str);
				array_set_strkey_intval(&item, "readmlev",  tstr->readmlev);
				array_set_strkey_intval(&item, "writemlev",  tstr->writemlev);
				array_set_strkey_strval(&item, "label", tstr->label);
				array_set_strkey_intval(&item, "active", MOD_ENABLED(tstr->module));

				temp1.type = PROG_ARRAY;
				temp1.data.array = item;
				array_set_intkey(&nu, i++, &temp1);
				CLEAR(&temp1);
			}
		}
		tstr++;
	}

	return nu;
}

const char *
tune_get_parmstring(const char *name, int mlev)
{
	static char buf[BUFFER_LEN + 50];
	struct tune_str_entry *tstr = tune_str_list;
	struct tune_time_entry *ttim = tune_time_list;
	struct tune_val_entry *tval = tune_val_list;
	struct tune_ref_entry *tref = tune_ref_list;
	struct tune_bool_entry *tbool = tune_bool_list;

	while (tstr->name) {
		if (!string_compare(name, tstr->name)) {
			if (tstr->readmlev > mlev)
				return "";
			return (*tstr->str);
		}
		tstr++;
	}

	while (ttim->name) {
		if (!string_compare(name, ttim->name)) {
			if (ttim->readmlev > mlev)
				return "";
			snprintf(buf, sizeof(buf), "%d", *ttim->tim);
			return (buf);
		}
		ttim++;
	}

	while (tval->name) {
		if (!string_compare(name, tval->name)) {
			if (tval->readmlev > mlev)
				return "";
			snprintf(buf, sizeof(buf), "%d", *tval->val);
			return (buf);
		}
		tval++;
	}

	while (tref->name) {
		if (!string_compare(name, tref->name)) {
			if (tref->readmlev > mlev)
				return "";
			snprintf(buf, sizeof(buf), "#%d", *tref->ref);
			return (buf);
		}
		tref++;
	}

	while (tbool->name) {
		if (!string_compare(name, tbool->name)) {
			if (tbool->readmlev > mlev)
				return "";
			snprintf(buf, sizeof(buf), "%s", ((*tbool->boolval) ? "yes" : "no"));
			return (buf);
		}
		tbool++;
	}

	/* Parameter was not found.  Return null string. */
	strcpyn(buf, sizeof(buf), "");
	return (buf);
}

void
tune_freeparms()
{
	struct tune_str_entry *tstr = tune_str_list;
	while (tstr->name) {
		if (!tstr->isdefault) {
			free((char *) *tstr->str);
		}
		tstr++;
	}
}

int
tune_setparm(const char *parmname, const char *val, int mlev)
{
	struct tune_str_entry *tstr = tune_str_list;
	struct tune_time_entry *ttim = tune_time_list;
	struct tune_val_entry *tval = tune_val_list;
	struct tune_ref_entry *tref = tune_ref_list;
	struct tune_bool_entry *tbool = tune_bool_list;

	char buf[BUFFER_LEN];
	char *parmval;
	strcpyn(buf, sizeof(buf), val);
	parmval = buf;

	while (tstr->name) {
		if (!string_compare(parmname, tstr->name)) {
			if (tstr->writemlev > mlev) return TUNESET_DENIED;
			if (!tstr->isnullable && !*parmval) return TUNESET_BADVAL;
			if (!tstr->isdefault)
				free((char *) *tstr->str);
			if (*parmval == '-')
				parmval++;
			*tstr->str = string_dup(parmval);
			tstr->isdefault = 0;

			return TUNESET_SUCCESS;
		}
		tstr++;
	}

	while (ttim->name) {
		if (!string_compare(parmname, ttim->name)) {
			if (ttim->writemlev > mlev) return TUNESET_DENIED;
			int days, hrs, mins, secs, result;
			char *end;

			days = hrs = mins = secs = 0;
			end = parmval + strlen(parmval) - 1;
			switch (*end) {
			case 's':
			case 'S':
				*end = '\0';
				if (!number(parmval))
					return TUNESET_SYNTAX;
				secs = atoi(parmval);
				break;
			case 'm':
			case 'M':
				*end = '\0';
				if (!number(parmval))
					return TUNESET_SYNTAX;
				mins = atoi(parmval);
				break;
			case 'h':
			case 'H':
				*end = '\0';
				if (!number(parmval))
					return TUNESET_SYNTAX;
				hrs = atoi(parmval);
				break;
			case 'd':
			case 'D':
				*end = '\0';
				if (!number(parmval))
					return TUNESET_SYNTAX;
				days = atoi(parmval);
				break;
			default:
				result = sscanf(parmval, "%dd %2d:%2d:%2d", &days, &hrs, &mins, &secs);
				if (result != 4)
					return TUNESET_SYNTAX;
				break;
			}
			*ttim->tim = (days * 86400) + (3600 * hrs) + (60 * mins) + secs;
			return TUNESET_SUCCESS;
		}
		ttim++;
	}

	while (tval->name) {
		if (!string_compare(parmname, tval->name)) {
			if (tval->writemlev > mlev) return TUNESET_DENIED;
			if (!number(parmval))
				return TUNESET_SYNTAX;
			*tval->val = atoi(parmval);
			return TUNESET_SUCCESS;
		}
		tval++;
	}

	while (tref->name) {
		if (!string_compare(parmname, tref->name)) {
			if (tref->writemlev > mlev) return TUNESET_DENIED;

			dbref obj;

			if (*parmval != NUMBER_TOKEN)
				return TUNESET_SYNTAX;
			if (!number(parmval + 1))
				return TUNESET_SYNTAX;
			obj = (dbref) atoi(parmval + 1);
			if (obj < 0 || obj >= db_top)
				return TUNESET_SYNTAX;
			if (tref->typ != NOTYPE && Typeof(obj) != tref->typ)
				return TUNESET_BADVAL;
			*tref->ref = obj;
			return TUNESET_SUCCESS;
		}
		tref++;
	}

	while (tbool->name) {
		if (!string_compare(parmname, tbool->name)) {
			if (tbool->writemlev > mlev) return TUNESET_DENIED;
			if (*parmval == 'y' || *parmval == 'Y') {
				*tbool->boolval = 1;
			} else if (*parmval == 'n' || *parmval == 'N') {
				*tbool->boolval = 0;
			} else {
				return TUNESET_SYNTAX;
			}
			return 0;
		}
		tbool++;
	}

	return TUNESET_UNKNOWN;
}

void
tune_load_parms_from_file(FILE * f, dbref player, int cnt)
{
	char buf[BUFFER_LEN];
	char *c, *p;
	int result=0;

	while (!feof(f) && (cnt < 0 || cnt--)) {
		fgets(buf, sizeof(buf), f);
		if (*buf != '#') {
			p = c = index(buf, '=');
			if (c) {
				*c++ = '\0';
				while (p > buf && isspace(*(--p)))
					*p = '\0';
				while (isspace(*c))
					c++;
				for (p = c; *p && *p != '\n' && *p != '\r'; p++) ;
				*p = '\0';
				for (p = buf; isspace(*p); p++) ;
				if (*p) {
					result = tune_setparm(p, c, MLEV_GOD);
				}
				switch (result) {
				case TUNESET_SUCCESS:
					strcatn(buf, sizeof(buf), ": Parameter set.");
					break;
				case TUNESET_UNKNOWN:
					strcatn(buf, sizeof(buf), ": Unknown parameter.");
					break;
				case TUNESET_SYNTAX:
					strcatn(buf, sizeof(buf), ": Bad parameter syntax.");
					break;
				case TUNESET_BADVAL:
					strcatn(buf, sizeof(buf), ": Bad parameter value.");
					break;
				case TUNESET_DENIED:
					strcatn(buf, sizeof(buf), ": Permission denied.");
					break;
				}
				if (result && player != NOTHING)
					notify(player, p);
			}
		}
	}
}

void
do_tune(dbref player, char *parmname, char *parmval, int full_command_has_delimiter)
{
	char *oldvalue;
	int result;
	int security = TUNE_MLEV(player);

	if (*parmname && full_command_has_delimiter) {
		if (force_level) {
			notify(player, "You cannot force setting a @tune.");
			return;
		}

		/* Duplicate the string, otherwise the oldvalue pointer will be overridden to the new value
		   when tune_setparm() is called. */
		oldvalue = string_dup(tune_get_parmstring(parmname, security));
		result = tune_setparm(parmname, parmval, security);
		switch (result) {
		case TUNESET_SUCCESS:
			log_status("TUNED: %s(%d) tuned %s from '%s' to '%s'",
					   NAME(player), player, parmname, oldvalue, parmval);
			notify(player, "Parameter set.");
			tune_display_parms(player, parmname, security);
			break;
		case TUNESET_UNKNOWN:
			notify(player, "Unknown parameter.");
			break;
		case TUNESET_SYNTAX:
			notify(player, "Bad parameter syntax.");
			break;
		case TUNESET_BADVAL:
			notify(player, "Bad parameter value.");
			break;
		case TUNESET_DENIED:
			notify(player, "Permission denied.");
			break;
		default:
			break;
		}
		/* Don't let the oldvalue hang around */
		if (*oldvalue)
			free(oldvalue);
	} else {
		tune_display_parms(player, parmname, security);
	}
}
