#include "config.h"

#ifdef MCP_SUPPORT
#include "commands.h"
#include "db.h"
#include "edit.h"
#include "fbstrings.h"
#include "inst.h"
#include "interface.h"
#include "match.h"
#include "mcp.h"
#include "mcppkg.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif				/* HAVE_MALLOC_H */

#define EMCP_SUCCESS             0      /* successful result */
#define EMCP_NOMCP                      -1      /* MCP isn't supported on this connection. */
#define EMCP_NOPACKAGE          -2      /* Package isn't supported for this connection. */
#define EMCP_ARGCOUNT           -3      /* Too many arguments in mesg. */
#define EMCP_ARGNAMELEN         -5      /* Arg name is too long. */
#define EMCP_MESGSIZE           -6      /* Message is too large. */

#define MAX_MCP_ARGNAME_LEN    30       /* max length of argument name. */
#define MAX_MCP_MESG_ARGS      30       /* max number of args per mesg. */
#define MAX_MCP_MESG_SIZE  262144       /* max mesg size in bytes. */

static McpPkg *mcp_PackageList = NULL;
static McpFrameList *mcp_frame_list;

/*****************************************************************/
/****************                *********************************/
/**************** INTERNAL STUFF *********************************/
/****************                *********************************/
/*****************************************************************/

static int
mcp_intern_is_ident(const char **in, char *buf, int buflen)
{
    int origbuflen = buflen;

    if (!isalpha(**in) && **in != '_')
	return 0;
    while (isalpha(**in) || **in == '_' || isdigit(**in) || **in == '-') {
	if (--buflen > 0) {
	    *buf++ = **in;
	}
	(*in)++;
    }
    if (origbuflen > 0)
	*buf = '\0';
    return 1;
}

static int
mcp_intern_is_simplechar(char in)
{
    if (in == '*' || in == ':' || in == '\\' || in == '"')
	return 0;
    if (!isprint(in) || in == ' ')
	return 0;
    return 1;
}

static int
mcp_intern_is_unquoted(const char **in, char *buf, int buflen)
{
    int origbuflen = buflen;

    if (!mcp_intern_is_simplechar(**in))
	return 0;
    while (mcp_intern_is_simplechar(**in)) {
	if (--buflen > 0) {
	    *buf++ = **in;
	}
	(*in)++;
    }
    if (origbuflen > 0)
	*buf = '\0';
    return 1;
}

static int
mcp_intern_is_quoted(const char **in, char *buf, int buflen)
{
    int origbuflen = buflen;
    const char *old = *in;

    if (**in != '"')
	return 0;
    (*in)++;
    while (**in) {
	if (**in == '\\') {
	    (*in)++;
	    if (**in && --buflen > 0) {
		*buf++ = **in;
	    }
	} else if (**in == '"') {
	    break;
	} else {
	    if (--buflen > 0) {
		*buf++ = **in;
	    }
	}
	(*in)++;
    }
    if (**in != '"') {
	*in = old;
	return 0;
    }
    (*in)++;
    if (origbuflen > 0) {
	*buf = '\0';
    }
    return 1;
}

static int
mcp_intern_is_keyval(McpMesg * msg, const char **in)
{
    char keyname[128];
    char value[BUFFER_LEN];
    const char *old = *in;
    int deferred = 0;

    if (!isspace(**in))
	return 0;
    while (isspace(**in))
	(*in)++;
    if (!mcp_intern_is_ident(in, keyname, sizeof(keyname))) {
	*in = old;
	return 0;
    }
    if (**in == '*') {
	msg->incomplete = 1;
	deferred = 1;
	(*in)++;
    }
    if (**in != ':') {
	*in = old;
	return 0;
    }
    (*in)++;
    if (!isspace(**in)) {
	*in = old;
	return 0;
    }
    while (isspace(**in))
	(*in)++;
    if (!mcp_intern_is_unquoted(in, value, sizeof(value)) &&
	!mcp_intern_is_quoted(in, value, sizeof(value))
	    ) {
	*in = old;
	return 0;
    }

    if (deferred) {
	mcp_mesg_arg_append(msg, keyname, NULL);
    } else {
	mcp_mesg_arg_append(msg, keyname, value);
    }
    return 1;
}

static int
mcp_intern_is_mesg_start(McpFrame * mfr, const char *in)
{
    char mesgname[128];
    char authkey[128];
    char *subname = NULL;
    McpMesg *newmsg = NULL;
    McpPkg *pkg = NULL;
    int longlen = 0;

    if (!mcp_intern_is_ident(&in, mesgname, sizeof(mesgname)))
	return 0;
    if (strcasecmp(mesgname, MCP_INIT_PKG)) {
	if (!isspace(*in))
	    return 0;
	while (isspace(*in))
	    in++;
	if (!mcp_intern_is_unquoted(&in, authkey, sizeof(authkey)))
	    return 0;
	if (strcmp(authkey, mfr->authkey))
	    return 0;
    }

    if (strncasecmp(mesgname, MCP_INIT_PKG, 3)) {
	for (pkg = mfr->packages; pkg; pkg = pkg->next) {
	    int pkgnamelen = strlen(pkg->pkgname);

	    if (!strncasecmp(pkg->pkgname, mesgname, pkgnamelen)) {
		if (mesgname[pkgnamelen] == '\0' || mesgname[pkgnamelen] == '-') {
		    if (pkgnamelen > longlen) {
			longlen = pkgnamelen;
		    }
		}
	    }
	}
    }
    if (!longlen) {
	int neglen = strlen(MCP_NEGOTIATE_PKG);

	if (!strncasecmp(mesgname, MCP_NEGOTIATE_PKG, neglen)) {
	    longlen = neglen;
	} else if (!strcasecmp(mesgname, MCP_INIT_PKG)) {
	    longlen = strlen(mesgname);
	} else {
	    return 0;
	}
    }
    subname = mesgname + longlen;
    if (*subname) {
	*subname++ = '\0';
    }

    newmsg = (McpMesg *) malloc(sizeof(McpMesg));
    mcp_mesg_init(newmsg, mesgname, subname);
    while (*in) {
	if (!mcp_intern_is_keyval(newmsg, &in)) {
	    mcp_mesg_clear(newmsg);
	    free(newmsg);
	    return 0;
	}
    }

    /* Okay, we've recieved a valid message. */
    if (newmsg->incomplete) {
	/* It's incomplete.  Remember it to finish later. */
	const char *msgdt = mcp_mesg_arg_getline(newmsg, MCP_DATATAG, 0);

	newmsg->datatag = string_dup(msgdt);
	mcp_mesg_arg_remove(newmsg, MCP_DATATAG);
	newmsg->next = mfr->messages;
	mfr->messages = newmsg;
    } else {
	/* It's complete.  Execute the callback function for this package. */
	mcp_frame_package_docallback(mfr, newmsg);
	mcp_mesg_clear(newmsg);
	free(newmsg);
    }
    return 1;
}

