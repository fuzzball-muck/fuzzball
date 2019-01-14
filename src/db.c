#include "config.h"

#include "compile.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "edit.h"
#include "fbstrings.h"
#include "fbmath.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "match.h"
#include "log.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

struct object *db = 0;
objnode *forcelist = 0;
dbref db_top = 0;
dbref recyclable = NOTHING;
static int db_load_format = 0;

#ifndef DB_INITIAL_SIZE
#define DB_INITIAL_SIZE 10000
#endif				/* DB_INITIAL_SIZE */

struct macrotable *macrotop;

static void
db_grow(dbref newtop)
{
    if (newtop > db_top) {
	db_top = newtop;
	if (db) {
	    if ((db = realloc(db, (size_t)db_top * sizeof(struct object))) == 0) {
		abort();
	    }
	} else {
	    int startsize = MAX(newtop, DB_INITIAL_SIZE);

	    if ((db = malloc((size_t)startsize * sizeof(struct object))) == 0) {
		abort();
	    }
	}
    }
}

void
db_clear_object(dbref i)
{
    struct object *o = DBFETCH(i);

    memset(o, 0, sizeof(struct object));

    NAME(i) = 0;
    ts_newobject(i);
    o->location = NOTHING;
    o->contents = NOTHING;
    o->exits = NOTHING;
    o->next = NOTHING;
    o->properties = 0;

#ifdef DISKBASE
    o->propsfpos = 0;
    o->propstime = 0;
    o->propsmode = PROPS_UNLOADED;
    o->nextold = NOTHING;
    o->prevold = NOTHING;
#endif

    /* DBDIRTY(i); */
    /* flags you must initialize yourself */
    /* type-specific fields you must also initialize */
}

dbref
new_object(void)
{
    dbref newobj;

    if (recyclable != NOTHING) {
	newobj = recyclable;
	recyclable = NEXTOBJ(newobj);
	db_free_object(newobj);
    } else {
	newobj = db_top;
	db_grow(db_top + 1);
    }

    /* clear it out */
    db_clear_object(newobj);
    DBDIRTY(newobj);
    return newobj;
}

static dbref
create_object(const char *name, dbref owner, object_flag_type flags)
{
    dbref newobj = new_object();

    NAME(newobj) = alloc_string(name);
    FLAGS(newobj) = flags;
    OWNER(newobj) = OWNER(owner);

    return newobj;
}

dbref
create_action(dbref player, const char *name, dbref source)
{
    dbref newact = create_object(name, player, TYPE_EXIT);

    set_source(newact, source);

    DBFETCH(newact)->sp.exit.ndest = 0;
    DBFETCH(newact)->sp.exit.dest = NULL;

    if (tp_autolink_actions) {
	DBFETCH(newact)->sp.exit.ndest = 1;
	DBFETCH(newact)->sp.exit.dest = malloc(sizeof(dbref));
	(DBFETCH(newact)->sp.exit.dest)[0] = NIL;
    }

    return newact;
}

dbref
create_program(dbref player, const char *name)
{
    char buf[BUFFER_LEN];
    int jj;
    dbref newprog = create_object(name, player, TYPE_PROGRAM);

    snprintf(buf, sizeof(buf), "A scroll containing a spell called %s", name);
    SETDESC(newprog, buf);
    LOCATION(newprog) = player;
    jj = MLevel(player);
    if (jj < 1)
	jj = 2;
    if (jj > 3)
        jj = 3;
    SetMLevel(newprog, jj);

    ALLOC_PROGRAM_SP(newprog);
    PROGRAM_SET_FIRST(newprog, NULL);
    PROGRAM_SET_CURR_LINE(newprog, 0);
    PROGRAM_SET_SIZ(newprog, 0);
    PROGRAM_SET_CODE(newprog, NULL);
    PROGRAM_SET_START(newprog, NULL);
    PROGRAM_SET_PUBS(newprog, NULL);
#ifdef MCP_SUPPORT
    PROGRAM_SET_MCPBINDS(newprog, NULL);
#endif
    PROGRAM_SET_PROFTIME(newprog, 0, 0);
    PROGRAM_SET_PROFSTART(newprog, 0);
    PROGRAM_SET_PROF_USES(newprog, 0);
    PROGRAM_SET_INSTANCES(newprog, 0);
    PROGRAM_SET_INSTANCES_IN_PRIMITIVE(newprog, 0);

    PUSH(newprog, CONTENTS(player));
    DBDIRTY(newprog);
    DBDIRTY(player);

    return newprog;
}

