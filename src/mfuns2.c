#include "config.h"

#include "boolexp.h"
#include "color.h"
#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "game.h"
#include "interp.h"
#include "interface.h"
#include "mfun.h"
#include "mpi.h"
#include "msgparse.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

const char *
mfn_owner(MFUNARGS)
{
    dbref obj;

    obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    if (obj == AMBIGUOUS || obj == NOTHING || obj == UNKNOWN)
	ABORT_MPI("OWNER", "Failed match.");
    if (obj == PERMDENIED)
	ABORT_MPI("OWNER", "Permission denied.");
    if (obj == HOME)
	obj = PLAYER_HOME(player);
    return ref2str(OWNER(obj), buf, BUFFER_LEN);
}


const char *
mfn_controls(MFUNARGS)
{

    dbref obj;
    dbref obj2;

    obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    if (obj == AMBIGUOUS || obj == NOTHING || obj == UNKNOWN)
	ABORT_MPI("CONTROLS", "Match failed. (arg1)");
    if (obj == PERMDENIED)
	ABORT_MPI("CONTROLS", "Permission denied. (arg1)");
    if (obj == HOME)
	obj = PLAYER_HOME(player);
    if (argc > 1) {
	obj2 = mesg_dbref_raw(descr, player, what, perms, argv[1]);
	if (obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == UNKNOWN)
	    ABORT_MPI("CONTROLS", "Match failed. (arg2)");
	if (obj2 == PERMDENIED)
	    ABORT_MPI("CONTROLS", "Permission denied. (arg2)");
	if (obj2 == HOME)
	    obj2 = PLAYER_HOME(player);
	obj2 = OWNER(obj2);
    } else {
	obj2 = OWNER(perms);
    }
    if (controls(obj2, obj)) {
	return "1";
    } else {
	return "0";
    }
}


const char *
mfn_links(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    dbref obj;
    int cnt;

    obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);
    if (obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING || obj == HOME)
	ABORT_MPI("LINKS", "Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("LINKS", "Permission denied.");
    switch (Typeof(obj)) {
    case TYPE_ROOM:
	obj = DBFETCH(obj)->sp.room.dropto;
	break;
    case TYPE_PLAYER:
	obj = PLAYER_HOME(obj);
	break;
    case TYPE_THING:
	obj = THING_HOME(obj);
	break;
    case TYPE_EXIT:
	*buf = '\0';
	cnt = DBFETCH(obj)->sp.exit.ndest;
	if (cnt) {
	    dbref obj2;

	    for (int i = 0; i < cnt; i++) {
		obj2 = DBFETCH(obj)->sp.exit.dest[i];
		ref2str(obj2, buf2, sizeof(buf2));
		if (strlen(buf) + strlen(buf2) + 2 < BUFFER_LEN) {
		    if (*buf)
			strcatn(buf, BUFFER_LEN, "\r");
		    strcatn(buf, BUFFER_LEN, buf2);
		}
	    }
	    return buf;
	} else {
	    return "#-1";
	}
    case TYPE_PROGRAM:
    default:
	return "#-1";
    }
    return ref2str(obj, buf, BUFFER_LEN);
}


const char *
mfn_locked(MFUNARGS)
{
    dbref who = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);
    dbref obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

    if (who == AMBIGUOUS || who == UNKNOWN || who == NOTHING || who == HOME)
	ABORT_MPI("LOCKED", "Match failed. (arg1)");
    if (who == PERMDENIED)
	ABORT_MPI("LOCKED", "Permission denied. (arg1)");
    if (obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING || obj == HOME)
	ABORT_MPI("LOCKED", "Match failed. (arg2)");
    if (obj == PERMDENIED)
	ABORT_MPI("LOCKED", "Permission denied. (arg2)");
    snprintf(buf, BUFFER_LEN, "%d", !could_doit(descr, who, obj));
    return buf;
}


const char *
mfn_testlock(MFUNARGS)
{
    struct boolexp *lok;
    dbref who = player;
    dbref obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

    if (argc > 2) {
	who = mesg_dbref_local(descr, player, what, perms, argv[2], mesgtyp);
    }
    if (who == AMBIGUOUS || who == UNKNOWN || who == NOTHING || who == HOME)
        ABORT_MPI("TESTLOCK", "Match failed. (arg3)");
    if (who == PERMDENIED)
        ABORT_MPI("TESTLOCK", "Permission denied. (arg3)");
    if (Typeof(who) != TYPE_PLAYER && Typeof(who) != TYPE_THING)
        ABORT_MPI("TESTLOCK", "Invalid object type. (arg3)");
    if (obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING || obj == HOME)
	ABORT_MPI("TESTLOCK", "Match failed. (arg1)");
    if (obj == PERMDENIED)
	ABORT_MPI("TESTLOCK", "Permission denied. (arg1)");
    if (Prop_System(argv[1]))
	ABORT_MPI("TESTLOCK", "Permission denied. (arg1)");
    if (!(mesgtyp & MPI_ISBLESSED)) {
	if (Prop_Hidden(argv[1]))
	    ABORT_MPI("TESTLOCK", "Permission denied. (arg2)");
	if (Prop_Private(argv[1]) && OWNER(perms) != OWNER(what))
	    ABORT_MPI("TESTLOCK", "Permission denied. (arg2)");
    }
    lok = get_property_lock(obj, argv[1]);
    if (argc > 3 && lok == TRUE_BOOLEXP)
	return (argv[3]);
    if (eval_boolexp(descr, who, lok, obj)) {
	return "1";
    } else {
	return "0";
    }
}


const char *
mfn_contents(MFUNARGS)
{
    char buf2[50];
    int list_limit = MAX_MFUN_LIST_LEN;
    dbref obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);
    int typchk, ownroom;
    int outlen, nextlen;

    if (obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING || obj == HOME)
	ABORT_MPI("CONTENTS", "Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("CONTENTS", "Permission denied.");

    typchk = NOTYPE;
    if (argc > 1) {
	if (!strcasecmp(argv[1], "Room")) {
	    typchk = TYPE_ROOM;
	} else if (!strcasecmp(argv[1], "Exit")) {
	    typchk = TYPE_EXIT;	/* won't find any, though */
	} else if (!strcasecmp(argv[1], "Player")) {
	    typchk = TYPE_PLAYER;
	} else if (!strcasecmp(argv[1], "Program")) {
	    typchk = TYPE_PROGRAM;
	} else if (!strcasecmp(argv[1], "Thing")) {
	    typchk = TYPE_THING;
	} else {
	    ABORT_MPI("CONTENTS",
		      "Type must be 'player', 'room', 'thing', 'program', or 'exit'. (arg2).");
	}
    }
    strcpyn(buf, buflen, "");
    outlen = 0;
    ownroom = controls(perms, obj);
    obj = CONTENTS(obj);
    while (obj != NOTHING && list_limit) {
	if ((typchk == NOTYPE || Typeof(obj) == typchk) &&
	    (ownroom || controls(perms, obj) ||
	     !((FLAGS(obj) & DARK) || (FLAGS(LOCATION(obj)) & DARK) ||
	       (Typeof(obj) == TYPE_PROGRAM && !(FLAGS(obj) & LINK_OK)))) &&
	    !(Typeof(obj) == TYPE_ROOM && typchk != TYPE_ROOM)) {
	    ref2str(obj, buf2, sizeof(buf2));
	    nextlen = strlen(buf2);
	    if ((outlen + nextlen) >= (BUFFER_LEN - 3))
		break;
	    if (outlen) {
		strcatn(buf + outlen, BUFFER_LEN - outlen, "\r");
		outlen++;
	    }
	    strcatn((buf + outlen), BUFFER_LEN - outlen, buf2);
	    outlen += nextlen;
	    list_limit--;
	}
	obj = NEXTOBJ(obj);
    }
    return buf;
}



