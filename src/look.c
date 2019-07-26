/** @file look.c
 *
 * Implementation of different aspects of the 'look' command.  This is
 * a surprisingly complicated thing!
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "db.h"
#include "dbsearch.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "match.h"
#include "mpi.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

/**
 * Notifies a player of the owner of 'thing'.  This is the message you
 * see, for instance, if you 'examine' an object you do not own.
 *
 * @private
 * @param player the player to notify
 * @param thing the thing to get owner information for
 */
static void
print_owner(dbref player, dbref thing)
{
    char buf[BUFFER_LEN];

    switch (Typeof(thing)) {
        case TYPE_PLAYER:
            snprintf(buf, sizeof(buf), "%s is a player.", NAME(thing));
            break;
        case TYPE_ROOM:
        case TYPE_THING:
        case TYPE_EXIT:
        case TYPE_PROGRAM:
            snprintf(buf, sizeof(buf), "Owner: %s", NAME(OWNER(thing)));
            break;
        case TYPE_GARBAGE:
            snprintf(buf, sizeof(buf), "%s is garbage.", NAME(thing));
            break;
    }

    notify(player, buf);
}

/**
 * Helper function to see if a given player can see a given thing.
 *
 * This does stuff such as check DARK flags and applies the other
 * "business logic" -- dark sleepers, nuances around if the player controls
 * the given object, etc.  can_see_loc is a boolean, true if the player is
 * able to see the surrounding room (i.e. that it isn't dark)
 *
 * @private
 * @param player the player we are checking visibility for
 * @param thing the thing we are checking the visibility of
 * @param can_see_loc is the location player is in visible?
 * @return boolean true if 'thing' is visible to 'player'.
 */
static int
can_see(dbref player, dbref thing, int can_see_loc)
{
    if (player == thing || Typeof(thing) == TYPE_EXIT || Typeof(thing) == TYPE_ROOM)
        return 0;

    if (can_see_loc) {
        switch (Typeof(thing)) {
            case TYPE_PROGRAM:
                return ((FLAGS(thing) & VEHICLE) || controls(player, thing));
            case TYPE_PLAYER:
                if (tp_dark_sleepers) {
                    return (!Dark(thing) && PLAYER_DESCRCOUNT(thing));
                }
            default:
                return (!Dark(thing) || (controls(player, thing) && !(FLAGS(player) & STICKY)));
        }
    } else {
        /* can't see loc */
        return (controls(player, thing) && !(FLAGS(player) & STICKY));
    }
}

/**
 * Display the contents portion of a given object 'loc' to player 'player'
 *
 * contents_name is a message that is displayed prior to contents being
 * shown.  If there are no visible contents, this message isn't displayed;
 * in the case of no visible contents, this call produces no output to the
 * player.
 *
 * Visibility is calculated with the can_see function.
 *
 * @see can_see
 *
 * @private
 * @param player the player looking.
 * @param loc the object to display contents for.
 * @param contents_name the prefix message to show before contents.
 */
static void
look_contents(dbref player, dbref loc, const char *contents_name)
{
    dbref thing;
    dbref can_see_loc;
    char unparse_buf[BUFFER_LEN];

    /* check to see if he can see the location */
    can_see_loc = (!Dark(loc) || controls(player, loc));

    /* check to see if there is anything there */
    DOLIST(thing, CONTENTS(loc)) {
        if (can_see(player, thing, can_see_loc)) {
            /* something exists!  show him everything */
            notify(player, contents_name);

            DOLIST(thing, CONTENTS(loc)) {
                if (can_see(player, thing, can_see_loc)) {
                    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
                    notify(player, unparse_buf);
                }
            }

            break;      /* we're done */
        }
    }
}

/**
 * Does a "simple look", which is to say, just views the description
 *
 * This is a component of the look command; the description portion.
 * The description is processed for MPI and @-symbol MUF calls.
 *
 * @see exec_or_notify
 *
 * This displays the 'You see nothing special' message if there is no
 * description set.
 *
 * @private
 * @param descr the descriptor of the person looking
 * @param player the person looking
 * @param thing the thing being looked at.
 */
static void
look_simple(int descr, dbref player, dbref thing)
{
    if (GETDESC(thing)) {
        exec_or_notify(descr, player, thing, GETDESC(thing), "(@Desc)",
                       Prop_Blessed(thing, MESGPROP_DESC) ? MPI_ISBLESSED : 0);
    } else {
        notify(player, tp_description_default);
    }
}

/**
 * Look at a room
 *
 * This handles all the nuances of looking at a room; displaying room name,
 * description, title, and success prop processing.
 *
 * @private
 * @param descr the descriptor of the looking person
 * @param player the looking person
 * @param loc the location to look at.
 */
