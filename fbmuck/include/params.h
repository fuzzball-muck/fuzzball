#ifndef _PARAMS_H
#define _PARAMS_H

#include "version.h"

/* penny related stuff */
/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)-5)/5)
#define OBJECT_GETCOST(endow) ((endow)*5+5)
#define OBJECT_DEPOSIT(endow) ((endow)*5+4)


/* timing stuff */
#define TIME_MINUTE(x)  (60 * (x))	/* 60 seconds */
#define TIME_HOUR(x)    ((x) * (TIME_MINUTE(60)))	/* 60 minutes */
#define TIME_DAY(x)     ((x) * (TIME_HOUR(24)))	/* 24 hours   */


#define DB_INITIAL_SIZE 100		/* initial malloc() size for the db */


/* User interface low level commands */
#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"
#define PREFIX_COMMAND "OUTPUTPREFIX"
#define SUFFIX_COMMAND "OUTPUTSUFFIX"
#define NULL_COMMAND "@@"

/* Turn this back on when you want MUD to set from root to some user_id */
/* #define MUD_ID "MUCK" */

/* Turn this on when you want MUCK to set to a specific group ID... */
/* #define MUD_GID "MUCK" */

/* Mucker levels */
#define MLEV_APPRENTICE	  1
#define MLEV_JOURNEYMAN   2
#define MLEV_MASTER	  3
#define MLEV_WIZARD	  4

#ifdef GOD_PRIV
# define MLEV_GOD		255
# define TUNE_MLEV(player)	(God(player) ? MLEV_GOD : MLEV_WIZARD)
#else
# define MLEV_GOD		MLEV_WIZARD
# define TUNE_MLEV(player)	MLEV_WIZARD
#endif

/* Used for breaking out of muf READs or for stopping foreground programs. */
#define BREAK_COMMAND "@Q"

#define EXIT_DELIMITER ';'		/* delimiter for lists of exit aliases  */
#define MAX_LINKS 50			/* maximum number of destinations for an exit */

#define MAX_PARENT_DEPTH 256            /* Maximum parenting depth allowed. */

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
#define OVERIDE_TOKEN '!'


/* @edit'or stuff */

#define EXIT_INSERT "."			/* character to exit from insert mode    */
#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
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

/* Usage comments:
   Line numbers start from 1, so when an argument variable is equal to 0, it
   means that it is non existent.

   I've chosen to put the parameters before the command, because this should
   more or less make the players get used to the idea of forth coding..     */



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

#define NO_IDLE_PING_PROP	"_/sys/no_idle_ping"
#define IGNORE_PROP		"@__sys__/ignore/def"
#define NO_RECYCLE_PROP		"@/precious"

/* Do we want MUF math operators (currently =) to work on strings? */
#undef STRINGMATH
 
#endif /* _PARAMS_H */