const char *
mfn_exits(MFUNARGS)
{
    int outlen, nextlen;
    char buf2[50];
    int list_limit = MAX_MFUN_LIST_LEN;
    dbref obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING || obj == HOME)
	ABORT_MPI("EXITS", "Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("EXITS", "Permission denied.");

    switch (Typeof(obj)) {
    case TYPE_ROOM:
    case TYPE_THING:
    case TYPE_PLAYER:
	obj = EXITS(obj);
	break;
    default:
	obj = NOTHING;
	break;
    }
    *buf = '\0';
    outlen = 0;
    while (obj != NOTHING && list_limit) {
	ref2str(obj, buf2, sizeof(buf2));
	nextlen = strlen(buf2);
	if ((outlen + nextlen) >= (BUFFER_LEN - 3))
	    break;
	if (outlen) {
	    strcatn(buf + outlen, BUFFER_LEN - outlen, "\r");
	    outlen++;
	}
	strcatn((buf + outlen), BUFFER_LEN - outlen, buf2);
	outlen += nextlen;
	list_limit--;
	obj = NEXTOBJ(obj);
    }
    return buf;
}


const char *
mfn_v(MFUNARGS)
{
    char *ptr = get_mvar(argv[0]);

    if (!ptr)
	ABORT_MPI("V", "No such variable defined.");
    return ptr;
}


const char *
mfn_set(MFUNARGS)
{
    char *ptr = get_mvar(argv[0]);

    if (!ptr)
	ABORT_MPI("SET", "No such variable currently defined.");
    strcpyn(ptr, BUFFER_LEN, argv[1]);
    return ptr;
}


const char *
mfn_ref(MFUNARGS)
{
    dbref obj;
    const char *p = argv[0];

    skip_whitespace(&p);
    if (*p == NUMBER_TOKEN && number(p + 1)) {
	obj = atoi(p + 1);
    } else {
	obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);
	if (obj == PERMDENIED)
	    ABORT_MPI("REF", "Permission denied.");
	if (obj == UNKNOWN)
	    obj = NOTHING;
    }
    snprintf(buf, BUFFER_LEN, "#%d", obj);
    return buf;
}


const char *
mfn_name(MFUNARGS)
{
    char *ptr;
    dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);

    if (obj == UNKNOWN)
	ABORT_MPI("NAME", "Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("NAME", "Permission denied.");
    if (obj == NOTHING) {
	strcpyn(buf, buflen, "#NOTHING#");
	return buf;
    }
    if (obj == AMBIGUOUS) {
	strcpyn(buf, buflen, "#AMBIGUOUS#");
	return buf;
    }
    if (obj == HOME) {
	strcpyn(buf, buflen, "#HOME#");
	return buf;
    }
    strcpyn(buf, buflen, NAME(obj));
    if (Typeof(obj) == TYPE_EXIT) {
	ptr = index(buf, EXIT_DELIMITER);
	if (ptr)
	    *ptr = '\0';
    }
    return buf;
}


const char *
mfn_fullname(MFUNARGS)
{
    dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);

    if (obj == UNKNOWN)
	ABORT_MPI("NAME", "Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("NAME", "Permission denied.");
    if (obj == NOTHING) {
	strcpyn(buf, buflen, "#NOTHING#");
	return buf;
    }
    if (obj == AMBIGUOUS) {
	strcpyn(buf, buflen, "#AMBIGUOUS#");
	return buf;
    }
    if (obj == HOME) {
	strcpyn(buf, buflen, "#HOME#");
	return buf;
    }
    strcpyn(buf, buflen, NAME(obj));
    return buf;
}

static int
countlitems(char *list, char *sep)
{
    char *ptr;
    int seplen;
    int count = 1;

    if (!list || !*list)
	return 0;
    seplen = strlen(sep);
    ptr = list;
    while (*ptr) {
	while (*ptr && strncmp(ptr, sep, seplen))
	    ptr++;
	if (*ptr) {
	    ptr += seplen;
	    count++;
	}
    }
    return count;
}

/* buf is outbut buffer.  list is list to take item from.
 * line is list line to take. */

static char *
getlitem(char *buf, int buflen, char *list, char *sep, int line)
{
    char *ptr, *ptr2;
    char tmpchr;
    int seplen;

    seplen = strlen(sep);
    ptr = ptr2 = list;
    while (*ptr && line--) {
	for (ptr2 = ptr; *ptr2 && strncmp(ptr2, sep, seplen); ptr2++) ;
	if (!line)
	    break;
	if (*ptr2) {
	    ptr2 += seplen;
	}
	ptr = ptr2;
    }
    tmpchr = *ptr2;
    *ptr2 = '\0';
    strcpyn(buf, buflen, ptr);
    *ptr2 = tmpchr;
    return buf;
}


const char *
mfn_sublist(MFUNARGS)
{
    char *ptr;
    char sepbuf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    int count = 1;
    int which;
    int end;
    int incr = 1;
    int pflag;

    if (argc > 1) {
	which = atoi(argv[1]);
    } else {
	strcpyn(buf, buflen, argv[0]);
	return buf;
    }

    strcpyn(sepbuf, sizeof(sepbuf), "\r");
    if (argc > 3) {
	if (!*argv[3])
	    ABORT_MPI("SUBLIST", "Can't use null seperator string.");
	strcpyn(sepbuf, sizeof(sepbuf), argv[3]);
    }

    count = countlitems(argv[0], sepbuf);	/* count of items in list */

    if (which == 0)
	return "";
    if (which > count)
	which = count;
    if (which < 0)
	which += count + 1;
    if (which < 1)
	which = 1;

    end = which;

    if (argc > 2) {
	end = atoi(argv[2]);
    }

    if (end == 0)
	return "";
    if (end > count)
	end = count;
    if (end < 0)
	end += count + 1;
    if (end < 1)
	end = 1;

    if (end < which) {
	incr = -1;
    }

    *buf = '\0';
    pflag = 0;
    for (int i = which; ((i <= end) && (incr == 1)) || ((i >= end) && (incr == -1)); i += incr) {
	if (pflag) {
	    strcatn(buf, BUFFER_LEN, sepbuf);
	} else {
	    pflag++;
	}
	ptr = getlitem(buf2, sizeof(buf2), argv[0], sepbuf, i);
	strcatn(buf, BUFFER_LEN, ptr);
    }
    return buf;
}


