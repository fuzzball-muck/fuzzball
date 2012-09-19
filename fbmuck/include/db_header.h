/* $Header: /cvsroot/fbmuck/fbmuck/include/db_header.h,v 1.1 2007/03/09 15:53:56 winged Exp $ */

#include "copyright.h"

#ifndef __DB_HEADER_H
#define __DB_HEADER_H

#include <stdio.h>

#include "db.h"

#define DB_VERSION_STRING "***Foxen9 TinyMUCK DUMP Format***"

/* return masks from db_identify */
#define DB_ID_VERSIONSTRING	0x00000001 /* has a returned **version */
#define DB_ID_DELTAS		0x00000002 /* doing a delta file */

#define DB_ID_GROW 		0x00000010 /* grow parameter will be set */
#define DB_ID_PARMSINFO		0x00000020 /* parmcnt set, need to do a tune_load_parms_from_file */
#define DB_ID_OLDCOMPRESS	0x00000040 /* whether it COULD be */
#define DB_ID_CATCOMPRESS	0x00000080 /* whether it COULD be */

/* identify which format a database is (or try to) */
extern int db_read_header( FILE *f, const char **version, int *load_format, int *grow, int *parmcnt );

/* Read a database reference from a file. */
extern dbref getref(FILE *);

/* read a string from file, don't require any allocation - however the result */
/* will always point to the same static buffer, so make a copy if you need it */
/* to hang around */
extern const char* getstring_noalloc(FILE *f);

#endif /* DB_HEADER_H */
