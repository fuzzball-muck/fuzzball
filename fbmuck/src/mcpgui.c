#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "interface.h"
#include "mcp.h"
#include "mcpgui.h"
#include "externs.h"
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */

typedef struct DlogValue_t {
	struct DlogValue_t *next;
	char *name;
	int lines;
	char **value;
} DlogValue;

typedef struct DlogData_t {
	struct DlogData_t *next;
	struct DlogData_t **prev;
	char *id;
	int descr;
	int dismissed;
	DlogValue *values;
	Gui_CB callback;
	GuiErr_CB error_cb;
	void *context;
} DlogData;

DlogData *dialog_list = NULL;
DlogData *dialog_last_accessed = NULL;


void gui_pkg_callback(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);


void
gui_initialize(void)
{
	McpVer minver = { 1, 0 };  /* { major, minor } */
	McpVer maxver = { 1, 3 };  /* { major, minor } */

	mcp_package_register(GUI_PACKAGE, minver, maxver, gui_pkg_callback, NULL, NULL);
}


DlogData *
gui_dlog_find(const char *dlogid)
{
	DlogData *ptr;

	ptr = dialog_last_accessed;
	if (ptr && !strcmp(ptr->id, dlogid)) {
		return ptr;
	}
	ptr = dialog_list;
	while (ptr) {
		if (!strcmp(ptr->id, dlogid)) {
			dialog_last_accessed = ptr;
			return ptr;
		}
		ptr = ptr->next;
	}
	return NULL;
}


void*
gui_dlog_get_context(const char *dlogid)
{
	DlogData *ptr = gui_dlog_find(dlogid);

	if (ptr) {
		return ptr->context;
	} else {
		return NULL;
	}
}


int
gui_dlog_get_descr(const char *dlogid)
{
	DlogData *ptr = gui_dlog_find(dlogid);

	if (ptr) {
		return ptr->descr;
	} else {
		return EGUINODLOG;
	}
}


int
GuiClosed(const char *dlogid)
{
	DlogData *ptr = gui_dlog_find(dlogid);

	if (ptr) {
		return ptr->dismissed;
	} else {
		return EGUINODLOG;
	}
}


int
gui_value_linecount(const char *dlogid, const char *id)
{
	DlogValue *ptr;
	DlogData *ddata = gui_dlog_find(dlogid);

	if (!ddata) {
		return EGUINODLOG;
	}
	ptr = ddata->values;
	while (ptr && strcmp(ptr->name, id)) {
		ptr = ptr->next;
	}
	if (!ptr) {
		return 0;
	}
	return ptr->lines;
}



const char *
GuiValueFirst(const char *dlogid)
{
	DlogData *ddata = gui_dlog_find(dlogid);

	if (!ddata || !ddata->values) {
		return NULL;
	}
	return ddata->values->name;
}


const char *
GuiValueNext(const char *dlogid, const char *id)
{
	DlogValue *ptr;
	DlogData *ddata = gui_dlog_find(dlogid);

	if (!ddata) {
		return NULL;
	}
	ptr = ddata->values;
	while (ptr && strcmp(ptr->name, id)) {
		ptr = ptr->next;
	}
	if (!ptr || !ptr->next) {
		return NULL;
	}
	return ptr->next->name;
}


const char *
gui_value_get(const char *dlogid, const char *id, int line)
{
	DlogValue *ptr;
	DlogData *ddata = gui_dlog_find(dlogid);

	if (!ddata) {
		return NULL;
	}
	ptr = ddata->values;
	while (ptr && strcmp(ptr->name, id)) {
		ptr = ptr->next;
	}
	if (!ptr || line < 0 || line >= ptr->lines) {
		return NULL;
	}
	return ptr->value[line];
}


