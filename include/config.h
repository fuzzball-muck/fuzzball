/** @file config.h
 *
 * Tunable parameters -- Edit to you heart's content 
 *
 * Parameters that control system behavior, and tell the system
 * what resources are available (most of this is now done by
 * the configure script).
 *
 * Most of the goodies that used to be here are now in \@tune.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "autoconf.h"

/**
 * Server version number
 */
#define VERSION "Muck2.2fb7.10"

/************************************************************************
   Administrative Options

   Various things that affect how the muck operates and what privs
 are compiled in.
 ************************************************************************/

/**
 * Makes God (\#1) immune to \@force, \@newpassword, and being set !Wizard.  
 */
#define GOD_PRIV

/**
 * To use a simple disk basing scheme where properties aren't loaded
 * from the input file until they are needed, define this. (Note: if
 * this is not defined, the MUCK will fork into the background to dump
 * the database, eliminating save delays.)
 *
 * There is almost no reason to use this anymore -- it was used for very
 * memory constricted systems.  That being said, if you are running a large
 * MUCK on a small device (such as a Pi or a very tightly configured VM)
 * this may be a viable option for you.  The penalties do not outweight the
 * memory savings in most modern day scenarios.
 */
#undef DISKBASE

/**
 * Port where tinymuck lives -- Note: If you use a port lower than
 * 1024, then the program *must* run suid to root!
 *
 * Port 4201 is a port with historical significance, as the original
 * TinyMUD Classic ran on that port.  It was the office number of James
 * Aspnes, who wrote TinyMUD from which TinyMUCK eventually was derived.
 */
#define TINYPORT 4201

/**
 * Some systems can hang for up to 30 seconds while trying to resolve
 * hostnames.  Define this to use a non-blocking second process to resolve
 * hostnames for you.  NOTE:  You need to compile the 'fb-resolver' program
 * (make resolver) and put it in the directory that the fbmuck program is
 * run from.  This build is default now.
 */
#define SPAWN_HOST_RESOLVER

/**
 * This is similar to the X-Forwarded-For request header in HTTP.
 * The purpose is to provide administrators running a FORWARDED
 * aware proxy or webclient accurate hostnames in WHO*.
 * 
 * A client must indicate their desire to use this option with
 * IAC WILL FORWARDED and receive a IAC DO FORWARDED in response,
 * after which they may at any time send a sequence like:
 * IAB SB FORWARDED IAC SE.
 *
 * This is only supported for gateways running on the localhost.
 */
#undef IP_FORWARDING

/**
 * This is a fairly interesting one -- if there's no DISKBASING, and thus
 * the saves are fork()ed off into the background, then then the child
 * may cause I/O contention with the parent (the interactive, player-
 * connected process).  If this occurs, you can set this to a number
 * greater than 0 to make it be slightly nicer to the rest of the
 * system.  (Usually, setting it to 1 is all that's needed.)
 */
#define NICEVAL 1

/**
 * This allows to use the SIGUSR2 signal as a request for an immediate clean
 * shutdown of the MUCK. Such a signal may be used, for instance, when the UPS
 * is going to fail and shuts down the system. In this case, sending SIGUSR2
 * to the MUCK server process prints a special message on the logged users'
 * screen ("Emergency signal received ! (power failure ?) The database will be
 * saved."), and saves cleanly the database before shutting down the MUCK.
 */
#define SIGEMERG

/* Exit codes for specific situations */
#define RESTART_EXIT_CODE	0           /**< Clean restart */
#define ARMAGEDDON_EXIT_CODE	1       /**< Emergency abort */

/* Defines for regex prims */
#define MUF_RE_CACHE_ITEMS 64   /**< size of the regex cache */
#define MATCH_ARR_SIZE 30       /**< size of the matches array */

/* Database and server limits */
#define MAX_COMMAND_LEN 2048    /**< max process_command arg length */
#define MAX_COMPLEXITY 18       /**< max nested stackranges (CHECKARGS) */
#define MAX_LINKS 50            /**< max destinations for an exit */
#define MAX_LISTEN_SOCKS 16     /**< max ports we will open */
#define MAX_PARENT_DEPTH 256    /**< max parenting depth allowed */

/**
 * Maximum buffer length
 *
 * Note that changing this will have a drastic effect on memory consumption,
 * as the MUCK is fond of using stack variables of size BUFFER_LEN willy-nilly
 * anywhere it wants a string.  In fact, it is taken for granted that this
 * is a "reasonable" size.
 *
 * For most modern systems, this probably won't be a problem, but making
 * this excessively large will have a noticable impact.
 */
#define BUFFER_LEN ((MAX_COMMAND_LEN)*4)

