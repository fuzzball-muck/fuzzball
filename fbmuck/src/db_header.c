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
db_read_header( FILE *f, int *load_format, dbref *grow)
{
	int result = 0;
	int grow_and_dbflags = 0;
	int parmcnt = 0;
	const char *special;
	char c;

	/* null out the outputs */
	*load_format = 0;
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
		*load_format = 10;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, DB_VERSION_STRING)) {
		*load_format = 11;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, "****Foxen8 Deltas Dump Extention***")) {
		*load_format = 10;
		result |= DB_ID_DELTAS;
	}

	/* All recent versions could have these */
	if ( grow_and_dbflags ) {
		int dbflags;

		*grow = getref(f);
		result |= DB_ID_GROW;

		dbflags = getref(f);

	        /* load the @tune values */
		parmcnt = getref(f);
		tune_load_parms_from_file(f, NOTHING, parmcnt);
		result |= DB_ID_PARMSINFO;
	}

	return result;
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
