#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "inst.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2, temp3;
static int result, tmp;
static dbref ref;
static char buf[BUFFER_LEN];

void
prim_pop(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    CLEAR(oper1);
}

void
prim_dup(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    CHECKOFLOW(1);
    copyinst(&arg[*top - 1], &arg[*top]);
    (*top)++;
}

void
prim_shallow_copy(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    CHECKOFLOW(1);
    if (arg[*top - 1].type != PROG_ARRAY) {
	copyinst(&arg[*top - 1], &arg[*top]);
	(*top)++;
    } else {
	stk_array *nu;
	stk_array *arr = arg[*top - 1].data.array;
	if (!arr) {
	    nu = new_array_packed(0, fr->pinning);
	} else {
	    nu = array_decouple(arr);
	}
	PushArrayRaw(nu);
    }
}

void
prim_deep_copy(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    CHECKOFLOW(1);
    deep_copyinst(&arg[*top - 1], &arg[*top], fr->pinning);
    (*top)++;
}

void
prim_pdup(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    if (!false_inst(&arg[*top - 1])) {
	CHECKOFLOW(1);
	copyinst(&arg[*top - 1], &arg[*top]);
	(*top)++;
    }
}

void
prim_popn(PRIM_PROTOTYPE)
{
    int count;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Operand not an integer.");
    count = oper1->data.number;
    if (count < 0)
	abort_interp("Operand is negative.");
    EXPECT_POP_STACK(count);
    for (int i = count; i > 0; i--) {
	oper2 = POP();
	CLEAR(oper2);
    }
    CLEAR(oper1);
}

void
prim_dupn(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Operand is not an integer.");
    result = oper1->data.number;
    if (result < 0)
	abort_interp("Operand is negative.");
    EXPECT_READ_STACK(result);
    CLEAR(oper1);
    nargs = 0;
    CHECKOFLOW(result);
    for (int i = result; i > 0; i--) {
	copyinst(&arg[*top - result], &arg[*top]);
	(*top)++;
    }
}

void
prim_ldup(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(1);
    nargs = 0;

    if (arg[*top - 1].type != PROG_INTEGER)
	abort_interp("Operand is not an integer.");

    result = arg[*top - 1].data.number;

    if (result >= INT_MAX)
        abort_interp("Integer overflow.");

    if (result < 0)
	abort_interp("Operand is negative.");

    result++;
    EXPECT_READ_STACK(result);
    CHECKOFLOW(result);

    for (int i = result; i > 0; i--) {
	copyinst(&arg[*top - result], &arg[*top]);
	(*top)++;
    }
}

void
prim_nip(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    CLEAR(oper2);
    arg[(*top)++] = *oper1;
}

void
prim_tuck(PRIM_PROTOTYPE)
{
    CHECKOFLOW(1);
    CHECKOP(2);
    oper1 = POP();
    temp2 = *(oper2 = POP());
    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp2;
    copyinst(&arg[*top - 2], &arg[*top]);
    (*top)++;
}

void
prim_at(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    temp1 = *(oper1 = POP());
    if ((temp1.type != PROG_VAR) && (temp1.type != PROG_LVAR) && (temp1.type != PROG_SVAR))
	abort_interp("Non-variable argument.");
    if (temp1.data.number >= MAX_VAR || temp1.data.number < 0)
	abort_interp("Variable number out of range.");
    if (temp1.type == PROG_LVAR) {
	/* LOCALVAR */
	struct localvars *tmp = localvars_get(fr, program);
	copyinst(&(tmp->lvars[temp1.data.number]), &arg[(*top)++]);
    } else if (temp1.type == PROG_VAR) {
	/* GLOBALVAR */
	copyinst(&(fr->variables[temp1.data.number]), &arg[(*top)++]);
    } else {
	/* SCOPEDVAR */
	struct inst *tmp;

	tmp = scopedvar_get(fr, 0, temp1.data.number);
	if (!tmp)
	    abort_interp("Scoped variable number out of range.");
	copyinst(tmp, &arg[(*top)++]);
    }
    CLEAR(&temp1);
}

