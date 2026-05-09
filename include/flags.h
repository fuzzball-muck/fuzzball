/** @file flags.h
 *
 * Header that supports the MUCK's object flag and type system.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef FLAGS_H
#define FLAGS_H

#include <stdbool.h>

#include "config.h"

/**
 * Definitions of the bit used in object flags.
 */
#define TYPE_ROOM           0x0 /**< room bit */
#define TYPE_THING          0x1 /**< thing bit */
#define TYPE_EXIT           0x2 /**< exit bit */
#define TYPE_PLAYER         0x3 /**< player bit */
#define TYPE_PROGRAM        0x4 /**< program bit */

/* 0x5 available */

#define TYPE_GARBAGE        0x6 /**< garbage bit */
#define TYPE_ANY            0x7 /**< no particular type */
#define TYPE_MASK           0x7 /**< bitmask for all types */

/* 0x8 available */

#define WIZARD             0x10 /**< gets automatic control */
#define LINK_OK            0x20 /**< anybody can link to this */
#define DARK               0x40 /**< contents of room are not printed */
#define INTERNAL           0x80 /**< internal-use-only flag */
#define STICKY            0x100 /**< this object goes home when dropped */
#define BUILDER           0x200 /**< this player can use construction commands */
#define CHOWN_OK          0x400 /**< allow ownership transfer or enable color */
#define JUMP_OK           0x800 /**< player and location config for jump commands */

/* 0x1000 available */
/* 0x2000 available */

#define KILL_OK          0x4000 /**< Kill_OK bit.  Means you can be killed. */
#define GUEST            0x8000 /**< Guest flag */
#define HAVEN           0x10000 /**< can't kill here */
#define ABODE           0x20000 /**< can set home here */
#define MUCKER          0x40000 /**< programmer */
#define QUELL           0x80000 /**< When set, wiz-perms are turned off */
#define SMUCKER        0x100000 /**< second programmer bit.  For levels */
#define INTERACTIVE    0x200000 /**< internal: player in MUF editor */
#define OBJECT_CHANGED 0x400000 /**< internal: set when an object is dbdirty()ed */

/* 0x800000 available */

#define VEHICLE       0x1000000 /**< Vehicle flag */
#define ZOMBIE        0x2000000 /**< Zombie flag */
#define LISTENER      0x4000000 /**< internal: listener flag */
#define XFORCIBLE     0x8000000 /**< externally forcible flag */
#define READMODE     0x10000000 /**< internal: when set, player is in a READ */
#define SANEBIT      0x20000000 /**< internal: used to check db sanity */
#define YIELD        0x40000000 /**< Yield flag */
#define OVERT        0x80000000 /**< Overt flag */

/** what flags to NOT dump to disk. */
#define DUMP_MASK   (INTERACTIVE | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)

/**
 * Returns the TYPE_ value of 'ref'
 *
 * Does not check to see if 'ref' is a valid ref.
 *
 * @param ref the db object to get type of
 * @return a type value - one of the TYPE_* constants
 */
#define OBJECT_TYPE(ref)   (ref == HOME ? TYPE_ROOM : (FLAGS(ref) & TYPE_MASK))

/**
 * Returns a pointer to the object_type struct for the given dbref.
 * 
 * Does not check to see if 'ref' is a valid ref.
 *
 * @param ref the db object to get type info for
 * @return the related object_type struct
 */
#define OBJECT_TYPE_INFO(ref) (&object_types[OBJECT_TYPE(ref)])

/**
 * The MUCKER level of the object, ignoring the WIZARD bit.
 *
 * Does not check to see if 'ref' is a valid object.
 *
 * @param ref the object to check
 * @return the MUCKER level of the object (0-3).
 */
#define OBJECT_MLEVEL(ref) \
    ((FLAG_CHECK((ref), 'M') ? 2 : 0) + (FLAG_CHECK((ref), 'N') ? 1 : 0))

/**
 * The effective MUCKER level of the object, including the WIZARD bit.
 *
 * Does not check to see if 'ref' is a valid object.
 *
 * @param ref the object to check
 * @return the effective MUCKER level of the object (0-4).
 */
#define OBJECT_EFFECTIVE_MLEVEL(ref) \
    ((OBJECT_MLEVEL(ref) > 0 && TrueWizard(ref)) ? 4 : OBJECT_MLEVEL(ref))

/**
 * Returns the priority level of the object, for exit matching. Objects set 'A'
 * with no MUCKER level are lowest priority.
 *
 * Does not check to see if 'ref' is a valid object.
 *
 * @param ref the object to check
 * @return  the priority level of the object (0-4).
 */
#define OBJECT_PRIORITY(ref) \
    ((OBJECT_MLEVEL(ref) > 0) ? (OBJECT_MLEVEL(ref) + 1) : (FLAG_CHECK((ref), 'A') ? 0 : 1))

/**
 * This structure and array define the fundamental object types within the
 * database and their associated display strings.
 */
struct object_type {
    char symbol;
    char *uppercase;
    char *titlecase;
    char *singular;
    char *plural;
};

