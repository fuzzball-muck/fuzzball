#ifndef P_MISC_H
#define P_MISC_H

#include "interp.h"

/**
 * Implementation of MUF TIME
 *
 * Returns the seconds, minutes, and hours of the current time.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_time(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DATE
 *
 * Returns the month day, month number, and 4-digit year of the current date.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_date(PRIM_PROTOTYPE);

/**
 * Implementation of MUF GMTOFFSET
 *
 * Returns the time zone offset from GMT.
 *
 * @see get_tz_offset
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_gmtoffset(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SYSTIME
 *
 * Returns the current Unix time as an integer.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_systime(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SYSTIME_PRECISE
 *
 * Returns the current Unix time as an floating point number.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_systime_precise(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TIMESPLIT
 *
 * Consumes an integer representing a Unix time, and returns the
 * corresponsing seconds, minutes, hours, month day, month number, 4-digit
 * year, weekday number, the day of the year.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_timesplit(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TIMEFMT
 *
 * Consumes a string and a Unix time, and returns a string formatted with
 * the time, via the system's strftime function.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_timefmt(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FMTTIME
 *
 * Consumes two strings (input and format), and returns the Unix time
 * resulting from applying the format to the input.
 *
 * @see time_string_to_seconds
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_fmttime(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CONVTIME
 *
 * Consumes a time string in the format "HH:MM:SS MO/DY/YR" into a Unix time.
 *
 * @see time_string_to_seconds
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_convtime(PRIM_PROTOTYPE);

/**
 * Implementation of MUF USERLOG
 *
 * Consumes a string, and formats it into a message to write to the user log,
 * which is by default 'logs/user'.  Requires MUCKER level tp_userlog_mlev.
 *
 * Format is:
 * player(#) [program.muf(#)] timestamp: message
 *
 * @see log_user
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_userlog(PRIM_PROTOTYPE);

/**
 * Implementation of MUF QUEUE
 *
 * Consumes an integer, a program dbref, and a string - and executes the
 * program after the given number of seconds, with the string as a parameter.
 * Requires MUCKER level 3.
 *
 * @see add_muf_delayq_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_queue(PRIM_PROTOTYPE);

/**
 * Implementation of MUF KILL
 *
 * Consumes an integer, and tries to kill the associated process - returning
 * 1 if successful and 0 if not.  Requires MUCKER level 3 to bypass ownership
 * checks.
 *
 * @see control_process
 * @see dequeue_process
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_kill(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FORCE
 *
 * Consumes a player or thing dbref and a non-null command string, and forces
 * the object to perform the command.  Requires MUCKER level 4.
 *
 * God cannot be forced by programs not owned by God, if GOD_PRIV is enabled.
 *
 * @see process_command
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_force(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TIMESTAMPS
 *
 * Consumes a dbref, and returns the created date, last modified date, last
 * used date, and use count.  Requires MUCKER level 2 to bypass basic
 * ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_timestamps(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FORK
 *
 * Spawns a copy of the current program in the background.  A child process
 * created in this way has an added 0 on its stack, and this primitive
 * returns the child's PID (or -1 if unsuccessful) to the parent.   Requires
 * MUCKER level 3.
 *
 * @see add_muf_delay_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_fork(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PID
 *
 * Returns the current program's PID.
 *
 * @see add_muf_delay_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pid(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STATS
 *
 * Consumes a player dbref, and returns the number of owned objects as a
 * series of integers: total, rooms, exits, things, programs, players, and
 * garbage.  Requires MUCKER level 3.
 *
 * If the dbref is NOTHING, the integers represent all objects MUCK-wide.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_stats(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ABORT
 *
 * Consumes a string, and aborts the current program with that message.
 *
 * @see abort_interp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_abort(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ISPID?
 *
 * Consumes an integer, and returns a boolean that represents if it is a
 * valid PID (either in the timequeue or the current PID)
 *
 * @see in_timequeue
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ispidp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PARSELOCK
 *
 * Consumes an string, and attempts to parse it into a lock.  Returns
 * the special lock [TRUE_BOOLEXP] if not successful.
 *
 * Match failure messages are emitted during this process.
 *
 * @see parselock
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_parselock(PRIM_PROTOTYPE);

/**
 * Implementation of MUF UNPARSELOCK
 *
 * Consumes a lock, and returns its raw string representation.
 *
 * @see unparse_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_unparselock(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PRETTYLOCK
 *
 * Consumes a lock, and returns it as a human-readable string.
 *
 * @see unparse_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_prettylock(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TESTLOCK
 *
 * Consumes a lock and a player or thing dbref, and returns a boolean that
 * represents the evaluation of the object against the lock.  Requires
 * MUCKER level 2 to bypass basic ownership checks.
 *
 * If the sysparm consistent_lock_source is on, the source object is the
 * trigger stored in the program frame, otherwise it is the player.
 *
 * @see eval_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_testlock(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SYSPARM
 *
 * Consumes a string, and returns the associated sysparm value, if the
 * current MUCKER level allows it.
 *
 * @see tune_get_parmstring
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_sysparm(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CANCALL?
 *
 * Consumes a program dbref and a string, and returns a boolean that
 * represents if the current program has permissions to call the function
 * in the given program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_cancallp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SETSYSPARM
 *
 * Consumes two strings, and sets the value of the sysparm represented by
 * the first to the second.  Requires MUCKER level 4 and cannot be forced.
 *
 * @see tune_setparm
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_setsysparm(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SYSPARM_ARRAY
 *
 * Consumes a string, and returns the associated sysparm details, if the
 * current MUCKER level allows it.
 *
 * @see tune_parms_array
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_sysparm_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TIMER_START
 *
 * Consumes an integer and a string, and starts a timer event with the given
 * name to come due after the given number of seconds.
 *
 * Each MUF process is limited to process_timer_limit timers.
 *
 * @see add_muf_timer_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_timer_start(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TIMER_STOP
 *
 * Consumes a string, and stops the timer with the given name.
 *
 * @see dequeue_timers
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_timer_stop(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EVENT_COUNT
 *
 * Returns the number of events that exist for the frame.
 *
 * @see event_count
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_event_count(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EVENT_EXISTS
 *
 * Consumes a string, and returns the number of events with the given type
 * that exist for the frame.
 *
 * @see event_exists
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_event_exists(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EVENT_SEND
 *
 * Consumes a process ID, event name, and a data item - and sends a USER
 * event to the given proess with the given data.  Requires MUCKER level 3.
 *.
 * @see muf_event_add
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_event_send(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXT-NAME-OK?
 *
 * Consumes a string and an object specification, and returns a boolean that
 * represents if an object can be created with the given name.
 * 
 * The specification can be an object that is of the same type of the object,
 * or a string representing the type.  If the second is used, the values can
 * be one of these case-sensitive strings:
 * - "e", "exit"
 * - "r", "room"
 * - "t", "thing"
 * - "p", "player"
 * - "f", "muf", "program"
 *
 * @see ok_object_name
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ext_name_okp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FORCE_LEVEL
 *
 * Returns the force level of the current program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_force_level(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FORCEDBY
 *
 * Returns the object that most recently forced the current program, or
 * NOTHING if no forcing has occurred.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_forcedby(PRIM_PROTOTYPE);

/**
 * Implementation of MUF FORCEDBY_ARRAY
 *
 * Returns an array of objects forcingthe current program.  Requires
 * MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_forcedby_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF WATCHPID
 *
 * Consumes a process ID, and schedules a PROC.EXIT event to be sent to the
 * current program when the process ends.  If the process does not exist,
 * the event is sent immediately.   Requires MUCKER level 3.
 *
 * @see muf_event_add
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_watchpid(PRIM_PROTOTYPE);

/**
 * Implementation of MUF READ_WANTS_BLANKS
 *
 * Specifies that future READs in this frame can return a blank string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_read_wants_blanks(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DEBUGGER_BREAK
 *
 * Starts the MUF debugger.  There cannot be more than MAX_BREAKS breakpoints.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_debugger_break(PRIM_PROTOTYPE);

/**
 * Implementation of MUF IGNORING?
 *
 * Consumes two dbrefs, and returns a boolean that represents if the first
 * object's owner is ignoring the second's.  Requires MUCKER level 3.
 *
 * Returns 0 if the sysparm ignore_support is off.
 *
 * @see ignore_is_ignoring
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ignoringp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF IGNORE_ADD
 *
 * Consumes two dbrefs, and adds the second's owner to the first's owner's
 * ignore list.  Requires MUCKER level 3.
 *
 * Does not do anything if the sysparm ignore_support is off.
 *
 * @see ignore_add_player
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ignore_add(PRIM_PROTOTYPE);

/**
 * Implementation of MUF IGNORE_DEL
 *
 * Consumes two dbrefs, and removes the second's owner to the first's owner's
 * ignore list.  Requires MUCKER level 3.
 *
 * Does not do anything if the sysparm ignore_support is off.
 *
 * @see ignore_removeplayer
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ignore_del(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DEBUG_ON
 *
 * Sets the current program DEBUG, allowing the owner to see the stack trace.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_debug_on(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DEBUG_OFF
 *
 * Sets the current program !DEBUG, turning off the stack trace.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_debug_off(PRIM_PROTOTYPE);

/**
 * Implementation of MUF DEBUG_LINE
 *
 * Shows the stack trace for the current line.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_debug_line(PRIM_PROTOTYPE);

/**
 * Implementation of MUF READ_WANTS_NO_BLANKS
 *
 * Specifies that future READs in this frame cannot return a blank string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_read_wants_no_blanks(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STATS_ARRAY
 *
 * Consumes a player dbref, and returns an array containing the number of
 * owned objects as a series of integers: garbage, players, programs, things,
 * exits, rooms, and total.  Requires MUCKER level 3.
 *
 * If the dbref is NOTHING, the integers represent all objects MUCK-wide.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_stats_array(PRIM_PROTOTYPE);

#define PRIMS_MISC_FUNCS prim_time, prim_date, prim_gmtoffset, \
    prim_systime, prim_timesplit, prim_timefmt, prim_fmttime, prim_convtime, \
    prim_userlog, prim_queue, prim_kill, prim_force, prim_timestamps, \
    prim_fork, prim_pid, prim_stats, prim_abort, prim_ispidp, prim_parselock, \
    prim_unparselock, prim_prettylock, prim_testlock, prim_sysparm, \
    prim_cancallp, prim_setsysparm, prim_timer_start, prim_timer_stop, \
    prim_event_count, prim_event_exists, prim_event_send, prim_ext_name_okp, \
    prim_force_level, prim_forcedby, prim_forcedby_array, prim_watchpid, \
    prim_read_wants_blanks, prim_sysparm_array, prim_debugger_break, \
    prim_ignoringp, prim_ignore_add, prim_ignore_del, prim_debug_on, \
    prim_debug_off, prim_debug_line, prim_systime_precise, \
    prim_read_wants_no_blanks, prim_stats_array

#define PRIMS_MISC_NAMES "TIME", "DATE", "GMTOFFSET", \
    "SYSTIME", "TIMESPLIT", "TIMEFMT", "FMTTIME", "CONVTIME", "USERLOG", \
    "QUEUE", "KILL", "FORCE", "TIMESTAMPS", "FORK", "PID", "STATS", "ABORT", \
    "ISPID?", "PARSELOCK", "UNPARSELOCK", "PRETTYLOCK", "TESTLOCK", "SYSPARM", \
    "CANCALL?",	"SETSYSPARM", "TIMER_START", "TIMER_STOP", "EVENT_COUNT", \
    "EVENT_EXISTS", "EVENT_SEND", "EXT-NAME-OK?", "FORCE_LEVEL", "FORCEDBY", \
    "FORCEDBY_ARRAY", "WATCHPID", "READ_WANTS_BLANKS", "SYSPARM_ARRAY", \
    "DEBUGGER_BREAK", "IGNORING?", "IGNORE_ADD", "IGNORE_DEL", "DEBUG_ON", \
    "DEBUG_OFF", "DEBUG_LINE", "SYSTIME_PRECISE", "READ_WANTS_NO_BLANKS", \
    "STATS_ARRAY"

#endif /* !P_MISC_H */
