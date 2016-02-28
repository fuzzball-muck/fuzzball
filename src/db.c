#include "config.h"

#include <ctype.h>

#include "db.h"
#include "props.h"
#include "params.h"
#include "interface.h"
#include "tune.h"
#include "externs.h"

struct object *db = 0;
dbref db_top = 0;
dbref recyclable = NOTHING;
int db_load_format = 0;

#ifndef DB_INITIAL_SIZE
#define DB_INITIAL_SIZE 10000
#endif							/* DB_INITIAL_SIZE */

#ifdef DB_DOUBLING

dbref db_size = DB_INITIAL_SIZE;

#endif							/* DB_DOUBLING */

struct macrotable *macrotop;

#ifndef MALLOC_PROFILING
extern char *alloc_string(const char *);
#endif

extern short db_conversion_flag;

#ifdef DB_DOUBLING

static void
db_grow(dbref newtop)
{
	struct object *newdb;

	if (newtop > db_top) {
		db_top = newtop;
		if (!db) {
			/* make the initial one */
			db_size = DB_INITIAL_SIZE;
			while (db_top > db_size)
				db_size += 1000;
			if ((db = (struct object *) malloc(db_size * sizeof(struct object))) == 0) {
				abort();
			}
		}
		/* maybe grow it */
		if (db_top > db_size) {
			/* make sure it's big enough */
			while (db_top > db_size)
				db_size += 1000;
			if ((newdb = (struct object *) realloc((void *) db,
												   db_size * sizeof(struct object))) == 0) {
				abort();
			}
			db = newdb;
		}
	}
}

#else							/* DB_DOUBLING */

static void
db_grow(dbref newtop)
{
	struct object *newdb;

	if (newtop > db_top) {
		db_top = newtop;
		if (db) {
			if ((newdb = (struct object *)
				 realloc((void *) db, db_top * sizeof(struct object))) == 0) {
				abort();
			}
			db = newdb;
		} else {
			/* make the initial one */
			int startsize = (newtop >= DB_INITIAL_SIZE) ? newtop : DB_INITIAL_SIZE;

			if ((db = (struct object *)
				 malloc(startsize * sizeof(struct object))) == 0) {
				abort();
			}
		}
	}
}

#endif							/* DB_DOUBLING */

