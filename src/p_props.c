#include "config.h"

#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "externs.h"
#include "interp.h"
#include "msgparse.h"
#include "mpi.h"
#include "params.h"
#include "props.h"
#include "tune.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int result;
static dbref ref;
static char buf[BUFFER_LEN];

int
prop_read_perms(dbref player, dbref obj, const char *name, int mlev)
{
    if (Prop_System(name))
	return 0;
    if ((mlev < 3) && Prop_Private(name) && !permissions(player, obj))
	return 0;
    if ((mlev < 4) && Prop_Hidden(name))
	return 0;
    return 1;
}

int
prop_write_perms(dbref player, dbref obj, const char *name, int mlev)
{
    if (Prop_System(name))
	return 0;

    if (mlev < 3) {
	if (!permissions(player, obj)) {
	    if (Prop_Private(name))
		return 0;
	    if (Prop_ReadOnly(name))
		return 0;
	    if (!string_compare(name, tp_gender_prop))
		return 0;
	}
	if (string_prefix(name, "_msgmacs/"))
	    return 0;
    }
    if (mlev < 4) {
	if (Prop_SeeOnly(name))
	    return 0;
	if (Prop_Hidden(name))
	    return 0;
    }
    return 1;
}


void
prim_getpropval(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);

    if (!prop_read_perms(ProgUID, oper2->data.objref, oper1->data.string->data, mlev))
	abort_interp("Permission denied.");

    {
	char type[BUFFER_LEN];
	int len = oper1->data.string->length;

	strcpyn(type, sizeof(type), oper1->data.string->data);
	while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
	    type[len] = '\0';
	}
	result = get_property_value(oper2->data.objref, type);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROPVAL: o=%d n=\"%s\" v=%d",
		 program, pc->line, oper2->data.objref, type, result);
#endif

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	   ts_lastuseobject(oper2->data.objref); */
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_getpropfval(PRIM_PROTOTYPE)
{
    double fresult;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);

    if (!prop_read_perms(ProgUID, oper2->data.objref, oper1->data.string->data, mlev))
	abort_interp("Permission denied.");

    {
	char type[BUFFER_LEN];
	int len = oper1->data.string->length;

	strcpyn(type, sizeof(type), oper1->data.string->data);
	while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
	    type[len] = '\0';
	}
	fresult = get_property_fvalue(oper2->data.objref, type);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROPFVAL: o=%d n=\"%s\" v=%d",
		 program, pc->line, oper2->data.objref, type, result);
#endif

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	   ts_lastuseobject(oper2->data.objref); */
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}


void
prim_getprop(PRIM_PROTOTYPE)
{
    const char *temp;
    PropPtr prptr;
    dbref obj2;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char type[BUFFER_LEN];
	int len = oper1->data.string->length;

	if (!prop_read_perms(ProgUID, oper2->data.objref, oper1->data.string->data, mlev))
	    abort_interp("Permission denied.");

	strcpyn(type, sizeof(type), oper1->data.string->data);
	while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
	    type[len] = '\0';
	}

	obj2 = oper2->data.objref;
	prptr = get_property(obj2, type);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROP: o=%d n=\"%s\"",
		 program, pc->line, oper2->data.objref, type);
#endif

	CLEAR(oper1);
	CLEAR(oper2);
	if (prptr) {
#ifdef DISKBASE
	    propfetch(obj2, prptr);
#endif
	    switch (PropType(prptr)) {
	    case PROP_STRTYP:
		temp = PropDataStr(prptr);
		PushString(temp);
		break;
	    case PROP_LOKTYP:
		if (PropFlags(prptr) & PROP_ISUNLOADED) {
		    PushLock(TRUE_BOOLEXP);
		} else {
		    PushLock(PropDataLok(prptr));
		}
		break;
	    case PROP_REFTYP:
		PushObject(PropDataRef(prptr));
		break;
	    case PROP_INTTYP:
		PushInt(PropDataVal(prptr));
		break;
	    case PROP_FLTTYP:
		PushFloat(PropDataFVal(prptr));
		break;
	    default:
		result = 0;
		PushInt(result);
		break;
	    }
	} else {
	    result = 0;
	    PushInt(result);
	}

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	   ts_lastuseobject(oper2->data.objref); */
    }
}


