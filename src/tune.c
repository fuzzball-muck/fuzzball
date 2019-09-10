/** @file tune.c
 *
 * Source defining the functions that support the @tune function and
 * dealing with tunable configuration parameters.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "commands.h"
#include "db.h"
#include "fbmuck.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "match.h"
#include "log.h"
#include "tune.h"
#include "tunelist.h"

/* NOTE:  Do NOT use this value as the name of a parameter.  Reserve it as a preprocessor define so
   it's a bit easier to change if needed.  If changed, don't forget to update the help files, too! */
#define TP_INFO_CMD "info"

static const char *str_objecttype[] = {
    "room", "thing", "exit", "player", "program", "garbage", "any"
};

/**
 * Count all available tune parameters.
 *
 * This is complicated by the fact that different parameter types are in
 * different arrays.
 *
 * @return integer number of tune paramters.
 */
int
tune_count_parms(void)
{
    return ARRAYSIZE(tune_list) - 1;
}

static int
tune_timespan_seconds(const char *value)
{
    int days, hrs, mins, secs;
    int result = sscanf(value, "%dd %2d:%2d:%2d", &days, &hrs, &mins, &secs);
    
    if (result == 4) {
      return (days * 86400) + (3600 * hrs) + (60 * mins) + secs;
    }

    int total = 0, subtotal = 0;

    for (int i = 0, n = strlen(value); i < n; i++) {
        if (isdigit(value[i])) {
            subtotal = (subtotal*10) + (value[i]-48);
        } else {
            switch (tolower(value[i])) {
            case 'd':
                total += (subtotal * 86400);
                break;
            case 'h':
                total += (subtotal * 3600);
                break;
            case 'm':
                total += (subtotal * 60);
                break;
            case 's':
                total += subtotal;
                break;
            default:
                return -1;
            }

            subtotal = 0;
        }
     }

     return (total == 0) ? -1 : total;
}

static const char *
tune_entry_value(dbref player, struct tune_entry *tent)
{
    static char buf[BUFFER_LEN];

    switch (tent->type) {
    case TP_TYPE_STRING:
        strcpyn(buf, sizeof(buf), *tent->currentval.s);
        break;
    case TP_TYPE_TIMESPAN:
    strcpyn(buf, sizeof(buf), timestr_full(*tent->currentval.t));
        break;
    case TP_TYPE_INTEGER:
        snprintf(buf, sizeof(buf), "%d", *tent->currentval.n);
        break;
    case TP_TYPE_DBREF:
        if (player == NOTHING) {
            snprintf(buf, sizeof(buf), "#%d", *tent->currentval.d);
        } else {            
            unparse_object(player, *tent->currentval.d, buf, sizeof(buf));
        }
        break;
    case TP_TYPE_BOOLEAN:
        snprintf(buf, sizeof(buf), "%s", *tent->currentval.b == true ? "yes" : "no");
        break;
    }

    return buf;
}

/**
 * Display tune parameters, optionally filtered, to the calling player
 *
 * If name is "", then this will show everything; otherwise it will
 * do an basic regex match of "name" against the parameter names to find it.
 *
 * @see equalstr
 *
 * mlev is the MUCKER level of the player and is used for validation
 * purposes.  Some tunable parameters are only visible to certain
 * MUCKER levels.
 *
 * If show_extended is true, it will show the tune entry group and
 * label as well.
 *
 * @private
 * @param player the player doing the call
 * @param name a parameter name to search for or "" -- NULL is not valid.
 * @param mlev the MUCKER level of the player.
 * @param show_extended boolean show extended values?
 */
static void
tune_display_parms(dbref player, char *name, int mlev, int show_extended)
{
    char buf[BUFFER_LEN];

    /* Show a helpful message at the end if nothing was found */
    int parms_found = 0;

    if (!name) return;

    /* Treat default parameters as normal */
    TP_CLEAR_FLAG_DEFAULT(name);

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->readmlev <= mlev) {
            if (!*name || equalstr(name, (char *)tent->name)) {
                strcpyn(buf, sizeof(buf), tune_entry_value(player, tent));

                notifyf(player, "%-6s %-20s = %.4096s%s%s",
                        str_tunetype[tent->type], tent->name, buf,
                        MOD_ENABLED(tent->module) ? "" : " [inactive]",
                        (tent->isdefault) ? " [default]" : "");

                if (show_extended)
                    notifyf(player, "%-27s %.4096s", tent->group, tent->label);

                parms_found = 1;
            }
        }
    }

    if (!parms_found)
        notify(player, "No matching parameters.");

    notify(player, "*done*");
}

