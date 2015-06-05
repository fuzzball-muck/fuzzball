#include "config.h"

#include "db.h"
#include "tune.h"

/* remove the first occurence of what in list headed by first */
dbref
remove_first(dbref first, dbref what)
{
	dbref prev;

	/* special case if it's the first one */
	if (first == what) {
		return DBFETCH(first)->next;
	} else {
		/* have to find it */
		DOLIST(prev, first) {
			if (DBFETCH(prev)->next == what) {
				DBSTORE(prev, next, DBFETCH(what)->next);
				return first;
			}
		}
		return first;
	}
}

int
member(dbref thing, dbref list)
{
	DOLIST(list, list) {
		if (list == thing)
			return 1;
		if ((DBFETCH(list)->contents)
			&& (member(thing, DBFETCH(list)->contents))) {
			return 1;
		}
	}

	return 0;
}

dbref
reverse(dbref list)
{
	dbref newlist;
	dbref rest;

	newlist = NOTHING;
	while (list != NOTHING) {
		rest = DBFETCH(list)->next;
		PUSH(list, newlist);
		DBDIRTY(newlist);
		list = rest;
	}
	return newlist;
}
