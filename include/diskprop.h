/** @file diskprop.h
 *
 * Header for diskprops - disk based properties for the diskbase database
 * system.
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
#ifndef DISKPROP_H
#define DISKPROP_H

#include "props.h"

/**
 * Set an object as having 'dirty' or changed props
 *
 * Does the necessary addition to the ring-queue as well
 *
 * @param obj the object to befoul
 */
void dirtyprops(dbref obj);

/**
 * Call to dispaly property cache information and disk fetch statistics
 *
 * Dumps a bunch of performance relevant data to the user.
 *
 * @param player the player to display information to
 */
void display_propcache(dbref player);

/**
 * Iterate over the entire database and expire properties based on time
 *
 * Expires properties on objects who have propstime older than
 * tp_clean_interval
 */
void dispose_all_oldprops(void);


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
void fetchprops(dbref obj, const char *pdir);

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
int fetch_propvals(dbref obj, const char *dir);

/**
 * @var PID of the process doing a DB dump
 */
extern pid_t global_dumper_pid;

/**
 * @var the number of cache "hits" (successful use of cache) we get
 */
extern long propcache_hits;

/**
 * @var the number of cache "misses" (requiring the loading of properties)
 *      we get
 */
extern long propcache_misses;

/**
 * Fetch a single property 'p', loading the data into it
 *
 * @param obj the object the property belongs to
 * @param p the property structure to load data for
 * @return boolean true if property was loaded
 */
int propfetch(dbref obj, PropPtr p);

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
void putprops_copy(FILE * f, dbref obj);

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
void skipproperties(FILE * f, dbref obj);

/**
 * Clear all in-memory properties off the given object
 *
 * Does all necessary cleanup; sets PROPS_UNLOADED, removes it from
 * the ringqueue that keeps track of objects with loaded properties, etc.
 *
 * @param obj the object to clean up props for
 */
void unloadprops_with_prejudice(dbref obj);

/**
 * Remove 'dirty' or changed props setting
 *
 * This removes the props from the object as well.  It does nothing if
 * the props are not loaded.
 *
 * @param obj the object to undirty
 */
void undirtyprops(dbref obj);

#endif /* !DISKPROP_H */
#endif /* DISKBASE */
