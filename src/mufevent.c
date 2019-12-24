/** @file mufevent.c
 *
 * Source file for MUF Events
 *
 * MUF Events are a form of messaging passing between MUF programs.  It is
 * also used by MCP
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "mufevent.h"

/*
 * A MUF event is a an element in a queue, and has some kind of name and
 * associated data which is some MUF instruction (usually a string, but it
 * can be any stack item type)
 */
struct mufevent {
    struct mufevent *next;  /* Next event in the queue linkage */
    char *event;            /* Name of this event */
    struct inst data;       /* Associated stack data */
};

/*
 * This is the queue of processes that are waiting for an event.
 *
 * Note that 'deleted' is used as a tombstone; when set true, the entry will
 * be cleaned up next time it shows up in the queue processing.
 */
static struct mufevent_process {
    struct mufevent_process *prev, *next;   /* Double-linked list      */
    dbref player;                           /* Player running program  */
    dbref prog;                             /* Program being run       */
    short filtercount;                      /* Number of event filters */
    short deleted;                          /* If true, delete this entry */
    char **filters;                         /* Event filter array of strings */
    struct frame *fr;                       /* The running program frame */
} *mufevent_processes;

/**
 * Frees up a mufevent_process once you are done with it.
 *
 * This will remove it from the queue, and runs prog_clean on the associated
 * program (@see prog_clean ) if PROGRAM_INSTANCES returns true.  All
 * filter strings are free'd
 *
 * You must set ->fr to NULL if you don't want your program free'd by
 * this call, which is kind of nasty gotchya.  This is an internal 'private'
 * call so I'm not suggeesting we improve it, but it seems a curious default
 * behavior.
 *
 * @private
 * @param ptr the mufevent_process to free
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

    /*
     * @TODO: This is just a waste of CPU to NULL this stuff out.  If we were
     *        re-using the structure it would make sense, but we're setting
     *        all this then freeing it.  Even in a theoretical threaded
     *        situation this doesn't make sense, because we already dequeue'd
     *        it.
     */
    ptr->prev = NULL;
    ptr->next = NULL;
    ptr->player = NOTHING;

    if (ptr->fr) {
        if (PROGRAM_INSTANCES(ptr->prog)) {
            prog_clean(ptr->fr);
        }

        /* @TODO: I know it's nitpicky, but this continues to be silly */
        ptr->fr = NULL;
    }

    /* @TODO: ....and this */
    ptr->prog = NOTHING;

    if (ptr->filters) {
        for (int i = 0; i < ptr->filtercount; i++) {
            free(ptr->filters[i]);
            ptr->filters[i] = NULL;
        }

        free(ptr->filters);

        /* @TODO: ....and this */
        ptr->filters = NULL;
        ptr->filtercount = 0;
    }

    /* @TODO: ....and this */
    ptr->deleted = 1;

    /*
     * See?  We just blew away all those assignments we set and rattled up
     * the CPU's cache for nothing.
     *
     * ... of course this is such a rarely used feature anyway, we probably
     * aren't saving all that much.
     */
    free(ptr);
}

/**
 * Add a MUF to the event wait queue, waiting for specific events
 *
 * Called when a MUF program enters EVENT_WAITFOR, to register that the
 * program is ready to process MUF events of the given ID type.  This
 * duplicates the eventids list for itself, so the caller is responsible for
 * freeing the original eventids list passed.
 *
 * @param player the player instigating the wait (i.e. running the program)
 * @param prog the program running
 * @param fr the frame that started the event_wait call
 * @param eventcount the number of entries in 'eventids'
 * @param eventids the array of strings that are the events we are filtering for
 */
