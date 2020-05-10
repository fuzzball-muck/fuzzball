/** @file timequeue.h
 *
 * Declaration for handling timequeues.  Time queues are how programs are run
 * on the MUCK, both MUF and MPI.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef TIMEQUEUE_H
#define TIMEQUEUE_H

#include <time.h>

#include "array.h"
#include "config.h"
#include "interp.h"

/**
 * Add an MPI event to the timequeue.
 *
 * @param delay seconds until MPI should run - can be 0 to run immediately
 * @param descr the player descriptor of the person running the MPI
 * @param player the player dbref of the person creating this queue entry
 * @param loc the location relevant to this program
 * @param trig the trigger object for the MPI
 * @param mpi the MPI to run
 * @param cmdstr the {&cmd} variable value
 * @param argstr the {&arg} variable value
 * @param listen_p boolean true if this is triggered by listen propqueue
 * @param omesg_p boolean true if this is triggered by osucc, ofail, etc.
 * @param blessed_p boolean true if MPI is blessed
 * @return integer event number created or 0 on failure
 */
int add_mpi_event(int delay, int descr, dbref player, dbref loc, dbref trig,
                  const char *mpi, const char *cmdstr, const char *argstr,
                  int listen_p, int omesg_p, int bless_p);

/**
 * Add a MUF read event to the timequeue
 *
 * @param descr the player descriptor of the person running the MUF
 * @param player the player dbref of the person running the MUF
 * @param prog the dbref of the program being run
 * @param fr the frame where thee READ is occuring
 * @return integer event number created or 0 on failure
 */
int add_muf_read_event(int descr, dbref player, dbref prog, struct frame *fr);

/**
 * Add a MUF delay event
 *
 * There is a few uses of this function which make it noteworthy.
 * First off, this is what underpins the SLEEP command.  It is also
 * used by interp to task switch.  And it is used by the FORK prim
 * to launch a copy of a MUF.  In some of these cases a delay of 0 is
 * used.
 *
 * @param delay the delay time in seconds
 * @param descr the player descriptor of the person running the MUF
 * @param player the player dbref of the person running the MUF
 * @param loc the relevant location to the program
 * @param trig the trigger for the program
 * @param prog the dbref of the program being run
 * @param fr the frame where thee READ is occuring
 * @param mode a string containing BACKGROUND, FOREGROUND or SLEEPING
 * @return integer event number created or 0 on failure
 */
int add_muf_delay_event(int delay, int descr, dbref player, dbref loc,
                        dbref trig, dbref prog, struct frame *fr,
                        const char *mode);

/**
 * Add a delayed MUF event to the timequeue
 *
 * @param delay the number of seconds to wait before starting the event
 * @param descr the player descriptor of the person running the MUF
 * @param player the player dbref of the person running the MUF
 * @param loc the location relevant to this program
 * @param trig the trigger object for the MPI
 * @param prog the database ref of the program being run
 * @param argstr the initial stack value
 * @param cmdstr the command @ variable value
 * @param listen_p boolean true if this is triggered by listen propqueue
 * @return integer event number created or 0 on failure
 */
int add_muf_delayq_event(int delay, int descr, dbref player, dbref loc,
                         dbref trig, dbref prog, const char *argstr,
                         const char *cmdstr, int listen_p);

/**
 * Add a MUF timer event to the timequeue
 *
 * These events are created by MUF, and the 'id' is supplied by the user.
 * Only the first 32 characters of the id are used.
 *
 * @param descr the player descriptor of the person running the MUF
 * @param player the player dbref of the person running the MUF
 * @param prog the dbref of the program being run
 * @param fr the frame where thee READ is occuring
 * @param delay the delay time in seconds
 * @param id a string ID code
 * @return integer event number created or 0 on failure
 */
int add_muf_timer_event(int descr, dbref player, dbref prog, struct frame *fr,
                        int delay, char *id);

