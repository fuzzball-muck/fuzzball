/**@file mfuns.c
 *
 * Implementation for some of the MPI calls declared in mfun.h -- the rest
 * are in mfuns2.c
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "db.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "mfun.h"
#include "mpi.h"
#include "props.h"
#include "tune.h"

/**
 * Implementation of the MPI 'func' Function
 *
 * This function allows the definition of a function in MPI and is the
 * equivalent of defining a msgmac.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_func(MFUNARGS)
{
    char *funcname;
    char *ptr = NULL, *def;
    char namebuf[BUFFER_LEN];
    char argbuf[BUFFER_LEN];
    char defbuf[BUFFER_LEN];
    char defbuf_copy[BUFFER_LEN];
    int i;

    /* Figure out the function name */
    funcname = MesgParse(argv[0], namebuf, sizeof(namebuf));
    CHECKRETURN(funcname, "FUNC", "name argument (1)");

    def = argv[argc - 1];

    /*
     * Iterate over the middle parameters -- these are the names of the
     * variables used in the function.
     */
    for (i = 1; i < argc - 1; i++) {
        ptr = MesgParse(argv[i], argbuf, sizeof(argbuf));
        CHECKRETURN(ptr, "FUNC", "variable name argument");
        strcpyn(defbuf_copy, sizeof(defbuf_copy), i == 1 ? def : defbuf);
        snprintf(defbuf, sizeof(defbuf), "{with:%.*s,{:%d},%.*s}",
                 MAX_MFUN_NAME_LEN, ptr, i,
                 (BUFFER_LEN - MAX_MFUN_NAME_LEN - 20), defbuf_copy);
    }

    /* Create the new function */
    i = new_mfunc(funcname, defbuf);

    if (i == 1)
        ABORT_MPI("FUNC", "Function Name too long.");

    if (i == 2)
        ABORT_MPI("FUNC", "Too many functions defined.");

    return "";
}