/**
 * Save tune parameters to a file
 *
 * Some notes on parameter saving:
 *
 * Comment out default values saved to the output.  This allows distinguishing
 * between available parameters and customized parameters while still creating
 * a list of new parameters in fresh or updated databases.
 *
 * If value is specified, save it as normal.  It won't be marked as default
 * again until it's removed from the database (including commenting it out).
 *
 * Example:
 * > Custom
 * ssl_min_protocol_version=TLSv1.2
 * > Default
 * %ssl_min_protocol_version=None
 *
 * @param f the file handle to write to
 */
void
tune_save_parms_to_file(FILE * f)
{
    char buf[BUFFER_LEN];

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        strcpyn(buf, sizeof(buf), tune_entry_value(NOTHING, tent));

        if (tent->isdefault)
            fprintf(f, "%c%s=%s\n", TP_FLAG_DEFAULT, tent->name, buf);
        else
            fprintf(f, "%s=%s\n", tent->name, buf);

    }
}

/**
 * Creates a MUF PACKED (sequential) array from the tune parameters.
 *
 * Takes an optional pattern which is used a name search; "" matches
 * everything.
 *
 * @see equalstr
 *
 * mlevel is the MUCKER level of the person doing the call for
 * permission checking.  Pinned is sent directly to new_array_packed
 *
 * @see new_array_packed
 *
 * The resulting array is an array of dictionaries.  The dictionaries
 * vary slightly in structure based on the "type" of tune data.  In
 * common, they have the following fields:
 *
 * type - boolean, timespan, integer, dbref, string
 * group - the tune group this parameter belongs to
 * name - the name of the parameter
 * value - the value of the parameter
 * mlev - the MUCKER level required to read this parameter.
 * readmlev - the same as mlev
 * writemlev - the MUCKER level required to write this paramter.
 * label - some label text to explain the parameter
 * default - the default value
 * active - if this tune parameter is 'active' or applies to the MUCK
 * nullable - is this field able to be null?
 *
 * It may additionally have 'objtype', for type == dbref, which will
 * be 'any', 'player', 'thing', 'room', 'exit', 'program', 'garbage',
 * or 'unknown'
 *
 * @param pattern the pattern to search for
 * @param mlev the MUCKER level of the calling player
 * @param pinned pin the array?
 * @return a packed stk_array type - you are responsible to free this memory
 */
stk_array *
tune_parms_array(const char *pattern, int mlev, int pinned)
{
    stk_array *nu = new_array_packed(0, pinned);

    struct inst temp1;
    char pat[BUFFER_LEN];
    int i = 0;

    strcpyn(pat, sizeof(pat), pattern);

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->readmlev <= mlev) {
            if (!*pattern || equalstr(pat, (char *)tent->name)) {
                stk_array *item = new_array_dictionary(pinned);
                array_set_strkey_strval(&item, "group", tent->group);
                array_set_strkey_strval(&item, "name", tent->name);
                array_set_strkey_intval(&item, "mlev", tent->readmlev);
                array_set_strkey_intval(&item, "readmlev", tent->readmlev);
                array_set_strkey_intval(&item, "writemlev", tent->writemlev);
                array_set_strkey_strval(&item, "label", tent->label);
                array_set_strkey_intval(&item, "nullable", tent->isnullable);
                array_set_strkey_intval(&item, "active", MOD_ENABLED(tent->module));
                array_set_strkey_intval(&item, "default", tent->isdefault);

                switch (tent->type) {
                case TP_TYPE_STRING:
                    array_set_strkey_strval(&item, "type", "string");
                    array_set_strkey_strval(&item, "value", *tent->currentval.s);
                    break;
                case TP_TYPE_TIMESPAN:
                    array_set_strkey_strval(&item, "type", "timespan");
                    array_set_strkey_intval(&item, "value", *tent->currentval.t);
                    break;
                case TP_TYPE_INTEGER:
                    array_set_strkey_strval(&item, "type", "integer");
                    array_set_strkey_intval(&item, "value", *tent->currentval.n);
                    break;
                case TP_TYPE_DBREF:
                    array_set_strkey_strval(&item, "type", "dbref");

                    if (tent->type > NOTYPE) {
                        array_set_strkey_strval(&item, "objtype", "unknown");
                    } else {
                        array_set_strkey_strval(&item, "objtype", str_objecttype[tent->objecttype]);
                    }

                    array_set_strkey_refval(&item, "value", *tent->currentval.d);
                    break;
                case TP_TYPE_BOOLEAN:
                    array_set_strkey_strval(&item, "type", "boolean");
                    array_set_strkey_intval(&item, "value", *tent->currentval.b);
                    break;
                }

                temp1.type = PROG_ARRAY;
                temp1.data.array = item;
                array_set_intkey(&nu, i++, &temp1);
                CLEAR(&temp1);
            }
        }
    }

    return nu;
}

