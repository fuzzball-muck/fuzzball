#include "config.h"

#ifdef MCP_SUPPORT
#ifndef _MCP_H
#define _MCP_H

/* the type used to specify the connection */
typedef void *connection_t;

#define MCP_MESG_PREFIX		"#$#"
#define MCP_QUOTE_PREFIX	"#$\""

#define MCP_ARG_EMPTY           "\"\""
#define MCP_INIT_PKG            "mcp"
#define MCP_DATATAG                     "_data-tag"
#define MCP_INIT_MESG           "mcp "
#define MCP_NEGOTIATE_PKG       "mcp-negotiate"

struct mcp_binding {
    struct mcp_binding *next;

    char *pkgname;
    char *msgname;
    struct inst *addr;
};

/* This is a convenient struct for dealing with MCP versions. */
typedef struct McpVersion_T {
    unsigned short vermajor;	/* major version number */
    unsigned short verminor;	/* minor version number */
} McpVer;

/* This is one line of a multi-line argument value. */
typedef struct McpArgPart_T {
    struct McpArgPart_T *next;
    char *value;
} McpArgPart;

/* This is one argument of a message. */
typedef struct McpArg_T {
    struct McpArg_T *next;
    char *name;
    McpArgPart *value;
    McpArgPart *last;
    int was_shown;
} McpArg;

/* This is an MCP message. */
typedef struct McpMesg_T {
    struct McpMesg_T *next;
    char *package;
    char *mesgname;
    char *datatag;
    McpArg *args;
    int incomplete;
    int bytes;
} McpMesg;

struct McpFrame_T;
typedef void (*McpPkg_CB) (struct McpFrame_T * mfr,
			   McpMesg * mesg, McpVer version, void *context);

typedef void (*ContextCleanup_CB) (void *context);

/* This is used to keep track of registered packages. */
typedef struct McpPkg_T {
    char *pkgname;		/* Name of the package */
    McpVer minver;		/* min supported version number */
    McpVer maxver;		/* max supported version number */
    McpPkg_CB callback;		/* function to call with mesgs */
    void *context;		/* user defined callback context */
    ContextCleanup_CB cleanup;	/* callback to use to free context */
    struct McpPkg_T *next;
} McpPkg;

/* This keeps connection specific data for MCP. */
typedef struct McpFrame_T {
    void *descriptor;		/* The descriptor to send output to */
    unsigned int enabled;	/* Flag denoting if MCP is enabled. */
    char *authkey;		/* Authorization key. */
    McpVer version;		/* Supported MCP version number. */
    McpPkg *packages;		/* Pkgs supported on this connection. */
    McpMesg *messages;		/* Partial messages, under construction. */
} McpFrame;

struct McpFrameList_t {
    McpFrame *mfr;
    struct McpFrameList_t *next;
};
typedef struct McpFrameList_t McpFrameList;

void clean_mcpbinds(struct mcp_binding *mypub);
McpFrame *descr_mcpframe(int c);
void mcp_frame_clear(McpFrame * mfr);
void mcp_frame_init(McpFrame * mfr, connection_t con);
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
int mcpframe_to_descr(McpFrame * ptr);
int mcpframe_to_user(McpFrame * ptr);
void mcp_initialize(void);
void mcp_negotiation_start(McpFrame * mfr);
void mcp_package_deregister(const char *pkgname);
void mcp_package_register(const char *pkgname, McpVer minver, McpVer maxver,
			  McpPkg_CB callback, void *context, ContextCleanup_CB cleanup);
int mcp_version_compare(McpVer v1, McpVer v2);
McpVer mcp_version_select(McpVer min1, McpVer max1, McpVer min2, McpVer max2);

#endif				/* _MCP_H */
#endif