/**
 * MPI function to return the MUCK name.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_muckname(MFUNARGS)
{
    return tp_muckname;
}


/**
 * MPI function to return the MUCK version.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_version(MFUNARGS)
{
    return VERSION;
}


/**
 * MPI function to fetch a prop off an object.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  This will search the environment for
 * the property.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_prop(MFUNARGS)
{
    dbref obj = what;
    const char *ptr, *pname;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("PROP", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("PROP", "Permission denied.");

    ptr = safegetprop(player, obj, perms, pname, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("PROP", "Failed read.");

    return ptr;
}

/**
 * MPI function to fetch a prop off an object without scanning environment.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  This will only look at either the trigger
 * object or the second parameter and not look up the environment chain.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_propbang(MFUNARGS)
{
    dbref obj = what;
    const char *ptr, *pname;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("PROP!", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("PROP!", "Permission denied.");

    ptr = safegetprop_strict(player, obj, perms, pname, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("PROP!", "Failed read.");

    return ptr;
}


/**
 * MPI function to store a property on an object.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is the property value and is also required.  The third parameter is
 * optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_store(MFUNARGS)
{
    dbref obj = what;
    char *ptr, *pname;

    pname = argv[1];
    ptr = argv[0];

    if (argc > 2) {
        obj = mesg_dbref_strict(descr, player, what, perms, argv[2], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("STORE", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("STORE", "Permission denied.");

    if (!safeputprop(obj, pname, ptr, mesgtyp))
        ABORT_MPI("STORE", "Permission denied.");

    return ptr;
}


/**
 * MPI function to bless a property on an object.
 *
 * Blessed props are basically wizard MPI props.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_bless(MFUNARGS)
{
    dbref obj = what;
    char *pname;

    pname = argv[0];

    if (argc > 1) {
        obj = mesg_dbref_strict(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("BLESS", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("BLESS", "Permission denied.");

    if (!safeblessprop(obj, pname, mesgtyp, 1))
        ABORT_MPI("BLESS", "Permission denied.");

    return "";
}


/**
 * MPI function to unbless a property on an object.
 *
 * Blessed props are basically wizard MPI props.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_unbless(MFUNARGS)
{
    dbref obj = what;
    char *pname;

    pname = argv[0];

    if (argc > 1) {
        obj = mesg_dbref_strict(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("UNBLESS", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("UNBLESS", "Permission denied.");

    if (!safeblessprop(obj, pname, mesgtyp, 0))
        ABORT_MPI("UNBLESS", "Permission denied.");

    return "";
}


/**
 * MPI function to delete a property on an object.
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_delprop(MFUNARGS)
{
    dbref obj = what;
    char *pname;

    pname = argv[0];

    if (argc > 1) {
        obj = mesg_dbref_strict(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("DELPROP", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("DELPROP", "Permission denied.");

    if (!safeputprop(obj, pname, NULL, mesgtyp))
        ABORT_MPI("DELPROP", "Permission denied.");

    return "";
}


/**
 * MPI function to evaluate the contents of a given property
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' and looks along
 * the environment for the prop if not found.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_exec(MFUNARGS)
{
    dbref trg, obj = what;
    const char *ptr, *pname;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("EXEC", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("EXEC", "Permission denied.");

    while (*pname == PROPDIR_DELIMITER)
        pname++;

    ptr = safegetprop(player, obj, perms, pname, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("EXEC", "Failed read.");

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname)
        || Prop_Hidden(pname))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "EXEC", "propval");

    return ptr;
}


/**
 * MPI function to evaluate the contents of a given property
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' but this version
 * does not scan the environment.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_execbang(MFUNARGS)
{
    dbref trg = (dbref) 0, obj = what;
    const char *ptr, *pname;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("EXEC!", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("EXEC!", "Permission denied.");

    while (*pname == PROPDIR_DELIMITER)
        pname++;

    ptr = safegetprop_strict(player, obj, perms, pname, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("EXEC!", "Failed read.");

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname)
        || Prop_Hidden(pname))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "EXEC!", "propval");

    return ptr;
}


/**
 * MPI function to execute the contents of the property in the property
 *
 * This is functionally the same as {exec:{prop:propname}}
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' and will scan the
 * environment for both the property and the property referred to by the
 * property.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_index(MFUNARGS)
{
    dbref trg = (dbref) 0, obj = what;
    dbref tmpobj = (dbref) 0;
    const char *pname, *ptr;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("INDEX", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("INDEX", "Permission denied.");

    tmpobj = obj;
    ptr = safegetprop(player, obj, perms, pname, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("INDEX", "Failed read.");

    if (!*ptr)
        return "";

    obj = tmpobj;
    ptr = safegetprop(player, obj, perms, ptr, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("INDEX", "Failed read.");

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr)
        || Prop_Hidden(ptr))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "INDEX", "listval");

    return ptr;
}



/**
 * MPI function to execute the contents of the property in the property
 *
 * This is functionally the same as {exec!:{prop:propname}}
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what' but this version
 * does not scan the environment for the properties.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_indexbang(MFUNARGS)
{
    dbref trg = (dbref) 0, obj = what;
    dbref tmpobj = (dbref) 0;
    const char *pname, *ptr;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("INDEX!", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("INDEX!", "Permission denied.");

    tmpobj = obj;
    ptr = safegetprop_strict(player, obj, perms, pname, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("INDEX!", "Failed read.");

    if (!*ptr)
        return "";

    obj = tmpobj;
    ptr = safegetprop_strict(player, obj, perms, ptr, mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("INDEX!", "Failed read.");

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr)
        || Prop_Hidden(ptr))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "INDEX!", "listval");

    return ptr;
}



/**
 * MPI function that returns true if the given prop name is a prop directory
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_propdir(MFUNARGS)
{
    dbref obj = what;
    const char *pname;

    pname = argv[0];

    /*
     * TODO: So a lot of these functions very, very similar argument
     * parsing routines where we say 'if argc...' then check the arguments.
     *
     * We can probably condense these into a common call or #define to
     * sorta centralize logic.
     */
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("PROPDIR", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("PROPDIR", "Permission denied.");

    if (is_propdir(obj, pname)) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns a list of all properties in a propdir
 *
 * First MPI argument is the prop name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'.  There is an
 * optional third parameter for pattern matching.
 *
 * The return value is a delimited string, delimited by \r, which is an
 * MPI list.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_listprops(MFUNARGS)
{
    dbref obj = what;
    char *ptr, *pname;
    char *endbuf, *pattern;
    char tmpbuf[BUFFER_LEN];
    char patbuf[BUFFER_LEN];
    char pnamebuf[BUFFER_LEN];
    int flag;

    strcpyn(pnamebuf, sizeof(pnamebuf), argv[0]);
    pname = pnamebuf;

    if (argc > 1) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LISTPROPS", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("LISTPROPS", "Permission denied.");

    if (argc > 2) {
        pattern = argv[2];
    } else {
        pattern = NULL;
    }

    endbuf = pname + strlen(pname);

    if (endbuf != pname) {
        endbuf--;
    }

    if (*endbuf != PROPDIR_DELIMITER && (endbuf - pname) < (BUFFER_LEN - 2)) {
        if (*endbuf != '\0')
            endbuf++;

        *endbuf++ = PROPDIR_DELIMITER;
        *endbuf++ = '\0';
    }

    *buf = '\0';
    endbuf = buf;

    /* Iterate over the props */
    do {
        ptr = next_prop_name(obj, tmpbuf, (int) sizeof(tmpbuf), pname);

        if (ptr && *ptr) {
            flag = 1; /* If this is true, add the prop to the list */

            /*
             * Don't show system props or hidden props based on permissions
             */
            if (Prop_System(ptr)) {
                flag = 0;
            } else if (!(mesgtyp & MPI_ISBLESSED)) {
                if (Prop_Hidden(ptr)) {
                    flag = 0;
                }

                if (Prop_Private(ptr) && OWNER(what) != OWNER(obj)) {
                    flag = 0;
                }

                if (obj != player && OWNER(obj) != OWNER(what)) {
                    flag = 0;
                }
            }

            /* This section does the pattern matching, if pattern provided */
            if ((flag != 0) && (pattern != NULL)) {
                char *nptr;

                nptr = strrchr(ptr, PROPDIR_DELIMITER);

                if (nptr && *nptr) {
                    strcpyn(patbuf, sizeof(patbuf), ++nptr);

                    if (!equalstr(pattern, patbuf)) {
                        flag = 0;
                    }
                }
            }

            if (flag) { /* If we made it this far, add it to the list */
                int entrylen = strlen(ptr);

                if ((endbuf - buf) + entrylen + 2 < BUFFER_LEN) {
                    if (*buf != '\0') {
                        *endbuf++ = '\r';
                    }

                    strcpyn(endbuf, BUFFER_LEN - (size_t)(endbuf - buf), ptr);
                    endbuf += entrylen;
                }
            }
        }

        pname = ptr;
    } while (ptr && *ptr);

    return buf;
}


/**
 * MPI function that concatinates the lines of a property list
 *
 * First MPI argument is the list name and is required.  The second parameter
 * is optional and can be a dbref.  It defaults to 'what'.  If there is a
 * # symbol at the end of the list name it will be removed.
 *
 * The return value is a delimited string, delimited by \r, which is an
 * MPI list.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_concat(MFUNARGS)
{
    dbref obj = what;
    char *pname;
    const char *ptr;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == PERMDENIED)
        ABORT_MPI("CONCAT", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("CONCAT", "Match failed.");

    ptr = get_concat_list(what, perms, obj, (char *) pname, buf, BUFFER_LEN, 1,
                          mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("CONCAT", "Failed list read.");

    return ptr;
}


/**
 * MPI function that picks a value of a single list item from a sparse list
 *
 * A sparse list is one that doesn't have a sequential number of properties.
 * The item chosen is the largest one that is less than or equal to the
 * given value.
 *
 * First MPI argument is the line number value, and is required along with
 * the second argument which is list name.  The third argument is an
 * optional object, that defaults to 'how'.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_select(MFUNARGS)
{
    char origprop[BUFFER_LEN];
    char propname[BUFFER_LEN];
    char bestname[BUFFER_LEN];
    dbref obj = what;
    dbref bestobj = 0;
    char *pname;
    const char *ptr;
    char *out, *in;
    int i, targval, bestval;
    int baselen;
    int limit;
    int blessed = 0;

    pname = argv[1];

    if (argc == 3) {
        obj = mesg_dbref(descr, player, what, perms, argv[2], mesgtyp);
    }

    if (obj == PERMDENIED)
        ABORT_MPI("SELECT", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("SELECT", "Match failed.");

    /*
     * Search contiguously for a bit, looking for a best match.
     * This allows fast hits on LARGE lists.
     */
    limit = 18;

    i = targval = atoi(argv[0]);

    do {
        ptr = get_list_item(player, obj, perms, (char *) pname, i--, mesgtyp, &blessed);
    } while (limit-- > 0 && i >= 0 && ptr && !*ptr);

    if (ptr == NULL)
        ABORT_MPI("SELECT", "Failed list read.");

    if (*ptr != '\0')
        return ptr;

    /*
     * If we didn't find it before, search only existing props.
     * This gets fast hits on very SPARSE lists.
     */

    /* First, normalize the base propname */
    out = origprop;
    in = argv[1];

    while (*in != '\0') {
        *out++ = PROPDIR_DELIMITER;

        while (*in == PROPDIR_DELIMITER)
            in++;

        while (*in && *in != PROPDIR_DELIMITER)
            *out++ = *in++;
    }

    *out++ = '\0';

    bestname[0] = '\0';
    bestval = 0;
    baselen = strlen(origprop);

    /*
     * Loop through the possible props.  Find the bestname, which will be
     * either the best prop for the job or an empty string as the case
     * may be.
     */
    for (; obj != NOTHING; obj = getparent(obj)) {
        pname = next_prop_name(obj, propname, sizeof(propname), origprop);

        while (pname && string_prefix(pname, origprop)) {
            ptr = pname + baselen;

            if (*ptr == NUMBER_TOKEN)
                ptr++;

            if (!*ptr && is_propdir(obj, pname)) {
                char propname2[BUFFER_LEN];
                char *pname2;
                int sublen = strlen(pname);

                pname2 = strcpyn(propname2, sizeof(propname2), pname);
                propname2[sublen++] = PROPDIR_DELIMITER;
                propname2[sublen] = '\0';

                pname2 = next_prop_name(obj, propname2, sizeof(propname2), pname2);

                while (pname2) {
                    ptr = pname2 + sublen;

                    if (number(ptr)) {
                        i = atoi(ptr);

                        /* This is a condition for a potential bestname */
                        if (bestval < i && i <= targval) {
                            bestval = i;
                            bestobj = obj;
                            strcpyn(bestname, sizeof(bestname), pname2);
                        }
                    }

                    pname2 = next_prop_name(obj, propname2, sizeof(propname2),
                                            pname2);
                }
            }

            ptr = pname + baselen;

            if (number(ptr)) {
                i = atoi(ptr);

                /* Another condition for a potential bestname */
                if (bestval < i && i <= targval) {
                    bestval = i;
                    bestobj = obj;
                    strcpyn(bestname, sizeof(bestname), pname);
                }
            }

            pname = next_prop_name(obj, propname, sizeof(propname), pname);
        }
    }

    /*
     * If get got it, load it and return it, otherwise send back an empty
     * string.
     */
    if (*bestname) {
        ptr = safegetprop_strict(player, bestobj, perms, bestname, mesgtyp,
                                 &blessed);

        if (!ptr)
            ABORT_MPI("SELECT", "Failed property read.");
    } else {
        ptr = "";
    }

    return ptr;
}


