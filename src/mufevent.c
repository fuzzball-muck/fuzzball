#include "config.h"

#include "array.h"
#include "db.h"
#include "externs.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "match.h"
#include "mufevent.h"
#include "tune.h"

static struct mufevent_process {
    struct mufevent_process *prev, *next;
    dbref player;
    dbref prog;
    short filtercount;
    short deleted;
    char **filters;
    struct frame *fr;
} *mufevent_processes;

/* static void muf_event_process_free(struct mufevent* ptr)
 * Frees up a mufevent_process once you are done with it.
 * This shouldn't be used outside this module.
 */
static void
muf_event_process_free(struct mufevent_process *ptr)
{
    if (ptr->next) {
	ptr->next->prev = ptr->prev;
    }
    if (ptr->prev) {
	ptr->prev->next = ptr->next;
    } else {
	mufevent_processes = ptr->next;
    }

    ptr->prev = NULL;
    ptr->next = NULL;
    ptr->player = NOTHING;
    if (ptr->fr) {
	if (PROGRAM_INSTANCES(ptr->prog)) {
	    prog_clean(ptr->fr);
	}
	ptr->fr = NULL;
    }
    ptr->prog = NOTHING;
    if (ptr->filters) {
	for (int i = 0; i < ptr->filtercount; i++) {
	    if (ptr->filters[i]) {
		free(ptr->filters[i]);
		ptr->filters[i] = NULL;
	    }
	}
	free(ptr->filters);
	ptr->filters = NULL;
	ptr->filtercount = 0;
    }
    ptr->deleted = 1;
    free(ptr);
}


/* void muf_event_register_specific(dbref player, dbref prog, struct frame* fr, int eventcount, char** eventids)
 * Called when a MUF program enters EVENT_WAITFOR, to register that
 * the program is ready to process MUF events of the given ID type.
 * This duplicates the eventids list for itself, so the caller is
 * responsible for freeing the original eventids list passed.
 */
void
muf_event_register_specific(dbref player, dbref prog, struct frame *fr, int eventcount,
			    char **eventids)
{
    struct mufevent_process *newproc;
    struct mufevent_process *ptr;

    newproc = (struct mufevent_process *) malloc(sizeof(struct mufevent_process));

    newproc->prev = NULL;
    newproc->next = NULL;
    newproc->player = player;
    newproc->prog = prog;
    newproc->fr = fr;
    newproc->deleted = 0;
    newproc->filtercount = eventcount;
    if (eventcount > 0) {
	newproc->filters = (char **) malloc(eventcount * sizeof(char **));
	for (int i = 0; i < eventcount; i++) {
	    newproc->filters[i] = string_dup(eventids[i]);
	}
    } else {
	newproc->filters = NULL;
    }

    ptr = mufevent_processes;
    while ((ptr != NULL) && ptr->next) {
	ptr = ptr->next;
    }
    if (!ptr) {
	mufevent_processes = newproc;
    } else {
	ptr->next = newproc;
	newproc->prev = ptr;
    }
}


/* void muf_event_register(dbref player, dbref prog, struct frame* fr)
 * Called when a MUF program enters EVENT_WAIT, to register that
 * the program is ready to process any type of MUF events.
 */
void
muf_event_register(dbref player, dbref prog, struct frame *fr)
{
    muf_event_register_specific(player, prog, fr, 0, NULL);
}


/* int muf_event_read_notify(int descr, dbref player)
 * Sends a "READ" event to the first foreground or preempt muf process
 * that is owned by the given player.  Returns 1 if an event was sent,
 * 0 otherwise.
 */
