#include "config.h"
#include "db.h"
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
    return NULL;		/* not found */
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
	    abort();		/* can't allocate new entry -- die */
	}
	hp->next = table[hashval];
	table[hashval] = hp;
	hp->name = (char *) string_dup(name);	/* This might be wasteful. */
	if (hp->name == NULL) {
	    perror("add_hash: out of memory!");
	    abort();		/* can't allocate new entry -- die */
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
	    *lp = hp->next;	/* got it.  fix the pointers */
	    free((void *) hp->name);
	    free((void *) hp);
	    return 0;
	}
    }
    return -1;			/* not found */
}

/* kill_hash:  kill an entire hash table, by freeing every entry */
void
kill_hash(hash_tab * table, unsigned int size, int freeptrs)
{
    register hash_entry *hp, *np;
    unsigned int i;

    for (i = 0; i < size; i++) {
	for (hp = table[i]; hp != NULL; hp = np) {
	    np = hp->next;	/* Don't dereference the pointer after */
	    free((void *) hp->name);
	    if (freeptrs) {
		free((void *) hp->dat.pval);
	    }
	    free((void *) hp);	/* we've freed it! */
	}
	table[i] = NULL;
    }
}
