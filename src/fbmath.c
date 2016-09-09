#include "config.h"

#include "fbmath.h"
#include "inst.h"

float
_int_f_rand(void)
{
    return (rand() / (float) RAND_MAX);
}

int
arith_good(double test)
{
    return ((test <= (double) (MAXINT)) && (test >= (double) (MININT)));
}

int
arith_type(short op1_type, short op2_type)
{
    return ((op1_type == PROG_INTEGER && op2_type == PROG_INTEGER)
            || (op1_type == PROG_OBJECT && op2_type == PROG_INTEGER)
            || (op1_type == PROG_VAR && op2_type == PROG_INTEGER)
            || (op1_type == PROG_LVAR && op2_type == PROG_INTEGER)
            || (op1_type == PROG_FLOAT && op2_type == PROG_FLOAT)
            || (op1_type == PROG_FLOAT && op2_type == PROG_INTEGER)
            || (op1_type == PROG_INTEGER && op2_type == PROG_FLOAT));
}

int
comp_t(short op_type)
{
    return (op_type == PROG_INTEGER || op_type == PROG_FLOAT || op_type == PROG_OBJECT);
}

int
no_good(double test)
{
    return test == INF || test == NINF;
}
