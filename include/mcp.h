/** @file mcp.h
 *
 * Header for handling MCP (MUD Client Protocol) events.
 * Specification is here: https://www.moo.mud.org/mcp/
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#ifdef MCP_SUPPORT
#ifndef MCP_H
#define MCP_H

/*
 * MCP messages are prefixed with these special tags.
 */
#define MCP_MESG_PREFIX     "#$#"
#define MCP_QUOTE_PREFIX    "#$\""

#define MCP_ARG_EMPTY       "\"\""              /* Empty argument string     */
#define MCP_INIT_PKG        "mcp"               /* MCP initial package name  */
#define MCP_DATATAG         "_data-tag"         /* For multi-line messages   */
#define MCP_INIT_MESG       "mcp "              /* Prefix for out of bound   */
#define MCP_NEGOTIATE_PKG   "mcp-negotiate"     /* Name of negotiate package */

/*
 * This structure handles binding MCP packages to callback functions.
 */
struct mcp_binding {
    struct mcp_binding *next;   /* Next on the linked list   */

    char *pkgname;              /* Name of MCP package       */
    char *msgname;              /* Name of MCP message       */
    struct inst *addr;          /* Link to callback function */
};

/* This is a convenient struct for dealing with MCP versions. */
typedef struct McpVersion_T {
    unsigned short vermajor;    /* major version number */
    unsigned short verminor;    /* minor version number */
} McpVer;

/* This is one line of a multi-line argument value. */
typedef struct McpArgPart_T {
    struct McpArgPart_T *next;  /* Next on linked list    */
    char *value;                /* The value of this line */
} McpArgPart;

/* This is one argument of a message. */
typedef struct McpArg_T {
    struct McpArg_T *next;      /* Next on linked list                */
    char *name;                 /* Name of this key part              */
    McpArgPart *value;          /* Value assoicated with key          */
    McpArgPart *last;           /* Pointer to last item on list       */
    int was_shown;              /* Has to do with multi-line messages */
} McpArg;

/* This is an MCP message. */
typedef struct McpMesg_T {
    struct McpMesg_T *next;     /* Linked list for messages         */
    char *package;              /* The MCP package for this message */
    char *mesgname;             /* The message for this package     */
    char *datatag;              /* Data-tag for multi-line mode     */
    McpArg *args;               /* Argument key-value pairs         */
    int incomplete;             /* Is message incomplete?           */
    int bytes;                  /* Size of message                  */
} McpMesg;

struct McpFrame_T;

/* Type definition for MCP callback function */
typedef void (*McpPkg_CB) (struct McpFrame_T * mfr,
			   McpMesg * mesg, McpVer version, void *context);

/* Type definition for MCP context cleanup callback function */
typedef void (*ContextCleanup_CB) (void *context);

/* This is used to keep track of registered packages. */
typedef struct McpPkg_T {
    char *pkgname;              /* Name of the package             */
    McpVer minver;              /* min supported version number    */
    McpVer maxver;              /* max supported version number    */
    McpPkg_CB callback;         /* function to call with mesgs     */
    void *context;              /* user defined callback context   */
    ContextCleanup_CB cleanup;  /* callback to use to free context */
    struct McpPkg_T *next;      /* Next package in the list        */
} McpPkg;

/* This keeps connection specific data for MCP. */
typedef struct McpFrame_T {
    void *descriptor;       /* The descriptor to send output to */
    unsigned int enabled;   /* Flag denoting if MCP is enabled. */
    char *authkey;          /* Authorization key. */
    McpVer version;         /* Supported MCP version number. */
    McpPkg *packages;       /* Pkgs supported on this connection. */
    McpMesg *messages;      /* Partial messages, under construction. */
} McpFrame;

struct McpFrameList_t {
    McpFrame *mfr;                  /* MCP Frame          */
    struct McpFrameList_t *next;    /* Next frame in list */
};

typedef struct McpFrameList_t McpFrameList;

/**
 * Clean up an MCP binding structure, freeing all related memory
 *
 * @param mypub the structure to clean up
 */
void clean_mcpbinds(struct mcp_binding *mypub);

/**
 * Get an McpFrame from a given descriptor number
 *
 * @param c the descriptor number
 * @return the associated MCP frame or NULL if 'c' was not found.
 */
McpFrame *descr_mcpframe(int c);
void mcp_frame_clear(McpFrame * mfr);
void mcp_frame_init(McpFrame * mfr, void * con);
void mcp_frame_output_inband(McpFrame * mfr, const char *lineout);
int mcp_frame_output_mesg(McpFrame * mfr, McpMesg * msg);
int mcp_frame_package_add(McpFrame * mfr, const char *package, McpVer minver, McpVer maxver);
int mcp_frame_package_docallback(McpFrame * mfr, McpMesg * msg);
void mcp_frame_package_remove(McpFrame * mfr, const char *package);
void mcp_frame_package_renegotiate(const char *package);
McpVer mcp_frame_package_supported(McpFrame * mfr, const char *package);
int mcp_frame_process_input(McpFrame * mfr, const char *linein, char *outbuf, int bufsize);
int mcp_mesg_arg_append(McpMesg * msg, const char *argname, const char *argval);
char *mcp_mesg_arg_getline(McpMesg * msg, const char *argname, int linenum);
int mcp_mesg_arg_linecount(McpMesg * msg, const char *name);
void mcp_mesg_arg_remove(McpMesg * msg, const char *argname);
void mcp_mesg_clear(McpMesg * msg);
void mcp_mesg_init(McpMesg * msg, const char *package, const char *mesgname);

/**
 * Get the descriptor associated with an MCP frame
 *
 * @param ptr the frame to get the descriptor from
 * @return the descriptor, which may be 0 if there is none
 */
int mcpframe_to_descr(McpFrame * ptr);

/**
 * Get the player reference associated with an MCP frame
 *
 * @param ptr the frame to get the descriptor from
 * @return the player dbref, which may be NOTHING is there is none.
 */
int mcpframe_to_user(McpFrame * ptr);
void mcp_initialize(void);
void mcp_negotiation_start(McpFrame * mfr);

/**
 * Deregister and cleanup the given package
 *
 * This cleans up the memory around the package, and will run the cleanup
 * callback if it was set.  After it is done, a package renegotiation is
 * done.
 *
 * @see mcp_frame_package_renegotiate
 *
 * @param pkgname the package name to deregister
 */
void mcp_package_deregister(const char *pkgname);

/**
 * Register a new MCP package
 *
 * Takes a package name, version information, a callback, a context, and
 * a cleanup callback and registers the package for use.
 *
 * The cleanup callback is to clean up 'context'.  Both context and cleanup
 * can be NULL if they are not needed.  The context is just some data available
 * to the callback -- right now it is used mostly by MUF registered MCP
 * calls, and is the pointer to the MUF function to call.
 *
 * @param pkgname the MCP package name
 * @param minver the minimum version
 * @param maxver the maximum version
 * @param callback the callback that is called when the package is run
 * @param context data which is accessible to the callback
 * @param cleanup a callback to clean up context
 */
void mcp_package_register(const char *pkgname, McpVer minver, McpVer maxver,
                          McpPkg_CB callback, void *context,
                          ContextCleanup_CB cleanup);
int mcp_version_compare(McpVer v1, McpVer v2);
McpVer mcp_version_select(McpVer min1, McpVer max1, McpVer min2, McpVer max2);

#endif /* !MCP_H */
#endif /* MCP_SUPPORT */
