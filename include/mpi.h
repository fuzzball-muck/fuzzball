/** @file mpi.h
 *
 * Header for handling MPI message processing.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef MPI_H
#define MPI_H

#include <time.h>
#include "config.h"

/* Some definitions for MPI parameters. */

#define MPI_ISPUBLIC        0x00    /* never test for this one */
#define MPI_ISPRIVATE       0x01    /* Private calls don't do omesg I think */
#define MPI_ISLISTENER      0x02    /* Triggered by listener */
#define MPI_ISLOCK          0x04    /* Triggered by lock */
#define MPI_ISDEBUG         0x08    /* Display debugging */
#define MPI_ISBLESSED       0x10    /* Has 'super user' permissions */
#define MPI_NOHOW           0x20    /* Do not allocate {&how} variable */

#define MAX_MFUN_NAME_LEN 16    /* Maximum length of MPI function name */
#define MAX_MFUN_LIST_LEN 512   /* Maximum list length */
#define MPI_MAX_VARIABLES 32    /* Maximum number of variables in a message */
#define MPI_MAX_FUNCTIONS 32    /* Maximum number of functions allowed */
#define MPI_RECURSION_LIMIT 26  /* Maximum Recursion depth */

/* Changing the following would be incredibly confusing / destructive */
#define MFUN_LITCHAR '`'    /* Literal delimiter */
#define MFUN_LEADCHAR '{'   /* Opening character for MPI function */
#define MFUN_ARGSTART ':'   /* Argument start delimiter */
#define MFUN_ARGSEP ','     /* Argument separator */
#define MFUN_ARGEND '}'     /* Ending character for MPI function */

#define UNKNOWN ((dbref)-88)    /* Error code for unknown object */
#define PERMDENIED ((dbref)-89) /* Error code for permission denied */

#undef WIZZED_DELAY

/*
 * Definition for an MPI variable
 */
struct mpivar {
    char name[MAX_MFUN_NAME_LEN + 1];   /* Name of variable  */
    char *buf;                          /* Value of variable */
};

/*
 * Definition of an MPI function
 */
struct mpifunc {
    char name[MAX_MFUN_NAME_LEN + 1];   /* Name of function */
    char *buf;                          /* Contents of function */
};

/**
 * Check return value
 *
 * Check return value - if 'vari' is null, display error and cause the
 * calling function to return NULL.
 *
 * @param vari the variable to check
 * @param funam the function name
 * @param num string containing the argument number
 */
#define CHECKRETURN(vari,funam,num) if (!vari) { notifyf_nolisten(player, "%s %c%s%c (%s)", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, num); return NULL; }

/**
 * Abort MPI processing
 *
 * Abort MPI processing with a given function name and message, and causes
 * the calling function to return NULL.
 *
 * @param funam the function name string
 * @param mesg the message to display
 */
#define ABORT_MPI(funam,mesg) { notifyf_nolisten(player, "%s %c%s%c: %s", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, mesg); return NULL; }

/**
 * Runs the MPI Message parser
 *
 * Runs the MPI message parser.  This assumes the variables descr, player,
 * what, perms and mesgtyp are in scope at the time of the call.
 *
 * @param in the input string
 * @param out the output buffer
 * @param outlen the size of the output buffer
 */
#define MesgParse(in,out,outlen) mesg_parse(descr, player, what, perms, (in), (out), (outlen), mesgtyp)

/**
 * @var time variable for MPI profiling
 */
extern time_t mpi_prof_start_time;

/**
 * @var keep track of how many variables are in scope -- NOT threadsafe
 */
extern int varc;

/**
 * Check to see if adding 'count' more variables would be too many
 *
 * This will return true if 'count' more variables will be more than
 * MPI_MAX_VARIABLES in total considering already allocated variables.
 *
 * @param count the number of variables you may wish to allocate
 * @return boolean true if you should NOT add more variables, false if you can
 */
int check_mvar_overflow(int count);

/**
 * Deallocate the oldest variable on the variable list
 *
 * @return 1 if there are no variables left, 0 if the variable was deallocated
 */
int free_top_mvar(void);

/**
 * Concatinate a list into a delimited string.
 *
 * How the string is concatinated is controlled by 'mode'.  If mode is
 * 0, then the list is delimited by \r characters such as with the {list}
 * macro.
 * 
 * For mode 1, most lines are separated by a single space.  If a line ends in '.', '?',
 * or '!' then a second space is added as well to the delimiter.
 *
 * For mode 2, there is no delimiter.
 *
 * Uses get_list_count to get the list count.  @see get_list_count
 *
 * @param what the thing that triggered the action
 * @param perms the object that is the source of the permissions
 * @param obj the object to fetch the list off of
 * @param listname the name of the list prop
 * @param buf the buffer to write to
 * @param maxchars the size of the buffer
 * @param mode the mode which is 0, 1, or 2 as described above.
 * @param mesgtyp the mesgtyp variable from the MPI call
 * @param blessed a pointer to an integer which is a boolean for blessed
 * @return the string result which may be NULL if there was a problem loading
 */