void
prim_getpropstr(PRIM_PROTOTYPE)
{
    const char *temp;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char type[BUFFER_LEN];
	PropPtr ptr;
	int len = oper1->data.string->length;

	if (!prop_read_perms(ProgUID, oper2->data.objref, oper1->data.string->data, mlev))
	    abort_interp("Permission denied.");

	strcpyn(type, sizeof(type), oper1->data.string->data);
	while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
	    type[len] = '\0';
	}

	ptr = get_property(oper2->data.objref, type);
	if (!ptr) {
	    temp = "";
	} else {
#ifdef DISKBASE
	    propfetch(oper2->data.objref, ptr);
#endif
	    switch (PropType(ptr)) {
	    case PROP_STRTYP:
		temp = PropDataStr(ptr);
		break;
		/*
		 *case PROP_INTTYP:
		 *    snprintf(buf, sizeof(buf), "%d", PropDataVal(ptr));
		 *    temp = buf;
		 *    break;
		 */
	    case PROP_REFTYP:
		snprintf(buf, sizeof(buf), "#%d", PropDataRef(ptr));
		temp = buf;
		break;
	    case PROP_LOKTYP:
		if (PropFlags(ptr) & PROP_ISUNLOADED) {
		    temp = "*UNLOCKED*";
		} else {
		    temp = unparse_boolexp(ProgUID, PropDataLok(ptr), 1);
		}
		break;
	    default:
		temp = "";
		break;
	    }
	}

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROPSTR: o=%d n=\"%s\" s=\"%s\"",
		 program, pc->line, oper2->data.objref, type, temp);
#endif

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	 *     ts_lastuseobject(oper2->data.objref); */
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(temp);
}


void
prim_remove_prop(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    strncpy(buf, DoNullInd(oper1->data.string), BUFFER_LEN);
    buf[BUFFER_LEN - 1] = '\0';

    {
	int len = strlen(buf);
	char *ptr = buf + len;

	while ((--len >= 0) && (*--ptr == PROPDIR_DELIMITER))
	    *ptr = '\0';
    }

    if (!*buf)
	abort_interp("Can't remove root propdir (2)");

    if (!prop_write_perms(ProgUID, oper2->data.objref, buf, mlev))
	abort_interp("Permission denied.");

    remove_property(oper2->data.objref, buf, 0);

#ifdef LOG_PROPS
    log2file("props.log", "#%d (%d) REMOVEPROP: o=%d n=\"%s\"",
	     program, pc->line, oper2->data.objref, buf);
#endif

    ts_modifyobject(oper2->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
}


void
prim_envprop(PRIM_PROTOTYPE)
{
    double fresult;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char tname[BUFFER_LEN];
	dbref what;
	PropPtr ptr;
	int len = oper1->data.string->length;

	strcpyn(tname, sizeof(tname), oper1->data.string->data);
	while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	    tname[len] = '\0';
	}

	what = oper2->data.objref;
	ptr = envprop(&what, tname, 0);
	if (what != NOTHING) {
	    if (!prop_read_perms(ProgUID, what, oper1->data.string->data, mlev))
		abort_interp("Permission denied.");
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(what);

	if (!ptr) {
	    result = 0;
	    PushInt(result);
	} else {
#ifdef DISKBASE
	    propfetch(what, ptr);
#endif
	    switch (PropType(ptr)) {
	    case PROP_STRTYP:
		PushString(PropDataStr(ptr));
		break;
	    case PROP_INTTYP:
		result = PropDataVal(ptr);
		PushInt(result);
		break;
	    case PROP_FLTTYP:
		fresult = PropDataFVal(ptr);
		PushFloat(fresult);
		break;
	    case PROP_REFTYP:
		ref = PropDataRef(ptr);
		PushObject(ref);
		break;
	    case PROP_LOKTYP:
		if (PropFlags(ptr) & PROP_ISUNLOADED) {
		    PushLock(TRUE_BOOLEXP);
		} else {
		    PushLock(PropDataLok(ptr));
		}
		break;
	    default:
		result = 0;
		PushInt(result);
		break;
	    }
	}
    }
}


