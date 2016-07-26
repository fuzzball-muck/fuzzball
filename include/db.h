#ifndef _DB_H
#define _DB_H

#include "config.h"

#ifdef HAVE_TIMEBITS_H
#  define __need_timeval 1
#  include <timebits.h>
#endif

#define DB_VERSION_STRING "***Foxen9 TinyMUCK DUMP Format***"

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 2048
#define BUFFER_LEN ((MAX_COMMAND_LEN)*4)

extern char match_args[BUFFER_LEN];
extern char match_cmdname[BUFFER_LEN];

#ifdef MCP_SUPPORT
#include "mcp.h"
#endif

#define DBFETCH(x)  (db + (x))
#ifdef DEBUGDBDIRTY
#  define DBDIRTY(x)  {if (!(db[x].flags & OBJECT_CHANGED))  \
			   log2file("dirty.out", "#%d: %s %d\n", (int)x, \
			   __FILE__, __LINE__); \
		       db[x].flags |= OBJECT_CHANGED;}
#else
#  define DBDIRTY(x)  {db[x].flags |= OBJECT_CHANGED;}
#endif

#define DBSTORE(x, y, z)    {DBFETCH(x)->y = z; DBDIRTY(x);}

#define NAME(x)     (db[x].name)
#define FLAGS(x)    (db[x].flags)
#define OWNER(x)    (db[x].owner)
#define LOCATION(x) (DBFETCH((x))->location)
#define CONTENTS(x) (DBFETCH((x))->contents)
#define EXITS(x)    (DBFETCH((x))->exits)
#define NEXTOBJ(x)  (DBFETCH((x))->next)

/* defines for possible data access mods. */
#define MESGPROP_DESC		"_/de"
#define MESGPROP_IDESC		"_/ide"
#define MESGPROP_SUCC		"_/sc"
#define MESGPROP_OSUCC		"_/osc"
#define MESGPROP_FAIL		"_/fl"
#define MESGPROP_OFAIL		"_/ofl"
#define MESGPROP_DROP		"_/dr"
#define MESGPROP_ODROP		"_/odr"
#define MESGPROP_DOING		"_/do"
#define MESGPROP_OECHO		"_/oecho"
#define MESGPROP_PECHO		"_/pecho"
#define MESGPROP_LOCK		"_/lok"
#define MESGPROP_FLOCK		"@/flk"
#define MESGPROP_CONLOCK	"_/clk"
#define MESGPROP_CHLOCK		"_/chlk"
#define MESGPROP_LINKLOCK	"_/lklk"
#define MESGPROP_PCON		"_/pcon"
#define MESGPROP_PDCON		"_/pdcon"
#define MESGPROP_VALUE		"@/value"

#define GETMESG(x,y)   (get_property_class(x, y))
#define GETDESC(x)	GETMESG(x, MESGPROP_DESC)
#define GETIDESC(x)	GETMESG(x, MESGPROP_IDESC)
#define GETSUCC(x)	GETMESG(x, MESGPROP_SUCC)
#define GETOSUCC(x)	GETMESG(x, MESGPROP_OSUCC)
#define GETFAIL(x)	GETMESG(x, MESGPROP_FAIL)
#define GETOFAIL(x)	GETMESG(x, MESGPROP_OFAIL)
#define GETDROP(x)	GETMESG(x, MESGPROP_DROP)
#define GETODROP(x)	GETMESG(x, MESGPROP_ODROP)
#define GETDOING(x)	GETMESG(x, MESGPROP_DOING)
#define GETOECHO(x)	GETMESG(x, MESGPROP_OECHO)
#define GETPECHO(x)	GETMESG(x, MESGPROP_PECHO)

#define SETMESG(x,y,z)    {add_property(x, y, z, 0);}
#define SETDESC(x,y)	SETMESG(x, MESGPROP_DESC, y)

#define GETLOCK(x)    (get_property_lock(x, MESGPROP_LOCK))
#define SETLOCK(x,y)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property(x, MESGPROP_LOCK, &mydat, 0);}
#define CLEARLOCK(x)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = TRUE_BOOLEXP; set_property(x, MESGPROP_LOCK, &mydat, 0); DBDIRTY(x);}

#define GETVALUE(x)	get_property_value(x, MESGPROP_VALUE)
#define SETVALUE(x,y)	add_property(x, MESGPROP_VALUE, NULL, y)

