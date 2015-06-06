#ifndef _DB_H
#define _DB_H

#include "config.h"
#include "defines.h"

#include <stdio.h>
#include <math.h>
#include <time.h>
#ifdef HAVE_TIMEBITS_H
#  define __need_timeval 1
#  include <timebits.h>
#endif

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 2048
#define BUFFER_LEN ((MAX_COMMAND_LEN)*4)
#define FILE_BUFSIZ ((BUFSIZ)*8)

/* Defining INF as infinite.  This is HUGE_VAL on IEEE754 systems. */
#if defined(HUGE_VAL)
# define INF (HUGE_VAL)
# define NINF (-HUGE_VAL)
#else
# define INF (9.9E999)
# define NINF (-9.9E999)
#endif

/* Defining Pi, Half Pi, and Quarter Pi.  */
#ifdef M_PI
# define F_PI M_PI
# define NF_PI -M_PI
#else
# define F_PI 3.141592653589793239
# define NF_PI -3.141592653589793239
#endif

#ifdef M_PI_2
# define H_PI M_PI_2
# define NH_PI -M_PI_2
#else
# define H_PI 1.5707963267949
# define NH_PI -1.5707963267949
#endif

#ifdef M_PI_4  /* A quarter slice.  Yum. */
# define Q_PI M_PI_4
# define NQ_PI -M_PI_4
#else
# define Q_PI 0.7853981633974
# define NQ_PI -0.7853981633974
#endif

extern char match_args[BUFFER_LEN];
extern char match_cmdname[BUFFER_LEN];

typedef int dbref;				/* offset into db */

#define TIME_INFINITE ((sizeof(time_t) == 4)? 0xefffffff : 0xefffffffffffffff)

#define DB_READLOCK(x)
#define DB_WRITELOCK(x)
#define DB_RELEASE(x)

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
#define MESGPROP_VALUE		"@/value"
#define MESGPROP_GUEST		"@/isguest"

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
#define SETIDESC(x,y)	SETMESG(x, MESGPROP_IDESC, y)
#define SETSUCC(x,y)	SETMESG(x, MESGPROP_SUCC, y)
#define SETFAIL(x,y)	SETMESG(x, MESGPROP_FAIL, y)
#define SETDROP(x,y)	SETMESG(x, MESGPROP_DROP, y)
#define SETOSUCC(x,y)	SETMESG(x, MESGPROP_OSUCC, y)
#define SETOFAIL(x,y)	SETMESG(x, MESGPROP_OFAIL, y)
#define SETODROP(x,y)	SETMESG(x, MESGPROP_ODROP, y)
#define SETDOING(x,y)	SETMESG(x, MESGPROP_DOING, y)
#define SETOECHO(x,y)	SETMESG(x, MESGPROP_OECHO, y)
#define SETPECHO(x,y)	SETMESG(x, MESGPROP_PECHO, y)

#define LOADMESG(x,y,z)    {add_prop_nofetch(x,y,z,0); DBDIRTY(x);}
#define LOADDESC(x,y)	LOADMESG(x, MESGPROP_DESC, y)
#define LOADIDESC(x,y)	LOADMESG(x, MESGPROP_IDESC, y)
#define LOADSUCC(x,y)	LOADMESG(x, MESGPROP_SUCC, y)
#define LOADFAIL(x,y)	LOADMESG(x, MESGPROP_FAIL, y)
#define LOADDROP(x,y)	LOADMESG(x, MESGPROP_DROP, y)
#define LOADOSUCC(x,y)	LOADMESG(x, MESGPROP_OSUCC, y)
#define LOADOFAIL(x,y)	LOADMESG(x, MESGPROP_OFAIL, y)
#define LOADODROP(x,y)	LOADMESG(x, MESGPROP_ODROP, y)

#define GETLOCK(x)    (get_property_lock(x, MESGPROP_LOCK))
#define SETLOCK(x,y)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property(x, MESGPROP_LOCK, &mydat);}
#define LOADLOCK(x,y) {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property_nofetch(x, MESGPROP_LOCK, &mydat); DBDIRTY(x);}
#define CLEARLOCK(x)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = TRUE_BOOLEXP; set_property(x, MESGPROP_LOCK, &mydat); DBDIRTY(x);}

