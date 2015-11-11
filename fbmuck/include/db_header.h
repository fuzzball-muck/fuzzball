#ifndef _DB_HEADER_H
#define _DB_HEADER_H

#include <stdio.h>

#include "db.h"

#define DB_VERSION_STRING "***Foxen9 TinyMUCK DUMP Format***"

#define DB_ID_PARMSINFO		0x00000020 /* legacy database value */

/* identify database format and system parmaters, if possible */
extern int db_read_header(FILE *f, int *grow);

/* output header information to a file. */
extern void db_write_header(FILE *f);

/* read a database reference from a file. */
extern dbref getref(FILE *);

/* read a string from a file */
extern const char *getstring(FILE *f);

#endif /* _DB_HEADER_H */