static int
mcp_intern_is_mesg_cont(McpFrame * mfr, const char *in)
{
    char datatag[128];
    char keyname[128];
    McpMesg *ptr;

    if (*in != '*') {
	return 0;
    }
    in++;
    if (!isspace(*in))
	return 0;
    while (isspace(*in))
	in++;
    if (!mcp_intern_is_unquoted(&in, datatag, sizeof(datatag)))
	return 0;
    if (!isspace(*in))
	return 0;
    while (isspace(*in))
	in++;
    if (!mcp_intern_is_ident(&in, keyname, sizeof(keyname)))
	return 0;
    if (*in != ':')
	return 0;
    in++;
    if (!isspace(*in))
	return 0;
    in++;

    for (ptr = mfr->messages; ptr; ptr = ptr->next) {
	if (!strcmp(datatag, ptr->datatag)) {
	    mcp_mesg_arg_append(ptr, keyname, in);
	    break;
	}
    }
    if (!ptr) {
	return 0;
    }
    return 1;
}

static int
mcp_intern_is_mesg_end(McpFrame * mfr, const char *in)
{
    char datatag[128];
    McpMesg *ptr, **prev;

    if (*in != ':') {
	return 0;
    }
    in++;
    if (!isspace(*in))
	return 0;
    while (isspace(*in))
	in++;
    if (!mcp_intern_is_unquoted(&in, datatag, sizeof(datatag)))
	return 0;
    if (*in)
	return 0;
    prev = &mfr->messages;
    for (ptr = mfr->messages; ptr; ptr = ptr->next, prev = &ptr->next) {
	if (!strcmp(datatag, ptr->datatag)) {
	    *prev = ptr->next;
	    break;
	}
    }
    if (!ptr) {
	return 0;
    }
    ptr->incomplete = 0;
    mcp_frame_package_docallback(mfr, ptr);
    mcp_mesg_clear(ptr);
    free(ptr);

    return 1;
}

static int
mcp_internal_parse(McpFrame * mfr, const char *in)
{
    if (mcp_intern_is_mesg_cont(mfr, in))
	return 1;
    if (mcp_intern_is_mesg_end(mfr, in))
	return 1;
    if (mcp_intern_is_mesg_start(mfr, in))
	return 1;
    return 0;
}

void
clean_mcpbinds(struct mcp_binding *mypub)
{
    struct mcp_binding *tmppub;

    while (mypub) {
	tmppub = mypub->next;
	free(mypub->pkgname);
	free(mypub->msgname);
	free(mypub);
	mypub = tmppub;
    }
}

static void
SendText(McpFrame * mfr, const char *text)
{
    queue_string((struct descriptor_data *) mfr->descriptor, text);
}

static void
FlushText(McpFrame * mfr)
{
    struct descriptor_data *d = (struct descriptor_data *) mfr->descriptor;
    if (d && !process_output(d)) {
	d->booted = 1;
    }
}

int
mcpframe_to_descr(McpFrame * ptr)
{
    return ((struct descriptor_data *) ptr->descriptor)->descriptor;
}

int
mcpframe_to_user(McpFrame * ptr)
{
    return ((struct descriptor_data *) ptr->descriptor)->player;
}

McpFrame *
descr_mcpframe(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);
    if (d) {
	return &d->mcpframe;
    }
    return NULL;
}

/* Used internally to escape and quote argument values. */
static int
msgarg_escape(char *buf, int bufsize, const char *in)
{
    char *out = buf;
    int len = 0;

    if (bufsize < 3) {
	return 0;
    }
    *out++ = '"';
    len++;
    while (*in && len < bufsize - 3) {
	if (*in == '"' || *in == '\\') {
	    *out++ = '\\';
	    len++;
	}
	*out++ = *in++;
	len++;
    }
    *out++ = '"';
    *out = '\0';
    len++;

    return len;
}

/*****************************************************************/
/***                          ************************************/
/*** MCP PACKAGE REGISTRATION ************************************/
/***                          ************************************/
/*****************************************************************/

void
mcp_package_register(const char *pkgname, McpVer minver, McpVer maxver, McpPkg_CB callback,
		     void *context, ContextCleanup_CB cleanup)
{
    McpPkg *nu = (McpPkg *) malloc(sizeof(McpPkg));

    nu->pkgname = string_dup(pkgname);
    nu->minver = minver;
    nu->maxver = maxver;
    nu->callback = callback;
    nu->context = context;
    nu->cleanup = cleanup;

    mcp_package_deregister(pkgname);
    nu->next = mcp_PackageList;
    mcp_PackageList = nu;
    mcp_frame_package_renegotiate(pkgname);
}

