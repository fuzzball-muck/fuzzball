/** @file crt_malloc.h
 *
 * This is the header file for the crt_malloc system, which is used for
 * memory profiling.  This system is only turned on when MALLOC_PROFILING
 * is defined.
 *
 * It overrides memory handling calls in order to inject profiling
 * equivalents.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */
#ifndef CRT_MALLOC_H
#define CRT_MALLOC_H

#include <stddef.h>

#include "config.h"
#include "fbmuck.h"

/* This is a define for some compilers */
#undef strdup

/* These defines override memory allocators with CrT equivalents. */

#define malloc(x)            CrT_malloc(           x,    __FILE__, __LINE__)
#define calloc(x,y)          CrT_calloc(           x, y, __FILE__, __LINE__)
#define realloc(x,y)         CrT_realloc(          x, y, __FILE__, __LINE__)
#define free(x)              CrT_free(             x,    __FILE__, __LINE__)
#define strdup(x)            CrT_strdup(           x,    __FILE__, __LINE__)
#define alloc_string(x)      CrT_alloc_string(     x,    __FILE__, __LINE__)
#define alloc_prog_string(x) CrT_alloc_prog_string(x,    __FILE__, __LINE__)

/**
 * This is the wrapper for alloc_string
 *
 * It works just like alloc_string, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param string the string to copy
 * @param file the originating file
 * @param line the originating line
 * @return a copy of 'string'
 */
char *CrT_alloc_string(const char *, const char *, int);

/**
 * This is the wrapper for alloc_prog_string
 *
 * It works just like alloc_prog_string, except it takes a file and line for
 * tracking purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param s the string to copy
 * @param file the originating file
 * @param line the originating line
 * @return a copy of 's' wrapped in a shared_string struct
 */
struct shared_string *CrT_alloc_prog_string(const char *, const char *, int);

/**
 * This is the wrapper for calloc
 *
 * It works just like calloc, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param num the number of elements to allocate
 * @param siz the size of the elements to allocate
 * @param file the originating file
 * @param line the originating line
 * @return allocated and tracked memory block
 */
void *CrT_calloc(size_t num, size_t size, const char *whatfile, int whatline);

/**
 * This is the wrapper for free
 *
 * It works just like free, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param p the memory block to free
 * @param file the originating file
 * @param line the originating line
 */
void CrT_free(void *p, const char *whatfile, int whatline);

/**
 * This is the wrapper for malloc.
 *
 * It works just like malloc except it takes a file and line argument pair
 * in order to track where the call came from.  __FILE__ and __LINE__ should
 * be used.  The memory passed into 'file' must not be deleted; a copy is
 * not made.
 *
 * @param size the size of the memory block to allocate.
 * @param file the file the originated this request.
 * @param line the line that originaed this request.
 */
void *CrT_malloc(size_t size, const char *whatfile, int whatline);

/**
 * This is the wrapper for realloc
 *
 * It works just like realloc, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param p the memory block to resize
 * @param size the new size
 * @param file the originating file
 * @param line the originating line
 * @return allocated and tracked memory block
 */
void *CrT_realloc(void *p, size_t size, const char *whatfile, int whatline);

/**
 * This is the wrapper for strdup
 *
 * It works just like strdup, except it takes a file and line for
 * tracking purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param s the string to copy
 * @param file the originating file
 * @param line the originating line
 * @return a copy of 's'
 */
char *CrT_strdup(const char *, const char *, int);

/**
 * Send memory profile summary to a given player
 *
 * @param player the dbref of the player to notify
 */
void CrT_summarize(dbref player);

/**
 * Summarize ram usage and write it to a file
 *
 * @private
 * @param file the file name to write to.
 * @param comment a comment to put at the top of the file, or NULL for none.
 */
void CrT_summarize_to_file(const char *file, const char *comment);

#endif /* !CRT_MALLOC_H */
