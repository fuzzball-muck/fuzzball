/** @file set.c
 *
 * Source defining primarily commands that set things (@name, @chown, @set)
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "match.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

/**
 * Implementation of the @name command
 *
 * This performs a name change for a given 'name' to 'newname'
 *
 * In the case of players, "newname" should also have the player's
 * password in it after a space.
 *
 * This does all permission checking.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param name the name of object to alter
 * @param newname the new name (Which may also have a password)
 */
void
do_name(int descr, dbref player, const char *name, char *newname)
{
    dbref thing;
    char *password;

    /* match_controlled checks if the player controls the matched thing */
    if ((thing = match_controlled(descr, player, name)) == NOTHING)
        return;

    /* check for bad name */
    if (*newname == '\0') {
        notify(player, "Give it what new name?");
        return;
    }

    /* check for renaming a player */
    if (Typeof(thing) == TYPE_PLAYER) {
        /* split off password */
        for (password = newname; *password && !isspace(*password); password++) ;

        if (*password) {
            *password++ = '\0';     /* terminate name */
            skip_whitespace((const char **)&password);
        }

        /* check for null password */
        if (!*password) {
            notify(player, "You must specify a password to change a player name.");
            notify(player, "E.g.: name player = newname password");
            return;
        }

        if (!check_password(thing, password)) {
            notify(player, "Incorrect password.");
            return;
        }

        if (strcasecmp(newname, NAME(thing))
                   && !ok_object_name(newname, TYPE_PLAYER)) {
            notify(player, "You can't give a player that name.");
            return;
        }

        /* everything ok, notify */
        log_status("NAME CHANGE: %s(#%d) to %s", NAME(thing), thing, newname);
        delete_player(thing);
        change_player_name(thing, newname);
        add_player(thing);
        notify(player, "Name set.");
        return;
    } else {
        if (!ok_object_name(newname, Typeof(thing))) {
            notify(player, "That is not a reasonable name.");
            return;
        }
    }

    /* everything ok, change the name */
    free((void *) NAME(thing));

    ts_modifyobject(thing);
    NAME(thing) = alloc_string(newname);
    notify(player, "Name set.");
    DBDIRTY(thing);
}

/**
 * Unlinks an object.  The results vary based on object type.
 *
 * If this is an exit, the exit will be unlinked.  If this is a room, the
 * dropto will be removed.  If this is a thing, its home will be reset
 * to its owner.  If this is a player, its home will be reset to player start.
 *
 * If quiet is true, only errors will be displayed and no other messaging.
 * Otherwise, full messaging as expected with @unlink will be used.
 *
 * This does do permission checking.
 *
 * @private
 * @param descr the descriptor of the calling player
 * @param player the calling player
 * @param name the name of the thing to unlink
 * @param quiet boolean if true, only display errors.
 */
static void
_do_unlink(int descr, dbref player, const char *name, bool quiet)
{
    dbref exit;
    struct match_data md;

    /* @TODO this code doesn't share with MUF's setlink command.  Therefore,
     *       this logic is pretty much duplicate with setlink's link to #-1
     *       code.  There should be some sort of "elemental" unlink and setlink
     *       used by both commands and MUF prims
     */

    init_match(descr, player, name, TYPE_EXIT, &md);
    match_everything(&md);

    if ((exit = noisy_match_result(&md)) == NOTHING)
        return;

    if (!controls(player, exit) && !controls_link(player, exit)) {
        notify(player, "Permission denied. (You don't control the exit or its link)");
    } else {
        switch (Typeof(exit)) {
            case TYPE_EXIT:
                if (DBFETCH(exit)->sp.exit.ndest != 0) {
                    SETVALUE(OWNER(exit), GETVALUE(OWNER(exit)) + tp_link_cost);
                    DBDIRTY(OWNER(exit));
                }

                ts_modifyobject(exit);
                DBSTORE(exit, sp.exit.ndest, 0);

                free(DBFETCH(exit)->sp.exit.dest);
                DBSTORE(exit, sp.exit.dest, NULL);

                if (!quiet)
                    notify(player, "Unlinked.");

                if (MLevRaw(exit)) {
                    SetMLevel(exit, 0);
                    DBDIRTY(exit);

                    if (!quiet)
                        notify(player, "Action priority Level reset to 0.");
                }

                break;
            case TYPE_ROOM:
                ts_modifyobject(exit);
                DBSTORE(exit, sp.room.dropto, NOTHING);

                if (!quiet)
                    notify(player, "Dropto removed.");

                break;
            case TYPE_THING:
                ts_modifyobject(exit);
                THING_SET_HOME(exit, OWNER(exit));
                DBDIRTY(exit);

                if (!quiet)
                    notify(player, "Thing's home reset to owner.");

                break;
            case TYPE_PLAYER:
                ts_modifyobject(exit);
                PLAYER_SET_HOME(exit, tp_player_start);
                DBDIRTY(exit);

                if (!quiet)
                    notify(player, "Player's home reset to default player start room.");

                break;
            default:
                notify(player, "You can't unlink that!");
                break;
        }
    }
}

