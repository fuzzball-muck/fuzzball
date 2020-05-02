/** @file p_props.c
 *
 * Implementation of property related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interp.h"
#include "mpi.h"
#include "props.h"
#include "tune.h"

/*
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
static int result;

/**
 * @private
 * @var I don't really have any rationale for this one :)  Just makes things
 *      not threadsafe for fun.
 */
static dbref ref;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

/**
 * Checks prop reading permissions
 *
 * For a given player, object, prop name, and effective MUCKER level,
 * returns true if the given prop can be read by the given player, false
 * otherwise.
 *
 * Note that regardless of MUCKER level, this will always return false
 * for props that Prop_System return true with.
 *
 * @see Prop_System
 * @see Prop_Private
 * @see permissions
 * @see Prop_Hidden
 *
 * @param player the player to check permissions for
 * @param obj the object the property is on
 * @param name the name of the property
 * @param mlev the effective MUCKER level
 * @return boolean true if player can read the prop, false otherwise
 */
int
prop_read_perms(dbref player, dbref obj, const char *name, int mlev)
{
    if (!*name)
        return 0;

    if (Prop_System(name))
        return 0;

    if ((mlev < 3) && Prop_Private(name) && !permissions(player, obj))
        return 0;

    if ((mlev < 4) && Prop_Hidden(name))
        return 0;

    return 1;
}

/**
 * Checks prop writing permissions
 *
 * For a given player, object, prop name, and effective MUCKER level,
 * returns true if the given prop can be written to by the given player, false
 * otherwise.
 *
 * Note that regardless of MUCKER level, this will always return false
 * for props that Prop_System return true with.
 *
 * @see Prop_System
 * @see Prop_Private
 * @see permissions
 * @see Prop_Hidden
 * @see Prop_Private
 * @see Prop_ReadOnly
 * @see Prop_SeeOnly
 *
 * @param player the player to check permissions for
 * @param obj the object the property is on
 * @param name the name of the property
 * @param mlev the effective MUCKER level
 * @return boolean true if player can write the prop, false otherwise
 */
int
prop_write_perms(dbref player, dbref obj, const char *name, int mlev)
{
    if (!*name)
        return 0;

    if (Prop_System(name))
        return 0;

    if (mlev < 3) {
        if (!permissions(player, obj)) {
            if (Prop_Private(name))
                return 0;

            if (Prop_ReadOnly(name))
                return 0;

            if (!strcasecmp(name, tp_gender_prop))
                return 0;
        }

        if (string_prefix(name, MPI_MACROS_PROPDIR))
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

/**
 * Implementation of MUF GETPROPVAL
 *
 * Consumes a dbref and a string property and returns an integer value
 * associated with that property.  If the property is unset, 0 is returned.
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
prim_getpropval(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper2))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    char type[BUFFER_LEN];
    strcpyn(type, sizeof(type), DoNullInd(oper1->data.string));

    if (!prop_read_perms(ProgUID, oper2->data.objref, type, mlev))
        abort_interp("Permission denied.");

    {
        int len = oper1->data.string ? oper1->data.string->length : 0;

        /* Trim off trailing slashs */
        while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
            type[len] = '\0';
        }

        result = get_property_value(oper2->data.objref, type);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF GETPROPFVAL
 *
 * Consumes a dbref and a string property and returns a float value
 * associated with that property.  If the property is unset, 0.0 is returned.
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
prim_getpropfval(PRIM_PROTOTYPE)
{
    double fresult;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper2))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    char type[BUFFER_LEN];
    strcpyn(type, sizeof(type), DoNullInd(oper1->data.string));

    if (!prop_read_perms(ProgUID, oper2->data.objref, type, mlev))
        abort_interp("Permission denied.");

    {
        int len = oper1->data.string ? oper1->data.string->length : 0;

        /* Remove trailing slashes */
        while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
            type[len] = '\0';
        }

        fresult = get_property_fvalue(oper2->data.objref, type);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}

