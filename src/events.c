/** @file events.c
 *
 * Implementation for functions that handle periodic events on the MUCK.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <time.h>

#include "config.h"

#include "compile.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "events.h"
#include "fbmath.h"
#include "game.h"
#include "interface.h"
#include "interp.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

/****************************************************************
 * Dump the database every so often.
 ****************************************************************/

/**
 * @private
 * @var last database dump time
 */
static time_t last_dump_time = 0L;

/**
 * @private
 * @var boolean has a dump warning been displayed for the upcoming dump?
 */
static int dump_warned = 0;

/**
 * Calculate next dump time
 *
 * This is based off the tp_dump_interval.
 *
 * @param now the time used as current
 * @return the calculated time until dump or 0 if the dump should be now.
 * @private
 */
static time_t
next_dump_time(time_t now)
{
    if (!last_dump_time)
        last_dump_time = now;

    if ((last_dump_time + tp_dump_interval) < now)
        return 0L;

    return (last_dump_time + tp_dump_interval - now);
}

/**
 * Checks if its time to take a dump, among other things.
 *
 * If its time to dump, we also:
 *
 *   * set a last dumped at property
 *   * free unused programs (@see free_unused_programs) if enabled
 *   * purge_for_pool and purge_try_pool
 *     (@see purge_for_pool @see purge_try_pool)
 *
 * If dump warnings are enabled (tp_dbdump_warning) and a warning has not
 * been displayed yet, then the dump warning message will be walled at the
 * appropriate time.
 *
 * The actual dump is fork_and_dump ... @see fork_and_dump
 *
 * @param now the time used as current
 * @private
 */
static void
check_dump_time(time_t now)
{
    time_t nexttime = next_dump_time(now);

    if (tp_dbdump_warning && !dump_warned && nexttime <= (time_t)tp_dump_warntime) {
        wall_and_flush(tp_dumpwarn_mesg);
        dump_warned = 1;
    }

    if (next_dump_time(now) == 0L) {
        last_dump_time = now;
        purge_for_pool();
        purge_try_pool();

        if (tp_periodic_program_purge)
            free_unused_programs();

        add_property((dbref) 0, SYS_LASTDUMPTIME_PROP, NULL, (int)now);
        fork_and_dump();
        dump_warned = 0;
    }
}

/**
 * Do a database dump now
 *
 * This doesn't do any of the purges that the periodic dump does, but
 * it does set the last dump time property.  It also sets the last
 * dump time to now.
 */
void
dump_db_now()
{
    time_t currtime = (time_t) time((time_t *) NULL);

    add_property((dbref) 0, SYS_LASTDUMPTIME_PROP, NULL, (int) currtime);
    last_dump_time = currtime;
    fork_and_dump();
    dump_warned = 0;
}

/*********************
 * Periodic cleanups *
 *********************/

/**
 * @private
 * @var last cleanup time for cleaning up the for pool and 
 */
static time_t last_clean_time = 0L;

/**
 * Calculate the next time to run cleanup based on tp_clean_interval.
 *
 * @param now the time used as current
 * @return the time until the next cleanup or 0 if it should be run now
 */
static time_t
next_clean_time(time_t now)
{
    if (!last_clean_time)
        last_clean_time = now;

    if ((last_clean_time + tp_clean_interval) < now)
        return 0L;

    return (last_clean_time + tp_clean_interval - now);
}

/**
 * Checks cleanup time and performs cleanup if needed
 *
 * Most of this stuff is done at the period DB dump time.  This is
 * separate so it can be run more often for machines with memory
 * constraints.
 *
 *   - purge_for_pool @see purge_for_pool
 *   - purge_try_pool @see purge_try_pool
 *   - free_unused_programs (if tp_periodic_program_purge)
 *     @see free_unused_programs
 *   - dispose_all_oldprops (if DISKBASE) @see dispose_all_oldprops
 *
 * @param now the time used as current
 * @private
 */
static void
check_clean_time(time_t now)
{
    if (next_clean_time(now) == 0L) {
        last_clean_time = now;
        purge_for_pool();
        purge_try_pool();

        if (tp_periodic_program_purge)
            free_unused_programs();

#ifdef DISKBASE
        dispose_all_oldprops();
#endif
    }
}

/**********************************************************************
 *  general handling for timed events like dbdumps, timequeues, etc.
 **********************************************************************/

/**
 * Calculate the time until a MUCK event will happen
 *
 * Could be an event, a dump, or a cleanup.  Whichever comes next
 * will determine which time is returned.
 *
 * @return the time until the next MUCK event
 */
time_t
next_muckevent_time()
{
    time_t nexttime = 1000L;
    time_t now = (time_t) time((time_t *) NULL);

    nexttime = MIN(next_event_time(now), nexttime);
    nexttime = MIN(next_dump_time(now), nexttime);
    nexttime = MIN(next_clean_time(now), nexttime);

    return nexttime;
}

/**
 * Runs muckevents
 *
 * This will run timequeue events, dumps, and cleanups.  While it tries
 * each function, the functions will only do something if something is
 * ready to happen.  This is safe to run whenever, but next_muckevent_time
 * will return the time until this will actually do something.
 *
 * @see next_muckevent_time
 */
void
next_muckevent()
{
    time_t now = (time_t) time((time_t *) NULL);

    next_timequeue_event(now);
    check_dump_time(now);
    check_clean_time(now);
}
