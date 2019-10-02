/**@file mfun.h
 *
 * Header files for all the different MPI built-in functions.
 *
 * These are implemented in mfuns.c and mfuns2.c
 *
 * Has the array that maps function names to calls.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef MFUN_H
#define MFUN_H

#include <stddef.h>

#include "config.h"

/*
 * Declaration list of arguments for MPI functions.  All MPI functions must
 * support this list of arguments.
 */
#define MFUNARGS int descr, dbref player, dbref what, dbref perms, int argc, \
        char **argv, char *buf, size_t buflen, int mesgtyp

/**
 * MPI function that performs an absolute value on the numeric value of arg0
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_abs(MFUNARGS);

/**
 * MPI function that numerically adds up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * adds them all up.  Returns the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_add(MFUNARGS);

/**
 * MPI function that calculates the 'and' logical expression on all arguments
 *
 * It stops evaluating as soon as a false condition is reached, from left
 * to right.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_and(MFUNARGS);
const char *mfn_attr(MFUNARGS);
const char *mfn_awake(MFUNARGS);

/**
 * MPI function to bless a property on an object.
 *
 * Blessed props are basically wizard MPI props.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_bless(MFUNARGS);

/**
 * MPI function to center justify a message
 *
 * arg0 is the message
 *
 * arg1 is the line width which defaults to 78
 *
 * arg2 is the padding string which defaults to " " but can be any
 * string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_center(MFUNARGS);
const char *mfn_commas(MFUNARGS);

/**
 * MPI function that concatinates the lines of a property list
 *
 * First MPI argument is the list name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'.  If there is a
 * # symbol at the end of the list name it will be removed.
 *
 * The return value is a delimited string, delimited by \r, which is an
 * MPI list.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_concat(MFUNARGS);

/**
 * MPI function that returns true if obj1 is inside obj2
 *
 * The first obj1 is the object to search for.  If obj2 is not provided,
 * defaults to the player's ref.
 *
 * The environment is scanned starting from 'obj2'.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_contains(MFUNARGS);
const char *mfn_contents(MFUNARGS);
const char *mfn_controls(MFUNARGS);

/**
 * MPI function converts a unix timestamp to a readable string
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_convsecs(MFUNARGS);

/**
 * MPI function that converts a time string to a UNIX timestamp
 *
 * The time string should be in the format HH:MM:SS MM/DD/YY
 *
 * This does NOT work for Windows unfortunately.
 *
 * @see time_string_to_seconds
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_convtime(MFUNARGS);
const char *mfn_count(MFUNARGS);

/**
 * MPI function to return a UNIX timestamp with object creation time.
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_created(MFUNARGS);

/**
 * MPI function that returns a date string for the given time zone.
 *
 * The date returned looks like mm/dd/yy
 *
 * If no timezone is provided, it is the MUCK time, otherwise it
 * uses the numeric offset provided.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_date(MFUNARGS);

/**
 * MPI function that returns true if obj1 and obj2 refer to the same object
 *
 * This does name matching of obj1 and obj2, along with #dbref number matching.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_dbeq(MFUNARGS);
const char *mfn_debug(MFUNARGS);
const char *mfn_debugif(MFUNARGS);

/**
 * MPI function that decrements the value of the given variable
 *
 * The first argument is a variable name.  The second argument is optional
 * and is the amount you wish to decrement the variable by.  It defaults to
 * 1
 *
 * The resulting value is returned.  The variable is not updated.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_dec(MFUNARGS);

/**
 * MPI function to provide a default value if the first value is null or false
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_default(MFUNARGS);
const char *mfn_delay(MFUNARGS);

/**
 * MPI function to delete a property on an object.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_delprop(MFUNARGS);


/**
 * MPI function to return the object use count
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_descr(MFUNARGS);

/**
 * MPI function that calculates random numbers through a "dice" like interface
 *
 * If one argument is passed, a number between 1 and arg0 is computed
 * With two, it generates arg1 numbers between 1 and arg0 and adds them up
 * With three, it works the same as two, but adds arg2 to the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_dice(MFUNARGS);

/**
 * MPI function that calculates distance between 2D or 3D points
 *
 * If two parameters are passed, it is the distance from 0,0
 * If three parameters are passed, it is from 0,0,0
 * If four parameters are passed, it is from the first pair to the second
 * If six parameters are passed, it is from the first triplet to the third
 *
 * The distance is computed as a floating point.  0.5 is added to the result,
 * then floor(...) is used to turn it into a regular integer.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_dist(MFUNARGS);

/**
 * MPI function that numerically divides up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * divides them "from left to right".  Returns the result.
 *
 * For errors such as divide by zero, this will return 0.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_div(MFUNARGS);

/**
 * MPI function that returns the boolean value arg0 == arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * For this call, that particular nuance doesn't matter too much.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_eq(MFUNARGS);
const char *mfn_escape(MFUNARGS);

/**
 * MPI function that takes a string and evaluates MPI commands within it
 *
 * The opposite of 'lit'.  Often used to parse the output of {list}
 * The MPI commands will always be ran unblessed.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_eval(MFUNARGS);

/**
 * MPI function that takes a string and evaluates MPI commands within it
 *
 * The opposite of 'lit'.  Often used to parse the output of {list}
 * The MPI commands will run with the same blessing level as the code
 * that executes the eval!.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_evalbang(MFUNARGS);

/**
 * MPI function to evaluate the contents of a given property
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' and looks along
 * the environment for the prop if not found.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_exec(MFUNARGS);

/**
 * MPI function to evaluate the contents of a given property
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' but this version
 * does not scan the environment.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_execbang(MFUNARGS);
const char *mfn_exits(MFUNARGS);
const char *mfn_filter(MFUNARGS);

/**
 * MPI function that returns the object flags for provided object as a string.
 *
 * The string is the return of @see unparse_flags
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_flags(MFUNARGS);
const char *mfn_fold(MFUNARGS);
const char *mfn_for(MFUNARGS);
const char *mfn_force(MFUNARGS);
const char *mfn_foreach(MFUNARGS);
const char *mfn_fox(MFUNARGS);

/**
 * MPI function that returns a time string with the specified format
 *
 * Takes a format string that is the same as MUF's 'timefmt' (which
 * also uses strftime under the hood) and takes optional timezone offset
 * and UNIX timestamp second count arguments.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_ftime(MFUNARGS);
const char *mfn_fullname(MFUNARGS);

/**
 * Implementation of the MPI 'func' Function
 *
 * This function allows the definition of a function in MPI and is the
 * equivalent of defining a msgmac.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_func(MFUNARGS);

/**
 * MPI function that returns the boolean value arg0 >= arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_ge(MFUNARGS);

/**
 * MPI function that returns the boolean value arg0 > arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_gt(MFUNARGS);

/**
 * MPI function that returns true if obj1's location is obj2
 *
 * This is a lot like {contains} but it doesn't search the environment.
 * Returns true if obj1 is inside obj2.  If obj2 is not provided, defaults
 * to the player's ref.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_holds(MFUNARGS);

/**
 * MPI function that returns player idle time in seconds.
 *
 * The time returned is the least idle descriptor.  Uses mesg_dbref_raw
 * to process the first argument which should be a player.
 *
 * @see mesg_dbref_raw
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_idle(MFUNARGS);
const char *mfn_if(MFUNARGS);

/**
 * MPI function that increments the value of the given variable
 *
 * The first argument is a variable name.  The second argument is optional
 * and is the amount you wish to increment the variable by.  It defaults to
 * 1
 *
 * The resulting value is returned.  The variable is not updated.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_inc(MFUNARGS);

/**
 * MPI function to execute the contents of the property in the property
 *
 * This is functionally the same as {exec:{prop:propname}}
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' and will scan the
 * environment for both the property and the property referred to by the
 * property.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_index(MFUNARGS);

/**
 * MPI function to execute the contents of the property in the property
 *
 * This is functionally the same as {exec!:{prop:propname}}
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' but this version
 * does not scan the environment for the properties.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_indexbang(MFUNARGS);
const char *mfn_instr(MFUNARGS);

/**
 * MPI function that returns true if argv0 is a #dbref number
 *
 * The ref must start with a # to be valid.
 *
 * Also checks to make sure that the ref actually belongs to a valid
 * DBREF.  @see OkObj
 *
 * @see number
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_isdbref(MFUNARGS);

/**
 * MPI function that returns true if argv0 is a number
 *
 * @see number
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_isnum(MFUNARGS);
const char *mfn_istype(MFUNARGS);
const char *mfn_kill(MFUNARGS);

/**
 * MPI function to return a UNIX timestamp with object last used time.
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_lastused(MFUNARGS);
const char *mfn_lcommon(MFUNARGS);

/**
 * MPI function that returns the boolean value arg0 <= arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_le(MFUNARGS);

/**
 * MPI function to left justify a message
 *
 * arg0 is the message
 *
 * arg1 is the line width which defaults to 78
 *
 * arg2 is the padding string which defaults to " " but can be any
 * string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_left(MFUNARGS);

/**
 * MPI function that returns a carriage-return delimited string from a proplist
 *
 * This works on a proplist structure.  It also executes the MPI in the
 * list when finished.
 *
 * First MPI argument is the prop list name.  The second argument is optional
 * and is an object; if not provided, it defaults to 'how'.
 *
 * Returns a carriage-return delimited string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_lexec(MFUNARGS);
const char *mfn_links(MFUNARGS);

/**
 * MPI function that returns a carriage-return delimited string from a proplist
 *
 * This works on a proplist structure.
 *
 * First MPI argument is the prop list name.  The second argument is optional
 * and is an object; if not provided, it defaults to 'how'.
 *
 * Returns a carriage-return delimited string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_list(MFUNARGS);

/**
 * MPI function that returns a list of all properties in a propdir
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'.  There is an
 * optional third parameter for pattern matching.
 *
 * The return value is a delimited string, delimited by \r, which is an
 * MPI list.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_listprops(MFUNARGS);

/**
 * MPI function to return a literal string without parsed things in it
 *
 * Simply returns a string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_lit(MFUNARGS);
const char *mfn_lmember(MFUNARGS);

/**
 * MPI function returns the dbref location of the given object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_loc(MFUNARGS);
const char *mfn_locked(MFUNARGS);
const char *mfn_log(MFUNARGS);
const char *mfn_lrand(MFUNARGS);
const char *mfn_lremove(MFUNARGS);
const char *mfn_lsort(MFUNARGS);

/**
 * MPI function that returns the boolean value arg0 < arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_lt(MFUNARGS);

/**
 * MPI function that converts a number of seconds to a long form breakdown
 *
 * For instance, "1 week, 2 days, 10 mins, 52 secs".
 *
 * @see timestr_long
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_ltimestr(MFUNARGS);
const char *mfn_lunion(MFUNARGS);
const char *mfn_lunique(MFUNARGS);

/**
 * MPI function that returns the larger of arg0 and arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * Returns the larger of the two.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_max(MFUNARGS);
const char *mfn_midstr(MFUNARGS);

/**
 * MPI function that returns the smaller of arg0 and arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * Returns the smaller of the two.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_min(MFUNARGS);

/**
 * MPI function that makes a list out of the arguments
 *
 * Returns a string delimited by \r's that is a concatination of the
 * arguments.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_mklist(MFUNARGS);

/**
 * MPI function that numerically "modulo"'s all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * mods them "from left to right".  Returns the result.
 *
 * For errors such as divide by zero, this will return 0.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_mod(MFUNARGS);

/**
 * MPI function to return a UNIX timestamp with object modified time.
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_modified(MFUNARGS);

/**
 * MPI function that returns the value of the provided object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_money(MFUNARGS);

/**
 * MPI function to return the MUCK name.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_muckname(MFUNARGS);
const char *mfn_muf(MFUNARGS);


/**
 * MPI function that numerically multiplies up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * multiplies them.  Returns the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_mult(MFUNARGS);
const char *mfn_name(MFUNARGS);

/**
 * MPI function that returns the boolean value arg0 != arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * For this call, that particular nuance doesn't matter too much.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_ne(MFUNARGS);

/**
 * MPI function that checks to see if arg0 is near arg1
 *
 * Nearby objcts are in the same location, or one contains the other,
 * or the two objects are the same object.
 *
 * If arg1 is not provided, defaults to the trigger object (what)
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_nearby(MFUNARGS);

/**
 * MPI function to return a carriage-return
 *
 * Simply returns a string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_nl(MFUNARGS);

/**
 * MPI function that calculates the 'not' logical expression on arg0
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_not(MFUNARGS);

/**
 * MPI function that returns nothing
 *
 * The arguments are parsed (not by this function, though, but rather by
 * the MPI parser), but this always returns an empty string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_null(MFUNARGS);

/**
 * MPI function that returns a list of players online
 *
 * This list is a carriage return delimited string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_online(MFUNARGS);

/**
 * MPI function that returns player online time in seconds.
 *
 * The time returned is the least idle descriptor.  Uses mesg_dbref_raw
 * to process the first argument which should be a player.
 *
 * @see mesg_dbref_raw
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_ontime(MFUNARGS);

/**
 * MPI function that calculates the 'or' logical expression on all arguments
 *
 * It stops evaluating as soon as a true condition is reached, from left
 * to right.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_or(MFUNARGS);

/**
 * MPI function to send a message to the given target room
 *
 * If only arg0 is provided, then it shows the message to the calling player's
 * location, excluding the calling player.
 *
 * If arg1 is provided, the message will go to that room, excluding the
 * calling player.
 *
 * If arg2 is provided as #-1, then it will send the message to everyone
 * in the room excluding nobody.  Otherwise it will exclude whatever dbref
 * is in arg2.
 *
 * The message will be prefixed with the calling player's name if it doesn't
 * already start with that name, under certain conditions.  The conditions
 * are a little difficult to summarize but here's the summary from the
 * MPI docs:
 *
 * If the trigger isn't a room, or an exit on a room, and if the message
 * doesn't already begin with the user's name, then the user's name will
 * be prepended to the message.
 *
 * The string is the return of @see unparse_flags
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_otell(MFUNARGS);
const char *mfn_owner(MFUNARGS);
const char *mfn_parse(MFUNARGS);

/**
 * MPI function that does pronoun substitutions in a string
 *
 * The first argument is a string to process.  By default it
 * uses the using player; the second argument can be given to
 * override that and use a different object for pronoun values.
 *
 * @see pronoun_substitute
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_pronouns(MFUNARGS);

/**
 * MPI function to fetch a prop off an object.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  This will search the environment for
 * the property.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_prop(MFUNARGS);

/**
 * MPI function to fetch a prop off an object without scanning environment.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  This will only look at either the trigger
 * object or the second parameter and not look up the environment chain.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_propbang(MFUNARGS);

/**
 * MPI function that returns true if the given prop name is a prop directory
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_propdir(MFUNARGS);

/**
 * MPI function that returns a randomly picked item from a proplist
 *
 * First MPI argument is the prop list name.  The second argument is optional
 * and is an object; if not provided, it defaults to 'how' and searches up
 * the environment.
 *
 * Returns a list item string.  That string is parsed for MPI before being
 * returned.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_rand(MFUNARGS);
const char *mfn_ref(MFUNARGS);
const char *mfn_revoke(MFUNARGS);

/**
 * MPI function to right justify a message
 *
 * arg0 is the message
 *
 * arg1 is the line width which defaults to 78
 *
 * arg2 is the padding string which defaults to " " but can be any
 * string.
 * 
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_right(MFUNARGS);

/**
 * MPI function that returns a unix timestamp
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_secs(MFUNARGS);

/**
 * MPI function that picks a value of a single list item from a sparse list
 *
 * A sparse list is one that doesn't have a sequential number of properties.
 * The item chosen is the largest one that is less than or equal to the
 * given value.
 *
 * First MPI argument is the line number value, and is required along with
 * the second argument which is list name.  The third argument is an
 * optional object, that defaults to 'how'.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_select(MFUNARGS);
const char *mfn_set(MFUNARGS);

/**
 * MPI function that provides a conditional based on the numeric expression
 *
 * If expression is negative, returns -1.  If positive, returns 1.  If
 * 0, returns 0.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_sign(MFUNARGS);
const char *mfn_smatch(MFUNARGS);

/**
 * MPI function that returns the most significant time unit of the time period
 *
 * This would return something such as "9d" or "10h" or whatever the
 * most significant portion of the given time period is.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_stimestr(MFUNARGS);

/**
 * MPI function to store a property on an object.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is the property value and is also required.  The third parameter is
 * optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_store(MFUNARGS);

/**
 * MPI function that strips spaces off a string
 *
 * Returns a stripped string
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_strip(MFUNARGS);
const char *mfn_strlen(MFUNARGS);

/**
 * MPI function to return a tune parameter
 *
 * arg0 is the tune parameter key to fetch
 *
 * The player's permission level is the key factor here.  Blessing the
 * prop has no effect on the permission level, however that probably
 * does not matter as most users have read-only permissions to most tune
 * variables as it turns out from my testing. (tanabi)
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_sysparm(MFUNARGS);
const char *mfn_sublist(MFUNARGS);
const char *mfn_subst(MFUNARGS);
const char *mfn_subt(MFUNARGS);

/**
 * MPI function to return a tab
 *
 * Simply returns a string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_tab(MFUNARGS);

/**
 * MPI function to send a message to the given target
 *
 * If only arg0 is provided, then it shows the message to the calling player.
 * If arg1 is provided, the message will be sent to that object.
 *
 * The message will be prefixed with the calling player's name if it doesn't
 * already start with that name, in the following conditions:
 *
 * - what is not a room
 * - owner of 'what' != object
 * - obj != the player
 * - what is an exit and the location != room
 *
 * The string is the return of @see unparse_flags
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_tell(MFUNARGS);
const char *mfn_testlock(MFUNARGS);

/**
 * MPI function that returns a time string for the given time zone.
 *
 * The time returned looks like hh:mm:ss
 *
 * If no timezone is provided, it is the MUCK time, otherwise it
 * uses the numeric offset provided.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_time(MFUNARGS);
const char *mfn_timing(MFUNARGS);

/**
 * MPI function that converts a number of seconds to a WHO-style timestamp
 *
 * Converts seconds to a string that looks like Xd HH:MM  The days will
 * be left off if there isn't a day amount.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_timestr(MFUNARGS);

/**
 * MPI function to select a line from a list based on the time
 *
 * This is rather complicated thing.  Given the first argument, a period
 * which is the number of seconds it would take to cycle through the whole
 * list, and given an offset as argument two which is added to the time
 * stamp and is usually 0.  The third argument is a list name, and the
 * last argument is an optional object that defaults to 'how'.
 *
 * Returns a list item string.  That string is parsed for MPI before being
 * returned.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_timesub(MFUNARGS);
const char *mfn_tolower(MFUNARGS);
const char *mfn_toupper(MFUNARGS);
const char *mfn_type(MFUNARGS);

/**
 * MPI function that returns local time zone offset from GMT in seconds
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_tzoffset(MFUNARGS);

/**
 * MPI function to unbless a property on an object.
 *
 * Blessed props are basically wizard MPI props.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_unbless(MFUNARGS);
const char *mfn_usecount(MFUNARGS);
const char *mfn_v(MFUNARGS);

/**
 * MPI function to return the MUCK version.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_version(MFUNARGS);

/**
 * MPI function to implement while loop
 *
 * The first argument is the check statement.  If true, it runs the
 * second argument.  If false, it breaks out of the loop.
 *
 * There is an iteration limi of MAX_MFUN_LIST_LEN
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_while(MFUNARGS);
const char *mfn_with(MFUNARGS);

/**
 * MPI function computes the exclusive-or (xor) of the two arguments
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *mfn_xor(MFUNARGS);

/*
 * This structure defines a single MPI function and is used by the list of
 * MPI functions.
 *
 * A note for parsep; if true, the arguments will be automatically parsed
 * (If I'm reading the code right).  Otherwise, they will not be parsed.
 * The reason for this appears to be to implement calls like {and:...} which
 * will parse each argument one at a time on its own to enable "short
 * circuiting" and make the parsing stop when a false is found.
 *
 * It doesn't look like 'postp' is used by anything but it appears to process
 * the return value of the function.
 */