/**
 * Implementation of MUF GETPROP
 *
 * Consumes a dbref and a string property and returns whatever value
 * of whatever type it is onto the stack.  0 is put on the stack if
 * the property is unset or is a propdir with no value
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

    if (!valid_object(oper2))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    char type[BUFFER_LEN];
    strcpyn(type, sizeof(type), DoNullInd(oper1->data.string));

    if (!prop_read_perms(ProgUID, oper2->data.objref, type, mlev))
        abort_interp("Permission denied.");

    {
        int len = oper1->data.string ? oper1->data.string->length : 0;

        /* Trim trailing slashes */
        while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
            type[len] = '\0';
        }

        obj2 = oper2->data.objref;
        prptr = get_property(obj2, type);

        CLEAR(oper1);
        CLEAR(oper2);

        if (prptr) { /* Did we find a value? */
#ifdef DISKBASE
            propfetch(obj2, prptr);
#endif
            switch (PropType(prptr)) {
                /* Convert it to the correct MUF type, or push 0 if unhandled */
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
            }
        } else {
            result = 0;
            PushInt(result);
        }
    }
}

/**
 * Implementation of MUF GETPROPSTR
 *
 * Consumes a dbref and a string property and returns the string value
 * of the property or an empty string if the property is unset.
 *
 * If the property is a ref, it will be converted to a #1234 style dbref.
 * If the property is a lock, it will be unparsed.
 *
 * Floats and ints will return empty string.
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
prim_getpropstr(PRIM_PROTOTYPE)
{
    const char *temp;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper2))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    char type[BUFFER_LEN];
    strcpyn(type, sizeof(type), DoNullInd(oper1->data.string));

    {
        int len = oper1->data.string ? oper1->data.string->length : 0;
        PropPtr ptr;

        /* Trim trailing slashes */
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
            /*
             * Convert locks and refs to strings.  This had a commented
             * out block for converting ints to strings; not sure why that
             * was presumably added then later removed.  Floats were never
             * handled by this.
             */
            switch (PropType(ptr)) {
                case PROP_STRTYP:
                    temp = PropDataStr(ptr);
                    break;

                case PROP_REFTYP:
                    snprintf(buf, sizeof(buf), "#%d", PropDataRef(ptr));
                    temp = buf;
                    break;

                case PROP_LOKTYP:
                    if (PropFlags(ptr) & PROP_ISUNLOADED) {
                        temp = PROP_UNLOCKED_VAL;
                    } else {
                        temp = unparse_boolexp(ProgUID, PropDataLok(ptr), 1);
                    }

                    break;

                default:
                    temp = "";
                    break;
            }
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushString(temp);
}

