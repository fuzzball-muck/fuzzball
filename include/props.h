/** @file props.h
 *
 * Header for common database property data along with function declarations
 * for most property primitives.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef PROPS_H
#define PROPS_H

#include <stdio.h>

#include "config.h"
#include "fbmuck.h"

/*
 * Property data union to support all available property types.
 *
 * String, lock, integer, double, or DB Reference number.
 *
 * I'm not sure what "pos" is, it does not appear to be used anywhere
 * as this union is accessed pretty exclusively with the SetPData* defines
 * below which does not have one for "pos".  Grepping the code didn't reveal
 * any uses, either.
 *
 * Perhaps it is there to ensure a minimum size of 4 bytes for the union.
 */
union pdata_u {
    char *str;
    struct boolexp *lok;
    int val;
    double fval;
    dbref ref;
    long pos;
};

/* data struct for setting data. */
struct pdata {
    unsigned short flags;
    union pdata_u data;
};
typedef struct pdata PData;

/* Property struct */
struct plist {
    unsigned short flags;
    short height;		/* satisfy the avl monster.  */
    union pdata_u data;
    struct plist *left, *right, *dir;
    char key[1];
};

/* property node pointer type */
typedef struct plist *PropPtr;

/* propload queue types */
#define PROPS_UNLOADED 0x0
#define PROPS_LOADED   0x1
#define PROPS_PRIORITY 0x2
#define PROPS_CHANGED  0x3

/* property value types */
#define PROP_DIRTYP   0x0
#define PROP_STRTYP   0x2
#define PROP_INTTYP   0x3
#define PROP_LOKTYP   0x4
#define PROP_REFTYP   0x5
#define PROP_FLTTYP   0x6
#define PROP_TYPMASK  0x7

/* Internally used prop flags.  Never stored on disk. */
#define PROP_ISUNLOADED  0x0200
#define PROP_TOUCHED     0x0400
#define PROP_DIRUNLOADED 0x0800

#define PROP_UNLOCKED_VAL	"*UNLOCKED*"

/* Blessed props evaluate with wizbit MPI perms. */
#define PROP_BLESSED     0x1000

/* You will never want to change these, or you will make your MUCK
 * incompatible with pretty much everything.
 */
#define EXEC_SIGNAL '@'
#define PROP_DELIMITER ':'
#define PROPDIR_DELIMITER '/'

/* You should never use the underlying property structure directly.  The
 * preferred method is to use these defines; theoretically, the underlying
 * structure could change in some fundamental way in the future, but this
 * API will be stable.
 */

/* Set and get a prop's directory */
#define SetPDir(x,y) {(x)->dir = y;}
#define PropDir(x) ((x)->dir)

/* These set the different kinds of property values.
 *
 * SetPDataUnion is used to copy a PData over without understanding the
 * underlying type, and is used (at the time of this writing) exclusively
 * by set_property_nofetch.  You would generally not want to use that one.
 */
#define SetPDataUnion(x,z) {(x)->data = z;}
#define SetPDataStr(x,z) {(x)->data.str = z;}   /* String         */
#define SetPDataVal(x,z) {(x)->data.val = z;}   /* integer        */
#define SetPDataRef(x,z) {(x)->data.ref = z;}   /* DBREF          */
#define SetPDataLok(x,z) {(x)->data.lok = z;}   /* Lock           */
#define SetPDataFVal(x,z) {(x)->data.fval = z;} /* Floating Point */

/* These are the getters that correspond to the setters above */
#define PropDataStr(x) ((x)->data.str)      /* String         */
#define PropDataVal(x) ((x)->data.val)      /* Integer        */
#define PropDataRef(x) ((x)->data.ref)      /* DBREF          */
#define PropDataLok(x) ((x)->data.lok)      /* Lock           */
#define PropDataFVal(x) ((x)->data.fval)    /* Floating Point */

/* Get prop name */
#define PropName(x) ((x)->key)

/* Set and get property flags */
#define SetPFlags(x,y) {(x)->flags = ((x)->flags & PROP_TYPMASK) | (unsigned short)y;}
#define PropFlags(x) ((x)->flags & ~PROP_TYPMASK)

/* Set and get property type */
#define SetPType(x,y) {(x)->flags = ((x)->flags & ~PROP_TYPMASK) | (unsigned short)y;}
#define PropType(x) ((x)->flags & PROP_TYPMASK)

/* Set and get the raw flags (unsigned short integer) */
#define SetPFlagsRaw(x,y) {(x)->flags = (unsigned short)y;}
#define PropFlagsRaw(x) ((x)->flags)

/* Check if property is blessed */
#define Prop_Blessed(obj,propname) (get_property_flags(obj, propname) & PROP_BLESSED)

/* property access macros
 *
 * If you change any of these, your MUCK will be drastically incompatible
 * with pretty much everything.
 */

#define PROP_RDONLY '_'
#define PROP_RDONLY2 '%'
#define PROP_PRIVATE '.'
#define PROP_HIDDEN '@'
#define PROP_SEEONLY '~'

/* Macros to check property access types */
#define Prop_ReadOnly(name) \
    (Prop_Check(name, PROP_RDONLY) || Prop_Check(name, PROP_RDONLY2))
#define Prop_Private(name) Prop_Check(name, PROP_PRIVATE)
#define Prop_SeeOnly(name) Prop_Check(name, PROP_SEEONLY)
#define Prop_Hidden(name) Prop_Check(name, PROP_HIDDEN)
#define Prop_System(name) is_prop_prefix(name, "@__sys__")

/**
 * Check each segment of the property path and see if 'what' is the first
 * character of the segment.
 *
 * It is recommended you use the property check macro's instead:
 * Prop_ReadOnly, Prop_Private, Prop_SeeOnly, Prop_Hidden, Prop_System
 *
 * @param name Property name string
 * @param what Single character
 * @return true if 'what' appears in any path segment.
 */