void
muf_event_register_specific(dbref player, dbref prog, struct frame *fr,
                            size_t eventcount, char **eventids)
{
    struct mufevent_process *newproc;
    struct mufevent_process *ptr;

    /* Create a new queue structure */
    newproc = malloc(sizeof(struct mufevent_process));

    /* Initialize it */

    /*
     * @TODO: recommend we do a
     *        memset(newproc, 0, sizeof(struct mufevent_process))
     *        rather than nulling out individual fields.  This is just smarter
     *        in case we add new fields in the future.
     */
    newproc->prev = NULL;
    newproc->next = NULL;
    newproc->player = player;
    newproc->prog = prog;
    newproc->fr = fr;
    newproc->deleted = 0;
    newproc->filtercount = (short)eventcount;

    /* Copy over event array */
    if (eventcount > 0) {
        newproc->filters = malloc(eventcount * sizeof(char *));

        for (unsigned int i = 0; i < eventcount; i++) {
            newproc->filters[i] = strdup(eventids[i]);
        }
    } else {
        newproc->filters = NULL;
    }

    ptr = mufevent_processes;

    /* Navigate the process list so we can add it to the end */
    while ((ptr != NULL) && ptr->next) {
        ptr = ptr->next;
    }

    /*
     * Add it to the head if we didn't find a last node, otherwise,
     * insert at the end of the queue.
     */
    if (!ptr) {
        mufevent_processes = newproc;
    } else {
        ptr->next = newproc;
        newproc->prev = ptr;
    }
}

/**
 * Send a READ event to the first non-backgrounded process owned by a player
 *
 * Sends a "READ" event to the first foreground or preempt muf process
 * that is owned by the given player.  Returns 1 if an event was sent,
 * 0 otherwise.
 *
 * NOTE: if 'read wants blanks' mode is turned off, sending an empty string
 * via this command will (probably) result in this call returning 0.  However,
 * if by some reason the player somehow has two programs in the foreground
 * then the blank string can be caught by another process that isn't the
 * first non-backgrounded.
 *
 * @param descr the descriptor of the person inputting the line being READ
 * @param player the player we are processing READs for
 * @param cmd the text typed in by the given player/descriptor combination
 * @return boolean true if event sent, false if not
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

                    /*
                     * @TODO: This is a potential oddity.  If the user has
                     *        two programs running in the foreground (not
                     *        sure if this is possible?  Does this happen
                     *        during a library CALL?) and they're both
                     *        waiting for input (... again, not sure if
                     *        possible) then the READ can be caught by
                     *        a process that isn't the first process in
                     *        FOREGROUND state because we don't return
                     *        here.  This, to me, seems to in the best case
                     *        not be possible so we'd be looping more for
                     *        no reason here, and in the worst case, violates
                     *        the "contract" of the function call by
                     *        enabling the wrong process to catch the READ.
                     *
                     *        So I think this should return 0 here instead of
                     *        continuing to loop, though a second pair of
                     *        eyes to make sure that's not going to cause
                     *        a weird problem would be appreciated!
                     *
                     *        Remember to update the docblock comment here and
                     *        in mufevent.h if you fix this.
                     */
                }
            }
        }

        ptr = ptr->next;
    }

    return 0;
}

/**
 * Removes the MUF program with the given PID from the EVENT_WAIT queue.
 *
 * This is not an immediate removal; it sets the deleted flag on the queue
 * entry, which will be cleaned up later by muf_event_process
 *
 * @see muf_event_process
 *
 * If a player is currently being blocked by waiting, and the process is
 * not backgrounded, then the player will be unblocked.
 *
 * This calls muf_event_purge on the frame.  @see muf_event_purge
 *
 * @param pid the pid to dequeue
 * @return number of processes removed from the queue, which may be 0 or more
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

/**
 * Scans 'proc's frames for references to 'program'
 *
 * Checks the MUF event queue for address references on the stack or
 * dbref references on the callstack.
 *
 * @private
 * @param program the program reference to check for
 * @param proc the mufevent_process entry with the frame we will scan
 * @return true if reference found, false if not found or if proc is
 *         deleted or has no frame.
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

/*
 * Deregisters a program from any instances of it in the EVENT_WAIT queue.
 *
 * killmode values:
 *     0: kill all matching processes (Equivalent to 1)
 *     1: kill all matching MUF processes
 *     2: kill all matching foreground/preempt MUF processes
 *
 * If the player is being blocked by the program on the event_wait queue,
 * they will be unblocked when this is called.  This does not immediately
 * remove the queue entry; the queue entry will be removed by the next
 * call to muf_event_process.  @see muf_event_process
 *
 * This calls muf_event_purge on the frame.  @see muf_event_purge
 *
 * If 'prog' is a player ref instead of a program ref, it will dequeue
 * all processes that belong to the player.
 *
 * @param prog the program to dequeue, or player to dequeue all player programs
 * @param killmode 0, 1, or 2 as described above
 * @return number of processes removed from the queue, which may be 0 or more
 */
