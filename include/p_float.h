#ifndef P_FLOAT_H
#define P_FLOAT_H

#include "interp.h"

void prim_ceil(PRIM_PROTOTYPE);
void prim_floor(PRIM_PROTOTYPE);
void prim_float(PRIM_PROTOTYPE);
void prim_sqrt(PRIM_PROTOTYPE);
void prim_pow(PRIM_PROTOTYPE);
void prim_frand(PRIM_PROTOTYPE);
void prim_gaussian(PRIM_PROTOTYPE);
void prim_sin(PRIM_PROTOTYPE);
void prim_cos(PRIM_PROTOTYPE);
void prim_tan(PRIM_PROTOTYPE);
void prim_asin(PRIM_PROTOTYPE);
void prim_acos(PRIM_PROTOTYPE);
void prim_atan(PRIM_PROTOTYPE);
void prim_atan2(PRIM_PROTOTYPE);
void prim_exp(PRIM_PROTOTYPE);
void prim_log(PRIM_PROTOTYPE);
void prim_log10(PRIM_PROTOTYPE);
void prim_fabs(PRIM_PROTOTYPE);
void prim_strtof(PRIM_PROTOTYPE);
void prim_ftostr(PRIM_PROTOTYPE);
void prim_ftostrc(PRIM_PROTOTYPE);
void prim_fmod(PRIM_PROTOTYPE);
void prim_modf(PRIM_PROTOTYPE);
void prim_pi(PRIM_PROTOTYPE);
void prim_epsilon(PRIM_PROTOTYPE);
void prim_inf(PRIM_PROTOTYPE);
void prim_round(PRIM_PROTOTYPE);
void prim_xyz_to_polar(PRIM_PROTOTYPE);
void prim_polar_to_xyz(PRIM_PROTOTYPE);
void prim_dist3d(PRIM_PROTOTYPE);
void prim_diff3(PRIM_PROTOTYPE);

#define PRIMS_FLOAT_FUNCS prim_ceil, prim_floor, prim_float, prim_sqrt,  \
	prim_pow, prim_frand, prim_gaussian, prim_sin, prim_cos, prim_tan, prim_asin,   \
	prim_acos, prim_atan, prim_atan2, prim_exp, prim_log, prim_log10,\
	prim_fabs, prim_strtof, prim_ftostr, prim_fmod, prim_modf,       \
	prim_pi, prim_inf, prim_round, prim_dist3d, prim_xyz_to_polar,  \
	prim_polar_to_xyz, prim_diff3, prim_epsilon, prim_ftostrc, prim_pow

#define PRIMS_FLOAT_NAMES "CEIL", "FLOOR", "FLOAT", "SQRT", \
	"POW", "FRAND", "GAUSSIAN", "SIN", "COS", "TAN", "ASIN",        \
	"ACOS", "ATAN", "ATAN2", "EXP", "LOG", "LOG10",     \
	"FABS", "STRTOF", "FTOSTR", "FMOD", "MODF",         \
	"PI", "INF", "ROUND", "DIST3D", "XYZ_TO_POLAR",     \
	"POLAR_TO_XYZ", "DIFF3", "EPSILON", "FTOSTRC", "**"

#endif /* !P_FLOAT_H */
