/** @file mcp.h
 *
 * Header for handling MCP (MUD Client Protocol) events.
 * Specification is here: https://www.moo.mud.org/mcp/
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

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
typedef void (*McpPkg_CB) (struct McpFrame_T *mfr,
			   McpMesg *mesg, McpVer version, void *context);

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

#include "interface.h"

#define MCPFRAME_DESCR(mfr) (((struct descriptor_data *)mfr->descriptor)->descriptor)
#define MCPFRAME_PLAYER(mfr) (((struct descriptor_data *)mfr->descriptor)->player)

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

/**
 * Cleans up an McpFrame for a closing connection.
 *
 * You MUST call this when you are done using an McpFrame.
 *
 * Does not free the 'mfr' itself, just everything associated with it,
 * including removing it from the mcp_frame_list
 *
 * @param mfr the McpFrame to clear
 */
void mcp_frame_clear(McpFrame *mfr);

/**
 * Initializes an McpFrame for a new connection.
 *
 * You MUST call this to initialize a new McpFrame.  The caller is responsible
 * for the allocation of the McpFrame.
 *
 * @param mfr the allocated McpFrame
 * @param con this will go in mfr->descriptor
 */
void mcp_frame_init(McpFrame *mfr, void *con);

/**
 * Sends a string to the given connection, using MCP escaping if needed
 * (and supported).
 *
 * If the frame isn't enabled, or the message is already quoted, it won't
 * be quoted further.
 *
 * @param mfr the frame we are working with
 * @param lineout the line of output to send
 */
void mcp_frame_output_inband(McpFrame *mfr, const char *lineout);

/**
 * Sends an MCP message to the given connection.
 *
 * Returns EMCP_SUCCESS if successful.
 * Returns EMCP_NOMCP if MCP isn't supported on this connection.
 * Returns EMCP_NOPACKAGE if this connection doesn't support the needed package.
 *
 * This automatically handles the multiline / data tag stuff if any of the
 * parameters need multi-line.
 *
 * @param mfr the frame to send the message to
 * @param msg the message to send
 * @return integer with one of the above codes.
 */
int mcp_frame_output_mesg(McpFrame *mfr, McpMesg *msg);

/**
 * Attempt to register a package for this connection.
 *
 * Returns EMCP_SUCCESS if the package was deemed supported.
 * Returns EMCP_NOMCP if MCP is not supported on this connection.
 * Returns EMCP_NOPACKAGE if the package versions didn't overlap.
 *
 * @param mfr the MCP Frame
 * @param package the package name to add
 * @param minver the minimum version
 * @param maxver the maximum version
 * @return integer code as described above
 */
int mcp_frame_package_add(McpFrame *mfr, const char *package, McpVer minver, McpVer maxver);

/**
 * Executes the callback function for the given message.
 *
 * If msg->package matches MCP_INIT_PKG, we'll run the init package.
 * @see mcp_basic_handler
 *
 * Otherwise, it tries to run the package's callback with the given
 * message.
 *
 * Returns EMCP_SUCCESS if the call completed successfully.
 * Returns EMCP_NOMCP if MCP is not supported for that connection.
 * Returns EMCP_NOPACKAGE if the package is not supported.
 *
 * @param mfr the frame we are working on
 * @param msg the message
 * @return integer return parameters as described above.
 */
int mcp_frame_package_docallback(McpFrame *mfr, McpMesg *msg);

/**
 * Removes a package from the list of supported packages for this McpFrame.
 *
 * Does exactly what it says it does!  Does nothing if the package is not
 * associated with the frame.
 *
 * @param mfr the frame to remove the package from
 * @param package the name of the package to remove.
 */
void mcp_frame_package_remove(McpFrame *mfr, const char *package);

/**
 * Indicates a package change or removal, and renegotiate
 *
 * This removes the package from all McpFrames and renegotiates the ones
 * that need it.  If the package is removed, it will have max/min version 0.
 *
 * @param package the package to renegotiate 
 */
void mcp_frame_package_renegotiate(const char *package);

/**
 * Returns the supported version of the given package.
 *
 * Returns {0,0} if the package is not supported.
 *
 * @param mfr the frame to check
 * @param package the name of the package to look for
 * @return McpVer structure if found, or {0, 0} if not found.
 */
McpVer mcp_frame_package_supported(McpFrame *mfr, const char *package);