dbref
create_room(dbref player, const char *name, dbref parent)
{
    dbref newroom = create_object(name, player,
	    TYPE_ROOM | (FLAGS(player) & JUMP_OK));

    LOCATION(newroom) = parent;
    PUSH(newroom, CONTENTS(parent));

    EXITS(newroom) = NOTHING;
    DBFETCH(newroom)->sp.room.dropto = NOTHING;

    DBDIRTY(parent);

    return newroom;
}

dbref
create_thing(dbref player, const char *name, dbref location)
{
    dbref loc;
    dbref newthing = create_object(name, player, TYPE_THING);

    LOCATION(newthing) = location;
    PUSH(newthing, CONTENTS(location));

    EXITS(newthing) = NOTHING;
    SETVALUE(newthing, 1);

    ALLOC_THING_SP(newthing);

    if ((loc = LOCATION(player)) != NOTHING && controls(player, loc)) {
	THING_SET_HOME(newthing, loc);
    } else {
	THING_SET_HOME(newthing, player);
    }

    DBDIRTY(location);

    return newthing;
}

void
putref(FILE * f, dbref ref)
{
    if (fprintf(f, "%d\n", ref) < 0) {
	abort();
    }
}

void
putstring(FILE * f, const char *s)
{
    if (s) {
	if (fputs(s, f) == EOF) {
	    abort();
	}
    }
    if (putc('\n', f) == EOF) {
	abort();
    }
}

void
putproperties(FILE * f, dbref obj)
{
    putstring(f, "*Props*");
    db_dump_props(f, obj);
    putstring(f, "*End*");
}

static int
db_write_object(FILE * f, dbref i)
{
    struct object *o = DBFETCH(i);
#ifdef DISKBASE
    long tmppos;
#endif				/* DISKBASE */

    putstring(f, NAME(i));
    putref(f, o->location);
    putref(f, o->contents);
    putref(f, o->next);
    putref(f, FLAGS(i) & ~DUMP_MASK);	/* write non-internal flags */

    putref(f, (int)o->ts_created);
    putref(f, (int)o->ts_lastused);
    putref(f, o->ts_usecount);
    putref(f, (int)o->ts_modified);


#ifdef DISKBASE
    tmppos = ftell(f) + 1;
    putprops_copy(f, i);
    o->propsfpos = tmppos;
    undirtyprops(i);
#else				/* !DISKBASE */
    putproperties(f, i);
#endif				/* DISKBASE */


    switch (Typeof(i)) {
    case TYPE_THING:
	putref(f, THING_HOME(i));
	putref(f, o->exits);
	putref(f, OWNER(i));
	break;

    case TYPE_ROOM:
	putref(f, o->sp.room.dropto);
	putref(f, o->exits);
	putref(f, OWNER(i));
	break;

    case TYPE_EXIT:
	putref(f, o->sp.exit.ndest);
	for (int j = 0; j < o->sp.exit.ndest; j++) {
	    putref(f, (o->sp.exit.dest)[j]);
	}
	putref(f, OWNER(i));
	break;

    case TYPE_PLAYER:
	putref(f, PLAYER_HOME(i));
	putref(f, o->exits);
	putstring(f, PLAYER_PASSWORD(i));
	break;

    case TYPE_PROGRAM:
	putref(f, OWNER(i));
	break;
    }

    return 0;
}

static int
do_peek(FILE * f)
{
    int peekch;

    ungetc((peekch = getc(f)), f);

    return peekch;
}

dbref
getref(FILE * f)
{
    static char buf[BUFFER_LEN];
    int peekch;

    /*
     * Compiled in with or without timestamps, Sep 1, 1990 by Fuzzy, added to
     * Muck by Kinomon.  Thanks Kino!
     */
    if ((peekch = do_peek(f)) == NUMBER_TOKEN || peekch == '*') {
	return (0);
    }
    fgets(buf, sizeof(buf), f);
    return atol(buf);
}

static char *
getstring(FILE * f)
{
    static char buf[BUFFER_LEN];
    char c;

    if (fgets(buf, sizeof(buf), f) == NULL) {
	return alloc_string("");
    }

    if (strlen(buf) == BUFFER_LEN - 1) {
	/* ignore whatever comes after */
	if (buf[BUFFER_LEN - 2] != '\n')
	    while ((c = fgetc(f)) != '\n') ;
    }
    for (char *p = buf; *p; p++) {
	if (*p == '\n') {
	    *p = '\0';
	    break;
	}
    }

    return alloc_string(buf);
}

static int
db_read_header(FILE * f, int *grow)
{
    int result = 0;
    int load_format = 0;
    char *special;

    *grow = 0;

    if (do_peek(f) != '*') {
	return result;
    }

    special = getstring(f);

    if (!strcmp(special, "***Foxen8 TinyMUCK DUMP Format***")) {
	load_format = 10;
    } else if (!strcmp(special, DB_VERSION_STRING)) {
	load_format = 11;
    }

    free(special);

    *grow = getref(f);

    getref(f);			/* ignore dbflags value */

    tune_load_parms_from_file(f, NOTHING, getref(f));

    return load_format;
}