char *get_concat_list(dbref what, dbref perms, dbref obj, char *listname,
                      char *buf, int maxchars, int mode, int mesgtyp,
                      int *blessed);

/**
 * Get the item count from a proplist
 *
 * This is not as simple as it seems at first blush because it supports
 * multiple list formats.  It supports listname#, listname/#, and if all
 * else fails, it iterates over the whole list up to MAX_MFUN_LIST_LEN
 * stopping when it finds an empty line.
 *
 * The list is searched for in the environment using safegetprop under the
 * hood.  @see safegetprop   When doing an iterative count, it uses
 * @see get_list_item
 *
 * @param player the player who triggered the action
 * @param what the object to load the prop off of
 * @param perms the object that is the source of permissions
 * @param listname the name of the list without the # mark
 * @param itemnum the line number to fetch from the list
 * @param mesgtyp the mesgtyp from the MPI call
 * @param blessed pointer to integer containing blessed boolean
 * @return string value or empty string if nothing found
 */
int get_list_count(dbref trig, dbref what, dbref perms, char *listname, int mesgtyp,
                   int *blessed);

/**
 * Get an item from a proplist
 *
 * 'itemnum' should be a number from 1 to the size of the list, but it isn't
 * validated at all and can be out of bounds (it should result in an empty
 * string being returned).
 *
 * The list is searched for in the environment using safegetprop under the
 * hood.  @see safegetprop
 *
 * @param player the player who triggered the action
 * @param what the object to load the prop off of
 * @param perms the object that is the source of permissions
 * @param listname the name of the list without the # mark
 * @param itemnum the line number to fetch from the list
 * @param mesgtyp the mesgtyp from the MPI call
 * @param blessed pointer to integer containing blessed boolean
 * @return string value or empty string if nothing found
 */
const char *get_list_item(dbref trig, dbref what, dbref perms, char *listname, int itemnum,
                          int mesgtyp, int *blessed);

/**
 * Fetch the value of an MPI variable by name
 *
 * Returns the value based on name, or NULL if the name does not exist.
 *
 * @param varname the name to look up
 * @return the value set to varname or NULL if varname does not exist
 */
char *get_mvar(const char *varname);

/**
 * Check if d1 is a neighbor of d2
 *
 * A neighbor is defined as:
 *
 * * d1 is d2
 * * d1 and d2 is not a room and they are in the same room
 * * d1 or d2 is a room and one is inside the other.
 *
 * @param d1 one object to check
 * @param d2 the other object to check
 * @return boolean true if d1 and d2 are neighbors
 */
int isneighbor(dbref d1, dbref d2);

/**
 * This is a thin wrapper around mesg_dbref_raw that verifies read permissions
 *
 * First matches 'buf' using @see mesg_dbref_raw
 *
 * Then verifies user can read the object using @see mesg_read_perms
 *
 * @param descr the descriptor of the person doing the match
 * @param player the player doing the matching
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param buf the string to match dbref
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return matched ref, UNKNOWN, or PERMDENIED as appropriate
 */
dbref mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp);

/**
 * Do a database string match and include a local perms match.
 *
 * This first matches using @see mesg_dbref_raw
 *
 * Then it runs @see mesg_local_perms
 *
 * If the mesg_local_perms call returns false, this will return
 * PERMDENIED
 *
 * @param descr the descriptor of the player running the MPI
 * @param player hte player doing the action
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param buf the string to match dbref
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return matched ref, UNKNOWN, or PERMDENIED as appropriate
 */ 
dbref mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf,
                       int mesgtyp);

/**
 * Handle the MPI way of getting a dbref from a string
 *
 * First checks "this", "me", "here", and "home".  Then uses
 * a match relative to the player, and finally does a remote
 * match.
 *
 * @param descr the descriptor of the calling player
 * @param player the player doing the search
 * @param what the object the search is relative to
 * @param buf the string to search for
 * @return either a dbref or UNNOWN if nothing found.
 */
dbref mesg_dbref_raw(int descr, dbref player, dbref what, const char *buf);

/**
 * Do a strict permissions database string match
 *
 * First, the matching is done with @see mesg_dbref_raw
 *
 * Then, we check to mesgtyp to see if MPI_ISBLESSED is set, or if
 * the result is owned by the same owner as 'perms'.  If neither is true,
 * we will return PERMDENIED
 *
 * @param descr the descriptor of the player running the MPI
 * @param player hte player doing the action
 * @param what the object involved in the call
 * @param perms the object to base permissions off of
 * @param buf the string to match dbref
 * @param mesgtyp the mesgtyp from the MPI call containg blessed perms
 * @return matched ref, UNKNOWN, or PERMDENIED as appropriate
 */ 
dbref mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms,
                        char *buf, int mesgtyp);

/**
 * Initialize the hash table with whatever is in mfun_list
 *
 * This must be run before MPI will work, but only has to be run once as
 * mfun_list doesn't change through the course of the server running.
 */
void mesg_init(void);