int Prop_Check(const char *name, const char what);

/**
 * Adds a new property to an object without diskbase handling.  It is
 * safer to use the add_property method, however this is exposed because
 * it used in both db.c and property.c
 *
 * This is a wrapper around set_property_nofetch, so if you need to
 * set values more complex than string or integer, use set_property_nofetch
 * instead.
 *
 * @internal
 * @param player Object to set property on
 * @param pname Property name
 * @param strval String value - pass NULL if you want an integer value
 * @param value Integer value - ignored if String value is used.
 */
void add_prop_nofetch(dbref player, const char *pname, const char *strval,
                      int value);

/**
 * Adds a new property to an object WITH diskbase handling.  You should
 * use this one if you want to add a property.
 *
 * This is a wrapper around set_property, so if you need to
 * set values more complex than string or integer, use set_property_nofetch
 * instead.
 *
 * @param player Object to set property on
 * @param pname Property name
 * @param strval String value - pass NULL if you want an integer value
 * @param value Integer value - ignored if String value is used.
 */
void add_property(dbref player, const char *pname, const char *strval,
                  int value);

/**
 * This allocates a property node for use in the property AVL tree.  The
 * note has the given name (memory is copied over) and is set dirty, but
 * otherwise is a blank slate ready to be added to the AVL.
 *
 * Chances are, you do not want to use this method.  It is exposed because
 * it is used in boolexp.c and props.c
 *
 * @internal
 * @param name String property name (memory will be copied)
 * @return allocated PropPtr AVL node.
 */
PropPtr alloc_propnode(const char *name);

/**
 * This clears the indicated flags off the property.  It does not clear
 * type flags.  This is primarily used to remove the PROP_BLESSED flag.
 *
 * If the property does not exist, this function does nothing, but will
 * not cause an error of any sort.
 *
 * @param player The object to clear flags off of.
 * @param pname The name of the property
 * @param flags The flags to clear.
 */
void clear_property_flags(dbref player, const char *pname, int flags);

/**
 * This clears the data out of a property and sets it as unloaded.
 * This will free a string or boolexp (lock) from memory if applicable,
 * and clears out the data.  It does not clear the name out, so it is
 * not the opposite of alloc_propnode.
 *
 * @internal
 * @param p PropPtr to clear
 */
void clear_propnode(PropPtr p);

/**
 * This copies all the properties on an object and returns the root node.
 * It is used, for example, by the COPYOBJ primitive to copy all the
 * properties on an object.
 *
 * @param old DBREF of original object.
 * @return a struct plist that is a copy of all properties on 'old'.
 */
struct plist *copy_prop(dbref old);

/**
 * This copies the properties from 'from' onto 'to'.  It does not blow
 * away 'to's existing properties but merges in 'from' into 'to'.
 *
 * This is essentially like copy_prop, except copy_prop would be used
 * for an object in the process of being creaetd, and this will work
 * better for an existing object.
 *
 * @param from Source DBREF
 * @param to Destination DBREF
 */
void copy_properties_onto(dbref from, dbref to);

/**
 * This is the underpinning for both copy_prop and copy_properties_onto
 *
 * It recursively copies properties from obj (the "old" PropPtr should
 * be the root properties from "obj") into a structure "newer".
 *
 * newer may be either NULL or an existing prop tree.  The 'obj' dbref is
 * needed for diskbase reasons, however it looks like all consumers of
 * this call do the diskbase load so that could probably be refactored out
 * pretty easily.
 *
 * As this is an internal call, it doesn't matter too much; you should use
 * either copy_prop or copy_properties_onto.  Were we to move this call to
 * property.c, we could avoid having it in this header.
 *
 * @TODO Consider refactoring this so it lives in property.c and remove
 *       the need for 'obj'
 * @internal
 * @param obj DBREF object that 'old' props belong to.
 * @param newer Essentially a pointer to a pointer; the target structure
 * @param old The source property list
 */
void copy_proplist(dbref obj, PropPtr * newer, PropPtr old);

/**
 * Starts a recursive dump of props to the given file handle for the
 * given object.
 *
 * Under the hood, this uses db_dump_props_rec
 *
 * @param f File handle to write properties to.
 * @param obj DBREF of object to dump
 */
void db_dump_props(FILE * f, dbref obj);

/**
 * This gets a single property from the database.  It is usually used
 * by a higher level method such as db_getprops or diskbase's propfetch
 * You should probably not use it yourself.
 *
 * A point of interest is "pos", this is the initial position to load
 * the prop from or 0.  In the case of diskbase props, if a prop is in
 * the UNLOADED state, the value stored in the prop is an integer which
 * is the file position of the prop in teh database file.
 *
 * If pos is non 0, we will load the prop from that position, which is
 * how diskbase properties are loaded.  If pos is 0, we will actually
 * load the next prop in the database and use the current database
 * position for the diskbase if needed.
 *
 * @internal
 * @param f DB File handle.
 * @param obj DBREF of object who's properties we are loading.
 * @param pos Position to load the property from, or 0 to load in squence.
 * @param pnode If we have an existing property AVL node to load into.
 *              This may be NULL if you do not have it.
 * @param pdir This is used exclusively for error display.  This is
 *             usually a propdir but can be NULL.
 *
 * @return -1 on failure, 0 if end of prop list in DB, 1 if a prop was loaded,
 *         2 if the prop is empty and skipped.
 */
int db_get_single_prop(FILE * f, dbref obj, long pos, PropPtr pnode,
                       const char *pdir);