/**
 * Get the string value of a given parameter
 *
 * The 'name' must match a parameter value, case insensitive.  If
 * it does not match a null string is returned.  If 'name' has
 * the default value prefix, that prefix is removed first.
 *
 * mlev is the player's MUCKER level for security checking.  If
 * security check fails, an empty string is returned.
 *
 * The string is in a static buffer, so this is not thread safe
 * and you must copy it if you want to ensure it doesn't mutate
 * out from under you.
 *
 * DBREFs will be prefixed with #, and booleans will return as "yes"
 * or "no".  Time will be a number in seconds.
 *
 * @param name the name of the parameter
 * @param mlev the MUCKER level of the calling player.
 * @return string as specified above.
 */
const char *
tune_get_parmstring(const char *name, int mlev)
{
    static char buf[BUFFER_LEN];

    /* Treat default parameters as normal */
    TP_CLEAR_FLAG_DEFAULT(name);

    if (!name) return "";

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (!strcasecmp(name, tent->name)) {
            strcpyn(buf, sizeof(buf), tune_entry_value(NOTHING, tent));

            return (tent->readmlev > mlev) ? "" : buf;
        }
    }

    /* Parameter was not found.  Return null string. */
    return "";
}

/**
 * Free all tune parameters
 *
 * This will probably render the MUCK unstable if casually used; it is
 * for the MUCK shutdown process and is, in fact, one the last cleanups
 * called.
 */
void
tune_freeparms()
{
    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->type == TP_TYPE_STRING && !tent->isdefault) {
            free((void *)*tent->currentval.s);
        }
    }
}

/**
 * Sets a tune parameter and handles all the intricaties therein
 *
 * For a given player 'player' with MUCKER level 'mlev', it will set
 * parameter 'parmname' to 'val', doing all necessary parsing.
 *
 * This will return one of the following integer constants:
 *
 * TUNESET_DENIED - permission denied
 * TUNESET_SUCCESS - success
 * TUNESET_SUCCESS_DEFAULT - successfully reset to default
 * TUNESET_UNKNOWN - unknown parmname
 * TUNESET_BADVAL - bad (invalid) value for parameter
 * TUNESET_SYNTAX - syntax error for val
 *
 * @param player the player doing the call
 * @param parmname the parameter to set
 * @param val the value to set the parameter to
 * @param mlev the MUCKER level of the player doing the call
 * @return integer result, as described above.
 */