void
prim_bang(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if ((oper1->type != PROG_VAR) && (oper1->type != PROG_LVAR) && (oper1->type != PROG_SVAR))
	abort_interp("Non-variable argument (2)");
    if (oper1->data.number >= MAX_VAR || oper1->data.number < 0)
	abort_interp("Variable number out of range. (2)");
    if (oper1->type == PROG_LVAR) {
	/* LOCALVAR */
	struct localvars *tmp = localvars_get(fr, program);
	CLEAR(&(tmp->lvars[oper1->data.number]));
	copyinst(oper2, &(tmp->lvars[oper1->data.number]));
    } else if (oper1->type == PROG_VAR) {
	/* GLOBALVAR */
	CLEAR(&(fr->variables[oper1->data.number]));
	copyinst(oper2, &(fr->variables[oper1->data.number]));
    } else {
	/* SCOPEDVAR */
	struct inst *tmp;

	tmp = scopedvar_get(fr, 0, oper1->data.number);
	if (!tmp)
	    abort_interp("Scoped variable number out of range.");
	CLEAR(tmp);
	copyinst(oper2, tmp);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_var(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument.");
    result = oper1->data.number;
    CLEAR(oper1);
    push(arg, top, PROG_VAR, &result);
}

void
prim_localvar(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument.");
    result = oper1->data.number;
    CLEAR(oper1);
    push(arg, top, PROG_LVAR, &result);
}

void
prim_swap(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    temp2 = *(oper2 = POP());
    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp2;
    /* don't clean! */
}

void
prim_over(PRIM_PROTOTYPE)
{
    EXPECT_READ_STACK(2);
    CHECKOFLOW(1);
    copyinst(&arg[*top - 2], &arg[*top]);
    (*top)++;
}

void
prim_pick(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    temp1 = *(oper1 = POP());
    if (temp1.type != PROG_INTEGER || temp1.data.number <= 0)
	abort_interp("Operand not a positive integer.");
    CHECKOP_READONLY(temp1.data.number);
    copyinst(&arg[*top - temp1.data.number], &arg[*top]);
    (*top)++;
}

void
prim_put(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_INTEGER || oper1->data.number <= 0)
	abort_interp("Operand not a positive integer.");
    tmp = oper1->data.number;
    EXPECT_WRITE_STACK(tmp);
    CLEAR(&arg[*top - tmp]);
    copyinst(oper2, &arg[*top - tmp]);
    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_rot(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    temp3 = *(oper3 = POP());
    arg[(*top)++] = *oper2;
    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp3;
}

void
prim_rrot(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    temp2 = *(oper2 = POP());
    temp3 = *(oper3 = POP());
    arg[(*top)++] = *oper1;
    arg[(*top)++] = temp3;
    arg[(*top)++] = temp2;
}

void
prim_rotate(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type.");
    int n = oper1->data.number;	/* Depth on stack */
    if (n == INT_MIN)
        abort_interp("Stack underflow.");
    EXPECT_WRITE_STACK(abs(n));
    if (n > 0) {
	temp2 = arg[*top - n];
	for (tmp = n; tmp > 0; tmp--)
	    arg[*top - tmp] = arg[*top - tmp + 1];
	arg[*top - 1] = temp2;
    } else if (n < 0) {
	temp2 = arg[*top - 1];
	for (tmp = -1; tmp > n; tmp--)
	    arg[*top + tmp] = arg[*top + tmp - 1];
	arg[*top + tmp] = temp2;
    }
    CLEAR(oper1);
}

void
prim_dbtop(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    ref = (dbref) db_top;
    CHECKOFLOW(1);
    PushObject(ref);
}

void
prim_depth(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    result = fr->trys.top ? *top - fr->trys.st->depth : *top;
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_fulldepth(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    result = *top;
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_version(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    PushString(VERSION);
}

void
prim_prog(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    ref = (dbref) program;
    CHECKOFLOW(1);
    PushObject(ref);
}

void
prim_trig(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    ref = (dbref) fr->trig;
    CHECKOFLOW(1);
    PushObject(ref);
}

void
prim_caller(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    ref = (dbref) fr->caller.st[fr->caller.top - 1];
    CHECKOFLOW(1);
    PushObject(ref);
}

void
prim_intp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_INTEGER);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_floatp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_FLOAT);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_arrayp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_ARRAY);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_dictionaryp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_ARRAY && oper1->data.array &&
	      oper1->data.array->type == ARRAY_DICTIONARY);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_stringp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_STRING);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_cmd(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    PushStrRaw(fr->cmd);
    if (fr->cmd)
	fr->cmd->links++;
}

void
prim_dbrefp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_OBJECT);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_addressp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_ADD);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_lockp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = (oper1->type == PROG_LOCK);
    CLEAR(oper1);
    PushInt(result);
}

