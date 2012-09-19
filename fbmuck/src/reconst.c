#include "copyright.h"
#include "config.h"
#include "params.h"
#include "tune.h"

#include "db.h"

char *
string_dup(const char *s)
{
	char *p;

	p = (char *) malloc(strlen(s) + 1);
	if (!p)
		return p;
	strcpy(p, s);  /* Guaranteed enough space. */
	return p;
}


int *nexted;

void
readd_contents(dbref obj)
{
	dbref what;
	dbref where = db[obj].location;

	switch (db[obj].flags & TYPE_MASK) {
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_PROGRAM:
	case TYPE_PLAYER:
		if (db[where].contents == NOTHING) {
			db[where].contents = obj;
			return;
		}
		what = db[where].contents;
		while (db[what].next != NOTHING)
			what = db[what].next;
		db[what].next = obj;
		break;
	case TYPE_EXIT:
		switch (db[where].flags & TYPE_MASK) {
		case TYPE_ROOM:
		case TYPE_THING:
		case TYPE_PLAYER:
			if (db[where].exits == NOTHING) {
				db[where].exits = obj;
				return;
			}
			what = db[where].exits;
			break;
		}
		while (db[what].next != NOTHING)
			what = db[what].next;
		db[what].next = obj;
		break;
	case TYPE_GARBAGE:
		break;
	}
}

void
check_contents(dbref obj)
{
	dbref where, lastwhere;
	dbref whattype = db[obj].flags & TYPE_MASK;

	if ((whattype == TYPE_PROGRAM) || (whattype == TYPE_EXIT) || (whattype == TYPE_GARBAGE))
		return;

	if (db[obj].contents != NOTHING) {
		while ((db[obj].contents != NOTHING) && (db[db[obj].contents].location != obj)) {
			lastwhere = db[obj].contents;
			db[obj].contents = db[lastwhere].next;
			db[lastwhere].next = NOTHING;
			readd_contents(lastwhere);
		}
		where = db[obj].contents;
		if (where != NOTHING) {
			while (db[where].next != NOTHING) {
				if (db[db[where].next].location != obj) {
					lastwhere = db[where].next;
					db[where].next = db[lastwhere].next;
					db[lastwhere].next = NOTHING;
					readd_contents(lastwhere);
				} else {
					where = db[where].next;
				}
			}
		}
	}
	if (db[obj].exits != NOTHING) {
		while ((db[obj].exits != NOTHING) && (db[db[obj].exits].location != obj)) {
			lastwhere = db[obj].exits;
			db[obj].exits = db[lastwhere].next;
			db[lastwhere].next = NOTHING;
			readd_contents(lastwhere);
		}
		where = db[obj].exits;
		if (where != NOTHING) {
			while (db[where].next != NOTHING) {
				if (db[db[where].next].location != obj) {
					lastwhere = db[where].next;
					db[where].next = db[lastwhere].next;
					db[lastwhere].next = NOTHING;
					readd_contents(lastwhere);
				} else {
					where = db[where].next;
				}
			}
		}
	}
}


void
check_common(dbref obj)
{
	char buf[BUFFER_LEN];

	if (!db[obj].name && db[obj].location == NOTHING) {
		db[obj].flags = TYPE_GARBAGE;
		db[obj].name = "<garbage>";
	}

	/* check name */
	if (!db[obj].name) {
		snprintf(buf, sizeof(buf), "Unknown%d", obj);
		db[obj].name = alloc_string(buf);
	}

	/* check location */
	if (db[obj].location >= db_top)
		db[obj].location = tp_player_start;

	if (db[obj].contents < db_top)
		nexted[db[obj].contents] = obj;
	else
		db[obj].contents = NOTHING;

	if (db[obj].next < db_top)
		nexted[db[obj].next] = obj;
	else
		db[obj].next = NOTHING;
}

void
check_room(dbref obj)
{
	if (db[obj].sp.room.dropto >= db_top ||
		(((db[db[obj].sp.room.dropto].flags & TYPE_MASK) != TYPE_ROOM) &&
		 db[obj].sp.room.dropto != NOTHING && db[obj].sp.room.dropto != HOME))
		db[obj].sp.room.dropto = NOTHING;

	if (db[obj].exits < db_top)
		nexted[db[obj].exits] = obj;
	else
		db[obj].exits = NOTHING;

	if (db[obj].owner >= db_top || ((db[db[obj].owner].flags & TYPE_MASK) != TYPE_PLAYER))
		db[obj].owner = GOD;
}

