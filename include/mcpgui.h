/** @file mcpgui.h
 *
 * Header for handling MCP (MUD Client Protocol) GUI events.  I believe these
 * are an extension that is used exclusively by Trebuchet MUCK client and thus
 * are of arguable utility. (tanabi)
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#ifdef MCPGUI_SUPPORT
#ifndef MCPGUI_H
#define MCPGUI_H

#include "interp.h"
#include "mcp.h"

/*
 * Error results.
 */
#define EGUINOSUPPORT -1    /* This connection doesn't support GUI MCP */
#define EGUINODLOG    -2    /* No dialog exists with the given ID. */

/* The MCP package name. */
#define GUI_PACKAGE   "org-fuzzball-gui"

/* Used in list related commands to refer to the end of a list. */
#define GUI_LIST_END  -1

/*
 * Defines for specifying control layout
 *
 * Cardinal directions are used for GUI layouts.
 */
#define GUI_STICKY_MASK  0xF
#define GUI_N     0x1
#define GUI_S     0x2
#define GUI_E     0x4
#define GUI_W     0x8
#define GUI_NS (GUI_N | GUI_S)
#define GUI_NW (GUI_N | GUI_W)
#define GUI_NE (GUI_N | GUI_E)
#define GUI_SE (GUI_E | GUI_S)
#define GUI_SW (GUI_W | GUI_S)
#define GUI_EW (GUI_E | GUI_W)
#define GUI_NSE (GUI_NS | GUI_E)
#define GUI_NSW (GUI_NS | GUI_W)
#define GUI_NEW (GUI_N | GUI_EW)
#define GUI_SEW (GUI_S | GUI_EW)
#define GUI_NSEW (GUI_NS | GUI_EW)

/*
 * For table-style layouts, provide span features for both rows and
 * columns.
 */
#define GUI_COLSPAN_MASK 0xF0
#define COLSPAN(val) (((val-1) & 0xF) << 4)
#define GET_COLSPAN(val) (((val & GUI_COLSPAN_MASK) >> 4) +1)

#define GUI_ROWSPAN_MASK 0xF00
#define ROWSPAN(val) (((val-1) & 0xF) << 8)
#define GET_ROWSPAN(val) (((val & GUI_ROWSPAN_MASK) >> 8) +1)

#define GUI_COLSKIP_MASK 0xF000
#define COLSKIP(val) ((val & 0xF) << 12)
#define GET_COLSKIP(val) ((val & GUI_COLSKIP_MASK) >> 12)

/* For defining top and left padding */
#define GUI_LEFTPAD_MASK 0xF0000
#define LEFTPAD(val) (((val>>1) & 0xF) << 16)
#define GET_LEFTPAD(val) ((val & GUI_LEFTPAD_MASK) >> 15)

#define GUI_TOPPAD_MASK 0xF00000
#define TOPPAD(val) (((val>>1) & 0xF) << 20)
#define GET_TOPPAD(val) ((val & GUI_TOPPAD_MASK) >> 19)

/*
 * Forgive me if I was a little lazy in figuring out what these
 * do exactly -- we are considering removing this entire module, so
 * my motivation to dig deeply is pretty low.  (tanabi)
 */
#define GUI_NONL     0x1000000  /* Turn off new lines */
#define GUI_HEXP     0x2000000  /* Turn on hweight whatever that is */
#define GUI_VEXP     0x4000000  /* Turn on vweight whatever that is */
#define GUI_REPORT   0x8000000  /* Turn on report whatever that is  */
#define GUI_REQUIRED 0x8000000  /* Flag as required */

/*
 * Defines the callback arguments and provides a typedef for them.
 *
 * Arguments are:
 *
 * - descriptor
 * - dlogid (dialog ID)
 * - id
 * - event (string)
 * - msg (MCP Message)
 * - did_dismiss boolean
 * - context pointer to whatever helper data a call needs
 */
#define GUI_EVENT_CB_ARGS \
    int   descr,          \
    const char * dlogid,  \
    const char * id,      \
    const char * event,   \
    McpMesg *    msg,     \
    int   did_dismiss,    \
    void* context

typedef void (*Gui_CB) (GUI_EVENT_CB_ARGS);


/*
 * Defines the error callback arguments, and provide a typedef for them.
 *
 * Arguments are:
 * - descriptor
 * - dlogid (dialog ID)
 * - id
 * - error code
 * - error message
 * - context pointer to whatever helper data a call needs
 */
#define GUI_ERROR_CB_ARGS \
    int   descr,          \
    const char * dlogid,  \
    const char * id,      \
    const char * errcode, \
    const char * errtext, \
    void* context

typedef void (*GuiErr_CB) (GUI_ERROR_CB_ARGS);

/*
 * Structure to contain values for GUI dialog elements
 */
