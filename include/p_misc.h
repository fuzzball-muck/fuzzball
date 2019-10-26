#ifndef P_MISC_H
#define P_MISC_H

#include "interp.h"

void prim_time(PRIM_PROTOTYPE);
void prim_date(PRIM_PROTOTYPE);
void prim_gmtoffset(PRIM_PROTOTYPE);
void prim_systime(PRIM_PROTOTYPE);
void prim_systime_precise(PRIM_PROTOTYPE);
void prim_timesplit(PRIM_PROTOTYPE);
void prim_timefmt(PRIM_PROTOTYPE);
void prim_fmttime(PRIM_PROTOTYPE);
void prim_convtime(PRIM_PROTOTYPE);
void prim_userlog(PRIM_PROTOTYPE);
void prim_queue(PRIM_PROTOTYPE);
void prim_kill(PRIM_PROTOTYPE);
void prim_force(PRIM_PROTOTYPE);
void prim_timestamps(PRIM_PROTOTYPE);
void prim_fork(PRIM_PROTOTYPE);
void prim_pid(PRIM_PROTOTYPE);
void prim_stats(PRIM_PROTOTYPE);
void prim_abort(PRIM_PROTOTYPE);
void prim_ispidp(PRIM_PROTOTYPE);
void prim_parselock(PRIM_PROTOTYPE);
void prim_unparselock(PRIM_PROTOTYPE);
void prim_prettylock(PRIM_PROTOTYPE);
void prim_testlock(PRIM_PROTOTYPE);
void prim_sysparm(PRIM_PROTOTYPE);
void prim_cancallp(PRIM_PROTOTYPE);
void prim_setsysparm(PRIM_PROTOTYPE);
void prim_sysparm_array(PRIM_PROTOTYPE);
void prim_timer_start(PRIM_PROTOTYPE);
void prim_timer_stop(PRIM_PROTOTYPE);
void prim_event_count(PRIM_PROTOTYPE);
void prim_event_exists(PRIM_PROTOTYPE);
void prim_event_send(PRIM_PROTOTYPE);
void prim_ext_name_okp(PRIM_PROTOTYPE);
void prim_force_level(PRIM_PROTOTYPE);
void prim_forcedby(PRIM_PROTOTYPE);
void prim_forcedby_array(PRIM_PROTOTYPE);
void prim_watchpid(PRIM_PROTOTYPE);
void prim_read_wants_blanks(PRIM_PROTOTYPE);
void prim_debugger_break(PRIM_PROTOTYPE);
void prim_ignoringp(PRIM_PROTOTYPE);
void prim_ignore_add(PRIM_PROTOTYPE);
void prim_ignore_del(PRIM_PROTOTYPE);
void prim_debug_on(PRIM_PROTOTYPE);
void prim_debug_off(PRIM_PROTOTYPE);
void prim_debug_line(PRIM_PROTOTYPE);
void prim_read_wants_no_blanks(PRIM_PROTOTYPE);
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
