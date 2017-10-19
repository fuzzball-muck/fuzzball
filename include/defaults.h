#ifndef _DEFAULTS_H
#define _DEFAULTS_H

#define DUMPWARN_MESG   "## Game will pause to save the database in a few minutes. ##"
#define DUMPING_MESG    "## Pausing to save database. This may take a while. ##"
#define DUMPDONE_MESG   "## Save complete. ##"


/* Change this to the name of your muck.  ie: FurryMUCK, or Tapestries */
#define MUCKNAME "TygryssMUCK"

/* name of the monetary unit being used */
#define PENNY "penny"
#define PENNIES "pennies"
#define CPENNY "Penny"
#define CPENNIES "Pennies"

/* message seen when a player enters a line the server doesn't understand */
#define HUH_MESSAGE "Huh?  (Type \"help\" for help.)"

/*  Goodbye message.  */
#define LEAVE_MESSAGE "Come back later!"

/*  Idleboot message.  */
#define IDLEBOOT_MESSAGE "Autodisconnecting for inactivity."

/*  How long someone can idle for.  */
#define MAXIDLE TIME_HOUR(2)

/*  Boot idle players?  */
#define IDLEBOOT 1

/* ping anti-idle time in seconds - just under 1 minute NAT timeout */
#define IDLE_PING_ENABLE 1
#define IDLE_PING_TIME 55

/* Limit max number of players to allow connected?  (wizards are immune) */
#define PLAYERMAX 0

/* How many connections before blocking? */
#define PLAYERMAX_LIMIT 56

/* The mesg to warn users that nonwizzes can't connect due to connect limits */
#define PLAYERMAX_WARNMESG "You likely won't be able to connect right now, since too many players are online."

/* The mesg to display when booting a connection because of connect limit */
#define PLAYERMAX_BOOTMESG "Sorry, but there are too many players online.  Please try reconnecting in a few minutes."

/*
 * Message if someone trys the create command and your system is
 * setup for registration.
 */
#define REG_MSG "Sorry, you can get a character by e-mailing XXXX@machine.net.address with a charname and password."

/* command to run when entering a room. */
#define AUTOLOOK_CMD "look"

/* various times */
#define AGING_TIME TIME_DAY(90)	/* Unused time before obj shows as old. */
#define DUMP_INTERVAL TIME_HOUR(4)	/* time between dumps */
#define DUMP_WARNTIME TIME_MINUTE(2)	/* warning time before a dump */
#define CLEAN_INTERVAL TIME_MINUTE(15)	/* time between unused obj purges */
#define PNAME_HISTORY_THRESHOLD TIME_DAY(30) /* length of player name change history */
#define PNAME_HISTORY_REPORTING 1	/* Report player name change history */

/* Information needed for SSL */
#define SSL_CERT_FILE "data/server.pem"	/* SSL certificate */
#define SSL_KEY_FILE "data/server.pem"	/* SSL private key */
#define SSL_KEYFILE_PASSWD ""
/* from https://wiki.mozilla.org/Security/Server_Side_TLS, "intermediate copmatability" */
#define SSL_CIPHER_PREFERENCE_LIST \
    "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:" \
    "ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:" \
    "kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:" \
    "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:" \
    "ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:" \
    "DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:" \
    "AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:" \
    "CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:" \
    "!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA"
#define STARTTLS_ALLOW 0
#define SERVER_CIPHER_PREFERENCE 1

/** Minimum SSL protocol version as a string, defaulting to allowing all
 *
 *  Must be specified as string according to known protocol versions.  See SSL_PROTOCOLS in interface_ssl.h
 */
#define SSL_MIN_PROTOCOL_VERSION "None"

/* amount of object endowment, based on cost */
#define MAX_OBJECT_ENDOWMENT 100

/* Block various penny-related functions at less than given mucker level. */
/* 1 = M1, 2 = M2, 3 = M3, 4 = Wiz */
#define MOVEPENNIES_MUF_MLEV 2
#define ADDPENNIES_MUF_MLEV 2
/* This define affects the {money:} MPI function as well. */
#define PENNIES_MUF_MLEV 1