#define GETFLOCK(x)    (get_property_lock(x, MESGPROP_FLOCK))
#define SETFLOCK(x,y)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property(x, MESGPROP_FLOCK, &mydat);}
#define LOADFLOCK(x,y) {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property_nofetch(x, MESGPROP_FLOCK, &mydat); DBDIRTY(x);}
#define CLEARFLOCK(x)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = TRUE_BOOLEXP; set_property(x, MESGPROP_FLOCK, &mydat); DBDIRTY(x);}

#define GETCONLOCK(x)    (get_property_lock(x, MESGPROP_CONLOCK))
#define SETCONLOCK(x,y)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property(x, MESGPROP_CONLOCK, &mydat);}
#define LOADCONLOCK(x,y) {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property_nofetch(x, MESGPROP_CONLOCK, &mydat); DBDIRTY(x);}
#define CLEARCONLOCK(x)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = TRUE_BOOLEXP; set_property(x, MESGPROP_CONLOCK, &mydat); DBDIRTY(x);}

#define GETCHLOCK(x)    (get_property_lock(x, MESGPROP_CHLOCK))
#define SETCHLOCK(x,y)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property(x, MESGPROP_CHLOCK, &mydat);}
#define LOADCHLOCK(x,y) {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = y; set_property_nofetch(x, MESGPROP_CHLOCK, &mydat); DBDIRTY(x);}
#define CLEARCHLOCK(x)  {PData mydat; mydat.flags = PROP_LOKTYP; mydat.data.lok = TRUE_BOOLEXP; set_property(x, MESGPROP_CHLOCK, &mydat); DBDIRTY(x);}

#define GETVALUE(x)	get_property_value(x, MESGPROP_VALUE)
#define SETVALUE(x,y)	add_property(x, MESGPROP_VALUE, NULL, y)
#define LOADVALUE(x,y)	add_prop_nofetch(x, MESGPROP_VALUE, NULL, y)

#define DB_PARMSINFO     0x0001
#define DB_COMPRESSED    0x0002

#define TYPE_ROOM           0x0
#define TYPE_THING          0x1
#define TYPE_EXIT           0x2
#define TYPE_PLAYER         0x3
#define TYPE_PROGRAM        0x4
#define NOTYPE1				0x5 /* Room for expansion */
#define TYPE_GARBAGE        0x6
#define NOTYPE              0x7	/* no particular type */
#define TYPE_MASK           0x7	/* room for expansion */

#define EXPANSION0		   0x08 /* Not a flag, but one add'l flag for
								 * expansion purposes */

#define WIZARD             0x10	/* gets automatic control */
#define LINK_OK            0x20	/* anybody can link to this */
#define DARK               0x40	/* contents of room are not printed */

/* This #define disabled to avoid accidentally triggerring debugging code */
/* #define DEBUG DARK */	/* Used to print debugging information on
				 * on MUF programs */

#define INTERNAL           0x80	/* internal-use-only flag */
#define STICKY            0x100	/* this object goes home when dropped */
#define SETUID STICKY			/* Used for programs that must run with the
								 * permissions of their owner */
#define SILENT STICKY
#define BUILDER           0x200	/* this player can use construction commands */
#define BOUND BUILDER
#define CHOWN_OK          0x400	/* this object can be @chowned, or
									this player can see color */
#define COLOR CHOWN_OK
#define JUMP_OK           0x800	/* A room which can be jumped from, or
								 * a player who can be jumped to */
#define EXPANSION1		 0x1000 /* Expansion bit */
#define EXPANSION2		 0x2000 /* Expansion bit */
#define KILL_OK	         0x4000	/* Kill_OK bit.  Means you can be killed. */
#define EXPANSION3		 0x8000 /* Expansion bit */
#define HAVEN           0x10000	/* can't kill here */
#define HIDE HAVEN
#define HARDUID HAVEN			/* Program runs with uid of trigger owner */
#define ABODE           0x20000	/* can set home here */
#define ABATE ABODE
#define AUTOSTART ABODE
#define MUCKER          0x40000	/* programmer */
#define QUELL           0x80000	/* When set, wiz-perms are turned off */
#define SMUCKER        0x100000	/* second programmer bit.  For levels */
#define INTERACTIVE    0x200000	/* internal: denotes player is in editor, or
								 * muf READ. */
#define OBJECT_CHANGED 0x400000	/* internal: when an object is dbdirty()ed,
								 * set this */
