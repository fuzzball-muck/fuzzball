#ifndef _P_ERROR_H
#define _P_ERROR_H

#define ERROR_NAME_MAX_LEN 15
#define ERROR_STRING_MAX_LEN 80
#define ERROR_NUM 5

union error_mask {
    struct {
        unsigned int div_zero:1;        /* Divide by zero */
        unsigned int nan:1;     /* Result would not be a number */
        unsigned int imaginary:1;       /* Result would be imaginary */
        unsigned int f_bounds:1;        /* Float boundary error */
        unsigned int i_bounds:1;        /* Integer boundary error */
    } error_flags;
    int is_flags;
};

struct err_type {
    int error_bit;
    char error_name[ERROR_NAME_MAX_LEN];
    char error_string[ERROR_STRING_MAX_LEN];
};

void prim_clear(PRIM_PROTOTYPE);
void prim_clear_error(PRIM_PROTOTYPE);
void prim_set_error(PRIM_PROTOTYPE);
void prim_is_error(PRIM_PROTOTYPE);
void prim_is_set(PRIM_PROTOTYPE);
void prim_error_str(PRIM_PROTOTYPE);
void prim_error_name(PRIM_PROTOTYPE);
void prim_error_bit(PRIM_PROTOTYPE);
void prim_error_num(PRIM_PROTOTYPE);

#define PRIMS_ERROR_FUNCS prim_clear, prim_clear_error, prim_is_error,    \
        prim_set_error, prim_is_set, prim_error_str, prim_error_name,     \
        prim_error_bit, prim_error_num

#define PRIMS_ERROR_NAMES "CLEAR","CLEAR_ERROR","ERROR?","SET_ERROR",     \
        "IS_SET?","ERROR_STR","ERROR_NAME","ERROR_BIT","ERROR_NUM"

#define PRIMS_ERROR_CNT 9

#endif				/* _P_ERROR_H */