void
check_thing(dbref obj)
{
	if (db[obj].sp.thing.home >= db_top ||
		(((db[db[obj].sp.thing.home].flags & TYPE_MASK) != TYPE_ROOM) &&
		 ((db[db[obj].sp.thing.home].flags & TYPE_MASK) != TYPE_PLAYER)))
		db[obj].sp.thing.home = tp_player_start;

	if (db[obj].exits < db_top)
		nexted[db[obj].exits] = obj;
	else
		db[obj].exits = NOTHING;

	if (db[obj].owner >= db_top || ((db[db[obj].owner].flags & TYPE_MASK) != TYPE_PLAYER))
		db[obj].owner = GOD;
}

void
check_exit(dbref obj)
{
	int i;

	for (i = 0; i < db[obj].sp.exit.ndest; i++)
		if ((db[obj].sp.exit.dest)[i] >= db_top)
			(db[obj].sp.exit.dest)[i] = NOTHING;

	if (db[obj].owner >= db_top || ((db[db[obj].owner].flags & TYPE_MASK) != TYPE_PLAYER))
		db[obj].owner = GOD;
}

void
check_player(dbref obj)
{
	if (db[obj].sp.player.home >= db_top ||
		((db[db[obj].sp.player.home].flags & TYPE_MASK) != TYPE_ROOM)) db[obj].sp.player.home =
				tp_player_start;

	if (db[obj].exits < db_top)
		nexted[db[obj].exits] = obj;
	else
		db[obj].exits = NOTHING;
}

void
check_program(dbref obj)
{
	if (db[obj].owner >= db_top || ((db[db[obj].owner].flags & TYPE_MASK) != TYPE_PLAYER))
		db[obj].owner = GOD;
}

FILE *input_file;
FILE *delta_infile = NULL;
FILE *delta_outfile = NULL;
FILE *output_file;

char *in_filename;
char *out_filename;

void
main(int argc, char **argv)
{
	int i;

	if (argc < 3 || argc > 3) {
		fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
		return;
	}

	if (!string_compare(argv[1], argv[2])) {
		fprintf(stderr, "%s: input and output files can't have same name.\n", argv[0]);
		return;
	}

	in_filename = (char *) string_dup(argv[1]);
	if ((input_file = fopen(in_filename, "rb")) == NULL) {
		fprintf(stderr, "%s: unable to open input file.\n", argv[0]);
		return;
	}

	out_filename = (char *) string_dup(argv[2]);
	if ((output_file = fopen(out_filename, "wb")) == NULL) {
		fprintf(stderr, "%s: unable to write to output file.\n", argv[0]);
		return;
	}

	db_free();
	db_read(input_file);

	nexted = malloc((db_top + 1) * sizeof(int));

	for (i = 0; i < db_top; i++)
		nexted[i] = NOTHING;

	for (i = 0; i < db_top; i++) {
		check_common(i);
		switch (db[i].flags & TYPE_MASK) {
		case TYPE_ROOM:
			check_room(i);
			break;
		case TYPE_THING:
			check_thing(i);
			break;
		case TYPE_EXIT:
			check_exit(i);
			break;
		case TYPE_PLAYER:
			check_player(i);
			break;
		case TYPE_PROGRAM:
			check_program(i);
			break;
		case TYPE_GARBAGE:
			break;
		default:
			db[i].flags &= ~TYPE_MASK;
			db[i].flags |= TYPE_GARBAGE;
			break;
		}
	}
	for (i = 0; i < db_top; i++)
		if ((nexted[i] == NOTHING) && (i != 0))
			readd_contents(i);
	for (i = 0; i < db_top; i++)
		check_contents(i);

	db_write(output_file);
}

/* dummy compiler */
void
do_compile(int descr, dbref p, dbref pr, int force_err_disp)
{
}
struct macrotable
*
new_macro(const char *name, const char *definition, dbref player)
{
	return NULL;
}

void
log_status(format, p1, p2, p3, p4, p5, p6, p7, p8)
char *format, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
}

void
clear_players(void)
{
}

void
clear_primitives(void)
{
}

void
add_event(int descr, dbref player, dbref loc, dbref trig, int dtime, dbref program,
		  struct frame *fr, const char *strdata)
{
}

void
init_primitives(void)
{
}

void
add_player(dbref who)
{
}

int
equalstr(const char *s1, const char *s2)
{
}

char *
do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf, const char *abuf, char *outbuf, int outbuflen, int mesgtyp)
{
	return NULL;
}
static const char *reconst_c_version = "$RCSfile: reconst.c,v $ $Revision: 1.7 $";
const char *get_reconst_c_version(void) { return reconst_c_version; }