extern struct object_type object_types[];

typedef long object_flag_type;  /**< Object flag type - need 32+ bits */

/**
 * Callback to determine flag management permissions. Currently unused.
 */
typedef bool (*fn_can_set)(int type, char flag, dbref player);

/**
 * Interface and permission data for a symbol / object type pair.
 */
struct flag_data {
    char symbol;
    const char *display_name;
    fn_can_set can_set;
    bool hidden;
};

/**
 * Processed flag context for lookup tables.
 */
struct flag_entry {
    object_flag_type value;
    char symbol;
    struct flag_data data;
};

#define NUM_SYMBOLS 26

/**
 * Flag lookup table. Declared here so it can be used by FLAG_VALUE().
 */
extern struct flag_entry flag_lists_by_symbol[TYPE_ANY+1][NUM_SYMBOLS];

/**
 * Resolves a flag symbol to its corresponding bitmask value.
 *
 * Uses the default TYPE_ANY value for translation.
 *
 * @param c the character symbol to look up (e.g., 'W')
 * @return  the object_flag_type bitmask value
 */
#define FLAG_VALUE(c) flag_lists_by_symbol[TYPE_ANY][(c) - 'A'].value

/**
 * Evaluates whether a specific flag is set on an object.
 *
 * Does not check to see if 'ref' is a valid ref.
 *
 * @param ref the object dbref to check
 * @param c   the character symbol of the flag
 * @return    a boolean representation (0 or 1) of the flag's state
 */
#define FLAG_CHECK(ref, c) (!!(db[(ref)].flags & FLAG_VALUE(c)))

/**
 * Initializes the flag lookup tables from the raw flag list.
 */
void flag_init(void);

/**
 * Evaluates a single flag string against an object.
 *
 * Recognizes flag symbols, names, and virtual flag 'truewizard'.
 *
 * @param ref  The object to check.
 * @param p    The flag string (e.g., "W", "!W", "Wizard").
 * @return     True if the object satisfies the expression.
 */
bool flag_eval(dbref ref, const char *spec);

/**
 * Evaluates a pattern of flag symbols against an object.
 *
 * Recognizes type symbols, flag symbols, and MUCKER levels.
 *
 * @param ref  The object to check.
 * @param p    The pattern string (e.g., "PW3" or "J!A").
 * @return     True if all conditions in the pattern match.
 */
bool flag_eval_pattern(dbref ref, const char *pattern);

/**
 * Generates a string representation of object flags. Uses the provided
 * buffer that has the given size.
 *
 * @param thing the object to construct a flag string for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void flag_list(dbref ref, char *buf, size_t size);

/**
 * Generate a long-form text description of the flags on a given object.
 * Uses the provided buffer that has the given size.
 *
 * @param thing to generate description for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void flag_list_verbose(dbref ref, char *buf, size_t size);

/**
 * "Unparse" an object, showing its name and list of flags if permissions allow
 *
 * Uses the provided buffer that has the given size.  Practically speaking,
 * names can be as long as BUFFER_LEN, so your buffer should probably
 * be BUFFER_LEN at least in size (This is the most common practice).  If
 * a name was actually its maximum length, then there is not enough room
 * for flags to show up.  Traditionally, Fuzzball has not cared about this
 * problem because, traditionally, names just don't get that long.
 *
 * Flags are only shown if:
 *
 * * player == NOTHING
 * * or player does not have the STICKY flag AND:
 *   * 'loc' is linkable - @see can_link_to
 *   * or 'loc' is not a player and 'player' controls 'loc'
 *   * or 'loc' is CHOWN_OK
 *
 * @param player the player doing the call or NOTHING
 * @param object the target to generate unparse text for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void flag_unparse_object(dbref player, dbref ref, char *buf, size_t size);

/**
 * Returns the flag associated with the given string, if any.
 *
 * Understands flag alias prefixes.
 * Passing "truewizard" here just returns the WIZARD flag.
 *
 * @param ref the object to check
 * @param flag_string the flag (or alias) to check
 * @return the flag corresponding to the string, or 0 if none match.
 */
object_flag_type str_to_flag(const char *flag_string);

/**
 * Determines if a given player is unable to (re)set the given flag
 *
 * The 'business logic' here is actually fairly dense and not easily
 * summed up in a comment. The reader is encouraged to review the very
 * well documented code for details.
 *
 * This is all pretty well documented in the MUCK's help files.
 *
 * Uses an error parameter to communicate the reason for failure. This
 * should be at least SMALL_BUFFER_LEN in size.
 *
 * @private
 * @param player the effective aplayer we are checking permissions for
 * @param mlev the effective mucker level we are checking permissions for
 * @param thing the thing we want to interact with
 * @param flag the flag we wish to interact with
 * @param value whether the desired state is on or off
 * @param error[out] error message, if any
 * @return boolean 1 if restricted from setting flag, 0 if okay to set.
 */
int unable_to_set_flag(dbref player, int mlev, dbref thing, object_flag_type flag, bool value, char *error);
#endif /* !FLAGS_H */
