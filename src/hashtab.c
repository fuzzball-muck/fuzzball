/** @file hashtab.c
 *
 * Source for the implementation of hash tables.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "fbstrings.h"
#include "hashtab.h"

/**
 * Compute hash value for a string (case-insensitive)
 *
 * This particular hash function is fairly simple.  Note that it
 * throws away the information from the 0x20 bit in the character.
 * This means that upper and lower case letters will hash to the
 * same value.
 *
 * Hash size is the page size of the hash.  The resulting number
 * returned will be between 0 and hash_size-1
 *
 * @param s the string to hash
 * @param hash_size the hash page size
 * @return the hash value as an unsigned integer
 */
unsigned int
hash(register const char *s, unsigned int hash_size)
{
    unsigned int hashval;

    for (hashval = 0; *s != '\0'; s++) {
        hashval = ((unsigned int)*s | 0x20) + 31 * hashval;
    }

    return hashval % hash_size;
}

/**
 * Lookup a name in a hash table
 *
 * Returns NULL if not found, otherwise a pointer to the data union.
 *
 * @param s the string to look up
 * @param table the table to look the string up in
 * @param size the hash page size
 * @return NULL if not found, otherwise a pointer to the data union
 */
hash_data *
find_hash(register const char *s, hash_tab * table, unsigned int size)
{
    for (register hash_entry *hp = table[hash(s, size)]; hp != NULL;
         hp = hp->next) {
        if (strcasecmp(s, hp->name) == 0) {
            return &(hp->dat); /* found */
        }
    }

    return NULL; /* not found */
}

/**
 * Add a string to a hash table
 *
 * Will supercede old values in the table, returns pointer to
 * the hash entry, or NULL on failure.
 *
 * The name is copied when a new entry is added.
 *
 * @param name the name of the entry to add
 * @param data the data to store
 * @param table the hash stable to store it in
 * @param size the page size of the hash table 
 */
hash_entry *
add_hash(register const char *name, hash_data data, hash_tab * table,
         unsigned int size)
{
    register hash_entry *hp;
    unsigned int hashval;

    hashval = hash(name, size);

    /* an inline find_hash */
    for (hp = table[hashval]; hp != NULL; hp = hp->next) {
        if (strcasecmp(name, hp->name) == 0) {
            break;
        }
    }

    /* If not found, set up a new entry */
    if (hp == NULL) {
        hp = malloc(sizeof(hash_entry));

        if (hp == NULL) {
            perror("add_hash: out of memory!");
            abort(); /* can't allocate new entry -- die */
        }

        hp->next = table[hashval];
        table[hashval] = hp;

        /*
         * @TODO The comment was WRONG and implied the hash name isn't
         *       copied, when in fact, it is.  We need to check where
         *       add_hash is being used and make sure we're not leaking
         *       memory with a double string-dup.
         */
        hp->name = strdup(name); /* This might be wasteful. */

        if (hp->name == NULL) {
            perror("add_hash: out of memory!");
            abort(); /* can't allocate new entry -- die */
        }
    }

    /* One way or another, the pointer is now valid */
    hp->dat = data;

    return hp;
}

/**
 * Free a hash table entry associated with a name
 *
 * Frees the dynamically allocated hash table entry associated with
 * a name.  Returns 0 on success, or -1 if the name cannot be found.
 * Frees the entry name, but does not free the data associated with
 * the entry.
 *
 * @TODO Would it make sense for this to return the data entry instead
 *       so that it can be deleted by the caller if necessary?  This just
 *       seems like its asking for a memory leak.
 *
 * @param name the name of the hash entry to free
 * @param table the table to delete from
 * @param size the page size
 * @return 0 on success, -1 if 'name' was not found.
 */
int
free_hash(register const char *name, hash_tab * table, unsigned int size)
{
    register hash_entry **lp;

    lp = &table[hash(name, size)];

    for (register hash_entry *hp = *lp;
         hp != NULL; lp = &(hp->next), hp = hp->next) {
        if (strcasecmp(name, hp->name) == 0) {
            *lp = hp->next; /* got it.  fix the pointers */
            free((void *) hp->name);
            free(hp);
            return 0;
        }
    }

    return -1; /* not found */
}

/**
 * Kill an entire hash table, by freeing every entry
 *
 * This will optionally also delete the data pointers if freeptrs is
 * true.
 *
 * @param table the hash table to operate on
 * @param size the page size
 * @param freeptrs boolean if true, free the data union value.
 */
void
kill_hash(hash_tab * table, unsigned int size, int freeptrs)
{
    register hash_entry *np;

    for (unsigned int i = 0; i < size; i++) {
        for (register hash_entry *hp = table[i]; hp != NULL; hp = np) {
            np = hp->next; /* Don't dereference the pointer after */
            free((void *) hp->name);

            if (freeptrs) {
                free(hp->dat.pval);
            }

            free(hp);
        }

        table[i] = NULL;
    }
}
