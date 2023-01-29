/** @file db.h
 *
 * Header for declaring database functions and DB-specific \#defines.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "config.h"
#include "fbmuck.h"
#include "mcp.h"

/**
 * This is used to identify the database dump file
 */
#define DB_VERSION_STRING "***Foxen9 TinyMUCK DUMP Format***"

/**
 * Parent of all rooms (always \#0)
 */
#define GLOBAL_ENVIRONMENT ((dbref) 0)

/**
 * @var match_args
 *      The super not threadsafe way user command arguments are passed
 *      from the interp loop to other functions.
 */
extern char match_args[BUFFER_LEN];

/** 
 * @var match_cmdname
 *      The super not threadsafe way the user typed in command is passed
 *      from the interp loop to other functions.
 */
extern char match_cmdname[BUFFER_LEN];

/**
 * Fetch a database object by dbref
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch an object for
 * @return a struct object
 */
#define DBFETCH(x)  (db + (x))

/**
 * Set a database object dirty.
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to mark dirty
 */
#define DBDIRTY(x)  {db[x].flags |= OBJECT_CHANGED;}

/**
 * Set a struct field 'y' for object 'x' to 'z' and mark the object dirty
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to alter
 * @param y the struct field for the object to modify
 * @param z the value toset on the struct field
 */
#define DBSTORE(x, y, z)    {DBFETCH(x)->y = z; DBDIRTY(x);}

/**
 * Get name of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch a name for
 * @return the name associated with x
 */
#define NAME(x)     (db[x].name)

/**
 * Get flags of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch flags for
 * @return the flags associated with x
 */
#define FLAGS(x)    (db[x].flags)

/**
 * Get owner of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch owner for
 * @return the owner associated with x
 */
#define OWNER(x)    (db[x].owner)

/**
 * Get location of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch location for
 * @return the location associated with x
 */
#define LOCATION(x) (DBFETCH((x))->location)

/**
 * Get contents of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch contents for
 * @return the contents associated with x
 */
#define CONTENTS(x) (DBFETCH((x))->contents)

/**
 * Get exits of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch exits for
 * @return the exits associated with x
 */
#define EXITS(x)    (DBFETCH((x))->exits)

/**
 * Get next field of dbref 'x'
 *
 * There is no database overrun protection with this call, so make sure
 * x is between 0 and dbtop before trying this.
 *
 * @param x the dbref to fetch next field for
 * @return the next field associated with x
 */
#define NEXTOBJ(x)  (DBFETCH((x))->next)

/* defines for possible data access mods. */
#define MESGPROP_DESC       "_/de"      /**< description prop */
#define MESGPROP_IDESC      "_/ide"     /**< idescription prop */
#define MESGPROP_SUCC       "_/sc"      /**< success prop */
#define MESGPROP_OSUCC      "_/osc"     /**< osuccess prop */
#define MESGPROP_FAIL       "_/fl"      /**< fail prop */
#define MESGPROP_OFAIL      "_/ofl"     /**< ofail prop */
#define MESGPROP_DROP       "_/dr"      /**< drop prop */
#define MESGPROP_ODROP      "_/odr"     /**< odrop prop */
#define MESGPROP_DOING      "_/do"      /**< doing prop */
#define MESGPROP_OECHO      "_/oecho"   /**< oecho prop */
#define MESGPROP_PECHO      "_/pecho"   /**< pecho prop */
#define MESGPROP_LOCK       "_/lok"     /**< lock prop */
#define MESGPROP_FLOCK      "@/flk"     /**< force lock prop */
#define MESGPROP_CONLOCK    "_/clk"     /**< con lock prop */
#define MESGPROP_CHLOCK     "_/chlk"    /**< chown lock prop */
#define MESGPROP_LINKLOCK   "_/lklk"    /**< link lock prop */
#define MESGPROP_READLOCK   "@/rlk"     /**< read lock prop */
#define MESGPROP_OWNLOCK    "@/olk"     /**< own lock prop */
#define MESGPROP_PCON       "_/pcon"    /**< pcon prop */
#define MESGPROP_PDCON      "_/pdcon"   /**< pdcon prop */
#define MESGPROP_VALUE      "@/value"   /**< value prop */

/**
 * Get a message from a prop -- an alias for get_property_class
 *
 * @see get_property_class
 *
 * @param x the object to look up a property for
 * @param y the property to look up
 * @return the value of the property or NULL if unset.
 */
#define GETMESG(x,y)    (get_property_class(x, y))

/**
 * Get description property
 *
 * @param x the object to get the description for
 * @return the description or NULL if unset
 */
#define GETDESC(x)      GETMESG(x, MESGPROP_DESC)

/**
 * Get idescription property
 *
 * idescriptions are interior descriptions for Vehicle objects.
 *
 * @param x the object to get the idescription for
 * @return the idescription or NULL if unset
 */
#define GETIDESC(x)     GETMESG(x, MESGPROP_IDESC)

/**
 * Get success message property
 *
 * @param x the object to get the success for
 * @return the message or NULL if unset
 */
#define GETSUCC(x)      GETMESG(x, MESGPROP_SUCC)

/**
 * Get osuccess message property
 *
 * @param x the object to get the osuccess for
 * @return the message or NULL if unset
 */
#define GETOSUCC(x)     GETMESG(x, MESGPROP_OSUCC)

/**
 * Get fail message property
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETFAIL(x)      GETMESG(x, MESGPROP_FAIL)

/**
 * Get ofail message property
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETOFAIL(x)     GETMESG(x, MESGPROP_OFAIL)

/**
 * Get drop message property
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETDROP(x)      GETMESG(x, MESGPROP_DROP)

/**
 * Get odrop message property
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETODROP(x)     GETMESG(x, MESGPROP_ODROP)

/**
 * Get doing message property
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETDOING(x)     GETMESG(x, MESGPROP_DOING)

/**
 * Get oecho message property
 *
 * oecho is the message prefixing vehicle messages for the interior.
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETOECHO(x)     GETMESG(x, MESGPROP_OECHO)

/**
 * Get pecho message property
 *
 * pecho is the message that prefixes zombie messages
 *
 * @param x the object to get the message for
 * @return the message or NULL if unset
 */
#define GETPECHO(x)     GETMESG(x, MESGPROP_PECHO)

/**
 * Set a property and set the object as modified
 *
 * This is an alias for @see add_property
 * Followed by @see ts_modifyobject
 *
 * @param x the object to modify
 * @param y the property to modify
 * @param z the value to set
 */
#define SETMESG(x,y,z)  {add_property(x, y, z, 0);ts_modifyobject(x);}

/**
 * Set a description message and mark the object as modified
 *
 * @param x the object to set the message on
 * @param y the message to set
 */
#define SETDESC(x,y)    SETMESG(x, MESGPROP_DESC, y)

/**
 * Get the struct boolexp associated with the lock property of x
 *
 * A thin wrappera round @see get_property_lock
 *
 * @param x the object to look up
 * @return the struct boolexp object or TRUE_BOOLEXP
 */
