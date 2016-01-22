/* do_parse_mesg()  Parses a message string, expanding {prop}s, {list}s, etc.
 * Written 4/93 - 5/93 by Foxen
 *
 * Args:
 *   int descr             Descriptor that triggered this.
 *   dbref player          Player triggering this.
 *   dbref what            Object that mesg is on.
 *   dbref perms           Object permissions are checked from.
 *   const char *inbuf     A pointer to the input raw mesg string.
 *   char *abuf            Argument string for {&how}.
 *   char *outbuf          Buffer to return results in.
 *   int mesgtyp           1 for personal messages, 0 for omessages.
 *
 *   extern char *match_args     Arg string for {&args}
 *   extern char *match_cmdname  Arg string for {&cmd}
 *
 * Returns a pointer to output string.  Usually in outbuf.
 */


#include "config.h"
#include <math.h>
#include <ctype.h>
#include "params.h"
#include "db.h"
#include "tune.h"

#include "mpi.h"
#include "externs.h"
#include "props.h"
#include "match.h"
#include "msgparse.h"
#include "mfun.h"

time_t mpi_prof_start_time;

int
safeblessprop(dbref obj, dbref perms, char *buf, int mesgtyp, int set_p)
{
	char *ptr;

	if (!buf)
		return 0;
	while (*buf == PROPDIR_DELIMITER)
		buf++;
	if (!*buf)
		return 0;

	/* disallow CR's and :'s in prop names. */
	for (ptr = buf; *ptr; ptr++)
		if (*ptr == '\r' || *ptr == PROP_DELIMITER)
			return 0;

	if (!(mesgtyp & MPI_ISBLESSED)) {
		return 0;
	}
	if (set_p) {
		set_property_flags(obj, buf, PROP_BLESSED);
	} else {
		clear_property_flags(obj, buf, PROP_BLESSED);
	}

	return 1;
}


int
safeputprop(dbref obj, dbref perms, char *buf, char *val, int mesgtyp)
{
	char *ptr;

	if (!buf)
		return 0;
	while (*buf == PROPDIR_DELIMITER)
		buf++;
	if (!*buf)
		return 0;

	/* disallow CR's and :'s in prop names. */
	for (ptr = buf; *ptr; ptr++)
		if (*ptr == '\r' || *ptr == PROP_DELIMITER)
			return 0;

	if (Prop_System(buf))
		return 0;
	
	if (!(mesgtyp & MPI_ISBLESSED)) {
		if (Prop_Hidden(buf))
			return 0;
		if (Prop_SeeOnly(buf))
			return 0;
		if (string_prefix(buf, "_msgmacs/"))
			return 0;
	}
	if (val == NULL) {
		remove_property(obj, buf, 0);
	} else {
		add_property(obj, buf, val, 0);
	}
	return 1;
}


const char *
safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int* blessed)
{
	const char *ptr;
	char bbuf[BUFFER_LEN];
	static char vl[32];

	*blessed = 0;

	if (!inbuf) {
		notify_nolisten(player, "PropFetch: Propname required.", 1);
		return NULL;
	}
	while (*inbuf == PROPDIR_DELIMITER)
		inbuf++;
	if (!*inbuf) {
		notify_nolisten(player, "PropFetch: Propname required.", 1);
		return NULL;
	}
	strcpyn(bbuf, sizeof(bbuf), inbuf);

	if (Prop_System(bbuf))
	{
		notify_nolisten(player, "PropFetch: Permission denied.", 1);
		return NULL;
	}

	if (!(mesgtyp & MPI_ISBLESSED)) {
		if (Prop_Hidden(bbuf)) {
			notify_nolisten(player, "PropFetch: Permission denied.", 1);
			return NULL;
		}
		if (Prop_Private(bbuf) && OWNER(perms) != OWNER(what)) {
			notify_nolisten(player, "PropFetch: Permission denied.", 1);
			return NULL;
		}
	}

	ptr = get_property_class(what, bbuf);
	if (!ptr) {
		int i;

		i = get_property_value(what, bbuf);
		if (!i) {
			dbref dd;

			dd = get_property_dbref(what, bbuf);
			if (dd == NOTHING) {
				*vl = '\0';
				ptr = vl;
				return ptr;
			} else {
				snprintf(vl, sizeof(vl), "#%d", dd);
				ptr = vl;
			}
		} else {
			snprintf(vl, sizeof(vl), "%d", i);
			ptr = vl;
		}
	}

	if (ptr) {
		if (Prop_Blessed(what, bbuf)) {
			*blessed = 1;
		}
	}
	return ptr;
}


const char *
safegetprop_limited(dbref player, dbref what, dbref whom, dbref perms, const char *inbuf, int mesgtyp, int* blessed)
{
	const char *ptr;

	while (what != NOTHING) {
		ptr = safegetprop_strict(player, what, perms, inbuf, mesgtyp, blessed);
		if (!ptr)
			return ptr;
		if (*ptr) {
			if (OWNER(what) == whom || *blessed) {
				return ptr;
			}
		}
		what = getparent(what);
	}
	return "";
}


const char *
safegetprop(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int* blessed)
{
	const char *ptr;

	while (what != NOTHING) {
		ptr = safegetprop_strict(player, what, perms, inbuf, mesgtyp, blessed);
		if (!ptr || *ptr)
			return ptr;
		what = getparent(what);
	}
	return "";
}


