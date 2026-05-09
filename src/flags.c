/** @file flags.c
 *
 * Implementation of the MUCK's object flag and type system.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdbool.h>
#include <strings.h>

#include "config.h"

#include "db.h"
#include "fbstrings.h"
#include "flags.h"
#include "game.h"

/**
 * This array defines the fundamental object types within the database and
 * their associated display strings.
 */
struct object_type object_types[] = {
    { 'R', "ROOM", "Room", "room", "rooms" },
    { 'T', "THING", "Thing", "thing", "things" },
    { 'E', "EXIT/ACTION", "Exit", "exit", "exits" },
    { 'P', "PLAYER", "Player", "player", "players" },
    { 'F', "PROGRAM", "Program", "program", "programs" }
};
/**
 * This structure and array support the listing of system flags and provides
 * context-specific aliasing based on the object type.
 *
 * Entries are stored in the order of presentation.
 * Each flag must have a default TYPE_ANY name.
 */
struct flag_raw {
    short bit;             /**< bitmask index for this flag */
    short object_type;     /**< associated object type (or TYPE_ANY) */
    struct flag_data data; /**< flag configuration */
};

static const struct flag_raw flag_list_raw[] = {
    {  4, TYPE_ANY, { 'W', "WIZARD" } },
    {  5, TYPE_ANY, { 'L', "LINK_OK" } },
    { 14, TYPE_ANY, { 'K', "KILL_OK" } },
    {  6, TYPE_ANY, { 'D', "DARK" } },
    {  6, TYPE_PROGRAM, { 'D', "DEBUG" } },
    {  8, TYPE_ANY, { 'S', "STICKY" } },
    {  8, TYPE_PLAYER, { 'S', "SILENT" } },
    {  8, TYPE_PROGRAM, { 'S', "SETUID" } },
    { 19, TYPE_ANY, { 'Q', "QUELL" } },
    {  9, TYPE_ANY, { 'B', "BOUND" } },
    {  9, TYPE_PLAYER, { 'B', "BUILDER" } },
    { 10, TYPE_ANY, { 'C', "CHOWN_OK" } },
    { 10, TYPE_PLAYER, { 'C', "COLOR" } },
    { 11, TYPE_ANY, { 'J', "JUMP_OK" } },
    { 15, TYPE_ANY, { 'G', "GUEST" } },
    { 15, TYPE_ROOM, { 'G', "GUEST_RESTRICT" } },
    { 15, TYPE_EXIT, { 'G', "GUEST_RESTRICT" } },
    { 16, TYPE_ANY, { 'H', "HAVEN" } },
    { 16, TYPE_THING, { 'H', "HIDE" } },
    { 16, TYPE_PROGRAM, { 'H', "HARDUID" } },
    { 17, TYPE_ANY, { 'A', "ABODE" } },
    { 17, TYPE_EXIT, { 'A', "ABATE" } },
    { 17, TYPE_PROGRAM, { 'A', "AUTOSTART" } },
    { 24, TYPE_ANY, { 'V', "VEHICLE" } },
    { 24, TYPE_ROOM, { 'V', "VEHICLE_RESTRICT" } },
    { 24, TYPE_EXIT, { 'V', "VEHICLE_RESTRICT" } },
    { 24, TYPE_PROGRAM, { 'V', "VIEWABLE" } },
    { 27, TYPE_ANY, { 'X', "XFORCIBLE" } },
    { 27, TYPE_EXIT, { 'X', "XPRESS" } },
    { 25, TYPE_ANY, { 'Z', "ZOMBIE" } },
    { 25, TYPE_ROOM, { 'Z', "ZOMBIE_RESTRICT" } },
    { 25, TYPE_EXIT, { 'Z', "ZOMBIE_RESTRICT" } },
    { 25, TYPE_PROGRAM, { 'Z', "ZMUF_DEBUGGER" } },
    { 30, TYPE_ANY, { 'Y', "YIELD" } },
    { 31, TYPE_ANY, { 'O', "OVERT" } },

    /* hidden from flag displays */
    { 18, TYPE_ANY, { 'M', "MUCKER", NULL, true } },
    { 20, TYPE_ANY, { 'N', "NUCKER", NULL, true } },
    { 21, TYPE_ANY, { 'I', "INTERACTIVE", NULL, true } },

    { -1, TYPE_ANY, { 0, NULL, NULL } }
};