/**
 * Implementation of @unlink command
 *
 * This is a thin wrapper around _do_unlink (a private function) that does
 * all the logic.  As it is a private function, the documentation will
 * be duplicated here:
 *
 * If this is an exit, the exit will be unlinked.  If this is a room, the
 * dropto will be removed.  If this is a thing, its home will be reset
 * to its owner.  If this is a player, its home will be reset to player start.
 *
 * This does do permission checking.
 *
 * @see _do_unlink
 *
 * @param descr the descriptor of the calling player
 * @param player the calling player
 * @param name the name of the thing to unlink
 */
void
do_unlink(int descr, dbref player, const char *name)
{
    /* do a regular, non-quiet unlink. */
    _do_unlink(descr, player, name, false);
}

/**
 * Implementation of @relink command
 *
 * This is basically the same as doing an @unlink followed by an @link.
 * Thus the details of what that entails is best described in those functions.
 *
 * @see do_unlink
 * @see do_link
 *
 * This does some checking to make sure that the target linking will
 * work, and it does do permission checking.
 *
 * @param descr the descriptor of the person running the command
 * @param player the person running the command
 * @param thing_name the thing to relink
 * @param dest_name the destination to relink to.
 */
void
do_relink(int descr, dbref player, const char *thing_name,
          const char *dest_name)
{
    /* The original comment had a to-do item in it:
     *
     * @TODO this shares some code with do_link() which should probably be
     *       moved into a separate function (is_link_ok() or similar).
     *       I would further add that MUF primitives should also use
     *       some common link-ok determination rather than have this
     *       defined in (at least) 3 places.
     */
    dbref thing;
    dbref dest;
    dbref good_dest[MAX_LINKS];
    struct match_data md;
    int ndest;

    init_match(descr, player, thing_name, TYPE_EXIT, &md);
    match_everything(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING)
        return;

    if (Typeof(thing) != TYPE_EXIT && strchr(dest_name, EXIT_DELIMITER)) {
        notify(player, "Only actions and exits can be linked to multiple destinations.");
        return;
    }

    /* first of all, check if the new target would be valid, so we can
       avoid breaking the old link if it isn't. */

    switch (Typeof(thing)) {
        case TYPE_EXIT:
            /* we're ok, check the usual stuff */
            if (DBFETCH(thing)->sp.exit.ndest != 0) {
                if (!controls(player, thing)) {
                    notify(player,
                           "Permission denied. (The exit is linked, and you don't control it)");
                    return;
                }
            } else {
                if (!Wizard(OWNER(player)) && (GETVALUE(player) < (tp_link_cost + tp_exit_cost))) {
                    notifyf(player, "It costs %d %s to link this exit.",
                            (tp_link_cost + tp_exit_cost),
                            (tp_link_cost + tp_exit_cost == 1) ? tp_penny : tp_pennies);
                    return;
                } else if (!Builder(player)) {
                    notify(player, "Only authorized builders may seize exits.");
                    return;
                }
                notifyf_nolisten(player, "Claiming unlinked exits: %s", DEPRECATED_FEATURE);
            }

            /* be anal: each and every new links destination has
               to be ok. Detailed error messages are given by
               link_exit_dry(). */

            ndest = link_exit_dry(descr, player, thing, (char *) dest_name, good_dest);

            if (ndest == 0) {
                notify(player, "Invalid target.");
                return;
            }

            break;
        case TYPE_THING:
        case TYPE_PLAYER:
            init_match(descr, player, dest_name, TYPE_ROOM, &md);
            match_neighbor(&md);
            match_absolute(&md);
            match_registered(&md);
            match_me(&md);
            match_here(&md);

            if (Typeof(thing) == TYPE_THING)
                match_possession(&md);

            if ((dest = noisy_match_result(&md)) == NOTHING)
                return;

            if (!controls(player, thing)
                || !can_link_to(player, Typeof(thing), dest)) {
                notify(player, "Permission denied. (You can't link to where you want to.");
                return;
            }

            if (parent_loop_check(thing, dest)) {
                notify(player, "That would cause a parent paradox.");
                return;
            }

            break;
        case TYPE_ROOM:     /* room dropto's */
            init_match(descr, player, dest_name, TYPE_ROOM, &md);
            match_neighbor(&md);
            match_possession(&md);
            match_registered(&md);
            match_absolute(&md);
            match_home(&md);

            if ((dest = noisy_match_result(&md)) == NOTHING)
                return;

            if (!controls(player, thing) || !can_link_to(player, Typeof(thing), dest)
                || (thing == dest)) {
                notify(player, "Permission denied. (You can't link to the dropto like that)");
                return;
            }

            break;
        case TYPE_PROGRAM:
            notify(player, "You can't link programs to things!");
            return;
        default:
            notify(player, "Internal error: weird object type.");
            log_status("PANIC: weird object: Typeof(%d) = %d", thing, Typeof(thing));
            return;
    }

    _do_unlink(descr, player, thing_name, true);
    notify(player, "Attempting to relink...");
    do_link(descr, player, thing_name, dest_name);
}

/**
 * Implementation of the @chown command
 *
 * This handles all the security aspects of the @chown command, including
 * the various weird details.  Such as THINGs must be held by the person
 * doing the @chowning, only wizards can transfer things from one player
 * to another, players can only @chown the room they are in, and players
 * can @chown any exit that links to something they control.
 *
 * Does not check the builder bit.
 *
 * @param descr the descriptor of the player doing the action
 * @param player the player doing the action
 * @param name the object having its ownership changed
 * @param newowner the new owner for the object, which may be empty string
 *                 to default to 'me'
 */
