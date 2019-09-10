/** @file mcpgui.c
 *
 * Source for handling MCP (MUD Client Protocol) GUI events.  I believe these
 * are an extension that is used exclusively by Trebuchet MUCK client and thus
 * are of arguable utility. (tanabi)
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#ifdef MCPGUI_SUPPORT

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fbstrings.h"
#include "interp.h"
#include "mcp.h"
#include "mcpgui.h"
#include "mcppkg.h"

/*
 * @TODO: Editor's note.  Seems like the author of this couldn't decide
 *        if they were doing CamelCase or underscores.  If we were keeping
 *        this module, I'd say standardize it (underscores is more consistent).
 *        However, we're probably getting rid of this, so I am not advising
 *        we do it unless we decide to keep it.  (tanabi)
 */

/**
 * @var the list of all dialog structures
 */
static DlogData *dialog_list = NULL;

/**
 * @var a pointer to the last accessed structure, for performance
 */
static DlogData *dialog_last_accessed = NULL;

/**
 * Find a dialog structure based on ID
 *
 * First check the last accessed structure, and if no match, scan the
 * entire linked list.  Whichever one is found will be the new last
 * accessed.  It will return NULL if not found.
 *
 * @private
 * @param dlogid the dialog ID to search for
 * @return either the DlogData structure associated with the ID or NULL.
 */
static DlogData *
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

/**
 * Callback for initializing the GUI system
 *
 * This is used exclusively by gui_initialize to register the GUI package
 * with MCP.  Handles the routing of messages to dialogs, running the
 * appropriate success or error callbacks.
 *
 * The MCP message should have a 'dlogid' at a minimum, with an optional
 * 'id' as well as arguments.
 *
 * Message name should be ctrl-value to set a value on the dialog,
 * ctrl-event to send an event (and run the dialog's callback), or
 * error to do the error calback.
 *
 * ctrl-value looks for an argument named 'value' to set.
 * ctrl-event looks for argument "event" and "dismissed"
 * error looks for "errcode" and "errtext"
 *
 * @private
 * @param mfr the MCP frame
 * @param msg the MCP message
 * @param ver the MCP version
 * @param context this will always be NULL and is not used by this call.
 */