static void
db_write_header(FILE * f)
{
    putstring(f, DB_VERSION_STRING);

    putref(f, db_top);
    putref(f, DB_PARMSINFO);
    putref(f, tune_count_parms());
    tune_save_parms_to_file(f);
}

dbref
db_write(FILE * f)
{
    db_write_header(f);

    for (dbref i = db_top; i-- > 0;) {
	if (fprintf(f, "#%d\n", i) < 0)
	    abort();
	db_write_object(f, i);
	FLAGS(i) &= ~OBJECT_CHANGED;	/* clear changed flag */
    }

    fseek(f, 0L, 2);
    putstring(f, "***END OF DUMP***");

    fflush(f);
    return db_top;
}

void
getproperties(FILE * f, dbref obj, const char *pdir)
{
    char buf[BUFFER_LEN * 3], *p;
    int datalen;

#ifdef DISKBASE
    /* if no props, then don't bother looking. */
    if (!DBFETCH(obj)->propsfpos)
	return;

    /* seek to the proper file position. */
    fseek(f, DBFETCH(obj)->propsfpos, 0);
#endif

    /* get rid of first line */
    fgets(buf, sizeof(buf), f);

    if (strcmp(buf, "Props*\n")) {
	/* initialize first line stuff */
	fgets(buf, sizeof(buf), f);
	while (1) {
	    /* fgets reads in \n too! */
	    if (!strcmp(buf, "*End*\n"))
		break;
	    p = strchr(buf, PROP_DELIMITER);
	    *(p++) = '\0';	/* Purrrrrrrrrr... */
	    datalen = strlen(p);
	    p[datalen - 1] = '\0';

	    if ((p - buf) >= BUFFER_LEN)
		buf[BUFFER_LEN - 1] = '\0';
	    if (datalen >= BUFFER_LEN)
		p[BUFFER_LEN - 1] = '\0';

	    if ((*p == '^') && (number(p + 1))) {
		add_prop_nofetch(obj, buf, NULL, atol(p + 1));
	    } else {
		if (*buf) {
		    add_prop_nofetch(obj, buf, p, 0);
		}
	    }
	    fgets(buf, sizeof(buf), f);
	}
    } else {
	db_getprops(f, obj, pdir);
    }
}

void
db_free_object(dbref i)
{
    struct object *o;

    /* do this before freeing the name because uncompile_program()
       may try to print some error messages.
       */
    if (Typeof(i) == TYPE_PROGRAM) {
	uncompile_program(i);
        if (PROGRAM_FIRST(i)) {
            free_prog_text(PROGRAM_FIRST(i));
            PROGRAM_SET_FIRST(i, NULL);
        }
	FREE_PROGRAM_SP(i);
    }

    o = DBFETCH(i);
    if (NAME(i))
	free((void *) NAME(i));

#ifdef DISKBASE
    unloadprops_with_prejudice(i);
#else
    if (o->properties) {
	delete_proplist(o->properties);
    }
#endif

    if (Typeof(i) == TYPE_EXIT && o->sp.exit.dest) {
	free(o->sp.exit.dest);
    } else if (Typeof(i) == TYPE_PLAYER) {
	if (PLAYER_PASSWORD(i)) {
	    free((void *) PLAYER_PASSWORD(i));
	}
	if (PLAYER_DESCRS(i)) {
	    free(PLAYER_DESCRS(i));
	    PLAYER_SET_DESCRS(i, NULL);
	    PLAYER_SET_DESCRCOUNT(i, 0);
	}
	ignore_flush_cache(i);
    }
    if (Typeof(i) == TYPE_THING) {
	FREE_THING_SP(i);
    }
    if (Typeof(i) == TYPE_PLAYER) {
	FREE_PLAYER_SP(i);
    }
}

void
db_free(void)
{
    if (db) {
	for (dbref i = 0; i < db_top; i++)
	    db_free_object(i);
	free(db);
	db = 0;
	db_top = 0;
    }
    clear_players();
    clear_primitives();
    recyclable = NOTHING;
}