char *
stripspaces(char *buf, int buflen, char *in)
{
	char *ptr;

	for (ptr = in; *ptr == ' '; ptr++) ;
	strcpyn(buf, buflen, ptr);
	ptr = strlen(buf) + buf - 1;
	while (*ptr == ' ' && ptr > buf)
		*(ptr--) = '\0';
	return buf;
}


char *
string_substitute(const char *str, const char *oldstr, const char *newstr,
                  char *buf, int maxlen)
{
	const char *ptr = str;
	char *ptr2 = buf;
	const char *ptr3;
	int len = strlen(oldstr);
	int clen = 0;

	if (len == 0) {
		strcpyn(buf, maxlen, str);
		return buf;
	}
	while (*ptr && clen < (maxlen+2)) {
		if (!strncmp(ptr, oldstr, len)) {
			for (ptr3 = newstr; ((ptr2 - buf) < (maxlen - 2)) && *ptr3;)
				*(ptr2++) = *(ptr3++);
			ptr += len;
			clen += len;
		} else {
			*(ptr2++) = *(ptr++);
			clen++;
		}
	}
	*ptr2 = '\0';
	return buf;
}


const char *
get_list_item(dbref player, dbref what, dbref perms, char *listname, int itemnum, int mesgtyp, int* blessed)
{
	char buf[BUFFER_LEN];
	const char *ptr;
	int len = strlen(listname);

	if (listname[len-1] == NUMBER_TOKEN) listname[len-1] = 0;

	snprintf(buf, sizeof(buf), "%.512s#/%d", listname, itemnum);
	ptr = safegetprop(player, what, perms, buf, mesgtyp, blessed);
	if (!ptr || *ptr)
		return ptr;

	snprintf(buf, sizeof(buf), "%.512s/%d", listname, itemnum);
	ptr = safegetprop(player, what, perms, buf, mesgtyp, blessed);
	if (!ptr || *ptr)
		return ptr;

	snprintf(buf, sizeof(buf), "%.512s%d", listname, itemnum);
	return (safegetprop(player, what, perms, buf, mesgtyp, blessed));
}


int
get_list_count(dbref player, dbref obj, dbref perms, char *listname, int mesgtyp, int* blessed)
{
	char buf[BUFFER_LEN];
	const char *ptr;
	int i, len = strlen(listname);

	if (listname[len-1] == NUMBER_TOKEN) listname[len-1] = 0;

	snprintf(buf, sizeof(buf), "%.512s#", listname);
	ptr = safegetprop(player, obj, perms, buf, mesgtyp, blessed);
	if (ptr && *ptr)
		return (atoi(ptr));

	snprintf(buf, sizeof(buf), "%.512s/#", listname);
	ptr = safegetprop(player, obj, perms, buf, mesgtyp, blessed);
	if (ptr && *ptr)
		return (atoi(ptr));

	for (i = 1; i < MAX_MFUN_LIST_LEN; i++) {
		ptr = get_list_item(player, obj, perms, listname, i, mesgtyp, blessed);
		if (!ptr)
			return 0;
		if (!*ptr)
			break;
	}
	if (i-- < MAX_MFUN_LIST_LEN)
		return i;
	return MAX_MFUN_LIST_LEN;
}



char *
get_concat_list(dbref player, dbref what, dbref perms, dbref obj, char *listname,
				char *buf, int maxchars, int mode, int mesgtyp, int* blessed)
{
	int line_limit = MAX_MFUN_LIST_LEN;
	int i, cnt, len;
	const char *ptr;
	char *pos = buf;
	int tmpbless;

	len = strlen(listname);
	if (listname[len-1] == NUMBER_TOKEN) listname[len-1] = 0;
	cnt = get_list_count(what, obj, perms, listname, mesgtyp, &tmpbless);

	*blessed = 1;

	if (!tmpbless) {
		*blessed = 0;
	}

	if (cnt == 0) {
		return NULL;
	}
	maxchars -= 2;
	*buf = '\0';
	for (i = 1; ((pos - buf) < (maxchars - 1)) && i <= cnt && line_limit--; i++) {
		ptr = get_list_item(what, obj, perms, listname, i, mesgtyp, &tmpbless);
		if (ptr) {
			if (!tmpbless) {
				*blessed = 0;
			}
			while (mode && isspace(*ptr))
				ptr++;
			if (pos > buf) {
				if (!mode) {
					*(pos++) = '\r';
					*pos = '\0';
				} else if (mode == 1) {
					char ch = *(pos - 1);

					if ((pos - buf) >= (maxchars - 2))
						break;
					if (ch == '.' || ch == '?' || ch == '!')
						*(pos++) = ' ';
					*(pos++) = ' ';
					*pos = '\0';
				} else {
					*pos = '\0';
				}
			}
			while (((pos - buf) < (maxchars - 1)) && *ptr)
				*(pos++) = *(ptr++);
			if (mode) {
				while (pos > buf && *(pos - 1) == ' ')
					pos--;
			}
			*pos = '\0';
			if ((pos - buf) >= (maxchars - 1))
				break;
		}
	}
	return (buf);
}