void
gui_value_set_local(const char *dlogid, const char *id, int lines, const char **value)
{
	DlogValue *ptr;
	DlogData *ddata = gui_dlog_find(dlogid);
	int i;
	int limit = 256;

	if (!ddata) {
		return;
	}
	ptr = ddata->values;
	while (ptr && strcmp(ptr->name, id)) {
		ptr = ptr->next;
		if (!limit--) {
			return;
		}
	}
	if (ptr) {
		for (i = 0; i < ptr->lines; i++) {
			free(ptr->value[i]);
		}
		free(ptr->value);
	} else {
		int ilen = strlen(id)+1;
		ptr = (DlogValue *) malloc(sizeof(DlogValue));
		ptr->name = (char *) malloc(ilen);
		strcpyn(ptr->name, ilen, id);
		ptr->next = ddata->values;
		ddata->values = ptr;
	}
	ptr->lines = lines;
	ptr->value = (char **) malloc(sizeof(char *) * lines);

	for (i = 0; i < lines; i++) {
		int vlen = strlen(value[i])+1;
		ptr->value[i] = (char *) malloc(vlen);
		strcpyn(ptr->value[i], vlen, value[i]);
	}
}


void
gui_value_free(DlogValue * ptr)
{
	int i;

	free(ptr->name);
	for (i = 0; i < ptr->lines; i++) {
		free(ptr->value[i]);
	}
	free(ptr->value);
}


void
gui_pkg_callback(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context)
{
	DlogData *dat;
	const char *dlogid = mcp_mesg_arg_getline(msg, "dlogid", 0);
	const char *id = mcp_mesg_arg_getline(msg, "id", 0);

	if (!dlogid || !*dlogid) {
		show_mcp_error(mfr, msg->mesgname, "Missing dialog ID.");
		return;
	}
	dat = gui_dlog_find(dlogid);
	if (!dat) {
		show_mcp_error(mfr, msg->mesgname, "Invalid dialog ID.");
		return;
	}
	if (!string_compare(msg->mesgname, "ctrl-value")) {
		int valcount = mcp_mesg_arg_linecount(msg, "value");
		int i;
		const char **value = (const char **) malloc(sizeof(const char *) * valcount);

		if (!id || !*id) {
			show_mcp_error(mfr, msg->mesgname, "Missing control ID.");
			return;
		}
		for (i = 0; i < valcount; i++) {
			value[i] = mcp_mesg_arg_getline(msg, "value", i);
		}
		gui_value_set_local(dlogid, id, valcount, value);
		free((char **)value);

	} else if (!string_compare(msg->mesgname, "ctrl-event")) {
		const char *evt = mcp_mesg_arg_getline(msg, "event", 0);
		const char *dismissed = mcp_mesg_arg_getline(msg, "dismissed", 0);
		int did_dismiss = 1;

		if (!id || !*id) {
			show_mcp_error(mfr, msg->mesgname, "Missing control ID.");
			return;
		}
		if (!evt || !*evt) {
			evt = "buttonpress";
		}
		if (NULL != dismissed) {
			if (!string_compare(dismissed, "false")) {
				did_dismiss = 0;
			} else if (!string_compare(dismissed, "0")) {
				did_dismiss = 0;
			}
		}
		if (did_dismiss) {
			dat->dismissed = 1;
		}
		if (dat->callback) {
			dat->callback(dat->descr, dlogid, id, evt, msg, did_dismiss, dat->context);
		}
	} else if (!string_compare(msg->mesgname, "error")) {
		const char *err  = mcp_mesg_arg_getline(msg, "errcode", 0);
		const char *text = mcp_mesg_arg_getline(msg, "errtext", 0);
		const char *id   = mcp_mesg_arg_getline(msg, "id", 0);

		if (!err) {
			err = "";
		}
		if (!text) {
			text = "";
		}
		if (dat->error_cb) {
			dat->error_cb(dat->descr, dlogid, id, err, text, dat->context);
		}
	}
}


