/** @file db.h
 *
 * Header for declaring database functions and DB-specific #defines.
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

/* This is used to identify the database dump file */
#define DB_VERSION_STRING "***Foxen9 TinyMUCK DUMP Format***"

#define GLOBAL_ENVIRONMENT ((dbref) 0)  /* parent of all rooms (always #0) */

extern char match_args[BUFFER_LEN];
extern char match_cmdname[BUFFER_LEN];

#define DBFETCH(x)  (db + (x))
#define DBDIRTY(x)  {db[x].flags |= OBJECT_CHANGED;}

#define DBSTORE(x, y, z)    {DBFETCH(x)->y = z; DBDIRTY(x);}

#define NAME(x)     (db[x].name)
#define FLAGS(x)    (db[x].flags)
#define OWNER(x)    (db[x].owner)
#define LOCATION(x) (DBFETCH((x))->location)
#define CONTENTS(x) (DBFETCH((x))->contents)
#define EXITS(x)    (DBFETCH((x))->exits)
#define NEXTOBJ(x)  (DBFETCH((x))->next)

/* defines for possible data access mods. */
#define MESGPROP_DESC       "_/de"
#define MESGPROP_IDESC      "_/ide"
#define MESGPROP_SUCC       "_/sc"
#define MESGPROP_OSUCC      "_/osc"
#define MESGPROP_FAIL       "_/fl"
#define MESGPROP_OFAIL      "_/ofl"
#define MESGPROP_DROP       "_/dr"
#define MESGPROP_ODROP      "_/odr"
#define MESGPROP_DOING      "_/do"
#define MESGPROP_OECHO      "_/oecho"
#define MESGPROP_PECHO      "_/pecho"
#define MESGPROP_LOCK       "_/lok"
#define MESGPROP_FLOCK      "@/flk"
#define MESGPROP_CONLOCK    "_/clk"
#define MESGPROP_CHLOCK     "_/chlk"
#define MESGPROP_LINKLOCK   "_/lklk"
#define MESGPROP_READLOCK   "@/rlk"
#define MESGPROP_OWNLOCK    "@/olk"
#define MESGPROP_PCON       "_/pcon"
#define MESGPROP_PDCON      "_/pdcon"
#define MESGPROP_VALUE      "@/value"

#define GETMESG(x,y)    (get_property_class(x, y))
#define GETDESC(x)      GETMESG(x, MESGPROP_DESC)
#define GETIDESC(x)     GETMESG(x, MESGPROP_IDESC)
#define GETSUCC(x)      GETMESG(x, MESGPROP_SUCC)
#define GETOSUCC(x)     GETMESG(x, MESGPROP_OSUCC)
#define GETFAIL(x)      GETMESG(x, MESGPROP_FAIL)
#define GETOFAIL(x)     GETMESG(x, MESGPROP_OFAIL)
#define GETDROP(x)      GETMESG(x, MESGPROP_DROP)
#define GETODROP(x)     GETMESG(x, MESGPROP_ODROP)
#define GETDOING(x)     GETMESG(x, MESGPROP_DOING)
#define GETOECHO(x)     GETMESG(x, MESGPROP_OECHO)
#define GETPECHO(x)     GETMESG(x, MESGPROP_PECHO)

#define SETMESG(x,y,z)  {add_property(x, y, z, 0);ts_modifyobject(x);}
#define SETDESC(x,y)    SETMESG(x, MESGPROP_DESC, y)

#define GETLOCK(x)      (get_property_lock(x, MESGPROP_LOCK))
#define SETLOCK(x,y)    {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property(x, MESGPROP_LOCK, &mydat, 0); ts_modifyobject(x); }
#define CLEARLOCK(x)    {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = TRUE_BOOLEXP; set_property(x, MESGPROP_LOCK, &mydat, 0); DBDIRTY(x); ts_modifyobject(x);}

#define GETVALUE(x)     get_property_value(x, MESGPROP_VALUE)
#define SETVALUE(x,y)   add_property(x, MESGPROP_VALUE, NULL, y)