int
mesg_read_perms(dbref player, dbref perms, dbref obj, int mesgtyp)
{
	if ((obj == 0) || (obj == player) || (obj == perms))
		return 1;
	if (OWNER(perms) == OWNER(obj))
		return 1;
	if ((mesgtyp & MPI_ISBLESSED))
		return 1;
	return 0;
}


int
isneighbor(dbref d1, dbref d2)
{
	if (d1 == d2)
		return 1;
	if (Typeof(d1) != TYPE_ROOM)
		if (getloc(d1) == d2)
			return 1;
	if (Typeof(d2) != TYPE_ROOM)
		if (getloc(d2) == d1)
			return 1;
	if (Typeof(d1) != TYPE_ROOM && Typeof(d2) != TYPE_ROOM)
		if (getloc(d1) == getloc(d2))
			return 1;
	return 0;
}


int
mesg_local_perms(dbref player, dbref perms, dbref obj, int mesgtyp)
{
	if (getloc(obj) != NOTHING && OWNER(perms) == OWNER(getloc(obj)))
		return 1;
	if (isneighbor(perms, obj))
		return 1;
	if (isneighbor(player, obj))
		return 1;
	if (mesg_read_perms(player, perms, obj, mesgtyp))
		return 1;
	return 0;
}


dbref
mesg_dbref_raw(int descr, dbref player, dbref what, dbref perms, const char *buf)
{
	struct match_data md;
	dbref obj = UNKNOWN;

	if (buf && *buf) {
		if (!string_compare(buf, "this")) {
			obj = what;
		} else if (!string_compare(buf, "me")) {
			obj = player;
		} else if (!string_compare(buf, "here")) {
			obj = getloc(player);
		} else if (!string_compare(buf, "home")) {
			obj = HOME;
		} else {
			init_match(descr, player, buf, NOTYPE, &md);
			match_absolute(&md);
			match_all_exits(&md);
			match_neighbor(&md);
			match_possession(&md);
			match_registered(&md);
			obj = match_result(&md);
			if (obj == NOTHING) {
				init_match_remote(descr, player, what, buf, NOTYPE, &md);
				match_player(&md);
				match_all_exits(&md);
				match_neighbor(&md);
				match_possession(&md);
				match_registered(&md);
				obj = match_result(&md);
			}
		}
	}

	if (obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE)
		obj = UNKNOWN;
	return obj;
}


dbref
mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp)
{
	dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

	if (obj == UNKNOWN)
		return obj;
	if (!mesg_read_perms(player, perms, obj, mesgtyp)) {
		obj = PERMDENIED;
	}
	return obj;
}


dbref
mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp)
{
	dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

	if (obj == UNKNOWN)
		return obj;
	if (!(mesgtyp & MPI_ISBLESSED) && OWNER(perms) != OWNER(obj)) {
		obj = PERMDENIED;
	}
	return obj;
}


dbref
mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp)
{
	dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

	if (obj == UNKNOWN)
		return obj;
	if (!mesg_local_perms(player, perms, obj, mesgtyp)) {
		obj = PERMDENIED;
	}
	return obj;
}


char *
ref2str(dbref obj, char *buf, size_t buflen)
{
	if (obj < -3 || obj >= db_top) {
		snprintf(buf, buflen, "Bad");
		return buf;
	}

	if (obj >= 0 && Typeof(obj) == TYPE_PLAYER) {
		snprintf(buf, buflen, "*%s", NAME(obj));
	} else {
		snprintf(buf, buflen, "#%d", obj);
	}
	return buf;
}


int
truestr(char *buf)
{
	while (isspace(*buf))
		buf++;
	if (!*buf || (number(buf) && !atoi(buf)))
		return 0;
	return 1;
}


/******** MPI Variable handling ********/

struct mpivar {
	char name[MAX_MFUN_NAME_LEN + 1];
	char *buf;
};

static struct mpivar varv[MPI_MAX_VARIABLES];
int varc = 0;

int check_mvar_overflow(int count)
{
	return (varc + count) > MPI_MAX_VARIABLES;
}

int
new_mvar(const char *varname, char *buf)
{
	if (strlen(varname) > MAX_MFUN_NAME_LEN)
		return 1;
	if (varc >= MPI_MAX_VARIABLES)
		return 2;
	strcpyn(varv[varc].name, sizeof(varv[varc].name), varname);
	varv[varc++].buf = buf;
	return 0;
}

char *
get_mvar(const char *varname)
{
	int i = 0;

	for (i = varc - 1; i >= 0 && string_compare(varname, varv[i].name); i--) ;
	if (i < 0)
		return NULL;
	return varv[i].buf;
}

int
free_top_mvar(void)
{
	if (varc < 1)
		return 1;
	varc--;
	return 0;
}


/***** MPI function handling *****/


struct mpifunc {
	char name[MAX_MFUN_NAME_LEN + 1];
	char *buf;
};

static struct mpifunc funcv[MPI_MAX_FUNCTIONS];
int funcc = 0;

int
new_mfunc(const char *funcname, const char *buf)
{
	if (strlen(funcname) > MAX_MFUN_NAME_LEN)
		return 1;
	if (funcc > MPI_MAX_FUNCTIONS)
		return 2;
	strcpyn(funcv[funcc].name, sizeof(funcv[funcc].name), funcname);
	funcv[funcc++].buf = (char *) string_dup(buf);
	return 0;
}

