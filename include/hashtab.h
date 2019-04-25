/** @file hashtab.h
 *
 * Header for the implementation of hash tables.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef HASHTAB_H
#define HASHTAB_H

#include "config.h"

/* Possible data types that may be stored in a hash table */
union u_hash_data {
    int ival;                   /* Store compiler tokens here */
    dbref dbval;                /* Player hashing will want this */
    void *pval;                 /* compiler $define strings use this */
};

/* The actual hash entry for each item */
struct t_hash_entry {
    struct t_hash_entry *next;  /* Pointer for conflict resolution */
    const char *name;           /* The name of the item */
    union u_hash_data dat;      /* Data value for item */
};

typedef union u_hash_data hash_data;
typedef struct t_hash_entry hash_entry;
typedef hash_entry *hash_tab;

#define PLAYER_HASH_SIZE   (1024)       /* Table for player lookups */
#define COMP_HASH_SIZE     (256)        /* Table for compiler keywords */
#define DEFHASHSIZE        (256)        /* Table for compiler $defines */

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
hash_entry *add_hash(const char *name, hash_data data, hash_tab * table,
                     unsigned int size);

/*
 * @TODO All these functions take a size parameter.  Wouldn't it make more
 *       sense to have a hash header struct that keeps track of that?  Its
 *       Maybe its not worth the effort, but it seems like something that
 *       would be easy to break if we're not careful.
 */

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
hash_data *find_hash(const char *s, hash_tab * table, unsigned size);

/**
 * Free a hash table entry associated with a name
 *
 * Frees the dynamically allocated hash table entry associated with
 * a name.  Returns 0 on success, or -1 if the name cannot be found.
 * Frees the entry name, but does not free the data associated with
 * the entry.
 *
 * @param name the name of the hash entry to free
 * @param table the table to delete from
 * @param size the page size
 * @return 0 on success, -1 if 'name' was not found.
 */
int free_hash(const char *name, hash_tab * table, unsigned size);

/**
 * Compute hash value for a string (case-insensitive)
 *
 * This particular hash function is fairly simple.  Note that it
 * throws away the information from the 0x20 bit in the character.
 * This means that upper and lower case letters will hash to the
 * same value.
 *
 * @param s the string to hash
 * @param hash_size the hash value will be between 0 and hash_size-1
 * @return the hash value as an integer
 */
unsigned int hash(const char *s, unsigned int hash_size);

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
void kill_hash(hash_tab * table, unsigned size, int freeptrs);

#endif /* !HASHTAB_H */
