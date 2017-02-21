#include "config.h"

#ifdef DISKBASE
#include "db.h"
#include "diskprop.h"
#include "fbstrings.h"
#include "game.h"
#include "interface.h"
#include "props.h"
#include "tune.h"

#define FETCHSTATS_INTERVAL1 120
#define FETCHSTATS_INTERVAL2 600
#define FETCHSTATS_INTERVAL3 3600

#define FETCHSTATS_SLOT_TIME 30
#define FETCHSTATS_SLOTS ((FETCHSTATS_INTERVAL3 / FETCHSTATS_SLOT_TIME) + 1)

struct pload_Q {
    dbref obj;
    long count;
    int Qtype;
};

struct pload_Q propchanged_Q = { NOTHING, 0, PROPS_CHANGED };
struct pload_Q proploaded_Q = { NOTHING, 0, PROPS_LOADED };
struct pload_Q proppri_Q = { NOTHING, 0, PROPS_PRIORITY };

long propcache_hits = 0L;
long propcache_misses = 0L;

static int fetchstats[FETCHSTATS_SLOTS];
static int lastfetchslot = -1;

static int fetchprops_priority(dbref obj, int mode, const char *pdir);

int
fetch_propvals(dbref obj, const char *dir)
{
    PropPtr p, pptr;
    int cnt = 0;
    char buf[BUFFER_LEN];
    char name[BUFFER_LEN];

    p = first_prop_nofetch(obj, dir, &pptr, name, sizeof(name));
    while (p) {
	cnt = (cnt || propfetch(obj, p));
	if (PropDir(p) || (PropFlags(p) & PROP_DIRUNLOADED)) {
	    strcpyn(buf, sizeof(buf), dir);
	    strcatn(buf, sizeof(buf), name);
	    strcatn(buf, sizeof(buf), "/");
	    if (PropFlags(p) & PROP_DIRUNLOADED) {
		SetPFlags(p, (PropFlags(p) & ~PROP_DIRUNLOADED));
		getproperties(input_file, obj, buf);
	    }
	    fetch_propvals(obj, buf);
	}
	p = next_prop(pptr, p, name, sizeof(name));
    }
    return cnt;
}

void
putprops_copy(FILE * f, dbref obj)
{
    char buf[BUFFER_LEN * 3];
    char *ptr;
    FILE *g;

    if (DBFETCH(obj)->propsmode != PROPS_UNLOADED) {
	if (fetch_propvals(obj, "/")) {
	    fseek(f, 0L, 2);
	}
	putproperties(f, obj);
	return;
    }
    if (db_conversion_flag) {
	if (fetchprops_priority(obj, 1, NULL) || fetch_propvals(obj, "/")) {
	    fseek(f, 0L, 2);
	}
	putproperties(f, obj);
	return;
    }
    g = input_file;
    putstring(f, "*Props*");
    if (DBFETCH(obj)->propsfpos) {
	fseek(g, DBFETCH(obj)->propsfpos, 0);
	ptr = fgets(buf, sizeof(buf), g);
	if (!ptr)
	    abort();
	for (;;) {
	    ptr = fgets(buf, sizeof(buf), g);
	    if (!ptr)
		abort();
	    if (!strcasecmp(ptr, "*End*\n"))
		break;
	    fputs(buf, f);
	}
    }
    putstring(f, "*End*");
}

void
skipproperties(FILE * f, dbref obj)
{
    char buf[BUFFER_LEN * 3];
    int islisten = 0;

    /* get rid of first line */
    fgets(buf, sizeof(buf), f);

    fgets(buf, sizeof(buf), f);
    while (strcmp(buf, "*End*\n")) {
	if (!islisten) {
	    if (string_prefix(buf, LISTEN_PROPQUEUE))
		islisten = 1;
	    if (string_prefix(buf, WLISTEN_PROPQUEUE))
		islisten = 1;
	    if (string_prefix(buf, WOLISTEN_PROPQUEUE))
		islisten = 1;
	}
	fgets(buf, sizeof(buf), f);
    }
    if (islisten) {
	FLAGS(obj) |= LISTENER;
    } else {
	FLAGS(obj) &= ~LISTENER;
    }
}