const char *
mfn_lrand(MFUNARGS)
{
    /* {lrand:list,sep}  */
    char sepbuf[BUFFER_LEN];
    int count = 1;
    int which = 0;

    strcpyn(sepbuf, sizeof(sepbuf), "\r");
    if (argc > 1) {
	if (!*argv[1])
	    ABORT_MPI("LRAND", "Can't use null seperator string.");
	strcpyn(sepbuf, sizeof(sepbuf), argv[1]);
    }

    count = countlitems(argv[0], sepbuf);
    if (count) {
	which = ((RANDOM() / 256) % count) + 1;
	getlitem(buf, buflen, argv[0], sepbuf, which);
    } else {
	*buf = '\0';
    }
    return buf;
}


const char *
mfn_count(MFUNARGS)
{
    strcpyn(buf, buflen, "\r");
    if (argc > 1) {
	if (!*argv[1])
	    ABORT_MPI("COUNT", "Can't use null seperator string.");
	strcpyn(buf, buflen, argv[1]);
    }
    snprintf(buf, BUFFER_LEN, "%d", countlitems(argv[0], buf));
    return buf;
}


const char *
mfn_with(MFUNARGS)
{
    char namebuf[BUFFER_LEN];
    char cmdbuf[BUFFER_LEN];
    char vbuf[BUFFER_LEN];
    char *ptr, *valptr;
    int v;

    ptr = MesgParse(argv[0], namebuf, sizeof(namebuf));
    CHECKRETURN(ptr, "WITH", "arg 1");
    valptr = MesgParse(argv[1], vbuf, sizeof(vbuf));
    CHECKRETURN(valptr, "WITH", "arg 2");
    v = new_mvar(ptr, vbuf);
    if (v == 1)
	ABORT_MPI("WITH", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("WITH", "Too many variables already defined.");
    *buf = '\0';
    for (int cnt = 2; cnt < argc; cnt++) {
	ptr = MesgParse(argv[cnt], cmdbuf, sizeof(cmdbuf));
	if (!ptr) {
	    notifyf(player, "%s %cWITH%c (arg %d)", get_mvar("how"),
		    MFUN_LEADCHAR, MFUN_ARGEND, cnt);
	    return NULL;
	}
    }
    free_top_mvar();
    return ptr;
}


const char *
mfn_fold(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char varname[BUFFER_LEN];
    char sepinbuf[BUFFER_LEN];
    char listbuf[BUFFER_LEN];
    char tmp[BUFFER_LEN];
    char tmp2[BUFFER_LEN];
    char *ptr, *ptr2;
    char *sepin = argv[4];
    int seplen, v;

    ptr = MesgParse(argv[0], varname, sizeof(varname));
    CHECKRETURN(ptr, "FOLD", "arg 1");
    v = new_mvar(ptr, tmp);
    if (v == 1)
	ABORT_MPI("FOLD", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("FOLD", "Too many variables already defined.");

    ptr = MesgParse(argv[1], varname, sizeof(varname));
    CHECKRETURN(ptr, "FOLD", "arg 2");
    v = new_mvar(ptr, tmp2);
    if (v == 1)
	ABORT_MPI("FOLD", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("FOLD", "Too many variables already defined.");

    if (argc > 4) {
	ptr = MesgParse(sepin, sepinbuf, sizeof(sepinbuf));
	CHECKRETURN(ptr, "FOLD", "arg 5");
	if (!*ptr)
	    ABORT_MPI("FOLD", "Can't use Null seperator string");
	sepin = sepinbuf;
    } else {
	sepin = sepinbuf;
	strcpyn(sepin, sizeof(sepin), "\r");
    }
    seplen = strlen(sepin);
    ptr = MesgParse(argv[2], listbuf, sizeof(listbuf));
    CHECKRETURN(ptr, "FOLD", "arg 3");
    for (ptr2 = ptr; *ptr2 && strncmp(ptr2, sepin, seplen); ptr2++) ;
    if (*ptr2) {
	*ptr2 = '\0';
	ptr2 += seplen;
    }
    strcpyn(buf, buflen, ptr);
    ptr = ptr2;
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && strncmp(ptr2, sepin, seplen); ptr2++) ;
	if (*ptr2) {
	    *ptr2 = '\0';
	    ptr2 += seplen;
	}
	strcpyn(tmp2, sizeof(tmp2), ptr);
	strcpyn(tmp, sizeof(tmp), buf);
	MesgParse(argv[3], buf, buflen);
	CHECKRETURN(ptr, "FOLD", "arg 4");
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("FOLD", "Iteration limit exceeded");
    }
    free_top_mvar();
    free_top_mvar();
    return buf;
}


const char *
mfn_for(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char scratch[BUFFER_LEN];
    char tmp[BUFFER_LEN];
    char *ptr, *dptr;
    int v, start, end, incr;

    ptr = MesgParse(argv[0], scratch, sizeof(scratch));
    CHECKRETURN(ptr, "FOR", "arg 1 (varname)");
    v = new_mvar(ptr, tmp);
    if (v == 1)
	ABORT_MPI("FOR", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("FOR", "Too many variables already defined.");

    dptr = MesgParse(argv[1], scratch, sizeof(scratch));
    CHECKRETURN(dptr, "FOR", "arg 2 (start num)");
    start = atoi(dptr);

    dptr = MesgParse(argv[2], scratch, sizeof(scratch));
    CHECKRETURN(dptr, "FOR", "arg 3 (end num)");
    end = atoi(dptr);

    dptr = MesgParse(argv[3], scratch, sizeof(scratch));
    CHECKRETURN(dptr, "FOR", "arg 4 (increment)");
    incr = atoi(dptr);

    *buf = '\0';
    for (int i = start; ((incr >= 0 && i <= end) || (incr < 0 && i >= end)); i += incr) {
	snprintf(tmp, sizeof(tmp), "%d", i);
	dptr = MesgParse(argv[4], buf, buflen);
	CHECKRETURN(dptr, "FOR", "arg 5 (repeated command)");
	if (!(--iter_limit))
	    ABORT_MPI("FOR", "Iteration limit exceeded");
    }
    free_top_mvar();
    return buf;
}


const char *
mfn_foreach(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char scratch[BUFFER_LEN];
    char listbuf[BUFFER_LEN];
    char tmp[BUFFER_LEN];
    char *ptr, *ptr2, *dptr;
    char *sepin;
    int seplen, v;

    ptr = MesgParse(argv[0], scratch, sizeof(scratch));
    CHECKRETURN(ptr, "FOREACH", "arg 1");
    v = new_mvar(ptr, tmp);
    if (v == 1)
	ABORT_MPI("FOREACH", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("FOREACH", "Too many variables already defined.");

    dptr = MesgParse(argv[1], listbuf, sizeof(listbuf));
    CHECKRETURN(dptr, "FOREACH", "arg 2");

    if (argc > 3) {
	ptr = MesgParse(argv[3], scratch, sizeof(scratch));
	CHECKRETURN(ptr, "FOREACH", "arg 4");
	if (!*ptr)
	    ABORT_MPI("FOREACH", "Can't use Null seperator string");
	sepin = ptr;
    } else {
	sepin = scratch;
	strcpyn(sepin, sizeof(scratch), "\r");
    }
    seplen = strlen(sepin);
    ptr = dptr;
    *buf = '\0';
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && strncmp(ptr2, sepin, seplen); ptr2++) ;
	if (*ptr2) {
	    *ptr2 = '\0';
	    ptr2 += seplen;
	}
	strcpyn(tmp, sizeof(tmp), ptr);
	dptr = MesgParse(argv[2], buf, buflen);
	CHECKRETURN(dptr, "FOREACH", "arg 3");
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("FOREACH", "Iteration limit exceeded");
    }
    free_top_mvar();
    return buf;
}


const char *
mfn_filter(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char scratch[BUFFER_LEN];
    char listbuf[BUFFER_LEN];
    char sepinbuf[BUFFER_LEN];
    char sepoutbuf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char tmp[BUFFER_LEN];
    char *ptr, *ptr2, *dptr;
    char *sepin = argv[3];
    char *sepbuf = argv[4];
    int seplen, v;
    int outcount = 0;

    ptr = MesgParse(argv[0], scratch, sizeof(scratch));
    CHECKRETURN(ptr, "FILTER", "arg 1");
    v = new_mvar(ptr, tmp);
    if (v == 1)
	ABORT_MPI("FILTER", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("FILTER", "Too many variables already defined.");

    dptr = MesgParse(argv[1], listbuf, sizeof(listbuf));
    CHECKRETURN(dptr, "FILTER", "arg 2");
    if (argc > 3) {
	ptr = MesgParse(sepin, sepinbuf, sizeof(sepinbuf));
	CHECKRETURN(ptr, "FILTER", "arg 4");
	if (!*ptr)
	    ABORT_MPI("FILTER", "Can't use Null seperator string");
	sepin = sepinbuf;
    } else {
	sepin = sepinbuf;
	strcpyn(sepin, sizeof(sepinbuf), "\r");
    }
    if (argc > 4) {
	ptr = MesgParse(sepbuf, sepoutbuf, sizeof(sepoutbuf));
	CHECKRETURN(ptr, "FILTER", "arg 5");
	sepbuf = sepoutbuf;
    } else {
	sepbuf = sepoutbuf;
	strcpyn(sepbuf, sizeof(sepoutbuf), sepin);
    }
    seplen = strlen(sepin);
    *buf = '\0';
    ptr = dptr;
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && strncmp(ptr2, sepin, seplen); ptr2++) ;
	if (*ptr2) {
	    *ptr2 = '\0';
	    ptr2 += seplen;
	}
	strcpyn(tmp, sizeof(tmp), ptr);
	dptr = MesgParse(argv[2], buf2, sizeof(buf2));
	CHECKRETURN(dptr, "FILTER", "arg 3");
	if (truestr(buf2)) {
	    if (outcount++)
		strcatn(buf, BUFFER_LEN, sepbuf);
	    strcatn(buf, BUFFER_LEN, ptr);
	}
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("FILTER", "Iteration limit exceeded");
    }
    free_top_mvar();
    return buf;
}

