#ifndef _RANDOM_H
#define _RANDOM_H

#include "config.h"

#ifdef HAVE_STDINT_H
# include <stdint.h>
typedef uint32_t word32;
typedef uint8_t byte;
#elif defined(HAVE_INTTYPES_H)
# include <inttypes.h>
typedef uint32_t word32;
typedef uint8_t byte;
#elif SIZEOF_LONG_INT==4
typedef unsigned long word32;
typedef unsigned char byte;
#elif SIZEOF_INT==4
typedef unsigned int word32;
typedef unsigned char byte;
#else
typedef unsigned long word32;
typedef unsigned char byte;
#endif

void delete_seed(void *buffer);
void *init_seed(char *seed);
void MD5base64(char *dest, const void *orig, int len);
word32 rnd(void *buffer);

#endif				/* _RANDOM_H */