static void
removeobj_ringqueue(dbref obj)
{
    struct pload_Q *ref = NULL;

    switch (DBFETCH(obj)->propsmode) {
    case PROPS_UNLOADED:
	return;
	break;
    case PROPS_LOADED:
	ref = &proploaded_Q;
	break;
    case PROPS_PRIORITY:
	ref = &proppri_Q;
	break;
    case PROPS_CHANGED:
	ref = &propchanged_Q;
	break;
    }

    if (DBFETCH(obj)->nextold == NOTHING || DBFETCH(obj)->prevold == NOTHING)
	return;
    if (!ref || ref->obj == NOTHING)
	return;

    if (DBFETCH(obj)->nextold == obj || DBFETCH(obj)->prevold == obj) {
	if (ref->obj == obj)
	    ref->obj = NOTHING;
    } else {
	DBFETCH(DBFETCH(obj)->prevold)->nextold = DBFETCH(obj)->nextold;
	DBFETCH(DBFETCH(obj)->nextold)->prevold = DBFETCH(obj)->prevold;
	if (ref->obj == obj)
	    ref->obj = DBFETCH(obj)->nextold;
    }
    DBFETCH(obj)->prevold = NOTHING;
    DBFETCH(obj)->nextold = NOTHING;
    ref->count--;
}

static void
addobject_ringqueue(dbref obj, int mode)
{
    struct pload_Q *ref = NULL;

    removeobj_ringqueue(obj);

    DBFETCH(obj)->propsmode = mode;
    switch (mode) {
    case PROPS_UNLOADED:
	DBFETCH(obj)->nextold = NOTHING;
	DBFETCH(obj)->prevold = NOTHING;
	return;
	break;
    case PROPS_LOADED:
	ref = &proploaded_Q;
	break;
    case PROPS_PRIORITY:
	ref = &proppri_Q;
	break;
    case PROPS_CHANGED:
	ref = &propchanged_Q;
	break;
    }

    if (ref->obj == NOTHING) {
	DBFETCH(obj)->nextold = obj;
	DBFETCH(obj)->prevold = obj;
	ref->obj = obj;
    } else {
	DBFETCH(obj)->nextold = ref->obj;
	DBFETCH(DBFETCH(ref->obj)->prevold)->nextold = obj;
	DBFETCH(obj)->prevold = DBFETCH(ref->obj)->prevold;
	DBFETCH(ref->obj)->prevold = obj;
    }
    ref->count++;
}


static dbref
first_ringqueue_obj(struct pload_Q *ref)
{
    if (!ref)
	return NOTHING;
    return (ref->obj);
}


static dbref
next_ringqueue_obj(struct pload_Q * ref, dbref obj)
{
    if (DBFETCH(obj)->nextold == ref->obj)
	return NOTHING;
    return (DBFETCH(obj)->nextold);
}

static void
update_fetchstats(void)
{
    time_t now;
    int slot;

    now = time(NULL);
    slot = ((now / FETCHSTATS_SLOT_TIME) % FETCHSTATS_SLOTS);
    if (slot != lastfetchslot) {
	if (lastfetchslot == -1) {
	    for (int i = 0; i < FETCHSTATS_SLOTS; i++) {
		fetchstats[i] = -1;
	    }
	}
	while (lastfetchslot != slot) {
	    lastfetchslot = ((lastfetchslot + 1) % FETCHSTATS_SLOTS);
	    fetchstats[lastfetchslot] = 0;
	}
    }
    fetchstats[slot] += 1;
}