int
muf_event_dequeue(dbref prog, int killmode)
{
    int count = 0;

    if (killmode == 0)
        killmode = 1;

    for (struct mufevent_process *proc = mufevent_processes; proc; proc = proc->next) {
        /* Skip tombstones */
        if (proc->deleted) {
            continue;
        }

        /*
         * TODO: I'm not sure the purpose of event_has_refs here.  This
         *       appears to mean that if another program is referring to
         *       to this program's ref it will be dequeued?  Which is maybe
         *       necessary for interdependencies -- for example, if X is
         *       waiting for Y, and you dequeue Y, then you'd have to
         *       dequeue X as well?
         *
         *       I'm not really sure.  Does this handle the case of
         *       X depends on Y depends on Z and delete Z as well?  From
         *       my reading of this, it wouldn't work.
         *
         *       This should be investigated deeper and the documentation
         *       updated to reflect what's going on.  Remember to update
         *       docs in mufevents.h as well.
         */
        if (proc->prog != prog && !event_has_refs(prog, proc)
            && proc->player != prog) {
            continue;
        }

        /*
         * TODO: This logic is rather fallible because it means if you
         *       were to use an undefined killmode by accident, you'd fall
         *       into a weird state especially if proc->fr is NULL
         *
         *       Rather than say if killmode == 2 / else if killmode == 1,
         *       I'd just remove the if killmode == 1 and make it a simple
         *       else.  Right now, there's only 2 modes and mode 1 seems
         *       to be intended as a default.
         *
         *       Alternatively, we could add an else that makes an error
         *       if killmode is neither 1 nor 2 (0 is turned into 1 higher
         *       up in the code so we don't have to check for it).  But
         *       I tend to prefer keeping it simple for such an unlikely
         *       (but possible) edge case.
         */
        if (killmode == 2) {
            /* In killmode 2, we only care about foreground/preempt */
            if (proc->fr && proc->fr->multitask == BACKGROUND) {
                continue;
            }
        } else if (killmode == 1) {
            if (!proc->fr) {
                continue;
            }
        }

        /*
         * @TODO: If the if statement is fixed above, we can remove this
         *        if proc->fr because proc->fr will NEVER be NULL at this
         *        point.  The stuff in the if can just be run inline since
         *        this if always evaluates true.
         */
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

/**
 * Return the frame associated with a given PID on the EVENT_WAIT queue
 *
 * @param pid the PID to look for
 * @return the 'struct frame' associated with the PID or NULL if PID not found
 */
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

/**
 * Returns true if the given player controls the given PID.
 *
 * @param player the player to check for
 * @param pid the pid to see if 'player' controls
 * @return boolean true if 'player' controls 'pid', false otherwise.
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

/**
 * List all processes in the EVENT_WAIT queue that the given player controls.
 *
 * This is used by the @ps command.
 *
 * 'pat' is intended to be something akin to this:
 *
 * **%10s %4s %4s %6s %4s %7s %-10.10s %-12s %.512s
 *
 * That's the format used in the only place this is called.
 *
 * The results are notified straight to the player using notify_nolisten
 * using 'pat' to format the output.  @see notify_nolisten
 *
 * @param player the player who's processes are are finding
 * @param pat a sprintf string pattern, see the notes above
 * @return number of processes found, which could be 0 or larger
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
        /*
         * @TODO Logic simplification: if !proc->detete && proc->fr, then
         *       remove the if (proc->fr) else continue and always run the
         *       'true' contents of the if statement
         *
         *       Actually, the way this is written now, if proc->fr is
         *       NULL, this will result in an infinite loop because
         *       we never advance proc to proc->next before hitting the
         *       'else continue' clause.  Whoops!
         *
         *       I've been seeing all these checks for if proc->fr is not
         *       null all over this .c file and I've been wondering when
         *       would proc->fr EVER be null, and I guess this code answers
         *       the question: never, cause otherwise MUCKs would be hanging
         *       on infinite loops during @ps-es.  Either that, or this is
         *       responsible for unexplained lockups.
         *
         *       As yet another note, I think (maybe?) MPI could show up
         *       as something with no frame, if MPI had event support,
         *       which it currently does not.  Lucky for us here!
         */
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
            } else continue;

            snprintf(pidstr, sizeof(pidstr), "%d", proc->fr->pid);
            snprintf(inststr, sizeof(inststr), "%d", (proc->fr->instcnt / 1000));
            snprintf(cpustr, sizeof(cpustr), "%4.1f", pcnt);

            if (proc->fr) {
                snprintf(progstr, sizeof(progstr), "#%d",
                         proc->fr->caller.st[1]);
                snprintf(prognamestr, sizeof(prognamestr), "%s",
                         NAME(proc->fr->caller.st[1]));
            } else {
                snprintf(progstr, sizeof(progstr), "#%d", proc->prog);
                snprintf(prognamestr, sizeof(prognamestr), "%s",
                         NAME(proc->prog));
            }

            snprintf(buf, sizeof(buf), pat, pidstr, "--",
                     time_format_2((time_t) (rtime - proc->fr->started)),
                     inststr, cpustr, progstr, prognamestr, NAME(proc->player),
                     "EVENT_WAITFOR");

            /*
             * @TODO: Why are we doing all this formatting, and then checking
             *        to see if the player is allowed to see it?
             *
             *        My recommendation: move the 'count++' up to the
             *        top (just below if (!proc->deleted && proc->fr ) ),
             *        then IF this if statement below resolves to false,
             *        continue.  Else, do all the display stuff.  Something
             *        like:
             *
             *        if (!Wizard(OWNER(player)) &&
             *            (OWNER(proc->prog) != OWNER(player)) &&
             *            (proc->player != player))
             *           continue
             *
             *        ...display code goes here ...
             */
            if (Wizard(OWNER(player)) || (OWNER(proc->prog) == OWNER(player))
                || (proc->player == player))
                notify_nolisten(player, buf, 1);

            count++;
        }

        proc = proc->next;
    }

    return count;
}