void
mcp_package_deregister(const char *pkgname)
{
    McpPkg *ptr = mcp_PackageList;
    McpPkg *prev = NULL;

    while (ptr && !strcasecmp(ptr->pkgname, pkgname)) {
	mcp_PackageList = ptr->next;
	if (ptr->cleanup)
	    ptr->cleanup(ptr->context);
	if (ptr->pkgname)
	    free(ptr->pkgname);
	free(ptr);
	ptr = mcp_PackageList;
    }

    prev = mcp_PackageList;
    if (ptr)
	ptr = ptr->next;

    while (ptr) {
	if (!strcasecmp(pkgname, ptr->pkgname)) {
	    prev->next = ptr->next;
	    if (ptr->cleanup)
		ptr->cleanup(ptr->context);
	    if (ptr->pkgname)
		free(ptr->pkgname);
	    free(ptr);
	    ptr = prev->next;
	} else {
	    prev = ptr;
	    ptr = ptr->next;
	}
    }
    mcp_frame_package_renegotiate(pkgname);
}

/*****************************************************************/
/***                       ***************************************/
/***  MCP PACKAGE HANDLER  ***************************************/
/***                       ***************************************/
/*****************************************************************/

void
mcp_basic_handler(McpFrame * mfr, McpMesg * mesg)
{
    McpVer myminver = { 2, 1 };
    McpVer mymaxver = { 2, 1 };
    McpVer minver = { 0, 0 };
    McpVer maxver = { 0, 0 };
    McpVer nullver = { 0, 0 };
    const char *ptr;
    const char *auth;

    if (!*mesg->mesgname) {
	auth = mcp_mesg_arg_getline(mesg, "authentication-key", 0);
	if (auth) {
	    mfr->authkey = string_dup(auth);
	} else {
	    McpMesg reply;
	    char authval[128];

	    mcp_mesg_init(&reply, MCP_INIT_PKG, "");
	    mcp_mesg_arg_append(&reply, "version", "2.1");
	    mcp_mesg_arg_append(&reply, "to", "2.1");
	    snprintf(authval, sizeof(authval), "%.8lX", (unsigned long) (RANDOM() ^ RANDOM()));
	    mcp_mesg_arg_append(&reply, "authentication-key", authval);
	    mfr->authkey = string_dup(authval);
	    mcp_frame_output_mesg(mfr, &reply);
	    mcp_mesg_clear(&reply);
	}

	ptr = mcp_mesg_arg_getline(mesg, "version", 0);
	if (!ptr)
	    return;
	while (isdigit(*ptr))
	    minver.vermajor = (minver.vermajor * 10) + (*ptr++ - '0');
	if (*ptr++ != '.')
	    return;
	while (isdigit(*ptr))
	    minver.verminor = (minver.verminor * 10) + (*ptr++ - '0');

	ptr = mcp_mesg_arg_getline(mesg, "to", 0);
	if (!ptr) {
	    maxver = minver;
	} else {
	    while (isdigit(*ptr))
		maxver.vermajor = (maxver.vermajor * 10) + (*ptr++ - '0');
	    if (*ptr++ != '.')
		return;
	    while (isdigit(*ptr))
		maxver.verminor = (maxver.verminor * 10) + (*ptr++ - '0');
	}

	mfr->version = mcp_version_select(myminver, mymaxver, minver, maxver);
	if (mcp_version_compare(mfr->version, nullver)) {
	    McpMesg cando;
	    char verbuf[32];
	    McpPkg *p = mcp_PackageList;

	    mfr->enabled = 1;
	    while (p) {
		if (strcasecmp(p->pkgname, MCP_INIT_PKG)) {
		    mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "can");
		    mcp_mesg_arg_append(&cando, "package", p->pkgname);
		    snprintf(verbuf, sizeof(verbuf), "%d.%d", p->minver.vermajor,
			     p->minver.verminor);
		    mcp_mesg_arg_append(&cando, "min-version", verbuf);
		    snprintf(verbuf, sizeof(verbuf), "%d.%d", p->maxver.vermajor,
			     p->maxver.verminor);
		    mcp_mesg_arg_append(&cando, "max-version", verbuf);
		    mcp_frame_output_mesg(mfr, &cando);
		    mcp_mesg_clear(&cando);
		}
		p = p->next;
	    }
	    mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "end");
	    mcp_frame_output_mesg(mfr, &cando);
	    mcp_mesg_clear(&cando);
	}
    }
}

/*****************************************************************/
/***                                 *****************************/
/***  MCP-NEGOTIATE PACKAGE HANDLER  *****************************/
/***                                 *****************************/
/*****************************************************************/

void
mcp_negotiate_handler(McpFrame * mfr, McpMesg * mesg, McpVer version, void *context)
{
    McpVer minver = { 0, 0 };
    McpVer maxver = { 0, 0 };
    const char *ptr;
    const char *pkg;

    if (!strcmp(mesg->mesgname, "can")) {
	pkg = mcp_mesg_arg_getline(mesg, "package", 0);
	if (!pkg)
	    return;
	ptr = mcp_mesg_arg_getline(mesg, "min-version", 0);
	if (!ptr)
	    return;
	while (isdigit(*ptr))
	    minver.vermajor = (minver.vermajor * 10) + (*ptr++ - '0');
	if (*ptr++ != '.')
	    return;
	while (isdigit(*ptr))
	    minver.verminor = (minver.verminor * 10) + (*ptr++ - '0');

	ptr = mcp_mesg_arg_getline(mesg, "max-version", 0);
	if (!ptr) {
	    maxver = minver;
	} else {
	    while (isdigit(*ptr))
		maxver.vermajor = (maxver.vermajor * 10) + (*ptr++ - '0');
	    if (*ptr++ != '.')
		return;
	    while (isdigit(*ptr))
		maxver.verminor = (maxver.verminor * 10) + (*ptr++ - '0');
	}

	mcp_frame_package_add(mfr, pkg, minver, maxver);
    } else if (!strcmp(mesg->mesgname, "end")) {
	/* nothing to do for end of package negotiations. */
    }
}

/*****************************************************************/
/***                    ******************************************/
/*** MCP INITIALIZATION ******************************************/
/***                    ******************************************/
/*****************************************************************/

/*****************************************************************
 *
 *   Initializes MCP globally at startup time.
 *
 *****************************************************************/