static int
list_contains(char *word, int len, char *list)
{
    char *w, *w2;

    w = w2 = list;
    do {
	for (; *w2 && *w2 != '\r'; w2++) {
	};
	if (w2 - w == len && !strncmp(word, w, len))
	    return 1;
	if (*w2)
	    w = ++w2;
    } while (*w2);

    return 0;
}

const char *
mfn_lremove(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char *ptr, *ptr2, *endbuf;
    int len;
    int firstResult = 1;

    ptr = argv[0];		/* the list we're removing from */
    endbuf = buf;
    *buf = '\0';		/* empty buf; this is what we're returning, I bet */
    while (*ptr) {		/* while more of the first list */
	/* Find the next word. */
	for (ptr2 = ptr; *ptr2 && *ptr2 != '\r'; ptr2++) {
	};
	len = ptr2 - ptr;

	/* If the second list contains the string, continue. */
	if (!list_contains(ptr, len, argv[1]) &&
	    /*
	     * If it's the first result, it already won't be in buf.
	     * This wouldn't be a problem except buf already contains
	     * the empty string, so if the first word to add is the
	     * empty string, it won't be added.
	     */
	    (firstResult || !list_contains(ptr, len, buf))
		) {
	    if (firstResult)
		firstResult = 0;
	    else
		*(endbuf++) = '\r';
	    strncpy(endbuf, ptr, len);
	    endbuf += len;
	    *endbuf = '\0';
	}

	/* Next word. */
	if (*ptr2)
	    ptr2++;
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("LREMOVE", "Iteration limit exceeded");
    }

    return buf;
}


const char *
mfn_lcommon(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char *ptr, *ptr2, *p, *q;
    int len;
    int outcount = 0;

    ptr = argv[1];
    *buf = '\0';
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && *ptr2 != '\r'; ptr2++) ;
	if (*ptr2)
	    *(ptr2++) = '\0';
	len = strlen(ptr);
	p = argv[0];
	do {
	    if (string_prefix(p, ptr) && (!p[len] || p[len] == '\r'))
		break;
	    while (*p && *p != '\r')
		p++;
	    if (*p)
		p++;
	} while (*p);
	q = buf;
	do {
	    if (string_prefix(q, ptr) && (!q[len] || q[len] == '\r'))
		break;
	    while (*q && *q != '\r')
		q++;
	    if (*q)
		q++;
	} while (*q);
	if (*p && !*q) {
	    if (outcount++)
		strcatn(buf, BUFFER_LEN, "\r");
	    strcatn(buf, BUFFER_LEN, ptr);
	}
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("LCOMMON", "Iteration limit exceeded");
    }
    return buf;
}


const char *
mfn_lunion(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char *ptr, *ptr2, *p;
    int len;
    int outlen, nextlen;
    int outcount = 0;

    *buf = '\0';
    outlen = 0;
    ptr = argv[0];
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && *ptr2 != '\r'; ptr2++) ;
	if (*ptr2)
	    *(ptr2++) = '\0';
	len = strlen(ptr);
	p = buf;
	do {
	    if (string_prefix(p, ptr) && (!p[len] || p[len] == '\r'))
		break;
	    while (*p && *p != '\r')
		p++;
	    if (*p)
		p++;
	} while (*p);
	if (!*p) {
	    nextlen = strlen(ptr);
	    if (outlen + nextlen > BUFFER_LEN - 3)
		break;
	    if (outcount++) {
		strcatn(buf + outlen, BUFFER_LEN - outlen, "\r");
		outlen++;
	    }
	    strcatn((buf + outlen), BUFFER_LEN - outlen, ptr);
	    outlen += nextlen;
	}
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("LUNION", "Iteration limit exceeded");
    }
    ptr = argv[1];
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && *ptr2 != '\r'; ptr2++) ;
	if (*ptr2)
	    *(ptr2++) = '\0';
	len = strlen(ptr);
	p = buf;
	do {
	    if (string_prefix(p, ptr) && (!p[len] || p[len] == '\r'))
		break;
	    while (*p && *p != '\r')
		p++;
	    if (*p)
		p++;
	} while (*p);
	if (!*p) {
	    nextlen = strlen(ptr);
	    if (outlen + nextlen > BUFFER_LEN - 3)
		break;
	    if (outcount++) {
		strcatn(buf + outlen, BUFFER_LEN - outlen, "\r");
		outlen++;
	    }
	    strcatn((buf + outlen), BUFFER_LEN - outlen, ptr);
	    outlen += nextlen;
	}
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("LUNION", "Iteration limit exceeded");
    }
    return buf;
}