#define NUM_BITS 32

/**
 * Lookup tables for flags indexed by type, then bit position or symbol.
 */
static struct flag_entry flag_lists_by_bit[TYPE_ANY+1][NUM_BITS] = {};
struct flag_entry flag_lists_by_symbol[TYPE_ANY+1][NUM_SYMBOLS] = {};

/**
 * Initializes the flag lookup tables from the raw flag list.
 */
void
flag_init()
{
    for (const struct flag_raw *flag = flag_list_raw; flag->bit != -1; flag++) {
        int index = flag->data.symbol - 'A';
        if (index < 0 || index >= NUM_SYMBOLS) continue;
        if (flag->bit >= NUM_BITS) continue;

        for (int i = 0; i <= TYPE_ANY; i++) {
            if (flag->object_type == i || flag->object_type == TYPE_ANY) {
                flag_lists_by_symbol[i][index].value = (1 << flag->bit);
                flag_lists_by_symbol[i][index].symbol = flag->data.symbol;
                flag_lists_by_symbol[i][index].data = flag->data;

                flag_lists_by_bit[i][flag->bit].value = (1 << flag->bit);
                flag_lists_by_bit[i][flag->bit].symbol = flag->data.symbol;
                flag_lists_by_bit[i][flag->bit].data = flag->data;
            }
        }
    }
}

/**
 * Helper to check if a flag entry matches a given name or symbol.
 *
 * @param entry  The flag entry to check.
 * @param flag   The flag name or symbol string to match against.
 * @return       True if the flag matches.
 */
static bool
flag_matches(struct flag_entry *entry, const char *flag)
{
    if (flag[1] == '\0') {
        return (flag[0] == entry->symbol);
    }

    if (entry->data.display_name) {
        return string_prefix(entry->data.display_name, flag);
    }

    return false;
}


/**
 * Evaluates a single flag string against an object.
 *
 * Recognizes flag symbols, names, and virtual flag 'truewizard'.
 *
 * @param ref  The object to check.
 * @param p    The flag string (e.g., "W", "!W", "Wizard").
 * @return     True if the object satisfies the expression.
 */
bool
flag_eval(dbref ref, const char *p)
{
    bool negate = false;
    while (*p == NOT_TOKEN) {
        negate = !negate;
        p++;
    }

    if (*p == '\0') return false;

    bool result = false;

    if (string_prefix("truewizard", p)) {
        result = TrueWizard(ref);
    } else if (string_prefix("wizard", p) || (p[0] == 'W' && p[1] == '\0')) {
        result = Wizard(ref);
    } else if (p[1] == '\0' && *p >= 'A' && *p <= 'Z') {
        result = FLAG_CHECK(ref, *p);
    } else {
        int type = OBJECT_TYPE(ref);

        for (int i = 0; i < NUM_BITS; i++) {
            struct flag_entry *entry = &flag_lists_by_bit[type][i];

            if (flag_matches(entry, p)) {
                result = FLAG_CHECK(ref, entry->symbol);
                break;
            }
        }
    }

    return negate ? !result : result;
}

/**
 * Evaluates a pattern of flag symbols against an object.
 *
 * Recognizes type symbols, flag symbols, and MUCKER levels.
 *
 * @param ref  The object to check.
 * @param p    The pattern string (e.g., "PW3" or "J!A").
 * @return     True if all conditions in the pattern match.
 */
bool
flag_eval_pattern(dbref ref, const char *p)
{
    int type = OBJECT_TYPE(ref);

    while (*p) {
        bool negate = false;
        while (*p == NOT_TOKEN) {
            negate = !negate;
            p++;
        }
        if (*p == '\0') break;

        bool has_flag;
        if (*p == 'W') {
            has_flag = Wizard(ref);
        } else if (*p == object_types[type].symbol) {
            has_flag = true;
        } else if (*p >= '0' && *p <= '3') {
            has_flag = (OBJECT_MLEVEL(ref) == (*p - 48));
        } else if (*p >= 'A' && *p <= 'Z') {
            has_flag = FLAG_CHECK(ref, *p);
        } else {
            return false;
        }

        if (negate == has_flag) return false;

        p++;
    }
    return true;
}

