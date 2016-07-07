#ifndef _BOOLEXP_H
#define _BOOLEXP_H

#include "config.h"

struct boolexp *copy_bool(struct boolexp *old);
int eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing);
void free_boolexp(struct boolexp *b);
struct boolexp *parse_boolexp(int descr, dbref player, const char *string, int dbloadp);
long size_boolexp(struct boolexp *b);
int test_lock(int descr, dbref player, dbref thing, const char *lockprop);
int test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop);
const char *unparse_boolexp(dbref player, struct boolexp *b, int fullname);

#endif				/* _BOOLEXP_H */