void
prim_envpropstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char tname[BUFFER_LEN];
	dbref what;
	PropPtr ptr;
	const char *temp;
	int len = oper1->data.string->length;

	strcpyn(tname, sizeof(tname), oper1->data.string->data);
	while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	    tname[len] = '\0';
	}

	what = oper2->data.objref;
	ptr = envprop(&what, tname, 0);
	if (!ptr) {
	    temp = "";
	} else {
#ifdef DISKBASE
	    propfetch(what, ptr);
#endif
	    switch (PropType(ptr)) {
	    case PROP_STRTYP:
		temp = PropDataStr(ptr);
		break;
		/*
		 *case PROP_INTTYP:
		 *    snprintf(buf, sizeof(buf), "%d", PropDataVal(ptr));
		 *    temp = buf;
		 *    break;
		 */
	    case PROP_REFTYP:
		snprintf(buf, sizeof(buf), "#%d", PropDataRef(ptr));
		temp = buf;
		break;
	    case PROP_LOKTYP:
		if (PropFlags(ptr) & PROP_ISUNLOADED) {
		    temp = "*UNLOCKED*";
		} else {
		    temp = unparse_boolexp(ProgUID, PropDataLok(ptr), 1);
		}
		break;
	    default:
		temp = "";
		break;
	    }
	}

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) ENVPROPSTR: o=%d so=%d n=\"%s\" s=\"%s\"",
		 program, pc->line, what, oper2->data.objref, tname, temp);
#endif

	if (what != NOTHING) {
	    if (!prop_read_perms(ProgUID, what, oper1->data.string->data, mlev))
		abort_interp("Permission denied.");
	    /* if (Typeof(what) != TYPE_PLAYER)
	     *     ts_lastuseobject(what); */
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(what);
	PushString(temp);
    }
}



void
prim_blessprop(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper1))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper1->data.objref);

    if (mlev < 4)
	abort_interp("Permission denied.");

    {
	char *tmpe;
	char tname[BUFFER_LEN];
	int len = oper2->data.string->length;

	tmpe = oper2->data.string->data;
	while (*tmpe && *tmpe != '\r' && *tmpe != ':')
	    tmpe++;
	if (*tmpe)
	    abort_interp("Illegal propname");

	strcpyn(tname, sizeof(tname), oper2->data.string->data);
	while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	    tname[len] = '\0';
	}

	set_property_flags(oper1->data.objref, tname, PROP_BLESSED);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) BLESSPROP: o=%d n=\"%s\"",
		 program, pc->line, oper1->data.objref, tname);
#endif

	ts_modifyobject(oper1->data.objref);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}


void
prim_unblessprop(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper1))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper1->data.objref);

    if (mlev < 4)
	abort_interp("Permission denied.");

    {
	char *tmpe;
	char tname[BUFFER_LEN];
	int len = oper2->data.string->length;

	tmpe = oper2->data.string->data;
	while (*tmpe && *tmpe != '\r' && *tmpe != ':')
	    tmpe++;
	if (*tmpe)
	    abort_interp("Illegal propname");

	strcpyn(tname, sizeof(tname), oper2->data.string->data);
	while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	    tname[len] = '\0';
	}

	clear_property_flags(oper1->data.objref, tname, PROP_BLESSED);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) BLESSPROP: o=%d n=\"%s\"",
		 program, pc->line, oper1->data.objref, tname);
#endif

	ts_modifyobject(oper1->data.objref);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}


void
prim_setprop(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if ((oper1->type != PROG_STRING) &&
	(oper1->type != PROG_INTEGER) &&
	(oper1->type != PROG_LOCK) &&
	(oper1->type != PROG_OBJECT) && (oper1->type != PROG_FLOAT))
	abort_interp("Invalid argument type (3)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper3))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper3->data.objref);

    if ((mlev < 2) && (!permissions(ProgUID, oper3->data.objref)))
	abort_interp("Permission denied.");

    if (!prop_write_perms(ProgUID, oper3->data.objref, oper2->data.string->data, mlev))
	abort_interp("Permission denied.");

    {
	char *tmpe;
	char tname[BUFFER_LEN];
	PData propdat;
	int len = oper2->data.string->length;

	tmpe = oper2->data.string->data;
	while (*tmpe && *tmpe != '\r' && *tmpe != ':')
	    tmpe++;
	if (*tmpe)
	    abort_interp("Illegal propname");

	strcpyn(tname, sizeof(tname), oper2->data.string->data);
	while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	    tname[len] = '\0';
	}

	switch (oper1->type) {
	case PROG_STRING:
	    propdat.flags = PROP_STRTYP;
	    propdat.data.str = oper1->data.string ? oper1->data.string->data : 0;
	    break;
	case PROG_INTEGER:
	    propdat.flags = PROP_INTTYP;
	    propdat.data.val = oper1->data.number;
	    break;
	case PROG_FLOAT:
	    propdat.flags = PROP_FLTTYP;
	    propdat.data.fval = oper1->data.fnumber;
	    break;
	case PROG_OBJECT:
	    propdat.flags = PROP_REFTYP;
	    propdat.data.ref = oper1->data.objref;
	    break;
	case PROG_LOCK:
	    propdat.flags = PROP_LOKTYP;
	    propdat.data.lok = copy_bool(oper1->data.lock);
	    break;
	}
	set_property(oper3->data.objref, tname, &propdat, 0);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) SETPROP: o=%d n=\"%s\"",
		 program, pc->line, oper3->data.objref, tname);