const char *
gui_dlog_alloc(int descr, Gui_CB callback, GuiErr_CB error_cb, void *context)
{
	char tmpid[32];
	DlogData *ptr;
	int tlen;

	while (1) {
		snprintf(tmpid, sizeof(tmpid), "%08lX", (unsigned long)RANDOM());
		if (!gui_dlog_find(tmpid)) {
			break;
		}
	}
	ptr = (DlogData *) malloc(sizeof(DlogData));
	tlen = strlen(tmpid)+1;
	ptr->id = (char *) malloc(tlen);
	strcpyn(ptr->id, tlen, tmpid);
	ptr->descr = descr;
	ptr->dismissed = 0;
	ptr->callback = callback;
	ptr->error_cb = error_cb;
	ptr->context = context;
	ptr->values = NULL;

	ptr->prev = &dialog_list;
	ptr->next = dialog_list;
	if (dialog_list)
		dialog_list->prev = &ptr->next;
	dialog_list = ptr;
	dialog_last_accessed = ptr;
	return ptr->id;
}


int
GuiFree(const char *id)
{
	DlogData *ptr;
	DlogValue *valptr;
	DlogValue *nextval;

	ptr = gui_dlog_find(id);
	if (!ptr) {
		return EGUINODLOG;
	}
	*ptr->prev = ptr->next;
	if (ptr->next)
		ptr->next->prev = ptr->prev;

	ptr->next = NULL;
	ptr->prev = NULL;
	free(ptr->id);

	valptr = ptr->values;
	while (valptr) {
		nextval = valptr->next;
		gui_value_free(valptr);
		valptr = nextval;
	}
	ptr->values = NULL;
	ptr->descr = 0;
	if (dialog_last_accessed == ptr) {
		dialog_last_accessed = NULL;
	}
	free(ptr);
	return 0;
}


int
gui_dlog_closeall_descr(int descr)
{
	DlogData *ptr;
	McpMesg msg;

	ptr = dialog_list;
	while (ptr) {
		while (ptr) {
			if (ptr->descr == descr) {
				break;
			}
			ptr = ptr->next;
		}
		if (!ptr) {
			return 0;
		}
		if (ptr->callback) {
			mcp_mesg_init(&msg, GUI_PACKAGE, "ctrl-event");
			mcp_mesg_arg_append(&msg, "dlogid", ptr->id);
			mcp_mesg_arg_append(&msg, "id", "_closed");
			mcp_mesg_arg_append(&msg, "dismissed", "1");
			mcp_mesg_arg_append(&msg, "event", "buttonpress");
			ptr->callback(ptr->descr, ptr->id, "_closed", "buttonpress", &msg, 1, ptr->context);
			mcp_mesg_clear(&msg);
		}
		ptr = ptr->next;
	}
	return 0;
}


int
gui_dlog_freeall_descr(int descr)
{
	DlogData *ptr;
	DlogData *next;
	DlogValue *valptr;

	ptr = dialog_list;
	while (ptr) {
		while (ptr) {
			if (ptr->descr == descr) {
				break;
			}
			ptr = ptr->next;
		}
		if (!ptr) {
			return 0;
		}
		next = ptr->next;
		*ptr->prev = next;
		if (next)
			next->prev = ptr->prev;

		ptr->next = NULL;
		ptr->prev = NULL;
		if (dialog_last_accessed == ptr) {
			dialog_last_accessed = NULL;
		}
		free(ptr->id);

		valptr = ptr->values;
		while (valptr) {
			ptr->values = valptr->next;
			gui_value_free(valptr);
			valptr = ptr->values;
		}
		ptr->values = NULL;
		ptr->descr = 0;
		free(ptr);
		ptr = next;
	}
	return 0;
}


McpVer
GuiVersion(int descr)
{
	McpVer supp = { 0, 0 };
	McpFrame *mfr;

	mfr = descr_mcpframe(descr);
	if (mfr) {
		supp = mcp_frame_package_supported(mfr, GUI_PACKAGE);
	}
	return supp;
}


int
GuiSupported(int descr)
{
	McpVer supp;
	McpFrame *mfr;

	mfr = descr_mcpframe(descr);
	if (mfr) {
		supp = mcp_frame_package_supported(mfr, GUI_PACKAGE);
		if (supp.verminor != 0 || supp.vermajor != 0) {
			return 1;
		}
	}
	return 0;
}


