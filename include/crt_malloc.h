/* CrT's own silly little malloc wrappers for debugging purposes: */

#ifndef _CRT_MALLOC_H
#define _CRT_MALLOC_H

#include <sys/types.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#else
# include <stdlib.h>
#endif

extern void CrT_check(const char *, int);
extern int CrT_check_everything(const char *, int);
extern void *CrT_malloc(size_t size, const char *whatfile, int whatline);
extern void *CrT_calloc(size_t num, size_t size, const char *whatfile, int whatline);
extern void *CrT_realloc(void *p, size_t size, const char *whatfile, int whatline);
extern void CrT_free(void *p, const char *whatfile, int whatline);
extern char *CrT_string_dup(const char *, const char *, int);
extern char *CrT_alloc_string(const char *, const char *, int);
extern struct shared_string *CrT_alloc_prog_string(const char *, const char *, int);


#define malloc(x)            CrT_malloc(           x,    __FILE__, __LINE__)
#define calloc(x,y)          CrT_calloc(           x, y, __FILE__, __LINE__)
#define realloc(x,y)         CrT_realloc(          x, y, __FILE__, __LINE__)
#define free(x)              CrT_free(             x,    __FILE__, __LINE__)
#define string_dup(x)        CrT_string_dup(       x,    __FILE__, __LINE__)
#define alloc_string(x)      CrT_alloc_string(     x,    __FILE__, __LINE__)
#define alloc_prog_string(x) CrT_alloc_prog_string(x,    __FILE__, __LINE__)

#endif /* _CRT_MALLOC_H */