#define GETLOCK(x)      (get_property_lock(x, MESGPROP_LOCK))

/**
 * Sets a lock from a boolexp structure
 *
 * Typically you would take the return value from @see parse_boolexp
 * and then use it as the 'y' parameter here.  The object modification
 * timestamp is updated.
 *
 * @param x the object to set the lock on
 * @param y the struct boolexp to use as the lock value.
 */
#define SETLOCK(x,y)    { \
    PData mydat; \
    mydat.flags = PROP_LOKTYP; \
    mydat.data.lok = y; \
    set_property(x, MESGPROP_LOCK, &mydat); \
    ts_modifyobject(x); \
}

/**
 * Clears the lock on x
 *
 * Also updates object modification timestamp.
 *
 * @param x the object to clear the lock on
 */
#define CLEARLOCK(x)    { \
    PData mydat; \
    mydat.flags = PROP_LOKTYP; \
    mydat.data.lok = TRUE_BOOLEXP; \
    set_property(x, MESGPROP_LOCK, &mydat); \
    DBDIRTY(x); \
    ts_modifyobject(x); \
}

/**
 * Get the integer value of a given object
 *
 * @param x the object to get the value of
 * @return an integer value
 */
#define GETVALUE(x)     get_property_value(x, MESGPROP_VALUE)

/**
 * Set the value of a given object
 *
 * @param x the object ot set the value of
 * @param y the integer value to set
 */
#define SETVALUE(x,y)   add_property(x, MESGPROP_VALUE, NULL, y)

#define DB_PARMSINFO     0x0001 /**< legacy database value */

#define TYPE_ROOM           0x0 /**< room bit */
#define TYPE_THING          0x1 /**< thing bit */
#define TYPE_EXIT           0x2 /**< exit bit */
#define TYPE_PLAYER         0x3 /**< player bit */
#define TYPE_PROGRAM        0x4 /**< program bit */

/* 0x5 available */

#define TYPE_GARBAGE        0x6 /**< garbage bit */
#define NOTYPE              0x7 /**< no particular type */
#define TYPE_MASK           0x7 /**< bitmask for all types */

/* 0x8 available */

#define WIZARD             0x10 /**< gets automatic control */
#define LINK_OK            0x20 /**< anybody can link to this */
#define DARK               0x40 /**< contents of room are not printed */
#define INTERNAL           0x80 /**< internal-use-only flag */
#define STICKY            0x100 /**< this object goes home when dropped */
#define BUILDER           0x200 /**< this player can use construction commands */
#define CHOWN_OK          0x400 /**< this object can be \@chowned, or
                                 *   this player can see color
                                 */
#define JUMP_OK           0x800 /**< A room which can be jumped from, or
                                 *   a player who can be jumped to
                                 */
/* 0x1000 available */
/* 0x2000 available */

#define KILL_OK          0x4000 /**< Kill_OK bit.  Means you can be killed. */
#define GUEST            0x8000 /**< Guest flag */
#define HAVEN           0x10000 /**< can't kill here */
#define ABODE           0x20000 /**< can set home here */
#define MUCKER          0x40000 /**< programmer */
#define QUELL           0x80000 /**< When set, wiz-perms are turned off */
#define SMUCKER        0x100000 /**< second programmer bit.  For levels */
#define INTERACTIVE    0x200000 /**< internal: player in MUF editor */
#define OBJECT_CHANGED 0x400000 /**< internal: set when an object is dbdirty()ed */

/* 0x800000 available */

#define VEHICLE       0x1000000 /**< Vehicle flag */
#define ZOMBIE        0x2000000 /**< Zombie flag */
#define LISTENER      0x4000000 /**< internal: listener flag */
#define XFORCIBLE     0x8000000 /**< externally forcible flag */
#define READMODE     0x10000000 /**< internal: when set, player is in a READ */
#define SANEBIT      0x20000000 /**< internal: used to check db sanity */
#define YIELD        0x40000000 /**< Yield flag */
#define OVERT        0x80000000 /**< Overt flag */

/** what flags to NOT dump to disk. */
#define DUMP_MASK   (INTERACTIVE | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)

/**
 * Returns the TYPE_ value of 'x'
 *
 * Does not check to see if 'x' is a valid ref.
 *
 * @param x the db object to get type of
 * @return a type valud - one of the TYPE_* constants
 */
#define Typeof(x)   (x == HOME ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))

#define GOD         ((dbref)  1)    /**< head player */
#define NOTHING     ((dbref) -1)    /**< null dbref */
#define AMBIGUOUS   ((dbref) -2)    /**< multiple possibilities, for matchers */
#define HOME        ((dbref) -3)    /**< virtual room, represents mover's home */
#define NIL         ((dbref) -4)    /**< do-nothing link, for actions */

/**
 * Check to see if 'd' is a valid dbref but not if it is a valid object
 *
 * This is a protection against segmentation faults as it will make sure taht
 * d is greater than or equal to 0 and less than db_top
 *
 * @param d the ref to check
 * @return boolean true if d is in the valid range, false otherwise
 */
#define ObjExists(d)    ((d) >= 0 && (d) < db_top)

/**
 * Check to see if 'd' is valid reference - either a valid dbref or NOTHING
 *
 * Be careful with this -- NOTHING will cause a segfault if accessed on the
 * db table.
 *
 * @param d the ref to check
 * @return true if d is either NOTHING, or in a valid range. @see ObjExists
 */
#define OkRef(d)        (ObjExists(d) || (d) == NOTHING)

/**
 * Check to see if 'd' is a valid object
 *
 * This is both ObjExists and object is not TYPE_GARBAGE.
 * @see ObjExists
 *
 * @param d the object to check
 * @return true if d is a valid and non-garbage object, false otherwise
 */
#define OkObj(d)        (ObjExists(d) && Typeof(d) != TYPE_GARBAGE)

#ifdef GOD_PRIV
/**
 * Is 'x' player \#1?
 *
 * Does not check if 'x' is a valid ref.
 *
 * @param x the ref to check
 * @return True if x is player \#1
 */
#define God(x)          ((x) == (GOD))
#endif

/**
 * Calculates the "raw" MUCKER level of an object, not counting WIZARD bit
 *
 * This will be a number between 0 and 3 inclusive.  'x' is not sanity
 * checked to be sure it exists.
 *
 * @param x the object to calculate MUCKER level for.
 * @return an integer between 0 and 3 inclusive
 */
#define MLevRaw(x)  (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0))

/**
 * Calculates the MUCKER level of an object, taking into account WIZARD bit
 *
 * This will be a number between 0 and 4 inclusive.  'x' is not sanity checked
 * to be sure it exists.
 *
 * @param x the object to calculate MUCKER level for.
 * @return an integer between 0 and 4 inclusive
 */
#define MLevel(x)   (((FLAGS(x) & WIZARD) && \
                    ((FLAGS(x) & MUCKER) || (FLAGS(x) & SMUCKER)))? 4 : \
                    (((FLAGS(x) & MUCKER)? 2 : 0) + \
                    ((FLAGS(x) & SMUCKER)? 1 : 0)))

