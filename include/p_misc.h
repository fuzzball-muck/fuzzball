#ifndef _P_MISC_H
#define _P_MISC_H

extern void prim_time(PRIM_PROTOTYPE);
extern void prim_date(PRIM_PROTOTYPE);
extern void prim_gmtoffset(PRIM_PROTOTYPE);
extern void prim_systime(PRIM_PROTOTYPE);
extern void prim_systime_precise(PRIM_PROTOTYPE);
extern void prim_timesplit(PRIM_PROTOTYPE);
extern void prim_timefmt(PRIM_PROTOTYPE);
extern void prim_userlog(PRIM_PROTOTYPE);
extern void prim_queue(PRIM_PROTOTYPE);
extern void prim_kill(PRIM_PROTOTYPE);
extern void prim_force(PRIM_PROTOTYPE);
extern void prim_timestamps(PRIM_PROTOTYPE);
extern void prim_fork(PRIM_PROTOTYPE);
extern void prim_pid(PRIM_PROTOTYPE);
extern void prim_stats(PRIM_PROTOTYPE);
extern void prim_abort(PRIM_PROTOTYPE);
extern void prim_ispidp(PRIM_PROTOTYPE);
extern void prim_parselock(PRIM_PROTOTYPE);
extern void prim_unparselock(PRIM_PROTOTYPE);
extern void prim_prettylock(PRIM_PROTOTYPE);
extern void prim_testlock(PRIM_PROTOTYPE);
extern void prim_sysparm(PRIM_PROTOTYPE);
extern void prim_cancallp(PRIM_PROTOTYPE);
extern void prim_setsysparm(PRIM_PROTOTYPE);
extern void prim_sysparm_array(PRIM_PROTOTYPE);
extern void prim_timer_start(PRIM_PROTOTYPE);
extern void prim_timer_stop(PRIM_PROTOTYPE);
extern void prim_event_count(PRIM_PROTOTYPE);
extern void prim_event_exists(PRIM_PROTOTYPE);
extern void prim_event_send(PRIM_PROTOTYPE);
extern void prim_pname_okp(PRIM_PROTOTYPE);
extern void prim_name_okp(PRIM_PROTOTYPE);
extern void prim_ext_name_okp(PRIM_PROTOTYPE);
extern void prim_force_level(PRIM_PROTOTYPE);
extern void prim_watchpid(PRIM_PROTOTYPE);
extern void prim_read_wants_blanks(PRIM_PROTOTYPE);
extern void prim_debugger_break(PRIM_PROTOTYPE);
extern void prim_ignoringp(PRIM_PROTOTYPE);
extern void prim_ignore_add(PRIM_PROTOTYPE);
extern void prim_ignore_del(PRIM_PROTOTYPE);
extern void prim_debug_on(PRIM_PROTOTYPE);
extern void prim_debug_off(PRIM_PROTOTYPE);
extern void prim_debug_line(PRIM_PROTOTYPE);

#define PRIMS_MISC_FUNCS prim_time, prim_date, prim_gmtoffset, \
    prim_systime, prim_timesplit, prim_timefmt, prim_userlog, \
    prim_queue, prim_kill, prim_force, prim_timestamps, prim_fork, \
    prim_pid, prim_stats, prim_abort, prim_ispidp, prim_parselock, \
    prim_unparselock, prim_prettylock, prim_testlock, prim_sysparm, \
    prim_cancallp, prim_setsysparm, prim_timer_start, prim_timer_stop, \
    prim_event_count, prim_event_exists, prim_event_send, prim_pname_okp, \
    prim_name_okp, prim_ext_name_okp, prim_force_level, prim_watchpid, \
    prim_read_wants_blanks, prim_sysparm_array, prim_debugger_break, \
    prim_ignoringp, prim_ignore_add, prim_ignore_del, prim_debug_on, \
    prim_debug_off, prim_debug_line, prim_systime_precise

#define PRIMS_MISC_NAMES "TIME", "DATE", "GMTOFFSET", \
    "SYSTIME", "TIMESPLIT", "TIMEFMT", "USERLOG", \
    "QUEUE", "KILL", "FORCE", "TIMESTAMPS", "FORK", \
    "PID", "STATS", "ABORT", "ISPID?", "PARSELOCK", \
    "UNPARSELOCK", "PRETTYLOCK", "TESTLOCK", "SYSPARM", \
    "CANCALL?",	"SETSYSPARM", "TIMER_START", "TIMER_STOP", \
    "EVENT_COUNT", "EVENT_EXISTS", "EVENT_SEND", "PNAME-OK?", \
    "NAME-OK?", "EXT-NAME-OK?", "FORCE_LEVEL", "WATCHPID", \
    "READ_WANTS_BLANKS", "SYSPARM_ARRAY", "DEBUGGER_BREAK", \
    "IGNORING?", "IGNORE_ADD", "IGNORE_DEL", "DEBUG_ON", \
    "DEBUG_OFF", "DEBUG_LINE", "SYSTIME_PRECISE"

#define PRIMS_MISC_CNT 43

#endif				/* _P_MISC_H */
