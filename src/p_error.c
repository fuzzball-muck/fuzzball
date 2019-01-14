#include "config.h"

#include "db.h"
#include "inst.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int result;
static char buf[BUFFER_LEN];

#define ERROR_NAME_MAX_LEN 15
#define ERROR_STRING_MAX_LEN 80
#define ERROR_NUM 5

struct err_type {
    int error_bit;
    char error_name[ERROR_NAME_MAX_LEN];
    char error_string[ERROR_STRING_MAX_LEN];
};

static union error_mask err_bits[ERROR_NUM];

static struct err_type err_defs[] = {
    {0, "DIV_ZERO", "Division by zero attempted."},
    {1, "NAN", "Result was not a number."},
    {2, "IMAGINARY", "Result was imaginary."},
    {3, "FBOUNDS", "Floating-point inputs were infinite or out of range."},
    {4, "IBOUNDS", "Calculation resulted in an integer overflow."},
    {5, "", ""}
};

static int err_init = 0;

static void
init_err_val(void)
{
    for (int i = 0; i < ERROR_NUM; i++)
	err_bits[i].is_flags = 0;
    err_bits[0].error_flags.div_zero = 1;
    err_bits[1].error_flags.nan = 1;
    err_bits[2].error_flags.imaginary = 1;
    err_bits[3].error_flags.f_bounds = 1;
    err_bits[4].error_flags.i_bounds = 1;
    err_init = 1;
}

void
prim_clear(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    fr->error.is_flags = 0;
}

void
prim_error_num(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    result = ERROR_NUM;
    PushInt(result);
}

void
prim_clear_error(PRIM_PROTOTYPE)
{
    int loop;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type. (1)");
    if (!err_init)
	init_err_val();
    if (oper1->type == PROG_INTEGER) {
	if ((oper1->data.number < 0) || (oper1->data.number >= ERROR_NUM)) {
	    result = 0;
	} else {
	    fr->error.is_flags = fr->error.is_flags & (~err_bits[oper1->data.number].is_flags);
	    result = 1;
	}
    } else {
	if (!oper1->data.string) {
	    result = 0;
	} else {
	    result = 0;
	    loop = 0;
	    while (loop < ERROR_NUM) {
		if (!strcasecmp(buf, err_defs[loop].error_name)) {
		    result = 1;
		    fr->error.is_flags = fr->error.is_flags & (~err_bits[loop].is_flags);
		    break;
		} else {
		    loop++;
		}
	    }
	}
    }
    CLEAR(oper1);
    PushInt(result);
}

void
prim_set_error(PRIM_PROTOTYPE)
{
    int loop;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type. (1)");
    if (!err_init)
	init_err_val();
    if (oper1->type == PROG_INTEGER) {
	if ((oper1->data.number < 0) || (oper1->data.number >= ERROR_NUM)) {
	    result = 0;
	} else {
	    fr->error.is_flags = fr->error.is_flags | err_bits[oper1->data.number].is_flags;
	    result = 1;
	}
    } else {
	if (!oper1->data.string) {
	    result = 0;
	} else {
	    result = 0;
	    loop = 0;
	    while (loop < ERROR_NUM) {
		if (!strcasecmp(buf, err_defs[loop].error_name)) {
		    result = 1;
		    fr->error.is_flags = fr->error.is_flags | err_bits[loop].is_flags;
		    break;
		} else {
		    loop++;
		}
	    }
	}
    }
    CLEAR(oper1);
    PushInt(result);
}

void
prim_is_set(PRIM_PROTOTYPE)
{
    int loop;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type. (1)");
    if (!err_init)
	init_err_val();
    if (oper1->type == PROG_INTEGER) {
	if ((oper1->data.number < 0) || (oper1->data.number >= ERROR_NUM)) {
	    result = 0;
	} else {
	    result = ((fr->error.is_flags & err_bits[oper1->data.number].is_flags) != 0);
	}
    } else {
	if (!oper1->data.string) {
	    result = 0;
	} else {
	    result = 0;
	    loop = 0;
	    while (loop < ERROR_NUM) {
		if (!strcasecmp(buf, err_defs[loop].error_name)) {
		    result = ((fr->error.is_flags & err_bits[loop].is_flags) != 0);
		    break;
		} else {
		    loop++;
		}
	    }
	}
    }
    CLEAR(oper1);
    PushInt(result);
}

void
prim_error_str(PRIM_PROTOTYPE)
{
    int loop;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING && oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type. (1)");
    if (!err_init)
	init_err_val();
    if (oper1->type == PROG_INTEGER) {
	if ((oper1->data.number < 0) || (oper1->data.number >= ERROR_NUM)) {
	    result = -1;
	} else {
	    result = oper1->data.number;
	}
    } else {
	if (!oper1->data.string) {
	    result = -1;
	} else {
	    result = -1;
	    loop = 0;
	    while (loop < ERROR_NUM) {
		if (!strcasecmp(buf, err_defs[loop].error_name)) {
		    result = loop;
		    break;
		} else {
		    loop++;
		}
	    }
	}
    }
    CLEAR(oper1);
    if (result == -1) {
	PushNullStr;
    } else {
	PushString(err_defs[result].error_string);
    }
}

void
prim_error_name(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Invalid argument type. (1)");
    if (!err_init)
	init_err_val();
    if ((oper1->data.number < 0) || (oper1->data.number >= ERROR_NUM)) {
	result = -1;
    } else {
	result = oper1->data.number;
    }
    CLEAR(oper1);
    if (result == -1) {
	PushNullStr;
    } else {
	PushString(err_defs[result].error_name);
    }
}

void
prim_error_bit(PRIM_PROTOTYPE)
{
    int loop;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type. (1)");
    if (!err_init)
	init_err_val();
    if (!oper1->data.string) {
	result = -1;
    } else {
	result = -1;
	loop = 0;
	while (loop < ERROR_NUM) {
	    if (!strcasecmp(buf, err_defs[loop].error_name)) {
		result = loop;
		break;
	    } else {
		loop++;
	    }
	}
    }
    CLEAR(oper1);
    PushInt(result);
}

void
prim_is_error(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    result = ((fr->error.is_flags) != 0);
    PushInt(result);
}
