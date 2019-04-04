#ifndef P_ARRAY_H
#define P_ARRAY_H

#include "interp.h"

void prim_array_make(PRIM_PROTOTYPE);
void prim_array_make_dict(PRIM_PROTOTYPE);
void prim_array_explode(PRIM_PROTOTYPE);
void prim_array_vals(PRIM_PROTOTYPE);
void prim_array_keys(PRIM_PROTOTYPE);
void prim_array_count(PRIM_PROTOTYPE);
void prim_array_first(PRIM_PROTOTYPE);
void prim_array_last(PRIM_PROTOTYPE);
void prim_array_next(PRIM_PROTOTYPE);
void prim_array_prev(PRIM_PROTOTYPE);
void prim_array_getitem(PRIM_PROTOTYPE);
void prim_array_setitem(PRIM_PROTOTYPE);
void prim_array_appenditem(PRIM_PROTOTYPE);
void prim_array_insertitem(PRIM_PROTOTYPE);
void prim_array_getrange(PRIM_PROTOTYPE);
void prim_array_setrange(PRIM_PROTOTYPE);
void prim_array_insertrange(PRIM_PROTOTYPE);
void prim_array_delitem(PRIM_PROTOTYPE);
void prim_array_delrange(PRIM_PROTOTYPE);
void prim_array_compare(PRIM_PROTOTYPE);
void prim_array_findval(PRIM_PROTOTYPE);
void prim_array_matchval(PRIM_PROTOTYPE);
void prim_array_matchkey(PRIM_PROTOTYPE);
void prim_array_extract(PRIM_PROTOTYPE);
void prim_array_excludeval(PRIM_PROTOTYPE);
void prim_array_sort(PRIM_PROTOTYPE);
void prim_array_sort_indexed(PRIM_PROTOTYPE);
void prim_array_join(PRIM_PROTOTYPE);
void prim_array_interpret(PRIM_PROTOTYPE);
void prim_array_cut(PRIM_PROTOTYPE);

void prim_array_n_union(PRIM_PROTOTYPE);
void prim_array_n_intersection(PRIM_PROTOTYPE);
void prim_array_n_difference(PRIM_PROTOTYPE);

void prim_array_notify(PRIM_PROTOTYPE);
void prim_array_reverse(PRIM_PROTOTYPE);

void prim_array_get_propdirs(PRIM_PROTOTYPE);
void prim_array_get_propvals(PRIM_PROTOTYPE);
void prim_array_get_proplist(PRIM_PROTOTYPE);
void prim_array_put_propvals(PRIM_PROTOTYPE);
void prim_array_put_proplist(PRIM_PROTOTYPE);

void prim_array_get_reflist(PRIM_PROTOTYPE);
void prim_array_put_reflist(PRIM_PROTOTYPE);

void prim_array_pin(PRIM_PROTOTYPE);
void prim_array_unpin(PRIM_PROTOTYPE);
void prim_array_default_pinning(PRIM_PROTOTYPE);

void prim_array_get_ignorelist(PRIM_PROTOTYPE);

void prim_array_nested_get(PRIM_PROTOTYPE);
void prim_array_nested_set(PRIM_PROTOTYPE);
void prim_array_nested_del(PRIM_PROTOTYPE);

void prim_array_filter_flags(PRIM_PROTOTYPE);
void prim_array_filter_lock(PRIM_PROTOTYPE);
void prim_array_notify_secure(PRIM_PROTOTYPE);

#define PRIMS_ARRAY_FUNCS prim_array_make, prim_array_make_dict, \
        prim_array_explode, prim_array_vals, prim_array_keys, \
        prim_array_first, prim_array_last, prim_array_next, prim_array_prev, \
        prim_array_count, prim_array_getitem, prim_array_setitem, \
        prim_array_insertitem, prim_array_getrange, prim_array_setrange, \
        prim_array_insertrange, prim_array_delitem, prim_array_delrange, \
        prim_array_n_union, prim_array_n_intersection, \
        prim_array_n_difference, prim_array_notify, prim_array_reverse, \
	prim_array_get_propvals, prim_array_get_propdirs, \
	prim_array_get_proplist, prim_array_put_propvals, \
	prim_array_put_proplist, prim_array_get_reflist, \
	prim_array_put_reflist, prim_array_appenditem, prim_array_findval, \
	prim_array_excludeval, prim_array_sort, prim_array_matchval, \
	prim_array_matchkey, prim_array_extract, prim_array_join, \
	prim_array_cut, prim_array_compare, prim_array_sort_indexed, \
	prim_array_pin, prim_array_unpin, prim_array_get_ignorelist, \
	prim_array_nested_get, prim_array_nested_set, prim_array_nested_del, \
	prim_array_filter_flags, prim_array_interpret, prim_array_notify_secure, \
	prim_array_default_pinning, prim_array_filter_lock

#define PRIMS_ARRAY_NAMES "ARRAY_MAKE", "ARRAY_MAKE_DICT", \
        "ARRAY_EXPLODE", "ARRAY_VALS", "ARRAY_KEYS", \
        "ARRAY_FIRST", "ARRAY_LAST", "ARRAY_NEXT", "ARRAY_PREV", \
        "ARRAY_COUNT", "ARRAY_GETITEM", "ARRAY_SETITEM", \
        "ARRAY_INSERTITEM", "ARRAY_GETRANGE", "ARRAY_SETRANGE", \
        "ARRAY_INSERTRANGE", "ARRAY_DELITEM", "ARRAY_DELRANGE", \
        "ARRAY_NUNION", "ARRAY_NINTERSECT", \
        "ARRAY_NDIFF", "ARRAY_NOTIFY", "ARRAY_REVERSE", \
	"ARRAY_GET_PROPVALS", "ARRAY_GET_PROPDIRS", \
	"ARRAY_GET_PROPLIST", "ARRAY_PUT_PROPVALS", \
	"ARRAY_PUT_PROPLIST", "ARRAY_GET_REFLIST", \
	"ARRAY_PUT_REFLIST", "ARRAY_APPENDITEM", "ARRAY_FINDVAL", \
	"ARRAY_EXCLUDEVAL", "ARRAY_SORT", "ARRAY_MATCHVAL", \
	"ARRAY_MATCHKEY", "ARRAY_EXTRACT", "ARRAY_JOIN", \
	"ARRAY_CUT", "ARRAY_COMPARE", "ARRAY_SORT_INDEXED", \
	"ARRAY_PIN", "ARRAY_UNPIN", "ARRAY_GET_IGNORELIST", \
	"ARRAY_NESTED_GET", "ARRAY_NESTED_SET", "ARRAY_NESTED_DEL", \
	"ARRAY_FILTER_FLAGS", "ARRAY_INTERPRET", "ARRAY_NOTIFY_SECURE", \
	"ARRAY_DEFAULT_PINNING", "ARRAY_FILTER_LOCK"

#endif /* !P_ARRAY_H */