int
tune_setparm(dbref player, const char *parmname, const char *val, int mlev)
{
    char buf[BUFFER_LEN];
    char *parmval;
    strcpyn(buf, sizeof(buf), val);
    parmval = buf;

    int reset_default = 0;

    if (TP_HAS_FLAG_DEFAULT(parmname)) {
        /*
         * Parameter name starts with the 'reset to default' flag.
         * Remove the flag and mark the parameter for resetting to
         * default.
         */
        TP_CLEAR_FLAG_DEFAULT(parmname);
        reset_default = 1;
    }

    if (!parmname)
        return TUNESET_UNKNOWN;

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (!strcasecmp(parmname, tent->name)) {
            if (tent->writemlev > mlev)
                return TUNESET_DENIED;

            if (reset_default) {
                /* Reset to default value */
                if (tent->type == TP_TYPE_STRING && !tent->isdefault)
                    free((void *)*tent->currentval.s);

                tent->isdefault = 1;

                switch (tent->type) {
                case TP_TYPE_STRING:
                    *tent->currentval.s = tent->defaultval.s;
                    break;
                case TP_TYPE_TIMESPAN:
                    *tent->currentval.t = tent->defaultval.t;
                    break;
                case TP_TYPE_INTEGER:
                    *tent->currentval.n = tent->defaultval.n;
                    break;
                case TP_TYPE_DBREF:
                    *tent->currentval.d = tent->defaultval.d;
                    break;
                case TP_TYPE_BOOLEAN:
                    *tent->currentval.b = tent->defaultval.b;
                    break;
                }

                return TUNESET_SUCCESS_DEFAULT;
            }

            /* Specify a new value */
            if (!tent->isnullable && !*parmval)
                return TUNESET_BADVAL;

            if (tent->type == TP_TYPE_STRING && !tent->isdefault)
                free((void *)*tent->currentval.s);

            switch (tent->type) {
            case TP_TYPE_STRING:
                *tent->currentval.s = strdup(parmval);
                break;
            case TP_TYPE_TIMESPAN: {
                int result = tune_timespan_seconds(parmval);

                if (result < 0)
                    return TUNESET_SYNTAX;

                *tent->currentval.t = result;
                break; }
            case TP_TYPE_INTEGER:
                if (!number(parmval))
                    return TUNESET_SYNTAX;

                *tent->currentval.n = atoi(parmval);
                break;
            case TP_TYPE_DBREF: {
                dbref obj;
                struct match_data md;

                init_match(NOTHING, player, parmval, NOTYPE, &md);
                match_absolute(&md);
                match_registered(&md);
                match_player(&md);
                match_me(&md);
                match_here(&md);

                if ((obj = match_result(&md)) == NOTHING)
                    return TUNESET_SYNTAX;
                if (!ObjExists(obj))
                    return TUNESET_SYNTAX;
                if (tent->type != NOTYPE && Typeof(obj) != tent->objecttype)
                    return TUNESET_BADVAL;

                *tent->currentval.d = obj;
                break; }
            case TP_TYPE_BOOLEAN:
                if (*parmval == 'y' || *parmval == 'Y' || *parmval == '1') {
                    *tent->currentval.b = 1;
                } else if (*parmval == 'n' || *parmval == 'N' || *parmval == '0') {
                    *tent->currentval.b = 0;
                } else {
                    return TUNESET_SYNTAX;
                }
                break;
            }

            tent->isdefault = 0;
            return TUNESET_SUCCESS;
        }
    }

    return TUNESET_UNKNOWN;
}

/**
 * Load default parameters into the global tune structures.
 *
 * This initializes all the global tune structures with default
 * values, marking each as isdefault=1 in the process.  If there
 * is an existing string value in a string tune setting, it will be
 * free'd (if not isdefault)
 *
 * This is a pretty destructive call.
 */
void
tune_load_parms_defaults()
{
    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->currentval.s && !tent->isdefault) {
            free((void *)*tent->currentval.s);
        }

        tent->isdefault = 1;

        switch (tent->type) {
        case TP_TYPE_STRING:
            *tent->currentval.s = tent->defaultval.s;
            break;
        case TP_TYPE_TIMESPAN:
            *tent->currentval.t = tent->defaultval.t;
            break;
        case TP_TYPE_INTEGER:
            *tent->currentval.n = tent->defaultval.n;
            break;
        case TP_TYPE_DBREF:
            *tent->currentval.d = tent->defaultval.d;
            break;
        case TP_TYPE_BOOLEAN:
            *tent->currentval.b = tent->defaultval.b;
            break;
        }
    }
}