#define DB_PARMSINFO     0x0001 /* legacy database value */

#define TYPE_ROOM           0x0
#define TYPE_THING          0x1
#define TYPE_EXIT           0x2
#define TYPE_PLAYER         0x3
#define TYPE_PROGRAM        0x4
/* 0x5 available */
#define TYPE_GARBAGE        0x6
#define NOTYPE              0x7 /* no particular type */
#define TYPE_MASK           0x7
/* 0x8 available */
#define WIZARD             0x10 /* gets automatic control */
#define LINK_OK            0x20 /* anybody can link to this */
#define DARK               0x40 /* contents of room are not printed */
#define INTERNAL           0x80 /* internal-use-only flag */
#define STICKY            0x100 /* this object goes home when dropped */
#define BUILDER           0x200 /* this player can use construction commands */
#define CHOWN_OK          0x400 /* this object can be @chowned, or
                                   this player can see color */
#define JUMP_OK           0x800 /* A room which can be jumped from, or
                                 * a player who can be jumped to */
/* 0x1000 available */
/* 0x2000 available */
#define KILL_OK          0x4000 /* Kill_OK bit.  Means you can be killed. */
#define GUEST            0x8000 /* Guest flag */
#define HAVEN           0x10000 /* can't kill here */
#define ABODE           0x20000 /* can set home here */
#define MUCKER          0x40000 /* programmer */
#define QUELL           0x80000 /* When set, wiz-perms are turned off */
#define SMUCKER        0x100000 /* second programmer bit.  For levels */
#define INTERACTIVE    0x200000 /* internal: player in MUF editor */
#define OBJECT_CHANGED 0x400000 /* internal: set when an object is dbdirty()ed */
/* 0x800000 available */
#define VEHICLE       0x1000000 /* Vehicle flag */
#define ZOMBIE        0x2000000 /* Zombie flag */
#define LISTENER      0x4000000 /* internal: listener flag */
#define XFORCIBLE     0x8000000 /* externally forcible flag */
#define READMODE     0x10000000 /* internal: when set, player is in a READ */
#define SANEBIT      0x20000000 /* internal: used to check db sanity */
#define YIELD        0x40000000 /* Yield flag */
#define OVERT        0x80000000 /* Overt flag */

/* what flags to NOT dump to disk. */
#define DUMP_MASK   (INTERACTIVE | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)

#define Typeof(x)   (x == HOME ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))

#define GOD         ((dbref)  1)    /* head player */
#define NOTHING     ((dbref) -1)    /* null dbref */
#define AMBIGUOUS   ((dbref) -2)    /* multiple possibilities, for matchers */
#define HOME        ((dbref) -3)    /* virtual room, represents mover's home */
#define NIL         ((dbref) -4)    /* do-nothing link, for actions */

#define ObjExists(d)    ((d) >= 0 && (d) < db_top)
#define OkRef(d)        (ObjExists(d) || (d) == NOTHING)
#define OkObj(d)        (ObjExists(d) && Typeof(d) != TYPE_GARBAGE)

#ifdef GOD_PRIV
#define God(x)          ((x) == (GOD))
#endif

#define MLevRaw(x)  (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0))
#define MLevel(x)   (((FLAGS(x) & WIZARD) && \
                    ((FLAGS(x) & MUCKER) || (FLAGS(x) & SMUCKER)))? 4 : \
                    (((FLAGS(x) & MUCKER)? 2 : 0) + \
                    ((FLAGS(x) & SMUCKER)? 1 : 0)))
#define PLevel(x)   ((FLAGS(x) & (MUCKER | SMUCKER))? \
                    (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0) + 1) : \
                    ((FLAGS(x) & ABODE)? 0 : 1))

