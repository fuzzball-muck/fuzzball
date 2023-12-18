/** @file p_connects.c
 *
 * Implementation for handling MUF connection primitives.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
#include <stddef.h>
#include <limits.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "player.h"

/**
 * @private
 * @var used to store the parameters passed into the primitives
 *      This seems to be used for conveinance, but makes this probably
 *      not threadsafe.
 */
static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2;

/**
 * @private
 * @var to store the result which is not a local variable for some reason
 *      This makes things very not threadsafe.
 */
static int tmp, result;
static dbref ref;

/**
 * Implementation of MUF AWAKE?
 *
 * Consumes a dbref, and returns the number of connections the player
 * (or the zombie's owner) has to the server.
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
prim_awakep(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (!valid_object(oper1))
        abort_interp("invalid argument.");

    ref = oper1->data.objref;

    if (Typeof(ref) == TYPE_THING && (FLAGS(ref) & ZOMBIE))
        ref = OWNER(ref);

    if (Typeof(ref) != TYPE_PLAYER)
        abort_interp("invalid argument.");

    result = PLAYER_DESCRCOUNT(ref);

    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF ONLINE
 *
 * Returns a stackrange of dbrefs representing the players associated with
 * each connection to the server.
 *
 * @see pdescrcount
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
prim_online(PRIM_PROTOTYPE)
{
    struct descriptor_data* d = descriptor_list;
    stk_array *duparr;

    CHECKOP(0);

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    /*
     * This could be MORE than is actually connected if there's people sitting
     * on the connection screen, but should be a descent worst-case for
     * overflow checking.
     */
    result = pdescrcount();

    CHECKOFLOW(result+1);

    /*
     * A hacky way to skip duplicates. Not perfect, but feels better
     * than looping back through the list to check each time.
     */
    duparr = new_array_dictionary(0);

    temp1.type = PROG_INTEGER;

    result = 0;

    for ( ; d; d = d->next) {
        if (d->connected) {
            temp1.data.number = d->player;

            if (!array_getitem(duparr, &temp1)) {
                array_setitem(&duparr, &temp1, &temp1);
                result++;
            }
        }
    }

    /*
     * We assume that, if the programmer is iterating over connected
     * players, he's starting from the top, so we walk backwards and
     * push in reverse order.
     */
    if (array_last(duparr, &temp2)) {
        do {
            PushObject(temp2.data.number);
        } while (array_prev(duparr, &temp2));
    }

    array_free(duparr);

    PushInt(result);
}

/**
 * Implementation of MUF ONLINE_ARRAY
 *
 * Returns an array of dbrefs representing the players associated with
 * each connection to the server.
 *
 * @see pdescrcount
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
prim_online_array(PRIM_PROTOTYPE)
{
    stk_array *duparr, *nu;
    struct descriptor_data* d = descriptor_list;

    CHECKOP(0);
 
    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    CHECKOFLOW(1);

    /*
     * A hacky way to skip duplicates. Not perfect, but feels better
     * than looping back through the list to check each time.
     */
    duparr = new_array_dictionary(0);

    temp1.type = PROG_INTEGER;

    result = 0;

    for ( ; d; d = d->next) {
        if (d->connected) {
            temp1.data.number = d->player;

            if (!array_getitem(duparr, &temp1)) {
                array_setitem(&duparr, &temp1, &temp1);
                result++;
            }
        }
    }

    nu = new_array_packed(result, fr->pinning);

    result = 0;

    if (array_first(duparr, &temp2)) {
        do {
            temp1.data.number = result;
            temp2.type = PROG_OBJECT;

            array_setitem(&nu, &temp1, &temp2);

            temp2.type = PROG_INTEGER;

            result++;
        } while (array_next(duparr, &temp2));
    }

    array_free(duparr);

    PushArrayRaw(nu);
}

/**
 * Implementation of MUF DESCRCOUNT
 *
 * Returns the number of connections to the server.
 *
 * @see pdescrcount
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
prim_descrcount(PRIM_PROTOTYPE)
{
    /* -- int */
    CHECKOP(0);

    result = pdescrcount();

    CHECKOFLOW(1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCR
 *
 * Returns the descriptor number that started the program.  This will be -1
 * if it was invoked during a listener event or when the MUCK started up.
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
prim_descr(PRIM_PROTOTYPE)
{
    /* -- int */
    CHECKOP(0);

    result = fr->descr;

    CHECKOFLOW(1);
    PushInt(result);
}

/**
 * Implementation of MUF DESCRDBREF
 *
 * Consumes a descriptor number and returns the associated player dbref.
 * This will be NOTHING if no player is connected using that descriptor.
 *
 * @see pdescrdbref
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
prim_descr_dbref(PRIM_PROTOTYPE)
{
    /* int -- dbref */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pdescrdbref(oper1->data.number);

    if (result < 0)
	result = NOTHING;

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushObject(result);
}

