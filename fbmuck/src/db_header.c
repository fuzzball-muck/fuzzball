/* $Header: */

/*
 * This file includes the logic needed to parse the start of a database file and
 * determine whether it is old or new format, has old or new compression, etc.
 *
 * Needed because olddecomp.c suddenly gets a lot more complicated when we
 * have to handle all the modern formats.
 *
 * It contains the minimum amount of smarts needed to read this without
 * having to link in everything else.
 *
*/

#include "copyright.h"
#include "config.h"

#include "db_header.h"
#include "params.h"

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

static char xyzzybuf[BUFFER_LEN];
const char *
getstring_noalloc(FILE * f)
{
	char *p;
	char c;

	if (fgets(xyzzybuf, sizeof(xyzzybuf), f) == NULL) {
		xyzzybuf[0] = '\0';
		return xyzzybuf;
	}

	if (strlen(xyzzybuf) == BUFFER_LEN - 1) {
		/* ignore whatever comes after */
		if (xyzzybuf[BUFFER_LEN - 2] != '\n')
			while ((c = fgetc(f)) != '\n') ;
	}
	for (p = xyzzybuf; *p; p++) {
		if (*p == '\n') {
			*p = '\0';
			break;
		}
	}

	return xyzzybuf;
}


int
db_read_header( FILE *f, const char **version, int *load_format, dbref *grow, int *parmcnt )
{
	int result = 0;
	int grow_and_dbflags = 0;
	const char *special;
	char c;

	/* null out the putputs */
	*version = NULL;
	*load_format = 0;
	*grow = 0;
	*parmcnt = 0;

	/* if the db doesn't start with a * it is incredibly ancient and has no header */
	/* this routine can deal with - just return */	
	c = getc( f );
	ungetc( c, f );
	if ( c != '*' ) {
		result |= DB_ID_OLDCOMPRESS; /* could be? */
		return result;
	}
	
	/* read the first line to id it */
	special = getstring_noalloc( f );

	/* save whatever the version string was */
	/* NOTE: This only works because we only do getstring_noalloc once */
	result |= DB_ID_VERSIONSTRING;
	*version = special;

	if (!strcmp(special, "***TinyMUCK DUMP Format***")) {
		*load_format = 1;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Lachesis TinyMUCK DUMP Format***") ||
			   !strcmp(special, "***WhiteFire TinyMUCK DUMP Format***")) {
		*load_format = 2;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Mage TinyMUCK DUMP Format***")) {
		*load_format = 3;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Foxen TinyMUCK DUMP Format***")) {
		*load_format = 4;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Foxen2 TinyMUCK DUMP Format***")) {
		*load_format = 5;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Foxen3 TinyMUCK DUMP Format***")) {
		*load_format = 6;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Foxen4 TinyMUCK DUMP Format***")) {
		*load_format = 6;
		*grow = getref(f);
		result |= DB_ID_GROW;
		result |= DB_ID_OLDCOMPRESS;
	} else if (!strcmp(special, "***Foxen5 TinyMUCK DUMP Format***")) {
		*load_format = 7;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, "***Foxen6 TinyMUCK DUMP Format***")) {
		*load_format = 8;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, "***Foxen7 TinyMUCK DUMP Format***")) {
		*load_format = 9;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, "***Foxen8 TinyMUCK DUMP Format***")) {
		*load_format = 10;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, "***Foxen9 TinyMUCK DUMP Format***")) {
		*load_format = 11;
		grow_and_dbflags = TRUE;
	} else if (!strcmp(special, "****Foxen Deltas Dump Extention***")) {
		*load_format = 4;
		result |= DB_ID_DELTAS;
	} else if (!strcmp(special, "****Foxen2 Deltas Dump Extention***")) {
		*load_format = 5;
		result |= DB_ID_DELTAS;
	} else if (!strcmp(special, "****Foxen4 Deltas Dump Extention***")) {
		*load_format = 6;
		result |= DB_ID_DELTAS;
	} else if (!strcmp(special, "****Foxen5 Deltas Dump Extention***")) {
		*load_format = 7;
		result |= DB_ID_DELTAS;
	} else if (!strcmp(special, "****Foxen6 Deltas Dump Extention***")) {
		*load_format = 8;
		result |= DB_ID_DELTAS;
	} else if (!strcmp(special, "****Foxen7 Deltas Dump Extention***")) {
		*load_format = 9;
		result |= DB_ID_DELTAS;
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
		if (dbflags & DB_PARMSINFO) {
			*parmcnt = getref(f);
			result |= DB_ID_PARMSINFO;
		}

		if (dbflags & DB_COMPRESSED) {
			result |= DB_ID_CATCOMPRESS;
		}
	}

	return result;
}
