/** @file mcp.c
 *
 * Source code for handling MCP (MUD Client Protocol) events.
 * Specification is here: https://www.moo.mud.org/mcp/
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "db.h"
#include "edit.h"
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "match.h"
#include "mcp.h"
#include "mcppkg.h"

#define EMCP_SUCCESS        0   /* successful result */
#define EMCP_NOMCP          -1  /* MCP isn't supported on this connection. */
#define EMCP_NOPACKAGE      -2  /* Package isn't supported for this connection. */
#define EMCP_ARGCOUNT       -3  /* Too many arguments in mesg. */
#define EMCP_ARGNAMELEN     -5  /* Arg name is too long. */
#define EMCP_MESGSIZE       -6  /* Message is too large. */

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

/**
 * Determine if the next series of characters in a buffer is an identifier
 *
 * Given a pointer to a pointer 'in', process whatever resides at 'in'
 * and see if it is an identifier.  If the first character of 'in' is
 * not alphabet character, this will return false and 'buf' will be unmodified.
 *
 * Otherwise, we will advance 'in' forward and copy what we find into
 * 'buf' until we either run out of buffer space (bufsize) or we reach
 * a character that is not part of an identifier.  'in' will be advanced
 * to the end of the identifier even if we run out of buffer.
 *
 * Identifiers may be alphanumeric, and can contain '_' or '-'.  The
 * Identifier must start with either a letter or an _
 *
 * @private
 * @param in a pointer to a pointer string buffer.  This will be modified
 *           to advance the pointer to the character after the end of the
 *           identifier if found.
 * @param buf the buffer to put the identifier in
 * @param buflen the length of 'buf'
 * @return boolean, 1 if identifier is found and loaded into buf, 0 otherwise
 */
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

/**
 * Check to see if a given character is a "simple character".
 *
 * A simple character is defined as a printable character that is not
 * space, *, :, backslash, or "
 *
 * This is used exclusively by mcp_intern_is_unquoted and is basically
 * used to gather string data that is not 'special' and requiring quote
 * processing.
 *
 * @private
 * @param in the character to check
 * @return boolean false if not simple character, true if is.
 */
static int
mcp_intern_is_simplechar(char in)
{
    if (in == '*' || in == ':' || in == '\\' || in == '"')
        return 0;

    if (!isprint(in) || in == ' ')
        return 0;

    return 1;
}

/**
 * Detects and packs an unquoted string into 'buf' from 'in'
 *
 * Takes a pointer to a pointer, 'in', and copies string data into
 * 'buf' until a delimiter character is found.  The delimiter character
 * is defined as 'something that makes mcp_intern_is_simplechar return false'
 * which is a whole set of things.
 *
 * @see mcp_intern_is_simplechar
 *
 * 'in' is advanced to the delimiter character, and 'buf' contains whatever
 * string data is found up to 'buflen' as a limit.
 *
 * @private
 * @param in pointer to pointer of input buffer
 * @param buf the buffer to copy the result into
 * @param buflen the length of buf
 * @return boolean 0 if no unquoted string found, 1 if something was put in buf
 */
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

/**
 * Detects and packs a quoted string into 'buf' from 'in'
 *
 * The initial character should be a ".  It will copy from the character
 * after that initial " on to the character before the terminal " into
 * 'buf' with a proper understanding of backslash escaped quotes.
 *
 * 'in' is advanced to the position immediately after the terminal "
 *
 * If there is no terminal ", or if the current position of 'in' is not
 * a starting ", this returns 0 and in is left unchanged.
 *
 * @private
 * @param in the pointer to pointer of the input buffer
 * @param buf the buffer to write to
 * @param buflen the size of buf
 * @return boolean 1 if a quoted string is found and 'buf' is populated, 0
 *         otherwise.
 */
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

    /* No terminal " */
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

/**
 * Processes 'in' to see if it points to a key-value pair
 *
 * Given the MCP message 'msg' and a pointer to a pointer containing
 * the input buffer, process it to see if there is a key-value pair
 * in it.
 *
 * If there is, the pointer to pointer 'in' will be advanced past the
 * key value pair, and the key/value will be added to msg's arguments.
 *
 * If the key/value pair is a multi-line variable, then a stub is put
 * in the argument list with a value of NULL.  Otherwise, the value is
 * put in.
 *
 * @see mcp_mesg_arg_append
 *
 * If it turns out that 'in' isn't a key value pair, or has a syntax
 * error, the pointer 'in' is reset to wherever it was when the function
 * started and then 0 is returned.  Otherwise, 1 is returned.
 *
 * @private
 * @param msg the MCP message to process
 * @param in pointer to input string
 * @return boolean true if we found a key/value pair, false otherwise
 */