/**
 * Implementation of MUF REMOVE_PROP
 *
 * Consumes a dbref and a string property and removes the given property
 * if the user is permitted to do so.
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

        /*
         * TODO: This remove trailing delimiter code is featured in basically
         *       all of these functions.  I think either the underlying
         *       suite of property functions should do this, or we should
         *       have a common static function here, but this code
         *       duplication is silly.
         */
        while ((--len >= 0) && (*--ptr == PROPDIR_DELIMITER))
            *ptr = '\0';
    }

    if (!*buf)
        abort_interp("Can't remove root propdir (2)");

    if (!prop_write_perms(ProgUID, oper2->data.objref, buf, mlev))
        abort_interp("Permission denied.");

    remove_property(oper2->data.objref, buf, 0);

    ts_modifyobject(oper2->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF ENVPROP
 *
 * Consumes a dbref and a string property and searches down the environment
 * tree starting from the given object for a property with the given name.
 * If the property is not found, #-1 and 0 are pushed on the stack.  If it
 * is found, the dbref of the object the property was found on and the
 * property value are pushed on the stack.  The value could be any type.
 *
 * @see envprop
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
prim_envprop(PRIM_PROTOTYPE)
{
    double fresult;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper2))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    {
        char tname[BUFFER_LEN];
        dbref what;
        PropPtr ptr;
        int len = oper1->data.string ? oper1->data.string->length : 0;

        strcpyn(tname, sizeof(tname), DoNullInd(oper1->data.string));

        /* Once again trimming the trailing slashes -- copypasta code */
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

            /*
             * TODO: This code is duplicate with getprop -- converting a C
             *       data type to a MUF thing.  In fact, I feel like I've seen
             *       this block elsewhere in the code as well.
             *
             *       At the very least, I think we could do a static function
             *       that pushes the content of a prop pointer onto the stack.
             *       Maybe instead of making it static, we make it available
             *       in the header as this could be useful elsewhere.
             */
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

/**
 * Implementation of MUF ENVPROPSTR
 *
 * Consumes a dbref and a string property and searches down the environment
 * tree starting from the given object for a property with the given name.
 * If the property is not found, #-1 and empty string are pushed on the stack.
 * If it is found, the dbref of the object the property was found on and the
 * string value are pushed on the stack.
 *
 * The logic for how strings are returned is the same as for GETPROPSTR
 *
 * @see envprop
 * @see prim_getpropstr
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
prim_envpropstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper2))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper2->data.objref);

    {
        char tname[BUFFER_LEN];
        dbref what;
        PropPtr ptr;
        const char *temp;
        int len = oper1->data.string ? oper1->data.string->length : 0;

        strcpyn(tname, sizeof(tname), DoNullInd(oper1->data.string));

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
            /*
             * TODO: This code is identical to the code in prim_getpropstr.
             *       Thus, they should share the same underlying function;
             *       move this to a static, or maybe even refactor the guts
             *       of these calls so one uses envprop and the other uses
             *       whatever prop fetch function it uses.
             */
            switch (PropType(ptr)) {
                case PROP_STRTYP:
                    temp = PropDataStr(ptr);
                    break;

                case PROP_REFTYP:
                    snprintf(buf, sizeof(buf), "#%d", PropDataRef(ptr));
                    temp = buf;
                    break;

                case PROP_LOKTYP:
                    if (PropFlags(ptr) & PROP_ISUNLOADED) {
                        temp = PROP_UNLOCKED_VAL;
                    } else {
                        temp = unparse_boolexp(ProgUID, PropDataLok(ptr), 1);
                    }

                    break;

                default:
                    temp = "";
            }
        }

        if (what != NOTHING) {
            if (!prop_read_perms(ProgUID, what, oper1->data.string->data, mlev))
                abort_interp("Permission denied.");
        }

        CLEAR(oper1);
        CLEAR(oper2);
        PushObject(what);
        PushString(temp);
    }
}