/**
 * MPI function that returns a carriage-return delimited string from a proplist
 *
 * This works on a proplist structure.
 *
 * First MPI argument is the prop list name.  The second argument is optional
 * and is an object; if not provided, it defaults to 'how'.
 *
 * Returns a carriage-return delimited string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_list(MFUNARGS)
{
    dbref obj = what;
    char *pname;
    const char *ptr;
    int blessed;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == PERMDENIED)
        ABORT_MPI("LIST", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LIST", "Match failed.");

    ptr = get_concat_list(what, perms, obj, (char *) pname, buf, BUFFER_LEN, 0,
                          mesgtyp, &blessed);

    if (!ptr)
        ptr = "";

    return ptr;
}


/**
 * MPI function that returns a carriage-return delimited string from a proplist
 *
 * This works on a proplist structure.  It also executes the MPI in the
 * list when finished.
 *
 * First MPI argument is the prop list name.  The second argument is optional
 * and is an object; if not provided, it defaults to 'how'.
 *
 * Returns a carriage-return delimited string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_lexec(MFUNARGS)
{
    dbref trg = (dbref) 0, obj = what;
    char *pname;
    const char *ptr;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == PERMDENIED)
        ABORT_MPI("LEXEC", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LEXEC", "Match failed.");

    while (*pname == PROPDIR_DELIMITER)
        pname++;

    ptr = get_concat_list(what, perms, obj, (char *) pname, buf, BUFFER_LEN, 2,
                          mesgtyp, &blessed);

    if (!ptr)
        ptr = "";

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname)
        || Prop_Hidden(pname))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "LEXEC", "listval");
    return ptr;
}



/**
 * MPI function that returns a randomly picked item from a proplist
 *
 * First MPI argument is the prop list name.  The second argument is optional
 * and is an object; if not provided, it defaults to 'how' and searches up
 * the environment.
 *
 * Returns a list item string.  That string is parsed for MPI before being
 * returned.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_rand(MFUNARGS)
{
    int num = 0;
    dbref trg = (dbref) 0, obj = what;
    const char *pname, *ptr;
    int blessed = 0;

    pname = argv[0];

    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1], mesgtyp);
    }

    if (obj == PERMDENIED)
        ABORT_MPI("RAND", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("RAND", "Match failed.");

    num = get_list_count(what, obj, perms, (char *) pname, mesgtyp, &blessed);

    if (!num)
        ABORT_MPI("RAND", "Failed list read.");

    ptr = get_list_item(what, obj, perms, (char *) pname,
                        (((RANDOM() / 256) % num) + 1),
                        mesgtyp, &blessed);

    if (!ptr)
        ABORT_MPI("RAND", "Failed list read.");

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr)
        || Prop_Hidden(ptr))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "RAND", "listval");
    return ptr;
}


/**
 * MPI function to select a line from a list based on the time
 *
 * This is rather complicated thing.  Given the first argument, a period
 * which is the number of seconds it would take to cycle through the whole
 * list, and given an offset as argument two which is added to the time
 * stamp and is usually 0.  The third argument is a list name, and the
 * last argument is an optional object that defaults to 'how'.
 *
 * Returns a list item string.  That string is parsed for MPI before being
 * returned.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_timesub(MFUNARGS)
{
    int num = 0;
    dbref trg = (dbref) 0, obj = what;
    const char *pname, *ptr;
    int period = 0, offset = 0;
    int blessed = 0;

    period = atoi(argv[0]);
    offset = atoi(argv[1]);
    pname = argv[2];

    if (argc == 4) {
        obj = mesg_dbref(descr, player, what, perms, argv[3], mesgtyp);
    }

    if (obj == PERMDENIED)
        ABORT_MPI("TIMESUB", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("TIMESUB", "Match failed.");

    num = get_list_count(what, obj, perms, (char *) pname, mesgtyp, &blessed);

    if (!num)
        ABORT_MPI("TIMESUB", "Failed list read.");

    if (period < 1)
        ABORT_MPI("TIMESUB", "Time period too short.");

    offset = (int) ((((long) time(NULL) + offset) % period) * num) / period;

    if (offset < 0)
        offset = -offset;

    ptr = get_list_item(what, obj, perms, (char *) pname, offset + 1, mesgtyp,
                        &blessed);

    if (!ptr)
        ABORT_MPI("TIMESUB", "Failed list read.");

    trg = what;

    if (blessed) {
        mesgtyp |= MPI_ISBLESSED;
    } else {
        mesgtyp &= ~MPI_ISBLESSED;
    }

    if (Prop_ReadOnly(ptr) || Prop_Private(ptr) || Prop_SeeOnly(ptr)
        || Prop_Hidden(ptr))
        trg = obj;

    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "TIMESUB", "listval");

    return ptr;
}


/**
 * MPI function to return a carriage-return
 *
 * Simply returns a string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_nl(MFUNARGS)
{
    return "\r";
}

/**
 * MPI function to return a tab
 *
 * Simply returns a string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_tab(MFUNARGS)
{
    return "\t";
}

/**
 * MPI function to return a literal string without parsed things in it
 *
 * Simply returns a string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_lit(MFUNARGS)
{
    size_t len, len2;

    strcpyn(buf, buflen, argv[0]);
    len = strlen(buf);

    for (int i = 1; i < argc; i++) {
        len2 = strlen(argv[i]);

        if (len2 + len + 3 >= BUFFER_LEN) {
            if (len + 3 < BUFFER_LEN) {
                strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
                buf[BUFFER_LEN - 3] = '\0';
            }

            break;
        }

        strcpyn(buf + len, buflen - len, ",");
        strcpyn(buf + len, buflen - len, argv[i]);
        len += len2;
    }

    return buf;
}


/**
 * MPI function that takes a string and evaluates MPI commands within it
 *
 * The opposite of 'lit'.  Often used to parse the output of {list}
 * The MPI commands will always be ran unblessed.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_eval(MFUNARGS)
{
    size_t len, len2;
    char buf2[BUFFER_LEN];
    char *ptr;

    strcpyn(buf, buflen, argv[0]);
    len = strlen(buf);

    for (int i = 1; i < argc; i++) {
        len2 = strlen(argv[i]);

        if (len2 + len + 3 >= BUFFER_LEN) {
            if (len + 3 < BUFFER_LEN) {
                strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
                buf[BUFFER_LEN - 3] = '\0';
            }

            break;
        }

        strcpyn(buf + len, buflen - len, ",");
        strcpyn(buf + len, buflen - len, argv[i]);
        len += len2;
    }

    strcpyn(buf2, sizeof(buf2), buf);
    ptr = mesg_parse(descr, player, what, perms, buf2, buf, BUFFER_LEN,
                     (mesgtyp & ~MPI_ISBLESSED));
    CHECKRETURN(ptr, "EVAL", "arg 1");
    return buf;
}


/**
 * MPI function that takes a string and evaluates MPI commands within it
 *
 * The opposite of 'lit'.  Often used to parse the output of {list}
 * The MPI commands will run with the same blessing level as the code
 * that executes the eval!.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_evalbang(MFUNARGS)
{
    size_t len, len2;
    char buf2[BUFFER_LEN];
    char *ptr;

    strcpyn(buf, buflen, argv[0]);
    len = strlen(buf);

    for (int i = 1; i < argc; i++) {
        len2 = strlen(argv[i]);

        if (len2 + len + 3 >= BUFFER_LEN) {
            if (len + 3 < BUFFER_LEN) {
                strncpy(buf + len, argv[i], (BUFFER_LEN - len - 3));
                buf[BUFFER_LEN - 3] = '\0';
            }

            break;
        }

        strcpyn(buf + len, buflen - len, ",");
        strcpyn(buf + len, buflen - len, argv[i]);
        len += len2;
    }

    strcpyn(buf2, sizeof(buf2), buf);
    ptr = mesg_parse(descr, player, what, perms, buf2, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr, "EVAL!", "arg 1");

    return buf;
}

/**
 * MPI function that strips spaces off a string
 *
 * Returns a stripped string
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_strip(MFUNARGS)
{
    return stripspaces(argv[0]);
}


/**
 * MPI function that makes a list out of the arguments
 *
 * Returns a string delimited by \r's that is a concatination of the
 * arguments.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_mklist(MFUNARGS)
{
    int len, tlen;
    int outcount = 0;

    tlen = 0;
    *buf = '\0';

    for (int i = 0; i < argc; i++) {
        len = strlen(argv[i]);

        if (tlen + len + 2 < BUFFER_LEN) {
            if (outcount++)
                strcatn(buf, BUFFER_LEN, "\r");

            strcatn(buf, BUFFER_LEN, argv[i]);
            tlen += len;
        } else {
            ABORT_MPI("MKLIST", "Max string length exceeded.");
        }
    }

    return buf;
}


/**
 * MPI function that does pronoun substitutions in a string
 *
 * The first argument is a string to process.  By default it
 * uses the using player; the second argument can be given to
 * override that and use a different object for pronoun values.
 *
 * @see pronoun_substitute
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_pronouns(MFUNARGS)
{
    dbref obj = player;

    if (argc > 1) {
        obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

        if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
            ABORT_MPI("PRONOUNS", "Match failed.");

        if (obj == PERMDENIED)
            ABORT_MPI("PRONOUNS", "Permission Denied.");
    }

    return pronoun_substitute(descr, obj, argv[0]);
}


/**
 * MPI function that returns player online time in seconds.
 *
 * The time returned is the least idle descriptor.  Uses mesg_dbref_raw
 * to process the first argument which should be a player.
 *
 * @see mesg_dbref_raw
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_ontime(MFUNARGS)
{
    dbref obj = mesg_dbref_raw(descr, player, what, argv[0]);
    int conn;

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        return "-1";

    if (obj == PERMDENIED)
        ABORT_MPI("ONTIME", "Permission denied.");

    conn = least_idle_player_descr(OWNER(obj));

    if (!conn)
        return "-1";

    snprintf(buf, BUFFER_LEN, "%d", pontime(conn));
    return buf;
}


/**
 * MPI function that returns player idle time in seconds.
 *
 * The time returned is the least idle descriptor.  Uses mesg_dbref_raw
 * to process the first argument which should be a player.
 *
 * @see mesg_dbref_raw
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_idle(MFUNARGS)
{
    dbref obj = mesg_dbref_raw(descr, player, what, argv[0]);
    int conn;

    if (obj == PERMDENIED)
        ABORT_MPI("IDLE", "Permission denied.");

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        return "-1";

    conn = least_idle_player_descr(OWNER(obj));

    if (!conn)
        return "-1";

    snprintf(buf, BUFFER_LEN, "%d", pidle(conn));
    return buf;
}


/**
 * MPI function that returns a list of players online
 *
 * This list is a carriage return delimited string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_online(MFUNARGS)
{
    int list_limit = MAX_MFUN_LIST_LEN;
    int count = pcount();
    char buf2[BUFFER_LEN];

    if (!(mesgtyp & MPI_ISBLESSED))
        ABORT_MPI("ONLINE", "Permission denied.");

    *buf = '\0';

    while (count && list_limit--) {
        if (*buf)
            strcatn(buf, BUFFER_LEN, "\r");

        ref2str(pdbref(count), buf2, sizeof(buf2));

        if ((strlen(buf) + strlen(buf2)) >= (BUFFER_LEN - 3))
            break;

        strcatn(buf, BUFFER_LEN, buf2);
        count--;
    }

    return buf;
}

/**
 * Compare two strings as either numbers or strings
 *
 * If both s1 and s2 are both strings containing numbers, then they are
 * compared as numbers.  If either one of them is a string, then they
 * are compared as strings.
 *
 * @see number
 *
 * This doesn't work at all for floating point numbers, though that is
 * pretty typical of all MPI.
 *
 * @private
 * @param s1 the first string
 * @param s2 the second string
 * @return returns the value of "s1 - s2" either as numbers or strings
 *         Similar to strcmp
 */