int
muf_event_read_notify(int descr, dbref player, const char *cmd)
{
    struct mufevent_process *ptr;

    ptr = mufevent_processes;
    while (ptr) {
	if (!ptr->deleted) {
	    if (ptr->player == player) {
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
	}
	ptr = ptr->next;
    }
    return 0;
}


/* int muf_event_dequeue_pid(int pid)
 * removes the MUF program with the given PID from the EVENT_WAIT queue.
 */
int
muf_event_dequeue_pid(int pid)
{
    struct mufevent_process *proc;
    int count = 0;

    proc = mufevent_processes;
    while (proc) {
	if (!proc->deleted) {
	    if (proc->fr->pid == pid) {
		if (!proc->fr->been_background)
		    PLAYER_SET_BLOCK(proc->player, 0);
		muf_event_purge(proc->fr);
		proc->deleted = 1;
		count++;
	    }
	}
	proc = proc->next;
    }
    return count;
}


/* static int event_has_refs(dbref program, struct mufevent_process *proc)
 * Checks the MUF event queue for address references on the stack or
 * dbref references on the callstack
 */
static int
event_has_refs(dbref program, struct mufevent_process *proc)
{
    struct frame *fr = NULL;

    if (proc->deleted) {
	return 0;
    }
    fr = proc->fr;
    if (!fr) {
	return 0;
    }


    for (int loop = 1; loop < fr->caller.top; loop++) {
	if (fr->caller.st[loop] == program) {
	    return 1;
	}
    }


    for (int loop = 0; loop < fr->argument.top; loop++) {
	if (fr->argument.st[loop].type == PROG_ADD &&
	    fr->argument.st[loop].data.addr->progref == program) {
	    return 1;
	}
    }

    return 0;
}


/* int muf_event_dequeue(dbref prog, int killmode)
 * Deregisters a program from any instances of it in the EVENT_WAIT queue.
 * killmode values:
 *     0: kill all matching processes (Equivalent to 1)
 *     1: kill all matching MUF processes
 *     2: kill all matching foreground MUF processes
 */
int
muf_event_dequeue(dbref prog, int killmode)
{
    struct mufevent_process *proc;
    int count = 0;

    if (killmode == 0)
	killmode = 1;

    for (proc = mufevent_processes; proc; proc = proc->next) {
	if (proc->deleted) {
	    continue;
	}
	if (proc->prog != prog && !event_has_refs(prog, proc) && proc->player != prog) {
	    continue;
	}
	if (killmode == 2) {
	    if (proc->fr && proc->fr->multitask == BACKGROUND) {
		continue;
	    }
	} else if (killmode == 1) {
	    if (!proc->fr) {
		continue;
	    }
	}
	if (proc->fr) {
	    if (!proc->fr->been_background)
		PLAYER_SET_BLOCK(proc->player, 0);
	    muf_event_purge(proc->fr);
	    prog_clean(proc->fr);
	}
	proc->deleted = 1;
	count++;
    }
    return count;
}


struct frame *
muf_event_pid_frame(int pid)
{
    struct mufevent_process *ptr = mufevent_processes;

    while (ptr) {
	if (!ptr->deleted && ptr->fr && ptr->fr->pid == pid)
	    return ptr->fr;
	ptr = ptr->next;
    }
    return NULL;
}


/* int muf_event_controls(dbref player, int pid)
 * Returns true if the given player controls the given PID.
 */
int
muf_event_controls(dbref player, int pid)
{
    struct mufevent_process *proc = mufevent_processes;

    while (proc && (proc->deleted || pid != proc->fr->pid)) {
	proc = proc->next;
    }

    if (!proc) {
	return 0;
    }
    if (!controls(player, proc->prog) && player != proc->player) {
	return 0;
    }

    return 1;
}


/* int muf_event_list(dbref player, char* pat)
 * List all processes in the EVENT_WAIT queue that the given player controls.
 * This is used by the @ps command.
 */
int
muf_event_list(dbref player, const char *pat)
{
    char buf[BUFFER_LEN];
    char pidstr[BUFFER_LEN];
    char inststr[BUFFER_LEN];
    char cpustr[BUFFER_LEN];
    char progstr[BUFFER_LEN];
    char prognamestr[BUFFER_LEN];
    int count = 0;
    time_t rtime = time((time_t *) NULL);
    time_t etime;
    double pcnt = 0;
    struct mufevent_process *proc = mufevent_processes;

    while (proc) {
	if (!proc->deleted) {
	    if (proc->fr) {
		etime = rtime - proc->fr->started;
		if (etime > 0) {
		    pcnt = proc->fr->totaltime.tv_sec;
		    pcnt += proc->fr->totaltime.tv_usec / 1000000;
		    pcnt = pcnt * 100 / etime;
		    if (pcnt > 99.9) {
			pcnt = 99.9;
		    }
		} else {
		    pcnt = 0.0;
		}
	    }
	    snprintf(pidstr, sizeof(pidstr), "%d", proc->fr->pid);
	    snprintf(inststr, sizeof(inststr), "%d", (proc->fr->instcnt / 1000));
	    snprintf(cpustr, sizeof(cpustr), "%4.1f", pcnt);
	    if (proc->fr) {
		snprintf(progstr, sizeof(progstr), "#%d", proc->fr->caller.st[1]);
		snprintf(prognamestr, sizeof(prognamestr), "%s", NAME(proc->fr->caller.st[1]));
	    } else {
		snprintf(progstr, sizeof(progstr), "#%d", proc->prog);
		snprintf(prognamestr, sizeof(prognamestr), "%s", NAME(proc->prog));
	    }
	    snprintf(buf, sizeof(buf), pat, pidstr, "--",
		     time_format_2((time_t) (rtime - proc->fr->started)),
		     inststr, cpustr, progstr, prognamestr, NAME(proc->player),
		     "EVENT_WAITFOR");
	    if (Wizard(OWNER(player)) || (OWNER(proc->prog) == OWNER(player))
		|| (proc->player == player))
		notify_nolisten(player, buf, 1);
	    count++;
	}
	proc = proc->next;
    }
    return count;
}


/* stk_array *get_mufevent_pids(stk_array *nw, dbref ref)
 * Given a muf list array, appends pids to it where ref
 * matches the trigger, program, or player.  If ref is #-1
 * then all processes waiting for mufevents are added.
 */
stk_array *
get_mufevent_pids(stk_array * nw, dbref ref)
{
    struct inst temp1, temp2;
    int count = 0;

    struct mufevent_process *proc = mufevent_processes;
    while (proc) {
	if (!proc->deleted) {
	    if (proc->player == ref || proc->prog == ref || proc->fr->trig == ref || ref < 0) {
		temp1.type = PROG_INTEGER;
		temp1.data.number = count++;
		temp2.type = PROG_INTEGER;
		temp2.data.number = proc->fr->pid;
		array_setitem(&nw, &temp1, &temp2);
		CLEAR(&temp1);
		CLEAR(&temp2);
	    }
	}
	proc = proc->next;
    }
    return nw;
}


stk_array *
get_mufevent_pidinfo(stk_array * nw, int pid)
{
    struct inst temp1, temp2;
    stk_array *arr;
    time_t rtime = time(NULL);
    time_t etime = 0;
    double pcnt = 0.0;

    struct mufevent_process *proc = mufevent_processes;
    while (proc && (proc->deleted || proc->fr->pid != pid)) {
	proc = proc->next;
    }
    if (proc && (proc->fr->pid == pid)) {
	if (proc->fr) {
	    etime = rtime - proc->fr->started;
	    if (etime > 0) {
		pcnt = proc->fr->totaltime.tv_sec;
		pcnt += proc->fr->totaltime.tv_usec / 1000000;
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
	temp2.data.number = proc->fr->pid;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("CALLED_PROG");
	temp2.type = PROG_OBJECT;
	temp2.data.objref = proc->prog;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("TRIG");
	temp2.type = PROG_OBJECT;
	temp2.data.objref = proc->fr->trig;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("PLAYER");
	temp2.type = PROG_OBJECT;
	temp2.data.objref = proc->player;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("CALLED_DATA");
	temp2.type = PROG_STRING;
	temp2.data.string = alloc_prog_string("EVENT_WAITFOR");
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("INSTCNT");
	temp2.type = PROG_INTEGER;
	temp2.data.number = proc->fr->instcnt;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("DESCR");
	temp2.type = PROG_INTEGER;
	temp2.data.number = proc->fr->descr;
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
	temp2.data.number = -1;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("STARTED");
	temp2.type = PROG_INTEGER;
	temp2.data.number = (int) proc->fr->started;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("TYPE");
	temp2.type = PROG_STRING;
	temp2.data.string = alloc_prog_string("MUFEVENT");
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("SUBTYPE");
	temp2.type = PROG_STRING;
	temp2.data.string = alloc_prog_string("");
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string("FILTERS");
	arr = new_array_packed(0);
	for (int i = 0; i < proc->filtercount; i++) {
	    array_set_intkey_strval(&arr, i, proc->filters[i]);
	}
	temp2.type = PROG_ARRAY;
	temp2.data.array = arr;
	array_setitem(&nw, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);
    }
    return nw;
}


/* int muf_event_count(struct frame* fr)
 * Returns how many events are waiting to be processed.
 */
int
muf_event_count(struct frame *fr)
{
    struct mufevent *ptr;
    int count = 0;

    for (ptr = fr->events; ptr; ptr = ptr->next)
	count++;

    return count;
}


/* int muf_event_exists(struct frame* fr, const char* eventid)
 * Returns how many events of the given event type are waiting to be processed.
 * The eventid passed can be an smatch string.
 */
int
muf_event_exists(struct frame *fr, const char *eventid)
{
    struct mufevent *ptr;
    int count = 0;
    char pattern[BUFFER_LEN];

    strcpyn(pattern, sizeof(pattern), eventid);

    for (ptr = fr->events; ptr; ptr = ptr->next)
	if (equalstr(pattern, ptr->event))
	    count++;

    return count;
}


/* static void muf_event_free(struct mufevent* ptr)
 * Frees up a MUF event once you are done with it.  This shouldn't be used
 * outside this module.
 */
static void
muf_event_free(struct mufevent *ptr)
{
    CLEAR(&ptr->data);
    free(ptr->event);
    ptr->event = NULL;
    ptr->next = NULL;
    free(ptr);
}


/* void muf_event_add(struct frame* fr, char* event, struct inst* val, int exclusive)
 * Adds a MUF event to the event queue for the given program instance.
 * If the exclusive flag is true, and if an item of the same event type
 * already exists in the queue, the new one will NOT be added.
 */
void
muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive)
{
    struct mufevent *newevent;
    struct mufevent *ptr;

    ptr = fr->events;
    while (ptr && ptr->next) {
	if (exclusive && !strcmp(event, ptr->event)) {
	    return;
	}
	ptr = ptr->next;
    }

    if (exclusive && ptr && !strcmp(event, ptr->event)) {
	return;
    }

    newevent = (struct mufevent *) malloc(sizeof(struct mufevent));
    newevent->event = string_dup(event);
    copyinst(val, &newevent->data);
    newevent->next = NULL;

    if (!ptr) {
	fr->events = newevent;
    } else {
	ptr->next = newevent;
    }
}



/* struct mufevent* muf_event_pop_specific(struct frame* fr, int eventcount, const char** events)
 * Removes the first event of one of the specified types from the event queue
 * of the given program instance.
 * Returns a pointer to the removed event to the caller.
 * Returns NULL if no matching events are found.
 * You will need to call muf_event_free() on the returned data when you
 * are done with it and wish to free it from memory.
 */
struct mufevent *
muf_event_pop_specific(struct frame *fr, int eventcount, char **events)
{
    struct mufevent *tmp = NULL;
    struct mufevent *ptr = NULL;

    for (int i = 0; i < eventcount; i++) {
	if (fr->events && equalstr(events[i], fr->events->event)) {
	    tmp = fr->events;
	    fr->events = tmp->next;
	    return tmp;
	}
    }

    ptr = fr->events;
    while (ptr && ptr->next) {
	for (int i = 0; i < eventcount; i++) {
	    if (equalstr(events[i], ptr->next->event)) {
		tmp = ptr->next;
		ptr->next = tmp->next;
		return tmp;
	    }
	}
	ptr = ptr->next;
    }

    return NULL;
}



/* void muf_event_remove(struct frame* fr, char* event, int doall)
 * Removes a given MUF event type from the event queue of the given
 * program instance.  If which is MUFEVENT_ALL, all instances are removed.
 * If which is MUFEVENT_FIRST, only the first instance is removed.
 * If which is MUFEVENT_LAST, only the last instance is removed.
 */
void
muf_event_remove(struct frame *fr, char *event, int which)
{
    struct mufevent *tmp = NULL;
    struct mufevent *ptr = NULL;

    while (fr->events && !strcmp(event, fr->events->event)) {
	if (which == MUFEVENT_LAST) {
	    break;
	} else {
	    tmp = fr->events;
	    fr->events = tmp->next;
	    muf_event_free(tmp);
	    if (which == MUFEVENT_FIRST) {
		return;
	    }
	}
    }

    ptr = fr->events;
    while (ptr && ptr->next) {
	if (!strcmp(event, ptr->next->event)) {
	    if (which == MUFEVENT_LAST) {
		ptr = ptr->next;
	    } else {
		tmp = ptr->next;
		ptr->next = tmp->next;
		muf_event_free(tmp);
		if (which == MUFEVENT_FIRST) {
		    return;
		}
	    }
	} else {
	    ptr = ptr->next;
	}
    }
}



/* static struct mufevent* muf_event_peek(struct frame* fr)
 * This returns a pointer to the top muf event of the given
 * program instance's event queue.  The event is not removed
 * from the queue.
 */
/*
 * Commented out by Winged for non-reference.  If you need it, de-comment.
 *
static struct mufevent *
muf_event_peek(struct frame *fr)
{
	return fr->events;
}
 */


/* static struct mufevent* muf_event_pop(struct frame* fr)
 * This pops the top muf event off of the given program instance's
 * event queue, and returns it to the caller.  The caller should
 * call muf_event_free() on the data when it is done with it.
 */
static struct mufevent *
muf_event_pop(struct frame *fr)
{
    struct mufevent *ptr = NULL;

    if (fr->events) {
	ptr = fr->events;
	fr->events = fr->events->next;
    }
    return ptr;
}



/* void muf_event_purge(struct frame* fr)
 * purges all muf events from the given program instance's event queue.
 */
void
muf_event_purge(struct frame *fr)
{
    while (fr->events) {
	muf_event_free(muf_event_pop(fr));
    }
}



/* void muf_event_process()
 * For all program instances who are in the EVENT_WAIT queue,
 * check to see if they have any items in their event queue.
 * If so, then process one each.  Up to ten programs can have
 * events processed at a time.
 *
 * This also needs to make sure that background processes aren't
 * waiting for READ events.
 */
void
muf_event_process(void)
{
    int limit = 10;
    struct mufevent_process *proc, *next;
    struct mufevent *ev;
    dbref current_program;
    int block, is_fg;

    proc = mufevent_processes;
    while (proc != NULL) {
	next = proc->next;
	if (proc->deleted) {
	    muf_event_process_free(proc);
	}
	proc = next;
    }

    proc = mufevent_processes;
    while (proc != NULL && limit > 0) {
	if (!proc->deleted && proc->fr) {
	    if (proc->filtercount > 0) {
		/* Make sure it's not waiting for a READ event, if it's
		 * backgrounded */

		if (proc->fr->been_background) {
		    for (int cnt = 0; cnt < proc->filtercount; cnt++) {
			if (0 == strcasecmp(proc->filters[cnt], "READ")) {
			    /* It's a backgrounded process, waiting for a READ...
			     * should we throw an error?  Should we push a
			     * null event onto the list?  At this point, I'm
			     * pushing a READ event with descr = -1, so that
			     * it will at least get out of its loop. -winged
			     */
			    struct inst temp;
			    temp.type = PROG_INTEGER;
			    temp.data.number = -1;
			    muf_event_add(proc->fr, "READ", &temp, 0);
			    CLEAR(&temp);
			    break;
			}
		    }
		}


		/* Search prog's event list for the apropriate event type. */

		/* HACK:  This is probably inefficient to be walking this
		 * queue over and over. Hopefully it's usually a short list.
		 *
		 * Would it be more efficient to use a hash table?  It'd
		 * be more wasteful of memory, I think. -winged
		 */
		ev = muf_event_pop_specific(proc->fr, proc->filtercount, proc->filters);
	    } else {
		/* Pop first event off of prog's event queue. */
		ev = muf_event_pop(proc->fr);
	    }
	    if (ev) {
		--limit;
		if (proc->fr->argument.top + 1 >= STACK_SIZE) {
		    /* Uh oh! That MUF program's stack is full!
		     * Print an error, free the frame, and exit.
		     */
		    notify_nolisten(proc->player, "Program stack overflow.", 1);
		    prog_clean(proc->fr);
		} else {
		    current_program = PLAYER_CURR_PROG(proc->player);
		    block = PLAYER_BLOCK(proc->player);
		    is_fg = (proc->fr->multitask != BACKGROUND);

		    copyinst(&ev->data, &(proc->fr->argument.st[proc->fr->argument.top]));
		    proc->fr->argument.top++;
		    push(proc->fr->argument.st, &(proc->fr->argument.top),
			 PROG_STRING, MIPSCAST alloc_prog_string(ev->event));

		    interp_loop(proc->player, proc->prog, proc->fr, 0);

		    if (!is_fg) {
			PLAYER_SET_BLOCK(proc->player, block);
			PLAYER_SET_CURR_PROG(proc->player, current_program);
		    }
		}
		muf_event_free(ev);

		proc->fr = NULL;	/* We do NOT want to free this program after every EVENT_WAIT. */
		proc->deleted = 1;
	    }
	}
	proc = proc->next;
    }
    proc = mufevent_processes;
    while (proc != NULL) {
	next = proc->next;
	if (proc->deleted) {
	    muf_event_process_free(proc);
	}
	proc = next;
    }
}