/**
 * Append PIDs to a MUF list where 'ref' matches the trigger, program, or player
 *
 * Given a muf list array, appends pids to it where ref matches the trigger,
 * program, or player.  If ref is #-1 then all processes waiting for mufevents
 * are added.
 *
 * This does not check the type of 'nw' to see if it is, in fact, a list and
 * not a dictionary ... but this code will (probably?) work right with a
 * dictionary as well so it doesn't matter too much.
 *
 * @param nw the array to append to
 * @param ref the ref to check for, or #-1
 * @return a pointer to nw
 */
stk_array *
get_mufevent_pids(stk_array * nw, dbref ref)
{
    struct inst temp1, temp2;
    int count = 0;

    struct mufevent_process *proc = mufevent_processes;

    while (proc) {
        if (!proc->deleted) {
            if (proc->player == ref || proc->prog == ref
                || proc->fr->trig == ref || ref < 0) {
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


/**
 * Populates a MUF dictionary with information about a given pid
 *
 * This does not check the type of 'nw'; if 'nw' is not a dictionary,
 * this will effectively do nothing and leave the list empty.
 *
 * IF 'pid' is not found, the dictionary will be empty.  If it is found,
 * the dictionary will have the following keys:
 *
 * CALLED_DATA which is always EVENT_WAITFOR (events are always this data)
 * CALLED_PROG which is the ref of the program
 * CPU which is the percentage CPU as a float
 * DESCR which is the descriptor as an integer
 * FILTERS which is an array of strings that are the event filters
 * INSTCNT which is the instruction count
 * MLEVEL which is hard-coded to 0 for some reason
 * PID which is the PID
 * PLAYER which is a ref to the player running the program
 * TRIG which is the trigger ref
 * NEXTRUN which is hard coded to 0 (we never know when we will next run)
 * STARTED which is an integer value UNIX timestamp of the start time
 * SUBTYPE which is hard coded to empty string (events never have a subtype)
 * TYPE which is hard coded to MUFEVENT (this is always a MUF event)
 * 
 * @param nw the dictionary to put informatin into
 * @param pid the PID to get information for
 * @param pinned pinned value for the "FILTERS" array in the dictionary
 * @return a pointer to nw
 */
stk_array *
get_mufevent_pidinfo(stk_array * nw, int pid, int pinned)
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

                /*
                 * TODO: Note, this does not match @ps' logic which rounds
                 *       any pcnt > 99.99 to 99.99
                 *
                 *       It would probably make sense for this and
                 *       muf_event_list (which underpins @ps) to use the
                 *       same fundamental logic, maybe add a static function
                 *       to return this pcnt number and have both functions
                 *       use it.
                 */
                if (pcnt > 100.0) {
                    pcnt = 100.0;
                }
            } else {
                pcnt = 0.0;
            }
        }

        array_set_strkey_strval(&nw, "CALLED_DATA", "EVENT_WAITFOR");
        array_set_strkey_refval(&nw, "CALLED_PROG", proc->prog);
        array_set_strkey_fltval(&nw, "CPU", pcnt);
        array_set_strkey_intval(&nw, "DESCR", proc->fr->descr);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("FILTERS");
        arr = new_array_packed(0, pinned);

        for (int i = 0; i < proc->filtercount; i++) {
            array_set_intkey_strval(&arr, i, proc->filters[i]);
        }

        temp2.type = PROG_ARRAY;
        temp2.data.array = arr;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        array_set_strkey_intval(&nw, "INSTCNT", proc->fr->instcnt);

        /*
         * @TODO MLEVEL is hard coded to 0 both here and in get_pidinfo
         *       which underpins the MUF prim GETPIDINFO
         *
         *       This means this is totally useless information and should
         *       either be implemented or removed from both here, get_pidinfo,
         *       and the man page documentation.
         *
         *       If you fix this, remember to update the docblock here and
         *       in the .h file
         */
        array_set_strkey_intval(&nw, "MLEVEL", 0);
        array_set_strkey_intval(&nw, "PID", proc->fr->pid);
        array_set_strkey_refval(&nw, "PLAYER", proc->player);
        array_set_strkey_refval(&nw, "TRIG", proc->fr->trig);
        array_set_strkey_intval(&nw, "NEXTRUN", 0);
        array_set_strkey_intval(&nw, "STARTED", (int)proc->fr->started);
        array_set_strkey_strval(&nw, "SUBTYPE", "");
        array_set_strkey_strval(&nw, "TYPE", "MUFEVENT");
    }

    return nw;
}

