#include "config.h"

#include "array.h"
#include "db.h"
#include "debugger.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "mufevent.h"
#include "mpi.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

#define TQ_MUF_TYP 0
#define TQ_MPI_TYP 1

#define TQ_MUF_QUEUE    0x0
#define TQ_MUF_DELAY    0x1
#define TQ_MUF_LISTEN   0x2
#define TQ_MUF_READ     0x3
#define TQ_MUF_TREAD    0x4
#define TQ_MUF_TIMER    0x5

#define TQ_MPI_QUEUE    0x0
#define TQ_MPI_DELAY    0x1

#define TQ_MPI_SUBMASK  0x7
#define TQ_MPI_LISTEN   0x8
#define TQ_MPI_OMESG   0x10
#define TQ_MPI_BLESSED 0x20

typedef struct timenode {
    struct timenode *next;
    int typ;
    int subtyp;
    time_t when;
    int descr;
    dbref called_prog;
    char *called_data;
    char *command;
    char *str3;
    dbref uid;
    dbref loc;
    dbref trig;
    struct frame *fr;
    struct inst *where;
    int eventnum;
} *timequeue;

/*
 * Events types and data:
 *  What, typ, sub, when, user, where, trig, prog, frame, str1, cmdstr, str3
 *  qmpi   1    0   1     user  loc    trig  --    --     mpi   cmd     arg
 *  dmpi   1    1   when  user  loc    trig  --    --     mpi   cmd     arg
 *  lmpi   1    8   1     spkr  loc    lstnr --    --     mpi   cmd     heard
 *  oqmpi  1   16   1     user  loc    trig  --    --     mpi   cmd     arg
 *  odmpi  1   17   when  user  loc    trig  --    --     mpi   cmd     arg
 *  olmpi  1   24   1     spkr  loc    lstnr --    --     mpi   cmd     heard
 *  qmuf   0    0   0     user  loc    trig  prog  --     stk_s cmd@    --
 *  lmuf   0    1   0     spkr  loc    lstnr prog  --     heard cmd@    --
 *  dmuf   0    2   when  user  loc    trig  prog  frame  mode  --      --
 *  rmuf   0    3   -1    user  loc    trig  prog  frame  mode  --      --
 *  trmuf  0    4   when  user  loc    trig  prog  frame  mode  --      --
 *  tevmuf 0    5   when  user  loc    trig  prog  frame  mode  event   --
 */

static timequeue tqhead = NULL;

static int process_count = 0;

static timequeue free_timenode_list = NULL;
static int free_timenode_count = 0;

static timequeue
alloc_timenode(int typ, int subtyp, time_t mytime, int descr, dbref player,
	       dbref loc, dbref trig, dbref program, struct frame *fr,
	       const char *strdata, const char *strcmd, const char *str3, timequeue nextone)
{
    timequeue ptr;

    if (free_timenode_list) {
	ptr = free_timenode_list;
	free_timenode_list = ptr->next;
	free_timenode_count--;
    } else {
	ptr = (timequeue) malloc(sizeof(struct timenode));
    }
    ptr->typ = typ;
    ptr->subtyp = subtyp;
    ptr->when = mytime;
    ptr->uid = player;
    ptr->loc = loc;
    ptr->trig = trig;
    ptr->descr = descr;
    ptr->fr = fr;
    ptr->called_prog = program;
    ptr->called_data = (char *) strdup((char *) strdata);
    ptr->command = alloc_string(strcmd);
    ptr->str3 = alloc_string(str3);
    ptr->eventnum = (fr) ? fr->pid : top_pid++;
    ptr->next = nextone;
    return (ptr);
}

static void
free_timenode(timequeue ptr)
{
    if (!ptr) {
	log_status("WARNING: free_timenode(): NULL ptr passed !  Ignored.");
	return;
    }

    if (ptr->command)
	free(ptr->command);
    if (ptr->called_data)
	free(ptr->called_data);
    if (ptr->str3)
	free(ptr->str3);
    if (ptr->fr) {
	DEBUGPRINT("free_timenode: ptr->type = MUF? %d  ptr->subtyp = MUF_TIMER? %d\n",
		   (ptr->typ == TQ_MUF_TYP), (ptr->subtyp == TQ_MUF_TIMER));
	if (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER) {
	    if (ptr->fr->multitask != BACKGROUND)
		PLAYER_SET_BLOCK(ptr->uid, 0);
	    prog_clean(ptr->fr);
	}
	if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
				       ptr->subtyp == TQ_MUF_TREAD)) {
	    FLAGS(ptr->uid) &= ~INTERACTIVE;
	    FLAGS(ptr->uid) &= ~READMODE;
	    notify_nolisten(ptr->uid,
			    "Data input aborted.  The command you were using was killed.", 1);
	}
    }
    if (free_timenode_count < tp_free_frames_pool) {
	ptr->next = free_timenode_list;
	free_timenode_list = ptr;
	free_timenode_count++;
    } else {
	free(ptr);
    }
}

#ifdef MEMORY_CLEANUP
void
purge_timenode_free_pool(void)
{
    timequeue ptr = free_timenode_list;
    timequeue nxt = NULL;

    while (ptr) {
	nxt = ptr->next;
	free((void *) ptr);
	ptr = nxt;
    }
    free_timenode_count = 0;
    free_timenode_list = NULL;
}
#endif

int
control_process(dbref player, int pid)
{
    timequeue ptr = tqhead;

    while ((ptr) && (pid != ptr->eventnum)) {
	ptr = ptr->next;
    }

    /* If the process isn't in the timequeue, that means it's
       waiting for an event, so let the event code handle
       it. */

    if (!ptr) {
	return muf_event_controls(player, pid);
    }

    /* However, if it is in the timequeue, we have to handle it.
       Other than a Wizard, there are three people who can kill it:
       the owner of the program, the owner of the trigger, and the
       person who is currently running it. */

    if (!controls(player, ptr->called_prog) && !controls(player, ptr->trig)
	&& (player != ptr->uid)) {
	return 0;
    }
    return 1;
}