void
do_chown(int descr, dbref player, const char *name, const char *newowner)
{
    dbref thing;
    dbref owner;
    struct match_data md;

    if (!*name) {
        notify(player, "You must specify what you want to take ownership of.");
        return;
    }

    init_match(descr, player, name, NOTYPE, &md);
    match_everything(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING)
        return;

    if (*newowner && strcasecmp(newowner, "me")) {
        if ((owner = lookup_player(newowner)) == NOTHING) {
            notify(player, "I couldn't find that player.");
            return;
        }
    } else {
        owner = OWNER(player);
    }

    if (!Wizard(OWNER(player)) && OWNER(player) != owner) {
        notify(player, "Only wizards can transfer ownership to others.");
        return;
    }

#ifdef GOD_PRIV
    if (Wizard(OWNER(player)) && !God(player) && God(owner)) {
        notify(player, "God doesn't need an offering or sacrifice.");
        return;
    }
#endif      /* GOD_PRIV */

    if (!Wizard(OWNER(player))) {
        if (Typeof(thing) != TYPE_EXIT ||
            (DBFETCH(thing)->sp.exit.ndest && !controls_link(player, thing))) {
            if (!(FLAGS(thing) & CHOWN_OK) ||
                Typeof(thing) == TYPE_PROGRAM
                || !test_lock(descr, player, thing, MESGPROP_CHLOCK)) {
                notify(player, "You can't take possession of that.");
                return;
            }
        }
    }

    /* handle costs */
    if (owner == player && Typeof(thing) == TYPE_EXIT && OWNER(thing) != OWNER(player)) {
	if (!Builder(player)) {
	    notify(player, "Only authorized builders may seize exits.");
	    return;
	}
	if (!payfor(player, tp_exit_cost)) {
	    notifyf(player, "It costs %d %s to seize this exit.",
		    tp_exit_cost,
		    (tp_exit_cost == 1) ? tp_penny : tp_pennies);
	    return;
	}
	/* pay the owner for his loss */
	SETVALUE(OWNER(thing), GETVALUE(OWNER(thing)) + tp_exit_cost);
	DBDIRTY(OWNER(thing));

        if (!Wizard(player)) {
            notifyf_nolisten(player, "Claiming unlinked exits: %s", DEPRECATED_FEATURE);
        }
    }

    if (tp_realms_control && !Wizard(OWNER(player)) && TrueWizard(thing) &&
        Typeof(thing) == TYPE_ROOM) {
        notify(player, "You can't take possession of that.");
        return;
    }

    switch (Typeof(thing)) {
        case TYPE_ROOM:
            if (!Wizard(OWNER(player)) && LOCATION(player) != thing) {
                notify(player, "You can only chown \"here\".");
                return;
            }

            ts_modifyobject(thing);
            OWNER(thing) = OWNER(owner);
            break;
        case TYPE_THING:
            if (!Wizard(OWNER(player)) && LOCATION(thing) != player) {
                notify(player, "You aren't carrying that.");
                return;
            }

            ts_modifyobject(thing);
            OWNER(thing) = OWNER(owner);
            break;
        case TYPE_PLAYER:
            notify(player, "Players always own themselves.");
            return;
        case TYPE_EXIT:
        case TYPE_PROGRAM:
            ts_modifyobject(thing);
            OWNER(thing) = OWNER(owner);
            break;
        case TYPE_GARBAGE:
            notify(player, "No one wants to own garbage.");
            return;
    }

    if (owner == player)
        notify(player, "Owner changed to you.");
    else {
        char unparse_buf[BUFFER_LEN];
        unparse_object(player, owner, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "Owner changed to %s.", unparse_buf);
    }

    DBDIRTY(thing);
}

/**
 * Determines if a given player is restricted from setting a given flag
 *
 * The 'business logic' here is actually fairly dense and not easily
 * summed up in a comment.  As this is a 'private' function, the
 * reader is encouraged to review the very well documented code for
 * details.
 *
 * This is all pretty well documented in the MUCK's help files.
 *
 * @private
 * @param player the player we are checking permissions for
 * @param thing the thing we want to set the flag on
 * @param flag the flag we wish to set.
 * @return boolean 1 if restricted from setting flag, 0 if okay to set.
 */