/**
 * Implementation of MUF BLESSPROP
 *
 * Consumes a dbref and a string property and "blesses" the given property
 * which is basically giving wizard permissions to an MPI prop.  Requires
 * wizard permissions.
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
prim_blessprop(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper1))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper1->data.objref);

    if (mlev < 4)
        abort_interp("Permission denied.");

    if (force_level)
        abort_interp("Cannot be forced.");

    {
        char *tmpe;
        char tname[BUFFER_LEN];
        int len = oper2->data.string ? oper2->data.string->length : 0;

        tmpe = DoNullInd(oper2->data.string);

        /* Check for carriage returns because I guess that's a problem! */
        while (*tmpe && *tmpe != '\r' && *tmpe != PROP_DELIMITER)
            tmpe++;

        if (*tmpe)
            abort_interp("Illegal propname");

        strcpyn(tname, sizeof(tname), DoNullInd(oper2->data.string));

        /* Yet another implementation of removing trailing slashes */
        while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
            tname[len] = '\0';
        }

        set_property_flags(oper1->data.objref, tname, PROP_BLESSED);

        ts_modifyobject(oper1->data.objref);
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF UNBLESSPROP
 *
 * Consumes a dbref and a string property and removes the blessed bit.
 * Requires wizard permissions.
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
prim_unblessprop(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
 
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (!valid_object(oper1))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper1->data.objref);

    if (mlev < 4)
        abort_interp("Permission denied.");

    if (force_level)
        abort_interp("Cannot be forced.");

    {
        char *tmpe;
        char tname[BUFFER_LEN];
        int len = oper2->data.string ? oper2->data.string->length : 0;

        tmpe = DoNullInd(oper2->data.string);

        while (*tmpe && *tmpe != '\r' && *tmpe != PROP_DELIMITER)
            tmpe++;

        if (*tmpe)
            abort_interp("Illegal propname");

        strcpyn(tname, sizeof(tname), DoNullInd(oper2->data.string));

        while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
            tname[len] = '\0';
        }

        clear_property_flags(oper1->data.objref, tname, PROP_BLESSED);

        ts_modifyobject(oper1->data.objref);
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF SETPROP
 *
 * Consumes a dbref, a string property, and a value which may be of type
 * string, integer, lock, object (dbref), or float.  Tries to set the
 * prop assuming the user has permissions to do so.
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

    if (!valid_object(oper3))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper3->data.objref);

    if ((mlev < 2) && (!permissions(ProgUID, oper3->data.objref)))
        abort_interp("Permission denied.");

    char tname[BUFFER_LEN];
    strcpyn(tname, sizeof(tname), DoNullInd(oper2->data.string));
    
    if (!prop_write_perms(ProgUID, oper3->data.objref, tname, mlev))
        abort_interp("Permission denied.");

    {
        PData propdat;
        int len = oper2->data.string ? oper2->data.string->length : 0;

        if (!is_valid_propname(tname))
            abort_interp("Illegal propname");

        while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
            tname[len] = '\0';
        }

        /*
         * TODO: A function that converts MUF types to property pointers
         *       would probably be a good idea.  I'm not sure we need it
         *       again in this file, but it just seems like a logical thing
         *       to have as I'm sure there are other cases for it.
         */
        switch (oper1->type) {
            case PROG_STRING:
                propdat.flags = PROP_STRTYP;
                propdat.data.str = DoNullInd(oper1->data.string);
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

        ts_modifyobject(oper3->data.objref);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

/**
 * Implementation of MUF ADDPROP
 *
 * Consumes a dbref, a property string, a potential value string, and a
 * potential value number.  If the string is empty, then the number is set
 * as a numeric prop; otherwise, the string is always used.
 *
 * Respects permissions like the other prop setting ones.
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

    if (!valid_object(oper4))
        abort_interp("Non-object argument (1)");

    CHECKREMOTE(oper4->data.objref);

    if ((mlev < 2) && (!permissions(ProgUID, oper4->data.objref)))
        abort_interp("Permission denied.");

    char tname[BUFFER_LEN];
    strcpyn(tname, sizeof(tname), DoNullInd(oper3->data.string));

    if (!prop_write_perms(ProgUID, oper4->data.objref, tname, mlev))
        abort_interp("Permission denied.");

    {
        const char *temp;
        char *tmpe;
        int len = oper3->data.string ? oper3->data.string->length : 0;

        temp = DoNullInd(oper2->data.string);
        tmpe = tname;

        /*
         * TODO: This check for carriage returns is nearly identical to
         *       setprop's... might be a few other places it shows up.
         *       Another good logic consolidation bit.
         */
        while (*tmpe && *tmpe != '\r')
            tmpe++;

        if (*tmpe)
            abort_interp("CRs not allowed in propname");

        /* Yet another implementation of trimming trailing slashes */
        while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
            tname[len] = '\0';
        }

        add_property(oper4->data.objref, tname, temp, oper1->data.number);
        ts_modifyobject(oper4->data.objref);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
}

/**
 * Implementation of MUF NEXTPROP
 *
 * Consumes a dbref and a property string and returns the name of the next
 * readable property, or empty string if there is no next property.
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
prim_nextprop(PRIM_PROTOTYPE)
{
    /* dbref pname -- pname */
    char *pname;
    char exbuf[BUFFER_LEN];

    CHECKOP(2);
    oper2 = POP();  /* pname */
    oper1 = POP();  /* dbref */

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

        while (pname && !prop_read_perms(ProgUID, ref, pname, mlev)) {
            tmpname = next_prop_name(ref, exbuf, sizeof(exbuf), pname);

            pname = tmpname;
        }
    }

    if (pname) {
        PushString(pname);
    } else {
        PushNullStr;
    }
}

