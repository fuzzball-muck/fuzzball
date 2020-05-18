/** @file p_props.h
 *
 * Declaration of property related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_PROPS_H
#define P_PROPS_H

#include "interp.h"

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
int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);

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
int prop_write_perms(dbref player, dbref obj, const char *name, int mlev);

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
void prim_getpropval(PRIM_PROTOTYPE);

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
void prim_getpropfval(PRIM_PROTOTYPE);

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
void prim_getpropstr(PRIM_PROTOTYPE);

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
void prim_remove_prop(PRIM_PROTOTYPE);

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
void prim_envprop(PRIM_PROTOTYPE);

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
void prim_envpropstr(PRIM_PROTOTYPE);

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
void prim_addprop(PRIM_PROTOTYPE);

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
void prim_nextprop(PRIM_PROTOTYPE);

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
void prim_propdirp(PRIM_PROTOTYPE);

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
void prim_parseprop(PRIM_PROTOTYPE);

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
void prim_getprop(PRIM_PROTOTYPE);

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
void prim_setprop(PRIM_PROTOTYPE);

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
void prim_blessprop(PRIM_PROTOTYPE);

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
void prim_unblessprop(PRIM_PROTOTYPE);


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
void prim_array_filter_prop(PRIM_PROTOTYPE);

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
void prim_reflist_find(PRIM_PROTOTYPE);

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
void prim_reflist_add(PRIM_PROTOTYPE);

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
void prim_reflist_del(PRIM_PROTOTYPE);

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
void prim_blessedp(PRIM_PROTOTYPE);

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
void prim_parsepropex(PRIM_PROTOTYPE);

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
void prim_parsempi(PRIM_PROTOTYPE);

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
void prim_parsempiblessed(PRIM_PROTOTYPE);

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
void prim_prop_name_okp(PRIM_PROTOTYPE);

/**
 * Primitive callback functions
 */
#define PRIMS_PROPS_FUNCS prim_getpropval, prim_getpropstr, prim_remove_prop, \
   prim_envprop, prim_envpropstr, prim_addprop, prim_nextprop, prim_propdirp, \
   prim_parseprop, prim_getprop, prim_setprop, prim_getpropfval, \
   prim_blessprop, prim_unblessprop, prim_array_filter_prop, \
   prim_reflist_find, prim_reflist_add, prim_reflist_del, prim_blessedp, \
   prim_parsepropex, prim_parsempi, prim_parsempiblessed, prim_prop_name_okp

/**
 * Primitive names - must be in same order as the callback functions
 */
#define PRIMS_PROPS_NAMES "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP",  \
    "ENVPROP", "ENVPROPSTR", "ADDPROP", "NEXTPROP", "PROPDIR?", \
    "PARSEPROP", "GETPROP", "SETPROP", "GETPROPFVAL", \
    "BLESSPROP", "UNBLESSPROP", "ARRAY_FILTER_PROP", \
    "REFLIST_FIND", "REFLIST_ADD", "REFLIST_DEL", "BLESSED?", \
    "PARSEPROPEX", "PARSEMPI", "PARSEMPIBLESSED", "PROP-NAME-OK?"

#endif /* !P_PROPS_H */
