#ifndef _PROPS_H
#define _PROPS_H

union pdata_u {
    char *str;
    struct boolexp *lok;
    int val;
    double fval;
    dbref ref;
    long pos;
};

/* data struct for setting data. */
struct pdata {
    unsigned short flags;
    union pdata_u data;
};
typedef struct pdata PData;


/* Property struct */
struct plist {
    unsigned short flags;
    short height;		/* satisfy the avl monster.  */
    union pdata_u data;
    struct plist *left, *right, *dir;
    char key[1];
};

/* property node pointer type */
typedef struct plist *PropPtr;

/* propload queue types */
#define PROPS_UNLOADED 0x0
#define PROPS_LOADED   0x1
#define PROPS_PRIORITY 0x2
#define PROPS_CHANGED  0x3


/* property value types */
#define PROP_DIRTYP   0x0
#define PROP_STRTYP   0x2
#define PROP_INTTYP   0x3
#define PROP_LOKTYP   0x4
#define PROP_REFTYP   0x5
#define PROP_FLTTYP   0x6
#define PROP_TYPMASK  0x7

/* Internally used prop flags.  Never stored on disk. */
#define PROP_ISUNLOADED  0x0200
#define PROP_TOUCHED     0x0400
#define PROP_DIRUNLOADED 0x0800

/* Blessed props evaluate with wizbit MPI perms. */
#define PROP_BLESSED     0x1000


/* Macros */
#define AVL_LF(x) (x)->left
#define AVL_RT(x) (x)->right

#define SetPDir(x,y) {(x)->dir = y;}
#define PropDir(x) ((x)->dir)

#define SetPDataUnion(x,z) {(x)->data = z;}
#define SetPDataStr(x,z) {(x)->data.str = z;}
#define SetPDataVal(x,z) {(x)->data.val = z;}
#define SetPDataRef(x,z) {(x)->data.ref = z;}
#define SetPDataLok(x,z) {(x)->data.lok = z;}
#define SetPDataFVal(x,z) {(x)->data.fval = z;}

#define PropDataStr(x) ((x)->data.str)
#define PropDataVal(x) ((x)->data.val)
#define PropDataRef(x) ((x)->data.ref)
#define PropDataLok(x) ((x)->data.lok)
#define PropDataFVal(x) ((x)->data.fval)

#define PropName(x) ((x)->key)

#define SetPFlags(x,y) {(x)->flags = ((x)->flags & PROP_TYPMASK) | (short)y;}
#define PropFlags(x) ((x)->flags & ~PROP_TYPMASK)

#define SetPType(x,y) {(x)->flags = ((x)->flags & ~PROP_TYPMASK) | (short)y;}
#define PropType(x) ((x)->flags & PROP_TYPMASK)

#define SetPFlagsRaw(x,y) {(x)->flags = (short)y;}
#define PropFlagsRaw(x) ((x)->flags)

#define Prop_Blessed(obj,propname) (get_property_flags(obj, propname) & PROP_BLESSED)

/* property access macros */
#define Prop_ReadOnly(name) \
    (Prop_Check(name, PROP_RDONLY) || Prop_Check(name, PROP_RDONLY2))
#define Prop_Private(name) Prop_Check(name, PROP_PRIVATE)
#define Prop_SeeOnly(name) Prop_Check(name, PROP_SEEONLY)
#define Prop_Hidden(name) Prop_Check(name, PROP_HIDDEN)
#define Prop_System(name) is_prop_prefix(name, "@__sys__")


/* Routines as they need to be:

     PropPtr locate_prop(PropPtr list, char *name)
       if list is NULL, return NULL.

     PropPtr new_prop(PropPtr *list, char *name)
       if *list is NULL, create a new propdir, then insert the prop

     PropPtr delete_prop(PropPtr *list, char *name)
       when last prop in dir is deleted, destroy propdir & change *list to NULL

     PropPtr first_node(PropPtr list)
       if list is NULL, return NULL

     PropPtr next_node(PropPtr list, char *name)
       if list is NULL, return NULL

 */

extern PropPtr alloc_propnode(const char *name);
extern void free_propnode(PropPtr node);
extern PropPtr first_node(PropPtr p);
extern PropPtr next_node(PropPtr p, char *c);
extern void putprop(FILE * f, PropPtr p);
extern int Prop_Check(const char *name, const char what);
extern PropPtr locate_prop(PropPtr l, char *path);
extern PropPtr new_prop(PropPtr * l, char *path);
extern PropPtr delete_prop(PropPtr * list, char *name);


extern void set_property(dbref player, const char *pname, PData * dat, int sync);
extern void add_property(dbref player, const char *type, const char *strval, int value);

extern void remove_property_list(dbref player, int all);
extern void remove_property(dbref player, const char *type, int sync);

extern int has_property(int descr, dbref player, dbref what, const char *type,
			const char *strval, int value);
extern int has_property_strict(int descr, dbref player, dbref what, const char *type,
			       const char *strval, int value);

extern const char *get_property_class(dbref player, const char *type);
extern double get_property_fvalue(dbref player, const char *type);
extern int get_property_value(dbref player, const char *type);
extern struct boolexp *get_property_lock(dbref player, const char *type);
extern dbref get_property_dbref(dbref player, const char *pname);
extern const char *envpropstr(dbref * where, const char *propname);
extern PropPtr get_property(dbref player, const char *type);
extern PropPtr envprop(dbref * where, const char *propname, int typ);
extern int get_property_flags(dbref player, const char *type);
extern void set_property_flags(dbref player, const char *type, int flags);
extern void clear_property_flags(dbref player, const char *type, int flags);

extern int genderof(int descr, dbref player);
extern struct plist *copy_prop(dbref old);

extern PropPtr first_prop(dbref player, const char *dir, PropPtr * list, char *name,
			  int maxlen);
extern PropPtr next_prop(PropPtr list, PropPtr prop, char *name, int maxlen);
extern char *next_prop_name(dbref player, char *outbuf, int outbuflen, char *name);

extern int is_propdir(dbref player, const char *dir);

extern void delete_proplist(PropPtr p);
extern void set_property_nofetch(dbref player, const char *pname, PData * dat, int sync);
extern void add_prop_nofetch(dbref player, const char *type, const char *strval, int value);
extern void remove_property_nofetch(dbref player, const char *type, int sync);
extern PropPtr first_prop_nofetch(dbref player, const char *dir, PropPtr * list, char *name,
				  int maxlen);

extern PropPtr propdir_new_elem(PropPtr * root, char *path);
extern PropPtr propdir_delete_elem(PropPtr root, char *path);
extern PropPtr propdir_get_elem(PropPtr root, char *path);
extern PropPtr propdir_first_elem(PropPtr root, char *path);
extern PropPtr propdir_next_elem(PropPtr root, char *path);
extern int propdir_check(PropPtr root, char *path);
extern const char *propdir_name(const char *name);
extern const char *propdir_unloaded(PropPtr root, const char *path);

extern void db_putprop(FILE * f, const char *dir, PropPtr p);
extern int db_get_single_prop(FILE * f, dbref obj, long pos, PropPtr pnode, const char *pdir);
extern void db_getprops(FILE * f, dbref obj, const char *pdir);
extern void db_dump_props(FILE * f, dbref obj);

extern void reflist_add(dbref obj, const char *propname, dbref toadd);
extern void reflist_del(dbref obj, const char *propname, dbref todel);
extern int reflist_find(dbref obj, const char *propname, dbref tofind);

#endif				/* _PROPS_H */