#define ABORT_CHECKARGS(msg) { if (*top == stackpos+1) snprintf(zbuf, sizeof(zbuf), "%s (top)", msg); else snprintf(zbuf, sizeof(zbuf), "%s (top-%d)", msg, ((*top)-stackpos-1));  abort_interp(zbuf); }

#define MaxComplexity 18

void
prim_checkargs(PRIM_PROTOTYPE)
{
    int currpos, stackpos;
    int rngstktop = 0;
    enum {
	itsarange, itsarepeat
    } rngstktyp[MaxComplexity];
    int rngstkpos[MaxComplexity];
    int rngstkcnt[MaxComplexity];
    char zbuf[BUFFER_LEN];

    CHECKOP(1);
    oper1 = POP();		/* string argument */
    if (oper1->type != PROG_STRING)
	abort_interp("Non string argument.");
    if (!oper1->data.string) {
	/* if null string, then no args expected. */
	CLEAR(oper1);
	return;
    }
    strcpyn(buf, sizeof(buf), oper1->data.string->data);	/* copy into local buffer */
    currpos = strlen(buf) - 1;
    stackpos = *top - 1;

    while (currpos >= 0) {
	if (isdigit(buf[currpos])) {
	    if (rngstktop >= MaxComplexity)
		abort_interp("Argument expression ridiculously complex.");
	    tmp = 1;
	    result = 0;
	    while ((currpos >= 0) && isdigit(buf[currpos])) {
		result = result + (tmp * (buf[currpos] - '0'));
		tmp = tmp * 10;
		currpos--;
	    }
	    if (result == 0)
		abort_interp("Bad multiplier '0' in argument expression.");
	    if (result >= STACK_SIZE)
		abort_interp("Multiplier too large in argument expression.");
	    rngstktyp[rngstktop] = itsarepeat;
	    rngstkcnt[rngstktop] = result;
	    rngstkpos[rngstktop] = currpos;
	    rngstktop++;
	} else if (buf[currpos] == '}') {
	    if (rngstktop >= MaxComplexity)
		abort_interp("Argument expression ridiculously complex.");
	    if (stackpos < 0)
		ABORT_CHECKARGS("Stack underflow.");
	    if (arg[stackpos].type != PROG_INTEGER)
		ABORT_CHECKARGS("Expected an integer range counter.");
	    result = arg[stackpos].data.number;
	    if (result < 0)
		ABORT_CHECKARGS("Range counter should be non-negative.");
	    rngstkpos[rngstktop] = currpos - 1;
	    rngstkcnt[rngstktop] = result;
	    rngstktyp[rngstktop] = itsarange;
	    rngstktop++;
	    currpos--;
	    if (result == 0) {
		while ((currpos > 0) && (buf[currpos] != '{'))
		    currpos--;
	    }
	    stackpos--;
	} else if (buf[currpos] == '{') {
	    if (rngstktop <= 0)
		abort_interp("Mismatched { in argument expression");
	    if (rngstktyp[rngstktop - 1] != itsarange)
		abort_interp("Misformed argument expression.");
	    if (--rngstkcnt[rngstktop - 1] > 0) {
		currpos = rngstkpos[rngstktop - 1];
	    } else {
		rngstktop--;
		currpos--;
		if (rngstktop && (rngstktyp[rngstktop - 1] == itsarepeat)) {
		    if (--rngstkcnt[rngstktop - 1] > 0) {
			currpos = rngstkpos[rngstktop - 1];
		    } else {
			rngstktop--;
		    }
		}
	    }
	} else {
	    switch (buf[currpos]) {
	    case 'i':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_INTEGER)
		    ABORT_CHECKARGS("Expected an integer.");
		break;
	    case 'n':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_FLOAT)
		    ABORT_CHECKARGS("Expected a float.");
		break;
	    case 's':
	    case 'S':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_STRING)
		    ABORT_CHECKARGS("Expected a string.");
		if (buf[currpos] == 'S' && !arg[stackpos].data.string)
		    ABORT_CHECKARGS("Expected a non-null string.");
		break;
	    case 'd':
	    case 'p':
	    case 'r':
	    case 't':
	    case 'e':
	    case 'f':
	    case 'D':
	    case 'P':
	    case 'R':
	    case 'T':
	    case 'E':
	    case 'F':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_OBJECT)
		    ABORT_CHECKARGS("Expected a dbref.");
		ref = arg[stackpos].data.objref;
		if (ref >= db_top || ref < HOME)
		    ABORT_CHECKARGS("Invalid dbref.");
		switch (buf[currpos]) {
		case 'D':
		    if ((ref < 0) && (ref != HOME))
			ABORT_CHECKARGS("Invalid dbref.");
		    if (Typeof(ref) == TYPE_GARBAGE)
			ABORT_CHECKARGS("Invalid dbref.");
		case 'd':
		    if (ref < HOME)
			ABORT_CHECKARGS("Invalid dbref.");
		    break;

		case 'P':
		    if (ref < 0)
			ABORT_CHECKARGS("Expected player dbref.");
		case 'p':
		    if ((ref >= 0) && (Typeof(ref) != TYPE_PLAYER))
			ABORT_CHECKARGS("Expected player dbref.");
		    if (ref == HOME)
			ABORT_CHECKARGS("Expected player dbref.");
		    break;

		case 'R':
		    if ((ref < 0) && (ref != HOME))
			ABORT_CHECKARGS("Expected room dbref.");
		case 'r':
		    if ((ref >= 0) && (Typeof(ref) != TYPE_ROOM))
			ABORT_CHECKARGS("Expected room dbref.");
		    break;

		case 'T':
		    if (ref < 0)
			ABORT_CHECKARGS("Expected thing dbref.");
		case 't':
		    if ((ref >= 0) && (Typeof(ref) != TYPE_THING))
			ABORT_CHECKARGS("Expected thing dbref.");
		    if (ref == HOME)
			ABORT_CHECKARGS("Expected thing dbref.");
		    break;

		case 'E':
		    if (ref < 0)
			ABORT_CHECKARGS("Expected exit dbref.");
		case 'e':
		    if ((ref >= 0) && (Typeof(ref) != TYPE_EXIT))
			ABORT_CHECKARGS("Expected exit dbref.");
		    if (ref == HOME)
			ABORT_CHECKARGS("Expected exit dbref.");
		    break;

		case 'F':
		    if (ref < 0)
			ABORT_CHECKARGS("Expected program dbref.");
		case 'f':
		    if ((ref >= 0) && (Typeof(ref) != TYPE_PROGRAM))
			ABORT_CHECKARGS("Expected program dbref.");
		    if (ref == HOME)
			ABORT_CHECKARGS("Expected program dbref.");
		    break;
		}
		break;
	    case '?':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		break;
	    case 'l':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_LOCK)
		    ABORT_CHECKARGS("Expected a lock boolean expression.");
		break;
	    case 'v':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_VAR &&
		    arg[stackpos].type != PROG_LVAR && arg[stackpos].type != PROG_SVAR) {
		    ABORT_CHECKARGS("Expected a variable.");
		}
		break;
	    case 'a':
		if (stackpos < 0)
		    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_ADD)
		    ABORT_CHECKARGS("Expected a function address.");
		break;
	    case 'x':
                if (stackpos < 0)
                    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_ARRAY ||
			!arg[stackpos].data.array || 
			arg[stackpos].data.array->type != ARRAY_DICTIONARY)
		    ABORT_CHECKARGS("Expected a dictionary array.");
		break;
	    case 'y':
	    case 'Y':
                if (stackpos < 0)
                    ABORT_CHECKARGS("Stack underflow.");
		if (arg[stackpos].type != PROG_ARRAY)
		    ABORT_CHECKARGS("Expected an array.");
		if (buf[currpos] == 'Y' && arg[stackpos].data.array &&
			arg[stackpos].data.array->type == ARRAY_DICTIONARY)
		    ABORT_CHECKARGS("Expected a non-dictionary array.");

		break;	
	    case ' ':
		/* this is meaningless space.  Ignore it. */
		stackpos++;
		break;
	    default:
		abort_interp("Unkown argument type in expression.");
	    }

	    currpos--;		/* decrement string index */
	    stackpos--;		/* move on to next stack item down */

	    /* are we expecting a repeat of the last argument or range? */
	    if ((rngstktop > 0) && (rngstktyp[rngstktop - 1] == itsarepeat)) {
		/* is the repeat is done yet? */
		if (--rngstkcnt[rngstktop - 1] > 0) {
		    /* no, repeat last argument or range */
		    currpos = rngstkpos[rngstktop - 1];
		} else {
		    /* yes, we're done with this repeat */
		    rngstktop--;
		}
	    }
	}
    }				/* while loop */

    if (rngstktop > 0)
	abort_interp("Badly formed argument expression.");
    /* Oops. still haven't finished a range or repeat expression. */

    CLEAR(oper1);		/* clear link to shared string */
}

