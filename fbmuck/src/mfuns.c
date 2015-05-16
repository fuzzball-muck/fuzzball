
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
#include "interp.h"
#include "interface.h"
#include "msgparse.h"


/***** Insert MFUNs here *****/

const char *
mfn_func(MFUNARGS)
{
	char *funcname;
	char *ptr=NULL, *def;
	char namebuf[BUFFER_LEN];
	char argbuf[BUFFER_LEN];
	char defbuf[BUFFER_LEN];
	int i;

	funcname = MesgParse(argv[0], namebuf, sizeof(namebuf));
	CHECKRETURN(funcname, "FUNC", "name argument (1)");

	def = argv[argc - 1];
	for (i = 1; i < argc - 1; i++) {
		ptr = MesgParse(argv[i], argbuf, sizeof(argbuf));
		CHECKRETURN(ptr, "FUNC", "variable name argument");
		snprintf(defbuf, sizeof(defbuf), "{with:%.*s,{:%d},%.*s}", MAX_MFUN_NAME_LEN, ptr, i,
				(BUFFER_LEN - MAX_MFUN_NAME_LEN - 20), def);
	}
	i = new_mfunc(funcname, defbuf);
	if (i == 1)
		ABORT_MPI("FUNC", "Function Name too long.");
	if (i == 2)
		ABORT_MPI("FUNC", "Too many functions defined.");

	return "";
}


const char *
mfn_muckname(MFUNARGS)
{
	return tp_muckname;
}


const char *
mfn_version(MFUNARGS)
{
	return VERSION;
}


