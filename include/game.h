/** @file game.h
 *
 * Header for game.c -- game.c is one of the "core" files that handles
 * most of the main game handling.  Stuff like handling the input from
 * users.  There's a lot of miscellaneous stuff here, so it is a little
 * hard to quantify in this little quip.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef GAME_H
#define GAME_H

#include <stdio.h>

#include "config.h"

#define DEPRECATED_FEATURE "This feature will be removed in the next version of Fuzzball."
/**
 * Define to see if a module is enabled which is to say if the string
 * "module" is in the string "compile_options" using strstr.  This is a
 * somewhat costly operation so you probably don't want to do it trivially.
 *
 * @param module the module that you want to check for in compile_options
 */
#define MOD_ENABLED(module) (module[0] == 0 || strstr(compile_options, module) != NULL)

/*
 * Various special commands and tokens.
 */
#define BREAK_COMMAND "@Q"
#define NULL_COMMAND "@@"
#define OVERRIDE_TOKEN '!'
#define POSE_TOKEN ':'
#define SAY_TOKEN '"'
#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"

/*
 * These are various special tokens related to parameters instead of
 * being commands themselves.
 */
#define AND_TOKEN '&'
#define LOOKUP_TOKEN '*'
#define NOT_TOKEN '!'
#define NUMBER_TOKEN '#'
#define OR_TOKEN '|'
#define REGISTERED_TOKEN '$'

/* Special delimiters */
#define ARG_DELIMITER '='
#define EXIT_DELIMITER ';'

/* The ASCII character number for escape */
#define ESCAPE_CHAR 27

/* Special property directories */
#define DEFINES_PROPDIR         "_defs"     /* For MUF defines              */
#define DETAILS_PROPDIR         "_details"  /* Looktrap prop dir            */
#define MPI_MACROS_PROPDIR      "_msgmacs"  /* MPI Macro prop dir           */
#define PRONOUNS_PROPDIR        "_pronouns" /* Pronoun prop dir             */
#define REGISTRATION_PROPDIR    "_reg"      /* Registration propdir         */
#define SYSTEM_PROPDIR_NOREAD   "@__sys__"  /* For ignore and name history  */
#define SYSTEM_PROPDIR_PROTECT1 "_/sys"     /* For no idle ping prop        */
#define SYSTEM_PROPDIR_PROTECT2 "_sys"      /* For a bunch of system props  */

/* Property directory to track player name history */
#define PNAME_HISTORY_PROPDIR   SYSTEM_PROPDIR_NOREAD "/name"

/* Player original name */
#define PLAYER_CREATED_AS_PROP  PNAME_HISTORY_PROPDIR "/created_as"

/* Ignore props */
#define IGNORE_PROP     SYSTEM_PROPDIR_NOREAD "/ignore/def"

/* Turn off idle ping for this player if set - value doesn't matter */
#define NO_IDLE_PING_PROP   SYSTEM_PROPDIR_PROTECT1 "/no_idle_ping"

/*
 * Various book-keeping props that are set on #0
 *
 * Dump interval, last dump time, maximum number of people that
 * have connected, maximum pennies, shutdown time, and startup time.
 */
#define SYS_DUMPINTERVAL_PROP   SYSTEM_PROPDIR_PROTECT2 "/dumpinterval"
#define SYS_LASTDUMPTIME_PROP   SYSTEM_PROPDIR_PROTECT2 "/lastdumptime"
#define SYS_MAX_CONNECTS_PROP   SYSTEM_PROPDIR_PROTECT2 "/max_connects"
#define SYS_MAXPENNIES_PROP     SYSTEM_PROPDIR_PROTECT2 "/maxpennies"
#define SYS_SHUTDOWNTIME_PROP   SYSTEM_PROPDIR_PROTECT2 "/shutdowntime"
#define SYS_STARTUPTIME_PROP    SYSTEM_PROPDIR_PROTECT2 "/startuptime"

/*
 * So this is a little bit of a misnomer.  It's really a "NO_TOAD_PROP"
 * as it won't allow the MUF toadplayer primitive to toad someone with
 * this property.  It has nothing to do with @recycle and doesn't impact
 * the @toad command.
 */