/**
 * Implementation of MUF DESCRIDLE
 *
 * Consumes a descriptor number and returns how many seconds it has been idle.
 *
 * @see pdescridle
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
prim_descr_idle(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pdescridle(oper1->data.number);

    if (result < 0)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRLEASTIDLE
 *
 * Consumes a dbref and returns the least idle descriptor number, or -1 if
 * that cannot be determined.
 *
 * @see least_idle_player_descr
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
prim_descr_least_idle(PRIM_PROTOTYPE)
{
    /* obj -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Argument not a dbref.");

    if (!valid_object(oper1))
        abort_interp("Bad dbref.");

    result = least_idle_player_descr(oper1->data.objref);

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRMOSTIDLE
 *
 * Consumes a dbref and returns the most idle descriptor number, or -1 if
 * that cannot be determined.
 *
 * @see most_idle_player_descr
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
prim_descr_most_idle(PRIM_PROTOTYPE)
{
    /* obj -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Argument not a dbref.");

    if (!valid_object(oper1))
        abort_interp("Bad dbref.");

    result = most_idle_player_descr(oper1->data.objref);

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRTIME
 *
 * Consumes a descriptor number and returns how many seconds it has been
 * connected to the server.
 *
 * @see pontime
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
prim_descr_time(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pdescrontime(oper1->data.number);

    if (result < 0)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRHOST
 *
 * Consumes a descriptor number and returns the hostname or IP associated with
 * the connection.
 *
 * @see pdescrhost
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
prim_descr_host(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char *pname;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Primitive is a wizbit only command.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = oper1->data.number;

    pname = pdescrhost(result);

    if (!pname)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushString(pname);
}

/**
 * Implementation of MUF DESCRUSER
 *
 * Consumes a descriptor number and returns the username associated with
 * the connection.
 *
 * @see pdescruser
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
prim_descr_user(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char *pname;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Primitive is a wizbit only command.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = oper1->data.number;

    pname = pdescruser(result);

    if (!pname)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushString(pname);
}

/**
 * Implementation of MUF DESCRBOOT
 *
 * Consumes a descriptor number and disconnects it from the server.
 *
 * @see pdescrboot
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
prim_descr_boot(PRIM_PROTOTYPE)
{
    /* int --  */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Primitive is a wizbit only command.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pdescrboot(oper1->data.number);

    if (!result)
        abort_interp("Invalid descriptor number. (1)");

    CLEAR(oper1);
}

/**
 * Implementation of MUF DESCRNOTIFY
 *
 * Consumes a descriptor number and a string, and sends the string over
 * the descriptor's connection.
 *
 * @see pdescrnotify
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
prim_descr_notify(PRIM_PROTOTYPE)
{
    /* int string --  */
    CHECKOP(2);
    oper2 = POP();              /* string */
    oper1 = POP();              /* int */

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Argument not an string. (2)");

    if (oper2->data.string) {
        result = pdescrnotify(oper1->data.number, oper2->data.string->data);
    } else {
        /*
         * If the string was empty, we don't send it. But we still need to
         * check if the descriptor was valid.
         */
        result = descrdata_by_descr(oper1->data.number) != NULL;
    }

    if (!result)
        abort_interp("Invalid descriptor number. (1)");

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF NEXTDESCR
 *
 * Consumes a descriptor number and returns the next descriptor number that
 * is connected to the game beyond the welcome screen.  This value will be 0
 * if there are no more descriptors or the input is invalid.
 *
 * @see pnextdescr
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
prim_nextdescr(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pnextdescr(oper1->data.number);

    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRIPTORS
 *
 * Consumes a dbref and returns a stackrange of descriptor numbers associated
 * with the player.  If the dbref is NOTHING, all players' descriptors are
 * included.
 *
 * @see pdescrcount
 * @see pdescr
 * @see get_player_descrs
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
prim_descriptors(PRIM_PROTOTYPE)
{
    int mycount = 0;
    int *darr;
    int dcount;
    struct descriptor_data* d;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->data.objref != NOTHING && !valid_player(oper1))
        abort_interp("Argument must be a player dbref or NOTHING.");

    ref = oper1->data.objref;

    CLEAR(oper1);

    if (ref == NOTHING) {
        d = descriptor_list_tail;
        result = pdescrcount();

        CHECKOFLOW(result + 1);

        for ( ; d; d = d->prev) {
            if (d->connected) {
                PushInt(d->descriptor);
                mycount++;
            }
        }
    } else {
        darr = get_player_descrs(ref, &dcount);

        CHECKOFLOW(dcount + 1);

        for (int di = 0; di < dcount; di++) {
            PushInt(darr[di]);
            mycount++;
        }
    }

    PushInt(mycount);
}