#define Dark(x)         ((FLAGS(x) & DARK) != 0)
#define Wizard(x)       ((FLAGS(x) & WIZARD) != 0 && (FLAGS(x) & QUELL) == 0)
#define TrueWizard(x)   ((FLAGS(x) & WIZARD) != 0)
#define Mucker(x)       (MLevel(x) != 0)
#define Builder(x)      ((FLAGS(x) & (WIZARD|BUILDER)) != 0)
#define Linkable(x)     ((x) == HOME || \
                        (((Typeof(x) == TYPE_ROOM || Typeof(x) == TYPE_THING) ? \
                        (FLAGS(x) & ABODE) : (FLAGS(x) & LINK_OK)) != 0))


#define SetMLevel(x,y)  { FLAGS(x) &= ~(MUCKER | SMUCKER); \
                            if (y>=2) FLAGS(x) |= MUCKER; \
                            if (y%2) FLAGS(x) |= SMUCKER; }


/*
 * ISGUEST determines whether a particular player is a guest, based on the
 * existence of the property MESGPROP_GUEST.  If GOD_PRIV is defined, then only
 * God can bypass the ISGUEST() check.  Otherwise, any TrueWizard can bypass
 * it.  (This is because @set is blocked from guests, and thus any Wizard who
 * had both MESGPROP_GUEST and QUELL set would be prevented from unsetting
 * their own QUELL flag to be able to clear MESGPROP_GUEST.)
 */
#ifdef GOD_PRIV
#define ISGUEST(x)  ((FLAGS(x) & GUEST) && !God(x))
#else /* !defined(GOD_PRIV) */
#define ISGUEST(x)  ((FLAGS(x) & GUEST) && (FLAGS(x) & TYPE_PLAYER) && !TrueWizard(x))
#endif /* GOD_PRIV */

#define NOGUEST(_cmd,x) \
    if(ISGUEST(x)) {   \
        log_status("Guest %s(#%d) failed attempt to %s.\n",NAME(x),x,_cmd); \
        notifyf_nolisten(x, "Guests are not allowed to %s.\r", _cmd); \
        return; \
    }

#define NOFORCE(_cmd,x) \
    if(force_level) {   \
        notifyf_nolisten(x, "You can't use %s from a @force or {force}.\r", _cmd); \
        return; \
    }

/* Mucker levels */
#define MLEV_APPRENTICE   1
#define MLEV_JOURNEYMAN   2
#define MLEV_MASTER       3
#define MLEV_WIZARD       4

#ifdef GOD_PRIV
# define MLEV_GOD               255
# define TUNE_MLEV(player)      (God(player) ? MLEV_GOD : MLevel(player))
#else
# define MLEV_GOD               MLEV_WIZARD
# define TUNE_MLEV(player)      MLevel(player)
#endif

#define BUILDERONLY(_cmd,x) \
    if(!Builder(x)) {   \
        notifyf_nolisten(x, "Only builders are allowed to %s.\r", _cmd); \
        return; \
    }

#define MUCKERONLY(_cmd,x) \
    if(!Mucker(x)) {   \
        notifyf_nolisten(x, "Only programmers are allowed to %s.\r", _cmd); \
        return; \
    }

#define PLAYERONLY(_cmd,x) \
    if(Typeof(x) != TYPE_PLAYER) {   \
        notifyf_nolisten(x, "Only players are allowed to %s.\r", _cmd); \
        return; \
    }

#define WIZARDONLY(_cmd,x) \
    if(!Wizard(OWNER(x))) {   \
        notifyf_nolisten(x, "You are not allowed to %s.\r", _cmd); \
        return; \
    }

#ifndef GOD_PRIV
#define GODONLY(_cmd,x) WIZARDONLY(_cmd, x); PLAYERONLY(_cmd,x)
#else
#define GODONLY(_cmd,x) \
    if(!God(x)) {   \
        notifyf_nolisten(x, "You are not allowed to %s.\r", _cmd); \
        return; \
    }
#endif