/**
 * Returns how many events are waiting to be processed for a given frame
 *
 * @param fr the frame to get a count for
 * @return the number of events waiting to be processed, or 0 if none
 */
int
muf_event_count(struct frame *fr)
{
    int count = 0;

    for (struct mufevent *ptr = fr->events; ptr; ptr = ptr->next)
        count++;

    return count;
}

/**
 * Returns how many events of the given event type are waiting to be processed.
 *
 * The eventid passed can be an smatch string. @see equalstr
 *
 * @param fr the frame to check for
 * @param an smatch pattern for event names to check for
 * @return count of matching MUF events, which may be 0 for no matches.
 */
int
muf_event_exists(struct frame *fr, const char *eventid)
{
    int count = 0;
    char pattern[BUFFER_LEN];

    strcpyn(pattern, sizeof(pattern), eventid);

    for (struct mufevent *ptr = fr->events; ptr; ptr = ptr->next)
        if (equalstr(pattern, ptr->event))
            count++;

    return count;
}

/**
 * Frees up a MUF event once you are done with it.
 *
 * @private
 * @param ptr the struct mufevent to free
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

/**
 * Adds a MUF event to the event queue for the given program instance.
 *
 * If the exclusive flag is true, and if an item of the same event type
 * already exists in the queue, the new one will NOT be added.
 *
 * 'event' and 'val' are copied so you can free them at will.
 *
 * @param fr the frame to add a MUF event for
 * @param event the name of the event
 * @param val the data associated with the event
 * @param exclusive boolean as described above
 */
