#ifndef _CRT_MALLOC_H
#define _CRT_MALLOC_H

#include "config.h"

/* {{{ #defines you might want to configure.                            */

/* Select whether to do debug checks also. */
/* These checks eat an additional 8 bytes  */
/* per allocated block, but can be some    */
/* help in tracking down obscure memory    */
/* trashing pointer bugs.                  */
/* #define MALLOC_PROFILING_EXTRA                  */

/* When debugging is selected, we check the */
/* last CRT_NEW_TO_CHECK blocks allocated   */
/* each time we are called, on the theory   */
/* that they are the most likely to have    */
/* been trashed by buggy code:              */
#ifndef CRT_NEW_TO_CHECK
#define CRT_NEW_TO_CHECK (128)  /* Make it a nonzero power of two.  */
#endif

/* When debugging is selected we also check */
/* CRT_OLD_TO_CHECK blocks on the used-ram  */
/* cycling through all allocated blocks in  */
/* time:                                    */
#ifndef CRT_OLD_TO_CHECK
#define CRT_OLD_TO_CHECK (128)
#endif

/* When MALLOC_PROFILING_EXTRA is true, we add the  */
/* following value to the end of block, so  */
/* we can later check to see if it got      */
/* overwritten by something:                */
#ifndef CRT_MAGIC
#define CRT_MAGIC ((char)231)
#endif
/* Something we write into the magic byte  */
/* of a block just before free()ing it, so */
/* as to maybe diagnose repeated free()s:  */
#ifndef CRT_FREE_MAGIC
#define CRT_FREE_MAGIC ((char)~CRT_MAGIC)
#endif

/* }}} */
/* {{{ block_list, a list of all malloc/free/etc calls in host program. */

/* Central data structure: a blocklist with one entry  */
/* for each textually distinct [mc]alloc() call:       */

struct CrT_block_rec {
    const char *file;
    int line;

    long tot_bytes_alloc;
    long tot_allocs_done;
    long live_blocks;
    long live_bytes;
    long max_blocks;
    long max_bytes;
    time_t max_bytes_time;

    struct CrT_block_rec *next;
};
typedef struct CrT_block_rec A_Block;
typedef struct CrT_block_rec *Block;

static Block block_list = NULL;

/* }}} */
/* {{{ Header, a header we add to each block allocated:                 */

struct CrT_header_rec {
    Block b;
    size_t size;
#ifdef MALLOC_PROFILING_EXTRA
    struct CrT_header_rec *next;
    struct CrT_header_rec *prev;
    char *end;
#endif
};
typedef struct CrT_header_rec A_Header;
typedef struct CrT_header_rec *Header;

/* }}} */

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#else
# include <stdlib.h>
#endif

#define malloc(x)            CrT_malloc(           x,    __FILE__, __LINE__)
#define calloc(x,y)          CrT_calloc(           x, y, __FILE__, __LINE__)
#define realloc(x,y)         CrT_realloc(          x, y, __FILE__, __LINE__)
#define free(x)              CrT_free(             x,    __FILE__, __LINE__)
#define string_dup(x)        CrT_string_dup(       x,    __FILE__, __LINE__)
#define alloc_string(x)      CrT_alloc_string(     x,    __FILE__, __LINE__)
#define alloc_prog_string(x) CrT_alloc_prog_string(x,    __FILE__, __LINE__)

char *CrT_alloc_string(const char *, const char *, int);
struct shared_string *CrT_alloc_prog_string(const char *, const char *, int);
void CrT_check(const char *, int);
int CrT_check_everything(const char *, int);
void *CrT_calloc(size_t num, size_t size, const char *whatfile, int whatline);
void CrT_free(void *p, const char *whatfile, int whatline);
void *CrT_malloc(size_t size, const char *whatfile, int whatline);
void *CrT_realloc(void *p, size_t size, const char *whatfile, int whatline);
char *CrT_string_dup(const char *, const char *, int);
void CrT_summarize(dbref player);
void CrT_summarize_to_file(const char *file, const char *comment);

#endif				/* _CRT_MALLOC_H */