/**
 * Priority level -- used for exit matching
 *
 * The higher the priority, the more likely it will match.  This does not
 * check to see if 'x' is a valid object.
 *
 * @param x the ref to get priority for
 * @return a numeric priority level
 */
#define PLevel(x)   ((FLAGS(x) & (MUCKER | SMUCKER))? \
                    (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0) + 1) : \
                    ((FLAGS(x) & ABODE)? 0 : 1))

/**
 * Check to see if object 'x' is dark
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param x the dbref to check
 * @return boolean true if x is dark, false otherwise
 */
#define Dark(x)         ((FLAGS(x) & DARK) != 0)

/**
 * Check to see if object 'x' is an unquelled WIZARD
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param x the dbref to check
 * @return boolean true if x is an unquelled wizard, false otherwise
 */
#define Wizard(x)       ((FLAGS(x) & WIZARD) != 0 && (FLAGS(x) & QUELL) == 0)

/**
 * Check to see if object 'x' is a WIZARD
 *
 * Does not check to see if 'x' is valid first.  Ignores quell flag.
 *
 * @param x the dbref to check
 * @return boolean true if x is a wizard, false otherwise
 */
#define TrueWizard(x)   ((FLAGS(x) & WIZARD) != 0)

/**
 * Check to see if object 'x' is a MUCKER
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param x the dbref to check
 * @return boolean true if x is a MUCKER
 */
#define Mucker(x)       (MLevel(x) != 0)

/**
 * Check to see if object 'x' can build
 *
 * This is BUILDER flag or WIZARD flag.
 * Does not check to see if 'x' is valid first.
 *
 * @param x the dbref to check
 * @return true if x can build
 */
#define Builder(x)      ((FLAGS(x) & (WIZARD|BUILDER)) != 0)

/**
 * Check to see if object 'x' can be linked to
 *
 * This is public linkability, it doesn't check based on the player's flag
 * or ownership of the exit.
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param x the dbref to check
 * @return true if x can be linked to
 */
#define Linkable(x)     ((x) == HOME || \
                        (((Typeof(x) == TYPE_ROOM || Typeof(x) == TYPE_THING) ? \
                        (FLAGS(x) & ABODE) : (FLAGS(x) & LINK_OK)) != 0))

/**
 * Set MUCKER level
 *
 * This sets the MUCKER level of 'x' to 'y'.  y should be 0, 1, 2, or 3
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param x the object to set the MUCKER level of
 * @param y the level to set
 */
#define SetMLevel(x,y)  { FLAGS(x) &= ~(MUCKER | SMUCKER); \
                            if (y>=2) FLAGS(x) |= MUCKER; \
                            if (y%2) FLAGS(x) |= SMUCKER; }


/**
 * Determine if a particular player is a guest
 *
 * ISGUEST determines whether a particular player is a guest, based on the
 * existence of the property MESGPROP_GUEST.  If GOD_PRIV is defined, then only
 * God can bypass the ISGUEST() check.  Otherwise, any TrueWizard can bypass
 * it.  (This is because \@set is blocked from guests, and thus any Wizard who
 * had both MESGPROP_GUEST and QUELL set would be prevented from unsetting
 * their own QUELL flag to be able to clear MESGPROP_GUEST.)
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param x the player to check if they are a guest
 * @return boolean true if x is a guest
 */
#ifdef GOD_PRIV
#define ISGUEST(x)  ((FLAGS(x) & GUEST) && !God(x))
#else /* !defined(GOD_PRIV) */
#define ISGUEST(x)  ((FLAGS(x) & GUEST) && (FLAGS(x) & TYPE_PLAYER) && !TrueWizard(x))
#endif /* GOD_PRIV */

/**
 * This injects a block of code that blocks a guest from using a command
 *
 * Checks to see if 'x' is a guest, and if they are, gives a standardizd
 * error message and causes the function to return.
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param _cmd the command that the guest is being blocked from running
 * @param x the potential guest ref
 */
#define NOGUEST(_cmd,x) \
    if(ISGUEST(x)) {   \
        log_status("Guest %s(#%d) failed attempt to %s.\n",NAME(x),x,_cmd); \
        notifyf_nolisten(x, "Guests are not allowed to %s.\r", _cmd); \
        return; \
    }

/**
 * This injects a block of code that makes sure the force_level isn't too deep
 *
 * In order to make sure infinite force loops don't happen.  If we're in
 * a force loop, this will emit a standard message and then force the
 * function to return.
 *
 * Does not check to see if 'x' is valid first.
 *
 * @param _cmd the command that is being called, used for error messages
 * @param x the ref of the person to be notified on failure.
 */
#define NOFORCE(_cmd, x) \
    if(force_level) {   \
        notifyf_nolisten(x, "You can't use %s from a @force or {force}.\r", _cmd); \
        return; \
    }

/* Mucker levels */
#define MLEV_APPRENTICE   1 /**< MUCKER 1 */
#define MLEV_JOURNEYMAN   2 /**< MUCKER 2 */
#define MLEV_MASTER       3 /**< MUCKER 3 */
#define MLEV_WIZARD       4 /**< MUCKER 4 */

#ifdef GOD_PRIV
# define MLEV_GOD               255 /**< MUCKER level overkill for \#1 */

/**
 * Determines MUCKER level for the purpose of \@tune
 *
 * If GOD_PRIV is on, this gives \#1 an overkill MUCKER level
 *
 * @param player the player to check
 * @return some integer MUCKER level
 */
# define TUNE_MLEV(player)      (God(player) ? MLEV_GOD : MLevel(player))
#else
# define MLEV_GOD               MLEV_WIZARD

/**
 * Determines MUCKER level for the purpose of \@tune
 *
 * @param player the player to check
 * @return some integer MUCKER level
 */
# define TUNE_MLEV(player)      MLevel(player)
#endif

/**
 * This define checks to see if 'x' is a builder and returns the caller if not
 *
 * If 'x' is not a builder, this will output a message using _cmd for
 * details and cause the function to return.
 *
 * @param _cmd string command that 'x' is trying to run
 * @param x the player to check a builder bit for
 */
#define BUILDERONLY(_cmd,x) \
    if(!Builder(x)) {   \
        notifyf_nolisten(x, "Only builders are allowed to %s.\r", _cmd); \
        return; \
    }

/**
 * This define checks to see if 'x' is a MUCKER and returns the caller if not
 *
 * If 'x' is not a MUCKER, this will output a message using _cmd for
 * details and cause the function to return.
 *
 * @param _cmd string command that 'x' is trying to run
 * @param x the player to check a MUCKER bit for
 */
#define MUCKERONLY(_cmd,x) \
    if(!Mucker(x)) {   \
        notifyf_nolisten(x, "Only programmers are allowed to %s.\r", _cmd); \
        return; \
    }

