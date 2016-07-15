#ifndef _BOOLEXP_H
#define _BOOLEXP_H

typedef char boolexp_type;

#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_PROP 4

struct boolexp {
    boolexp_type type;
    struct boolexp *sub1;
    struct boolexp *sub2;
    dbref thing;
    struct plist *prop_check;
};

#define TRUE_BOOLEXP ((struct boolexp *) 0)

struct boolexp *copy_bool(struct boolexp *old);
int eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing);
void free_boolexp(struct boolexp *b);
struct boolexp *parse_boolexp(int descr, dbref player, const char *string, int dbloadp);
long size_boolexp(struct boolexp *b);
int test_lock(int descr, dbref player, dbref thing, const char *lockprop);
int test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop);
const char *unparse_boolexp(dbref player, struct boolexp *b, int fullname);

#endif				/* _BOOLEXP_H */