#define SAVED_DELTA    0x800000	/* internal: object last saved to delta file */
#define VEHICLE       0x1000000	/* Vehicle flag */
#define VIEWABLE VEHICLE
#define ZOMBIE        0x2000000	/* Zombie flag */
#define ZMUF_DEBUGGER ZOMBIE
#define LISTENER      0x4000000	/* internal: listener flag */
#define XFORCIBLE     0x8000000	/* externally forcible flag */
#define XPRESS XFORCIBLE
#define READMODE     0x10000000	/* internal: when set, player is in a READ */
#define SANEBIT      0x20000000	/* internal: used to check db sanity */
#define YIELD	     0x40000000 /* Yield flag */
#define OVERT        0x80000000 /* Overt flag */


/* what flags to NOT dump to disk. */
#define DUMP_MASK    (INTERACTIVE | SAVED_DELTA | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)


typedef long object_flag_type;

#define GOD ((dbref) 1)

#ifdef GOD_PRIV
#define God(x) ((x) == (GOD))
#endif							/* GOD_PRIV */

#define DoNull(s) ((s) ? (s) : "")
#define Typeof(x) (x == HOME ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))
#define Wizard(x) ((FLAGS(x) & WIZARD) != 0 && (FLAGS(x) & QUELL) == 0)

/* TrueWizard is only appropriate when you care about whether the person
   or thing is, well, truely a wizard. Ie it ignores QUELL. */
#define TrueWizard(x) ((FLAGS(x) & WIZARD) != 0)
#define Dark(x) ((FLAGS(x) & DARK) != 0)

/* ISGUEST determines whether a particular player is a guest, based on the existence
   of the property MESGPROP_GUEST.  If GOD_PRIV is defined, then only God can bypass
   the ISGUEST() check.  Otherwise, any TrueWizard can bypass it.  (This is because
   @set is blocked from guests, and thus any Wizard who had both MESGPROP_GUEST and
   QUELL set would be prevented from unsetting their own QUELL flag to be able to
   clear MESGPROP_GUEST.) */
#ifdef GOD_PRIV
#define ISGUEST(x)	(get_property(x, MESGPROP_GUEST) != NULL && !God(x))
#else /* !defined(GOD_PRIV)*/
#define ISGUEST(x)	(get_property(x, MESGPROP_GUEST) != NULL && ((FLAGS(x) & TYPE_PLAYER) && !TrueWizard(x)))
#endif /* GOD_PRIV */
#define NOGUEST(_cmd,x) \
if(ISGUEST(x)) \
{   \
    char tmpstr[BUFFER_LEN]; \
    log_status("Guest %s(#%d) failed attempt to %s.\n",NAME(x),x,_cmd); \
    snprintf(tmpstr, sizeof(tmpstr), "Guests are not allowed to %s.\r", _cmd); \
    notify_nolisten(x,tmpstr,1); \
    return; \
}


#define MLevRaw(x) (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0))

/* Setting a program M0 is supposed to make it not run, but if it's set
 * Wizard, it used to run anyway without the extra double-check for MUCKER
 * or SMUCKER -- now it doesn't, change by Winged */
#define MLevel(x) (((FLAGS(x) & WIZARD) && \
			((FLAGS(x) & MUCKER) || (FLAGS(x) & SMUCKER)))? 4 : \
		   (((FLAGS(x) & MUCKER)? 2 : 0) + \
		    ((FLAGS(x) & SMUCKER)? 1 : 0)))

#define SetMLevel(x,y) { FLAGS(x) &= ~(MUCKER | SMUCKER); \
			 if (y>=2) FLAGS(x) |= MUCKER; \
                         if (y%2) FLAGS(x) |= SMUCKER; }

#define PLevel(x) ((FLAGS(x) & (MUCKER | SMUCKER))? \
                   (((FLAGS(x) & MUCKER)? 2:0) + ((FLAGS(x) & SMUCKER)? 1:0) + 1) : \
                    ((FLAGS(x) & ABODE)? 0 : 1))

#define PREEMPT 0
#define FOREGROUND 1
#define BACKGROUND 2

#define Mucker(x) (MLevel(x) != 0)

#define Builder(x) ((FLAGS(x) & (WIZARD|BUILDER)) != 0)

