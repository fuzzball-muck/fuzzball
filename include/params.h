#ifndef _PARAMS_H
#define _PARAMS_H

/* penny related stuff */
/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)-5)/5)
#define OBJECT_GETCOST(endow) ((endow)*5+5)

#define DB_INITIAL_SIZE 100	/* initial malloc() size for the db */

/* User interface low level commands */
#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"
#define NULL_COMMAND "@@"

/* Used for breaking out of muf READs or for stopping foreground programs. */
#define BREAK_COMMAND "@Q"

#define EXIT_DELIMITER ';'	/* delimiter for lists of exit aliases  */
#define MAX_LINKS 50		/* maximum number of destinations for an exit */

#define MAX_PARENT_DEPTH 256	/* Maximum parenting depth allowed. */

#define GLOBAL_ENVIRONMENT ((dbref) 0)	/* parent of all rooms.  Always #0 */

/* magic cookies (not chocolate chip) :) */

#define ESCAPE_CHAR 27
#define NOT_TOKEN '!'
#define AND_TOKEN '&'
#define OR_TOKEN '|'
#define LOOKUP_TOKEN '*'
#define REGISTERED_TOKEN '$'
#define NUMBER_TOKEN '#'
#define ARG_DELIMITER '='
#define PROP_DELIMITER ':'
#define PROPDIR_DELIMITER '/'
#define PROP_RDONLY '_'
#define PROP_RDONLY2 '%'
#define PROP_PRIVATE '.'
#define PROP_HIDDEN '@'
#define PROP_SEEONLY '~'

/* magic command cookies (oh me, oh my!) */

#define SAY_TOKEN '"'
#define POSE_TOKEN ':'
#define OVERRIDE_TOKEN '!'

/* @edit'or stuff */

#define EXIT_INSERT "."		/* character to exit from insert mode    */
#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
#define CANCEL_EDIT_COMMAND 'x'
#define COMPILE_COMMAND 'c'
#define LIST_COMMAND   'l'
#define EDITOR_HELP_COMMAND 'h'
#define KILL_COMMAND 'k'
#define SHOW_COMMAND 's'
#define SHORTSHOW_COMMAND 'a'
#define VIEW_COMMAND 'v'
#define UNASSEMBLE_COMMAND 'u'
#define NUMBER_COMMAND 'n'
#define PUBLICS_COMMAND 'p'

/* maximum number of arguments */
#define MAX_ARG  2

/* ANSI attributes and color codes */

#define ANSI_RESET	"\033[0m"

#define ANSI_BOLD	"\033[1m"
#define ANSI_DIM      	"\033[2m"
#define ANSI_ITALIC   	"\033[3m"
#define ANSI_UNDERLINE	"\033[4m"
#define ANSI_FLASH	"\033[5m"
#define ANSI_REVERSE	"\033[7m"
#define ANSI_OSTRIKE	"\033[9m"

#define ANSI_FG_BLACK	"\033[30m"
#define ANSI_FG_RED	"\033[31m"
#define ANSI_FG_YELLOW	"\033[33m"
#define ANSI_FG_GREEN	"\033[32m"
#define ANSI_FG_CYAN	"\033[36m"
#define ANSI_FG_BLUE	"\033[34m"
#define ANSI_FG_MAGENTA	"\033[35m"
#define ANSI_FG_WHITE	"\033[37m"

#define ANSI_BG_BLACK	"\033[40m"
#define ANSI_BG_RED	"\033[41m"
#define ANSI_BG_YELLOW	"\033[43m"
#define ANSI_BG_GREEN	"\033[42m"
#define ANSI_BG_CYAN	"\033[46m"
#define ANSI_BG_BLUE	"\033[44m"
#define ANSI_BG_MAGENTA	"\033[45m"
#define ANSI_BG_WHITE	"\033[47m"

/* Property defines */

#define ARRIVE_PROPQUEUE	"_arrive"
#define CONNECT_PROPQUEUE	"_connect"
#define DEPART_PROPQUEUE	"_depart"
#define DISCONNECT_PROPQUEUE	"_disconnect"
#define LISTEN_PROPQUEUE	"_listen"
#define LOOK_PROPQUEUE		"_lookq"
#define OARRIVE_PROPQUEUE	"_oarrive"
#define OCONNECT_PROPQUEUE	"_oconnect"
#define ODEPART_PROPQUEUE	"_odepart"
#define ODISCONNECT_PROPQUEUE	"_odisconnect"
#define WLISTEN_PROPQUEUE	"~listen"
#define WOLISTEN_PROPQUEUE	"~olisten"

#define DETAILS_PROPDIR		"_details"
#define MPI_MACROS_PROPDIR	"_msgmacs"
#define PRONOUNS_PROPDIR	"_pronouns"
#define REGISTRATION_PROPDIR	"_reg"

#define MUF_AUTHOR_PROP		"_author"
#define MUF_ERRCOUNT_PROP	".debug/errcount"
#define MUF_LASTCRASH_PROP	".debug/lastcrash"
#define MUF_LASTCRASHTIME_PROP	".debug/lastcrashtime"
#define MUF_LASTERR_PROP	".debug/lasterr"
#define MUF_LIB_VERSION_PROP	"_lib-version"
#define MUF_NOTE_PROP		"_note"
#define MUF_VERSION_PROP	"_version"

#define SYS_DUMPINTERVAL_PROP	"_sys/dumpinterval"
#define SYS_LASTDUMPTIME_PROP	"_sys/lastdumptime"
#define SYS_MAX_CONNECTS_PROP	"_sys/max_connects"
#define SYS_MAXPENNIES_PROP	"_sys/maxpennies"
#define SYS_SHUTDOWNTIME_PROP	"_sys/shutdowntime"
#define SYS_STARTUPTIME_PROP	"_sys/startuptime"

#define NO_IDLE_PING_PROP	"_/sys/no_idle_ping"
#define IGNORE_PROP		"@__sys__/ignore/def"
#define NO_RECYCLE_PROP		"@/precious"
#define LEGACY_GENDER_PROP	"sex"
#define LEGACY_GUEST_PROP	"~/isguest"

#endif				/* _PARAMS_H */