static void
db_read_object(FILE * f, dbref objno, int dtype)
{
    int tmp, c, prop_flag = 0;
    int j = 0;
    const char *password;
    struct object *o;

    if (dtype == 0)
	return;

    db_clear_object(objno);

    FLAGS(objno) = 0;
    NAME(objno) = getstring(f);

    o = DBFETCH(objno);
    o->location = getref(f);
    o->contents = getref(f);
    o->next = getref(f);

    tmp = getref(f);		/* flags list */
    tmp &= ~DUMP_MASK;
    FLAGS(objno) |= tmp;

    o->ts_created = getref(f);
    o->ts_lastused = getref(f);
    o->ts_usecount = getref(f);
    o->ts_modified = getref(f);

    c = getc(f);
    if (c == '*') {

#ifdef DISKBASE
	o->propsfpos = ftell(f);
	if (o->propsmode == PROPS_CHANGED) {
	    getproperties(f, objno, NULL);
	} else {
	    skipproperties(f, objno);
	}
#else
	getproperties(f, objno, NULL);
#endif

	prop_flag++;
    } else {
	/* do our own getref */
	int sign = 0;
	char buf[BUFFER_LEN];
	int i = 0;

	if (c == '-')
	    sign = 1;
	else if (c != '+') {
	    buf[i] = c;
	    i++;
	}
	while ((c = getc(f)) != '\n') {
	    buf[i] = c;
	    i++;
	}
	buf[i] = '\0';
	j = atol(buf);
	if (sign)
	    j = -j;
    }

    switch (FLAGS(objno) & TYPE_MASK) {
    case TYPE_THING:
	ALLOC_THING_SP(objno);
	THING_SET_HOME(objno, prop_flag ? getref(f) : j);
	o->exits = getref(f);
	OWNER(objno) = getref(f);
	break;
    case TYPE_ROOM:
	o->sp.room.dropto = prop_flag ? getref(f) : j;
	o->exits = getref(f);
	OWNER(objno) = getref(f);
	break;
    case TYPE_EXIT:
	o->sp.exit.ndest = prop_flag ? getref(f) : j;
	if (o->sp.exit.ndest > 0)	/* only allocate space for linked exits */
	    o->sp.exit.dest = malloc(sizeof(dbref) * (size_t)(o->sp.exit.ndest));
	for (j = 0; j < o->sp.exit.ndest; j++) {
	    (o->sp.exit.dest)[j] = getref(f);
	}
	OWNER(objno) = getref(f);
	break;
    case TYPE_PLAYER:
	ALLOC_PLAYER_SP(objno);
	PLAYER_SET_HOME(objno, prop_flag ? getref(f) : j);
	o->exits = getref(f);
	password = getstring(f);
	set_password_raw(objno, password);
	PLAYER_SET_CURR_PROG(objno, NOTHING);
	PLAYER_SET_INSERT_MODE(objno, 0);
	PLAYER_SET_DESCRS(objno, NULL);
	PLAYER_SET_DESCRCOUNT(objno, 0);
	PLAYER_SET_IGNORE_CACHE(objno, NULL);
	PLAYER_SET_IGNORE_COUNT(objno, 0);
	PLAYER_SET_IGNORE_LAST(objno, NOTHING);
	OWNER(objno) = objno;
	add_player(objno);
	break;
    case TYPE_PROGRAM:
	ALLOC_PROGRAM_SP(objno);
	OWNER(objno) = prop_flag ? getref(f) : j;
	FLAGS(objno) &= ~INTERNAL;
	PROGRAM_SET_CURR_LINE(objno, 0);
	PROGRAM_SET_FIRST(objno, 0);
	PROGRAM_SET_CODE(objno, 0);
	PROGRAM_SET_SIZ(objno, 0);
	PROGRAM_SET_START(objno, 0);
	PROGRAM_SET_PUBS(objno, 0);
#ifdef MCP_SUPPORT
	PROGRAM_SET_MCPBINDS(objno, 0);
#endif
	PROGRAM_SET_PROFTIME(objno, 0, 0);
	PROGRAM_SET_PROFSTART(objno, 0);
	PROGRAM_SET_PROF_USES(objno, 0);
	PROGRAM_SET_INSTANCES(objno, 0);
        PROGRAM_SET_INSTANCES_IN_PRIMITIVE(objno, 0);
	break;
    case TYPE_GARBAGE:
	break;
    }
}

static void
autostart_progs(void)
{
    struct line *tmp;

    if (db_conversion_flag) {
	return;
    }

    for (dbref i = 0; i < db_top; i++) {
	if (Typeof(i) == TYPE_PROGRAM) {
	    if ((FLAGS(i) & ABODE) && TrueWizard(OWNER(i))) {
		/* pre-compile AUTOSTART programs. */
		/* They queue up when they finish compiling. */
		/* Uncomment when DBFETCH "does" something. */
		/* FIXME: DBFETCH(i); */
		tmp = PROGRAM_FIRST(i);
		PROGRAM_SET_FIRST(i, (struct line *) read_program(i));
		do_compile(-1, OWNER(i), i, 0);
		free_prog_text(PROGRAM_FIRST(i));
		PROGRAM_SET_FIRST(i, tmp);
	    }
	}
    }
}