#define DB_PARMSINFO     0x0001	/* legacy database value */

#define TYPE_ROOM           0x0
#define TYPE_THING          0x1
#define TYPE_EXIT           0x2
#define TYPE_PLAYER         0x3
#define TYPE_PROGRAM        0x4
#define NOTYPE1 	    0x5	/* Room for expansion */
#define TYPE_GARBAGE        0x6
#define NOTYPE              0x7	/* no particular type */
#define TYPE_MASK           0x7

#define EXPANSION0	   0x08	/* Expansion bit */

#define WIZARD             0x10	/* gets automatic control */
#define LINK_OK            0x20	/* anybody can link to this */
#define DARK               0x40	/* contents of room are not printed */
#define INTERNAL           0x80	/* internal-use-only flag */
#define STICKY            0x100	/* this object goes home when dropped */
#define BUILDER           0x200	/* this player can use construction commands */
#define CHOWN_OK          0x400	/* this object can be @chowned, or
				   this player can see color */
#define JUMP_OK           0x800	/* A room which can be jumped from, or
				 * a player who can be jumped to */
#define EXPANSION1	 0x1000	/* Expansion bit */
#define EXPANSION2	 0x2000	/* Expansion bit */
#define KILL_OK	         0x4000	/* Kill_OK bit.  Means you can be killed. */
#define GUEST		 0x8000	/* Guest flag */
#define HAVEN           0x10000	/* can't kill here */
#define ABODE           0x20000	/* can set home here */
#define MUCKER          0x40000	/* programmer */
#define QUELL           0x80000	/* When set, wiz-perms are turned off */
#define SMUCKER        0x100000	/* second programmer bit.  For levels */
#define INTERACTIVE    0x200000	/* internal: player in MUF editor */
#define OBJECT_CHANGED 0x400000	/* internal: set when an object is dbdirty()ed */
#define EXPANSION3     0x800000 /* Expansion bit */
#define VEHICLE       0x1000000	/* Vehicle flag */
#define ZOMBIE        0x2000000	/* Zombie flag */
#define LISTENER      0x4000000	/* internal: listener flag */
#define XFORCIBLE     0x8000000	/* externally forcible flag */
#define READMODE     0x10000000	/* internal: when set, player is in a READ */
#define SANEBIT      0x20000000	/* internal: used to check db sanity */
#define YIELD	     0x40000000	/* Yield flag */
#define OVERT        0x80000000	/* Overt flag */

#define ABATE		ABODE
#define AUTOSTART	ABODE
#define BOUND		BUILDER
#define COLOR		CHOWN_OK
#define HARDUID		HAVEN
#define HIDE		HAVEN
#define SETUID		STICKY
#define SILENT		STICKY
#define VIEWABLE	VEHICLE
#define XPRESS		XFORCIBLE
#define ZMUF_DEBUGGER	ZOMBIE

/* what flags to NOT dump to disk. */
#define DUMP_MASK    (INTERACTIVE | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)

#define Typeof(x) (x == HOME ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))

#define GOD		((dbref)  1)	/* head player */
#define NOTHING		((dbref) -1)	/* null dbref */
#define AMBIGUOUS	((dbref) -2)	/* multiple possibilities, for matchers */
#define HOME		((dbref) -3)	/* virtual room, represents mover's home */
#define NIL		((dbref) -4)	/* do-nothing link, for actions */

#define ObjExists(d)	((d) >= 0 && (d) < db_top)
#define OkRef(d)	(ObjExists(d) || (d) == NOTHING)
#define OkObj(d)	(ObjExists(d) && Typeof(d) != TYPE_GARBAGE)

#ifdef GOD_PRIV
#define God(x) ((x) == (GOD))
#endif

#define MLevRaw(x)	(((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0))
#define MLevel(x)	(((FLAGS(x) & WIZARD) && \
			((FLAGS(x) & MUCKER) || (FLAGS(x) & SMUCKER)))? 4 : \
			(((FLAGS(x) & MUCKER)? 2 : 0) + \
			((FLAGS(x) & SMUCKER)? 1 : 0)))
#define PLevel(x)	((FLAGS(x) & (MUCKER | SMUCKER))? \
			(((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0) + 1) : \
			((FLAGS(x) & ABODE)? 0 : 1))