void
look_room(int descr, dbref player, dbref loc)
{
    char obj_num[20];
    char unparse_buf[BUFFER_LEN];

    if (loc == NOTHING) return;

    /* tell him the name, and the number if he can link to it */
    unparse_object(player, loc, unparse_buf, sizeof(unparse_buf));
    notify(player, unparse_buf);

    /* tell him the description
     *
     * This doesn't use look_simple because look_simple has a message
     * for no-description and rooms with no descriptions have no message
     * at all.
     *
     * @TODO: refactor simple_look to take a no-description message and
     *        use it for all description processing.  If the no-description
     *        message is NULL, don't show any message, for instance.
     *
     *        We can even have simple_look process idesc's ... or maybe
     *        even take this whole if statement into simple_look
     */
    if (Typeof(loc) == TYPE_ROOM) {
        if (GETDESC(loc)) {
            exec_or_notify(descr, player, loc, GETDESC(loc), "(@Desc)",
                           Prop_Blessed(loc, MESGPROP_DESC) ? MPI_ISBLESSED : 0);
        }

        /* tell him the appropriate messages if he has the key
         *
         * This is what processes @succ messages
         */
        can_doit(descr, player, loc, 0);
    } else {
        if (GETIDESC(loc)) {
            exec_or_notify(descr, player, loc, GETIDESC(loc), "(@Idesc)",
                           Prop_Blessed(loc, MESGPROP_IDESC) ? MPI_ISBLESSED : 0);
        }
    }

    ts_useobject(loc);

    /* tell him the contents */
    look_contents(player, loc, "Contents:");
    snprintf(obj_num, sizeof(obj_num), "#%d", loc);
    envpropqueue(descr, player, loc, player, loc, NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
}

/**
 * Implementation for 'look' command (aka read)
 *
 * If name (the thing we're looking at) is "" or "here", this will be
 * routed to look_room.
 *
 * @see look_room
 *
 * Otherwise, match and look.  This does have a concept of look-traps
 * (prop-based pseudo-objects) which are checked if either nothing
 * is found (looktrap searched for on current room) or if the 'detail'
 * parameter is searched.  Look trap props are under DETAILS_PROPDIR,
 * which is usually _details.
 *
 * Look-traps support exit-style aliasing and will run MPI or @-style
 * MUF calls.
 *
 * @see exec_or_notify
 *
 * Permissions checking is done.
 *
 * @param descr the descriptor of the person running the call
 * @param player the player running the call
 * @param name The name of what is being looked at or "" for here
 * @param detail the "look trap" or detail prop to examine, or ""
 */
void
do_look_at(int descr, dbref player, const char *name, const char *detail)
{
    dbref thing;
    struct match_data md;
    char buf[BUFFER_LEN];
    char obj_num[20];

    /* Rooms are presented slightly differently */
    if (*name == '\0' || !strcasecmp(name, "here")) {
        look_room(descr, player, LOCATION(player));
    } else {
        /* look at a thing here */
        init_match(descr, player, name, NOTYPE, &md);
        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);

        if (Wizard(OWNER(player))) {
            match_absolute(&md);
            match_player(&md);
        }

        match_here(&md);
        match_me(&md);

        thing = match_result(&md);

        if (thing != NOTHING && thing != AMBIGUOUS && !*detail) {
            /* The syntax used was "look <something>" and we found the
             * <something> with match.
             */
            switch (Typeof(thing)) {
                /* The rules for rooms, players, and things are all
                 * slightly different.
                 *
                 * Rooms are the most basic.  Players and things are very
                 * similar, except HAVEN'd things do not show contents,
                 *
                 * @TODO: This could be simplified; if TYPE_ROOM, then
                 *        run look_room.  Else, use the default look.
                 *        PLAYER / THING / OTHER have near 100% overlap
                 *        with only a few little quirks ... for instance,
                 *        you would display contents if thing is player or
                 *        (thing and !HAVEN).  We could reduce a lot of
                 *        redundant lines here, just be careful with the
                 *        logic structure to maintain existing functionality.
                 */
                case TYPE_ROOM:
                    if (LOCATION(player) != thing
                        && !can_link_to(player, TYPE_ROOM, thing)) {
                        notify(player,
                               "Permission denied. (you're not where you want to look, and can't link to it)");
                    } else {
                        look_room(descr, player, thing);
                    }

                    break;
                case TYPE_PLAYER:
                    if (LOCATION(player) != LOCATION(thing)
                        && !controls(player, thing)) {
                        notify(player,
                               "Permission denied. (Your location isn't the same as what you're looking at)");
                    } else {
                        look_simple(descr, player, thing);
                        look_contents(player, thing, "Carrying:");
                        snprintf(obj_num, sizeof(obj_num), "#%d", thing);
                        envpropqueue(descr, player, thing, player, thing,
                                     NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
                    }

                    break;
                case TYPE_THING:
                    if (LOCATION(player) != LOCATION(thing)
                        && LOCATION(thing) != player
                        && !controls(player, thing)) {
                        notify(player,
                               "Permission denied. (You're not in the same room as or carrying the object)");
                    } else {
                        look_simple(descr, player, thing);

                        if (!(FLAGS(thing) & HAVEN)) {
                            look_contents(player, thing, "Contains:");
                            ts_useobject(thing);
                        }

                        snprintf(obj_num, sizeof(obj_num), "#%d", thing);
                        envpropqueue(descr, player, thing, player, thing,
                                     NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);
                    }

                    break;
                default:
                    look_simple(descr, player, thing);

                    if (Typeof(thing) != TYPE_PROGRAM)
                        ts_useobject(thing);

                    snprintf(obj_num, sizeof(obj_num), "#%d", thing);
                    envpropqueue(descr, player, thing, player, thing,
                                 NOTHING, LOOK_PROPQUEUE, obj_num, 1, 1);

                    break;
            }
        } else if (thing == NOTHING || (*detail && thing != AMBIGUOUS)) {
            /* If we get to this block of code, then either we are looking
             * for something and didn't find it, so we will check look
             * traps, or we used syntax: look <thing>=<detail>
             */
            int ambig_flag = 0;
            char propname[BUFFER_LEN];
            PropPtr propadr, pptr, lastmatch = NULL;

            /* @TODO This does something that is kind of ... technially
             *       wrong maybe?  Let's say you have a looktrap called
             *       'feh' on the current room.  @set here=_details/feh:Hi!
             *
             *       If you type 'look feh', you'll see it -- that's cool.
             *       But if you type 'look feh=whatever' it will trigger 'feh'
             *       and ignore the look trap.  It seems to be in that
             *       case an error would be 'more useful', or perhaps we
             *       should use 'detail' instead of 'name' if detail is
             *       provided and name does not match.
             *
             *       This is super edge-case-y but it is kind of odd
             *       so pointing it out.
             */
            if (thing == NOTHING) {
                /* We will look for details in our current location if
                 * we didn't match a thing.
                 */
                thing = LOCATION(player);
                snprintf(buf, sizeof(buf), "%s", name);
            } else {
                snprintf(buf, sizeof(buf), "%s", detail);
            }

#ifdef DISKBASE
            fetchprops(thing, DETAILS_PROPDIR);
#endif

            lastmatch = NULL;
            propadr = first_prop(thing, DETAILS_PROPDIR, &pptr, propname, sizeof(propname));

            /* Iterate over the props and do 'exit style' prop matching */
            while (propadr) {
                if (exit_prefix(propname, buf)) {
                    if (lastmatch) {
                        lastmatch = NULL;
                        ambig_flag = 1;
                        break;
                    } else {
                        lastmatch = propadr;
                    }
                }

                propadr = next_prop(pptr, propadr, propname, sizeof(propname));
            }

            if (lastmatch && PropType(lastmatch) == PROP_STRTYP) {
#ifdef DISKBASE
                propfetch(thing, lastmatch);    /* DISKBASE PROPVALS */
#endif
                exec_or_notify(descr, player, thing, PropDataStr(lastmatch), "(@detail)",
                               (PropFlags(lastmatch) & PROP_BLESSED) ? MPI_ISBLESSED : 0);
            } else if (ambig_flag) {
                notifyf_nolisten(player, match_msg_ambiguous(buf, 0));
            } else if (*detail) {
                notify(player, tp_description_default);
            } else {
                notifyf_nolisten(player, match_msg_nomatch(buf, 0));
            }
        } else {
            notifyf_nolisten(player, match_msg_ambiguous(detail, 0));
        }
    }
}

/**
 * Generate a long-form text description of the flags on a given object.
 *
 * Note: This uses a static buffer so it is not threadsafe and the contents
 * of the buffer may well mutate if you don't copy the string elsewhere.
 *
 * @private
 * @param thing to generate description for
 * @return string containing descriptive flag text
 */
static const char *
flag_description(dbref thing)
{
    static char buf[BUFFER_LEN];
    int jj;

    strcpyn(buf, sizeof(buf), "Type: ");

    switch (Typeof(thing)) {
        case TYPE_ROOM:
            strcatn(buf, sizeof(buf), "ROOM");
            break;
        case TYPE_EXIT:
            strcatn(buf, sizeof(buf), "EXIT/ACTION");
            break;
        case TYPE_THING:
            strcatn(buf, sizeof(buf), "THING");
            break;
        case TYPE_PLAYER:
            strcatn(buf, sizeof(buf), "PLAYER");
            break;
        case TYPE_PROGRAM:
            strcatn(buf, sizeof(buf), "PROGRAM");
            break;
        case TYPE_GARBAGE:
            strcatn(buf, sizeof(buf), "GARBAGE");
            break;
        default:
            strcatn(buf, sizeof(buf), "***UNKNOWN TYPE***");
            break;
    }

    if (FLAGS(thing) & ~TYPE_MASK & ~DUMP_MASK) {
        /* print flags */
        strcatn(buf, sizeof(buf), "  Flags:");

        if (FLAGS(thing) & WIZARD)
            strcatn(buf, sizeof(buf), " WIZARD");

        if (FLAGS(thing) & QUELL)
            strcatn(buf, sizeof(buf), " QUELL");

        if (FLAGS(thing) & STICKY) {
            if (Typeof(thing) == TYPE_PROGRAM) {
                strcatn(buf, sizeof(buf), " SETUID");
            } else if (Typeof(thing) == TYPE_PLAYER) {
                strcatn(buf, sizeof(buf), " SILENT");
            } else {
                strcatn(buf, sizeof(buf), " STICKY");
            }
        }

        if (FLAGS(thing) & DARK) {
            if (Typeof(thing) == TYPE_PROGRAM) {
                strcatn(buf, sizeof(buf), " DEBUG");
            } else {
                strcatn(buf, sizeof(buf), " DARK");
            }
        }

        if (FLAGS(thing) & LINK_OK)
            strcatn(buf, sizeof(buf), " LINK_OK");

        if (FLAGS(thing) & KILL_OK)
            strcatn(buf, sizeof(buf), " KILL_OK");

        if ((jj = MLevRaw(thing))) {
            strcatn(buf, sizeof(buf), " MUCKER");
            strcatn(buf, sizeof(buf), (char[]){jj+48,0});	 
        }

        if (FLAGS(thing) & BUILDER) {
            if (Typeof(thing) == TYPE_PROGRAM) {
                strcatn(buf, sizeof(buf), " BOUND");
            } else {
                strcatn(buf, sizeof(buf), " BUILDER");
            }
        }

        if (FLAGS(thing) & CHOWN_OK) {
            if (Typeof(thing) == TYPE_PLAYER) {
                strcatn(buf, sizeof(buf), " COLOR");
            } else {
                strcatn(buf, sizeof(buf), " CHOWN_OK");
            }
        }

        if (FLAGS(thing) & JUMP_OK)
            strcatn(buf, sizeof(buf), " JUMP_OK");

        if (FLAGS(thing) & VEHICLE) {
            if (Typeof(thing) == TYPE_PROGRAM) {
                strcatn(buf, sizeof(buf), " VIEWABLE");
            } else {
                strcatn(buf, sizeof(buf), " VEHICLE");
            }
        }

        if (FLAGS(thing) & YIELD)
            strcatn(buf, sizeof(buf), " YIELD");

        if (FLAGS(thing) & OVERT)
            strcatn(buf, sizeof(buf), " OVERT");

        if (FLAGS(thing) & XFORCIBLE) {
            if (Typeof(thing) == TYPE_EXIT) {
                strcatn(buf, sizeof(buf), " XPRESS");
            } else {
                strcatn(buf, sizeof(buf), " XFORCIBLE");
            }
        }

        if (FLAGS(thing) & ZOMBIE)
            strcatn(buf, sizeof(buf), " ZOMBIE");

        if (FLAGS(thing) & GUEST) {
            if (Typeof(thing) == TYPE_PLAYER || Typeof(thing) == TYPE_THING) {
                strcatn(buf, sizeof(buf), " GUEST");
            } else {
                strcatn(buf, sizeof(buf), " NOGUEST");
            }
        }

        if (FLAGS(thing) & HAVEN) {
            if (Typeof(thing) == TYPE_PROGRAM) {
                strcatn(buf, sizeof(buf), " HARDUID");
            } else if (Typeof(thing) == TYPE_THING) {
                strcatn(buf, sizeof(buf), " HIDE");
            } else {
                strcatn(buf, sizeof(buf), " HAVEN");
            }
        }

        if (FLAGS(thing) & ABODE) {
            if (Typeof(thing) == TYPE_PROGRAM) {
                strcatn(buf, sizeof(buf), " AUTOSTART");
            } else if (Typeof(thing) == TYPE_EXIT) {
                strcatn(buf, sizeof(buf), " ABATE");
            } else {
                strcatn(buf, sizeof(buf), " ABODE");
            }
        }
    }

    return buf;
}

/**
 * List props on an object with support for wildcards.
 *
 * It supports the '*' wildcard and the '**' recursive wildcard.
 * They work pretty much how you would expect.
 *
 * There is NO permission check with regards to if 'player' is allowed
 * to see props on 'thing', however, permissions are checked to make
 * sure 'player' cannot see hidden props unless permitted.
 *
 * Does some weird gymnastics around legacy gender/legacy guest props.
 * If tp_show_legacy_props is false, it will hide these props from the
 * list.
 *
 * @private
 * @param player the player searching props
 * @param thing the thing we are checking props on
 * @param dir root dir to search from - used for recursion -- pass "" for root
 * @param wild the pattern to search for.
 * @return int number of props returned.
 */
static int
listprops_wildcard(dbref player, dbref thing, const char *dir, const char *wild)
{
    char propname[BUFFER_LEN];
    char wld[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char *ptr, *wldcrd;
    PropPtr propadr, pptr;
    int i, cnt = 0;
    int recurse = 0;

    strcpyn(wld, sizeof(wld), wild);
    i = strlen(wld);

    if (i && wld[i - 1] == PROPDIR_DELIMITER)
        strcatn(wld, sizeof(wld), "*");

    for (wldcrd = wld; *wldcrd == PROPDIR_DELIMITER; wldcrd++) ;

    if (!strcmp(wldcrd, "**"))
        recurse = 1;

    for (ptr = wldcrd; *ptr && *ptr != PROPDIR_DELIMITER; ptr++) ;

    if (*ptr)
        *ptr++ = '\0';

    propadr = first_prop(thing, (char *) dir, &pptr, propname, sizeof(propname));

    /* Iterate over the propdir we are working on. */
    while (propadr) {
        if (equalstr(wldcrd, propname)) {
            const char *current, *tmpname;
            static const char *legacy_gender;
            static const char *legacy_guest;
            current = tp_gender_prop;
            legacy_gender = LEGACY_GENDER_PROP;
            legacy_guest = LEGACY_GUEST_PROP;

            snprintf(buf, sizeof(buf), "%s%c%s", dir, PROPDIR_DELIMITER, propname);
            tmpname = buf;

            while (*current == PROPDIR_DELIMITER)
                current++;

            while (*legacy_gender == PROPDIR_DELIMITER)
                legacy_gender++;

            while (*legacy_guest == PROPDIR_DELIMITER)
                legacy_guest++;

            while (*tmpname == PROPDIR_DELIMITER)
                tmpname++;

            /* Check if we're skipping over legacy props. */
            if (!tp_show_legacy_props &&
                (!strcasecmp(tmpname, legacy_guest) || (!strcasecmp(tmpname, legacy_gender) && strcasecmp(current, legacy_gender)))) {
                propadr = next_prop(pptr, propadr, propname, sizeof(propname));
                continue;
            }

            if (!Prop_System(buf) && (!Prop_Hidden(buf) || Wizard(OWNER(player)))) {
                if (!*ptr || recurse) {
                    cnt++;

                    displayprop(player, thing, buf, buf2, sizeof(buf2));
                    notify(player, buf2);
                }

                if (recurse)
                    ptr = "**";

                cnt += listprops_wildcard(player, thing, buf, ptr);
            }
        }

        propadr = next_prop(pptr, propadr, propname, sizeof(propname));
    }

    return cnt;
}

/**
 * Implementation of the examine command
 *
 * This is a pretty huge infodump of the object 'name'.  Optionally, if
 * 'dir' is provided, we'll dump a listing of props instead.  'dir' can
 * have * or ** (recursive) wildcards.
 *
 * So it is tons of little if <has data> then <show data> types of
 * things, but is overall pretty simple.
 *
 * This does permission checking to make sure the user can examine
 * the given object.
 *
 * @param descr the descriptor of the examining player.
 * @param player the player doing the examining
 * @param name the name of the thing to be examined - defaults to 'here'
 * @param dir the prop directory to look for, or "" to not show props.
 */
void
do_examine(int descr, dbref player, const char *name, const char *dir)
{
    dbref thing;
    char unparse_buf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    const char *temp;
    dbref content;
    dbref exit;
    int cnt;
    struct match_data md;
    struct tm *time_tm;     /* used for timestamps */

    if (*name == '\0' || !strcasecmp(name, "here")) {
        thing = LOCATION(player);
    } else {
        /* look it up */
        init_match(descr, player, name, NOTYPE, &md);
        match_everything(&md);

        /* get result */
        if ((thing = noisy_match_result(&md)) == NOTHING)
            return;
    }

    if (!can_link(player, thing) && !test_lock_false_default(descr, player, thing, MESGPROP_READLOCK)) {
        print_owner(player, thing);
        return;
    }

    if (*dir) {
        /* show him the properties */
        cnt = listprops_wildcard(player, thing, "", dir);
        notifyf(player, "%d propert%s listed.", cnt, (cnt == 1 ? "y" : "ies"));
        return;
    }

    unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));

    switch (Typeof(thing)) {
        case TYPE_ROOM:
            snprintf(buf, sizeof(buf), "%.*s  Owner: %s  Parent: ",
                     (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                     unparse_buf, NAME(OWNER(thing)));
            unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
            strcatn(buf, sizeof(buf), unparse_buf);
            break;
        case TYPE_THING:
            snprintf(buf, sizeof(buf), "%.*s  Owner: %s  Value: %d",
                     (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                     unparse_buf, NAME(OWNER(thing)), GETVALUE(thing));
            break;
        case TYPE_PLAYER:
            snprintf(buf, sizeof(buf), "%.*s  %s: %d  ",
                     (int) (BUFFER_LEN - strlen(tp_cpennies) - 35),
                     unparse_buf, tp_cpennies, GETVALUE(thing));
            break;
        case TYPE_EXIT:
        case TYPE_PROGRAM:
            snprintf(buf, sizeof(buf), "%.*s  Owner: %s",
                     (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                     unparse_buf, NAME(OWNER(thing)));
            break;
        case TYPE_GARBAGE:
            strcpyn(buf, sizeof(buf), unparse_buf);
            break;
    }

    notify(player, buf);

    if (tp_verbose_examine)
        notify(player, flag_description(thing));

    if (GETDESC(thing))
        notify(player, GETDESC(thing));

    if (strcmp(temp = unparse_boolexp(player, GETLOCK(thing), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Key: %s", temp);

    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_LINKLOCK), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Link_OK Key: %s", temp);

    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_CHLOCK), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Chown_OK Key: %s", temp);

    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_CONLOCK), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Container Key: %s", temp);

    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_FLOCK), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Force Key: %s", temp);

    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_READLOCK), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Read Key: %s", temp);

    if (strcmp(temp = unparse_boolexp(player, get_property_lock(thing, MESGPROP_OWNLOCK), 1), PROP_UNLOCKED_VAL))
        notifyf(player, "Ownership Key: %s", temp);

    if (GETSUCC(thing)) {
        notifyf(player, "Success: %s", GETSUCC(thing));
    }

    if (GETFAIL(thing)) {
        notifyf(player, "Fail: %s", GETFAIL(thing));
    }

    if (GETDROP(thing)) {
        notifyf(player, "Drop: %s", GETDROP(thing));
    }

    if (GETOSUCC(thing)) {
        notifyf(player, "Osuccess: %s", GETOSUCC(thing));
    }

    if (GETOFAIL(thing)) {
        notifyf(player, "Ofail: %s", GETOFAIL(thing));
    }

    if (GETODROP(thing)) {
        notifyf(player, "Odrop: %s", GETODROP(thing));
    }

    if (GETDOING(thing)) {
        notifyf(player, "Doing: %s", GETDOING(thing));
    }

    if (GETOECHO(thing)) {
        notifyf(player, "Oecho: %s", GETOECHO(thing));
    }

    if (GETPECHO(thing)) {
        notifyf(player, "Pecho: %s", GETPECHO(thing));
    }

    if (GETIDESC(thing)) {
        notifyf(player, "Idesc: %s", GETIDESC(thing));
    }

    /* Timestamps */
    time_tm = localtime(&(DBFETCH(thing)->ts_created));
    strftime(buf, BUFFER_LEN, "%c %Z", time_tm);
    notifyf(player, "Created:  %s", buf);

    time_tm = localtime(&(DBFETCH(thing)->ts_modified));
    strftime(buf, BUFFER_LEN, "%c %Z", time_tm);
    notifyf(player, "Modified: %s", buf);

    time_tm = localtime(&(DBFETCH(thing)->ts_lastused));
    strftime(buf, BUFFER_LEN, "%c %Z", time_tm);
    notifyf(player, "Lastused: %s", buf);

    if (Typeof(thing) == TYPE_PROGRAM) {
        snprintf(buf, sizeof(buf), "Usecount: %d     Instances: %d",
                 DBFETCH(thing)->ts_usecount, PROGRAM_INSTANCES(thing));
    } else {
        snprintf(buf, sizeof(buf), "Usecount: %d", DBFETCH(thing)->ts_usecount);
    }

    notify(player, buf);

    notify(player, "[ Use 'examine <object>=/' to list root properties. ]");

    notifyf(player, "Memory used: %ld bytes", size_object(thing, 1));

    /* show him the contents */
    if (CONTENTS(thing) != NOTHING) {
        if (Typeof(thing) == TYPE_PLAYER)
            notify(player, "Carrying:");
        else
            notify(player, "Contents:");

        DOLIST(content, CONTENTS(thing)) {
            unparse_object(player, content, unparse_buf, sizeof(unparse_buf));
            notify(player, unparse_buf);
        }
    }

    switch (Typeof(thing)) {
        case TYPE_ROOM:
            /* tell him about exits */
            if (EXITS(thing) != NOTHING) {
                notify(player, "Exits:");

                DOLIST(exit, EXITS(thing)) {
                    unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
                    notify(player, unparse_buf);
                }
            } else {
                notify(player, "No exits.");
            }

            /* print dropto if present */
            if (DBFETCH(thing)->sp.room.dropto != NOTHING) {
                unparse_object(player, DBFETCH(thing)->sp.room.dropto, unparse_buf, sizeof(unparse_buf));
                notifyf(player, "Dropped objects go to: %s", unparse_buf);
            }

            break;
        case TYPE_THING:
            /* print home */
            unparse_object(player, THING_HOME(thing), unparse_buf, sizeof(unparse_buf));
            notifyf(player, "Home: %s", unparse_buf);

            /* print location if player can link to it */
            if (LOCATION(thing) != NOTHING && (controls(player, LOCATION(thing))
                || can_link_to(player, NOTYPE, LOCATION(thing)))) {
                unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
                notifyf(player, "Location: %s", unparse_buf);
            }

            /* print thing's actions, if any */
            if (EXITS(thing) != NOTHING) {
                notify(player, "Actions/exits:");

                DOLIST(exit, EXITS(thing)) {
                    unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
                    notify(player, unparse_buf);
                }
            } else {
                notify(player, "No actions attached.");
            }

            break;
        case TYPE_PLAYER:
            /* print home */
            unparse_object(player, PLAYER_HOME(thing), unparse_buf, sizeof(unparse_buf));
            notifyf(player, "Home: %s", unparse_buf);       /* home */

            /* print location if player can link to it */
            if (LOCATION(thing) != NOTHING && (controls(player, LOCATION(thing))
                || can_link_to(player, NOTYPE, LOCATION(thing)))) {
                unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
                notifyf(player, "Location: %s", unparse_buf);
            }

            /* print player's actions, if any */
            if (EXITS(thing) != NOTHING) {
                notify(player, "Actions/exits:");

                DOLIST(exit, EXITS(thing)) {
                    unparse_object(player, exit, unparse_buf, sizeof(unparse_buf));
                    notify(player, unparse_buf);
                }
            } else {
                notify(player, "No actions attached.");
            }

            break;
        case TYPE_EXIT:
            if (LOCATION(thing) != NOTHING) {
                unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
                notifyf(player, "Source: %s", unparse_buf);
            }

            /* print destinations */
            if (DBFETCH(thing)->sp.exit.ndest == 0)
                break;

            for (int i = 0; i < DBFETCH(thing)->sp.exit.ndest; i++) {
                switch ((DBFETCH(thing)->sp.exit.dest)[i]) {
                    case NOTHING:
                        break;
                    case HOME:
                        notify(player, "Destination: *HOME*");
                        break;
                    default:
                        unparse_object(player, (DBFETCH(thing)->sp.exit.dest)[i], unparse_buf, sizeof(unparse_buf));
                        notifyf(player, "Destination: %s", unparse_buf);
                        break;
                }
            }

            break;
        case TYPE_PROGRAM:
            if (PROGRAM_SIZ(thing)) {
                struct timeval tv = PROGRAM_PROFTIME(thing);
                notifyf(player, "Program compiled size: %d instructions", PROGRAM_SIZ(thing));
                notifyf(player, "Cumulative runtime: %d.%06d seconds ", (int) tv.tv_sec,
                        (int) tv.tv_usec);
            } else {
                notify(player, "Program not compiled.");
            }

            /* print location if player can link to it */
            if (LOCATION(thing) != NOTHING && (controls(player, LOCATION(thing))
                || can_link_to(player, NOTYPE, LOCATION(thing)))) {
                unparse_object(player, LOCATION(thing), unparse_buf, sizeof(unparse_buf));
                notifyf(player, "Location: %s", unparse_buf);
            }

            break;
        default:
            /* do nothing */
            break;
    }
}

/**
 * Implementation of uptime command
 *
 * Just outputs uptime information to player.
 *
 * @param player the player doing the call
 */
void
do_uptime(dbref player)
{
    char buf[BUFFER_LEN];
    time_t startup = get_property_value(0, SYS_STARTUPTIME_PROP);
    strftime(buf, sizeof(buf), "%c %Z", localtime(&startup));
    notifyf(player, "Up %s since %s", timestr_long((long)(time(NULL) - startup)), buf);
}

/**
 * Implementation of inventory command
 *
 * It is important to note that this is different from the look_contents
 * call because it is simpler.  A player can see everything in their inventory
 * regardless of darkness or player flags.
 *
 * @param player the player doing the call.
 */
void
do_inventory(dbref player)
{
    dbref thing;
    char unparse_buf[BUFFER_LEN];

    if ((thing = CONTENTS(player)) == NOTHING) {
        notify(player, "You aren't carrying anything.");
    } else {
        notify(player, "You are carrying:");
            DOLIST(thing, thing) {
                unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
                notify(player, unparse_buf);
            }
    }

    do_score(player);
}

/**
 * Initialize the checkflags search system
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
int
init_checkflags(dbref player, const char *flags, struct flgchkdat *check)
{
    char buf[BUFFER_LEN];
    char *cptr;
    int output_type = 0;
    int mode = 0;

    strcpyn(buf, sizeof(buf), flags);

    for (cptr = buf; *cptr && (*cptr != ARG_DELIMITER); cptr++) ;

    if (*cptr == ARG_DELIMITER)
        *(cptr++) = '\0';

    flags = buf;
    skip_whitespace((const char **)&cptr);

    /* Determine output type */
    if (!*cptr) {
        output_type = 0;
    } else if (string_prefix("owners", cptr)) {
        output_type = 1;
    } else if (string_prefix("locations", cptr)) {
        output_type = 3;
    } else if (string_prefix("links", cptr)) {
        output_type = 2;
    } else if (string_prefix("count", cptr)) {
        output_type = 4;
    } else if (string_prefix("size", cptr)) {
        output_type = 5;
    } else {
        output_type = 0;
    }

    memset(check, 0, sizeof(struct flgchkdat));

    /* Flags are processed in a loop.  Most flags except size are just
     * a single character.  Size must come last because it is a weirdo.
     */
    while (*flags) {
        switch (toupper(*flags)) {
            case '!':
                if (mode)
                    mode = 0;
                else
                    mode = 2;

                break;
            case 'R':
                if (mode) {
                    check->isnotroom = 1;
                } else {
                    check->fortype = 1;
                    check->istype = TYPE_ROOM;
                }

                break;
            case 'T':
                if (mode) {
                    check->isnotthing = 1;
                } else {
                    check->fortype = 1;
                    check->istype = TYPE_THING;
                }

                break;
            case 'E':
                if (mode) {
                    check->isnotexit = 1;
                } else {
                    check->fortype = 1;
                    check->istype = TYPE_EXIT;
                }

                break;
            case 'P':
                if (mode) {
                    check->isnotplayer = 1;
                } else {
                    check->fortype = 1;
                    check->istype = TYPE_PLAYER;
                }

                break;
            case 'F':
                if (mode) {
                    check->isnotprog = 1;
                } else {
                    check->fortype = 1;
                    check->istype = TYPE_PROGRAM;
                }

                break;
            case '~':
            case '^':
                check->loadedsize = (Wizard(player) && *flags == '^');
                check->size = atoi(flags + 1);
                check->issize = !mode;

                while (isdigit(flags[1]))
                    flags++;

                break;
            case 'U':
                check->forlink = 1;

                if (mode) {
                    check->islinked = 1;
                } else {
                    check->islinked = 0;
                }

                break;
            case '@':
                check->forold = 1;

                if (mode) {
                    check->isold = 0;
                } else {
                    check->isold = 1;
                }

                break;
            case '0':
                if (mode) {
                    check->isnotzero = 1;
                } else {
                    check->forlevel = 1;
                    check->islevel = 0;
                }

                break;
            case '1':
                if (mode) {
                    check->isnotone = 1;
                } else {
                    check->forlevel = 1;
                    check->islevel = 1;
                }

                break;
            case '2':
                if (mode) {
                    check->isnottwo = 1;
                } else {
                    check->forlevel = 1;
                    check->islevel = 2;
                }

                break;
            case '3':
                if (mode) {
                    check->isnotthree = 1;
                } else {
                    check->forlevel = 1;
                    check->islevel = 3;
                }

                break;
            case 'M':
                if (mode) {
                    check->forlevel = 1;
                    check->islevel = 0;
                } else {
                    check->isnotzero = 1;
                }

                break;
            case 'A':
                if (mode)
                    check->clearflags |= ABODE;
                else
                    check->setflags |= ABODE;

                break;
            case 'B':
                if (mode)
                    check->clearflags |= BUILDER;
                else
                    check->setflags |= BUILDER;

                break;
            case 'C':
                if (mode)
                    check->clearflags |= CHOWN_OK;
                else
                    check->setflags |= CHOWN_OK;

                break;
            case 'D':
                if (mode)
                    check->clearflags |= DARK;
                else
                    check->setflags |= DARK;

                break;
            case 'G':
                if (mode)
                    check->clearflags |= GUEST;
                else
                    check->setflags |= GUEST;

                break;
            case 'H':
                if (mode)
                    check->clearflags |= HAVEN;
                else
                    check->setflags |= HAVEN;

                break;
            case 'J':
                if (mode)
                    check->clearflags |= JUMP_OK;
                else
                    check->setflags |= JUMP_OK;

                break;
            case 'K':
                if (mode)
                    check->clearflags |= KILL_OK;
                else
                    check->setflags |= KILL_OK;

                break;
            case 'L':
                if (mode)
                    check->clearflags |= LINK_OK;
                else
                    check->setflags |= LINK_OK;

                break;
            case 'O':
                if (mode)
                    check->clearflags |= OVERT;
                else
                    check->setflags |= OVERT;

                break;
            case 'Q':
                if (mode)
                    check->clearflags |= QUELL;
                else
                    check->setflags |= QUELL;

                break;
            case 'S':
                if (mode)
                    check->clearflags |= STICKY;
                else
                    check->setflags |= STICKY;

                break;
            case 'V':
                if (mode)
                    check->clearflags |= VEHICLE;
                else
                    check->setflags |= VEHICLE;

                break;
            case 'Y':
                if (mode)
                    check->clearflags |= YIELD;
                else
                    check->setflags |= YIELD;

                break;
            case 'Z':
                if (mode)
                    check->clearflags |= ZOMBIE;
                else
                    check->setflags |= ZOMBIE;

                break;
            case 'W':
                if (mode)
                    check->clearflags |= WIZARD;
                else
                    check->setflags |= WIZARD;

                break;
            case 'X':
                if (mode)
                    check->clearflags |= XFORCIBLE;
                else
                    check->setflags |= XFORCIBLE;

                break;
            case ' ':
                if (mode)
                    mode = 2;

                break;
        }

        if (mode)
            mode--;

        flags++;
    }

    return output_type;
}


