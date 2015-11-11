#include "config.h"
#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "externs.h"


/****************************************************************
 * Dump the database every so often.
 ****************************************************************/

static long last_dump_time = 0L;
static int dump_warned = 0;

long
next_dump_time(void)
{
	long currtime = (long) time((time_t *) NULL);

	if (!last_dump_time)
		last_dump_time = (long) time((time_t *) NULL);

	if (tp_dbdump_warning && !dump_warned) {
		if (((last_dump_time + tp_dump_interval) - tp_dump_warntime)
			< currtime) {
			return (0L);
		} else {
			return (last_dump_time + tp_dump_interval - tp_dump_warntime - currtime);
		}
	}

	if ((last_dump_time + tp_dump_interval) < currtime)
		return (0L);

	return (last_dump_time + tp_dump_interval - currtime);
}


void
check_dump_time(void)
{
	long currtime = (long) time((time_t *) NULL);

	if (!last_dump_time)
		last_dump_time = (long) time((time_t *) NULL);

	if (!dump_warned) {
		if (((last_dump_time + tp_dump_interval) - tp_dump_warntime)
			< currtime) {
			dump_warning();
			dump_warned = 1;
		}
	}

	if ((last_dump_time + tp_dump_interval) < currtime) {
		last_dump_time = currtime;
		add_property((dbref) 0, "_sys/lastdumptime", NULL, (int) currtime);

		if (tp_periodic_program_purge)
			free_unused_programs();
		purge_for_pool();
		purge_try_pool();

		fork_and_dump();

		dump_warned = 0;
	}
}


void
dump_db_now(void)
{
	long currtime = (long) time((time_t *) NULL);

	add_property((dbref) 0, "_sys/lastdumptime", NULL, (int) currtime);
	fork_and_dump();
	last_dump_time = currtime;
	dump_warned = 0;
}

/*********************
 * Periodic cleanups *
 *********************/

static long last_clean_time = 0L;

long
next_clean_time(void)
{
	long currtime = (long) time((time_t *) NULL);

	if (!last_clean_time)
		last_clean_time = (long) time((time_t *) NULL);

	if ((last_clean_time + tp_clean_interval) < currtime)
		return (0L);

	return (last_clean_time + tp_clean_interval - currtime);
}


void
check_clean_time(void)
{
	long currtime = (long) time((time_t *) NULL);

	if (!last_clean_time)
		last_clean_time = (long) time((time_t *) NULL);

	if ((last_clean_time + tp_clean_interval) < currtime) {
		last_clean_time = currtime;
		purge_for_pool();
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

long
mintime(long a, long b)
{
	return ((a > b) ? b : a);
}


long
next_muckevent_time(void)
{
	long nexttime = 1000L;

	nexttime = mintime(next_event_time(), nexttime);
	nexttime = mintime(next_dump_time(), nexttime);
	nexttime = mintime(next_clean_time(), nexttime);

	return (nexttime);
}

void
next_muckevent(void)
{
	next_timequeue_event();
	check_dump_time();
	check_clean_time();
}
