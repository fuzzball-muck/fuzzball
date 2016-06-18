#include "config.h"

#include "db.h"
#include "externs.h"
#include "interp.h"
#include "inst.h"

void
disassemble(dbref player, dbref program)
{
    struct inst *curr;
    struct inst *codestart;
    char buf[BUFFER_LEN];

    codestart = curr = PROGRAM_CODE(program);
    if (!PROGRAM_SIZ(program)) {
	notify(player, "Nothing to disassemble!");
	return;
    }
    for (int i = 0; i < PROGRAM_SIZ(program); i++, curr++) {
	switch (curr->type) {
	case PROG_PRIMITIVE:
	    if (curr->data.number >= BASE_MIN && curr->data.number <= BASE_MAX)
		snprintf(buf, sizeof(buf), "%d: (line %d) PRIMITIVE: %s", i,
			 curr->line, base_inst[curr->data.number - BASE_MIN]);
	    else
		snprintf(buf, sizeof(buf), "%d: (line %d) PRIMITIVE: %d", i, curr->line,
			 curr->data.number);
	    break;
	case PROG_MARK:
	    snprintf(buf, sizeof(buf), "%d: (line %d) MARK", i, curr->line);
	    break;
	case PROG_STRING:
	    snprintf(buf, sizeof(buf), "%d: (line %d) STRING: \"%s\"", i, curr->line,
		     DoNullInd(curr->data.string));
	    break;
	case PROG_ARRAY:
	    snprintf(buf, sizeof(buf), "%d: (line %d) ARRAY: %d items", i, curr->line,
		     curr->data.array ? curr->data.array->items : 0);
	    break;
	case PROG_FUNCTION:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FUNCTION: %s, VARS: %d, ARGS: %d", i,
		     curr->line, DoNull(curr->data.mufproc->procname),
		     curr->data.mufproc->vars, curr->data.mufproc->args);
	    break;
	case PROG_LOCK:
	    snprintf(buf, sizeof(buf), "%d: (line %d) LOCK: [%s]", i, curr->line,
		     curr->data.lock == TRUE_BOOLEXP ? "TRUE_BOOLEXP" :
		     unparse_boolexp(0, curr->data.lock, 0));
	    break;
	case PROG_INTEGER:
	    snprintf(buf, sizeof(buf), "%d: (line %d) INTEGER: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_FLOAT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FLOAT: %.17g", i, curr->line,
		     curr->data.fnumber);
	    break;
	case PROG_ADD:
	    snprintf(buf, sizeof(buf), "%d: (line %d) ADDRESS: %d", i,
		     curr->line, (int) (curr->data.addr->data - codestart));
	    break;
	case PROG_TRY:
	    snprintf(buf, sizeof(buf), "%d: (line %d) TRY: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_IF:
	    snprintf(buf, sizeof(buf), "%d: (line %d) IF: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_JMP:
	    snprintf(buf, sizeof(buf), "%d: (line %d) JMP: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_EXEC:
	    snprintf(buf, sizeof(buf), "%d: (line %d) EXEC: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_OBJECT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) OBJECT REF: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_VAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) VARIABLE: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_SVAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) SCOPEDVAR: %d (%s)", i, curr->line,
		     curr->data.number, scopedvar_getname_byinst(curr, curr->data.number));
	    break;
	case PROG_SVAR_AT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH SCOPEDVAR: %d (%s)", i, curr->line,
		     curr->data.number, scopedvar_getname_byinst(curr, curr->data.number));
	    break;
	case PROG_SVAR_AT_CLEAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH SCOPEDVAR (clear optim): %d (%s)",
		     i, curr->line, curr->data.number, scopedvar_getname_byinst(curr,
										curr->data.
										number));
	    break;
	case PROG_SVAR_BANG:
	    snprintf(buf, sizeof(buf), "%d: (line %d) SET SCOPEDVAR: %d (%s)", i, curr->line,
		     curr->data.number, scopedvar_getname_byinst(curr, curr->data.number));
	    break;
	case PROG_LVAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) LOCALVAR: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_LVAR_AT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH LOCALVAR: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_LVAR_AT_CLEAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH LOCALVAR (clear optim): %d", i,
		     curr->line, curr->data.number);
	    break;
	case PROG_LVAR_BANG:
	    snprintf(buf, sizeof(buf), "%d: (line %d) SET LOCALVAR: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_CLEARED:
	    snprintf(buf, sizeof(buf), "%d: (line ?) CLEARED INST AT %s:%d", i,
		     (char *) curr->data.addr, curr->line);
	default:
	    snprintf(buf, sizeof(buf), "%d: (line ?) UNKNOWN INST", i);
	}
	notify(player, buf);
    }
}