void
mcp_initialize(void)
{
    McpVer oneoh = { 1, 0 };
    McpVer twooh = { 2, 0 };

    /* McpVer twoone = {2,1}; */

    mcp_package_register(MCP_NEGOTIATE_PKG, oneoh, twooh, mcp_negotiate_handler, NULL, NULL);
    mcp_package_register("org-fuzzball-help", oneoh, oneoh, mcppkg_help_request, NULL, NULL);
    mcp_package_register("org-fuzzball-notify", oneoh, oneoh, mcppkg_simpleedit, NULL, NULL);
    mcp_package_register("org-fuzzball-simpleedit", oneoh, oneoh, mcppkg_simpleedit, NULL,
			 NULL);
    mcp_package_register("dns-org-mud-moo-simpleedit", oneoh, oneoh, mcppkg_simpleedit, NULL,
			 NULL);
}

/*****************************************************************
 *
 *   Starts MCP negotiations, if any are to be had.
 *
 *****************************************************************/

void
mcp_negotiation_start(McpFrame * mfr)
{
    McpMesg reply;

    mfr->enabled = 1;
    mcp_mesg_init(&reply, MCP_INIT_PKG, "");
    mcp_mesg_arg_append(&reply, "version", "2.1");
    mcp_mesg_arg_append(&reply, "to", "2.1");
    mcp_frame_output_mesg(mfr, &reply);
    mcp_mesg_clear(&reply);
    mfr->enabled = 0;
}

/*****************************************************************
 *
 *   Initializes an McpFrame for a new connection.
 *   You MUST call this to initialize a new McpFrame.
 *   The caller is responsible for the allocation of the McpFrame.
 *
 *****************************************************************/

void
mcp_frame_init(McpFrame * mfr, connection_t con)
{
    McpFrameList *mfrl;

    mfr->descriptor = con;
    mfr->version.verminor = 0;
    mfr->version.vermajor = 0;
    mfr->authkey = NULL;
    mfr->packages = NULL;
    mfr->messages = NULL;
    mfr->enabled = 0;

    mfrl = (McpFrameList *) malloc(sizeof(McpFrameList));
    mfrl->mfr = mfr;
    mfrl->next = mcp_frame_list;
    mcp_frame_list = mfrl;
}

/*****************************************************************
 *
 *   Cleans up an McpFrame for a closing connection.
 *   You MUST call this when you are done using an McpFrame.
 *
 *****************************************************************/

void
mcp_frame_clear(McpFrame * mfr)
{
    McpPkg *tmp = mfr->packages;
    McpMesg *tmp2 = mfr->messages;
    McpFrameList *mfrl = mcp_frame_list;
    McpFrameList *prev;

    if (mfr->authkey) {
	free(mfr->authkey);
	mfr->authkey = NULL;
    }
    while (tmp) {
	mfr->packages = tmp->next;
	if (tmp->pkgname)
	    free(tmp->pkgname);
	free(tmp);
	tmp = mfr->packages;
    }
    while (tmp2) {
	mfr->messages = tmp2->next;
	mcp_mesg_clear(tmp2);
	free(tmp2);
	tmp2 = mfr->messages;
    }

    while (mfrl && mfrl->mfr == mfr) {
	mcp_frame_list = mfrl->next;
	free(mfrl);
	mfrl = mcp_frame_list;
    }
    if (!mcp_frame_list) {
	return;
    }
    prev = mcp_frame_list;
    mfrl = prev->next;
    while (mfrl) {
	if (mfrl->mfr == mfr) {
	    prev->next = mfrl->next;
	    free(mfrl);
	    mfrl = prev->next;
	} else {
	    prev = mfrl;
	    mfrl = mfrl->next;
	}
    }
}

/*****************************************************************
 *
 *   Removes a package from the list of supported packages
 *   for all McpFrames, and initiates renegotiation of that
 *   package.
 *
 *****************************************************************/

void
mcp_frame_package_renegotiate(const char *package)
{
    McpVer nullver = { 0, 0 };
    McpFrameList *mfrl = mcp_frame_list;
    McpFrame *mfr;
    McpMesg cando;
    char verbuf[32];
    McpPkg *p;

    p = mcp_PackageList;
    while (p) {
	if (!strcasecmp(p->pkgname, package)) {
	    break;
	}
	p = p->next;
    }

    if (!p) {
	mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "can");
	mcp_mesg_arg_append(&cando, "package", package);
	mcp_mesg_arg_append(&cando, "min-version", "0.0");
	mcp_mesg_arg_append(&cando, "max-version", "0.0");
    } else {
	mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "can");
	mcp_mesg_arg_append(&cando, "package", p->pkgname);
	snprintf(verbuf, sizeof(verbuf), "%d.%d", p->minver.vermajor, p->minver.verminor);
	mcp_mesg_arg_append(&cando, "min-version", verbuf);
	snprintf(verbuf, sizeof(verbuf), "%d.%d", p->maxver.vermajor, p->maxver.verminor);
	mcp_mesg_arg_append(&cando, "max-version", verbuf);
    }

    while (mfrl) {
	mfr = mfrl->mfr;
	if (mfr->enabled) {
	    if (mcp_version_compare(mfr->version, nullver) > 0) {
		mcp_frame_package_remove(mfr, package);
		mcp_frame_output_mesg(mfr, &cando);
	    }
	}
	mfrl = mfrl->next;
    }
    mcp_mesg_clear(&cando);

    /*
       mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "end");
       mcp_frame_output_mesg(mfr, &cando);
       mcp_mesg_clear(&cando);
     */
}

/*****************************************************************
 *
 *   Attempt to register a package for this connection.
 *   Returns EMCP_SUCCESS if the package was deemed supported.
 *   Returns EMCP_NOMCP if MCP is not supported on this connection.
 *   Returns EMCP_NOPACKAGE if the package versions didn't overlap.
 *
 *****************************************************************/