#endif

	ts_modifyobject(oper3->data.objref);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}


void
prim_addprop(PRIM_PROTOTYPE)
{
    CHECKOP(4);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    oper4 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument (4)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (3)");
    if (oper3->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper3->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper4))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper4->data.objref);

    if ((mlev < 2) && (!permissions(ProgUID, oper4->data.objref)))
	abort_interp("Permission denied.");

    if (!prop_write_perms(ProgUID, oper4->data.objref, oper3->data.string->data, mlev))
	abort_interp("Permission denied.");

    {
	const char *temp;
	char *tmpe;
	char tname[BUFFER_LEN];
	int len = oper3->data.string->length;

	temp = (oper2->data.string ? oper2->data.string->data : 0);
	tmpe = oper3->data.string->data;
	while (*tmpe && *tmpe != '\r')
	    tmpe++;
	if (*tmpe)
	    abort_interp("CRs not allowed in propname");

	strcpyn(tname, sizeof(tname), oper3->data.string->data);
	while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	    tname[len] = '\0';
	}

	/* if ((temp) || (oper1->data.number)) */
	{
	    add_property(oper4->data.objref, tname, temp, oper1->data.number);

#ifdef LOG_PROPS
	    log2file("props.log", "#%d (%d) ADDPROP: o=%d n=\"%s\" s=\"%s\" v=%d",
		     program, pc->line, oper4->data.objref, tname, temp, oper1->data.number);
#endif

	    ts_modifyobject(oper4->data.objref);
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
}



void
prim_nextprop(PRIM_PROTOTYPE)
{
    /* dbref pname -- pname */
    char *pname;
    char exbuf[BUFFER_LEN];

    CHECKOP(2);
    oper2 = POP();		/* pname */
    oper1 = POP();		/* dbref */
    if (mlev < 3)
	abort_interp("Permission denied.");
    if (oper2->type != PROG_STRING)
	abort_interp("String required. (2)");
    if (oper1->type != PROG_OBJECT)
	abort_interp("Dbref required. (1)");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref. (1)");

    ref = oper1->data.objref;
    (void) strcpyn(buf, sizeof(buf), DoNullInd(oper2->data.string));

    CLEAR(oper1);
    CLEAR(oper2);

    {
	char *tmpname;

	pname = next_prop_name(ref, exbuf, sizeof(exbuf), buf);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) NEXTPROP: o=%d n=\"%s\" on=\"%s\"",
		 program, pc->line, ref, pname, buf);
#endif

	while (pname && !prop_read_perms(ProgUID, ref, pname, mlev)) {
	    tmpname = next_prop_name(ref, exbuf, sizeof(exbuf), pname);

#ifdef LOG_PROPS
	    log2file("props.log", "#%d (%d) NEXTPROP: o=%d n=\"%s\" on=\"%s\"",
		     program, pc->line, ref, tmpname, pname);
#endif

	    pname = tmpname;
	}
    }
    if (pname) {
	PushString(pname);
    } else {
	PushNullStr;
    }
}


void
prim_propdirp(PRIM_PROTOTYPE)
{
    /* dbref dir -- int */
    CHECKOP(2);
    oper2 = POP();		/* prop name */
    oper1 = POP();		/* dbref */
    if (mlev < 2)
	abort_interp("Permission denied.");
    if (oper1->type != PROG_OBJECT)
	abort_interp("Argument must be a dbref (1)");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not a string. (2)");
    if (!oper2->data.string)
	abort_interp("Null string not allowed. (2)");
    ref = oper1->data.objref;
    (void) strcpyn(buf, sizeof(buf), oper2->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);

    result = is_propdir(ref, buf);

#ifdef LOG_PROPS
    log2file("props.log", "#%d (%d) PROPDIR?: o=%d n=\"%s\" v=%d",
	     program, pc->line, ref, buf, result);
#endif

    PushInt(result);
}



