#ifndef _FBMATH_H
#define _FBMATH_H

#include "inst.h"

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
# define NF_PI -M_PI
#else
# define F_PI 3.141592653589793239
# define NF_PI -3.141592653589793239
#endif

#ifdef M_PI_2
# define H_PI M_PI_2
# define NH_PI -M_PI_2
#else
# define H_PI 1.5707963267949
# define NH_PI -1.5707963267949
#endif

#ifdef M_PI_4
# define Q_PI M_PI_4
# define NQ_PI -M_PI_4
#else
# define Q_PI 0.7853981633974
# define NQ_PI -0.7853981633974
#endif

#ifndef MAXINT
#define MAXINT ~(1<<((sizeof(int)*8)-1))
#endif
#ifndef MININT
#define MININT (1<<((sizeof(int)*8)-1))
#endif

#define MIN(p,q) ((p >= q) ? q : p)
#define MAX(p,q) ((p >= q) ? p : q)

float _int_f_rand(void);
int arith_good(double test);
int arith_type(struct inst *op1, struct inst *op2);
int comp_t(struct inst *op);
int no_good(double test);

#endif				/* _FBMATH_H */
