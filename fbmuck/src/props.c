/*
  AVL binary tree code by Lynx (or his instructor)

  modified for MUCK use by Sthiss
  Remodified by Foxen
*/

#include <string.h>
#include "config.h"
#include "params.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "externs.h"
#include "interface.h"

static PropPtr
find(char *key, PropPtr avl)
{
	int cmpval;

	while (avl) {
		cmpval = string_compare(key, PropName(avl));
		if (cmpval > 0) {
			avl = AVL_RT(avl);
		} else if (cmpval < 0) {
			avl = AVL_LF(avl);
		} else {
			break;
		}
	}
	return avl;
}

static int
height_of(PropPtr node)
{
	if (node)
		return node->height;
	else
		return 0;
}

static int
height_diff(PropPtr node)
{
	if (node)
		return (height_of(AVL_RT(node)) - height_of(AVL_LF(node)));
	else
		return 0;
}

static void
fixup_height(PropPtr node)
{
	if (node)
		node->height = 1 + MAX(height_of(AVL_LF(node)), height_of(AVL_RT(node)));
}

static PropPtr
rotate_left_single(PropPtr a)
{
	PropPtr b = AVL_RT(a);

	AVL_RT(a) = AVL_LF(b);
	AVL_LF(b) = a;

	fixup_height(a);
	fixup_height(b);

	return b;
}

static PropPtr
rotate_left_double(PropPtr a)
{
	PropPtr b = AVL_RT(a), c = AVL_LF(b);

	AVL_RT(a) = AVL_LF(c);
	AVL_LF(b) = AVL_RT(c);
	AVL_LF(c) = a;
	AVL_RT(c) = b;

	fixup_height(a);
	fixup_height(b);
	fixup_height(c);

	return c;
}

static PropPtr
rotate_right_single(PropPtr a)
{
	PropPtr b = AVL_LF(a);

	AVL_LF(a) = AVL_RT(b);
	AVL_RT(b) = a;

	fixup_height(a);
	fixup_height(b);

	return (b);
}

static PropPtr
rotate_right_double(PropPtr a)
{
	PropPtr b = AVL_LF(a), c = AVL_RT(b);

	AVL_LF(a) = AVL_RT(c);
	AVL_RT(b) = AVL_LF(c);
	AVL_RT(c) = a;
	AVL_LF(c) = b;

	fixup_height(a);
	fixup_height(b);
	fixup_height(c);

	return c;
}

static PropPtr
balance_node(PropPtr a)
{
	int dh = height_diff(a);

	if (abs(dh) < 2) {
		fixup_height(a);
	} else {
		if (dh == 2)
			if (height_diff(AVL_RT(a)) >= 0)
				a = rotate_left_single(a);
			else
				a = rotate_left_double(a);
		else if (height_diff(AVL_LF(a)) <= 0)
			a = rotate_right_single(a);
		else
			a = rotate_right_double(a);
	}
	return a;
}

PropPtr
alloc_propnode(const char *name)
{
	PropPtr new_node;
	int nlen = strlen(name);

	new_node = (PropPtr) malloc(sizeof(struct plist) + nlen);

	if (!new_node) {
		fprintf(stderr, "alloc_propnode(): Out of Memory!\n");
		abort();
	}

	AVL_LF(new_node) = NULL;
	AVL_RT(new_node) = NULL;
	new_node->height = 1;

	strcpyn(PropName(new_node), nlen+1, name);
	SetPFlagsRaw(new_node, PROP_DIRTYP);
	SetPDataVal(new_node, 0);
	SetPDir(new_node, NULL);
	return new_node;
}

void
free_propnode(PropPtr p)
{
	if (!(PropFlags(p) & PROP_ISUNLOADED)) {
		if (PropType(p) == PROP_STRTYP)
			free((void *) PropDataStr(p));
		if (PropType(p) == PROP_LOKTYP)
			free_boolexp(PropDataLok(p));
	}
	free(p);
}

void
clear_propnode(PropPtr p)
{
	if (!(PropFlags(p) & PROP_ISUNLOADED)) {
 	        if (PropType(p) == PROP_STRTYP) {
			free((void *) PropDataStr(p));
			PropDataStr(p) = NULL;
	        }
		if (PropType(p) == PROP_LOKTYP)
			free_boolexp(PropDataLok(p));
	}
	SetPDataVal(p, 0);
	SetPFlags(p, (PropFlags(p) & ~PROP_ISUNLOADED));
	SetPType(p, PROP_DIRTYP);
}


static PropPtr
insert(char *key, PropPtr * avl)
{
	PropPtr ret;
	register PropPtr p = *avl;
	register int cmp;
	static short balancep;

	if (p) {
		cmp = string_compare(key, PropName(p));
		if (cmp > 0) {
			ret = insert(key, &(AVL_RT(p)));
		} else if (cmp < 0) {
			ret = insert(key, &(AVL_LF(p)));
		} else {
			balancep = 0;
			return (p);
		}
		if (balancep) {
			*avl = balance_node(p);
		}
		return ret;
	} else {
		p = *avl = alloc_propnode(key);
		balancep = 1;
		return (p);
	}
}

static PropPtr
getmax(PropPtr avl)
{
	if (avl && AVL_RT(avl))
		return getmax(AVL_RT(avl));
	return avl;
}