#define DOLIST(var, first) \
  for ((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)

#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}

typedef long object_flag_type;

/*
 * Each type of object may have a _specific structure associated with it.
 *
 * This one is for MUF programs.
 */
struct program_specific {
    unsigned short instances;   /* number of instances of this prog running */
    unsigned short instances_in_primitive;  /* number instances running a primitive */
    short curr_line;            /* current-line */
    int siz;                    /* size of code */
    struct inst *code;          /* byte-compiled code */
    struct inst *start;         /* place to start executing */
    struct line *first;         /* first line */
    struct publics *pubs;       /* public subroutine addresses */
    struct mcp_binding *mcpbinds;   /* MCP message bindings. */
    struct timeval proftime;    /* profiling time spent in this program. */
    time_t profstart;           /* time when profiling started for this prog */
    unsigned int profuses;      /* #calls to this program while profiling */
};

#define PROGRAM_SP(x)           (DBFETCH(x)->sp.program.sp)
#define ALLOC_PROGRAM_SP(x)     { PROGRAM_SP(x) = (struct program_specific *)malloc(sizeof(struct program_specific)); }
#define FREE_PROGRAM_SP(x)      { dbref foo = x; if(PROGRAM_SP(foo)) free(PROGRAM_SP(foo)); PROGRAM_SP(foo) = (struct program_specific *)NULL; }

#define PROGRAM_INSTANCES(x)    (PROGRAM_SP(x)!=NULL?PROGRAM_SP(x)->instances:0)
#define PROGRAM_INSTANCES_IN_PRIMITIVE(x)   (PROGRAM_SP(x)!=NULL?PROGRAM_SP(x)->instances_in_primitive:0)
#define PROGRAM_CURR_LINE(x)    (PROGRAM_SP(x)->curr_line)
#define PROGRAM_SIZ(x)          (PROGRAM_SP(x)->siz)
#define PROGRAM_CODE(x)         (PROGRAM_SP(x)->code)
#define PROGRAM_START(x)        (PROGRAM_SP(x)->start)
#define PROGRAM_FIRST(x)        (PROGRAM_SP(x)->first)
#define PROGRAM_PUBS(x)         (PROGRAM_SP(x)->pubs)
#define PROGRAM_PROFTIME(x)     (PROGRAM_SP(x)->proftime)
#define PROGRAM_PROFSTART(x)    (PROGRAM_SP(x)->profstart)
#define PROGRAM_PROF_USES(x)    (PROGRAM_SP(x)->profuses)

#define PROGRAM_INC_INSTANCES(x)    (PROGRAM_SP(x)->instances++)
#define PROGRAM_DEC_INSTANCES(x)    (PROGRAM_SP(x)->instances--)
#define PROGRAM_INC_INSTANCES_IN_PRIMITIVE(x)   (PROGRAM_SP(x)->instances_in_primitive++)
#define PROGRAM_DEC_INSTANCES_IN_PRIMITIVE(x)   (PROGRAM_SP(x)->instances_in_primitive--)
#define PROGRAM_INC_PROF_USES(x)    (PROGRAM_SP(x)->profuses++)

#define PROGRAM_SET_INSTANCES(x,y)  (PROGRAM_SP(x)->instances = y)
#define PROGRAM_SET_INSTANCES_IN_PRIMITIVE(x,y) (PROGRAM_SP(x)->instances_in_primitive = y)
#define PROGRAM_SET_CURR_LINE(x,y)  (PROGRAM_SP(x)->curr_line = y)
#define PROGRAM_SET_SIZ(x,y)        (PROGRAM_SP(x)->siz = y)
#define PROGRAM_SET_CODE(x,y)       (PROGRAM_SP(x)->code = y)
#define PROGRAM_SET_START(x,y)      (PROGRAM_SP(x)->start = y)
#define PROGRAM_SET_FIRST(x,y)      (PROGRAM_SP(x)->first = y)
#define PROGRAM_SET_PUBS(x,y)       (PROGRAM_SP(x)->pubs = y)
#define PROGRAM_SET_PROFTIME(x,y,z) (PROGRAM_SP(x)->proftime.tv_usec = z, PROGRAM_SP(x)->proftime.tv_sec = y)
#define PROGRAM_SET_PROFSTART(x,y)  (PROGRAM_SP(x)->profstart = y)
#define PROGRAM_SET_PROF_USES(x,y)  (PROGRAM_SP(x)->profuses = y)
#define PROGRAM_MCPBINDS(x)         (PROGRAM_SP(x)->mcpbinds)
#define PROGRAM_SET_MCPBINDS(x,y)   (PROGRAM_SP(x)->mcpbinds = y)

/*
 * Players an things share the same _specific structure at this time.
 * Probably to help support zombies.
 */
struct player_specific {
    dbref home;             /* Home location */
    dbref curr_prog;        /* program I'm currently editing */
    short insert_mode;      /* in insert mode? */
    short block;            /* Is the player 'blocked' by a MUF program? */
    const char *password;   /* Player password */
    int *descrs;            /* List if descriptors */
    int descr_count;        /* Number of descriptors */
    dbref *ignore_cache;    /* People the player is ignoring */
    int ignore_count;       /* Number of entries in the ignore cahche */
    dbref ignore_last;      /* The last DBREF in the ignore cache */
};

#define THING_SP(x)             (DBFETCH(x)->sp.player.sp)
#define ALLOC_THING_SP(x)       { PLAYER_SP(x) = (struct player_specific *)malloc(sizeof(struct player_specific)); }
#define FREE_THING_SP(x)        { dbref foo = x; free(PLAYER_SP(foo)); PLAYER_SP(foo) = NULL; }

#define THING_HOME(x)           (PLAYER_SP(x)->home)

#define THING_SET_HOME(x,y)     (PLAYER_SP(x)->home = y)

#define PLAYER_SP(x)            (DBFETCH(x)->sp.player.sp)
#define ALLOC_PLAYER_SP(x)      { PLAYER_SP(x) = (struct player_specific *)malloc(sizeof(struct player_specific)); memset(PLAYER_SP(x),0,sizeof(struct player_specific));}
#define FREE_PLAYER_SP(x)       { dbref foo = x; free(PLAYER_SP(foo)); PLAYER_SP(foo) = NULL; }

#define PLAYER_HOME(x)          (PLAYER_SP(x)->home)
#define PLAYER_CURR_PROG(x)     (PLAYER_SP(x)->curr_prog)
#define PLAYER_INSERT_MODE(x)   (PLAYER_SP(x)->insert_mode)
#define PLAYER_BLOCK(x)         (PLAYER_SP(x)->block)
#define PLAYER_PASSWORD(x)      (PLAYER_SP(x)->password)
#define PLAYER_DESCRS(x)        (PLAYER_SP(x)->descrs)
#define PLAYER_DESCRCOUNT(x)    (PLAYER_SP(x)->descr_count)
#define PLAYER_IGNORE_CACHE(x)  (PLAYER_SP(x)->ignore_cache)
#define PLAYER_IGNORE_COUNT(x)  (PLAYER_SP(x)->ignore_count)
#define PLAYER_IGNORE_LAST(x)   (PLAYER_SP(x)->ignore_last)

#define PLAYER_SET_HOME(x,y)        (PLAYER_SP(x)->home = y)
#define PLAYER_SET_CURR_PROG(x,y)   (PLAYER_SP(x)->curr_prog = y)
#define PLAYER_SET_INSERT_MODE(x,y) (PLAYER_SP(x)->insert_mode = y)
#define PLAYER_SET_BLOCK(x,y)       (PLAYER_SP(x)->block = y)
#define PLAYER_SET_PASSWORD(x,y)    (PLAYER_SP(x)->password = y)
#define PLAYER_SET_DESCRS(x,y)      (PLAYER_SP(x)->descrs = y)
#define PLAYER_SET_DESCRCOUNT(x,y)  (PLAYER_SP(x)->descr_count = y)
#define PLAYER_SET_IGNORE_CACHE(x,y)    (PLAYER_SP(x)->ignore_cache = y)
#define PLAYER_SET_IGNORE_COUNT(x,y)    (PLAYER_SP(x)->ignore_count = y)
#define PLAYER_SET_IGNORE_LAST(x,y)     (PLAYER_SP(x)->ignore_last = y)

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))
#endif