/**
 * This fetches all props for a given object.  It assumes the file pointer
 * 'f' is in position to start loading properties.  It will load all
 * properties (or, if diskbase is turned on, prop stubs) from the database.
 *
 * @param f The File pointer of the database.
 * @param obj The DBREF of the object we are loading props for.
 * @param pdir The propdir we are loading -- this should always be "/"
 *             but it really doesn't matter since it is just used in
 *             error messages.
 */
void db_getprops(FILE * f, dbref obj, const char *pdir);

/**
 * Delete a property from the given prop set, with the given property
 * name.  It removes it from the AVL of 'list'.  Does not save it to
 * the database right away.
 *
 * This is something of a low level call -- you probably want
 * remove_property
 *
 * @see remove_property
 *
 * @param list Pointer to a PropPtr which is (usually) the root of the
 *             property AVL tree.
 * @param name The name of the property to delete
 * @return Returns the pointer that 'list' is pointing to.  Because 'list'
 *         is modified, there is probably no reason to use the return
 *         value.
 */
PropPtr delete_prop(PropPtr * list, char *name);

/**
 * Recursively deletes an entire proplist AVL with 'p' as the root,
 * and frees 'p' itself.
 *
 * @param p The proplist to delete.
 */
void delete_proplist(PropPtr p);

/**
 * This function takes a property and generates a line akin to what
 * you would see on a prop listing (e.g. ex me=/)
 *
 * So a line that looks like this:
 *
 * - str /whatever:value
 *
 * The first character is either - or B for blessed.  Then the type.
 * Then the name, and finally the value.
 *
 * Any errors (no such property) will be written to the buffer.
 *
 * @param player is the player doing the call, and is used to determine
 *        permissions for unparse_object ('ref' type props)
 * @param obj is the object who's property is being examined.
 * @param name is the property name
 * @param buf is a buffer that you provide for property information to
 *        be loaded into.
 * @param bufsiz is the size of the buffer you are providing.
 *
 * @return the buffer (though using the return value is not necessary
 *         since the buffer is modified).
 */
char *displayprop(dbref player, dbref obj, const char *name, char *buf,
                  size_t bufsiz);

/**
 * This scans the environment for a given propname, starting with
 * the DBREF 'where' and crawling up the environment.  Please note
 * that this modifies 'where'; 'where' will ultimately contain the
 * object that the prop was found on.
 *
 * Will return NULL if the property is not found, and 'where' will
 * become NOTHING at that point.
 *
 * @param where A pointer to dbref object which contains the start object
 * @param propname The name of the property to search for.
 * @param typ The property type to look for, or 0 for any type.
 *
 * @return Either the prop structure we found, or NULL if not found.
 *         Note that 'where' is mutated as well.
 */
PropPtr envprop(dbref * where, const char *propname, int typ);

/**
 * This scans the environment for a given propname and returns the
 * string contents of the property.  The property MUST be a string
 * or this will fail.  Returns NULL if property is not found.
 *
 * 'where' will be mutated to be the dbref of the object where the
 * property was found or NOTHING if not found.
 *
 * @param where A pointer to dbref object which contains the start object
 * @param propname The name of the property to search for.
 *
 * @return Either the prop structure we found, or NULL if not found.
 *         Note that 'where' is mutated as well.
 */
const char *envpropstr(dbref * where, const char *propname);

/**
 * exec_or_notify is the thing that is used to process various "message"
 * props such as the props for @success, @odrop, etc.  It understands:
 *
 * - Strings that start with '@' symbol and run a MUF program
 * - Any other string.  MPI will be parsed.
 *
 * MUFs are run with PREEMPT and HARDUID.  Error conditions are notify'd
 * to the player.
 *
 * @param descr - integer descriptor to notify.
 * @param player - The DBREF of the calling player.
 * @param thing - The DBREF of the thing emitting the message.
 * @param message - The message to emit
 * @param whatcalled - This is the caller context -- its the
 *                     MPI &how or the MUF command.  Such as "(@Desc)"
 * @param mpiflags - What MPI flags to use - MPI_ISPRIVATE is added.
 */
void exec_or_notify(int descr, dbref player, dbref thing, const char *message,
                    const char *whatcalled, int mpiflags);

/**
 * @see exec_or_notify
 *
 * This does the same thing as exec_or_notify except it does the prop load
 * for you and also handles the MPI flags (handling if a property is blessed
 * or not).
 *
 * If the prop is not set, there is no notification at all.  Any errors are
 * notified to the user.
 *
 * @param descr - integer descriptor to notify.
 * @param player - The DBREF of the calling player.
 * @param thing - The DBREF of the thing emitting the message.
 * @param propname - The property to load off 'thing'.
 * @param whatcalled - This is the caller context -- its the
 *                     MPI &how or the MUF command.  Such as "(@Desc)"
 */
void exec_or_notify_prop(int descr, dbref player, dbref thing,
                         const char *propname, const char *whatcalled);         

/**
 * Finds the first node ("leftmost") on the property AVL list 'p' or
 * returns NULL if p has no nodes on it.
 *
 * @param p PropPtr of the root of the AVL property list you want to scan.
 *
 * @return First/leftmost node on proplist.
 */
PropPtr first_node(PropPtr p);

/**
 * Returns a pointer to the first property on an object for the given
 * propdir.  This is useful for iterating over properties, such as with
 * the NEXTPROP primitive.
 *
 * This version has 'diskbase' wrappers; under the hood, it calls
 * first_prop_nofetch.  You should probably use this version.
 *
 * dir may be NULL to indicate the / root property.
 *
 * @param player the DBREF of the object to get the properties from.
 * @param dir The string name of the propdir
 * @param list pointer to a proplist.  We will use this field to return
 *        the root node to you.
 * @param name pointer to a string buffer - this will be used to return
 *        the property name to you.
 * @param maxlen the size of the buffer.
 *
 * @return a pointer to the first property in a propdir, or NULL if
 *         the property list is empty.  If there is no property, then
 *         name will be an empty string.
 */
