#include <time.h>
#include "config.h"
#include "db.h"
#include "tune.h"

void
ts_newobject(struct object *thing)
{
	time_t now = time(NULL);

	thing->ts.created = now;
	thing->ts.modified = now;
	thing->ts.lastused = now;
	thing->ts.usecount = 0;
}

void
ts_useobject(dbref thing)
{
	if (thing == NOTHING)
		return;
	DBFETCH(thing)->ts.lastused = time(NULL);
	DBFETCH(thing)->ts.usecount++;
	DBDIRTY(thing);
	if (Typeof(thing) == TYPE_ROOM)
		ts_useobject(DBFETCH(thing)->location);
}

void
ts_lastuseobject(dbref thing)
{
	if (thing == NOTHING)
		return;
	DBSTORE(thing, ts.lastused, time(NULL));
	if (Typeof(thing) == TYPE_ROOM)
		ts_lastuseobject(DBFETCH(thing)->location);
}

void
ts_modifyobject(dbref thing)
{
	DBSTORE(thing, ts.modified, time(NULL));
}
