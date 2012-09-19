#ifndef _P_ERROR_H
#define _P_ERROR_H

extern void prim_clear(PRIM_PROTOTYPE);
extern void prim_clear_error(PRIM_PROTOTYPE);
extern void prim_set_error(PRIM_PROTOTYPE);
extern void prim_is_error(PRIM_PROTOTYPE);
extern void prim_is_set(PRIM_PROTOTYPE);
extern void prim_error_str(PRIM_PROTOTYPE);
extern void prim_error_name(PRIM_PROTOTYPE);
extern void prim_error_bit(PRIM_PROTOTYPE);
extern void prim_error_num(PRIM_PROTOTYPE);

#define PRIMS_ERROR_FUNCS prim_clear, prim_clear_error, prim_is_error,    \
        prim_set_error, prim_is_set, prim_error_str, prim_error_name,     \
        prim_error_bit, prim_error_num

#define PRIMS_ERROR_NAMES "CLEAR","CLEAR_ERROR","ERROR?","SET_ERROR",     \
        "IS_SET?","ERROR_STR","ERROR_NAME","ERROR_BIT","ERROR_NUM"

#define PRIMS_ERROR_CNT 9

#endif /* _P_ERROR_H */

#ifdef DEFINE_HEADER_VERSIONS

#ifndef p_errorh_version
#define p_errorh_version
const char *p_error_h_version = "$RCSfile: p_error.h,v $ $Revision: 1.6 $";
#endif
#else
extern const char *p_error_h_version;
#endif