static int
restricted(dbref player, dbref thing, object_flag_type flag)
{
    switch (flag) {
        case ABODE:
            /* Trying to set a program AUTOSTART requires TrueWizard */
            return (!TrueWizard(OWNER(player)) && (Typeof(thing) == TYPE_PROGRAM));
        case GUEST:
            /* Guest operations require a wizard */
            return (!(Wizard(OWNER(player))));
        case YIELD:
            /* Mucking with the env-chain matching requires TrueWizard */
            return (!(Wizard(OWNER(player))));
        case OVERT:
            /* Mucking with the env-chain matching requires TrueWizard */
            return (!(Wizard(OWNER(player))));
        case ZOMBIE:
            /* Restricting a player from using zombies requires a wizard. */
            if (Typeof(thing) == TYPE_PLAYER)
                return (!(Wizard(OWNER(player))));
            /* If a player's set Zombie, he's restricted from using them...
             * unless he's a wizard, in which case he can do whatever.
             */
            if ((Typeof(thing) == TYPE_THING) && (FLAGS(OWNER(player)) & ZOMBIE))
                return (!(Wizard(OWNER(player))));

            return (0);
        case VEHICLE:
            /* Restricting a player from using vehicles requires a wizard. */
            if (Typeof(thing) == TYPE_PLAYER)
                return (!(Wizard(OWNER(player))));

            /* If only wizards can create vehicles... */
            if (tp_wiz_vehicles) {
                /* then only a wizard can create a vehicle. :) */
                if (Typeof(thing) == TYPE_THING)
                    return (!(Wizard(OWNER(player))));
            } else {
                /* But, if vehicles aren't restricted to wizards, then
                 * players who have not been restricted can do so
                 */
                if ((Typeof(thing) == TYPE_THING) && (FLAGS(player) & VEHICLE))
                    return (!(Wizard(OWNER(player))));
            }

            return (0);
        case DARK:
            /* Dark can be set on a Program or Room by anyone. */
            if (!Wizard(OWNER(player))) {
                /* Setting a player dark requires a wizard. */
                if (Typeof(thing) == TYPE_PLAYER)
                    return (1);

                /* If exit darking is restricted, it requires a wizard. */
                if (!tp_exit_darking && Typeof(thing) == TYPE_EXIT)
                    return (1);

                /* If thing darking is restricted, it requires a wizard. */
                if (!tp_thing_darking && Typeof(thing) == TYPE_THING)
                    return (1);
            }

            return (0);
        case QUELL:
#ifdef GOD_PRIV
            /* Only God (or God's stuff) can quell or unquell another wizard. */
            return (God(OWNER(player)) || (TrueWizard(thing) && (thing != player) &&
                    Typeof(thing) == TYPE_PLAYER));
#else
            /* You cannot quell or unquell another wizard. */
            return (TrueWizard(thing) && (thing != player) && (Typeof(thing) == TYPE_PLAYER));
#endif
        /* The following three cases aren't used, as far as I can tell, but
         * for consistency's sake let's make sure they reflect the documented
         * behaviors:
         */
        case MUCKER:
            /* Setting a program M2 is limited to players of at least M2. */
            if (Typeof(thing) == TYPE_PROGRAM)
                return (MLevel(OWNER(player)) < 2);

            /* Setting anything else M2 takes a wizard. */
            return (!Wizard(OWNER(player)));
        case SMUCKER:
            /* Setting a program M1 is limited to players of at least M1. */
            if (Typeof(thing) == TYPE_PROGRAM)
                return (MLevel(OWNER(player)) < 1);

            /* Setting anything else M1 takes a wizard. */
            return (!Wizard(OWNER(player)));
        case (SMUCKER | MUCKER):
            /* Setting a program M3 is limited to players of at least M3. */

            if (Typeof(thing) == TYPE_PROGRAM)
                return (MLevel(OWNER(player)) < 3);

            /* Setting anything else M3 takes a wizard. */
            return (!Wizard(OWNER(player)));
        case BUILDER:
            /* Setting a program Bound causes any function called therein to be
             * put in preempt mode, regardless of the mode it had before.
             * Since this is just a convenience for atomic-functionwriters,
             * why is it limited to only a Wizard? -winged
             */
            /* That's a very good question! Let's limit it to players of at
             * least M2 instead. -aidanPB
             */
            if (Typeof(thing) == TYPE_PROGRAM)
                return (MLevel(OWNER(player)) < 2);

            /* Setting a player Builder or a room Bound is limited to a Wizard. */
            return (!Wizard(OWNER(player)));
        case WIZARD:
            /* To do anything with a Wizard flag requires a Wizard. */
            if (Wizard(OWNER(player))) {
#ifdef GOD_PRIV
                /* ...but only God can make a player a Wizard, or re-mort
                 * one.
                 */
                return ((Typeof(thing) == TYPE_PLAYER) && !God(player));
#else
                /* We don't want someone setting themselves !W, to prevent
                 * a case where there are no wizards at all
                 */
                return ((Typeof(thing) == TYPE_PLAYER && thing == OWNER(player)));
#endif
            } else
                return 1;
        default:
            /* No other flags are restricted. */
            return 0;
    }
}

/**
 * Implementation of @set command
 *
 * This can set flags or props.  The determining factor is if there is
 * a PROP_DELIMITER (:) character in the 'flag' string.  The special
 * case of ':clear' will clear all properties that the user controls
 * on teh object (i.e. players cannot unset wizprops with that).
 *
 * Otherwise, it will look at the flag and and set whatever desired flag
 * on the object in question.  Thisi s done with
 * string_prefix("FULL_NAME", flag) basically, so any fraction of a flag
 * name will match.  It supports "!FLAGNAME" as well to unset.
 *
 * Permission checking is done.
 *
 * @param descr the descriptor of the person doing the call.
 * @param player the player doing the call
 * @param name the name of the object to modify
 * @param flag the flag or property to set.
 */