int
mcp_frame_package_add(McpFrame * mfr, const char *package, McpVer minver, McpVer maxver)
{
    McpPkg *nu;
    McpPkg *ptr;
    McpVer selver;

    if (!mfr->enabled) {
	return EMCP_NOMCP;
    }

    for (ptr = mcp_PackageList; ptr; ptr = ptr->next) {
	if (!strcasecmp(ptr->pkgname, package)) {
	    break;
	}
    }
    if (!ptr) {
	return EMCP_NOPACKAGE;
    }

    mcp_frame_package_remove(mfr, package);

    selver = mcp_version_select(ptr->minver, ptr->maxver, minver, maxver);
    nu = (McpPkg *) malloc(sizeof(McpPkg));
    nu->pkgname = string_dup(ptr->pkgname);
    nu->minver = selver;
    nu->maxver = selver;
    nu->callback = ptr->callback;
    nu->context = ptr->context;
    nu->next = NULL;

    if (!mfr->packages) {
	mfr->packages = nu;
    } else {
	McpPkg *lastpkg = mfr->packages;

	while (lastpkg->next)
	    lastpkg = lastpkg->next;
	lastpkg->next = nu;
    }
    return EMCP_SUCCESS;
}

/*****************************************************************
 *
 *   Removes a package from the list of supported packages
 *   for this McpFrame.
 *
 *****************************************************************/

void
mcp_frame_package_remove(McpFrame * mfr, const char *package)
{
    McpPkg *tmp;
    McpPkg *prev;

    while (mfr->packages && !strcasecmp(mfr->packages->pkgname, package)) {
	tmp = mfr->packages;
	mfr->packages = tmp->next;
	if (tmp->pkgname)
	    free(tmp->pkgname);
	free(tmp);
    }

    prev = mfr->packages;
    while (prev && prev->next) {
	if (!strcasecmp(prev->next->pkgname, package)) {
	    tmp = prev->next;
	    prev->next = tmp->next;
	    if (tmp->pkgname)
		free(tmp->pkgname);
	    free(tmp);
	} else {
	    prev = prev->next;
	}
    }
}

/*****************************************************************
 *
 *   Returns the supported version of the given package.
 *   Returns {0,0} if the package is not supported.
 *
 *****************************************************************/

McpVer
mcp_frame_package_supported(McpFrame * mfr, const char *package)
{
    McpPkg *ptr;
    McpVer errver = { 0, 0 };

    if (!mfr->enabled) {
	return errver;
    }

    for (ptr = mfr->packages; ptr; ptr = ptr->next) {
	if (!strcasecmp(ptr->pkgname, package)) {
	    break;
	}
    }
    if (!ptr) {
	return errver;
    }

    return (ptr->minver);
}

/*****************************************************************
 *
 *   Executes the callback function for the given message.
 *   Returns EMCP_SUCCESS if the call completed successfully.
 *   Returns EMCP_NOMCP if MCP is not supported for that connection.
 *   Returns EMCP_NOPACKAGE if the package is not supported.
 *
 *****************************************************************/

int
mcp_frame_package_docallback(McpFrame * mfr, McpMesg * msg)
{
    McpPkg *ptr = NULL;

    if (!strcasecmp(msg->package, MCP_INIT_PKG)) {
	mcp_basic_handler(mfr, msg);
    } else {
	if (!mfr->enabled) {
	    return EMCP_NOMCP;
	}

	for (ptr = mfr->packages; ptr; ptr = ptr->next) {
	    if (!strcasecmp(ptr->pkgname, msg->package)) {
		break;
	    }
	}
	if (!ptr) {
	    if (!strcasecmp(msg->package, MCP_NEGOTIATE_PKG)) {
		McpVer twooh = { 2, 0 };

		mcp_negotiate_handler(mfr, msg, twooh, NULL);
	    } else {
		return EMCP_NOPACKAGE;
	    }
	} else {
	    ptr->callback(mfr, msg, ptr->maxver, ptr->context);
	}
    }
    return EMCP_SUCCESS;
}

/*****************************************************************
 *
 *   Check a line of input for MCP commands.
 *   Returns 0 if the line was an out-of-band MCP message.
 *   Returns 1 if the line was in-band data.
 *     outbuf will contain the in-band data on return, if any.
 *
 *****************************************************************/

int
mcp_frame_process_input(McpFrame * mfr, const char *linein, char *outbuf, int bufsize)
{
    if (!strncasecmp(linein, MCP_MESG_PREFIX, 3)) {
	/* treat it as an out-of-band message, and parse it. */
	if (mfr->enabled || !strncasecmp(MCP_INIT_MESG, linein + 3, 4)) {
	    if (!mcp_internal_parse(mfr, linein + 3)) {
		strncpy(outbuf, linein, bufsize);
		outbuf[bufsize - 1] = '\0';
		return 1;
	    }
	    return 0;
	} else {
	    strncpy(outbuf, linein, bufsize);
	    outbuf[bufsize - 1] = '\0';
	    return 1;
	}
    } else if (mfr->enabled && !strncasecmp(linein, MCP_QUOTE_PREFIX, 3)) {
	/* It's quoted in-band data.  Strip the quoting. */
	strncpy(outbuf, linein + 3, bufsize);
    } else {
	/* It's in-band data.  Return it raw. */
	strncpy(outbuf, linein, bufsize);
    }
    outbuf[bufsize - 1] = '\0';
    return 1;
}

/*****************************************************************
 *
 *   Sends a string to the given connection, using MCP escaping
 *     if needed and supported.
 *
 *****************************************************************/

void
mcp_frame_output_inband(McpFrame * mfr, const char *lineout)
{
    if (!mfr->enabled ||
	(strncmp(lineout, MCP_MESG_PREFIX, 3) && strncmp(lineout, MCP_QUOTE_PREFIX, 3))
	    ) {
	SendText(mfr, lineout);
    } else {
	SendText(mfr, MCP_QUOTE_PREFIX);
	SendText(mfr, lineout);
    }
    /* SendText(mfr, "\r\n"); */
}