static int
add_event(int event_typ, int subtyp, int dtime, int descr, dbref player, dbref loc,
	  dbref trig, dbref program, struct frame *fr,
	  const char *strdata, const char *strcmd, const char *str3)
{
    timequeue ptr;
    timequeue lastevent = NULL;
    time_t rtime = time((time_t *) NULL) + (time_t) dtime;
    int mypids = 0;

    if (tqhead) {
	for (ptr = tqhead, mypids = 0; ptr; ptr = ptr->next) {
	    if (ptr->uid == player)
		mypids++;
	    lastevent = ptr;
	}
    }

    if (event_typ == TQ_MUF_TYP && subtyp == TQ_MUF_READ) {
	process_count++;
	if (lastevent) {
	    lastevent->next = alloc_timenode(event_typ, subtyp, rtime, descr,
					     player, loc, trig, program, fr,
					     strdata, strcmd, str3, NULL);
	    return (lastevent->next->eventnum);
	} else {
	    tqhead = alloc_timenode(event_typ, subtyp, rtime, descr,
				    player, loc, trig, program, fr,
				    strdata, strcmd, str3, NULL);
	    return (tqhead->eventnum);
	}
    }
    if (!(event_typ == TQ_MUF_TYP && subtyp == TQ_MUF_TREAD)) {
	if (process_count > tp_max_process_limit ||
	    (mypids > tp_max_plyr_processes && !Wizard(OWNER(player)))) {
	    if (fr) {
		if (fr->multitask != BACKGROUND)
		    PLAYER_SET_BLOCK(player, 0);
		prog_clean(fr);
	    }
	    notify_nolisten(player, "Event killed.  Timequeue table full.", 1);
	    return 0;
	}
    }
    process_count++;

    if (!tqhead) {
	tqhead = alloc_timenode(event_typ, subtyp, rtime, descr, player, loc, trig,
				program, fr, strdata, strcmd, str3, NULL);
	return (tqhead->eventnum);
    }
    if (rtime < tqhead->when || (tqhead->typ == TQ_MUF_TYP && tqhead->subtyp == TQ_MUF_READ)
	    ) {
	tqhead = alloc_timenode(event_typ, subtyp, rtime, descr, player, loc, trig,
				program, fr, strdata, strcmd, str3, tqhead);
	return (tqhead->eventnum);
    }

    ptr = tqhead;
    while (ptr && ptr->next && rtime >= ptr->next->when &&
	   !(ptr->next->typ == TQ_MUF_TYP && ptr->next->subtyp == TQ_MUF_READ)) {
	ptr = ptr->next;
    }

    ptr->next = alloc_timenode(event_typ, subtyp, rtime, descr, player, loc, trig,
			       program, fr, strdata, strcmd, str3, ptr->next);
    return (ptr->next->eventnum);
}

int
add_mpi_event(int delay, int descr, dbref player, dbref loc, dbref trig,
	      const char *mpi, const char *cmdstr, const char *argstr,
	      int listen_p, int omesg_p, int blessed_p)
{
    int subtyp = TQ_MPI_QUEUE;

    if (delay >= 1) {
	subtyp = TQ_MPI_DELAY;
    }
    if (blessed_p) {
	subtyp |= TQ_MPI_BLESSED;
    }
    if (listen_p) {
	subtyp |= TQ_MPI_LISTEN;
    }
    if (omesg_p) {
	subtyp |= TQ_MPI_OMESG;
    }
    return add_event(TQ_MPI_TYP, subtyp, delay, descr, player, loc, trig,
		     NOTHING, NULL, mpi, cmdstr, argstr);
}

int
add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig, dbref prog,
		    const char *argstr, const char *cmdstr, int listen_p)
{
    return add_event(TQ_MUF_TYP, (listen_p ? TQ_MUF_LISTEN : TQ_MUF_QUEUE), 0,
		     descr, player, loc, trig, prog, NULL, argstr, cmdstr, NULL);
}

int
add_muf_delayq_event(int delay, int descr, dbref player, dbref loc, dbref trig,
		     dbref prog, const char *argstr, const char *cmdstr, int listen_p)
{
    return add_event(TQ_MUF_TYP, (listen_p ? TQ_MUF_LISTEN : TQ_MUF_QUEUE),
		     delay, descr, player, loc, trig, prog, NULL, argstr, cmdstr, NULL);
}

int
add_muf_read_event(int descr, dbref player, dbref prog, struct frame *fr)
{
    if (!fr) {
	panic("add_muf_read_event(): NULL frame passed !");
    }

    FLAGS(player) |= (INTERACTIVE | READMODE);
    return add_event(TQ_MUF_TYP, TQ_MUF_READ, -1, descr, player, -1, fr->trig,
		     prog, fr, "READ", NULL, NULL);
}

int
add_muf_tread_event(int descr, dbref player, dbref prog, struct frame *fr, int delay)
{
    if (!fr) {
	panic("add_muf_tread_event(): NULL frame passed !");
    }

    FLAGS(player) |= (INTERACTIVE | READMODE);
    return add_event(TQ_MUF_TYP, TQ_MUF_TREAD, delay, descr, player, -1, fr->trig,
		     prog, fr, "READ", NULL, NULL);
}

int
add_muf_timer_event(int descr, dbref player, dbref prog, struct frame *fr, int delay, char *id)
{
    if (!fr) {
	panic("add_muf_timer_event(): NULL frame passed !");
    }

    char buf[40];
    snprintf(buf, sizeof(buf), "TIMER.%.32s", id);
    fr->timercount++;
    return add_event(TQ_MUF_TYP, TQ_MUF_TIMER, delay, descr, player, -1, fr->trig,
		     prog, fr, buf, NULL, NULL);
}

int
add_muf_delay_event(int delay, int descr, dbref player, dbref loc, dbref trig, dbref prog,
		    struct frame *fr, const char *mode)
{
    return add_event(TQ_MUF_TYP, TQ_MUF_DELAY, delay, descr, player, loc, trig,
		     prog, fr, mode, NULL, NULL);
}

