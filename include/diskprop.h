#include "config.h"

#ifdef DISKBASE
#ifndef DISKPROP_H
#define DISKPROP_H

#include "props.h"

void dirtyprops(dbref obj);
void display_propcache(dbref player);
void dispose_all_oldprops(void);
void fetchprops(dbref obj, const char *pdir);
int fetch_propvals(dbref obj, const char *dir);
extern pid_t global_dumper_pid;
extern long propcache_hits;
extern long propcache_misses;
int propfetch(dbref obj, PropPtr p);
void putprops_copy(FILE * f, dbref obj);
void skipproperties(FILE * f, dbref obj);
void unloadprops_with_prejudice(dbref obj);
void undirtyprops(dbref obj);

#endif /* !DISKPROP_H */
#endif