/**
 * Add a MUF event to the timequeue.
 *
 * @param descr the player descriptor of the person running the MUF
 * @param player the player dbref of the person running the MUF
 * @param loc the location relevant to this program
 * @param trig the trigger object for the MPI
 * @param prog the database ref of the program being run
 * @param argstr the initial stack value
 * @param cmdstr the command @ variable value
 * @param listen_p boolean true if this is triggered by listen propqueue
 * @return integer event number created or 0 on failure
 */
int add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig,
                        dbref prog, const char *argstr, const char *cmdstr,
                        int listen_p);

/**
 * Check to see if a given player controls a given PID
 *
 * Someone who controls a PID can see it on the process list and can kill
 * it if desired.  If the PID is a muf event, then the request is routed
 * to muf_event_controls
 *
 * @see muf_event_controls
 *
 * Otherwise, someone controls the process if:
 *
 * - the player controls the called program or the trigger
 *   @see controls
 *
 * - the player is the person running the program
 *
 * @param player the player to check for control powers
 * @param pid the PID to check if the given player controls
 * @return boolean true if player controls pid, false otherwise
 */
int control_process(dbref player, int procnum);

/**
 * Dequeue a process based on PID -- this is the underpinning of \@kill
 *
 * This does all the necessary cleanup of the process, including events,
 * and then returns boolean if it was killed or not.
 *
 * @param pid the PID to kill
 * @return boolean true if killed, false if not
 */
int dequeue_process(int procnum);

/**
 * The implementation for the dequeue_prog define, killing processes
 *
 * dequeue_prog automatically injects the 'file' and 'line' parameters
 * based on compile constants.
 *
 * This kills a process running on the MUCK.  Kill mode can be one of the
 * following:
 *
 *     0: kill all matching processes, MUF or MPI
 *     1: kill all matching MUF processes
 *     2: kill all matching foreground MUF processes
 *
 * file and line are used for debug purposes and should be the file/line
 * that called the function.  It is recommended you use dequeue_prog instead
 * of this call directly so you don't have to worry about those details.
 *
 * @param program the program dbref to kill
 * @param killmode the kill mode, which can be a value noted above
 * @param file the calling file
 * @param line the calling line
 * @return count of processes killed by this call.
 */
int dequeue_prog_real(dbref, int, const char *, const int);

/**
 * The wrapper for the dequeue_prog_real function, killing processes
 *
 * dequeue_prog automatically injects the 'file' and 'line' parameters
 * based on compile constants.
 *
 * This kills a process running on the MUCK.  Kill mode can be one of the
 * following:
 *
 *     0: kill all matching processes, MUF or MPI
 *     1: kill all matching MUF processes
 *     2: kill all matching foreground MUF processes
 *
 * file and line are used for debug purposes and should be the file/line
 * that called the function.  It is recommended you use dequeue_prog instead
 * of this call directly so you don't have to worry about those details.
 *
 * @param program the program dbref to kill
 * @param killmode the kill mode, which can be a value noted above
 * @return count of processes killed by this call.
 */
#define dequeue_prog(x,i) dequeue_prog_real(x,i,__FILE__,__LINE__)

/**
 * Dequeue timers associated with a given PID
 *
 * If 'id' is NULL, then all timers will be dequeued.  If it is an ID
 * string, only timers matching that ID will be dequeued.
 *
 * @param pid the PID to dequeue timers for
 * @param id either NULL to dequeue all timers, or a specific timer ID
 * @return boolean true if something was dequeued, false otherwise
 */
int dequeue_timers(int procnum, char *timerid);

/**
 * Scan the environment for propqueues of name 'propname' to run.
 *
 * This starts the with the object 'what' and goes up the environment from
 * there, running ALL propqueues named 'propname' it finds -- so it doesn't
 * stop when it finds, the first propqueue, it keeps going.
 *
 * Functionally it uses propqueue under the hood.  @see propqueue
 *
 * @param descr the person triggering the propqueue
 * @param player the player triggering the propqueue
 * @param where the location where the propqueue was triggered
 * @param trigger the ref of the thing that triggered the propqueue
 * @param what the object to load props from
 * @param xclude program ref to exclude from running
 * @param propname the name of the prop to load; if it is a propdir, then
 *                 its children will be recursively run as well.
 * @param toparg the argument for the propqueue program
 * @param mlev the MUCKER level to run at
 * @param mt if true, this is a "public" message (such as an o-message) and
 *           certain additional rules may apply (mostly to MPI).  For instance,
 *           if true, filter notifications against ignore lists and force
 *           otell name suffix.
 */