/**
 * Checks an object 'what' against a set of flags 'check'
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
int
checkflags(dbref what, struct flgchkdat check)
{
    if (check.fortype && (Typeof(what) != check.istype))
        return (0);

    if (check.isnotroom && (Typeof(what) == TYPE_ROOM))
        return (0);

    if (check.isnotexit && (Typeof(what) == TYPE_EXIT))
        return (0);

    if (check.isnotthing && (Typeof(what) == TYPE_THING))
        return (0);

    if (check.isnotplayer && (Typeof(what) == TYPE_PLAYER))
        return (0);

    if (check.isnotprog && (Typeof(what) == TYPE_PROGRAM))
        return (0);

    if (check.forlevel && (MLevRaw(what) != check.islevel))
        return (0);

    if (check.isnotzero && (MLevRaw(what) == 0))
        return (0);

    if (check.isnotone && (MLevRaw(what) == 1))
        return (0);

    if (check.isnottwo && (MLevRaw(what) == 2))
        return (0);

    if (check.isnotthree && (MLevRaw(what) == 3))
        return (0);

    if (FLAGS(what) & check.clearflags)
        return (0);

    if ((~FLAGS(what)) & check.setflags)
        return (0);

    if (check.forlink) {
        switch (Typeof(what)) {
            case TYPE_ROOM:
                if ((DBFETCH(what)->sp.room.dropto == NOTHING) != (!check.islinked))
                    return (0);

                break;
            case TYPE_EXIT:
                if ((!DBFETCH(what)->sp.exit.ndest) != (!check.islinked))
                    return (0);

                break;
            case TYPE_PLAYER:
            case TYPE_THING:
                if (!check.islinked)
                    return (0);

                break;
            default:
                if (check.islinked)
                    return (0);
        }
    }

    if (check.forold) {
        if (((((time(NULL)) - DBFETCH(what)->ts_lastused) < tp_aging_time) ||
             (((time(NULL)) - DBFETCH(what)->ts_modified) < tp_aging_time))
             != (!check.isold))
            return (0);
    }

    if (check.size) {
        if ((size_object(what, check.loadedsize) < check.size)
            != (!check.issize)) {
            return 0;
        }
    }

    return (1);
}

/**
 * Display an object using output_type
 *
 * This is used as a helper for @find to output a found object to the
 * user using their output_type preference.  For output_types, see
 * init_checkflags
 *
 * @see init_checkflags
 *
 * @private
 * @param player the player to display output to
 * @param obj the object to parse for the player.
 * @param output_type the output type to use.
 */