PropPtr first_prop(dbref player, const char *dir, PropPtr * list, char *name,
                   size_t maxlen);

/**
 * Returns a pointer to the first property on an object for the given
 * propdir.  This is useful for iterating over properties, such as with
 * the NEXTPROP primitive.
 *
 * This version does not fetch from the diskbase and is mostly for internal
 * use; chances are you want first_prop.
 *
 * dir may be NULL to indicate the / root property.
 *
 * @internal
 * @param player the DBREF of the object to get the properties from.
 * @param dir The string name of the propdir
 * @param list pointer to a proplist.  We will use this field to return
 *        the root node to you.
 * @param name pointer to a string buffer - this will be used to return
 *        the property name to you.
 * @param maxlen the size of the buffer.
 *
 * @return a pointer to the first property in a propdir, or NULL if
 *         the property list is empty.  If there is no property, then
 *         name will be an empty string.
 */
PropPtr first_prop_nofetch(dbref player, const char *dir, PropPtr * list,
                           char *name, size_t maxlen);

/**
 * This is the opposite of alloc_propnode, and is used to free the
 * PropPtr datastructure.  It will free whatever data is associated
 * with the prop as well as the PropPtr as well.
 *
 * @param node the prop to free
 */
void free_propnode(PropPtr node);

/**
 * Get a property and return its PropPtr, for a given object.
 *
 * This version handles diskbase and is the one you should normally use.
 * Returns NULL if the property was not found.
 *
 * @param player The object to search for the property on.
 * @param pname The property name
 *
 * @return a PropPtr object with the property, or NULL if not found.
 */
PropPtr get_property(dbref player, const char *type);

/**
 * The name of this call is a little bit of a misnomer; this actually
 * returns the STRING property value of a given property name.  If the property
 * is not a string property (i.e. if it is an integer or ref), this will
 * return NULL.  If the property does not exist, it will also return NULL.
 *
 * @param player The object to look up the ref on
 * @param pname The property name to look up.
 *
 * @return either the string value of the property or NULL as described above.
 */
const char *get_property_class(dbref player, const char *pname);

/**
 * This gets the DBREF property value of a given property name.  If the
 * property is not a dbref property (like if its a string or integer), this
 * will return NULL.  If the property does not exist, it will also return
 * NOTHING.
 *
 * @param player the object to look up the ref on.
 * @param pname The property name to look up.
 *
 * @return either the dbref value of the property or NOTHING as described above.
 */
dbref get_property_dbref(dbref player, const char *pname);

/**
 * Get the flags for a given property name.  At the time of this writing,
 * the flags are mostly used for BLESSED properties but you can also use it
 * to see if the property is loaded from diskbase or not.  This doesn't include
 * the property type flags.
 *
 * @param player DBREF of the object to look up.
 * @param pname The property name to look up
 *
 * @return integer flags of property - or 0 if not found.
 */
int get_property_flags(dbref player, const char *pname);

/**
 * Get the floating point value for a given property name.  If the property
 * is not a floating point (like if it is a string or integer), this will
 * return 0.0.  If the property does not exist, it will also return 0.0.
 *
 * @param player DBREF of the object to look up.
 * @param pname The property name to look up
 *
 * @return Floating point value of property, or 0.0 as described above.
 */
double get_property_fvalue(dbref player, const char *pname);

/**
 * Get the boolexp (lock) value for a given property name.  If the
 * property is not a lock prop, if it does not exist, or if it won't
 * load from the diskbase (... not sure under what condition that would
 * happen!) then this returns TRUE_BOOLEXP.
 *
 * @param player DBREF of object to loop up.
 * @param pname the property name to look up
 *
 * @return boolexp structure if a lock property, otherwise TRUE_BOOLEXP
 *         as described above.
 */
struct boolexp *get_property_lock(dbref player, const char *pname);

/**
 * This is another misnomer; get_property_value actually gets the
 * INTEGER value of a property.  If it is a STRING or other property
 * type, it will return 0 instead.  It will also return 0 if the
 * property does not exist.
 *
 * @param player DBREF of object to look up
 * @param pname the property name to look up.
 *
 * @return integer property value or 0 as described above.
 */
int get_property_value(dbref player, const char *pname);

/**
 * has_property scans an object and all its contents to see if it has
 * a given property name, and checks to see if it matches the values
 * provided.
 *
 * If the found property type is string, it matches against the 'strval'
 * and will (potentially) MPI parse the property to see if it resolves
 * into 'strval' (more below).
 *
 * If the found property is an integer, it is compared against 'value'.
 * If the found property is a floating point, it is also compared against
 * integer after the float has been cast to an integer.  'Ref' and 'lock'
 * type props will always return not found under this call.
 *
 * The fact that the property type rather than the type of parameter
 * passed into this call determines the type of comparison is very odd;
 * it could have unexpected behavior if, for instance, you want a string
 * comparison but the property is set as an integer.  It could accidentally
 * match the integer instead of the string, which may be a way to bypass
 * locks (as this call seems to be used for locking primarily).
 *
 * @TODO: Code review - would a call such as :
 *        has_property(x, db, db2, "whatever/prop", "PassString", 0)
 *        successfully pass the lock check if the property is an
 *        integer propval '0' instead of 'PassString' which would be
 *        the expected value?  Potential security hole.
 *
 * MPI in string properties is parsed based on a has_prop_recursion_limit
 * static variable.  It looks like a measure to prevent the MPI from
 * causing a loop where has_property calls has_property on the same property
 * downstream.  If the recursion limit is reached, the string property is
 * just treated as a regular string without MPI parsing.
 *
 * If the tune parameter 'lock_envcheck' is set to yes, then this call
 * will also check the environment for the property and value.
 *
 * @see has_property_strict which is used under the hood.
 *
 * @param descr Integer descriptor of the caller.
 * @param player the DBREF of the player that has triggered the has_property
 *        check
 * @param what the object that triggered teh check
 * @param pname The property name to check
 * @param strval the string value to check
 * @param value the integer value to check
 *
 * @return boolean - true if property exists with value, false otherwise.
 */
