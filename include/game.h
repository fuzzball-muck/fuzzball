#ifndef _GAME_H
#define _GAME_H

#define MOD_ENABLED(module) (module[0] == 0 || strstr(compile_options, module) != NULL)

#define BREAK_COMMAND "@Q"
#define NULL_COMMAND "@@"
#define OVERRIDE_TOKEN '!'
#define POSE_TOKEN ':'
#define SAY_TOKEN '"'
#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"

#define AND_TOKEN '&'
#define LOOKUP_TOKEN '*'
#define NOT_TOKEN '!'
#define NUMBER_TOKEN '#'
#define OR_TOKEN '|'
#define REGISTERED_TOKEN '$'

#define ARG_DELIMITER '='
#define EXIT_DELIMITER ';'

#define ESCAPE_CHAR 27

#define DEFINES_PROPDIR         "_defs"
#define DETAILS_PROPDIR         "_details"
#define MPI_MACROS_PROPDIR      "_msgmacs"
#define PRONOUNS_PROPDIR        "_pronouns"
#define REGISTRATION_PROPDIR    "_reg"
#define SYSTEM_PROPDIR_NOREAD	"@__sys__"
#define SYSTEM_PROPDIR_PROTECT1	"_/sys"
#define SYSTEM_PROPDIR_PROTECT2	"_sys"

#define PNAME_HISTORY_PROPDIR	SYSTEM_PROPDIR_NOREAD "/name"

#define IGNORE_PROP		SYSTEM_PROPDIR_NOREAD "/ignore/def"
#define NO_IDLE_PING_PROP	SYSTEM_PROPDIR_PROTECT1 "/no_idle_ping"

#define SYS_DUMPINTERVAL_PROP	SYSTEM_PROPDIR_PROTECT2 "/dumpinterval"
#define SYS_LASTDUMPTIME_PROP	SYSTEM_PROPDIR_PROTECT2 "/lastdumptime"
#define SYS_MAX_CONNECTS_PROP	SYSTEM_PROPDIR_PROTECT2 "/max_connects"
#define SYS_MAXPENNIES_PROP	SYSTEM_PROPDIR_PROTECT2 "/maxpennies"
#define SYS_SHUTDOWNTIME_PROP	SYSTEM_PROPDIR_PROTECT2 "/shutdowntime"
#define SYS_STARTUPTIME_PROP	SYSTEM_PROPDIR_PROTECT2 "/startuptime"

#define NO_RECYCLE_PROP         "@/precious"
#define LEGACY_GENDER_PROP      "sex"
#define LEGACY_GUEST_PROP       "~/isguest"

#define WELCOME_PROPLIST        "welcome"

#define ARRIVE_PROPQUEUE        "_arrive"
#define CONNECT_PROPQUEUE       "_connect"
#define DEPART_PROPQUEUE        "_depart"
#define DISCONNECT_PROPQUEUE    "_disconnect"
#define LISTEN_PROPQUEUE        "_listen"
#define LOOK_PROPQUEUE          "_lookq"
#define OARRIVE_PROPQUEUE       "_oarrive"
#define OCONNECT_PROPQUEUE      "_oconnect"
#define ODEPART_PROPQUEUE       "_odepart"
#define ODISCONNECT_PROPQUEUE   "_odisconnect"
#define WLISTEN_PROPQUEUE       "~listen"
#define WOLISTEN_PROPQUEUE      "~olisten"

extern const char *compile_options;
extern int force_level;
extern FILE *input_file;

void cleanup_game(void);
void fork_and_dump(void);
int init_game(const char *infile, const char *outfile);
void panic(const char *);
void process_command(int descr, dbref player, const char *command);

#endif				/* _GAME_H */