/************************************************************************
   Various Messages 
 
   Printed from the server at times, esp. during login.
 ************************************************************************/

/**
 * Welcome message if you don't have a welcome.txt
 */
#define DEFAULT_WELCOME_MESSAGE \
    "Welcome to TinyMUCK.\r\n" \
    "To connect to your existing character, enter \"connect name password\"\r\n" \
    "To create a new character, enter \"create name password\"\r\n" \
    "IMPORTANT! Use the news command to get up-to-date news on program changes.\r\n\r\n" \
    "You can disconnect using the QUIT command, which must be capitalized as shown.\r\n\r\n" \
    "Abusing or harrassing other players will result in expellation.\r\n" \
    "Use the WHO command to find out who is currently active.\r\n"

/**
 * Error messeges spewed by the help system.
 */
#define NO_INFO_MSG \
    "That file does not exist.  Type 'info' to get a list of the info files available."

#define MACRO_FILE  "muf/macros"    /**< Where macros are stored */
#define PID_FILE    "fbmuck.pid"    /**< Where the PID is stored */

/*
 * Debugging options (these may be removed or changed at any time)
 */

/**
 * Define this to log all SSL connection messages, including those that
 * generally aren't problematic.  Undefined by default to avoid spamming
 * the status log.
 */
#undef DEBUG_SSL_LOG_ALL

/************************************************************************
  System Dependency Defines. 

  You probably will not have to monkey with this unless the muck fails
 to compile for some reason.
 ************************************************************************/

/* Prevent Many Fine Cores. */
#undef NOCOREDUMP

/* if do_usage() in wiz.c gives you problems compiling, define this */
#undef NO_USAGE_COMMAND

/* if do_memory() in wiz.c gives you problems compiling, define this */
#undef NO_MEMORY_COMMAND

/** Enable MCP-GUI functionality. */
#define MCPGUI_SUPPORT

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
/** Enable SSL */
# define USE_SSL
#endif

/*
 * Include all the good standard headers here.
 * Not anymore!
 * TODO: figure out what to do with the RANDOM() ifdef and stdlib.h
 * TODO: figure out how to resolve conflict when string.h and crt_malloc.h
 *       are included in "wrong" order
 */
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
# undef NDEBUG
#include <assert.h>
/**
 * Simple printf to stderr which is conditional on DEBUG being set.
 */
#define DEBUGPRINT(...) fprintf(stderr, __VA_ARGS__)
#else
# define NDEBUG /**< We're not in debug mode */
#include <assert.h>
/**
 * Simple printf to stderr which is conditional on DEBUG being set.
 */
#define DEBUGPRINT(...)
#endif /* DEBUG */

#ifdef HAVE_ARC4RANDOM_UNIFORM
/**
 * Seed the random number generator
 *
 * @param seed integer seed
 */
# define SRANDOM(seed)	srand((seed))

/**
 * Produce a seeded random number.
 *
 * @return some random number
 */
# define RANDOM()	arc4random_uniform((unsigned)RAND_MAX + 1)
#else
/**
 * Seed the random number generator
 *
 * @param seed integer seed
 */
# define SRANDOM(seed)	srand((seed))

/**
 * Produce a seeded random number.
 *
 * @return some random number
 */
# define RANDOM()	rand()
#endif

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#ifdef MALLOC_PROFILING
# include "crt_malloc.h"
#endif	

/******************************************************************/
/* System configuration stuff... Figure out who and what we are.  */
/******************************************************************/

#ifdef __linux__
/** Standard define if Linux */
# define SYSV
#endif

#if defined(sun) || defined(__sun)
/** I miss Sun */
# define SUN_OS
# ifndef _POSIX_SOURCE
/** Make sure we've got this set */
#  define _POSIX_SOURCE
# endif
#endif

#if defined(ultrix) || defined(__ultrix) || defined(__ultrix__)
/** Does anyone still use this?  Probably not. */
# define ULTRIX
#endif

#ifdef _AIX
/** Does anyone still use this?  Probably not. */
# define AIX
/** AIX is no fun */
# define NO_MEMORY_COMMAND
# include <sys/select.h>
#endif

/*
 * Windows compile environment.
 */
#ifdef WIN32
# undef SPAWN_HOST_RESOLVER
# define NO_MEMORY_COMMAND  /** Not supported by Windows */
# define NO_USAGE_COMMAND   /** Not supported by Windows */
# define NOCOREDUMP         /** Not supported by Windows */
# include "win32.h"
#else
# include <arpa/inet.h>
# include <netdb.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <strings.h>
# include <sys/ioctl.h>
# include <sys/resource.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

#endif /* !CONFIG_H */
