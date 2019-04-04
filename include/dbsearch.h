/** @file dbsearch.h
 *
 * Header for declaring the "checkflags" search system that is used by
 * @find, @entrances, etc.  The definitions for these calls are in
 * look.c
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef DBSEARCH_H
#define DBSEARCH_H

#include <stddef.h>

#include "config.h"

struct flgchkdat {
    int fortype;        /* check FOR a type? */
    int istype;         /* If check FOR a type, which one? */
    int isnotroom;      /* not a room. */
    int isnotexit;      /* not an exit. */
    int isnotthing;     /* not type thing */
    int isnotplayer;    /* not a player */
    int isnotprog;      /* not a program */
    int forlevel;       /* check for a mucker level? */
    int islevel;        /* if check FOR a mucker level, which level? */
    int isnotzero;      /* not ML0 */
    int isnotone;       /* not ML1 */
    int isnottwo;       /* not ML2 */
    int isnotthree;     /* not ML3 */
    int setflags;       /* flags that are set to check for */
    int clearflags;     /* flags to check are cleared. */
    int forlink;        /* check linking? */
    int islinked;       /* if yes, check if not unlinked */
    int forold;         /* check for old object? */
    int isold;          /* if yes, check if old */
    int loadedsize;     /* check for propval-loaded size? */
    int issize;         /* list objs larger than size? */
    size_t size;        /* what size to check against. No check if 0 */
};

/**
 * Checks an object 'what' against a set of flags 'check'
 *
 * This is defined in look.c
 *
 * 'check' should be initialized with init_checkflags.  Then you
 * can pass 'what' in to check that object against the flags.
 *
 * @see init_checkflags
 *
 * @param what the object to check
 * @param check the flags to check -- see init_checkflags
 * @return boolean - 1 if object matches, 0 if not
 */
int checkflags(dbref what, struct flgchkdat check);

/**
 * Initialize the checkflags search system
 *
 * This is defined in look.c
 *
 * This is the underpinning of @find, @owned, @entrance, and a few
 * other similar calls.  It parses over a set of 'flags' to consider,
 * and loads the struct 'check' with the necessary filter paramters.
 *
 * Flags is parsed as follows; it may have an = in it, in which case
 * the "right" side of the = is 
 *
 * owners (1), locations (3), links (2), count (4), size (5) or default (0)
 *
 * The numbers are the "ID"s associated with each type, and are the numbers
 * returned by this function.
 *
 * On the left side are the flags to check for.  Flags can be the first
 * letter of any flag (like A for ABODE, B for BUILDER, etc.), a number
 * for MUCKER levels, or a type: E(xit), (MU)F, G(arbage), P(layer),
 * R(oom), and T(hing).  God help us if we ever have an overlap in
 * flag names and types.
 *
 * It can also check U(nlinked), @ (objects older than about 90 days old),
 * ~size (match greater than or equal to current memory usage , must be
 * last modifier) or ^size  (match greater than or equal to total memory
 * usage - wizard only, uses ~size for regular players).
 *
 * @param player the player doing the checking
 * @param flags a string containing the flags and output format
 * @param check the flgchkdat object to initialize
 * @return output type integer
 */
int init_checkflags(dbref player, const char *flags, struct flgchkdat *check);

#endif /* !DBSEARCH_H */
