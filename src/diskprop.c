/** @file diskprop.c
 *
 * Implementation for diskprops - disk based properties for the diskbase
 * database system.
 *
 * Diskprop is used to store properties on disk instead of memory, to
 * save compute resources on memory-tight systems.  It is part of the DISKBASE
 * system which seeks to keep things on the disk unless they are needed.
 *
 * It requires the DISKBASE define be set.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#ifdef DISKBASE
#include "db.h"
#include "diskprop.h"
#include "fbstrings.h"
#include "game.h"
#include "interface.h"
#include "props.h"
#include "tune.h"

/* These are for calculating disk fetches for a given time period.
 *
 * Slot time is the granularity of the calculation (i.e. we keep
 * track of how many accesses happen in each 30 second chunk).
 *
 * INTERVAL1, INTERVAL2, and INTERVAL3 are used to display to
 * the user these statistics in sort of the same style as UNIX "w"
 * or maybe more accurately iostat/vmstat.
 */
#define FETCHSTATS_INTERVAL1 120
#define FETCHSTATS_INTERVAL2 600
#define FETCHSTATS_INTERVAL3 3600

#define FETCHSTATS_SLOT_TIME 30
#define FETCHSTATS_SLOTS ((FETCHSTATS_INTERVAL3 / FETCHSTATS_SLOT_TIME) + 1)

/**
 * @private
 * @var a rotating list of access counts
 *
 * Each slot represents a FETCHSTATS_SLOT_TIME interval of disk
 * accesses.  There are enough slots for FETCHSTATS_INTERVAL3 amount
 * of time.
 */
static int fetchstats[FETCHSTATS_SLOTS];

/**
 * @private
 * @var keep track of the last used fetch slot
 */
static int lastfetchslot = -1;

/* Struct to support ring queues */
struct pload_Q {
    dbref obj;
    long count;
    int Qtype;
};

/*
 * These are "ring queues" which are used to keep track of objects with
 * loaded props, changed props, and priority props.  Basically for
 * book keeping -- knowing what needs to be saved, what can be knocked
 * off an LRU (least recently used) list to free memory, etc.
 *
 * They are DB linked lists that use the nextold/prevold db links.
 */

/**
 * @private
 * @var ringqueue for changed props
 */
static struct pload_Q propchanged_Q = { NOTHING, 0, PROPS_CHANGED };

/**
 * @private
 * @var ringqueue for loaded props
 */
static struct pload_Q proploaded_Q = { NOTHING, 0, PROPS_LOADED };

/**
 * @private
 * @var ringqueue for priority props
 */
static struct pload_Q proppri_Q = { NOTHING, 0, PROPS_PRIORITY };

/**
 * @var the number of cache "hits" (successful use of cache) we get
 */
long propcache_hits = 0L;

/**
 * @var the number of cache "misses" (requiring the loading of properties)
 *      we get
 */
long propcache_misses = 0L;

/* See definition for docblock */
static int fetchprops_priority(dbref obj, int mode, const char *pdir);

/**
 * Fetch property values off the disk
 *
 * This recursively fetches property values off the disk.  Typically,
 * you would pass "/" as the dir to get an entire property structure
 * for the given object.
 *
 * @param obj the object to load properties for.
 * @param dir the directory to load property values for
 * @return boolean true if props were fetched, false if not
 */
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
            strcatn(buf, sizeof(buf), (char[]){PROPDIR_DELIMITER,0});

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

/**
 * This handles writing properties to the disk for the diskbase system
 *
 * It is called by db_write_object - @see db_write_object
 *
 * The reason it is called _copy is because if properties are not loaded
 * in memory, this function will load the prop data off the input_file
 * and write it to 'f' as a straight copy.
 *
 * @param f the file to write out to
 * @param obj the object to write properties for
 */
void
putprops_copy(FILE * f, dbref obj)
{
    char buf[BUFFER_LEN * 3];
    char *ptr;

    /* If the props are loaded, save them */
    if (DBFETCH(obj)->propsmode != PROPS_UNLOADED) {
        if (fetch_propvals(obj, (char[]){PROPDIR_DELIMITER,0})) {
            fseek(f, 0L, SEEK_END);
        }

        putproperties(f, obj);
        return;
    }

    /*
     * If -convert is used, that requires some special handling;
     * if props are fetched, then seek to the end of the file.
     */
    if (db_conversion_flag) {
        if (fetchprops_priority(obj, 1, NULL)
            || fetch_propvals(obj, (char[]){PROPDIR_DELIMITER,0})) {
            fseek(f, 0L, SEEK_END);
        }

        putproperties(f, obj);
        return;
    }

    /* 
     * If we got this far, then we're copying properties from 'input_file'
     * to the output.
     */

    putstring(f, "*Props*");

    if (DBFETCH(obj)->propsfpos) {
        fseek(input_file, DBFETCH(obj)->propsfpos, SEEK_SET);
        ptr = fgets(buf, sizeof(buf), input_file);

        if (!ptr)
            abort();

        for (;;) {
            ptr = fgets(buf, sizeof(buf), input_file);

            if (!ptr)
                abort();

            if (!strcasecmp(ptr, "*End*\n"))
                break;

            fputs(buf, f);
        }
    }

    putstring(f, "*End*");
}