/**
 * Implementation of MUF DESCR_ARRAY
 *
 * Consumes a dbref and returns an array of descriptor numbers associated
 * with the player.  If the dbref is NOTHING, all players' descriptors are
 * included.
 *
 * @see pdescrcount
 * @see pdescr
 * @see get_player_descrs
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
prim_descr_array(PRIM_PROTOTYPE)
{
    stk_array *newarr;
    int *darr;
    int dcount;
    struct descriptor_data* d;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->data.objref != NOTHING && !valid_player(oper1))
        abort_interp("Argument must be a player dbref or NOTHING.");

    ref = oper1->data.objref;

    CLEAR(oper1);

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_INTEGER;

    if (ref == NOTHING) {
        result = 0;

        for (d = descriptor_list; d; d = d->next) {
            if (d->connected)
                result++;
        }

        newarr = new_array_packed(result, fr->pinning);

        d = descriptor_list;

        for (int i = 0; d; d = d->next) {
            if (d->connected) {
                temp1.data.number = i;
                temp2.data.number = d->descriptor;

                array_setitem(&newarr, &temp1, &temp2);
                i++;
            }
        }
    } else {
        darr = get_player_descrs(ref, &dcount);
        newarr = new_array_packed(dcount, fr->pinning);

        for (int di = 0; di < dcount; di++) {
            temp1.data.number = (dcount - 1) - di;
            temp2.data.number = darr[di];

            array_setitem(&newarr, &temp1, &temp2);
        }
    }
    PushArrayRaw(newarr);
}

/**
 * Implementation of MUF DESCR_SETUSER
 *
 * Consumes a descriptor, a player dbref, and password - and reconnects
 * the descriptor to the player with the given credentials.  Providing
 * a dbref of NOTHING drops the connection withbout the usual messages.
 *
 * @see pset_user
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
prim_descr_setuser(PRIM_PROTOTYPE)
{
    char *ptr;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Requires Wizbit.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected. (1)");

    if (oper2->data.objref != NOTHING && !valid_player(oper2))
        abort_interp("Argument must be a player dbref or NOTHING.");

    ref = oper2->data.objref;

    if (oper3->type != PROG_STRING)
        abort_interp("Password string expected.");

    if (ref != NOTHING) {
        ptr = oper3->data.string ? oper3->data.string->data : NULL;

        if (!check_password(ref, ptr))
            abort_interp("Incorrect password.");

        log_status("DESCR_SETUSER: %s(%d) to %s(%d) on descriptor %d",
                   NAME(player), player, NAME(ref), ref, oper1->data.number);
    }

    tmp = oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    result = pset_user(tmp, ref);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRFLUSH
 *
 * Consumes a descriptor number and processes its output text.  If the input
 * is -1, output text for all descriptors is flushed.
 *
 * @see pdescrflush
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
prim_descrflush(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3 or better.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected.");

    tmp = oper1->data.number;

    CLEAR(oper1);

    result = pdescrflush(tmp);
}

/**
 * Implementation of MUF FIRSTDESCR
 *
 * Consumes a dbref and returns the first descriptor number associated with
 * that player, or all players if the input is NOTHING.  The return value will
 * be 0 if there are no descriptors available in the context.
 *
 * @see pfirstdescr
 * @see get_player_descrs
 * @see index_descr
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
prim_firstdescr(PRIM_PROTOTYPE)
{
    /* ref -- int */
    int *darr;
    int dcount;

    CHECKOP(1);

    oper1 = POP();
    if (mlev < 3)
        abort_interp("Requires Mucker Level 3.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");

    ref = oper1->data.objref;

    if (ref != NOTHING && !valid_player(oper1))
        abort_interp("Player dbref expected (2)");

    if (ref == NOTHING) {
        result = pfirstdescr();
    } else {
        if (PLAYER_DESCRCOUNT(ref)) {
            darr = get_player_descrs(ref, &dcount);
            result = index_descr(darr[dcount - 1]);
        } else {
            result = 0;
        }
    }

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF LASTDESCR
 *
 * Consumes a dbref and returns the last descriptor number associated with
 * that player, or all players if the input is NOTHING.  The return value will
 * be 0 if there are no descriptors available in the context.
 *
 * @see plastdescr
 * @see get_player_descrs
 * @see index_descr
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
prim_lastdescr(PRIM_PROTOTYPE)
{
    /* ref -- int */
    int *darr;
    int dcount;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");

    ref = oper1->data.objref;

    if (ref != NOTHING && !valid_player(oper1))
        abort_interp("Player dbref expected (2)");

    if (ref == NOTHING) {
        result = plastdescr();
    } else {
        if (Typeof(ref) != TYPE_PLAYER)
            abort_interp("invalid argument");

        if (PLAYER_DESCRCOUNT(ref)) {
            darr = get_player_descrs(ref, &dcount);
            result = index_descr(darr[0]);
        } else {
            result = 0;
        }
    }

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRSECURE?
 *
 * Consumes a descriptor number and returns 1 if it is secure, otherwise 0.
 *
 * @see pdescrsecure
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
prim_descr_securep(PRIM_PROTOTYPE)
{
    /* descr -> bool */

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected.");

    result = pdescrsecure(oper1->data.number);

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRBUFSIZE
 *
 * Consumes a descriptor number and returns the number of bytes available.
 *
 * @see pdescrbufsize
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
prim_descr_bufsize(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pdescrbufsize(oper1->data.number);

    if (result < 0)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF SETWIDTH
 *
 * Consumes a descriptor number and screensize.  Returns nothing.
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
prim_setwidth(PRIM_PROTOTYPE)
{
    struct descriptor_data* d;

    /* int int -- */
    CHECKOP(2);
    oper1 = POP(); /* size */
    oper2 = POP(); /* descriptor */

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (2)");

    if ((oper1->data.number < 0) || (oper1->data.number > USHRT_MAX))
        abort_interp("Width must be between 0 and 65535.");

    d = descrdata_by_descr(oper2->data.number);

    if (!d)
        abort_interp("Invalid descriptor number (2)");

    d->detected_width = (int)oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF SETHEIGHT
 *
 * Consumes a descriptor number and screensize.  Returns nothing.
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
prim_setheight(PRIM_PROTOTYPE)
{
    struct descriptor_data* d;

    /* int int -- */
    CHECKOP(2);
    oper1 = POP(); /* size */
    oper2 = POP(); /* descriptor */

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (2)");

    if ((oper1->data.number < 0) || (oper1->data.number > USHRT_MAX))
        abort_interp("Height must be between 0 and 65535.");

    d = descrdata_by_descr(oper2->data.number);

    if (!d)
        abort_interp("Invalid descriptor number (2)");

    d->detected_height = (int)oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF WIDTH
 *
 * Consumes a descriptor.  Puts the detected width on the stack.  This width
 * maybe 0 if it is unknown.
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
prim_width(PRIM_PROTOTYPE)
{
    const struct descriptor_data* d;

    /* int -- int */
    CHECKOP(1);
    oper1 = POP(); /* descriptor */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    d = descrdata_by_descr(oper1->data.number);

    if (!d)
        abort_interp("Invalid descriptor number (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    /* Convert short int to int */
    result = d->detected_width; 

    PushInt(result);
}

/**
 * Implementation of MUF HEIGHT
 *
 * Consumes a descriptor.  Puts the detected height on the stack.  This height
 * maybe 0 if it is unknown.
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
prim_height(PRIM_PROTOTYPE)
{
    const struct descriptor_data* d;

    /* int -- int */
    CHECKOP(1);
    oper1 = POP(); /* descriptor */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    d = descrdata_by_descr(oper1->data.number);

    if (!d)
        abort_interp("Invalid descriptor number (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    /* Convert short int to int */
    result = d->detected_height; 

    PushInt(result);
}
