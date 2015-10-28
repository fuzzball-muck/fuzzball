#ifndef _DB_HEADER_H
#define _DB_HEADER_H

#include <stdio.h>

#include "db.h"

#define DB_VERSION_STRING "***Foxen9 TinyMUCK DUMP Format***"

/* return masks from db_identify */
#define DB_ID_VERSIONSTRING	0x00000001 /* has a returned **version */
#define DB_ID_DELTAS		0x00000002 /* doing a delta file */

#define DB_ID_GROW 		0x00000010 /* grow parameter will be set */
#define DB_ID_PARMSINFO		0x00000020 /* parmcnt set, need to do a tune_load_parms_from_file */

/* identify which format a database is (or try to) */
extern int db_read_header( FILE *f, const char **version, int *load_format, int *grow, int *parmcnt );

/* Read a database reference from a file. */
extern dbref getref(FILE *);

/* read a string from a file */
extern const char *getstring(FILE *f);

#endif /* _DB_HEADER_H */