/**
 * This define checks to see if 'x' is a player and returns the caller if not
 *
 * If 'x' is not a player, this will output a message using _cmd for
 * details and cause the function to return.
 *
 * @param _cmd string command that 'x' is trying to run
 * @param x the player to check a player bit for
 */
#define PLAYERONLY(_cmd,x) \
    if(Typeof(x) != TYPE_PLAYER) {   \
        notifyf_nolisten(x, "Only players are allowed to %s.\r", _cmd); \
        return; \
    }

/**
 * This define checks to see if 'x' is a wizard and returns the caller if not
 *
 * If 'x' is not a wizard, this will output a message using _cmd for
 * details and cause the function to return.
 *
 * @param _cmd string command that 'x' is trying to run
 * @param x the player to check a wizard bit for
 */
#define WIZARDONLY(_cmd,x) \
    if(!Wizard(OWNER(x))) {   \
        notifyf_nolisten(x, "You are not allowed to %s.\r", _cmd); \
        return; \
    }

#ifndef GOD_PRIV
/**
 * This define checks to see if 'x' is superuser and returns the caller if not
 *
 * If 'x' is not a superuser, this will output a message using _cmd for
 * details and cause the function to return.
 *
 * @param _cmd string command that 'x' is trying to run
 * @param x the player to check if is a superuser
 */
#define GODONLY(_cmd,x) WIZARDONLY(_cmd, x); PLAYERONLY(_cmd,x)
#else

/**
 * This define checks to see if 'x' is superuser and returns the caller if not
 *
 * If 'x' is not a superuser, this will output a message using _cmd for
 * details and cause the function to return.
 *
 * @param _cmd string command that 'x' is trying to run
 * @param x the player to check if is a superuser
 */
#define GODONLY(_cmd,x) \
    if(!God(x)) {   \
        notifyf_nolisten(x, "You are not allowed to %s.\r", _cmd); \
        return; \
    }
#endif

/**
 * Iterator for traversing an object list
 *
 * Object lists are "connected" via the ->next property on a dbref-based
 * linked list.  This does a for loop to traverse the entire linked list.
 *
 * Use this in place of 'for(...)'
 *
 * @param var the variable to use as an iterator - dbref will be stored here
 * @param first the starting dbref
 */
#define DOLIST(var, first) \
  for ((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)

/**
 * This adds 'thing' to 'locative's list, and sets locative equal to 'thing'
 *
 * To start a new list, use a variable that contains NOTHING as 'locative'
 * To add to contents, push 'thing' into locative 'CONTENTS(target)'.
 *
 * @param thing the thing to push onto the list
 * @param locative the list to add to
 */
#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}

typedef long object_flag_type;  /**< Object flag type - need 32+ bits */

/**
 * Each type of object may have a _specific structure associated with it.
 *
 * This one is for MUF programs.
 */
struct program_specific {
    unsigned short instances;   /**< number of instances of this prog running */
    unsigned short instances_in_primitive;  /**< number instances running a primitive */
    short curr_line;            /**< current-line */
    int siz;                    /**< size of code */
    struct inst *code;          /**< byte-compiled code */
    struct inst *start;         /**< place to start executing */
    struct line *first;         /**< first line */
    struct publics *pubs;       /**< public subroutine addresses */
    struct mcp_binding *mcpbinds;   /**< MCP message bindings. */
    struct timeval proftime;    /**< profiling time spent in this program. */
    time_t profstart;           /**< time when profiling started for this prog */
    unsigned int profuses;      /**< \#calls to this program while profiling */
};

/**
 * Fetch an object's program specific structure
 *
 * Does not validate the type or existance of 'x' first.
 *
 * @param x the program to load program specific structure for
 * @return a struct program_specific
 */
#define PROGRAM_SP(x)           (DBFETCH(x)->sp.program.sp)

/**
 * Allocate a program specific structure for 'x'
 *
 * Does not validate the type or existance of 'x' first.  The structure is
 * automatically set on 'x'.
 *
 * @param x the program to initialize a program specific structure for
 */
#define ALLOC_PROGRAM_SP(x)     { \
    PROGRAM_SP(x) = malloc(sizeof(struct program_specific)); \
}

/**
 * Free a program specific structure for 'x'
 *
 * Does not validate the type or existance of 'x' first.  However, it does
 * check to see if the existing program specific structure is NULL before
 * trying to free it.  Sets the program specific structure to NULL when
 * complete, ensuring cleanliness.
 *
 * @param x the program to free a program specific structure for
 */
#define FREE_PROGRAM_SP(x)      { \
    dbref foo = x; \
    if(PROGRAM_SP(foo)) \
        free(PROGRAM_SP(foo)); \
    PROGRAM_SP(foo) = (struct program_specific *)NULL; \
}

/**
 * Accessor for a program specific field with NULL check
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * We check to make sure the program specific structure is not NULL
 * prior to looking up; NULL structures will return 0 here.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field or 0 if program specific is NULL.
 */
#define PROGRAM_INSTANCES(x)    (PROGRAM_SP(x)!=NULL?PROGRAM_SP(x)->instances:0)

/**
 * Accessor for a program specific field with NULL check
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * We check to make sure the program specific structure is not NULL
 * prior to looking up; NULL structures will return 0 here.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field or 0 if program specific is NULL.
 */
#define PROGRAM_INSTANCES_IN_PRIMITIVE(x)   (PROGRAM_SP(x)!=NULL?PROGRAM_SP(x)->instances_in_primitive:0)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_CURR_LINE(x)    (PROGRAM_SP(x)->curr_line)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_SIZ(x)          (PROGRAM_SP(x)->siz)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_CODE(x)         (PROGRAM_SP(x)->code)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_START(x)        (PROGRAM_SP(x)->start)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_FIRST(x)        (PROGRAM_SP(x)->first)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_PUBS(x)         (PROGRAM_SP(x)->pubs)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_PROFTIME(x)     (PROGRAM_SP(x)->proftime)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_PROFSTART(x)    (PROGRAM_SP(x)->profstart)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_PROF_USES(x)    (PROGRAM_SP(x)->profuses)

/**
 * Increment the program instance count
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to increment
 * @return the resultant value
 */ 
#define PROGRAM_INC_INSTANCES(x)    (PROGRAM_SP(x)->instances++)

/**
 * Decrement the program instance count
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to decrement
 * @return the resultant value
 */ 
#define PROGRAM_DEC_INSTANCES(x)    (PROGRAM_SP(x)->instances--)

/**
 * Increment the program instance in primitive count
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to increment
 * @return the resultant value
 */ 
#define PROGRAM_INC_INSTANCES_IN_PRIMITIVE(x)   (PROGRAM_SP(x)->instances_in_primitive++)

/**
 * Decrement the program instance in primitive count
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to decrement
 * @return the resultant value
 */ 
#define PROGRAM_DEC_INSTANCES_IN_PRIMITIVE(x)   (PROGRAM_SP(x)->instances_in_primitive--)

/**
 * Increment the prof uses count
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to increment
 * @return the resultant value
 */ 