dbref
db_read(FILE * f)
{
    dbref grow;
    const char *special;
    char c;

    /* Parse the header */
    db_load_format = db_read_header(f, &grow);
    if (db_load_format == 0) {
	return -1;
    }

    db_grow(grow);

    c = getc(f);		/* get next char */
    for (int i = 0; ; i++) {
	switch (c) {
	case NUMBER_TOKEN:
	    db_read_object(f, getref(f), db_load_format);
	    break;
	case '*':
	    special = getstring(f);
	    if (strcmp(special, "**END OF DUMP***")) {
		free((void *) special);
		return -1;
	    } else {
		free((void *) special);
		special = getstring(f);
		if (special)
		    free((void *) special);

		rewind(f);
		free((void *) getstring(f));
		getref(f);
		getref(f);
		tune_load_parms_from_file(f, NOTHING, getref(f));

		for (dbref j = 0; j < db_top; j++) {
		    if (Typeof(j) == TYPE_GARBAGE) {
			NEXTOBJ(j) = recyclable;
			recyclable = j;
		    }
		}
		autostart_progs();
		return db_top;
	    }
	default:
	    return -1;
	    /* break; */
	}
	c = getc(f);
    }				/* for */
}				/* db_read */

int
objnode_find(objnode *head, dbref data)
{
    objnode *tmp = head;

    while (tmp) {
	if (tmp->data == data) return 1;
	tmp = tmp->next;
    }

    return 0;
}

void
objnode_push(objnode **head, dbref data)
{
    objnode *tmp;

    if (!(tmp = malloc(sizeof(objnode)))) {
        fprintf(stderr, "objnode_push: out of memory\n");
        return;
    }

    tmp->data = data;
    tmp->next = *head;
    *head = tmp;
}

void objnode_pop(objnode **head)
{
    if (!*head) return;

    objnode *tmp = *head;
    *head = tmp->next;
    free(tmp);
}

const char *
unparse_flags(dbref thing)
{
    static char buf[BUFFER_LEN];
    char *p;
    const char *type_codes = "R-EPFG";

    p = buf;
    if (Typeof(thing) != TYPE_THING)
	*p++ = type_codes[Typeof(thing)];
    if (FLAGS(thing) & ~TYPE_MASK) {
	/* print flags */
	if (FLAGS(thing) & WIZARD)
	    *p++ = 'W';
	if (FLAGS(thing) & LINK_OK)
	    *p++ = 'L';

	if (FLAGS(thing) & KILL_OK)
	    *p++ = 'K';

	if (FLAGS(thing) & DARK)
	    *p++ = 'D';
	if (FLAGS(thing) & STICKY)
	    *p++ = 'S';
	if (FLAGS(thing) & QUELL)
	    *p++ = 'Q';
	if (FLAGS(thing) & BUILDER)
	    *p++ = 'B';
	if (FLAGS(thing) & CHOWN_OK)
	    *p++ = 'C';
	if (FLAGS(thing) & JUMP_OK)
	    *p++ = 'J';
	if (FLAGS(thing) & GUEST)
	    *p++ = 'G';
	if (FLAGS(thing) & HAVEN)
	    *p++ = 'H';
	if (FLAGS(thing) & ABODE)
	    *p++ = 'A';
	if (FLAGS(thing) & VEHICLE)
	    *p++ = 'V';
	if (FLAGS(thing) & XFORCIBLE)
	    *p++ = 'X';
	if (FLAGS(thing) & ZOMBIE)
	    *p++ = 'Z';
	if (FLAGS(thing) & YIELD && tp_enable_match_yield)
	    *p++ = 'Y';
	if (FLAGS(thing) & OVERT && tp_enable_match_yield)
	    *p++ = 'O';
	if (MLevRaw(thing)) {
	    *p++ = 'M';
	    switch (MLevRaw(thing)) {
	    case 1:
		*p++ = '1';
		break;
	    case 2:
		*p++ = '2';
		break;
	    case 3:
		*p++ = '3';
		break;
	    }
	}
    }
    *p = '\0';
    return buf;
}

void
unparse_object(dbref player, dbref loc, char *buffer, size_t size)
{
    if (player != NOTHING)
	player = OWNER(player);

    switch (loc) {
    case NOTHING:
	strcpyn(buffer, size, "*NOTHING*");
        return;
    case AMBIGUOUS:
	strcpyn(buffer, size, "*AMBIGUOUS*");
        return;
    case HOME:
	strcpyn(buffer, size, "*HOME*");
        return;
    case NIL:
	strcpyn(buffer, size, "*NIL*");
        return;
    default:
	if (!ObjExists(loc)) {
            strcpyn(buffer, size, "*INVALID*");
            return;
        }

	if ((player == NOTHING) || (!(FLAGS(player) & STICKY) &&
				    (can_link_to(player, NOTYPE, loc) ||
				     ((Typeof(loc) != TYPE_PLAYER) &&
				      (controls_link(player, loc)
				       || (FLAGS(loc) & CHOWN_OK)))))) {
	    /* show everything */
	    snprintf(buffer, size, "%.*s(#%d%s)", (BUFFER_LEN / 2), NAME(loc), loc,
		     unparse_flags(loc));
	} else {
	    /* show only the name */
	    strcpyn(buffer, size, NAME(loc));
	}
    }
}