void
do_set(int descr, dbref player, const char *name, const char *flag)
{
    dbref thing;
    const char *p;
    object_flag_type f;

    /* find thing */
    if ((thing = match_controlled(descr, player, name)) == NOTHING)
        return;

#ifdef GOD_PRIV
    /* Only God can set anything on any of his stuff */
    if (tp_strict_god_priv && !God(player) && God(OWNER(thing))) {
        notify(player, "Only God may touch God's property.");
        return;
    }
#endif

    /* move p past NOT_TOKEN if present */
    for (p = flag; *p && (*p == NOT_TOKEN || isspace(*p)); p++) ;

    /* Now we check to see if it's a property reference */
    /* if this gets changed, please also modify boolexp.c */
    if (strchr(flag, PROP_DELIMITER)) {
        /* @TODO This seems like it should probably be in its own
         *       function as it is a pretty big chunk of code.
         *       It is probably also useful elsewhere, such as maybe
         *       with MUF SETPROPSTR ?
         *
         *       The only logic here specific to @set is the support
         *       for :clear, so the structure would probably be if
         *       flag == :clear then unset, else set, and use
         *       common functions for clear and set.
         */

        /* copy the string so we can muck with it */
        const char *type = alloc_string(flag);  /* type */
        char *pname = (char *) strchr(type, PROP_DELIMITER);    /* propname */
        const char *x;      /* to preserve string location so we can free it */
        char *temp;
        int ival = 0;
        int wizperms = Wizard(OWNER(player));

        x = type;
        while (isspace(*type) && (*type != PROP_DELIMITER))
            type++;

        if (*type == PROP_DELIMITER) {
            /* clear all properties */
            type++;
            skip_whitespace(&type);

            if (strcasecmp(type, "clear")) {
                notify(player, "Use '@set <obj>=:clear' to clear all props on an object");
                free((void *) x);
                return;
            }

            remove_property_list(thing, wizperms);
            ts_modifyobject(thing);
            notifyf(player, "All %sproperties removed.", wizperms ? "" : "user-owned ");
            free((void *) x);
            return;
        }

        for (temp = pname - 1; temp >= type && isspace(*temp); temp--) ;

        while (temp >= type && *temp == PROPDIR_DELIMITER)
            temp--;

        *(++temp) = '\0';

        pname++;        /* move to next character */

        if (*pname == '^' && number(pname + 1))
            ival = atoi(++pname);

        if (Prop_System(type)
            || (!Wizard(OWNER(player)) && (Prop_SeeOnly(type) || Prop_Hidden(type)))) {
            notify(player, "Permission denied. (The property is restricted.)");
            free((void *) x);
            return;
        }

        if (!(*pname)) {
            ts_modifyobject(thing);
            remove_property(thing, type, 0);
            notify(player, "Property removed.");
        } else {
            ts_modifyobject(thing);

            if (ival) {
                add_property(thing, type, NULL, ival);
            } else {
                add_property(thing, type, pname, 0);
            }

            notify(player, "Property set.");
        }

        free((void *) x);
        return;
    }

    /* identify flag */

    /*
     * @TODO: This gnarly 'if' statement should probably be a function that
     * takes a flag string and converts it to a flag.  The exception
     * being the MUCKER flags which are a little tricky.
     *
     * So if (matches MUCKER bits) run existing code, else
     * flag = get_flag_from_string( ... )
     *
     * The SET prim should use the same function because this logic is
     * currently duplicated over there.
     */
    if (*p == '\0') {
        notify(player, "You must specify a flag to set.");
        return;
    } else if ((!strcasecmp("0", p) || !strcasecmp("M0", p)) ||
               ((string_prefix("MUCKER", p)) && (*flag == NOT_TOKEN))) {
        if (!Wizard(OWNER(player))) {
            if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)) {
                notify(player, "Permission denied. (You can't clear that mucker flag)");
                return;
            }
        }

        if (force_level) {
            notify(player, "Can't set this flag from an @force or {force}.");
            return;
        }

        SetMLevel(thing, 0);
        notify(player, "Mucker level set.");
        return;
    } else if (!strcasecmp("1", p) || !strcasecmp("M1", p)) {
        if (!Wizard(OWNER(player))) {
            if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)
                || (MLevRaw(player) < 1)) {
                notify(player, "Permission denied. (You may not set that M1)");
                return;
            }
        }

        if (force_level) {
            notify(player, "Can't set this flag from an @force or {force}.");
            return;
        }

        SetMLevel(thing, 1);
        notify(player, "Mucker level set.");
        return;
    } else if ((!strcasecmp("2", p) || !strcasecmp("M2", p)) ||
                ((string_prefix("MUCKER", p)) && (*flag != NOT_TOKEN))) {
        if (!Wizard(OWNER(player))) {
            if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)
                || (MLevRaw(player) < 2)) {
                notify(player, "Permission denied. (You may not set that M2)");
                return;
            }
        }

        if (force_level) {
            notify(player, "Can't set this flag from an @force or {force}.");
            return;
        }

        SetMLevel(thing, 2);
        notify(player, "Mucker level set.");
        return;
    } else if (!strcasecmp("3", p) || !strcasecmp("M3", p)) {
        if (!Wizard(OWNER(player))) {
            if ((OWNER(player) != OWNER(thing)) || (Typeof(thing) != TYPE_PROGRAM)
                || (MLevRaw(player) < 3)) {
                notify(player, "Permission denied. (You may not set that M3)");
                return;
            }
        }

        if (force_level) {
            notify(player, "Can't set this flag from an @force or {force}.");
            return;
        }

        SetMLevel(thing, 3);
        notify(player, "Mucker level set.");
        return;
    } else if (!strcasecmp("4", p) || !strcasecmp("M4", p)) {
        notify(player, "To set Mucker Level 4, set the Wizard bit and another Mucker bit.");
        return;
    } else if (string_prefix("WIZARD", p)) {
        if (force_level) {
            notify(player, "Can't set this flag from an @force or {force}.");
            return;
        }

        f = WIZARD;
    } else if (string_prefix("ZOMBIE", p)) {
        f = ZOMBIE;
    } else if (string_prefix("VEHICLE", p)
                || string_prefix("VIEWABLE", p)) {
        if (*flag == NOT_TOKEN && Typeof(thing) == TYPE_THING) {
            dbref obj = CONTENTS(thing);

            for (; obj != NOTHING; obj = NEXTOBJ(obj)) {
                if (Typeof(obj) == TYPE_PLAYER) {
                    notify(player, "That vehicle still has players in it!");
                    return;
                }
            }
        }

        f = VEHICLE;
    } else if (string_prefix("LINK_OK", p)) {
        f = LINK_OK;
    } else if (string_prefix("XFORCIBLE", p) || string_prefix("XPRESS", p)) {
        if (force_level) {
            notify(player, "Can't set this flag from an @force or {force}.");
            return;
        }

        if (Typeof(thing) == TYPE_EXIT) {
            if (!Wizard(OWNER(player))) {
                notify(player,
                       "Permission denied. (Only a Wizard may set the M-level of an exit)");
                return;
            }
        }

        f = XFORCIBLE;
    } else if (string_prefix("KILL_OK", p)) {
        f = KILL_OK;
    } else if ((string_prefix("DARK", p)) || (string_prefix("DEBUG", p))) {
        f = DARK;
    } else if ((string_prefix("STICKY", p)) || (string_prefix("SETUID", p)) ||
                (string_prefix("SILENT", p))) {
        f = STICKY;
    } else if (string_prefix("QUELL", p)) {
        f = QUELL;
    } else if (string_prefix("BUILDER", p) || string_prefix("BOUND", p)) {
        f = BUILDER;
    } else if (string_prefix("CHOWN_OK", p) || string_prefix("COLOR", p)) {
        f = CHOWN_OK;
    } else if (string_prefix("JUMP_OK", p)) {
        f = JUMP_OK;
    } else if (string_prefix("GUEST", p)) {
        f = GUEST;
    } else if (string_prefix("HAVEN", p) || string_prefix("HARDUID", p)) {
        f = HAVEN;
    } else if ((string_prefix("ABODE", p)) ||
               (string_prefix("AUTOSTART", p)) || (string_prefix("ABATE", p))) {
        f = ABODE;
    } else if (string_prefix("YIELD", p)) {
        f = YIELD;
    } else if (string_prefix("OVERT", p)) {
        f = OVERT;
    } else {
        notify(player, "I don't recognize that flag.");
        return;
    }

    /* check for restricted flag */
    if (restricted(player, thing, f)) {
        notify(player, "Permission denied. (restricted flag)");
        return;
    }

    /* check for stupid wizard */
    if (f == WIZARD && *flag == NOT_TOKEN && thing == player) {
        notify(player, "You cannot make yourself mortal.");
        return;
    }

    /* else everything is ok, do the set */
    if (*flag == NOT_TOKEN) {
        /* reset the flag */
        ts_modifyobject(thing);
        FLAGS(thing) &= ~f;
        DBDIRTY(thing);

        if (f == GUEST && Typeof(thing) == TYPE_PLAYER) {
            remove_property(thing, LEGACY_GUEST_PROP, 0);
        }

        notify(player, "Flag reset.");
    } else {
        /* set the flag */
        ts_modifyobject(thing);
        FLAGS(thing) |= f;
        DBDIRTY(thing);

        if (f == GUEST && Typeof(thing) == TYPE_PLAYER) {
            PData property;
            property.flags = PROP_STRTYP;
            property.data.str = "yes";
            set_property(thing, LEGACY_GUEST_PROP, &property, 0);
        }

        notify(player, "Flag set.");
    }
}

