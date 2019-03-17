#ifndef _CRT_MALLOC_H
#define _CRT_MALLOC_H

#include <stddef.h>

#include "config.h"

#undef strdup

#define malloc(x)            CrT_malloc(           x,    __FILE__, __LINE__)
#define calloc(x,y)          CrT_calloc(           x, y, __FILE__, __LINE__)
#define realloc(x,y)         CrT_realloc(          x, y, __FILE__, __LINE__)
#define free(x)              CrT_free(             x,    __FILE__, __LINE__)
#define strdup(x)            CrT_strdup(           x,    __FILE__, __LINE__)
#define alloc_string(x)      CrT_alloc_string(     x,    __FILE__, __LINE__)
#define alloc_prog_string(x) CrT_alloc_prog_string(x,    __FILE__, __LINE__)

char *CrT_alloc_string(const char *, const char *, int);
struct shared_string *CrT_alloc_prog_string(const char *, const char *, int);
void *CrT_calloc(size_t num, size_t size, const char *whatfile, int whatline);
void CrT_free(void *p, const char *whatfile, int whatline);
void *CrT_malloc(size_t size, const char *whatfile, int whatline);
void *CrT_realloc(void *p, size_t size, const char *whatfile, int whatline);
char *CrT_strdup(const char *, const char *, int);
void CrT_summarize(dbref player);
void CrT_summarize_to_file(const char *file, const char *comment);

#endif				/* _CRT_MALLOC_H */