int has_property(int descr, dbref player, dbref what, const char *pname,
                 const char *strval, int value);

/**
 * has_property_strict is the call that underpins has_property, and
 * unlike has_property, it checks property values for a *single*
 * object and not the object, all its contents, etc.
 *
 * Otherwise, all the notes for has_property apply here and you should
 * read the information there for more details as that call goes into
 * great detail about how this works.
 *
 * @see has_property
 * @param descr Integer descriptor of the caller.
 * @param player the DBREF of the player that has triggered the has_property
 *        check
 * @param what the object that triggered teh check
 * @param pname The property name to check
 * @param strval the string value to check
 * @param value the integer value to check
 *
 * @return boolean - true if property exists with value, false otherwise.
 */
int has_property_strict(int descr, dbref player, dbref what, const char *type,
                        const char *strval, int value);

/**
 * Checks to see if the property 'dir' is a propdir or not on the object
 * 'player'
 *
 * @param player DB of object to check
 * @param dir The property path to check.
 *
 * @return boolean - 1 if is propdir, otherwise 0 (including if prop does
 *         not exist.
 */
int is_propdir(dbref player, const char *dir);

/**
 * This finds a prop named 'key' in the AVL proplist 'avl'.  It is basically
 * a primitive for looking up items in the AVLs.
 *
 * @param avl the AVL to search
 * @param key the key to look up
 *
 * @return the found node, or NULL if not found.
 */
PropPtr locate_prop(PropPtr avl, char *key);

/**
 * This creates a new node in the AVL then returns the created node
 * so that you might populate it with data.  If the key already
 * exists, then the existing node is returned.
 *
 * @param avl the AVL to add a property to.
 * @param key the key to add to the AVL.
 *
 * @return the newly created AVL node.
 */
PropPtr new_prop(PropPtr *avl, char *key);

/**
 * next_node locates and returns the next node in the AVL (prop directory)
 * or NULL if there is no more.  It is used for traversing a prop AVL.
 *
 * Name should be a single prop name and not a prop path; this is not for
 * navigating a whole property path.  If you want to navigate a path,
 * use propdir_next_elem or next_prop
 *
 * @see propdir_next_elem
 * @see next_prop_name
 *
 * @param ptr the AVL to navigate
 * @param name The "previous name" ... what is returned is the next name
 *        after this one
 * @return the property we found, or NULL
 */
PropPtr next_node(PropPtr ptr, char *name);

/**
 * next_prop is a wrapper around next_node to provide a slightly different
 * way to get the next property.  It does not navigate a whole path;
 * for that, use propdir_next_elem or next_prop_name
 *
 * @see propdir_next_elem
 * @see next_prop_name
 *
 * Given the root of an AVL 'list' and a property 'prop', this function
 * returns the next property after 'prop' or NULL if there is no next
 * property.
 *
 * 'name' is a buffer used to store the name of the returned property;
 * it is not read from, it is strictly used to return the name.  As
 * you can easily get the name from the property with PropName, this
 * has sort of questionable utility but is available if useful to you
 *
 * maxlen is the length of your buffer.
 *
 * @param list the AVL prop list root
 * @param prop the 'previous' node - we will get the next node after this one
 * @param name a buffer to copy the property name into
 * @param maxlen the length of that buffer.
 *
 * @return the next property node or NULL if no more.
 */
PropPtr next_prop(PropPtr list, PropPtr prop, char *name, size_t maxlen);

/**
 * next_prop_name returns the string name of the next property on a
 * given object (player) with the "previous proprty" being "name".
 * outbuf is used to store the name of the property after the property "name"
 *
 * This will return 'outbuf' if there is a next property, or NULL if there
 * is not.  If there is no next property, outbuf will be an empty string.
 *
 * Call this with name "" to get the first property.
 *
 * @param player the object to fetch properties from
 * @param outbuf the output buffer to use for storing the next prop name
 * @param outbuflen The size of the output buffer.
 * @param name the name of the previous property
 *
 * @return either 'outbuf' or NULL as described above.
 */
char *next_prop_name(dbref player, char *outbuf, size_t outbuflen, char *name);

/**
 * An "oprop" is something like an @OSuccess or @ODrop ... a message that
 * is prefixed with the player's name.  This loads the property, parses
 * pronouns and MPI, then emits the message to the room the triggering
 * player is in.
 *
 * This is equivalent to exec_or_notify which works for @Success, etc.
 * messages.
 *
 * @param descr The descriptor of the person triggering
 * @param player The player DBREF that triggered the action.
 * @param dest The destination DBREF.
 * @param exit The exit DBREF that triggered the action.
 * @param propname The name of the property to load.
 * @param prefix What will be prefixed to this message before broadcast.
 *        This is pretty much always the player's name.  You do not need
 *        to include a trailing space.
 * @param whatcalled The &how / command verb, such as (@OSucc)
 */
void parse_oprop(int descr, dbref player, dbref dest, dbref exit,
                 const char *propname, const char *prefix,
                 const char *whatcalled);

/**
 * This deletes an element (path) from a propdir.  Note that you cannot
 * delete the root property (path "/") with this call; that actually does
 * nothing.  If you delete a propdir, it will delete all child properties
 * within the propdir.
 *
 * This is something of a low-level call; you may rather use remove_property
 *
 * @see remove_property
 *
 * @param root The root property node to start your search
 * @param path the path you are searching for to delete.
 *
 * @return the updated root node with the property removed.  Because this
 *         mutates the passed structure, this is equivalent to the 'root'
 *         parameter.
 */