static int
msg_compare(const char *s1, const char *s2)
{
    if (*s1 && *s2 && number(s1) && number(s2)) {
        return (atoi(s1) - atoi(s2));
    } else {
        return strcasecmp(s1, s2);
    }
}


/**
 * MPI function that returns true if obj1 is inside obj2
 *
 * The first obj1 is the object to search for.  If obj2 is not provided,
 * defaults to the player's ref.
 *
 * The environment is scanned starting from 'obj2'.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_contains(MFUNARGS)
{
    dbref obj2 = mesg_dbref_raw(descr, player, what, argv[0]);
    dbref obj1 = player;

    if (argc > 1)
        obj1 = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

    if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == HOME)
        ABORT_MPI("CONTAINS", "Match failed (1).");

    if (obj2 == PERMDENIED)
        ABORT_MPI("CONTAINS", "Permission Denied (1).");

    if (obj1 == UNKNOWN || obj1 == AMBIGUOUS || obj1 == NOTHING || obj1 == HOME)
        ABORT_MPI("CONTAINS", "Match failed (2).");

    if (obj1 == PERMDENIED)
        ABORT_MPI("CONTAINS", "Permission Denied (2).");

    while (obj2 != NOTHING && obj2 != obj1)
        obj2 = LOCATION(obj2);

    if (obj1 == obj2) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns true if obj1's location is obj2
 *
 * This is a lot like {contains} but it doesn't search the environment.
 * Returns true if obj1 is inside obj2.  If obj2 is not provided, defaults
 * to the player's ref.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_holds(MFUNARGS)
{
    dbref obj1 = mesg_dbref_raw(descr, player, what, argv[0]);
    dbref obj2 = player;

    if (argc > 1)
        obj2 = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

    if (obj1 == UNKNOWN || obj1 == AMBIGUOUS || obj1 == NOTHING || obj1 == HOME)
        ABORT_MPI("HOLDS", "Match failed (1).");

    if (obj1 == PERMDENIED)
        ABORT_MPI("HOLDS", "Permission Denied (1).");

    if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == HOME)
        ABORT_MPI("HOLDS", "Match failed (2).");

    if (obj2 == PERMDENIED)
        ABORT_MPI("HOLDS", "Permission Denied (2).");

    if (obj2 == LOCATION(obj1)) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns true if obj1 and obj2 refer to the same object
 *
 * This does name matching of obj1 and obj2, along with #dbref number matching.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_dbeq(MFUNARGS)
{
    dbref obj1 = mesg_dbref_raw(descr, player, what, argv[0]);
    dbref obj2 = mesg_dbref_raw(descr, player, what, argv[1]);

    if (obj1 == UNKNOWN || obj1 == PERMDENIED)
        ABORT_MPI("DBEQ", "Match failed (1).");

    if (obj2 == UNKNOWN || obj2 == PERMDENIED)
        ABORT_MPI("DBEQ", "Match failed (2).");

    if (obj1 == obj2) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the boolean value arg0 != arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * For this call, that particular nuance doesn't matter too much.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_ne(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) == 0) {
        return "0";
    } else {
        return "1";
    }
}


/**
 * MPI function that returns the boolean value arg0 == arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * For this call, that particular nuance doesn't matter too much.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_eq(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) == 0) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the boolean value arg0 > arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_gt(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) > 0) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the boolean value arg0 < arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_lt(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) < 0) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the boolean value arg0 >= arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_ge(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) >= 0) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the boolean value arg0 <= arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_le(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) <= 0) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the smaller of arg0 and arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * Returns the smaller of the two.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_min(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) <= 0) {
        return argv[0];
    } else {
        return argv[1];
    }
}


/**
 * MPI function that returns the larger of arg0 and arg1
 *
 * If arg0 and arg1 are both strings containing numbers, they will be 
 * compared as numbers.  Otherwise, they will be compared as strings.
 *
 * Returns the larger of the two.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_max(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) >= 0) {
        return argv[0];
    } else {
        return argv[1];
    }
}


/**
 * MPI function that returns true if argv0 is a number
 *
 * @see number
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_isnum(MFUNARGS)
{
    if (*argv[0] && number(argv[0])) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns true if argv0 is a #dbref number
 *
 * The ref must start with a # to be valid.
 *
 * Also checks to make sure that the ref actually belongs to a valid
 * DBREF.  @see OkObj
 *
 * @see number
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_isdbref(MFUNARGS)
{
    dbref obj;
    const char *ptr = argv[0];

    skip_whitespace(&ptr);

    if (*ptr++ != NUMBER_TOKEN)
        return "0";

    if (!number(ptr))
        return "0";

    obj = (dbref) atoi(ptr);

    if (!OkObj(obj))
        return "0";

    return "1";
}



/**
 * MPI function that increments the value of the given variable
 *
 * The first argument is a variable name.  The second argument is optional
 * and is the amount you wish to increment the variable by.  It defaults to
 * 1
 *
 * The resulting value is returned.  The variable is not updated.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_inc(MFUNARGS)
{
    int x = 1;
    char *ptr = get_mvar(argv[0]);

    if (!ptr)
        ABORT_MPI("INC", "No such variable currently defined.");

    if (argc > 1)
        x = atoi(argv[1]);

    snprintf(buf, BUFFER_LEN, "%d", (atoi(ptr) + x));
    strcpyn(ptr, BUFFER_LEN, buf);
    return (buf);
}


/**
 * MPI function that decrements the value of the given variable
 *
 * The first argument is a variable name.  The second argument is optional
 * and is the amount you wish to decrement the variable by.  It defaults to
 * 1
 *
 * The resulting value is returned.  The variable is not updated.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_dec(MFUNARGS)
{
    int x = 1;
    char *ptr = get_mvar(argv[0]);

    if (!ptr)
        ABORT_MPI("DEC", "No such variable currently defined.");

    if (argc > 1)
        x = atoi(argv[1]);

    snprintf(buf, BUFFER_LEN, "%d", (atoi(ptr) - x));
    strcpyn(ptr, BUFFER_LEN, buf);
    return (buf);
}


/**
 * MPI function that numerically adds up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * adds them all up.  Returns the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_add(MFUNARGS)
{
    int i = atoi(argv[0]);

    for (int j = 1; j < argc; j++)
        i += atoi(argv[j]);

    snprintf(buf, BUFFER_LEN, "%d", i);
    return (buf);
}


/**
 * MPI function that numerically subtracts up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * subtracts them "from left to right".  Returns the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_subt(MFUNARGS)
{
    int i = atoi(argv[0]);

    for (int j = 1; j < argc; j++)
        i -= atoi(argv[j]);

    snprintf(buf, BUFFER_LEN, "%d", i);
    return (buf);
}


/**
 * MPI function that numerically multiplies up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * multiplies them.  Returns the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_mult(MFUNARGS)
{
    int i = atoi(argv[0]);

    for (int j = 1; j < argc; j++)
        i *= atoi(argv[j]);

    snprintf(buf, BUFFER_LEN, "%d", i);
    return (buf);
}


/**
 * MPI function that numerically divides up all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * divides them "from left to right".  Returns the result.
 *
 * For errors such as divide by zero, this will return 0.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_div(MFUNARGS)
{
    int k, i = atoi(argv[0]);

    for (int j = 1; j < argc; j++) {
        k = atoi(argv[j]);

        if (!k || (i == INT_MIN && k == -1)) {
            i = 0;
        } else {
            i /= k;
        }
    }

    snprintf(buf, BUFFER_LEN, "%d", i);
    return (buf);
}


/**
 * MPI function that numerically "modulo"'s all the arguments
 *
 * Loops over all the arguments, runs atoi(...) on each argument, and
 * mods them "from left to right".  Returns the result.
 *
 * For errors such as divide by zero, this will return 0.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_mod(MFUNARGS)
{
    int k, i = atoi(argv[0]);

    for (int j = 1; j < argc; j++) {
        k = atoi(argv[j]);

        if (!k || (i == INT_MIN && k == -1)) {
            i = 0;
        } else {
            i %= k;
        }
    }

    snprintf(buf, BUFFER_LEN, "%d", i);
    return (buf);
}



/**
 * MPI function that performs an absolute value on the numeric value of arg0
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_abs(MFUNARGS)
{
    int val = atoi(argv[0]);

    if (val == 0) {
        return "0";
    }

    if (val < 0) {
        val = -val;
    }

    snprintf(buf, BUFFER_LEN, "%d", val);
    return (buf);
}


/**
 * MPI function that provides a conditional based on the numeric expression
 *
 * If expression is negative, returns -1.  If positive, returns 1.  If
 * 0, returns 0.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_sign(MFUNARGS)
{
    int val;

    val = atoi(argv[0]);

    if (val == 0) {
        return "0";
    } else if (val < 0) {
        return "-1";
    } else {
        return "1";
    }
}



/**
 * MPI function that calculates distance between 2D or 3D points
 *
 * If two parameters are passed, it is the distance from 0,0
 * If three parameters are passed, it is from 0,0,0
 * If four parameters are passed, it is from the first pair to the second
 * If six parameters are passed, it is from the first triplet to the third
 *
 * The distance is computed as a floating point.  0.5 is added to the result,
 * then floor(...) is used to turn it into a regular integer.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_dist(MFUNARGS)
{
    int a, b, c;
    int a2, b2, c2;
    double result;

    a2 = b2 = c = c2 = 0;
    a = atoi(argv[0]);
    b = atoi(argv[1]);

    if (argc == 3) {
        c = atoi(argv[2]);
    } else if (argc == 4) {
        a2 = atoi(argv[2]);
        b2 = atoi(argv[3]);
    } else if (argc == 6) {
        c = atoi(argv[2]);
        a2 = atoi(argv[3]);
        b2 = atoi(argv[4]);
        c2 = atoi(argv[5]);
    } else if (argc != 2) {
        ABORT_MPI("DIST", "Takes 2,3,4, or 6 arguments.");
    }

    a -= a2;
    b -= b2;
    c -= c2;
    result = sqrt((double) (a * a) + (double) (b * b) + (double) (c * c));

    snprintf(buf, BUFFER_LEN, "%.0f", floor(result + 0.5));
    return buf;
}


/**
 * MPI function that calculates the 'not' logical expression on arg0
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_not(MFUNARGS)
{
    if (truestr(argv[0])) {
        return "0";
    } else {
        return "1";
    }
}


/**
 * MPI function that calculates the 'or' logical expression on all arguments
 *
 * It stops evaluating as soon as a true condition is reached, from left
 * to right.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_or(MFUNARGS)
{
    char *ptr;
    char buf2[16];

    for (int i = 0; i < argc; i++) {
        ptr = MesgParse(argv[i], buf, buflen);
        snprintf(buf2, sizeof(buf2), "arg %d", i + 1);
        CHECKRETURN(ptr, "OR", buf2);

        if (truestr(ptr)) {
            return "1";
        }
    }
    return "0";
}


/**
 * MPI function computes the exclusive-or (xor) of the two arguments
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_xor(MFUNARGS)
{
    if (truestr(argv[0]) && !truestr(argv[1])) {
        return "1";
    }

    if (!truestr(argv[0]) && truestr(argv[1])) {
        return "1";
    }

    return "0";
}


/**
 * MPI function that calculates the 'and' logical expression on all arguments
 *
 * It stops evaluating as soon as a false condition is reached, from left
 * to right.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_and(MFUNARGS)
{
    char *ptr;
    char buf2[16];

    for (int i = 0; i < argc; i++) {
        ptr = MesgParse(argv[i], buf, buflen);
        snprintf(buf2, sizeof(buf2), "arg %d", i + 1);
        CHECKRETURN(ptr, "AND", buf2);

        if (!truestr(ptr)) {
            return "0";
        }
    }

    return "1";
}


/**
 * MPI function that calculates random numbers through a "dice" like interface
 *
 * If one argument is passed, a number between 1 and arg0 is computed
 * With two, it generates arg1 numbers between 1 and arg0 and adds them up
 * With three, it works the same as two, but adds arg2 to the result.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_dice(MFUNARGS)
{
    int num = 1;
    int sides = 1;
    int offset = 0;
    int total = 0;

    if (argc >= 3)
        offset = atoi(argv[2]);

    if (argc >= 2)
        num = atoi(argv[1]);

    sides = atoi(argv[0]);

    if (num > 8888)
        ABORT_MPI("DICE", "Too many dice!");

    if (sides == 0)
        return "0";

    while (num-- > 0)
        total += (((RANDOM() / 256) % sides) + 1);

    snprintf(buf, BUFFER_LEN, "%d", (total + offset));
    return buf;
}


/**
 * MPI function to provide a default value if the first value is null or false
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_default(MFUNARGS)
{
    char *ptr;

    *buf = '\0';
    ptr = MesgParse(argv[0], buf, buflen);
    CHECKRETURN(ptr, "DEFAULT", "arg 1");

    if (ptr && truestr(buf)) {
        if (!ptr)
            ptr = "";
    } else {
        ptr = MesgParse(argv[1], buf, buflen);
        CHECKRETURN(ptr, "DEFAULT", "arg 2");
    }

    return ptr;
}


/**
 * MPI function to implement an if/else statement
 *
 * The first argument is the check statement.  If true, it runs the
 * second argument.  If false, it runs the optional third option if it
 * is provided.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_if(MFUNARGS)
{
    char *fbr, *ptr;

    if (argc == 3) {
        fbr = argv[2];
    } else {
        fbr = "";
    }

    ptr = MesgParse(argv[0], buf, buflen);
    CHECKRETURN(ptr, "IF", "arg 1");

    if (ptr && truestr(buf)) {
        ptr = MesgParse(argv[1], buf, buflen);
        CHECKRETURN(ptr, "IF", "arg 2");
    } else if (*fbr) {
        ptr = MesgParse(fbr, buf, buflen);
        CHECKRETURN(ptr, "IF", "arg 3");
    } else {
        *buf = '\0';
        ptr = "";
    }

    return ptr;
}



/**
 * MPI function to implement while loop
 *
 * The first argument is the check statement.  If true, it runs the
 * second argument.  If false, it breaks out of the loop.
 *
 * There is an iteration limi of MAX_MFUN_LIST_LEN
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_while(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char buf2[BUFFER_LEN];
    char *ptr;

    *buf = '\0';

    while (1) {
        ptr = MesgParse(argv[0], buf2, sizeof(buf2));
        CHECKRETURN(ptr, "WHILE", "arg 1");

        if (!truestr(ptr))
            break;

        ptr = MesgParse(argv[1], buf, buflen);
        CHECKRETURN(ptr, "WHILE", "arg 2");

        if (!(--iter_limit))
            ABORT_MPI("WHILE", "Iteration limit exceeded");
    }

    return buf;
}


/**
 * MPI function that returns nothing
 *
 * The arguments are parsed (not by this function, though, but rather by
 * the MPI parser), but this always returns an empty string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_null(MFUNARGS)
{
    return "";
}


/**
 * MPI function that returns local time zone offset from GMT in seconds
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_tzoffset(MFUNARGS)
{
    snprintf(buf, BUFFER_LEN, "%ld", get_tz_offset());
    return buf;
}


/**
 * MPI function that returns a time string for the given time zone.
 *
 * The time returned looks like hh:mm:ss
 *
 * If no timezone is provided, it is the MUCK time, otherwise it
 * uses the numeric offset provided.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_time(MFUNARGS)
{
    time_t lt;

    lt = time((time_t *) NULL);

    if (argc == 1) {
        lt += (3600 * atoi(argv[0]));
        lt -= get_tz_offset();
    }

    struct tm *tm = localtime(&lt);

    if (tm) {
        strftime(buf, BUFFER_LEN - 1, "%T", tm);
    } else {
        ABORT_MPI("TIME", "Out of range time argument.");
    }

    return buf;
}


/**
 * MPI function that returns a date string for the given time zone.
 *
 * The date returned looks like mm/dd/yy
 *
 * If no timezone is provided, it is the MUCK time, otherwise it
 * uses the numeric offset provided.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_date(MFUNARGS)
{
    time_t lt;

    time(&lt);

    if (argc == 1) {
        lt += (3600 * atoi(argv[0]));
        lt -= get_tz_offset();
    }

    struct tm *tm = localtime(&lt);

    if (tm) {
        strftime(buf, BUFFER_LEN - 1, "%D", tm);
    } else {
        ABORT_MPI("DATE", "Out of range time argument.");
    }

    return buf;
}


/**
 * MPI function that returns a time string with the specified format
 *
 * Takes a format string that is the same as MUF's 'timefmt' (which
 * also uses strftime under the hood) and takes optional timezone offset
 * and UNIX timestamp second count arguments.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_ftime(MFUNARGS)
{
    time_t lt;

    if (argc == 3) {
        lt = atol(argv[2]);
    } else {
        time(&lt);
    }

    if (argc > 1 && *argv[1]) {
        int offval = atoi(argv[1]);

        if (offval < 25 && offval > -25) {
            lt += 3600 * offval;
        } else {
            lt += offval;
        }

        lt -= get_tz_offset();
    }

    struct tm *tm = localtime(&lt);

    if (tm) {
        strftime(buf, BUFFER_LEN - 1, argv[0], tm);
    } else {
        ABORT_MPI("FTIME", "Out of range time argument.");
    }

    return buf;
}


/**
 * MPI function that converts a time string to a UNIX timestamp
 *
 * The time string should be in the format HH:MM:SS MM/DD/YY
 *
 * This does NOT work for Windows unfortunately.
 *
 * @see time_string_to_seconds
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_convtime(MFUNARGS)
{
#ifdef WIN32
    ABORT_MPI("CONVTIME", CURRENTLY_UNAVAILABLE);
#else
    char *error = 0;

    time_t seconds = time_string_to_seconds(argv[0], "%T%t%D", &error);

    if (error)
        ABORT_MPI("CONVTIME", error);

    snprintf(buf, BUFFER_LEN, "%lld", (long long)seconds);
    
    return buf;
#endif
}

/**
 * MPI function that converts a number of seconds to a long form breakdown
 *
 * For instance, "1 week, 2 days, 10 mins, 52 secs".
 *
 * @see timestr_long
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_ltimestr(MFUNARGS)
{
    return timestr_long(atol(argv[0]));
}


/**
 * MPI function that converts a number of seconds to a WHO-style timestamp
 *
 * Converts seconds to a string that looks like Xd HH:MM  The days will
 * be left off if there isn't a day amount.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_timestr(MFUNARGS)
{
    int tm = atol(argv[0]);
    int dy, hr, mn;

    /*
     * TODO: This is very similar to timestr_full, but timestr_full
     *       always returns day.  It's also very similar to time_format_1
     *       which formats almost identically but does a time delta instead.
     *
     *       I'm not sure if it is worth trying to consolidate this logic
     *       somehow, but we have 3 things that are almost idendical so
     *       maybe we should.  The easiest way seems to be to add a
     *       parameter to timestr_full to disable showing day if day is
     *       0.
     */


    dy = hr = mn = 0;

    if (tm >= 86400) {
        dy = tm / 86400;    /* Days */
        tm %= 86400;
    }

    if (tm >= 3600) {
        hr = tm / 3600;     /* Hours */
        tm %= 3600;
    }

    if (tm >= 60) {
        mn = tm / 60;       /* Minutes */
    }

    *buf = '\0';

    if (dy) {
        snprintf(buf, BUFFER_LEN, "%dd %02d:%02d", dy, hr, mn);
    } else {
        snprintf(buf, BUFFER_LEN, "%02d:%02d", hr, mn);
    }

    return buf;
}