static void
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

    if (!strcasecmp(msg->mesgname, "ctrl-value")) {
        size_t valcount = (size_t)mcp_mesg_arg_linecount(msg, "value");
        const char **value;

        if (!id || !*id) {
            show_mcp_error(mfr, msg->mesgname, "Missing control ID.");
            return;
        }

        value = malloc(sizeof(const char *) * valcount);

        for (unsigned int i = 0; i < valcount; i++) {
            value[i] = mcp_mesg_arg_getline(msg, "value", i);
        }

        gui_value_set_local(dlogid, id, valcount, value);
        free((void **) value);

    } else if (!strcasecmp(msg->mesgname, "ctrl-event")) {
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
            if (!strcasecmp(dismissed, "false")) {
                did_dismiss = 0;
            } else if (!strcasecmp(dismissed, "0")) {
                did_dismiss = 0;
            }
        }

        if (did_dismiss) {
            dat->dismissed = 1;
        }

        if (dat->callback) {
            dat->callback(dat->descr, dlogid, id, evt, msg, did_dismiss, dat->context);
        }
    } else if (!strcasecmp(msg->mesgname, "error")) {
        const char *err = mcp_mesg_arg_getline(msg, "errcode", 0);
        const char *text = mcp_mesg_arg_getline(msg, "errtext", 0);

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

/**
 * Initializes the GUI package with MCP
 *
 * @see mcp_package_register
 */
void
gui_initialize(void)
{
    McpVer minver = { 1, 0 };	/* { major, minor } */
    McpVer maxver = { 1, 3 };	/* { major, minor } */

    mcp_package_register(GUI_PACKAGE, minver, maxver, gui_pkg_callback, NULL, NULL);
}

/**
 * Get the descriptor associated with a dlogid
 *
 * @param dlogid the dialog ID
 * @return integer descriptor ID or EGUINODLOG if no log ID
 */
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

/**
 * Return boolean true if GUI is closed, or false if not
 *
 * @private
 * @param dlogid the dialog ID to check
 * @return boolean true if dismissed, false if not, or EGUINODLOG if no dialog
 */
static int
GuiClosed(const char *dlogid)
{
    DlogData *ptr = gui_dlog_find(dlogid);

    if (ptr) {
        return ptr->dismissed;
    } else {
        return EGUINODLOG;
    }
}

/**
 * Get the line count associated with a given dialog and value ID
 *
 * Line count will be 0 if there is no id match.
 *
 * @param dlogid the dialog ID
 * @param id the value ID
 * @return either line count or EGUINODLOG if no dialog match.
 */
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

/**
 * Get the name of the first value associated with a dlogid
 *
 * Or NULL if there is no value (or if dlogid doesn't exist).
 *
 * Do not try to free the memory returned by this, it is owned by
 * the dialog and not the caller.
 *
 * @param dlogid the dialog ID
 * @return the name of the first value, or NULL if there is none.
 */
const char *
GuiValueFirst(const char *dlogid)
{
    DlogData *ddata = gui_dlog_find(dlogid);

    if (!ddata || !ddata->values) {
        return NULL;
    }

    return ddata->values->name;
}

/**
 * Get the next value name after value named 'id' for dialog 'dlogid'
 *
 * Returns NULL if dlogid doesn't exist.
 *
 * @param dlogid the dialog ID
 * @param id the id to look for
 * @return the next value's name after 'id' or NULL if not found
 */
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

/**
 * Fetch a line from the given value ID for a dialog
 *
 * This will look up the value named 'id' associated with dlogid,
 * and return the line number 'line' which starts with line 0.
 *
 * Returns NULL if the line number is out of bounds, the id is
 * not found, or the dlogid is not found.
 *
 * @param dlogid the dialog ID to look up
 * @param id the value name
 * @param line the line number to fetch
 * @return the associated string or NULL if something isn't found
 */
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

/**
 * Sets a key/value pair for the given dialog
 *
 * For the given 'id', sets the value to 'value' with 'lines' being
 * the line count.  If 'id' already exists, the old value will be
 * freed up.
 *
 * The memory in 'value' is copied over so no need to retain it after
 * using this call.  'id' is also copied if a new node is created.
 *
 * @param dlogid the dialog ID
 * @param id the key for the value
 * @param lines the number of lines in 'value'
 * @param value the value to set
 */
void
gui_value_set_local(const char *dlogid, const char *id, int lines,
                    const char **value)
{
    DlogValue *ptr;
    DlogData *ddata = gui_dlog_find(dlogid);
    int limit = 256;

    if (!ddata) {
        return;
    }

    ptr = ddata->values;

    /* Find the 'id' if it is alrady there */
    while (ptr && strcmp(ptr->name, id)) {
        ptr = ptr->next;

        if (!limit--) {
            return;
        }
    }

    /* Free existing if needed */
    if (ptr) {
        for (int i = 0; i < ptr->lines; i++) {
            free(ptr->value[i]);
        }

        free(ptr->value);
    } else { /* Otherwise make a new node */
        size_t ilen = strlen(id) + 1;
        ptr = malloc(sizeof(DlogValue));
        ptr->name = malloc(ilen);
        strcpyn(ptr->name, ilen, id);
        ptr->next = ddata->values;
        ddata->values = ptr;
    }

    /* Allocate and copy the value */
    ptr->lines = lines;
    ptr->value = malloc(sizeof(char *) * (size_t)lines);

    for (int i = 0; i < lines; i++) {
        size_t vlen = strlen(value[i]) + 1;
        ptr->value[i] = malloc(vlen);
        strcpyn(ptr->value[i], vlen, value[i]);
    }
}

/**
 * Free a dialog value node
 *
 * @private
 * @param ptr the dialog value node to free up.
 */
static void
gui_value_free(DlogValue * ptr)
{
    free(ptr->name);

    for (int i = 0; i < ptr->lines; i++) {
        free(ptr->value[i]);
    }

    free(ptr->value);
}

/**
 * Allocate a dialog node with the given initial value parameters.
 *
 * The new dialog will have a randomly generated dialog ID string which
 * is basically a random number converted to hexidecimal.
 *
 * Returns the newly allocated node.  The node is automatically put on the
 * dialog list and set as the last accessed node.
 *
 * @param descr the descriptor
 * @param callback the callback for the dialog
 * @param error_cb the error callback for the dialog
 * @param context context data
 * @return the allocated dialog structure
 */
const char *
gui_dlog_alloc(int descr, Gui_CB callback, GuiErr_CB error_cb, void *context)
{
    char tmpid[32];
    DlogData *ptr;
    size_t tlen;

    while (1) {
        snprintf(tmpid, sizeof(tmpid), "%08lX", (unsigned long) RANDOM());

        if (!gui_dlog_find(tmpid)) {
            break;
        }
    }

    ptr = malloc(sizeof(DlogData));
    tlen = strlen(tmpid) + 1;
    ptr->id = malloc(tlen);
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

/**
 * Free a dialog struct based on ID
 *
 * Find it, delete it, remove it from the linked lists.
 *
 * @param id the dialog to delete
 * @return either 0 on success or EGUINODLOG if 'id' was not found.
 */
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

/**
 * Send close events to all dialogs for a given descriptor
 *
 * Sends a ctrl-event id = _closed, dismissed = 1, event = buttonpress
 * to each dialog's callback.  Doesn't transmit anything or delete memory;
 * that is left up to the callback.
 *
 * @param descr the descriptor who's dialogs we are closing
 * @return always returns 0
 */
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
            ptr->callback(ptr->descr, ptr->id, "_closed", "buttonpress", &msg, 1,
                          ptr->context);
            mcp_mesg_clear(&msg);
        }

        ptr = ptr->next;
    }

    return 0;
}

