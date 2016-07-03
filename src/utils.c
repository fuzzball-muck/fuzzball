#include "config.h"

#include "db.h"

dbref
remove_first(dbref first, dbref what)
{
    dbref prev;

    /* special case if it's the first one */
    if (first == what) {
	return NEXTOBJ(first);
    } else {
	/* have to find it */
	DOLIST(prev, first) {
	    if (NEXTOBJ(prev) == what) {
		DBSTORE(prev, next, NEXTOBJ(what));
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
	if ((CONTENTS(list)) && (member(thing, CONTENTS(list)))) {
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
	rest = NEXTOBJ(list);
	PUSH(list, newlist);
	DBDIRTY(newlist);
	list = rest;
    }
    return newlist;
}