/**
 * MPI function that returns the most significant time unit of the time period
 *
 * This would return something such as "9d" or "10h" or whatever the
 * most significant portion of the given time period is.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_stimestr(MFUNARGS)
{
    int tm = atol(argv[0]);
    int dy, hr, mn;

    /*
     * TODO this is almost identical to timestr, it just returns a
     *      slightly abreviated string.  This, in turn, is slightly different
     *      from timestr_full.  All of these should probably use the same
     *      function, with some sort of flags that determine how the final
     *      string is constructed
     */
    dy = hr = mn = 0;

    if (tm >= 86400) {
        dy = tm / 86400;    /* Days */
        tm %= 86400;
    }

    if (tm >= 3600) {
        hr = tm / 3600;     /* Hours */
        tm %= 3600;
    }

    if (tm >= 60) {
        mn = tm / 60;       /* Minutes */
        tm %= 60;           /* Seconds */
    }

    *buf = '\0';
    if (dy) {
        snprintf(buf, BUFFER_LEN, "%dd", dy);
        return buf;
    }

    if (hr) {
        snprintf(buf, BUFFER_LEN, "%dh", hr);
        return buf;
    }

    if (mn) {
        snprintf(buf, BUFFER_LEN, "%dm", mn);
        return buf;
    }

    snprintf(buf, BUFFER_LEN, "%ds", tm);

    return buf;
}


