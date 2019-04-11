/** @file events.h
 *
 * Header for functions that handle periodic events on the MUCK.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef EVENTS_H
#define EVENTS_H

#include <time.h>

/**
 * Do a database dump now
 *
 * This doesn't do any of the purges that the periodic dump does, but
 * it does set the last dump time property.  It also sets the last
 * dump time to now.
 */
void dump_db_now(void);

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
void next_muckevent(void);

/**
 * Calculate the time until a MUCK event will happen
 *
 * Could be an event, a dump, or a cleanup.  Whichever comes next
 * will determine which time is returned.
 *
 * @return the time until the next MUCK event
 */
time_t next_muckevent_time(void);

#endif /* !EVENTS_H */