dbref
remove_first(dbref first, dbref what)
{
    dbref prev;

    /* special case if it's the first one */
    if (first == what) {
	return NEXTOBJ(first);
    } else {
	/* have to find it */
	DOLIST(prev, first) {
	    if (NEXTOBJ(prev) == what) {
		DBSTORE(prev, next, NEXTOBJ(what));
		return first;
	    }
	}
	return first;
    }
}

int
member(dbref thing, dbref list)
{
    DOLIST(list, list) {
	if (list == thing)
	    return 1;
	if ((CONTENTS(list)) && (member(thing, CONTENTS(list)))) {
	    return 1;
	}
    }

    return 0;
}

dbref
reverse(dbref list)
{
    dbref newlist;
    dbref rest;

    newlist = NOTHING;
    while (list != NOTHING) {
	rest = NEXTOBJ(list);
	PUSH(list, newlist);
	DBDIRTY(newlist);
	list = rest;
    }
    return newlist;
}

long
size_object(dbref i, int load)
{
    long byts;
    byts = sizeof(struct object);

    if (NAME(i)) {
        byts += strlen(NAME(i)) + 1;
    }
    byts += size_properties(i, load);

    if (Typeof(i) == TYPE_EXIT && DBFETCH(i)->sp.exit.dest) {
        byts += sizeof(dbref) * (size_t)(DBFETCH(i)->sp.exit.ndest);
    } else if (Typeof(i) == TYPE_PLAYER && PLAYER_PASSWORD(i)) {
        byts += strlen(PLAYER_PASSWORD(i)) + 1;
    } else if (Typeof(i) == TYPE_PROGRAM) {
        byts += size_prog(i);
    }
    return byts;
}


static int
ok_ascii_any(const char *name)
{
    for (const unsigned char *scan = (const unsigned char *) name; *scan; ++scan) {
        if (*scan > 127)
            return 0;
    }
    return 1;
}

int
ok_ascii_thing(const char *name)
{
    return !tp_7bit_thing_names || ok_ascii_any(name);
}

int
ok_ascii_other(const char *name)
{
    return !tp_7bit_other_names || ok_ascii_any(name);
}

int
ok_name(const char *name)
{
    return (name
            && *name
            && *name != NOT_TOKEN
            && *name != LOOKUP_TOKEN
            && *name != REGISTERED_TOKEN
            && *name != NUMBER_TOKEN
            && !strchr(name, ARG_DELIMITER)
            && !strchr(name, AND_TOKEN)
            && !strchr(name, OR_TOKEN)
            && !strchr(name, '\r')
            && !strchr(name, ESCAPE_CHAR)
            && strcasecmp(name, "me")
            && strcasecmp(name, "here")
            && strcasecmp(name, "home")
            && (!*tp_reserved_names || !equalstr((char *) tp_reserved_names, (char *) name)
            ));
}

static dbref
getparent_logic(dbref obj)
{
    if (obj == NOTHING)
        return NOTHING;
    if (Typeof(obj) == TYPE_THING && (FLAGS(obj) & VEHICLE)) {
        obj = THING_HOME(obj);
        if (obj != NOTHING && Typeof(obj) == TYPE_PLAYER) {
            obj = PLAYER_HOME(obj);
        }
    } else {
        obj = LOCATION(obj);
    }
    return obj;
}

dbref
getparent(dbref obj)
{
    dbref ptr, oldptr;

    if (tp_thing_movement) {
        obj = LOCATION(obj);
    } else {
        ptr = getparent_logic(obj);
        do {
            obj = getparent_logic(obj);
        } while (obj != (oldptr = ptr = getparent_logic(ptr)) &&
                 obj != (ptr = getparent_logic(ptr)) &&
                 obj != NOTHING && Typeof(obj) == TYPE_THING);
        if (obj != NOTHING && (obj == oldptr || obj == ptr)) {
            obj = GLOBAL_ENVIRONMENT;
        }
    }
    return obj;
}

