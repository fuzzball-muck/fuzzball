#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "inst.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static double fresult;
static char buf[BUFFER_LEN];

void
prim_inf(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    fresult = INF;
    CHECKOFLOW(1);
    PushFloat(fresult);
}

void
prim_ceil(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = ceil(oper1->data.fnumber);
    } else {
	fresult = oper1->data.fnumber;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_floor(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = floor(oper1->data.fnumber);
    } else {
	fresult = oper1->data.fnumber;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_sqrt(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	if (oper1->data.fnumber < 0.0) {
	    fresult = 0.0;
	    fr->error.error_flags.imaginary = 1;
	} else {
	    fresult = sqrt(oper1->data.fnumber);
	}
    } else {
	fresult = oper1->data.fnumber;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_pi(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    fresult = M_PI;
    CHECKOFLOW(1);
    PushFloat(fresult);
}

void
prim_epsilon(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    fresult = DBL_EPSILON;
    CHECKOFLOW(1);
    PushFloat(fresult);
}

void
prim_round(PRIM_PROTOTYPE)
{
    double temp, tshift, tnum, fstore;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument. (2)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");

    if (!no_good(oper2->data.fnumber)) {
	temp = pow(10.0, oper1->data.number);
	tshift = temp * (oper2->data.fnumber);
	tnum = modf(tshift, &fstore);
	if (tnum >= 0.5) {
	    fstore = fstore + 1.0;
	} else {
	    if (tnum <= -0.5) {
		fstore = fstore - 1.0;
	    }
	}
	fstore = fstore / temp;
	fresult = fstore;
    } else {
	fresult = 0.0;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}

void
prim_sin(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = sin(oper1->data.fnumber);
    } else {
	/* FIXME:  This should be NaN. */
	fresult = 0.0;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_cos(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = cos(oper1->data.fnumber);
    } else {
	/* FIXME:  This should be NaN. */
	fresult = 0.0;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_tan(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = fmod((oper1->data.fnumber - M_PI_2), M_PI);
	if (fabs(fresult) > DBL_EPSILON && fabs(fresult - M_PI) > DBL_EPSILON) {
	    fresult = tan(oper1->data.fnumber);
	} else {
	    fresult = 0.0;
	    fr->error.error_flags.nan = 1;
	}
    } else {
	/* FIXME:  This should be NaN. */
	fresult = 0.0;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_asin(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if ((oper1->data.fnumber >= -1.0) && (oper1->data.fnumber <= 1.0)) {
	fresult = asin(oper1->data.fnumber);
    } else {
	fresult = 0.0;
	fr->error.error_flags.nan = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_acos(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if ((oper1->data.fnumber >= -1.0) && (oper1->data.fnumber <= 1.0)) {
	fresult = acos(oper1->data.fnumber);
    } else {
	fresult = 0.0;
	fr->error.error_flags.nan = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_atan(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = atan(oper1->data.fnumber);
    } else {
	fresult = M_PI_2;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_atan2(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    fresult = atan2(oper1->data.fnumber, oper2->data.fnumber);
    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}

void
prim_dist3d(PRIM_PROTOTYPE)
{
    double dist;
    double x, y, z;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Non-float argument. (3)");

    x = oper1->data.fnumber;
    y = oper2->data.fnumber;
    z = oper3->data.fnumber;
    dist = sqrt((x * x) + (y * y) + (z * z));

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushFloat(dist);
}

void
prim_diff3(PRIM_PROTOTYPE)
{
    double xout, yout, zout;
    double x, y, z;
    double x2, y2, z2;

    EXPECT_POP_STACK(6);
    // three things to CLEAR on abort_interp()
    nargs = 3;
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (4)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (5)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Non-float argument. (6)");

    x = oper1->data.fnumber;
    y = oper2->data.fnumber;
    z = oper3->data.fnumber;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Non-float argument. (3)");

    x2 = oper1->data.fnumber;
    y2 = oper2->data.fnumber;
    z2 = oper3->data.fnumber;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    xout = x - x2;
    yout = y - y2;
    zout = z - z2;

    PushFloat(xout);
    PushFloat(yout);
    PushFloat(zout);
}

void
prim_xyz_to_polar(PRIM_PROTOTYPE)
{
    double dist, theta, phi;
    double x, y, z;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Non-float argument. (3)");

    x = oper1->data.fnumber;
    y = oper2->data.fnumber;
    z = oper3->data.fnumber;

    if (no_good(x) || no_good(y) || no_good(z)) {
	dist = 0.0;
	theta = 0.0;
	phi = 0.0;
	fr->error.error_flags.nan = 1;
    } else {
	dist = sqrt((x * x) + (y * y) + (z * z));
	if (dist > 0.0) {
	    theta = atan2(y, x);
	    phi = acos(z / dist);
	} else {
	    theta = 0.0;
	    phi = 0.0;
	}
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushFloat(dist);
    PushFloat(theta);
    PushFloat(phi);
}


void
prim_polar_to_xyz(PRIM_PROTOTYPE)
{
    double x, y, z;
    double dist, theta, phi;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Non-float argument. (3)");

    dist = oper1->data.fnumber;
    theta = oper2->data.fnumber;
    phi = oper3->data.fnumber;

    if (no_good(dist) || no_good(theta) || no_good(phi)) {
	x = 0.0;
	y = 0.0;
	z = 0.0;
	fr->error.error_flags.nan = 1;
    } else {
	x = (dist * cos(theta) * sin(phi));
	y = (dist * sin(theta) * sin(phi));
	z = (dist * cos(phi));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushFloat(x);
    PushFloat(y);
    PushFloat(z);
}

void
prim_exp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = exp(oper1->data.fnumber);
    } else if (oper1->data.fnumber == INF) {
	fresult = INF;
	fr->error.error_flags.f_bounds = 1;
    } else {
	fresult = 0.0;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_log(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber) && oper1->data.fnumber > 0.0) {
	fresult = log(oper1->data.fnumber);
    } else if (oper1->data.fnumber > 0.0) {
	fresult = INF;
	fr->error.error_flags.f_bounds = 1;
    } else {
	fresult = 0.0;
	fr->error.error_flags.imaginary = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_log10(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber) && oper1->data.fnumber > 0.0) {
	fresult = log10(oper1->data.fnumber);
    } else if (oper1->data.fnumber > 0.0) {
	fresult = INF;
	fr->error.error_flags.f_bounds = 1;
    } else {
	fresult = 0.0;
	fr->error.error_flags.imaginary = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_fabs(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = fabs(oper1->data.fnumber);
    } else {
	fresult = INF;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_float(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument. (1)");
    fresult = oper1->data.number;
    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_pow(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    if (!no_good(oper1->data.fnumber) && !no_good(oper2->data.fnumber)) {
	if (fabs(oper2->data.fnumber) < DBL_EPSILON) {
	    fresult = 0.0;
	} else if (oper2->data.fnumber < 0.0 &&
		   oper1->data.fnumber != floor(oper1->data.fnumber)) {
	    fresult = 0.0;
	    fr->error.error_flags.imaginary = 1;
	} else {
	    fresult = pow(oper2->data.fnumber, oper1->data.fnumber);
	}
    } else {
	fresult = 0.0;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}

void
prim_frand(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);
    fresult = _int_f_rand();
    PushFloat(fresult);
}

void
prim_gaussian(PRIM_PROTOTYPE)
{
    /* We use these two statics to prevent lost work. */
    double srca = 0.0, srcb = 0.0;
    double resulta;
    double radius = 1.0;
    static double resultb;
    static char second_call = 0;

    CHECKOP(2);
    oper1 = POP();		/* Arg1 - mean */
    oper2 = POP();		/* Arg2 - std dev. */
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");

    /* This is a Box-Muller polar conversion.
     * Taken in part from code as demonstrated by Everett F. Carter, Jr.
     * This code is not copyrighted. */
    if (second_call) {
	/* We should have a correlated value to use from the
	 * previous call, still. */
	resulta = resultb;
	second_call = 0;
    } else {
	while (radius >= 1.0) {
	    srca = 2.0 * _int_f_rand() - 1.0;
	    srcb = 2.0 * _int_f_rand() - 1.0;
	    radius = srca * srca + srcb * srcb;
	}

	radius = sqrt((-2.0 * log(radius)) / radius);
	resulta = srca * radius;
	resultb = srcb * radius;
	second_call = 1;	/* Prime for next call in. */
    }

    fresult = oper1->data.fnumber + resulta * oper2->data.fnumber;
    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}

void
prim_fmod(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper2->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (2)");
    if (fabs(oper1->data.fnumber) < DBL_EPSILON) {
	fresult = INF;
	fr->error.error_flags.div_zero = 1;
    } else {
	fresult = fmod(oper2->data.fnumber, oper1->data.fnumber);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushFloat(fresult);
}

void
prim_modf(PRIM_PROTOTYPE)
{
    double tresult;
    double dresult;

    CHECKOP(1);
    oper1 = POP();
    CHECKOFLOW(2);
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    if (!no_good(oper1->data.fnumber)) {
	fresult = modf(oper1->data.fnumber, &dresult);
    } else {
	fresult = 0.0;
	dresult = oper1->data.fnumber;
	fr->error.error_flags.f_bounds = 1;
    }
    CLEAR(oper1);
    tresult = dresult;
    PushFloat(tresult);
    PushFloat(fresult);
}

void
prim_strtof(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument. (1)");

    if (!oper1->data.string || (!ifloat(oper1->data.string->data)
				&& !number(oper1->data.string->data))) {
	fresult = 0.0;
	fr->error.error_flags.nan = 1;
    } else {
	sscanf(oper1->data.string->data, "%lg", &fresult);
    }

    CLEAR(oper1);
    PushFloat(fresult);
}

void
prim_ftostr(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type == PROG_INTEGER) {
	oper1->type = PROG_FLOAT;
	oper1->data.fnumber = oper1->data.number;
    }
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    snprintf(buf, sizeof(buf), "%#.15g", oper1->data.fnumber);
    CLEAR(oper1);
    PushString(buf);
}

void
prim_ftostrc(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type == PROG_INTEGER) {
	oper1->type = PROG_FLOAT;
	oper1->data.fnumber = oper1->data.number;
    }
    if (oper1->type != PROG_FLOAT)
	abort_interp("Non-float argument. (1)");
    snprintf(buf, sizeof(buf), "%.15g", oper1->data.fnumber);
    if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'n')) {
	strcatn(buf, sizeof(buf), ".0");
    }

    CLEAR(oper1);
    PushString(buf);
}