int
read_event_notify(int descr, dbref player, const char *cmd)
{
    timequeue ptr;

    if (muf_event_read_notify(descr, player, cmd)) {
	return 1;
    }

    ptr = tqhead;
    while (ptr) {
	if (ptr->uid == player) {
	    if (ptr->fr && ptr->fr->multitask != BACKGROUND) {
		if (*cmd || ptr->fr->wantsblanks) {
		    struct inst temp;

		    temp.type = PROG_INTEGER;
		    temp.data.number = descr;
		    muf_event_add(ptr->fr, "READ", &temp, 1);
		    return 1;
		}
	    }
	}
	ptr = ptr->next;
    }
    return 0;
}

void
handle_read_event(int descr, dbref player, const char *command)
{
    struct frame *fr;
    timequeue ptr, lastevent;
    int flag, typ, nothing_flag;
    int oldflags;
    dbref prog;

    nothing_flag = 0;
    if (command == NULL) {
	nothing_flag = 1;
    }
    oldflags = FLAGS(player);
    FLAGS(player) &= ~(INTERACTIVE | READMODE);

    ptr = tqhead;
    lastevent = NULL;
    while (ptr) {
	if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
				       ptr->subtyp == TQ_MUF_TREAD) && ptr->uid == player) {
	    break;
	}
	lastevent = ptr;
	ptr = ptr->next;
    }

    /*
     * When execution gets to here, either ptr will point to the
     * READ event for the player, or else ptr will be NULL.
     */

    if (ptr) {
	/* remember our program, and our execution frame. */
	fr = ptr->fr;
	if (fr == NULL) {
	    log_status("WARNING: handle_read_event(): NULL frame !  Ignored.");
	    return;
	}
	if (!fr->brkpt.debugging || fr->brkpt.isread) {
	    if (!fr->wantsblanks && command && !*command) {
		FLAGS(player) = oldflags;
		return;
	    }
	}
	typ = ptr->subtyp;
	prog = ptr->called_prog;
	if (command) {
	    /* remove the READ timequeue node from the timequeue */
	    process_count--;
	    if (lastevent) {
		lastevent->next = ptr->next;
	    } else {
		tqhead = ptr->next;
	    }
	}
	/* remember next timequeue node, to check for more READs later */
	lastevent = ptr;
	ptr = ptr->next;

	/* Make SURE not to let the program frame get freed.  We need it. */
	lastevent->fr = NULL;
	if (command) {
	    /*
	     * Free up the READ timequeue node
	     * we just removed from the queue.
	     */
	    free_timenode(lastevent);
	}

	if (fr->brkpt.debugging && !fr->brkpt.isread) {

	    /* We're in the MUF debugger!  Call it with the input line. */
	    if (command) {
		if (muf_debugger(descr, player, prog, command, fr)) {
		    /* MUF Debugger exited.  Free up the program frame & exit */
		    prog_clean(fr);
		    return;
		}
	    } else {
		if (muf_debugger(descr, player, prog, "", fr)) {
		    /* MUF Debugger exited.  Free up the program frame & exit */
		    prog_clean(fr);
		    return;
		}
	    }
	} else {
	    /* This is a MUF READ event. */
	    if (command && !strcasecmp(command, BREAK_COMMAND)) {

		/* Whoops!  The user typed @Q.  Free the frame and exit. */
		prog_clean(fr);
		return;
	    }

	    if ((fr->argument.top >= STACK_SIZE) ||
		(nothing_flag && fr->argument.top >= STACK_SIZE - 1)) {

		/*
		 * Uh oh! That MUF program's stack is full!
		 * Print an error, free the frame, and exit.
		 */
		notify_nolisten(player, "Program stack overflow.", 1);
		prog_clean(fr);
		return;
	    }

	    /*
	     * Everything looks okay.  Lets stuff the input line
	     * on the program's argument stack as a string item.
	     */
	    fr->argument.st[fr->argument.top].type = PROG_STRING;
	    fr->argument.st[fr->argument.top++].data.string =
		    alloc_prog_string(DoNull(command));
	    if (typ == TQ_MUF_TREAD) {
		if (nothing_flag) {
		    fr->argument.st[fr->argument.top].type = PROG_INTEGER;
		    fr->argument.st[fr->argument.top++].data.number = 0;
		} else {
		    fr->argument.st[fr->argument.top].type = PROG_INTEGER;
		    fr->argument.st[fr->argument.top++].data.number = 1;
		}
	    }
	}

	/*
	 * When using the MUF Debugger, the debugger will set the
	 * INTERACTIVE bit on the user, if it does NOT want the MUF
	 * program to resume executing.
	 */
	flag = (FLAGS(player) & INTERACTIVE);

	if (!flag && fr) {
	    interp_loop(player, prog, fr, 0);
	    /* WORK: if more input is pending, send the READ mufevent again. */
	    /* WORK: if no input is pending, clear READ mufevent from all of this player's programs. */
	}

	/*
	 * Check for any other READ events for this player.
	 * If there are any, set the READ related flags.
	 */
	while (ptr) {
	    if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
					   ptr->subtyp == TQ_MUF_TREAD)) {
		if (ptr->uid == player) {
		    FLAGS(player) |= (INTERACTIVE | READMODE);
		}
	    }
	    ptr = ptr->next;
	}
    }
}

