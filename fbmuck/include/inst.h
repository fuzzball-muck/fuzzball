#ifndef _INST_H
#define _INST_H

#define IN_JMP        1
#define IN_READ       2
#define IN_SLEEP      3
#define IN_CALL       4
#define IN_EXECUTE    5
#define IN_RET        6
#define IN_EVENT_WAITFOR 7
#define IN_CATCH      8
#define IN_CATCH_DETAILED  9

#define BASE_MIN 1
#define BASE_MAX (9 + PRIMS_CONNECTS_CNT + PRIMS_DB_CNT + PRIMS_MATH_CNT + \
    PRIMS_MISC_CNT + PRIMS_PROPS_CNT + PRIMS_STACK_CNT + PRIMS_STRINGS_CNT + \
    PRIMS_ARRAY_CNT + PRIMS_FLOAT_CNT + PRIMS_ERROR_CNT + PRIMS_MCP_CNT \
  + PRIMS_REGEX_CNT + PRIMS_INTERNAL_CNT)

/* now refer to tables to map instruction number to name */
extern const char *base_inst[];

extern char *insttotext(struct frame *, int, struct inst *, char *, int, int, dbref, int);

/* and declare debug instruction diagnostic routine */
extern char *debug_inst(struct frame *, int, struct inst *, int, struct inst *, char *, int, int, dbref);

#endif /* _INST_H */
