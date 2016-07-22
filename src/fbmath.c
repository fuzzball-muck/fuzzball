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
arith_type(struct inst *op1, struct inst *op2)
{
    return ((op1->type == PROG_INTEGER && op2->type == PROG_INTEGER)    /* real stuff */
            ||(op1->type == PROG_OBJECT && op2->type == PROG_INTEGER)   /* inc. dbref */
            ||(op1->type == PROG_VAR && op2->type == PROG_INTEGER)      /* offset array */
            ||(op1->type == PROG_LVAR && op2->type == PROG_INTEGER)
            || (op1->type == PROG_FLOAT && op2->type == PROG_FLOAT)
            || (op1->type == PROG_FLOAT && op2->type == PROG_INTEGER)
            || (op1->type == PROG_INTEGER && op2->type == PROG_FLOAT));
}

int
comp_t(struct inst *op)
{
    return (op->type == PROG_INTEGER || op->type == PROG_FLOAT || op->type == PROG_OBJECT);
}

int
no_good(double test)
{
    return test == INF || test == NINF;
}