const char *
mfn_lsort(MFUNARGS)
{
    char *litem[MAX_MFUN_LIST_LEN];
    char listbuf[BUFFER_LEN];
    char scratch[BUFFER_LEN];
    char vbuf[BUFFER_LEN];
    char vbuf2[BUFFER_LEN];
    char *ptr, *ptr2, *tmp;
    int j, count;
    int outcount = 0;

    if (argc > 1 && argc < 4)
	ABORT_MPI("LSORT", "Takes 1 or 4 arguments.");
    for (int i = 0; i < MAX_MFUN_LIST_LEN; i++)
	litem[i] = NULL;
    ptr = MesgParse(argv[0], listbuf, sizeof(listbuf));
    CHECKRETURN(ptr, "LSORT", "arg 1");
    if (argc > 1) {
	ptr2 = MesgParse(argv[1], scratch, sizeof(scratch));
	CHECKRETURN(ptr2, "LSORT", "arg 2");
	j = new_mvar(ptr2, vbuf);
	if (j == 1)
	    ABORT_MPI("LSORT", "Variable name too long.");
	if (j == 2)
	    ABORT_MPI("LSORT", "Too many variables already defined.");
	ptr2 = MesgParse(argv[2], scratch, sizeof(scratch));
	CHECKRETURN(ptr2, "LSORT", "arg 3");
	j = new_mvar(ptr2, vbuf2);
	if (j == 1)
	    ABORT_MPI("LSORT", "Variable name too long.");
	if (j == 2)
	    ABORT_MPI("LSORT", "Too many variables already defined.");
    }
    count = 0;
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && *ptr2 != '\r'; ptr2++) ;
	if (*ptr2 == '\r')
	    *(ptr2++) = '\0';
	litem[count++] = ptr;
	ptr = ptr2;
	if (count >= MAX_MFUN_LIST_LEN)
	    ABORT_MPI("LSORT", "Iteration limit exceeded");
    }
    for (int i = 0; i < count; i++) {
	for (j = i + 1; j < count; j++) {
	    if (argc > 1) {
		strcpyn(vbuf, sizeof(vbuf), litem[i]);
		strcpyn(vbuf2, sizeof(vbuf2), litem[j]);
		ptr = MesgParse(argv[3], buf, buflen);
		CHECKRETURN(ptr, "LSORT", "arg 4");
		if (truestr(buf)) {
		    tmp = litem[i];
		    litem[i] = litem[j];
		    litem[j] = tmp;
		}
	    } else {
		if (alphanum_compare(litem[i], litem[j]) > 0) {
		    tmp = litem[i];
		    litem[i] = litem[j];
		    litem[j] = tmp;
		}
	    }
	}
    }
    *buf = '\0';
    for (int i = 0; i < count; i++) {
	if (outcount++)
	    strcatn(buf, BUFFER_LEN, "\r");
	strcatn(buf, BUFFER_LEN, litem[i]);
    }
    if (argc > 1) {
	free_top_mvar();
	free_top_mvar();
    }
    return buf;
}


const char *
mfn_lunique(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char *ptr, *ptr2, *p;
    int len;
    int outlen, nextlen;
    int outcount = 0;

    *buf = '\0';
    outlen = 0;
    ptr = argv[0];
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && *ptr2 != '\r'; ptr2++) ;
	if (*ptr2)
	    *(ptr2++) = '\0';
	len = strlen(ptr);
	p = buf;
	do {
	    if (string_prefix(p, ptr) && (!p[len] || p[len] == '\r'))
		break;
	    while (*p && *p != '\r')
		p++;
	    if (*p)
		p++;
	} while (*p);
	if (!*p) {
	    nextlen = strlen(ptr);
	    if (outcount++) {
		strcatn(buf + outlen, BUFFER_LEN - outlen, "\r");
		outlen++;
	    }
	    strcatn((buf + outlen), BUFFER_LEN - outlen, ptr);
	    outlen += nextlen;
	}
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("LUNIQUE", "Iteration limit exceeded");
    }
    return buf;
}


const char *
mfn_parse(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char listbuf[BUFFER_LEN];
    char sepinbuf[BUFFER_LEN];
    char sepoutbuf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char tmp[BUFFER_LEN];
    char *ptr, *ptr2, *dptr;
    char *sepin = argv[3];
    char *sepbuf = argv[4];
    int outcount = 0;
    int seplen, oseplen, v;
    int outlen, nextlen;

    ptr = MesgParse(argv[0], buf2, sizeof(buf2));
    CHECKRETURN(ptr, "PARSE", "arg 1");
    v = new_mvar(ptr, tmp);
    if (v == 1)
	ABORT_MPI("PARSE", "Variable name too long.");
    if (v == 2)
	ABORT_MPI("PARSE", "Too many variables already defined.");

    dptr = MesgParse(argv[1], listbuf, sizeof(listbuf));
    CHECKRETURN(dptr, "PARSE", "arg 2");

    if (argc > 3) {
	ptr = MesgParse(sepin, sepinbuf, sizeof(sepinbuf));
	CHECKRETURN(ptr, "PARSE", "arg 4");
	if (!*ptr)
	    ABORT_MPI("PARSE", "Can't use Null seperator string");
	sepin = sepinbuf;
    } else {
	sepin = sepinbuf;
	strcpyn(sepin, sizeof(sepinbuf), "\r");
    }

    if (argc > 4) {
	ptr = MesgParse(sepbuf, sepoutbuf, sizeof(sepoutbuf));
	CHECKRETURN(ptr, "PARSE", "arg 5");
	sepbuf = sepoutbuf;
    } else {
	sepbuf = sepoutbuf;
	strcpyn(sepbuf, sizeof(sepoutbuf), sepin);
    }
    seplen = strlen(sepin);
    oseplen = strlen(sepbuf);

    *buf = '\0';
    outlen = 0;
    ptr = dptr;
    while (*ptr) {
	for (ptr2 = ptr; *ptr2 && strncmp(ptr2, sepin, seplen); ptr2++) ;
	if (*ptr2) {
	    *ptr2 = '\0';
	    ptr2 += seplen;
	}
	strcpyn(tmp, sizeof(tmp), ptr);
	dptr = MesgParse(argv[2], buf2, sizeof(buf2));
	CHECKRETURN(dptr, "PARSE", "arg 3");
	nextlen = strlen(buf2);
	if (outlen + nextlen + oseplen > BUFFER_LEN - 3)
	    break;
	if (outcount++) {
	    strcatn(buf + outlen, BUFFER_LEN - outlen, sepbuf);
	    outlen += oseplen;
	}
	strcatn((buf + outlen), BUFFER_LEN - outlen, buf2);
	outlen += nextlen;
	ptr = ptr2;
	if (!(--iter_limit))
	    ABORT_MPI("PARSE", "Iteration limit exceeded");
    }
    free_top_mvar();
    return buf;
}


const char *
mfn_smatch(MFUNARGS)
{
    if (equalstr(argv[1], argv[0])) {
	return "1";
    } else {
	return "0";
    }
}


const char *
mfn_strlen(MFUNARGS)
{
    snprintf(buf, BUFFER_LEN, "%d", (int) strlen(argv[0]));
    return buf;
}