static void
display_objinfo(dbref player, dbref obj, int output_type)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char buf3[BUFFER_LEN];

    unparse_object(player, obj, buf2, sizeof(buf2));

    switch (output_type) {
        case 1:         /* owners */
            unparse_object(player, OWNER(obj), buf3, sizeof(buf3));
            snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);
            break;
        case 2:         /* links */
            switch (Typeof(obj)) {
                case TYPE_ROOM:
                    unparse_object(player, DBFETCH(obj)->sp.room.dropto,
                                   buf3, sizeof(buf3));
                    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);

                    break;
                case TYPE_EXIT:
                    if (DBFETCH(obj)->sp.exit.ndest == 0) {
                        snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2,
                                 "*UNLINKED*");
                        break;
                    }

                    if (DBFETCH(obj)->sp.exit.ndest > 1) {
                        snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2,
                                 "*METALINKED*");
                        break;
                    }

                    unparse_object(player, DBFETCH(obj)->sp.exit.dest[0], buf3,
                                   sizeof(buf3));
                    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);

                    break;
                case TYPE_PLAYER:
                    unparse_object(player, PLAYER_HOME(obj), buf3, sizeof(buf3));
                    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);

                    break;
                case TYPE_THING:
                    unparse_object(player, THING_HOME(obj), buf3, sizeof(buf3));
                    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);

                    break;
                default:
                    snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, "N/A");
                    break;
            }

            break;
        case 3:     /* locations */
            unparse_object(player, LOCATION(obj), buf3, sizeof(buf3));
            snprintf(buf, sizeof(buf), "%-38.512s  %.512s", buf2, buf3);

            break;
        case 4:
            return;
        case 5:
            snprintf(buf, sizeof(buf), "%-38.512s  %ld bytes.", buf2,
                     size_object(obj, 0));
            break;
        case 0:
        default:
            strcpyn(buf, sizeof(buf), buf2);
            break;
    }

    notify(player, buf);
}

