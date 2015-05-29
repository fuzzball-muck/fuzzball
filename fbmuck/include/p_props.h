#ifndef _P_PROPS_H
#define _P_PROPS_H

extern void prim_getpropval(PRIM_PROTOTYPE);
extern void prim_getpropfval(PRIM_PROTOTYPE);
extern void prim_getpropstr(PRIM_PROTOTYPE);
extern void prim_remove_prop(PRIM_PROTOTYPE);
extern void prim_envprop(PRIM_PROTOTYPE);
extern void prim_envpropstr(PRIM_PROTOTYPE);
extern void prim_addprop(PRIM_PROTOTYPE);
extern void prim_nextprop(PRIM_PROTOTYPE);
extern void prim_propdirp(PRIM_PROTOTYPE);
extern void prim_parseprop(PRIM_PROTOTYPE);
extern void prim_getprop(PRIM_PROTOTYPE);
extern void prim_setprop(PRIM_PROTOTYPE);
extern void prim_blessprop(PRIM_PROTOTYPE);
extern void prim_unblessprop(PRIM_PROTOTYPE);
extern void prim_array_filter_prop(PRIM_PROTOTYPE);
extern void prim_reflist_find(PRIM_PROTOTYPE);
extern void prim_reflist_add(PRIM_PROTOTYPE);
extern void prim_reflist_del(PRIM_PROTOTYPE);
extern void prim_blessedp(PRIM_PROTOTYPE);
extern void prim_parsepropex(PRIM_PROTOTYPE);

#define PRIMS_PROPS_FUNCS prim_getpropval, prim_getpropstr, prim_remove_prop, \
   prim_envprop, prim_envpropstr, prim_addprop, prim_nextprop, prim_propdirp, \
   prim_parseprop, prim_getprop, prim_setprop, prim_getpropfval, \
   prim_blessprop, prim_unblessprop, prim_array_filter_prop, \
   prim_reflist_find, prim_reflist_add, prim_reflist_del, prim_blessedp, \
   prim_parsepropex

#define PRIMS_PROPS_NAMES "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP",  \
    "ENVPROP", "ENVPROPSTR", "ADDPROP", "NEXTPROP", "PROPDIR?", \
    "PARSEPROP", "GETPROP", "SETPROP", "GETPROPFVAL", \
	"BLESSPROP", "UNBLESSPROP", "ARRAY_FILTER_PROP", \
	"REFLIST_FIND", "REFLIST_ADD", "REFLIST_DEL", "BLESSED?", \
	"PARSEPROPEX"

#define PRIMS_PROPS_CNT 20

#endif /* _P_PROPS_H */
