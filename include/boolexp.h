/** @file boolexp.h
 *
 * Header for handling "boolean expressions", which is to say, the lock
 * type in Fuzzball.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef BOOLEXP_H
#define BOOLEXP_H

#include <stddef.h>

#include "config.h"
#include "props.h"

/* These are the different elements of a boolean expression */
#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_PROP 4

/* True is a null boolean expression */
#define TRUE_BOOLEXP ((struct boolexp *) 0)

/* Boolean expressions are trees of expression nodes that are recursively
 * evaluated.
 *
 * Leaf nodes are always either BOOLEXP_PROP or BOOLEXP_CONST.
 */
struct boolexp {
    short type;                 /* Type of node               */
    struct boolexp *sub1;       /* 'Left' side subexpression  */
    struct boolexp *sub2;       /* 'Right' side subexpression */
    dbref thing;                /* Value for type constant    */
    struct plist *prop_check;   /* Value for type property    */
};

/**
 * Make a copy of a boolexp structure
 *
 * This is pretty straight forward; copies the entire boolexp structure
 * 'old', recursively.
 *
 * @param old the structure to copy
 * @return the copied structure -- be sure to use free_boolexp on it.
 */
struct boolexp *copy_bool(struct boolexp *old);

/**
 * Evaluate a boolean expression.
 *
 * How property checking works is somewhat complicated;
 * has_property_strict is called with the player and the 'thing' first,
 * then has_property is called with the player and the player second.
 * Only string props are supported.
 *
 * @see has_property_strict
 * @see has_property
 *
 * If a BOOLEXP_CONST is a dbref that belongs to a program, that program
 * is evaluated and as longa s it returns something other '0' or nothing
 * it will be counted as true.  Otherwise it looks for that DBREF in
 * a variety of ways (checks to see if its the player, checks to see if
 * its an object the player has, checks to see if its the player's location)
 *
 * Zombies are understood by this call and the owner of the zombie is
 * used in most places for locking purposes.
 *
 * This call temporarily copies the b boolexp structure to ensure the
 * original is not mutated.  I'm not sure if this is strictly necessary,
 * but that is how it is presently written.
 *
 * @param descr the descriptor that initiated the lock evaluation
 * @param player the person or thing that triggered the lock evaluation
 * @param b the boolexp node we are working on
 * @param thing the thing the lock is on.
 * @return boolean - 1 if lock checks out, 0 if it does not
 */
int eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing);

/**
 * Recursively free a boolean expression structure
 *
 * This will clear all the child nodes, and any property node values
 * if necessary.
 *
 * @param b the structure to free
 */
void free_boolexp(struct boolexp *b);

/**
 * Parse a boolean expression string into a boolexp object
 *
 * This essentially "compiles" a lock expression.  Some notes:
 *
 * * Ultimately, this only understands prop key-value pairs (values cannot
 *   have spaces), MUF return values, and DBREFs.  The syntax is very
 *   basic.
 *
 * * The expressions can have parenthesis grouping ( ) and it understands
 *   the logical operators & (and), | (or), and ! (not)
 *
 * * You should pass 0 to dbloadp unless you are a very special case.
 *   It is just used by the property disk loader.
 *
 * @param descr the descriptor belonging to the person compiling the lock
 * @param player the person or thing compiling the lock
 * @param buf the lock string
 * @param dbloadp 1 if being called by the property disk loader, 0 otherwise
 * @return struct boolexp - the "head" of the boolean expression node tree.
 *         or TRUE_BOOLEXP on failure.
 */
struct boolexp *parse_boolexp(int descr, dbref player, const char *string, int dbloadp);

/**
 * Recursively compute the in-memory byte size of boolexp *b
 *
 * @param b the boolexp to compute the size on
 * @return the size in bytes
 */
size_t size_boolexp(struct boolexp *b);

/**
 * Test a lock - shortcut for a prop load and eval_boolexp
 *
 * This loads a prop 'lockprop' from 'thing' and tests it against 'player'.
 *
 * @see eval_boolexp
 *
 * @param descr the descriptor of the person triggering the lock
 * @param player the person or object triggering the lock
 * @param thing the thing the lock is on
 * @param lockprop the name of the lock property
 *
 * @return boolean result of eval_boolexp (1 for pass, 0 for fail)
 */
int test_lock(int descr, dbref player, dbref thing, const char *lockprop);

/**
 * Test a lock - if the lock does not exist, return failure.
 *
 * This loads a prop 'lockprop' from 'thing' and tests it against 'player'.
 * Unlike test_lock, if there is no lock property on the object, we will
 * return failure (false)
 *
 * @see eval_boolexp
 * @see test_lock
 *
 * @param descr the descriptor of the person triggering the lock
 * @param player the person or object triggering the lock
 * @param thing the thing the lock is on
 * @param lockprop the name of the lock property
 *
 * @return boolean result of eval_boolexp (1 for pass, 0 for fail)
 */
int test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop);

/**
 * Translate a boolexp structure into something human-readable
 *
 * Please note that this uses a static buffer of size BUFLEN, so you
 * must be aware that the buffer can mutate under you if another
 * unparse_boolexp happens.  Also, this means the call is not threadsafe.
 *
 * If fullname is true, then DBREFS will be run through unparse_object.
 * Otherwise, #12345 style DBREFs will be displayed.
 *
 * @see unparse_object
 *
 * @param player the player who is going to see the output of this command
 * @param b the boolean expression to process
 * @param fullname Show names instead of DBREFs?  True or false.
 *
 * @return pointer to static buffer containing string for display.
 */ 
const char *unparse_boolexp(dbref player, struct boolexp *b, int fullname);

#endif /* !BOOLEXP_H */