struct mfun_dat {
    char *name;                     /* Function name, all upper case */
    const char *(*mfn) (MFUNARGS);  /* Function implementation */
    short parsep;                   /* If true, the arguments will be parsed */
    short postp;                    /* If true, auto-parse the return value */
    short stripp;                   /* If true, strip spaces of arguments */
    short minargs;                  /* Minimum number of arguments */
    short maxargs;                  /* Maximum number of arguments */
};

/**
 * @var The map of MPI functions names to function calls.
 *
 * Because this is kind of a huge list, to see documentation of each
 * call, look at the documentation for the functions themselves rather
 * than the entries here.
 */
static struct mfun_dat mfun_list[] = {
    {"ABS", mfn_abs, 1, 0, 1, 1, 1},
    {"ADD", mfn_add, 1, 0, 1, 2, 9},
    {"AND", mfn_and, 0, 0, 1, 2, 9},
    {"ATTR", mfn_attr, 1, 0, 1, 2, 9},
    {"AWAKE", mfn_awake, 1, 0, 1, 1, 1},
    {"BLESS", mfn_bless, 1, 0, 1, 1, 2},
    {"CENTER", mfn_center, 1, 0, 0, 1, 3},
    {"COMMAS", mfn_commas, 0, 0, 0, 1, 4},
    {"CONCAT", mfn_concat, 1, 0, 1, 1, 2},
    {"CONTAINS", mfn_contains, 1, 0, 1, 1, 2},
    {"CONTENTS", mfn_contents, 1, 0, 1, 1, 2},
    {"CONTROLS", mfn_controls, 1, 0, 1, 1, 2},
    {"CONVSECS", mfn_convsecs, 1, 0, 1, 1, 1},
    {"CONVTIME", mfn_convtime, 1, 0, 1, 1, 1},
    {"COUNT", mfn_count, 1, 0, 0, 1, 2},
    {"CREATED", mfn_created, 1, 0, 1, 1, 1},
    {"DATE", mfn_date, 1, 0, 1, 0, 1},
    {"DBEQ", mfn_dbeq, 1, 0, 1, 2, 2},
    {"DEBUG", mfn_debug, 0, 0, 0, 1, 1},
    {"DEBUGIF", mfn_debugif, 0, 0, 0, 2, 2},
    {"DEC", mfn_dec, 1, 0, 1, 1, 2},
    {"DEFAULT", mfn_default, 0, 0, 0, 2, 2},
    {"DELAY", mfn_delay, 1, 0, 1, 2, 2},
    {"DELPROP", mfn_delprop, 1, 0, 1, 1, 2},
    {"DESCR", mfn_descr, 1, 0, 1, 0, 0},
    {"DICE", mfn_dice, 1, 0, 1, 1, 3},
    {"DIST", mfn_dist, 1, 0, 1, 2, 6},
    {"DIV", mfn_div, 1, 0, 1, 2, 9},
    {"ESCAPE", mfn_escape, 1, 0, 0, 1, 1},
    {"EQ", mfn_eq, 1, 0, 0, 2, 2},
    {"EVAL", mfn_eval, 1, 0, 0, 1, 1},
    {"EVAL!", mfn_evalbang, 1, 0, 0, 1, 1},
    {"EXEC", mfn_exec, 1, 0, 1, 1, 2},
    {"EXEC!", mfn_execbang, 1, 0, 1, 1, 2},
    {"EXITS", mfn_exits, 1, 0, 1, 1, 1},
    {"FILTER", mfn_filter, 0, 0, 0, 3, 5},
    {"FLAGS", mfn_flags, 1, 0, 1, 1, 1},
    {"FOLD", mfn_fold, 0, 0, 0, 4, 5},
    {"FOR", mfn_for, 0, 0, 0, 5, 5},
    {"FORCE", mfn_force, 1, 0, 1, 2, 2},
    {"FOREACH", mfn_foreach, 0, 0, 0, 3, 4},
    {"FOX", mfn_fox, 0, 0, 0, 0, 0},
    {"FTIME", mfn_ftime, 1, 0, 0, 1, 3},
    {"FULLNAME", mfn_fullname, 1, 0, 1, 1, 1},
    {"FUNC", mfn_func, 0, 0, 1, 2, 9},	/* define a function */
    {"GE", mfn_ge, 1, 0, 1, 2, 2},
    {"GT", mfn_gt, 1, 0, 1, 2, 2},
    {"HOLDS", mfn_holds, 1, 0, 1, 1, 2},
    {"IDLE", mfn_idle, 1, 0, 1, 1, 1},
    {"IF", mfn_if, 0, 0, 0, 2, 3},
    {"INC", mfn_inc, 1, 0, 1, 1, 2},
    {"INDEX", mfn_index, 1, 0, 1, 1, 2},
    {"INDEX!", mfn_indexbang, 1, 0, 1, 1, 2},
    {"INSTR", mfn_instr, 1, 0, 0, 2, 2},
    {"ISDBREF", mfn_isdbref, 1, 0, 1, 1, 1},
    {"ISNUM", mfn_isnum, 1, 0, 1, 1, 1},
    {"ISTYPE", mfn_istype, 1, 0, 1, 2, 2},
    {"KILL", mfn_kill, 1, 0, 1, 1, 1},
    {"LASTUSED", mfn_lastused, 1, 0, 1, 1, 1},
    {"LCOMMON", mfn_lcommon, 1, 0, 0, 2, 2},	/* items in both 1 & 2 */
    {"LE", mfn_le, 1, 0, 1, 2, 2},
    {"LEFT", mfn_left, 1, 0, 0, 1, 3},
    {"LEXEC", mfn_lexec, 1, 0, 1, 1, 2},
    {"LINKS", mfn_links, 1, 0, 1, 1, 1},
    {"LIST", mfn_list, 1, 0, 1, 1, 2},
    {"LISTPROPS", mfn_listprops, 1, 0, 1, 1, 3},
    {"LIT", mfn_lit, 0, 0, 0, 1, -1},
    {"LMEMBER", mfn_lmember, 1, 0, 0, 2, 3},
    {"LOC", mfn_loc, 1, 0, 1, 1, 1},
    {"LOCKED", mfn_locked, 1, 0, 1, 2, 2},
    {"LRAND", mfn_lrand, 1, 0, 0, 1, 2},	/* returns random list item */
    {"LREMOVE", mfn_lremove, 1, 0, 0, 2, 2},	/* items in 1 not in 2 */
    {"LSORT", mfn_lsort, 0, 0, 0, 1, 4},	/* sort list items */
    {"LT", mfn_lt, 1, 0, 1, 2, 2},
    {"LTIMESTR", mfn_ltimestr, 1, 0, 1, 1, 1},
    {"LUNION", mfn_lunion, 1, 0, 0, 2, 2},	/* items from both */
    {"LUNIQUE", mfn_lunique, 1, 0, 0, 1, 1},	/* make items unique */
    {"MAX", mfn_max, 1, 0, 1, 2, 2},
    {"MIDSTR", mfn_midstr, 1, 0, 0, 2, 3},
    {"MIN", mfn_min, 1, 0, 1, 2, 2},
    {"MKLIST", mfn_mklist, 1, 0, 0, 0, 9},
    {"MOD", mfn_mod, 1, 0, 1, 2, 9},
    {"MODIFIED", mfn_modified, 1, 0, 1, 1, 1},
    {"MONEY", mfn_money, 1, 0, 1, 1, 1},
    {"MUCKNAME", mfn_muckname, 0, 0, 0, 0, 0},
    {"MUF", mfn_muf, 1, 0, 0, 2, 2},
    {"MULT", mfn_mult, 1, 0, 1, 2, 9},
    {"NAME", mfn_name, 1, 0, 1, 1, 1},
    {"NE", mfn_ne, 1, 0, 0, 2, 2},
    {"NEARBY", mfn_nearby, 1, 0, 1, 1, 2},
    {"NL", mfn_nl, 0, 0, 0, 0, 0},
    {"NOT", mfn_not, 1, 0, 1, 1, 1},
    {"NULL", mfn_null, 1, 0, 0, 0, 9},
    {"ONLINE", mfn_online, 0, 0, 0, 0, 0},
    {"ONTIME", mfn_ontime, 1, 0, 1, 1, 1},
    {"OR", mfn_or, 0, 0, 1, 2, 9},
    {"OTELL", mfn_otell, 1, 0, 0, 1, 3},
    {"OWNER", mfn_owner, 1, 0, 1, 1, 1},
    {"PARSE", mfn_parse, 0, 0, 0, 3, 5},
    {"PRONOUNS", mfn_pronouns, 1, 0, 0, 1, 2},
    {"PROP", mfn_prop, 1, 0, 1, 1, 2},
    {"PROP!", mfn_propbang, 1, 0, 1, 1, 2},
    {"PROPDIR", mfn_propdir, 1, 0, 1, 1, 2},
    {"RAND", mfn_rand, 1, 0, 1, 1, 2},
    {"RIGHT", mfn_right, 1, 0, 0, 1, 3},
    {"REF", mfn_ref, 1, 0, 1, 1, 1},
    {"REVOKE", mfn_revoke, 0, 0, 0, 1, 1},
    {"SECS", mfn_secs, 0, 0, 0, 0, 0},
    {"SELECT", mfn_select, 1, 0, 1, 2, 3},
    {"SET", mfn_set, 1, 0, 0, 2, 2},
    {"SIGN", mfn_sign, 1, 0, 1, 1, 1},
    {"SMATCH", mfn_smatch, 1, 0, 0, 2, 2},
    {"STIMESTR", mfn_stimestr, 1, 0, 1, 1, 1},
    {"STORE", mfn_store, 1, 0, 1, 2, 3},
    {"STRIP", mfn_strip, 1, 0, 0, 1, -1},
    {"STRLEN", mfn_strlen, 1, 0, 0, 1, 1},
    {"SYSPARM", mfn_sysparm, 1, 0, 1, 1, 1},
    {"SUBLIST", mfn_sublist, 1, 0, 0, 1, 4},
    {"SUBST", mfn_subst, 1, 0, 0, 3, 3},
    {"SUBT", mfn_subt, 1, 0, 1, 2, 9},
    {"TAB", mfn_tab, 0, 0, 0, 0, 0},
    {"TESTLOCK", mfn_testlock, 1, 0, 1, 2, 4},
    {"TELL", mfn_tell, 1, 0, 0, 1, 2},
    {"TIME", mfn_time, 1, 0, 1, 0, 1},
    {"TIMING", mfn_timing, 0, 0, 0, 1, 1},
    {"TIMESTR", mfn_timestr, 1, 0, 1, 1, 1},
    {"TIMESUB", mfn_timesub, 1, 0, 1, 3, 4},
    {"TOLOWER", mfn_tolower, 1, 0, 0, 1, 1},
    {"TOUPPER", mfn_toupper, 1, 0, 0, 1, 1},
    {"TYPE", mfn_type, 1, 0, 1, 1, 1},
    {"TZOFFSET", mfn_tzoffset, 0, 0, 0, 0, 0},
    {"UNBLESS", mfn_unbless, 1, 0, 1, 1, 2},
    {"USECOUNT", mfn_usecount, 1, 0, 1, 1, 1},
    {"V", mfn_v, 1, 0, 1, 1, 1},	/* variable value */
    {"VERSION", mfn_version, 0, 0, 0, 0, 0},
    {"WHILE", mfn_while, 0, 0, 0, 2, 2},
    {"WITH", mfn_with, 0, 0, 0, 3, 9},	/* declares var & val */
    {"XOR", mfn_xor, 1, 0, 1, 2, 2},	/* logical XOR */

    {NULL, NULL, 0, 0, 0, 0, 0}	/* Ends the mfun list */
};

#endif /* !MFUN_H */