#define Linkable(x) ((x) == HOME || \
                     (((Typeof(x) == TYPE_ROOM || Typeof(x) == TYPE_THING) ? \
                      (FLAGS(x) & ABODE) : (FLAGS(x) & LINK_OK)) != 0))


/* Boolean expressions, for locks */
typedef char boolexp_type;

#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_PROP 4

struct boolexp {
	boolexp_type type;
	struct boolexp *sub1;
	struct boolexp *sub2;
	dbref thing;
	struct plist *prop_check;
};

#define TRUE_BOOLEXP ((struct boolexp *) 0)

/* special dbref's */
#define NOTHING ((dbref) -1)	/* null dbref */
#define AMBIGUOUS ((dbref) -2)	/* multiple possibilities, for matchers */
#define HOME ((dbref) -3)		/* virtual room, represents mover's home */

/* editor data structures */

/* Line data structure */
struct line {
	const char *this_line;		/* the line itself */
	struct line *next, *prev;	/* the next line and the previous line */
};

/* constants and defines for MUV data types */
#define MUV_ARRAY_OFFSET		16
#define MUV_ARRAY_MASK			(0xff << MUV_ARRAY_OFFSET)
#define MUV_ARRAYOF(x)			(x + (1 << MUV_ARRAY_OFFSET))
#define MUV_TYPEOF(x)			(x & ~MUV_ARRAY_MASK)
#define MUV_ARRAYSETLEVEL(x,l)	((l << MUV_ARRAY_OFFSET) | MUF_TYPEOF(x))
#define MUV_ARRAYGETLEVEL(x)	((x & MUV_ARRAY_MASK) >> MUV_ARRAY_OFFSET)


/* stack and object declarations */
/* Integer types go here */
#define PROG_VARIES      255    /* MUV flag denoting variable number of args */
#define PROG_VOID        254    /* MUV void return type */
#define PROG_UNTYPED     253    /* MUV unknown var type */

#define PROG_CLEARED     0
#define PROG_PRIMITIVE   1		/* forth prims and hard-coded C routines */
#define PROG_INTEGER     2		/* integer types */
#define PROG_FLOAT       3		/* float types */
#define PROG_OBJECT      4		/* database objects */
#define PROG_VAR         5		/* variables */
#define PROG_LVAR        6		/* local variables, unique per program */
#define PROG_SVAR        7		/* scoped variables, unique per procedure */

/* Pointer types go here, numbered *AFTER* PROG_STRING */
#define PROG_STRING      9		/* string types */
#define PROG_FUNCTION    10		/* function names for debugging. */
#define PROG_LOCK        11		/* boolean expression */
#define PROG_ADD         12		/* program address - used in calls&jmps */
#define PROG_IF          13		/* A low level IF statement */
#define PROG_EXEC        14		/* EXECUTE shortcut */
#define PROG_JMP         15		/* JMP shortcut */
#define PROG_ARRAY       16		/* Array of other stack items. */
#define PROG_MARK        17		/* Stack marker for [ and ] */
#define PROG_SVAR_AT     18		/* @ shortcut for scoped vars */
#define PROG_SVAR_AT_CLEAR 19	/* @ for scoped vars, with var clear optim */
#define PROG_SVAR_BANG   20		/* ! shortcut for scoped vars */
#define PROG_TRY         21		/* TRY shortcut */
#define PROG_LVAR_AT     22		/* @ shortcut for local vars */
#define PROG_LVAR_AT_CLEAR 23	/* @ for local vars, with var clear optim */
#define PROG_LVAR_BANG   24		/* ! shortcut for local vars */

#define MAX_VAR         54		/* maximum number of variables including the
								   * basic ME, LOC, TRIGGER, and COMMAND vars */
#define RES_VAR          4		/* no of reserved variables */

#define STACK_SIZE       1024	/* maximum size of stack */

struct shared_string {			/* for sharing strings in programs */
	int links;					/* number of pointers to this struct */
	int length;					/* length of string data */
	char data[1];				/* shared string data */
};

struct prog_addr {				/* for 'address references */
	int links;					/* number of pointers */
	dbref progref;				/* program dbref */
	struct inst *data;			/* pointer to the code */
};

struct stack_addr {				/* for the system callstack */
	dbref progref;				/* program call was made from */
	struct inst *offset;		/* the address of the call */
};

struct stk_array_t;

struct muf_proc_data {
    char *procname;
	int vars;
	int args;
	const char **varnames;
};