const char *
get_mfunc(const char *funcname)
{
	int i = 0;

	for (i = funcc - 1; i >= 0 && string_compare(funcname, funcv[i].name); i--) ;
	if (i < 0)
		return NULL;
	return funcv[i].buf;
}

int
free_mfuncs(int downto)
{
	if (funcc < 1)
		return 1;
	if (downto < 0)
		return 1;
	while (funcc > downto)
		free(funcv[--funcc].buf);
	return 0;
}



/*** End of MFUNs. ***/

int
msg_is_macro(dbref player, dbref what, dbref perms, const char *name, int mesgtyp)
{
	const char *ptr;
	char buf2[BUFFER_LEN];
	dbref obj;
	int blessed;

	if (!*name)
		return 0;
	snprintf(buf2, sizeof(buf2), "_msgmacs/%s", name);
	obj = what;
	ptr = get_mfunc(name);
	if (!ptr || !*ptr)
		ptr = safegetprop_strict(player, OWNER(obj), perms, buf2, mesgtyp, &blessed);
	if (!ptr || !*ptr)
		ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2, mesgtyp, &blessed);
	if (!ptr || !*ptr)
		ptr = safegetprop_strict(player, 0, perms, buf2, mesgtyp, &blessed);
	if (!ptr || !*ptr)
		return 0;
	return 1;
}


void
msg_unparse_macro(dbref player, dbref what, dbref perms, char *name, int argc, argv_typ argv,
				  char *rest, int maxchars, int mesgtyp)
{
	const char *ptr;
	char *ptr2;
	char buf[BUFFER_LEN];
	char buf2[BUFFER_LEN];
	dbref obj;
	int i, p = 0;
	int blessed;

	strcpyn(buf, sizeof(buf), rest);
	snprintf(buf2, sizeof(buf2), "_msgmacs/%s", name);
	obj = what;
	ptr = get_mfunc(name);
	if (!ptr || !*ptr)
		ptr = safegetprop_strict(player, OWNER(obj), perms, buf2, mesgtyp, &blessed);
	if (!ptr || !*ptr)
		ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2, mesgtyp, &blessed);
	if (!ptr || !*ptr)
		ptr = safegetprop_strict(player, 0, perms, buf2, mesgtyp, &blessed);
	while (ptr && *ptr && p < (maxchars - 1)) {
		if (*ptr == '\\') {
			if (*(ptr + 1) == 'r') {
				rest[p++] = '\r';
				ptr++;
				ptr++;
			} else if (*(ptr + 1) == '[') {
				rest[p++] = ESCAPE_CHAR;
				ptr++;
				ptr++;
			} else {
				rest[p++] = *(ptr++);
				rest[p++] = *(ptr++);
			}
		} else if (*ptr == MFUN_LEADCHAR) {
			if (*(ptr + 1) == MFUN_ARGSTART && isdigit(*(ptr + 2)) &&
				*(ptr + 3) == MFUN_ARGEND) {
				ptr++;
				ptr++;
				i = *(ptr++) - '1';
				ptr++;
				if (i >= argc || i < 0) {
					ptr2 = NULL;
				} else {
					ptr2 = argv[i];
				}
				while (ptr2 && *ptr2 && p < (maxchars - 1)) {
					rest[p++] = *(ptr2++);
				}
			} else {
				rest[p++] = *(ptr++);
			}
		} else {
			rest[p++] = *(ptr++);
		}
	}
	ptr2 = buf;
	while (ptr2 && *ptr2 && p < (maxchars - 1)) {
		rest[p++] = *(ptr2++);
	}
	rest[p] = '\0';
}


#ifndef MSGHASHSIZE
#define MSGHASHSIZE 256
#endif

static hash_tab msghash[MSGHASHSIZE];

int
find_mfn(const char *name)
{
	hash_data *exp = find_hash(name, msghash, MSGHASHSIZE);

	if (exp)
		return (exp->ival);
	return (0);
}


void
insert_mfn(const char *name, int i)
{
	hash_data hd;

	(void) free_hash(name, msghash, MSGHASHSIZE);
	hd.ival = i;
	(void) add_hash(name, hd, msghash, MSGHASHSIZE);
}


void
purge_mfns(void)
{
	kill_hash(msghash, MSGHASHSIZE, 0);
}


#define DEFINE_MFUN_LIST
#include "mfunlist.h"


/******** HOOK ********/
void
mesg_init(void)
{
	int i;

	for (i = 0; mfun_list[i].name; i++)
		insert_mfn(mfun_list[i].name, i + 1);
	mpi_prof_start_time = time(NULL);
}