#define PROGRAM_INC_PROF_USES(x)    (PROGRAM_SP(x)->profuses++)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_INSTANCES(x,y)  (PROGRAM_SP(x)->instances = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_INSTANCES_IN_PRIMITIVE(x,y) (PROGRAM_SP(x)->instances_in_primitive = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_CURR_LINE(x,y)  (PROGRAM_SP(x)->curr_line = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_SIZ(x,y)        (PROGRAM_SP(x)->siz = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_CODE(x,y)       (PROGRAM_SP(x)->code = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_START(x,y)      (PROGRAM_SP(x)->start = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_FIRST(x,y)      (PROGRAM_SP(x)->first = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_PUBS(x,y)       (PROGRAM_SP(x)->pubs = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set (tv_sec)
 * @param z the value to set (tv_usec)
 * @return the contents of the field
 */
#define PROGRAM_SET_PROFTIME(x,y,z) (PROGRAM_SP(x)->proftime.tv_usec = z, PROGRAM_SP(x)->proftime.tv_sec = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_PROFSTART(x,y)  (PROGRAM_SP(x)->profstart = y)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_PROF_USES(x,y)  (PROGRAM_SP(x)->profuses = y)

/**
 * Accessor for a program specific field
 *
 * The field accessed is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @return the contents of the field
 */
#define PROGRAM_MCPBINDS(x)         (PROGRAM_SP(x)->mcpbinds)

/**
 * Setter for a program specific field
 *
 * The field set is obviously named as the section after PROGRAM_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not check for nulls, so it will segfault if the program specific
 * pointer is NULL.
 *
 * @param x the program to fetch the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PROGRAM_SET_MCPBINDS(x,y)   (PROGRAM_SP(x)->mcpbinds = y)

/**
 * Players an things share the same _specific structure at this time.
 * Probably to help support zombies.
 */
struct player_specific {
    dbref home;             /**< Home location */
    dbref curr_prog;        /**< program I'm currently editing */
    short insert_mode;      /**< in insert mode? */
    short block;            /**< Is the player 'blocked' by a MUF program? */
    const char *password;   /**< Player password */
    int *descrs;            /**< List if descriptors */
    int descr_count;        /**< Number of descriptors */
    dbref *ignore_cache;    /**< People the player is ignoring */
    int ignore_count;       /**< Number of entries in the ignore cahche */
    dbref ignore_last;      /**< The last DBREF in the ignore cache */
};

/**
 * Fetch an object's thing specific structure
 *
 * Does not validate the type or existance of 'x' first.
 *
 * @param x the thing to load thing specific structure for
 * @return a struct player_specific
 */
#define THING_SP(x)             (DBFETCH(x)->sp.player.sp)

/**
 * Allocate a thing specific structure for 'x'
 *
 * Does not validate the type or existance of 'x' first.  The structure is
 * automatically set on 'x'.
 *
 * @param x the thing to initialize a thing specific structure for
 */
#define ALLOC_THING_SP(x)       { \
    PLAYER_SP(x) = malloc(sizeof(struct player_specific)); \
}

/**
 * Free a thing specific structure for 'x'
 *
 * Does not validate the type or existance of 'x' first.  However, it does
 * check to see if the existing thing specific structure is NULL before
 * trying to free it.  Sets the thing specific structure to NULL when
 * complete, ensuring cleanliness.
 *
 * @param x the thing to free a thing specific structure for
 */
#define FREE_THING_SP(x)        { \
    dbref foo = x; \
    free(PLAYER_SP(foo)); \
    PLAYER_SP(foo) = NULL; \
}

/**
 * Accessor for a thing specific field
 *
 * The field accessed is obviously named as the section after THING_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to fetch the field for
 * @return the contents of the field
 */
#define THING_HOME(x)           (PLAYER_SP(x)->home)

/**
 * Setter for a thing specific field
 *
 * The field accessed is obviously named as the section after THING_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define THING_SET_HOME(x,y)     (PLAYER_SP(x)->home = y)

/**
 * Fetch an object's player specific structure
 *
 * Does not validate the type or existance of 'x' first.
 *
 * @param x the player to load thing specific structure for
 * @return a struct player_specific
 */
#define PLAYER_SP(x)            (DBFETCH(x)->sp.player.sp)

/**
 * Allocate a player specific structure for 'x'
 *
 * Does not validate the type or existance of 'x' first.  The structure is
 * automatically set on 'x'.  This also clears the memory of the newly
 * allocated structure.
 *
 * @param x the thing to initialize a player specific structure for
 */
#define ALLOC_PLAYER_SP(x)      { \
    PLAYER_SP(x) = malloc(sizeof(struct player_specific)); \
    memset(PLAYER_SP(x),0,sizeof(struct player_specific)); \
}

/**
 * Free a player specific structure for 'x'
 *
 * Does not validate the type or existance of 'x' first.  However, it does
 * check to see if the existing player specific structure is NULL before
 * trying to free it.  Sets the player specific structure to NULL when
 * complete, ensuring cleanliness.
 *
 * @param x the thing to free a player specific structure for
 */
#define FREE_PLAYER_SP(x)       { \
    dbref foo = x; \
    free(PLAYER_SP(foo)); \
    PLAYER_SP(foo) = NULL; \
}

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_HOME(x)          (PLAYER_SP(x)->home)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_CURR_PROG(x)     (PLAYER_SP(x)->curr_prog)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_INSERT_MODE(x)   (PLAYER_SP(x)->insert_mode)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_BLOCK(x)         (PLAYER_SP(x)->block)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_PASSWORD(x)      (PLAYER_SP(x)->password)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_DESCRS(x)        (PLAYER_SP(x)->descrs)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_DESCRCOUNT(x)    (PLAYER_SP(x)->descr_count)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_IGNORE_CACHE(x)  (PLAYER_SP(x)->ignore_cache)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_IGNORE_COUNT(x)  (PLAYER_SP(x)->ignore_count)

/**
 * Accessor for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the player to fetch the field for
 * @return the contents of the field
 */
#define PLAYER_IGNORE_LAST(x)   (PLAYER_SP(x)->ignore_last)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_HOME(x,y)        (PLAYER_SP(x)->home = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_CURR_PROG(x,y)   (PLAYER_SP(x)->curr_prog = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_INSERT_MODE(x,y) (PLAYER_SP(x)->insert_mode = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_BLOCK(x,y)       (PLAYER_SP(x)->block = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_PASSWORD(x,y)    (PLAYER_SP(x)->password = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_DESCRS(x,y)      (PLAYER_SP(x)->descrs = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_DESCRCOUNT(x,y)  (PLAYER_SP(x)->descr_count = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_IGNORE_CACHE(x,y)    (PLAYER_SP(x)->ignore_cache = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_IGNORE_COUNT(x,y)    (PLAYER_SP(x)->ignore_count = y)