struct inst {					/* instruction */
	short type;
	short line;
	union {
		struct shared_string *string;	/* strings */
		struct boolexp *lock;	/* booleam lock expression */
		int number;				/* used for both primitives and integers */
		double fnumber;			/* used for float storage */
		dbref objref;			/* object reference */
		struct stk_array_t *array;	/* pointer to muf array */
		struct inst *call;		/* use in IF and JMPs */
		struct prog_addr *addr;	/* the result of 'funcname */
		struct muf_proc_data *mufproc;	/* Data specific to each procedure */
	} data;
};

#include "array.h"
#include "mufevent.h"

typedef struct inst vars[MAX_VAR];

struct forvars {
	int didfirst;
	struct inst cur;
	struct inst end;
	int step;
	struct forvars *next;
};

struct tryvars {
	int depth;
	int call_level;
	int for_count;
	struct inst *addr;
	struct tryvars *next;
};

struct stack {
	int top;
	struct inst st[STACK_SIZE];
};

struct sysstack {
	int top;
	struct stack_addr st[STACK_SIZE];
};

struct callstack {
	int top;
	dbref st[STACK_SIZE];
};

struct localvars {
	struct localvars *next;
	struct localvars **prev;
	dbref prog;
	vars lvars;
};

struct forstack {
	int top;
	struct forvars *st;
};

struct trystack {
	int top;
	struct tryvars *st;
};

#define MAX_BREAKS 16
struct debuggerdata {
	unsigned debugging:1;		/* if set, this frame is being debugged */
	unsigned force_debugging:1;	/* if set, debugger is active, even if not set Z */
	unsigned bypass:1;			/* if set, bypass breakpoint on starting instr */
	unsigned isread:1;			/* if set, the prog is trying to do a read */
	unsigned showstack:1;		/* if set, show stack debug line, each inst. */
	unsigned dosyspop:1;		/* if set, fix up system stack before returning. */
	int lastlisted;				/* last listed line */
	char *lastcmd;				/* last executed debugger command */
	short breaknum;				/* the breakpoint that was just caught on */

	dbref lastproglisted;		/* What program's text was last loaded to list? */
	struct line *proglines;		/* The actual program text last loaded to list. */

	short count;				/* how many breakpoints are currently set */
	short temp[MAX_BREAKS];		/* is this a temp breakpoint? */
	short level[MAX_BREAKS];	/* level breakpnts.  If -1, no check. */
	struct inst *lastpc;		/* Last inst interped.  For inst changes. */
	struct inst *pc[MAX_BREAKS];	/* pc breakpoint.  If null, no check. */
	int pccount[MAX_BREAKS];	/* how many insts to interp.  -2 for inf. */
	int lastline;				/* Last line interped.  For line changes. */
	int line[MAX_BREAKS];		/* line breakpts.  -1 no check. */
	int linecount[MAX_BREAKS];	/* how many lines to interp.  -2 for inf. */
	dbref prog[MAX_BREAKS];		/* program that breakpoint is in. */
};

struct scopedvar_t {
	int count;
	const char** varnames;
	struct scopedvar_t *next;
	struct inst vars[1];
};

struct dlogidlist {
	struct dlogidlist *next;
	char dlogid[32];
};

struct mufwatchpidlist {
	struct mufwatchpidlist *next;
	int pid;
};

#define dequeue_prog(x,i) dequeue_prog_real(x,i,__FILE__,__LINE__)

#define STD_REGUID 0
#define STD_SETUID 1
#define STD_HARDUID 2

