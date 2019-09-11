#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "db.h"
#include "fbstrings.h"
#include "inst.h"
#include "interp.h"
#include "mcp.h"
#include "mcpgui.h"
#include "mufevent.h"
#include "timequeue.h"
#include "tune.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int result;

struct mcp_muf_context {
    dbref prog;
};

struct mcpevent_context {
    int pid;
};

/* This allows using GUI if the player owns the MUF, which drops the
   requirement from an M3. This is not .top because .top will usually 
   be $lib/gui. This could be expanded to check the entire MUF chain. */
#define CHECKMCPPERM() \
  if ((mlev < tp_mcp_muf_mlev) && !(player == OWNER(fr->caller.st[1]))) \
	  abort_interp("Permission denied!!!");

static void
muf_mcp_context_cleanup(void *context)
{
    struct mcp_muf_context *mmc = (struct mcp_muf_context *) context;

    free(mmc);
}

static void
muf_mcp_callback(McpFrame * mfr, McpMesg * mesg, McpVer version, void *context)
{
    struct mcp_muf_context *mmc = (struct mcp_muf_context *) context;
    dbref obj = mmc->prog;
    struct mcp_binding *ptr;
    char *pkgname = mesg->package;
    char *msgname = mesg->mesgname;
    dbref user;
    int descr;

    descr = MCPFRAME_DESCR(mfr);
    user = MCPFRAME_PLAYER(mfr);

    for (ptr = PROGRAM_MCPBINDS(obj); ptr; ptr = ptr->next) {
	if (ptr->pkgname && ptr->msgname) {
	    if (!strcasecmp(ptr->pkgname, pkgname)) {
		if (!strcasecmp(ptr->msgname, msgname)) {
		    break;
		}
	    }
	}
    }

    if (ptr) {
	struct frame *tmpfr;
	stk_array *argarr;
	struct inst argname, argval, argpart;

	tmpfr = interp(descr, user, LOCATION(user), obj, -1, PREEMPT, STD_REGUID, 0);
	if (tmpfr) {
	    oper1 = tmpfr->argument.st + --tmpfr->argument.top;
	    CLEAR(oper1);
	    argarr = new_array_dictionary(tmpfr->pinning);
	    for (McpArg *arg = mesg->args; arg; arg = arg->next) {
		if (!arg->value) {
		    argval.type = PROG_STRING;
		    argval.data.string = NULL;
		} else if (!arg->value->next) {
		    argval.type = PROG_STRING;
		    argval.data.string = alloc_prog_string(arg->value->value);
		} else {
		    int count = 0;

		    argname.type = PROG_INTEGER;
		    argval.type = PROG_ARRAY;
		    argval.data.array = new_array_packed(
			mcp_mesg_arg_linecount(mesg, arg->name),
			tmpfr->pinning
		    );

		    for (McpArgPart *partptr = arg->value; partptr; partptr = partptr->next) {
			argname.data.number = count++;
			argpart.type = PROG_STRING;
			argpart.data.string = alloc_prog_string(partptr->value);
			array_setitem(&argval.data.array, &argname, &argpart);
			CLEAR(&argpart);
		    }
		}
		argname.type = PROG_STRING;
		argname.data.string = alloc_prog_string(arg->name);
		array_setitem(&argarr, &argname, &argval);
		CLEAR(&argname);
		CLEAR(&argval);
	    }
	    push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_INTEGER, &descr);
	    push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_ARRAY, argarr);
	    tmpfr->pc = ptr->addr;
	    interp_loop(user, obj, tmpfr, 0);
	}
    }
}

static void
mcpevent_context_cleanup(void *context)
{
    struct mcpevent_context *mec = (struct mcpevent_context *) context;

    free(mec);
}