int
controls(dbref who, dbref what)
{
    /* No one controls invalid objects */
    if (!OkObj(what))
        return 0;

    who = OWNER(who);

    /* Wizard controls everything */
    if (Wizard(who)) {
#ifdef GOD_PRIV
        if (tp_strict_god_priv && God(OWNER(what)) && !God(who))
            /* Only God controls God's objects */
            return 0;
        else
#endif
            return 1;
    }

    if (tp_realms_control) {
        /* Realm Owner controls everything under his environment. */
        /* To set up a Realm, a Wizard sets the W flag on a room.  The
         * owner of that room controls every Room object contained within
         * that room, all the way to the leaves of the tree.
         * -winged */
        for (dbref index = what; index != NOTHING; index = LOCATION(index)) {
            if ((OWNER(index) == who) && (Typeof(index) == TYPE_ROOM)
                && Wizard(index)) {
                /* Realm Owner doesn't control other Player objects */
                if (Typeof(what) == TYPE_PLAYER) {
                    return 0;
                } else {
                    return 1;
                }
            }
        }
    }

    /* exits are also controlled by the owners of the source and destination */
    /* ACTUALLY, THEY AREN'T.  IT OPENS A BAD MPI SECURITY HOLE. */
    /* any MPI on an exit's @succ or @fail would be run in the context
     * of the owner, which would allow the owner of the src or dest to
     * write malicious code for the owner of the exit to run.  Allowing them
     * control would allow them to modify _ properties, thus enabling the
     * security hole. -winged */
    /*
     * if (Typeof(what) == TYPE_EXIT) {
     *    int     i = DBFETCH(what)->sp.exit.ndest;
     *
     *    while (i > 0) {
     *        if (who == OWNER(DBFETCH(what)->sp.exit.dest[--i]))
     *            return 1;
     *    }
     *    if (who == OWNER(LOCATION(what)))
     *        return 1;
     * }
     */

    /* owners control their own stuff */
    if (who == OWNER(what))
	return 1;

    return (test_lock_false_default(NOTHING, who, what, MESGPROP_OWNLOCK));
}

int
controls_link(dbref who, dbref what)
{
    switch (Typeof(what)) {
    case TYPE_EXIT:
        {
            int i = DBFETCH(what)->sp.exit.ndest;

            while (i > 0) {
                if (controls(who, DBFETCH(what)->sp.exit.dest[--i]))
                    return 1;
            }
            if (who == OWNER(LOCATION(what)))
                return 1;
            return 0;
        }

    case TYPE_ROOM:
        {
            if (controls(who, DBFETCH(what)->sp.room.dropto))
                return 1;
            return 0;
        } 

    case TYPE_PLAYER:
        {
            if (controls(who, PLAYER_HOME(what)))
                return 1;
            return 0;
        }

    case TYPE_THING:
        {
            if (controls(who, THING_HOME(what)))
                return 1;
            return 0;
        }

    case TYPE_PROGRAM:
    default:
        return 0;
    }
}

void
set_source(dbref action, dbref source)
{
    PUSH(action, EXITS(source));
    LOCATION(action) = source;
    DBDIRTY(source);
}

void
unset_source(dbref action)
{
    dbref source = LOCATION(action);
    EXITS(source) = remove_first(EXITS(source), action);
    DBDIRTY(source);
    DBDIRTY(action);
}

/* parse_linkable_dest()
 *
 * A utility for open and link which checks whether a given destination
 * string is valid.  It returns a parsed dbref on success, and NOTHING
 * on failure.
 */

static dbref
parse_linkable_dest(int descr, dbref player, dbref exit, const char *dest_name)
{
    dbref dobj;                 /* destination room/player/thing/link */
    struct match_data md;

    init_match(descr, player, dest_name, NOTYPE, &md);
    match_everything(&md);
    match_home(&md);
    match_nil(&md);

    if ((dobj = noisy_match_result(&md)) == NOTHING) {
        return NOTHING;
    }

    if (!tp_teleport_to_player && Typeof(dobj) == TYPE_PLAYER) {
        char unparse_buf[BUFFER_LEN];
        unparse_object(player, dobj, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "You can't link to players.  Destination %s ignored.",
                unparse_buf);
        return NOTHING;
    }

    if (!can_link(player, exit)) {
        notify(player, "You can't link that.");
        return NOTHING;
    }
    if (!can_link_to(player, Typeof(exit), dobj)) {
        char unparse_buf[BUFFER_LEN];
        unparse_object(player, dobj, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "You can't link to %s.", unparse_buf);
        return NOTHING;
    } else {
        return dobj;
    }
}