/**
 * Implementation of @propset
 *
 * Propset can set props along with types.  'prop' can be of the
 * format: <type>:<property>:<value> or erase:<property>
 *
 * Type can be omitted and will default to string,  but there still
 * must be two ':'s in order to set a property.  Type can
 * be: string, integer, float, lock, dbref.  Types can be shortened
 * to just a prefix (s, i, f, str, lo, etc.)  Erase can also be
 * prefixed (e, er, etc.)
 *
 * This does do permission checking.
 *
 * @param descr the descriptor of the person setting
 * @param player the person setting
 * @param name the object we are setting a prop on
 * @param prop the prop specification as described above.
 */
void
do_propset(int descr, dbref player, const char *name, const char *prop)
{
    dbref thing, ref;
    char *p;
    char buf[BUFFER_LEN];
    char *type, *pname, *value;
    struct match_data md;
    struct boolexp *lok;
    PData mydat;

    /* find thing */
    if ((thing = match_controlled(descr, player, name)) == NOTHING)
        return;

    skip_whitespace(&prop);
    strcpyn(buf, sizeof(buf), prop);

    /* Find type, pname, and value */

    type = p = buf;
    while (*p && *p != PROP_DELIMITER)
        p++;

    if (*p)
        *p++ = '\0';

    if (*type) {
        remove_ending_whitespace(&type);
    }

    pname = p;
    while (*p && *p != PROP_DELIMITER)
        p++;

    if (*p)
        *p++ = '\0';

    value = p;

    while (*pname == PROPDIR_DELIMITER || isspace(*pname))
        pname++;

    if (*pname) {
        remove_ending_whitespace(&pname);
    }

    if (!*pname) {
        notify(player, "I don't know which property you want to set!");
        return;
    }

    if (Prop_System(pname)
        || (!Wizard(OWNER(player)) && (Prop_SeeOnly(pname) || Prop_Hidden(pname)))) {
        notify(player, "Permission denied. (The property is restricted.)");
        return;
    }

    /* Process type and set property accordingly */
    if (!*type || string_prefix("string", type)) {
        add_property(thing, pname, value, 0);
    } else if (string_prefix("integer", type)) {
        if (!number(value)) {
            notify(player, "That's not an integer!");
            return;
        }

        add_property(thing, pname, NULL, atoi(value));
    } else if (string_prefix("float", type)) {
        if (!ifloat(value)) {
            notify(player, "That's not a floating point number!");
            return;
        }

        mydat.flags = PROP_FLTTYP;
        mydat.data.fval = strtod(value, NULL);
        set_property(thing, pname, &mydat, 0);
    } else if (string_prefix("dbref", type)) {
        init_match(descr, player, value, NOTYPE, &md);
        match_everything(&md);

        if ((ref = noisy_match_result(&md)) == NOTHING)
            return;

        mydat.flags = PROP_REFTYP;
        mydat.data.ref = ref;
        set_property(thing, pname, &mydat, 0);
    } else if (string_prefix("lock", type)) {
        lok = parse_boolexp(descr, player, value, 0);

        if (lok == TRUE_BOOLEXP) {
            notify(player, "I don't understand that lock.");
            return;
        }

        mydat.flags = PROP_LOKTYP;
        mydat.data.lok = lok;
        set_property(thing, pname, &mydat, 0);
    } else if (string_prefix("erase", type)) {
        if (*value) {
            notify(player, "Don't give a value when erasing a property.");
            return;
        }

        remove_property(thing, pname, 0);
        notify(player, "Property erased.");
        return;
    } else {
        notify(player, "I don't know what type of property you want to set!");
        notify(player, "Valid types are string, integer, float, dbref, lock, and erase.");
        return;
    }

    notify(player, "Property set.");
}

