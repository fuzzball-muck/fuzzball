#ifndef P_PROPS_H
#define P_PROPS_H

#include "interp.h"

int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);
int prop_write_perms(dbref player, dbref obj, const char *name, int mlev);

void prim_getpropval(PRIM_PROTOTYPE);
void prim_getpropfval(PRIM_PROTOTYPE);
void prim_getpropstr(PRIM_PROTOTYPE);
void prim_remove_prop(PRIM_PROTOTYPE);
void prim_envprop(PRIM_PROTOTYPE);
void prim_envpropstr(PRIM_PROTOTYPE);
void prim_addprop(PRIM_PROTOTYPE);
void prim_nextprop(PRIM_PROTOTYPE);
void prim_propdirp(PRIM_PROTOTYPE);
void prim_parseprop(PRIM_PROTOTYPE);
void prim_getprop(PRIM_PROTOTYPE);
void prim_setprop(PRIM_PROTOTYPE);
void prim_blessprop(PRIM_PROTOTYPE);
void prim_unblessprop(PRIM_PROTOTYPE);
void prim_array_filter_prop(PRIM_PROTOTYPE);
void prim_reflist_find(PRIM_PROTOTYPE);
void prim_reflist_add(PRIM_PROTOTYPE);
void prim_reflist_del(PRIM_PROTOTYPE);
void prim_blessedp(PRIM_PROTOTYPE);
void prim_parsepropex(PRIM_PROTOTYPE);
void prim_parsempi(PRIM_PROTOTYPE);
void prim_parsempiblessed(PRIM_PROTOTYPE);
void prim_prop_name_okp(PRIM_PROTOTYPE);

#define PRIMS_PROPS_FUNCS prim_getpropval, prim_getpropstr, prim_remove_prop, \
   prim_envprop, prim_envpropstr, prim_addprop, prim_nextprop, prim_propdirp, \
   prim_parseprop, prim_getprop, prim_setprop, prim_getpropfval, \
   prim_blessprop, prim_unblessprop, prim_array_filter_prop, \
   prim_reflist_find, prim_reflist_add, prim_reflist_del, prim_blessedp, \
   prim_parsepropex, prim_parsempi, prim_parsempiblessed, prim_prop_name_okp

#define PRIMS_PROPS_NAMES "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP",  \
    "ENVPROP", "ENVPROPSTR", "ADDPROP", "NEXTPROP", "PROPDIR?", \
    "PARSEPROP", "GETPROP", "SETPROP", "GETPROPFVAL", \
	"BLESSPROP", "UNBLESSPROP", "ARRAY_FILTER_PROP", \
	"REFLIST_FIND", "REFLIST_ADD", "REFLIST_DEL", "BLESSED?", \
	"PARSEPROPEX", "PARSEMPI", "PARSEMPIBLESSED", "PROP-NAME-OK?"

#endif /* !P_PROPS_H */
