#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "color.h"
#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "sha1.h"
#include "timequeue.h"
#include "tune.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2, temp3;
static int tmp, result;
static dbref ref;
static char buf[BUFFER_LEN];
static char *pname;

/* FMTTOKEN defines the start of a variable formatting string insertion */
#define FMTTOKEN '%'

void
prim_fmtstring(PRIM_PROTOTYPE)
{
    int slen, scnt, tstop, tlen, tnum, i;
    int slrj, spad1, spad2, slen1, slen2, temp;
    char sstr[BUFFER_LEN], sfmt[255], hold[256], tbuf[BUFFER_LEN];
    char *ptr, *begptr;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Top argument must be a string.");
    if (!oper1->data.string) {
	CLEAR(oper1);
	PushNullStr;
	return;
    }
    /* We now have the non-null format string, parse it */
    result = 0;			/* End of current string, must be smaller than BUFFER_LEN */
    tmp = 0;			/* Number of props to search for/found */
    slen = strlen(oper1->data.string->data);
    scnt = 0;
    tstop = 0;
    strcpyn(sstr, sizeof(sstr), oper1->data.string->data);
    CLEAR(oper1);
    while ((scnt < slen) && (result < BUFFER_LEN)) {
	CHECKOP(0);
	if (sstr[scnt] == FMTTOKEN) {
	    if (sstr[scnt + 1] == FMTTOKEN) {
		buf[result++] = FMTTOKEN;
		scnt += 2;
	    } else {
		scnt++;
		if ((sstr[scnt] == '-') || (sstr[scnt] == '|')) {
		    if (sstr[scnt] == '-')
			slrj = 1;
		    else
			slrj = 2;
		    scnt++;
		} else {
		    slrj = 0;
		}
		if ((sstr[scnt] == '+') || (sstr[scnt] == ' ')) {
		    if (sstr[scnt] == '+')
			spad1 = 1;
		    else
			spad1 = 2;
		    scnt++;
		} else {
		    spad1 = 0;
		}
		if (sstr[scnt] == '0') {
		    scnt++;
		    spad2 = 1;
		} else {
		    spad2 = 0;
		}
		slen1 = atoi(&sstr[scnt]);
		if ((sstr[scnt] >= '0') && (sstr[scnt] <= '9')) {
		    while ((sstr[scnt] >= '0') && (sstr[scnt] <= '9'))
			scnt++;
		} else {
		    if (sstr[scnt] == '*') {
			scnt++;
			CHECKOP(1);
			oper1 = POP();
			if (oper1->type != PROG_INTEGER)
			    abort_interp("Format specified integer argument not found.");
			slen1 = oper1->data.number;
			CLEAR(oper1);
                        CHECKOP(0);
		    } else {
			slen1 = 0;
		    }
		}
		if (sstr[scnt] == '.') {
		    scnt++;
		    slen2 = atoi(&sstr[scnt]);
		    if ((sstr[scnt] >= '0') && (sstr[scnt] <= '9')) {
			while ((sstr[scnt] >= '0') && (sstr[scnt] <= '9'))
			    scnt++;
		    } else {
			if (sstr[scnt] == '*') {
			    scnt++;
			    CHECKOP(1);
			    oper1 = POP();
			    if (oper1->type != PROG_INTEGER)
				abort_interp("Format specified integer argument not found.");
			    if (oper1->data.number < 0)
				abort_interp
					("Dynamic precision value must be a positive integer.");
			    slen2 = oper1->data.number;
			    CLEAR(oper1);
                            CHECKOP(0);
			} else {
			    abort_interp("Invalid format string.");
			}
		    }
		} else {
		    slen2 = -1;
		}

		/* If s is the format and oper1 is really a string, repair the lengths to account for ansi codes. */
		CHECKOP(1);
		oper1 = POP();
		if (('s' == sstr[scnt]) && (PROG_STRING == oper1->type)
		    && (oper1->data.string)) {
		    ptr = oper1->data.string->data;

		    i = 0;
		    while ((-1 == slen2 || i < slen2) && *ptr) {	/* adapted from prim_ansi_strlen */
			begptr = ptr;
			if (*ptr++ == ESCAPE_CHAR) {
			    if (*ptr == '\0') {;
			    } else if (*ptr != '[') {
				ptr++;
			    } else {
				ptr++;
				while (isdigit(*ptr) || *ptr == ';')
				    ptr++;
				if (*ptr == 'm')
				    ptr++;
			    }
			    i += (int) (ptr - begptr);
			    slen1 += (int) (ptr - begptr);
			    if (-1 != slen2)
				slen2 += (int) (ptr - begptr);
			} else {
			    i++;
			};
		    }
		}

		if (slen1 && ((abs(slen1) + result) > BUFFER_LEN))
		    abort_interp("Specified format field width too large.");
		sfmt[0] = '%';
		sfmt[1] = '\0';
		if (slrj == 1)
		    strcatn(sfmt, sizeof(sfmt), "-");
		if (spad1) {
		    if (spad1 == 1)
			strcatn(sfmt, sizeof(sfmt), "+");
		    else
			strcatn(sfmt, sizeof(sfmt), " ");
		}
		if (spad2)
		    strcatn(sfmt, sizeof(sfmt), "0");
		if (slen1 != 0) {
		    snprintf(tbuf, sizeof(tbuf), "%d", slen1);
		    strcatn(sfmt, sizeof(sfmt), tbuf);
		}
		if (slen2 != -1) {
		    snprintf(tbuf, sizeof(tbuf), ".%d", slen2);
		    strcatn(sfmt, sizeof(sfmt), tbuf);
		}

		if (sstr[scnt] == '~') {
		    switch (oper1->type) {
		    case PROG_OBJECT:
			sstr[scnt] = 'd';
			break;
		    case PROG_FLOAT:
			sstr[scnt] = 'g';
			break;
		    case PROG_INTEGER:
			sstr[scnt] = 'i';
			break;
		    case PROG_LOCK:
			sstr[scnt] = 'l';
			break;
		    case PROG_STRING:
			sstr[scnt] = 's';
			break;
		    default:
			sstr[scnt] = '?';
			break;
		    }
		}
		switch (sstr[scnt]) {
		case 'i':
		    strcatn(sfmt, sizeof(sfmt), "d");
		    if (oper1->type != PROG_INTEGER)
			abort_interp("Format specified integer argument not found.");
		    snprintf(tbuf, sizeof(tbuf), sfmt, oper1->data.number);
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (tlen + result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += tlen;
		    break;
		case 'S':
		case 's':
		    strcatn(sfmt, sizeof(sfmt), "s");
		    if (oper1->type != PROG_STRING)
			abort_interp("Format specified string argument not found.");
		    snprintf(tbuf, sizeof(tbuf), sfmt, DoNullInd(oper1->data.string));
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (strlen(tbuf) + (size_t)result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += strlen(tbuf);
		    break;
		case '?':
		    strcatn(sfmt, sizeof(sfmt), "s");
		    switch (oper1->type) {
		    case PROG_OBJECT:
			strcpyn(hold, sizeof(hold), "OBJECT");
			break;
		    case PROG_FLOAT:
			strcpyn(hold, sizeof(hold), "FLOAT");
			break;
		    case PROG_INTEGER:
			strcpyn(hold, sizeof(hold), "INTEGER");
			break;
		    case PROG_LOCK:
			strcpyn(hold, sizeof(hold), "LOCK");
			break;
		    case PROG_STRING:
			strcpyn(hold, sizeof(hold), "STRING");
			break;
		    case PROG_VAR:
			strcpyn(hold, sizeof(hold), "VARIABLE");
			break;
		    case PROG_LVAR:
			strcpyn(hold, sizeof(hold), "LOCAL-VARIABLE");
			break;
		    case PROG_SVAR:
			strcpyn(hold, sizeof(hold), "SCOPED-VARIABLE");
			break;
		    case PROG_ADD:
			strcpyn(hold, sizeof(hold), "ADDRESS");
			break;
		    case PROG_ARRAY:
			strcpyn(hold, sizeof(hold), "ARRAY");
			break;
		    case PROG_FUNCTION:
			strcpyn(hold, sizeof(hold), "FUNCTION-NAME");
			break;
		    case PROG_IF:
			strcpyn(hold, sizeof(hold), "IF-STATEMENT");
			break;
		    case PROG_EXEC:
			strcpyn(hold, sizeof(hold), "EXECUTE");
			break;
		    case PROG_JMP:
			strcpyn(hold, sizeof(hold), "JUMP");
			break;
		    case PROG_PRIMITIVE:
			strcpyn(hold, sizeof(hold), "PRIMITIVE");
			break;
		    default:
			strcpyn(hold, sizeof(hold), "UNKNOWN");
		    }
		    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (strlen(tbuf) + (size_t)result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += strlen(tbuf);
		    break;
		case 'd':
		    strcatn(sfmt, sizeof(sfmt), "s");
		    if (oper1->type != PROG_OBJECT)
			abort_interp("Format specified object not found.");
		    snprintf(hold, sizeof(hold), "#%d", oper1->data.objref);
		    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (strlen(tbuf) + (size_t)result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += strlen(tbuf);
		    break;
		case 'D':
		    strcatn(sfmt, sizeof(sfmt), "s");
		    if (oper1->type != PROG_OBJECT)
			abort_interp("Format specified object not found.");
		    if (!valid_object(oper1))
			abort_interp("Format specified object not valid.");
		    ref = oper1->data.objref;
		    CHECKREMOTE(ref);
		    /* if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
		       ts_lastuseobject(ref); */
		    if (NAME(ref)) {
			strcpyn(hold, sizeof(hold), NAME(ref));
		    } else {
			hold[0] = '\0';
		    }
		    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (strlen(tbuf) + (size_t)result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += strlen(tbuf);
		    break;
		case 'l':
		    strcatn(sfmt, sizeof(sfmt), "s");
		    if (oper1->type != PROG_LOCK)
			abort_interp("Format specified lock not found.");
		    strcpyn(hold, sizeof(hold), unparse_boolexp(ProgUID, oper1->data.lock, 1));
		    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (strlen(tbuf) + (size_t)result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += strlen(tbuf);
		    break;
		case 'f':
		case 'e':
		case 'g':
		    strcatn(sfmt, sizeof(sfmt), "l");
		    snprintf(hold, sizeof(hold), "%c", sstr[scnt]);
		    strcatn(sfmt, sizeof(sfmt), hold);
		    if (oper1->type != PROG_FLOAT)
			abort_interp("Format specified float not found.");
		    snprintf(tbuf, sizeof(tbuf), sfmt, oper1->data.fnumber);
		    tlen = strlen(tbuf);
		    if (slrj == 2) {
			tnum = 0;
			while ((tbuf[tnum] == ' ') && (tnum < tlen))
			    tnum++;
			if ((tnum) && (tnum < tlen)) {
			    temp = tnum / 2;
			    for (i = tnum; i < tlen; i++)
				tbuf[i - temp] = tbuf[i];
			    for (i = tlen - temp; i < tlen; i++)
				tbuf[i] = ' ';
			}
		    }
		    if (strlen(tbuf) + (size_t)result > BUFFER_LEN)
			abort_interp("Resultant string would overflow buffer.");
		    buf[result] = '\0';
		    strcatn(buf, sizeof(buf), tbuf);
		    result += strlen(tbuf);
		    break;
		default:
		    abort_interp("Invalid format string.");
		}
                CLEAR(oper1);
                CHECKOP(0);
		scnt++;
		tstop += strlen(tbuf);
	    }
	} else {
	    if ((sstr[scnt] == '\\') && (sstr[scnt + 1] == 't')) {
		if ((result - (tstop % 8) + 1) >= BUFFER_LEN)
		    abort_interp("Resultant string would overflow buffer.");
		if ((tstop % 8) == 0) {
		    buf[result++] = ' ';
		    tstop++;
		}
		while ((tstop % 8) != 0) {
		    buf[result++] = ' ';
		    tstop++;
		}
		scnt += 2;
		tstop = 0;
	    } else {
		if (sstr[scnt] == '\r') {
		    tstop = 0;
		    scnt++;
		    buf[result++] = '\r';
		} else {
		    buf[result++] = sstr[scnt++];
		    tstop++;
		}
	    }
	}
    }
    CHECKOP(0);
    if (result >= BUFFER_LEN)
	abort_interp("Resultant string would overflow buffer.");
    buf[result] = '\0';
    if (result)
	PushString(buf);
    else
	PushNullStr;
}

void
prim_array_fmtstrings(PRIM_PROTOTYPE)
{
    int slen, scnt, tstop, tlen, tnum, i;
    int slrj, spad1, spad2, slen1, slen2, temp;
    char sstr[BUFFER_LEN], sfmt[255], hold[256], tbuf[BUFFER_LEN];
    char *ptr, *begptr;
    char fieldbuf[BUFFER_LEN];
    char *fieldname;
    char *fmtstr = NULL;
    stk_array *arr = NULL;
    stk_array *arr2 = NULL;
    stk_array *nu = NULL;
    const char *abort_message = NULL;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (oper1->type != PROG_ARRAY)
	abort_interp("Argument not an array of arrays. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_ARRAY))
	abort_interp("Argument not a homogenous array of arrays. (1)");
    arr = oper1->data.array;

    if (oper2->type != PROG_STRING)
	abort_interp("Expected string argument. (2)");
    fmtstr = DoNullInd(oper2->data.string);
    slen = strlen(fmtstr);

    nu = new_array_packed(0, fr->pinning);
    if (array_first(arr, &temp1)) {
	do {
	    strcpyn(sstr, sizeof(sstr), fmtstr);
	    result = 0;		/* End of current string, must be smaller than BUFFER_LEN */
	    tmp = 0;		/* Number of props to search for/found */
	    scnt = 0;
	    tstop = 0;
	    /*   "%-20.19[name]s %6[dbref]d"   */
	    while ((scnt < slen) && (result < BUFFER_LEN)) {
		if (sstr[scnt] == FMTTOKEN) {
		    if (sstr[scnt + 1] == FMTTOKEN) {
			buf[result++] = FMTTOKEN;
			scnt += 2;
		    } else {
			scnt++;
			if ((sstr[scnt] == '-') || (sstr[scnt] == '|')) {
			    if (sstr[scnt] == '-')
				slrj = 1;
			    else
				slrj = 2;
			    scnt++;
			} else {
			    slrj = 0;
			}
			if ((sstr[scnt] == '+') || (sstr[scnt] == ' ')) {
			    if (sstr[scnt] == '+')
				spad1 = 1;
			    else
				spad1 = 2;
			    scnt++;
			} else {
			    spad1 = 0;
			}
			if (sstr[scnt] == '0') {
			    scnt++;
			    spad2 = 1;
			} else {
			    spad2 = 0;
			}
			slen1 = atoi(&sstr[scnt]);
			if ((sstr[scnt] >= '0') && (sstr[scnt] <= '9')) {
			    while ((sstr[scnt] >= '0') && (sstr[scnt] <= '9'))
				scnt++;
			} else {
			    slen1 = -1;
			}
			if (sstr[scnt] == '.') {
			    scnt++;
			    slen2 = atoi(&sstr[scnt]);
			    if ((sstr[scnt] >= '0') && (sstr[scnt] <= '9')) {
				while ((sstr[scnt] >= '0') && (sstr[scnt] <= '9'))
				    scnt++;
			    } else {
                                abort_message = "Invalid format string.";
                                goto cleanup_and_abort;
			    }
			} else {
			    slen2 = -1;
			}

			if (sstr[scnt] == '[') {
			    scnt++;
			    fieldname = fieldbuf;
			    while (sstr[scnt] && sstr[scnt] != ']') {
				*fieldname++ = sstr[scnt++];
			    }
			    if (sstr[scnt] != ']') {
                                abort_message = 
                                    "Specified format field didn't have an array index terminator ']'.";
                                goto cleanup_and_abort;
			    }
			    scnt++;
			    *fieldname++ = '\0';

			    oper3 = array_getitem(arr, &temp1);
			    arr2 = oper3->data.array;
			    oper3 = NULL;
			    if (number(fieldbuf)) {
				temp2.type = PROG_INTEGER;
				temp2.data.number = atoi(fieldbuf);
				oper3 = array_getitem(arr2, &temp2);
			    }
			    if (!oper3) {
				temp2.type = PROG_STRING;
				temp2.data.string = alloc_prog_string(fieldbuf);
				oper3 = array_getitem(arr2, &temp2);
				CLEAR(&temp2);
			    }
			    if (!oper3) {
				switch (sstr[scnt]) {
				case 'l':
				    temp3.type = PROG_LOCK;
				    temp3.data.lock = TRUE_BOOLEXP;
				    oper3 = &temp3;
				    break;
				case 'i':
				    temp3.type = PROG_INTEGER;
				    temp3.data.number = 0;
				    oper3 = &temp3;
				    break;
				case 'e':
				case 'f':
				case 'g':
				    temp3.type = PROG_FLOAT;
				    temp3.data.fnumber = 0.0;
				    oper3 = &temp3;
				    break;
				case 'd':
				case 'D':
				    temp3.type = PROG_OBJECT;
				    temp3.data.objref = NOTHING;
				    oper3 = &temp3;
				    break;
				case 's':
				case 'S':
				case '?':
				case '~':
				    temp3.type = PROG_STRING;
				    temp3.data.string = NULL;
				    oper3 = &temp3;
				    break;
				default:
				    break;
				}
			    }
			    nargs = 2;
			} else {
                            abort_message = "Specified format field didn't have an array index.";
                            goto cleanup_and_abort;
			}

			/* If s is the format and oper3 is really a string, repair the lengths to account for ansi codes. */
			if (('s' == sstr[scnt]) && (PROG_STRING == oper3->type)
			    && (oper3->data.string)) {
			    ptr = oper3->data.string->data;

			    i = 0;
			    while ((-1 == slen2 || i < slen2) && *ptr) {	/* adapted from prim_ansi_strlen */
				begptr = ptr;
				if (*ptr++ == ESCAPE_CHAR) {
				    if (*ptr == '\0') {;
				    } else if (*ptr != '[') {
					ptr++;
				    } else {
					ptr++;
					while (isdigit(*ptr) || *ptr == ';')
					    ptr++;
					if (*ptr == 'm')
					    ptr++;
				    }
				    i += (int) (ptr - begptr);
				    slen1 += (int) (ptr - begptr);
				    if (-1 != slen2)
					slen2 += (int) (ptr - begptr);
				} else {
				    i++;
				};
			    }
			}
			if ((slen1 > 0) && ((slen1 + result) > BUFFER_LEN)) {
                            abort_message = "Specified format field width too large.";
                            goto cleanup_and_abort;
                        }

			sfmt[0] = '%';
			sfmt[1] = '\0';
			if (slrj == 1)
			    strcatn(sfmt, sizeof(sfmt), "-");
			if (spad1) {
			    if (spad1 == 1)
				strcatn(sfmt, sizeof(sfmt), "+");
			    else
				strcatn(sfmt, sizeof(sfmt), " ");
			}
			if (spad2)
			    strcatn(sfmt, sizeof(sfmt), "0");
			if (slen1 != -1) {
			    snprintf(tbuf, sizeof(tbuf), "%d", slen1);
			    strcatn(sfmt, sizeof(sfmt), tbuf);
			}
			if (slen2 != -1) {
			    snprintf(tbuf, sizeof(tbuf), ".%d", slen2);
			    strcatn(sfmt, sizeof(sfmt), tbuf);
			}
			if (sstr[scnt] == '~') {
			    switch (oper3->type) {
			    case PROG_OBJECT:
				sstr[scnt] = 'D';
				break;
			    case PROG_FLOAT:
				sstr[scnt++] = 'l';
				sstr[scnt] = 'g';
				break;
			    case PROG_INTEGER:
				sstr[scnt] = 'i';
				break;
			    case PROG_LOCK:
				sstr[scnt] = 'l';
				break;
			    case PROG_STRING:
				sstr[scnt] = 's';
				break;
			    default:
				sstr[scnt] = '?';
				break;
			    }
			}
			switch (sstr[scnt]) {
			case 'i':
			    strcatn(sfmt, sizeof(sfmt), "d");
			    if (oper3->type != PROG_INTEGER) {
				abort_message = "Format specified integer argument not found.";
                                goto cleanup_and_abort;
                            }
			    snprintf(tbuf, sizeof(tbuf), sfmt, oper3->data.number);
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (tlen + result > BUFFER_LEN) {
				abort_message = "Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += tlen;
			    break;
			case 'S':
			case 's':
			    strcatn(sfmt, sizeof(sfmt), "s");
			    if (oper3->type != PROG_STRING) {
				abort_message = "Format specified string argument not found.";
                                goto cleanup_and_abort;
                            }
			    snprintf(tbuf, sizeof(tbuf), sfmt, DoNullInd(oper3->data.string));
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (strlen(tbuf) + (size_t)result > BUFFER_LEN) {
				abort_message ="Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += strlen(tbuf);
			    break;
			case '?':
			    strcatn(sfmt, sizeof(sfmt), "s");
			    switch (oper3->type) {
			    case PROG_OBJECT:
				strcpyn(hold, sizeof(hold), "OBJECT");
				break;
			    case PROG_FLOAT:
				strcpyn(hold, sizeof(hold), "FLOAT");
				break;
			    case PROG_INTEGER:
				strcpyn(hold, sizeof(hold), "INTEGER");
				break;
			    case PROG_LOCK:
				strcpyn(hold, sizeof(hold), "LOCK");
				break;
			    case PROG_STRING:
				strcpyn(hold, sizeof(hold), "STRING");
				break;
			    case PROG_VAR:
				strcpyn(hold, sizeof(hold), "VARIABLE");
				break;
			    case PROG_LVAR:
				strcpyn(hold, sizeof(hold), "LOCAL-VARIABLE");
				break;
			    case PROG_SVAR:
				strcpyn(hold, sizeof(hold), "SCOPED-VARIABLE");
				break;
			    case PROG_ADD:
				strcpyn(hold, sizeof(hold), "ADDRESS");
				break;
			    case PROG_ARRAY:
				strcpyn(hold, sizeof(hold), "ARRAY");
				break;
			    case PROG_FUNCTION:
				strcpyn(hold, sizeof(hold), "FUNCTION-NAME");
				break;
			    case PROG_IF:
				strcpyn(hold, sizeof(hold), "IF-STATEMENT");
				break;
			    case PROG_EXEC:
				strcpyn(hold, sizeof(hold), "EXECUTE");
				break;
			    case PROG_JMP:
				strcpyn(hold, sizeof(hold), "JUMP");
				break;
			    case PROG_PRIMITIVE:
				strcpyn(hold, sizeof(hold), "PRIMITIVE");
				break;
			    default:
				strcpyn(hold, sizeof(hold), "UNKNOWN");
			    }
			    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (strlen(tbuf) + (size_t)result > BUFFER_LEN) {
				abort_message = "Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += strlen(tbuf);
			    break;
			case 'd':
			    strcatn(sfmt, sizeof(sfmt), "s");
			    if (oper3->type != PROG_OBJECT) {
				abort_message = "Format specified object not found.";
                                goto cleanup_and_abort;
                            }
			    if (!valid_object(oper3) && oper3->data.objref != NOTHING) {
				abort_message = "Format specified object not found.";
                                goto cleanup_and_abort;
                            }
			    snprintf(hold, sizeof(hold), "#%d", oper3->data.objref);
			    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (strlen(tbuf) + (size_t)result > BUFFER_LEN) {
				abort_message = "Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += strlen(tbuf);
			    break;
			case 'D':
			    strcatn(sfmt, sizeof(sfmt), "s");
			    if (oper3->type != PROG_OBJECT) {
				abort_message = "Format specified object not found.";
                                goto cleanup_and_abort;
                            }
			    if (!valid_object(oper3) && oper3->data.objref != NOTHING) {
                                abort_message = "Format specified object not found.";
                                goto cleanup_and_abort;
                            }
			    ref = oper3->data.objref;
			    CHECKREMOTE(ref);
			    if (ref != NOTHING && NAME(ref)) {
				strcpyn(hold, sizeof(hold), NAME(ref));
			    } else {
				hold[0] = '\0';
			    }
			    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (strlen(tbuf) + (size_t)result > BUFFER_LEN) {
                                abort_message = "Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += strlen(tbuf);
			    break;
			case 'l':
			    strcatn(sfmt, sizeof(sfmt), "s");
			    if (oper3->type != PROG_LOCK) {
				abort_message = "Format specified lock not found.";
                                goto cleanup_and_abort;
                            }
			    strcpyn(hold, sizeof(hold),
				    unparse_boolexp(ProgUID, oper3->data.lock, 1));
			    snprintf(tbuf, sizeof(tbuf), sfmt, hold);
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (strlen(tbuf) + (size_t)result > BUFFER_LEN) {
				abort_message = "Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += strlen(tbuf);
			    break;
			case 'f':
			case 'e':
			case 'g':
			    strcatn(sfmt, sizeof(sfmt), "l");
			    snprintf(hold, sizeof(hold), "%c", sstr[scnt]);
			    strcatn(sfmt, sizeof(sfmt), hold);
			    if (oper3->type != PROG_FLOAT) {
				abort_message = "Format specified float not found.";
                                goto cleanup_and_abort;
                            }
			    snprintf(tbuf, sizeof(tbuf), sfmt, oper3->data.fnumber);
			    tlen = strlen(tbuf);
			    if (slrj == 2) {
				tnum = 0;
				while ((tbuf[tnum] == ' ') && (tnum < tlen))
				    tnum++;
				if ((tnum) && (tnum < tlen)) {
				    temp = tnum / 2;
				    for (i = tnum; i < tlen; i++)
					tbuf[i - temp] = tbuf[i];
				    for (i = tlen - temp; i < tlen; i++)
					tbuf[i] = ' ';
				}
			    }
			    if (strlen(tbuf) + (size_t)result > BUFFER_LEN) {
				abort_message ="Resultant string would overflow buffer.";
                                goto cleanup_and_abort;
                            }
			    buf[result] = '\0';
			    strcatn(buf, sizeof(buf), tbuf);
			    result += strlen(tbuf);
			    break;
			default:
			    abort_interp("Invalid format string.");
			}
			nargs = 2;
			scnt++;
			tstop += strlen(tbuf);
		    }
		} else {
		    if ((sstr[scnt] == '\\') && (sstr[scnt + 1] == 't')) {
			if ((result - (tstop % 8) + 1) >= BUFFER_LEN) {
                            abort_message = "Resultant string would overflow buffer.";
                            goto cleanup_and_abort;
                        }
			if ((tstop % 8) == 0) {
			    buf[result++] = ' ';
			    tstop++;
			}
			while ((tstop % 8) != 0) {
			    buf[result++] = ' ';
			    tstop++;
			}
			scnt += 2;
			tstop = 0;
		    } else {
			if (sstr[scnt] == '\r') {
			    tstop = 0;
			    scnt++;
			    buf[result++] = '\r';
			} else {
			    buf[result++] = sstr[scnt++];
			    tstop++;
			}
		    }
		}
	    }
	    if (result >= BUFFER_LEN) {
                abort_message = "Resultant string would overflow buffer.";
                goto cleanup_and_abort;
            }
	    buf[result] = '\0';

	    temp2.type = PROG_STRING;
	    temp2.data.string = alloc_prog_string(buf);
	    array_appenditem(&nu, &temp2);
	    CLEAR(&temp2);
	} while (array_next(arr, &temp1));
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nu);
    return;

cleanup_and_abort:
    array_free(nu);
    abort_interp(abort_message);
}

void
prim_split(PRIM_PROTOTYPE)
{
    char *temp = NULL;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument. (2)");
    if (!oper1->data.string)
	abort_interp("Null string split argument. (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument. (1)");

    *buf = '\0';
    if (!oper2->data.string) {
	result = 0;
    } else {
	strcpyn(buf, sizeof(buf), oper2->data.string->data);
	pname = strstr(buf, oper1->data.string->data);
	if (!pname) {
	    result = -1;
	} else {
	    temp = pname + oper1->data.string->length;
	    *pname = '\0';
	    result = 1;
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    if (result > 0) {
	if (result == 1) {
	    if (buf[0] == '\0') {
		PushNullStr;
	    } else {
		PushString(buf);
	    }
	    if (!temp || temp[0] == '\0') {
		PushNullStr;
	    } else {
		PushString(temp);
	    }
	} else {
	    PushString(buf);
	    PushNullStr;
	}
    } else {
	PushString(buf);
	PushNullStr;
    }
}

void
prim_rsplit(PRIM_PROTOTYPE)
{
    char *temp = NULL, *hold = NULL;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument. (2)");
    if (!oper1->data.string)
	abort_interp("Null split argument. (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument. (1)");

    *buf = '\0';
    if (!oper2->data.string) {
	result = 0;
    } else {
	if (oper1->data.string->length > oper2->data.string->length) {
	    result = -1;
	} else {
	    strcpyn(buf, sizeof(buf), oper2->data.string->data);
	    temp = buf + (oper2->data.string->length - oper1->data.string->length);
	    hold = NULL;
	    while ((temp >= buf) && (!hold)) {
		if (*temp == *(oper1->data.string->data))
		    if (!strncmp(temp, oper1->data.string->data, oper1->data.string->length))
			hold = temp;
		temp--;
	    }
	    if (!hold) {
		result = -1;
	    } else {
		*hold = '\0';
		hold += oper1->data.string->length;
		result = 1;
	    }
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    if (result) {
	if (result == 1) {
	    if (buf[0] == '\0') {
		PushNullStr;
	    } else {
		PushString(buf);
	    }
	    if (hold && hold[0] == '\0') {
		PushNullStr;
	    } else {
		PushString(hold);
	    }
	} else {
	    PushString(buf);
	    PushNullStr;
	}
    } else {
	PushString(buf);
	PushNullStr;
    }
}

void
prim_ctoi(PRIM_PROTOTYPE)
{
    char c;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument. (1)");
    if (!oper1->data.string) {
	c = '\0';
    } else {
	c = oper1->data.string->data[0];
    }
    result = c;
    CLEAR(oper1);
    PushInt(result);
}

void
prim_itoc(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if ((oper1->type != PROG_INTEGER) || (oper1->data.number < 0))
	abort_interp("Argument must be a positive integer. (1)");
    if ((!isprint((char) oper1->data.number & 127) &&
	 ((char) oper1->data.number != '\r') && ((char) oper1->data.number != ESCAPE_CHAR))) {
	result = 0;
    } else {
	result = 1;
	buf[0] = (char) oper1->data.number;
	buf[1] = '\0';
    }
    CLEAR(oper1);
    if (result) {
	PushString(buf);
    } else {
	PushNullStr;
    }
}

void
prim_stod(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument. (1)");

    if (!oper1->data.string) {
	ref = NOTHING;
    } else {
	const char *ptr = oper1->data.string->data;
	const char *nptr = NULL;

	skip_whitespace(&ptr);
	if (*ptr == NUMBER_TOKEN)
	    ptr++;
	if (*ptr == '+')
	    ptr++;
	nptr = ptr;
	if (*nptr == '-')
	    nptr++;
	while (*nptr && !isspace(*nptr) && (*nptr >= '0' && *nptr <= '9')) {
	    nptr++;
	}			/* while */
	if (*nptr && !isspace(*nptr)) {
	    ref = NOTHING;
	} else {
	    ref = (dbref) atoi(ptr);
	}			/* if */
    }
    CLEAR(oper1);
    PushObject(ref);
}

void
prim_midstr(PRIM_PROTOTYPE)
{
    size_t start, range;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    result = 0;
    if (oper3->type != PROG_STRING)
	abort_interp("Non-string argument. (1)");
    if (oper2->type != PROG_INTEGER)
	abort_interp("Non-integer argument. (2)");
    if (oper2->data.number < 1)
	abort_interp("Data must be a positive integer. (2)");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument. (3)");
    if (oper1->data.number < 0)
	abort_interp("Data must be a positive integer. (3)");
    if (!oper3->data.string) {
	result = 1;
    } else {
	if ((size_t)oper2->data.number > oper3->data.string->length) {
	    result = 1;
	} else {
	    start = (size_t)oper2->data.number - 1;
	    if (((size_t)oper1->data.number + start) > oper3->data.string->length) {
		range = oper3->data.string->length - start;
	    } else {
		range = (size_t)oper1->data.number;
	    }
	    memmove(buf, oper3->data.string->data + start, range);
	    buf[range] = '\0';
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    if (result) {
	PushNullStr;
    } else {
	PushString(buf);
    }
}

void
prim_numberp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING || !oper1->data.string)
	result = 0;
    else
	result = number(oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_stringcmp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string == oper2->data.string)
	result = 0;
    else if (!(oper2->data.string && oper1->data.string))
	result = oper1->data.string ? -1 : 1;
    else
	result = strcasecmp(oper2->data.string->data, oper1->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_strcmp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string == oper2->data.string)
	result = 0;
    else if (!(oper2->data.string && oper1->data.string))
	result = oper1->data.string ? -1 : 1;
    else
	result = strcmp(oper2->data.string->data, oper1->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_strncmp(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument.");
    if (oper2->type != PROG_STRING || oper3->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper2->data.string == oper3->data.string)
	result = 0;
    else
	result = strncmp(DoNullInd(oper3->data.string),
			 DoNullInd(oper2->data.string), (size_t)oper1->data.number);
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

void
prim_strcut(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    temp1 = *(oper1 = POP());
    temp2 = *(oper2 = POP());
    if (temp1.type != PROG_INTEGER)
	abort_interp("Non-integer argument (2)");
    if (temp1.data.number < 0)
	abort_interp("Argument must be a positive integer.");
    if (temp2.type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!temp2.data.string) {
	PushNullStr;
	PushNullStr;
    } else {
	if ((size_t)temp1.data.number > temp2.data.string->length) {
	    temp2.data.string->links++;
	    PushStrRaw(temp2.data.string);
	    PushNullStr;
	} else {
	    memmove(buf, temp2.data.string->data, temp1.data.number);
	    buf[temp1.data.number] = '\0';
	    PushString(buf);
	    if (temp2.data.string->length > (size_t)temp1.data.number) {
		memmove(buf, temp2.data.string->data + temp1.data.number, temp2.data.string->length - (size_t)temp1.data.number + 1);
		PushString(buf);
	    } else {
		PushNullStr;
	    }
	}
    }
    CLEAR(&temp1);
    CLEAR(&temp2);
}

void
prim_strlen(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string)
	result = 0;
    else
	result = oper1->data.string->length;
    CLEAR(oper1);
    PushInt(result);
}

void
prim_strcat(PRIM_PROTOTYPE)
{
    struct shared_string *string;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (!oper1->data.string && !oper2->data.string)
	string = NULL;
    else if (!oper2->data.string) {
	oper1->data.string->links++;
	string = oper1->data.string;
    } else if (!oper1->data.string) {
	oper2->data.string->links++;
	string = oper2->data.string;
    } else if (oper1->data.string->length + oper2->data.string->length > (BUFFER_LEN) - 1) {
	abort_interp("Operation would result in overflow.");
    } else {
	memmove(buf, oper2->data.string->data, oper2->data.string->length);
	memmove(buf + oper2->data.string->length, oper1->data.string->data, oper1->data.string->length + 1);
	string = alloc_prog_string(buf);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushStrRaw(string);
}

void
prim_atoi(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING || !oper1->data.string)
	result = 0;
    else
	result = atoi(oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_notify(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;	/* prevents re-entrancy issues! */

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid object argument (1)");
    CHECKREMOTE(oper2->data.objref);

    if (oper1->data.string) {
	if (tp_force_mlev1_name_notify && mlev < 2 && player != oper2->data.objref) {
	    prefix_message(buf, oper1->data.string->data, NAME(player), BUFFER_LEN, 1);
	} else {
	    /* TODO: The original code made this copy, is it really necessary? */
	    strcpyn(buf, sizeof(buf), oper1->data.string->data);
	}

	notify_listeners(player, program, oper2->data.objref,
			 LOCATION(oper2->data.objref), buf, 1);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_tell(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (1)");

    if (oper1->data.string) {
	notify_listeners(player, program, player, LOCATION(player),
		oper1->data.string->data, 1);
    }

    CLEAR(oper1);
}

void
prim_notify_nolisten(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;	/* prevents re-entrancy issues! */

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Invalid object argument (1)");
    CHECKREMOTE(oper2->data.objref);

    if (oper1->data.string) {
	if (tp_force_mlev1_name_notify && mlev < 2 && player != oper2->data.objref) {
	    prefix_message(buf, oper1->data.string->data, NAME(player), BUFFER_LEN, 1);
	} else {
	    /* TODO: The original code made this copy, is it really necessary? */
	    strcpyn(buf, sizeof(buf), oper1->data.string->data);
	}

	notify_nolisten(player, buf, 1);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_notify_exclude(PRIM_PROTOTYPE)
{
    /* roomD excludeDn ... excludeD1 nI messageS  -- */
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;	/* prevents re-entrancy issues! */

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper2->type != PROG_INTEGER)
	abort_interp("non-integer count argument (top-1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string message argument (top)");

    if (tp_force_mlev1_name_notify && mlev < 2) {
	prefix_message(buf, DoNullInd(oper1->data.string), NAME(player), BUFFER_LEN, 1);
    } else
	strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));

    result = oper2->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    {
	dbref what, where, excluded[STACK_SIZE];
	int count, i;

	CHECKOP(0);
	count = i = result;
	if (i >= STACK_SIZE || i < 0)
	    abort_interp("Count argument is out of range.");
	while (i > 0) {
	    CHECKOP(1);
	    oper1 = POP();
	    if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid object argument.");
	    excluded[--i] = oper1->data.objref;
	    CLEAR(oper1);
	}
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
	    abort_interp("Non-object argument (1)");
	where = oper1->data.objref;
	if (Typeof(where) != TYPE_ROOM && Typeof(where) != TYPE_THING &&
	    Typeof(where) != TYPE_PLAYER)
	    abort_interp("Invalid location argument (1)");
	CHECKREMOTE(where);
	what = CONTENTS(where);
	CLEAR(oper1);
	if (*buf) {
	    while (what != NOTHING) {
		if (Typeof(what) != TYPE_ROOM) {
		    for (tmp = 0, i = count; i-- > 0;) {
			if (excluded[i] == what)
			    tmp = 1;
		    }
		} else {
		    tmp = 1;
		}
		if (!tmp)
		    notify_listeners(player, program, what, where, buf, 0);
		what = NEXTOBJ(what);
	    }
	}

	if (tp_allow_listeners) {
	    for (tmp = 0, i = count; i-- > 0;) {
		if (excluded[i] == where)
		    tmp = 1;
	    }
	    if (!tmp)
		notify_listeners(player, program, where, where, buf, 0);
	    if (tp_allow_listeners_env && !tmp) {
		what = LOCATION(where);
		for (; what != NOTHING; what = LOCATION(what))
		    notify_listeners(player, program, what, where, buf, 0);
	    }
	}
    }
}

void
prim_otell(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;	/* prevents re-entrancy issues! */

    dbref what, where;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (1)");

    if (tp_force_mlev1_name_notify && mlev < 2) {
	prefix_message(buf, DoNullInd(oper1->data.string), NAME(player), BUFFER_LEN, 1);
    } else
	strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));

    where = LOCATION(player);
    what = CONTENTS(where);

    if (*buf) {
	for (; what != NOTHING; what = NEXTOBJ(what)) {
	    if (Typeof(what) != TYPE_ROOM && what != player) {
		notify_listeners(player, program, what, where, buf, 0);
	    }
	}
    }

    if (tp_allow_listeners) {
	notify_listeners(player, program, where, where, buf, 0);
	if (tp_allow_listeners_env) {
	    for (what = LOCATION(where); what != NOTHING; what = LOCATION(what)) {
		notify_listeners(player, program, what, where, buf, 0);
	    }
	}
    }
    CLEAR(oper1);
}

void
prim_intostr(PRIM_PROTOTYPE)
{
    char *ptr = NULL;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type >= PROG_STRING)	/* accepts: int, flt, ref, var */
	abort_interp("Invalid argument.");
    if (oper1->type == PROG_FLOAT) {
	snprintf(buf, sizeof(buf), "%.15g", oper1->data.fnumber);
	if (!strchr(buf, '.') && !strchr(buf, 'n') && !strchr(buf, 'e')) {
	    strcatn(buf, sizeof(buf), ".0");
	}
    } else {
	snprintf(buf, sizeof(buf), "%d", oper1->data.number);
    }
    ptr = buf;
    CLEAR(oper1);
    PushString(ptr);
}

void
prim_explode(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    temp1 = *(oper1 = POP());
    temp2 = *(oper2 = POP());
    oper1 = &temp1;
    oper2 = &temp2;
    if (temp1.type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (temp2.type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!temp1.data.string)
	abort_interp("Empty string argument (2)");
    {
	const char *delimit = temp1.data.string->data;

	if (!temp2.data.string) {
	    result = 1;
	    CLEAR(&temp1);
	    CLEAR(&temp2);
	    PushNullStr;
	    PushInt(result);
	    return;
	} else {
	    result = 0;
	    memmove(buf, temp2.data.string->data, temp2.data.string->length + 1);
	    for (int i = temp2.data.string->length - 1; i >= 0; i--) {
		if (!strncmp(buf + i, delimit, temp1.data.string->length)) {
		    buf[i] = '\0';
		    CHECKOFLOW(1);
		    PushString((buf + i + temp1.data.string->length));
		    result++;
		}
	    }
	    CHECKOFLOW(1);
	    PushString(buf);
	    result++;
	}
    }
    CHECKOFLOW(1);
    CLEAR(&temp1);
    CLEAR(&temp2);
    PushInt(result);
}

void
prim_explode_array(PRIM_PROTOTYPE)
{
    stk_array *nu;
    char *tempPtr;
    char *lastPtr;
    CHECKOP(2);
    temp1 = *(oper1 = POP());
    temp2 = *(oper2 = POP());
    oper1 = &temp1;
    oper2 = &temp2;
    if (temp1.type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (temp2.type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!temp1.data.string)
	abort_interp("Empty string argument (2)");
    CHECKOFLOW(1);

    {
	const char *delimit = temp1.data.string->data;
	size_t delimlen = temp1.data.string->length;

	nu = new_array_packed(0, fr->pinning);
	if (!temp2.data.string) {
	    lastPtr = "";
	} else {
	    strcpyn(buf, sizeof(buf), temp2.data.string->data);
	    tempPtr = lastPtr = buf;
	    while (*tempPtr) {
		if (!strncmp(tempPtr, delimit, delimlen)) {
		    *tempPtr = '\0';
		    tempPtr += delimlen;

		    temp3.type = PROG_STRING;
		    temp3.data.string = alloc_prog_string(lastPtr);
		    array_appenditem(&nu, &temp3);
		    CLEAR(&temp3);

		    lastPtr = tempPtr;
		} else {
		    tempPtr++;
		}
	    }
	}
    }

    temp3.type = PROG_STRING;
    temp3.data.string = alloc_prog_string(lastPtr);
    array_appenditem(&nu, &temp3);

    CLEAR(&temp1);
    CLEAR(&temp2);
    CLEAR(&temp3);

    PushArrayRaw(nu);
}

void
prim_subst(PRIM_PROTOTYPE)
{
    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (3)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (3)");
    {
	int i = 0, j = 0, k = 0;
	const char *match;
	const char *replacement;
	char xbuf[BUFFER_LEN];

	buf[0] = '\0';
	if (oper3->data.string) {
	    memmove(xbuf, oper3->data.string->data, oper3->data.string->length + 1);
	    match = oper1->data.string->data;
	    replacement = DoNullInd(oper2->data.string);
	    k = *replacement ? oper2->data.string->length : 0;
	    while (xbuf[i]) {
		if (!strncmp(xbuf + i, match, oper1->data.string->length)) {
		    if ((j + k + 1) > BUFFER_LEN)
			abort_interp("Operation would result in overflow.");
		    strcatn(buf, sizeof(buf), replacement);
		    i += oper1->data.string->length;
		    j += k;
		} else {
		    if ((j + 2) > BUFFER_LEN)
			abort_interp("Operation would result in overflow.");
		    buf[j++] = xbuf[i++];
		    buf[j] = '\0';
		}
	    }
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushString(buf);
}

void
prim_instr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper2->data.string) {
	result = 0;
    } else {

	const char *remaining = oper2->data.string->data;
	const char *match = oper1->data.string->data;
	int step = 1;

	result = 0;
	do {
	    if (!strncmp(remaining, match, oper1->data.string->length)) {
		result = remaining - oper2->data.string->data + 1;
		break;
	    }
	    remaining += step;
	} while (remaining >= oper2->data.string->data && *remaining);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_rinstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper2->data.string) {
	result = 0;
    } else {

	const char *remaining = oper2->data.string->data;
	const char *match = oper1->data.string->data;
	int step = -1;

	remaining += oper2->data.string->length - 1;

	result = 0;
	do {
	    if (!strncmp(remaining, match, oper1->data.string->length)) {
		result = remaining - oper2->data.string->data + 1;
		break;
	    }
	    remaining += step;
	} while (remaining >= oper2->data.string->data && *remaining);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_instring(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper2->data.string) {
	result = 0;
    } else {
	char buf2[BUFFER_LEN];
	const char *remaining, *match;
	char *cp;

	strcpyn(buf, sizeof(buf), oper1->data.string->data);
	strcpyn(buf2, sizeof(buf2), oper2->data.string->data);
	for (cp = buf; *cp; cp++)
	    *cp = tolower(*cp);
	for (cp = buf2; *cp; cp++)
	    *cp = tolower(*cp);
	match = buf;
	remaining = buf2;

	int step = 1;

	result = 0;
	do {
	    if (!strncmp(remaining, match, oper1->data.string->length)) {
		result = remaining - buf2 + 1;
		break;
	    }
	    remaining += step;
	} while (remaining >= buf2 && *remaining);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_rinstring(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument type (2)");
    if (!(oper1->data.string))
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (1)");
    if (!oper2->data.string) {
	result = 0;
    } else {
	char buf2[BUFFER_LEN];
	const char *remaining, *match;
	char *cp;

	strcpyn(buf, sizeof(buf), oper1->data.string->data);
	strcpyn(buf2, sizeof(buf2), oper2->data.string->data);
	for (cp = buf; *cp; cp++)
	    *cp = tolower(*cp);
	for (cp = buf2; *cp; cp++)
	    *cp = tolower(*cp);
	match = buf;
	remaining = buf2;

	int step = -1;

	remaining += oper2->data.string->length - 1;

	result = 0;
	do {
	    if (!strncmp(remaining, match, oper1->data.string->length)) {
		result = remaining - buf2 + 1;
		break;
	    }
	    remaining += step;
	} while (remaining >= buf2 && *remaining);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_pronoun_sub(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();		/* oper1 is a string, oper2 a dbref */
    if (!valid_object(oper2))
	abort_interp("Invalid argument (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Invalid argument (2)");
    if (oper1->data.string) {
	strcpyn(buf, sizeof(buf), pronoun_substitute(fr->descr, oper2->data.objref,
						     oper1->data.string->data));
    } else {
	buf[0] = '\0';
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(buf);
}

void
prim_toupper(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string) {
	strcpyn(buf, sizeof(buf), oper1->data.string->data);
    } else {
	buf[0] = '\0';
    }
    char *p = buf;
    toupper_string(&p);
    CLEAR(oper1);
    PushString(buf);
}

void
prim_tolower(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string) {
	strcpyn(buf, sizeof(buf), oper1->data.string->data);
    } else {
	buf[0] = '\0';
    }
    char *p = buf;
    tolower_string(&p);
    CLEAR(oper1);
    PushString(buf);
}

void
prim_unparseobj(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_OBJECT)
	abort_interp("Non-object argument.");
    {
	result = oper1->data.objref;
	switch (result) {
	case NOTHING:
	    snprintf(buf, sizeof(buf), "*NOTHING*");
	    break;
	case HOME:
	    snprintf(buf, sizeof(buf), "*HOME*");
	    break;
	case NIL:
	    snprintf(buf, sizeof(buf), "*NIL*");
	    break;
	default:
	    if (!ObjExists(result))
		snprintf(buf, sizeof(buf), "*INVALID*");
	    else
		snprintf(buf, sizeof(buf), "%s(#%d%s)", NAME(result), result,
			 unparse_flags(result));
	}
	CLEAR(oper1);
	PushString(buf);
    }
}

void
prim_smatch(PRIM_PROTOTYPE)
{
    char xbuf[BUFFER_LEN];

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));
    strcpyn(xbuf, sizeof(xbuf), DoNullInd(oper2->data.string));
    result = equalstr(buf, xbuf);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_striplead(PRIM_PROTOTYPE)
{				/* string -- string' */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");
    strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));
    pname = buf;
    skip_whitespace((const char **)&pname);
    CLEAR(oper1);
    PushString(pname);
}

void
prim_striptail(PRIM_PROTOTYPE)
{				/* string -- string' */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");
    strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));
    pname = buf;
    remove_ending_whitespace(&pname);
    CLEAR(oper1);
    PushString(buf);
}

void
prim_stringpfx(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
	abort_interp("Non-string argument.");
    if (oper1->data.string == oper2->data.string)
	result = 1;
    else {
	result = string_prefix(DoNullInd(oper2->data.string), DoNullInd(oper1->data.string));
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_strencrypt(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING) {
	abort_interp("Non-string argument.");
    }
    if (!oper2->data.string || !*(oper2->data.string->data)) {
	abort_interp("Key cannot be a null string. (2)");
    }
    ptr = strencrypt(DoNullInd(oper1->data.string), oper2->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(ptr);
}

void
prim_strdecrypt(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING) {
	abort_interp("Non-string argument.");
    }
    if (!oper2->data.string || !*(oper2->data.string->data)) {
	abort_interp("Key cannot be a null string. (2)");
    }
    ptr = strdecrypt(DoNullInd(oper1->data.string), oper2->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(ptr);
}

void
prim_textattr(PRIM_PROTOTYPE)
{
    int totallen;
    int done = 0;
    const char *ptr;
    char *ptr2;
    char buf[BUFFER_LEN];
    char attr[BUFFER_LEN];

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING) {
	abort_interp("Non-string argument. (1)");
    }
    if (oper2->type != PROG_STRING) {
	abort_interp("Non-string argument. (2)");
    }

    if (!oper1->data.string || !oper2->data.string) {
	strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));
    } else {
	*buf = '\0';
	ptr = oper2->data.string->data;
	ptr2 = attr;
	while (!done) {
	    switch (*ptr) {
	    case ' ':{
		    ptr++;
		    break;
		}

	    case '\0':
	    case ',':{
		    *ptr2++ = '\0';
		    if (!strcasecmp(attr, "reset")) {
			strcatn(buf, sizeof(buf), ANSI_RESET);
		    } else if (!strcasecmp(attr, "normal")) {
			strcatn(buf, sizeof(buf), ANSI_RESET);
		    } else if (!strcasecmp(attr, "bold")) {
			strcatn(buf, sizeof(buf), ANSI_BOLD);
		    } else if (!strcasecmp(attr, "dim")) {
			strcatn(buf, sizeof(buf), ANSI_DIM);
		    } else if (!strcasecmp(attr, "italic")) {
			strcatn(buf, sizeof(buf), ANSI_ITALIC);
		    } else if (!strcasecmp(attr, "uline") ||
			       !strcasecmp(attr, "underline")) {
			strcatn(buf, sizeof(buf), ANSI_UNDERLINE);
		    } else if (!strcasecmp(attr, "flash")) {
			strcatn(buf, sizeof(buf), ANSI_FLASH);
		    } else if (!strcasecmp(attr, "reverse")) {
			strcatn(buf, sizeof(buf), ANSI_REVERSE);
		    } else if (!strcasecmp(attr, "ostrike")) {
			strcatn(buf, sizeof(buf), ANSI_OSTRIKE);
		    } else if (!strcasecmp(attr, "overstrike")) {
			strcatn(buf, sizeof(buf), ANSI_OSTRIKE);

		    } else if (!strcasecmp(attr, "black")) {
			strcatn(buf, sizeof(buf), ANSI_FG_BLACK);
		    } else if (!strcasecmp(attr, "red")) {
			strcatn(buf, sizeof(buf), ANSI_FG_RED);
		    } else if (!strcasecmp(attr, "yellow")) {
			strcatn(buf, sizeof(buf), ANSI_FG_YELLOW);
		    } else if (!strcasecmp(attr, "green")) {
			strcatn(buf, sizeof(buf), ANSI_FG_GREEN);
		    } else if (!strcasecmp(attr, "cyan")) {
			strcatn(buf, sizeof(buf), ANSI_FG_CYAN);
		    } else if (!strcasecmp(attr, "blue")) {
			strcatn(buf, sizeof(buf), ANSI_FG_BLUE);
		    } else if (!strcasecmp(attr, "magenta")) {
			strcatn(buf, sizeof(buf), ANSI_FG_MAGENTA);
		    } else if (!strcasecmp(attr, "white")) {
			strcatn(buf, sizeof(buf), ANSI_FG_WHITE);

		    } else if (!strcasecmp(attr, "bg_black")) {
			strcatn(buf, sizeof(buf), ANSI_BG_BLACK);
		    } else if (!strcasecmp(attr, "bg_red")) {
			strcatn(buf, sizeof(buf), ANSI_BG_RED);
		    } else if (!strcasecmp(attr, "bg_yellow")) {
			strcatn(buf, sizeof(buf), ANSI_BG_YELLOW);
		    } else if (!strcasecmp(attr, "bg_green")) {
			strcatn(buf, sizeof(buf), ANSI_BG_GREEN);
		    } else if (!strcasecmp(attr, "bg_cyan")) {
			strcatn(buf, sizeof(buf), ANSI_BG_CYAN);
		    } else if (!strcasecmp(attr, "bg_blue")) {
			strcatn(buf, sizeof(buf), ANSI_BG_BLUE);
		    } else if (!strcasecmp(attr, "bg_magenta")) {
			strcatn(buf, sizeof(buf), ANSI_BG_MAGENTA);
		    } else if (!strcasecmp(attr, "bg_white")) {
			strcatn(buf, sizeof(buf), ANSI_BG_WHITE);
		    } else {
			abort_interp
				("Unrecognized attribute tag.  Try one of reset, bold, dim, italic, underline, flash, reverse, overstrike, black, red, green, yellow, blue, magenta, cyan, white, bg_black, bg_red, bg_green, bg_yellow, bg_blue, bg_magenta, bg_cyan, or bg_white.");
		    }

		    ptr2 = attr;
		    if (!*ptr) {
			done++;
		    } else {
			ptr++;
		    }
		    break;
		}

	    default:{
		    *ptr2++ = *ptr++;
		}
	    }
	}
	totallen = strlen(buf);
	totallen += oper1->data.string->length;
	totallen += strlen(ANSI_RESET);
	if (totallen >= BUFFER_LEN) {
	    abort_interp("Operation would result in too long of a string.");
	}
	strcatn(buf, sizeof(buf), oper1->data.string->data);
    }
    strcatn(buf, sizeof(buf), ANSI_RESET);

    CLEAR(oper1);
    CLEAR(oper2);
    PushString(buf);
}

void
prim_tokensplit(PRIM_PROTOTYPE)
{				/* string delims escapechar -- prestr poststr charstr */
    char *ptr, *delim, *out;
    char esc;
    char escisdel;
    char outbuf[BUFFER_LEN];

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Not a string argument. (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Not a string argument. (3)");
    if (!oper2->data.string || oper2->data.string->length < 1)
	abort_interp("Invalid null delimiter string. (2)");
    if (oper3->data.string) {
	esc = oper3->data.string->data[0];
    } else {
	esc = '\0';
    }
    escisdel = strchr(oper2->data.string->data, esc) != 0;
    strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));
    ptr = buf;
    out = outbuf;
    while (*ptr) {
	if ((*ptr == esc) && (!escisdel || (ptr[1] == esc))) {
	    ptr++;
	    if (!*ptr)
		break;
	} else {
	    delim = oper2->data.string->data;
	    while (*delim) {
		if (*delim == *ptr)
		    break;
		delim++;
	    }
	    if (*delim == *ptr)
		break;
	}
	*out++ = *ptr++;
    }
    *out = '\0';
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    if (ptr && *ptr) {
	char charbuf[2];

	charbuf[0] = *ptr;
	charbuf[1] = '\0';
	*ptr++ = '\0';
	PushString(outbuf);
	PushString(ptr);
	PushString(charbuf);
    } else {
	PushString(outbuf);
	PushNullStr;
	PushNullStr;
    }
}

void
prim_ansi_strlen(PRIM_PROTOTYPE)
{
    char *ptr;
    int i;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");

    if (!oper1->data.string) {
	CLEAR(oper1);
	i = 0;
	PushInt(i);
	/* Weird PushInt() #define requires that. */
	return;
    }

    i = 0;

    ptr = oper1->data.string->data;

    while (*ptr) {
	if (*ptr++ == ESCAPE_CHAR) {
	    if (*ptr == '\0') {;
	    } else if (*ptr != '[') {
		ptr++;
	    } else {
		ptr++;
		while (isdigit(*ptr) || *ptr == ';')
		    ptr++;
		if (*ptr == 'm')
		    ptr++;
	    }
	} else {
	    i++;
	}
    }
    CLEAR(oper1);
    PushInt(i);
}

void
prim_ansi_strcut(PRIM_PROTOTYPE)
{
    char *ptr;
    char *op;
    char outbuf1[BUFFER_LEN];
    char outbuf2[BUFFER_LEN];
    int loc;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument. (1)");
    if (oper2->type != PROG_INTEGER)
	abort_interp("Not an integer argument. (2)");
    if (!oper1->data.string) {
	CLEAR(oper1);
	CLEAR(oper2);
	PushNullStr;
	PushNullStr;
	return;
    }

    loc = 0;

    if ((size_t)oper2->data.number >= oper1->data.string->length) {
	strcpyn(buf, sizeof(buf), oper1->data.string->data);
	CLEAR(oper1);
	CLEAR(oper2);
	PushString(buf);
	PushNullStr;
	return;
    } else if (oper2->data.number <= 0) {
	strcpyn(buf, sizeof(buf), oper1->data.string->data);
	CLEAR(oper1);
	CLEAR(oper2);
	PushNullStr;
	PushString(buf);
	return;
    }

    ptr = oper1->data.string->data;

    *outbuf2 = '\0';
    op = outbuf1;
    while (*ptr) {
	if ((*op++ = *ptr++) == ESCAPE_CHAR) {
	    if (*ptr == '\0') {
		break;
	    } else if (*ptr != '[') {
		*op++ = *ptr++;
	    } else {
		*op++ = *ptr++;
		while (isdigit(*ptr) || *ptr == ';')
		    *op++ = *ptr++;
		if (*ptr == 'm')
		    *op++ = *ptr++;
	    }
	} else {
	    loc++;
	    if (loc == oper2->data.number) {
		break;
	    }
	}
    }
    *op = '\0';
    memcpy((void *) outbuf2, (const void *) ptr,
	   oper1->data.string->length - (size_t)(ptr - oper1->data.string->data) + 1);

    CLEAR(oper1);
    CLEAR(oper2);
    PushString(outbuf1);
    if (!*outbuf2) {
	PushNullStr;
    } else {
	PushString(outbuf2);
    }
}

void
prim_ansi_strip(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument.");

    if (!oper1->data.string) {
	CLEAR(oper1);
	PushNullStr;
	return;
    }

    strip_ansi(buf, oper1->data.string->data);
    CLEAR(oper1);
    PushString(buf);
}

void
prim_ansi_midstr(PRIM_PROTOTYPE)
{
    int loc, start, range;
    const char *ptr;
    char *op;

    CHECKOP(3);
    oper1 = POP();		/* length */
    oper2 = POP();		/* begin */
    oper3 = POP();		/* string */

    if (oper3->type != PROG_STRING)
	abort_interp("Non-string argument! (3)");
    if (oper2->type != PROG_INTEGER)
	abort_interp("Non-integer argument! (2)");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument! (1)");
    if (oper2->data.number < 1)
	abort_interp("Data must be a positve integer. (2)");
    if (oper1->data.number < 0)
	abort_interp("Data must be a postive integer. (1)");

    start = oper2->data.number - 1;
    range = oper1->data.number;

    if (!oper3->data.string || (size_t)start > oper3->data.string->length || range == 0) {
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	PushNullStr;
	return;
    }

    ptr = oper3->data.string->data;
    op = buf;
    loc = 0;

    if (start != 0) {
	/* First, loop till the beginning of the section to copy... */
	while (*ptr && loc < start) {
	    if ((*ptr++) == ESCAPE_CHAR) {
		if (*ptr == '\0') {
		    break;
		} else if (*ptr != '[') {
		    ptr++;
		} else {
		    ptr++;
		    while (isdigit(*ptr) || *ptr == ';')
			ptr++;
		    if (*ptr == 'm')
			ptr++;
		}
	    } else {
		loc++;
	    }
	}
    }

    loc = 0;
    /* Then, start copying, and counting while we do... */
    while (*ptr) {
	if ((*op++ = *ptr++) == ESCAPE_CHAR) {
	    if (*ptr == '\0') {
		break;
	    } else if (*ptr != '[') {
		*op++ = *ptr++;
	    } else {
		*op++ = *ptr++;
		while (isdigit(*ptr) || *ptr == ';')
		    *op++ = *ptr++;
		if (*ptr == 'm')
		    *op++ = *ptr++;
	    }
	} else {
	    loc++;
	    if (loc == range) {
		break;
	    }
	}
    }
    *op = '\0';

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushString(buf);
}

void
prim_notify_secure(PRIM_PROTOTYPE)
{
    int *darr;
    int dcount;

    if (mlev < 3)
	abort_interp("Mucker level 3 primitive.");

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Not a string argument. (2)");
    if (!valid_player(oper3))
	abort_interp("Not a valid player. (3)");

    ref = oper3->data.objref;

    darr = get_player_descrs(ref, &dcount);

    for (int di = 0; di < dcount; di++) {
	if (pdescrsecure(darr[di])) {
	    pdescrnotify(darr[di], oper2->data.string->data);
	} else {
	    pdescrnotify(darr[di], oper1->data.string->data);
	}
    }

    if (tp_allow_listeners && tp_allow_listeners_obj) {
	listenqueue(-1, player, LOCATION(ref), ref, ref, program, LISTEN_PROPQUEUE,
		    oper1->data.string->data, tp_listen_mlev, 1, 0);
	listenqueue(-1, player, LOCATION(ref), ref, ref, program, WLISTEN_PROPQUEUE,
		    oper1->data.string->data, tp_listen_mlev, 1, 1);
	listenqueue(-1, player, LOCATION(ref), ref, ref, program, WOLISTEN_PROPQUEUE,
		    oper1->data.string->data, tp_listen_mlev, 0, 1);
    }


    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_md5hash(PRIM_PROTOTYPE)
{
    char hash[33];
    const char *p;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument. (1)");

    if (!oper1->data.string) {
	MD5hex(hash, "", 0);
    } else {
	MD5hex(hash, oper1->data.string->data, strlen(oper1->data.string->data));
    }

    p = hash;

    CLEAR(oper1);
    PushString(p);
}

void
prim_sha1hash(PRIM_PROTOTYPE)
{
    char hash[41];
    const char *p;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument. (1)");

    p = DoNullInd(oper1->data.string);

    sha1nfo info;
    sha1_init(&info);
    while (*p) {
	sha1_writebyte(&info, (uint8_t)*p);
	p++;
    }

    hash2hex(sha1_result(&info), hash, sizeof(hash));

    p = hash;
    CLEAR(oper1);
    PushString(p);
}

void
prim_strip(PRIM_PROTOTYPE)
{				/* string -- string' */
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");
    strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));
    pname = buf;
    skip_whitespace((const char **)&pname);
    remove_ending_whitespace(&pname);
    CLEAR(oper1);
    PushString(pname);
}

void
prim_pose_separatorp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
	abort_interp("Not a string argument.");

    result = is_valid_pose_separator((DoNullInd(oper1->data.string))[0]);

    CLEAR(oper1);
    PushInt(result);
}