/* minimum costs for various things */
#define OBJECT_COST 10		/* Amount it costs to make an object    */
#define EXIT_COST 1		/* Amount it costs to make an exit      */
#define LINK_COST 1		/* Amount it costs to make a link       */
#define ROOM_COST 10		/* Amount it costs to dig a room        */
#define LOOKUP_COST 0		/* cost to do a scan                    */
#define MAX_PENNIES 10000	/* amount at which temple gets cheap    */
#define PENNY_RATE 8		/* 1/chance of getting a penny per room */
#define START_PENNIES 50	/* # of pennies a player's created with */

/* costs of kill command */
#define KILL_BASE_COST 100	/* prob = expenditure/KILL_BASE_COST    */
#define KILL_MIN_COST 10	/* minimum amount needed to kill        */
#define KILL_BONUS 50		/* amount of "insurance" paid to victim */

/* player spam input limiters */
#define COMMAND_BURST_SIZE 500	/* commands allowed per user in a burst */
#define COMMANDS_PER_TIME 2	/* commands per time slice after burst  */
#define COMMAND_TIME_MSEC 1000	/* time slice length in milliseconds    */


/* Max %of db in unchanged objects allowed to be loaded.  Generally 5% */
/* This is only needed if you defined DISKBASE in config.h */
#define MAX_LOADED_OBJS 5

/* Maximum number of forces processed within a command. */
#define MAX_FORCE_LEVEL 1

/* Max # of timequeue events allowed. */
#define MAX_PROCESS_LIMIT 400

/* Max # of timequeue events allowed for any one player. */
#define MAX_PLYR_PROCESSES 32

/* Max # of instrs in uninterruptable muf programs before timeout. */
#define MAX_INSTR_COUNT 20000

/* Max # of instrs in uninterruptable muf programs before timeout at ML4. */
#define MAX_ML4_PREEMPT_COUNT 0

/* Max # of recursive interpreter calls at ML4. */
#define MAX_ML4_NESTED_INTERP_LOOP_COUNT 0

/* Max # of recursive interpreter calls other than at ML4. */
#define MAX_NESTED_INTERP_LOOP_COUNT 16

/* INSTR_SLICE is the number of instructions to run before forcing a
 * context switch to the next waiting muf program.  This is for the
 * multitasking interpreter.
 */
#define INSTR_SLICE 2000


/* Max # of instrs in uninterruptable programs before timeout. */
#define MPI_MAX_COMMANDS 2048


/* PAUSE_MIN is the minimum time in milliseconds the server will pause
 * in select() between player input/output servicings.  Larger numbers
 * slow FOREGROUND and BACKGROUND MUF programs, but reduce CPU usage.
 */
#define PAUSE_MIN 0

/* FREE_FRAMES_POOL is the number of program frames that are always
 *  available without having to allocate them.  Helps prevent memory
 *  fragmentation.
 */
#define FREE_FRAMES_POOL 8




#define PLAYER_START ((dbref) 0)	/* room number of player start location */




/* Use gethostbyaddr() for hostnames in logs and the wizard WHO list. */
#define HOSTNAMES 0

/* Server support of @doing (reads the _/do prop on WHOs) */
#define WHO_DOING 1

/* To enable logging of all commands */
#define LOG_COMMANDS 1

/* To enable logging of commands while in the read or interactive mode */
#define LOG_INTERACTIVE 1

/* Log failed commands ( HUH'S ) to status log */
#define LOG_FAILED_COMMANDS 0

/* run an m3 exit with the commandline being the parameter on HUH */
#define M3_HUH 0

/* Log the text of changed programs when they are saved.  This is helpful
 * for catching people who upload scanners, use them, then recycle them. */
#define LOG_PROGRAMS 1

/* give a bit of warning before a database dump. */
#define DBDUMP_WARNING 1

/* When a database dump completes, announce it. */
#define DUMPDONE_WARNING 1

/* clear out unused programs every so often */
#define PERIODIC_PROGRAM_PURGE 1

/* Makes WHO unavailable before connecting to a player, or when Interactive.
 * This lets you prevent bypassing of a global @who program. */
#define SECURE_WHO 0

/* Makes all items under the environment of a room set Wizard, be controlled
 * by the owner of that room, as well as by the object owner, and Wizards. */
#define REALMS_CONTROL 0

/* Forbid MCP and MCP-GUI calls at less than given mucker level. 4 = wiz */
#define MCP_MUF_MLEV 3