/**
 * Returns the supported GUI version for a given descriptor
 *
 * If not supported, this will return McpVer { 0, 0 }
 *
 * @param descr the descriptor to get the GUI MCP package version for
 * @return the MCP Version
 */
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

/**
 * Return boolean true if the given descriptor supports GUI functions
 *
 * @param descr the descriptor to check for GUI support
 * @return boolean true if GUI supported, false otherwise.
 */
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

/**
 * Sets up a dialog structure for a simple dialog box
 *
 * Returns NULL if MCP or GUI is not supported.
 *
 * Uses the dlog-create message with type = simple and title being
 * whatever title is provided.
 *
 * @param descr the descriptor to set up GUI For
 * @param title the title of the dialog box
 * @param callback our GUI callback for events
 * @param error_cb our error callback
 * @param context our context if we need it
 * @return either the dialog ID or NULL if we could not do the GUI
 */
const char *
GuiSimple(int descr, const char *title, Gui_CB callback, GuiErr_CB error_cb,
          void *context)
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

/**
 * Send a dlog-show event to the given dialog ID
 *
 * @param id the ID to send the show event to
 * @return 0 on success, EGUINODLOG, or EGUINOSUPPORT for not found or supported
 */
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

/**
 * Send a dlog-close command to the given ID
 *
 * @param id the dialog to close
 * @return 0 on success, EGUINODLOG, or EGUINOSUPPORT for not found or supported
 */
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

/**
 * Set a key/value pair for a dialog, syncing it with the client
 *
 * This will send a ctrl-value message to the dialog updating the
 * client with the value as well.
 *
 * @param dlogid the dialog ID
 * @param id the value key we are setting
 * @param lines the number of lines in value
 * @param value the value to set
 * @return 0 on success, EGUINODLOG, or EGUINOSUPPORT for not found or supported
 */
int
GuiSetVal(const char *dlogid, const char *id, int lines, const char **value)
{
    McpMesg msg;
    McpFrame *mfr;
    int descr = gui_dlog_get_descr(dlogid);

    mfr = descr_mcpframe(descr);

    if (!mfr) {
        return EGUINODLOG;
    }

    if (GuiSupported(descr)) {
        mcp_mesg_init(&msg, GUI_PACKAGE, "ctrl-value");
        mcp_mesg_arg_append(&msg, "dlogid", dlogid);
        mcp_mesg_arg_append(&msg, "id", id);

        for (int i = 0; i < lines; i++) {
            mcp_mesg_arg_append(&msg, "value", value[i]);
        }

        mcp_frame_output_mesg(mfr, &msg);
        mcp_mesg_clear(&msg);
        gui_value_set_local(dlogid, id, lines, value);

        return 0;
    }

    return EGUINOSUPPORT;
}

