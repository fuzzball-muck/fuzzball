#include "config.h"

#include "boolexp.h"
#include "db.h"
#include "fbstrings.h"
#include "inst.h"
#include "interp.h"
#include "tune.h"

#undef DEBUGARRAYS

extern const char *base_inst[];

/* converts an instruction into a printable string, stores the string in
   buffer and returns a pointer to it.
   the first byte of the return value will be NULL if a buffer overflow
   would have occured.
 */
char *
insttotext(struct frame *fr, int lev, struct inst *theinst, char *buffer, int buflen,
	   int strmax, dbref program, int expandarrs)
{
    const char *ptr;
    char buf2[BUFFER_LEN];
    struct inst temp1;
    struct inst *oper2;
    int length = -2;		/* unset mark. We don't use -1 since some snprintf() version will
				   return that if we would've overflowed. */
    int firstflag = 1;
    int arrcount = 0;

    assert(buflen > 0);

    strmax = (strmax > buflen - 3) ? buflen - 3 : strmax;

    switch (theinst->type) {
    case PROG_PRIMITIVE:
	if (theinst->data.number >= 1 && theinst->data.number <= prim_count) {
	    ptr = base_inst[theinst->data.number - 1];
	    if (strlen(ptr) >= (size_t) buflen)
		*buffer = '\0';
	    else
		strcpyn(buffer, buflen, ptr);
	} else {
	    if (buflen > 3)
		strcpyn(buffer, buflen, "???");
	    else
		*buffer = '\0';
	}
	break;
    case PROG_STRING:
	if (!theinst->data.string) {
	    if (buflen > 2)
		strcpyn(buffer, buflen, "\"\"");
	    else
		*buffer = '\0';
	    break;
	}
	if (strmax <= 0) {
	    *buffer = '\0';
	    break;
	}
	/* we know we won't overflow, so don't set length */
	snprintf(buffer, buflen, "\"%1.*s\"", (strmax - 1), theinst->data.string->data);
	if (theinst->data.string->length > strmax)
	    strcatn(buffer, buflen, "_");
	break;
    case PROG_MARK:
	if (buflen > 4)
	    strcpyn(buffer, buflen, "MARK");
	else
	    *buffer = '\0';
	break;
    case PROG_ARRAY:
	if (!theinst->data.array) {
	    if (buflen > 3)
		strcpyn(buffer, buflen, "0{}");
	    else
		*buffer = '\0';
	    break;
	}
	if (tp_expanded_debug && expandarrs) {
#ifdef DEBUGARRAYS
	    length = snprintf(buffer, buflen, "R%dC%d{", theinst->data.array->links,
			      theinst->data.array->items);
#else
	    length = snprintf(buffer, buflen, "%d{", theinst->data.array->items);
#endif
	    if (length >= buflen || length == -1) {
		/* >= because we need room for one more charctor at the end */
		*buffer = '\0';
		break;
	    }

	    /* - 1 for the "\0" at the end. */
	    length = buflen - length - 1;
	    firstflag = 1;
	    arrcount = 0;
	    if (array_first(theinst->data.array, &temp1)) {
		do {
		    char *inststr;

		    if (arrcount++ >= 8) {
			strcatn(buffer, buflen, "_");
			break;
		    }

		    if (!firstflag) {
			strcatn(buffer, buflen, " ");
			length--;
		    }
		    firstflag = 0;
		    oper2 = array_getitem(theinst->data.array, &temp1);

		    if (length <= 2) {	/* no space left, let's not pass a buflen of 0 */
			strcatn(buffer, buflen, "_");
			break;
		    }

		    /* length - 2 so we have room for the ":_" */
		    inststr =
			    insttotext(fr, lev, &temp1, buf2, length - 2, strmax, program, 0);
		    if (!*inststr) {
			/* overflow problem. */
			strcatn(buffer, buflen, "_");
			break;
		    }
		    length -= strlen(inststr) + 1;
		    strcatn(buffer, buflen, inststr);
		    strcatn(buffer, buflen, ":");

		    if (length <= 2) {	/* no space left, let's not pass a buflen of 0 */
			strcatn(buffer, buflen, "_");
			break;
		    }

		    inststr = insttotext(fr, lev, oper2, buf2, length, strmax, program, 0);
		    if (!*inststr) {
			/* we'd overflow if we did that */
			/* as before add a "_" and let it be. */
			strcatn(buffer, buflen, "_");
			break;
		    }
		    length -= strlen(inststr);
		    strcatn(buffer, buflen, inststr);

		    if (length < 2) {
			/* we should have a length of exactly 1, if we get here.
			 * So we just have enough room for a '_' now.
			 * Just append the "_" and stop this madness. */
			strcatn(buffer, buflen, "_");
			length--;
			break;
		    }
		} while (array_next(theinst->data.array, &temp1));
	    }
	    strcatn(buffer, buflen, "}");
	} else {
	    length = snprintf(buffer, buflen, "%d{...}", theinst->data.array->items);
	}
	break;
    case PROG_INTEGER:
	length = snprintf(buffer, buflen, "%d", theinst->data.number);
	break;
    case PROG_FLOAT:
	length = snprintf(buffer, buflen, "%.16g", theinst->data.fnumber);
	if (!strchr(buffer, '.') && !strchr(buffer, 'n') && !strchr(buffer, 'e')) {
	    strcatn(buffer, buflen, ".0");
	}
	break;
    case PROG_ADD:
	if (theinst->data.addr->data->type == PROG_FUNCTION &&
	    theinst->data.addr->data->data.mufproc != NULL) {
	    if (theinst->data.addr->progref != program)
		length = snprintf(buffer, buflen, "'#%d'%s", theinst->data.addr->progref,
				  theinst->data.addr->data->data.mufproc->procname);
	    else
		length = snprintf(buffer, buflen, "'%s",
				  theinst->data.addr->data->data.mufproc->procname);
	} else {
	    if (theinst->data.addr->progref != program)
		length = snprintf(buffer, buflen, "'#%d'line%d?", theinst->data.addr->progref,
				  theinst->data.addr->data->line);
	    else
		length = snprintf(buffer, buflen, "'line%d?", theinst->data.addr->data->line);
	}
	break;
    case PROG_TRY:
	length = snprintf(buffer, buflen, "TRY->line%d", theinst->data.call->line);
	break;
    case PROG_IF:
	length = snprintf(buffer, buflen, "IF->line%d", theinst->data.call->line);
	break;
    case PROG_EXEC:
	if (theinst->data.call->type == PROG_FUNCTION) {
	    length = snprintf(buffer, buflen, "EXEC->%s",
			      theinst->data.call->data.mufproc->procname);
	} else {
	    length = snprintf(buffer, buflen, "EXEC->line%d", theinst->data.call->line);
	}
	break;
    case PROG_JMP:
	if (theinst->data.call->type == PROG_FUNCTION) {
	    length = snprintf(buffer, buflen, "JMP->%s",
			      theinst->data.call->data.mufproc->procname);
	} else {
	    length = snprintf(buffer, buflen, "JMP->line%d", theinst->data.call->line);
	}
	break;
    case PROG_OBJECT:
	length = snprintf(buffer, buflen, "#%d", theinst->data.objref);
	break;
    case PROG_VAR:
	length = snprintf(buffer, buflen, "V%d", theinst->data.number);
	break;
    case PROG_SVAR:
	if (fr) {
	    length = snprintf(buffer, buflen, "SV%d:%s", theinst->data.number,
			      scopedvar_getname(fr, lev, theinst->data.number));
	} else {
	    length = snprintf(buffer, buflen, "SV%d", theinst->data.number);
	}
	break;
    case PROG_SVAR_AT:
    case PROG_SVAR_AT_CLEAR:
	if (fr) {
	    length = snprintf(buffer, buflen, "SV%d:%s @", theinst->data.number,
			      scopedvar_getname(fr, lev, theinst->data.number));
	} else {
	    length = snprintf(buffer, buflen, "SV%d @", theinst->data.number);
	}
	break;
    case PROG_SVAR_BANG:
	if (fr) {
	    length = snprintf(buffer, buflen, "SV%d:%s !", theinst->data.number,
			      scopedvar_getname(fr, lev, theinst->data.number));
	} else {
	    length = snprintf(buffer, buflen, "SV%d !", theinst->data.number);
	}
	break;
    case PROG_LVAR:
	length = snprintf(buffer, buflen, "LV%d", theinst->data.number);
	break;
    case PROG_LVAR_AT:
    case PROG_LVAR_AT_CLEAR:
	length = snprintf(buffer, buflen, "LV%d @", theinst->data.number);
	break;
    case PROG_LVAR_BANG:
	length = snprintf(buffer, buflen, "LV%d !", theinst->data.number);
	break;
    case PROG_FUNCTION:
	length = snprintf(buffer, buflen, "INIT FUNC: %s (%d arg%s)",
			  theinst->data.mufproc->procname,
			  theinst->data.mufproc->args,
			  theinst->data.mufproc->args == 1 ? "" : "s");
	break;
    case PROG_LOCK:
	if (theinst->data.lock == TRUE_BOOLEXP) {
	    /*                  12345678901234 */
	    /* 14 */
	    if (buflen > 14)
		strcpyn(buffer, buflen, "[TRUE_BOOLEXP]");
	    else
		*buffer = '\0';
	    break;
	}
	length = snprintf(buffer, buflen, "[%1.*s]",
			  (strmax - 1), unparse_boolexp(0, theinst->data.lock, 0));
	break;
    case PROG_CLEARED:
	length = snprintf(buffer, buflen, "?<%s:%d>", (char *) theinst->data.addr,
			  theinst->line);
	break;
    default:
	if (buflen > 3)
	    strcpyn(buffer, buflen, "?");
	else
	    *buffer = '\0';
	break;
    }

    if (length == -1 || length > buflen)
	*buffer = '\0';
    return buffer;
}