/**
 * MPI function that returns a unix timestamp
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_secs(MFUNARGS)
{
    time_t lt;

    time(&lt);
    snprintf(buf, BUFFER_LEN, "%lld", (long long) lt);
    return buf;
}


/**
 * MPI function converts a unix timestamp to a readable string
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_convsecs(MFUNARGS)
{
    time_t lt;
    char timebuf[BUFFER_LEN];

    lt = atol(argv[0]);

    struct tm *tm = localtime(&lt);

    if (tm) {
        strftime(timebuf, sizeof(timebuf), "%a %b %d %T %Z %Y", tm);
    } else {
        ABORT_MPI("CONVSECS", "Out of range time argument.");
    }

    strcpyn(buf, BUFFER_LEN, timebuf);
    return buf;
}


/**
 * MPI function returns the dbref location of the given object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_loc(MFUNARGS)
{
    dbref obj;

    obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LOC", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("LOC", "Permission denied.");

    return ref2str(LOCATION(obj), buf, BUFFER_LEN);
}


/**
 * MPI function that checks to see if arg0 is near arg1
 *
 * Nearby objcts are in the same location, or one contains the other,
 * or the two objects are the same object.
 *
 * If arg1 is not provided, defaults to the trigger object (what)
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_nearby(MFUNARGS)
{
    dbref obj;
    dbref obj2;

    obj = mesg_dbref_raw(descr, player, what, argv[0]);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING)
        ABORT_MPI("NEARBY", "Match failed (arg1).");

    if (obj == PERMDENIED)
        ABORT_MPI("NEARBY", "Permission denied (arg1).");

    if (obj == HOME)
        obj = PLAYER_HOME(player);

    if (argc > 1) {
        obj2 = mesg_dbref_raw(descr, player, what, argv[1]);

        if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING)
            ABORT_MPI("NEARBY", "Match failed (arg2).");

        if (obj2 == PERMDENIED)
            ABORT_MPI("NEARBY", "Permission denied (arg2).");

        if (obj2 == HOME)
            obj2 = PLAYER_HOME(player);
    } else {
        obj2 = what;
    }

    if (!(mesgtyp & MPI_ISBLESSED) && !isneighbor(obj, what) && !isneighbor(obj2, what) &&
        !isneighbor(obj, player) && !isneighbor(obj2, player)
        ) {
        ABORT_MPI("NEARBY", "Permission denied.  Neither object is local.");
    }

    if (isneighbor(obj, obj2)) {
        return "1";
    } else {
        return "0";
    }
}


/**
 * MPI function that returns the value of the provided object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_money(MFUNARGS)
{
    dbref obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("MONEY", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("MONEY", "Permission denied.");

    if (tp_pennies_muf_mlev > 1 && !(mesgtyp & MPI_ISBLESSED))
        ABORT_MPI("MONEY", "Permission denied.");

    switch (Typeof(obj)) {
        case TYPE_THING:
            snprintf(buf, BUFFER_LEN, "%d", GETVALUE(obj));
            break;

        case TYPE_PLAYER:
            snprintf(buf, BUFFER_LEN, "%d", GETVALUE(obj));
            break;

        default:
            strcpyn(buf, buflen, "0");
            break;
    }

    return buf;
}


/**
 * MPI function that returns the object flags for provided object as a string.
 *
 * The string is the return of @see unparse_flags
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_flags(MFUNARGS)
{
    dbref obj = mesg_dbref_local(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("FLAGS", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("FLAGS", "Permission denied.");

    return unparse_flags(obj);
}

/**
 * MPI function to send a message to the given target
 *
 * If only arg0 is provided, then it shows the message to the calling player.
 * If arg1 is provided, the message will be sent to that object.
 *
 * The message will be prefixed with the calling player's name if it doesn't
 * already start with that name, in the following conditions:
 *
 * - what is not a room
 * - owner of 'what' != object
 * - obj != the player
 * - what is an exit and the location != room
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_tell(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr2;
    dbref obj = player;

    if (argc > 1)
        obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("TELL", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("TELL", "Permission denied.");

    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("TELL", "Permission denied.");

    *buf = '\0';
    strcpyn(buf2, sizeof(buf2), argv[0]);

    for (char *ptr = buf2; (ptr != NULL) && *ptr != '\0'; ptr = ptr2) {
        ptr2 = strchr(ptr, '\r');

        if (ptr2 != NULL) {
            *ptr2++ = '\0';
        } else {
            ptr2 = ptr + strlen(ptr);
        }

        if (Typeof(what) == TYPE_ROOM || OWNER(what) == obj || player == obj ||
            (Typeof(what) == TYPE_EXIT && Typeof(LOCATION(what)) == TYPE_ROOM) ||
            string_prefix(argv[0], NAME(player))) {
            snprintf(buf, BUFFER_LEN, "%s%.4093s",
                     ((obj == OWNER(perms) || obj == player) ? "" : "> "), ptr);
        } else {
            snprintf(buf, BUFFER_LEN, "%s%.16s%s%.4078s",
                     ((obj == OWNER(perms) || obj == player) ? "" : "> "),
                     NAME(player), ((*argv[0] == '\'' || isspace(*argv[0])) ? "" : " "), ptr);
        }

        notify_from_echo(player, obj, buf, 1);
    }

    return argv[0];
}

/**
 * Check to see if 'child' is under 'parent' in the environment
 *
 * @TODO I have a hard time beliving this isn't defined somewhere else.
 *       Though maybe this similar logic is just copy/pasted all over town.
 *
 * @private
 * @param parent the parent to search under
 * @param child the child to check for
 * @return boolean true if child is located under parent's environment
 */
