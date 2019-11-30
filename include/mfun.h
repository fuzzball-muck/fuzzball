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

/**
 * MPI function that adds ANSI attributes to a string
 *
 * This takes a variable number of arguments, with a minimum of two.
 * The last argument will be the string.  The arguments preceeding the last
 * one are attributs which will be prefixed to the string.  The string will
 * always have have a RESET after it to clear all attributes.
 *
 * The attributs can be the following strings:
 *
 * bold, dim, italic, uline, underline, flash, reverse, ostrike, overstrike,
 * black, red, yellow, green, cyan, blue, magenta, white, bg_black,
 * bg_red, bg_yellow, bg_green, bg_cyan, bg_blue, bg_magenta, bg_white
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
const char *mfn_attr(MFUNARGS);

/**
 * MPI function that does returns connection count for the given player
 *
 * If arg0 isn't a player, this returns 0, which is the same as if the
 * a player is not connected.
 *
 * @see string_substitute
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

/**
 * MPI function that turns a list into a comma delimited string
 *
 * arg0 is a list.  By default, it will make a string that looks like:
 *
 * Item1, Item2, and Item3
 *
 * You can control the "and" by providing arg1 as a subtitution.  arg2
 * and arg3 may be provided in order to evaluate an expression for each
 * item.  The item will be placed in the variable named arg2 and the
 * expression arg3 will be run.
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

/**
 * MPI function that returns an optionally filtered contents list
 *
 * Returns the contents as a \r delimited list of refs.  Player refs
 * appear to be *Name whereas everything else is a dbref.
 *
 * @see ref2str
 *
 * arg0 is the object to get contents for, and arg1 is an optional
 * type specifier which can be "room", "exit", "player", "program",
 * or "thing".
 *
 * There are a number of constraints here; MAX_MFUN_LIST_LEN is the
 * maximum contents items that can be returned, and the entire list
 * of contents must fit in BUFFER_LEN space.  Also if player name length
 * is 50 characters or longer, it will get truncated, but that seems like
 * an unlikely situation.  If your MUCK allows that, you probably have
 * bigger problems.
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
const char *mfn_contents(MFUNARGS);

/**
 * MPI function that returns 1 if player controls arg0, 0 otherwise
 *
 * You can also provide arg1 as an alternate player to check control
 * permissions for, though the default is the calling player.
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

/**
 * MPI function that counts number of items in list arg0
 *
 * arg0 is a string containing a delimited list.  arg1 is a
 * delimiter/separator string and defaults to \r
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

/**
 * MPI function that runs code in debug mode
 *
 * arg0 will be run with MPI_ISDEBUG flag set.
 *
 * @see mesg_parse
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
const char *mfn_debug(MFUNARGS);

/**
 * MPI function that runs code in debug mode if the conditional returns true
 *
 * arg0 is the conditional expression.  If true, arg1 will be run with the
 * MPI_ISDEBUG flag set.  Otherwise, it will be run normally.
 *
 * @see mesg_parse
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

/**
 * MPI function that puts some MPI on the timequeue to run after a bit
 *
 * arg0 is the number of seconds to wait before execution.  arg1 is an
 * expression; the result of that expression is what is put on the timequeue,
 * so it will probably need to be your code in a {lit:...} block.
 *
 * It will return the process ID.
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

/**
 * MPI function that escapes a string in back-tick quotes
 *
 * This surrounds arg0 in backticks (`) and escapes any back-ticks that
 * are in the string using backslash.  It will also escape backslashes.
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

/**
 * MPI function that returns a list of exits associatd with arg0
 *
 * Returns the exits as a \r delimited list of refs.  Player refs
 * appear to be *Name whereas everything else is a dbref.
 *
 * @see ref2str
 *
 * There are a number of constraints here; MAX_MFUN_LIST_LEN is the
 * maximum contents items that can be returned, and the entire list
 * of contents must fit in BUFFER_LEN space.  Also if player name length
 * is 50 characters or longer, it will get truncated, but that seems like
 * an unlikely situation.  If your MUCK allows that, you probably have
 * bigger problems.
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
const char *mfn_exits(MFUNARGS);

/**
 * MPI function that runs an expression against every item in a list and filters
 *
 * This operates in a variable context similar to {with}
 *
 * @see mfn_with
 * @see mfn_foreach
 *
 * This is similar to {foreach} except it returns a list and is more
 * for morphing a list rather than simply iterating over that list.
 * The return list will only have items where the list expression returned
 * true.
 *
 * Each item of the list is put in the variable named arg0, with the list
 * being in arg1, and the expression to run against each line in the list
 * being arg2.  An optional separator is in arg3, which defaults to \r
 *
 * arg4 can be provided to make the returned list use a different seperator
 * (arg4) than the source list.  If arg4 is left off, it will use the same
 * separator as the source list.
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

/**
 * MPI function that takes a list, and evaluates a an expression with variables
 *
 * Arguments arg0 and arg1 are variable names; they will be populated with
 * the first two items of the list provided in arg3.  arg4 is the expression
 * to run.  It is run as a variable context -- similar to {with} as you can
 * see @see mfn_with
 *
 * arg5 is optional and is the separator string.  It defaults to \r
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
const char *mfn_fold(MFUNARGS);

/**
 * MPI function that does a for loop
 *
 * This operates in a variable context similar to {with}
 *
 * @see mfn_with
 *
 * arg0 is the variable name to use.  arg1 is the start value, and
 * arg2 is the end value.  arg3 is the increment amount.  arg4 is
 * the command to run each iteration.
 *
 * Note that this only works with numeric increments that are stored
 * in the variable named arg0
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
const char *mfn_for(MFUNARGS);

/**
 * MPI function that forces an object
 *
 * arg0 is the object to force, and arg1 is the command to run.
 *
 * arg1 can be a list of commands, in which case each command will be run
 * in turn.
 *
 * This obeys the maxiumum force recursion setting (tp_max_force_level)
 *
 * There are many permission restrictions here.  For instance, objects must
 * be XFORCABLE and force-locked to the caller (or the property must be
 * blessed).  Objects with the same names as players cannot be forced.
 * tp_allow_zombies, if false, will disable this feature.
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
const char *mfn_force(MFUNARGS);

/**
 * MPI function that does a foreach loop
 *
 * This operates in a variable context similar to {with}
 *
 * @see mfn_with
 * @see mfn_for
 *
 * This iterates over every item in a list, storing that list item
 * in a variable that is available to the executed code.
 *
 * arg0 is the variable name to use.  arg1 is the list to iterate over.
 * arg2 is the expression to run for each iteration of the loop.  arg3
 * is an optional seperator string that defaults to \r
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
const char *mfn_foreach(MFUNARGS);

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

/**
 * MPI function that returns the name of arg0
 *
 * The name is left intact in case of exits.  Otherwise this is identical
 * to {name}.
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

/**
 * MPI function to implement an if/else statement
 *
 * The first argument is the check statement.  If true, it runs the
 * second argument.  If false, it runs the optional third option if it
 * is provided.
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

/**
 * MPI function that finds a substring
 *
 * arg0 is the string to search and arg1 is the string to search for.
 * Returns the first position of the substring found, or 0 if not found.
 * 1 would be the first position.
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

/**
 * MPI function that returns true if arg0 is of type arg1
 *
 * arg1 can be "Room", "Player", "Exit", "Thing", "Program", or "Bad"
 *
 * arg0 should be something that resolves into an object.
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
const char *mfn_istype(MFUNARGS);

/**
 * MPI function that kills a process on the timequeue
 *
 * arg0 is either 0 to kill all processes done by the trigger, or a PID
 * of a specific process.  Returns number of processes killed.
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

/**
 * MPI function that returns a list with list items common to arg0 and arg1
 *
 * All list items that are in both arg0 and arg1 will be returned in a
 * list.  Any duplicates are removed.
 *
 * This will iterate up to MAX_MFUN_LIST_LEN times.
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

/**
 * MPI function that returns the ref of whatever arg0 is linked to
 *
 * For multi-linked exits, this may be a \r delimited list.  For invalid
 * things or unlinked things, this may be #-1
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

/**
 * MPI function that finds anitem in a list
 *
 * arg0 is the list to search, arg1 is the item to find.  Optionally, arg2
 * is a delimiter string.
 *
 * This starts with item '1' rather than item 0 when returning results.
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

/**
 * MPI function that tests the lock of arg1 against arg0
 *
 * Returns 1 if locked, 0 if not locked.
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
const char *mfn_locked(MFUNARGS);

/**
 * MPI function that returns a random item from list arg0
 *
 * arg0 is a string containing a delimited list.  arg1 is a
 * delimiter/separator string and defaults to \r
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
const char *mfn_lrand(MFUNARGS);

/**
 * MPI function that filters one list by another list
 *
 * The list in arg0 has any rows that match a row in arg1 removed and the
 * result is returned.
 *
 * This will iterate up to MAX_MFUN_LIST_LEN times.
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
const char *mfn_lremove(MFUNARGS);

/**
 * MPI function that returns a sorted version of arg0
 *
 * The list is provided in arg0.  If no other arguments are provided,
 * then this function will be used to sort the items: @see alphanum_compare
 *
 * Otherwise, one can define a custom sorter.  arg1 and arg2 are variable
 * names, and arg3 is the expression.  Compare arg1 and arg2, and return
 * true if arg1 and arg2 should be swapped, false otherwise.  This is
 * different from the usual positive/negative/zero sort callback message
 * used by C and most languages.
 *
 * Returns the sorted list.
 *
 * This will iterate up to MAX_MFUN_LIST_LEN times.
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

/**
 * MPI function that returns a list that is arg0 and arg1 combined.
 *
 * The returned list will have all items from arg0 and arg1, with duplicates
 * removed.
 *
 * This will iterate up to MAX_MFUN_LIST_LEN times.
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
const char *mfn_lunion(MFUNARGS);

/**
 * MPI function that returns a version of arg0 with all duplicate rows removed
 *
 * This will iterate up to MAX_MFUN_LIST_LEN times.
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

/**
 * MPI function that returns a substring
 *
 * Takes a string in arg0.  Arg1 is the start position to get a substring.
 * Arg2, if provided, will be the last character to fetch otherwise it will
 * fetch until the end of string.
 *
 * The argments can be negative to start from the end of the string instead.
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

/**
 * MPI function that runs a MUF
 *
 * arg0 is the MUF to run, arg1 is what will be put on the stack.
 * If the trigger was from a propqueue, the MUF must be at least MUCKER3.
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

/**
 * MPI function that returns the name of arg0
 *
 * Exit names are truncated at the first sign of a ';' so that it looks
 * pretty.  Use {fullname} to get the entire name of an object.
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

/**
 * MPI function that returns the ref of the player that owns arg0
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
const char *mfn_owner(MFUNARGS);

/**
 * MPI function that runs an expression against every item in a list
 *
 * This operates in a variable context similar to {with}
 *
 * @see mfn_with
 * @see mfn_foreach
 *
 * This is similar to {foreach} except it returns a list and is more
 * for morphing a list rather than simply iterating over that list.
 * The resulting list consists of the return values of the expression
 * in arg2 against the list items in arg1.
 *
 * Each item of the list is put in the variable named arg0, with the list
 * being in arg1, and the expression to run against each line in the list
 * being arg2.  An optional separator is in arg3, which defaults to \r
 *
 * arg4 can be provided to make the returned list use a different seperator
 * (arg4) than the source list.  If arg4 is left off, it will use the same
 * separator as the source list.
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


/**
 * MPI function that returns the reference of arg0 as a #dbref
 *
 * This will always be a #dbref and doesn't return *Playername like
 * some calls (such as contents).  It is a great way to normlaize things
 * to a ref.
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
const char *mfn_ref(MFUNARGS);

/**
 * MPI function that runs code with no blessing set
 *
 * arg0 will be run "unblessed" even if the MPI is currently blessed.
 *
 * @see mesg_parse
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

/**
 * MPI function that set the contents of a variable arg0 to arg1
 *
 * Variable must be defined first in the current context, which is
 * done with {with} or with certain functions that support variables
 * such as some of the loops.
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

/**
 * MPI function that does a regex match of pattern arg1 against string arg0
 *
 * Just a thin wrapper around equalstr
 *
 * @see equalstr
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

/**
 * MPI function that returns the length of string arg0
 *
 * Just a thin wrapper around strlen
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

/**
 * MPI function that returns a subset of list arg0
 *
 * arg0 is a string containing a delimited list.  arg1 is the starting
 * element to get a subset of, with 0 being the first one.
 *
 * arg2 is optional and is the last item on the list; it defaults to the
 * same value as arg1, and can be less than arg1 to get elements in the
 * reverse direction.
 *
 * arg3 is a delimiter/separator string and defaults to \r
 *
 * As an "undocumented feature" if only arg0 is passed, then the list is
 * just copied as-is.  This is not in the MPI docs.
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
const char *mfn_sublist(MFUNARGS);

/**
 * MPI function that does a string substitution
 *
 * Substitutes all instances of arg1 with arg2 in arg0
 *
 * @see string_substitute
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
const char *mfn_subst(MFUNARGS);

/**
 * MPI function that numerically subtracts up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * subtracts them "from left to right".  Returns the result.
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

/**
 * MPI function that tests a lock property
 *
 * At a minimum needs arg0 to be an object, and arg1 to be a lock property
 * to check.  If arg2 is not provided, the lock will be checked against the
 * calling player.  If arg3 is provided, that will be the default value of
 * the lock is the property doesn't exist, otherwise the return value will
 * be 0 (unlocked) if the property doesn't exist.  arg3 can be anything and
 * is not restricted to being 1 or 0.
 *
 * Returns 1 if locked, 0 if not locked.
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

/**
 * MPI function that times (and displays timing of) some MPI code
 *
 * When arg0 completes, a timing message is displayed.
 *
 * @see mesg_parse
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

/**
 * MPI function that lowercases arg0
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
const char *mfn_tolower(MFUNARGS);

/**
 * MPI function that uppercases arg0
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
const char *mfn_toupper(MFUNARGS);

/**
 * MPI function that does returns a text description of the object arg0
 *
 * Returns "Room", "Player", "Exit", "Thing", "Program", or "Bad" based
 * on the object type.
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
const char *mfn_usecount(MFUNARGS);

/**
 * MPI function that returns the contents of a variable arg0
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

/**
 * MPI function that defines a variable context
 *
 * MPI is kind of weird and variables run within a context in the
 * format of: {with:var_name, var_value, code to run...}
 *
 * These contexts can be nested for multiple variables.  You can't define
 * a variable outside of a context, and you can't set/read a variable
 * outside a context either.
 *
 * There are other "contexts" out there, for instance certain loops and
 * function structures.
 *
 * arg0 is the name of the variable.  arg1 is the initial value of the
 * variable.  arg2 is the code to run, with the created variable available
 * in that code block.  All parameters are required.
 *
 * Probably needless to say, but I will say anyway, the variable is deleted
 * when this function exits.
 *
 * @see new_mvar
 * @see free_top_mvar
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
    {"FTIME", mfn_ftime, 1, 0, 0, 1, 3},
    {"FULLNAME", mfn_fullname, 1, 0, 1, 1, 1},
    {"FUNC", mfn_func, 0, 0, 1, 2, 9},  /* define a function */
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
    {"LCOMMON", mfn_lcommon, 1, 0, 0, 2, 2},    /* items in both 1 & 2 */
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
    {"LRAND", mfn_lrand, 1, 0, 0, 1, 2},    /* returns random list item */
    {"LREMOVE", mfn_lremove, 1, 0, 0, 2, 2},    /* items in 1 not in 2 */
    {"LSORT", mfn_lsort, 0, 0, 0, 1, 4},    /* sort list items */
    {"LT", mfn_lt, 1, 0, 1, 2, 2},
    {"LTIMESTR", mfn_ltimestr, 1, 0, 1, 1, 1},
    {"LUNION", mfn_lunion, 1, 0, 0, 2, 2},  /* items from both */
    {"LUNIQUE", mfn_lunique, 1, 0, 0, 1, 1},    /* make items unique */
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
    {"V", mfn_v, 1, 0, 1, 1, 1},    /* variable value */
    {"VERSION", mfn_version, 0, 0, 0, 0, 0},
    {"WHILE", mfn_while, 0, 0, 0, 2, 2},
    {"WITH", mfn_with, 0, 0, 0, 3, 9},  /* declares var & val */
    {"XOR", mfn_xor, 1, 0, 1, 2, 2},    /* logical XOR */

    {NULL, NULL, 0, 0, 0, 0, 0} /* Ends the mfun list */
};

#endif /* !MFUN_H */