/* system parameter types */
#define TP_TYPE_STRING   0
#define TP_TYPE_TIMESPAN 1
#define TP_TYPE_INTEGER  2
#define TP_TYPE_DBREF    3
#define TP_TYPE_BOOLEAN  4

/* union of type-specific fields */

union specific {        /* I've been railroaded! */
    struct {            /* ROOM-specific fields */
        dbref dropto;
    } room;

    struct {            /* EXIT-specific fields */
        int ndest;
        dbref *dest;
    } exit;

    struct {            /* PLAYER- and THING- specific fields */
        struct player_specific *sp;
    } player;

    struct {            /* PROGRAM-specific fields */
        struct program_specific *sp;
    } program;
};

struct object {
    const char *name;   /* Object name */
    dbref location;     /* pointer to container */
    dbref owner;        /* Object owner */
    dbref contents;     /* Head of the object's contents db list */
    dbref exits;        /* Head of the object's exits db list */
    dbref next;         /* pointer to next in contents/exits chain */
    struct plist *properties;   /* Root of properties tree */
#ifdef DISKBASE
    long propsfpos;     /* File position for properties in the DB file */
    time_t propstime;   /* Last time props were used */
    dbref nextold;      /* Ringqueue for diskbase next db */
    dbref prevold;      /* Ringqueue for diskbase previous db */
    short propsmode;    /* State of the props - PROPS_UNLOADED, PROPS_CHANGED */
    short spacer;       /* Not used by anything */
#endif
    object_flag_type flags;         /* Object flags */
    unsigned int mpi_prof_use;      /* MPI profiler number of uses */
    struct timeval mpi_proftime;    /* Time spent running MPI */
    time_t ts_created;              /* Created time */
    time_t ts_modified;             /* Modified time */
    time_t ts_lastused;             /* Last used time */
    int ts_usecount;                /* Usage counter */
    union specific sp;              /* Per-type specific structure */
};

