#ifndef PREDICATES_H
#define PREDICATES_H

#include "config.h"
#include "db.h"

int can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg);
int can_link(dbref who, dbref what);
int can_link_to(dbref who, object_flag_type what_type, dbref where);
int can_move(int descr, dbref player, const char *direction, int lev);
int could_doit(int descr, dbref player, dbref thing);
int exit_loop_check(dbref source, dbref dest);
int parent_loop_check(dbref source, dbref dest);

#endif /* !PREDICATES_H */