/**
 * Given an MCP message, add layout based on 'layout' bitvector flags
 *
 * Whatever GUI_* flags, GET_COL*, GET_ROW*, GET_*PAD will be applied
 * to the 'msg'.  This sets a lot of args on the msg:
 *
 * * sticky which may be  some combination of n, s, e, and w for GUI_N,
 *   GUI_S, GUI_E, and GUI_W
 *
 * * colskip which will be the number from GET_COLSKIP if greater than 0
 *
 * * colspan which works similar to colskip
 *
 * * rowspan which works similar to colskip
 *
 * * leftpad which works similar to colskip
 *
 * * toppad which works similar to colskip
 *
 * * newline which will be 0 if GUI_NONL is set
 *
 * * hweight which will be 1 if GUI_HEXP is set
 *
 * * vweight which will be 1 if GUI_VEXP is set
 *
 * * report which will be 1 if GUI_REPORT is set
 *
 * * required which will be 1 if GUI_REQUIRED is set
 *
 * @private
 * @param msg the message to append layout information to
 * @param layout the bitvector to apply to 'msg'
 */
static void
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

/**
 * Automates the construction of a message for a given dialog
 *
 * Unfortunately there is no example of how this is used anywhere in the
 * system.  This can take variable arguments; the arguments are in pairs,
 * where the first is a name, and the second is the value.  For instance:
 *
 * gui_ctrl_make_l(dlogid, type, pane, id, text, value, layout,
 *                 "arg_key", "arg_value", ...)
 *
 * 'type' is whatever event type we are using.  'pane' can be NULL, if
 * you don't need it, otherwise it sets a 'pane' argument.  'text' and
 * 'value' can also be NULL, or they will set text/value arguments.
 *
 * @param dlogid the id
 * @param type the event type
 * @param pane the pane argument or NULL
 * @param id an ID for the message
 * @param text if set, argument "text" will map to 'text'
 * @param value if set, argument "value" will be set to "value"
 *        If 'id' and 'value' are both set, it will set a local variable
 *        @see gui_value_set_local
 * @param layout set layout parameters - @see gui_ctrl_process_layout
 * @param ... key/value pairs as described above
 * @return 0 on success, EGUINODLOG, or EGUINOSUPPORT for not found or supported
 */
int
gui_ctrl_make_l(const char *dlogid, const char *type, const char *pane,
                const char *id, const char *text, const char *value,
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

/**
 * The underpinning function of the gui_dlog_create primitive
 *
 * For a given MCP frame and dialog ID, associate the dialog to the
 * frame.
 *
 * @param fr the frame
 * @param dlogid the dialog ID
 */
void
muf_dlog_add(struct frame *fr, const char *dlogid)
{
    struct dlogidlist *item = malloc(sizeof(struct dlogidlist));
    strcpyn(item->dlogid, sizeof(item->dlogid), dlogid);
    item->next = fr->dlogids;
    fr->dlogids = item;
}

/**
 * The underpinning function of the gui_dlog_close primitive
 *
 * For a given MCP frame and dialog ID, remove the dialog from the
 * list of dialogs on the frame and free the memory.
 *
 * @param fr the frame we are working for
 * @param dlogid the dialog ID
 */
void
muf_dlog_remove(struct frame *fr, const char *dlogid)
{
    struct dlogidlist **prev = &fr->dlogids;

    while (*prev) {
        if (!strcasecmp(dlogid, (*prev)->dlogid)) {
            struct dlogidlist *tmp = *prev;

            *prev = (*prev)->next;
            free(tmp);
        } else {
            prev = &((*prev)->next);
        }
    }

    GuiFree(dlogid);
}

/**
 * Purge all dialogs associated with a frame
 *
 * This is used when cleaning up a program to clear all associated
 * dialogs.
 *
 * @param fr the frame to clean up
 */
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
#endif
