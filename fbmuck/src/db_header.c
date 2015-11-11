/*
 * This file includes the logic needed to parse the start of a database file and
 * determine its format.
 *
 * It contains the minimum amount of smarts needed to read this without
 * having to link in everything else.
 *
*/

#include "config.h"

#include "db_header.h"
#include "params.h"
#include "tune.h"

static int
do_peek(FILE * f)
{
	int peekch;

	ungetc((peekch = getc(f)), f);

	return (peekch);
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
	return (atol(buf));
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
db_read_header( FILE *f, int *grow)
{
	int result = 0;
	int dbflags;
	int parmcnt = 0;
	int load_format = 0;
	const char *special;
	char c;

	/* null out the output */
	*grow = 0;

	/* if the db doesn't start with a * it is incredibly ancient and has no header */
	/* this routine can deal with - just return */	
	c = getc( f );
	ungetc( c, f );
	if ( c != '*' ) {
		return result;
	}

	/* read the first line to id it */
	special = getstring(f);

	if (!strcmp(special, "***Foxen8 TinyMUCK DUMP Format***")) {
		load_format = 10;
	} else if (!strcmp(special, DB_VERSION_STRING)) {
		load_format = 11;
	}

	*grow = getref(f);
	dbflags = getref(f);

	parmcnt = getref(f);
	tune_load_parms_from_file(f, NOTHING, parmcnt);

	return load_format;
}

void
db_write_header(FILE *f)
{
        putstring(f, DB_VERSION_STRING );

        putref(f, db_top);
        putref(f, DB_PARMSINFO );
        putref(f, tune_count_parms());
        tune_save_parms_to_file(f);
}