void
next_timequeue_event(void)
{
    struct frame *tmpfr;
    int tmpbl, tmpfg;
    timequeue lastevent, event;
    int maxruns = 0;
    int forced_pid = 0;
    time_t rtime;

    time(&rtime);

    lastevent = tqhead;
    while ((lastevent) && (rtime >= lastevent->when) && (maxruns < 10)) {
	lastevent = lastevent->next;
	maxruns++;
    }

    while (tqhead && (tqhead != lastevent) && (maxruns--)) {
	if (tqhead->typ == TQ_MUF_TYP && tqhead->subtyp == TQ_MUF_READ) {
	    break;
	}
	event = tqhead;
	tqhead = tqhead->next;
	process_count--;
	forced_pid = event->eventnum;
	event->eventnum = 0;
	if (event->typ == TQ_MPI_TYP) {
	    char cbuf[BUFFER_LEN];
	    int ival;

	    strcpyn(match_args, sizeof(match_args), DoNull(event->str3));
	    strcpyn(match_cmdname, sizeof(match_cmdname), DoNull(event->command));
	    ival = (event->subtyp & TQ_MPI_OMESG) ? MPI_ISPUBLIC : MPI_ISPRIVATE;
	    if (event->subtyp & TQ_MPI_BLESSED) {
		ival |= MPI_ISBLESSED;
	    }
	    if (event->subtyp & TQ_MPI_LISTEN) {
		ival |= MPI_ISLISTENER;
		do_parse_mesg(event->descr, event->uid, event->trig, event->called_data,
			      "(MPIlisten)", cbuf, sizeof(cbuf), ival);
	    } else if ((event->subtyp & TQ_MPI_SUBMASK) == TQ_MPI_DELAY) {
		do_parse_mesg(event->descr, event->uid, event->trig, event->called_data,
			      "(MPIdelay)", cbuf, sizeof(cbuf), ival);
	    } else {
		do_parse_mesg(event->descr, event->uid, event->trig, event->called_data,
			      "(MPIqueue)", cbuf, sizeof(cbuf), ival);
	    }
	    if (*cbuf) {
		if (!(event->subtyp & TQ_MPI_OMESG)) {
		    notify_filtered(event->uid, event->uid, cbuf, 1);
		} else {
		    char bbuf[BUFFER_LEN];
		    dbref plyr;

		    snprintf(bbuf, sizeof(bbuf), ">> %.4000s %.*s",
			     NAME(event->uid),
			     (int) (4000 - strlen(NAME(event->uid))),
			     pronoun_substitute(event->descr, event->uid, cbuf));
		    plyr = CONTENTS(event->loc);
		    for (; plyr != NOTHING; plyr = NEXTOBJ(plyr)) {
			if (Typeof(plyr) == TYPE_PLAYER && plyr != event->uid)
			    notify_filtered(event->uid, plyr, bbuf, 0);
		    }
		}
	    }
	} else if (event->typ == TQ_MUF_TYP) {
	    if (Typeof(event->called_prog) == TYPE_PROGRAM) {
		if (event->subtyp == TQ_MUF_DELAY) {
		    /* Uncomment when DBFETCH "does" something */
		    /* FIXME: DBFETCH(event->uid); */
		    tmpbl = PLAYER_BLOCK(event->uid);
		    tmpfg = (event->fr->multitask != BACKGROUND);
		    interp_loop(event->uid, event->called_prog, event->fr, 0);
		    if (!tmpfg) {
			PLAYER_SET_BLOCK(event->uid, tmpbl);
		    }
		} else if (event->subtyp == TQ_MUF_TIMER) {
		    struct inst temp;

		    temp.type = PROG_INTEGER;
		    temp.data.number = (int)event->when;
		    event->fr->timercount--;
		    muf_event_add(event->fr, event->called_data, &temp, 0);
		} else if (event->subtyp == TQ_MUF_TREAD) {
		    handle_read_event(event->descr, event->uid, NULL);
		} else {
		    strcpyn(match_args, sizeof(match_args), DoNull(event->called_data));
		    strcpyn(match_cmdname, sizeof(match_cmdname), DoNull(event->command));
		    tmpfr = interp(event->descr, event->uid, event->loc, event->called_prog,
				   event->trig, BACKGROUND, STD_HARDUID, forced_pid);
		    if (tmpfr) {
			interp_loop(event->uid, event->called_prog, tmpfr, 0);
		    }
		}
	    }
	}
	event->fr = NULL;
	free_timenode(event);
    }
}

int
in_timequeue(int pid)
{
    timequeue ptr = tqhead;

    if (!pid)
	return 0;
    if (muf_event_pid_frame(pid))
	return 1;
    if (!tqhead)
	return 0;
    while ((ptr) && (ptr->eventnum != pid))
	ptr = ptr->next;
    if (ptr)
	return 1;
    return 0;
}

struct frame *
timequeue_pid_frame(int pid)
{
    struct frame *out = NULL;
    timequeue ptr = tqhead;

    if (!pid)
	return NULL;
    out = muf_event_pid_frame(pid);
    if (out != NULL)
	return out;

    if (!tqhead)
	return NULL;
    while ((ptr) && (ptr->eventnum != pid))
	ptr = ptr->next;
    if (ptr)
	return ptr->fr;
    return NULL;
}

time_t
next_event_time(void)
{
    time_t rtime = time((time_t *) NULL);

    if (tqhead) {
	if (tqhead->when == -1) {
	    return (-1L);
	} else if (rtime >= tqhead->when) {
	    return (0L);
	} else {
	    return ((time_t) (tqhead->when - rtime));
	}
    }
    return (-1L);
}

/* Checks the MUF timequeue for address references on the stack or */
/* dbref references on the callstack */
static int
has_refs(dbref program, timequeue ptr)
{
    if (!ptr) {
	log_status("WARNING: has_refs(): NULL ptr passed !  Ignored.");
	return 0;
    }

    if (ptr->typ != TQ_MUF_TYP || !(ptr->fr) ||
	Typeof(program) != TYPE_PROGRAM || !(PROGRAM_INSTANCES(program)))
	return 0;

    for (int loop = 1; loop < ptr->fr->caller.top; loop++) {
	if (ptr->fr->caller.st[loop] == program)
	    return 1;
    }

    for (int loop = 0; loop < ptr->fr->argument.top; loop++) {
	if (ptr->fr->argument.st[loop].type == PROG_ADD &&
	    ptr->fr->argument.st[loop].data.addr->progref == program)
	    return 1;
    }

    return 0;
}