static void
muf_mcp_event_callback(McpFrame * mfr, McpMesg * mesg, McpVer version, void *context)
{
    struct mcpevent_context *mec = (struct mcpevent_context *) context;
    int destpid = mec->pid;
    struct frame *destfr = timequeue_pid_frame(destpid);
    int descr = MCPFRAME_DESCR(mfr);
    char *pkgname = mesg->package;
    char *msgname = mesg->mesgname;

    if (destfr) {
	char buf[BUFFER_LEN];
	stk_array *argarr;
	stk_array *contarr;
	struct inst argname, argval, argpart;

	argarr = new_array_dictionary(destfr->pinning);
	for (McpArg *arg = mesg->args; arg; arg = arg->next) {
	    if (!arg->value) {
		argval.type = PROG_ARRAY;
		argval.data.array = NULL;
	    } else {
		int count = 0;

		argname.type = PROG_INTEGER;
		argval.type = PROG_ARRAY;
		argval.data.array = new_array_packed(
		    mcp_mesg_arg_linecount(mesg, arg->name),
		    destfr->pinning
		);

		for (McpArgPart *partptr = arg->value; partptr; partptr = partptr->next) {
		    argname.data.number = count++;
		    argpart.type = PROG_STRING;
		    argpart.data.string = alloc_prog_string(partptr->value);
		    array_setitem(&argval.data.array, &argname, &argpart);
		    CLEAR(&argpart);
		}
	    }
	    argname.type = PROG_STRING;
	    argname.data.string = alloc_prog_string(arg->name);
	    array_setitem(&argarr, &argname, &argval);
	    CLEAR(&argname);
	    CLEAR(&argval);
	}

	contarr = new_array_dictionary(destfr->pinning);
	array_set_strkey_intval(&contarr, "descr", descr);
	array_set_strkey_strval(&contarr, "package", pkgname);
	array_set_strkey_strval(&contarr, "message", msgname);

	argname.type = PROG_STRING;
	argname.data.string = alloc_prog_string("args");
	argval.type = PROG_ARRAY;
	argval.data.array = argarr;
	array_setitem(&contarr, &argname, &argval);
	CLEAR(&argname);
	CLEAR(&argval);

	argval.type = PROG_ARRAY;
	argval.data.array = contarr;
	if (msgname && *msgname) {
	    snprintf(buf, sizeof(buf), "MCP.%.128s-%.128s", pkgname, msgname);
	} else {
	    snprintf(buf, sizeof(buf), "MCP.%.128s", pkgname);
	}
	muf_event_add(destfr, buf, &argval, 0);
	CLEAR(&argval);
    }
}

static int
stuff_dict_in_mesg(stk_array * arr, McpMesg * msg)
{
    struct inst argname, *argval;
    char buf[64];
    int result;

    result = array_first(arr, &argname);
    while (result) {
	if (argname.type != PROG_STRING) {
	    CLEAR(&argname);
	    return -1;
	}
	if (!argname.data.string || !*argname.data.string->data) {
	    CLEAR(&argname);
	    return -2;
	}
	argval = array_getitem(arr, &argname);
	switch (argval->type) {
	case PROG_ARRAY:{
		struct inst subname, *subval;
		int contd = array_first(argval->data.array, &subname);

		mcp_mesg_arg_remove(msg, argname.data.string->data);
		while (contd) {
		    subval = array_getitem(argval->data.array, &subname);
		    switch (subval->type) {
		    case PROG_STRING:
			mcp_mesg_arg_append(msg, argname.data.string->data,
					    DoNullInd(subval->data.string));
			break;
		    case PROG_INTEGER:
			snprintf(buf, sizeof(buf), "%d", subval->data.number);
			mcp_mesg_arg_append(msg, argname.data.string->data, buf);
			break;
		    case PROG_OBJECT:
			snprintf(buf, sizeof(buf), "#%d", subval->data.number);
			mcp_mesg_arg_append(msg, argname.data.string->data, buf);
			break;
		    case PROG_FLOAT:
			snprintf(buf, sizeof(buf), "%.15g", subval->data.fnumber);
			mcp_mesg_arg_append(msg, argname.data.string->data, buf);
			break;
		    default:
			CLEAR(&argname);
			return -3;
		    }
		    contd = array_next(argval->data.array, &subname);
		}
		break;
	    }
	case PROG_STRING:
	    mcp_mesg_arg_remove(msg, argname.data.string->data);
	    mcp_mesg_arg_append(msg, argname.data.string->data,
				DoNullInd(argval->data.string));
	    break;
	case PROG_INTEGER:
	    snprintf(buf, sizeof(buf), "%d", argval->data.number);
	    mcp_mesg_arg_remove(msg, argname.data.string->data);
	    mcp_mesg_arg_append(msg, argname.data.string->data, buf);
	    break;
	case PROG_OBJECT:
	    snprintf(buf, sizeof(buf), "#%d", argval->data.number);
	    mcp_mesg_arg_remove(msg, argname.data.string->data);
	    mcp_mesg_arg_append(msg, argname.data.string->data, buf);
	    break;
	case PROG_FLOAT:
	    snprintf(buf, sizeof(buf), "%.15g", argval->data.fnumber);
	    mcp_mesg_arg_remove(msg, argname.data.string->data);
	    mcp_mesg_arg_append(msg, argname.data.string->data, buf);
	    break;
	default:
	    CLEAR(&argname);
	    return -4;
	}
	result = array_next(arr, &argname);
    }
    return 0;
}