/**
 * Implementation of @find command
 *
 * This implements the find command, which is powered by
 * checkflags.  See that function for full details of how flags work.
 *
 * @see init_checkflags
 *
 * Takes a search string to look for, and iterates over the entire
 * database to find it.  There is an option to charge players for
 * @find's, probably since this is kind of nasty on the DB.
 *
 * @param player the player doing the find
 * @param name the search criteria
 * @param flags the flags for init_checkflags
 */
void
do_find(dbref player, const char *name, const char *flags)
{
    struct flgchkdat check;
    char buf[BUFFER_LEN + 2];
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    strcpyn(buf, sizeof(buf), "*");
    strcatn(buf, sizeof(buf), name);
    strcatn(buf, sizeof(buf), "*");

    if (!payfor(player, tp_lookup_cost)) {
        notifyf(player, "You don't have enough %s.", tp_pennies);
    } else {
        for (dbref i = 0; i < db_top; i++) {
            if (Typeof(i) == TYPE_GARBAGE) continue;

            if ((Wizard(OWNER(player)) || OWNER(i) == OWNER(player)) &&
                checkflags(i, check) && NAME(i) && (!*name
                || equalstr(buf, (char *) NAME(i)))) {
                display_objinfo(player, i, output_type);
                total++;
            }
        }

        notify(player, "***End of List***");
        notifyf(player, "%d objects found.", total);
    }
}