/**
 * Implementation of MUF PROPDIR?
 *
 * Consumes a dbref and a property string and returns boolean true if
 * the property is a propdir, assuming the user can read it.
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
prim_propdirp(PRIM_PROTOTYPE)
{
    /* dbref dir -- int */
    CHECKOP(2);

    oper2 = POP();  /* prop name */
    oper1 = POP();  /* dbref */

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

    if (!prop_read_perms(ProgUID, ref, buf, mlev))
        abort_interp("Permission denied.");

    CLEAR(oper1);
    CLEAR(oper2);

    result = is_propdir(ref, buf);

    PushInt(result);
}

/**
 * Implementation of MUF PARSEMPI
 *
 * Consumes a dbref, an MPI string to parse, a string input for &how, and
 * a boolean - true for delay messages to be sent to player only, false for
 * everyone in the room.  Requires MUCKER 3.  Pushes results of parser onto
 * the stack.
 *
 * @see do_parse_message
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
prim_parsempi(PRIM_PROTOTYPE)
{
    const char *temp;
    char *ptr;
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper3 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper4 = NULL;  /* prevents re-entrancy issues! */

    char buf[BUFFER_LEN];

    CHECKOP(4);
    oper4 = POP();  /* int */
    oper2 = POP();  /* arg str */
    oper1 = POP();  /* mpi str */
    oper3 = POP();  /* object dbref */

    if (mlev < 3)
        abort_interp("Permission denied.");

    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");

    if (!OkObj(oper3->data.objref))
        abort_interp("Invalid object (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("String expected (3)");

    if (oper1->type != PROG_STRING)
        abort_interp("String expected (2)");

    if (oper4->type != PROG_INTEGER)
        abort_interp("Integer expected (4)");

    if (oper4->data.number < 0 || oper4->data.number > 1)
        abort_interp("Integer of 0 or 1 expected (4)");

    CHECKREMOTE(oper3->data.objref);

    temp = DoNullInd(oper1->data.string);
    ptr  = DoNullInd(oper2->data.string);

    if (temp && *temp && ptr) {
        result = 0;

        if (oper4->data.number)
            result |= MPI_ISPRIVATE;

        ptr = do_parse_mesg(fr->descr, player, oper3->data.objref, temp,
                              ptr, buf, sizeof(buf), result);
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

/**
 * Implementation of MUF PARSEMPIBLESSED
 *
 * @see prim_parsempi
 *
 * Same as parse MPI but runs it as if it was blessed.  Requires wizard
 * permissions.
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
prim_parsempiblessed(PRIM_PROTOTYPE)
{
    const char *temp;
    char *ptr;
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper3 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper4 = NULL;  /* prevents re-entrancy issues! */

    char buf[BUFFER_LEN];

    /*
     * TODO: The guts of this are copy/paste from parsempi.  We should
     *       probably make these prims thin wrappers around the same
     *       static base code.
     */

    CHECKOP(4);
    oper4 = POP();  /* int */
    oper2 = POP();  /* arg str */
    oper1 = POP();  /* mpi str */
    oper3 = POP();  /* object dbref */

    if (mlev < 4)
        abort_interp("Permission denied.");

    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");

    if (!OkObj(oper3->data.objref))
        abort_interp("Invalid object (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("String expected (3)");

    if (oper1->type != PROG_STRING)
        abort_interp("String expected (2)");

    if (oper4->type != PROG_INTEGER)
        abort_interp("Integer expected (4)");

    if (oper4->data.number < 0 || oper4->data.number > 1)
        abort_interp("Integer of 0 or 1 expected (4)");

    CHECKREMOTE(oper3->data.objref);

    temp = DoNullInd(oper1->data.string);
    ptr  = DoNullInd(oper2->data.string);

    if (temp && *temp && ptr) {
        result = 0;

        if (oper4->data.number)
            result |= MPI_ISPRIVATE;

        result |= MPI_ISBLESSED;
        ptr = do_parse_mesg(fr->descr, player, oper3->data.objref, temp,
                            ptr, buf, sizeof(buf), result);
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

/**
 * Implementation of MUF PARSEPROP
 *
 * Consumes a dbref, a property name, a string input for &how, and
 * a boolean - true for delay messages to be sent to player only, false for
 * everyone in the room.  Requires MUCKER 3.  Pushes results of parser onto
 * the stack.  Runs whatever property's MPI, with whatever blessed state the
 * property has, assuming the caller has permission to see the property.
 *
 * @see do_parse_message
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
prim_parseprop(PRIM_PROTOTYPE)
{
    const char *temp;
    char *ptr;
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper3 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper4 = NULL;  /* prevents re-entrancy issues! */

    char buf[BUFFER_LEN];
    char type[BUFFER_LEN];

    CHECKOP(4);
    oper4 = POP();  /* int */
    oper2 = POP();  /* arg str */
    oper1 = POP();  /* propname str */
    oper3 = POP();  /* object dbref */

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

    if (oper4->type != PROG_INTEGER)
        abort_interp("Integer expected. (4)");

    if (oper4->data.number < 0 || oper4->data.number > 1)
        abort_interp("Integer of 0 or 1 expected. (4)");

    CHECKREMOTE(oper3->data.objref);

    {
        strcpyn(type, sizeof(type), DoNullInd(oper1->data.string));
        int len = oper1->data.string ? oper1->data.string->length : 0;

        if (!prop_read_perms(ProgUID, oper3->data.objref, type, mlev))
            abort_interp("Permission denied.");

        if (mlev < 3 && !permissions(player, oper3->data.objref) &&
            prop_write_perms(ProgUID, oper3->data.objref, type, mlev))
            abort_interp("Permission denied.");

        while (len-- > 0 && type[len] == PROPDIR_DELIMITER) {
            type[len] = '\0';
        }

        temp = get_property_class(oper3->data.objref, type);
    }

    ptr = DoNullInd(oper2->data.string);

    if (temp) {
        result = 0;

        if (oper4->data.number)
            result |= MPI_ISPRIVATE;

        if (Prop_Blessed(oper3->data.objref, type))
            result |= MPI_ISBLESSED;

        ptr = do_parse_mesg(fr->descr, player, oper3->data.objref, temp, ptr,
                            buf, sizeof(buf), result);
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

/**
 * Implementation of MUF ARRAY_FILTER_PROP
 *
 * Consimes a list array of just dbrefs, a property name, and a property
 * value smatch string.  Returns a list array with only thos dbrefs that have
 * that property set to a value that matches the given smatch pattern.
 *
 * When the property is loaded, it will be converted to a string, and this
 * works as you would expect from an integer or float.  DBREFs start with #,
 * and locks are unparsed before string comparison.
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
    oper3 = POP();  /* str     pattern */
    oper2 = POP();  /* str     propname */
    oper1 = POP();  /* refarr  Array */

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

    /* Yet another implementation of the delimiter trim */
    while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
        tname[len] = '\0';
    }

    nu = new_array_packed(0, fr->pinning);
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
                                    strncpy(buf, PROP_UNLOCKED_VAL, BUFFER_LEN);
                                } else {
                                    strncpy(buf,
                                            unparse_boolexp(ProgUID,
                                                            PropDataLok(pptr),
                                                            0),
                                            BUFFER_LEN);
                                }

                                break;

                            case PROP_REFTYP:
                                snprintf(buf, BUFFER_LEN, "#%i",
                                         PropDataRef(pptr));
                                break;

                            case PROP_INTTYP:
                                snprintf(buf, BUFFER_LEN, "%i",
                                         PropDataVal(pptr));
                                break;

                            case PROP_FLTTYP:
                                snprintf(buf, BUFFER_LEN, "%g",
                                         PropDataFVal(pptr));
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

/**
 * Implementation of MUF REFLIST_FIND
 *
 * Consumes an object ref, a property string that contains a reflist,
 * and a dbref to search for.  Returns the position in the reflist that the
 * ref was found, starting with '1' being the first entry.  0 is not-found.
 *
 * @see reflist_find
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
prim_reflist_find(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();  /* dbref */
    oper2 = POP();  /* propname */
    oper1 = POP();  /* propobj */

    if (!valid_object(oper1))
        abort_interp("Non-object argument (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument (3)");

    char *b = DoNullInd(oper2->data.string);

    if (!prop_read_perms(ProgUID, oper1->data.objref, b, mlev))
        abort_interp("Permission denied.");

    CHECKREMOTE(oper1->data.objref);

    result = reflist_find(oper1->data.objref, b, oper3->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

/**
 * Implementation of MUF REFLIST_ADD
 *
 * Consumes an object ref, a property string that contains a reflist,
 * and a dbref to add to the reflist. Adds the ref to the end of the
 * list if it is already on the list.
 *
 * @see reflist_add
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
prim_reflist_add(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();  /* dbref */
    oper2 = POP();  /* propname */
    oper1 = POP();  /* propobj */

    if (!valid_object(oper1))
        abort_interp("Non-object argument (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument (3)");

    char *b = DoNullInd(oper2->data.string);

    if (!prop_write_perms(ProgUID, oper1->data.objref, b, mlev))
        abort_interp("Permission denied.");

    CHECKREMOTE(oper1->data.objref);

    reflist_add(oper1->data.objref, b, oper3->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

/**
 * Implementation of MUF REFLIST_DEL
 *
 * Consumes an object ref, a property string that contains a reflist,
 * and a dbref to delete from the reflist.  Removes the ref if it is
 * on the list, does nothing otherwise.
 *
 * @see reflist_del
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
prim_reflist_del(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();  /* dbref */
    oper2 = POP();  /* propname */
    oper1 = POP();  /* propobj */

    if (!valid_object(oper1))
        abort_interp("Non-object argument (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");

    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument (3)");

    char *b = DoNullInd(oper2->data.string);

    if (!prop_write_perms(ProgUID, oper1->data.objref, b, mlev))
        abort_interp("Permission denied.");

    CHECKREMOTE(oper1->data.objref);

    reflist_del(oper1->data.objref, b, oper3->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

/**
 * Implementation of MUF BLESSED?
 *
 * Consumes an object ref and a property string.  Returns boolean true if
 * the property is blessed, false otherwise.
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
prim_blessedp(PRIM_PROTOTYPE)
{
    /* dbref prop -- int */
    CHECKOP(2);
    oper2 = POP();  /* prop name */
    oper1 = POP();  /* dbref */

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

    if (!prop_read_perms(ProgUID, ref, buf, mlev))
        abort_interp("Permission denied.");

    CLEAR(oper1);
    CLEAR(oper2);

    result = Prop_Blessed(ref, buf) ? 1 : 0;

    PushInt(result);
}

/**
 * Implementation of MUF PARSEPROPEX
 *
 * Consumes a dbref, a property name, a dictionary of variables, and
 * a boolean - true for delay messages to be sent to player only, false for
 * everyone in the room.  Requires MUCKER 3.  Pushes a dictionary of variables
 * and the results of parser onto the stack.  Runs whatever property's MPI,
 * with whatever blessed state the property has, assuming the caller has
 * permission to see the property.
 *
 * @see do_parse_message
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
prim_parsepropex(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;  /* prevents reentrancy issues! */
    struct inst *oper2 = NULL;  /* prevents reentrancy issues! */
    struct inst *oper3 = NULL;  /* prevents reentrancy issues! */
    struct inst *oper4 = NULL;  /* prevents reentrancy issues! */
    stk_array *vars;
    const char *mpi;
    char *str = 0;
    array_iter idx;
    int mvarcnt = 0;
    char *buffers = NULL;
    size_t novars;
    int hashow = 0;
    int i;
    int len;
    char tname[BUFFER_LEN];
    char buf[BUFFER_LEN];

    CHECKOP(4);

    oper4 = POP();  /* int:Private */
    oper3 = POP();  /* dict:Vars */
    oper2 = POP();  /* str:Prop */
    oper1 = POP();  /* ref:Object */

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

    if ((oper4->data.number != 0) && (oper4->data.number != 1))
        abort_interp("Integer of 0 or 1 expected. (4)");

    CHECKREMOTE(oper1->data.objref);

    strcpyn(tname, sizeof(tname), DoNullInd(oper2->data.string));
    len = oper2->data.string ? oper2->data.string->length : 0;

    if (!prop_read_perms(ProgUID, oper1->data.objref, tname, mlev))
        abort_interp("Permission denied.");

    /* Once again trimming off trailing slashes */
    while (len-- > 0 && tname[len] == PROPDIR_DELIMITER) {
        tname[len] = '\0';
    }

    mpi = get_property_class(oper1->data.objref, tname);
    vars = oper3->data.array;
    novars = (size_t)array_count(vars);

    if (check_mvar_overflow(novars))
        abort_interp("Out of MPI variables. (3)");

    /* Convert the array into MPI variables -- validate first */
    if (array_first(vars, &idx)) {
        do {
            array_data *val = array_getitem(vars, &idx);

            /*
             * TODO: These MPI variable validations should be centralized
             *       so that MUF and MPI don't accidentally have separate
             *       rules.
             */
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

            /* Make sure it is a valid type */
            switch (val->type) {
                case PROG_INTEGER:
                case PROG_FLOAT:
                case PROG_OBJECT:
                case PROG_STRING:
                case PROG_LOCK:
                    break;

                default:
                    CLEAR(&idx);
                    abort_interp(
                        "Only integer, float, dbref, string and lock values "
                        "supported. (3)"
                    );
            }

            if (strcasecmp(idx.data.string->data, "how") == 0)
                hashow = 1;
        }

        while (array_next(vars, &idx));
    }

    if (mpi && *mpi) {
        if (novars > 0) {
            mvarcnt = varc;

            if ((buffers = malloc(novars * BUFFER_LEN)) == NULL)
                abort_interp("Out of memory.");

            /* Set up the variables */
            if (array_first(vars, &idx)) {
                i = 0;

                do {
                    char *var_buf = buffers + (i++ * BUFFER_LEN);
                    array_data *val;

                    val = array_getitem(vars, &idx);

                    switch (val->type) {
                        case PROG_INTEGER:
                            snprintf(var_buf, BUFFER_LEN, "%i",
                                     val->data.number);
                            break;

                        case PROG_FLOAT:
                            snprintf(var_buf, BUFFER_LEN, "%g",
                                     val->data.fnumber);
                            break;

                        case PROG_OBJECT:
                            snprintf(var_buf, BUFFER_LEN, "#%i",
                                     val->data.objref);
                            break;

                        case PROG_STRING:
                            strncpy(var_buf, DoNullInd(val->data.string),
                                    BUFFER_LEN);
                            break;

                        case PROG_LOCK:
                            strncpy(var_buf, unparse_boolexp(ProgUID,
                                    val->data.lock, 1), BUFFER_LEN);
                            break;

                        default:
                            var_buf[0] = '\0';
                            break;
                    }

                    var_buf[BUFFER_LEN - 1] = '\0';
                    new_mvar(idx.data.string->data, var_buf);
                } while (array_next(vars, &idx));
            }
        }

        result = 0;

        if (oper4->data.number)
            result |= MPI_ISPRIVATE;

        if (Prop_Blessed(oper1->data.objref, tname))
            result |= MPI_ISBLESSED;

        if (hashow)
            result |= MPI_NOHOW;

        str = do_parse_mesg(fr->descr, player, oper1->data.objref, mpi,
                            "(parsepropex)", buf, sizeof(buf), result);

        /* Get result variables */
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
                } while (array_next(vars, &idx));
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

/**
 * Implementation of MUF PROP_NAME_OK?
 *
 * Consumes a string prop and pushes on a boolean true/false.  If true,
 * the string prop name is a valid prop name, otherwise it is not.
 *
 * @see is_valid_propname
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
prim_prop_name_okp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Property name string expected.");

    result = is_valid_propname(DoNullInd(oper1->data.string));
    
    CLEAR(oper1);
    PushInt(result);
}