void
prim_mcp_register(PRIM_PROTOTYPE)
{
    char *pkgname;
    McpVer vermin, vermax;
    struct mcp_muf_context *mmc;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    /* oper1  oper2   oper3  */
    /* pkgstr minver  maxver */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("Package name string expected. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Floating point minimum version number expected. (2)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Floating point maximum version number expected. (3)");
    if (!oper1->data.string || !*oper1->data.string->data)
	abort_interp("Package name cannot be a null string. (1)");

    pkgname = oper1->data.string->data;
    vermin.vermajor = (int) oper2->data.fnumber;
    vermin.verminor = (int) (oper2->data.fnumber * 1000) % 1000;
    vermax.vermajor = (int) oper3->data.fnumber;
    vermax.verminor = (int) (oper3->data.fnumber * 1000) % 1000;

    mmc = malloc(sizeof(struct mcp_muf_context));
    mmc->prog = program;

    mcp_package_register(pkgname, vermin, vermax, muf_mcp_callback, (void *) mmc,
			 muf_mcp_context_cleanup);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_mcp_register_event(PRIM_PROTOTYPE)
{
    char *pkgname;
    McpVer vermin, vermax;
    struct mcpevent_context *mec;


    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    /* oper1  oper2   oper3  */
    /* pkgstr minver  maxver */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("Package name string expected. (1)");
    if (oper2->type != PROG_FLOAT)
	abort_interp("Floating point minimum version number expected. (2)");
    if (oper3->type != PROG_FLOAT)
	abort_interp("Floating point maximum version number expected. (3)");
    if (!oper1->data.string || !*oper1->data.string->data)
	abort_interp("Package name cannot be a null string. (1)");

    pkgname = oper1->data.string->data;
    vermin.vermajor = (int) oper2->data.fnumber;
    vermin.verminor = (int) (oper2->data.fnumber * 1000) % 1000;
    vermax.vermajor = (int) oper3->data.fnumber;
    vermax.verminor = (int) (oper3->data.fnumber * 1000) % 1000;

    mec = malloc(sizeof(struct mcpevent_context));
    mec->pid = fr->pid;

    mcp_package_register(pkgname, vermin, vermax, muf_mcp_event_callback, (void *) mec,
			 mcpevent_context_cleanup);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_mcp_supports(PRIM_PROTOTYPE)
{
    char *pkgname;
    int descr;
    McpVer ver;
    McpFrame *mfr;
    double fver;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    /* oper1 oper2  */
    /* descr pkgstr */

    if (oper1->type != PROG_INTEGER)
	abort_interp("Integer descriptor number expected. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Package name string expected. (2)");
    if (!oper2->data.string || !*oper2->data.string->data)
	abort_interp("Package name cannot be a null string. (2)");

    pkgname = oper2->data.string->data;
    descr = oper1->data.number;

    fver = 0.0;
    mfr = descr_mcpframe(descr);
    if (mfr) {
	ver = mcp_frame_package_supported(mfr, pkgname);
	fver = ver.vermajor + (ver.verminor / 1000.0);
    }

    CLEAR(oper1);
    CLEAR(oper2);

    PushFloat(fver);
}

void
prim_mcp_bind(PRIM_PROTOTYPE)
{
    char *pkgname, *msgname;
    struct mcp_binding *ptr;

    CHECKOP(3);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    /* oper1  oper2  oper3 */
    /* pkgstr msgstr address */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("Package name string expected. (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Message name string expected. (2)");
    if (oper3->type != PROG_ADD)
	abort_interp("Function address expected. (3)");
    if (!oper1->data.string || !*oper1->data.string->data)
	abort_interp("Package name cannot be a null string. (1)");
    if (!ObjExists(oper3->data.addr->progref) ||
	Typeof(oper3->data.addr->progref) != TYPE_PROGRAM) {
	abort_interp("Invalid address. (3)");
    }
    if (program != oper3->data.addr->progref) {
	abort_interp("Destination address outside current program. (3)");
    }

    pkgname = oper1->data.string->data;
    msgname = DoNullInd(oper2->data.string);

    for (ptr = PROGRAM_MCPBINDS(program); ptr; ptr = ptr->next) {
	if (ptr->pkgname && ptr->msgname) {
	    if (!strcasecmp(ptr->pkgname, pkgname)) {
		if (!strcasecmp(ptr->msgname, msgname)) {
		    break;
		}
	    }
	}
    }
    if (!ptr) {
	ptr = malloc(sizeof(struct mcp_binding));

	ptr->pkgname = strdup(pkgname);
	ptr->msgname = strdup(msgname);
	ptr->next = PROGRAM_MCPBINDS(program);
	PROGRAM_SET_MCPBINDS(program, ptr);
    }
    ptr->addr = oper3->data.addr->data;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_mcp_send(PRIM_PROTOTYPE)
{
    char *pkgname, *msgname;
    int descr;
    McpFrame *mfr;
    stk_array *arr;

    CHECKOP(4);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();
    oper4 = POP();

    /* oper4 oper1  oper2  oper3 */
    /* descr pkgstr msgstr argsarray */

    CHECKMCPPERM();
    if (oper4->type != PROG_INTEGER)
	abort_interp("Integer descriptor number expected. (1)");
    if (oper1->type != PROG_STRING)
	abort_interp("Package name string expected. (2)");
    if (!oper1->data.string || !*oper1->data.string->data)
	abort_interp("Package name cannot be a null string. (2)");
    if (oper2->type != PROG_STRING)
	abort_interp("Message name string expected. (3)");
    if (oper3->type != PROG_ARRAY)
	abort_interp("Arguments dictionary expected. (4)");

    pkgname = oper1->data.string->data;
    msgname = DoNullInd(oper2->data.string);
    arr = oper3->data.array;
    descr = oper4->data.number;

    mfr = descr_mcpframe(descr);
    if (mfr) {
	McpMesg msg;
	McpVer ver = mcp_frame_package_supported(mfr, pkgname);

	if (ver.verminor == 0 && ver.vermajor == 0)
	    abort_interp("MCP package not supported by that descriptor.");

	mcp_mesg_init(&msg, pkgname, msgname);

	result = stuff_dict_in_mesg(arr, &msg);
	if (result) {
	    mcp_mesg_clear(&msg);
	    switch (result) {
	    case -1:
		abort_interp("Args dictionary can only have string keys. (4)");
	    case -2:
		abort_interp("Args dictionary cannot have a null string key. (4)");
	    case -3:
		abort_interp("Unsupported value type in list value. (4)");
	    case -4:
		abort_interp("Unsupported value type in args dictionary. (4)");
	    }
	}

	mcp_frame_output_mesg(mfr, &msg);
	mcp_mesg_clear(&msg);
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

#ifdef MCPGUI_SUPPORT
static void
fbgui_muf_event_cb(GUI_EVENT_CB_ARGS)
{
    char buf[BUFFER_LEN];
    const char *name;
    struct frame *fr = (struct frame *) context;
    struct inst temp;
    struct inst temp1;
    struct inst temp2;
    stk_array *nu;
    int lines;

    nu = new_array_dictionary(fr->pinning);
    name = GuiValueFirst(dlogid);
    while (name) {
	lines = gui_value_linecount(dlogid, name);

        if (lines < 0)
            lines = 0;

	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string(name);

	temp2.type = PROG_ARRAY;
	temp2.data.array = new_array_packed(lines, fr->pinning);

	for (int i = 0; i < lines; i++) {
	    struct inst temp3;
	    array_data temp4;

	    temp3.type = PROG_INTEGER;
	    temp3.data.number = i;

	    temp4.type = PROG_STRING;
	    temp4.data.string = alloc_prog_string(gui_value_get(dlogid, name, i));

	    array_setitem(&temp2.data.array, &temp3, &temp4);
	    CLEAR(&temp3);
	    CLEAR(&temp4);
	}

	array_setitem(&nu, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);

	name = GuiValueNext(dlogid, name);
    }

    temp.type = PROG_ARRAY;
    temp.data.array = new_array_dictionary(fr->pinning);

    array_set_strkey_intval(&temp.data.array, "dismissed", did_dismiss);
    array_set_strkey_intval(&temp.data.array, "descr", descr);
    array_set_strkey_strval(&temp.data.array, "dlogid", dlogid);
    array_set_strkey_strval(&temp.data.array, "id", id);
    array_set_strkey_strval(&temp.data.array, "event", event);

    temp2.type = PROG_ARRAY;
    temp2.data.array = nu;
    array_set_strkey(&temp.data.array, "values", &temp2);
    CLEAR(&temp2);

    lines = mcp_mesg_arg_linecount(msg, "data");
    if (lines > 0) {
	temp2.type = PROG_ARRAY;
	temp2.data.array = new_array_packed(lines, fr->pinning);
	for (int i = 0; i < lines; i++) {
	    struct inst temp3;
	    array_data temp4;

	    temp3.type = PROG_INTEGER;
	    temp3.data.number = i;

	    temp4.type = PROG_STRING;
	    temp4.data.string = alloc_prog_string(mcp_mesg_arg_getline(msg, "data", i));

	    array_setitem(&temp2.data.array, &temp3, &temp4);
	    CLEAR(&temp4);
	}
	array_set_strkey(&temp.data.array, "data", &temp2);
	CLEAR(&temp2);
    }

    /*
       if (did_dismiss) {
       muf_dlog_remove(fr, dlogid);
       }
     */

    snprintf(buf, sizeof(buf), "GUI.%s", dlogid);
    muf_event_add(fr, buf, &temp, 0);
    CLEAR(&temp);
}

static void
fbgui_muf_error_cb(GUI_ERROR_CB_ARGS)
{
    char buf[BUFFER_LEN];
    struct frame *fr = (struct frame *) context;
    struct inst temp;

    temp.type = PROG_ARRAY;
    temp.data.array = new_array_dictionary(fr->pinning);

    array_set_strkey_intval(&temp.data.array, "descr", descr);
    array_set_strkey_strval(&temp.data.array, "dlogid", dlogid);
    if (id) {
	array_set_strkey_strval(&temp.data.array, "id", id);
    }
    array_set_strkey_strval(&temp.data.array, "errcode", errcode);
    array_set_strkey_strval(&temp.data.array, "errtext", errtext);

    snprintf(buf, sizeof(buf), "GUI.%s", dlogid);
    muf_event_add(fr, buf, &temp, 0);
    CLEAR(&temp);
}

void
prim_gui_available(PRIM_PROTOTYPE)
{
    McpVer ver;
    double fver;

    CHECKOP(1);
    oper1 = POP();		/* descr */

    if (oper1->type != PROG_INTEGER)
	abort_interp("Integer descriptor number expected. (1)");

    ver = GuiVersion(oper1->data.number);
    fver = ver.vermajor + (ver.verminor / 1000.0);

    CLEAR(oper1);

    PushFloat(fver);
}

void
prim_gui_dlog_create(PRIM_PROTOTYPE)
{
    const char *dlogid = NULL;
    char *title = NULL;
    char *wintype = NULL;
    stk_array *arr = NULL;
    McpMesg msg;
    McpFrame *mfr;

    CHECKOP(4);
    oper4 = POP();		/* arr  args */
    oper3 = POP();		/* str  title */
    oper2 = POP();		/* str  type */
    oper1 = POP();		/* int  descr */

    CHECKMCPPERM();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Integer descriptor number expected. (1)");
    if (!GuiSupported(oper1->data.number))
	abort_interp("The MCP GUI package is not supported for this connection.");
    if (oper2->type != PROG_STRING)
	abort_interp("Window type string expected. (3)");
    if (oper3->type != PROG_STRING)
	abort_interp("Window title string expected. (3)");
    if (oper4->type != PROG_ARRAY)
	abort_interp("Window options array expected. (4)");

    title = DoNullInd(oper3->data.string);
    wintype = oper2->data.string ? oper2->data.string->data : "simple";
    arr = oper4->data.array;
    mfr = descr_mcpframe(oper1->data.number);

    if (!mfr)
	abort_interp("Invalid descriptor number. (1)");

    dlogid = gui_dlog_alloc(oper1->data.number, fbgui_muf_event_cb, fbgui_muf_error_cb, fr);
    mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-create");
    mcp_mesg_arg_append(&msg, "title", title);

    result = stuff_dict_in_mesg(arr, &msg);
    if (result) {
	mcp_mesg_clear(&msg);
	switch (result) {
	case -1:
	    abort_interp("Args dictionary can only have string keys. (4)");
	case -2:
	    abort_interp("Args dictionary cannot have a null string key. (4)");
	case -3:
	    abort_interp("Unsupported value type in list value. (4)");
	case -4:
	    abort_interp("Unsupported value type in args dictionary. (4)");
	}
    }

    mcp_mesg_arg_remove(&msg, "type");
    mcp_mesg_arg_append(&msg, "type", wintype);
    mcp_mesg_arg_remove(&msg, "dlogid");
    mcp_mesg_arg_append(&msg, "dlogid", dlogid);

    mcp_frame_output_mesg(mfr, &msg);
    mcp_mesg_clear(&msg);
    muf_dlog_add(fr, dlogid);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);

    PushString(dlogid);
}

void
prim_gui_dlog_show(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();		/* str  dlogid */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("String dialog ID expected.");
    if (!oper1->data.string || !oper1->data.string->data[0])
	abort_interp("Invalid dialog ID.");

    result = GuiShow(oper1->data.string->data);
    if (result == EGUINOSUPPORT)
	abort_interp("GUI not available.  Internal error.");
    if (result == EGUINODLOG)
	abort_interp("Invalid dialog ID.");

    CLEAR(oper1);
}

void
prim_gui_dlog_close(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();		/* str  dlogid */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("String dialog ID expected.");
    if (!oper1->data.string || !oper1->data.string->data[0])
	abort_interp("Invalid dialog ID.");

    result = GuiClose(oper1->data.string->data);
    if (result == EGUINOSUPPORT)
	abort_interp("Internal error: GUI not available.");
    if (result == EGUINODLOG)
	abort_interp("Invalid dialog ID.");

    muf_dlog_remove(fr, oper1->data.string->data);

    CLEAR(oper1);
}

void
prim_gui_ctrl_create(PRIM_PROTOTYPE)
{
    size_t vallines = 0;
    char **vallist = NULL;
    char *dlogid = NULL;
    char *ctrlid = NULL;
    char *ctrltype = NULL;
    char *valname = NULL;
    stk_array *arr;
    McpMesg msg;
    McpFrame *mfr;
    int descr;
    char cmdname[64];

    CHECKOP(4);
    oper4 = POP();		/* dict args */
    oper3 = POP();		/* str  ctrlid */
    oper2 = POP();		/* str  type */
    oper1 = POP();		/* str  dlogid */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("Dialog ID string expected. (1)");
    if (!oper1->data.string || !oper1->data.string->data[0])
	abort_interp("Invalid dialog ID. (1)");

    if (oper2->type != PROG_STRING)
	abort_interp("Control type string expected. (2)");
    if (!oper2->data.string || !oper2->data.string->data[0])
	abort_interp("Invalid control type. (2)");

    if (oper3->type != PROG_STRING)
	abort_interp("Control ID string expected. (3)");

    if (oper4->type != PROG_ARRAY)
	abort_interp("Dictionary of arguments expected. (4)");

    dlogid = oper1->data.string->data;
    ctrltype = oper2->data.string->data;
    ctrlid = oper3->data.string ? oper3->data.string->data : NULL;
    arr = oper4->data.array;

    descr = gui_dlog_get_descr(dlogid);
    mfr = descr_mcpframe(descr);

    if (!mfr)
	abort_interp("No such dialog currently exists. (1)");

    if (!GuiSupported(descr))
	abort_interp
		("Internal error: The given dialog's descriptor doesn't support the GUI package. (1)");

    snprintf(cmdname, sizeof(cmdname), "ctrl-%.55s", ctrltype);
    mcp_mesg_init(&msg, GUI_PACKAGE, cmdname);

    result = stuff_dict_in_mesg(arr, &msg);
    if (result) {
	mcp_mesg_clear(&msg);
	switch (result) {
	case -1:
	    abort_interp("Args dictionary can only have string keys. (4)");
	case -2:
	    abort_interp("Args dictionary cannot have a null string key. (4)");
	case -3:
	    abort_interp("Unsupported value type in list value. (4)");
	case -4:
	    abort_interp("Unsupported value type in args dictionary. (4)");
	}
    }

    vallines = (size_t)mcp_mesg_arg_linecount(&msg, "value");
    valname = mcp_mesg_arg_getline(&msg, "valname", 0);
    if (!valname || !*valname) {
	valname = ctrlid;
    }
    if (valname && vallines > 0) {
	vallist = malloc(sizeof(char *) * vallines);
	for (unsigned int i = 0; i < vallines; i++)
	    vallist[i] = mcp_mesg_arg_getline(&msg, "value", i);
	gui_value_set_local(dlogid, valname, vallines, (const char **) vallist);
	free(vallist);
    }

    mcp_mesg_arg_remove(&msg, "dlogid");
    mcp_mesg_arg_append(&msg, "dlogid", dlogid);

    mcp_mesg_arg_remove(&msg, "id");
    mcp_mesg_arg_append(&msg, "id", ctrlid);

    mcp_frame_output_mesg(mfr, &msg);
    mcp_mesg_clear(&msg);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
}

void
prim_gui_ctrl_command(PRIM_PROTOTYPE)
{
    char *dlogid = NULL;
    char *ctrlid = NULL;
    char *ctrlcmd = NULL;
    stk_array *arr;
    McpMesg msg;
    McpFrame *mfr;
    int descr;

    CHECKOP(4);
    oper4 = POP();		/* dict args */
    oper2 = POP();		/* str  command */
    oper3 = POP();		/* str  ctrlid */
    oper1 = POP();		/* str  dlogid */

    if (oper1->type != PROG_STRING)
	abort_interp("Dialog ID string expected. (1)");
    if (!oper1->data.string || !*oper1->data.string->data)
	abort_interp("Non-null dialog ID string expected. (1)");

    if (oper3->type != PROG_STRING)
	abort_interp("Control ID string expected. (2)");
    if (!oper3->data.string || !*oper3->data.string->data)
	abort_interp("Non-null control ID string expected. (2)");

    if (oper2->type != PROG_STRING)
	abort_interp("Control command string expected. (3)");
    if (!oper2->data.string || !*oper2->data.string->data)
	abort_interp("Non-null control command string expected. (3)");

    if (oper4->type != PROG_ARRAY)
	abort_interp("Dictionary of arguments expected. (4)");

    dlogid = oper1->data.string->data;
    ctrlcmd = oper2->data.string->data;
    ctrlid = oper3->data.string ? oper3->data.string->data : NULL;
    arr = oper4->data.array;

    descr = gui_dlog_get_descr(dlogid);
    mfr = descr_mcpframe(descr);

    if (!mfr)
	abort_interp("No such dialog currently exists. (1)");

    if (!GuiSupported(descr))
	abort_interp
		("Internal error: The given dialog's descriptor doesn't support the GUI package. (1)");

    mcp_mesg_init(&msg, GUI_PACKAGE, "ctrl-command");

    result = stuff_dict_in_mesg(arr, &msg);
    if (result) {
	mcp_mesg_clear(&msg);
	switch (result) {
	case -1:
	    abort_interp("Args dictionary can only have string keys. (4)");
	case -2:
	    abort_interp("Args dictionary cannot have a null string key. (4)");
	case -3:
	    abort_interp("Unsupported value type in list value. (4)");
	case -4:
	    abort_interp("Unsupported value type in args dictionary. (4)");
	}
    }

    mcp_mesg_arg_remove(&msg, "dlogid");
    mcp_mesg_arg_append(&msg, "dlogid", dlogid);

    mcp_mesg_arg_remove(&msg, "id");
    mcp_mesg_arg_append(&msg, "id", ctrlid);

    mcp_mesg_arg_remove(&msg, "command");
    mcp_mesg_arg_append(&msg, "command", ctrlcmd);

    mcp_frame_output_mesg(mfr, &msg);
    mcp_mesg_clear(&msg);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
}

void
prim_gui_value_set(PRIM_PROTOTYPE)
{
    size_t count;
    char buf[BUFFER_LEN];
    char *name;
    char *dlogid;
    char *value;
    char **valarray;
    struct inst temp1;
    array_data *temp2;

    CHECKOP(3);
    oper3 = POP();		/* str value  */
    oper2 = POP();		/* str ctrlid */
    oper1 = POP();		/* str dlogid */

    CHECKMCPPERM();
    if (oper3->type != PROG_STRING && oper3->type != PROG_ARRAY)
	abort_interp("String or string list control value expected. (3)");

    if (oper2->type != PROG_STRING)
	abort_interp("String control ID expected. (2)");
    if (!oper2->data.string || !oper2->data.string->data[0])
	abort_interp("Invalid dialog ID. (2)");

    if (oper1->type != PROG_STRING)
	abort_interp("String dialog ID expected. (1)");
    if (!oper1->data.string || !oper1->data.string->data[0])
	abort_interp("Invalid dialog ID. (1)");

    dlogid = oper1->data.string->data;
    name = oper2->data.string->data;

    if (oper3->type == PROG_STRING) {
	count = 1;
	valarray = malloc(sizeof(char *) * count);

	value = DoNullInd(oper3->data.string);
	valarray[0] = strdup(value);
    } else {
	count = (size_t)array_count(oper3->data.array);
	valarray = malloc(sizeof(char *) * count);

	for (unsigned int i = 0; i < count; i++) {
	    temp1.type = PROG_INTEGER;
	    temp1.data.number = i;

	    temp2 = array_getitem(oper3->data.array, &temp1);
	    if (!temp2) {
		break;
	    }
	    switch (temp2->type) {
	    case PROG_STRING:
		value = DoNullInd(temp2->data.string);
		break;
	    case PROG_INTEGER:
		snprintf(buf, sizeof(buf), "%d", temp2->data.number);
		value = buf;
		break;
	    case PROG_OBJECT:
		snprintf(buf, sizeof(buf), "#%d", temp2->data.number);
		value = buf;
		break;
	    case PROG_FLOAT:
		snprintf(buf, sizeof(buf), "%.15g", temp2->data.fnumber);
		value = buf;
		break;
	    default:
		while (i-- > 0) {
		    free(valarray[i]);
		}
		free(valarray);
		abort_interp("Unsupported value type in list value. (3)");
	    }
	    valarray[i] = strdup(value);
	}
    }

    GuiSetVal(dlogid, name, count, (const char **) valarray);

    while (count-- > 0) {
	free(valarray[count]);
    }
    free(valarray);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

void
prim_gui_values_get(PRIM_PROTOTYPE)
{
    const char *name;
    char *dlogid;
    stk_array *nu;
    struct inst temp1;
    array_data temp2;

    CHECKOP(1);
    oper1 = POP();		/* str  dlogid */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("String dialog ID expected.");
    if (!oper1->data.string || !oper1->data.string->data[0])
	abort_interp("Invalid dialog ID.");

    dlogid = oper1->data.string->data;
    nu = new_array_dictionary(fr->pinning);
    name = GuiValueFirst(oper1->data.string->data);
    while (name) {
	int lines = gui_value_linecount(dlogid, name);

        if (lines < 0)
            lines = 0;

	temp1.type = PROG_STRING;
	temp1.data.string = alloc_prog_string(name);

	temp2.type = PROG_ARRAY;
	temp2.data.array = new_array_packed(lines, fr->pinning);

	for (int i = 0; i < lines; i++) {
	    struct inst temp3;
	    array_data temp4;

	    temp3.type = PROG_INTEGER;
	    temp3.data.number = i;

	    temp4.type = PROG_STRING;
	    temp4.data.string = alloc_prog_string(gui_value_get(dlogid, name, i));

	    array_setitem(&temp2.data.array, &temp3, &temp4);
	    CLEAR(&temp3);
	    CLEAR(&temp4);
	}
	array_setitem(&nu, &temp1, &temp2);
	CLEAR(&temp1);
	CLEAR(&temp2);

	name = GuiValueNext(dlogid, name);
    }

    CLEAR(oper1);
    PushArrayRaw(nu);
}

void
prim_gui_value_get(PRIM_PROTOTYPE)
{
    char *dlogid;
    char *ctrlid;
    stk_array *nu;
    struct inst temp1;
    array_data temp2;
    int lines;

    CHECKOP(2);
    oper2 = POP();		/* str  ctrlid */
    oper1 = POP();		/* str  dlogid */

    CHECKMCPPERM();
    if (oper1->type != PROG_STRING)
	abort_interp("String dialog ID expected. (1)");
    if (!oper1->data.string || !*oper1->data.string->data)
	abort_interp("Non-null string dialog ID expected. (1)");

    if (oper2->type != PROG_STRING)
	abort_interp("String control ID expected. (2)");
    if (!oper2->data.string || !*oper2->data.string->data)
	abort_interp("Non-null string control ID expected. (2)");

    dlogid = oper1->data.string->data;
    ctrlid = oper2->data.string->data;

    lines = gui_value_linecount(dlogid, ctrlid);
    if (lines < 0)
        lines = 0;
    nu = new_array_packed(lines, fr->pinning);

    for (int i = 0; i < lines; i++) {
	temp1.type = PROG_INTEGER;
	temp1.data.number = i;

	temp2.type = PROG_STRING;
	temp2.data.string = alloc_prog_string(gui_value_get(dlogid, ctrlid, i));

	array_setitem(&nu, &temp1, &temp2);

	CLEAR(&temp1);
	CLEAR(&temp2);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(nu);
}
#endif /* MCPGUI_SUPPORT */