/**
 * Setter for a player specific field
 *
 * The field accessed is obviously named as the section after PLAYER_
 * Forgive the generic comment -- there are a million of these and they
 * all work the same.
 *
 * This does not do NULL checking, so it will segfault if the underlying
 * specific structure is NULL.
 *
 * @param x the thing to set the field for
 * @param y the value to set
 * @return the contents of the field
 */
#define PLAYER_SET_IGNORE_LAST(x,y)     (PLAYER_SP(x)->ignore_last = y)

#ifndef ARRAYSIZE
/**
 * Calculate the number of elements in an array
 *
 * @param a the array to get an element count of
 * @return the element count
 */
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))
#endif

/* system parameter types */
#define TP_TYPE_STRING   0  /**< String tune parmaeter type */
#define TP_TYPE_TIMESPAN 1  /**< Time span tune parameter type */
#define TP_TYPE_INTEGER  2  /**< Integer tune parameter type */
#define TP_TYPE_DBREF    3  /**< DBREF tune parameter type */
#define TP_TYPE_BOOLEAN  4  /**< Boolean tune parameter type */

/**
 * union of type-specific fields
 */
union specific {        /* I've been railroaded! */
    struct {
        dbref dropto;   /**< Drop-to location */
    } room;             /**< ROOM-specific fields */

    struct {
        int ndest;      /**< Number of destinations */
        dbref *dest;    /**< The destination array*/
    } exit;             /**< EXIT-specific fields */

    struct {
        struct player_specific *sp; /** Player/thing specific pointer */
    } player;           /**< PLAYER- and THING- specific fields */

    struct {
        struct program_specific *sp;    /** Program specific pointer */
    } program;          /**< PROGRAM-specific fields */
};

/**
 * Database object
 */
struct object {
    const char *name;   /**< Object name */
    dbref location;     /**< pointer to container */
    dbref owner;        /**< Object owner */
    dbref contents;     /**< Head of the object's contents db list */
    dbref exits;        /**< Head of the object's exits db list */
    dbref next;         /**< pointer to next in contents/exits chain */
    struct plist *properties;   /**< Root of properties tree */
#ifdef DISKBASE
    long propsfpos;     /**< File position for properties in the DB file */
    time_t propstime;   /**< Last time props were used */
    dbref nextold;      /**< Ringqueue for diskbase next db */
    dbref prevold;      /**< Ringqueue for diskbase previous db */
    short propsmode;    /**< State of the props - PROPS_UNLOADED, PROPS_CHANGED */
    short spacer;       /**< Not used by anything */
#endif
    object_flag_type flags;         /**< Object flags */
    unsigned int mpi_prof_use;      /**< MPI profiler number of uses */
    struct timeval mpi_proftime;    /**< Time spent running MPI */
    time_t ts_created;              /**< Created time */
    time_t ts_modified;             /**< Modified time */
    time_t ts_lastused;             /**< Last used time */
    int ts_usecount;                /**< Usage counter */
    union specific sp;              /**< Per-type specific structure */
};

/**
 * Calculate the initial value of a THING
 *
 * @param cost the amount spent to create the THING
 * @return the value that should be set on the THING
 */
#define OBJECT_ENDOWMENT(cost)  (((cost)-5)/5)

/**
 * Calculate the cost of a THING, given an endowment
 *
 * This is basically the reverse of OBJECT_ENDOWMENT and is used by clone to
 * determine clone cost.  @see OBJECT_ENDOWMENT
 *
 * @param endow the endowment to convert to cost
 * @return the cost associated with the endowment.
 */
#define OBJECT_GETCOST(endow)   ((endow)*5+5)

/**
 * This structure is used for MUFTOPS to calculate statistics about running
 * programs.
 */
struct profnode {
    struct profnode *next;  /**< Linked list implementation */
    dbref prog;             /**< Program DBREF */
    double proftime;        /**< Profile time tracking */
    double pcnt;            /**< Percentage */
    time_t comptime;        /**< Profile length (how long its been profiled) */
    long usecount;          /**< Usage count */
    short type;             /**< If true, MUF.  If false, MPI */
};

/**
 * This is for making an arbitrary linked list of objects.  Such as
 * for the forcelist.
 */
typedef struct objnode {
    dbref data;             /**< Data for the node */
    struct objnode *next;   /**< Linked list */
} objnode;

/*
 * These (particularly db) are used all over the place.  Very important
 * externs.
 */

/**
 * @var db
 *      the array of objects that is our DB
 */
extern struct object *db;

/**
 * @var forcelist
 *      the things currently being forced.
 */
extern objnode *forcelist;

/**
 * @var db_top
 *      the 'top' of the database; it is the highest dbref + 1
 */
extern dbref db_top;

/**
 * @var recyclable
 *      the head of the garbage dbref list -- recycle-able objects
 *                  This may be NOTHING.
 */
extern int recyclable;

/**
 * Determine if 'who' controls object 'what'
 *
 * The logic here is relatively simple.  If 'what' is invalid, return
 * false.  If 'who' is a wizard, return true (with GOD_PRIV checking).
 *
 * Next, we check for realms; if a room is set 'W', then the owner of
 * that room controls everything under that room's environment except
 * other players.
 *
 * Then, we check if who owns what.
 *
 * Lastly, we check the own lock.
 *
 * @param who the player we are checking for control
 * @param what the object we are checking to see if player controls
 * @return boolean true if who controls what, false otherwise.
 */
int controls(dbref who, dbref what);

/**
 * Determine if who controls the ability to manipulate the link of what
 *
 * For unlinking, this determines if 'who' is able to unlink 'what'.
 * For linking, this logic is only applied if 'what' is already
 * determined to be an exit -- if you aren't careful with this, you can
 * allow someone to arbitrarily re-home other players that live in
 * a room that someone owns, for instance.
 *
 * @param who the person doing the link/unlink
 * @param what the object being linked/unlinked
 * @return boolean true if who may link/unlink what
 */
int controls_link(dbref who, dbref what);

/**
 * Allocate an EXIT type object
 *
 * With a given owner, name, and source (location).  Returns the DBREF
 * of the exit.  The 'name' memory is copied.
 *
 * @param player the owner dbref
 * @param name the name of the new exit
 * @param source the location to put the exit
 * @return the dbref of the new exit
 */
dbref create_action(dbref player, const char *name, dbref source);

/**
 * Allocate an PROGRAM type object
 *
 * With a given name and owner/creator.  Returns the DBREF of the program. 
 * The 'name' memory is copied.  Initializes the "special" fields.
 *
 * @param name the name of the new exit
 * @param player the owner dbref
 * @return the dbref of the new program
 */
dbref create_program(dbref player, const char *name);

/**
 * Allocate an ROOM type object
 *
 * With a given owner/creator, name, and parent (location).  Returns the DBREF
 * of the room.   The 'name' memory is copied.  Initializes the "special"
 * fields for the room.
 *
 * If the player is JUMP_OK, then the created room will be JUMP_OK as well.
 *
 * @param player the owner dbref
 * @param name the name of the new room
 * @param parent the parent room's dbref
 * @return the dbref of the new room
 */