void
db_clear_object(dbref i)
{
	struct object *o = DBFETCH(i);

	bzero(o, sizeof(struct object));

	NAME(i) = 0;
	ts_newobject(o);
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
putproperties_rec(FILE * f, const char *dir, dbref obj)
{
	PropPtr pref;
	PropPtr p, pptr;
	char buf[BUFFER_LEN];
	char name[BUFFER_LEN];

	pref = first_prop_nofetch(obj, dir, &pptr, name, sizeof(name));
	while (pref) {
		p = pref;
		db_putprop(f, dir, p);
		strcpyn(buf, sizeof(buf), dir);
		strcatn(buf, sizeof(buf), name);
		if (PropDir(p)) {
			strcatn(buf, sizeof(buf), "/");
			putproperties_rec(f, buf, obj);
		}
		pref = next_prop(pptr, pref, name, sizeof(name));
	}
}

void
putproperties(FILE * f, dbref obj)
{
	putstring(f, "*Props*");
	db_dump_props(f, obj);
	putstring(f, "*End*");
}


extern FILE *input_file;

int
db_write_object(FILE * f, dbref i)
{
	struct object *o = DBFETCH(i);
	int j;
#ifdef DISKBASE
	long tmppos;
#endif							/* DISKBASE */

	putstring(f, NAME(i));
	putref(f, o->location);
	putref(f, o->contents);
	putref(f, o->next);
	putref(f, FLAGS(i) & ~DUMP_MASK);	/* write non-internal flags */

	putref(f, o->ts.created);
	putref(f, o->ts.lastused);
	putref(f, o->ts.usecount);
	putref(f, o->ts.modified);


#ifdef DISKBASE
	tmppos = ftell(f) + 1;
	putprops_copy(f, i);
	o->propsfpos = tmppos;
	undirtyprops(i);
#else							/* !DISKBASE */
	putproperties(f, i);
#endif							/* DISKBASE */


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
		for (j = 0; j < o->sp.exit.ndest; j++) {
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
	if ((peekch = do_peek(f)) == NUMBER_TOKEN || peekch == LOOKUP_TOKEN) {
		return (0);
	}
	fgets(buf, sizeof(buf), f);
	return atol(buf);
}

const char *
getstring(FILE * f)
{
	static char buf[BUFFER_LEN];
	char *p;
	char c;

	if (fgets(buf, sizeof(buf), f) == NULL) {
		return alloc_string("");
	}

	if (strlen(buf) == BUFFER_LEN - 1) {
		/* ignore whatever comes after */
		if (buf[BUFFER_LEN - 2] != '\n')
			while ((c = fgetc(f)) != '\n') ;
	}
	for (p = buf; *p; p++) {
		if (*p == '\n') {
			*p = '\0';
			break;
		}
	}

	return alloc_string(buf);
}

int
db_read_header(FILE *f, int *grow)
{
	int result = 0;
	int load_format = 0;
	const char *special;

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

	*grow = getref(f);

	getref(f); /* ignore dbflags value */

	tune_load_parms_from_file(f, NOTHING, getref(f));

	return load_format;
}

void
db_write_header(FILE *f)
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
	dbref i;

	db_write_header(f);

	for (i = db_top; i-- > 0;) {
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
			if (!strcmp(buf, "***Property list end ***\n") || !strcmp(buf, "*End*\n"))
				break;
			p = index(buf, PROP_DELIMITER);
			*(p++) = '\0';		/* Purrrrrrrrrr... */
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
		free((void *) o->sp.exit.dest);
    } else if (Typeof(i) == TYPE_PLAYER) {
        if (PLAYER_PASSWORD(i)) {
			free((void*)PLAYER_PASSWORD(i));
        }
        if (PLAYER_DESCRS(i)){ 
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
	if (Typeof(i) == TYPE_PROGRAM) {
		uncompile_program(i);
		FREE_PROGRAM_SP(i);
	}
}

void
db_free(void)
{
	dbref i;

	if (db) {
		for (i = 0; i < db_top; i++)
			db_free_object(i);
		free((void *) db);
		db = 0;
		db_top = 0;
	}
	clear_players();
	clear_primitives();
	recyclable = NOTHING;
}

void
db_read_object(FILE * f, dbref objno, int dtype)
{
	int tmp, c, prop_flag = 0;
	int j = 0;
	const char *password;
	struct object *o;

	db_clear_object(objno);

	FLAGS(objno) = 0;
	NAME(objno) = getstring(f);

	o = DBFETCH(objno);
	o->location = getref(f);
	o->contents = getref(f);
	o->next = getref(f);

	tmp = getref(f);			/* flags list */
	tmp &= ~DUMP_MASK;
	FLAGS(objno) |= tmp;

	o->ts.created = getref(f);
	o->ts.lastused = getref(f);
	o->ts.usecount = getref(f);
	o->ts.modified = getref(f);

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
			o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * (o->sp.exit.ndest));
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
		break;
	case TYPE_GARBAGE:
		break;
	}
}

void
autostart_progs(void)
{
	dbref i;
	struct line *tmp;

	if (db_conversion_flag) {
		return;
	}

	for (i = 0; i < db_top; i++) {
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
	int i;
	dbref grow;
	const char *special;
	char c;

	/* Parse the header */
	db_load_format = db_read_header(f, &grow);
	if (db_load_format == 0) {
		return -1;
	}

	db_grow(grow);

	c = getc(f);			/* get next char */
	for (i = 0;; i++) {
		switch (c) {
		case NUMBER_TOKEN:
			db_read_object(f, getref(f), db_load_format);
			break;
		case LOOKUP_TOKEN:
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

				for (i = 0; i < db_top; i++) {
					if (Typeof(i) == TYPE_GARBAGE) {
						NEXTOBJ(i) = recyclable;
						recyclable = i;
					}
				}
				autostart_progs();
				return db_top;
			}
			break;
		default:
			return -1;
			/* break; */
		}
		c = getc(f);
	}							/* for */
}								/* db_read */