void
do_process_status(dbref player)
{
    char buf[BUFFER_LEN];
    char pidstr[128];
    char duestr[128];
    char runstr[128];
    char inststr[128];
    char cpustr[128];
    char progstr[128];
    char prognamestr[128];
    int count = 0;
    timequeue ptr = tqhead;
    time_t rtime = time((time_t *) NULL);
    time_t etime;
    double pcnt;
    char *strfmt = "**%10s %4s %4s %6s %4s %7s %-10.10s %-12s %.512s";

    notifyf_nolisten(player, strfmt, "PID", "Next", "Run", "KInst", "%CPU", "Prog#",
		     "ProgName", "Player", "");

    while (ptr) {
	/* pid */
	snprintf(pidstr, sizeof(pidstr), "%d", ptr->eventnum);
	/* next due */
	strcpyn(duestr, sizeof(duestr), ((ptr->when - rtime) > 0) ?
		time_format_2((time_t) (ptr->when - rtime)) : "Due");
	/* Run length */
	strcpyn(runstr, sizeof(runstr), ptr->fr ?
		time_format_2((time_t) (rtime - ptr->fr->started)) : "0s");
	/* Thousand Instructions executed */
	snprintf(inststr, sizeof(inststr), "%d", ptr->fr ? (ptr->fr->instcnt / 1000) : 0);

	/* If it's actually a program, instead of MPI... */
	/* we need to figure out how much CPU it's used */
	if (ptr->fr) {
	    etime = rtime - ptr->fr->started;
	    if (etime > 0) {
		pcnt = ptr->fr->totaltime.tv_sec;
		pcnt += ptr->fr->totaltime.tv_usec / 1000000;
		pcnt = pcnt * 100 / etime;
		if (pcnt > 99.9) {
		    pcnt = 99.9;
		}
	    } else {
		pcnt = 0.0;
	    }
	} else {
	    pcnt = 0.0;
	}
	snprintf(cpustr, sizeof(cpustr), "%4.1f", pcnt);
	/* Get the dbref! */
	if (ptr->fr) {
	    /* if it's a program... */
	    snprintf(progstr, sizeof(progstr), "#%d", ptr->fr->caller.st[1]);
	    snprintf(prognamestr, sizeof(prognamestr), "%s", NAME(ptr->fr->caller.st[1]));
	} else if (ptr->typ == TQ_MPI_TYP) {
	    /* if it's MPI... */
	    snprintf(progstr, sizeof(progstr), "#%d", ptr->trig);
	    snprintf(prognamestr, sizeof(prognamestr), "%s", "");
	} else {
	    /* if it's anything else... */
	    snprintf(progstr, sizeof(progstr), "#%d", ptr->called_prog);
	    snprintf(prognamestr, sizeof(prognamestr), "%s", NAME(ptr->called_prog));
	}

	/* Now, the next due is based on if it's waiting on a READ */
	if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_READ) {
	    strcpyn(duestr, sizeof(duestr), "--");
	} else if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_TIMER) {
	    /* if it's a timer event, it gives the eventnum */
	    snprintf(pidstr, sizeof(pidstr), "(%d)", ptr->eventnum);
	} else if (ptr->typ == TQ_MPI_TYP) {
	    /* and if it's MPI, undo most of the stuff we did
	     * before, and set it up for mostly MPI stuff */
	    strcpyn(runstr, sizeof(runstr), "--");
	    strcpyn(inststr, sizeof(inststr), "MPI");
	    strcpyn(cpustr, sizeof(cpustr), "--");
	}
	(void) snprintf(buf, sizeof(buf), strfmt, pidstr, duestr, runstr,
			inststr, cpustr, progstr, prognamestr, NAME(ptr->uid),
			DoNull(ptr->called_data));
	if (Wizard(OWNER(player)) || ptr->uid == player) {
	    notify_nolisten(player, buf, 1);
	} else if (ptr->called_prog != NOTHING && OWNER(ptr->called_prog) == OWNER(player)) {
	    notify_nolisten(player, buf, 1);
	}
	ptr = ptr->next;
	count++;
    }
    count += muf_event_list(player, strfmt);
    notifyf_nolisten(player, "%d events.", count);
}

stk_array *
get_pids(dbref ref)
{
    struct inst temp1, temp2;
    stk_array *nw;
    int count = 0;

    timequeue ptr = tqhead;
    nw = new_array_packed(0);
    while (ptr) {
	if (((ptr->typ != TQ_MPI_TYP) ? (ptr->called_prog == ref) : (ptr->trig == ref)) ||
	    (ptr->uid == ref) || (ref < 0)) {
	    temp2.type = PROG_INTEGER;
	    temp2.data.number = ptr->eventnum;
	    temp1.type = PROG_INTEGER;
	    temp1.data.number = count++;
	    array_setitem(&nw, &temp1, &temp2);
	    CLEAR(&temp1);
	    CLEAR(&temp2);
	}
	ptr = ptr->next;
    }
    nw = get_mufevent_pids(nw, ref);
    return nw;
}

stk_array *
get_pidinfo(int pid)
{
    struct inst temp1, temp2;
    stk_array *nw;
    time_t rtime = time(NULL);
    time_t etime = 0;
    double pcnt = 0.0;

    timequeue ptr = tqhead;
    nw = new_array_dictionary();
    while (ptr) {
	if (ptr->eventnum == pid) {
	    if (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER) {
		break;
	    }
	}
	ptr = ptr->next;
    }
    if (ptr && (ptr->eventnum == pid) &&
	(ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER)) {
	if (ptr->fr) {
	    etime = rtime - ptr->fr->started;
	    if (etime > 0) {
		pcnt = ptr->fr->totaltime.tv_sec;
		pcnt += ptr->fr->totaltime.tv_usec / 1000000;
		pcnt = pcnt * 100 / etime;
		if (pcnt > 100.0) {
		    pcnt = 100.0;
		}
	    } else {
		pcnt = 0.0;
	    }
	}
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("PID");
	temp2.type = PROG_INTEGER;
	temp2.data.number = ptr->eventnum;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("CALLED_PROG");
	temp2.type = PROG_OBJECT;
	temp2.data.objref = ptr->called_prog;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("TRIG");
	temp2.type = PROG_OBJECT;
	temp2.data.objref = ptr->trig;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("PLAYER");
	temp2.type = PROG_OBJECT;
	temp2.data.objref = ptr->uid;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("CALLED_DATA");
	temp2.type = PROG_STRING;
	temp2.data.string = alloc_prog_string(ptr->called_data);
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("INSTCNT");
	temp2.type = PROG_INTEGER;
	temp2.data.number = ptr->fr ? ptr->fr->instcnt : 0;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("DESCR");
	temp2.type = PROG_INTEGER;
	temp2.data.number = ptr->descr;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("CPU");
	temp2.type = PROG_FLOAT;
	temp2.data.fnumber = pcnt;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("NEXTRUN");
	temp2.type = PROG_INTEGER;
	temp2.data.number = (int) ptr->when;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("STARTED");
	temp2.type = PROG_INTEGER;
	temp2.data.number = (int) (ptr->fr ? ptr->fr->started : 0);
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("TYPE");
	temp2.type = PROG_STRING;
	temp2.data.string = (ptr->typ == TQ_MUF_TYP) ? alloc_prog_string("MUF") :
		(ptr->typ == TQ_MPI_TYP) ? alloc_prog_string("MPI") : alloc_prog_string("UNK");
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("SUBTYPE");
	temp2.type = PROG_STRING;
	if (ptr->typ == TQ_MUF_TYP) {
	    temp2.data.string = (ptr->subtyp == TQ_MUF_READ) ? alloc_prog_string("READ") :
		    (ptr->subtyp == TQ_MUF_TREAD) ? alloc_prog_string("TREAD") :
		    (ptr->subtyp == TQ_MUF_QUEUE) ? alloc_prog_string("QUEUE") :
		    (ptr->subtyp == TQ_MUF_LISTEN) ? alloc_prog_string("LISTEN") :
		    (ptr->subtyp == TQ_MUF_TIMER) ? alloc_prog_string("TIMER") :
		    (ptr->subtyp == TQ_MUF_DELAY) ? alloc_prog_string("DELAY") :
		    alloc_prog_string("");
	} else if (ptr->typ == TQ_MPI_TYP) {
	    int subtyp = (ptr->subtyp & TQ_MPI_SUBMASK);
	    temp2.data.string = (subtyp == TQ_MPI_QUEUE) ? alloc_prog_string("QUEUE") :
		    (subtyp == TQ_MPI_DELAY) ? alloc_prog_string("DELAY") :
		    alloc_prog_string("");
	} else {
	    temp2.data.string = alloc_prog_string("");
	}
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
    } else {
	nw = get_mufevent_pidinfo(nw, pid);
    }
    return nw;
}