#define OBJECT_ENDOWMENT(cost)  (((cost)-5)/5)
#define OBJECT_GETCOST(endow)   ((endow)*5+5)

/*
 * This structure is used for MUFTOPS to calculate statistics about running
 * programs.
 */
struct profnode {
    struct profnode *next;  /* Linked list implementation */
    dbref prog;             /* Program DBREF */
    double proftime;        /* Profile time tracking */
    double pcnt;            /* Percentage */
    time_t comptime;        /* Profile length (how long its been profiled) */
    long usecount;          /* Usage count */
    short type;             /* If true, MUF.  If false, MPI */
};

/*
 * This is for making an arbitrary linked list of objects.  Such as
 * for the forcelist.
 */
typedef struct objnode {
    dbref data;
    struct objnode *next;
} objnode;

/*
 * These (particularly db) are used all over the place.  Very important
 * externs.
 */

/**
 * @var the array of objects that is our DB
 */
extern struct object *db;

/**
 * @var the things currently being forced.
 */
extern objnode *forcelist;

/**
 * @var the 'top' of the database; it is the highest dbref + 1
 */
extern dbref db_top;

/**
 * @var the head of the garbage dbref list -- recycle-able objects
 *      This may be NOTHING.
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
 * @return the database ref of the object
 */
dbref new_object(void);

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
 * If head is NULL, nothing happens.  Removes the node and frees the
 * memory but does not return the pop'd dbref.
 *
 * @param head pointer to pointer of list to pop from.
 */
void objnode_pop(objnode **head);

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
dbref reverse(dbref);

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
 * @param loc the target to generate unparse text for
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
