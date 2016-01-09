#ifdef DISKBASE
#ifndef _DISKPROP_H
#define _DISKPROP_H
extern void dirtyprops(dbref obj);
extern void diskbase_debug(dbref player);
extern int disposeprops(dbref obj);
extern int disposeprops_notime(dbref obj);
extern void dispose_all_oldprops(void);
extern void fetchprops(dbref obj, const char *pdir);
extern int fetchprops_nostamp(dbref obj);
extern int fetchprops_priority(dbref obj, int mode, const char *pdir);
extern int fetch_propvals(dbref obj, const char *dir);
extern pid_t global_dumper_pid;
extern long propcache_hits;
extern long propcache_misses;
extern int propfetch(dbref obj, PropPtr p);
extern void putprops_copy(FILE * f, dbref obj);
extern void skipproperties(FILE * f, dbref obj);
extern void unloadprops_with_prejudice(dbref obj);
extern void undirtyprops(dbref obj);
#endif /* _DISKPROP_H */
#endif