/* Allows 'listeners' (see CHANGES file) */
#define LISTENERS 1

/* Forbid listener programs of less than given mucker level. 4 = wiz */
#define LISTEN_MLEV 3

/* Allows listeners to be placed on objects. */
#define LISTENERS_OBJ 1

/* Searches up the room environments for listeners */
#define LISTENERS_ENV 1

/* Minimum mucker level to write to the userlog. 4 = wiz */
#define USERLOG_MLEV 3

/* Allow mortal players to @force around objects that are set ZOMBIE. */
#define ZOMBIES 1

/* Allow only wizards to @set the VEHICLE flag on Thing objects. */
#define WIZ_VEHICLES 0

/* Force Mucker Level 1 muf progs to prepend names on notify's to players
 * other than the using player, and on notify_except's and notify_excludes. */
#define FORCE_MLEV1_NAME_NOTIFY 1
/* Define these to let players set TYPE_THING and TYPE_EXIT objects dark. */
#define EXIT_DARKING 1
#define THING_DARKING 1

/* Define this to 1 to make sleeping players effectively DARK */
#define DARK_SLEEPERS 0

/* Define this to 1 if you want DARK players to not show up on the WHO list
 * for mortals, in addition to not showing them in the room contents. */
#define WHO_HIDES_DARK 1

/* Allow a player to use teleport-to-player destinations for exits */
#define TELEPORT_TO_PLAYER 1

/* Make using a personal action require that the source room
 * be controlled by the player, or else be set JUMP_OK */
#define SECURE_TELEPORT 0

/* Allow MUF to perform bytecode optimizations. */
#define OPTIMIZE_MUF 1

/* force MUF comments to use strict oldstyle, and not allow recursion. */
#define MUF_COMMENTS_STRICT 1

/* Enable or diable the global 'HOME' command. */
#define ALLOW_HOME 1

/* Enable or disable in-server 'prefix' or 'abbreviated' commands. */
/* When enabled, a wizard may set the 'A' flag on an action that the wiz */
/* owns.  At this point, that action now does prefix matching, rather */
/* than first-space matching.  This can be used much like ':' and '"' */
/* are used for say and pose.  Note that when enabled, the ':' and '"' are */
/* substituted _after_ a failed command search, so they can be replaced. */
#define ENABLE_PREFIX 0

/* Enable or disable the server's ability to 'skip' rooms in the environment */
/* chain when trying to match a player's exit or command request.  Turning */
/* this on allows the 'Y'ield and 'O'vert flags to function on things or */
/* rooms, changing the way in which command or exit names are found. */
#define ENABLE_MATCH_YIELD 0

/* Enable or disable triggering of movement propqueues when any type of */
/* object moves, not just objects set ZOMBIE or VEHICLE.  This can vastly */
/* increase the amount of processing done, but does allow servers to be */
/* able to programmatically track all movement, so people can't MPI force */
/* an object past a lock, then set it Z or V on the other side. */
#define SECURE_THING_MOVEMENT 0

/* With this defined to 1, exits that aren't on TYPE_THING objects will */
/* always act as if they have at least a Priority Level of 1.  */
/* Define this if you want to use this server with an old db, and don't want */
/* to re-set the Levels of all the LOOK, DISCONNECT, and CONNECT actions. */
#define COMPATIBLE_PRIORITIES 1

/* With this defined to 1, Messages and Omessages run thru the MPI parser. */
/* This means that whatever imbedded MPI commands are in the strings get */
/* interpreted and evaluated.  MPI commands are sort of like MUSH functions, */
/* only possibly more bizzarre. The users will probably love it. */
#define DO_MPI_PARSING 1

/* Only allow killing people with the Kill_OK bit. */
#define RESTRICT_KILL 1

/* To have a private MUCK, this restricts player
 * creation to Wizards using the @pcreate command */
#define REGISTRATION 1

/* Define to 1 to allow locks to check down the environment for props. */
#define LOCK_ENVCHECK 0

/* Define to 0 to prevent diskbasing of property values, or to 1 to allow. */
#define DISKBASE_PROPVALS 1

/* Define to 1 to cause muf debug tracing to display expanded arrays. */
#define EXPANDED_DEBUG_TRACE 1

/* Specifies the maximum number of timers allowed per process. */
#define PROCESS_TIMER_LIMIT 4