static void
report_fetchstats(dbref player)
{
    int i, count, slot;
    double sum, minv, maxv;
    time_t now;

    now = time(NULL);
    i = slot = ((now / FETCHSTATS_SLOT_TIME) % FETCHSTATS_SLOTS);

    while (lastfetchslot != slot) {
	lastfetchslot = ((lastfetchslot + 1) % FETCHSTATS_SLOTS);
	fetchstats[lastfetchslot] = 0;
    }

    sum = maxv = 0.0;
    minv = fetchstats[i];
    count = 0;

    for (; count < (FETCHSTATS_INTERVAL1 / FETCHSTATS_SLOT_TIME); count++) {
	if (fetchstats[i] == -1)
	    break;
	sum += fetchstats[i];
	if (fetchstats[i] < minv)
	    minv = fetchstats[i];
	if (fetchstats[i] > maxv)
	    maxv = fetchstats[i];
	i = ((i + FETCHSTATS_SLOTS - 1) % FETCHSTATS_SLOTS);
    }
    notifyf(player, "Disk Fetches %2g minute min/avg/max: %.2f/%.2f/%.2f",
	    (FETCHSTATS_INTERVAL1 / 60.0),
	    (minv * 60.0 / FETCHSTATS_SLOT_TIME),
	    (sum * 60.0 / (FETCHSTATS_SLOT_TIME * count)),
	    (maxv * 60.0 / FETCHSTATS_SLOT_TIME));

    for (; count < (FETCHSTATS_INTERVAL2 / FETCHSTATS_SLOT_TIME); count++) {
	if (fetchstats[i] == -1)
	    break;
	sum += fetchstats[i];
	if (fetchstats[i] < minv)
	    minv = fetchstats[i];
	if (fetchstats[i] > maxv)
	    maxv = fetchstats[i];
	i = ((i + FETCHSTATS_SLOTS - 1) % FETCHSTATS_SLOTS);
    }
    notifyf(player, "Disk Fetches %2g minute min/avg/max: %.2f/%.2f/%.2f",
	    (FETCHSTATS_INTERVAL2 / 60.0),
	    (minv * 60.0 / FETCHSTATS_SLOT_TIME),
	    (sum * 60.0 / (FETCHSTATS_SLOT_TIME * count)),
	    (maxv * 60.0 / FETCHSTATS_SLOT_TIME));

    for (; count < (FETCHSTATS_INTERVAL3 / FETCHSTATS_SLOT_TIME); count++) {
	if (fetchstats[i] == -1)
	    break;
	sum += fetchstats[i];
	if (fetchstats[i] < minv)
	    minv = fetchstats[i];
	if (fetchstats[i] > maxv)
	    maxv = fetchstats[i];
	i = ((i + FETCHSTATS_SLOTS - 1) % FETCHSTATS_SLOTS);
    }
    notifyf(player, "Disk Fetches %2g minute min/avg/max: %.2f/%.2f/%.2f",
	    (FETCHSTATS_INTERVAL3 / 60.0),
	    (minv * 60.0 / FETCHSTATS_SLOT_TIME),
	    (sum * 60.0 / (FETCHSTATS_SLOT_TIME * count)),
	    (maxv * 60.0 / FETCHSTATS_SLOT_TIME));
}

static void
report_cachestats(dbref player)
{
    dbref obj;
    int count, total, checked, gap, ipct;
    time_t when, now;
    double pct;

    notify(player, "LRU proploaded cache time distribution graph.");

    total = proploaded_Q.count;
    checked = 0;
    gap = 0;
    when = now = time(NULL);
    notify(player, "Mins  Objs (%of db) Graph of #objs vs. age.");
    for (; checked < total; when -= 60) {
	count = 0;
	obj = first_ringqueue_obj(&proploaded_Q);
	while (obj != NOTHING) {
	    if (DBFETCH(obj)->propstime > (when - 60) && DBFETCH(obj)->propstime <= when)
		count++;
	    obj = next_ringqueue_obj(&proploaded_Q, obj);
	}
	checked += count;
	pct = count * 100.0 / total;
	ipct = count * 100 / total;
	if (ipct > 50)
	    ipct = 50;
	if (count) {
	    if (gap)
		notify(player, "[gap]");
	    notifyf(player, "%3ld:%6d (%5.2f%%) %*s", ((now - when) / 60), count, pct, ipct,
		    "*");
	    gap = 0;
	} else {
	    gap = 1;
	}
    }
}

void
display_propcache(dbref player)
{
    double ph, pm;

    ph = propcache_hits;
    pm = propcache_misses;

    notify(player, "Cache info:");
    notifyf(player, "Propcache hit ratio: %.3f%% (%ld hits / %ld fetches)",
	    (100.0 * ph / (ph + pm)), propcache_hits, propcache_misses);
    report_fetchstats(player);

    notifyf(player, "PropLoaded count: %d", proploaded_Q.count);
    notifyf(player, "PropPriority count: %d", proppri_Q.count);
    notifyf(player, "PropChanged count: %d", propchanged_Q.count);
    report_cachestats(player);
    notify(player, "Done.");
}

