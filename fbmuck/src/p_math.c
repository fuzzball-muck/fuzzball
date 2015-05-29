/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "fbstrings.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int tmp, result;
static double tl;
static double fresult, tf1, tf2;

#ifndef MAXINT
#define MAXINT ~(1<<((sizeof(int)*8)-1))
#endif
#ifndef MININT
#define MININT (1<<((sizeof(int)*8)-1))
#endif

int
arith_good(double test)
{
	return ((test <= (double) (MAXINT)) && (test >= (double) (MININT)));
}

int
arith_type(struct inst *op1, struct inst *op2)
{
	return ((op1->type == PROG_INTEGER && op2->type == PROG_INTEGER)	/* real stuff */
			||(op1->type == PROG_OBJECT && op2->type == PROG_INTEGER)	/* inc. dbref */
			||(op1->type == PROG_VAR && op2->type == PROG_INTEGER)	/* offset array */
			||(op1->type == PROG_LVAR && op2->type == PROG_INTEGER)
			|| (op1->type == PROG_FLOAT && op2->type == PROG_FLOAT)
			|| (op1->type == PROG_FLOAT && op2->type == PROG_INTEGER)
			|| (op1->type == PROG_INTEGER && op2->type == PROG_FLOAT));
}


int
nogood(double test)
{
	return (((test == INF) || (test == NINF)));
}