#define NO_RECYCLE_PROP         "@/precious"

/*
 * This is probably the gender prop from before it was tunable.  It
 * is used mostly for some hacky prop-syncing.
 */
#define LEGACY_GENDER_PROP      "sex"

/*
 * This is before the GUEST flag and is also used for prop-syncing.
 */
#define LEGACY_GUEST_PROP       "~/isguest"

/* Proplist for setting the Welcome screen.  Prop is set on #0 */
#define WELCOME_PROPLIST        "welcome"

/* The different propqueu's.  These are triggered by different events */
#define ARRIVE_PROPQUEUE        "_arrive"       /* Triggered on arrive      */
#define CONNECT_PROPQUEUE       "_connect"      /* Triggered on connect     */
#define DEPART_PROPQUEUE        "_depart"       /* Triggered on leave       */
#define DISCONNECT_PROPQUEUE    "_disconnect"   /* Triggered on disconnect  */
#define LISTEN_PROPQUEUE        "_listen"       /* Triggered on output      */
#define LOOK_PROPQUEUE          "_lookq"        /* Triggered on look        */
#define OARRIVE_PROPQUEUE       "_oarrive"      /* Triggered on arrive      */
#define OCONNECT_PROPQUEUE      "_oconnect"     /* Triggered on connect     */
#define ODEPART_PROPQUEUE       "_odepart"      /* Triggered on leave       */
#define ODISCONNECT_PROPQUEUE   "_odisconnect"  /* Triggered on disconnect  */
#define WLISTEN_PROPQUEUE       "~listen"       /* Triggered on output      */
#define WOLISTEN_PROPQUEUE      "~olisten"      /* Triggered on output      */

/**
 * @var A string variable that is basically a string delimited list of
 *      compile options formed by a series of ifdefs.  The string will
 *      have a trailing space.  Use the MOD_DEFINED(...) define to
 *      figure out if a given option is enabled.
 *
 *      @see MOD_DEFINED
 */
extern const char *compile_options;

/**
 * @var the dump file path
 */
extern const char *dumpfile;

/**
 * @var a variable to keep track of the force level.  This is incremented
 *      when a 'force' command is used to avoid recursive forcing.
 */
extern int force_level;

/**
 * @var boolean value - if true, we are the dump child process.  This is
 *      set immediately after the fork and should be false for the parent
 *      (actual MUCK) process.
 */
extern int forked_dump_process_flag;

/**
 * @var file handle for the input database file
 */
extern FILE *input_file;

/**
 * Clean up certain global strings allocated by game.c
 *
 * This is only available if MEMORY_CLEANUP is defined.
 *
 */
#ifdef MEMORY_CLEANUP
void cleanup_game(void);
#endif

/**
 * Perform a database dump without forking.
 *
 * This is used in cases such as when shutting down the MUCK or whenever
 * you want everything to wait for the database to save.
 *
 */
void dump_database(void);

/**
 * Perform a dump operation, forking a new process if it is supported
 *
 * If DISKBASE is defined, or we are running windows, no process is
 * forked and the database is dumped "inline".
 *
 * Otherwise, a process is forked, its nice level is set to NICELEVEL
 * if defined, and the database is dumped to disk.
 */
void fork_and_dump(void);

/**
 * Initialize the game
 *
 * This does a series of important stuff:
 *
 * - Loads the macro file
 * - Opens th input file
 * - Clears the DB in-memory structures (which initializes it)
 * - Initalize MUF primitives
 * - Initialize MPI
 * - Initialize random number generator
 * - Load DB
 * - Set the book-keeping ~sys properties on #0
 *
 * @param infile the path to the input database file
 * @param outfile the path to the output database file
 * @return returns 0 on success, -1 on any error condition
 */
int init_game(const char *infile, const char *outfile);

/**
 * Process command input from a given user
 *
 * Does all the processing of a given command and directs it to
 * the appropriate target.  This target could be an action/exit, or
 * it could be a command, or if the player is in interactive mode
 * it could be sent to whatever the player is engaging with.
 *
 * @param descr the descriptor of the player
 * @param player the player running the command
 * @param command the command input from the player
 */
void process_command(int descr, dbref player, const char *command);

#endif /* !GAME_H */
