/** @file p_connects.c
 *
 * Implementation for handling MUF connection primitives.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
#include <stddef.h>

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
 * @see pcount
 * @see pdbref
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
    CHECKOP(0);

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    result = pcount();

    CHECKOFLOW(result+1);

    for (int i = result; i > 0; i--) {
        ref = pdbref(i);
        PushObject(ref);
    }

    PushInt(result);
}

/**
 * Implementation of MUF ONLINE_ARRAY
 *
 * Returns an array of dbrefs representing the players associated with
 * each connection to the server.
 *
 * @see pcount
 * @see pdbref
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
    stk_array *nu;

    CHECKOP(0);
 
    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    result = pcount();

    CHECKOFLOW(1);

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_OBJECT;

    temp1.line = 0;
    temp2.line = 0;

    nu = new_array_packed(result, fr->pinning);

    for (int i = 0; i < result; i++) {
        temp1.data.number = i;
        temp2.data.number = pdbref(i + 1);

        array_setitem(&nu, &temp1, &temp2);
    }

    PushArrayRaw(nu);
}

/**
 * Implementation of MUF CONCOUNT
 *
 * Returns the number of connections to the server.
 *
 * @see pcount
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
prim_concount(PRIM_PROTOTYPE)
{
    /* -- int */
    CHECKOP(0);

    result = pcount();

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
 * Implementation of MUF CONDBREF
 *
 * Consumes a connection number and returns the associated player dbref.
 * This will be NOTHING if no player is connected using that number.
 *
 * @see pdbref
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
prim_condbref(PRIM_PROTOTYPE)
{
    /* int -- dbref */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER || oper1->data.number < 1)
        abort_interp("Argument not a positive integer. (1)");

    result = pdbref(oper1->data.number);

    if (result == NOTHING) {
        abort_interp("Invalid connection number. (1)");
    }
    
    CHECKOFLOW(1);
    CLEAR(oper1);

    PushObject(result);
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
 * Implementation of MUF CONIDLE
 *
 * Consumes a connection number and returns how many seconds it has been idle.
 *
 * @see pidle
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
prim_conidle(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = oper1->data.number;

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    result = pidle(result);

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
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
 * Consumes a dbref and returns the least idle descriptor number.
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

    result = pdescr(least_idle_player_descr(oper1->data.objref));

    if (result == 0)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRMOSTIDLE
 *
 * Consumes a descriptor number and returns how many seconds it has been idle.
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

    result = pdescr(most_idle_player_descr(oper1->data.objref));

    if (result == 0)
        abort_interp("Invalid descriptor number. (1)");

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF CONTIME
 *
 * Consumes a connection number and returns how many seconds it has been
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
prim_contime(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = oper1->data.number;

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    result = pontime(result);

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
 * Implementation of MUF CONHOST
 *
 * Consumes a connection number and returns the hostname or IP associated with
 * the connection.
 *
 * @see phost
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
prim_conhost(PRIM_PROTOTYPE)
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

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    pname = phost(result);

    CHECKOFLOW(1);
    CLEAR(oper1);

    PushString(pname);
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
 * Implementation of MUF CONUSER
 *
 * Consumes a connection number and returns the username associated with
 * the connection.
 *
 * @see puser
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
prim_conuser(PRIM_PROTOTYPE)
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

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    pname = puser(result);

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
 * Implementation of MUF CONBOOT
 *
 * Consumes a connection number and disconnects it from the server.
 *
 * @see pboot
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
prim_conboot(PRIM_PROTOTYPE)
{
    /* int --  */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 4)
        abort_interp("Primitive is a wizbit only command.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = oper1->data.number;

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    CLEAR(oper1);

    pboot(result);
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
 * Implementation of MUF CONNOTIFY
 *
 * Consumes a connection number and a string, and sends the string over
 * the connection.
 *
 * @see pnotify
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
prim_connotify(PRIM_PROTOTYPE)
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

    result = oper1->data.number;

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    if (oper2->data.string)
        pnotify(result, oper2->data.string->data);

    CLEAR(oper1);
    CLEAR(oper2);
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

    if (oper2->data.string)
        result = pdescrnotify(oper1->data.number, oper2->data.string->data);

    if (!result)
        abort_interp("Invalid descriptor number. (1)");

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF CONDESCR
 *
 * Consumes a connection number and returns the associated descriptor number.
 *
 * @see pdescr
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
prim_condescr(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = oper1->data.number;

    if ((result < 1) || (result > pcount()))
        abort_interp("Invalid connection number. (1)");

    result = pdescr(result);

    CLEAR(oper1);

    PushInt(result);
}

/**
 * Implementation of MUF DESCRCON
 *
 * Consumes a descriptor number and returns the associated connection number.
 *
 * @see pdescrcon
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
prim_descrcon(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (1)");

    result = pdescrcon(oper1->data.number);

    CLEAR(oper1);

    PushInt(result);
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
 * @see pcount
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
    int mydescr, mycount = 0;
    int *darr;
    int dcount;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Mucker level 3 primitive.");

    if (oper1->data.objref != NOTHING && !valid_player(oper1))
        abort_interp("Argument must be a player dbref or NOTHING.");

    ref = oper1->data.objref;

    CLEAR(oper1);

    if (ref == NOTHING) {
        result = pcount();

        CHECKOFLOW(result + 1);

        while (result) {
            mydescr = pdescr(result);

            PushInt(mydescr);
            mycount++;
            result--;
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
 * @see pcount
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
        result = pcount();
        newarr = new_array_packed(result, fr->pinning);

        for (int i = 0; i < result; i++) {
            temp1.data.number = i;
            temp2.data.number = pdescr(i + 1);

            array_setitem(&newarr, &temp1, &temp2);
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