void
prim_add(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		tf1 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : oper1->data.number;
		tf2 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : oper2->data.number;
		if (!nogood(tf1) && !nogood(tf2)) {
			fresult = tf1 + tf2;
		} else {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		}
	} else {
		result = oper1->data.number + oper2->data.number;
		tl = (double) oper1->data.number + (double) oper2->data.number;
		if (!arith_good(tl))
			fr->error.error_flags.i_bounds = 1;
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void
prim_subtract(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : oper2->data.number;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : oper1->data.number;
		if (!nogood(tf1) && !nogood(tf2)) {
			fresult = tf1 - tf2;
		} else {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		}
	} else {
		result = oper2->data.number - oper1->data.number;
		tl = (double) oper2->data.number - (double) oper1->data.number;
		if (!arith_good(tl))
			fr->error.error_flags.i_bounds = 1;
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void
prim_multiply(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		tf1 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : oper1->data.number;
		tf2 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : oper2->data.number;
		if (!nogood(tf1) && !nogood(tf2)) {
			fresult = tf1 * tf2;
		} else {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		}
	} else {
		result = oper1->data.number * oper2->data.number;
		tl = (double) oper1->data.number * (double) oper2->data.number;
		if (!arith_good(tl))
			fr->error.error_flags.i_bounds = 1;
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void
prim_divide(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		if ((oper1->type == PROG_INTEGER && !oper1->data.number) ||
			(oper1->type == PROG_FLOAT && fabs(oper1->data.fnumber) < DBL_EPSILON)) {
			/* FIXME: This should be NaN.  */
			fresult = INF;
			fr->error.error_flags.div_zero = 1;
		} else {
			tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : oper2->data.number;
			tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : oper1->data.number;
			if (!nogood(tf1) && !nogood(tf2)) {
				fresult = tf1 / tf2;
			} else {
				fresult = 0.0;
				fr->error.error_flags.f_bounds = 1;
			}
		}
	} else {
		if (oper1->data.number) {
			result = oper2->data.number / oper1->data.number;
		} else {
			result = 0;
			fr->error.error_flags.div_zero = 1;
		}
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void
prim_mod(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if ((!arith_type(oper2, oper1)) || (oper1->type == PROG_FLOAT) ||
		(oper2->type == PROG_FLOAT)) abort_interp("Invalid argument type.");
	if (oper1->data.number)
		result = oper2->data.number % oper1->data.number;
	else
		result = 0;
	tmp = oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	push(arg, top, tmp, MIPSCAST & result);
}


void
prim_bitor(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	result = oper2->data.number | oper1->data.number;
	tmp = oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	push(arg, top, tmp, MIPSCAST & result);
}


void
prim_bitxor(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	result = oper2->data.number ^ oper1->data.number;
	tmp = oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	push(arg, top, tmp, MIPSCAST & result);
}


void
prim_bitand(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	result = oper2->data.number & oper1->data.number;
	tmp = oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	push(arg, top, tmp, MIPSCAST & result);
}


void
prim_bitshift(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!arith_type(oper2, oper1) || oper1->type != PROG_INTEGER)
		abort_interp("Invalid argument type.");
	if (oper1->data.number > 0)
		result = oper2->data.number << oper1->data.number;
	else if (oper1->data.number < 0)
		result = oper2->data.number >> (-(oper1->data.number));
	else
		result = oper2->data.number;
	tmp = oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	push(arg, top, tmp, MIPSCAST & result);
}


void
prim_and(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	result = !false_inst(oper1) && !false_inst(oper2);
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void
prim_or(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	result = !false_inst(oper1) || !false_inst(oper2);
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void
prim_xor(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (false_inst(oper1)) {
		result = !false_inst(oper2);
	} else {
		result = false_inst(oper2);
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void
prim_not(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	result = false_inst(oper1);
	CLEAR(oper1);
	PushInt(result);
}

int
comp_t(struct inst *op)
{
	return (op->type == PROG_INTEGER || op->type == PROG_FLOAT || op->type == PROG_OBJECT);
}

void
prim_lessthan(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref;
		result = tf1 < tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  < ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_greathan(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref;
		result = tf1 > tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  > ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_equal(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
#ifdef STRINGMATH
	if (oper1->type == PROG_STRING && oper2->type == PROG_STRING) {
		/* Code to handle RFE#859214 -- by winged */
		/* easier than prim_strcmp has to be      */
		/* Not strictly math, but a convenience   */
		if(oper1->data.string == oper2->data.string) {
			result = 0;
		} else {
			result = strcmp(DoNullInd(oper1->data.string),DoNullInd(oper2->data.string));
			result = result ? 0 : 1;
		}
	} else {
#endif /* STRINGMATH */
		if (!comp_t(oper1) || !comp_t(oper2))
			abort_interp("Invalid argument type.");
		if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
			tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
					(oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref;
			tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
					(oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref;
			result = tf1 == tf2;
		} else {
			result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
					  ==
					  ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
		}
#ifdef STRINGMATH
	}
#endif /* STRINGMATH */
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_lesseq(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref;
		result = tf1 <= tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  <=
				  ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_greateq(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref;
		result = tf1 >= tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  >=
				  ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void
prim_random(PRIM_PROTOTYPE)
{
	result = RANDOM();
	CHECKOFLOW(1);
	PushInt(result);
}

void
prim_srand(PRIM_PROTOTYPE)
{
	CHECKOP(0);
	CHECKOFLOW(1);
	if (!(fr->rndbuf)) {
		fr->rndbuf = init_seed(NULL);
	}
	result = (int) rnd(fr->rndbuf);
	PushInt(result);
}

void
prim_getseed(PRIM_PROTOTYPE)
{
	char buf[33];
	char buf2[17];
	int loop;

	CHECKOP(0);
	CHECKOFLOW(1);
	if (!(fr->rndbuf)) {
		PushNullStr;
	} else {
		memcpy(buf2, fr->rndbuf, 16);
		buf2[16] = '\0';
		for (loop = 0; loop < 16; loop++) {
			buf[loop * 2] = (buf2[loop] & 0x0F) + 65;
			buf[(loop * 2) + 1] = ((buf2[loop] & 0xF0) >> 4) + 65;
		}
		buf[32] = '\0';
		PushString(buf);
	}
}

void
prim_setseed(PRIM_PROTOTYPE)
{
	int slen, sloop;
	char holdbuf[33];
	char buf[17];

	CHECKOP(1);
	oper1 = POP();
	if (!(oper1->type == PROG_STRING))
		abort_interp("Invalid argument type.");
	if (fr->rndbuf) {
		delete_seed(fr->rndbuf);
		fr->rndbuf = NULL;
	}
	if (!oper1->data.string) {
		fr->rndbuf = init_seed(NULL);
		CLEAR(oper1);
		return;
	} else {
		slen = strlen(oper1->data.string->data);
		if (slen < 32) {
			for (sloop = 0; sloop < 32; sloop++)
				holdbuf[sloop] = oper1->data.string->data[sloop % slen];
		} else {
			memcpy(holdbuf, oper1->data.string->data, 32);
		}
		for (sloop = 0; sloop < 16; sloop++)
			buf[sloop] = ((holdbuf[sloop * 2] - 65) & 0x0F) |
					(((holdbuf[(sloop * 2) + 1] - 65) & 0x0F) << 4);
		buf[16] = '\0';
		fr->rndbuf = init_seed(buf);
	}
	CLEAR(oper1);
}

void
prim_int(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!(oper1->type == PROG_OBJECT || oper1->type == PROG_VAR ||
		  oper1->type == PROG_LVAR || oper1->type == PROG_FLOAT))
		abort_interp("Invalid argument type.");
	if ((!(oper1->type == PROG_FLOAT)) ||
		(oper1->type == PROG_FLOAT && arith_good((double) oper1->data.fnumber))) {
		result = (int) ((oper1->type == PROG_OBJECT) ?
						oper1->data.objref : (oper1->type == PROG_FLOAT) ?
						oper1->data.fnumber : oper1->data.number);
	} else {
		result = 0;
		fr->error.error_flags.i_bounds = 1;
	}
	CLEAR(oper1);
	PushInt(result);
}


void
prim_plusplus(PRIM_PROTOTYPE)
{
	struct inst *tmp;
	struct inst temp1;
	struct inst temp2;
	int vnum;

	CHECKOP(1)
	temp1 = *(oper1 = POP());

	if(oper1->type == PROG_VAR || oper1->type == PROG_SVAR || oper1->type == PROG_LVAR)
		if(oper1->data.number > MAX_VAR || oper1->data.number < 0)
			abort_interp("Variable number out of range.");

	switch(oper1->type) {
		case PROG_VAR:
			copyinst(&(fr->variables[temp1.data.number]), &temp2);
			break;
		case PROG_SVAR:
			tmp = scopedvar_get(fr, 0, temp1.data.number);
			copyinst(tmp, &temp2);
			break;
		case PROG_LVAR: {
			struct localvars *tmp2 = localvars_get(fr, program);
			copyinst(&(tmp2->lvars[temp1.data.number]), &temp2);
			break;
		}
		case PROG_INTEGER:
			oper1->data.number++;
			result = oper1->data.number;
			CLEAR(oper1);
			PushInt(result);
			return;
		case PROG_OBJECT:
			oper1->data.objref++;
			result = oper1->data.objref;
			CLEAR(oper1);
			PushObject(result);
			return;
		case PROG_FLOAT:
			oper1->data.fnumber++;
			fresult = oper1->data.fnumber;
			CLEAR(oper1);
			PushFloat(fresult);
			return;
		default:
			abort_interp("Invalid datatype.");
	}

	vnum = temp1.data.number;
	switch(temp2.type) {
		case PROG_INTEGER:
			temp2.data.number++;
			break;
		case PROG_OBJECT:
			temp2.data.objref++;
			break;
		case PROG_FLOAT:
			temp2.data.fnumber++;
			break;
		default:
			abort_interp("Invalid datatype in variable.");
	}

	switch(temp1.type) {
		case PROG_VAR: {
			CLEAR(&(fr->variables[vnum]));
			copyinst(&temp2, &(fr->variables[vnum]));
			break;
		}
		case PROG_SVAR: {
			struct inst *tmp2;
			tmp2 = scopedvar_get(fr, 0, vnum);
			if (!tmp2)
				abort_interp("Scoped variable number out of range.");
			CLEAR(tmp2);
			copyinst(&temp2, tmp2);
			break;
		}
		case PROG_LVAR: {
			struct localvars *tmp2 = localvars_get(fr, program);
			CLEAR(&(tmp2->lvars[vnum]));
			copyinst(&temp2, &(tmp2->lvars[vnum]));
			break;
		}
	}
	CLEAR(oper1);
}



void
prim_minusminus(PRIM_PROTOTYPE)
{
	struct inst *tmp;
	struct inst temp1;
	struct inst temp2;
	int vnum;

	CHECKOP(1)
	temp1 = *(oper1 = POP());

	if(oper1->type == PROG_VAR || oper1->type == PROG_SVAR || oper1->type == PROG_LVAR)
		if(oper1->data.number > MAX_VAR || oper1->data.number < 0)
			abort_interp("Variable number out of range.");

	switch(oper1->type) {
		case PROG_VAR:
			copyinst(&(fr->variables[temp1.data.number]), &temp2);
			break;
		case PROG_SVAR:
			tmp = scopedvar_get(fr, 0, temp1.data.number);
			copyinst(tmp, &temp2);
			break;
		case PROG_LVAR: {
			struct localvars *tmp2 = localvars_get(fr, program);
			copyinst(&(tmp2->lvars[temp1.data.number]), &temp2);
			break;
		}
		case PROG_INTEGER:
			oper1->data.number--;
			result = oper1->data.number;
			CLEAR(oper1);
			PushInt(result);
			return;
		case PROG_OBJECT:
			oper1->data.objref--;
			result = oper1->data.objref;
			CLEAR(oper1);
			PushObject(result);
			return;
		case PROG_FLOAT:
			oper1->data.fnumber--;
			fresult = oper1->data.fnumber;
			CLEAR(oper1);
			PushFloat(fresult);
			return;
		default:
			abort_interp("Invalid datatype.");
	}

	vnum = temp1.data.number;
	switch(temp2.type) {
		case PROG_INTEGER:
			temp2.data.number--;
			break;
		case PROG_OBJECT:
			temp2.data.objref--;
			break;
		case PROG_FLOAT:
			temp2.data.fnumber--;
			break;
		default:
			abort_interp("Invalid datatype in variable.");
	}

	switch(temp1.type) {
		case PROG_VAR: {
			CLEAR(&(fr->variables[vnum]));
			copyinst(&temp2, &(fr->variables[vnum]));
			break;
		}
		case PROG_SVAR: {
			struct inst *tmp2;
			tmp2 = scopedvar_get(fr, 0, vnum);
			if (!tmp2)
				abort_interp("Scoped variable number out of range.");
			CLEAR(tmp2);
			copyinst(&temp2, tmp2);
			break;
		}
		case PROG_LVAR: {
			struct localvars *tmp2 = localvars_get(fr, program);
			CLEAR(&(tmp2->lvars[vnum]));
			copyinst(&temp2, &(tmp2->lvars[vnum]));
			break;
		}
	}
	CLEAR(oper1);
}


void
prim_abs(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	result = oper1->data.number;
	if (result < 0)
		result = -result;
	CLEAR(oper1);
	PushInt(result);
}

void
prim_sign(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	if (oper1->data.number > 0) {
		result = 1;
	} else if (oper1->data.number < 0) {
		result = -1;
	} else {
		result = 0;
	}
	CLEAR(oper1);
	PushInt(result);
}