/**
 * Implementation of @register command
 *
 * Does property registration similar to the 'classic' MUF @register
 * command.  Thus, the parameters are a little complex.
 *
 * By default, this operates on #0 and propdir base _reg.  This
 * can be modified by a prefix that will show up in arg1.
 *
 * Prefix can be #me for operating on 'me', propdir base _reg
 * Or it can be #prop [<target>:]<propdir> to target a given
 * prop directory instead.  If <target> is not provided, #0 will be
 * the default.
 *
 * If arg2 is "", then arg1 will be either just a prefix or a prefix
 * plus a propdir to list registrations in either the base propdir or
 * a subdir.
 *
 * If arg2 is set, then arg1 is the prefix + object name or just
 * the prefix and arg2 is the target registration name.  If there
 * is no object name, then the registration target is cleared.  Otherwise,
 * object is registered to the given name.
 *
 * See help @register on the MUCK for a more concise explanation :)
 *
 * This does permission checking.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param arg1 The first argument as described above
 * @param arg2 the second argument as described above.
 */
void
do_register(int descr, dbref player, char *arg1, const char *arg2)
{
    dbref target, object;
    struct match_data md;
    char buf[BUFFER_LEN], unparse_buf[BUFFER_LEN];
    char *propdir, *objectstr;
    char *remaining = arg1;

    target = GLOBAL_ENVIRONMENT;
    propdir = REGISTRATION_PROPDIR;
    objectstr = arg1;

    if (string_prefix(arg1, "#me")) {
        (void) strtok_r(arg1, " \t", &remaining);
        target = player;
        objectstr = remaining;
    } else if (string_prefix(arg1, "#prop")) {
        char *pattern, *targetstr;

        (void) strtok_r(arg1, " \t", &remaining);
        pattern = strtok_r(remaining, " \t", &remaining);
        objectstr = remaining;

        if (pattern && *pattern == ':') {
            targetstr = "me";
            propdir = pattern + 1;
        } else {
            targetstr = strtok_r(pattern, ":", &remaining);
            propdir = remaining;
        }

        if (propdir) {
            char *p = propdir + strlen(propdir) - 1;
            while (p > propdir && *p == PROPDIR_DELIMITER) *(p--) = '\0';
        }

        if (!propdir || !*propdir) {
            notifyf_nolisten(player, "You must specify a propdir when using #prop.");
            return;
        }

        init_match(descr, player, targetstr, NOTYPE, &md);

        if (*targetstr == REGISTERED_TOKEN) {
            match_registered(&md);
        } else {
            match_all_exits(&md);
            match_neighbor(&md);
            match_possession(&md);
            match_me(&md);
            match_here(&md);
            match_nil(&md);
        }

        if (Wizard(OWNER(player))) {
            match_absolute(&md);
            match_player(&md);
        }

        if ((target = noisy_match_result(&md)) == NOTHING) {
            return;
        }

        /* match_controlled casts a much wider "match net" than
         * we are casting here, so we'll do the controls check
         * separately to preserve the limited match behavior.
         */
        if (!controls(player, target)) {
            notifyf_nolisten(player, "Permission denied. (You don't control what was matched)");
            return;
        }
    }

    if (!*arg2) {
        PropPtr propadr, pptr;
        char dir[BUFFER_LEN], propname[BUFFER_LEN], detail[BUFFER_LEN];
        dbref detailref = -50, invalidref = -50;

        if (!objectstr || !*objectstr) {
            strcpyn(dir, sizeof(dir), propdir);
        } else {
            snprintf(dir, sizeof(dir), "%s%c%s", propdir, PROPDIR_DELIMITER, objectstr);
        }

        unparse_object(player, target, unparse_buf, sizeof(unparse_buf));
        notifyf_nolisten(player, "Registered objects on %s:", unparse_buf);

        propadr = first_prop(target, dir, &pptr, propname, sizeof(propname));
        while (propadr) {
            char *strval;
            snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);

            if (!Prop_Hidden(buf) || Wizard(OWNER(player))) {
                switch (PropType(propadr)) {
                    case PROP_DIRTYP:
                        snprintf(detail, sizeof(detail), "/ (directory)");
                        break;
                    case PROP_STRTYP:
                        strval = PropDataStr(propadr);

                        if (*strval == NUMBER_TOKEN && number(strval+1)) {
                            detailref = atoi(strval+1);
                        } else if (number(strval)) {
                            detailref = atoi(strval);
                        } else {
                            detailref = invalidref;
                        }

                        break;
                    case PROP_INTTYP:
                        detailref = PropDataVal(propadr);
                        break;
                    case PROP_REFTYP:
                        detailref = PropDataRef(propadr);
                        break;
                    default:
                        detailref = invalidref;
                        break;
                }

                if (PropType(propadr) != PROP_DIRTYP) {
                    unparse_object(player, detailref, unparse_buf, sizeof(unparse_buf));
                    snprintf(detail, sizeof(detail), ": %s", unparse_buf);
                }

                if (!objectstr || !*objectstr) {
                    notifyf_nolisten(player, "  %s%s", propname, detail);
                } else {
                    char *p = objectstr + strlen(objectstr) - 1;

                    while (p > objectstr && *p == PROPDIR_DELIMITER)
                        *(p--) = '\0';

                    notifyf_nolisten(player, "  %s%c%s%s", objectstr, PROPDIR_DELIMITER, propname, detail);
                }
            }

            propadr = next_prop(pptr, propadr, propname, sizeof(propname));
        }

        notifyf_nolisten(player, "Done.");
        return;
    }

    if (!objectstr || !*objectstr) {
        register_object(player, target, propdir, (char *)arg2, NOTHING);
        return;
    }

    init_match(descr, player, objectstr, NOTYPE, &md);

    if (*objectstr == REGISTERED_TOKEN) {
        match_registered(&md);
    } else {
        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);
        match_me(&md);
        match_here(&md);
        match_nil(&md);
    }

    if (Wizard(OWNER(player))) {
        match_absolute(&md);
        match_player(&md);
    }

    if ((object = noisy_match_result(&md)) == NOTHING) {
        return;
    }

    snprintf(buf, sizeof(buf), "%s%c%s", propdir, PROPDIR_DELIMITER, arg2);

    if ((!Wizard(OWNER(player)) && (target != OWNER(player) || 
        Prop_SeeOnly(buf) || Prop_Hidden(buf)))) {
        notifyf_nolisten(player, "Permission denied. (You can't register an object there.)");
        return;
    }

    register_object(player, target, propdir, (char *)arg2, object);
}

