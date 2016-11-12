/*
 * config.h
 *
 * Tunable parameters -- Edit to you heart's content 
 *
 * Parameters that control system behavior, and tell the system
 * what resources are available (most of this is now done by
 * the configure script).
 *
 * Most of the goodies that used to be here are now in @tune.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

#include "autoconf.h"

#define VERSION "Muck2.2fb7.00a2"

/************************************************************************
   Administrative Options 

   Various things that affect how the muck operates and what privs
 are compiled in.
 ************************************************************************/

/* Detaches the process as a daemon so that it don't cause problems
 * keeping a terminal line open and such. Logs normal output to a file
 * and writes out a fbmuck.pid file 
 */
#define DETACH

/* Makes God (#1) immune to @force, @newpassword, and being set !Wizard.  
 */
#define GOD_PRIV

/* To use a simple disk basing scheme where properties aren't loaded
 * from the input file until they are needed, define this. (Note: if
 * this is not defined, the MUCK will fork into the background to dump
 * the database, eliminating save delays.)
 */
#undef DISKBASE

/*
 * Port where tinymuck lives -- Note: If you use a port lower than
 * 1024, then the program *must* run suid to root!
 * Port 4201 is a port with historical significance, as the original
 * TinyMUD Classic ran on that port.  It was the office number of James
 * Aspnes, who wrote TinyMUD from which TinyMUCK eventually was derived.
 */
#define TINYPORT 4201		/* Port that players connect to */

/*
 * Some systems can hang for up to 30 seconds while trying to resolve
 * hostnames.  Define this to use a non-blocking second process to resolve
 * hostnames for you.  NOTE:  You need to compile the 'fb-resolver' program
 * (make resolver) and put it in the directory that the fbmuck program is
 * run from.
 */
#define SPAWN_HOST_RESOLVER

/*
 * This is a fairly interesting one -- if there's no DISKBASING, and thus
 * the saves are fork()ed off into the background, then then the child
 * may cause I/O contention with the parent (the interactive, player-
 * connected process).  If this occurs, you can set this to a number
 * greater than 0 to make it be slightly nicer to the rest of the
 * system.  (Usually, setting it to 1 is all that's needed.)
 */
#define NICEVAL 1

/*
 * This allows to use the SIGUSR2 signal as a request for an immediate clean
 * shutdown of the MUCK. Such a signal may be used, for instance, when the UPS
 * is going to fail and shuts down the system. In this case, sending SIGUSR2
 * to the MUCK server process prints a special message on the logged users'
 * screen ("Emergency signal received ! (power failure ?) The database will be
 * saved."), and saves cleanly the database before shutting down the MUCK.
 */
#define SIGEMERG

/************************************************************************
   Game Options

   These are the ones players will notice. 
 ************************************************************************/

/* Make the `examine' command display full names for types and flags */
#define VERBOSE_EXAMINE

/* limit on player name length */
#define PLAYER_NAME_LIMIT 16

/************************************************************************
   Various Messages 
 
   Printed from the server at times, esp. during login.
 ************************************************************************/

/*
 * Welcome message if you don't have a welcome.txt
 */
#define DEFAULT_WELCOME_MESSAGE "Welcome to TinyMUCK.\r\nTo connect to your existing character, enter \"connect name password\"\r\nTo create a new character, enter \"create name password\"\r\nIMPORTANT! Use the news command to get up-to-date news on program changes.\r\n\r\nYou can disconnect using the QUIT command, which must be capitalized as shown.\r\n\r\nAbusing or harrassing other players will result in expellation.\r\nUse the WHO command to find out who is currently active.\r\n"

/*
 * Error messeges spewed by the help system.
 */
#define NO_INFO_MSG "That file does not exist.  Type 'info' to get a list of the info files available."

#define MACRO_FILE  "muf/macros"
#define PID_FILE    "fbmuck.pid"

/************************************************************************
  System Dependency Defines. 

  You probably will not have to monkey with this unless the muck fails
 to compile for some reason.
 ************************************************************************/

/* If you get problems compiling strftime.c, define this. */
#undef USE_STRFTIME

/* Use this only if your realloc does not allocate in powers of 2
 * (if your realloc is clever, this option will cause you to waste space).
 * SunOS requires DB_DOUBLING.  ULTRIX doesn't.  */
#undef DB_DOUBLING

/* Prevent Many Fine Cores. */
#undef NOCOREDUMP

/* if do_usage() in wiz.c gives you problems compiling, define this */
#undef NO_USAGE_COMMAND

/* if do_memory() in wiz.c gives you problems compiling, define this */
#undef NO_MEMORY_COMMAND

/* Enable functionality conforming to MUD Client Protocol (MCP). */
#define MCP_SUPPORT

/* Turn this on when you want MUD to set from root to some user_id */
/* #define MUD_ID "MUCK" */

/* Turn this on when you want MUCK to set to a specific group ID... */
/* #define MUD_GID "MUCK" */

/************************************************************************/
/************************************************************************/
/*    FOR INTERNAL USE ONLY.  DON'T CHANGE ANYTHING PAST THIS POINT.    */
/************************************************************************/
/************************************************************************/

#ifdef HAVE_LIBSSL
# define USE_SSL
#endif

/*
 * Memory/malloc stuff.
 */
#undef LOG_PROPS
#undef DEBUGDBDIRTY

/*
 * Include all the good standard headers here.
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

typedef int dbref;

#ifdef DEBUG
# undef NDEBUG
#include <assert.h>
#define DEBUGPRINT(...) fprintf(stderr, __VA_ARGS__)
#else
# define NDEBUG
#include <assert.h>
#define DEBUGPRINT(...)
#endif				/* DEBUG */

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/*
 * Which set of memory commands do we have here...
 */
#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif				/* not STDC_HEADERS and HAVE_MEMORY_H */
/* Map BSD funcs to ANSI ones. */
# define index		strchr
# define rindex		strrchr
# define bcopy(s, d, n) memcpy ((d), (s), (n))
# define bzero(s, n) memset ((s), 0, (n))
#else				/* not STDC_HEADERS and not HAVE_STRING_H */
# include <strings.h>
/* Map ANSI funcs to BSD ones. */
# define strchr		index
# define strrchr	rindex
# define memcpy(d, s, n) bcopy((s), (d), (n))
/* no real way to map memset to bzero, unfortunatly. */
#endif				/* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_RANDOM
# define SRANDOM(seed)	srandom((seed))
# define RANDOM()	random()
#else
# define SRANDOM(seed)	srand((seed))
# define RANDOM()	rand()
#endif

/*
 * Time stuff.
 */
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Include some of the useful local headers here.
 */
#ifdef MALLOC_PROFILING
# include "crt_malloc.h"
#endif

/******************************************************************/
/* System configuration stuff... Figure out who and what we are.  */
/******************************************************************/

/*
 * Try and figure out what we are.
 */

#if defined(linux) || defined(__linux__) || defined(LINUX)
# define SYSV
#endif

#ifdef sun
# define SUN_OS
#endif

#ifdef ultrix
# define ULTRIX
#endif

#ifdef _AIX
# define AIX
# define NO_MEMORY_COMMAND
#endif

/*
 * Windows compile environment.
 */
#ifdef WIN32
#undef SPAWN_HOST_RESOLVER
#define NO_MEMORY_COMMAND
#define NO_USAGE_COMMAND
#define NOCOREDUMP
#include "win32.h"
#endif

#endif				/* _CONFIG_H */