/**
 * Load tune params from a file.
 *
 * This loads the parameters from a file in the format written
 * by tune_save_parms_to_file.  It ignores lines that start with a #
 * mark.
 *
 * @see tune_save_parms_to_file
 * @see tune_setparm
 *
 * It will read up to 'cnt' number of lines, or all lines in the file if
 * cnt is a negative number.
 *
 * Player may either be NOTHING, or a player that will receive notifications.
 * The tune settings are done with MLEV_GOD no matter what MUCKER level
 * 'player' actually has.  If player is NOTHING, the process is silent.
 *
 * @param f the file handle to read from
 * @param player The player who will receive notifications, or NOTHING
 * @param cnt the count of lines to read or -1 to read all lines.
 */
void
tune_load_parms_from_file(FILE * f, dbref player, int cnt)
{
    char buf[BUFFER_LEN];
    char *c, *p;
    int result = 0;

    while (!feof(f) && (cnt < 0 || cnt--)) {
        fgets(buf, sizeof(buf), f);

        if (*buf != '#') {
            c = strchr(buf, ARG_DELIMITER);

            if (c) {
                *c++ = '\0';
                p = buf;
                remove_ending_whitespace(&p);
                skip_whitespace((const char **)&c);

                for (p = c; *p && *p != '\n' && *p != '\r'; p++) ;

                *p = '\0';
                p = buf;
                skip_whitespace((const char **)&p);

                if (*p) {
                    result = tune_setparm((dbref)1, p, c, MLEV_GOD);
                }

                switch (result) {
                    case TUNESET_SUCCESS:
                        strcatn(buf, sizeof(buf), ": Parameter set.");
                        break;
                    case TUNESET_SUCCESS_DEFAULT:
                        strcatn(buf, sizeof(buf), ": Parameter reset to default.");
                        break;
                    case TUNESET_UNKNOWN:
                        strcatn(buf, sizeof(buf), ": Unknown parameter.");
                        break;
                    case TUNESET_SYNTAX:
                        strcatn(buf, sizeof(buf), ": Bad parameter syntax.");
                        break;
                    case TUNESET_BADVAL:
                        strcatn(buf, sizeof(buf), ": Bad parameter value.");
                        break;
                    case TUNESET_DENIED:
                        strcatn(buf, sizeof(buf), ": Permission denied.");
                        break;
                }

                if (result && player != NOTHING)
                    notify(player, p);
            }
        }
    }
}

/**
 * Implementation of @tune command
 *
 * This does a lot of different things based on if 'parmname' and
 * 'parmval' are set and to what.
 *
 * This does SOME permission checking; it checks to see if the player is
 * being forced or not, and checks if their MUCKER level allows them to
 * read/write parameters as appropriate.  However it doesn't stop people
 * with no MUCKER/Wizard from running it.
 *
 * If parmname and parmval are both set, or parmname starts with the
 * TP_FLAG_DEFAULT symbol (usually %) then the parameter will be set.
 *
 * @see tune_setparm
 *
 * If the parmname is "save" or "load", it will save or load parameters
 * from file accordingly.
 *
 * Otherwise, parmname should be either "" to list all @tunes or
 * a simple regex can be used to show only certain parameters.
 *
 * The parmname "info" will display more detained tune information.
 *
 * @param player the player running the command
 * @param parmname command / parameter name - see description
 * @param parmval Parameter value to set, or ""
 */