const char *
GuiSimple(int descr, const char *title, Gui_CB callback, GuiErr_CB error_cb, void *context)
{
	McpMesg msg;
	McpFrame *mfr = descr_mcpframe(descr);
	const char *id;

	if (!mfr) {
		return NULL;
	}
	if (GuiSupported(descr)) {
		id = gui_dlog_alloc(descr, callback, error_cb, context);
		mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-create");
		mcp_mesg_arg_append(&msg, "dlogid", id);
		mcp_mesg_arg_append(&msg, "type", "simple");
		mcp_mesg_arg_append(&msg, "title", title);
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return id;
	} else {
		return NULL;
	}
}


const char *
GuiTabbed(int descr,
		  const char *title,
		  int pagecount, const char **pagenames, const char **pageids,
		  Gui_CB callback, GuiErr_CB error_cb, void *context)
{
	McpMesg msg;
	McpFrame *mfr = descr_mcpframe(descr);
	const char *id;
	int i;

	if (!mfr) {
		return NULL;
	}
	if (GuiSupported(descr)) {
		id = gui_dlog_alloc(descr, callback, error_cb, context);
		mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-create");
		mcp_mesg_arg_append(&msg, "dlogid", id);
		mcp_mesg_arg_append(&msg, "type", "tabbed");
		mcp_mesg_arg_append(&msg, "title", title);
		for (i = 0; i < pagecount; i++) {
			mcp_mesg_arg_append(&msg, "panes", pageids[i]);
			mcp_mesg_arg_append(&msg, "names", pagenames[i]);
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return id;
	} else {
		return NULL;
	}
}


const char *
GuiHelper(int descr,
		  const char *title,
		  int pagecount, const char **pagenames, const char **pageids,
		  Gui_CB callback, GuiErr_CB error_cb, void *context)
{
	McpMesg msg;
	McpFrame *mfr = descr_mcpframe(descr);
	const char *id;
	int i;

	if (!mfr) {
		return NULL;
	}
	if (GuiSupported(descr)) {
		id = gui_dlog_alloc(descr, callback, error_cb, context);
		mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-create");
		mcp_mesg_arg_append(&msg, "dlogid", id);
		mcp_mesg_arg_append(&msg, "type", "helper");
		mcp_mesg_arg_append(&msg, "title", title);
		for (i = 0; i < pagecount; i++) {
			mcp_mesg_arg_append(&msg, "panes", pageids[i]);
			mcp_mesg_arg_append(&msg, "names", pagenames[i]);
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return id;
	} else {
		return NULL;
	}
}


int
GuiShow(const char *id)
{
	McpMesg msg;
	McpFrame *mfr;
	int descr = gui_dlog_get_descr(id);

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		if (!GuiClosed(id)) {
			mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-show");
			mcp_mesg_arg_append(&msg, "dlogid", id);
			mcp_frame_output_mesg(mfr, &msg);
			mcp_mesg_clear(&msg);
		}
		return 0;
	}
	return EGUINOSUPPORT;
}



int
GuiClose(const char *id)
{
	McpMesg msg;
	McpFrame *mfr;
	int descr = gui_dlog_get_descr(id);

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		if (!GuiClosed(id)) {
			mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-close");
			mcp_mesg_arg_append(&msg, "dlogid", id);
			mcp_frame_output_mesg(mfr, &msg);
			mcp_mesg_clear(&msg);
		}
		return 0;
	}
	return EGUINOSUPPORT;
}



int
GuiSetVal(const char *dlogid, const char *id, int lines, const char **value)
{
	McpMesg msg;
	McpFrame *mfr;
	int i;
	int descr = gui_dlog_get_descr(dlogid);

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		mcp_mesg_init(&msg, GUI_PACKAGE, "ctrl-value");
		mcp_mesg_arg_append(&msg, "dlogid", dlogid);
		mcp_mesg_arg_append(&msg, "id", id);
		for (i = 0; i < lines; i++) {
			mcp_mesg_arg_append(&msg, "value", value[i]);
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		gui_value_set_local(dlogid, id, lines, value);
		return 0;
	}
	return EGUINOSUPPORT;
}