/**
 * Implemenation of @doing command
 *
 * This, at present, only allows you to to set a doing message on
 * 'me' or you can leave 'name' as an empty string.
 *
 * @param descr the descriptor of the person doing the action
 * @param player the player doing the action
 * @param name the name of the person to set the doing on or "" for 'me'
 * @param mesg the doing message to set.
 */
void
do_doing(int descr, dbref player, const char *name, const char *mesg)
{
    /* @TODO: This limitation makes no sense.  It should have wizard check
     *        and allow wizards to set on anyone.  Remember to update
     *        help file.
     */
    if (*name && strcasecmp(name, "me")) {
        notifyf_nolisten(player, "This property can only be set on yourself.");
        return;
    }

    set_standard_property(descr, player, name, MESGPROP_DOING, "Doing", mesg);
}


/**
 * Implementation of @unlock command
 *
 * Unlocks a given object.  This does do permission checks.
 *
 * @param descr the descriptor of the person unlocking the object.
 * @param player the player unlocking the object.
 * @param name the name to find
 */
void
do_unlock(int descr, dbref player, const char *name)
{
    dbref object;

    if ((object = match_controlled(descr, player, name)) != NOTHING) {
        CLEARLOCK(object);
        notifyf_nolisten(player, "Unlocked.");
    }
}