/**
 * Implementation of @owned command
 *
 * Like do_find, this is underpinned by the checkflags system.
 * For details of how the flags work, see init_checkflags
 *
 * This does do permission checks.  Like @find, it iterates over the
 * entire database and supports a lookup cost.
 *
 * @see init_checkflags
 * @see do_find
 *
 * @param player the player doing the call
 * @param name the name of the player to check ownership of - or "" for me
 * @param flags flags passed into init_checkflags
 */
void
do_owned(dbref player, const char *name, const char *flags)
{
    dbref victim;
    struct flgchkdat check;
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    if (!payfor(player, tp_lookup_cost)) {
        notifyf(player, "You don't have enough %s.", tp_pennies);
        return;
    }

    if (Wizard(OWNER(player)) && *name) {
        if ((victim = lookup_player(name)) == NOTHING) {
            notify(player, "I couldn't find that player.");
            return;
        }
    } else
        victim = player;

    for (dbref i = 0; i < db_top; i++) {
        if ((OWNER(i) == OWNER(victim)) && checkflags(i, check)) {
            display_objinfo(player, i, output_type);
            total++;
        }
    }

    notify(player, "***End of List***");
    notifyf(player, "%d objects found.", total);
}

/**
 * Implementation of @trace command
 *
 * This takes a room ref or "" for here and goes up the environment
 * chain until the global environment room is reached or we've traversed
 * 'depth' number of parent rooms.  Each room in the chain is displayed
 * to the user.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param name the room dbref or ""
 * @param depth the number of rooms to iterate over.
 */