static int
isancestor(dbref parent, dbref child)
{
    while (child != NOTHING && child != parent) {
        child = getparent(child);
    }

    return child == parent;
}


/**
 * MPI function to send a message to the given target room
 *
 * If only arg0 is provided, then it shows the message to the calling player's
 * location, excluding the calling player.
 *
 * If arg1 is provided, the message will go to that room, excluding the
 * calling player.
 *
 * If arg2 is provided as #-1, then it will send the message to everyone
 * in the room excluding nobody.  Otherwise it will exclude whatever dbref
 * is in arg2.
 *
 * The message will be prefixed with the calling player's name if it doesn't
 * already start with that name, under certain conditions.  The conditions
 * are a little difficult to summarize but here's the summary from the
 * MPI docs:
 *
 * If the trigger isn't a room, or an exit on a room, and if the message
 * doesn't already begin with the user's name, then the user's name will
 * be prepended to the message.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_otell(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr2;
    dbref obj = LOCATION(player);
    dbref eobj = player;
    dbref thing;

    if (argc > 1)
        obj = mesg_dbref_local(descr, player, what, perms, argv[1], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("OTELL", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("OTELL", "Permission denied.");

    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("OTELL", "Permission denied.");

    if (argc > 2)
        eobj = mesg_dbref_raw(descr, player, what, argv[2]);

    strcpyn(buf2, sizeof(buf2), argv[0]);

    for (char *ptr = buf2; *ptr; ptr = ptr2) {
        ptr2 = strchr(ptr, '\r');

        if (ptr2) {
            *ptr2 = '\0';
        } else {
            ptr2 = ptr + strlen(ptr);
        }

        /*
         * TODO: This logic is ... very close to the one for tell.  Can we
         *       merge this somehow?
         */
        if (((OWNER(what) == OWNER(obj) || isancestor(what, obj)) &&
             (Typeof(what) == TYPE_ROOM ||
              (Typeof(what) == TYPE_EXIT && Typeof(LOCATION(what)) == TYPE_ROOM))) ||
                string_prefix(argv[0], NAME(player))) {
            strcpyn(buf, buflen, ptr);
        } else {
            snprintf(buf, BUFFER_LEN, "%.16s%s%.4078s", NAME(player),
                     ((*argv[0] == '\'' || isspace(*argv[0])) ? "" : " "), ptr);
        }

        thing = CONTENTS(obj);

        while (thing != NOTHING) {
            if (thing != eobj) {
                notify_from_echo(player, thing, buf, 0);
            }

            thing = NEXTOBJ(thing);
        }
    }

    return argv[0];
}


