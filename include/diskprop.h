#ifdef DISKBASE
#ifndef _DISKPROP_H
#define _DISKPROP_H

#include "props.h"

struct pload_Q {
    dbref obj;
    long count;
    int Qtype;
};

void dirtyprops(dbref obj);
int disposeprops(dbref obj);
int disposeprops_notime(dbref obj);
void dispose_all_oldprops(void);
void fetchprops(dbref obj, const char *pdir);
int fetchprops_nostamp(dbref obj);
int fetchprops_priority(dbref obj, int mode, const char *pdir);
int fetch_propvals(dbref obj, const char *dir);
extern pid_t global_dumper_pid;
extern long propcache_hits;
extern long propcache_misses;
int propfetch(dbref obj, PropPtr p);
void putprops_copy(FILE * f, dbref obj);
void skipproperties(FILE * f, dbref obj);
void unloadprops_with_prejudice(dbref obj);
void undirtyprops(dbref obj);

#endif				/* _DISKPROP_H */
#endif
