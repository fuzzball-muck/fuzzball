#ifndef _HASHTAB_H
#define _HASHTAB_H

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

hash_entry *add_hash(const char *name, hash_data data, hash_tab * table, unsigned size);
hash_data *find_hash(const char *s, hash_tab * table, unsigned size);
int free_hash(const char *name, hash_tab * table, unsigned size);
unsigned int hash(const char *s, unsigned int hash_size);
void kill_hash(hash_tab * table, unsigned size, int freeptrs);

#endif				/* _HASHTAB_H */