const char *
mfn_subst(MFUNARGS)
{
    return string_substitute(argv[0], argv[1], argv[2], buf, BUFFER_LEN);
}


const char *
mfn_awake(MFUNARGS)
{
    dbref obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == PERMDENIED || obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING ||
	obj == HOME)
	return ("0");

    if (Typeof(obj) == TYPE_THING && (FLAGS(obj) & ZOMBIE)) {
	obj = OWNER(obj);
    } else if (Typeof(obj) != TYPE_PLAYER) {
	return ("0");
    }

    snprintf(buf, BUFFER_LEN, "%d", online(obj));
    return (buf);
}


const char *
mfn_type(MFUNARGS)
{
    dbref obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == NOTHING || obj == AMBIGUOUS || obj == UNKNOWN)
	return ("Bad");
    if (obj == HOME)
	return ("Room");
    if (obj == PERMDENIED)
	ABORT_MPI("TYPE", "Permission Denied.");

    switch (Typeof(obj)) {
    case TYPE_PLAYER:
	return "Player";
    case TYPE_ROOM:
	return "Room";
    case TYPE_EXIT:
	return "Exit";
    case TYPE_THING:
	return "Thing";
    case TYPE_PROGRAM:
	return "Program";
    default:
	return "Bad";
    }
}


const char *
mfn_istype(MFUNARGS)
{
    dbref obj;
    obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == NOTHING || obj == AMBIGUOUS || obj == UNKNOWN)
	return (strcasecmp(argv[1], "Bad") ? "0" : "1");
    if ((strcasecmp(argv[1], "Bad") == 0) &&
	(obj == NOTHING || obj == AMBIGUOUS || obj == UNKNOWN || obj == PERMDENIED))
	return "1";
    if (obj == PERMDENIED)
	ABORT_MPI("TYPE", "Permission Denied.");
    if (obj == HOME)
	return (strcasecmp(argv[1], "Room") ? "0" : "1");

    switch (Typeof(obj)) {
    case TYPE_PLAYER:
	return (strcasecmp(argv[1], "Player") ? "0" : "1");
    case TYPE_ROOM:
	return (strcasecmp(argv[1], "Room") ? "0" : "1");
    case TYPE_EXIT:
	return (strcasecmp(argv[1], "Exit") ? "0" : "1");
    case TYPE_THING:
	return (strcasecmp(argv[1], "Thing") ? "0" : "1");
    case TYPE_PROGRAM:
	return (strcasecmp(argv[1], "Program") ? "0" : "1");
    default:
	return (strcasecmp(argv[1], "Bad") ? "0" : "1");
    }
}


const char *
mfn_fox(MFUNARGS)
{
    return "You found the easter egg!";
}

const char *
mfn_debugif(MFUNARGS)
{
    char *ptr = MesgParse(argv[0], buf, buflen);

    CHECKRETURN(ptr, "DEBUGIF", "arg 1");
    if (truestr(argv[0])) {
	ptr = mesg_parse(descr, player, what, perms, argv[1],
			 buf, BUFFER_LEN, (mesgtyp | MPI_ISDEBUG));
    } else {
	ptr = MesgParse(argv[1], buf, buflen);
    }
    CHECKRETURN(ptr, "DEBUGIF", "arg 2");
    return buf;
}


const char *
mfn_debug(MFUNARGS)
{
    char *ptr = mesg_parse(descr, player, what, perms, argv[0],
			   buf, BUFFER_LEN, (mesgtyp | MPI_ISDEBUG));

    CHECKRETURN(ptr, "DEBUG", "arg 1");
    return buf;
}


const char *
mfn_revoke(MFUNARGS)
{
    char *ptr = mesg_parse(descr, player, what, perms, argv[0],
			   buf, BUFFER_LEN, (mesgtyp & ~MPI_ISBLESSED));

    CHECKRETURN(ptr, "REVOKE", "arg 1");
    return buf;
}


const char *
mfn_timing(MFUNARGS)
{
    char *ptr;
    struct timeval start_time, end_time;
    int secs;
    int usecs;
    double timelen;

    gettimeofday(&start_time, (struct timezone *) 0);

    ptr = mesg_parse(descr, player, what, perms, argv[0], buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "TIMING", "arg 1");

    gettimeofday(&end_time, (struct timezone *) 0);
    secs = end_time.tv_sec - start_time.tv_sec;
    usecs = end_time.tv_usec - start_time.tv_usec;
    if (usecs > 1000000) {
	secs += 1;
	usecs -= 1000000;
    }
    timelen = ((double) secs) + (((double) usecs) / 1000000);
    notifyf_nolisten(player, "Time elapsed: %.6f seconds", timelen);

    return buf;
}


const char *
mfn_delay(MFUNARGS)
{
    char *argchr, *cmdchr;
    int i = atoi(argv[0]);

    if (i < 1)
	i = 1;
    if (i > 31622400)
	ABORT_MPI("DELAY", "Delaying more than a year in MPI is just silly.");
#ifdef WIZZED_DELAY
    if (!(mesgtyp & MPI_ISBLESSED))
	ABORT_MPI("DELAY", "Permission denied.");
#endif
    cmdchr = get_mvar("cmd");
    argchr = get_mvar("arg");
    i = add_mpi_event(i, descr, player, LOCATION(player), perms, argv[1], cmdchr, argchr,
		      (mesgtyp & MPI_ISLISTENER), (!(mesgtyp & MPI_ISPRIVATE)),
		      (mesgtyp & MPI_ISBLESSED));
    snprintf(buf, BUFFER_LEN, "%d", i);
    return buf;
}



const char *
mfn_kill(MFUNARGS)
{
    int i = atoi(argv[0]);

    if (i > 0) {
	if (in_timequeue(i)) {
	    if (!control_process(perms, i)) {
		ABORT_MPI("KILL", "Permission denied.");
	    }
	    i = dequeue_process(i);
	} else {
	    i = 0;
	}
    } else if (i == 0) {
	i = dequeue_prog(perms, 0);
    } else {
	ABORT_MPI("KILL", "Invalid process ID.");
    }
    snprintf(buf, BUFFER_LEN, "%d", i);
    return buf;
}



static int mpi_muf_call_levels = 0;