PropPtr propdir_delete_elem(PropPtr root, char *path);

/**
 * This gets the first element of a propdir given a certain path.
 * As this is something of a low level call, you may prefer to use
 * first_prop instead.
 *
 * @see first_prop
 *
 * @param root The root of the property directory tree.
 * @param path The path you want to get the first element of; "" or "/" both
 *        count as root.
 *
 * @return the first element of the given path or NULL if not found.
 */
PropPtr propdir_first_elem(PropPtr root, char *path);

/**
 * Fetches a given property from the property path structure 'root'.
 * As this is something of a low level call you may prefer to use 
 * get_property
 *
 * @see get_property
 * @see get_property_class
 * @see get_property_dbref
 * @see get_property_fvalue
 * @see get_property_lock
 * @see get_property_value
 *
 * @param root The root of the property directory tree
 * @param path the property you wish to return
 *
 * @return the found property or NULL if not found.
 */
PropPtr propdir_get_elem(PropPtr root, char *path);

/**
 * This is basically the equivalent of the POSIX "dirname", which retrieves
 * the property directory path portion of a given propname.  Its primary
 * purpose is for the diskbase; in order to load a propdir's properties,
 * you must first fetch the propdir itself from the diskbase.
 *
 * This call uses a static buffer so it is not threadsafe (though none of
 * the MUCK is) but also will mutate over subsequent calls.  In the
 * core usecase of this method, this does not matter, but if you are
 * doing something more creative you may wish to copy the directory out
 * of its buffer.  Do not try to free this buffer.
 *
 * @param name The property name to get a propdir name for.
 * @return the property's propdir
 */
const char *propdir_name(const char *name);

/**
 * This creates a new property node at the given path on the given root
 * propdir object.  If there is already a property at that path location,
 * the existing property is returned; otherwise a new node is returned.
 *
 * According to the original documentation, if the property name is "bad"
 * this can return NULL.  However, practically speaking, this isn't actually
 * implemented.  Note that creating a property with a ':' in the name is
 * a particularly bad idea but is currently not checked for.  This *will*
 * return NULL if you put an empty string or just /'s.
 *
 * This is a "low level" call and probably not what you want; you
 * will probably want to use add_property or set_property.  set_property in
 * particular offers the same flexibility as this call but operates with
 * dbrefs and lets you set a value in one call.
 *
 * @see add_property
 * @see set_property
 *
 * @param root The root of the property directory structure.
 * @param path The path to create.
 *
 * @return the newly created node, or the existing node at the given path,
 *         or NULL on error
 */
PropPtr propdir_new_elem(PropPtr * root, char *path);

/**
 * Returns pointer to the next property after the given one in the given
 * given structure (root).  Please note that this is a "low level call"
 * and you may prefer to use next_prop_name
 *
 * @see next_prop_name
 *
 * @param root The root of the property directory structure
 * @param path The 'previous' property path.
 *
 * @return the next property in the propdir or NULL if no more.
 */
PropPtr propdir_next_elem(PropPtr root, char *path);

/**
 * Returns the path of the first unloaded propdir in a given path,
 * or NULL if all the propdirs to the path are loaded.  You will
 * probably never use this call.
 *
 * @param root The root propdir node
 * @param path The path to operate on.
 *
 * @return path name as described above, or NULL.
 */
const char *propdir_unloaded(PropPtr root, const char *path);

/**
 * A reflist is a space-delimited set of DBREFs in a string, each
 * ref starting with a hash mark, such as:
 *
 * #123 #456 #789
 *
 * This is a convienance method to add a dbref to a ref list.  If
 * the propname given already exists and is a 'ref' type prop, it
 * will be converted to a string with the old and new refs in it.
 * If the property is empty, this ref will be added to it as a
 * string prop to start a new reflist.
 *
 * If the toadd ref is already in the reflist, it will 'migrate' to
 * the end of the reflist.  This method does not allow refs to be
 * duplicate.
 *
 * @param obj The object to operate on
 * @param propname the property name for our reflist
 * @param toadd the ref to add to the reflist
 */
void reflist_add(dbref obj, const char *propname, dbref toadd);

/**
 * Removes a ref from a reflist.  See the description of reflist_add
 * for a description of what a reflist is
 *
 * @see reflist_add
 *
 * @param obj the object to work on
 * @param propname the property name of the reflist
 * @param todel the ref to delete.
 */
void reflist_del(dbref obj, const char *propname, dbref todel);

/**
 * This finds a ref in the reflist.  The number returned is either '0'
 * if not found, or it is the "position" of the ref in the reflist.
 *
 * So ... if you have a reflist:
 *
 * #123 #345 #678
 *
 * #123 is position 1, #345 is position 2, and #678 is position 3.
 *
 * This method is in support of the REFLIST_FIND primitive which is why
 * the odd return value.
 *
 * @param obj The object to work on
 * @param propname The reflist property name
 * @param tofind the ref to find in the list
 *
 * @return an integer describing the position of the ref as described above,
 *         or 0 if not found.
 */
int reflist_find(dbref obj, const char *propname, dbref tofind);

/**
 * This removes a property from a given object.  "sync" is for some funky
 * logic around the gender property.  If the MUCK's @tune'd gender property
 * is not the same as LEGACY_GENDER_PROP (usually "sex"), and sync is 0,
 * then LEGACY_GENDER_PROP and the tune'd gender property receive the
 * same fate.
 *
 * Thus, if sync is 0, and you delete the tune'd gender prop, it will
 * also delete LEGACY_GENDER_PROP or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  This call handles
 * all the diskbase stuff.
 *
 * @param player The object to operate on.
 * @param pname the property name to delete
 * @param sync Do not sync gender props?  See description above.
 */