static int
mcp_intern_is_keyval(McpMesg *msg, const char **in)
{
    char keyname[128];
    char value[BUFFER_LEN];
    const char *old = *in;
    int deferred = 0;

    if (!isspace(**in))
        return 0;

    skip_whitespace(in);

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

    skip_whitespace(in);

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

/**
 * Determine if we have the start of a message and potentially dispatch
 *
 * This will take a message in the 'in' buf and determine if it is the
 * start of an MCP message.
 *
 * If it is not, this returns false.
 *
 * If the message is found, and valid, one of two things may happen.
 * If the message is complete, we will do the callback.
 *
 * @see mcp_frame_package_docallback
 *
 * If the message is not complete (multi-line) then we'll flag it as incomplete
 * and try to finish it.
 *
 * In either of those cases, this will return true.
 *
 * @private
 * @param mfr the MCP frame
 * @param in the input character buffer
 * @return boolean true if message was handled, false if not.
 */
static int
mcp_intern_is_mesg_start(McpFrame *mfr, const char *in)
{
    char mesgname[128];
    char authkey[128];
    char *subname = NULL;
    McpMesg *newmsg = NULL;
    size_t longlen = 0;

    if (!mcp_intern_is_ident(&in, mesgname, sizeof(mesgname)))
        return 0;

    if (strcasecmp(mesgname, MCP_INIT_PKG)) {
        if (!isspace(*in))
            return 0;

        skip_whitespace(&in);

        if (!mcp_intern_is_unquoted(&in, authkey, sizeof(authkey)))
            return 0;

        if (strcmp(authkey, mfr->authkey))
            return 0;
    }

    if (strncasecmp(mesgname, MCP_INIT_PKG, 3)) {
        for (McpPkg *pkg = mfr->packages; pkg; pkg = pkg->next) {
            size_t pkgnamelen = strlen(pkg->pkgname);

            if (!strncasecmp(pkg->pkgname, mesgname, pkgnamelen)) {
                if (mesgname[pkgnamelen] == '\0'
                    || mesgname[pkgnamelen] == '-') {
                    if (pkgnamelen > longlen) {
                        longlen = pkgnamelen;
                    }
                }
            }
        }
    }

    if (!longlen) {
        size_t neglen = strlen(MCP_NEGOTIATE_PKG);

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

    newmsg = malloc(sizeof(McpMesg));
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

        newmsg->datatag = strdup(msgdt);
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

/**
 * Is this a continued message?  (i.e. multiline message body)
 *
 * This checks to see if 'in' has a continued multiline message.
 * Returns true if it does, and adds to the message in 'mfr'.
 *
 * @see mcp_mesg_arg_append
 *
 * @private
 * @param mfr the frame we are working with
 * @param in the input buffer to check
 * @return boolean true if we found a message, false if not.
 */
static int
mcp_intern_is_mesg_cont(McpFrame *mfr, const char *in)
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

    skip_whitespace(&in);

    if (!mcp_intern_is_unquoted(&in, datatag, sizeof(datatag)))
        return 0;

    if (!isspace(*in))
        return 0;

    skip_whitespace(&in);

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

/**
 * Is this the end of a multi-line message?
 *
 * If the end of a multi-line message is detected, the callback is
 * done and this returns true.
 *
 * @see mcp_frame_package_docallback
 *
 * If a multi-line message end is not found, this returns false.
 *
 * @private
 * @param mfr our frame
 * @param in the string buffer to check for the terminal message
 * @return boolean true if we've found and processed an end message.
 */
static int
mcp_intern_is_mesg_end(McpFrame *mfr, const char *in)
{
    char datatag[128];
    McpMesg *ptr, **prev;

    if (*in != ':') {
        return 0;
    }

    in++;

    if (!isspace(*in))
        return 0;

    skip_whitespace(&in);

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

/**
 * Process MCP messages in 'in'
 *
 * Returns true if a message was found and processed.  False if not.
 *
 * @private
 * @param mfr the frame we are working with
 * @param in the input buffer to scan over
 * @return boolean true if we processed a message, false if not
 */
static int
mcp_internal_parse(McpFrame *mfr, const char *in)
{
    if (mcp_intern_is_mesg_cont(mfr, in))
        return 1;

    if (mcp_intern_is_mesg_end(mfr, in))
        return 1;

    if (mcp_intern_is_mesg_start(mfr, in))
        return 1;

    return 0;
}

/**
 * Clean up an MCP binding structure, freeing all related memory
 *
 * @param mypub the structure to clean up
 */
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

/**
 * Send some text to a given MCP Frame
 *
 * The send is not immediate, it is queued to the descriptor with queue_write
 *
 * @see queue_write
 *
 * @private
 * @param mfr the frame we're transmitting to
 * @param text the message to send
 */
static void
SendText(McpFrame *mfr, const char *text)
{
    queue_write((struct descriptor_data *) mfr->descriptor, text, strlen(text));
}

/**
 * Flushes the output queue associated with the frame mfr
 *
 * @private
 * @param mfr the frame who's output queue we are flushing
 */
static void
FlushText(McpFrame *mfr)
{
    struct descriptor_data *d = (struct descriptor_data *) mfr->descriptor;

    if (d)
        process_output(d);
}

/**
 * Get an McpFrame from a given descriptor number
 *
 * @param c the descriptor number
 * @return the associated MCP frame or NULL if 'c' was not found.
 */
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

/**
 * Used internally to escape and quote argument values.
 *
 * The string 'in' has all " and \ symbols escaped with \, into
 * the target 'buf'.  If bufsize is less than 3, this won't work.
 * The quoting will continue up to bufsize-3 ... anything past that
 * point will be truncated.
 *
 * @private
 * @param buf the output buffer
 * @param bufsize the output buffer size
 * @param in the input string to escape
 * @return length of string in 'buf', which may be 0
 */
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
void
mcp_package_register(const char *pkgname, McpVer minver, McpVer maxver,
                     McpPkg_CB callback, void *context,
                     ContextCleanup_CB cleanup)
{
    McpPkg *nu = malloc(sizeof(McpPkg));

    nu->pkgname = strdup(pkgname);
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
void
mcp_package_deregister(const char *pkgname)
{
    McpPkg *ptr = mcp_PackageList;
    McpPkg *prev = NULL;

    /*
     * This matches if pkgname is the head of the mcp_PackageList
     */
    while (ptr && !strcasecmp(ptr->pkgname, pkgname)) {
        mcp_PackageList = ptr->next;

        if (ptr->cleanup)
            ptr->cleanup(ptr->context);

        free(ptr->pkgname);
        free(ptr);
        ptr = mcp_PackageList;
    }

    /*
     * And this handles if pkgname is something in the list other than
     * the head.
     */
    prev = mcp_PackageList;

    if (ptr)
        ptr = ptr->next;

    while (ptr) {
        if (!strcasecmp(pkgname, ptr->pkgname)) {
            prev->next = ptr->next;

            if (ptr->cleanup)
                ptr->cleanup(ptr->context);

            free(ptr->pkgname);

            free(ptr);
            ptr = prev->next;
        } else {
            prev = ptr;
            ptr = ptr->next;
        }
    }

    /* Send the renegotiation message */
    mcp_frame_package_renegotiate(pkgname);
}

/*****************************************************************/
/***                       ***************************************/
/***  MCP PACKAGE HANDLER  ***************************************/
/***                       ***************************************/
/*****************************************************************/

/**
 * Handles MCP_INIT_PACKAGE -- basically protocol initialization
 *
 * Does nothing if the message is invalid.
 *
 * @private
 * @param mfr the MCP frame
 * @param mesg the message we are processing
 */
static void
mcp_basic_handler(McpFrame *mfr, McpMesg *mesg)
{
    McpVer myminver = { 2, 1 };
    McpVer mymaxver = { 2, 1 };
    McpVer minver = { 0, 0 };
    McpVer maxver = { 0, 0 };
    McpVer nullver = { 0, 0 };
    const char *ptr;
    const char *auth;

    if (!*mesg->mesgname) {
        /* Figure out authentication key */
        auth = mcp_mesg_arg_getline(mesg, "authentication-key", 0);

        if (auth) {
            mfr->authkey = strdup(auth);
        } else {
            McpMesg reply;
            char authval[128];

            mcp_mesg_init(&reply, MCP_INIT_PKG, "");
            mcp_mesg_arg_append(&reply, "version", "2.1");
            mcp_mesg_arg_append(&reply, "to", "2.1");
            snprintf(authval, sizeof(authval), "%.8lX",
                     (unsigned long) (RANDOM() ^ RANDOM()));
            mcp_mesg_arg_append(&reply, "authentication-key", authval);
            mfr->authkey = strdup(authval);
            mcp_frame_output_mesg(mfr, &reply);
            mcp_mesg_clear(&reply);
        }

        /* Figure out desired version */
        ptr = mcp_mesg_arg_getline(mesg, "version", 0);

        if (!ptr)
            return;

        while (isdigit(*ptr))
            minver.vermajor = (unsigned short)((minver.vermajor * 10)
                              + (*ptr++ - '0'));

        if (*ptr++ != '.')
            return;

        while (isdigit(*ptr))
            minver.verminor = (unsigned short)((minver.verminor * 10)
                              + (*ptr++ - '0'));

        ptr = mcp_mesg_arg_getline(mesg, "to", 0);

        if (!ptr) {
            maxver = minver;
        } else {
            while (isdigit(*ptr))
                maxver.vermajor = (unsigned short)((maxver.vermajor * 10)
                                  + (*ptr++ - '0'));

            if (*ptr++ != '.')
                return;

            while (isdigit(*ptr))
                maxver.verminor = (unsigned short)((maxver.verminor * 10)
                                  + (*ptr++ - '0'));
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
                    snprintf(verbuf, sizeof(verbuf), "%d.%d",
                             p->minver.vermajor, p->minver.verminor);
                    mcp_mesg_arg_append(&cando, "min-version", verbuf);
                    snprintf(verbuf, sizeof(verbuf), "%d.%d",
                             p->maxver.vermajor, p->maxver.verminor);
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

/**
 * Handle the MCP negotiate package and associated messages
 *
 * The only message that really does anything here is 'can', it will
 * add the packages you support to the mfr frame.
 *
 * @private
 * @param mfr the frame we are working with
 * @param mesg the message we are processing
 * @param version our MCP version, ignored
 * @param context not used
 */
static void
mcp_negotiate_handler(McpFrame *mfr, McpMesg *mesg, McpVer version,
                      void *context)
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
            minver.vermajor = (unsigned short)((minver.vermajor * 10)
                                               + (*ptr++ - '0'));

        if (*ptr++ != '.')
            return;

        while (isdigit(*ptr))
            minver.verminor = (unsigned short)((minver.verminor * 10)
                                               + (*ptr++ - '0'));

        ptr = mcp_mesg_arg_getline(mesg, "max-version", 0);

        if (!ptr) {
            maxver = minver;
        } else {
            while (isdigit(*ptr))
                maxver.vermajor = (unsigned short)((maxver.vermajor * 10)
                                                   + (*ptr++ - '0'));

            if (*ptr++ != '.')
                return;

            while (isdigit(*ptr))
                maxver.verminor = (unsigned short)((maxver.verminor * 10)
                                                   + (*ptr++ - '0'));
        }

        mcp_frame_package_add(mfr, pkg, minver, maxver);
    }

    /* nothing to do for end of package negotiations ("end" message). */
}

/*****************************************************************/
/***                    ******************************************/
/*** MCP INITIALIZATION ******************************************/
/***                    ******************************************/
/*****************************************************************/


/**
 * Initialize MCP globall at startup time.
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
void
mcp_initialize(void)
{
    McpVer oneoh = { 1, 0 };
    McpVer twooh = { 2, 0 };

    /* McpVer twoone = {2,1}; */

    mcp_package_register(MCP_NEGOTIATE_PKG, oneoh, twooh,
                         mcp_negotiate_handler, NULL, NULL);
    mcp_package_register("org-fuzzball-help", oneoh, oneoh,
                         mcppkg_help_request, NULL, NULL);
    mcp_package_register("org-fuzzball-notify", oneoh, oneoh,
                         mcppkg_simpleedit, NULL, NULL);
    mcp_package_register("org-fuzzball-simpleedit", oneoh, oneoh,
                         mcppkg_simpleedit, NULL, NULL);
    mcp_package_register("org-fuzzball-languages", oneoh, oneoh,
                         mcppkg_languages, NULL, NULL);
    mcp_package_register("dns-org-mud-moo-simpleedit", oneoh, oneoh,
                         mcppkg_simpleedit, NULL, NULL);
}

/**
 * Starts MCP negotiations, if any are to be had.
 *
 * This displays the package init message when a user connects for instance.
 *
 * @param mfr our MCP Frame
 */
void
mcp_negotiation_start(McpFrame *mfr)
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

/**
 * Initializes an McpFrame for a new connection.
 *
 * You MUST call this to initialize a new McpFrame.  The caller is responsible
 * for the allocation of the McpFrame.
 *
 * @param mfr the allocated McpFrame
 * @param con this will go in mfr->descriptor
 */
void
mcp_frame_init(McpFrame *mfr, void *con)
{
    McpFrameList *mfrl;

    memset(mfr, 0, sizeof(McpFrame));
    mfr->descriptor = con;

    mfrl = malloc(sizeof(McpFrameList));
    mfrl->mfr = mfr;
    mfrl->next = mcp_frame_list;
    mcp_frame_list = mfrl;
}

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
void
mcp_frame_clear(McpFrame *mfr)
{
    McpPkg *tmp = mfr->packages;
    McpMesg *tmp2 = mfr->messages;
    McpFrameList *mfrl = mcp_frame_list;
    McpFrameList *prev;

    /* Clean up auth key */
    free(mfr->authkey);
    mfr->authkey = NULL;

    /* Clean up packages */
    while (tmp) {
        mfr->packages = tmp->next;
        free(tmp->pkgname);
        free(tmp);
        tmp = mfr->packages;
    }

    /* Clean up message s*/
    while (tmp2) {
        mfr->messages = tmp2->next;
        mcp_mesg_clear(tmp2);
        free(tmp2);
        tmp2 = mfr->messages;
    }

    /* Clean up items at the head of the frame list */
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

    /* Clean up items in the rest of the frame list */
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

/**
 * Indicates a package change or removal, and renegotiate
 *
 * This removes the package from all McpFrames and renegotiates the ones
 * that need it.  If the package is removed, it will have max/min version 0.
 *
 * @param package the package to renegotiate 
 */
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

    /* Find the package in the list */
    while (p) {
        if (!strcasecmp(p->pkgname, package)) {
            break;
        }

        p = p->next;
    }

    if (!p) { /* This means we deleted the package */
        mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "can");
        mcp_mesg_arg_append(&cando, "package", package);
        mcp_mesg_arg_append(&cando, "min-version", "0.0");
        mcp_mesg_arg_append(&cando, "max-version", "0.0");
    } else { /* This means the version potentially changed */
        mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "can");
        mcp_mesg_arg_append(&cando, "package", p->pkgname);
        snprintf(verbuf, sizeof(verbuf), "%d.%d", p->minver.vermajor,
                 p->minver.verminor);
        mcp_mesg_arg_append(&cando, "min-version", verbuf);
        snprintf(verbuf, sizeof(verbuf), "%d.%d", p->maxver.vermajor,
                 p->maxver.verminor);
        mcp_mesg_arg_append(&cando, "max-version", verbuf);
    }

    /* Go through the frame list and remove the package as needed */
    while (mfrl) {
        mfr = mfrl->mfr;

        if (mfr->enabled) {
            /* If the version is out of bounds, we'll remove the package */
            if (mcp_version_compare(mfr->version, nullver) > 0) {
                mcp_frame_package_remove(mfr, package);
                mcp_frame_output_mesg(mfr, &cando);
            }
        }

        mfrl = mfrl->next;
    }

    mcp_mesg_clear(&cando);
}

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
int
mcp_frame_package_add(McpFrame *mfr, const char *package, McpVer minver,
                      McpVer maxver)
{
    McpPkg *nu;
    McpPkg *ptr;
    McpVer selver;

    /* If the frame hasn't been initialized, no MCP */
    if (!mfr->enabled) {
        return EMCP_NOMCP;
    }

    /* Find the package */
    for (ptr = mcp_PackageList; ptr; ptr = ptr->next) {
        if (!strcasecmp(ptr->pkgname, package)) {
            break;
        }
    }

    /* Package didn't exist */
    if (!ptr) {
        return EMCP_NOPACKAGE;
    }

    /* Remove it, if it is already on the frame */
    mcp_frame_package_remove(mfr, package);

    /* Add it to the frame */
    selver = mcp_version_select(ptr->minver, ptr->maxver, minver, maxver);
    nu = malloc(sizeof(McpPkg));
    nu->pkgname = strdup(ptr->pkgname);
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

    /* Success! */
    return EMCP_SUCCESS;
}

/**
 * Removes a package from the list of supported packages for this McpFrame.
 *
 * Does exactly what it says it does!  Does nothing if the package is not
 * associated with the frame.
 *
 * @param mfr the frame to remove the package from
 * @param package the name of the package to remove.
 */
void
mcp_frame_package_remove(McpFrame *mfr, const char *package)
{
    McpPkg *tmp;
    McpPkg *prev;

    while (mfr->packages && !strcasecmp(mfr->packages->pkgname, package)) {
        tmp = mfr->packages;

        mfr->packages = tmp->next;
        free(tmp->pkgname);
        free(tmp);
    }

    prev = mfr->packages;

    while (prev && prev->next) {
        if (!strcasecmp(prev->next->pkgname, package)) {
            tmp = prev->next;
            prev->next = tmp->next;
            free(tmp->pkgname);
            free(tmp);
        } else {
            prev = prev->next;
        }
    }
}

/**
 * Returns the supported version of the given package.
 *
 * Returns {0,0} if the package is not supported.
 *
 * @param mfr the frame to check
 * @param package the name of the package to look for
 * @return McpVer structure if found, or {0, 0} if not found.
 */
McpVer
mcp_frame_package_supported(McpFrame *mfr, const char *package)
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
int
mcp_frame_package_docallback(McpFrame *mfr, McpMesg *msg)
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
int
mcp_frame_process_input(McpFrame *mfr, const char *linein, char *outbuf,
                        int bufsize)
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
void
mcp_frame_output_inband(McpFrame *mfr, const char *lineout)
{
    if (!mfr->enabled ||
        (strncmp(lineout, MCP_MESG_PREFIX, 3)
         && strncmp(lineout, MCP_QUOTE_PREFIX, 3))) {
        SendText(mfr, lineout);
    } else {
        SendText(mfr, MCP_QUOTE_PREFIX);
        SendText(mfr, lineout);
    }
}

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
int
mcp_frame_output_mesg(McpFrame *mfr, McpMesg *msg)
{
    char outbuf[BUFFER_LEN * 2];
    int bufrem = sizeof(outbuf);
    char mesgname[128];
    char datatag[32];
    int mlineflag = 0;
    char *p;
    char *out;
    int flushcount = 8;

    if (!mfr->enabled && strcasecmp(msg->package, MCP_INIT_PKG)) {
        return EMCP_NOMCP;
    }

    /* Create the message name from the package and message subnames */
    if (msg->mesgname && *msg->mesgname) {
        snprintf(mesgname, sizeof(mesgname), "%s-%s", msg->package,
                 msg->mesgname);
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

    /*
     * If the argument lines contain newlines, split them into separate lines.
     */
    for (McpArg *anarg = msg->args; anarg; anarg = anarg->next) {
        if (anarg->value) {
            McpArgPart *ap = anarg->value;

            while (ap) {
                p = ap->value;

                while (*p) {
                    if (*p == '\n' || *p == '\r') {
                        McpArgPart *nu = malloc(sizeof(McpArgPart));

                        nu->next = ap->next;
                        ap->next = nu;
                        *p++ = '\0';
                        nu->value = strdup(p);
                        ap->value = realloc(ap->value, strlen(ap->value) + 1);
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

    for (McpArg *anarg = msg->args; anarg; anarg = anarg->next) {
        out += strlen(out);
        bufrem = outbuf + sizeof(outbuf) - out;

        if (!anarg->value) {
            anarg->was_shown = 1;
            snprintf(out, bufrem, " %s: %s", anarg->name, MCP_ARG_EMPTY);
            out += strlen(out);
            bufrem = outbuf + sizeof(outbuf) - out;
        } else {
            int totlen = strlen(anarg->value->value) + strlen(anarg->name) + 5;

            if (anarg->value->next
                || totlen > ((BUFFER_LEN - (out - outbuf)) / 2)) {
                /*
                 * Value is multi-line or too long.  Send on separate line(s).
                 */
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
        snprintf(datatag, sizeof(datatag), "%.8lX",
                 (unsigned long) (RANDOM() ^ RANDOM()));
        snprintf(out, bufrem, " %s: %s", MCP_DATATAG, datatag);
    }

    /* Send the initial line. */
    SendText(mfr, outbuf);
    SendText(mfr, "\r\n");

    if (mlineflag) {
        /*
         * Start sending arguments whose values weren't already sent.
         * This is usually just multi-line argument values.
         */
        for (McpArg *anarg = msg->args; anarg; anarg = anarg->next) {
            if (!anarg->was_shown) {
                McpArgPart *ap = anarg->value;

                while (ap) {
                    *outbuf = '\0';
                    snprintf(outbuf, sizeof(outbuf), "%s* %s %s: %s",
                             MCP_MESG_PREFIX, datatag,
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
void
mcp_mesg_init(McpMesg *msg, const char *package, const char *mesgname)
{
    memset(msg, 0, sizeof(McpMesg));
    msg->package = strdup(package);
    msg->mesgname = strdup(mesgname);
}

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
void
mcp_mesg_clear(McpMesg *msg)
{
    free(msg->package);
    free(msg->mesgname);
    free(msg->datatag);

    while (msg->args) {
        McpArg *tmp = msg->args;

        msg->args = tmp->next;
        free(tmp->name);

        while (tmp->value) {
            McpArgPart *ptr2 = tmp->value;

            tmp->value = tmp->value->next;
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

/**
 * Returns the count of the number of lines in the given arg of the message.
 *
 * If 'name' doesn't exist, this will just return 0
 *
 * @param msg the message to look up line count for
 * @param name the name of the argument to look up the line count for
 */
int
mcp_mesg_arg_linecount(McpMesg *msg, const char *name)
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
char *
mcp_mesg_arg_getline(McpMesg *msg, const char *argname, int linenum)
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
int
mcp_mesg_arg_append(McpMesg *msg, const char *argname, const char *argval)
{
    McpArg *ptr = msg->args;
    size_t namelen = strlen(argname);
    size_t vallen = argval ? strlen(argval) : 0;

    if (namelen > MAX_MCP_ARGNAME_LEN) {
        return EMCP_ARGNAMELEN;
    }

    if (vallen + (size_t)msg->bytes > MAX_MCP_MESG_SIZE) {
        return EMCP_MESGSIZE;
    }

    while (ptr && strcasecmp(ptr->name, argname)) {
        ptr = ptr->next;
    }

    /* Create the argument if it doesn't exist yet. */
    if (!ptr) {
        if (namelen + vallen + (size_t)msg->bytes > MAX_MCP_MESG_SIZE) {
            return EMCP_MESGSIZE;
        }

        ptr = malloc(sizeof(McpArg));
        ptr->name = malloc(namelen + 1);
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

    /* Set the argument value */
    if (argval) {
        McpArgPart *nu = malloc(sizeof(McpArgPart));

        nu->value = malloc(vallen + 1);
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

/**
 * Removes the named argument from the given message.
 *
 * @param msg the message to alter
 * @param argname the argument to remove
 */
void
mcp_mesg_arg_remove(McpMesg *msg, const char *argname)
{
    McpArg *ptr = msg->args;
    McpArg *prev = NULL;

    /*
     * This is to delete the argument if it is at the head
     * of the args list.
     */
    while (ptr && !strcasecmp(ptr->name, argname)) {
        msg->args = ptr->next;
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
        ptr = msg->args;
    }

    prev = msg->args;

    if (ptr)
        ptr = ptr->next;

    /* This deletes any entries if they are after the head of the list. */
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
int
mcp_version_compare(McpVer v1, McpVer v2)
{
    if (v1.vermajor != v2.vermajor) {
        return (v1.vermajor - v2.vermajor);
    }

    return (v1.verminor - v2.verminor);
}

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

/**
 * The common underpinning of do_mcpedit and do_mcpprogram
 *
 * This initializes the MCP editing process with the appropriate MCP
 * calls.  Basically, this does the common permission checking and then
 * dumps the program to the client using MCP.  If MCP is not enabled in
 * the client, it falls back to the standard editor.
 *
 * Saving the changes is done via other MCP calls.
 *
 * @see mcppkg_simpledit
 *
 * @private
 * @param descr descriptor of the person doing the call
 * @param player the person doing the call
 * @param program the ref of the program to edit.
 */
static void
mcpedit_program(int descr, dbref player, dbref program)
{
    char namestr[BUFFER_LEN];
    char refstr[BUFFER_LEN];
    McpMesg msg;
    McpFrame *mfr;
    McpVer supp;

    mfr = descr_mcpframe(descr);

    /* If MCP is not initialized at all, fall back */
    if (!mfr) {
        edit_program(player, program);
        return;
    }

    /* See if simpleedit is supported, if not, fall back */
    supp = mcp_frame_package_supported(mfr, "dns-org-mud-moo-simpleedit");

    if (supp.verminor == 0 && supp.vermajor == 0) {
        edit_program(player, program);
        return;
    }

    /* Basic permission checking */
    if ((Typeof(program) != TYPE_PROGRAM) || !controls(player, program)) {
        show_mcp_error(mfr, "edit program", "Permission denied!");
        return;
    }

    if (FLAGS(program) & INTERNAL) {
        show_mcp_error(mfr, "edit program",
                       "Sorry, this program is currently being edited by someone else.  Try again later.");
        return;
    }

    PROGRAM_SET_FIRST(program, read_program(program));
    PLAYER_SET_CURR_PROG(player, program);
    DBDIRTY(player);

    snprintf(refstr, sizeof(refstr), "%d.prog.", program);
    snprintf(namestr, sizeof(namestr), "a program named %s(%d)", NAME(program), program);

    mcp_mesg_init(&msg, "dns-org-mud-moo-simpleedit", "content");
    mcp_mesg_arg_append(&msg, "reference", refstr);
    mcp_mesg_arg_append(&msg, "type", "muf-code");
    mcp_mesg_arg_append(&msg, "name", namestr);

    /* Dump the program text to the client for editing */
    for (struct line *curr = PROGRAM_FIRST(program); curr; curr = curr->next) {
        mcp_mesg_arg_append(&msg, "content", DoNull(curr->this_line));
    }

    mcp_frame_output_mesg(mfr, &msg);
    mcp_mesg_clear(&msg);

    free_prog_text(PROGRAM_FIRST(program));
    PROGRAM_SET_FIRST(program, NULL);
}

/**
 * Implementation of @mcpedit command
 *
 * The MCP-enabled version of @edit.  The underlying call to the private
 * function mcpedit_program does check permissions.  This is a wrapper to
 * do object matching.  If MCP is not supported by the client, it enters
 * the regular editor.
 *
 * This starts the MCP process and dumps the program text to the client,
 * but there is a separate MCP call to save any changes.
 *
 * @see mcppkg_simpleedit
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param name the program to edit
 */
void
do_mcpedit(int descr, dbref player, const char *name)
{
    dbref program;
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

    if ((program = noisy_match_result(&md)) == NOTHING) {
        return;
    }

    mcpedit_program(descr, player, program);
}

/**
 * Implementation of @mcpprogram command
 *
 * The MCP-enabled version of @program.  It creates the program and then
 * hands off to the private function mcpedit_program to start editing.
 * It does NOT check to see if the user has a MUCKER bit, so do not rely
 * on this for permission checking.
 *
 * This starts the MCP process and dumps the program text to the client,
 * but there is a separate MCP call to save any changes.
 *
 * @see mcppkg_simpleedit
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param name the program to edit
 * @param rname the registration name or "" for none.
 */
void
do_mcpprogram(int descr, dbref player, const char *name, const char *rname)
{
    dbref program;
    struct match_data md;
    char unparse_buf[BUFFER_LEN];

    /* @TODO Should this sanity check be a part of create_program
     *       instead?  A little scary to think we're creating programs
     *       in two places (here and @program) but we're relying on
     *       the sanity checks to be the same -- what if there is
     *       a change to one and we forget to change the other?
     *
     *       We may want to consider moving all similar sanity checks
     *       to the create_* functions -- a quick look suggests that
     *       the create_* functions are "gullible" and accept whatever
     *       is given to them.  This is really hazardous from an API
     *       perspective.
     */

    if (!ok_object_name(name, TYPE_PROGRAM)) {
        notify(player, "Please specify a valid name for this program.");
        return;
    }

    init_match(descr, player, name, TYPE_PROGRAM, &md);

    match_possession(&md);
    match_neighbor(&md);
    match_registered(&md);
    match_absolute(&md);

    if (*rname || (program = match_result(&md)) == NOTHING) {
        program = create_program(player, name);
        unparse_object(player, program, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "Program %s created.", unparse_buf);

        if (*rname) {
            register_object(player, player, REGISTRATION_PROPDIR, (char *)rname, program);
        }
    } else if (program == AMBIGUOUS) {
        notifyf_nolisten(player, match_msg_ambiguous(name, 0));
        return;
    }

    mcpedit_program(descr, player, program);
}