void
do_tune(dbref player, char *parmname, char *parmval)
{
    char *oldvalue;
    int result;
    int security = TUNE_MLEV(player);

    /*
     * If parmname exists, and either has parmvalue or the reset to default
     * flag, try to set the value.  Otherwise, fall back to displaying it.
     */
    if (*parmname && (strchr(match_args, ARG_DELIMITER) || TP_HAS_FLAG_DEFAULT(parmname))) {
        if (force_level) {
            notify(player, "You cannot force setting a @tune.");
            return;
        }

        /*
         * Duplicate the string, otherwise the oldvalue pointer will be
         * overridden to the new value when tune_setparm() is called.
         */
        oldvalue = strdup(tune_get_parmstring(parmname, security));
        result = tune_setparm(player, parmname, parmval, security);

        switch (result) {
            case TUNESET_SUCCESS:
            case TUNESET_SUCCESS_DEFAULT:
                if (result == TUNESET_SUCCESS_DEFAULT) {
                    TP_CLEAR_FLAG_DEFAULT(parmname);
                    log_status("TUNED: %s(%d) tuned %s from '%s' to default",
                               NAME(player), player, parmname, oldvalue);
                    notify(player, "Parameter reset to default.");
                } else {
                    log_status("TUNED: %s(%d) tuned %s from '%s' to '%s'",
                               NAME(player), player, parmname, oldvalue, DoNull(parmval));
                    notify(player, "Parameter set.");
                }

                tune_display_parms(player, parmname, security, 0);
                break;
            case TUNESET_UNKNOWN:
                notify(player, "Unknown parameter.");
                break;
            case TUNESET_SYNTAX:
                notify(player, "Bad parameter syntax.");
                break;
            case TUNESET_BADVAL:
                notify(player, "Bad parameter value.");
                break;
            case TUNESET_DENIED:
                notify(player, "Permission denied.");
                break;
            default:
                break;
        }

        /* Don't let the oldvalue hang around */
        if (oldvalue)
            free(oldvalue);
    } else if (*parmname && string_prefix(parmname, TP_INFO_CMD)) {
        /*
         * Space-separated parameters are all in parmname.  Trim out the
         * 'info' command and any extra spaces
         */
        const char *p_trim = strchr(parmname, ' ');

        if (p_trim != NULL) {
            skip_whitespace(&p_trim);

            if (*p_trim) {
                tune_display_parms(player, (char *)p_trim, security, 1);
            } else {
                notify(player, "Usage is @tune " TP_INFO_CMD " [optional: <parameter>]");
            }
        } else {
            /* Show expanded information on all parameters */
            tune_display_parms(player, "", security, 1);
        }
    } else {
        tune_display_parms(player, parmname, security, 0);
    }
}

/**
 * This takes an object and a string of flags, and sets them on the object
 *
 * The tunestr can have the following characters in it.  Each corresponds
 * to a flag.  Unknown flags are ignored.  '0' and 'W' cannot be set and
 * will be ignored.
 *
 * 1 2 3 A B C D G H J K L M O Q S V X Y Z
 *
 * Each corresponds to the first letter of the flag in question, with the
 * numbers being MUCKER levels.
 *
 * @param obj the object to set flags on
 * @param tunestr the string of flags
 */
void
set_flags_from_tunestr(dbref obj, const char *tunestr)
{
    const char *p = tunestr;
    object_flag_type f = 0;

    for (;;) {
        char pcc = toupper(*p);

        if (pcc == '\0' || pcc == '\n' || pcc == '\r') {
            break;
        } else if (pcc == '0') {
            /* SetMLevel(obj, 0); * This flag is ignored. */
        } else if (pcc == '1') {
            SetMLevel(obj, 1);
        } else if (pcc == '2') {
            SetMLevel(obj, 2);
        } else if (pcc == '3') {
            SetMLevel(obj, 3);
        } else if (pcc == 'A') {
            f = ABODE;
        } else if (pcc == 'B') {
            f = BUILDER;
        } else if (pcc == 'C') {
            f = CHOWN_OK;
        } else if (pcc == 'D') {
            f = DARK;
        } else if (pcc == 'G') {
            f = GUEST;
        } else if (pcc == 'H') {
            f = HAVEN;
        } else if (pcc == 'J') {
            f = JUMP_OK;
        } else if (pcc == 'K') {
            f = KILL_OK;
        } else if (pcc == 'L') {
            f = LINK_OK;
        } else if (pcc == 'M') {
            SetMLevel(obj, 2);
        } else if (pcc == 'O') {
            f = OVERT;
        } else if (pcc == 'Q') {
            f = QUELL;
        } else if (pcc == 'S') {
            f = STICKY;
        } else if (pcc == 'V') {
            f = VEHICLE;
        } else if (pcc == 'W') {
            /* f = WIZARD;        * This is very bad to auto-set. */
        } else if (pcc == 'X') {
            f = XFORCIBLE;
        } else if (pcc == 'Y') {
            f = YIELD;
        } else if (pcc == 'Z') {
            f = ZOMBIE;
        }

        FLAGS(obj) |= f;
        p++;
    }

    ts_modifyobject(obj);
    DBDIRTY(obj);
}