/**
 * Check a line of input for MCP commands.
 *
 * Returns 0 if the line was an out-of-band MCP message.
 * Returns 1 if the line was in-band data.
 *
 * outbuf will contain the in-band data on return, if any.
 *
 * @param mfr the frame to work with
 * @param linein the line to check for messages
 * @param outbuf the output buffer for in-band data
 * @param bufsize the size of outbuf
 * @return boolean true if the line as in-band data, false if out-of-band MCP
 */
int mcp_frame_process_input(McpFrame *mfr, const char *linein, char *outbuf, int bufsize);

/**
 * Appends to the list value of the named arg in the given mesg.
 *
 * If that named argument doesn't exist yet, it will be created.
 * This is used to construct arguments that have lists as values.
 * Returns the success state of the call.  EMCP_SUCCESS if the
 * call was successful.  EMCP_ARGCOUNT if this would make too
 * many arguments in the message.  EMCP_ARGLENGTH is this would
 * cause an argument to exceed the max allowed number of lines.
 *
 * @param msg the msg to append an argument to
 * @param argname the argument to append
 * @param argval the value to add to the argument
 * @return an integer constant as described above
 */
int mcp_mesg_arg_append(McpMesg *msg, const char *argname, const char *argval);

/**
 * Gets the value of a named argument in the given message.
 *
 * If 'linenum' is out of bounds, or 'argname' doesn't exist, this will
 * return NULL.
 *
 * @param msg the message to get a line for
 * @param argname the argument name to look up
 * @param linenum the line number to fetch, starting with 0
 * @return either the line string, or NULL
 */
char *mcp_mesg_arg_getline(McpMesg *msg, const char *argname, int linenum);

/**
 * Returns the count of the number of lines in the given arg of the message.
 *
 * If 'name' doesn't exist, this will just return 0
 *
 * @param msg the message to look up line count for
 * @param name the name of the argument to look up the line count for
 */
int mcp_mesg_arg_linecount(McpMesg *msg, const char *name);

/**
 * Removes the named argument from the given message.
 *
 * @param msg the message to alter
 * @param argname the argument to remove
 */
void mcp_mesg_arg_remove(McpMesg *msg, const char *argname);

/**
 * Clear the given MCP message.
 *
 * You MUST clear every message after you are done using it, to
 * free the memory used by the message.
 *
 * The message 'msg' is not freed and can be reused, after doing
 * mcp_mesg_init of course.  @see mcp_mesg_init
 *
 * @param msg the message to free up
 */
void mcp_mesg_clear(McpMesg *msg);

/**
 * Initializes an MCP message.
 *
 * You MUST initialize a message before using it.
 * You MUST also mcp_mesg_clear() a mesg once you are done using it.
 *
 * 'package' and 'mesgname' will be copied.
 *
 * @param msg the message to initialize
 * @param package the package the message will belong to
 * @param mesgname the name of the message
 */
void mcp_mesg_init(McpMesg *msg, const char *package, const char *mesgname);

/**
 * Initialize MCP
 *
 * Loads up the default MCP packages, which includes*
 *
 * - negotiate
 * - org-fuzzball-help
 * - org-fuzzball-notify
 * - org-fuzzball-simpleedit
 * - org-fuzzball-languages
 * - dns-org-mud-moo-simpleedit
 */
void mcp_initialize(void);

/**
 * Starts MCP negotiations, if any are to be had.
 *
 * This displays the package init message when a user connects for instance.
 *
 * @param mfr our MCP Frame
 */
void mcp_negotiation_start(McpFrame *mfr);

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

/**
 * Compares two McpVer structs.
 *
 * Results are similar to strcmp():
 *
 * * Returns negative if v1 <  v2
 * * Returns 0 (zero) if v1 == v2
 * * Returns positive if v1 >  v2
 *
 * @param v1 the first operand to compare
 * @param v2 the second operand to compare
 * @return comparison integer value as described above.
 */
int mcp_version_compare(McpVer v1, McpVer v2);

/**
 * Pick the highest common version number for two packages.
 *
 * Given the min and max package versions supported by a client
 * and server, this will return the highest version that is
 * supported by both.
 *
 * Returns a McpVer of {0, 0} if there is no version overlap.
 *
 * @param min1 the minimum version of the first package.
 * @param max1 the maximum version of the first package.
 * @param min2 the minimum version of the second package.
 * @param max2 the maximum version of the second package.
 * @return either McpVer {0, 0} if no version, otherwise the overlapped version
 */
McpVer mcp_version_select(McpVer min1, McpVer max1, McpVer min2, McpVer max2);

#endif /* !MCP_H */
