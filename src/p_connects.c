#include <stddef.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "player.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2;
static int tmp, result;
static dbref ref;

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

void
prim_online(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    if (mlev < 3)
	abort_interp("Mucker level 3 primitive.");
    result = pcount();
    CHECKOFLOW(result + 1);
    while (result) {
	ref = pdbref(result--);
	PushObject(ref);
    }
    result = pcount();
    PushInt(result);
}

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

void
prim_concount(PRIM_PROTOTYPE)
{
    /* -- int */
    CHECKOP(0);
    result = pcount();
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_descr(PRIM_PROTOTYPE)
{
    /* -- int */
    CHECKOP(0);
    result = fr->descr;
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_condbref(PRIM_PROTOTYPE)
{
    /* int -- dbref */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < 3)
	abort_interp("Mucker level 3 primitive.");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer. (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number. (1)");
    result = pdbref(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushObject(result);
}

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

void
prim_connotify(PRIM_PROTOTYPE)
{
    /* int string --  */
    CHECKOP(2);
    oper2 = POP();		/* string */
    oper1 = POP();		/* int */
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

void
prim_descr_notify(PRIM_PROTOTYPE)
{
    /* int string --  */
    CHECKOP(2);
    oper2 = POP();		/* string */
    oper1 = POP();		/* int */
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
    result = oper1->data.number;
    result = pdescrcon(result);
    CLEAR(oper1);
    PushInt(result);
}

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
    result = oper1->data.number;
    result = pnextdescr(result);
    CLEAR(oper1);
    PushInt(result);
}

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
    if (oper1->type != PROG_OBJECT)
	abort_interp("Argument not a dbref.");
    if (oper1->data.objref != NOTHING && !valid_object(oper1))
	abort_interp("Bad dbref.");
    ref = oper1->data.objref;
    if ((ref != NOTHING) && (!valid_player(oper1)))
	abort_interp("Non-player argument.");
    CLEAR(oper1);
    CHECKOP(0);

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
    if (oper1->type != PROG_OBJECT)
	abort_interp("Argument not a dbref.");
    if (oper1->data.objref != NOTHING && !valid_object(oper1))
	abort_interp("Bad dbref.");
    ref = oper1->data.objref;
    if ((ref != NOTHING) && (!valid_player(oper1)))
	abort_interp("Non-player argument.");

    CLEAR(oper1);

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_INTEGER;
    temp1.line = 0;
    temp2.line = 0;
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
    if (oper2->type != PROG_OBJECT)
	abort_interp("Player dbref expected. (2)");
    ref = oper2->data.objref;
    if (ref != NOTHING && !valid_player(oper2))
	abort_interp("Player dbref expected. (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Password string expected.");
    ptr = oper3->data.string ? oper3->data.string->data : NULL;
    if (ref != NOTHING) {
	if (!check_password(ref, ptr))
	    abort_interp("Incorrect password.");
    }

    if (ref != NOTHING) {
	const char *destname = "*NOBODY*";
	if (ref != NOTHING) {
	    destname = NAME(ref);
	}
	log_status("DESCR_SETUSER: %s(%d) to %s(%d) on descriptor %d",
		   NAME(player), player, destname, ref, oper1->data.number);
    }
    tmp = oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    result = pset_user(tmp, ref);

    PushInt(result);
}

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
	if (Typeof(ref) != TYPE_PLAYER)
	    abort_interp("invalid argument");
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
    PushInt(result);
}

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
