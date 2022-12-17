/** @file p_db.c
 *
 * Implementation of DB related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "compile.h"
#include "db.h"
#include "dbsearch.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "edit.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "move.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"
#include "events.h"

/**
 * TODO: These globals really probably shouldn't be globals.  I can only guess
 *       this is either some kind of primitive code re-use because all the
 *       functions use them, or it's some kind of an optimization to avoid
 *       local variables.  But it kills the thread safety and (IMO) makes the
 *       code harder to read/follow.
 */

/**
 * @private
 * @var used to store the parameters passed into the primitives
 *      This seems to be used for conveinance, but makes this probably
 *      not threadsafe.
 */
static struct inst *oper1, *oper2, *oper3, *oper4;

/**
 * @private
 * @var to store the result which is not a local variable for some reason
 *      This makes things very not threadsafe.
 */
static int tmp, result;
static dbref ref;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

/**
 * Implementation of MUF ADDPENNIES
 *
 * Consumes a dbref and an integer, and adds that value to the player or
 * thing dbref.  Requires MUCKER level tp_addpennies_muf_mlev for players
 * and 4 for things.
 *
 * Needs MUCKER level 4 to bypass range checks for players.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_addpennies(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < tp_addpennies_muf_mlev)
        abort_interp("Permission Denied (mlev < tp_addpennies_muf_mlev)");

    if (!valid_object(oper2))
        abort_interp("Invalid object.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (2)");

    ref = oper2->data.objref;
    if (Typeof(ref) == TYPE_PLAYER) {
        result = GETVALUE(ref);

        /**
         * @TODO It makes sense that Wizard programs can bypass the limit for
         *       pennies.  However, the negative checks may need to apply to
         *       all programs.
         */
        if (mlev < 4) {
            /* @TODO Not the right place for this, but we need more of this
             *       "valid transaction" check happening for both giving
             *       pennies and paying for commands.
             */
            if (oper1->data.number > 0) {
                if (result > (result + oper1->data.number))
                    abort_interp("Would roll over player's score.");

                if ((result + oper1->data.number) > tp_max_pennies)
                    abort_interp("Would exceed MAX_PENNIES.");
            } else {
                if (result < (result + oper1->data.number))
                    abort_interp("Would roll over player's score.");

                if ((result + oper1->data.number) < 0)
                    abort_interp("Result would be negative.");
            }
        }

        /**
         * @TODO This line seems ineffective.
         */
        result += oper1->data.number;

        SETVALUE(ref, GETVALUE(ref) + oper1->data.number);

        /**
         * @TODO Move this DBDIRTY and the next outside the if.
         */
        DBDIRTY(ref);
    } else if (Typeof(ref) == TYPE_THING) {
        if (mlev < 4)
            abort_interp("Permission denied.");

        SETVALUE(ref, (GETVALUE(ref) + oper1->data.number));

        DBDIRTY(ref);
    } else {
        abort_interp("Invalid object type.");
    }

    /**
     * @TODO Move the previous two abort checks to the top with the others.
     */
    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF MOVETO
 *
 * Consumes two dbrefs, and moves the first object to the non-exit second.
 *
 * Needs MUCKER level 3 to bypass basic ownership and JUMP_OK checks, and
 * also to move an exit.
 *
 * This code should be further reviewed, and restructured to be easier to
 * understand and maintain - and match other movement logic when applicable.
 * It *seems* like we may find some unexpected consequences of using
 * fallthrough logic in the main switch.
 *
 * Here we go --
 *
 * Moving a player will fail if the destination is not a room or basic vehicle.
 *
 * Moving a player/thing will fail if it would create a parent loop.
 *
 * Moving a player/thing with less than MUCKER level 3 will fail if:
 * - source location or destination is not JUMP_OK
 * - source location or destination does not pass basic ownership checks
 * - destinaton is a basic vehicle and source is not in the same room
 * - source is a guest and the destination room does not accept guests
 *
 * Moving a player/thing/program will fail if the destination is not a player,
 * thing, or room.
 *
 * Moving a player/thing/program with less than MUCKER level three will fail if
 * the source location or destination is not JUMP_OK or does not pass basic
 * ownership checks.
 *
 * Moving an exit will fail if the destination is not a player, thing, or
 * room (as above!) - but also if the destination is HOME.
 *
 * Moving an exit with less that MUCKER level 3 will fail if neither source
 * nor destination pass basic ownership checks.
 *
 * Moving a room will fail if:
 * - the desintation is also not a room and tp_secure_thing_movement is off
 * - the desitnation is the GLOBAL_ENVIRONMENT
 *
 * Moving a room with less than MUCKER level 3 will fail if the destination
 * is HOME and the source and source location do not pass basic ownership
 * checks.  Otherwise, the source must pass basic ownership and teleport
 * checks and the movement must not create a parent loop.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_moveto(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (fr->level > tp_max_interp_recursion)
        abort_interp("Interp call loops not allowed.");

    /* Needs to be an object, and object needs to be valid */
    if (!(oper2->type == PROG_OBJECT && valid_object(oper2)))
        abort_interp("Non-object argument. (2)");

    /* Needs to be an object, and object needs to be valid or HOME */
    if (!(oper1->type == PROG_OBJECT && (valid_object(oper1)
            || is_home(oper1))))
        abort_interp("Non-object argument. (1)");

    {
        dbref victim, dest;

        victim = oper2->data.objref;
        dest = oper1->data.objref;

        if (Typeof(dest) == TYPE_EXIT)
            abort_interp("Destination argument is an exit.");

        if (Typeof(victim) == TYPE_EXIT && (mlev < 3))
            abort_interp("Permission denied.");

        if (!(FLAGS(victim) & JUMP_OK)
            && !permissions(ProgUID, victim) && (mlev < 3))
            abort_interp("Object can't be moved.");

        switch (Typeof(victim)) {
            case TYPE_PLAYER:
                if (Typeof(dest) != TYPE_ROOM
                        && !(Typeof(dest) == TYPE_THING
                        && (FLAGS(dest) & VEHICLE)))
                    abort_interp("Bad destination.");

            case TYPE_THING:
                if (parent_loop_check(victim, dest))
                    abort_interp("Things can't contain themselves.");

                if ((mlev < 3)) {
                    if (!(FLAGS(LOCATION(victim)) & JUMP_OK)
                            && !permissions(ProgUID, LOCATION(victim)))
                        abort_interp("Source not JUMP_OK.");

                    if (!is_home(oper1) && !(FLAGS(dest) & JUMP_OK)
                            && !permissions(ProgUID, dest))
                        abort_interp("Destination not JUMP_OK.");

                    if (Typeof(dest) == TYPE_THING
                            && LOCATION(victim) != LOCATION(dest))
                        abort_interp("Not in same location as vehicle.");

                    if (ISGUEST(victim) && (FLAGS(dest) & GUEST)
                            && Typeof(dest) == TYPE_ROOM)
                        abort_interp("Destination doesn't accept guests.");
                }

                if (Typeof(victim) == TYPE_PLAYER) {
                    enter_room(fr->descr, victim, dest, program);
                    break;
                }

                /**
                 * @TODO These two checks, should they only be for things?
                 */
                if (mlev < 3 && (FLAGS(victim) & VEHICLE) &&
                        (FLAGS(dest) & VEHICLE) && Typeof(dest) != TYPE_THING)
                    abort_interp("Destination doesn't accept vehicles.");

                if (mlev < 3 && (FLAGS(victim) & ZOMBIE) &&
                        (FLAGS(dest) & ZOMBIE) && Typeof(dest) != TYPE_THING)
                    abort_interp("Destination doesn't accept zombies.");

                ts_lastuseobject(victim);

            case TYPE_PROGRAM: {
                dbref matchroom = NOTHING;

                if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_PLAYER
                        && Typeof(dest) != TYPE_THING)
                    abort_interp("Bad destination.");

                if ((mlev < 3)) {
                    if (permissions(ProgUID, dest))
                        matchroom = dest;

                    if (permissions(ProgUID, LOCATION(victim)))
                        matchroom = LOCATION(victim);

                    if (matchroom != NOTHING && !(FLAGS(matchroom) & JUMP_OK)
                                && !permissions(ProgUID, victim))
                        abort_interp("Permission denied.");
                }

                if (Typeof(victim) == TYPE_THING
                        && (tp_secure_thing_movement
                        || (FLAGS(victim) & ZOMBIE))) {
                    enter_room(fr->descr, victim, dest, program);
                } else {
                    moveto(victim, dest);
                }
                break;
            }

            case TYPE_EXIT:
                if ((mlev < 3) && (!permissions(ProgUID, victim)
                        || !permissions(ProgUID, dest)))
                    abort_interp("Permission denied.");

                if ((Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_THING
                        && Typeof(dest) != TYPE_PLAYER) || dest == HOME)
                    abort_interp("Bad destination object.");

                unset_source(victim);
                set_source(victim, dest);
                SetMLevel(victim, 0);
                break;

            case TYPE_ROOM:
                if (!tp_secure_thing_movement && Typeof(dest) != TYPE_ROOM)
                    abort_interp("Bad destination.");

                if (victim == GLOBAL_ENVIRONMENT)
                    abort_interp("Permission denied.");

                if (dest == HOME) {
                    /* Allow the owner of the room or the owner of
                       the room's location to reparent the room to
                       #0 */
                    if ((mlev < 3) && (!permissions(ProgUID, victim)
                            && !permissions(ProgUID, LOCATION(victim))))
                        abort_interp("Permission denied.");

                    dest = GLOBAL_ENVIRONMENT;
                } else {
                    if ((mlev < 3) && (!permissions(ProgUID, victim)
                            || !can_teleport_to(ProgUID, dest)))
                        abort_interp("Permission denied.");

                    if (parent_loop_check(victim, dest)) {
                        abort_interp("Parent room would create a loop.");
                    }
                }

                ts_lastuseobject(victim);
                moveto(victim, dest);
                break;

            default:
                abort_interp("Invalid object type (1)");
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF PENNIES
 *
 * Consumes a player or thing dbref and returns its value.  Requires
 * MUCKER level tp_pennies_muf_mlev and remote-read permissions when
 * applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_pennies(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid argument.");

    if (mlev < tp_pennies_muf_mlev)
        abort_interp("Permission Denied (mlev < tp_pennies_muf_mlev)");

    CHECKREMOTE(oper1->data.objref);

    /**
     * @TODO Move the type check to the top, and flatten this switch.
     */
    switch (Typeof(oper1->data.objref)) {
        case TYPE_PLAYER:
            result = GETVALUE(oper1->data.objref);
            break;

        case TYPE_THING:
            result = GETVALUE(oper1->data.objref);
            break;

        default:
            abort_interp("Invalid argument.");
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF DBREF
 *
 * Consumes an integer, and returns it as a dbref - which is not guaranteed
 * to be valid.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_dbref(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument.");

    ref = (dbref) oper1->data.number;

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF CONTENTS
 *
 * Consumes a dbref, and returns the first dbref in its contents chain -
 * which for MUCKER level 1 only returns controlled non-DARK objects.
 * Requires remote-read permissions when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_contents(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid argument type.");

    CHECKREMOTE(oper1->data.objref);

    ref = CONTENTS(oper1->data.objref);

    while (mlev < 2 && ref != NOTHING && (FLAGS(ref) & DARK)
            && !controls(ProgUID, ref))
        ref = NEXTOBJ(ref);

    /**
     * @TODO Define all the cases when the lastuse timestamp and count
     *       should be updated.
     */
    /*
    if (Typeof(oper1->data.objref) != TYPE_PLAYER
            && Typeof(oper1->data.objref) != TYPE_PROGRAM)
        ts_lastuseobject(oper1->data.objref);
    */

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF EXITS
 *
 * Consumes a dbref, and returns the first dbref in its exits chain.
 * Requires remote-read privileges when applicable.
 *
 * Needs MUCKER level 3 to bypass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_exits(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    ref = oper1->data.objref;
    CHECKREMOTE(ref);

    if ((mlev < 3) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    /**
     * @TODO Move type check to the top, and flatten this switch.
     */
    switch (Typeof(ref)) {
        case TYPE_ROOM:
        case TYPE_THING:
            /* ts_lastuseobject(ref); */

        case TYPE_PLAYER:
            ref = EXITS(ref);
            break;

        default:
            abort_interp("Invalid object.");
    }

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF NEXT
 *
 * Consumes a dbref, and returns the next dbref in its contents/exits
 * chain - which for MUCKER level 1 only traverses controlled non-DARK exits.
 * Requires remote-read permissions when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_next(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);

    ref = NEXTOBJ(oper1->data.objref);

    while (mlev < 2 && ref != NOTHING && Typeof(ref) != TYPE_EXIT
            && ((FLAGS(ref) & DARK) || Typeof(ref) == TYPE_ROOM)
            && !controls(ProgUID, ref))
        ref = NEXTOBJ(ref);

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF NEXTOWNED
 *
 * Consumes a dbref, and returns the next dbref in the database that is
 * owned by the same player.  If a player dbref is consumed, the entire
 * database is searched.  Requires MUCKER level 2.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_nextowned(PRIM_PROTOTYPE)
{
    dbref ownr;

    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");
    if (mlev < 2)
        abort_interp("Permission denied.");

    ref = oper1->data.objref;

    /**
     * @TODO This should never abort, since mlev >= 2.
     */
    CHECKREMOTE(ref);

    ownr = OWNER(ref);

    if (Typeof(ref) == TYPE_PLAYER) {
        ref = 0;
    } else {
        ref++;
    }

    while (ref < db_top && (OWNER(ref) != ownr || ref == ownr))
        ref++;

    if (ref >= db_top) {
        ref = NOTHING;
    }

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF NAME
 *
 * Consumes a dbref, and returns the associated name.  Requires remote-read
 * privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_name(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Invalid argument type.");

    ref = oper1->data.objref;
    if (!ObjExists(ref))
        abort_interp("Invalid object.");

    if (Typeof(ref) == TYPE_GARBAGE) {
        strcpyn(buf, sizeof(buf), "<garbage>");
    } else {
        CHECKREMOTE(ref);

        /*
        if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
           ts_lastuseobject(ref);
        */

        if (NAME(ref)) {
            strcpyn(buf, sizeof(buf), NAME(ref));
        } else {
            buf[0] = '\0';
        }
    }

    CLEAR(oper1);
    PushString(buf);
}

/**
 * Implementation of MUF SETNAME
 *
 * Consumes a dbref and a string, and sets the name associated with the
 * object.  Setting a player name requires MUCKER level 4 and the player's
 * password inside the given string (delimited by a space).  Also required
 * to change the name of objects that don't pass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_setname(PRIM_PROTOTYPE)
{
    char *password;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!valid_object(oper2))
        abort_interp("Invalid argument type (1)");

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    ref = oper2->data.objref;
    if ((mlev < 4) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    {
        const char *b = DoNullInd(oper1->data.string);

        if (Typeof(ref) == TYPE_PLAYER) {
            strcpyn(buf, sizeof(buf), b);
            b = buf;

            if (mlev < 4) {
                abort_interp("Permission denied.");
            }

            /* split off password */
            for (password = buf; *password && !isspace(*password); password++) ;

            if (*password) {
                *password++ = '\0';     /* terminate name */
                skip_whitespace_var(&password);
            }

            /* check for null password */
            if (!*password) {
                abort_interp("Player namechange requires password.");
            } else if (!check_password(ref, password)) {
                abort_interp("Incorrect password.");
            } else if (strcasecmp(b, NAME(ref)) && !ok_object_name(b, TYPE_PLAYER)) {
                abort_interp("You can't give a player that name.");
            }

            /* everything ok, notify */
            log_status("NAME CHANGE (MUF): %s(#%d) to %s", NAME(ref), ref, b);

            delete_player(ref);
            change_player_name(ref, b);
            add_player(ref);
        } else {
            if (!ok_object_name(b, Typeof(ref))) {
                abort_interp("Invalid name.");
            }

            free((void *) NAME(ref));
            NAME(ref) = alloc_string(b);
            ts_modifyobject(ref);
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF PMATCH
 *
 * Consumes a string, and returns the player dbref with a name that matches
 * the ANSI-stripped string, or NOTHING if no match.  Recognizes 'me'.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_pmatch(PRIM_PROTOTYPE)
{
    dbref ref;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument.");

    strip_ansi(buf, DoNullInd(oper1->data.string));

    if (!strcasecmp(buf, "me")) {
        ref = player;
    } else {
        ref = lookup_player(buf);
    }
    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF MATCH
 *
 * Consumes a string, and returns the dbref that matches the ANSI-stripped
 * string.  Recognizes registered objects, local matches, 'me', 'here',
 * 'home', and 'nil'.  Wizard programs and users can also match dbref
 * strings and player lookups.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_match(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL;
    dbref ref;
    char buf2[BUFFER_LEN];

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument.");
    {
        char tmppp[BUFFER_LEN];
        struct match_data md;

        (void) strcpyn(buf, sizeof(buf), match_args);
        (void) strcpyn(tmppp, sizeof(tmppp), match_cmdname);

        strip_ansi(buf2, DoNullInd(oper1->data.string));

        init_match(fr->descr, player, buf2, NOTYPE, &md);

        if (buf2[0] == REGISTERED_TOKEN) {
            match_registered(&md);
        } else {
            match_all_exits(&md);
            match_neighbor(&md);
            match_possession(&md);
            match_me(&md);
            match_here(&md);
            match_home(&md);
            match_nil(&md);
        }

        if (Wizard(ProgUID) || (mlev >= 4)) {
            match_absolute(&md);
            match_player(&md);
        }

        ref = match_result(&md);

        (void) strcpyn(match_args, sizeof(match_args), buf);
        (void) strcpyn(match_cmdname, sizeof(match_cmdname), tmppp);
    }

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF RMATCH
 *
 * Consumes a dbref and a string, and returns the dbref that matches one of
 * the object's contents or exits.  Requires remote-read privileges when
 * applicable.  Aborts when given an exit or program dbref.
 *
 * Does not ANSI-strip the string before matching.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_rmatch(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL;
    dbref ref;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument (2)");

    if (oper2->type != PROG_OBJECT
        || !ObjExists(oper2->data.objref)
        || Typeof(oper2->data.objref) == TYPE_PROGRAM
        || Typeof(oper2->data.objref) == TYPE_EXIT)
        abort_interp("Invalid argument (1)");

    CHECKREMOTE(oper2->data.objref);

    {
        char tmppp[BUFFER_LEN];
        struct match_data md;

        (void) strcpyn(buf, sizeof(buf), match_args);
        (void) strcpyn(tmppp, sizeof(tmppp), match_cmdname);

        init_match(fr->descr, player, DoNullInd(oper1->data.string),
                TYPE_THING, &md);

        match_rmatch(oper2->data.objref, &md);

        ref = match_result(&md);

        (void) strcpyn(match_args, sizeof(match_args), buf);
        (void) strcpyn(match_cmdname, sizeof(match_cmdname), tmppp);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(ref);
}

/**
 * Implementation of MUF COPYOBJ
 *
 * Consumes a thing dbref, copies the object's name and properties, and
 * returns the new dbref.  Requires remote-read privilges when applicable.
 *
 * Programs below MUCKER level 3 are restricted to creating one object per
 * frame.  That object must pass basic ownership checks and its copy has
 * its value reset to 0.
 *
 * Currently calls copyobj.  @see copyobj
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_copyobj(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);

    if ((mlev < 3) && (fr->already_created))
        abort_interp("An object was already created this program run.");

    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_THING)
        abort_interp("Invalid object type.");

    if ((mlev < 3) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    if (!ok_object_name(NAME(oper1->data.objref), TYPE_THING))
        abort_interp("Invalid name.");

    fr->already_created++;

    dbref newobj = clone_thing(ref, player, mlev == 4);

    CLEAR(oper1);
    PushObject(newobj);
}

/**
 * Implementation of MUF SET
 *
 * Consumes a dbref and a string, and sets or resets the flag on the object
 * accoringly.  Reognizes flag aliases and Requires remote-read privileges
 * when applicable.
 *
 * Programs below MUCKER level 4 cannot act on:
 *    - objects that don't pass basic ownership checks
 *    - ABODE flags on programs
 *    - BUIDLER flags
 *    - DARK flags on exits with exit_darking sysparm = no
 *    - DARK flags on players
 *    - DARK flags on things with thing_darking sysparm = no
 *    - GUEST flags
 *    - INTERACTIVE, MUCKER, or SMUCKER ("nucker") internal flags
 *    - OVERT flags
 *    - QUELL flags
 *    - WIZARD flags
 *    - XFORCIBLE flags
 *    - YIELD flags
 *    - ZOMBIE flags on players
 *    - ZOMBIE flags on things if effective user is maked ZOMBIE.
 *
 * No program is able to:
 *    - reset the VEHICLE flag of a basic vehicle with polayers in it
 *    - interact with the OVERT flag on objects that aren't a thing or room
 *    - interact with the YIELD flag on objects that aren't a thing or room
 *
 *
 * "truewizard" is merely an alias for the WIZARD flag here.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_set(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument type (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid object.");

    ref = oper2->data.objref;
    CHECKREMOTE(ref);

    /**
     * @TODO So, it would be nice if flags, their type aliases, basic
     *       restrictions, and perhaps error messages could be combined
     *       into a data structure. It could have MANY uses from do_examine
     *       to MUF.
     */
    tmp = 0;
    {
        char *flag = DoNullInd(oper1->data.string);
        result = (*flag == '!');

        if (result)
            flag++;

        if (!*flag)
            tmp = 0;
        if (string_prefix("abode", flag)
                || string_prefix("autostart", flag)
                || string_prefix("abate", flag))
            tmp = ABODE;
        else if (string_prefix("builder", flag))
            tmp = BUILDER;
        else if (string_prefix("chown_ok", flag)
                || string_prefix("color", flag))
            tmp = CHOWN_OK;
        else if (string_prefix("dark", flag)
                || string_prefix("debug", flag))
            tmp = DARK;
        else if (string_prefix("guest", flag))
            tmp = GUEST;
        else if (string_prefix("haven", flag)
                || string_prefix("harduid", flag))
            tmp = HAVEN;
        else if (string_prefix("interactive", flag))
            tmp = INTERACTIVE;
        else if (string_prefix("jump_ok", flag))
            tmp = JUMP_OK;
        else if (string_prefix("kill_ok", flag))
            tmp = KILL_OK;
        else if (string_prefix("link_ok", flag))
            tmp = LINK_OK;
        else if (string_prefix("mucker", flag))
            tmp = MUCKER;
        else if (string_prefix("nucker", flag))
            tmp = SMUCKER;
        else if (string_prefix("overt", flag))
            tmp = (int)OVERT;
        else if (string_prefix("quell", flag))
            tmp = QUELL;
        else if (string_prefix("sticky", flag)
                || string_prefix("silent", flag))
            tmp = STICKY;
        else if (string_prefix("vehicle", flag)
                || string_prefix("viewable", flag))
            tmp = VEHICLE;
        else if (string_prefix("wizard", flag))
            tmp = WIZARD;
        else if (string_prefix("truewizard", flag))
            tmp = WIZARD;
        else if (string_prefix("xforcible", flag))
            tmp = XFORCIBLE;
        else if (string_prefix("yield", flag))
            tmp = YIELD;
        else if (string_prefix("zombie", flag))
            tmp = ZOMBIE;
    }

    if (!tmp)
        abort_interp("Unrecognized flag.");

    if ((mlev < 4) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    /**
     * @TODO Shall we harmonize these checks with the restricted function?
     *       @see restricted
     */
    if (((mlev < 4) && ((tmp == DARK && ((Typeof(ref) == TYPE_PLAYER)
            || (!tp_exit_darking && Typeof(ref) == TYPE_EXIT)
            || (!tp_thing_darking && Typeof(ref) == TYPE_THING)))
            || ((tmp == ZOMBIE) && (Typeof(ref) == TYPE_THING)
                && (FLAGS(ProgUID) & ZOMBIE))
            || ((tmp == ZOMBIE) && (Typeof(ref) == TYPE_PLAYER))
            || (tmp == BUILDER) || (tmp == YIELD) || ((unsigned) tmp == OVERT)
            || (tmp == GUEST)))
            || (tmp == WIZARD) || (tmp == QUELL) || (tmp == INTERACTIVE)
            || ((tmp == ABODE) && (Typeof(ref) == TYPE_PROGRAM))
            || (tmp == MUCKER) || (tmp == SMUCKER) || (tmp == XFORCIBLE))
        abort_interp("Permission denied.");

    if (((tmp == YIELD) || ((unsigned) tmp == OVERT)) &&
        (Typeof(ref) != TYPE_THING && Typeof(ref) != TYPE_ROOM)) {
        abort_interp("Permission denied.");
    }

    if (result && Typeof(ref) == TYPE_THING && tmp == VEHICLE) {
        dbref obj = CONTENTS(ref);

        for (; obj != NOTHING; obj = NEXTOBJ(obj)) {
            if (Typeof(obj) == TYPE_PLAYER) {
                abort_interp("That vehicle still has players in it!");
            }
        }
    }

    if (!result) {
        FLAGS(ref) |= tmp;

        /**
         * @TODO Move this DBDIRTY and the next outside the if.
         */
        DBDIRTY(ref);
    } else {
        FLAGS(ref) &= ~tmp;

        DBDIRTY(ref);
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF MLEVEL
 *
 * Consumes a dbref, and returns its raw MUCKER level.  Objects with the
 * WIZARD flag and a MUCKER level are returned as 4.  Requires remote-read
 * privileges when applicable.
 *
 * If the given dbref is NOTHING, returns the effective MUCKER level of
 * the running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_mlevel(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->data.objref == NOTHING) {
        result = mlev;
    } else {
        if (!valid_object(oper1))
            abort_interp("Invalid object.");

        ref = oper1->data.objref;

        CHECKREMOTE(ref);

        result = MLevRaw(ref);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF FLAG?
 *
 * Consumes a dbref and a string, and returns a boolean that represents if
 * the object's flag is set accordingly.  Requires remote-read priviliges
 * when applicable.
 *
 * Checking "truewizard" is the same as checking "wizard" and "!quell".
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_flagp(PRIM_PROTOTYPE)
{
    int truwiz = 0;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument type (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid object.");

    ref = oper2->data.objref;
    CHECKREMOTE(ref);

    tmp = 0;
    result = 0;
    {
        char *flag = DoNullInd(oper1->data.string);

        while (*flag == '!') {
            flag++;
            result = (!result);
        }

        if (!*flag)
            abort_interp("Unknown flag.");

        if (string_prefix("abode", flag)
                || string_prefix("autostart", flag)
                || string_prefix("abate", flag))
            tmp = ABODE;
        else if (string_prefix("builder", flag))
            tmp = BUILDER;
        else if (string_prefix("chown_ok", flag)
                || string_prefix("color", flag))
            tmp = CHOWN_OK;
        else if (string_prefix("dark", flag)
                || string_prefix("debug", flag))
            tmp = DARK;
        else if (string_prefix("guest", flag))
            tmp = GUEST;
        else if (string_prefix("haven", flag)
                || string_prefix("harduid", flag))
            tmp = HAVEN;
        else if (string_prefix("interactive", flag))
            tmp = INTERACTIVE;
        else if (string_prefix("jump_ok", flag))
            tmp = JUMP_OK;
        else if (string_prefix("kill_ok", flag))
            tmp = KILL_OK;
        else if (string_prefix("link_ok", flag))
            tmp = LINK_OK;
        else if (string_prefix("mucker", flag))
            tmp = MUCKER;
        else if (string_prefix("nucker", flag))
            tmp = SMUCKER;
        else if (string_prefix("overt", flag))
            tmp = (int)OVERT;
        else if (string_prefix("quell", flag))
            tmp = QUELL;
        else if (string_prefix("sticky", flag)
                || string_prefix("silent", flag))
            tmp = STICKY;
        else if (string_prefix("vehicle", flag)
                || string_prefix("viewable", flag))
            tmp = VEHICLE;
        else if (string_prefix("wizard", flag))
            tmp = WIZARD;
        else if (string_prefix("truewizard", flag)) {
            tmp = WIZARD;
            truwiz = 1;
        } else if (string_prefix("xforcible", flag))
            tmp = XFORCIBLE;
        else if (string_prefix("yield", flag))
            tmp = YIELD;
        else if (string_prefix("zombie", flag))
            tmp = ZOMBIE;
    }

    if (result) {
        if ((!truwiz) && (tmp == WIZARD)) {
            result = (!Wizard(ref));
        } else {
            result = (tmp && ((FLAGS(ref) & tmp) == 0));
        }
    } else {
        if ((!truwiz) && (tmp == WIZARD)) {
            result = Wizard(ref);
        } else {
            result = (tmp && ((FLAGS(ref) & tmp) != 0));
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF PLAYER?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a player.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_playerp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Invalid argument type.");

    /**
     * @TODO HOME is defined as a room. It eventually returns 0.
     */
    if (!valid_object(oper1) && !is_home(oper1)) {
        result = 0;
    } else {
        ref = oper1->data.objref;
        CHECKREMOTE(ref);

        result = (Typeof(ref) == TYPE_PLAYER);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF THING?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a thing.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_thingp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Invalid argument type.");

    /**
     * @TODO HOME is defined as a room. It eventually returns 0.
     */
    if (!valid_object(oper1) && !is_home(oper1)) {
        result = 0;
    } else {
        ref = oper1->data.objref;
        CHECKREMOTE(ref);

        result = (Typeof(ref) == TYPE_THING);
    }
    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF ROOM?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a room.  Requires remote-read privilges when applicable.
 *
 * Any recycled object, NOTHING, AMBIGUOUS, or NIL returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_roomp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Invalid argument type.");

    if (!valid_object(oper1) && !is_home(oper1)) {
        result = 0;
    } else {
        ref = oper1->data.objref;
        CHECKREMOTE(ref);

        result = (Typeof(ref) == TYPE_ROOM);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF PROGRAM?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is a program.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_programp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Invalid argument type.");

    /**
     * @TODO HOME is defined as a room. It eventually returns 0.
     */
    if (!valid_object(oper1) && !is_home(oper1)) {
        result = 0;
    } else {
        ref = oper1->data.objref;
        CHECKREMOTE(ref);

        result = (Typeof(ref) == TYPE_PROGRAM);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF EXIT?
 *
 * Consumes a dbref, and returns a boolean that represents if the object
 * is an exit/action.  Requires remote-read privilges when applicable.
 *
 * Any recycled object or negative dbref returns 0.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_exitp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Invalid argument type.");

    /**
     * @TODO HOME is defined as a room. It eventually returns 0.
     */
    if (!valid_object(oper1) && !is_home(oper1)) {
        result = 0;
    } else {
        ref = oper1->data.objref;
        CHECKREMOTE(ref);

        result = (Typeof(ref) == TYPE_EXIT);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF OK?
 *
 * Consumes a stack item, and returns a boolean that represents if it is an
 * object that is not currently recycled.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_okp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    result = (valid_object(oper1));

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF LOCATION
 *
 * Consumes a dbref, and returns the dbref of its location.  Requires
 * remote-read privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_location(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);

    ref = LOCATION(oper1->data.objref);

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF OWNER
 *
 * Consumes a dbref, and returns the dbref of its owner.  Requires
 * remote-read privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_owner(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);

    ref = OWNER(oper1->data.objref);

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF CONTROLS
 *
 * Consumes two dbrefs, and returns a boolean representing if the first
 * controls the second.  Requires remote-read privileges for the second
 * object when applicable.
 *
 * @see controls
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_controls(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object. (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid object. (1)");

    CHECKREMOTE(oper1->data.objref);

    result = controls(oper2->data.objref, oper1->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF GETLINK
 *
 * Consumes a dbref, and returns the dbref of its (first) link.  Requires
 * remote-read privileges when applicable.
 *
 * Currently does not work on programs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_getlink(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);

    /**
     * @TODO A years-old suggestion would have this returning the
     *       link of a program as the home of its owner.  Does this
     *       make sense?
     */
    if (Typeof(oper1->data.objref) == TYPE_PROGRAM)
        abort_interp("Illegal object referenced.");

    /**
     * @TODO Seems like this "link resolution" logic would be useful
     *       as a standalone function?
     */
    switch (Typeof(oper1->data.objref)) {
        case TYPE_EXIT:
            ref = (DBFETCH(oper1->data.objref)->sp.exit.ndest) ?
                    (DBFETCH(oper1->data.objref)->sp.exit.dest)[0] : NOTHING;
            break;

        case TYPE_PLAYER:
            ref = PLAYER_HOME(oper1->data.objref);
            break;

        case TYPE_THING:
            ref = THING_HOME(oper1->data.objref);
            break;

        case TYPE_ROOM:
            ref = DBFETCH(oper1->data.objref)->sp.room.dropto;
            break;

        default:
            ref = NOTHING;
            break;
    }

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF GETLINKS
 *
 * Consumes a dbref, and returns a stackrange of dbrefs representing the
 * object's links.  Requires remote-read privileges when applicable.
 *
 * Currently does not work on programs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_getlinks(PRIM_PROTOTYPE)
{
    int count;
    dbref my_obj;

    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);

    if (Typeof(oper1->data.objref) == TYPE_PROGRAM)
        abort_interp("Illegal object referenced.");

    my_obj = oper1->data.objref;

    /**
     * @TODO Move this to the bottom for consistency.
     */
    CLEAR(oper1);

    /**
     * @TODO I don't understand the need for this.
     */
    nargs = 0;

    switch (Typeof(my_obj)) {
        case TYPE_EXIT:
            count = DBFETCH(my_obj)->sp.exit.ndest;

            CHECKOFLOW(count + 1);

            for (int i = 0; i < count; i++) {
                PushObject((DBFETCH(my_obj)->sp.exit.dest)[i]);
            }

            /**
             * @TODO We only need one instance of this, outside the switch.
             */
            PushInt(count);
            break;

        case TYPE_PLAYER:
            CHECKOFLOW(2);

            ref = PLAYER_HOME(my_obj);
            count = 1;

            PushObject(ref);
            PushInt(count);
            break;

        case TYPE_THING:
            CHECKOFLOW(2);

            ref = THING_HOME(my_obj);

            count = 1;

            PushObject(ref);
            PushInt(count);
            break;

        case TYPE_ROOM:
            ref = DBFETCH(my_obj)->sp.room.dropto;

            if (ref != NOTHING) {
                count = 0;
                PushInt(count);
            } else {
                CHECKOFLOW(2);
                count = 1;
                PushObject(ref);
                PushInt(count);
            }
            break;

        default:
            count = 0;
            PushInt(count);
            break;
    }
}

/**
 * Determines whether if a link can be set, with MUCKER level support.
 *
 * Linking anything to HOME, or an EXIT to NIL, is always allowed.
 *
 * The following links are not allowed:
 * - to recycled or invalid objects (except in the cases of HOME or NIL above)
 * - to non-room objects from a player
 * - to non-room and non-thing objects from a room
 * - to exits and programs from a thing
 * - to anything from a program
 *
 * If it is still undetermined, the following conditions allow the link:
 * - Program has an effective MUCKER level of 4
 * - the source has basic ownership of the destination
 * - the target is publicly linkable with and the source passe its linklock
 *
 * @private
 * @param mlev the effective MUCKER level
 * @param who the dbref to test a linklock against
 * @param what_type the object type of the source
 * @param where the dbref of the potential link
 */
static int
prog_can_link_to(int mlev, dbref who, object_flag_type what_type, dbref where)
{
    /* Can always link to HOME */
    if (where == HOME)
        return 1;

    /* Exits can be linked to NIL */
    if (what_type == TYPE_EXIT && where == NIL)
        return 1;

    /* Can't link to an invalid or recycled dbref */
    if (!OkObj(where))
        return 0;

    /* Players can only be linked to rooms */
    if (what_type == TYPE_PLAYER && Typeof(where) != TYPE_ROOM)
        return 0;

    /* Rooms can only be linked to things or other rooms */
    if (what_type == TYPE_ROOM
        && Typeof(where) != TYPE_THING && Typeof(where) != TYPE_ROOM)
        return 0;

    /* Things cannot be linked to exits or programs */
    if (what_type == TYPE_THING
        && (Typeof(where) == TYPE_EXIT || Typeof(where) == TYPE_PROGRAM))
        return 0;

    /* Programs cannot be linked */
    if (what_type == TYPE_PROGRAM)
        return 0;

    /* Effective mucker level must be 3 or more, source must have permission,
       or target must be publicly linkable with its linklock passed */

    /**
     * @TODO Show we add a function argument for fr->descr so test_lock
     *       dosesn't need to use NOTHING?  This would go for can_link_to
     *       in predicates.c as well.
     */
    return mlev > 3 || permissions(who, where) || (Linkable(where) &&
            test_lock(NOTHING, who, where, MESGPROP_LINKLOCK));
}

/**
 * Implementation of MUF SETLINK
 *
 * Consumes two dbrefs, and sets the link of the first non-program object to
 * the second object.  Cannot link if it would create an exit/parent loop, or
 * the source is unable to be linked to the target.  @see prog_can_link_to
 *
 * Needs MUCKER level 4 to bypass basic ownership checks on the source.
 *
 * A target of NOTHING unlinks the given exit or room source.  AMBIGUOUS
 * is not a valid target.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_setlink(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();              /* dbref: destination */
    oper2 = POP();              /* dbref: source */

    if ((oper1->type != PROG_OBJECT) || (oper2->type != PROG_OBJECT))
        abort_interp("setlink requires two dbrefs.");

    if (!valid_object(oper2))
        abort_interp("Invalid object. (1)");

    ref = oper2->data.objref;

    /**
     * @TODO Centralize the setlink logic so it can be used elewhere.
     */
    if (oper1->data.objref == NOTHING) {
        /**
         * @TODO Move this check above the if, removing others.
         */
        if ((mlev < 4) && !permissions(ProgUID, ref))
            abort_interp("Permission denied.");

        switch (Typeof(ref)) {
            case TYPE_EXIT:
                DBSTORE(ref, sp.exit.ndest, 0);
                free(DBFETCH(ref)->sp.exit.dest);
                DBSTORE(ref, sp.exit.dest, NULL);

                if (MLevRaw(ref))
                    SetMLevel(ref, 0);

                break;

            case TYPE_ROOM:
                DBSTORE(ref, sp.room.dropto, NOTHING);
                break;

            default:
                abort_interp("Invalid object. (1)");
        }
    } else {
        if (oper1->data.objref != HOME && oper1->data.objref != NIL
                && !valid_object(oper1))
            abort_interp("Invalid object. (2)");

        if (Typeof(ref) == TYPE_PROGRAM)
            abort_interp("Program objects are not linkable. (1)");

        if (!prog_can_link_to(mlev, ProgUID, Typeof(ref), oper1->data.objref))
            abort_interp("Can't link source to destination.");

        switch (Typeof(ref)) {
            case TYPE_EXIT:
                if ((mlev < 4) && !permissions(ProgUID, ref))
                    abort_interp("Permission denied.");

                if (DBFETCH(ref)->sp.exit.ndest != 0
                        && (DBFETCH(ref)->sp.exit.dest)[0] != NIL)
                    abort_interp("Exit is already linked.");

                if (exit_loop_check(ref, oper1->data.objref))
                    abort_interp("Link would cause a loop.");

                DBFETCH(ref)->sp.exit.ndest = 1;
                DBFETCH(ref)->sp.exit.dest = malloc(sizeof(dbref));
                (DBFETCH(ref)->sp.exit.dest)[0] = oper1->data.objref;
                DBDIRTY(ref);
                break;

            case TYPE_PLAYER:
                if ((mlev < 4) && !permissions(ProgUID, ref))
                    abort_interp("Permission denied.");

                if (oper1->data.objref == HOME)
                    abort_interp("Cannot link player to HOME.");

                PLAYER_SET_HOME(ref, oper1->data.objref);
                DBDIRTY(ref);
                break;

            case TYPE_THING:
                if ((mlev < 4) && !permissions(ProgUID, ref))
                    abort_interp("Permission denied.");

                if (oper1->data.objref == HOME)
                    abort_interp("Cannot link thing to HOME.");

                if (parent_loop_check(ref, oper1->data.objref))
                    abort_interp("That would cause a parent paradox.");

                THING_SET_HOME(ref, oper1->data.objref);
                DBDIRTY(ref);
                break;

            case TYPE_ROOM:
                if ((mlev < 4) && !permissions(ProgUID, ref))
                    abort_interp("Permission denied.");

                DBFETCH(ref)->sp.room.dropto = oper1->data.objref;
                DBDIRTY(ref);
                break;
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF SETOWN
 *
 * Consumes two dbrefs, and sets the owner of the first non-player object
 * to the second player.
 *
 * Programs with a MUCKER level less than 4 cannot change the owner:
 * - to other players
 * - of non-chownable objects to players don't pass the chown-lock
 * - of rooms that the player is not located in
 * - of things that the player is not carrying
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_setown(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();              /* dbref: new owner */
    oper2 = POP();              /* dbref: what */

    if (!valid_object(oper2))
        abort_interp("Invalid argument (1)");

    if (!valid_player(oper1))
        abort_interp("Invalid argument (2)");

    ref = oper2->data.objref;
    if ((mlev < 4) && oper1->data.objref != player)
        abort_interp("Permission denied. (2)");

    if ((mlev < 4) && (!(FLAGS(ref) & CHOWN_OK) ||
            !test_lock(fr->descr, player, ref, MESGPROP_CHLOCK)))
        abort_interp("Permission denied. (1)");

    switch (Typeof(ref)) {
        case TYPE_ROOM:
            if ((mlev < 4) && LOCATION(player) != ref)
                abort_interp("Permission denied: not in room. (1)");

            break;

        case TYPE_THING:
            if ((mlev < 4) && LOCATION(ref) != player)
                abort_interp("Permission denied: object not carried. (1)");
            break;

        case TYPE_PLAYER:
            abort_interp("Permission denied: cannot set owner of player. (1)");

        case TYPE_EXIT:
        case TYPE_PROGRAM:
            break;

        /**
         * @TODO This is caught in the valid_object check at the top.
         */
        case TYPE_GARBAGE:
            abort_interp("Permission denied: who would want to own garbage? (1)");
    }

    OWNER(ref) = OWNER(oper1->data.objref);
    DBDIRTY(ref);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF NEWOBJECT
 *
 * Consumes a string and a dbref, creates a thing with the given name and
 * player/room object as its home, and returns the new dbref.  Requires
 * remote-read privilges when applicable.
 *
 * Programs below MUCKER level 3 are restricted to creating one object per
 * frame.  The home object must pass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_newobject(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();              /* string: name */
    oper2 = POP();              /* dbref: location */

    if ((mlev < 3) && (fr->already_created))
        abort_interp("An object was already created this program run.");

    CHECKOFLOW(1);

    ref = oper2->data.objref;
    if (!valid_object(oper2) || !(valid_player(oper2)
            || (Typeof(ref) == TYPE_ROOM)))
        abort_interp("Invalid argument (1)");

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument (2)");

    CHECKREMOTE(ref);

    if ((mlev < 3) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    fr->already_created++;

    {
        const char *b = DoNullInd(oper1->data.string);

       /**
        * @TODO Consider folding the name check into all the create_object
        *       function.  Same comment for the other NEW* primitives.
        */
        if (!ok_object_name(b, TYPE_THING))
            abort_interp("Invalid name. (2)");

        ref = create_thing(ProgUID, b, oper2->data.objref);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(ref);
}

/**
 * Implementation of MUF NEWROOM
 *
 * Consumes a string and a dbref, creates a room with the given name and
 * room object as its dropto, and returns the new dbref.  Requires
 * remote-read privilges when applicable.
 *
 * Programs below MUCKER level 3 are restricted to creating one object per
 * frame.  The dropto object must pass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_newroom(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();              /* string: name */
    oper2 = POP();              /* dbref: location */

    if ((mlev < 3) && (fr->already_created))
        abort_interp("An object was already created this program run.");

    CHECKOFLOW(1);

    ref = oper2->data.objref;
    if (!valid_object(oper2) || (Typeof(ref) != TYPE_ROOM))
        abort_interp("Invalid argument (1)");

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument (2)");

    if ((mlev < 3) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    fr->already_created++;

    {
        const char *b = DoNullInd(oper1->data.string);

        if (!ok_object_name(b, TYPE_ROOM))
            abort_interp("Invalid name. (2)");

        ref = create_room(ProgUID, b, ref);

        CLEAR(oper1);
        CLEAR(oper2);
        PushObject(ref);
    }
}

/**
 * Implementation of MUF NEWEXIT
 *
 * Consumes a string and a dbref, creates an exit with the given name and
 * object as its location, and returns the new dbref.
 *
 * Requires MUCKER level 3.  Level 4 is needed to bypass basic ownership
 * checks on the location object.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_newexit(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();              /* string: name */
    oper2 = POP();              /* dbref: location */

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3.");

    CHECKOFLOW(1);

    ref = oper2->data.objref;
    if (!valid_object(oper2) ||
        ((!valid_player(oper2)) && (Typeof(ref) != TYPE_ROOM)
            && (Typeof(ref) != TYPE_THING)))
        abort_interp("Invalid argument (1)");

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument (2)");

    /**
     * @TODO This should never abort, since mlev >= 2.
     */
    CHECKREMOTE(ref);

    if ((mlev < 4) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    {
        const char *b = DoNullInd(oper1->data.string);

        if (!ok_object_name(b, TYPE_EXIT))
            abort_interp("Invalid name. (2)");

        ref = create_action(ProgUID, oper1->data.string->data,
                oper2->data.objref);

        CLEAR(oper1);
        CLEAR(oper2);
        PushObject(ref);
    }
}

/**
 * Implementation of MUF LOCKED?
 *
 * Consumes two dbrefs and returns a boolean representing if the first
 * player/thing object is locked against the second object.  Requires
 * remote-read privileges for both objects when applicable.
 *
 * @see could_do_it
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_lockedp(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */

    CHECKOP(2);
    oper1 = POP();              /* objdbref */
    oper2 = POP();              /* player dbref */

    if (fr->level > tp_max_interp_recursion)
        abort_interp("Interp call loops not allowed.");

    /**
     * @TODO This appears to be a redundant check.
     */
    if (!valid_object(oper2))
        abort_interp("invalid object (1).");

    /**
     * @TODO Argument can be a thing, too.  Change the message.
     */
    if (!valid_player(oper2) && Typeof(oper2->data.objref) != TYPE_THING)
        abort_interp("Non-player argument (1).");

    CHECKREMOTE(oper2->data.objref);

    if (!valid_object(oper1))
        abort_interp("invalid object (2).");

    CHECKREMOTE(oper1->data.objref);

    result = !could_doit(fr->descr, oper2->data.objref, oper1->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF RECYCLE
 *
 * Consumes a non-player dbref, and recyles it.
 *
 * Requires MUCKER level 3.  Level 4 is needed to bypass basic ownership
 * checks.
 *
 * In addition, cannot use this to recycle:
 * - the global environment
 * - objects that are referred to by sysparm values
 * - the currently-running program or those on the active call stack
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_recycle(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();              /* object dbref to recycle */

    /**
     * @TODO This seems to be a redundant check.
     */
    if (oper1->type != PROG_OBJECT)
        abort_interp("Non-object argument (1).");

    if (!valid_object(oper1))
        abort_interp("Invalid object (1).");

    result = oper1->data.objref;

    if ((mlev < 3) || ((mlev < 4) && !permissions(ProgUID, result)))
        abort_interp("Permission denied.");

    if (result == GLOBAL_ENVIRONMENT)
        abort_interp("Cannot recycle the global environment.");

    if (Typeof(result) == TYPE_PLAYER)
        abort_interp("Cannot recycle a player.");

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->type == TP_TYPE_DBREF && result == *tent->currentval.d) {
            abort_interp("Cannot currently recycle that object.");
        }
    }

    if (result == program)
        abort_interp("Cannot recycle currently running program.");

    for (int ii = 0; ii < fr->caller.top; ii++)
        if (fr->caller.st[ii] == result)
            abort_interp("Cannot recycle active program.");

    if (Typeof(result) == TYPE_EXIT)
        unset_source(result);

    CLEAR(oper1);
    recycle(fr->descr, player, result);
}

/**
 * Sets a lock (with the given key) on an object.
 *
 * Returns 0 if the key is not well-formed, otherwise 1.
 *
 * @private
 * @param descr the player's descriptor
 * @param player the player requesting the change
 * @param thing dbref of the target object
 * @param keyname string that represents the pass condition
 */
static int
setlockstr(int descr, dbref player, dbref thing, const char *keyname)
{
    /**
     * @TODO Should this logic be exposed to the various lock
     *       commands?
     */
    struct boolexp *key;

    if (*keyname != '\0') {
        key = parse_boolexp(descr, player, keyname, 0);
        if (key == TRUE_BOOLEXP) {
            return 0;
        } else {
            /* everything ok, do it */
            SETLOCK(thing, key);
            return 1;
        }
    } else {
        CLEARLOCK(thing);
        return 1;
    }
}

/**
 * Implementation of MUF SETLOCKSTR
 *
 * Consumes a dbref and a string, sets the lock accordingly, and returns
 * a boolean representing success or failure.  Needs MUCKER level 4 to
 * bypass basic ownership checks.
 *
 * @see setlockstr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_setlockstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (!valid_object(oper2))
        abort_interp("Invalid argument type (1)");

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    ref = oper2->data.objref;
    if ((mlev < 4) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    result = setlockstr(fr->descr, player, ref, DoNullInd(oper1->data.string));

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF GETLOCKSTR
 *
 * Consumes a dbref, and returns its lock string or PROP_UNLOCKED_VAL if
 * unlocked.  Requires remote-read privileges when applicable.
 *
 * Needs MUCKER level 3 to bypass basic ownership chekcs.
 *
 * @see unparse_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_getlockstr(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid argument type");

    ref = oper1->data.objref;
    CHECKREMOTE(ref);

    if ((mlev < 3) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    {
        const char *tmpstr = (char *) unparse_boolexp(player, GETLOCK(ref), 0);

        CLEAR(oper1);
        PushString(tmpstr);
    }
}

/**
 * Implementation of MUF PART_PMATCH
 *
 * Consumes a string, and returns the dbref of an online player that has
 * a name that begins with the string.  Requires MUCKER level 3.
 *
 * Can return NOTHING or AMBIGUOUS.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_part_pmatch(PRIM_PROTOTYPE)
{
    dbref ref;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument.");

    if (mlev < 3)
        abort_interp("Permission denied.  Requires Mucker Level 3.");

    ref = partial_pmatch(DoNullInd(oper1->data.string));

    CLEAR(oper1);
    PushObject(ref);
}

/**
 * Implementation of MUF CHECKPASSWORD
 *
 * Consumes a player dbref and a string, and returns a boolean representing
 * if the given password is correct for that player.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_checkpassword(PRIM_PROTOTYPE)
{
    char *ptr;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Player dbref expected. (1)");

    ref = oper1->data.objref;

    /**
     * @TODO The NOTHING parts of this check are confusing.
     */
    if ((ref != NOTHING && !valid_player(oper1)) || ref == NOTHING)
        abort_interp("Player dbref expected. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Password string expected. (2)");

    ptr = DoNullInd(oper2->data.string);

    /**
     * @TODO NOTHING should be caught before this point.
     */
    if (ref != NOTHING) {
        result = check_password(ref, ptr);
    } else {
        result = 0;
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF MOVEPENNIES
 *
 * Consumes two dbrefs and a non-negative integer, and moves that value from
 * the first player/thing to the second.  Requires MUCKER level
 * tp_movepennies_muf_mlev for players and 4 for things.
 *
 * Needs MUCKER level 4 to bypass range checks for players.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_movepennies(PRIM_PROTOTYPE)
{
    dbref ref2;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (mlev < tp_movepennies_muf_mlev)
        abort_interp("Permission Denied (mlev < tp_movepennies_muf_mlev)");

    if (!valid_object(oper3))
        abort_interp("Invalid object. (1)");

    if (!valid_object(oper2))
        abort_interp("Invalid object. (2)");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");

    if (oper1->data.number < 0)
        abort_interp("Invalid argument. (3)");

    ref = oper3->data.objref;
    ref2 = oper2->data.objref;

    /**
     * @TODO See prim_addpennies for comments about this if block.
     */
    if (mlev < 4) {
        if (Typeof(ref) == TYPE_PLAYER) {
            if (GETVALUE(ref) < (GETVALUE(ref) - oper1->data.number))
                abort_interp("Would roll over player's score. (1)");

            if ((GETVALUE(ref) - oper1->data.number) < 0)
                abort_interp("Result would be negative. (1)");
        } else {
            abort_interp("Permission denied. (2)");
        }

        if (Typeof(ref2) == TYPE_PLAYER) {
            if (GETVALUE(ref2) > (GETVALUE(ref2) + oper1->data.number))
                abort_interp("Would roll over player's score. (2)");

            if ((GETVALUE(ref2) + oper1->data.number) > tp_max_pennies)
                abort_interp("Would exceed MAX_PENNIES. (2)");
        } else {
            abort_interp("Permission denied. (2)");
        }
    }

    /**
     * @TODO No aborting if either dbref is a room, exit, or program?
     */
    SETVALUE(ref, GETVALUE(ref) - oper1->data.number);
    DBDIRTY(ref);
    SETVALUE(ref2, GETVALUE(ref2) + oper1->data.number);
    DBDIRTY(ref2);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

/**
 * Implementation of MUF FINDNEXT
 *
 * Consumes two dbrefs (current object and an owner object) and two strings
 * (name pattern and flag pattern), and returns the next object in the
 * database that matches both patterns and has the specified owner.
 *
 * Requires MUCKER level 2.  Needs MUCKER level 3 to search for objects
 * with an owner other than the player (including specifying NOTHING to
 * bypass the owner check.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_findnext(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    dbref who, item, ref;
    const char *name;

    CHECKOP(4);
    oper4 = POP();              /* str:flags */
    oper3 = POP();              /* str:namepattern */
    oper2 = POP();              /* ref:owner */
    oper1 = POP();              /* ref:currobj */

    if (oper4->type != PROG_STRING)
        abort_interp("Expected string argument. (4)");

    if (oper3->type != PROG_STRING)
        abort_interp("Expected string argument. (3)");

    if (!valid_player(oper2) && (oper2->data.objref != NOTHING))
        abort_interp("Expected player argument. (2)");

    /**
     * @TODO Can the next three checks be combined into:
     *           if (!valid_object(oper1) && oper1->type != NOTHING)
     */
    if (oper1->type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");

    if (!OkRef(oper1->data.objref))
        abort_interp("Bad object. (1)");

    if (oper1->data.objref != NOTHING
            && Typeof(oper1->data.objref) == TYPE_GARBAGE)
        abort_interp("Garbage object. (1)");

    item = oper1->data.objref;
    who = oper2->data.objref;
    name = DoNullInd(oper3->data.string);

    if (mlev < 2)
        abort_interp("Permission denied.  Requires at least Mucker Level 2.");

    if (mlev < 3) {
        if (who == NOTHING) {
            abort_interp("Permission denied.  "
                    "Owner inspecific searches require Mucker Level 3.");
        } else if (who != ProgUID) {
            abort_interp("Permission denied.  Searching for "
                    "other people's stuff requires Mucker Level 3.");
        }
    }

    if (item == NOTHING) {
        item = 0;
    } else {
        item++;
    }

    strcpyn(buf, sizeof(buf), name);

    ref = NOTHING;

    init_checkflags(player, DoNullInd(oper4->data.string), &check);

    for (dbref i = item; i < db_top; i++) {
        if ((who == NOTHING || OWNER(i) == who) &&
            checkflags(i, check) && NAME(i) && Typeof(i) != TYPE_GARBAGE &&
            (!*name || equalstr(buf, (char *) NAME(i)))) {
            ref = i;
            break;
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
    PushObject(ref);
}

/**
 * Implementation of MUF NEXTENTRANCE
 *
 * Consumes two dbrefs, and returns the next object in the database after
 * the second that is linked to the first.  Requires MUCKER level 3.
 *
 * This primitive does resolve HOME to the player's home.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_nextentrance(PRIM_PROTOTYPE)
{
    dbref linkref, ref;
    int foundref = 0;
    int count;

    if (mlev < 3)
        abort_interp("Permission denied.  Requires Mucker Level 3.");

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    linkref = oper1->data.objref;
    ref = oper2->data.objref;

    if (!valid_object(oper1) && (linkref != NOTHING) && (linkref != HOME))
        abort_interp("Invalid link reference object (2)");

    if (!valid_object(oper2) && ref != NOTHING)
        abort_interp("Invalid reference object (1)");

    if (linkref == HOME)
        linkref = PLAYER_HOME(player);

    (void) ref++;

    for (; ref < db_top; ref++) {
        oper2->data.objref = ref;

        if (valid_object(oper2)) {
            switch (Typeof(ref)) {
                case TYPE_PLAYER:
                    if (PLAYER_HOME(ref) == linkref)
                        foundref = 1;
                    break;

                case TYPE_ROOM:
                    if (DBFETCH(ref)->sp.room.dropto == linkref)
                        foundref = 1;
                    break;

                case TYPE_THING:
                    if (THING_HOME(ref) == linkref)
                        foundref = 1;
                    break;

                case TYPE_EXIT:
                    count = DBFETCH(ref)->sp.exit.ndest;

                    for (int i = 0; i < count; i++) {
                        if (DBFETCH(ref)->sp.exit.dest[i] == linkref)
                            foundref = 1;
                    }
                    break;
            }

            if (foundref)
                break;
        }
    }

    if (!foundref)
        ref = NOTHING;

    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(ref);
}

/**
 * Implementation of MUF NEWPLAYER
 *
 * Consumes two strings (name and password), and returns the dbref of
 * the new player created if successful.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_newplayer(PRIM_PROTOTYPE)
{
    dbref newplayer;
    char *name, *password;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument. (2)");

    name = DoNullInd(oper2->data.string);
    password = DoNullInd(oper1->data.string);

    newplayer = create_player(name, password);

    if (newplayer == NOTHING)
        abort_interp("Create failed.");

    log_status("PCREATED[MUF]: %s(%d) by %s(%d)",
            NAME(newplayer), (int) newplayer, NAME(player), (int) player);

    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(newplayer);
}

/**
 * Implementation of MUF COPYPLAYER
 *
 * Consumes a dbref and two strings (name and password), and returns the
 * dbref of new player created from the template object if successful.
 * Requires MUCKER level 4.
 *
 * The new player has the flags, properties, home, and value of the template
 * object - and is relocated to its home.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_copyplayer(PRIM_PROTOTYPE)
{
    dbref newplayer, ref;
    char *name, *password;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (3)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument. (2)");

    ref = oper3->data.objref;

    if ((ref != NOTHING && !valid_player(oper3)) || ref == NOTHING)
        abort_interp("Player dbref expected. (1)");

    /**
     * @TODO This should never abort, since mlev >= 2.
     */
    CHECKREMOTE(ref);

    name = DoNullInd(oper2->data.string);
    password = DoNullInd(oper1->data.string);

    newplayer = create_player(name, password);

    if (newplayer == NOTHING)
        abort_interp("Create failed.");

    /* initialize everything */
    FLAGS(newplayer) = FLAGS(ref);

    copy_properties_onto(ref, newplayer);

    PLAYER_SET_HOME(newplayer, PLAYER_HOME(ref));
    SETVALUE(newplayer, GETVALUE(newplayer) + GETVALUE(ref));
    moveto(newplayer, PLAYER_HOME(ref));

    /* link him to player_start */
    log_status("PCREATE[MUF]: %s(%d) by %s(%d)",
               NAME(newplayer), (int) newplayer, NAME(player), (int) player);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushObject(newplayer);
}

/**
 * Implementation of MUF TOADPLAYER
 *
 * Consumes two different player dbrefs, and toads the first.  Its possessions
 * and macros are chowned to the second player.  Requires MUCKER level 4.
 *
 * This primitive cannot be forced.
 *
 * In addition, cannot use this to toad players that are:
 * - protected (with NO_RECYCLE_PROP - usually '@/precious')
 * - wizards (or God with GOD_PRIV)
 * - referred to by sysparm values
 *
 * @see toad_player
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_toadplayer(PRIM_PROTOTYPE)
{
    dbref victim;
    dbref recipient;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (force_level)
        abort_interp("Cannot be forced.");

    victim = oper1->data.objref;

    if (!valid_player(oper1))
        abort_interp("Player dbref expected for player to be toaded (2)");

    recipient = oper2->data.objref;

    if (!valid_player(oper2))
        abort_interp("Player dbref expected for recipient (1)");

    /**
     * @TODO These should never abort, since mlev >= 2.
     */
    CHECKREMOTE(victim);
    CHECKREMOTE(recipient);

    if (victim == recipient) {
        abort_interp("Victim and recipient must be different players.");
    }

    if (get_property_class(victim, NO_RECYCLE_PROP)) {
        abort_interp("That player is precious.");
    }

#ifdef GOD_PRIV
    if (God(victim)) {
        abort_interp("God may not be toaded. (2)");
    }
#endif

    if ((FLAGS(victim) & WIZARD)) {
        abort_interp("You can't toad a wizard.");
    }

    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        if (tent->type == TP_TYPE_DBREF && victim == *tent->currentval.d) {
            abort_interp("That player cannot currently be @toaded.");
        }
    }

    log_status("TOADED[MUF]: %s(%d) by %s(%d)", NAME(victim), victim, NAME(player), player);
    toad_player(fr->descr, player, victim, recipient);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF OBJMEM
 *
 * Consumes a dbref, and returns the number of bytes of memory it currently
 * uses.  This works on recycled objects.
 *
 * @see size_object
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_objmem(PRIM_PROTOTYPE)
{
    int i;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_OBJECT)
        abort_interp("Argument must be a dbref.");

    ref = oper1->data.objref;

    if (!ObjExists(ref))
        abort_interp("Invalid object.");

    CLEAR(oper1);
    i = size_object(ref, 0);
    PushInt(i);
}

/**
 * Implementation of MUF INSTANCES
 *
 * Consumes a program dbref, and returns the number of active processes
 * that are running its code.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_instances(PRIM_PROTOTYPE)
{
    unsigned short a = 0;
    int b = 0;

    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Object must be a program.");

    CLEAR(oper1);

    a = PROGRAM_INSTANCES(ref);
    b = a;
    PushInt(b);
}

/**
 * Implementation of MUF COMPILED?
 *
 * Consumes a program dbref, and returns the number of compiled instructions
 * it has, or 0 if it is not compiled.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_compiledp(PRIM_PROTOTYPE)
{
    int i = 0;
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Object must be a program.");

    CLEAR(oper1);
    i = PROGRAM_SIZ(ref);
    PushInt(i);
}

/**
 * Implementation of MUF NEWPASSWORD
 *
 * Consumes a player dbref and a string, and sets the player's password.
 * Requires MUCKER level 4.
 *
 * With GOD_PRIV defined, only God can use this primitive and cannot be the
 * target of it.
 *
 * @see set_password
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_newpassword(PRIM_PROTOTYPE)
{
    char *ptr2;
    char pad_char[] = "";

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (oper1->type != PROG_STRING)
        abort_interp("Password string expected. (2)");

    if (oper2->type != PROG_OBJECT)
        abort_interp("Player dbref expected. (1)");

    if (!oper1->data.string || !ok_password(oper1->data.string->data))
        abort_interp("Invalid password. (2)");

    /**
     * @TODO Change to: ptr2 = DoNullInd(oper1->data.string);
     */
    ptr2 = oper1->data.string ? oper1->data.string->data : pad_char;
    ref = oper2->data.objref;
    if (!valid_player(oper2))
        abort_interp("Player dbref expected. (1)");

    /**
     * @TODO This should never abort, since mlev >= 2.
     */
    CHECKREMOTE(ref);

#ifdef GOD_PRIV
    if (God(ref))
        abort_interp("God cannot be newpassworded.");
    if (!God(player) && TrueWizard(ref) && (player != ref))
        abort_interp("Only God can change a wizards password");
#endif

    set_password(ref, ptr2);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF NEWPROGRAM
 *
 * Consumes a string, creates a program with that name, and returns its
 * dbref.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_newprogram(PRIM_PROTOTYPE)
{
    dbref newprog;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (oper1->type != PROG_STRING)
       abort_interp("Expected string argument.");

    char *b = DoNullInd(oper1->data.string);

    if (!ok_object_name(b, TYPE_PROGRAM))
        abort_interp("Invalid name (2)");

    newprog = create_program(ProgUID, b);

    CLEAR(oper1);
    PushObject(newprog);
}

/**
 * Implementation of MUF COMPILE
 *
 * Consumes a program dbref and a boolean, compiles the program, and returns
 * the number of compiled instructions if successful, otherwise 0.  Requires
 * MUCKER level 4.
 *
 * If the boolean is true, errors and warnings are shown to the player.
 *
 * Cannot recompile a running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_compile(PRIM_PROTOTYPE)
{
    dbref ref;
    struct line *tmpline;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    /**
     * @TODO Combine these two checks.
     */
    if (!valid_object(oper1))
        abort_interp("No program dbref given. (1)");

    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("No program dbref given. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("No boolean integer given. (2)");

    if (PROGRAM_INSTANCES(ref) > 0)
        abort_interp("That program is currently in use.");

    tmpline = PROGRAM_FIRST(ref);
    PROGRAM_SET_FIRST(ref, read_program(ref));

    do_compile(fr->descr, player, ref, oper2->data.number);

    free_prog_text(PROGRAM_FIRST(ref));
    PROGRAM_SET_FIRST(ref, tmpline);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(PROGRAM_SIZ(ref));
}

/**
 * Implementation of MUF UNCOMPILE
 *
 * Consumes a program dbref, and uncompiles the program. Requires
 * MUCKER level 4.
 *
 * Cannot uncompile a running program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_uncompile(PRIM_PROTOTYPE)
{
    dbref ref;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    /**
     * @TODO Combine these two checks.
     */
    if (!valid_object(oper1))
        abort_interp("No program dbref given.");

    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("No program dbref given.");

    if (PROGRAM_INSTANCES(ref) > 0)
        abort_interp("That program is currently in use.");

    uncompile_program(ref);

    CLEAR(oper1);
}

/**
 * Implementation of MUF GETPIDS
 *
 * Consumes a dbref, and returns an array of associated process IDs.
 * Requires MUCKER level 3.
 *
 * @see get_pids
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_getpids(PRIM_PROTOTYPE)
{
    stk_array *nw;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Permission denied.  Requires Mucker Level 3.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");

    /**
     * @TODO Consider modifying this function to handle the running program.
     */
    nw = get_pids(oper1->data.objref, fr->pinning);

    if (program == oper1->data.objref) {
        struct inst temp;

        temp.type = PROG_INTEGER;
        temp.data.number = fr->pid;

        array_appenditem(&nw, &temp);

        CLEAR(&temp);
    }

    CLEAR(oper1);
    PushArrayRaw(nw);
}

/**
 * Implementation of MUF GETPIDINFO
 *
 * Consumes an integer, and returns a dictionary with information related
 * to the process ID.
 *
 * Needs MUCKER level 3 to view informtion on another process.
 *
 * @see get_pidinfo
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_getpidinfo(PRIM_PROTOTYPE)
{
    struct inst temp1, temp2;
    stk_array *nu;
    double cpu;
    time_t etime;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");

    if (mlev < 3 && oper1->data.number != fr->pid)
        abort_interp("Permission denied.  Requires Mucker Level 3.");

    if (oper1->data.number == fr->pid) {
        if ((nu = new_array_dictionary(fr->pinning)) == NULL)
            abort_interp("Out of memory");

        if ((etime = time(NULL) - fr->started) > 0) {
            cpu = ((fr->totaltime.tv_sec +
                    (fr->totaltime.tv_usec / 1000000.0)) * 100.0) / etime;

            if (cpu > 100.0)
                cpu = 100.0;
        } else
            cpu = 0.0;

        array_set_strkey_strval(&nu, "CALLED_DATA", "");
        array_set_strkey_refval(&nu, "CALLED_PROG", program);
        array_set_strkey_fltval(&nu, "CPU", cpu);
        array_set_strkey_intval(&nu, "DESCR", fr->descr);

        /**
         * @TODO Keep the array_set_* calls together.
         */
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("FILTERS");
        temp2.type = PROG_ARRAY;
        temp2.data.array = new_array_packed(0, fr->pinning);

        array_setitem(&nu, &temp1, &temp2);

        CLEAR(&temp1);
        CLEAR(&temp2);

        array_set_strkey_intval(&nu, "INSTCNT", fr->instcnt);
        array_set_strkey_intval(&nu, "MLEVEL", mlev);
        array_set_strkey_intval(&nu, "NEXTRUN", 0);
        array_set_strkey_intval(&nu, "PID", fr->pid);
        array_set_strkey_refval(&nu, "PLAYER", player);
        array_set_strkey_intval(&nu, "STARTED", (int)fr->started);
        array_set_strkey_strval(&nu, "SUBTYPE", "");
        array_set_strkey_refval(&nu, "TRIG", fr->trig);
        array_set_strkey_strval(&nu, "TYPE", "MUF");
    } else
        /**
         * @TODO Consider modifying this function to handle the running pid.
         */
        nu = get_pidinfo(oper1->data.number, fr->pinning);

    CLEAR(oper1);
    PushArrayRaw(nu);
}

/**
 * Implementation of MUF CONTENTS_ARRAY
 *
 * Consumes a dbref, and returns an array of its contents.  Requires
 * remote-read permissions when applicable.
 *
 * Unlike prim_contents, this has no MUCKER level 1 restrictions.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_contents_array(PRIM_PROTOTYPE)
{
    stk_array *nw;
    int count = 0;

    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid dbref (1)");

    ref = oper1->data.objref;
    if ((Typeof(ref) == TYPE_PROGRAM) || (Typeof(ref) == TYPE_EXIT)) {
        PushArrayRaw(new_array_packed(0, fr->pinning));
        return;
    }

    CHECKREMOTE(oper1->data.objref);

    for (ref = CONTENTS(oper1->data.objref); ObjExists(ref); ref = NEXTOBJ(ref))
        count++;

    nw = new_array_packed(count, fr->pinning);

    for (ref = CONTENTS(oper1->data.objref), count = 0; ObjExists(ref);
             ref = NEXTOBJ(ref))
        array_set_intkey_refval(&nw, count++, ref);

    CLEAR(oper1);
    PushArrayRaw(nw);
}

/**
 * Implementation of MUF EXITS_ARRAY
 *
 * Consumes a dbref, and returns an array of its exits.  Requires
 * remote-read permissions when applicable.
 *
 * Needs MUCKER level 3 to byp[ass basic ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_exits_array(PRIM_PROTOTYPE)
{
    stk_array *nw;
    int count = 0;

    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid dbref (1)");

    ref = oper1->data.objref;
    CHECKREMOTE(oper1->data.objref);

    if ((mlev < 3) && !permissions(ProgUID, ref))
        abort_interp("Permission denied.");

    if ((Typeof(ref) == TYPE_PROGRAM) || (Typeof(ref) == TYPE_EXIT)) {
        PushArrayRaw(new_array_packed(0, fr->pinning));
        return;
    }

    for (ref = EXITS(oper1->data.objref); ObjExists(ref); ref = NEXTOBJ(ref))
        count++;

    nw = new_array_packed(count, fr->pinning);

    for (ref = EXITS(oper1->data.objref), count = 0; ObjExists(ref);
            ref = NEXTOBJ(ref))
        array_set_intkey_refval(&nw, count++, ref);

    CLEAR(oper1);
    PushArrayRaw(nw);
}

/**
 * Returns an array of links to the given object.
 *
 * @private
 * @param obj dbref of the object to get links for
 * @param pinned the pinned bit of the current frame
 */
static stk_array *
array_getlinks(dbref obj, int pinned)
{
    stk_array *nw = new_array_packed(0, pinned);
    int count = 0;

    if (ObjExists(obj)) {
        switch (Typeof(obj)) {
            case TYPE_ROOM:
                array_set_intkey_refval(&nw, count++,
                        DBFETCH(obj)->sp.room.dropto);
                break;

            case TYPE_THING:
                array_set_intkey_refval(&nw, count++, THING_HOME(obj));
                break;

            case TYPE_PLAYER:
                array_set_intkey_refval(&nw, count++, PLAYER_HOME(obj));
                break;

            case TYPE_EXIT:
                for (count = 0; count < (DBFETCH(obj)->sp.exit.ndest); count++)
                    array_set_intkey_refval(&nw, count,
                            (DBFETCH(obj)->sp.exit.dest)[count]);
                break;
        }
    }

    return nw;
}

/**
 * Implementation of MUF GETLINKS_ARRAY
 *
 * Consumes a dbref, and returns an array of its links.  Requires remote-read
 * privileges when applicable.
 *
 * Unlike prim_getlinks, does not abort on program objects.
 *
 * @see array_getlinks
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_getlinks_array(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object dbref. (1)");

    ref = oper1->data.objref;
    CHECKREMOTE(oper1->data.objref);

    CLEAR(oper1);
    PushArrayRaw(array_getlinks(ref, fr->pinning));
}

/**
 * Implementation of MUF ENTRANCES_ARRAY
 *
 * Consumes a dbref, and returns an array of objects that link to it.
 * Requires remote-read privileges when applicable.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_entrances_array(PRIM_PROTOTYPE)
{
    /**
     * @TODO NEXTENTRANCE requires MUCKER levl 3. Should this?
     */
    stk_array *nw;
    int count = 0;

    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object dbref. (1)");

    ref = oper1->data.objref;
    nw = new_array_packed(0, fr->pinning);

    for (dbref i = 0; i < db_top; i++) {
        switch (Typeof(i)) {
            case TYPE_EXIT:
                for (dbref j = DBFETCH(i)->sp.exit.ndest; j--;) {
                    if (DBFETCH(i)->sp.exit.dest[j] == ref)
                        array_set_intkey_refval(&nw, count++, i);
                }
                break;

            case TYPE_PLAYER:
                if (PLAYER_HOME(i) == ref)
                    array_set_intkey_refval(&nw, count++, i);
                break;

            case TYPE_THING:
                if (THING_HOME(i) == ref)
                    array_set_intkey_refval(&nw, count++, i);
                break;

            case TYPE_ROOM:
                if (DBFETCH(i)->sp.room.dropto == ref)
                    array_set_intkey_refval(&nw, count++, i);
                break;

            /**
             * @TODO These two cases are unnecessary.
             */
            case TYPE_PROGRAM:
            case TYPE_GARBAGE:
                break;
        }
    }

    CLEAR(oper1);
    PushArrayRaw(nw);
}

/**
 * Implementation of MUF PROGRAM_GETLINES
 *
 * Consumes a program dbref and two non-negative integers, and returns
 * an array of code lines within that range.
 *
 * Needs MUCKER level 4 to bypass advanced controls checks or read
 * non-VIEWABLE programs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_program_getlines(PRIM_PROTOTYPE)
{
    stk_array *ary;
    int start, end;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("Invalid object dbref. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected integer. (2)");

    if (oper3->type != PROG_INTEGER)
        abort_interp("Expected integer. (3)");

    ref = oper1->data.objref;
    start = oper2->data.number;
    end = oper3->data.number;

    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Non-program object. (1)");

    if (mlev < 4 && !controls(ProgUID, ref) && !(FLAGS(ref) & VEHICLE))
        abort_interp("Permission denied.");

    if (start < 0 || end < 0)
        abort_interp("Line indexes must be non-negative.");

    if (start == 0)
        start = 1;

    if (end && start > end)
        abort_interp("Illogical line range.");

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    {
        /* we make two passes over our linked list's data,
         * first we figure out how many lines are
         * actually there. This is so we only allocate
         * our array once, rather re-allocating 4000 times
         * for a 4000-line program listing, while avoiding
         * letting calls like '#xxx 1 999999 program_getlines'
         * taking up tones of memory.
         */
        int i, count;
        /* current line we're iterating over */
        struct line *curr;
        /* first line in the program */
        struct line *first;
        /* starting line in our segment of the program */
        struct line *segment;

        curr = first = read_program(ref);

        /* find our line */
        for (i = 1; curr && i < start; i++)
            curr = curr->next;

        if (!curr) {
            /* alright, we have no data! */
            free_prog_text(first);
            PushArrayRaw(new_array_packed(0, fr->pinning));
            return;
        }

        segment = curr;         /* we need to keep this line */

        /* continue our looping */
        for (; curr && (!end || i < end); i++)
            curr = curr->next;

        count = i - start + 1;

        if (!curr)              /* back up if needed */
            count--;

        ary = new_array_packed(count, fr->pinning);

        /*
         * so we count down from the number of lines we have,
         * and set our array appropriatly.
         */
        for (curr = segment, i = 0; count--; i++, curr = curr->next) {
            array_set_intkey_strval(&ary, i, curr->this_line);
        }

        free_prog_text(first);
    }
    PushArrayRaw(ary);
}

/**
 * Implementation of MUF PROGRAM_SETLINES
 *
 * Consumes a program dbref and a sequential array of strings, and sets the
 * program code lines to the contents of the array.  Requires MUCKER level 4.
 *
 * Effective user must control the program, and it must not be being edited.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_program_setlines(PRIM_PROTOTYPE)
{
    char unparse_buf[BUFFER_LEN];
    struct line *lines = 0;
    struct line *prev = 0;
    array_iter idx;

    CHECKOP(2);

    oper2 = POP();              /* list:Lines */
    oper1 = POP();              /* ref:Program */

    if (mlev < 4)
        abort_interp("Mucker level 4 or greater required.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Non-object argument. (1)");

    if (oper2->type != PROG_ARRAY)
        abort_interp("Non-array argument. (2)");

    /**
     * @TODO Combine the next two checks.
     */
    if (!valid_object(oper1))
        abort_interp("Invalid object. (1)");

    if (Typeof(oper1->data.objref) != TYPE_PROGRAM)
        abort_interp("Non-program object. (1)");

    if (oper2->data.array && (oper2->data.array->type != ARRAY_PACKED))
        abort_interp("Array list type required. (2)");

    if (!array_is_homogenous(oper2->data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (2)");

    if (!controls(ProgUID, oper1->data.objref))
        abort_interp("Permission denied.");

    if (FLAGS(oper1->data.objref) & INTERNAL)
        abort_interp("Program already being edited.");

    if (array_first(oper2->data.array, &idx)) {
        do {
            array_data *val = array_getitem(oper2->data.array, &idx);
            struct line *ln = get_new_line();

            ln->this_line = alloc_string(val->data.string ?
                     val->data.string->data : " ");

            if (prev) {
                prev->next = ln;
                ln->prev = prev;
            } else
                lines = ln;

            prev = ln;
        }
        while (array_next(oper2->data.array, &idx));
    }

    write_program(lines, oper1->data.objref);
    unparse_object(player, oper1->data.objref, unparse_buf,
            sizeof(unparse_buf));
    log_status("PROGRAM SAVED: %s by %s(%d)", unparse_buf,
            NAME(player), player);

    if (tp_log_programs)
        log_program_text(lines, player, oper1->data.objref);

    free_prog_text(lines);

    CLEAR(oper1);
    CLEAR(oper2);

    DBDIRTY(program);
}

/**
 * Implementation of MUF SETLINKS_ARRAY
 *
 * Consumes a dbref and a sequential array of dbrefs, and sets the links of
 * the single dbref to the contents of the array.  The array may contain
 * more than one item (up to MAX_LINKS) if the source is an exit.
 *
 * Needs MUCKER level 4 to bypass basic ownership checks on the source.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_setlinks_array(PRIM_PROTOTYPE)
{
    array_iter idx;
    size_t dest_count;
    dbref what;
    stk_array *arr;

    CHECKOP(2);

    oper2 = POP();              /* arr:Destination(s) */
    oper1 = POP();              /* ref:Source */

    if (oper1->type != PROG_OBJECT)
        abort_interp("Non-object argument. (1)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Non-array argument. (3)");

    if (!array_is_homogenous(oper2->data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (2)");

    if (!valid_object(oper1))
        abort_interp("Invalid object. (1)");

    if ((mlev < 4) && !permissions(ProgUID, oper1->data.objref))
        abort_interp("Permission denied. (1)");

    if ((dest_count = (size_t)array_count(oper2->data.array)) >= MAX_LINKS)
        abort_interp("Too many destinations. (2)");

    if ((dest_count > 1) && (Typeof(oper1->data.objref) != TYPE_EXIT))
        abort_interp("Only exit may be linked to multiple destinations.");

    what = oper1->data.objref;
    arr = oper2->data.array;

    if (array_first(arr, &idx)) {
        int found_prp = 0;

        do {
            array_data *val = array_getitem(arr, &idx);
            dbref where = val->data.objref;

            if ((where != HOME) && (where != NIL) && !valid_object(val)) {
                    CLEAR(&idx);
                abort_interp("Invalid object. (2)");
            }

            if (!prog_can_link_to(mlev, ProgUID, Typeof(what), where)) {
                    CLEAR(&idx);
                abort_interp("Can't link source to destination. (2)");
            }

            switch (Typeof(what)) {
                case TYPE_EXIT:
                    if (where == NIL)
                        break;

                    switch (Typeof(where)) {
                        case TYPE_PLAYER:
                        case TYPE_ROOM:
                        case TYPE_PROGRAM:
                            if (found_prp != 0) {
                                CLEAR(&idx);
                               abort_interp("Only one player, room, or "
                                       "program destination allowed.");
                            }

                            found_prp = 1;
                            break;

                        case TYPE_THING:
                            break;

                        case TYPE_EXIT:
                            if (exit_loop_check(what, where)) {
                                CLEAR(&idx);
                                abort_interp("Destination would create loop.");
                            }
                            break;

                        default:
                            CLEAR(&idx);
                            abort_interp("Invalid object. (2)");
                    }
                    break;

                case TYPE_PLAYER:
                    if (where == HOME) {
                        CLEAR(&idx);
                        abort_interp("Cannot link player to HOME.");
                    }
                    break;

                case TYPE_THING:
                    if (where == HOME) {
                        CLEAR(&idx);
                        abort_interp("Cannot link thing to HOME.");
                    }

                    if (parent_loop_check(what, where)) {
                        CLEAR(&idx);
                        abort_interp("That would case a parent paradox.");
                    }
                    break;

                case TYPE_ROOM:
                    break;

                default:
                    CLEAR(&idx);
                    abort_interp("Invalid object. (1)");
            }
        } while (array_next(arr, &idx));
    }

    if (Typeof(what) == TYPE_EXIT) {
        if (MLevRaw(what))
            SetMLevel(what, 0);

        free(DBFETCH(what)->sp.exit.dest);
    }

    if (dest_count == 0) {
        switch (Typeof(what)) {
            case TYPE_EXIT:
                DBSTORE(what, sp.exit.ndest, 0);
                DBSTORE(what, sp.exit.dest, NULL);
                break;

            case TYPE_ROOM:
                DBSTORE(what, sp.room.dropto, NOTHING);
                break;

            default:
                abort_interp("Only exits and rooms may be linked to nothing.");
        }
    } else {
        switch (Typeof(what)) {
            case TYPE_EXIT: {
                dbref *dests = malloc(sizeof(dbref) * dest_count);

                if (dests == NULL) {
                    DBSTORE(what, sp.exit.ndest, 0);
                    DBSTORE(what, sp.exit.dest, NULL);
                    abort_interp("Out of memory.");
                }

                if (array_first(arr, &idx)) {
                    unsigned int i = 0;

                    do {
                        if (i < dest_count)
                            dests[i++] = array_getitem(arr, &idx)->data.objref;
                    }
                    while (array_next(arr, &idx));
                }

                DBSTORE(what, sp.exit.ndest, dest_count);
                DBSTORE(what, sp.exit.dest, dests);
            }
            break;

            case TYPE_ROOM:
                if (array_first(arr, &idx)) {
                    DBSTORE(what, sp.room.dropto,
                            array_getitem(arr, &idx)->data.objref);
                    CLEAR(&idx);
                }
                break;

            case TYPE_PLAYER:
                if (array_first(arr, &idx)) {
                    PLAYER_SET_HOME(what,
                            array_getitem(arr, &idx)->data.objref);
                    CLEAR(&idx);
                }
                break;

            case TYPE_THING:
                if (array_first(arr, &idx)) {
                    THING_SET_HOME(what,
                            array_getitem(arr, &idx)->data.objref);
                    CLEAR(&idx);
                }
                break;

            default:
                abort_interp("Invalid object. (1)");
        }
    }

    DBDIRTY(what);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF SUPPLICANT
 *
 * Returns the dbref of the object being tested against a lock.  If the
 * program is no being run from a lock, returns NOTHING.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_supplicant(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    ref = fr->supplicant;

    CHECKOFLOW(1);
    PushObject(ref);
}

/**
 * Implementation of MUF PNAME_HISTORY
 *
 * Consumes a player dbref, and returns an array containing the name changes
 * that have been recorded for the player.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_pname_history(PRIM_PROTOTYPE)
{
    stk_array *nu;
    struct inst temp1, temp2;
    PropPtr propadr, pptr;
    char propname[BUFFER_LEN];

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

    if (!valid_player(oper1))
        abort_interp("Non-player argument (1).");

    if ((nu = new_array_dictionary(fr->pinning)) == NULL)
        abort_interp("Out of memory");

    ref = oper1->data.objref;

    if (tp_pname_history_reporting) {
        propadr = first_prop(ref, PNAME_HISTORY_PROPDIR, &pptr, propname,
                sizeof(propname));


        while (propadr) {
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string(propname);
            temp2.type = PROG_STRING;
            temp2.data.string = alloc_prog_string(PropDataStr(propadr));

            array_setitem(&nu, &temp1, &temp2);

            CLEAR(&temp1);
            CLEAR(&temp2);

            propadr = next_prop(pptr, propadr, propname, sizeof(propname));
        }
    }

    /**
     * @TODO CLEAR(oper1);
     */
    PushArrayRaw(nu);
}

/**
 * Implementation of MUF DUMP
 *
 * Takes no parameters and returns boolean; true if the dump was triggered,
 * false if it was not.  A dump may not be triggered in the case where a
 * dump is already in progress.
 *
 * On completion of the dump, it generates an event with eventID DUMP
 * which can be waited for / caught as desired.  It will have the integer
 * value of 1.
 *
 * Wizards only.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_dump(PRIM_PROTOTYPE)
{
    int result;

    if (mlev < 4)
        abort_interp("Permission denied.  Requires Wizbit.");

#ifndef DISKBASE
    /* Are we already dumping? */
    if (global_dumper_pid != 0) {
        result = 0;
        PushInt(result);
        return;
    }
#endif

    dump_db_now();

    result = 1;
    PushInt(result);
}