dbref create_room(dbref player, const char *name, dbref parent);

/**
 * Allocate an THING type object
 *
 * With a given owner/creator, name, and location.  Returns the DBREF
 * of the thing.   The 'name' memory is copied.  Initializes the "special"
 * fields for the thing.
 *
 * The home is set to the current room if the player controls the room;
 * otherwise the home is set to the player.
 *
 * @param player the owner dbref
 * @param name the name of the new thing
 * @param location the location to place the object
 * @return the dbref of the new thing
 */
dbref create_thing(dbref player, const char *name, dbref location);

/**
 * Clones a thing.
 *
 * @param thing the thing to clone
 * @param player the player for determining the cloned thing's home
 * @param copy_hidden_props if true, this copies hidden properties
 * @return the dbref of the cloned thing
 */
dbref clone_thing(dbref thing, dbref player, int copy_hidden_props);

/**
 * Clear a DB object
 *
 * This will take a ref and zero out the object, making it suitable for
 * use (or reuse, in the case of recycled garbage).
 *
 * This does NOT clear out memory, so you SHOULD NOT use clear out an
 * object with allocated memory -- this should only be used with objects
 * that are brand new (i.e. freshly allocated) or that have been totally
 * cleaned out already (garbage).
 *
 * Flags must be initialized after this is called.  Also, type-specific
 * fields must also be initialized.  The object is not set dirty by this.
 *
 * @param i the dbref of the object to clear out.
 */
void db_clear_object(dbref i);

/**
 * Free the memory for the whole database
 *
 * This also runs clear_players and clear_primitives
 *
 * @see clear_players
 * @see clear_primitives
 */
void db_free(void);

/**
 * Free the memory associated with an object
 *
 * This does not shrink 'db', but it frees all the different pointers
 * used by an object -- name, "special" structure, program code for programs,
 * etc.
 *
 * If this is a program, it is uncompiled first.  @see uncompile_program
 *
 * Not a lot of checking is done, so it would be pretty easy to double-free
 * an object if you are not careful; mutating the object into a TYPE_GARBAGE
 * after it has been free'd will render it fairly safe.
 *
 * @param i the dbref of the object to free memory for.
 */
void db_free_object(dbref i);

/**
 * Read the FuzzBall DB from the given file handle.
 *
 * Returns the dbtop value, or -1 if the header is invalid.  If
 * there is a problem loading the database, chances are it will trigger
 * an abort() as there is no gentle error handling in this process.
 *
 * @param f the file handle to load from
 * @return the dbtop value or #-1 if the header is invalid
 */
dbref db_read(FILE * f);

/**
 * Write the database out to a given file handle
 *
 * The file format is the header followed by the objects in reverse
 * order (highest number first).  If you wish to see the file format
 * information, see the db_write_header and db_write_object function
 * calls in db.c.
 *
 * The dump ends with ***END OF DUMP***
 *
 * If there is an error writing, this abort()s the program.
 *
 * @param f the file handle to write to
 * @return db_top value
 */
dbref db_write(FILE * f);

/**
 * Calculate the environmental distance between 'from' and 'to'.
 *
 * This is used in matching to try and find what potential match is
 * more relevant.  The parent of 'to' is found, and then we find the
 * number of parents between 'from' and 'to'.
 *
 * If from is the parent of to, this will return 0.  Otherwise, it
 * returns the number of hops between 'from' and 'to's parent; if
 * 'from' isn't a descendant of 'to', then the returned number will
 * be the number of hops from 'from' to #0.
 *
 * @see getparent
 *
 * @param from the starting reference
 * @param to the destination reference
 * @return the number of hops between 'from' and 'to's parent.
 */
int env_distance(dbref from, dbref to);

/**
 * Get the parent for a given object
 *
 * This logic is weirdly complicated.  If tp_thing_movement is true, which
 * is to say, moving things is set to act like a player is moving, then
 * this simply returns the location of the object.
 *
 * Otherwise, we iterate.  The complexity is around vehicle things;
 * if a THING is set vehicle, then its parent room is either the vehicle's
 * home, or the vehicle's home is a player, then the player's home.
 *
 * The exact logic here is difficult to compress into a little text blurb,
 * but basically we find the next valid room to be our parent or default
 * to GLOBAL_ENVIRONMENT in a pinch.
 *
 * @param obj the object to get the parent for
 * @return the parent database reference
 */
dbref getparent(dbref obj);

/**
 * Load properties from the file handle
 *
 * This assumes that the file pointer is in the correct position to
 * load properties for object 'obj' *unless* DISKBASE is true, in which
 * case we will seek to the correct position first.
 *
 * This call assumes the first '*' has been nibbled already, and if the
 * line 'Props*' remains in the file stream we will use the "new" prop
 * loader.  @see db_getprops
 *
 * Otherwise, it uses the "old" prop load process which is the bulk
 * of this function.  The "old" prop load function only supports integer
 * and string loads.
 *
 * @param f the file handle to load from
 * @param obj the object to load props for
 * @param pdir the root prop directory to load into.  This can be NULL for
 *        an initial prop load.  It is used for diskbase prop loading.
 */
void getproperties(FILE * f, dbref obj, const char *pdir);

/**
 * Get a reference from the given file handle
 *
 * @param f the file handle to get a reference from
 * @return the dbref
 */
dbref getref(FILE * f);

/**
 * This routine connects an exit to (potentially) a bunch of destinations.
 *
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of destinations
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.  It is assumed to be MAX_LINKS in size.
 *
 * The destinations are delimited by ; if doing multiple.
 *
 * A lot of different checks are done to make sure the exit doesn't
 * make a loop, permissions are done correctly, and that multiple
 * links are only allowed for EXITS and THINGS.
 *
 * @param descr the descriptor of the player
 * @param player the player doing the linkage
 * @param exit the ref of the exit we are linking
 * @param dest_name the destination or destinations
 * @param dest_list the array to store ref dbrefs, MAX_LINKS in size
 * @return the count of destinations linked.
 */
int link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list);

/**
 * A "dry run" version of link_exit to check if we can do the links
 *
 * The underpinnings of this are identical to link_exits.  However,
 * no linking is done.  Instead, if there would have been errors,
 * this will display error messages and return 0.  If it would have
 * been successful, this will return the number of links that
 * would have been made and dest_list will be populated.
 *
 * For more documentation and deatils, @see link_exit
 *
 * @param descr the descriptor of the player
 * @param player the player doing the linkage
 * @param exit the ref of the exit we are linking
 * @param dest_name the destination or destinations
 * @param dest_list the array to store ref dbrefs, MAX_LINKS in size
 * @return the count of destinations linked.
 */
int link_exit_dry(int descr, dbref player, dbref exit, char *dest_name,
                         dbref * dest_list);

/**
 * Check to see if 'thing' is on ref-list 'list' or any of 'list's contents
 *
 * This is used by boolexp to see if 'thing' has any relation to 'list'.
 * It scans 'list', then recursively scans every item in 'list'.
 *
 * @param thing the thing to check for
 * @param list the object to scan to see if 'thing' is on its next list
 * @return boolean true if found, false if not
 */