/*****************************************************************
 *
 *   Sends an MCP message to the given connection.
 *   Returns EMCP_SUCCESS if successful.
 *   Returns EMCP_NOMCP if MCP isn't supported on this connection.
 *   Returns EMCP_NOPACKAGE if this connection doesn't support the needed package.
 *
 *****************************************************************/

int
mcp_frame_output_mesg(McpFrame * mfr, McpMesg * msg)
{
    char outbuf[BUFFER_LEN * 2];
    int bufrem = sizeof(outbuf);
    char mesgname[128];
    char datatag[32];
    McpArg *anarg = NULL;
    int mlineflag = 0;
    char *p;
    char *out;
    int flushcount = 8;

    if (!mfr->enabled && strcasecmp(msg->package, MCP_INIT_PKG)) {
	return EMCP_NOMCP;
    }

    /* Create the message name from the package and message subnames */
    if (msg->mesgname && *msg->mesgname) {
	snprintf(mesgname, sizeof(mesgname), "%s-%s", msg->package, msg->mesgname);
    } else {
	snprintf(mesgname, sizeof(mesgname), "%s", msg->package);
    }

    strcpyn(outbuf, sizeof(outbuf), MCP_MESG_PREFIX);
    strcatn(outbuf, sizeof(outbuf), mesgname);
    if (strcasecmp(mesgname, MCP_INIT_PKG)) {
	McpVer nullver = { 0, 0 };

	strcatn(outbuf, sizeof(outbuf), " ");
	strcatn(outbuf, sizeof(outbuf), mfr->authkey);
	if (strcasecmp(msg->package, MCP_NEGOTIATE_PKG)) {
	    McpVer ver = mcp_frame_package_supported(mfr, msg->package);

	    if (!mcp_version_compare(ver, nullver)) {
		return EMCP_NOPACKAGE;
	    }
	}
    }

    /* If the argument lines contain newlines, split them into separate lines. */
    for (anarg = msg->args; anarg; anarg = anarg->next) {
	if (anarg->value) {
	    McpArgPart *ap = anarg->value;

	    while (ap) {
		p = ap->value;
		while (*p) {
		    if (*p == '\n' || *p == '\r') {
			McpArgPart *nu = (McpArgPart *) malloc(sizeof(McpArgPart));

			nu->next = ap->next;
			ap->next = nu;
			*p++ = '\0';
			nu->value = string_dup(p);
			ap->value = (char *) realloc(ap->value, strlen(ap->value) + 1);
			ap = nu;
			p = nu->value;
		    } else {
			p++;
		    }
		}
		ap = ap->next;
	    }
	}
    }

    /* Build the initial message string */
    out = outbuf;
    bufrem = outbuf + sizeof(outbuf) - out;
    for (anarg = msg->args; anarg; anarg = anarg->next) {
	out += strlen(out);
	bufrem = outbuf + sizeof(outbuf) - out;
	if (!anarg->value) {
	    anarg->was_shown = 1;
	    snprintf(out, bufrem, " %s: %s", anarg->name, MCP_ARG_EMPTY);
	    out += strlen(out);
	    bufrem = outbuf + sizeof(outbuf) - out;
	} else {
	    int totlen = strlen(anarg->value->value) + strlen(anarg->name) + 5;

	    if (anarg->value->next || totlen > ((BUFFER_LEN - (out - outbuf)) / 2)) {
		/* Value is multi-line or too long.  Send on separate line(s). */
		mlineflag = 1;
		anarg->was_shown = 0;
		snprintf(out, bufrem, " %s*: %s", anarg->name, MCP_ARG_EMPTY);
	    } else {
		anarg->was_shown = 1;
		snprintf(out, bufrem, " %s: ", anarg->name);
		out += strlen(out);
		bufrem = outbuf + sizeof(outbuf) - out;

		msgarg_escape(out, bufrem, anarg->value->value);
		out += strlen(out);
	    }
	    out += strlen(out);
	    bufrem = outbuf + sizeof(outbuf) - out;
	}
    }

    /* If the message is multi-line, make sure it has a _data-tag field. */
    if (mlineflag) {
	snprintf(datatag, sizeof(datatag), "%.8lX", (unsigned long) (RANDOM() ^ RANDOM()));
	snprintf(out, bufrem, " %s: %s", MCP_DATATAG, datatag);
    }

    /* Send the initial line. */
    SendText(mfr, outbuf);
    SendText(mfr, "\r\n");

    if (mlineflag) {
	/* Start sending arguments whose values weren't already sent. */
	/* This is usually just multi-line argument values. */
	for (anarg = msg->args; anarg; anarg = anarg->next) {
	    if (!anarg->was_shown) {
		McpArgPart *ap = anarg->value;

		while (ap) {
		    *outbuf = '\0';
		    snprintf(outbuf, sizeof(outbuf), "%s* %s %s: %s", MCP_MESG_PREFIX, datatag,
			     anarg->name, ap->value);
		    SendText(mfr, outbuf);
		    SendText(mfr, "\r\n");
		    if (!--flushcount) {
			FlushText(mfr);
			flushcount = 8;
		    }
		    ap = ap->next;
		}
	    }
	}

	/* Let the other side know we're done sending multi-line arg vals. */
	snprintf(outbuf, sizeof(outbuf), "%s: %s", MCP_MESG_PREFIX, datatag);
	SendText(mfr, outbuf);
	SendText(mfr, "\r\n");
    }

    return EMCP_SUCCESS;
}

/*****************************************************************/
/***                   *******************************************/
/***  MESSAGE METHODS  *******************************************/
/***                   *******************************************/
/*****************************************************************/

/*****************************************************************
 *
 *   Initializes an MCP message.
 *   You MUST initialize a message before using it.
 *   You MUST also mcp_mesg_clear() a mesg once you are done using it.
 *
 *****************************************************************/

void
mcp_mesg_init(McpMesg * msg, const char *package, const char *mesgname)
{
    msg->package = string_dup(package);
    msg->mesgname = string_dup(mesgname);
    msg->datatag = NULL;
    msg->args = NULL;
    msg->incomplete = 0;
    msg->bytes = 0;
    msg->next = NULL;
}