/* Log commands that take longer than this many milliseconds */
#define CMD_LOG_THRESHOLD_MSEC 1000

/* max. amount of queued output in bytes, before you get <output flushed> */
#define MAX_OUTPUT 131071

/* Flags that new players will be created with. */
#define PCREATE_FLAGS "B"

/* Smatch pattern of names that cannot be used. */
#define RESERVED_NAMES ""

/* Smatch pattern of player names that cannot be used. */
#define RESERVED_PLAYER_NAMES ""

/* Enable support for ignoring players */
#define IGNORE_SUPPORT 1

/* Enable bidirectional ignoring for players */
#define IGNORE_BIDIRECTIONAL 1

/* Verbose @clone command. */
#define VERBOSE_CLONE 0

/* Expanded flag description in examine command. */
#define VERBOSE_EXAMINE 1

/* Limit on player name length. */
#define PLAYER_NAME_LIMIT 16

/* Recognize null command. */
#define RECOGNIZE_NULL_COMMAND 0

/* Strict GOD_PRIV */
#define STRICT_GOD_PRIV 1

/* Default owner for @toaded player's things */
#define TOAD_DEFAULT_RECIPIENT GOD

/* Recycle newly-created toads */
#define TOAD_RECYCLE 0

/* Place for things without a home */
 #define LOST_AND_FOUND PLAYER_START

/* Force 7-bit names */
#define ASCII_THING_NAMES 1
#define ASCII_OTHER_NAMES 0

/* Show legacy props to player */
#define SHOW_LEGACY_PROPS 0

/* Automatically link @actions to NIL. */
#define AUTOLINK_ACTIONS 0

/* Maintain trigger as lock source in TESTLOCK. */
#define CONSISTENT_LOCK_SOURCE 1

/* Suppress basic arrive and depart notifications. */
#define QUIET_MOVES 0

/* Allow some math operators to work with string operands. */
#define MUF_STRING_MATH 0

/* System data and log files. */
#define WELC_FILE "data/welcome.txt"		/* For the opening screen	*/
#define MOTD_FILE "data/motd.txt"		/* For the message of the day	*/
#define CREDITS_FILE "data/credits.txt"		/* For acknowledgements		*/
#define CONHELP_FILE "data/connect-help.txt"	/* For 'help' before login	*/
#define PARM_FILE "data/parmfile.cfg"		/* For system parameters	*/
#define HELP_FILE "data/help.txt"		/* For the 'help' command	*/
#define HELP_DIR  "data/help"			/* For 'help' subtopic files	*/
#define NEWS_FILE "data/news.txt"		/* For the 'news' command	*/
#define NEWS_DIR  "data/news"			/* For 'news' subtopic files	*/
#define MAN_FILE  "data/man.txt"		/* For the 'man' command	*/
#define MAN_DIR   "data/man"			/* For 'man' subtopic files	*/
#define MPI_FILE  "data/mpihelp.txt"		/* For the 'mpi' command	*/
#define MPI_DIR   "data/mpihelp"        	/* For 'mpi' subtopic files	*/
#define INFO_DIR  "data/info/"			/* For 'info' files		*/
#define EDITOR_HELP_FILE "data/edit-help.txt"   /* editor help file		*/

#define LOG_CMD_TIMES "logs/cmd-times"		/* Command times Log		*/
#define LOG_GRIPE   "logs/gripes"		/* Gripes Log			*/
#define LOG_MALLOC  "logs/malloc"		/* Memory allocations		*/
#define LOG_STATUS  "logs/status"		/* System errors and stats	*/
#define LOG_SANITY  "logs/sanity"		/* Database corruption and errors */
#define LOG_SANFIX  "logs/sanfixed"		/* Database fixes               */
#define LOG_MUF     "logs/muf-errors"		/* Muf compiler errors and warnings. */
#define COMMAND_LOG "logs/commands"		/* Player commands */
#define PROGRAM_LOG "logs/programs"		/* text of changed programs */
#define USER_LOG    "logs/user"			/* log of player/program-init msgs. */

#define LOG_FILE "logs/fbmuck"			/* Log stdout to ...		*/
#define LOG_ERR_FILE "logs/fbmuck.err"		/* Log stderr to ...	*/

#endif				/* _DEFAULTS_H */