/*
 * killmode values:
 *     0: kill all matching processes, MUF or MPI
 *     1: kill all matching MUF processes
 *     2: kill all matching foreground MUF processes
 */
int
dequeue_prog_real(dbref program, int killmode, const char *file, const int line)
{
    int count = 0, ocount;
    timequeue ptr;

#ifdef DEBUG
    fprintf(stderr, "[debug] dequeue_prog(#%d, %d) called from %s:%d\n", program, killmode,
	    file, line);
#endif				/* DEBUG */
    DEBUGPRINT("dequeue_prog: tqhead = %p\n", tqhead);
    while (tqhead) {
	DEBUGPRINT("dequeue_prog: tqhead->called_prog = #%d, has_refs = %d ",
		   tqhead->called_prog, has_refs(program, tqhead));
	DEBUGPRINT("tqhead->uid = #%d\n", tqhead->uid);
	if (tqhead->called_prog != program && !has_refs(program, tqhead)
	    && tqhead->uid != program) {
	    break;
	}
	if (killmode == 2) {
	    if (tqhead->fr && tqhead->fr->multitask == BACKGROUND) {
		break;
	    }
	} else if (killmode == 1) {
	    if (!tqhead->fr) {
		DEBUGPRINT("dequeue_prog: killmode 1, no frame\n");
		break;
	    }
	}
	ptr = tqhead;
	tqhead = tqhead->next;
	free_timenode(ptr);
	process_count--;
	count++;
    }

    if (tqhead) {
	for (timequeue tmp = tqhead, ptr = tqhead->next; ptr; tmp = ptr, ptr = ptr->next) {
	    DEBUGPRINT("dequeue_prog(2): ptr->called_prog=#%d, has_refs()=%d ",
		       ptr->called_prog, has_refs(program, ptr));
	    DEBUGPRINT("ptr->uid=#%d.\n", ptr->uid);
	    if (ptr->called_prog != program && !has_refs(program, ptr) && ptr->uid != program) {
		continue;
	    }
	    if (killmode == 2) {
		if (ptr->fr && ptr->fr->multitask == BACKGROUND) {
		    continue;
		}
	    } else if (killmode == 1) {
		if (!ptr->fr) {
		    DEBUGPRINT("dequeue_prog(2): killmode 1, no frame.\n");
		    continue;
		}
	    }
	    tmp->next = ptr->next;
	    free_timenode(ptr);
	    process_count--;
	    count++;
	    ptr = tmp;
	}

	DEBUGPRINT("dequeue_prog(3): about to muf_event_dequeue(#%d, %d)\n", program,
		   killmode);
	ocount = count;
	count += muf_event_dequeue(program, killmode);
	if (ocount < count && tqhead->fr)
	    prog_clean(tqhead->fr);
	for (ptr = tqhead; ptr; ptr = ptr->next) {
	    if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
					   ptr->subtyp == TQ_MUF_TREAD)) {
		FLAGS(ptr->uid) |= (INTERACTIVE | READMODE);
	    }
	}
    }
    /* and just to make sure we got them all... otherwise, we need
     * to rethink what we're doing here. */
    /* assert(PROGRAM_INSTANCES(program) == 0); */
#ifdef DEBUG
    /* KLUDGE by premchai21 */
    if (Typeof(program) == TYPE_PROGRAM)
	fprintf(stderr, "[debug] dequeue_prog: %d instances of #%d\n",
		PROGRAM_INSTANCES(program), program);
#endif
    return (count);
}

int
dequeue_process(int pid)
{
    timequeue tmp, ptr;
    int deqflag = 0;

    if (!pid)
	return 0;

    if (muf_event_dequeue_pid(pid)) {
	process_count--;
	deqflag = 1;
    }

    tmp = ptr = tqhead;
    while (ptr) {
	if (pid == ptr->eventnum) {
	    if (tmp == ptr) {
		tqhead = tmp = tmp->next;
		free_timenode(ptr);
		ptr = tmp;
	    } else {
		tmp->next = ptr->next;
		free_timenode(ptr);
		ptr = tmp->next;
	    }
	    process_count--;
	    deqflag = 1;
	} else {
	    tmp = ptr;
	    ptr = ptr->next;
	}
    }

    if (!deqflag) {
	return 0;
    }

    for (ptr = tqhead; ptr; ptr = ptr->next) {
	if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
				       ptr->subtyp == TQ_MUF_TREAD)) {
	    FLAGS(ptr->uid) |= (INTERACTIVE | READMODE);
	}
    }
    return 1;
}