/*****************************************************************
 *
 *   Clear the given MCP message.
 *   You MUST clear every message after you are done using it, to
 *     free the memory used by the message.
 *
 *****************************************************************/

void
mcp_mesg_clear(McpMesg * msg)
{
    if (msg->package)
	free(msg->package);
    if (msg->mesgname)
	free(msg->mesgname);
    if (msg->datatag)
	free(msg->datatag);

    while (msg->args) {
	McpArg *tmp = msg->args;

	msg->args = tmp->next;
	if (tmp->name)
	    free(tmp->name);
	while (tmp->value) {
	    McpArgPart *ptr2 = tmp->value;

	    tmp->value = tmp->value->next;
	    if (ptr2->value)
		free(ptr2->value);
	    free(ptr2);
	}
	free(tmp);
    }
    msg->bytes = 0;
}

/*****************************************************************/
/***                  ********************************************/
/*** ARGUMENT METHODS ********************************************/
/***                  ********************************************/
/*****************************************************************/

/*****************************************************************
 *
 *   Returns the count of the number of lines in the given arg of
 *   the given message.
 *
 *****************************************************************/

int
mcp_mesg_arg_linecount(McpMesg * msg, const char *name)
{
    McpArg *ptr = msg->args;
    int cnt = 0;

    while (ptr && strcasecmp(ptr->name, name)) {
	ptr = ptr->next;
    }
    if (ptr) {
	McpArgPart *ptr2 = ptr->value;

	while (ptr2) {
	    ptr2 = ptr2->next;
	    cnt++;
	}
    }
    return cnt;
}

/*****************************************************************
 *
 *   Gets the value of a named argument in the given message.
 *
 *****************************************************************/

char *
mcp_mesg_arg_getline(McpMesg * msg, const char *argname, int linenum)
{
    McpArg *ptr = msg->args;

    while (ptr && strcasecmp(ptr->name, argname)) {
	ptr = ptr->next;
    }
    if (ptr) {
	McpArgPart *ptr2 = ptr->value;

	while (linenum > 0 && ptr2) {
	    ptr2 = ptr2->next;
	    linenum--;
	}
	if (ptr2) {
	    return (ptr2->value);
	}
	return NULL;
    }
    return NULL;
}

/*****************************************************************
 *
 *   Appends to the list value of the named arg in the given mesg.
 *   If that named argument doesn't exist yet, it will be created.
 *   This is used to construct arguments that have lists as values.
 *   Returns the success state of the call.  EMCP_SUCCESS if the
 *   call was successful.  EMCP_ARGCOUNT if this would make too
 *   many arguments in the message.  EMCP_ARGLENGTH is this would
 *   cause an argument to exceed the max allowed number of lines.
 *
 *****************************************************************/

int
mcp_mesg_arg_append(McpMesg * msg, const char *argname, const char *argval)
{
    McpArg *ptr = msg->args;
    int namelen = strlen(argname);
    int vallen = argval ? strlen(argval) : 0;

    if (namelen > MAX_MCP_ARGNAME_LEN) {
	return EMCP_ARGNAMELEN;
    }
    if (vallen + msg->bytes > MAX_MCP_MESG_SIZE) {
	return EMCP_MESGSIZE;
    }
    while (ptr && strcasecmp(ptr->name, argname)) {
	ptr = ptr->next;
    }
    if (!ptr) {
	if (namelen + vallen + msg->bytes > MAX_MCP_MESG_SIZE) {
	    return EMCP_MESGSIZE;
	}
	ptr = (McpArg *) malloc(sizeof(McpArg));
	ptr->name = (char *) malloc(namelen + 1);
	strcpyn(ptr->name, namelen + 1, argname);
	ptr->value = NULL;
	ptr->last = NULL;
	ptr->next = NULL;
	if (!msg->args) {
	    msg->args = ptr;
	} else {
	    int limit = MAX_MCP_MESG_ARGS;
	    McpArg *lastarg = msg->args;

	    while (lastarg->next) {
		if (limit-- <= 0) {
		    free(ptr->name);
		    free(ptr);
		    return EMCP_ARGCOUNT;
		}
		lastarg = lastarg->next;
	    }
	    lastarg->next = ptr;
	}
	msg->bytes += sizeof(McpArg) + namelen + 1;
    }

    if (argval) {
	McpArgPart *nu = (McpArgPart *) malloc(sizeof(McpArgPart));

	nu->value = (char *) malloc(vallen + 1);
	strcpyn(nu->value, vallen + 1, argval);
	nu->next = NULL;

	if (!ptr->last) {
	    ptr->value = ptr->last = nu;
	} else {
	    ptr->last->next = nu;
	    ptr->last = nu;
	}
	msg->bytes += sizeof(McpArgPart) + vallen + 1;
    }
    ptr->was_shown = 0;
    return EMCP_SUCCESS;
}

/*****************************************************************
 *
 *   Removes the named argument from the given message.
 *
 *****************************************************************/

void
mcp_mesg_arg_remove(McpMesg * msg, const char *argname)
{
    McpArg *ptr = msg->args;
    McpArg *prev = NULL;

    while (ptr && !strcasecmp(ptr->name, argname)) {
	msg->args = ptr->next;
	msg->bytes -= sizeof(McpArg);
	if (ptr->name) {
	    free(ptr->name);
	    msg->bytes -= strlen(ptr->name) + 1;
	}
	while (ptr->value) {
	    McpArgPart *ptr2 = ptr->value;

	    ptr->value = ptr->value->next;
	    msg->bytes -= sizeof(McpArgPart);
	    if (ptr2->value) {
		msg->bytes -= strlen(ptr2->value) + 1;
		free(ptr2->value);
	    }
	    free(ptr2);
	}
	free(ptr);
	ptr = msg->args;
    }

    prev = msg->args;
    if (ptr)
	ptr = ptr->next;

    while (ptr) {
	if (!strcasecmp(argname, ptr->name)) {
	    prev->next = ptr->next;
	    msg->bytes -= sizeof(McpArg);
	    if (ptr->name) {
		msg->bytes -= strlen(ptr->name) + 1;
		free(ptr->name);
	    }
	    while (ptr->value) {
		McpArgPart *ptr2 = ptr->value;

		ptr->value = ptr->value->next;
		msg->bytes -= sizeof(McpArgPart);
		if (ptr2->value) {
		    msg->bytes -= strlen(ptr2->value) + 1;
		    free(ptr2->value);
		}
		free(ptr2);
	    }
	    free(ptr);
	    ptr = prev->next;
	} else {
	    prev = ptr;
	    ptr = ptr->next;
	}
    }
}

