/** @file mufevent.h
 *
 * Header file for MUF Events
 *
 * MUF Events are a form of messaging passing between MUF programs.  It is
 * also used by MCP
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef MUFEVENT_H
#define MUFEVENT_H

#include <stddef.h>

#include "array.h"
#include "config.h"
#include "inst.h"
#include "interp.h"


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
stk_array *get_mufevent_pids(stk_array * nw, dbref ref);

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
stk_array *get_mufevent_pidinfo(stk_array * nw, int pid, int pinned);

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
void muf_event_add(struct frame *fr, char *event, struct inst *val,
                   int exclusive);

/**
 * Returns true if the given player controls the given PID.
 *
 * @param player the player to check for
 * @param pid the pid to see if 'player' controls
 * @return boolean true if 'player' controls 'pid', false otherwise.
 */
int muf_event_controls(dbref player, int pid);

/**
 * Returns how many events are waiting to be processed for a given frame
 *
 * @param fr the frame to get a count for
 * @return the number of events waiting to be processed, or 0 if none
 */
int muf_event_count(struct frame *fr);

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
int muf_event_dequeue(dbref prog, int sleeponly);

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
int muf_event_dequeue_pid(int pid);

/**
 * Returns how many events of the given event type are waiting to be processed.
 *
 * The eventid passed can be an smatch string. @see equalstr
 *
 * @param fr the frame to check for
 * @param an smatch pattern for event names to check for
 * @return count of matching MUF events, which may be 0 for no matches.
 */
int muf_event_exists(struct frame *fr, const char *eventid);

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
int muf_event_list(dbref player, const char *pat);

/**
 * Return the frame associated with a given PID on the EVENT_WAIT queue
 *
 * @param pid the PID to look for
 * @return the 'struct frame' associated with the PID or NULL if PID not found
 */
struct frame *muf_event_pid_frame(int pid);

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
void muf_event_process(void);

/**
 * Purges all muf events from the given program instance's event queue.
 *
 * @param fr the frame who's events we are purging
 */
void muf_event_purge(struct frame *fr);

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
int muf_event_read_notify(int descr, dbref player, const char *cmd);

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
void muf_event_register_specific(
                dbref player, dbref prog, struct frame *fr, size_t eventcount,
                char **eventids);

#endif /* !MUFEVENT_H */