/**
 * Generate a long-form text description of the flags on a given object.
 * Uses the provided buffer that has the given size.
 *
 * @param thing to generate description for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void
flag_list_verbose(dbref ref, char *buf, size_t size)
{
    int type = OBJECT_TYPE(ref);
    char *p = buf;
    char *end = buf + size;
    object_flag_type flags = db[ref].flags;

    while (flags) {
        int i = ffsl(flags) - 1;
        struct flag_entry *entry = &flag_lists_by_bit[type][i];

        if (entry->symbol && !entry->data.hidden) {
            p += snprintf(p, end - p, "%s ", entry->data.display_name);
        }
        flags &= ~(1U << i);
    }

    int mlevel = OBJECT_MLEVEL(ref);
    if (mlevel > 0) {
        p += snprintf(p, end - p, "MUCKER%d ", mlevel);
    }

    if (p > buf) p--;

    *p = '\0';
}

/**
 * Generates a string representation of object flags. Uses the provided
 * buffer that has the given size.
 *
 * @param thing the object to construct a flag string for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void
flag_list(dbref ref, char *buf, size_t size)
{
    int type = OBJECT_TYPE(ref);
    char *p = buf;
    char *end = buf + size;
    object_flag_type flags = db[ref].flags;

    if (size == 0) return;

    *p = '\0';

    while (flags) {
        int i = ffsl(flags) - 1;
        struct flag_entry *entry = &flag_lists_by_bit[type][i];

        if (entry->symbol && !entry->data.hidden) {
            if (p < end - 1) {
              *p++ = entry->symbol;
            }
        }

        flags &= ~(1U << i);
    }

    int mlevel = OBJECT_MLEVEL(ref);
    if (mlevel > 0 && (p + 1 < end)) {
        *p++ = mlevel + '0';
    }

    *p = '\0';
}

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
void
flag_unparse_object(dbref player, dbref ref, char *buf, size_t size)
{
    char flags_buf[MINI_BUFFER_LEN];

    if (player != NOTHING) {
        player = OWNER(player);
    }

    switch (ref) {
        case NOTHING:   strcpyn(buf, size, "*NOTHING*"); return;
        case AMBIGUOUS: strcpyn(buf, size, "*AMBIGUOUS*"); return;
        case HOME:      strcpyn(buf, size, "*HOME*"); return;
        case NIL:       strcpyn(buf, size, "*NIL*"); return;

        default:
            if (!ObjExists(ref)) {
                strcpyn(buf, size, "*INVALID*");
                return;
            }

            if (player == NOTHING ||
                (!FLAG_CHECK(player, 'S') && (
                    can_see_flags(player, ref) || (
                        OBJECT_TYPE(ref) != TYPE_PLAYER && (
                            controls_link(player, ref) ||
                            FLAG_CHECK(ref, 'C')
                        )
                    )
                ))) {

                flag_list(ref, flags_buf, sizeof(flags_buf));
                snprintf(buf, size, "%.*s(#%d%c%s)", BUFFER_LEN / 2, NAME(ref), ref, object_types[OBJECT_TYPE(ref)].symbol, flags_buf);
            } else {
                strcpyn(buf, size, NAME(ref));
            }
    }
}

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
object_flag_type
str_to_flag(const char *flag_string)
{
    if (!*flag_string) {
        return 0;
    }

    if (string_prefix("abode", flag_string)
            || string_prefix("autostart", flag_string)
            || string_prefix("abate", flag_string)) {
        return ABODE;
    } else if (string_prefix("builder", flag_string)
            || string_prefix("bound", flag_string)) {
        return BUILDER;
    } else if (string_prefix("chown_ok", flag_string)
            || string_prefix("color", flag_string)) {
        return CHOWN_OK;
    } else if (string_prefix("dark", flag_string)
            || string_prefix("debug", flag_string)) {
        return DARK;
    } else if (string_prefix("guest", flag_string)) {
        return GUEST;
    } else if (string_prefix("haven", flag_string)
            || string_prefix("hide", flag_string)
            || string_prefix("harduid", flag_string)) {
        return HAVEN;
    } else if (string_prefix("interactive", flag_string)) {
        return INTERACTIVE;
    } else if (string_prefix("jump_ok", flag_string)) {
        return JUMP_OK;
    } else if (string_prefix("kill_ok", flag_string)) {
        return KILL_OK;
    } else if (string_prefix("link_ok", flag_string)) {
        return LINK_OK;
    } else if (string_prefix("mucker", flag_string)) {
        return MUCKER;
    } else if (string_prefix("nucker", flag_string)) {
        return SMUCKER;
    } else if (string_prefix("overt", flag_string)) {
        return OVERT;
    } else if (string_prefix("quell", flag_string)) {
        return QUELL;
    } else if (string_prefix("sticky", flag_string)
            || string_prefix("silent", flag_string)
            || string_prefix("setuid", flag_string)) {
        return STICKY;
    } else if (string_prefix("vehicle", flag_string)
            || string_prefix("viewable", flag_string)) {
        return VEHICLE;
    } else if (string_prefix("wizard", flag_string)) {
        return WIZARD;
    } else if (string_prefix("truewizard", flag_string)) {
        return WIZARD;
    } else if (string_prefix("xforcible", flag_string)
            || string_prefix("xpress", flag_string)) {
        return XFORCIBLE;
    } else if (string_prefix("yield", flag_string)) {
        return YIELD;
    } else if (string_prefix("zombie", flag_string)) {
        return ZOMBIE;
    }

    return 0;
}

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
int
unable_to_set_flag(dbref player, int mlev, dbref thing, object_flag_type flag, bool value, char *error)
{
    if (force_level && (
            flag == WIZARD
            || (flag == XFORCIBLE && OBJECT_TYPE(thing) != TYPE_EXIT)
            || (flag & MUCKER)
            || (flag & SMUCKER)
    )) {
        snprintf(error, SMALL_BUFFER_LEN, "That flag cannot be forced.");
        return 1;
    }

    /* Non-wizards can only set their own programs M0. */
    if (!value && (flag & (MUCKER | SMUCKER))) {
        if (!Wizard(OWNER(player))) {
            if ((OWNER(player) != OWNER(thing)) || (OBJECT_TYPE(thing) != TYPE_PROGRAM)) {
                snprintf(error, SMALL_BUFFER_LEN, "Permission denied. (You can't set that M0)");
                return 1;
            }
        }

        return 0;
    }

    /* Non-wizards can only set their programs up to their own mucker level. */
    if (flag & (MUCKER | SMUCKER)) {
        unsigned short requested_mlevel = (((flag & MUCKER ? 2 : 0) + (flag & SMUCKER ? 1 : 0)));
        if (!Wizard(OWNER(player))) {
            if ((OWNER(player) != OWNER(thing)) || (OBJECT_TYPE(thing) != TYPE_PROGRAM)
                || (OBJECT_MLEVEL(player) < requested_mlevel)) {
                snprintf(error, SMALL_BUFFER_LEN, "Permission denied. (You can't set that M%d)", requested_mlevel);
                return 1;
            }
        }

        return 0;
    }

    switch (flag) {
        case ABODE:
            /* Trying to set a program AUTOSTART requires TrueWizard */
            return (!TrueWizard(OWNER(player)) && (OBJECT_TYPE(thing) == TYPE_PROGRAM));

        case GUEST:
            /* Guest operations require a wizard */
            return (!(Wizard(OWNER(player))));

        case YIELD:
        case OVERT:
            /* Mucking with the env-chain matching requires TrueWizard */
            if (!(Wizard(OWNER(player)))) {
                return 1;
            }

            /* ...and only for makes sense for things or rooms. */
            if (OBJECT_TYPE(thing) != TYPE_THING && OBJECT_TYPE(thing) != TYPE_ROOM) {
                return 1;
            }

            return 0;

        case ZOMBIE:
            /* Restricting a player from using zombies requires a wizard. */
            if (OBJECT_TYPE(thing) == TYPE_PLAYER) {
                return (!(Wizard(OWNER(player))));
            }

            /* If a player's set Zombie, he's restricted from using them...
             * unless he's a wizard, in which case he can do whatever.
             */
            if ((OBJECT_TYPE(thing) == TYPE_THING) && FLAG_CHECK(player, 'Z')) {
                return (!(Wizard(OWNER(player))));
            }

            return 0;

        case VEHICLE:
            /* Restricting a player from using vehicles requires a wizard. */
            if (OBJECT_TYPE(thing) == TYPE_PLAYER){
                return (!(Wizard(OWNER(player))));
            }

            /* Can only set things !V if they have no players in them. */
            if (!value && OBJECT_TYPE(thing) == TYPE_THING) {
                dbref obj = CONTENTS(thing);

                for (; obj != NOTHING; obj = NEXTOBJ(obj)) {
                    if (OBJECT_TYPE(obj) == TYPE_PLAYER) {
                        snprintf(error, SMALL_BUFFER_LEN, "That vehicle still has players in it!");
                        return 1;
                    }
                }
            }

            /* If only wizards can create vehicles... */
            if (tp_wiz_vehicles) {
                /* then only a wizard can create a vehicle. :) */
                if (OBJECT_TYPE(thing) == TYPE_THING) {
                    return (!(Wizard(OWNER(player))));
                }
            } else {
                /* But, if vehicles aren't restricted to wizards, then
                 * players who have not been restricted can do so
                 */
                if ((OBJECT_TYPE(thing) == TYPE_THING) && FLAG_CHECK(player, 'V')) {
                    return (!(Wizard(OWNER(player))));
                }
            }

            return (0);

        case DARK:
            /* Dark can be set on a Program or Room by anyone. */
            if (!Wizard(OWNER(player))) {
                /* Setting a player dark requires a wizard. */
                if (OBJECT_TYPE(thing) == TYPE_PLAYER) {
                    return 1;
                }

                /* If exit darking is restricted, it requires a wizard. */
                if (!tp_exit_darking && OBJECT_TYPE(thing) == TYPE_EXIT) {
                    return 1;
                }

                /* If thing darking is restricted, it requires a wizard. */
                if (!tp_thing_darking && OBJECT_TYPE(thing) == TYPE_THING) {
                    return 1;
                }
            }

            return 0;

        case QUELL:
#ifdef GOD_PRIV
            /* Only God (or God's stuff) can quell or unquell another wizard. */
            return (TrueWizard(thing) && (thing != player) && !God(OWNER(player)) &&
                    (OBJECT_TYPE(thing) == TYPE_PLAYER));
#else
            /* You cannot quell or unquell another wizard. */
            return (TrueWizard(thing) && (thing != player) && (OBJECT_TYPE(thing) == TYPE_PLAYER));
#endif

        case BUILDER:
            /* Setting a program BOUND reguires M2. */
            if (OBJECT_TYPE(thing) == TYPE_PROGRAM) {
                return (mlev < 2);
            }

            /* Setting a player Builder or a room Bound is limited to a Wizard. */
            return (!Wizard(OWNER(player)));

        case WIZARD:
            /* To do anything with a Wizard flag requires a Wizard. */
            if (Wizard(OWNER(player))) {
                /* check for stupid wizard */
                if (!value && thing == player) {
                    snprintf(error, SMALL_BUFFER_LEN, "You cannot make yourself mortal.");
                    return 1;
                }

#ifdef GOD_PRIV
                /* Only God can make a player a Wizard, or re-mort one. */
                return ((OBJECT_TYPE(thing) == TYPE_PLAYER) && !God(player));
#else
                /* We don't want someone setting themselves !W, to prevent
                 * a case where there are no wizards at all
                 */
                return ((OBJECT_TYPE(thing) == TYPE_PLAYER && thing == OWNER(player)));
#endif
            } else {
                return 1;
            }

        case XFORCIBLE:
            return (!Wizard(OWNER(player)) && OBJECT_TYPE(thing) == TYPE_EXIT);

        default:
            /* No other flags are restricted. */
            return 0;
    }
}