/**
 * MPI function to right justify a message
 *
 * arg0 is the message
 *
 * arg1 is the line width which defaults to 78
 *
 * arg2 is the padding string which defaults to " " but can be any
 * string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_right(MFUNARGS)
{
    /* {right:string,fieldwidth,padstr} */
    /* Right justify string to a fieldwidth, filling with padstr */

    /*
     * TODO: As part of issue #424 it would be cool if we introspected
     *       the proper terminal size by default if possible.
     */

    char *ptr;
    char *fptr;
    int i, len;
    char *fillstr;

    len = (argc > 1) ? atoi(argv[1]) : 78;

    if (len > BUFFER_LEN - 1)
        ABORT_MPI("RIGHT", "Fieldwidth too big.");

    fillstr = (argc > 2) ? argv[2] : " ";

    if (!*fillstr)
        ABORT_MPI("RIGHT", "Null pad string.");

    for (ptr = buf, fptr = fillstr, i = strlen(argv[0]); i < len; i++) {
        *ptr++ = *fptr++;

        if (!*fptr)
            fptr = fillstr;
    }

    strcpyn(ptr, buflen - (size_t)(ptr - buf), argv[0]);
    return buf;
}


/**
 * MPI function to left justify a message
 *
 * arg0 is the message
 *
 * arg1 is the line width which defaults to 78
 *
 * arg2 is the padding string which defaults to " " but can be any
 * string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_left(MFUNARGS)
{
    /* {left:string,fieldwidth,padstr} */
    /* Left justify string to a fieldwidth, filling with padstr */

    /*
     * TODO: As part of issue #424 it would be cool if we introspected
     *       the proper terminal size by default if possible.
     */

    char *ptr;
    char *fptr;
    int i, len;
    char *fillstr;

    len = (argc > 1) ? atoi(argv[1]) : 78;

    if (len > BUFFER_LEN - 1)
        ABORT_MPI("LEFT", "Fieldwidth too big.");

    fillstr = (argc > 2) ? argv[2] : " ";

    if (!*fillstr)
        ABORT_MPI("LEFT", "Null pad string.");

    strcpyn(buf, buflen, argv[0]);

    for (i = strlen(argv[0]), ptr = &buf[i], fptr = fillstr; i < len; i++) {
        *ptr++ = *fptr++;

        if (!*fptr)
            fptr = fillstr;
    }

    *ptr = '\0';
    return buf;
}


/**
 * MPI function to center justify a message
 *
 * arg0 is the message
 *
 * arg1 is the line width which defaults to 78
 *
 * arg2 is the padding string which defaults to " " but can be any
 * string.
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_center(MFUNARGS)
{
    /* {center:string,fieldwidth,padstr} */
    /* Center justify string to a fieldwidth, filling with padstr */

    /*
     * TODO: As part of issue #424 it would be cool if we introspected
     *       the proper terminal size by default if possible.
     */

    char *ptr;
    char *fptr;
    int i, len, halflen;
    char *fillstr;

    len = (argc > 1) ? atoi(argv[1]) : 78;

    if (len > BUFFER_LEN - 1)
        ABORT_MPI("CENTER", "Fieldwidth too big.");

    halflen = len / 2;

    fillstr = (argc > 2) ? argv[2] : " ";

    if (!*fillstr)
        ABORT_MPI("CENTER", "Null pad string.");

    for (ptr = buf, fptr = fillstr, i = strlen(argv[0]) / 2; i < halflen; i++) {
        *ptr++ = *fptr++;

        if (!*fptr)
            fptr = fillstr;
    }

    strcpyn(ptr, buflen - (size_t)(ptr - buf), argv[0]);

    for (i = strlen(buf), ptr = &buf[i], fptr = fillstr; i < len; i++) {
        *ptr++ = *fptr++;

        if (!*fptr)
            fptr = fillstr;
    }

    *ptr = '\0';
    return buf;
}


/**
 * MPI function to return a UNIX timestamp with object creation time.
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_created(MFUNARGS)
{
    dbref obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("CREATED", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("CREATED", "Permission denied.");

    snprintf(buf, BUFFER_LEN, "%lld", (long long) DBFETCH(obj)->ts_created);

    return buf;
}


/**
 * MPI function to return a UNIX timestamp with object last used time.
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_lastused(MFUNARGS)
{
    dbref obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LASTUSED", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("LASTUSED", "Permission denied.");

    snprintf(buf, BUFFER_LEN, "%lld", (long long) DBFETCH(obj)->ts_lastused);

    return buf;
}


/**
 * MPI function to return a UNIX timestamp with object modified time.
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_modified(MFUNARGS)
{
    dbref obj;

    /*
     * TODO: mfn_modified, mfn_created, and mfn_lastused are identical
     *       except for the field fetched from the object.  Maybe we
     *       should centralize the logic in a function and just thinly
     *       wrap it?  I don't normally like thin wrapper functions, but
     *       in this case it will be necessary since we need a function
     *       pointer (and thus can't use a define)
     *
     *       Looks like 'usecount' is basically the same, too.
     */

    obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("MODIFIED", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("MODIFIED", "Permission denied.");

    snprintf(buf, BUFFER_LEN, "%lld", (long long) DBFETCH(obj)->ts_modified);

    return buf;
}


/**
 * MPI function to return the object use count
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_usecount(MFUNARGS)
{
    dbref obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0], mesgtyp);

    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("USECOUNT", "Match failed.");

    if (obj == PERMDENIED)
        ABORT_MPI("USECOUNT", "Permission denied.");

    snprintf(buf, BUFFER_LEN, "%d", DBFETCH(obj)->ts_usecount);

    return buf;
}

/**
 * MPI function to return the caller's descriptor
 *
 * arg0 is the object
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_descr(MFUNARGS)
{
    strcpy(buf, intostr(descr));
    return buf;
}


/**
 * MPI function to return a tune parameter
 *
 * arg0 is the tune parameter key to fetch
 *
 * The player's permission level is the key factor here.  Blessing the
 * prop has no effect on the permission level, however that probably
 * does not matter as most users have read-only permissions to most tune
 * variables as it turns out from my testing. (tanabi)
 *
 * @param descr the descriptor of the caller
 * @param player the ref of the calling player
 * @param what the dbref of the trigger
 * @param perms the dbref for permission consideration
 * @param argc the number of arguments
 * @param argv the array of strings for arguments
 * @param buf the working buffer
 * @param buflen the size of the buffer
 * @param mesgtyp the type of the message
 * @return string parsed results
 */
const char *
mfn_sysparm(MFUNARGS)
{
    const char *ptr;

    ptr = tune_get_parmstring(argv[0], TUNE_MLEV(player));
    strcpy(buf, ptr);

    return (buf);
}