void remove_property(dbref player, const char *pname, int sync);

/**
 * This call is to remove ALL properties on an object; it is used
 * for @set whatever=:clear exclusively at the time of this writing.
 *
 * If 'all' is 0, "wizard" properties (@ and ~ properties, or more
 * specifically, PROP_HIDDEN and PROP_SEEONLY properties) and the "_/"
 * propdir are left alone.
 *
 * If 'all' is 1, then everything is blown away regardless.
 *
 * @param player The object to clear properties on.
 * @param all Clear all properties? boolean -- see description above.
 */
void remove_property_list(dbref player, int all);

/**
 * This removes a property from a given object.  "sync" is for some funky
 * logic around the gender property.  If the MUCK's @tune'd gender property
 * is not the same as LEGACY_GENDER_PROP (usually "sex"), and sync is 0,
 * then LEGACY_GENDER_PROP and the tune'd gender property receive the
 * same fate.
 *
 * Thus, if sync is 0, and you delete the tune'd gender prop, it will
 * also delete LEGACY_GENDER_PROP or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  This call DOES NOT
 * handle the diskbase stuff and therefore you probably want remove_property
 * instead.
 *
 * @see remove_property
 *
 * @internal
 * @param player The object to operate on.
 * @param pname the property name to delete
 * @param sync Do not sync gender props?  See description above.
 */
void remove_property_nofetch(dbref player, const char *type, int sync);

/**
 * This command is the underpinning of @lock, @flock, @linklock and @chlock.
 * Please note that part of this function's functionality relies on the
 * global 'match_args', so this is not really an API call.  This is
 * a command implementation.
 *
 * If objname is an empty string, 'player' will be used as the target
 * object.  Otherwise, the name will be matched.  If the match fails,
 * a message will be emitted to the user.
 *
 * If there is no '=' mark in match_args, it will emit a message
 * describing the lock on the object and does not set anything.
 *
 * If there is an = but no string after the equals (i.e. '@lock x='),
 * then it clears the lock
 *
 * Otherwise, the lock is 'compiled' and, if valid, set on the property
 * propname.  Otherwise, an error is emitted to the user.
 *
 * @param descr Descriptor
 * @param player The player doing the command
 * @param objname The name of the object we are locking or ""
 * @param propname The name of the prop to set
 * @param proplabel This is used for messaging and is the type of lock.
 *        The first letter should be capitalized.  For instance, "Lock",
 *        "Ownership Lock", etc.
 * @param keyvalue The lock itself or "" to clear the lock.
 */
void set_standard_lock(int descr, dbref player, const char *objname,
                       const char *propname, const char *proplabel,
                       const char *keyvalue);

/**
 * This command is the underpinning of all of the @ commands that just
 * set a property under _/ .... for instance, @odrop, @describe, etc.
 * As such, this is not really an API call and it relies on the
 * global 'match_args' as well as emits messages to the user.
 *
 * If objname is an empty string, 'player' will be used as the target
 * object.  Otherwise, the name will be matched.  If the match fails,
 * a message will be emitted to the user.
 *
 * If there is no '=' mark in match_args, then the 'propname' is cleared.
 *
 * Appropriate messages based on proplabel are emitted.  Proplabel
 * should be somethinglike "Object Description", "Drop Message", etc.
 * and is used to emit messages.
 *
 * @param descr Descriptor
 * @param player The player doing the action
 * @param objname The object name to match, or "" for the player.
 * @param propname The property we are going to set, like "_/de"
 * @param proplabel Used for messaging -- see description above.
 * @param propvalue The value to set.
 */
void set_standard_property(int descr, dbref player, const char *objname,
                           const char *propname, const char *proplabel,
                           const char *propvalue);

/**
 * Set a property on an object (the 'player'), with name pname and
 * with property data 'dat'.  'sync' has to do with syncing gender
 * props -- more on that in a moment.
 *
 * This method (unlike some others) does full property name validation.
 * Leading / are trimmed off, and if there is a ':' the prop name will
 * be terminated at that point (the : becomes a \0).  The : is defined
 * as PROP_DELIMITER.  
 * 
 * If there is no property name after cleanup, this just returns with nothing
 * done.
 *
 * As for 'dat' ... its flags field is used to initialize the flags on the
 * newly made property.  If the flags field in 'dat' has PROP_ISUNLOADED, then
 * it is set on the prop as-is.  In this way, a diskbase'd property can
 * be set to its diskbase'd value.
 *
 * Otherwise, type is derived from the flags field in dat.  If it is a
 * string, a *copy* of the string is made for the new prop so you should
 * clean up your own string as necessary.
 *
 * Float, int, and dbref are all trivial and just are copied over.  If
 * you create a PROP_DIRTYP (propdir) it must already have props in it.
 * For a LOCK -- and this is very important -- the boolexp lock structure
 * is NOT COPIED and the property 'owns' the lock memory from this point
 * forward.  Do not free it!
 *
 * If you set the property to what is considered an empty value
 * ("" for string, 0 for int, 0.0 for float, or a PROP_DIRTYP with no
 * properties in it), it will be deleted instead.
 *
 * And if this comment wasn't long enough -- we've also got "sync"!
 * "sync" is for some funky logic around the gender property.  If the MUCK's
 * @tune'd gender property is not the same as LEGACY_GENDER_PROP
 * (usually "sex"), and sync is 0, then LEGACY_GENDER_PROP and the tune'd
 * gender property receive the same fate.
 *
 * Thus, if sync is 0, and you set the tune'd gender prop, it will
 * also set LEGACY_GENDER_PROP to the same or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  The original purpose
 * around sync is to avoid a recursion problem.
 *
 * This version of set_property handles all the diskbase stuff.
 *
 * @param player The ref of the object to set the property on.
 * @param pname The property name to set.
 * @param dat a PData structure, loaded with the right flags and prop data.
 * @param sync the weird gender sync option.
 */