/******** HOOK ********/
int
mesg_args(char *wbuf, int maxlen, argv_typ argv, char ulv, char sep, char dlv, char quot, int maxargs)
{
	int r, lev, argc = 0;
	char buf[BUFFER_LEN];
	char *ptr;
	int litflag = 0;

	/* for (ptr = wbuf; ptr && isspace(*ptr); ptr++); */
	strcpyn(buf, sizeof(buf), wbuf);
	ptr = buf;
	for (lev = r = 0; (r < (BUFFER_LEN - 2)); r++) {
		if (buf[r] == '\0') {
			return (-1);
		} else if (buf[r] == '\\') {
			r++;
			continue;
		} else if (buf[r] == quot) {
			litflag = (!litflag);
		} else if (!litflag && buf[r] == ulv) {
			lev++;
			continue;
		} else if (!litflag && buf[r] == dlv) {
			lev--;
			if (lev < 0) {
				buf[r] = '\0';
				if (argv[argc]) {
					free(argv[argc]);
					argv[argc] = NULL;
				}
				argv[argc] = (char*)malloc(((buf + r) - ptr) + 1);
				strcpyn(argv[argc++], ((buf + r) - ptr) + 1, ptr);
				ptr = buf + r + 1;
				break;
			}
			continue;
		} else if (!litflag && lev < 1 && buf[r] == sep) {
			if (argc < maxargs - 1) {
				buf[r] = '\0';
				if (argv[argc]) {
					free(argv[argc]);
					argv[argc] = NULL;
				}
				argv[argc] = (char*)malloc(((buf + r) - ptr) + 1);
				strcpyn(argv[argc++], ((buf + r) - ptr) + 1, ptr);
				ptr = buf + r + 1;
			}
		}
	}
	buf[BUFFER_LEN - 1] = '\0';
	strcpyn(wbuf, maxlen, ptr);
	return argc;
}


char *
cr2slash(char *buf, int buflen, const char *in)
{
	char *ptr = buf;
	const char *ptr2 = in;

	for (ptr = buf, ptr2 = in; *ptr2 && ptr-buf<buflen-3; ptr2++) {
		if (*ptr2 == '\r') {
			*(ptr++) = '\\';
			*(ptr++) = 'r';
		} else if (*ptr2 == ESCAPE_CHAR) {
			*(ptr++) = '\\';
			*(ptr++) = '[';
		} else if (*ptr2 == '`') {
			*(ptr++) = '\\';
			*(ptr++) = '`';
		} else if (*ptr2 == '\\') {
			*(ptr++) = '\\';
			*(ptr++) = '\\';
		} else {
			*(ptr++) = *ptr2;
		}
	}
	*(ptr++) = '\0';
	return buf;
}


static int mesg_rec_cnt = 0;
static int mesg_instr_cnt = 0;