#define Dark(x)		((FLAGS(x) & DARK) != 0)
#define Wizard(x)	((FLAGS(x) & WIZARD) != 0 && (FLAGS(x) & QUELL) == 0)
#define TrueWizard(x)	((FLAGS(x) & WIZARD) != 0)
#define Mucker(x)	(MLevel(x) != 0)
#define Builder(x)	((FLAGS(x) & (WIZARD|BUILDER)) != 0)
#define Linkable(x)	((x) == HOME || \
			(((Typeof(x) == TYPE_ROOM || Typeof(x) == TYPE_THING) ? \
			(FLAGS(x) & ABODE) : (FLAGS(x) & LINK_OK)) != 0))


#define SetMLevel(x,y) { FLAGS(x) &= ~(MUCKER | SMUCKER); \
			 if (y>=2) FLAGS(x) |= MUCKER; \
                         if (y%2) FLAGS(x) |= SMUCKER; }


/* ISGUEST determines whether a particular player is a guest, based on the existence
   of the property MESGPROP_GUEST.  If GOD_PRIV is defined, then only God can bypass
   the ISGUEST() check.  Otherwise, any TrueWizard can bypass it.  (This is because
   @set is blocked from guests, and thus any Wizard who had both MESGPROP_GUEST and
   QUELL set would be prevented from unsetting their own QUELL flag to be able to
   clear MESGPROP_GUEST.) */
#ifdef GOD_PRIV
#define ISGUEST(x)	((FLAGS(x) & GUEST) && !God(x))
#else				/* !defined(GOD_PRIV) */
#define ISGUEST(x)	((FLAGS(x) & GUEST) && (FLAGS(x) & TYPE_PLAYER) && !TrueWizard(x))
#endif				/* GOD_PRIV */