int
GuiListInsert(const char *dlogid, const char *id, int after, int lines, const char **value)
{
	McpMesg msg;
	McpFrame *mfr;
	char numbuf[32];
	int i;
	int descr = gui_dlog_get_descr(dlogid);

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		mcp_mesg_init(&msg, GUI_PACKAGE, "list-insert");
		mcp_mesg_arg_append(&msg, "dlogid", dlogid);
		mcp_mesg_arg_append(&msg, "id", id);
		if (after > 0) {
			snprintf(numbuf, sizeof(numbuf), "%d", after);
			mcp_mesg_arg_append(&msg, "after", numbuf);
		}
		for (i = 0; i < lines; i++) {
			mcp_mesg_arg_append(&msg, "values", value[i]);
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return 0;
	}
	return EGUINOSUPPORT;
}



int
GuiListDel(const char *dlogid, const char *id, int from, int to)
{
	McpMesg msg;
	McpFrame *mfr;
	char numbuf[32];
	int descr = gui_dlog_get_descr(dlogid);

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		mcp_mesg_init(&msg, GUI_PACKAGE, "list-delete");
		mcp_mesg_arg_append(&msg, "dlogid", dlogid);
		mcp_mesg_arg_append(&msg, "id", id);
		if (from == GUI_LIST_END) {
			mcp_mesg_arg_append(&msg, "from", "end");
		} else {
			snprintf(numbuf, sizeof(numbuf), "%d", from);
			mcp_mesg_arg_append(&msg, "from", numbuf);
		}
		if (to == GUI_LIST_END) {
			mcp_mesg_arg_append(&msg, "to", "end");
		} else {
			snprintf(numbuf, sizeof(numbuf), "%d", to);
			mcp_mesg_arg_append(&msg, "to", numbuf);
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return 0;
	}
	return EGUINOSUPPORT;
}



int
GuiMenuItem(const char *dlogid, const char *id, const char *type, const char *name, const char **args)
{
	McpMesg msg;
	McpFrame *mfr;
	int i;
	int descr = gui_dlog_get_descr(dlogid);

	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		mcp_mesg_init(&msg, GUI_PACKAGE, "menu-item");
		mcp_mesg_arg_append(&msg, "dlogid", dlogid);
		mcp_mesg_arg_append(&msg, "id", id);
		mcp_mesg_arg_append(&msg, "name", name);
		mcp_mesg_arg_append(&msg, "type", name);
		i = 0;
		while (args && args[i]) {
			const char *arg = args[i];
			const char *val = args[i + 1];

			mcp_mesg_arg_append(&msg, arg, val);
			i += 2;
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return 0;
	}
	return EGUINOSUPPORT;
}



int
GuiMenuCmd(const char *dlogid, const char *id, const char *name)
{
	return GuiMenuItem(dlogid, id, "command", name, NULL);
}



int
GuiMenuCheckBtn(const char *dlogid, const char *id, const char *name, const char **args)
{
	return GuiMenuItem(dlogid, id, "checkbutton", name, args);
}