static int
_link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list, int dryrun)
{
    char *p, *q;
    int prdest;
    dbref dest;
    int ndest, error;
    char qbuf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];

    prdest = 0;
    ndest = 0;
    error = 0;

    while (*dest_name) {
        skip_whitespace((const char **)&dest_name);
	p = dest_name;
	while (*dest_name && (*dest_name != EXIT_DELIMITER))
	    dest_name++;
	q = (char *) strncpy(qbuf, p, BUFFER_LEN);	/* copy word */
	q[(dest_name - p)] = '\0';	/* terminate it */
	if (*dest_name) {
	    dest_name++;
	    skip_whitespace((const char **)&dest_name);
	}
	if ((dest = parse_linkable_dest(descr, player, exit, q)) == NOTHING)
	    continue;

	if (dest == NIL) {
	    if (!dryrun) {
		notify(player, "Linked to NIL.");
	    }
	    dest_list[ndest++] = dest;
	    continue;
	}

        unparse_object(player, dest, unparse_buf, sizeof(unparse_buf));

	switch (Typeof(dest)) {
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_PROGRAM:
	    if (prdest) {
		notifyf(player,
			"Only one player, room, or program destination allowed. Destination %s ignored.",
                        unparse_buf);

		if (dryrun)
		    error = 1;

		continue;
	    }
	    dest_list[ndest++] = dest;
	    prdest = 1;
	    break;
	case TYPE_THING:
	    dest_list[ndest++] = dest;
	    break;
	case TYPE_EXIT:
	    if (exit_loop_check(exit, dest)) {
		notifyf(player, "Destination %s would create a loop, ignored.",
			unparse_buf);

		if (dryrun)
		    error = 1;

		continue;
	    }
	    dest_list[ndest++] = dest;
	    break;
	default:
	    notify(player, "Internal error: weird object type.");
	    log_status("PANIC: weird object: Typeof(%d) = %d", dest, Typeof(dest));

	    if (dryrun)
		error = 1;

	    break;
	}
	if (!dryrun) {
	    if (dest == HOME) {
		notify(player, "Linked to HOME.");
	    } else {
		notifyf(player, "Linked to %s.", unparse_buf);
	    }
	}

	if (ndest >= MAX_LINKS) {
	    notify(player, "Too many destinations, rest ignored.");

	    if (dryrun)
		error = 1;

	    break;
	}
    }

    if (dryrun && error)
	return 0;

    return ndest;
}

/*
 * link_exit()
 *
 * This routine connects an exit to a bunch of destinations.
 *
 * 'player' contains the player's name.
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of exits.
 *
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.
 *
 */

int
link_exit(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list)
{
    return _link_exit(descr, player, exit, dest_name, dest_list, 0);
}

/*
 * link_exit_dry()
 *
 * like link_exit(), but only checks whether the link would be ok or not.
 * error messages are still output.
 */
int
link_exit_dry(int descr, dbref player, dbref exit, char *dest_name, dbref * dest_list)
{
    return _link_exit(descr, player, exit, dest_name, dest_list, 1);
}

void
register_object(dbref player, dbref location, const char *propdir, char *name, dbref object)
{
    PData mydat;
    PropPtr p;
    dbref prevobj = -50;
    char buf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN], unparse_buf2[BUFFER_LEN];
    char *strval;

    snprintf(buf, sizeof(buf), "%s/%s", propdir, name);

    if ((p = get_property(location, buf))) {
#ifdef DISKBASE
	propfetch(location, p);
#endif
	switch (PropType(p)) {
	case PROP_STRTYP:
	    strval = PropDataStr(p);
	    if (*strval == NUMBER_TOKEN) strval++;
	    if (number(strval)) {
		prevobj = atoi(strval);
	    } else {
		prevobj = AMBIGUOUS;
	    }
	    break;
	case PROP_INTTYP:
	    prevobj = PropDataVal(p);
	    break;
	case PROP_REFTYP:
	    prevobj = PropDataRef(p);
	    break;
	default:
	    break;
        }

	if (prevobj != -50) {
	    unparse_object(player, prevobj, unparse_buf, sizeof(unparse_buf));
	    notifyf_nolisten(player, "Used to be registered as %s: %s", buf, unparse_buf);
	}
    } else if (object == NOTHING) {
	notifyf_nolisten(player, "Nothing to remove.");
	return;
    }

    unparse_object(player, object, unparse_buf, sizeof(unparse_buf));
    unparse_object(player, location, unparse_buf2, sizeof(unparse_buf2));

    if (object == NOTHING) {
	remove_property(location, buf, 0);
	notifyf_nolisten(player, "Registry entry on %s removed.", unparse_buf2);
    } else {
	mydat.flags = PROP_REFTYP;
	mydat.data.ref = object;
	set_property(location, buf, &mydat, 0);
	notifyf_nolisten(player, "Now registered as %s: %s on %s", buf, unparse_buf, unparse_buf2);
    }
}

int
env_distance(dbref from, dbref to)
{
    int distance = 0;
    dbref dest = getparent(to);

    if (from == dest) return 0;

    do {
	distance++;
    } while ((from = getparent(from)) != dest && from != NOTHING);

    return distance;
}