/* frame data structure necessary for executing programs */
struct frame {
	struct frame *next;
	struct sysstack system;		/* system stack */
	struct stack argument;		/* argument stack */
	struct callstack caller;	/* caller prog stack */
	struct forstack fors;		/* for loop stack */
	struct trystack trys;		/* try block stack */
	struct localvars* lvars;	/* local variables */
	vars variables;				/* global variables */
	struct inst *pc;			/* next executing instruction */
	short writeonly;			/* This program should not do reads */
	short multitask;			/* This program's multitasking mode */
	short timercount;			/* How many timers currently exist. */
	short level;				/* prevent interp call loops */
	int perms;					/* permissions restrictions on program */
	short already_created;		/* this prog already created an object */
	short been_background;		/* this prog has run in the background */
	short skip_declare;         /* tells interp to skip next scoped var decl */
	short wantsblanks;          /* specifies program will accept blank READs */
	dbref trig;					/* triggering object */
	long started;				/* When this program started. */
	int instcnt;				/* How many instructions have run. */
	int pid;					/* what is the process id? */
	char* errorstr;             /* the error string thrown */
	char* errorinst;            /* the instruction name that threw an error */
	dbref errorprog;            /* the program that threw an error */
	int errorline;              /* the program line that threw an error */
	int descr;					/* what is the descriptor that started this? */
	void *rndbuf;				/* buffer for seedable random */
	struct scopedvar_t *svars;	/* Variables with function scoping. */
	struct debuggerdata brkpt;	/* info the debugger needs */
	struct timeval proftime;    /* profiling timing code */
    struct timeval totaltime;   /* profiling timing code */
	struct mufevent *events;	/* MUF event list. */
	struct dlogidlist *dlogids;	/* List of dlogids this frame uses. */
	struct mufwatchpidlist *waiters;
	struct mufwatchpidlist *waitees;
	union {
		struct {
			unsigned int div_zero:1;	/* Divide by zero */
			unsigned int nan:1;	/* Result would not be a number */
			unsigned int imaginary:1;	/* Result would be imaginary */
			unsigned int f_bounds:1;	/* Float boundary error */
			unsigned int i_bounds:1;	/* Integer boundary error */
		} error_flags;
		int is_flags;
	} error;
};


struct publics {
	char *subname;
	int mlev;
	union {
		struct inst *ptr;
		int no;
	} addr;
	struct publics *next;
};


struct mcp_binding {
	struct mcp_binding *next;

	char *pkgname;
	char *msgname;
	struct inst *addr;
};

struct program_specific {
	unsigned short instances;	/* number of instances of this prog running */
	short curr_line;			/* current-line */
	int siz;					/* size of code */
	struct inst *code;			/* byte-compiled code */
	struct inst *start;			/* place to start executing */
	struct line *first;			/* first line */
	struct publics *pubs;		/* public subroutine addresses */
	struct mcp_binding *mcpbinds;	/* MCP message bindings. */
	struct timeval proftime;	/* profiling time spent in this program. */
	time_t profstart;			/* time when profiling started for this prog */
	unsigned int profuses;		/* #calls to this program while profiling */
};

#define PROGRAM_SP(x)			(DBFETCH(x)->sp.program.sp)
#define ALLOC_PROGRAM_SP(x)     { PROGRAM_SP(x) = (struct program_specific *)malloc(sizeof(struct program_specific)); }
#define FREE_PROGRAM_SP(x)      { dbref foo = x; if(PROGRAM_SP(foo)) free(PROGRAM_SP(foo)); PROGRAM_SP(foo) = (struct program_specific *)NULL; }

#define PROGRAM_INSTANCES(x)	(PROGRAM_SP(x)!=NULL?PROGRAM_SP(x)->instances:0)
#define PROGRAM_CURR_LINE(x)	(PROGRAM_SP(x)->curr_line)
#define PROGRAM_SIZ(x)			(PROGRAM_SP(x)->siz)
#define PROGRAM_CODE(x)			(PROGRAM_SP(x)->code)
#define PROGRAM_START(x)		(PROGRAM_SP(x)->start)
#define PROGRAM_FIRST(x)		(PROGRAM_SP(x)->first)
#define PROGRAM_PUBS(x)			(PROGRAM_SP(x)->pubs)
#define PROGRAM_MCPBINDS(x)		(PROGRAM_SP(x)->mcpbinds)
#define PROGRAM_PROFTIME(x)		(PROGRAM_SP(x)->proftime)
#define PROGRAM_PROFSTART(x)	(PROGRAM_SP(x)->profstart)
#define PROGRAM_PROF_USES(x)	(PROGRAM_SP(x)->profuses)

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
#define PROGRAM_SET_MCPBINDS(x,y)	(PROGRAM_SP(x)->mcpbinds = y)
#define PROGRAM_SET_PROFTIME(x,y,z)	(PROGRAM_SP(x)->proftime.tv_usec = z, PROGRAM_SP(x)->proftime.tv_sec = y)
#define PROGRAM_SET_PROFSTART(x,y)	(PROGRAM_SP(x)->profstart = y)
#define PROGRAM_SET_PROF_USES(x,y)	(PROGRAM_SP(x)->profuses = y)


