/** @file fbmuck.h
 *
 * This .... has the definition for the 'dbref' type. Why is this in its own
 * header again?  I suspect it is defined this way because tunelist.h
 * contains a lot of stuff that should probably be in a .c file and thus
 * we need this one type separated from everything else.  Just a hunch,
 * though.
 *
 * @todo this really doesn't need its own headerfile.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef FBMUCK_H
#define FBMUCK_H

typedef int dbref;  /**< Type wrapper for dbref - must support negatives */

#endif /* !FBMUCK_H */