void
prim_parseprop(PRIM_PROTOTYPE)
{
    const char *temp;
    char *ptr;
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper3 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper4 = NULL;	/* prevents re-entrancy issues! */

    char buf[BUFFER_LEN];
    char type[BUFFER_LEN];

    CHECKOP(4);
    oper4 = POP();		/* int */
    oper2 = POP();		/* arg str */
    oper1 = POP();		/* propname str */
    oper3 = POP();		/* object dbref */
    if (mlev < 3)
	abort_interp("Mucker level 3 or greater required.");
    if (oper3->type != PROG_OBJECT)
	abort_interp("Non-object argument. (1)");
    if (!valid_object(oper3))
	abort_interp("Invalid object. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("String expected. (3)");
    if (oper1->type != PROG_STRING)
	abort_interp("String expected. (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (oper4->type != PROG_INTEGER)
	abort_interp("Integer expected. (4)");
    if (oper4->data.number < 0 || oper4->data.number > 1)
	abort_interp("Integer of 0 or 1 expected. (4)");
    CHECKREMOTE(oper3->data.objref);
    {
	int len = oper1->data.string->length;

	if (!prop_read_perms(ProgUID, oper3->data.objref, oper1->data.string->data, mlev))
	    abort_interp("Permission denied.");

	if (mlev < 3 && !permissions(player, oper3->data.objref) &&
	    prop_write_perms(ProgUID, oper3->data.objref, oper1->data.string->data, mlev))
	    abort_interp("Permission denied.");

	strcpyn(type, sizeof(type), oper1->data.string->data);
	while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
	    type[len] = '\0';
	}

	temp = get_property_class(oper3->data.objref, type);
#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROPSTR: o=%d n=\"%s\" s=\"%s\"",
		 program, pc->line, oper3->data.objref, type, temp);
#endif

    }
    ptr = DoNullInd(oper2->data.string);
    if (temp) {
	result = 0;
	if (oper4->data.number)
	    result |= MPI_ISPRIVATE;
	if (Prop_Blessed(oper3->data.objref, type))
	    result |= MPI_ISBLESSED;
	ptr = do_parse_mesg(fr->descr, player, oper3->data.objref, temp, ptr, buf, sizeof(buf),
			    result);
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
	PushString(ptr);
    } else {
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
	PushNullStr;
    }
}



void
prim_array_filter_prop(PRIM_PROTOTYPE)
{
    char pattern[BUFFER_LEN];
    char tname[BUFFER_LEN];
    struct inst *in;
    struct inst temp1;
    stk_array *arr;
    stk_array *nu;
    char *prop;
    int len;

    CHECKOP(3);
    oper3 = POP();		/* str     pattern */
    oper2 = POP();		/* str     propname */
    oper1 = POP();		/* refarr  Array */
    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_OBJECT))
	abort_interp("Argument not an array of dbrefs. (1)");
    if (oper2->type != PROG_STRING || !oper2->data.string)
	abort_interp("Argument not a non-null string. (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Argument not a string pattern. (3)");

    len = oper2->data.string ? oper2->data.string->length : 0;
    strcpyn(tname, sizeof(tname), DoNullInd(oper2->data.string));
    while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	tname[len] = '\0';
    }

    nu = new_array_packed(0);
    arr = oper1->data.array;
    prop = tname;
    strcpyn(pattern, sizeof(pattern), DoNullInd(oper3->data.string));
    if (array_first(arr, &temp1)) {
	do {
	    in = array_getitem(arr, &temp1);
	    if (valid_object(in)) {
		ref = in->data.objref;
		CHECKREMOTE(ref);
		if (prop_read_perms(ProgUID, ref, prop, mlev)) {
		    PropPtr pptr = get_property(ref, prop);

		    if (pptr) {
			switch (PropType(pptr)) {
			case PROP_STRTYP:
			    strncpy(buf, PropDataStr(pptr), BUFFER_LEN);
			    break;

			case PROP_LOKTYP:
			    if (PropFlags(pptr) & PROP_ISUNLOADED) {
				strncpy(buf, "*UNLOCKED*", BUFFER_LEN);
			    } else {
				strncpy(buf, unparse_boolexp(ProgUID, PropDataLok(pptr), 0),
					BUFFER_LEN);
			    }
			    break;

			case PROP_REFTYP:
			    snprintf(buf, BUFFER_LEN, "#%i", PropDataRef(pptr));
			    break;

			case PROP_INTTYP:
			    snprintf(buf, BUFFER_LEN, "%i", PropDataVal(pptr));
			    break;

			case PROP_FLTTYP:
			    snprintf(buf, BUFFER_LEN, "%g", PropDataFVal(pptr));
			    break;

			default:
			    strncpy(buf, "", BUFFER_LEN);
			    break;
			}
		    } else
			strncpy(buf, "", BUFFER_LEN);

		    if (equalstr(pattern, buf)) {
			array_appenditem(&nu, in);
		    }
		}
	    }
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu);
}