/******** HOOK ********/
char *
mesg_parse(int descr, dbref player, dbref what, dbref perms,
    const char *inbuf, char *outbuf, int maxchars, int mesgtyp)
{
	char wbuf[BUFFER_LEN];
	char buf[BUFFER_LEN];
	char buf2[BUFFER_LEN];
	char dbuf[BUFFER_LEN];
	char ebuf[BUFFER_LEN];
	char cmdbuf[MAX_MFUN_NAME_LEN + 1];
	const char *ptr;
	char *dptr;
	int p, q, s;
	int i;
	char *argv[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	int argc = 0;
	int showtextflag = 0;
	int literalflag = 0;

	mesg_rec_cnt++;
	if (mesg_rec_cnt > 26) {
		char *zptr = get_mvar("how");
		snprintf(dbuf, sizeof(dbuf), "%s Recursion limit exceeded.", zptr);
		notify_nolisten(player, dbuf, 1);
		mesg_rec_cnt--;
		outbuf[0] = '\0';
		return NULL;
	}
	if (Typeof(player) == TYPE_GARBAGE) {
		mesg_rec_cnt--;
		outbuf[0] = '\0';
		return NULL;
	}
	if (Typeof(what) == TYPE_GARBAGE) {
		notify_nolisten(player, "MPI Error: Garbage trigger.", 1);
		mesg_rec_cnt--;
		outbuf[0] = '\0';
		return NULL;
	}
	strcpyn(wbuf, sizeof(wbuf), inbuf);
	for (p = q = 0; wbuf[p] && (p < maxchars - 1) && q < (maxchars - 1); p++) {
		if (wbuf[p] == '\\') {
			p++;
			showtextflag = 1;
			if (wbuf[p] == 'r') {
				outbuf[q++] = '\r';
			} else if (wbuf[p] == '[') {
				outbuf[q++] = ESCAPE_CHAR;
			} else {
				outbuf[q++] = wbuf[p];
			}
		} else if (wbuf[p] == MFUN_LITCHAR) {
			literalflag = (!literalflag);
		} else if (!literalflag && wbuf[p] == MFUN_LEADCHAR) {
			if (wbuf[p + 1] == MFUN_LEADCHAR) {
				showtextflag = 1;
				outbuf[q++] = wbuf[p++];
				ptr = "";
			} else {
				ptr = wbuf + (++p);
				s = 0;
				while (wbuf[p] && wbuf[p] != MFUN_LEADCHAR &&
					   !isspace(wbuf[p]) && wbuf[p] != MFUN_ARGSTART &&
					   wbuf[p] != MFUN_ARGEND && s < MAX_MFUN_NAME_LEN) {
					p++;
					s++;
				}
				if (s < MAX_MFUN_NAME_LEN &&
					(wbuf[p] == MFUN_ARGSTART || wbuf[p] == MFUN_ARGEND)) {
					int varflag;

					strncpy(cmdbuf, ptr, s);
					cmdbuf[s] = '\0';

					varflag = 0;
					if (*cmdbuf == '&') {
						s = find_mfn("sublist");
						varflag = 1;
					} else if (*cmdbuf) {
						s = find_mfn(cmdbuf);
					} else {
						s = 0;
					}
					if (s) {
						s--;
						if (++mesg_instr_cnt > tp_mpi_max_commands) {
							char *zptr = get_mvar("how");

							snprintf(dbuf, sizeof(dbuf), "%s %c%s%c: Instruction limit exceeded.",
									zptr, MFUN_LEADCHAR,
									(varflag ? cmdbuf : mfun_list[s].name), MFUN_ARGEND);
							notify_nolisten(player, dbuf, 1);
							mesg_rec_cnt--;
							outbuf[0] = '\0';
							return NULL;
						}
						for (i = 0; i < argc; i++) {
							free(argv[i]);
							argv[i] = NULL;
						}
						if (wbuf[p] == MFUN_ARGEND) {
							argc = 0;
						} else {
							argc = mfun_list[s].maxargs;
							if (argc < 0) {
								argc = mesg_args((wbuf + p + 1), (sizeof(wbuf) - p - 1),
												 &argv[(varflag ? 1 : 0)],
												 MFUN_LEADCHAR, MFUN_ARGSEP,
												 MFUN_ARGEND, MFUN_LITCHAR,
												 (-argc) + (varflag ? 1 : 0));
							} else {
								argc = mesg_args((wbuf + p + 1), (sizeof(wbuf) - p - 1),
												 &argv[(varflag ? 1 : 0)],
												 MFUN_LEADCHAR, MFUN_ARGSEP,
												 MFUN_ARGEND, MFUN_LITCHAR, (varflag ? 8 : 9));
							}
							if (argc == -1) {
								char *zptr = get_mvar("how");

								snprintf(ebuf, sizeof(ebuf), "%s %c%s%c: End brace not found.",
										zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
								notify_nolisten(player, ebuf, 1);
								for (i = 0; i < argc; i++) {
									free(argv[i + (varflag? 1 : 0)]);
								}
								mesg_rec_cnt--;
								outbuf[0] = '\0';
								return NULL;
							}
						}
						if (varflag) {
							char *zptr;

							zptr = get_mvar(cmdbuf + 1);
							if (!zptr) {
								zptr = get_mvar("how");
								snprintf(ebuf, sizeof(ebuf), "%s %c%s%c: Unrecognized variable.",
										zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
								notify_nolisten(player, ebuf, 1);
								for (i = 0; i < argc; i++) {
									if (argv[i + (varflag? 1 : 0)]) {
										free(argv[i + (varflag? 1 : 0)]);
									}
								}
								mesg_rec_cnt--;
								outbuf[0] = '\0';
								return NULL;
							}
							if (argv[0]) {
								free(argv[0]);
								argv[0] = NULL;
							}
							argv[0] = string_dup(zptr);
							argc++;
						}
						if (mesgtyp & MPI_ISDEBUG) {
							char *zptr = get_mvar("how");

							snprintf(dbuf, sizeof(dbuf), "%s %*s%c%s%c", zptr,
									(mesg_rec_cnt * 2 - 4), "", MFUN_LEADCHAR,
									(varflag ? cmdbuf : mfun_list[s].name), MFUN_ARGSTART);
							for (i = (varflag ? 1 : 0); i < argc; i++) {
								if (i) {
									const char tbuf[] = { MFUN_ARGSEP, '\0' };
									strcatn(dbuf, sizeof(dbuf), tbuf);
								}
								cr2slash(ebuf, sizeof(ebuf)/8, argv[i]);
								strcatn(dbuf, sizeof(dbuf), "`");
								strcatn(dbuf, sizeof(dbuf), ebuf);
								if (strlen(ebuf) >= (sizeof(ebuf)/8)-2) {
									strcatn(dbuf, sizeof(dbuf), "...");
								}
								strcatn(dbuf, sizeof(dbuf), "`");
							}
							{
								const char tbuf[] = { MFUN_ARGEND, '\0' };
								strcatn(dbuf, sizeof(dbuf), tbuf);
							}
							notify_nolisten(player, dbuf, 1);
						}
						if (mfun_list[s].stripp) {
							for (i = (varflag ? 1 : 0); i < argc; i++) {
								stripspaces(buf, sizeof(buf), argv[i]);
								/*
								 * stripspaces() can only shorten a string.
								 * The argv[i] buffer will therefore always
								 * be large enough.
								 */
								strcpyn(argv[i], strlen(buf)+1, buf);
							}
						}
						if (mfun_list[s].parsep) {
							for (i = (varflag ? 1 : 0); i < argc; i++) {
								ptr = MesgParse(argv[i], buf, sizeof(buf));
								if (!ptr) {
									char *zptr = get_mvar("how");

									snprintf(dbuf, sizeof(dbuf), "%s %c%s%c (arg %d)", zptr,
											MFUN_LEADCHAR,
											(varflag ? cmdbuf : mfun_list[s].name),
											MFUN_ARGEND, i + 1);
									notify_nolisten(player, dbuf, 1);
									for (i = 0; i < argc; i++) {
										free(argv[i]);
									}
									mesg_rec_cnt--;
									outbuf[0] = '\0';
									return NULL;
								}
								argv[i] = (char*)realloc(argv[i], strlen(buf) + 1);
								strcpyn(argv[i], strlen(buf)+1, buf);
							}
						}
						if (mesgtyp & MPI_ISDEBUG) {
							char *zptr = get_mvar("how");

							snprintf(dbuf, sizeof(dbuf), "%.512s %*s%c%.512s%c", zptr,
									(mesg_rec_cnt * 2 - 4), "", MFUN_LEADCHAR,
									(varflag ? cmdbuf : mfun_list[s].name), MFUN_ARGSTART);
							for (i = (varflag ? 1 : 0); i < argc; i++) {
								if (i) {
									const char tbuf[] = { MFUN_ARGSEP, '\0' };
									strcatn(dbuf, sizeof(dbuf), tbuf);
								}
								cr2slash(ebuf, sizeof(ebuf)/8, argv[i]);
								strcatn(dbuf, sizeof(dbuf), "`");
								strcatn(dbuf, sizeof(dbuf), ebuf);
								if (strlen(ebuf) >= (sizeof(ebuf)/8)-2) {
									strcatn(dbuf, sizeof(dbuf), "...");
								}
								strcatn(dbuf, sizeof(dbuf), "`");
							}
							{
								const char tbuf[] = { MFUN_ARGEND, '\0' };
								strcatn(dbuf, sizeof(dbuf), tbuf);
							}
						}
						if (argc < mfun_list[s].minargs) {
							char *zptr = get_mvar("how");

							snprintf(ebuf, sizeof(ebuf), "%s %c%s%c: Too few arguments",
									zptr, MFUN_LEADCHAR,
									(varflag ? cmdbuf : mfun_list[s].name), MFUN_ARGEND);
							notify_nolisten(player, ebuf, 1);
							for (i = 0; i < argc; i++) {
								free(argv[i]);
							}
							mesg_rec_cnt--;
							outbuf[0] = '\0';
							return NULL;
						} else if (mfun_list[s].maxargs > 0 && argc > mfun_list[s].maxargs) {
							char *zptr = get_mvar("how");

							snprintf(ebuf, sizeof(ebuf), "%s %c%s%c: Too many arguments",
									zptr, MFUN_LEADCHAR,
									(varflag ? cmdbuf : mfun_list[s].name), MFUN_ARGEND);
							notify_nolisten(player, ebuf, 1);
							for (i = 0; i < argc; i++) {
								free(argv[i]);
							}
							mesg_rec_cnt--;
							outbuf[0] = '\0';
							return NULL;
						} else {
							ptr = mfun_list[s].mfn(descr, player, what, perms,
							    argc, argv, buf, sizeof(buf), mesgtyp);
							if (!ptr) {
								outbuf[q] = '\0';
								for (i = 0; i < argc; i++) {
									free(argv[i]);
								}
								mesg_rec_cnt--;
								outbuf[0] = '\0';
								return NULL;
							}
							if (mfun_list[s].postp) {
								dptr = MesgParse(ptr, buf, sizeof(buf));
								if (!dptr) {
									char *zptr = get_mvar("how");

									snprintf(ebuf, sizeof(ebuf), "%s %c%s%c (returned string)",
											zptr, MFUN_LEADCHAR,
											(varflag ? cmdbuf : mfun_list[s].name),
											MFUN_ARGEND);
									notify_nolisten(player, ebuf, 1);
									for (i = 0; i < argc; i++) {
										free(argv[i]);
									}
									mesg_rec_cnt--;
									outbuf[0] = '\0';
									return NULL;
								}
								ptr = dptr;
							}
						}
						if (mesgtyp & MPI_ISDEBUG) {
							strcatn(dbuf, sizeof(dbuf), " = `");
							cr2slash(ebuf, sizeof(ebuf)/8, ptr);
							strcatn(dbuf, sizeof(dbuf), ebuf);
							if (strlen(ebuf) >= (sizeof(ebuf)/8)-2) {
								strcatn(dbuf, sizeof(dbuf), "...");
							}
							strcatn(dbuf, sizeof(dbuf), "`");
							notify_nolisten(player, dbuf, 1);
						}
					} else if (msg_is_macro(player, what, perms, cmdbuf, mesgtyp)) {
						for (i = 0; i < argc; i++) {
							free(argv[i]);
							argv[i] = NULL;
						}
						if (wbuf[p] == MFUN_ARGEND) {
							argc = 0;
							p++;
						} else {
							p++;
							argc = mesg_args(wbuf + p, (sizeof(wbuf) - p), argv, MFUN_LEADCHAR,
											 MFUN_ARGSEP, MFUN_ARGEND, MFUN_LITCHAR, 9);
							if (argc == -1) {
								char *zptr = get_mvar("how");

								snprintf(ebuf, sizeof(ebuf), "%s %c%s%c: End brace not found.",
										zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
								notify_nolisten(player, ebuf, 1);
								for (i = 0; i < argc; i++) {
									free(argv[i]);
								}
								mesg_rec_cnt--;
								outbuf[0] = '\0';
								return NULL;
							}
						}
						msg_unparse_macro(player, what, perms, cmdbuf, argc,
										  argv, (wbuf + p), (BUFFER_LEN - p),
										  mesgtyp);
						p--;
						ptr = NULL;
					} else {
						/* unknown function */
						char *zptr = get_mvar("how");

						snprintf(ebuf, sizeof(ebuf), "%s %c%s%c: Unrecognized function.",
								zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
						notify_nolisten(player, ebuf, 1);
						for (i = 0; i < argc; i++) {
							free(argv[i]);
						}
						mesg_rec_cnt--;
						outbuf[0] = '\0';
						return NULL;
					}
				} else {
					showtextflag = 1;
					ptr--;
					i = s + 1;
					while (ptr && *ptr && i-- && q < (maxchars - 1)) {
						outbuf[q++] = *(ptr++);
					}
					outbuf[q] = '\0';
					p = (int) (ptr - wbuf) - 1;
					ptr = "";	/* unknown substitution type */
				}
				while (ptr && *ptr && q < (maxchars - 1)) {
					outbuf[q++] = *(ptr++);
				}
			}
		} else {
			outbuf[q++] = wbuf[p];
			showtextflag = 1;
		}
	}
	outbuf[q] = '\0';
	if ((mesgtyp & MPI_ISDEBUG) && showtextflag) {
		char *zptr = get_mvar("how");

		snprintf(dbuf, sizeof(dbuf), "%s %*s`%.512s`",
		        zptr, (mesg_rec_cnt * 2 - 4), "",
				cr2slash(buf2, sizeof(buf2), outbuf));
		notify_nolisten(player, dbuf, 1);
	}
	for (i = 0; i < argc; i++) {
		free(argv[i]);
	}
	mesg_rec_cnt--;
	outbuf[maxchars - 1] = '\0';
	return (outbuf);
}

char *
do_parse_mesg_2(int descr, dbref player, dbref what, dbref perms,
    const char *inbuf, const char *abuf, char *outbuf, int outbuflen,
    int mesgtyp)
{

	char howvar[BUFFER_LEN];
	char cmdvar[BUFFER_LEN];
	char argvar[BUFFER_LEN];
	char tmparg[BUFFER_LEN];
	char tmpcmd[BUFFER_LEN];
	char *dptr;
	int mvarcnt = varc;
	int mfunccnt = funcc;
	int tmprec_cnt = mesg_rec_cnt;
	int tmpinst_cnt = mesg_instr_cnt;

	*outbuf = '\0';

	if ((mesgtyp & MPI_NOHOW) == 0)
	{
		if (new_mvar("how", howvar))
		{
			snprintf(howvar, sizeof(howvar), "%s Out of MPI variables.", abuf);
			notify_nolisten(player, howvar, 1);
			varc = mvarcnt;
			return outbuf;
		}
		strcpyn(howvar, sizeof(howvar), abuf);
	}

	if (new_mvar("cmd", cmdvar))
	{
		snprintf(cmdvar, sizeof(cmdvar), "%s Out of MPI variables.", abuf);
		notify_nolisten(player, cmdvar, 1);
		varc = mvarcnt;
		return outbuf;
	}
	strcpyn(cmdvar, sizeof(cmdvar), match_cmdname);
	strcpyn(tmpcmd, sizeof(tmpcmd), match_cmdname);

	if (new_mvar("arg", argvar))
	{
		snprintf(argvar, sizeof(argvar), "%s Out of MPI variables.", abuf);
		notify_nolisten(player, argvar, 1);
		varc = mvarcnt;
		return outbuf;
	}
	strcpyn(argvar, sizeof(argvar), match_args);
	strcpyn(tmparg, sizeof(tmparg), match_args);

	dptr = MesgParse(inbuf, outbuf, outbuflen);
	if (!dptr) {
		*outbuf = '\0';
	}

	varc = mvarcnt;
	free_mfuncs(mfunccnt);
	mesg_rec_cnt = tmprec_cnt;
	mesg_instr_cnt = tmpinst_cnt;

	strcpyn(match_cmdname, sizeof(match_cmdname), tmpcmd);
	strcpyn(match_args, sizeof(match_args), tmparg);


	return outbuf;
}


char *
do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf, const char *abuf, char *outbuf, int outbuflen, int mesgtyp)
{
	if (tp_do_mpi_parsing) {
		char *tmp = NULL;
		struct timeval st, et;

		/* Quickie additions to do rough per-object MPI profiling */
		gettimeofday(&st,NULL);
		tmp = do_parse_mesg_2(descr, player, what, what, inbuf, abuf, outbuf, outbuflen, mesgtyp);
		gettimeofday(&et,NULL);
		if (strcmp(tmp,inbuf)) {
			if (st.tv_usec > et.tv_usec) {
				et.tv_usec += 1000000;
				et.tv_sec -= 1;
			}
			et.tv_usec -= st.tv_usec;
			et.tv_sec -= st.tv_sec;
			DBFETCH(what)->mpi_proftime.tv_sec += et.tv_sec;
			DBFETCH(what)->mpi_proftime.tv_usec += et.tv_usec;
			if (DBFETCH(what)->mpi_proftime.tv_usec >= 1000000) {
				DBFETCH(what)->mpi_proftime.tv_usec -= 1000000;
				DBFETCH(what)->mpi_proftime.tv_sec += 1;
			}
			DBFETCH(what)->mpi_prof_use++;
		}
		return(tmp);
	} else {
		strcpyn(outbuf, outbuflen, inbuf);
	}
	return outbuf;
}

char *
do_parse_prop(int descr, dbref player, dbref what, const char *propname, const char *abuf, char *outbuf, int outbuflen, int mesgtyp)
{
	const char* propval = get_property_class(what, propname);
	if (!propval)
		return NULL;
	if (Prop_Blessed(what, propname))
		mesgtyp |= MPI_ISBLESSED;
	return do_parse_mesg(descr, player, what, propval, abuf, outbuf, outbuflen, mesgtyp);
}
