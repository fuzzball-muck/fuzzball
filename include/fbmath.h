#ifndef _FBMATH_H
#define _FBMATH_H

#include <float.h>
#include <math.h>

#if defined(HUGE_VAL)
# define INF (HUGE_VAL)
# define NINF (-HUGE_VAL)
#else
# define INF (9.9E999)
# define NINF (-9.9E999)
#endif

#ifdef M_PI
# define F_PI M_PI
#else
# define F_PI 3.141592653589793239
#endif

#ifdef M_PI_2
# define H_PI M_PI_2
#else
# define H_PI 1.5707963267949
#endif

/*
#ifdef M_PI_4
# define Q_PI M_PI_4
#else
# define Q_PI 0.7853981633974
#endif
*/

#ifndef MAXINT
#define MAXINT ~(1<<((sizeof(int)*8)-1))
#endif
#ifndef MININT
#define MININT (1<<((sizeof(int)*8)-1))
#endif

#define MIN(p,q) ((p >= q) ? q : p)
#define MAX(p,q) ((p >= q) ? p : q)

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

double _int_f_rand(void);
int arith_good(double test);
int arith_type(short op1_type, short op2_type);
int comp_t(short op_type);
void delete_seed(void *buffer);
void *init_seed(char *seed);
void MD5base64(char *dest, const void *orig, size_t len);
void MD5hex(void *dest, const void *orig, size_t len);
int no_good(double test);
word32 rnd(void *buffer);

#endif				/* _FBMATH_H */