const char *
mfn_muf(MFUNARGS)
{
    char *ptr;
    struct inst *rv = NULL;
    struct frame *tmpfr;
    dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);

    if (obj == UNKNOWN)
	ABORT_MPI("MUF", "Match failed.");
    if (obj <= NOTHING || Typeof(obj) != TYPE_PROGRAM)
	ABORT_MPI("MUF", "Bad program reference.");
    if (!(FLAGS(obj) & LINK_OK) && !controls(perms, obj))
	ABORT_MPI("MUF", "Permission denied.");
    if ((mesgtyp & (MPI_ISLISTENER | MPI_ISLOCK)) && (MLevel(obj) < 3))
	ABORT_MPI("MUF", "Permission denied.");

    if (++mpi_muf_call_levels > 18)
	ABORT_MPI("MUF", "Too many call levels.");

    strcpyn(match_args, sizeof(match_args), argv[1]);
    ptr = get_mvar("how");
    snprintf(match_cmdname, sizeof(match_cmdname), "%s(MPI)", ptr);
    tmpfr = interp(descr, player, LOCATION(player), obj, perms, PREEMPT, STD_HARDUID, 0);
    if (tmpfr) {
	rv = interp_loop(player, obj, tmpfr, 1);
    }

    mpi_muf_call_levels--;

    if (!rv)
	return "";
    switch (rv->type) {
    case PROG_STRING:
	if (rv->data.string) {
	    strcpyn(buf, buflen, rv->data.string->data);
	    CLEAR(rv);
	    return buf;
	} else {
	    CLEAR(rv);
	    return "";
	}
    case PROG_INTEGER:
	snprintf(buf, BUFFER_LEN, "%d", rv->data.number);
	CLEAR(rv);
	return buf;
    case PROG_FLOAT:
	snprintf(buf, BUFFER_LEN, "%.15g", rv->data.fnumber);
	CLEAR(rv);
	return buf;
    case PROG_OBJECT:
	ptr = ref2str(rv->data.objref, buf, BUFFER_LEN);
	CLEAR(rv);
	return ptr;
    default:
	CLEAR(rv);
	return "";
    }
}


const char *
mfn_force(MFUNARGS)
{
    char *nxt, *ptr;
    dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);

    if (obj == AMBIGUOUS || obj == UNKNOWN || obj == NOTHING || obj == HOME)
	ABORT_MPI("FORCE", "Failed match. (arg1)");
    if (obj == PERMDENIED)
	ABORT_MPI("FORCE", "Permission denied. (arg1)");
    if (Typeof(obj) != TYPE_THING && Typeof(obj) != TYPE_PLAYER)
	ABORT_MPI("FORCE", "Bad object reference. (arg1)");
    if (!*argv[1])
	ABORT_MPI("FORCE", "Null command string. (arg2)");
    if (!tp_zombies && !(mesgtyp & MPI_ISBLESSED))
	ABORT_MPI("FORCE", "Permission Denied.");
    if (!(mesgtyp & MPI_ISBLESSED)) {
	ptr = (char *)NAME(obj);
	char objname[BUFFER_LEN], *ptr2;
	dbref loc = LOCATION(obj);

	if (Typeof(obj) == TYPE_THING) {
	    if (FLAGS(obj) & DARK)
		ABORT_MPI("FORCE", "Cannot force a dark puppet.");
	    if ((FLAGS(OWNER(obj)) & ZOMBIE))
		ABORT_MPI("FORCE", "Permission denied.");
	    if (loc != NOTHING && (FLAGS(loc) & ZOMBIE) && Typeof(loc) == TYPE_ROOM)
		ABORT_MPI("FORCE", "Cannot force a Puppet in a no-puppets room.");
	    for (ptr2 = objname; *ptr && !isspace(*ptr);)
		*(ptr2++) = *(ptr++);
	    *ptr2 = '\0';
	    if (lookup_player(objname) != NOTHING)
		ABORT_MPI("FORCE", "Cannot force a thing named after a player.");
	}
	if (!(FLAGS(obj) & XFORCIBLE)) {
	    ABORT_MPI("FORCE", "Permission denied: forced object not @set Xforcible.");
	}
	if (!test_lock_false_default(descr, perms, obj, MESGPROP_FLOCK)) {
	    ABORT_MPI("FORCE", "Permission denied: Object not force-locked to trigger.");
	}
    }
#ifdef GOD_PRIV
    if (God(obj))
	ABORT_MPI("FORCE", "Permission denied: You can't force God.");
#endif

    if (force_level > (tp_max_force_level - 1))
	ABORT_MPI("FORCE", "Permission denied: You can't force recursively.");
    strcpyn(buf, buflen, argv[1]);
    ptr = buf;
    do {
	const char *ptr2 = NAME(obj);
	char objname[BUFFER_LEN], *ptr3;

	nxt = index(ptr, '\r');
	if (nxt)
	    *nxt++ = '\0';

	/* fixing bug #986845 */
	/* follows same definition above, with different variable names */
	for (ptr3 = objname; *ptr2 && !isspace(*ptr2);)
	    *(ptr3++) = *(ptr2++);
	*ptr3 = '\0';
	if (lookup_player(objname) != NOTHING && Typeof(obj) != TYPE_PLAYER) {
	    ABORT_MPI("FORCE", "Cannot force a thing named after a player. [2]");
	}
        objnode_push(&forcelist, player);
	objnode_push(&forcelist, what);
	force_level++;
	if (*ptr)
	    process_command(dbref_first_descr(obj), obj, ptr);
	force_level--;
	objnode_pop(&forcelist);
        objnode_pop(&forcelist);
	ptr = nxt;
    } while (ptr);
    *buf = '\0';
    return "";
}


const char *
mfn_midstr(MFUNARGS)
{
    int len = strlen(argv[0]);
    int pos1 = atoi(argv[1]);
    int pos2 = pos1;
    char *ptr = buf;

    if (argc > 2)
	pos2 = atoi(argv[2]);

    if (pos1 == 0)
	return "";
    if (pos1 > len)
	pos1 = len;
    if (pos1 < 0)
	pos1 += len + 1;
    if (pos1 < 1)
	pos1 = 1;

    if (pos2 == 0)
	return "";
    if (pos2 > len)
	pos2 = len;
    if (pos2 < 0)
	pos2 += len + 1;
    if (pos2 < 1)
	pos2 = 1;

    if (pos2 >= pos1) {
	for (int i = pos1; i <= pos2; i++)
	    *(ptr++) = argv[0][i - 1];
    } else {
	for (int i = pos1; i >= pos2; i--)
	    *(ptr++) = argv[0][i - 1];
    }
    *ptr = '\0';
    return buf;
}


const char *
mfn_instr(MFUNARGS)
{
    char *ptr;

    if (!*argv[1])
	ABORT_MPI("INSTR", "Can't search for a null string.");
    for (ptr = argv[0]; *ptr && !string_prefix(ptr, argv[1]); ptr++) ;
    if (!*ptr)
	return "0";
    snprintf(buf, BUFFER_LEN, "%d", (int) (ptr - argv[0] + 1));
    return buf;
}


const char *
mfn_lmember(MFUNARGS)
{
    /* {lmember:list,item,delim} */
    int i = 1;
    char *ptr = argv[0];
    char *delim = NULL;
    int len;
    int len2 = strlen(argv[1]);

    if (argc < 3)
	delim = "\r";
    else
	delim = argv[2];
    if (!*delim)
	ABORT_MPI("LMEMBER", "List delimiter cannot be a null string.");
    len = strlen(delim);
    while (*ptr && !(string_prefix(ptr, argv[1]) &&
		     (!ptr[len2] || string_prefix(ptr + len2, delim)))) {
	while (*ptr && !string_prefix(ptr, delim))
	    ptr++;
	if (*ptr)
	    ptr += len;
	i++;
    }
    if (!*ptr)
	return "0";
    snprintf(buf, BUFFER_LEN, "%d", i);
    return buf;
}


const char *
mfn_tolower(MFUNARGS)
{
    char *ptr = argv[0];
    char *ptr2 = buf;

    while (*ptr) {
	if (isupper(*ptr)) {
	    *ptr2++ = tolower(*ptr++);
	} else {
	    *ptr2++ = *ptr++;
	}
    }
    *ptr2++ = '\0';
    return buf;
}