/*****************************************************************/
/***                 *********************************************/
/*** VERSION METHODS *********************************************/
/***                 *********************************************/
/*****************************************************************/

/*****************************************************************
 *
 *   Compares two McpVer structs.
 *   Results are similar to strcmp():
 *     Returns negative if v1 <  v2
 *     Returns 0 (zero) if v1 == v2
 *     Returns positive if v1 >  v2
 *
 *****************************************************************/

int
mcp_version_compare(McpVer v1, McpVer v2)
{
    if (v1.vermajor != v2.vermajor) {
	return (v1.vermajor - v2.vermajor);
    }
    return (v1.verminor - v2.verminor);
}

/*****************************************************************
 *
 *   Given the min and max package versions supported by a client
 *     and server, this will return the highest version that is
 *     supported by both.
 *   Returns a McpVer of {0, 0} if there is no version overlap.
 *
 *****************************************************************/

McpVer
mcp_version_select(McpVer min1, McpVer max1, McpVer min2, McpVer max2)
{
    McpVer result = { 0, 0 };

    if (mcp_version_compare(min1, max1) > 0) {
	return result;
    }
    if (mcp_version_compare(min2, max2) > 0) {
	return result;
    }
    if (mcp_version_compare(min1, max2) > 0) {
	return result;
    }
    if (mcp_version_compare(min2, max1) > 0) {
	return result;
    }
    if (mcp_version_compare(max1, max2) > 0) {
	return max2;
    } else {
	return max1;
    }
}

static void
mcpedit_program(int descr, dbref player, dbref prog, const char *name)
{
    char namestr[BUFFER_LEN];
    char refstr[BUFFER_LEN];
    struct line *curr;
    McpMesg msg;
    McpFrame *mfr;
    McpVer supp;

    mfr = descr_mcpframe(descr);
    if (!mfr) {
	do_edit(descr, player, name);
	return;
    }

    supp = mcp_frame_package_supported(mfr, "dns-org-mud-moo-simpleedit");

    if (supp.verminor == 0 && supp.vermajor == 0) {
	do_edit(descr, player, name);
	return;
    }

    if ((Typeof(prog) != TYPE_PROGRAM) || !controls(player, prog)) {
	show_mcp_error(mfr, "@mcpedit", "Permission denied!");
	return;
    }
    if (FLAGS(prog) & INTERNAL) {
	show_mcp_error(mfr, "@mcpedit",
		       "Sorry, this program is currently being edited by someone else.  Try again later.");
	return;
    }
    PROGRAM_SET_FIRST(prog, read_program(prog));
    PLAYER_SET_CURR_PROG(player, prog);

    snprintf(refstr, sizeof(refstr), "%d.prog.", prog);
    snprintf(namestr, sizeof(namestr), "a program named %s(%d)", NAME(prog), prog);
    mcp_mesg_init(&msg, "dns-org-mud-moo-simpleedit", "content");
    mcp_mesg_arg_append(&msg, "reference", refstr);
    mcp_mesg_arg_append(&msg, "type", "muf-code");
    mcp_mesg_arg_append(&msg, "name", namestr);
    for (curr = PROGRAM_FIRST(prog); curr; curr = curr->next) {
	mcp_mesg_arg_append(&msg, "content", DoNull(curr->this_line));
    }
    mcp_frame_output_mesg(mfr, &msg);
    mcp_mesg_clear(&msg);

    free_prog_text(PROGRAM_FIRST(prog));
    PROGRAM_SET_FIRST(prog, NULL);
}

void
do_mcpedit(int descr, dbref player, const char *name)
{
    dbref prog;
    struct match_data md;
    McpFrame *mfr;
    McpVer supp;

    if (!*name) {
	notify(player, "No program name given.");
	return;
    }

    mfr = descr_mcpframe(descr);
    if (!mfr) {
	do_edit(descr, player, name);
	return;
    }

    supp = mcp_frame_package_supported(mfr, "dns-org-mud-moo-simpleedit");

    if (supp.verminor == 0 && supp.vermajor == 0) {
	do_edit(descr, player, name);
	return;
    }

    init_match(descr, player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    prog = match_result(&md);
    if (prog == NOTHING) {
	/* FIXME: must arrange this to query user. */
	notify(player, "I don't see that here!");
	return;
    }

    if (prog == AMBIGUOUS) {
	notify(player, "I don't know which one you mean!");
	return;
    }

    mcpedit_program(descr, player, prog, name);
}

void
do_mcpprogram(int descr, dbref player, const char *name)
{
    dbref prog;
    struct match_data md;

    if (!*name) {
	notify(player, "No program name given.");
	return;
    }
    init_match(descr, player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    prog = match_result(&md);

    if (prog == NOTHING) {
	if (!ok_ascii_other(name)) {
	    notify(player, "Program names are limited to 7-bit ASCII.");
	    return;
	}
	if (!ok_name(name)) {
	    notify(player, "That's a strange name for a program!");
	    return;
	}
	prog = create_program(player, name);
	notifyf(player, "Program %s created as #%d.", name, prog);
    } else if (prog == AMBIGUOUS) {
	notify(player, "I don't know which one you mean!");
	return;
    }

    mcpedit_program(descr, player, prog, name);
}
#endif