void envpropqueue(int descr, dbref player, dbref where, dbref trigger,
                  dbref what, dbref xclude, const char *propname,
                  const char *toparg, int mlev, int mt);

/**
 * Fetch PID info for a given pid into a MUF dictionary
 *
 * If the PID belongs to a MPI timequeue entry or a MUF who's subtype
 * is not TQ_MUF_TIMER, then this will produce a dictionary with the
 * following keys:
 *
 * CALLED_DATA - the initial stack string
 * CALLED_PROG - the dbref of the program associated with the PID
 * CPU - the amount of CPU time used by the process (percentage float)
 * DESCR - the descriptor that called the program
 * FILTERS -  list array of event strings being watched for
 * INSTCNT - the number of instructions run so far
 * MLEVEL - current MUCKER level
 * NEXTRUN - when the process is due to run again (timestamp)
 * PID - the process ID
 * PLAYER - the player DBREf that is running this process
 * STARTED - timestamp of when the process started
 * SUBTYPE - text version of subtyp
 * TRIG - dbref of trigger
 * TYPE - "MUF" or "MPI"
 *
 * Otherwise, this will return the results of @see get_mufevent_pidinfo
 *
 * @param pid the PID to look up information for
 * @param pin boolean true to pin array, false if not
 * @return stk_array dictionary as described above
 */
stk_array *get_pidinfo(int pid, int pin);

/**
 * Get PIDs associated to a given dbref, creating a MUF list array
 *
 * The ref can be either a program dbref (MUF only), the trigger that started
 * the program (MPI), or the user ref who's running the program.
 *
 * -1 can be used as a 'ref' to get all the processes.
 *
 * No permission checking is done.  'pin' is sent straight to new_array_packed
 *
 * @see new_array_packed
 *
 * @param ref a dbref which can be multiple things, see above
 * @param pin boolean true to make a pinned array, false otherwise
 * @return a stk_array list containing all the PIDs requested
 */
stk_array *get_pids(dbref ref, int pin);

/**
 * This is another part of the MUF input read infrastructure
 *
 * This handles things such as the debugger intake.  At the time of
 * this writing, I am unsure of how this vs. read_event_notify work;
 * this seems to be "older".  A lot of tracing will need to be done
 * to figure out which is which.
 *
 * @see read_event_notify
 *
 * @param descr the descriptor of the player submitting input
 * @param player the dbref of the player submitting input
 * @param command the text being submitted to READ
 */
void handle_read_event(int descr, dbref player, const char *command);

/**
 * Listen queue: this is like propqueue but specifically to handle listen
 *
 * This has a lot of overlap with propqueue but is slightly different.
 * MPI is only run if it is mpi_p is true.  This uses event queue-ing instead
 * of do_parse_mesg / interp calls.  I believe this means listen events are
 * delayed whereas normal propqueue events are processed immediately.
 *
 * Additionally, listen propqueue items have a weird quirk.  If the
 * property value is formatted such as:
 *
 * Message=MPI or MUF ref
 *
 * then the MPI or MUF ref is only run when 'Message' is the text in
 * 'toparg'.  This allows an odd conditionality that isn't present in
 * any of the other propqueues.
 *
 * @param descr the person triggering the propqueue
 * @param player the player triggering the propqueue
 * @param where the location where the propqueue was triggered
 * @param trigger the ref of the thing that triggered the propqueue
 * @param what the object to load props from
 * @param xclude program ref to exclude from running
 * @param propname the name of the prop to load; if it is a propdir, then
 *                 its children will be recursively run as well.
 * @param toparg the argument for the propqueue program
 * @param mlev the MUCKER level to run at
 * @param mt if true, this is a "public" message (such as an o-message) and
 *           certain additional rules may apply (mostly to MPI).  For instance,
 *           if true, filter notifications against ignore lists and force
 *           otell name suffix.
 * @param mpi_p if true, allow MPI to run.
 */