void
prim_reflist_find(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();		/* dbref */
    oper2 = POP();		/* propname */
    oper1 = POP();		/* propobj */
    if (!valid_object(oper1))
	abort_interp("Non-object argument (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (oper3->type != PROG_OBJECT)
	abort_interp("Non-object argument (3)");
    if (!prop_read_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
	abort_interp("Permission denied.");
    CHECKREMOTE(oper1->data.objref);

    result = reflist_find(oper1->data.objref, oper2->data.string->data, oper3->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}


void
prim_reflist_add(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();		/* dbref */
    oper2 = POP();		/* propname */
    oper1 = POP();		/* propobj */
    if (!valid_object(oper1))
	abort_interp("Non-object argument (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (oper3->type != PROG_OBJECT)
	abort_interp("Non-object argument (3)");
    if (!prop_write_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
	abort_interp("Permission denied.");
    CHECKREMOTE(oper1->data.objref);

    reflist_add(oper1->data.objref, oper2->data.string->data, oper3->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}


void
prim_reflist_del(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();		/* dbref */
    oper2 = POP();		/* propname */
    oper1 = POP();		/* propobj */
    if (!valid_object(oper1))
	abort_interp("Non-object argument (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (oper3->type != PROG_OBJECT)
	abort_interp("Non-object argument (3)");
    if (!prop_write_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
	abort_interp("Permission denied.");
    CHECKREMOTE(oper1->data.objref);

    reflist_del(oper1->data.objref, oper2->data.string->data, oper3->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}


void
prim_blessedp(PRIM_PROTOTYPE)
{
    /* dbref prop -- int */
    CHECKOP(2);
    oper2 = POP();		/* prop name */
    oper1 = POP();		/* dbref */
    if (mlev < 2)
	abort_interp("Permission denied.");
    if (oper1->type != PROG_OBJECT)
	abort_interp("Argument must be a dbref (1)");
    if (!valid_object(oper1))
	abort_interp("Invalid dbref (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not a string. (2)");
    if (!oper2->data.string)
	abort_interp("Null string not allowed. (2)");
    ref = oper1->data.objref;
    (void) strcpyn(buf, sizeof(buf), oper2->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);

    result = Prop_Blessed(ref, buf) ? 1 : 0;

#ifdef LOG_PROPS
    log2file("props.log", "#%d (%d) BLESSED?: o=%d n=\"%s\" v=%d",
	     program, pc->line, ref, buf, result);
#endif

    PushInt(result);
}

void
prim_parsepropex(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;	/* prevents reentrancy issues! */
    struct inst *oper2 = NULL;	/* prevents reentrancy issues! */
    struct inst *oper3 = NULL;	/* prevents reentrancy issues! */
    struct inst *oper4 = NULL;	/* prevents reentrancy issues! */
    stk_array *vars;
    const char *mpi;
    char *str = 0;
    array_iter idx;
    int mvarcnt = 0;
    char *buffers = NULL;
    int novars;
    int hashow = 0;
    int i;
    int len;
    char tname[BUFFER_LEN];
    char buf[BUFFER_LEN];

    CHECKOP(4);

    oper4 = POP();		/* int:Private */
    oper3 = POP();		/* dict:Vars */
    oper2 = POP();		/* str:Prop */
    oper1 = POP();		/* ref:Object */

    if (mlev < 3)
	abort_interp("Mucker level 3 or greater required.");

    if (oper1->type != PROG_OBJECT)
	abort_interp("Non-object argument. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument. (2)");
    if (oper3->type != PROG_ARRAY)
	abort_interp("Non-array argument. (3)");
    if (oper3->data.array && (oper3->data.array->type != ARRAY_DICTIONARY))
	abort_interp("Dictionary array expected. (3)");
    if (oper4->type != PROG_INTEGER)
	abort_interp("Non-integer argument. (4)");

    if (!valid_object(oper1))
	abort_interp("Invalid object. (1)");
    if (!oper2->data.string)
	abort_interp("Empty string argument. (2)");
    if ((oper4->data.number != 0) && (oper4->data.number != 1))
	abort_interp("Integer of 0 or 1 expected. (4)");

    CHECKREMOTE(oper1->data.objref);

    if (!prop_read_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
	abort_interp("Permission denied.");

    len = oper2->data.string->length;
    strcpyn(tname, sizeof(tname), oper2->data.string->data);
    while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
	tname[len] = '\0';
    }

    mpi = get_property_class(oper1->data.objref, tname);
    vars = oper3->data.array;
    novars = array_count(vars);

    if (check_mvar_overflow(novars))
	abort_interp("Out of MPI variables. (3)");

    if (array_first(vars, &idx)) {
	do {
	    array_data *val = array_getitem(vars, &idx);

	    if (idx.type != PROG_STRING) {
		CLEAR(&idx);
		abort_interp("Only string keys supported. (3)");
	    }

	    if (idx.data.string == NULL) {
		CLEAR(&idx);
		abort_interp("Empty string keys not supported. (3)");
	    }

	    if (strlen(idx.data.string->data) > MAX_MFUN_NAME_LEN) {
		CLEAR(&idx);
		abort_interp("Key too long to be an MPI variable. (3)");
	    }

	    switch (val->type) {
	    case PROG_INTEGER:
	    case PROG_FLOAT:
	    case PROG_OBJECT:
	    case PROG_STRING:
	    case PROG_LOCK:
		break;

	    default:
		CLEAR(&idx);
		abort_interp
			("Only integer, float, dbref, string and lock values supported. (3)");
	    }

	    if (string_compare(idx.data.string->data, "how") == 0)
		hashow = 1;
	}
	while (array_next(vars, &idx));
    }

    if (mpi && *mpi) {
	if (novars > 0) {
	    mvarcnt = varc;

	    if ((buffers = (char *) malloc(novars * BUFFER_LEN)) == NULL)
		abort_interp("Out of memory.");

	    if (array_first(vars, &idx)) {
		i = 0;

		do {
		    char *var_buf = buffers + (i++ * BUFFER_LEN);
		    array_data *val;

		    val = array_getitem(vars, &idx);

		    switch (val->type) {
		    case PROG_INTEGER:
			snprintf(var_buf, BUFFER_LEN, "%i", val->data.number);
			break;

		    case PROG_FLOAT:
			snprintf(var_buf, BUFFER_LEN, "%g", val->data.fnumber);
			break;

		    case PROG_OBJECT:
			snprintf(var_buf, BUFFER_LEN, "#%i", val->data.objref);
			break;

		    case PROG_STRING:
			strncpy(var_buf, DoNullInd(val->data.string), BUFFER_LEN);
			break;

		    case PROG_LOCK:
			strncpy(var_buf, unparse_boolexp(ProgUID, val->data.lock, 1),
				BUFFER_LEN);
			break;

		    default:
			var_buf[0] = '\0';
			break;
		    }

		    var_buf[BUFFER_LEN - 1] = '\0';

		    new_mvar(idx.data.string->data, var_buf);
		}
		while (array_next(vars, &idx));
	    }
	}

	result = 0;

	if (oper4->data.number)
	    result |= MPI_ISPRIVATE;

	if (Prop_Blessed(oper1->data.objref, oper2->data.string->data))
	    result |= MPI_ISBLESSED;

	if (hashow)
	    result |= MPI_NOHOW;

	str = do_parse_mesg(fr->descr, player, oper1->data.objref, mpi, "(parsepropex)", buf,
			    sizeof(buf), result);

	if (novars > 0) {
	    if (array_first(vars, &idx)) {
		i = 0;

		do {
		    char *var_buf = buffers + (i++ * BUFFER_LEN);
		    struct inst temp;

		    temp.type = PROG_STRING;
		    temp.data.string = alloc_prog_string(var_buf);

		    array_setitem(&vars, &idx, &temp);

		    CLEAR(&temp);
		}
		while (array_next(vars, &idx));
	    }

	    free(buffers);

	    varc = mvarcnt;
	}
    }

    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);

    PushArrayRaw(vars);
    PushString(str);
}