void
muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive)
{
    struct mufevent *newevent;
    struct mufevent *ptr;

    ptr = fr->events;

    /* Find the end of the queue, checking the exclusive flag along the way */
    while (ptr && ptr->next) {
        if (exclusive && !strcmp(event, ptr->event)) {
            return;
        }

        ptr = ptr->next;
    }

    /* Make sure last queue item (if exists) doesn't violate exclusive check */
    if (exclusive && ptr && !strcmp(event, ptr->event)) {
        return;
    }

    /* Allocate new event */
    newevent = malloc(sizeof(struct mufevent));
    newevent->event = strdup(event);
    copyinst(val, &newevent->data);
    newevent->next = NULL;

    /* Queue it */
    if (!ptr) {
        fr->events = newevent;
    } else {
        ptr->next = newevent;
    }
}

/**
 * Removes the first event of one of the specified types from the event queue
 *
 * This operates only on the given program instance.  Returns a pointer to the
 * removed event to the caller.
 *
 * Returns NULL if no matching events are found.
 *
 * You will need to call muf_event_free() on the returned data when you
 * are done with it and wish to free it from memory.  @see muf_event_free
 *
 * Events are matched using smatch patterns.  @see equalstr
 *
 * @private
 * @param fr the frame to find events for
 * @param count the number of events in 'events'
 * @param events an array of strings with event names
 * @return either NULL or the found (and removed from list) mufevent
 */
static struct mufevent *
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

/**
 * This pops the top muf event off of the given program instance's event queue
 *
 * The popped item is returned to the caller.  The caller should call
 * muf_event_free() on the data when it is done with it.
 *
 * @see muf_event_free
 *
 * @private
 * @param fr the frame to pop an event for
 * @return NULL if no events on queue, or the item that was removed from queue
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

/**
 * Purges all muf events from the given program instance's event queue.
 *
 * @param fr the frame who's events we are purging
 */
void
muf_event_purge(struct frame *fr)
{
    while (fr->events) {
        muf_event_free(muf_event_pop(fr));
    }
}