void
gui_ctrl_process_layout(McpMesg * msg, int layout)
{
	char buf[32];

	buf[0] = '\0';
	if ((layout & GUI_N))
		strcatn(buf, sizeof(buf), "n");

	if ((layout & GUI_S))
		strcatn(buf, sizeof(buf), "s");

	if ((layout & GUI_E))
		strcatn(buf, sizeof(buf), "e");

	if ((layout & GUI_W))
		strcatn(buf, sizeof(buf), "w");

	if (strcmp(buf, ""))
		mcp_mesg_arg_append(msg, "sticky", buf);


	snprintf(buf, sizeof(buf), "%d", GET_COLSKIP(layout));
	if (strcmp(buf, "0"))
		mcp_mesg_arg_append(msg, "colskip", buf);


	snprintf(buf, sizeof(buf), "%d", GET_COLSPAN(layout));
	if (strcmp(buf, "1"))
		mcp_mesg_arg_append(msg, "colspan", buf);


	snprintf(buf, sizeof(buf), "%d", GET_ROWSPAN(layout));
	if (strcmp(buf, "1"))
		mcp_mesg_arg_append(msg, "rowspan", buf);


	snprintf(buf, sizeof(buf), "%d", GET_LEFTPAD(layout));
	if (strcmp(buf, "0"))
		mcp_mesg_arg_append(msg, "leftpad", buf);


	snprintf(buf, sizeof(buf), "%d", GET_TOPPAD(layout));
	if (strcmp(buf, "0"))
		mcp_mesg_arg_append(msg, "toppad", buf);


	if ((layout & GUI_NONL))
		mcp_mesg_arg_append(msg, "newline", "0");

	if ((layout & GUI_HEXP))
		mcp_mesg_arg_append(msg, "hweight", "1");

	if ((layout & GUI_VEXP))
		mcp_mesg_arg_append(msg, "vweight", "1");

	if ((layout & GUI_REPORT))
		mcp_mesg_arg_append(msg, "report", "1");

	if ((layout & GUI_REQUIRED))
		mcp_mesg_arg_append(msg, "required", "1");
}



int
gui_ctrl_make_v(const char *dlogid, const char *type, const char *pane,
		        const char *id, const char *text, const char *value,
				int layout, const char **args)
{
	McpMesg msg;
	McpFrame *mfr;
	int descr;
	int i;

	descr = gui_dlog_get_descr(dlogid);
	mfr = descr_mcpframe(descr);
	if (!mfr) {
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		char cmdname[64];

		snprintf(cmdname, sizeof(cmdname), "ctrl-%.55s", type);
		mcp_mesg_init(&msg, GUI_PACKAGE, cmdname);
		gui_ctrl_process_layout(&msg, layout);
		mcp_mesg_arg_append(&msg, "dlogid", dlogid);
		mcp_mesg_arg_append(&msg, "id", id);
		if (text)
			mcp_mesg_arg_append(&msg, "text", text);
		if (value) {
			mcp_mesg_arg_append(&msg, "value", value);
			if (id) {
				gui_value_set_local(dlogid, id, 1, &value);
			}
		}
		if (pane)
			mcp_mesg_arg_append(&msg, "pane", pane);
		i = 0;
		while (args && args[i]) {
			const char *arg = args[i];
			const char *val = args[i + 1];

			if (id && !string_compare(arg, "value")) {
				gui_value_set_local(dlogid, id, 1, &val);
			}
			mcp_mesg_arg_append(&msg, arg, val);
			i += 2;
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		return 0;
	}
	return EGUINOSUPPORT;
}



int
gui_ctrl_make_l(const char *dlogid, const char *type, const char *pane, const char *id, const char *text, const char *value,
				int layout, ...)
{
	va_list ap;
	McpMesg msg;
	McpFrame *mfr;
	int descr;

	va_start(ap, layout);

	descr = gui_dlog_get_descr(dlogid);
	mfr = descr_mcpframe(descr);
	if (!mfr) {
		va_end(ap);
		return EGUINODLOG;
	}
	if (GuiSupported(descr)) {
		char cmdname[64];

		snprintf(cmdname, sizeof(cmdname), "ctrl-%.55s", type);
		mcp_mesg_init(&msg, GUI_PACKAGE, cmdname);
		gui_ctrl_process_layout(&msg, layout);
		mcp_mesg_arg_append(&msg, "dlogid", dlogid);
		mcp_mesg_arg_append(&msg, "id", id);
		if (text)
			mcp_mesg_arg_append(&msg, "text", text);
		if (value) {
			mcp_mesg_arg_append(&msg, "value", value);
			if (id) {
				gui_value_set_local(dlogid, id, 1, &value);
			}
		}
		if (pane)
			mcp_mesg_arg_append(&msg, "pane", pane);
		while (1) {
			const char *val;
			const char *arg;

			arg = va_arg(ap, const char *);

			if (!arg)
				break;
			val = va_arg(ap, const char *);

			mcp_mesg_arg_append(&msg, arg, val);
		}
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
		va_end(ap);
		return 0;
	}
	va_end(ap);
	return EGUINOSUPPORT;
}



int
GuiEdit(const char *dlogid, const char *pane, const char *id, const char *text, const char *value, int width, int layout)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d", width);
	return gui_ctrl_make_l(dlogid, "edit", pane, id, text, value, layout, "width", buf, NULL);
}