const char *
mfn_toupper(MFUNARGS)
{
    char *ptr = argv[0];
    char *ptr2 = buf;

    while (*ptr) {
	if (islower(*ptr)) {
	    *ptr2++ = toupper(*ptr++);
	} else {
	    *ptr2++ = *ptr++;
	}
    }
    *ptr2++ = '\0';
    return buf;
}


const char *
mfn_commas(MFUNARGS)
{
    int v, count, itemlen;
    char *ptr;
    char *out;
    char listbuf[BUFFER_LEN];
    char sepbuf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char tmp[BUFFER_LEN];
    tmp[0] = '\0';

    if (argc == 3)
	ABORT_MPI("COMMAS", "Takes 1, 2, or 4 arguments.");

    ptr = MesgParse(argv[0], listbuf, sizeof(listbuf));
    CHECKRETURN(ptr, "COMMAS", "arg 1");
    count = countlitems(listbuf, "\r");
    if (count == 0)
	return "";

    if (argc > 1) {
	ptr = MesgParse(argv[1], sepbuf, sizeof(sepbuf));
	CHECKRETURN(ptr, "COMMAS", "arg 2");
    } else {
	strcpyn(sepbuf, sizeof(sepbuf), " and ");
    }

    if (argc > 2) {
	ptr = MesgParse(argv[2], buf2, sizeof(buf2));
	CHECKRETURN(ptr, "COMMAS", "arg 3");
	v = new_mvar(ptr, tmp);
	if (v == 1)
	    ABORT_MPI("COMMAS", "Variable name too long.");
	if (v == 2)
	    ABORT_MPI("COMMAS", "Too many variables already defined.");
    }

    *buf = '\0';
    out = buf;
    for (int i = 1; i <= count; i++) {
	ptr = getlitem(buf2, sizeof(buf2), listbuf, "\r", i);
	if (argc > 2) {
	    strcpyn(tmp, BUFFER_LEN, ptr);
	    ptr = MesgParse(argv[3], buf2, sizeof(buf2));
	    CHECKRETURN(ptr, "COMMAS", "arg 3");
	}
	itemlen = strlen(ptr);
	if ((out - buf) + itemlen >= BUFFER_LEN) {
	    if (argc > 2)
		free_top_mvar();
	    return buf;
	}
	strcatn(out, BUFFER_LEN - (out - buf), ptr);
	out += itemlen;
	switch (count - i) {
	case 0:
	    if (argc > 2)
		free_top_mvar();
	    return buf;
	case 1:
	    itemlen = strlen(sepbuf);
	    if ((out - buf) + itemlen >= BUFFER_LEN) {
		if (argc > 2)
		    free_top_mvar();
		return buf;
	    }
	    strcatn(out, BUFFER_LEN - (out - buf), sepbuf);
	    out += itemlen;
	    break;
	default:
	    if ((out - buf) + 2 >= BUFFER_LEN) {
		if (argc > 2)
		    free_top_mvar();
		return buf;
	    }
	    strcatn(out, BUFFER_LEN - (out - buf), ", ");
	    out += strlen(out);
	    break;
	}
    }
    if (argc > 2)
	free_top_mvar();
    return buf;
}


const char *
mfn_attr(MFUNARGS)
{
    int exlen;

    buf[0] = '\0';
    for (int i = 0; i < argc - 1; i++) {
	if (!strcasecmp(argv[i], "reset") || !strcasecmp(argv[i], "normal")) {
	    strcatn(buf, BUFFER_LEN, ANSI_RESET);
	} else if (!strcasecmp(argv[i], "bold")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BOLD);
	} else if (!strcasecmp(argv[i], "dim")) {
	    strcatn(buf, BUFFER_LEN, ANSI_DIM);
	} else if (!strcasecmp(argv[i], "italic")) {
	    strcatn(buf, BUFFER_LEN, ANSI_ITALIC);
	} else if (!strcasecmp(argv[i], "uline") || !strcasecmp(argv[i], "underline")) {
	    strcatn(buf, BUFFER_LEN, ANSI_UNDERLINE);
	} else if (!strcasecmp(argv[i], "flash")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FLASH);
	} else if (!strcasecmp(argv[i], "reverse")) {
	    strcatn(buf, BUFFER_LEN, ANSI_REVERSE);
	} else if (!strcasecmp(argv[i], "ostrike")
		   || !strcasecmp(argv[i], "overstrike")) {
	    strcatn(buf, BUFFER_LEN, ANSI_OSTRIKE);

	} else if (!strcasecmp(argv[i], "black")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_BLACK);
	} else if (!strcasecmp(argv[i], "red")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_RED);
	} else if (!strcasecmp(argv[i], "yellow")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_YELLOW);
	} else if (!strcasecmp(argv[i], "green")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_GREEN);
	} else if (!strcasecmp(argv[i], "cyan")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_CYAN);
	} else if (!strcasecmp(argv[i], "blue")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_BLUE);
	} else if (!strcasecmp(argv[i], "magenta")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_MAGENTA);
	} else if (!strcasecmp(argv[i], "white")) {
	    strcatn(buf, BUFFER_LEN, ANSI_FG_WHITE);

	} else if (!strcasecmp(argv[i], "bg_black")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_BLACK);
	} else if (!strcasecmp(argv[i], "bg_red")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_RED);
	} else if (!strcasecmp(argv[i], "bg_yellow")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_YELLOW);
	} else if (!strcasecmp(argv[i], "bg_green")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_GREEN);
	} else if (!strcasecmp(argv[i], "bg_cyan")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_CYAN);
	} else if (!strcasecmp(argv[i], "bg_blue")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_BLUE);
	} else if (!strcasecmp(argv[i], "bg_magenta")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_MAGENTA);
	} else if (!strcasecmp(argv[i], "bg_white")) {
	    strcatn(buf, BUFFER_LEN, ANSI_BG_WHITE);
	} else if (!strcasecmp(argv[i], "")) {
	} else {
	    ABORT_MPI("ATTR",
		      "Unrecognized ansi tag.  Try one of reset, bold, dim, italic, underline, reverse, overstrike, black, red, yellow, green, cyan, blue, magenta, white, bg_black, bg_red, bg_yellow, bg_green, bg_cyan, bg_blue, bg_magenta, or bg_white.");
	}
    }
    exlen = strlen(buf) + strlen(ANSI_RESET) + 1;
    strncat(buf, argv[argc - 1], (BUFFER_LEN - exlen));
    strcatn(buf, BUFFER_LEN, ANSI_RESET);
    return buf;
}


const char *
mfn_escape(MFUNARGS)
{
    char *out;
    const char *in;
    int done = 0;

    in = argv[0];
    out = buf;
    *out++ = '`';
    while (*in && !done) {
	switch (*in) {
	case '\\':
	case '`':
	    if (out - buf >= BUFFER_LEN - 2) {
		done = 1;
		break;
	    }
	    *out++ = '\\';
	    *out++ = *in++;
	    break;

	default:
	    if (out - buf >= BUFFER_LEN - 1) {
		done = 1;
		break;
	    }
	    *out++ = *in++;
	}
    }
    *out++ = '`';
    *out = '\0';
    return buf;
}