void
unloadprops_with_prejudice(dbref obj)
{
    PropPtr l;

    if ((l = DBFETCH(obj)->properties)) {
	/* if it has props, then dispose */
	delete_proplist(l);
	DBFETCH(obj)->properties = NULL;
    }
    removeobj_ringqueue(obj);
    DBFETCH(obj)->propsmode = PROPS_UNLOADED;
    DBFETCH(obj)->propstime = 0;
}

static int
disposeprops_notime(dbref obj)
{
    if (DBFETCH(obj)->propsmode == PROPS_UNLOADED)
	return 0;
    if (DBFETCH(obj)->propsmode == PROPS_CHANGED)
	return 0;

    unloadprops_with_prejudice(obj);
    return 1;
}

static int
disposeprops(dbref obj)
{
    if ((time(NULL) - DBFETCH(obj)->propstime) < tp_clean_interval)
	return 0;		/* don't dispose if less than X minutes old */
    return disposeprops_notime(obj);
}

void
dispose_all_oldprops(void)
{
    time_t now = time(NULL);

    for (dbref i = 0; i < db_top; i++) {
	if ((now - DBFETCH(i)->propstime) >= tp_clean_interval)
	    disposeprops_notime(i);
    }
}

static void
housecleanprops(void)
{
    int limit, max;
    dbref i, j;

    if ((proploaded_Q.count < 100) ||
	(proploaded_Q.count < (tp_max_loaded_objs * db_top / 100)))
	return;

    limit = 40;
    max = db_top;
    i = first_ringqueue_obj(&proploaded_Q);
    while (limit > 0 && max-- > 0 && i != NOTHING) {
	j = next_ringqueue_obj(&proploaded_Q, i);
	if (disposeprops_notime(i))
	    limit--;
	i = j;
    }
}

static int
fetchprops_priority(dbref obj, int mode, const char *pdir)
{
    const char *s;
    int hitflag = 0;

    /* update fetched timestamp */
    DBFETCH(obj)->propstime = time(NULL);

    /* if in memory, don't try to reload. */
    if (DBFETCH(obj)->propsmode != PROPS_UNLOADED) {

	/* but do update the queue position */
	addobject_ringqueue(obj, DBFETCH(obj)->propsmode);
	if (!pdir)
	    pdir = "/";
	while ((s = propdir_unloaded(DBFETCH(obj)->properties, pdir))) {
	    propcache_misses++;
	    hitflag++;
	    if (!mode)
		update_fetchstats();
	    getproperties(input_file, obj, s);
	}
	if (hitflag) {
	    return 1;
	} else {
	    propcache_hits++;
	    return 0;
	}
    }

    propcache_misses++;

    housecleanprops();

    /* actually load in root properties */
    getproperties(input_file, obj, "/");

    /* update fetch statistics */
    if (!mode)
	update_fetchstats();

    /* add object to appropriate queue */
    addobject_ringqueue(obj, ((mode) ? PROPS_PRIORITY : PROPS_LOADED));

    return 1;
}

void
fetchprops(dbref obj, const char *pdir)
{
    fetchprops_priority(obj, 0, pdir);
}

void
dirtyprops(dbref obj)
{
    if (DBFETCH(obj)->propsmode == PROPS_CHANGED)
	return;

    addobject_ringqueue(obj, PROPS_CHANGED);
}

void
undirtyprops(dbref obj)
{
    if (DBFETCH(obj)->propsmode == PROPS_UNLOADED)
	return;

    if (DBFETCH(obj)->propsmode != PROPS_CHANGED) {
	disposeprops(obj);
	return;
    }

    addobject_ringqueue(obj, PROPS_LOADED);
    disposeprops(obj);
}

int
propfetch(dbref obj, PropPtr p)
{
    if (!p)
	return 0;
    SetPFlags(p, (PropFlags(p) | PROP_TOUCHED));
    if (PropFlags(p) & PROP_ISUNLOADED) {
	db_get_single_prop(input_file, obj, (long) PropDataVal(p), p, NULL);
	return 1;
    }
    return 0;
}

#endif				/* DISKBASE */