typedef struct DlogValue_t {
    struct DlogValue_t *next;   /* Values are in a linked list */
    char *name;                 /* Each has a key */
    int lines;                  /* Number of lines in 'value' */
    char **value;               /* Multiple lines of strings */
} DlogValue;

/*
 * Structure for dialog data
 */
typedef struct DlogData_t {
    struct DlogData_t *next;    /* Dialog data is on a double linked list */
    struct DlogData_t **prev;
    char *id;                   /* ID for this dialog */
    int descr;                  /* Descriptor associated with it */
    int dismissed;              /* Dismissed boolean */
    DlogValue *values;          /* Key/values associated with it */
    Gui_CB callback;            /* Callback function */
    GuiErr_CB error_cb;         /* Error callback function */
    void *context;              /* Context data */
} DlogData;

/**
 * Send a dlog-close command to the given ID
 *
 * @param id the dialog to close
 * @return 0 on success, EGUINODLOG, or EGUINOSUPPORT for not found or supported
 */
int GuiClose(const char *id);

/**
 * Free a dialog struct based on ID
 *
 * Find it, delete it, remove it from the linked lists.
 *
 * @param id the dialog to delete
 * @return either 0 on success or EGUINODLOG if 'id' was not found.
 */
int GuiFree(const char *id);

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
int GuiSetVal(const char *dlogid, const char *id, int lines, const char **value);

/**
 * Send a dlog-show event to the given dialog ID
 *
 * @param id the ID to send the show event to
 * @return 0 on success, EGUINODLOG, or EGUINOSUPPORT for not found or supported
 */
int GuiShow(const char *id);

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
const char *GuiSimple(int descr, const char *title, Gui_CB callback,
                      GuiErr_CB error_cb, void *context);

/**
 * Return boolean true if the given descriptor supports GUI functions
 *
 * @param descr the descriptor to check for GUI support
 * @return boolean true if GUI supported, false otherwise.
 */
int GuiSupported(int descr);

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
const char *GuiValueFirst(const char *dlogid);

/**
 * Get the next value name after value named 'id' for dialog 'dlogid'
 *
 * Returns NULL if dlogid doesn't exist.
 *
 * @param dlogid the dialog ID
 * @param id the id to look for
 * @return the next value's name after 'id' or NULL if not found
 */
const char *GuiValueNext(const char *dlogid, const char *prev);

/**
 * Returns the supported GUI version for a given descriptor
 *
 * If not supported, this will return McpVer { 0, 0 }
 *
 * @param descr the descriptor to get the GUI MCP package version for
 * @return the MCP Version
 */
McpVer GuiVersion(int descr);

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
int gui_ctrl_make_l(const char *dlogid, const char *type, const char *pane,
                    const char *id, const char *text, const char *value,
                    int layout, ...);

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
const char *gui_dlog_alloc(int descr, Gui_CB callback, GuiErr_CB error_cb, void *context);

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
int gui_dlog_closeall_descr(int descr);

/**
 * Get the descriptor associated with a dlogid
 *
 * @param dlogid the dialog ID
 * @return integer descriptor ID or EGUINODLOG if no log ID
 */
int gui_dlog_get_descr(const char *dlogid);

/**
 * Initializes the GUI package with MCP
 *
 * @see mcp_package_register
 */
void gui_initialize(void);

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
const char *gui_value_get(const char *dlogid, const char *id, int line);

/**
 * Get the line count associated with a given dialog and value ID
 *
 * Line count will be 0 if there is no id match.
 *
 * @param dlogid the dialog ID
 * @param id the value ID
 * @return either line count or EGUINODLOG if no dialog match.
 */
int gui_value_linecount(const char *dlogid, const char *id);

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
void gui_value_set_local(const char *dlogid, const char *id, int lines, const char **value);

/**
 * The underpinning function of the gui_dlog_create primitive
 *
 * For a given MCP frame and dialog ID, associate the dialog to the
 * frame.
 *
 * @param fr the frame
 * @param dlogid the dialog ID
 */
void muf_dlog_add(struct frame *fr, const char *dlogid);

/**
 * Purge all dialogs associated with a frame
 *
 * This is used when cleaning up a program to clear all associated
 * dialogs.
 *
 * @param fr the frame to clean up
 */
void muf_dlog_purge(struct frame *fr);

/**
 * The underpinning function of the gui_dlog_close primitive
 *
 * For a given MCP frame and dialog ID, remove the dialog from the
 * list of dialogs on the frame and free the memory.
 *
 * @param fr the frame we are working for
 * @param dlogid the dialog ID
 */
void muf_dlog_remove(struct frame *fr, const char *dlogid);

#endif /* !MCPGUI_H */
#endif /* MCPGUI_SUPPORT */
