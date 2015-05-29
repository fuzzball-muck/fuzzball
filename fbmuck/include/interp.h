#ifndef _INTERP_H
#define _INTERP_H

/* Stuff the interpreter needs. */
/* Some icky machine/compiler #defines. --jim */
#ifdef MIPS
typedef char *voidptr;

#define MIPSCAST (char *)
#else
typedef void *voidptr;

#define MIPSCAST
#endif

#define UPCASE(x) (toupper(x))
#define DOWNCASE(x) (tolower(x))

void purge_try_pool(void);

#define DoNullInd(x) ((x) ? (x) -> data : "")

extern void RCLEAR(struct inst *oper, char *file, int line);

#define CLEAR(oper) RCLEAR(oper, __FILE__, __LINE__)
extern void push(struct inst *stack, int *top, int type, voidptr res);
extern int valid_object(struct inst *oper);

extern struct localvars *localvars_get(struct frame *fr, dbref prog);
extern void localvar_dupall(struct frame *fr, struct frame *oldfr);
extern struct inst *scopedvar_get(struct frame *fr, int level, int varnum);
extern const char* scopedvar_getname(struct frame *fr, int level, int varnum);
extern const char* scopedvar_getname_byinst(struct inst *pc, int varnum);
extern int scopedvar_getnum(struct frame *fr, int level, const char* varname);
extern void scopedvar_dupall(struct frame *fr, struct frame *oldfr);
extern int false_inst(struct inst *p);

extern void copyinst(struct inst *from, struct inst *to);

#define abort_loop(S, C1, C2) \
{ \
	do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
	if (fr && fr->trys.top) \
		break; \
	else \
		return 0; \
}

#define abort_loop_hard(S, C1, C2) \
{ \
	int tmp = 0; \
	if (fr) { tmp = fr->trys.top; fr->trys.top = 0; } \
	do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
	if (fr) fr->trys.top = tmp; \
	return 0; \
}

extern void do_abort_loop(dbref player, dbref program, const char *msg,
						  struct frame *fr, struct inst *pc,

						  int atop, int stop, struct inst *clinst1, struct inst *clinst2);

extern void interp_err(dbref player, dbref program, struct inst *pc,
					   struct inst *arg, int atop, dbref origprog,

					   const char *msg1, const char *msg2);

extern void push(struct inst *stack, int *top, int type, voidptr res);

extern int valid_player(struct inst *oper);

extern int valid_object(struct inst *oper);

extern int is_home(struct inst *oper);

extern int permissions(dbref player, dbref thing);

extern int arith_type(struct inst *op1, struct inst *op2);

#define POP() (arg + --(*top))

#define abort_interp(C) \
{ \
  do_abort_interp(player, (C), pc, arg, *top, fr, oper1, oper2, oper3, oper4, \
                  nargs, program, __FILE__, __LINE__); \
  return; \
}

#define CHECKOP_READONLY(N) \
{ \
    nargs = (N); \
    if (*top < nargs) { \
		nargs = 0; \
        abort_interp("Stack underflow."); \
	} \
}

#define CHECKOP(N) \
{ \
	CHECKOP_READONLY(N); \
    if (fr->trys.top && *top - fr->trys.st->depth < nargs)  { \
		nargs = 0; \
        abort_interp("Stack protection fault."); \
	} \
}

extern void do_abort_interp(dbref player, const char *msg, struct inst *pc,
							struct inst *arg, int atop, struct frame *fr,
							struct inst *oper1, struct inst *oper2, struct inst *oper3,
							struct inst *oper4, int nargs, dbref program, char *file,

							int line);


#define Min(x,y) ((x < y) ? x : y)
#define ProgMLevel(x) (find_mlev(x, fr, fr->caller.top))

#define ProgUID find_uid(player, fr, fr->caller.top, program)
extern dbref find_uid(dbref player, struct frame *fr, int st, dbref program);

#define CHECKREMOTE(x) if ((mlev < 2) && (getloc(x) != player) &&  \
                           (getloc(x) != getloc(player)) && \
                           ((x) != getloc(player)) && ((x) != player) \
			   && !controls(ProgUID, x)) \
                 abort_interp("Mucker Level 2 required to get remote info.");

#define PushObject(x)   push(arg, top, PROG_OBJECT, MIPSCAST &x)
#define PushInt(x)      push(arg, top, PROG_INTEGER, MIPSCAST &x)
#define PushFloat(x)    push(arg, top, PROG_FLOAT, MIPSCAST &x)
#define PushLock(x)     push(arg, top, PROG_LOCK, MIPSCAST copy_bool(x))
#define PushTrueLock(x) push(arg, top, PROG_LOCK, MIPSCAST TRUE_BOOLEXP)

#define PushMark()      push(arg, top, PROG_MARK, MIPSCAST 0)
#define PushStrRaw(x)   push(arg, top, PROG_STRING, MIPSCAST x)
#define PushString(x)   PushStrRaw(alloc_prog_string(x))
#define PushNullStr     PushStrRaw(0)

#define PushArrayRaw(x) push(arg, top, PROG_ARRAY, MIPSCAST x)
#define PushNullArray   PushArrayRaw(0)
#define PushInst(x) copyinst(x, &arg[((*top)++)])

#define CHECKOFLOW(x) if((*top + (x - 1)) >= STACK_SIZE) \
			  abort_interp("Stack Overflow!");

#define PRIM_PROTOTYPE dbref player, dbref program, int mlev, \
                       struct inst *pc, struct inst *arg, int *top, \
                       struct frame *fr

#define SORTTYPE_CASEINSENS     0x1
#define SORTTYPE_DESCENDING     0x2

#define SORTTYPE_CASE_ASCEND    0
#define SORTTYPE_NOCASE_ASCEND  (SORTTYPE_CASEINSENS)
#define SORTTYPE_CASE_DESCEND   (SORTTYPE_DESCENDING)
#define SORTTYPE_NOCASE_DESCEND (SORTTYPE_CASEINSENS | SORTTYPE_DESCENDING)
#define SORTTYPE_SHUFFLE        4

extern int nargs;				/* DO NOT TOUCH THIS VARIABLE */

#include "p_array.h"
#include "p_connects.h"
#include "p_db.h"
#include "p_error.h"
#include "p_float.h"
#include "p_math.h"
#include "p_misc.h"
#include "p_props.h"
#include "p_stack.h"
#include "p_mcp.h"
#include "p_strings.h"
#include "p_regex.h"

#endif /* _INTERP_H */
