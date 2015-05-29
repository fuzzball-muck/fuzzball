/*
 *
 * $Log: hashtab.c,v $
 * Revision 1.4  2005/07/04 12:04:21  winged
 * Initial revisions for everything.
 *
 * Revision 1.3  2001/10/15 02:08:49  revar
 * Minor cleanups for possible Win32 port plans.
 *
 * Revision 1.2  2000/03/29 12:21:02  revar
 * Reformatted all code into consistent format.
 * 	Tabs are 4 spaces.
 * 	Indents are one tab.
 * 	Braces are generally K&R style.
 * Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
 * Rewrote restart script as a bourne shell script.
 *
 * Revision 1.1.1.1  1999/12/16 03:23:29  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.1.1.1  1999/12/12 07:27:43  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/12 02:22:25  foxen
 * Initial revision
 *
 * Revision 5.4  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.3  1994/03/14  12:08:46  foxen
 * Initial portability mods and bugfixes.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.3  90/09/28  12:21:59  rearl
 * Made some speed enhancements, fixed declarations of const char's.
 *
 * Revision 1.2  90/09/18  07:56:50  rearl
 * Incorporated hash routines into TinyMUCK.
 *
 * Revision 1.1  90/09/16  22:45:01  rearl
 * Initial revision
 *
 *
 * Baseline code 90/09/12  code donated by Gazer, (dbriggs@nrao.edu)
 *                         loosely based on hashing code in K&R II.
 *
 */

#include "config.h"
#include "db.h"
#include "props.h"
#include "externs.h"

/* hash:  compute hash value for a string
 *
 * this particular hash function is fairly simple.  Note that it
 * throws away the information from the 0x20 bit in the character.
 * This means that upper and lower case letters will hash to the
 * same value.
 */

unsigned int
hash(register const char *s, unsigned int hash_size)
{
	unsigned int hashval;

	for (hashval = 0; *s != '\0'; s++) {
		hashval = (*s | 0x20) + 31 * hashval;
	}
	return hashval % hash_size;
}

/* find_hash:  lookup a name in a hash table
 *
 * returns NULL if not found, otherwise a pointer to the data union.
 */
hash_data *
find_hash(register const char *s, hash_tab * table, unsigned int size)
{
	register hash_entry *hp;

	for (hp = table[hash(s, size)]; hp != NULL; hp = hp->next) {
		if (string_compare(s, hp->name) == 0) {
			return &(hp->dat);	/* found */
		}
	}
	return NULL;				/* not found */
}

/* add_hash:  add a string to a hash table
 *
 * will supercede old values in the table, returns pointer to
 * the hash entry, or NULL on failure.
 *
 * NB: These hash routines assume that the names are static entities.
 * The hash entries store only a pointer to the names.  Be sure to
 * make a static copy of the name before adding it to the table.
 */
hash_entry *
add_hash(register const char *name, hash_data data, hash_tab * table, unsigned int size)
{
	register hash_entry *hp;
	unsigned int hashval;

	hashval = hash(name, size);

	/* an inline find_hash */
	for (hp = table[hashval]; hp != NULL; hp = hp->next) {
		if (string_compare(name, hp->name) == 0) {
			break;
		}
	}

	/* If not found, set up a new entry */
	if (hp == NULL) {
		hp = (hash_entry *) malloc(sizeof(hash_entry));
		if (hp == NULL) {
			perror("add_hash: out of memory!");
			abort();			/* can't allocate new entry -- die */
		}
		hp->next = table[hashval];
		table[hashval] = hp;
		hp->name = (char *) string_dup(name);	/* This might be wasteful. */
		if (hp->name == NULL) {
			perror("add_hash: out of memory!");
			abort();			/* can't allocate new entry -- die */
		}
	}
	/* One way or another, the pointer is now valid */
	hp->dat = data;
	return hp;
}

/* free_hash:  free a hash table entry
 *
 * frees the dynamically allocated hash table entry associated with
 * a name.  Returns 0 on success, or -1 if the name cannot be found.
 */
int
free_hash(register const char *name, hash_tab * table, unsigned int size)
{
	register hash_entry **lp, *hp;

	lp = &table[hash(name, size)];
	for (hp = *lp; hp != NULL; lp = &(hp->next), hp = hp->next) {
		if (string_compare(name, hp->name) == 0) {
			*lp = hp->next;		/* got it.  fix the pointers */
			free((void *) hp->name);
			free((void *) hp);
			return 0;
		}
	}
	return -1;					/* not found */
}

/* kill_hash:  kill an entire hash table, by freeing every entry */
void
kill_hash(hash_tab * table, unsigned int size, int freeptrs)
{
	register hash_entry *hp, *np;
	unsigned int i;

	for (i = 0; i < size; i++) {
		for (hp = table[i]; hp != NULL; hp = np) {
			np = hp->next;		/* Don't dereference the pointer after */
			free((void *) hp->name);
			if (freeptrs) {
				free((void *) hp->dat.pval);
			}
			free((void *) hp);	/* we've freed it! */
		}
		table[i] = NULL;
	}
}