int
dequeue_timers(int pid, char *id)
{
    char buf[40];
    timequeue tmp, ptr;
    int deqflag = 0;

    if (!pid)
	return 0;

    if (id)
	snprintf(buf, sizeof(buf), "TIMER.%.30s", id);

    tmp = ptr = tqhead;
    while (ptr) {
	if (pid == ptr->eventnum &&
	    ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_TIMER &&
	    (!id || !strcmp(ptr->called_data, buf))) {
	    if (tmp == ptr) {
		tqhead = tmp = tmp->next;
		ptr->fr->timercount--;
		ptr->fr = NULL;
		free_timenode(ptr);
		ptr = tmp;
	    } else {
		tmp->next = ptr->next;
		ptr->fr->timercount--;
		ptr->fr = NULL;
		free_timenode(ptr);
		ptr = tmp->next;
	    }
	    process_count--;
	    deqflag = 1;
	} else {
	    tmp = ptr;
	    ptr = ptr->next;
	}
    }

    return deqflag;
}

void
do_kill_process(int descr, dbref player, const char *arg1)
{
    int count;
    dbref match;
    struct match_data md;
    timequeue tmp, ptr = tqhead;


    if (*arg1 == '\0') {
	notify_nolisten(player, "What event do you want to dequeue?", 1);
    } else {
	if (!strcasecmp(arg1, "all")) {
	    if (!Wizard(OWNER(player))) {
		notify_nolisten(player, "Permission denied", 1);
		return;
	    }
	    while (ptr) {
		tmp = ptr;
		tqhead = ptr = ptr->next;
		free_timenode(tmp);
		process_count--;
	    }
	    tqhead = NULL;
	    muf_event_dequeue(NOTHING, 0);
	    notify_nolisten(player, "Time queue cleared.", 1);
	} else {
	    if (!number(arg1)) {
		init_match(descr, player, arg1, NOTYPE, &md);
		match_absolute(&md);
		match_everything(&md);

		if ((match = noisy_match_result(&md)) == NOTHING) {
		    return;
		}

		if (!OkObj(match)) {
		    notify_nolisten(player, "I don't recognize that object.", 1);
		    return;
		}
		if ((!Wizard(OWNER(player))) && (OWNER(match) != OWNER(player))) {
		    notify_nolisten(player, "Permission denied.", 1);
		    return;
		}
		count = dequeue_prog(match, 0);
		if (!count) {
		    notify_nolisten(player, "That program wasn't in the time queue.", 1);
		    return;
		}
		if (count > 1) {
		    notifyf_nolisten(player, "%d processes dequeued.", count);
		} else {
		    notify_nolisten(player, "Process dequeued.", 1);
		}
	    } else {
		if ((count = atoi(arg1))) {
		    if (!(control_process(player, count))) {
			notify_nolisten(player, "Permission denied.", 1);
			return;
		    }
		    if (!(dequeue_process(count))) {
			notify_nolisten(player, "No such process!", 1);
			return;
		    }
		    process_count--;
		    notify_nolisten(player, "Process dequeued.", 1);
		} else {
		    notify_nolisten(player, "What process do you want to dequeue?", 1);
		}
	    }
	}
    }
    return;
}

int
scan_instances(dbref program)
{
    timequeue tq = tqhead;
    int i = 0;

    while (tq) {
	if (tq->typ == TQ_MUF_TYP && tq->fr) {
	    if (tq->called_prog == program) {
		i++;
	    }
	    for (int loop = 1; loop < tq->fr->caller.top; loop++) {
		if (tq->fr->caller.st[loop] == program)
		    i++;
	    }
	    for (int loop = 0; loop < tq->fr->argument.top; loop++) {
		if (tq->fr->argument.st[loop].type == PROG_ADD &&
		    tq->fr->argument.st[loop].data.addr->progref == program)
		    i++;
	    }
	}
	tq = tq->next;
    }
    return i;
}

static int propq_level = 0;
void
propqueue(int descr, dbref player, dbref where, dbref trigger, dbref what, dbref xclude,
	  const char *propname, const char *toparg, int mlev, int mt)
{
    const char *tmpchar;
    const char *pname;
    dbref the_prog;
    char buf[BUFFER_LEN];
    char exbuf[BUFFER_LEN];

    tmpchar = NULL;

    /* queue up program referred to by the given property */
    if (((the_prog = get_property_dbref(what, propname)) != NOTHING) ||
	(tmpchar = get_property_class(what, propname))) {

	if ((tmpchar && *tmpchar) || the_prog != NOTHING) {
	    if (tmpchar) {
		if (*tmpchar == '&') {
		    the_prog = AMBIGUOUS;
		} else if (*tmpchar == NUMBER_TOKEN && number(tmpchar + 1)) {
		    the_prog = (dbref) atoi(++tmpchar);
		} else if (*tmpchar == REGISTERED_TOKEN) {
		    the_prog = find_registered_obj(what, tmpchar);
		} else if (number(tmpchar)) {
		    the_prog = (dbref) atoi(tmpchar);
		} else {
		    the_prog = NOTHING;
		}
	    } else {
		if (the_prog == AMBIGUOUS)
		    the_prog = NOTHING;
	    }
	    if (the_prog != AMBIGUOUS) {
		if (!ObjExists(the_prog)) {
		    the_prog = NOTHING;
		} else if (Typeof(the_prog) != TYPE_PROGRAM) {
		    the_prog = NOTHING;
		} else if ((OWNER(the_prog) != OWNER(player)) && !(FLAGS(the_prog) & LINK_OK)) {
		    the_prog = NOTHING;
		} else if (MLevel(the_prog) < mlev) {
		    the_prog = NOTHING;
		} else if (MLevel(OWNER(the_prog)) < mlev) {
		    the_prog = NOTHING;
		} else if (the_prog == xclude) {
		    the_prog = NOTHING;
		}
	    }
	    if (propq_level < 8) {
		propq_level++;
		if (the_prog == AMBIGUOUS) {
		    char cbuf[BUFFER_LEN];
		    int ival;

		    strcpyn(match_args, sizeof(match_args), "");
		    strcpyn(match_cmdname, sizeof(match_cmdname), toparg);
		    ival = (mt == 0) ? MPI_ISPUBLIC : MPI_ISPRIVATE;
		    if (Prop_Blessed(what, propname))
			ival |= MPI_ISBLESSED;
		    do_parse_mesg(descr, player, what, tmpchar + 1, "(MPIqueue)", cbuf,
				  sizeof(cbuf), ival);
		    if (*cbuf) {
			if (mt) {
			    notify_filtered(player, player, cbuf, 1);
			} else {
			    char bbuf[BUFFER_LEN];
			    dbref plyr;

			    snprintf(bbuf, sizeof(bbuf), ">> %.4000s",
				     pronoun_substitute(descr, player, cbuf));
			    plyr = CONTENTS(where);
			    while (plyr != NOTHING) {
				if (Typeof(plyr) == TYPE_PLAYER && plyr != player)
				    notify_filtered(player, plyr, bbuf, 0);
				plyr = NEXTOBJ(plyr);
			    }
			}
		    }
		} else if (the_prog != NOTHING) {
		    struct frame *tmpfr;

		    strcpyn(match_args, sizeof(match_args), DoNull(toparg));
		    strcpyn(match_cmdname, sizeof(match_cmdname), "Queued event.");
		    tmpfr = interp(descr, player, where, the_prog, trigger,
				   BACKGROUND, STD_HARDUID, 0);
		    if (tmpfr) {
			interp_loop(player, the_prog, tmpfr, 0);
		    }
		}
		propq_level--;
	    } else {
		notify_nolisten(player, "Propqueue stopped to prevent infinite loop.", 1);
	    }
	}
    }
    strcpyn(buf, sizeof(buf), propname);
    if (is_propdir(what, buf)) {
	strcatn(buf, sizeof(buf), "/");
	while ((pname = next_prop_name(what, exbuf, sizeof(exbuf), buf))) {
	    strcpyn(buf, sizeof(buf), pname);
	    propqueue(descr, player, where, trigger, what, xclude, buf, toparg, mlev, mt);
	}
    }
}