/**
 * Skip over the properties block in file handle 'f'
 *
 * This assumes the next line in 'f' is the *Props* marker that starts
 * the prop list.  This is prety vulnerable against malformed files;
 * there is no error checking here.
 *
 * If there are LISTEN_PROPQUEUE, WLISTEN_PROPQUEUE, or WOLISTEN_PROPQUEUE
 * props then the "LISTENER" flag will be added to 'obj'.  Otherwise,
 * the flag will be removed if already set.
 *
 * The file handle will be positioned ready to read the next line after
 * the prop's *End* line.
 *
 * @param f the file handle to work on
 * @param obj the current object we are skipping properties for
 */
void
skipproperties(FILE * f, dbref obj)
{
    char buf[BUFFER_LEN * 3];
    int islisten = 0;

    /* Get rid of first line - should be *Props* */
    fgets(buf, sizeof(buf), f);

    /*
     * Get the next line before entering our while loop so 'buf' is
     * initialized.
     */
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

/**
 * Remove an object from its ring queue
 *
 * The logic is a little funky; the propsmode value determines which
 * queue the object is presumed to be on.
 *
 * @private
 * @param obj the object to remove from the ring queue
 */
static void
removeobj_ringqueue(dbref obj)
{
    struct pload_Q *ref = NULL;

    switch (DBFETCH(obj)->propsmode) {
        case PROPS_UNLOADED:
            return;

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

/**
 * Add an object to the ringqueue.  Mode determines which queue.
 *
 * If mode is PROPS_UNLOADED, the nextold/prevold links will be set
 * to NOTHING and this function will return.
 *
 * Otherwise, PROPS_LOADED goes to the propsloaded_Q.  PROPS_PRIORITY
 * goes to proppri_Q.  PROPS_CHANGED goes to propchanged_Q.
 *
 * If the queue is empty, this node will be the only node on the queue.
 * Otherwise, this will be put on the 'end' of the queue.
 *
 * @private
 * @param obj the object to add to the queue
 * @param mode the PROPS_* mode as described above.
 */
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

        case PROPS_LOADED:
            ref = &proploaded_Q;
            break;

        case PROPS_PRIORITY:
            ref = &proppri_Q;
            break;

        case PROPS_CHANGED:
            ref = &propchanged_Q;
            break;

        default:
            return;
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

/**
 * Return the first ringqueue object for the given queue ref
 *
 * Returns NOTHING if the queue is empty.
 *
 * @private
 * @param ref the reference for the ring queue
 * @return dbref the first ringqueue object ref or NOTHING if queue is empty
 */
static dbref
first_ringqueue_obj(struct pload_Q *ref)
{
    if (!ref)
        return NOTHING;

    return (ref->obj);
}

/**
 * Return the next item after the given obj on the ringqueue
 *
 * This returns NOTHING if there is only one item on the queue or if
 * obj is the last item on the queue
 *
 * @private
 * @param ref the queue reference
 * @param obj the object you want to get the next object for
 * @return dbref the next object on the queue or NOTHING if 'obj' is last
 */
static dbref
next_ringqueue_obj(struct pload_Q * ref, dbref obj)
{
    if (DBFETCH(obj)->nextold == ref->obj)
        return NOTHING;

    return (DBFETCH(obj)->nextold);
}

/**
 * Update Disk Fetch Statistics
 *
 * This is called by fetchprops_priority which is, in turn, called by
 * fetchprops.
 *
 * @see fetchprops_priority
 * @see fetchprops
 *
 * Each time fetchprops_priority fetches something from disk, it calls
 * this to update the counter.  The counter data structure is described
 * at the top of this file.  ITs basically a circular array.
 *
 * @private
 */
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

/**
 * Generate a report on disk fetch statistics and show it to the player
 *
 * The resulting display is a little like vmstat/iostat or 'w' in UNIX.
 *
 * @private
 * @param player the player to display stats to
 */
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

/**
 * Display a report based on cache statistics
 *
 * This traverses the loaded prop ringqueue and does some calculations
 * to show cache utilization to the indicated user.
 *
 * @private
 * @param player the user to display the report to
 */
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

/**
 * Call to dispaly property cache information and disk fetch statistics
 *
 * Dumps a bunch of performance relevant data to the user.
 *
 * @param player the player to display information to
 */
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

/**
 * Clear all in-memory properties off the given object
 *
 * Does all necessary cleanup; sets PROPS_UNLOADED, removes it from
 * the ringqueue that keeps track of objects with loaded properties, etc.
 *
 * @param obj the object to clean up props for
 */
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

/**
 * Unload properties if props are not already unloaded and not changed
 *
 * This doesn't take into account age of loaded properties.  Generally you
 * would want to delete things that haven't been accessed in a long time.
 *
 * @private
 * @param obj the object to clear props for
 * @return boolean true if props were unloaded, false otherwise
 */
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

/**
 * Unload properties, taking into account age of properties
 *
 * Also takes into account if props are changed or already unloaded.
 * Only deletes the props if the propstime is older than tp_clean_interval
 *
 * @private
 * @param obj the object to (potentially) clean props for
 * @return boolean true if props were unloaded, false if not
 */
static int
disposeprops(dbref obj)
{
    if ((time(NULL) - DBFETCH(obj)->propstime) < tp_clean_interval)
        return 0;   /* don't dispose if less than X minutes old */

    return disposeprops_notime(obj);
}

/**
 * Iterate over the entire database and expire properties based on time
 *
 * Expires properties on objects who have propstime older than
 * tp_clean_interval
 */
void
dispose_all_oldprops(void)
{
    time_t now = time(NULL);

    for (dbref i = 0; i < db_top; i++) {
        if ((now - DBFETCH(i)->propstime) >= tp_clean_interval)
            disposeprops_notime(i);
    }
}

/**
 * Do regular cleanup work
 *
 * If there are less than 100 objects with loaded props, this does nothing.
 * If the loaded count is less than tp_max_loaded_objs percent of
 * the database, then this call does nothing.
 *
 * Otherwise, try to clear props for up to 40 eligible objects.  An
 * eligible object is one that has properties loaded and does not have
 * any modifications.
 *
 * This doesn't take into account time.
 *
 * @private
 */
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

/**
 * Fetch properties, with priority support, doing all the book-keeping stuff
 *
 * The book-keeping stuff consists of keeping track of cache misses and
 * making sure the props are on the correct position of the ringqueue.
 *
 * If the object is not marked PROPS_UNLOADED, then make sure all the
 * properties under the provided "pdir" (which can be NULL for "/") are loaded.
 *
 * Otherwise, run housecleaning and then load all the props for the object
 * (in this case, pdir is ignored).
 *
 * If 'mode' is true, then this is loaded into the PROPS_PRIORITY queue.
 * Otherwise it goes to the regular PROPS_LOADED queue.
 *
 * @see housecleanprops
 *
 * @private
 * @param obj the object to fetch props for
 * @param mode boolean true if priorty, false otherwise.
 * @param pdir the directory to load or NULL for "/"
 * @return boolean true if props were loaded, false on a cache hit
 */
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
            pdir = (char[]){PROPDIR_DELIMITER,0};

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
    getproperties(input_file, obj, (char[]){PROPDIR_DELIMITER,0});

    /* update fetch statistics */
    if (!mode)
        update_fetchstats();

    /* add object to appropriate queue */
    addobject_ringqueue(obj, ((mode) ? PROPS_PRIORITY : PROPS_LOADED));

    return 1;
}

/**
 * Fetch properties for an object
 *
 * Checks to see if the object's properties are already loaded first,
 * and if not, will fetch them.  This also runs the periodic clean-up
 * of properties to keep the cache from getting too large.
 *
 * @param obj the object to fetch properties for
 * @param pdir the prop directory to load, or NULL for "/"
 */
inline void
fetchprops(dbref obj, const char *pdir)
{
    fetchprops_priority(obj, 0, pdir);
}

/**
 * Set an object as having 'dirty' or changed props
 *
 * Does the necessary addition to the ring-queue as well
 *
 * @param obj the object to befoul
 */
void
dirtyprops(dbref obj)
{
    if (DBFETCH(obj)->propsmode == PROPS_CHANGED)
        return;

    addobject_ringqueue(obj, PROPS_CHANGED);
}

/**
 * Remove 'dirty' or changed props setting
 *
 * This removes the props from the object as well.  It does nothing if
 * the props are not loaded.
 *
 * @param obj the object to undirty
 */
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

/**
 * Fetch a single property 'p', loading the data into it
 *
 * @param obj the object the property belongs to
 * @param p the property structure to load data for
 * @return boolean true if property was loaded
 */
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

#endif /* DISKBASE */