/**
 * This is the "main loop" of the event queue and should be run periodically
 *
 * For all program instances who are in the EVENT_WAIT queue, check to see if
 * they have any items in their event queue.  If so, then process one each.
 * Up to ten programs can have events processed at a time.  This limit is
 * hard-coded.
 *
 * This also needs to make sure that background processes aren't waiting for
 * READ events and it cleans up any queue items that have ->deleted == true
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

    /* Clean out all deleted nodes first */
    while (proc != NULL) {
        next = proc->next;

        if (proc->deleted) {
            muf_event_process_free(proc);
        }

        proc = next;
    }

    proc = mufevent_processes;

    while (proc != NULL && limit > 0) {
        /*
         * At first blush, you may think checking for !proc->deleted is silly
         * since we just deleted all the deleted nodes.
         *
         * And you're probably right.
         *
         * But this loop marks things deleted as it goes, so I think this
         * is here as a precaution?  It doesn't really hurt anything to leave
         * this as-is.
         */
        if (!proc->deleted && proc->fr) {
            if (proc->filtercount > 0) {
                /*
                 * Make sure it's not waiting for a READ event, if it's
                 * backgrounded.
                 */
                if (proc->fr->been_background) {
                    for (int cnt = 0; cnt < proc->filtercount; cnt++) {
                        if (0 == strcasecmp(proc->filters[cnt], "READ")) {
                            /*
                             * It's a backgrounded process, waiting for a
                             * READ...
                             *
                             * should we throw an error?  Should we push a
                             * null event onto the list?  At this point, I'm
                             * pushing a READ event with descr = -1, so that
                             * it will at least get out of its loop. -winged
                             *
                             * @TODO
                             * Tanabi's commentary: muf_event_read_notify
                             * won't add a read event to a background process.
                             * We could a similar check to muf_event_add, or
                             * move the check FROM muf_event_read_notify
                             * TO muf_event_add, and then this can't happen.
                             *
                             * If I understand correctly, a MUF cannot go
                             * into the background while it's waiting for
                             * something, so you can't wait for a READ then
                             * somehow go into the background while waiting
                             * for that READ.
                             *
                             * That being said, this check is a decent sanity
                             * precaution because the results would be a
                             * program stuck in the wait queue forever.  If
                             * muf_event_add is modified, this needs to be
                             * changed to throw an error -- see a few lines
                             * below for the stack overflow case for a code
                             * example of how to make this error out.
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

                /*
                 * HACK:  This is probably inefficient to be walking this
                 * queue over and over. Hopefully it's usually a short list.
                 *
                 * Would it be more efficient to use a hash table?  It'd
                 * be more wasteful of memory, I think. -winged
                 *
                 * This usually is a really short list, so I'm inclined to
                 * agree with winged here. -Tanabi
                 */
                ev = muf_event_pop_specific(proc->fr, proc->filtercount,
                                            proc->filters);
            } else {
                /* Pop first event off of prog's event queue. */
                ev = muf_event_pop(proc->fr);
            }

            /* If we have an event to process ...*/
            if (ev) {
                --limit; /* Deduct from our processing limit */

                if (proc->fr->argument.top + 1 >= STACK_SIZE) {
                    /*
                     * Uh oh! That MUF program's stack is full!
                     * Print an error, free the frame, and exit.
                     */
                    notify_nolisten(proc->player, "Program stack overflow.", 1);
                    prog_clean(proc->fr);
                } else {
                    /*
                     * Keep track of the player's current state, so that we
                     * don't interrupt them in case they are in another
                     * program when this event happens.
                     */
                    current_program = PLAYER_CURR_PROG(proc->player);
                    block = PLAYER_BLOCK(proc->player);
                    is_fg = (proc->fr->multitask != BACKGROUND);

                    /* Copy the event result into place */
                    copyinst(&ev->data,
                             &(proc->fr->argument.st[proc->fr->argument.top]));
                    proc->fr->argument.top++;

                    push(proc->fr->argument.st, &(proc->fr->argument.top),
                         PROG_STRING, alloc_prog_string(ev->event));

                    interp_loop(proc->player, proc->prog, proc->fr, 0);

                    /*
                     * If it's a background program, restore player's blocked
                     * state and current program
                     */
                    if (!is_fg) {
                        PLAYER_SET_BLOCK(proc->player, block);
                        PLAYER_SET_CURR_PROG(proc->player, current_program);
                    }
                }

                muf_event_free(ev);

                /*
                 * We do NOT want to free this program after every EVENT_WAIT.
                 */
                proc->fr = NULL; 
                proc->deleted = 1;
            }
        }

        proc = proc->next;
    }

    proc = mufevent_processes;

    /* Clean up any deleted ones we accumulated during processing */
    while (proc != NULL) {
        next = proc->next;

        if (proc->deleted) {
            muf_event_process_free(proc);
        }

        proc = next;
    }
}
