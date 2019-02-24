#ifndef _DBSEARCH_H
#define _DBSEARCH_H

struct flgchkdat {
	int fortype;		/* check FOR a type? */
	int istype;		/* If check FOR a type, which one? */
	int isnotroom;		/* not a room. */
	int isnotexit;		/* not an exit. */
	int isnotthing;		/* not type thing */
	int isnotplayer;	/* not a player */
	int isnotprog;		/* not a program */
	int forlevel;		/* check for a mucker level? */
	int islevel;		/* if check FOR a mucker level, which level? */
	int isnotzero;		/* not ML0 */
	int isnotone;		/* not ML1 */
	int isnottwo;		/* not ML2 */
	int isnotthree;		/* not ML3 */
	int setflags;		/* flags that are set to check for */
	int clearflags;		/* flags to check are cleared. */
	int forlink;		/* check linking? */
	int islinked;		/* if yes, check if not unlinked */
	int forold;		/* check for old object? */
	int isold;		/* if yes, check if old */
	int loadedsize;		/* check for propval-loaded size? */
	int issize;		/* list objs larger than size? */
	size_t size;		/* what size to check against. No check if 0 */
};

int checkflags(dbref what, struct flgchkdat check);
int init_checkflags(dbref player, const char *flags, struct flgchkdat *check);

#endif /* _DBSEARCH_H */