/**
 * Parse an MPI message
 *
 * MPI can only recurse 25 deep.  This is hard coded.  Only BUFFER_LEN of
 * 'inbuf' will be processed even if inbuf is longer than BUFFER_LEN.
 *
 * @param descr the descriptor of the user running the parser
 * @param player the player running the parser
 * @param what the triggering object
 * @param perms the object that dictates the permission
 * @param inbuf the string to process
 * @param outbuf the output buffer
 * @param maxchars the maximum size of the output buffer
 * @param mesgtyp permission bitvector
 * @return NULL on failure, outbuf on success
 */
char *mesg_parse(int descr, dbref player, dbref what, dbref perms,
                 const char *inbuf, char *outbuf, int maxchars, int mesgtyp);

/**
 * Process an MPI message, handling variable allocations and cleanup
 *
 * The importance of this function is that it allocates certain standard
 * MPI variables: how (if MPI_NOHOW is not in mesgtyp), cmd, and arg
 * 
 * Then it runs MesgParse to process the message after those variables
 * are set up.  And finally, it does cleanup.
 *
 * Statistics are kept as part of this call, to keep track of how much
 * time MPI is taking up to run.  This updates all the profile numbers.
 * 
 * @param descr the descriptor of the calling player
 * @param player the player doing the MPI parsing call
 * @param what the object that triggered the MPI parse call
 * @param perms the object from which permissions are sourced
 * @param inbuf the input string to process
 * @param abuf identifier string for error messages
 * @param outbuf the buffer to store output results
 * @param outbuflen the length of outbuf
 * @param mesgtyp the bitvector of permissions
 * @return a pointer to outbuf
 */
char *do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf,
                    const char *abuf, char *outbuf, int buflen, int mesgtyp);

char *do_parse_prop(int descr, dbref player, dbref what, const char *propname,
                    const char *abuf, char *outbuf, int buflen, int mesgtyp);

/**
 * Allocate a new MPI function with the given name and code
 *
 * This will return 0 on success.  1 if the function name is too long, and
 * 2 if we don't have enough space for another function in the function array.
 *
 * Memory is copied for both funcname and buf.
 *
 * @param funcname the name of the function
 * @param buf the code associated with the name
 * @return integer return status code which may be 0 (success), or 1, or 2
 */
int new_mfunc(const char *funcname, const char *buf);

/**
 * Allocate a new MPI variable if possible
 *
 * This will return 0 on success, 1 if the variable name is too long, or
 * 2 if there are already too many variables allocated.
 *
 * varname is copied, but 'buf' is used as-is.
 *
 * @param varnam the variable name to allocate
 * @param buf the initial value of the variable
 * @return 0 on success, or potentially 1 or 2 on error as described above
 */
int new_mvar(const char *varname, char *buf);

/**
 * Purge all the hash entries for MPI functions.
 *
 * This is used to clean up MPI memory.  It should only be used on server
 * shutdown.
 */
void purge_mfns(void);

/**
 * Safely bless (or unbless) a property
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * @param obj the object to set bless on
 * @param buf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param set_p if true, set blessed.  If false, unbless.
 * @return boolean true if successful, false if a problem happened.
 */
int safeblessprop(dbref obj, char *buf, int mesgtyp, int set_p);

/**
 * Safely fetch a property value, scanning the environment
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * If there is a problem, the player is notified.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden is true, or if Prop_Private is true and the owner of the
 * object doesn't match the owner of what.  @see Prop_Hidden @see Prop_Private
 *
 * This scans the environment for the prop as well.
 *
 * @param player the player looking up the prop
 * @param what the object to load the prop off of
 * @param perms the object we are basing permissions off of (trigger)
 * @param inbuf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param blessed whether or not the prop is blessed will be stored here
 * @return NULL if error, contents of property otherwise.
 */
const char *safegetprop(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int *blessed);

/**
 * Safely fetch a property value without searching the entire environment
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * If there is a problem, the player is notified.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden is true, or if Prop_Private is true and the owner of the
 * object doesn't match the owner of what.  @see Prop_Hidden @see Prop_Private
 *
 * @param player the player looking up the prop
 * @param what the object to load the prop off of
 * @param perms the object we are basing permissions off of (trigger)
 * @param inbuf the name of the property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @param blessed whether or not the prop is blessed will be stored here
 * @return NULL if error, contents of property otherwise.
 */
const char *safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int *blessed);

/**
 * Safely store a value to a property, or to clear it
 *
 * The 'safety' appears to be that it clears out leading slashes and
 * returns false if the prop name is empty, or if the prop is invalid.
 * It also checks to see if 'mesgtyp' is blessed.
 *
 * This will always fail if Prop_System is true.  @see Prop_System
 *
 * If mesgtyp doesn't include MPI_ISBLESSED, this will fail if
 * Prop_Hidden, Prop_SeeOnly are true (@see Prop_Hidden  @see Prop_SeeOnly )
 * or if the prop name starts with MPI_MACROS_PROPDIR
 *
 * val, if NULL, will clear the property.
 *
 * @param obj the object to set bless on
 * @param buf the name of the property
 * @param val the value to set, or NULL to clear a property
 * @param mesgtyp the mesgtyp variable from an MPI call
 * @return boolean true if successful, false if a problem happened.
 */
int safeputprop(dbref obj, char *buf, char *val, int mesgtyp);

#endif /* !MPI_H */