const char *
mfn_prop(MFUNARGS)
{
	dbref obj = what;
	const char *ptr, *pname;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("PROP", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("PROP", "Permission denied.");
	ptr = safegetprop(player, obj, perms, pname, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("PROP", "Failed read.");
	return ptr;
}


const char *
mfn_propbang(MFUNARGS)
{
	dbref obj = what;
	const char *ptr, *pname;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("PROP!", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("PROP!", "Permission denied.");
	ptr = safegetprop_strict(player, obj, perms, pname, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("PROP!", "Failed read.");
	return ptr;
}


const char *
mfn_store(MFUNARGS)
{
	dbref obj = what;
	char *ptr, *pname;

	pname = argv[1];
	ptr = argv[0];
	if (argc > 2) {
		obj = mesg_dbref_strict(descr, player, what, perms, argv[2], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("STORE", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("STORE", "Permission denied.");
	if (!safeputprop(obj, perms, pname, ptr, mesgtyp))
		ABORT_MPI("STORE", "Permission denied.");
	return ptr;
}


const char *
mfn_bless(MFUNARGS)
{
	dbref obj = what;
	char *pname;

	pname = argv[0];
	if (argc > 1) {
		obj = mesg_dbref_strict(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("BLESS", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("BLESS", "Permission denied.");
	if (!safeblessprop(obj, perms, pname, mesgtyp, 1))
		ABORT_MPI("BLESS", "Permission denied.");
	return "";
}


const char *
mfn_unbless(MFUNARGS)
{
	dbref obj = what;
	char *pname;

	pname = argv[0];
	if (argc > 1) {
		obj = mesg_dbref_strict(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("UNBLESS", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("UNBLESS", "Permission denied.");
	if (!safeblessprop(obj, perms, pname, mesgtyp, 0))
		ABORT_MPI("UNBLESS", "Permission denied.");
	return "";
}


const char *
mfn_delprop(MFUNARGS)
{
	dbref obj = what;
	char *pname;

	pname = argv[0];
	if (argc > 1) {
		obj = mesg_dbref_strict(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("DELPROP", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("DELPROP", "Permission denied.");
	if (!safeputprop(obj, perms, pname, NULL, mesgtyp))
		ABORT_MPI("DELPROP", "Permission denied.");
	return "";
}


const char *
mfn_exec(MFUNARGS)
{
	dbref trg, obj = what;
	const char *ptr, *pname;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("EXEC", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("EXEC", "Permission denied.");
	while (*pname == PROPDIR_DELIMITER)
		pname++;
	ptr = safegetprop(player, obj, perms, pname, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("EXEC", "Failed read.");
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname) || Prop_Hidden(pname))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "EXEC", "propval");
	return ptr;
}


const char *
mfn_execbang(MFUNARGS)
{
	dbref trg = (dbref) 0, obj = what;
	const char *ptr, *pname;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("EXEC!", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("EXEC!", "Permission denied.");
	while (*pname == PROPDIR_DELIMITER)
		pname++;
	ptr = safegetprop_strict(player, obj, perms, pname, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("EXEC!", "Failed read.");
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname) || Prop_Hidden(pname))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "EXEC!", "propval");
	return ptr;
}


const char *
mfn_index(MFUNARGS)
{
	dbref trg = (dbref) 0, obj = what;
	dbref tmpobj = (dbref) 0;
	const char *pname, *ptr;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("INDEX", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("INDEX", "Permission denied.");
	tmpobj = obj;
	ptr = safegetprop(player, obj, perms, pname, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("INDEX", "Failed read.");
	if (!*ptr)
		return "";
	obj = tmpobj;
	ptr = safegetprop(player, obj, perms, ptr, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("INDEX", "Failed read.");
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr) || Prop_Hidden(ptr))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "INDEX", "listval");
	return ptr;
}



const char *
mfn_indexbang(MFUNARGS)
{
	dbref trg = (dbref) 0, obj = what;
	dbref tmpobj = (dbref) 0;
	const char *pname, *ptr;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("INDEX!", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("INDEX!", "Permission denied.");
	tmpobj = obj;
	ptr = safegetprop_strict(player, obj, perms, pname, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("INDEX!", "Failed read.");
	if (!*ptr)
		return "";
	obj = tmpobj;
	ptr = safegetprop_strict(player, obj, perms, ptr, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("INDEX!", "Failed read.");
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr) || Prop_Hidden(ptr))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "INDEX!", "listval");
	return ptr;
}



const char *
mfn_propdir(MFUNARGS)
{
	dbref obj = what;
	const char *pname;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("PROPDIR", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("PROPDIR", "Permission denied.");

	if (is_propdir(obj, pname)) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_listprops(MFUNARGS)
{
	dbref obj = what;
	char *ptr, *pname;
	char *endbuf, *pattern;
	char tmpbuf[BUFFER_LEN];
	char patbuf[BUFFER_LEN];
	char pnamebuf[BUFFER_LEN];
	int flag;

	strcpyn(pnamebuf, sizeof(pnamebuf), argv[0]);
	pname = pnamebuf;
	if (argc > 1) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("LISTPROPS", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("LISTPROPS", "Permission denied.");

	if (argc > 2) {
		pattern = argv[2];
	} else {
		pattern = NULL;
	}

	endbuf = pname + strlen(pname);
	if (endbuf != pname) {
		endbuf--;
	}
	if (*endbuf != PROPDIR_DELIMITER && (endbuf - pname) < (BUFFER_LEN - 2)) {
		if (*endbuf != '\0')
			endbuf++;
		*endbuf++ = PROPDIR_DELIMITER;
		*endbuf++ = '\0';
	}

	*buf = '\0';
	endbuf = buf;
	do {
		ptr = next_prop_name(obj, tmpbuf, (int) sizeof(tmpbuf), pname);
		if (ptr && *ptr) {
			flag = 1;
			if (Prop_System(ptr)) {
				flag = 0;
			} else if (!(mesgtyp & MPI_ISBLESSED)) {
				if (Prop_Hidden(ptr)) {
					flag = 0;
				}
				if (Prop_Private(ptr) && OWNER(what) != OWNER(obj)) {
					flag = 0;
				}
				if (obj != player && OWNER(obj) != OWNER(what)) {
					flag = 0;
				}
			}
			if ((flag != 0) && (pattern != NULL)) {
				char *nptr;

				nptr = rindex(ptr, PROPDIR_DELIMITER);
				if (nptr && *nptr) {
					strcpyn(patbuf, sizeof(patbuf), ++nptr);
					if (!equalstr(pattern, patbuf)) {
						flag = 0;
					}
				}
			}
			if (flag) {
				int entrylen = strlen(ptr);
				if ((endbuf - buf) + entrylen + 2 < BUFFER_LEN) {
					if (*buf != '\0') {
						*endbuf++ = '\r';
					}
					strcpyn(endbuf, BUFFER_LEN - (endbuf - buf), ptr);
					endbuf += entrylen;
				}
			}
		}
		pname = ptr;
	} while (ptr && *ptr);

	return buf;
}


const char *
mfn_concat(MFUNARGS)
{
	dbref obj = what;
	char *pname;
	const char *ptr;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("CONCAT", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("CONCAT", "Match failed.");
	ptr = get_concat_list(player, what, perms, obj, (char *)pname, buf, BUFFER_LEN, 1, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("CONCAT", "Failed list read.");
	return ptr;
}



const char *
mfn_select(MFUNARGS)
{
	char origprop[BUFFER_LEN];
	char propname[BUFFER_LEN];
	char bestname[BUFFER_LEN];
	dbref obj = what;
	dbref bestobj = 0;
	char *pname;
	const char *ptr;
	char *out, *in;
	int i, targval, bestval;
	int baselen;
	int limit;
	int blessed = 0;

	pname = argv[1];
	if (argc == 3) {
		obj = mesg_dbref(descr, player, what, perms, argv[2], mesgtyp);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("SELECT", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("SELECT", "Match failed.");

	/*
	 * Search contiguously for a bit, looking for a best match.
	 * This allows fast hits on LARGE lists.
	 */

	limit = 18;
	i = targval = atoi(argv[0]);
	do {
		ptr = get_list_item(player, obj, perms, (char *)pname, i--, mesgtyp, &blessed);
	} while (limit-->0 && i >= 0 && ptr && !*ptr);
	if (ptr == NULL)
		ABORT_MPI("SELECT", "Failed list read.");
	if (*ptr != '\0')
		return ptr;

	/*
	 * If we didn't find it before, search only existing props.
	 * This gets fast hits on very SPARSE lists.
	 */

	/* First, normalize the base propname */
	out = origprop;
	in = argv[1];
	while (*in != '\0') {
		*out++ = PROPDIR_DELIMITER;
		while (*in == PROPDIR_DELIMITER) in++;
		while (*in && *in != PROPDIR_DELIMITER) *out++ = *in++;
	}
	*out++ = '\0';

	i = targval;
	bestname[0] = '\0';
	bestval = 0;
	baselen = strlen(origprop);
	for (; obj != NOTHING; obj = getparent(obj)) {
		pname = next_prop_name(obj, propname, sizeof(propname), origprop);
		while (pname && string_prefix(pname, origprop)) {
			ptr = pname + baselen;
			if (*ptr == NUMBER_TOKEN) ptr++;
			if (!*ptr && is_propdir(obj, pname)) {
				char propname2[BUFFER_LEN];
				char *pname2;
				int sublen = strlen(pname);

				pname2 = strcpyn(propname2, sizeof(propname2), pname);
				propname2[sublen++] = PROPDIR_DELIMITER;
				propname2[sublen] = '\0';

				pname2 = next_prop_name(obj, propname2, sizeof(propname2), pname2);
				while (pname2) {
					ptr = pname2 + sublen;
					if (number(ptr)) {
						i = atoi(ptr);
						if (bestval < i && i <= targval) {
							bestval = i;
							bestobj = obj;
							strcpyn(bestname, sizeof(bestname), pname2);
						}
					}
					pname2 = next_prop_name(obj, propname2, sizeof(propname2), pname2);
				}
			}
			ptr = pname + baselen;
			if (number(ptr)) {
				i = atoi(ptr);
				if (bestval < i && i <= targval) {
					bestval = i;
					bestobj = obj;
					strcpyn(bestname, sizeof(bestname), pname);
				}
			}
			pname = next_prop_name(obj, propname, sizeof(propname), pname);
		}
	}
	
	if (*bestname) {
		ptr = safegetprop_strict(player, bestobj, perms, bestname, mesgtyp, &blessed);
		if (!ptr)
			ABORT_MPI("SELECT", "Failed property read.");
	} else {
		ptr = "";
	}
	return ptr;
}


const char *
mfn_list(MFUNARGS)
{
	dbref obj = what;
	char *pname;
	const char *ptr;
	int blessed;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("LIST", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("LIST", "Match failed.");
	ptr = get_concat_list(player, what, perms, obj, (char *)pname, buf, BUFFER_LEN, 0, mesgtyp, &blessed);
	if (!ptr)
		ptr = "";
	return ptr;
}


const char *
mfn_lexec(MFUNARGS)
{
	dbref trg = (dbref) 0, obj = what;
	char *pname;
	const char *ptr;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("LEXEC", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("LEXEC", "Match failed.");
	while (*pname == PROPDIR_DELIMITER)
		pname++;
	ptr = get_concat_list(player, what, perms, obj, (char *)pname, buf, BUFFER_LEN, 2, mesgtyp, &blessed);
	if (!ptr)
		ptr = "";
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname) || Prop_Hidden(pname))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "LEXEC", "listval");
	return ptr;
}



const char *
mfn_rand(MFUNARGS)
{
	int num = 0;
	dbref trg = (dbref) 0, obj = what;
	const char *pname, *ptr;
	int blessed = 0;

	pname = argv[0];
	if (argc == 2) {
		obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("RAND", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("RAND", "Match failed.");
	num = get_list_count(what, obj, perms, (char *)pname, mesgtyp, &blessed);
	if (!num)
		ABORT_MPI("RAND", "Failed list read.");
	ptr = get_list_item(what, obj, perms, (char *)pname, (((RANDOM() / 256) % num) + 1), mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("RAND", "Failed list read.");
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr) || Prop_Hidden(ptr))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "RAND", "listval");
	return ptr;
}


const char *
mfn_timesub(MFUNARGS)
{
	int num = 0;
	dbref trg = (dbref) 0, obj = what;
	const char *pname, *ptr;
	int period = 0, offset = 0;
	int blessed = 0;

	period = atoi(argv[0]);
	offset = atoi(argv[1]);
	pname = argv[2];
	if (argc == 4) {
		obj = mesg_dbref(descr, player, what, perms, argv[3], mesgtyp);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("TIMESUB", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("TIMESUB", "Match failed.");
	num = get_list_count(what, obj, perms, (char *)pname, mesgtyp, &blessed);
	if (!num)
		ABORT_MPI("TIMESUB", "Failed list read.");
	if (period < 1)
		ABORT_MPI("TIMESUB", "Time period too short.");
	offset = (int)((((long) time(NULL) + offset) % period) * num) / period;
	if (offset < 0)
		offset = -offset;
	ptr = get_list_item(what, obj, perms, (char *)pname, offset + 1, mesgtyp, &blessed);
	if (!ptr)
		ABORT_MPI("TIMESUB", "Failed list read.");
	trg = what;
	if (blessed) {
		mesgtyp |= MPI_ISBLESSED;
	} else {
		mesgtyp &= ~MPI_ISBLESSED;
	}
	if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr) || Prop_Hidden(ptr))
		trg = obj;
	ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "TIMESUB", "listval");
	return ptr;
}


const char *
mfn_nl(MFUNARGS)
{
	return "\r";
}


const char *
mfn_lit(MFUNARGS)
{
	int i, len, len2;

	strcpyn(buf, buflen, argv[0]);
	len = strlen(buf);
	for (i = 1; i < argc; i++) {
		len2 = strlen(argv[i]);
		if (len2 + len + 3 >= BUFFER_LEN) {
			if (len + 3 < BUFFER_LEN) {
				strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
				buf[BUFFER_LEN - 3] = '\0';
			}
			break;
		}
		strcpyn(buf + len, buflen - len, ",");
		strcpyn(buf + len, buflen - len, argv[i]);
		len += len2;
	}
	return buf;
}


const char *
mfn_eval(MFUNARGS)
{
	int i, len, len2;
	char buf2[BUFFER_LEN];
	char* ptr;

	strcpyn(buf, buflen, argv[0]);
	len = strlen(buf);
	for (i = 1; i < argc; i++) {
		len2 = strlen(argv[i]);
		if (len2 + len + 3 >= BUFFER_LEN) {
			if (len + 3 < BUFFER_LEN) {
				strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
				buf[BUFFER_LEN - 3] = '\0';
			}
			break;
		}
		strcpyn(buf + len, buflen - len, ",");
		strcpyn(buf + len, buflen - len, argv[i]);
		len += len2;
	}
	strcpyn(buf2, sizeof(buf2), buf);
	ptr = mesg_parse(descr, player, what, perms, buf2, buf, BUFFER_LEN, (mesgtyp & ~MPI_ISBLESSED));
	CHECKRETURN(ptr, "EVAL", "arg 1");
	return buf;
}


const char *
mfn_evalbang(MFUNARGS)
{
	int i, len, len2;
	char buf2[BUFFER_LEN];
	char* ptr;

	strcpyn(buf, buflen, argv[0]);
	len = strlen(buf);
	for (i = 1; i < argc; i++) {
		len2 = strlen(argv[i]);
		if (len2 + len + 3 >= BUFFER_LEN) {
			if (len + 3 < BUFFER_LEN) {
				strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
				buf[BUFFER_LEN - 3] = '\0';
			}
			break;
		}
		strcpyn(buf + len, buflen - len, ",");
		strcpyn(buf + len, buflen - len, argv[i]);
		len += len2;
	}
	strcpyn(buf2, sizeof(buf2), buf);
	ptr = mesg_parse(descr, player, what, perms, buf2, buf, BUFFER_LEN, mesgtyp);
	CHECKRETURN(ptr, "EVAL!", "arg 1");
	return buf;
}


const char *
mfn_strip(MFUNARGS)
{
	int i, len, len2;
	char *ptr;

	for (ptr = argv[0]; *ptr == ' '; ptr++) ;
	strcpyn(buf, buflen, ptr);
	len = strlen(buf);
	for (i = 1; i < argc; i++) {
		len2 = strlen(argv[i]);
		if (len2 + len + 3 >= BUFFER_LEN) {
			if (len + 3 < BUFFER_LEN) {
				strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
				buf[BUFFER_LEN - 3] = '\0';
			}
			break;
		}
		strcpyn(buf + len, buflen - len, ",");
		strcpyn(buf + len, buflen - len, argv[i]);
		len += len2;
	}
	ptr = &buf[strlen(buf) - 1];
	while (ptr >= buf && isspace(*ptr))
		*(ptr--) = '\0';
	return buf;
}


const char *
mfn_mklist(MFUNARGS)
{
	int i, len, tlen;
	int outcount = 0;

	tlen = 0;
	*buf = '\0';
	for (i = 0; i < argc; i++) {
		len = strlen(argv[i]);
		if (tlen + len + 2 < BUFFER_LEN) {
			if (outcount++)
				strcatn(buf, BUFFER_LEN, "\r");
			strcatn(buf, BUFFER_LEN, argv[i]);
			tlen += len;
		} else {
			ABORT_MPI("MKLIST", "Max string length exceeded.");
		}
	}
	return buf;
}


const char *
mfn_pronouns(MFUNARGS)
{
	dbref obj = player;

	if (argc > 1) {
		obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);
		if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
			ABORT_MPI("PRONOUNS", "Match failed.");
		if (obj == PERMDENIED)
			ABORT_MPI("PRONOUNS", "Permission Denied.");
	}
	return pronoun_substitute(descr, obj, argv[0]);
}


const char *
mfn_ontime(MFUNARGS)
{
	dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
	int conn;

	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		return "-1";
	if (obj == PERMDENIED)
		ABORT_MPI("ONTIME", "Permission denied.");
	if (Typeof(obj) != TYPE_PLAYER)
		obj = OWNER(obj);
	conn = least_idle_player_descr(obj);
	if (!conn)
		return "-1";
	snprintf(buf, BUFFER_LEN, "%d", pontime(conn));
	return buf;
}


const char *
mfn_idle(MFUNARGS)
{
	dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
	int conn;

	if (obj == PERMDENIED)
		ABORT_MPI("IDLE", "Permission denied.");
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		return "-1";
	if (Typeof(obj) != TYPE_PLAYER)
		obj = OWNER(obj);
	conn = least_idle_player_descr(obj);
	if (!conn)
		return "-1";
	snprintf(buf, BUFFER_LEN, "%d", pidle(conn));
	return buf;
}


const char *
mfn_online(MFUNARGS)
{
	int list_limit = MAX_MFUN_LIST_LEN;
	int count = pcount();
	char buf2[BUFFER_LEN];

	if (!(mesgtyp & MPI_ISBLESSED))
		ABORT_MPI("ONLINE", "Permission denied.");
	*buf = '\0';
	while (count && list_limit--) {
		if (*buf)
			strcatn(buf, BUFFER_LEN, "\r");
		ref2str(pdbref(count), buf2, sizeof(buf2));
		if ((strlen(buf) + strlen(buf2)) >= (BUFFER_LEN - 3))
			break;
		strcatn(buf, BUFFER_LEN, buf2);
		count--;
	}
	return buf;
}


int
msg_compare(const char *s1, const char *s2)
{
	if (*s1 && *s2 && number(s1) && number(s2)) {
		return (atoi(s1) - atoi(s2));
	} else {
		return string_compare(s1, s2);
	}
}


const char *
mfn_contains(MFUNARGS)
{
	dbref obj2 = mesg_dbref_raw(descr, player, what, perms, argv[0]);
	dbref obj1 = player;

	if (argc > 1)
		obj1 = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

	if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == HOME)
		ABORT_MPI("CONTAINS", "Match failed (1).");
	if (obj2 == PERMDENIED)
		ABORT_MPI("CONTAINS", "Permission Denied (1).");

	if (obj1 == UNKNOWN || obj1 == AMBIGUOUS || obj1 == NOTHING || obj1 == HOME)
		ABORT_MPI("CONTAINS", "Match failed (2).");
	if (obj1 == PERMDENIED)
		ABORT_MPI("CONTAINS", "Permission Denied (2).");

	while (obj2 != NOTHING && obj2 != obj1)
		obj2 = getloc(obj2);
	if (obj1 == obj2) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_holds(MFUNARGS)
{
	dbref obj1 = mesg_dbref_raw(descr, player, what, perms, argv[0]);
	dbref obj2 = player;

	if (argc > 1)
		obj2 = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

	if (obj1 == UNKNOWN || obj1 == AMBIGUOUS || obj1 == NOTHING || obj1 == HOME)
		ABORT_MPI("HOLDS", "Match failed (1).");
	if (obj1 == PERMDENIED)
		ABORT_MPI("HOLDS", "Permission Denied (1).");

	if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == HOME)
		ABORT_MPI("HOLDS", "Match failed (2).");
	if (obj2 == PERMDENIED)
		ABORT_MPI("HOLDS", "Permission Denied (2).");

	if (obj2 == getloc(obj1)) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_dbeq(MFUNARGS)
{
	dbref obj1 = mesg_dbref_raw(descr, player, what, perms, argv[0]);
	dbref obj2 = mesg_dbref_raw(descr, player, what, perms, argv[1]);

	if (obj1 == UNKNOWN || obj1 == PERMDENIED)
		ABORT_MPI("DBEQ", "Match failed (1).");
	if (obj2 == UNKNOWN || obj2 == PERMDENIED)
		ABORT_MPI("DBEQ", "Match failed (2).");

	if (obj1 == obj2) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_ne(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) == 0) {
		return "0";
	} else {
		return "1";
	}
}


const char *
mfn_eq(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) == 0) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_gt(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) > 0) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_lt(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) < 0) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_ge(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) >= 0) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_le(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) <= 0) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_min(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) <= 0) {
		return argv[0];
	} else {
		return argv[1];
	}
}


const char *
mfn_max(MFUNARGS)
{
	if (msg_compare(argv[0], argv[1]) >= 0) {
		return argv[0];
	} else {
		return argv[1];
	}
}


const char *
mfn_isnum(MFUNARGS)
{
	if (*argv[0] && number(argv[0])) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_isdbref(MFUNARGS)
{
	dbref obj;
	char *ptr = argv[0];

	while (isspace(*ptr))
		ptr++;
	if (*ptr++ != NUMBER_TOKEN)
		return "0";
	if (!number(ptr))
		return "0";
	obj = (dbref) atoi(ptr);
	if (obj < 0 || obj >= db_top)
		return "0";
	if (Typeof(obj) == TYPE_GARBAGE)
		return "0";
	return "1";
}



const char *
mfn_inc(MFUNARGS)
{
	int x = 1;
	char *ptr = get_mvar(argv[0]);

	if (!ptr)
		ABORT_MPI("INC", "No such variable currently defined.");
	if (argc > 1)
		x = atoi(argv[1]);
	snprintf(buf, BUFFER_LEN, "%d", (atoi(ptr) + x));
	strcpyn(ptr, BUFFER_LEN, buf);
	return (buf);
}


const char *
mfn_dec(MFUNARGS)
{
	int x = 1;
	char *ptr = get_mvar(argv[0]);

	if (!ptr)
		ABORT_MPI("DEC", "No such variable currently defined.");
	if (argc > 1)
		x = atoi(argv[1]);
	snprintf(buf, BUFFER_LEN, "%d", (atoi(ptr) - x));
	strcpyn(ptr, BUFFER_LEN, buf);
	return (buf);
}


const char *
mfn_add(MFUNARGS)
{
	int j, i = atoi(argv[0]);

	for (j = 1; j < argc; j++)
		i += atoi(argv[j]);
	snprintf(buf, BUFFER_LEN, "%d", i);
	return (buf);
}


const char *
mfn_subt(MFUNARGS)
{
	int j, i = atoi(argv[0]);

	for (j = 1; j < argc; j++)
		i -= atoi(argv[j]);
	snprintf(buf, BUFFER_LEN, "%d", i);
	return (buf);
}


const char *
mfn_mult(MFUNARGS)
{
	int j, i = atoi(argv[0]);

	for (j = 1; j < argc; j++)
		i *= atoi(argv[j]);
	snprintf(buf, BUFFER_LEN, "%d", i);
	return (buf);
}


const char *
mfn_div(MFUNARGS)
{
	int k, j, i = atoi(argv[0]);

	for (j = 1; j < argc; j++) {
		k = atoi(argv[j]);
		if (!k) {
			i = 0;
		} else {
			i /= k;
		}
	}
	snprintf(buf, BUFFER_LEN, "%d", i);
	return (buf);
}


const char *
mfn_mod(MFUNARGS)
{
	int k, j, i = atoi(argv[0]);

	for (j = 1; j < argc; j++) {
		k = atoi(argv[j]);
		if (!k) {
			i = 0;
		} else {
			i %= k;
		}
	}
	snprintf(buf, BUFFER_LEN, "%d", i);
	return (buf);
}



const char *
mfn_abs(MFUNARGS)
{
	int val = atoi(argv[0]);

	if (val == 0) {
		return "0";
	}
	if (val < 0) {
		val = -val;
	}
	snprintf(buf, BUFFER_LEN, "%d", val);
	return (buf);
}


const char *
mfn_sign(MFUNARGS)
{
	int val;

	val = atoi(argv[0]);
	if (val == 0) {
		return "0";
	} else if (val < 0) {
		return "-1";
	} else {
		return "1";
	}
}



const char *
mfn_dist(MFUNARGS)
{
	int a, b, c;
	int a2, b2, c2;
	double result;

	a2 = b2 = c = c2 = 0;
	a = atoi(argv[0]);
	b = atoi(argv[1]);
	if (argc == 3) {
		c = atoi(argv[2]);
	} else if (argc == 4) {
		a2 = atoi(argv[2]);
		b2 = atoi(argv[3]);
	} else if (argc == 6) {
		c = atoi(argv[2]);
		a2 = atoi(argv[3]);
		b2 = atoi(argv[4]);
		c2 = atoi(argv[5]);
	} else if (argc != 2) {
		ABORT_MPI("DIST", "Takes 2,3,4, or 6 arguments.");
	}
	a -= a2;
	b -= b2;
	c -= c2;
	result = sqrt((double) (a * a) + (double) (b * b) + (double) (c * c));

	snprintf(buf, BUFFER_LEN, "%.0f", floor(result + 0.5));
	return buf;
}


const char *
mfn_not(MFUNARGS)
{
	if (truestr(argv[0])) {
		return "0";
	} else {
		return "1";
	}
}


const char *
mfn_or(MFUNARGS)
{
	char *ptr;
	char buf2[16];
	int i;

	for (i = 0; i < argc; i++) {
		ptr = MesgParse(argv[i], buf, buflen);
		snprintf(buf2, sizeof(buf2), "arg %d", i + 1);
		CHECKRETURN(ptr, "OR", buf2);
		if (truestr(ptr)) {
			return "1";
		}
	}
	return "0";
}


const char *
mfn_xor(MFUNARGS)
{
	if (truestr(argv[0]) && !truestr(argv[1])) {
		return "1";
	}
	if (!truestr(argv[0]) && truestr(argv[1])) {
		return "1";
	}
	return "0";
}


const char *
mfn_and(MFUNARGS)
{
	char *ptr;
	char buf2[16];
	int i;

	for (i = 0; i < argc; i++) {
		ptr = MesgParse(argv[i], buf, buflen);
		snprintf(buf2, sizeof(buf2), "arg %d", i + 1);
		CHECKRETURN(ptr, "AND", buf2);
		if (!truestr(ptr)) {
			return "0";
		}
	}
	return "1";
}


const char *
mfn_dice(MFUNARGS)
{
	int num = 1;
	int sides = 1;
	int offset = 0;
	int total = 0;

	if (argc >= 3)
		offset = atoi(argv[2]);
	if (argc >= 2)
		num = atoi(argv[1]);
	sides = atoi(argv[0]);
	if (num > 8888)
		ABORT_MPI("DICE", "Too many dice!");
	if (sides == 0)
		return "0";
	while (num-- > 0)
		total += (((RANDOM() / 256) % sides) + 1);
	snprintf(buf, BUFFER_LEN, "%d", (total + offset));
	return buf;
}


const char *
mfn_default(MFUNARGS)
{
	char *ptr;

	*buf = '\0';
	ptr = MesgParse(argv[0], buf, buflen);
	CHECKRETURN(ptr, "DEFAULT", "arg 1");
	if (ptr && truestr(buf)) {
		if (!ptr)
			ptr = "";
	} else {
		ptr = MesgParse(argv[1], buf, buflen);
		CHECKRETURN(ptr, "DEFAULT", "arg 2");
	}
	return ptr;
}


const char *
mfn_if(MFUNARGS)
{
	char *fbr, *ptr;

	if (argc == 3) {
		fbr = argv[2];
	} else {
		fbr = "";
	}
	ptr = MesgParse(argv[0], buf, buflen);
	CHECKRETURN(ptr, "IF", "arg 1");
	if (ptr && truestr(buf)) {
		ptr = MesgParse(argv[1], buf, buflen);
		CHECKRETURN(ptr, "IF", "arg 2");
	} else if (*fbr) {
		ptr = MesgParse(fbr, buf, buflen);
		CHECKRETURN(ptr, "IF", "arg 3");
	} else {
		*buf = '\0';
		ptr = "";
	}
	return ptr;
}


const char *
mfn_while(MFUNARGS)
{
	int iter_limit = MAX_MFUN_LIST_LEN;
	char buf2[BUFFER_LEN];
	char *ptr;

	*buf = '\0';
	while (1) {
		ptr = MesgParse(argv[0], buf2, sizeof(buf2));
		CHECKRETURN(ptr, "WHILE", "arg 1");
		if (!truestr(ptr))
			break;
		ptr = MesgParse(argv[1], buf, buflen);
		CHECKRETURN(ptr, "WHILE", "arg 2");
		if (!(--iter_limit))
			ABORT_MPI("WHILE", "Iteration limit exceeded");
	}
	return buf;
}


const char *
mfn_null(MFUNARGS)
{
	return "";
}


const char *
mfn_tzoffset(MFUNARGS)
{
	snprintf(buf, BUFFER_LEN, "%ld", get_tz_offset());
	return buf;
}


const char *
mfn_time(MFUNARGS)
{
	time_t lt;
	struct tm *tm;

	lt = time((time_t*) NULL);
	if (argc == 1) {
		lt += (3600 * atoi(argv[0]));
		lt += get_tz_offset();
	}
#ifndef WIN32
	tm = localtime(&lt);
#else
	tm = uw32localtime(&lt);
#endif
	format_time(buf, BUFFER_LEN - 1, "%T", tm);
	return buf;
}


const char *
mfn_date(MFUNARGS)
{
	time_t lt;
	struct tm *tm;

	lt = time((time_t*) NULL);
	if (argc == 1) {
		lt += (3600 * atoi(argv[0]));
		lt += get_tz_offset();
	}
#ifndef WIN32
	tm = localtime(&lt);
#else
	tm = uw32localtime(&lt);
#endif
	format_time(buf, BUFFER_LEN - 1, "%D", tm);
	return buf;
}


const char *
mfn_ftime(MFUNARGS)
{
	time_t lt;
	struct tm *tm;

	if (argc == 3) {
		lt = atol(argv[2]);
	} else {
		time(&lt);
	}
	if (argc > 1 && *argv[1]) {
		int offval = atoi(argv[1]);
		if (offval < 25 && offval > -25) {
			lt += 3600 * offval;
		} else {
			lt -= offval;
		}
		lt += get_tz_offset();
	}
#ifndef WIN32
	tm = localtime(&lt);
#else
	tm = uw32localtime(&lt);
#endif
	format_time(buf, BUFFER_LEN - 1, argv[0], tm);
	return buf;
}


const char *
mfn_convtime(MFUNARGS)
{
	struct tm otm;
	int mo, dy, yr, hr, mn, sc;

	yr = 70;
	mo = dy = 1;
	hr = mn = sc = 0;
	if (sscanf(argv[0], "%d:%d:%d %d/%d/%d", &hr, &mn, &sc, &mo, &dy, &yr) != 6)
		ABORT_MPI("CONVTIME", "Needs HH:MM:SS MO/DY/YR time string format.");
	if (hr < 0 || hr > 23)
		ABORT_MPI("CONVTIME", "Bad Hour");
	if (mn < 0 || mn > 59)
		ABORT_MPI("CONVTIME", "Bad Minute");
	if (sc < 0 || sc > 59)
		ABORT_MPI("CONVTIME", "Bad Second");
	if (yr < 0 || yr > 99)
		ABORT_MPI("CONVTIME", "Bad Year");
	if (mo < 1 || mo > 12)
		ABORT_MPI("CONVTIME", "Bad Month");
	if (dy < 1 || dy > 31)
		ABORT_MPI("CONVTIME", "Bad Day");
	otm.tm_mon = mo - 1;
	otm.tm_mday = dy;
	otm.tm_hour = hr;
	otm.tm_min = mn;
	otm.tm_sec = sc;
	otm.tm_year = (yr >= 70) ? yr : (yr + 100);
#ifdef SUNOS
	snprintf(buf, BUFFER_LEN, "%ld", timelocal(&otm));
#else
	snprintf(buf, BUFFER_LEN, "%ld", mktime(&otm));
#endif
	return buf;
}


const char *
mfn_ltimestr(MFUNARGS)
{
	int tm = atol(argv[0]);
	int yr, mm, wk, dy, hr, mn;
	char buf2[BUFFER_LEN];

	yr = mm = wk = dy = hr = mn = 0;
	if (tm >= 31556736) {
		yr = tm / 31556736;	        /* Years */
		tm %= 31556736;
	}
	if (tm >= 2621376) {
		mm = tm / 2621376;	        /* Months */
		tm %= 2621376;
	}
	if (tm >= 604800) {
		wk = tm / 604800;	        /* Weeks */
		tm %= 604800;
	}
	if (tm >= 86400) {
		dy = tm / 86400;		/* Days */
		tm %= 86400;
	}
	if (tm >= 3600) {
		hr = tm / 3600;			/* Hours */
		tm %= 3600;
	}
	if (tm >= 60) {
		mn = tm / 60;			/* Minutes */
		tm %= 60;			/* Seconds */
	}

	*buf = '\0';
	if (yr) {
		snprintf(buf, BUFFER_LEN, "%d year%s", yr, (yr == 1) ? "" : "s");
	}
	if (mm) {
		snprintf(buf2, BUFFER_LEN, "%d month%s", mm, (mm == 1) ? "" : "s");
		if (*buf) {
			strcatn(buf, BUFFER_LEN, ", ");
		}
		strcatn(buf, BUFFER_LEN, buf2);
	}
	if (wk) {
		snprintf(buf2, BUFFER_LEN, "%d week%s", wk, (wk == 1) ? "" : "s");
		if (*buf) {
			strcatn(buf, BUFFER_LEN, ", ");
		}
		strcatn(buf, BUFFER_LEN, buf2);
	}
	if (dy) {
		snprintf(buf2, BUFFER_LEN, "%d day%s", dy, (dy == 1) ? "" : "s");
		if (*buf) {
			strcatn(buf, BUFFER_LEN, ", ");
		}
		strcatn(buf, BUFFER_LEN, buf2);
	}
	if (hr) {
		snprintf(buf2, BUFFER_LEN, "%d hour%s", hr, (hr == 1) ? "" : "s");
		if (*buf) {
			strcatn(buf, BUFFER_LEN, ", ");
		}
		strcatn(buf, BUFFER_LEN, buf2);
	}
	if (mn) {
		snprintf(buf2, BUFFER_LEN, "%d min%s", mn, (mn == 1) ? "" : "s");
		if (*buf) {
			strcatn(buf, BUFFER_LEN, ", ");
		}
		strcatn(buf, BUFFER_LEN, buf2);
	}
	if (tm || !*buf) {
		snprintf(buf2, BUFFER_LEN, "%d sec%s", tm, (tm == 1) ? "" : "s");
		if (*buf) {
			strcatn(buf, BUFFER_LEN, ", ");
		}
		strcatn(buf, BUFFER_LEN, buf2);
	}
	return buf;
}


const char *
mfn_timestr(MFUNARGS)
{
	int tm = atol(argv[0]);
	int dy, hr, mn;

	dy = hr = mn = 0;
	if (tm >= 86400) {
		dy = tm / 86400;		/* Days */
		tm %= 86400;
	}
	if (tm >= 3600) {
		hr = tm / 3600;			/* Hours */
		tm %= 3600;
	}
	if (tm >= 60) {
		mn = tm / 60;			/* Minutes */
		tm %= 60;				/* Seconds */
	}

	*buf = '\0';
	if (dy) {
		snprintf(buf, BUFFER_LEN, "%dd %02d:%02d", dy, hr, mn);
	} else {
		snprintf(buf, BUFFER_LEN, "%02d:%02d", hr, mn);
	}
	return buf;
}


const char *
mfn_stimestr(MFUNARGS)
{
	int tm = atol(argv[0]);
	int dy, hr, mn;

	dy = hr = mn = 0;
	if (tm >= 86400) {
		dy = tm / 86400;		/* Days */
		tm %= 86400;
	}
	if (tm >= 3600) {
		hr = tm / 3600;			/* Hours */
		tm %= 3600;
	}
	if (tm >= 60) {
		mn = tm / 60;			/* Minutes */
		tm %= 60;				/* Seconds */
	}

	*buf = '\0';
	if (dy) {
		snprintf(buf, BUFFER_LEN, "%dd", dy);
		return buf;
	}
	if (hr) {
		snprintf(buf, BUFFER_LEN, "%dh", hr);
		return buf;
	}
	if (mn) {
		snprintf(buf, BUFFER_LEN, "%dm", mn);
		return buf;
	}
	snprintf(buf, BUFFER_LEN, "%ds", tm);
	return buf;
}


const char *
mfn_secs(MFUNARGS)
{
	time_t lt;

	time(&lt);
	snprintf(buf, BUFFER_LEN, "%ld", lt);
	return buf;
}


const char *
mfn_convsecs(MFUNARGS)
{
	time_t lt;

	lt = atol(argv[0]);
	snprintf(buf, BUFFER_LEN, "%s", ctime(&lt));
	buf[strlen(buf) - 1] = '\0';
	return buf;
}


const char *
mfn_loc(MFUNARGS)
{
	dbref obj;

	obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("LOC", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("LOC", "Permission denied.");
	return ref2str(getloc(obj), buf, BUFFER_LEN);
}


const char *
mfn_nearby(MFUNARGS)
{
	dbref obj;
	dbref obj2;

	obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING)
		ABORT_MPI("NEARBY", "Match failed (arg1).");
	if (obj == PERMDENIED)
		ABORT_MPI("NEARBY", "Permission denied (arg1).");
	if (obj == HOME)
		obj = PLAYER_HOME(player);
	if (argc > 1) {
		obj2 = mesg_dbref_raw(descr, player, what, perms, argv[1]);
		if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING)
			ABORT_MPI("NEARBY", "Match failed (arg2).");
		if (obj2 == PERMDENIED)
			ABORT_MPI("NEARBY", "Permission denied (arg2).");
		if (obj2 == HOME)
			obj2 = PLAYER_HOME(player);
	} else {
		obj2 = what;
	}
	if (!(mesgtyp & MPI_ISBLESSED) && !isneighbor(obj, what) && !isneighbor(obj2, what) &&
		!isneighbor(obj, player) && !isneighbor(obj2, player)
			) {
		ABORT_MPI("NEARBY", "Permission denied.  Neither object is local.");
	}
	if (isneighbor(obj, obj2)) {
		return "1";
	} else {
		return "0";
	}
}


const char *
mfn_money(MFUNARGS)
{
	dbref obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("MONEY", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("MONEY", "Permission denied.");
	if (tp_pennies_muf_mlev > 1 && !(mesgtyp & MPI_ISBLESSED))
		ABORT_MPI("MONEY", "Permission denied.");
	switch (Typeof(obj)) {
	case TYPE_THING:
		snprintf(buf, BUFFER_LEN, "%d", GETVALUE(obj));
		break;
	case TYPE_PLAYER:
		snprintf(buf, BUFFER_LEN, "%d", GETVALUE(obj));
		break;
	default:
		strcpyn(buf, buflen, "0");
		break;
	}
	return buf;
}


const char *
mfn_flags(MFUNARGS)
{
	dbref obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("FLAGS", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("FLAGS", "Permission denied.");
	return unparse_flags(obj);
}

const char *
mfn_tell(MFUNARGS)
{
	char buf2[BUFFER_LEN];
	char *ptr, *ptr2;
	dbref obj = player;

	if (argc > 1)
		obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("TELL", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("TELL", "Permission denied.");
	if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
		ABORT_MPI("TELL", "Permission denied.");
	*buf = '\0';
	strcpyn(buf2, sizeof(buf2), argv[0]);
	for (ptr = buf2; (ptr != NULL) && *ptr != '\0'; ptr = ptr2) {
		ptr2 = index(ptr, '\r');
		if (ptr2 != NULL) {
			*ptr2++ = '\0';
		} else {
			ptr2 = ptr + strlen(ptr);
		}
		if (Typeof(what) == TYPE_ROOM || OWNER(what) == obj || player == obj ||
			(Typeof(what) == TYPE_EXIT && Typeof(getloc(what)) == TYPE_ROOM) ||
			string_prefix(argv[0], NAME(player))) {
			snprintf(buf, BUFFER_LEN, "%s%.4093s",
					((obj == OWNER(perms) || obj == player) ? "" : "> "), ptr);
		} else {
			snprintf(buf, BUFFER_LEN, "%s%.16s%s%.4078s",
					((obj == OWNER(perms) || obj == player) ? "" : "> "),
					NAME(player), ((*argv[0] == '\'' || isspace(*argv[0])) ? "" : " "), ptr);
		}
		notify_from_echo(player, obj, buf, 1);
	}
	return argv[0];
}

const char *
mfn_otell(MFUNARGS)
{
	char buf2[BUFFER_LEN];
	char *ptr, *ptr2;
	dbref obj = getloc(player);
	dbref eobj = player;
	dbref thing;

	if (argc > 1)
		obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("OTELL", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("OTELL", "Permission denied.");
	if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
		ABORT_MPI("OTELL", "Permission denied.");
	if (argc > 2)
		eobj = mesg_dbref_raw(descr, player, what, perms, argv[2]);
	strcpyn(buf2, sizeof(buf2), argv[0]);
	for (ptr = buf2; *ptr; ptr = ptr2) {
		ptr2 = index(ptr, '\r');
		if (ptr2) {
			*ptr2 = '\0';
		} else {
			ptr2 = ptr + strlen(ptr);
		}
		if (((OWNER(what) == OWNER(obj) || isancestor(what, obj)) &&
			 (Typeof(what) == TYPE_ROOM ||
			  (Typeof(what) == TYPE_EXIT && Typeof(getloc(what)) == TYPE_ROOM))) ||
			string_prefix(argv[0], NAME(player))) {
			strcpyn(buf, buflen, ptr);
		} else {
			snprintf(buf, BUFFER_LEN, "%.16s%s%.4078s", NAME(player),
					((*argv[0] == '\'' || isspace(*argv[0])) ? "" : " "), ptr);
		}
		thing = DBFETCH(obj)->contents;
		while (thing != NOTHING) {
			if (thing != eobj) {
				notify_from_echo(player, thing, buf, 0);
			}
			thing = DBFETCH(thing)->next;
		}
	}
	return argv[0];
}


const char *
mfn_right(MFUNARGS)
{
	/* {right:string,fieldwidth,padstr} */
	/* Right justify string to a fieldwidth, filling with padstr */

	char *ptr;
	char *fptr;
	int i, len;
	char *fillstr;

	len = (argc > 1) ? atoi(argv[1]) : 78;
	if (len > BUFFER_LEN - 1)
		ABORT_MPI("RIGHT", "Fieldwidth too big.");
	fillstr = (argc > 2) ? argv[2] : " ";
	if (!*fillstr)
		ABORT_MPI("RIGHT", "Null pad string.");
	for (ptr = buf, fptr = fillstr, i = strlen(argv[0]); i < len; i++) {
		*ptr++ = *fptr++;
		if (!*fptr)
			fptr = fillstr;
	}
	strcpyn(ptr, buflen - (ptr - buf), argv[0]);
	return buf;
}


const char *
mfn_left(MFUNARGS)
{
	/* {left:string,fieldwidth,padstr} */
	/* Left justify string to a fieldwidth, filling with padstr */

	char *ptr;
	char *fptr;
	int i, len;
	char *fillstr;

	len = (argc > 1) ? atoi(argv[1]) : 78;
	if (len > BUFFER_LEN - 1)
		ABORT_MPI("LEFT", "Fieldwidth too big.");
	fillstr = (argc > 2) ? argv[2] : " ";
	if (!*fillstr)
		ABORT_MPI("LEFT", "Null pad string.");
	strcpyn(buf, buflen, argv[0]);
	for (i = strlen(argv[0]), ptr = &buf[i], fptr = fillstr; i < len; i++) {
		*ptr++ = *fptr++;
		if (!*fptr)
			fptr = fillstr;
	}
	*ptr = '\0';
	return buf;
}


const char *
mfn_center(MFUNARGS)
{
	/* {center:string,fieldwidth,padstr} */
	/* Center justify string to a fieldwidth, filling with padstr */

	char *ptr;
	char *fptr;
	int i, len, halflen;
	char *fillstr;

	len = (argc > 1) ? atoi(argv[1]) : 78;
	if (len > BUFFER_LEN - 1)
		ABORT_MPI("CENTER", "Fieldwidth too big.");
	halflen = len / 2;

	fillstr = (argc > 2) ? argv[2] : " ";
	if (!*fillstr)
		ABORT_MPI("CENTER", "Null pad string.");

	for (ptr = buf, fptr = fillstr, i = strlen(argv[0]) / 2; i < halflen; i++) {
		*ptr++ = *fptr++;
		if (!*fptr)
			fptr = fillstr;
	}
	strcpyn(ptr, buflen - (ptr - buf), argv[0]);
	for (i = strlen(buf), ptr = &buf[i], fptr = fillstr; i < len; i++) {
		*ptr++ = *fptr++;
		if (!*fptr)
			fptr = fillstr;
	}
	*ptr = '\0';
	return buf;
}


const char *
mfn_created(MFUNARGS)
{
	dbref obj;

	obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("CREATED", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("CREATED", "Permission denied.");

	snprintf(buf, BUFFER_LEN, "%ld", DBFETCH(obj)->ts.created);

	return buf;
}


const char *
mfn_lastused(MFUNARGS)
{
	dbref obj;

	obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("LASTUSED", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("LASTUSED", "Permission denied.");

	snprintf(buf, BUFFER_LEN, "%ld", DBFETCH(obj)->ts.lastused);

	return buf;
}


const char *
mfn_modified(MFUNARGS)
{
	dbref obj;

	obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("MODIFIED", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("MODIFIED", "Permission denied.");

	snprintf(buf, BUFFER_LEN, "%ld", DBFETCH(obj)->ts.modified);

	return buf;
}


const char *
mfn_usecount(MFUNARGS)
{
	dbref obj;

	obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("USECOUNT", "Match failed.");
	if (obj == PERMDENIED)
		ABORT_MPI("USECOUNT", "Permission denied.");

	snprintf(buf, BUFFER_LEN, "%d", DBFETCH(obj)->ts.usecount);

	return buf;
}
static const char *mfuns_c_version = "$RCSfile: mfuns.c,v $ $Revision: 1.36 $";
const char *get_mfuns_c_version(void) { return mfuns_c_version; }