int
GuiText(const char *dlogid, const char *pane, const char *id, const char *value, int width, int layout)
{
	char widthbuf[32];

	snprintf(widthbuf, sizeof(widthbuf), "%d", width);
	return gui_ctrl_make_l(dlogid, "text", pane, id, NULL, value, layout, "width", widthbuf, NULL);
}


int
GuiSpinner(const char *dlogid, const char *pane, const char *id, const char *text, int value, int width, int min,
		   int max, int layout)
{
	char widthbuf[32];
	char valbuf[32];
	char minbuf[32];
	char maxbuf[32];

	snprintf(widthbuf, sizeof(widthbuf), "%d", width);
	snprintf(valbuf, sizeof(valbuf), "%d", value);
	snprintf(minbuf, sizeof(minbuf), "%d", min);
	snprintf(maxbuf, sizeof(maxbuf), "%d", max);
	return gui_ctrl_make_l(dlogid, "spinner", pane, id, text, valbuf, layout,
					"width", widthbuf, "min", minbuf, "max", maxbuf, NULL);
}


int
GuiCombo(const char *dlogid, const char *pane, const char *id, const char *text, const char *value, int width, int editable,
		 int layout)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d", width);
	return gui_ctrl_make_l(dlogid, "combobox", pane, id, text, value, layout,
					"width", buf, "editable", editable ? "1" : "0", NULL);
}


int
GuiMulti(const char *dlogid, const char *pane, const char *id, const char *value, int width, int height, int fixed,
		 int layout)
{
	char widthbuf[32];
	char heightbuf[32];

	snprintf(widthbuf, sizeof(widthbuf), "%d", width);
	snprintf(heightbuf, sizeof(heightbuf), "%d", height);
	return gui_ctrl_make_l(dlogid, "multiedit", pane, id, NULL, value, layout,
					"width", widthbuf,
					"height", heightbuf, "font", (fixed ? "fixed" : "variable"), NULL);
}


int
GuiHRule(const char *dlogid, const char *pane, const char *id, int height, int layout)
{
	char heightbuf[32];

	snprintf(heightbuf, sizeof(heightbuf), "%d", height);
	return gui_ctrl_make_l(dlogid, "hrule", pane, id, NULL, NULL, layout, "height", heightbuf, NULL);
}


int
GuiVRule(const char *dlogid, const char *pane, const char *id, int thickness, int layout)
{
	char widthbuf[32];

	snprintf(widthbuf, sizeof(widthbuf), "%d", thickness);
	return gui_ctrl_make_l(dlogid, "vrule", pane, id, NULL, NULL, layout, "width", widthbuf, NULL);
}


int
GuiFrame(const char *dlogid, const char *pane, const char *id, int layout)
{
	return gui_ctrl_make_l(dlogid, "frame", pane, id, NULL, NULL, layout, NULL);
}


int
GuiGroupBox(const char *dlogid, const char *pane, const char *id, const char *text, int collapsible, int collapsed,
			int layout)
{
	return gui_ctrl_make_l(dlogid, "frame", pane, id, text, NULL, layout,
					"visible", "1",
					"collapsible", collapsible ? "1" : "0",
					"collapsed", collapsed ? "1" : "0", NULL);
}