static PropPtr
remove_propnode(char *key, PropPtr * root)
{
	PropPtr save;
	PropPtr tmp;
	PropPtr avl = *root;
	int cmpval;

	save = avl;
	if (avl) {
		cmpval = string_compare(key, PropName(avl));
		if (cmpval < 0) {
			save = remove_propnode(key, &AVL_LF(avl));
		} else if (cmpval > 0) {
			save = remove_propnode(key, &AVL_RT(avl));
		} else if (!(AVL_LF(avl))) {
			avl = AVL_RT(avl);
		} else if (!(AVL_RT(avl))) {
			avl = AVL_LF(avl);
		} else {
			tmp = remove_propnode(PropName(getmax(AVL_LF(avl))), &AVL_LF(avl));
			if (!tmp) {	/* this shouldn't be possible. */
				panic("remove_propnode() returned NULL !");
			}
			AVL_LF(tmp) = AVL_LF(avl);
			AVL_RT(tmp) = AVL_RT(avl);
			avl = tmp;
		}
		if (save) {
			AVL_LF(save) = NULL;
			AVL_RT(save) = NULL;
		}
		*root = balance_node(avl);
	}
	return save;
}


static PropPtr
delnode(char *key, PropPtr avl)
{
	PropPtr save;

	save = remove_propnode(key, &avl);
	if (save)
		free_propnode(save);
	return avl;
}


void
delete_proplist(PropPtr p)
{
	if (!p)
		return;
	delete_proplist(AVL_LF(p));
	delete_proplist(PropDir(p));
	delete_proplist(AVL_RT(p));
	free_propnode(p);
}

PropPtr
locate_prop(PropPtr list, char *name)
{
	return find(name, list);
}

PropPtr
new_prop(PropPtr * list, char *name)
{
	return insert(name, list);
}

PropPtr
delete_prop(PropPtr * list, char *name)
{
	*list = delnode(name, *list);
	return (*list);
}

PropPtr
first_node(PropPtr list)
{
	if (!list)
		return ((PropPtr) NULL);

	while (AVL_LF(list))
		list = AVL_LF(list);

	return (list);
}


PropPtr
next_node(PropPtr ptr, char *name)
{
	PropPtr from;
	int cmpval;

	if (!ptr)
		return NULL;
	if (!name || !*name)
		return (PropPtr) NULL;
	cmpval = string_compare(name, PropName(ptr));
	if (cmpval < 0) {
		from = next_node(AVL_LF(ptr), name);
		if (from)
			return from;
		return ptr;
	} else if (cmpval > 0) {
		return next_node(AVL_RT(ptr), name);
	} else if (AVL_RT(ptr)) {
		from = AVL_RT(ptr);
		while (AVL_LF(from))
			from = AVL_LF(from);
		return from;
	} else {
		return NULL;
	}
}


/* copies properties */
void
copy_proplist(dbref obj, PropPtr * nu, PropPtr old)
{
	PropPtr p;

	if (old) {
#ifdef DISKBASE
		propfetch(obj, old);
#endif
		p = new_prop(nu, PropName(old));
		SetPFlagsRaw(p, PropFlagsRaw(old));
		switch (PropType(old)) {
		case PROP_STRTYP:
			SetPDataStr(p, alloc_string(PropDataStr(old)));
			break;
		case PROP_LOKTYP:
			if (PropFlags(old) & PROP_ISUNLOADED) {
				SetPDataLok(p, TRUE_BOOLEXP);
				SetPFlags(p, (PropFlags(p) & ~PROP_ISUNLOADED));
			} else {
				SetPDataLok(p, copy_bool(PropDataLok(old)));
			}
			break;
		case PROP_DIRTYP:
			SetPDataVal(p, 0);
			break;
		case PROP_FLTTYP:
			SetPDataFVal(p, PropDataFVal(old));
			break;
		default:
			SetPDataVal(p, PropDataVal(old));
			break;
		}
		copy_proplist(obj, &PropDir(p), PropDir(old));
		copy_proplist(obj, &AVL_LF(p), AVL_LF(old));
		copy_proplist(obj, &AVL_RT(p), AVL_RT(old));
		/*
		   copy_proplist(obj, nu, AVL_LF(old));
		   copy_proplist(obj, nu, AVL_RT(old));
		 */
	}
}



long
size_proplist(PropPtr avl)
{
	long bytes = 0;

	if (!avl)
		return 0;
	bytes += sizeof(struct plist);

	bytes += strlen(PropName(avl));
	if (!(PropFlags(avl) & PROP_ISUNLOADED)) {
		switch (PropType(avl)) {
		case PROP_STRTYP:
			bytes += strlen(PropDataStr(avl)) + 1;
			break;
		case PROP_LOKTYP:
			bytes += size_boolexp(PropDataLok(avl));
			break;
		default:
			break;
		}
	}
	bytes += size_proplist(AVL_LF(avl));
	bytes += size_proplist(AVL_RT(avl));
	bytes += size_proplist(PropDir(avl));
	return bytes;
}



int
Prop_Check(const char *name, const char what)
{
	if (*name == what)
		return (1);
	while ((name = index(name, PROPDIR_DELIMITER))) {
		if (name[1] == what)
			return (1);
		name++;
	}
	return (0);
}