void
envpropqueue(int descr, dbref player, dbref where, dbref trigger, dbref what, dbref xclude,
	     const char *propname, const char *toparg, int mlev, int mt)
{
    while (what != NOTHING) {
	propqueue(descr, player, where, trigger, what, xclude, propname, toparg, mlev, mt);
	what = getparent(what);
    }
}

void
listenqueue(int descr, dbref player, dbref where, dbref trigger, dbref what, dbref xclude,
	    const char *propname, const char *toparg, int mlev, int mt, int mpi_p)
{
    const char *tmpchar;
    const char *pname, *sep, *ptr;
    dbref the_prog = NOTHING;
    char buf[BUFFER_LEN];
    char exbuf[BUFFER_LEN];
    char *ptr2;

    if (!(FLAGS(what) & LISTENER) && !(FLAGS(OWNER(what)) & ZOMBIE))
	return;

    tmpchar = NULL;

    /* queue up program referred to by the given property */
    if (((the_prog = get_property_dbref(what, propname)) != NOTHING) ||
	(tmpchar = get_property_class(what, propname))) {

	if (tmpchar) {
	    sep = tmpchar;
	    while (*sep) {
		if (*sep == '\\') {
		    sep++;
		} else if (*sep == '=') {
		    break;
		}
		if (*sep)
		    sep++;
	    }
	    if (*sep == '=') {
		for (ptr = tmpchar, ptr2 = buf; ptr < sep; *ptr2++ = *ptr++) ;
		*ptr2 = '\0';
		strcpyn(exbuf, sizeof(exbuf), toparg);
		if (!equalstr(buf, exbuf)) {
		    tmpchar = NULL;
		} else {
		    tmpchar = ++sep;
		}
	    }
	}

	if ((tmpchar && *tmpchar) || the_prog != NOTHING) {
	    if (tmpchar) {
		if (*tmpchar == '&') {
		    the_prog = AMBIGUOUS;
		} else if (*tmpchar == NUMBER_TOKEN && number(tmpchar + 1)) {
		    the_prog = (dbref) atoi(++tmpchar);
		} else if (*tmpchar == REGISTERED_TOKEN) {
		    the_prog = find_registered_obj(what, tmpchar);
		} else if (number(tmpchar)) {
		    the_prog = (dbref) atoi(tmpchar);
		} else {
		    the_prog = NOTHING;
		}
	    } else {
		if (the_prog == AMBIGUOUS)
		    the_prog = NOTHING;
	    }
	    if (the_prog != AMBIGUOUS) {
		if (!ObjExists(the_prog)) {
		    the_prog = NOTHING;
		} else if (Typeof(the_prog) != TYPE_PROGRAM) {
		    the_prog = NOTHING;
		} else if (OWNER(the_prog) != OWNER(player) && !(FLAGS(the_prog) & LINK_OK)) {
		    the_prog = NOTHING;
		} else if (MLevel(the_prog) < mlev) {
		    the_prog = NOTHING;
		} else if (MLevel(OWNER(the_prog)) < mlev) {
		    the_prog = NOTHING;
		} else if (the_prog == xclude) {
		    the_prog = NOTHING;
		}
	    }
	    if (the_prog == AMBIGUOUS) {
		if (mpi_p) {
		    add_mpi_event(1, descr, player, where, trigger, tmpchar + 1,
				  (mt ? "Listen" : "Olisten"), toparg, 1, (mt == 0),
				  Prop_Blessed(what, propname));
		}
	    } else if (the_prog != NOTHING) {
		add_muf_queue_event(descr, player, where, trigger, the_prog, toparg,
				    "(_Listen)", 1);
	    }
	}
    }
    strcpyn(buf, sizeof(buf), propname);
    if (is_propdir(what, buf)) {
	strcatn(buf, sizeof(buf), "/");
	while ((pname = next_prop_name(what, exbuf, sizeof(exbuf), buf))) {
	    *buf = '\0';
	    strcatn(buf, sizeof(buf), pname);
	    listenqueue(descr, player, where, trigger, what, xclude, buf,
			toparg, mlev, mt, mpi_p);
	}
    }
}