int
GuiButton(const char *dlogid, const char *pane, const char *id, const char *text, int width, int dismiss, int layout)
{
	char widthbuf[32];

	snprintf(widthbuf, sizeof(widthbuf), "%d", width);
	return gui_ctrl_make_l(dlogid, "button", pane, id, text, NULL, layout,
					"width", widthbuf, "dismiss", dismiss ? "1" : "0", NULL);
}






/******************************************************************************************/
/* MUF gui functions
 **********************/


void
muf_dlog_add(struct frame *fr, const char *dlogid)
{
	struct dlogidlist *item = (struct dlogidlist *) malloc(sizeof(struct dlogidlist));

	strcpyn(item->dlogid, sizeof(item->dlogid), dlogid);
	item->next = fr->dlogids;
	fr->dlogids = item;
}


void
muf_dlog_remove(struct frame *fr, const char *dlogid)
{
	struct dlogidlist **prev = &fr->dlogids;

	while (*prev) {
		if (!string_compare(dlogid, (*prev)->dlogid)) {
			struct dlogidlist *tmp = *prev;

			*prev = (*prev)->next;
			free(tmp);
		} else {
			prev = &((*prev)->next);
		}
	}
	GuiFree(dlogid);
}


void
muf_dlog_purge(struct frame *fr)
{
	while (fr->dlogids) {
		struct dlogidlist *tmp = fr->dlogids;

		GuiClose(fr->dlogids->dlogid);
		GuiFree(fr->dlogids->dlogid);
		fr->dlogids = fr->dlogids->next;
		free(tmp);
	}
}



/***************************************************************************
 ***************************************************************************
 ***************************************************************************/

void
post_dlog_cb(GUI_EVENT_CB_ARGS)
{
	if (!string_compare(id, "post")) {
		char buf[BUFFER_LEN];
		const char *subject = gui_value_get(dlogid, "subj", 0);
		const char *keywords = gui_value_get(dlogid, "keywd", 0);
		/* int bodycnt = gui_value_linecount(dlogid, "body"); */

		snprintf(buf, sizeof(buf), "Subject: %s", subject);
		pnotify(pdescrcon(descr), buf);
		snprintf(buf, sizeof(buf), "Keywords: %s", keywords);
		pnotify(pdescrcon(descr), buf);
	} else if (!string_compare(id, "cancel")) {
		pnotify(pdescrcon(descr), "Posting cancelled.");
	} else {
		pnotify(pdescrcon(descr), "Invalid event!");
	}
	if (did_dismiss) {
		GuiFree(dlogid);
	}
}


void
do_post_dlog(int descr, const char *text)
{
	const char *keywords[] = { "Misc.", "Wedding", "Party", "Toading", "New MUCK" };
	const char *dlg = GuiSimple(descr, "A demonstration dialog", post_dlog_cb, NULL, NULL);

	GuiEdit(dlg, NULL, "subj", "Subject", text, 60, GUI_EW);
	GuiCombo(dlg, NULL, "keywd", "Keywords", "Misc.", 60, 1, GUI_EW | GUI_HEXP);
	GuiListInsert(dlg, "keywd", GUI_LIST_END, 5, keywords);

	GuiMulti(dlg, NULL, "body", NULL, 80, 12, 1, GUI_NSEW | GUI_VEXP | COLSPAN(2));
	GuiHRule(dlg, NULL, NULL, 2, COLSPAN(2));
	GuiFrame(dlg, NULL, "bfr", GUI_EW | COLSPAN(2) | TOPPAD(0));
	GuiFrame(dlg, "bfr", NULL, GUI_EW | GUI_HEXP | GUI_NONL);
	GuiVRule(dlg, NULL, NULL, 2, GUI_NONL);
	GuiButton(dlg, NULL, "post", "Post", 8, 1, GUI_E | GUI_NONL);
	GuiButton(dlg, NULL, "cancel", "Cancel", 8, 1, GUI_E | TOPPAD(0));

	GuiShow(dlg);
}