struct player_specific {
	dbref home;
	dbref curr_prog;			/* program I'm currently editing */
	short insert_mode;			/* in insert mode? */
	short block;
	const char *password;
	int *descrs;
	int descr_count;
	dbref* ignore_cache;
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
#define PLAYER_DESCRS(x)    (PLAYER_SP(x)->descrs)
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

union specific {				/* I've been railroaded! */
	struct {					/* ROOM-specific fields */
		dbref dropto;
	} room;
/*    struct {		*//* THING-specific fields */
/*	dbref   home;   */
/*    }       thing;    */
	struct {					/* EXIT-specific fields */
		int ndest;
		dbref *dest;
	} exit;
	struct {					/* PLAYER-specific fields */
		struct player_specific *sp;
	} player;
	struct {					/* PROGRAM-specific fields */
		struct program_specific *sp;
	} program;
};


/* timestamps record */

struct timestamps {
	time_t created;
	time_t modified;
	time_t lastused;
	int usecount;
};


struct object {

	const char *name;
	dbref location;				/* pointer to container */
	dbref owner;
	dbref contents;
	dbref exits;
	dbref next;					/* pointer to next in contents/exits chain */
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

	struct timestamps ts;
	union specific sp;
};

struct macrotable {
	char *name;
	char *definition;
	dbref implementor;
	struct macrotable *left;
	struct macrotable *right;
};

/* Possible data types that may be stored in a hash table */
union u_hash_data {
	int ival;					/* Store compiler tokens here */
	dbref dbval;				/* Player hashing will want this */
	void *pval;					/* compiler $define strings use this */
};

/* The actual hash entry for each item */
struct t_hash_entry {
	struct t_hash_entry *next;	/* Pointer for conflict resolution */
	const char *name;			/* The name of the item */
	union u_hash_data dat;		/* Data value for item */
};

typedef union u_hash_data hash_data;
typedef struct t_hash_entry hash_entry;
typedef hash_entry *hash_tab;

#define PLAYER_HASH_SIZE   (1024)	/* Table for player lookups */
#define COMP_HASH_SIZE     (256)	/* Table for compiler keywords */
#define DEFHASHSIZE        (256)	/* Table for compiler $defines */

extern struct object *db;
extern struct macrotable *macrotop;
extern dbref db_top;

#ifndef MALLOC_PROFILING
extern char *alloc_string(const char *);
extern struct shared_string *alloc_prog_string(const char *);
#endif

extern dbref new_object(void);		/* return a new object */

extern struct boolexp *getboolexp(FILE *);	/* get a boolexp */
extern void putboolexp(FILE *, struct boolexp *);	/* put a boolexp */

extern int db_write_object(FILE *, dbref);	/* write one object to file */

extern dbref db_write(FILE * f);	/* write db to file, return # of objects */

extern dbref db_read(FILE * f);	/* read db from file, return # of objects */

 /* Warning: destroys existing db contents! */

extern void db_free(void);

extern dbref parse_dbref(const char *);	/* parse a dbref */

#define DOLIST(var, first) \
  for((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)
#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}
#define getloc(thing) (DBFETCH(thing)->location)

/*
  Usage guidelines:

  To obtain an object pointer use DBFETCH(i).  Pointers returned by DBFETCH
  may become invalid after a call to new_object().

  To update an object, use DBSTORE(i, f, v), where i is the object number,
  f is the field (after ->), and v is the new value.

  If you have updated an object without using DBSTORE, use DBDIRTY(i) before
  leaving the routine that did the update.

  When using PUSH, be sure to call DBDIRTY on the object which contains
  the locative (see PUSH definition above).

  Some fields are now handled in a unique way, since they are always memory
  resident, even in the GDBM_DATABASE disk-based muck.  These are: name,
  flags and owner.  Refer to these by NAME(i), FLAGS(i) and OWNER(i).
  Always call DBDIRTY(i) after updating one of these fields.

  The programmer is responsible for managing storage for string
  components of entries; db_read will produce malloc'd strings.  The
  alloc_string routine is provided for generating malloc'd strings
  duplicates of other strings.  Note that db_free and db_read will
  attempt to free any non-NULL string that exists in db when they are
  invoked.
*/

#endif /* _DB_H */
