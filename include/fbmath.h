#ifndef _FBMATH_H
#define _FBMATH_H

#include <float.h>
#include <math.h>
#include <stdint.h>

#if defined(HUGE_VAL)
# define INF (HUGE_VAL)
# define NINF (-HUGE_VAL)
#else
# define INF (9.9E999)
# define NINF (-9.9E999)
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
# define M_PI_2 1.57079632679489661923
#endif

#define MIN(p,q) ((p >= q) ? q : p)
#define MAX(p,q) ((p >= q) ? p : q)

double _int_f_rand(void);
int arith_good(double test);
int arith_type(short op1_type, short op2_type);
int comp_t(short op_type);
void delete_seed(void *buffer);
void *init_seed(char *seed);
void MD5base64(char *dest, const void *orig, size_t len);
void MD5hex(void *dest, const void *orig, size_t len);
int no_good(double test);
uint32_t rnd(void *buffer);

#endif				/* _FBMATH_H */