void
do_trace(int descr, dbref player, const char *name, int depth)
{
    dbref thing;
    struct match_data md;
    char unparse_buf[BUFFER_LEN];

    if (*name == '\0' || !strcasecmp(name, "here")) {
        thing = LOCATION(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_everything(&md);

        if ((thing = noisy_match_result(&md)) == NOTHING)
            return;
    }

    for (int i = 0; (!depth || i < depth) && thing != NOTHING; i++) {
        unparse_object(player, thing, unparse_buf, sizeof(unparse_buf));
        notify(player, unparse_buf);
        thing = LOCATION(thing);
    }

    notify(player, "***End of List***");
}

/**
 * Implementation of the @entrances command
 *
 * This supports the same sort of flag searches that @find does,
 * except it searches for exits on the given object (which defaults to here)
 *
 * Under the hood, this uses the checkflags series of methods.
 *
 * @see init_checkflags
 * @see checkflags
 * @see do_find
 *
 * See documentation for checkflags for the details of what flags are
 * supported.
 *
 * This does the necessary permission checking.
 *
 * @param descr the descriptor of the player making the call
 * @param player the player making the call
 * @param name the name of the object to search entrances on, or "" for here.
 * @param flags the flags to pass into checkflags
 */
void
do_entrances(int descr, dbref player, const char *name, const char *flags)
{
    dbref thing;
    struct match_data md;
    struct flgchkdat check;
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    if (*name == '\0' || !strcasecmp(name, "here")) {
        thing = LOCATION(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_everything(&md);

        if ((thing = noisy_match_result(&md)) == NOTHING) {
            return;
        }
    }

    if (!controls(OWNER(player), thing)) {
        notify(player,
           "Permission denied. (You can't list entrances of objects you don't control)");
        return;
    }

    init_checkflags(player, flags, &check);

    for (dbref i = 0; i < db_top; i++) {
        if (checkflags(i, check)) {
            switch (Typeof(i)) {
                case TYPE_EXIT:
                    for (int j = DBFETCH(i)->sp.exit.ndest; j--;) {
                        if (DBFETCH(i)->sp.exit.dest[j] == thing) {
                            display_objinfo(player, i, output_type);
                            total++;
                        }
                    }

                    break;
                case TYPE_PLAYER:
                    if (PLAYER_HOME(i) == thing) {
                        display_objinfo(player, i, output_type);
                        total++;
                    }

                    break;
                case TYPE_THING:
                    if (THING_HOME(i) == thing) {
                        display_objinfo(player, i, output_type);
                        total++;
                    }

                    break;
                case TYPE_ROOM:
                    if (DBFETCH(i)->sp.room.dropto == thing) {
                        display_objinfo(player, i, output_type);
                        total++;
                    }

                    break;
                case TYPE_PROGRAM:
                case TYPE_GARBAGE:
                    break;
            }
        }
    }

    notify(player, "***End of List***");
    notifyf(player, "%d objects found.", total);
}

/**
 * Implementation of @contents
 *
 * This searches the contents of a given object, similar to the way @find
 * works except confined to a certain object.  It supports a similar
 * syntax.  If 'name' is not provided, defaults to here.  'flags' can have
 * types and also an output type.
 *
 * This does do appropriate permission checking.
 *
 * @see do_find for more syntatical details.
 * @see checkflags
 *
 * @param descr the descriptor of the person running the command
 * @param player the person running the command
 * @param name the name of the object to check or "" for here
 * @param flags optional "find" syntax flags, or "" for none.
 */
void
do_contents(int descr, dbref player, const char *name, const char *flags)
{
    dbref i;
    dbref thing;
    struct match_data md;
    struct flgchkdat check;
    int total = 0;
    int output_type = init_checkflags(player, flags, &check);

    if (*name == '\0' || !strcasecmp(name, "here")) {
        thing = LOCATION(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_everything(&md);

        if ((thing = noisy_match_result(&md)) == NOTHING) {
            return;
        }
    }

    if (!controls(OWNER(player), thing)) {
        notify(player,
           "Permission denied. (You can't get the contents of something you don't control)");
        return;
    }

    init_checkflags(player, flags, &check);

    DOLIST(i, CONTENTS(thing)) {
        if (checkflags(i, check)) {
            display_objinfo(player, i, output_type);
            total++;
        }
    }

    switch (Typeof(thing)) {
        case TYPE_EXIT:
        case TYPE_PROGRAM:
        case TYPE_GARBAGE:
            i = NOTHING;
            break;
        case TYPE_ROOM:
        case TYPE_THING:
        case TYPE_PLAYER:
            i = EXITS(thing);
            break;
    }

    DOLIST(i, i) {
        if (checkflags(i, check)) {
            display_objinfo(player, i, output_type);
            total++;
        }
    }

    notify(player, "***End of List***");
    notifyf(player, "%d objects found.", total);
}

/**
 * Does a given exit dbref match the given name?
 *
 * Does exit parsing (meaning, it understands the delimited exit aliases)
 * on a given object's name.  If exactMatch is 1, then only an exact
 * match with an alias will pass; if 0, then a prefix match will be done.
 *
 * @see string_prefix
 *
 * @private
 * @param exit the object who's name we are trying to match
 * @param name the name to match
 * @param exactMatch boolean as described above
 * @return boolean true if matched, false if not.
 */
static int
exit_matches_name(dbref exit, const char *name, int exactMatch)
{
    char buf[BUFFER_LEN];
    char *ptr;

    /* @TODO I have a hard time believing this function doesn't exist
     *       elsewhere in a more public fashion.  If I find it while
     *       documenting I'll try to come back and note it.
     */

    strcpyn(buf, sizeof(buf), NAME(exit));

    for (char *ptr2 = ptr = buf; *ptr; ptr = ptr2) {
        while (*ptr2 && *ptr2 != EXIT_DELIMITER)
            ptr2++;

        if (*ptr2)
            *ptr2++ = '\0';

        while (*ptr2 == EXIT_DELIMITER)
            ptr2++;

        if ((exactMatch ? !strcasecmp(name, ptr) : string_prefix(name, ptr)) &&
            DBFETCH(exit)->sp.exit.ndest)
            return 1;
    }

    return 0;
}

/**
 * Checks to see if a given exit name exits on a given obj
 *
 * This is a wrapper around exit_matches_name to check every exit
 * on a given object and see if it matches the given name.  If
 * exactMatch is 0, then string_prefix is used instead of strcasecmp.
 *
 * This does some notification specific to the use of 'do_sweep', to let
 * players know if certain commands are overriden (such as pose, whisper, etc.)
 *
 * @see exit_matches_name
 * @see string_prefix
 * @see do_sweep
 *
 * @private
 * @param player the player doing the lookup
 * @param obj the object to scan for exits on
 * @param name the exit name to search for
 * @param exactMatch boolean as described above
 * @return boolean, true if exit exits, false if not.
 */
static int
exit_match_exists(dbref player, dbref obj, const char *name, int exactMatch)
{
    dbref exit;

    exit = EXITS(obj);
    while (exit != NOTHING) {
        if (exit_matches_name(exit, name, exactMatch)) {
            char unparse_buf[BUFFER_LEN];
            unparse_object(player, obj, unparse_buf, sizeof(unparse_buf));
            notifyf(player, "  %ss are trapped on %.2048s", name, unparse_buf);
            return 1;
        }

        exit = NEXTOBJ(exit);
    }

    return 0;
}

/**
 * Implementation of the sweep command
 *
 * Does a 'security sweep' of a given location, checking it for listening
 * players and objects.  The parameter given is either a room reference
 * or it can be "" to default for here.  This does do permission checking.
 *
 * Security sweeps are done up the environment chain, looking for anything
 * that might be listening or common communication commands that may be
 * overridden in the room and thus used for spying.
 *
 * This has nothing to do with the sweep that removes sleeping players.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param name the room to scan or "" to default to "here"
 */
void
do_sweep(int descr, dbref player, const char *name)
{
    dbref thing, ref, loc;
    int flag, tellflag;
    struct match_data md;
    char buf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];

    if (*name == '\0' || !strcasecmp(name, "here")) {
        thing = LOCATION(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_everything(&md);

        if ((thing = noisy_match_result(&md)) == NOTHING) {
            return;
        }
    }

    /* Permissions check */
    if (*name && !controls(OWNER(player), thing)) {
        notify(player,
               "Permission denied. (You can't perform a security sweep in a room you don't own)");
        return;
    }
    
    unparse_object(player, thing, unparse_buf, sizeof unparse_buf);
    notifyf(player, "Listeners in %s:", unparse_buf);

    ref = CONTENTS(thing);

    /* Check the contents for listeners */
    for (; ref != NOTHING; ref = NEXTOBJ(ref)) {

        switch (Typeof(ref)) {
            case TYPE_PLAYER:
                /* Players are pretty easy, though note this does not
                 * pick up dark players.  So this isn't much of a security
                 * sweep really.
                 */
                if (!Dark(thing) || PLAYER_DESCRCOUNT(ref)) {
                    unparse_object(player, ref, unparse_buf, sizeof unparse_buf);
                    notifyf(player, "  %s is a %splayer.", unparse_buf,
                            PLAYER_DESCRCOUNT(ref) ? "" : "sleeping ");
                }

                break;
            case TYPE_THING:
                /* Things are only picked up if they are listeners */
                if (FLAGS(ref) & (ZOMBIE | LISTENER)) {
                    tellflag = 0;
                    unparse_object(player, ref, unparse_buf, sizeof unparse_buf);
                    snprintf(buf, sizeof(buf), "  %.255s is a", unparse_buf);

                    /* See if the zombie is sleeping */
                    if (FLAGS(ref) & ZOMBIE) {
                        tellflag = 1;

                        if (!PLAYER_DESCRCOUNT(OWNER(ref))) {
                            tellflag = 0;
                            strcatn(buf, sizeof(buf), " sleeping");
                        }

                        strcatn(buf, sizeof(buf), " zombie");
                    }

                    if ((FLAGS(ref) & LISTENER) &&
                        (get_property(ref, LISTEN_PROPQUEUE) ||
                         get_property(ref, WLISTEN_PROPQUEUE) ||
                         get_property(ref, WOLISTEN_PROPQUEUE))) {
                        strcatn(buf, sizeof(buf), " listener");
                        tellflag = 1;
                    }

                    strcatn(buf, sizeof(buf), " object owned by ");
                    unparse_object(player, OWNER(ref), unparse_buf, sizeof unparse_buf);
                    strcatn(buf, sizeof(buf), unparse_buf);
                    strcatn(buf, sizeof(buf), ".");

                    if (tellflag)
                        notify(player, buf);
                }

                /* See if certain commands are overridden */
                exit_match_exists(player, ref, "page", 0);
                exit_match_exists(player, ref, "whisper", 0);

                if (!exit_match_exists(player, ref, "pose", 1))
                    if (!exit_match_exists(player, ref, "pos", 1))
                        exit_match_exists(player, ref, "po", 1);

                exit_match_exists(player, ref, "say", 0);
                break;
        }
    }

    flag = 0;
    loc = thing;

    /* And finally check the environment */
    while (loc != NOTHING) {
        if (!flag) {
            notify(player, "Listening rooms down the environment:");
            flag = 1;
        }

        if ((FLAGS(loc) & LISTENER) &&
            (get_property(loc, LISTEN_PROPQUEUE) ||
             get_property(loc, WLISTEN_PROPQUEUE) ||
             get_property(loc, WOLISTEN_PROPQUEUE))) {
            unparse_object(player, loc, unparse_buf, sizeof(unparse_buf));
            notifyf(player, "  %s is a listening room.", unparse_buf);
        }

        exit_match_exists(player, loc, "page", 0);
        exit_match_exists(player, loc, "whisper", 0);

        if (!exit_match_exists(player, loc, "pose", 1))
            if (!exit_match_exists(player, loc, "pos", 1))
                exit_match_exists(player, loc, "po", 1);

        exit_match_exists(player, loc, "say", 0);

        loc = getparent(loc);
    }

    notify(player, "**End of list**");
}