int member(dbref thing, dbref list);

/**
 * Return an object ready to use.
 *
 * This may be a recycled garbage object or a new object.  Either way,
 * it will be empty and ready for use.
 *
 * @param isplayer boolean true if the object is to be a player, false
 *       otherwise
 * @return the database ref of the object
 */
dbref new_object(bool isplayer);

/**
 * Find a dbref in an objnode list
 *
 * @param head the head of the objnode list
 * @param data the dbref to look for
 * @return boolean true if dbref is on the list, false if not.
 */
int objnode_find(objnode *head, dbref data);

/**
 * Push a dbref onto an objnode
 *
 * The new ref is put at the head of the objnode list, which is why
 * we need a pointer to a pointer here.
 *
 * @param head pointer to pointer to head of objnode list.
 * @param data the dbref to add to the head of the list.
 */
void objnode_push(objnode **head, dbref data);

/**
 * Pop a dbref off the objnode list.
 *
 * If head is NULL, nothing happens.  Removes the node, frees the memory,
 * and returns the pop'd dbref.
 *
 * @param head pointer to pointer of list to pop from.
 * @return the dbref from the node just removed
 */
dbref objnode_pop(objnode **head);

/**
 * Validates that the name is appropriate for the given object type.
 *
 * * Names cannot start with !, *, #, or $
 * * Names cannot contain the characters =, &, |, or carriage return/escape.
 * * The name cannot be the strings 'me', 'here', or 'home'
 * * The name cannot match 'tp_reserved_names'
 * * Players must pass ok_player_name.
 * * Things must pass ok_ascii_any if 7bit_thing_names is on.
 * * All other objects must pass ok_ascii_any if 7bit_other_names is on.
 *
 * @see ok_ascii_any
 * @see ok_player_name
 *
 * @param name the name to check
 * @param type the type of the object, as diffent types have different rules
 * @return boolean true if name passes the check, false otherwise
 */
bool ok_object_name(const char *name, object_flag_type type);

/**
 * Write a ref to the given file handle
 *
 * This will abort the program if the write fails.  A newline character is
 * also written along with the ref.
 *
 * @param f the file to write to
 * @param ref the reference to write
 */
void putref(FILE * f, dbref ref);

/**
 * Write properties to a given file handle
 *
 * This injects the *Props* and *End* delimiters that mark the start and
 * end of the block of properties in the database.
 *
 * @param f the file to write to
 * @param obj the object to dump props for
 */
void putproperties(FILE * f, dbref obj);

/**
 * Write a string to the given file handle
 *
 * This will abort the program if the write fails.  A newline character is
 * also written along with the string.
 *
 * @param f the file to write to
 * @param s the string to write
 */
void putstring(FILE * f, const char *s);

/**
 * Register an object ref on the given location in the given propdir
 *
 * 'name' will be concatenated onto propdir with a / delimiter.
 * propdir is usually either _reg or a propqueue.  The object dbref is
 * stored as a ref-style property on the object.
 *
 * Displays appropriate messages; if the reference previously existed,
 * its old value is displayed.
 *
 * If object is NOTHING, then the property is cleared.  Permissions
 * are not checked.
 *
 * @param player the player doing the registration
 * @param location the location to do the registration on
 * @param propdir the prop dir to register into
 * @param name the registration name
 * @param object the object to register
 */
void register_object(dbref player, dbref location, const char *propdir, char *name, dbref object);

/**
 * Remove something from the "next" dbref list
 *
 * This is used, for instance, to remove something from the contents
 * list of a room.  dbref list refers to the dbref-linked list structure
 * that is used by objects (ala the NEXT prim in MUF)
 *
 * @param first the object to find 'what' in
 * @param what the object to find
 * @return either 'first' or first's NEXTOBJ if first == what
 */
dbref remove_first(dbref first, dbref what);

/**
 * This reverses a dbref linked list
 *
 * Basically it changes all the links around so that what was last will
 * now be first on the list.
 *
 * @param list the list to reverse
 * @return the dbref that is the start of the reversed list
 */
dbref reverse(dbref list);

/**
 * Set the source of an action -- basically move 'action' to 'source'
 *
 * This is the proper way to 'move' an action from one location to another,
 * updating the action's location and the source's exit list.
 *
 * This is only for exits; no error checking is done, so be careful with it.
 *
 * If you are moving an exit, you should unset_source(action) first to remove
 * it from its current exit linked list.  If you don't do that, you'll
 * probably corrupt the DB.
 *
 * @see unset_source
 *
 * @param action the action to move
 * @param source the destination to move it to
 */
void set_source(dbref action, dbref source);

/**
 * Calculate the memory size of an object
 *
 * This is an estimated size of the object as stored on disk.
 * Either that, or it is an inaccurate calculation of memory usage
 * since it doesn't take into account the memory allocated for
 * the 'specific' structures nor the memory allocated for player
 * descriptor lists and ignore cache.
 *
 * Looking at its usage in the code, it appears to be in-memory size,
 * in which case this calculation is inaccurate.
 *
 * Note that the 'load' parameter doesn't have any impact if you are
 * not using DISKBASE.
 *
 * @param i the object to calculate size for
 * @param load boolean if true load props before calculating size.
 * @return the calculated size of the object.
 */
size_t size_object(dbref i, int load);

/**
 * "Unparses" flags, or rather, gives a string representation of object flags
 *
 * This uses a static buffer, so make sure to copy it if you want to keep
 * it.  This is, of course, not threadsafe.
 *
 * @param thing the object to construct a flag string for
 * @return the constructed flag string in a static buffer
 */
const char *unparse_flags(dbref thing);

/**
 * "Unparse" an object, showing itsnam and list of flags if permissions allow
 *
 * Uses the provided buffer that has the given size.  Practically speaking,
 * names can be as long as BUFFER_LEN, so your buffer should probably
 * be BUFFER_LEN at least in size (This is the most common practice).  If
 * a name was actually its maximum length, then there is not enough room
 * for flags to show up.  Traditionally, Fuzzball has not cared about this
 * problem because, traditionally, names just don't get that long.
 *
 * Flags are only shown if:
 *
 * * player == NOTHING
 * * or player does not have STICKY flag AND:
 *   * 'loc' is linkable - @see can_link_to
 *   * or 'loc' is not a player and 'player' controls 'loc'
 *   * or 'loc' is CHOWN_OK
 *
 * @param player the player doing the call or NOTHING
 * @param object the target to generate unparse text for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void unparse_object(dbref player, dbref object, char *buffer, size_t size);

/**
 * Remove the action from its current location's exit list
 *
 * This doesn't alter action's location, so after calling this, the
 * action is in weird limbo.  You should do something with it,
 * such as recycle it or use set_source to give it a new location.
 *
 * @see set_source
 *
 * @param action the action to remove from its location's exist list
 */
void unset_source(dbref action);

#endif /* !DB_H */