#undef ABORT_CHECKARGS

void
prim_mode(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    result = fr->multitask;
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_mark(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    PushMark();
}

void
prim_findmark(PRIM_PROTOTYPE)
{
    int depth, height, count;

    CHECKOP(0);
    depth = 1;
    height = *top - 1;
    while (height >= 0 && arg[height].type != PROG_MARK) {
	height--;
	depth++;
    }
    count = depth - 1;
    if (height < 0)
	abort_interp("No matching mark on stack!");
    if (depth > 1) {
	temp2 = arg[*top - depth];
	for (; depth > 1; depth--)
	    arg[*top - depth] = arg[*top - depth + 1];
	arg[*top - 1] = temp2;
    }
    EXPECT_POP_STACK(1);
    oper1 = POP();
    CLEAR(oper1);
    PushInt(count);
}

void
prim_preempt(PRIM_PROTOTYPE)
{
    fr->multitask = PREEMPT;
}

void
prim_foreground(PRIM_PROTOTYPE)
{
    if (fr->been_background)
	abort_interp("Cannot FOREGROUND a BACKGROUNDed program.");

    fr->multitask = FOREGROUND;

}

void
prim_background(PRIM_PROTOTYPE)
{
    fr->multitask = BACKGROUND;
    fr->been_background = 1;
    fr->writeonly = 1;
}

void
prim_setmode(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type.");
    result = oper1->data.number;
    switch (result) {
    case BACKGROUND:
	fr->been_background = 1;
	fr->writeonly = 1;
	break;
    case FOREGROUND:
	if (fr->been_background)
	    abort_interp("Cannot FOREGROUND a BACKGROUNDed program.");
	break;
    case PREEMPT:
	break;
    default:
	abort_interp("Invalid mode.");
    }
    fr->multitask = result;
    CLEAR(oper1);
}

void
prim_interp(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper3 = NULL;	/* prevents re-entrancy issues! */

    struct inst *rv = NULL;
    char buf[BUFFER_LEN];
    struct frame *tmpfr;

    CHECKOP(3);
    oper3 = POP();		/* string -- top stack argument */
    oper2 = POP();		/* dbref  --  trigger */
    oper1 = POP();		/* dbref  --  Program to run */

    if (!valid_object(oper1) || Typeof(oper1->data.objref) != TYPE_PROGRAM)
	abort_interp("Bad program reference. (1)");
    if (!valid_object(oper2))
	abort_interp("Bad object. (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Expected a string. (3)");
    if ((mlev < 3) && !permissions(ProgUID, oper2->data.objref))
	abort_interp("Permission denied.");
    if (fr->level > 8)
	abort_interp("Interp call loops not allowed.");
    CHECKREMOTE(oper2->data.objref);

    strcpyn(buf, sizeof(buf), match_args);
    strcpyn(match_args, sizeof(match_args), DoNullInd(oper3->data.string));
    tmpfr = interp(fr->descr, player, LOCATION(player), oper1->data.objref,
		   oper2->data.objref, PREEMPT, STD_HARDUID, 0);
    if (tmpfr) {
	rv = interp_loop(player, oper1->data.objref, tmpfr, 1);
    }
    strcpyn(match_args, sizeof(match_args), buf);

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    if (rv) {
	if (rv->type < PROG_STRING) {
	    push(arg, top, rv->type, &rv->data.number);
	} else {
	    push(arg, top, rv->type, rv->data.string);
	}
    } else {
	PushNullStr;
    }

}

void
prim_for(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper3 = POP();		/* step */
    oper2 = POP();		/* end */
    oper1 = POP();		/* start */

    if (oper1->type != PROG_INTEGER)
	abort_interp("Starting count expected. (1)");
    if (oper2->type != PROG_INTEGER)
	abort_interp("Ending count expected. (2)");
    if (oper3->type != PROG_INTEGER)
	abort_interp("Step count expected. (3)");
    if (fr->fors.top >= STACK_SIZE)
	abort_interp("Too many nested FOR loops.");

    fr->fors.top++;
    fr->fors.st = push_for(fr->fors.st);
    copyinst(oper1, &fr->fors.st->cur);
    copyinst(oper2, &fr->fors.st->end);
    fr->fors.st->step = oper3->data.number;
    fr->fors.st->didfirst = 0;

    if (fr->trys.st)
	fr->trys.st->for_count++;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_foreach(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
	abort_interp("Array argument expected. (1)");
    if (fr->fors.top >= STACK_SIZE)
	abort_interp("Too many nested FOR loops.");

    fr->fors.top++;
    fr->fors.st = push_for(fr->fors.st);

    fr->fors.st->cur.type = PROG_INTEGER;
    fr->fors.st->cur.line = 0;
    fr->fors.st->cur.data.number = 0;

    if (fr->trys.st)
	fr->trys.st->for_count++;

    copyinst(oper1, &fr->fors.st->end);
    fr->fors.st->step = 0;
    fr->fors.st->didfirst = 0;

    CLEAR(oper1);
}

void
prim_foriter(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    if (!fr->fors.st)
	abort_interp("Internal error; FOR stack underflow.");

    if (fr->fors.st->end.type == PROG_ARRAY) {
	stk_array *arr = fr->fors.st->end.data.array;

	if (fr->fors.st->didfirst) {
	    result = array_next(arr, &fr->fors.st->cur);
	} else {
	    result = array_first(arr, &fr->fors.st->cur);
	    fr->fors.st->didfirst = 1;
	}
	if (result) {
	    array_data *val = array_getitem(arr, &fr->fors.st->cur);

	    if (val) {
		CHECKOFLOW(2);
		PushInst(&fr->fors.st->cur);	/* push key onto stack */
		PushInst(val);	/* push value onto stack */
		tmp = 1;	/* tell following IF to NOT branch out of loop */
	    } else {
		tmp = 0;	/* tell following IF to branch out of loop */
	    }
	} else {
	    fr->fors.st->cur.type = PROG_INTEGER;
	    fr->fors.st->cur.line = 0;
	    fr->fors.st->cur.data.number = 0;
	    tmp = 0;		/* tell following IF to branch out of loop */
	}
    } else {
	int cur = fr->fors.st->cur.data.number;
	int end = fr->fors.st->end.data.number;

	tmp = fr->fors.st->step > 0;
	if (tmp)
	    tmp = !(cur > end);
	else
	    tmp = !(cur < end);
	if (tmp) {
	    CHECKOFLOW(1);
	    result = cur;
	    fr->fors.st->cur.data.number += fr->fors.st->step;
	    PushInt(result);
	}
    }
    CHECKOFLOW(1);
    PushInt(tmp);
}

void
prim_forpop(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    if (!(fr->fors.top))
	abort_interp("Internal error; FOR stack underflow.");

    CLEAR(&fr->fors.st->cur);
    CLEAR(&fr->fors.st->end);

    if (fr->trys.st)
	fr->trys.st->for_count--;
    fr->fors.top--;
    fr->fors.st = pop_for(fr->fors.st);
}

void
prim_trypop(PRIM_PROTOTYPE)
{
    CHECKOP(0);

    if (!(fr->trys.top))
	abort_interp("Internal error; TRY stack underflow.");

    fr->trys.top--;
    fr->trys.st = pop_try(fr->trys.st);
}

void
prim_reverse(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type.");
    tmp = oper1->data.number;	/* Depth on stack */
    if (tmp < 0)
	abort_interp("Argument must be positive.");
    EXPECT_WRITE_STACK(tmp);
    if (tmp > 0) {
	for (int i = 0; i < (tmp / 2); i++) {
	    temp2 = arg[*top - (tmp - i)];
	    arg[*top - (tmp - i)] = arg[*top - (i + 1)];
	    arg[*top - (i + 1)] = temp2;
	}
    }
    CLEAR(oper1);
}

void
prim_lreverse(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type.");
    tmp = oper1->data.number;	/* Depth on stack */
    if (tmp < 0)
	abort_interp("Argument must be positive.");
    EXPECT_WRITE_STACK(tmp);
    if (tmp > 0) {
	for (int i = 0; i < (tmp / 2); i++) {
	    temp2 = arg[*top - (tmp - i)];
	    arg[*top - (tmp - i)] = arg[*top - (i + 1)];
	    arg[*top - (i + 1)] = temp2;
	}
    }
    CLEAR(oper1);
    PushInt(tmp);
}

void
prim_secure_sysvars(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CLEAR(&(fr->variables[0]));
    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = player;
    CLEAR(&(fr->variables[1]));
    fr->variables[1].type = PROG_OBJECT;
    fr->variables[1].data.objref = LOCATION(player);
    CLEAR(&(fr->variables[2]));
    fr->variables[2].type = PROG_OBJECT;
    fr->variables[2].data.objref = fr->trig;
    CLEAR(&(fr->variables[3]));
    fr->variables[3].type = PROG_STRING;
    fr->variables[3].data.string = fr->cmd;
    if (fr->cmd)
	fr->cmd->links++;
}