void listenqueue(int descr, dbref player, dbref where, dbref trigger,
                 dbref what, dbref xclude, const char *propname,
                 const char *toparg, int mlev, int mt, int mpi_p);

/**
 * Check to see if the given PID is in the time queue
 *
 * @param pid the pid to check for
 * @returns boolean true if pid is in the time queue, false otherwise
 */
int in_timequeue(int pid);

/** 
 * Return the seconds until the the next event will run
 * 
 * This may be -1 if either the next event is is set to when = -1, or
 * if there is nothing on the queue.  It could be 0 if there is something
 * available to run immediately.  Otherwise, it will be the seconds until
 * the next run.
 *
 * @param now the current timestamp
 * @return seconds until next run, -1, or 0
 */
time_t next_event_time(time_t now);

/**
 * This runs any timequeue events that are due to run at the given time
 *
 * This is called by next_muckevent.  @see next_muckevent
 *
 * This will run up to 10 events before returning.  It runs MPI or MUF
 * that is waiting on the queue.  Chances are, you don't want to run
 * this function, it pretty much exists just for next_muckevent
 *
 * @param now the current UNIX timestamp
 */
void next_timequeue_event(time_t now);

/**
 * Runs the given propqueue
 *
 * A propqueue is a property that may have a single value, or it may
 * be a directory with a set of properties (the "queue").  They can
 * contain MPI or dbref references to MUF programs.
 *
 * Propqueues can only be triggered to a recursion level less than 8
 * That is cumulative across all prop queue types.
 *
 * Listen, arrive, drop, and connect are all common propqueues but this
 * function will work with any directory and puts no restriction, making
 * treating any arbitrary prop like a propqueue easy.
 *
 * @param descr the person triggering the propqueue
 * @param player the player triggering the propqueue
 * @param where the location where the propqueue was triggered
 * @param trigger the ref of the thing that triggered the propqueue
 * @param what the object to load props from
 * @param xclude program ref to exclude from running
 * @param propname the name of the prop to load; if it is a propdir, then
 *                 its children will be recursively run as well.
 * @param toparg the argument for the propqueue program
 * @param mlev the MUCKER level to run at
 * @param mt if true, this is a "public" message (such as an o-message) and
 *           certain additional rules may apply (mostly to MPI).  For instance,
 *           if true, filter notifications against ignore lists and force
 *           otell name suffix.
 */
void propqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
               dbref xclude, const char *propname, const char *toparg,
               int mlev, int mt);

/**
 * Function to purge the free_timenode_list
 *
 * This is only used by the MUCK shutdown sequence, if MEMORY_CLEANUP is
 * defined.
 */
void purge_timenode_free_pool(void);

/**
 * Send a read event on behalf of a player
 *
 * So when a player sends input to a MUF program, this is where it goes.
 * It will first try to use muf_event_read_notify which will send the
 * READ string to the first foregrounded program the player has.
 *
 * @see muf_event_read_notify
 *
 * If that fails, then the event is added to MUF programs that are not
 * BACKGROUNDed and are being run by the player using muf_event_add; the
 * first such program found will receive the event.
 *
 * @see muf_event_add
 *
 * If both methods fail, this will return false, otherwise it returns true
 * if the read was successfully received or queued by a program.
 *
 * @param descr the descriptor the input is coming from
 * @param player the player typing in the command
 * @param cmd the command that was typed in by the player
 * @return boolean true if it was successfully received or queued by a MUF
 */
int read_event_notify(int descr, dbref player, const char *cmd);

/**
 * Count number of instances / references to a given program DBREF
 *
 * This will count:
 *
 * - actively running processes for the given dbref
 * - any programs that are using the given dbref as a library
 *
 * @param program the program to check for
 * @return integer the count of instances / references for the program
 */
int scan_instances(dbref program);

/**
 * Get the frame associated with a given PID
 *
 * @param pid the pid to fetch
 * @returns the associated frame, or NULL if not found or not a MUF pid
 */
struct frame *timequeue_pid_frame(int pid);

#endif /* !TIMEQUEUE_H */
