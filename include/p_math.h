#ifndef P_MATH_H
#define P_MATH_H

#include "interp.h"

void prim_add(PRIM_PROTOTYPE);
void prim_subtract(PRIM_PROTOTYPE);
void prim_multiply(PRIM_PROTOTYPE);
void prim_divide(PRIM_PROTOTYPE);
void prim_mod(PRIM_PROTOTYPE);
void prim_bitor(PRIM_PROTOTYPE);
void prim_bitxor(PRIM_PROTOTYPE);
void prim_bitand(PRIM_PROTOTYPE);
void prim_bitshift(PRIM_PROTOTYPE);
void prim_and(PRIM_PROTOTYPE);
void prim_or(PRIM_PROTOTYPE);
void prim_xor(PRIM_PROTOTYPE);
void prim_not(PRIM_PROTOTYPE);
void prim_lessthan(PRIM_PROTOTYPE);
void prim_greathan(PRIM_PROTOTYPE);
void prim_equal(PRIM_PROTOTYPE);
void prim_lesseq(PRIM_PROTOTYPE);
void prim_greateq(PRIM_PROTOTYPE);
void prim_random(PRIM_PROTOTYPE);
void prim_int(PRIM_PROTOTYPE);
void prim_srand(PRIM_PROTOTYPE);
void prim_getseed(PRIM_PROTOTYPE);
void prim_setseed(PRIM_PROTOTYPE);
void prim_plusplus(PRIM_PROTOTYPE);
void prim_minusminus(PRIM_PROTOTYPE);
void prim_abs(PRIM_PROTOTYPE);
void prim_sign(PRIM_PROTOTYPE);
void prim_notequal(PRIM_PROTOTYPE);

#define PRIMS_MATH_FUNCS prim_add, prim_subtract, prim_multiply, prim_divide, \
    prim_mod, prim_bitor, prim_bitxor, prim_bitand, prim_bitshift, prim_and,  \
    prim_or, prim_not, prim_lessthan, prim_greathan, prim_equal, prim_lesseq, \
    prim_greateq, prim_random, prim_int, prim_srand, prim_setseed, prim_xor,  \
    prim_getseed, prim_plusplus, prim_minusminus, prim_abs, prim_sign,        \
    prim_notequal, prim_bitand, prim_bitor, prim_bitxor, prim_bitshift

#define PRIMS_MATH_NAMES  "+",  "-",  "*",  "/", "%", "BITOR", \
    "BITXOR", "BITAND", "BITSHIFT", "AND", "OR",  "NOT",  "<", \
     ">",  "=",  "<=", ">=", "RANDOM", "INT", "SRAND",         \
     "SETSEED", "XOR", "GETSEED", "++", "--", "ABS", "SIGN",   \
     "!=", "&", "|", "^", "<<"

#endif /* !P_MATH_H */