void set_property(dbref player, const char *pname, PData * dat, int sync);

/**
 * Sets the provided flags on a property named 'type' on object 'player'.
 *
 * Does not allow setting of property type flag; this is primarily used
 * to set BLESSED.
 *
 * @param player The object to operate on
 * @param pname The property name
 * @param flags The flags to set.
 */
void set_property_flags(dbref player, const char *pname, int flags);

/**
 * Set a property on an object (the 'player'), with name pname and
 * with property data 'dat'.  'sync' has to do with syncing gender
 * props -- more on that in a moment.
 *
 * This method (unlike some others) does full property name validation.
 * Leading / are trimmed off, and if there is a ':' the prop name will
 * be terminated at that point (the : becomes a \0).  The : is defined
 * as PROP_DELIMITER.  
 * 
 * If there is no property name after cleanup, this just returns with nothing
 * done.
 *
 * As for 'dat' ... its flags field is used to initialize the flags on the
 * newly made property.  If the flags field in 'dat' has PROP_ISUNLOADED, then
 * it is set on the prop as-is.  In this way, a diskbase'd property can
 * be set to its diskbase'd value.
 *
 * Otherwise, type is derived from the flags field in dat.  If it is a
 * string, a *copy* of the string is made for the new prop so you should
 * clean up your own string as necessary.
 *
 * Float, int, and dbref are all trivial and just are copied over.  If
 * you create a PROP_DIRTYP (propdir) it must already have props in it.
 * For a LOCK -- and this is very important -- the boolexp lock structure
 * is NOT COPIED and the property 'owns' the lock memory from this point
 * forward.  Do not free it!
 *
 * If you set the property to what is considered an empty value
 * ("" for string, 0 for int, 0.0 for float, or a PROP_DIRTYP with no
 * properties in it), it will be deleted instead.
 *
 * And if this comment wasn't long enough -- we've also got "sync"!
 * "sync" is for some funky logic around the gender property.  If the MUCK's
 * @tune'd gender property is not the same as LEGACY_GENDER_PROP
 * (usually "sex"), and sync is 0, then LEGACY_GENDER_PROP and the tune'd
 * gender property receive the same fate.
 *
 * Thus, if sync is 0, and you set the tune'd gender prop, it will
 * also set LEGACY_GENDER_PROP to the same or vise versa.
 *
 * 'sync' and the behavior surrounding it only applies to players; sync
 * has absolutely no effect on any other type of DB object.
 *
 * There is an additional wrinkle; there is a LEGACY_GUEST_PROP
 * (usually "~/isguest").  If this is deleted, then the GUEST dbflag
 * is removed from the player as well.  This is regardless of the 'sync'
 * setting, and only happens for object type == Player
 *
 * You will typically want to run this with sync = 1.  The original purpose
 * around sync is to avoid a recursion problem.
 *
 * This version of set_property DOES NOT do diskbase -- you probably want
 * to use set_property instead.
 *
 * @see set_property
 *
 * @internal
 * @param player The ref of the object to set the property on.
 * @param pname The property name to set.
 * @param dat a PData structure, loaded with the right flags and prop data.
 * @param sync the weird gender sync option.
 */
void set_property_nofetch(dbref player, const char *pname, PData * dat, int sync);

/**
 * Get the size in bytes of the properties on an object.  If load is 1,
 * we will actually load all properties on the object before calculating
 * size; this will give an accurate depiction of the total size, though
 * load = 0 will tell you the size of what is currently loaded in memory.
 *
 * @param player The object to check
 * @param load a boolean who's use is described above.
 *
 * @return the size in bytes consumed by the object in memory.
 */
size_t size_properties(dbref player, int load);

/**
 * Calculates the size of the given property directory AVL list.  This
 * will iterate over the entire structure to give the entire size.  It
 * is the low level equivalent of size_properties.
 *
 * @see size_properties
 *
 * @param avl the Property directory AVL to check
 * @return the size of the loaded properties in memory -- this does NOT
 *         do any diskbase loading.
 */
size_t size_proplist(PropPtr avl);

/**
 * This function is a progressive iteration over the entire database,
 * keeping track of its last position with a static variable.  It will
 * iterate over "limit" number of database refs, marking all properties
 * on that database object as untouched (in other words, removing the
 * PROP_TOUCHED property).
 *
 * Once it reaches the end of the database, it starts over again.
 *
 * This is called once per game loop.  I am honestly not sure
 * what the purpose of this is, but it would likely be very disruptive
 * to mess with it.  Recommend not touching this call.
 *
 * @internal
 * @param limit The number of database objects to process before returning.
 */
void untouchprops_incremental(int limit);

/**
 * Check to see if a property name is valid.  Which, at present, means
 * the name does not contain a '\r' or PROP_DELIMITER (':') in it.
 *
 * Note - This function is used by MUF primitives but not by the rest of
 * the property 'library' here.  The preference with the underlying library
 * seems to be to treat the PROP_DELIMITER as a null terminator rather than
 * doing some more proper validation / error handling.
 *
 * This is a rather odd design choice, but perhaps a dangerous one to
 * reverse at this time.  getprop / remove prop don't validate the prop
 * name string at all and just assume setprop won't let something bad
 * get past the goalie.
 *
 * ext-name-ok and other such string checks also check ok_ascii_other;
 * props do no such checking, so theoretically you could get some weird
 * characters in the propname.  Do we care?  I assume not!
 *
 * @param pname The property name to validate
 * @return boolean 1 if valid, 0 if not
 */
int is_valid_propname(const char* pname);

#endif /* !PROPS_H */