#define NOGUEST(_cmd,x) \
if(ISGUEST(x)) \
{   \
    log_status("Guest %s(#%d) failed attempt to %s.\n",NAME(x),x,_cmd); \
    notifyf_nolisten(x, "Guests are not allowed to %s.\r", _cmd); \
    return; \
}
#define NOFORCE(_cmd,fl,x) \
if(fl) \
{   \
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

#define PREEMPT 0
#define FOREGROUND 1
#define BACKGROUND 2


#define BUILDERONLY(_cmd,x) \
if(!Builder(x)) \
{   \
    notifyf_nolisten(x, "Only builders are allowed to %s.\r", _cmd); \
    return; \
}

#define MUCKERONLY(_cmd,x) \
if(!Mucker(x)) \
{   \
    notifyf_nolisten(x, "Only programmers are allowed to %s.\r", _cmd); \
    return; \
}

#define PLAYERONLY(_cmd,x) \
if(Typeof(x) != TYPE_PLAYER) \
{   \
    notifyf_nolisten(x, "Only players are allowed to %s.\r", _cmd); \
    return; \
}

#define WIZARDONLY(_cmd,x) \
if(!Wizard(OWNER(x))) \
{   \
    notifyf_nolisten(x, "You are not allowed to %s.\r", _cmd); \
    return; \
}

#ifndef GOD_PRIV
#define GODONLY(_cmd,x) WIZARDONLY(_cmd, x); PLAYERONLY(_cmd,x)
#else
#define GODONLY(_cmd,x) \
if(!God(x)) \
{   \
    notifyf_nolisten(x, "You are not allowed to %s.\r", _cmd); \
    return; \
}
#endif

#define DOLIST(var, first) \
  for ((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)
#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}

typedef long object_flag_type;

struct program_specific {
    unsigned short instances;	/* number of instances of this prog running */
    short curr_line;		/* current-line */
    int siz;			/* size of code */
    struct inst *code;		/* byte-compiled code */
    struct inst *start;		/* place to start executing */
    struct line *first;		/* first line */
    struct publics *pubs;	/* public subroutine addresses */
#ifdef MCP_SUPPORT
    struct mcp_binding *mcpbinds;	/* MCP message bindings. */
#endif
    struct timeval proftime;	/* profiling time spent in this program. */
    time_t profstart;		/* time when profiling started for this prog */
    unsigned int profuses;	/* #calls to this program while profiling */
};

#define PROGRAM_SP(x)			(DBFETCH(x)->sp.program.sp)
#define ALLOC_PROGRAM_SP(x)     { PROGRAM_SP(x) = (struct program_specific *)malloc(sizeof(struct program_specific)); }
#define FREE_PROGRAM_SP(x)      { dbref foo = x; if(PROGRAM_SP(foo)) free(PROGRAM_SP(foo)); PROGRAM_SP(foo) = (struct program_specific *)NULL; }

#define PROGRAM_INSTANCES(x)		(PROGRAM_SP(x)!=NULL?PROGRAM_SP(x)->instances:0)
#define PROGRAM_CURR_LINE(x)		(PROGRAM_SP(x)->curr_line)
#define PROGRAM_SIZ(x)			(PROGRAM_SP(x)->siz)
#define PROGRAM_CODE(x)			(PROGRAM_SP(x)->code)
#define PROGRAM_START(x)		(PROGRAM_SP(x)->start)
#define PROGRAM_FIRST(x)		(PROGRAM_SP(x)->first)
#define PROGRAM_PUBS(x)			(PROGRAM_SP(x)->pubs)
#define PROGRAM_PROFTIME(x)		(PROGRAM_SP(x)->proftime)
#define PROGRAM_PROFSTART(x)		(PROGRAM_SP(x)->profstart)
#define PROGRAM_PROF_USES(x)		(PROGRAM_SP(x)->profuses)

#define PROGRAM_INC_INSTANCES(x)	(PROGRAM_SP(x)->instances++)
#define PROGRAM_DEC_INSTANCES(x)	(PROGRAM_SP(x)->instances--)
#define PROGRAM_INC_PROF_USES(x)	(PROGRAM_SP(x)->profuses++)

#define PROGRAM_SET_INSTANCES(x,y)	(PROGRAM_SP(x)->instances = y)
#define PROGRAM_SET_CURR_LINE(x,y)	(PROGRAM_SP(x)->curr_line = y)
#define PROGRAM_SET_SIZ(x,y)		(PROGRAM_SP(x)->siz = y)
#define PROGRAM_SET_CODE(x,y)		(PROGRAM_SP(x)->code = y)
#define PROGRAM_SET_START(x,y)		(PROGRAM_SP(x)->start = y)
#define PROGRAM_SET_FIRST(x,y)		(PROGRAM_SP(x)->first = y)
#define PROGRAM_SET_PUBS(x,y)		(PROGRAM_SP(x)->pubs = y)
#define PROGRAM_SET_PROFTIME(x,y,z)	(PROGRAM_SP(x)->proftime.tv_usec = z, PROGRAM_SP(x)->proftime.tv_sec = y)
#define PROGRAM_SET_PROFSTART(x,y)	(PROGRAM_SP(x)->profstart = y)
#define PROGRAM_SET_PROF_USES(x,y)	(PROGRAM_SP(x)->profuses = y)

#ifdef MCP_SUPPORT
#define PROGRAM_MCPBINDS(x)		(PROGRAM_SP(x)->mcpbinds)
#define PROGRAM_SET_MCPBINDS(x,y)	(PROGRAM_SP(x)->mcpbinds = y)
#endif

struct player_specific {
    dbref home;
    dbref curr_prog;		/* program I'm currently editing */
    short insert_mode;		/* in insert mode? */
    short block;
    const char *password;
    int *descrs;
    int descr_count;
    dbref *ignore_cache;
    int ignore_count;
    dbref ignore_last;
};

#define THING_SP(x)		(DBFETCH(x)->sp.player.sp)
#define ALLOC_THING_SP(x)       { PLAYER_SP(x) = (struct player_specific *)malloc(sizeof(struct player_specific)); }
#define FREE_THING_SP(x)        { dbref foo = x; free(PLAYER_SP(foo)); PLAYER_SP(foo) = NULL; }

#define THING_HOME(x)		(PLAYER_SP(x)->home)

#define THING_SET_HOME(x,y)	(PLAYER_SP(x)->home = y)

#define PLAYER_SP(x)		(DBFETCH(x)->sp.player.sp)
#define ALLOC_PLAYER_SP(x)      { PLAYER_SP(x) = (struct player_specific *)malloc(sizeof(struct player_specific)); bzero(PLAYER_SP(x),sizeof(struct player_specific));}
#define FREE_PLAYER_SP(x)       { dbref foo = x; free(PLAYER_SP(foo)); PLAYER_SP(foo) = NULL; }

#define PLAYER_HOME(x)		(PLAYER_SP(x)->home)
#define PLAYER_CURR_PROG(x)	(PLAYER_SP(x)->curr_prog)
#define PLAYER_INSERT_MODE(x)	(PLAYER_SP(x)->insert_mode)
#define PLAYER_BLOCK(x)		(PLAYER_SP(x)->block)
#define PLAYER_PASSWORD(x)	(PLAYER_SP(x)->password)
#define PLAYER_DESCRS(x)	(PLAYER_SP(x)->descrs)
#define PLAYER_DESCRCOUNT(x)    (PLAYER_SP(x)->descr_count)
#define PLAYER_IGNORE_CACHE(x)  (PLAYER_SP(x)->ignore_cache)
#define PLAYER_IGNORE_COUNT(x)  (PLAYER_SP(x)->ignore_count)
#define PLAYER_IGNORE_LAST(x)   (PLAYER_SP(x)->ignore_last)

#define PLAYER_SET_HOME(x,y)		(PLAYER_SP(x)->home = y)
#define PLAYER_SET_CURR_PROG(x,y)	(PLAYER_SP(x)->curr_prog = y)
#define PLAYER_SET_INSERT_MODE(x,y)	(PLAYER_SP(x)->insert_mode = y)
#define PLAYER_SET_BLOCK(x,y)		(PLAYER_SP(x)->block = y)
#define PLAYER_SET_PASSWORD(x,y)	(PLAYER_SP(x)->password = y)
#define PLAYER_SET_DESCRS(x,y)		(PLAYER_SP(x)->descrs = y)
#define PLAYER_SET_DESCRCOUNT(x,y)	(PLAYER_SP(x)->descr_count = y)
#define PLAYER_SET_IGNORE_CACHE(x,y)	(PLAYER_SP(x)->ignore_cache = y)
#define PLAYER_SET_IGNORE_COUNT(x,y)	(PLAYER_SP(x)->ignore_count = y)
#define PLAYER_SET_IGNORE_LAST(x,y)		(PLAYER_SP(x)->ignore_last = y)

/* union of type-specific fields */

union specific {		/* I've been railroaded! */
    struct {			/* ROOM-specific fields */
	dbref dropto;
    } room;
    /*
    struct {			// THING-specific fields
	dbref home;
    } thing;
    */
    struct {			/* EXIT-specific fields */
	int ndest;
	dbref *dest;
    } exit;
    struct {			/* PLAYER-specific fields */
	struct player_specific *sp;
    } player;
    struct {			/* PROGRAM-specific fields */
	struct program_specific *sp;
    } program;
};

struct object {
    const char *name;
    dbref location;		/* pointer to container */
    dbref owner;
    dbref contents;
    dbref exits;
    dbref next;			/* pointer to next in contents/exits chain */
    struct plist *properties;
#ifdef DISKBASE
    long propsfpos;
    time_t propstime;
    dbref nextold;
    dbref prevold;
    short propsmode;
    short spacer;
#endif
    object_flag_type flags;
    unsigned int mpi_prof_use;
    struct timeval mpi_proftime;
    time_t ts_created;
    time_t ts_modified;
    time_t ts_lastused;
    int ts_usecount;
    union specific sp;
};

struct profnode {
    struct profnode *next;
    dbref prog;
    double proftime;
    double pcnt;
    time_t comptime;
    long usecount;
    short type;
};

extern struct object *db;
extern dbref db_top;
extern int recyclable;

int controls(dbref who, dbref what);
int controls_link(dbref who, dbref what);
dbref create_program(dbref player, const char *name);
void db_clear_object(dbref i);
void db_free(void);
void db_free_object(dbref i);
dbref db_read(FILE * f);
dbref db_write(FILE * f);
dbref getparent(dbref obj);
void getproperties(FILE * f, dbref obj, const char *pdir);
dbref getref(FILE * f);
int link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list);
int link_exit_dry(int descr, dbref player, dbref exit, char *dest_name,
                         dbref * dest_list);
int member(dbref thing, dbref list);
dbref new_object(void);
int ok_ascii_other(const char *name);
int ok_ascii_thing(const char *name);
int ok_name(const char *name);
void putref(FILE * f, dbref ref);
void putproperties(FILE * f, dbref obj);
void putstring(FILE * f, const char *s);
dbref remove_first(dbref first, dbref what);
dbref reverse(dbref);
void set_source(dbref player, dbref action, dbref source);
long size_object(dbref i, int load);
const char *unparse_flags(dbref thing);
const char *unparse_object(dbref player, dbref object);
int unset_source(dbref player, dbref action);

#endif				/* _DB_H */
