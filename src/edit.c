/** @file edit.c
 *
 * Source code implementing the MUCK's code editing interface for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "commands.h"
#include "compile.h"
#include "db.h"
#include "edit.h"
#include "fbstrings.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "timequeue.h"
#include "tune.h"

/**
 * @var the MUF macro table.
 */
struct macrotable *macrotop;

/**
 * Free the memory of a line structure, including its string data as applicable
 *
 * @private
 * @param l the line structure to delete
 */
static void
free_line(struct line *l)
{
    free((void *) l->this_line);
    free(l);
}

/**
 * Frees an entire linked list of struct line's
 *
 * @param l the head of the list to delete
 */
void
free_prog_text(struct line *l)
{
    struct line *next;

    while (l) {
        next = l->next;
        free_line(l);
        l = next;
    }
}

/**
 * Locates a macro with a given name in the macrotable.
 *
 * Returns the macro definition or NULL if not found.
 *
 * Returns the macro definition or NULL if not found.  The returned memory
 * belongs to the caller -- remember to free it!
 *
 * @param match the string name to find, case insensitive
 * @return the macro definition or NULL if not found.
 */
char *
macro_expansion(const char *match)
{
    struct macrotable *node = macrotop;

    while (node) {
        register int value = strcasecmp(match, node->name);

        if (value < 0)
            node = node->left;
        else if (value > 0)
            node = node->right;
        else
            return alloc_string(node->definition);
    }
    return NULL;
}

/**
 * Create a new macrotable node with the given data
 *
 * 'name' is reduced to lowercase and may not be longer than BUFFER_LEN
 * including the \0.  The strings are copied so you may do whatever you
 * want with name / definition afterwards.
 *
 * @private
 * @param name the name of the macro
 * @param definition the macro definition
 * @param player the player creating the macro
 * @return the new macrotable node
 */
static struct macrotable *
new_macro(const char *name, const char *definition, dbref player)
{
    struct macrotable *newmacro = malloc(sizeof(struct macrotable));
    char buf[BUFFER_LEN];
    int i;

    for (i = 0; name[i]; i++)
        buf[i] = tolower(name[i]);

    buf[i] = '\0';
    newmacro->name = alloc_string(buf);
    newmacro->definition = alloc_string(definition);
    newmacro->implementor = player;
    newmacro->left = NULL;
    newmacro->right = NULL;
    return (newmacro);
}

/**
 * Grow the macro tree, adding a new node to it.
 *
 * @private
 * @param node the top node of the macro tree
 * @param newmacro the new macro to add to the tree
 * @return boolean true if a node was added, false if was already there
 */
static int
grow_macro_tree(struct macrotable *node, struct macrotable *newmacro)
{
    while (node) {
        register int value = strcasecmp(newmacro->name, node->name);

        if (!value)
            return 0;
        else if (value < 0) {
            if (node->left)
                node = node->left;
            else {
                node->left = newmacro;
                return 1;
            }
        } else if (node->right)
            node = node->right;
        else {
            node->right = newmacro;
            return 1;
        }
    }

    return 0;
}

/**
 * Insert a new macro into the macro table
 *
 * @see grow_macro_tree
 *
 * @private
 * @param macroname the name of the macro to create
 * @param macrodef the macro definition
 * @param player the player creating the macro
 * @return boolean true if node was added, false if node was already there
 */
static int
insert_macro(const char *macroname, const char *macrodef,
             dbref player)
{
    struct macrotable *newmacro;

    newmacro = new_macro(macroname, macrodef, player);

    if (!macrotop) {
        macrotop = newmacro;
        return 1;
    }

    return grow_macro_tree(macrotop, newmacro);
}

/**
 * Recursively list the macro tree to the given player.
 *
 * This has some complex logic around showing a range of macros;
 * there must be a first and last.  To list all macros, pass first
 * "\001" and last "\377" (yes, that's how its done, not exactly obvious)
 *
 * It is true to see definitions with one macro per output line.
 * If 'length' is false, then the lines are cut off at 70 characters and just
 * the macro names are shown in an "abridged" format that shows multiple
 * names per line which, honestly, is of questionable usefulness.  On our
 * test MUCK it actually makes a bunch of the macro names run together.
 *
 * @private
 * @param node the head node
 * @param first the starting string - we will not show macros "less" than this
 * @param last the end string - we will not show macros "greater" than this
 * @param full boolean true for long form, false for abridged form
 * @param player the player requesting the listing
 */
static void
do_list_tree(struct macrotable *node, const char *first, const char *last,
             int full, dbref player)
{
    static char buf[BUFFER_LEN];

    /*
     * @TODO I've been ragging on these recursive methods, but I gotta
     *       admit, this one would be rough to refactor to use
     *       a loop.  Recommend we refactor this to add a "depth" parameter
     *       to track how deep we go.  Increment it by 1 each time we
     *       go deeper.
     *
     *       My research suggests that a limit of 10000 would be well
     *       under most system's stack limits and yet ensure that the
     *       stack will not overflow.  If we reach that 10,000 limit,
     *       display a message to the user.  I think under all reasonable
     *       circumstances there will never be 10,000 macros, this will
     *       just save us from a potential attack vector. (tanabi)
     */

    if (!node)
        return;
    else {
        if (strncmp(node->name, first, strlen(first)) >= 0)
            do_list_tree(node->left, first, last, full, player);

        if (strncmp(node->name, first, strlen(first)) >= 0 &&
            strncmp(node->name, last, strlen(last)) <= 0) {
            if (full) {
                notifyf(player, "%-16s %-16s  %s", node->name,
                NAME(node->implementor), node->definition);
                buf[0] = '\0';
            } else {
                size_t blen = strlen(buf);
                snprintf(buf + blen, sizeof(buf) - blen, "%-16s", node->name);
                buf[sizeof(buf) - 1] = '\0';

                if (strlen(buf) > 70) {
                    notify(player, buf);
                    buf[0] = '\0';
                }
            }
        }

        if (strncmp(last, node->name, strlen(last)) >= 0)
            do_list_tree(node->right, first, last, full, player);

        if (node == macrotop && !full) {
            notify(player, buf);
            buf[0] = '\0';
        }
    }
}

/**
 * Implementation of list macros, for both full and abridged listing
 *
 * @see do_list_tree
 *
 * The usage of 'word' and 'k' is really weird here as well.  It
 * defines a string range from the editor arguments.  'word' will
 * be an array of strings that looks something like:
 *
 * ['s'] or ['single', 's'] or ['start', 'end', 's']
 *
 * 'k' is the number of items in the array, which will be at least 1;
 * if it is 0 or less, this will segfault.
 *
 * If 'k' is 1, then we will display all macros by using the range
 * \001 to \377.  If 'k' is 2, then the start and end range will be
 * the same value.  And if 'k' is 3, then we will ue both start and end.
 * Values of 'k' greater than 3 would work, but would perform oddly
 * (basically, the first and second-to-last string will be used)
 *
 * This comment is officially way bigger than the code, but the logic
 * is a little mind-bendy so it is worth the effort.
 *
 * @private
 * @param word the array of words - see description above
 * @param k the size of 'word' array
 * @param player the player to notify
 * @param length a boolean value, true for full mode, false for abridged
 */
static void
list_macros(char *word[], int k, dbref player, int full)
{
    if (!k--) {
        do_list_tree(macrotop, "\001", "\377", full, player);
    } else {
        do_list_tree(macrotop, word[0], word[k], full, player);
    }

    notify(player, "End of list.");
    return;
}

/**
 * Recursively free memory associated with the macro table
 *
 * @param node the macrotable node to free
 */
void
purge_macro_tree(struct macrotable *node)
{
    if (!node)
        return;

    purge_macro_tree(node->left);
    purge_macro_tree(node->right);

    free(node->name);
    free(node->definition);
    free(node);
}

/**
 * Recursively find and erase a node from the macro table
 *
 * @TODO See the TODO note for kill_macro - these two are kind
 *       of buddies.   There is a lot of repeticious code here,
 *       and it might be nice to refactor these two together.
 *
 * @private
 * @param oldnode pointer to parent of this node
 * @param node the current node to search
 * @param killname the macro to delete
 * @return boolean true if a node was deleted, false if not found
 */
static int
erase_node(struct macrotable *oldnode, struct macrotable *node,
           const char *killname)
{
    if (!node)
        return 0;

    int value = strcmp(killname, node->name);

    if (value < 0)
        return erase_node(node, node->left, killname);
    else if (value > 0)
        return erase_node(node, node->right, killname);
    else {
        if (node == oldnode->left) {
            oldnode->left = node->left;

            if (node->right)
                grow_macro_tree(macrotop, node->right);
        } else {
            oldnode->right = node->right;

            if (node->left)
                grow_macro_tree(macrotop, node->left);
        }

        free(node->name);
        free(node->definition);
        free(node);
        return 1;
    }
}

/**
 * Delete a macro
 *
 * @TODO This whole delete process is really clunky.  'kill_macro' handles
 *       the case of mtop being empty or being the node that we want to
 *       delete -- otherwise it hands off to erase_node which recursively
 *       searches for the node and deletes it.
 *
 *       Erase node then has a bunch of duplicate code in it.  I feel
 *       like this could be done safer / more securely with a loop.
 *       That being said, I'm not against recursion here since this
 *       is wizard only, but maybe tidy up how its done.
 *
 *       Also, I see absolutely no reason to pass 'player' into this.
 *       We don't use it for anything but a player == NOTHING check, but,
 *       the only place we call this does a is player a wizard check so
 *       player will always != NOTHING (tanabi)
 *
 * @private
 * @param macroname the macro to delete
 * @param player not really used
 * @return boolean true if something was deleted, false it not found.
 */
static int
kill_macro(const char *macroname, dbref player)
{
    if (!macrotop || player == NOTHING)
        return 0;

    if (!strcasecmp(macroname, macrotop->name)) {
        struct macrotable *macrotemp = macrotop;
        int whichway = macrotop->left ? 1 : 0;

        macrotop = whichway ? macrotop->left : macrotop->right;

        if (macrotop && (whichway ? macrotemp->right : macrotemp->left))
            grow_macro_tree(macrotop, whichway ? macrotemp->right : macrotemp->left);

        free(macrotemp->name);
        free(macrotemp->definition);
        free(macrotemp);
        return 1;
    }

    return erase_node(macrotop, macrotop, macroname);
}

/**
 * Recursively changes ownership (implementor) of macros
 *
 * This is the underpinning of 'chown_macros'.  Please note this is only
 * called if a player is toaded, so it is safe to leave this recursive
 * and cannot be used as an attack vector.
 *
 * @param node the node we are working on
 * @param from the source player
 * @param to the destination player
 */
void
chown_macros(struct macrotable *node, dbref from, dbref to)
{
    if (!node)
        return;

    chown_macros(node->left, from, to);

    if (node->implementor == from)
        node->implementor = to;

    chown_macros(node->right, from, to);
}

/**
 * Implementation of the disassemble editor command
 *
 * This iterates over the program and and displays each instruction to
 * the end user.  It gives some insight over how MUF is interpreted, though
 * not sure how useful it really is.
 *
 * @private
 * @param player the player to display output to
 * @param program the program to disassemble
 */
static void
disassemble(dbref player, dbref program)
{
    struct inst *curr;
    struct inst *codestart;
    char buf[BUFFER_LEN];

    codestart = curr = PROGRAM_CODE(program);

    if (!PROGRAM_SIZ(program)) {
        notify(player, "Nothing to disassemble!");
        return;
    }

    for (int i = 0; i < PROGRAM_SIZ(program); i++, curr++) {
        /*
         * The type of the instruction determines how we display it.
         * There is a lot of redundant code here, but I'm not sure
         * it is terribly feasible to consolidate it.
         */
        switch (curr->type) {
            case PROG_PRIMITIVE:
                if (curr->data.number >= 1 && curr->data.number <= prim_count)
                    snprintf(buf, sizeof(buf), "%d: (line %d) PRIMITIVE: %s", i,
                             curr->line, base_inst[curr->data.number - 1]);
                else
                    snprintf(buf, sizeof(buf), "%d: (line %d) PRIMITIVE: %d", i, curr->line,
                             curr->data.number);
                break;
            case PROG_MARK:
                snprintf(buf, sizeof(buf), "%d: (line %d) MARK", i, curr->line);
                break;
            case PROG_STRING:
                snprintf(buf, sizeof(buf), "%d: (line %d) STRING: \"%s\"", i,
                         curr->line, DoNullInd(curr->data.string));
                break;
            case PROG_ARRAY:
                snprintf(buf, sizeof(buf), "%d: (line %d) ARRAY: %d items", i,
                         curr->line,
                         curr->data.array ? curr->data.array->items : 0);
                break;
            case PROG_FUNCTION:
                snprintf(buf, sizeof(buf),
                         "%d: (line %d) FUNCTION: %s, VARS: %d, ARGS: %d", i,
                         curr->line, DoNull(curr->data.mufproc->procname),
                         curr->data.mufproc->vars, curr->data.mufproc->args);
                break;
            case PROG_LOCK:
                snprintf(buf, sizeof(buf), "%d: (line %d) LOCK: [%s]", i,
                         curr->line,
                         curr->data.lock == TRUE_BOOLEXP ? "TRUE_BOOLEXP" :
                         unparse_boolexp(0, curr->data.lock, 0));
                break;
            case PROG_INTEGER:
                snprintf(buf, sizeof(buf), "%d: (line %d) INTEGER: %d", i,
                         curr->line, curr->data.number);
                break;
            case PROG_FLOAT:
                snprintf(buf, sizeof(buf), "%d: (line %d) FLOAT: %.17g", i,
                         curr->line, curr->data.fnumber);
                break;
            case PROG_ADD:
                snprintf(buf, sizeof(buf), "%d: (line %d) ADDRESS: %d", i,
                         curr->line, (int) (curr->data.addr->data - codestart));
                break;
            case PROG_TRY:
                snprintf(buf, sizeof(buf), "%d: (line %d) TRY: %d", i,
                         curr->line, (int) (curr->data.call - codestart));
                break;
            case PROG_IF:
                snprintf(buf, sizeof(buf), "%d: (line %d) IF: %d", i,
                         curr->line, (int) (curr->data.call - codestart));
                break;
            case PROG_JMP:
                snprintf(buf, sizeof(buf), "%d: (line %d) JMP: %d", i,
                         curr->line, (int) (curr->data.call - codestart));
                break;
            case PROG_EXEC:
                snprintf(buf, sizeof(buf), "%d: (line %d) EXEC: %d", i,
                         curr->line, (int) (curr->data.call - codestart));
                break;
            case PROG_OBJECT:
                snprintf(buf, sizeof(buf), "%d: (line %d) OBJECT REF: %d", i,
                         curr->line, curr->data.number);
                break;
            case PROG_VAR:
                snprintf(buf, sizeof(buf), "%d: (line %d) VARIABLE: %d", i,
                         curr->line, curr->data.number);
                break;
            case PROG_SVAR:
                snprintf(buf, sizeof(buf), "%d: (line %d) SCOPEDVAR: %d (%s)",
                         i, curr->line, curr->data.number,
                         scopedvar_getname_byinst(curr, curr->data.number));
                break;
            case PROG_SVAR_AT:
                snprintf(buf, sizeof(buf),
                         "%d: (line %d) FETCH SCOPEDVAR: %d (%s)", i,
                         curr->line, curr->data.number,
                         scopedvar_getname_byinst(curr, curr->data.number));
                break;
            case PROG_SVAR_AT_CLEAR:
                snprintf(buf, sizeof(buf),
                         "%d: (line %d) FETCH SCOPEDVAR (clear optim): %d (%s)",
                         i, curr->line, curr->data.number,
                         scopedvar_getname_byinst(curr, curr->data.number));
                break;
            case PROG_SVAR_BANG:
                snprintf(buf, sizeof(buf),
                         "%d: (line %d) SET SCOPEDVAR: %d (%s)", i, curr->line,
                         curr->data.number,
                         scopedvar_getname_byinst(curr, curr->data.number));
                break;
            case PROG_LVAR:
                snprintf(buf, sizeof(buf), "%d: (line %d) LOCALVAR: %d", i,
                         curr->line, curr->data.number);
                break;
            case PROG_LVAR_AT:
                snprintf(buf, sizeof(buf), "%d: (line %d) FETCH LOCALVAR: %d",
                         i, curr->line, curr->data.number);
                break;
            case PROG_LVAR_AT_CLEAR:
                snprintf(buf, sizeof(buf),
                         "%d: (line %d) FETCH LOCALVAR (clear optim): %d", i,
                         curr->line, curr->data.number);
                break;
            case PROG_LVAR_BANG:
                snprintf(buf, sizeof(buf), "%d: (line %d) SET LOCALVAR: %d", i,
                         curr->line, curr->data.number);
                break;
            case PROG_CLEARED:
                snprintf(buf, sizeof(buf), "%d: (line ?) CLEARED INST AT %s:%d",
                         i, (char *) curr->data.addr, curr->line);
            default:
                snprintf(buf, sizeof(buf), "%d: (line ?) UNKNOWN INST", i);
        }

        notify(player, buf);
    }
}

/**
 * Puts player into insert mode
 *
 * 'arg' is an array of integers, and if 'argc' is non-zero we will
 * us the first element in 'arg' as the starting line number.
 *
 * @private
 * @param player the player doing the editing
 * @param program the program to be edited
 * @param arg arguments array - only the first one matters
 * @param argc count of elements in arg
 */
static void
do_insert(dbref player, dbref program, int arg[], int argc)
{
    PLAYER_SET_INSERT_MODE(player, PLAYER_INSERT_MODE(player) + 1);

    /* set current line to something else if necessary */
    if (argc)
        PROGRAM_SET_CURR_LINE(program, arg[0] - 1);
}

/**
 * Delete lines from a program
 *
 * Deletes line arg[0] if one argument, lines arg[0] through arg[1] if two
 * arguments.  And finally, current line if no argument.
 *
 * WARNING: arg *must* have 2 entries in it or this will segfault,
 *          regardless of the value of 'argc'.
 *
 * @private
 * @param player the player doing the deleting
 * @param program the program to delete lines from
 * @param arg the numeric arguments - line numbers as described above
 * @param argc the count of items in arg
 */
static void
do_delete(dbref player, dbref program, int arg[], int argc)
{
    struct line *curr, *temp;
    int i;

    switch (argc) {
        case 0:
            arg[0] = PROGRAM_CURR_LINE(program);
        case 1:
            arg[1] = arg[0];
        case 2:
            /*
             * delete from line 1 to line 2
             *
             * first, check for conflict
             */
            if (arg[0] > arg[1]) {
                notify(player, "Nonsensical arguments.");
                return;
            }

            i = arg[0] - 1;

            for (curr = PROGRAM_FIRST(program); curr && i; i--)
                curr = curr->next;

            if (curr) {
                int n = 0;

                PROGRAM_SET_CURR_LINE(program, arg[0]);
                i = arg[1] - arg[0] + 1;

                /* delete n lines */
                while (i && curr) {
                    temp = curr;

                    if (curr->prev)
                        curr->prev->next = curr->next;
                    else
                        PROGRAM_SET_FIRST(program, curr->next);

                    if (curr->next)
                        curr->next->prev = curr->prev;

                    curr = curr->next;
                    free_line(temp);
                    i--;
                }

                n = arg[1] - arg[0] - i + 1;
                notifyf(player, "%d line%s deleted", n, n != 1 ? "s" : "");
            } else
                notify(player, "No line to delete!");
                break;
        default:
            notify(player, "Too many arguments!");
            break;
    }
}

/**
 * Quit from edit mode.  Put player back into the regular game mode.
 *
 * This is a quit and save action.  Also does the logginng of the program
 * text if logging is turned on.
 *
 * @private
 * @param player the player editing
 * @param program the program being edited
 */
static void
do_quit(dbref player, dbref program)
{
    char unparse_buf[BUFFER_LEN];
    unparse_object(player, program, unparse_buf, sizeof(unparse_buf));
    log_status("PROGRAM SAVED: %s by %s(%d)", unparse_buf,
               NAME(player), player);
    write_program(PROGRAM_FIRST(program), program);

    if (tp_log_programs)
        log_program_text(PROGRAM_FIRST(program), player, program);

    free_prog_text(PROGRAM_FIRST(program));
    PROGRAM_SET_FIRST(program, NULL);
    FLAGS(program) &= ~INTERNAL;
    FLAGS(player) &= ~INTERACTIVE;
    PLAYER_SET_CURR_PROG(player, NOTHING);
    DBDIRTY(player);
    DBDIRTY(program);
}

/**
 * Quit from edit mode, with no changes written.
 *
 * @private
 */
static void
do_cancel(dbref player, dbref program)
{
    uncompile_program(program);
    free_prog_text(PROGRAM_FIRST(program));
    PROGRAM_SET_FIRST(program, NULL);
    FLAGS(program) &= ~INTERNAL;
    FLAGS(player) &= ~INTERACTIVE;
    PLAYER_SET_CURR_PROG(player, NOTHING);
    DBDIRTY(player);
}

/**
 * List program using the given arguments
 *
 * If no argument, redisplay the current line.  If 1 argument, display that
 * line.  If 2 arguments, display all in between.
 *
 * WARNING: oarg *must* have 2 entries in it or this will segfault,
 *          regardless of the value of 'argc'.
 *
 * If comment_sysmsg is true, extra messages injected by the system
 * such as "X lines displayed" usually displayed at the end of the listing
 * will be commented out.
 *
 * @param player the player listing code
 * @param program the program to be listed
 * @param oarg argument array with line numbers
 * @param argc the number of arguments in oarg
 * @param comment_sysmsg boolean if true any extra messages are commented
 */
void
list_program(dbref player, dbref program, int *oarg, int argc,
             int comment_sysmsg)
{
    struct line *curr;
    int i, count;
    int arg[2];
    char *msg_start = comment_sysmsg ? "( " : "";
    char *msg_end = comment_sysmsg ? " )" : "";

    if (oarg) {
        arg[0] = oarg[0];

        if (argc > 1)
            arg[1] = oarg[1];
    } else
        arg[0] = arg[1] = 0;

    switch (argc) {
        case 0:
            arg[0] = PROGRAM_CURR_LINE(program);
        case 1:
            arg[1] = arg[0];
        case 2:
            if ((arg[0] > arg[1]) && (arg[1] != -1)) {
                notifyf_nolisten(player, "%sArguments don't make sense!%s", msg_start, msg_end);
                return;
            }

            i = arg[0] - 1;

            for (curr = PROGRAM_FIRST(program); i && curr; i--)
                curr = curr->next;

            if (curr) {
                i = arg[1] - arg[0] + 1;

                /* display n lines */
                for (count = arg[0]; curr && (i || (arg[1] == -1)); i--) {
                    if (FLAGS(player) & INTERNAL)
                        notifyf_nolisten(player, "%3d: %s", count, DoNull(curr->this_line));
                    else
                        notifyf_nolisten(player, "%s", DoNull(curr->this_line));

                    count++;
                    curr = curr->next;
                }

                if (count - arg[0] > 1) {
                    notifyf_nolisten(player, "%s%d lines displayed.%s", msg_start, count - arg[0], msg_end);
                }
            } else
                notifyf_nolisten(player, "%sLine not available for display.%s", msg_start, msg_end);

            break;
        default:
            notifyf_nolisten(player, "%sToo many arguments!%s", msg_start, msg_end);
            break;
    }
}

/**
 * List a program's header comments
 *
 * This iterates over the first lines of a program and outputs all the
 * ones that start with (.  Stops listing when it comes across a line
 * that doesn't start with (.
 *
 * @private
 * @param player the player to list to
 * @param program the program to list
 */
static void
do_list_header(dbref player, dbref program)
{
    struct line *curr = read_program(program);

    /*
     * @TODO This is really a dumb way to do this ... we should instead
     *       find the last closing ) that isn't followed by another ( on
     *       the next line.
     */
    while (curr && (curr->this_line)[0] == '(') {
        notify(player, curr->this_line);
        curr = curr->next;
    }

    if (!(FLAGS(program) & INTERNAL)) {
        free_prog_text(curr);
    }

    notify(player, "Done.");
}

/**
 * Lists the header of a given program
 *
 * The program should be a dbref in arg[0], and argc must be 1, or this
 * will trigger an error.
 *
 * Does permission checks to make sure 'player' can view the program.
 *
 * @private
 * @param player the player looking at the listing
 * @param arg the argument array - see notes above
 * @param argc must be 1
 */
static void
val_and_head(dbref player, int arg[], int argc)
{
    dbref program;

    if (argc != 1) {
        notify(player, "I don't understand which header you're trying to look at.");
        return;
    }

    program = arg[0];

    if (!ObjExists(program) || Typeof(program) != TYPE_PROGRAM) {
        notify(player, "That isn't a program.");
        return;
    }

    if (!(controls(player, program) || (FLAGS(program) & VEHICLE))) {
        notify(player, "That's not a public program.");
        return;
    }

    do_list_header(player, program);
}

/**
 * List publics on a program
 *
 * If argc == 1 and arg[0] is a program ref, that is the program that publics
 * are listed on.  If argc is 0, then the currently edited program is
 * listed.
 *
 * This does all the permission checks needed and compiles the program if
 * it isn't already compiled so the publics structures are populated.
 *
 * @private
 * @param descr the descriptor of the listing player
 * @param player the listing player
 * @param arg the argument array
 * @param argc the number of arguments
 */
static void
list_publics(int descr, dbref player, int arg[], int argc)
{
    dbref program;

    if (argc > 1) {
        notify(player,
               "I don't understand which program you want to list PUBLIC functions for.");
        return;
    }

    program = (argc == 0) ? PLAYER_CURR_PROG(player) : arg[0];

    if (!OkObj(program) || Typeof(program) != TYPE_PROGRAM) {
        notify(player, "That isn't a program.");
        return;
    }

    if (!(controls(player, program) || (FLAGS(program) & VEHICLE))) {
        notify(player, "That's not a public program.");
        return;
    }

    if (!(PROGRAM_CODE(program))) {
        if (program == PLAYER_CURR_PROG(player)) {
            do_compile(descr, OWNER(program), program, 0);
        } else {
            struct line *tmpline;

            tmpline = PROGRAM_FIRST(program);
            PROGRAM_SET_FIRST(program, (struct line *) read_program(program));
            do_compile(descr, OWNER(program), program, 0);
            free_prog_text(PROGRAM_FIRST(program));
            PROGRAM_SET_FIRST(program, tmpline);
        }

        if (!PROGRAM_CODE(program)) {
            notify(player, "Program not compilable.");
            return;
        }
    }

    notify(player, "PUBLIC functions:");
    for (struct publics *ptr = PROGRAM_PUBS(program); ptr; ptr = ptr->next)
        notify(player, ptr->subname);
}

/**
 * Toggle line number display
 *
 * If argc evaluates to true, then arg[0] is checked.  If arg[0] is
 * 0, then line numbers are turned off, otherwise they are turned on.
 *
 * If argc evaluates to false, then line numbers are toggled.
 *
 * The flag INTERNAL is used to determine if line numbers are on or off.
 *
 * @private
 * @param player the player toggling line numbers
 * @param arg the arguments as described above
 * @param argc the argument count as described above
 */
static void
toggle_numbers(dbref player, int arg[], int argc)
{
    if (argc) {
        switch (arg[0]) {
            case 0:
                FLAGS(player) &= ~INTERNAL;
                notify(player, "Line numbers off.");
                break;

            default:
                FLAGS(player) |= INTERNAL;
                notify(player, "Line numbers on.");
                break;
        }
    } else if (FLAGS(player) & INTERNAL) {
        FLAGS(player) &= ~INTERNAL;
        notify(player, "Line numbers off.");
    } else {
        FLAGS(player) |= INTERNAL;
        notify(player, "Line numbers on.");
    }
}

/**
 * Insert this line into program (or exit editor if line is EXIT_INSERT)
 *
 * This is surprisingly complex.  Handles the check for exiting insert
 * mode and adds the line in the appropriate place otherwise.
 *
 * @private
 * @param player the player doing the editing
 * @param line the line to process
 */
static void
insert_line(dbref player, const char *line)
{
    dbref program;
    int i;
    struct line *curr;
    struct line *new_line;

    program = PLAYER_CURR_PROG(player);
    if (!strcasecmp(line, EXIT_INSERT)) {
        PLAYER_SET_INSERT_MODE(player, 0);  /* turn off insert mode */
        notify_nolisten(player, "Exiting insert mode.", 1);
        return;
    }

    i = PROGRAM_CURR_LINE(program) - 1;

    for (curr = PROGRAM_FIRST(program); curr && i && i + 1; i--)
        curr = curr->next;

    new_line = get_new_line();  /* initialize line */

    if (!*line) {
        new_line->this_line = alloc_string(" ");
    } else {
        new_line->this_line = alloc_string(line);
    }

    if (!PROGRAM_FIRST(program)) {  /* nothing --- insert in front */
        PROGRAM_SET_FIRST(program, new_line);
        PROGRAM_SET_CURR_LINE(program, 2);  /* insert at the end */

        /* DBDIRTY(program); */
        return;
    }

    if (!curr) {    /* insert at the end */
        i = 1;

        for (curr = PROGRAM_FIRST(program); curr->next; curr = curr->next)
            i++;    /* count lines */

        PROGRAM_SET_CURR_LINE(program, i + 2);
        new_line->prev = curr;
        curr->next = new_line;
        /* DBDIRTY(program); */
        return;
    }

    if (!PROGRAM_CURR_LINE(program)) {     /* insert at the beginning */
        PROGRAM_SET_CURR_LINE(program, 1); /* insert after this new
                                            * line
                                            */
        new_line->next = PROGRAM_FIRST(program);
        PROGRAM_SET_FIRST(program, new_line);
        /* DBDIRTY(program); */
        return;
    }

    /* inserting in the middle */
    PROGRAM_SET_CURR_LINE(program, PROGRAM_CURR_LINE(program) + 1);
    new_line->prev = curr;
    new_line->next = curr->next;

    if (new_line->next)
        new_line->next->prev = new_line;

    curr->next = new_line;
}

/**
 * The editor itself -- this gets called each time every time to parse commands.
 *
 * 'command' is the input text.  It is parsed for commands, and this
 * handles either inserting the line or running the appropriate command.
 *
 * @private
 * @param descr the descriptor of the editing user
 * @param player the player editing
 * @param command the input line
 */
static void
editor(int descr, dbref player, const char *command)
{
    dbref program;
    int arg[MAX_ARG + 1];
    char buf[BUFFER_LEN];
    char *word[MAX_ARG + 1];
    int i, j;   /* loop variables */

    program = PLAYER_CURR_PROG(player);

    /* check to see if we are insert mode */
    if (PLAYER_INSERT_MODE(player)) {
        insert_line(player, command);   /* insert it! */
        return;
    }

    /* parse the commands */
    for (i = 0; i <= MAX_ARG && *command; i++) {
        skip_whitespace(&command);
        j = 0;

        while (*command && !isspace(*command)) {
            buf[j] = *command;
            command++;
            j++;
        }

        buf[j] = '\0';
        word[i] = alloc_string(buf);

        if ((i == 1) && !strcasecmp(word[0], "def")) {
            if (word[1]
                && (word[1][0] == '.'
                    || (word[1][0] >= '0' && word[1][0] <= '9'))) {
                notify(player, "Invalid macro name.");
                for (; i >= 0; i--) {
                    free(word[i]);
                }
                return;
            }

            skip_whitespace(&command);
            word[2] = alloc_string(command);

            if (!word[2])
                notify(player, "Invalid definition syntax.");
            else {
                if (insert_macro(word[1], word[2], player)) {
                    notify(player, "Entry created.");
                } else {
                    notify(player, "That macro already exists!");
                }
            }

            for (; i >= 0; i--) {
                free(word[i]);
            }

            return;
        }

        arg[i] = atoi(buf);

        if (arg[i] < 0) {
            notify(player, "Negative arguments not allowed!");

            for (; i >= 0; i--) {
                free(word[i]);
            }

            return;
        }
    }

    i--;

    while ((i >= 0) && !word[i])
        i--;

    if (i < 0) {
        return;
    } else {
        /*
         * Process the commands, which are all single characters after
         * we have checked for def.
         */
        switch (word[i][0]) {
            case KILL_COMMAND:
                if (!Wizard(player)) {
                    notify(player, "I'm sorry Dave, but I can't let you do that.");
                } else {
                    if (kill_macro(word[0], player))
                        notify(player, "Macro entry deleted.");
                    else
                        notify(player, "Macro to delete not found.");
                }

                break;

            case SHOW_COMMAND:
                list_macros(word, i, player, 1);
                break;

            case SHORTSHOW_COMMAND:
                list_macros(word, i, player, 0);
                break;

            case INSERT_COMMAND:
                do_insert(player, program, arg, i);
                notify(player, "Entering insert mode.");
                break;

            case DELETE_COMMAND:
                do_delete(player, program, arg, i);
                break;

            case QUIT_EDIT_COMMAND:
                do_quit(player, program);
                notify(player, "Editor exited.");
                break;

            case CANCEL_EDIT_COMMAND:
                do_cancel(player, program);
                notify(player, "Changes cancelled.");
                break;

            case COMPILE_COMMAND:
                /* compile code belongs in compile.c, not in the editor */
                do_compile(descr, player, program, 1);
                notify(player, "Compiler done.");
                break;

            case LIST_COMMAND:
                list_program(player, program, arg, i, 0);
                break;

            case EDITOR_HELP_COMMAND:
                spit_file_segment(player, tp_file_editor_help, "");
                break;

            case VIEW_COMMAND:
                val_and_head(player, arg, i);
                break;

            case UNASSEMBLE_COMMAND:
                disassemble(player, program);
                break;

            case NUMBER_COMMAND:
                toggle_numbers(player, arg, i);
                break;

            case PUBLICS_COMMAND:
                list_publics(descr, player, arg, i);
                break;

            default:
                notify(player, "Illegal editor command.");
                break;
        }
    }

    for (; i >= 0; i--) {
        free(word[i]);
    }
}

/**
 * Allocate a new line structure
 *
 * @return newly allocated struct line
 */
struct line *
get_new_line(void)
{
    struct line *nu;

    nu = malloc(sizeof(struct line));

    if (!nu) {
        fprintf(stderr, "get_new_line(): Out of memory!\n");
        abort();
    }

    memset(nu, 0, sizeof(struct line));

    return nu;
}

/**
 * Read a program from the disk and return it as a linked list of struct line
 *
 * @param i the dbref of the program to load
 * @return the program text as a linked list of struct line
 */
struct line *
read_program(dbref i)
{
    char buf[BUFFER_LEN];
    struct line *first;
    struct line *prev = NULL;
    struct line *nu;
    FILE *f;
    int len;

    first = NULL;
    snprintf(buf, sizeof(buf), "muf/%d.m", (int) i);
    f = fopen(buf, "rb");

    if (!f)
        return 0;

    while (fgets(buf, BUFFER_LEN, f)) {
        nu = get_new_line();
        len = strlen(buf);

        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            len--;
        }

        if (len > 0 && buf[len - 1] == '\r') {
            buf[len - 1] = '\0';
            len--;
        }

        if (!*buf)
            strcpyn(buf, sizeof(buf), " ");
            nu->this_line = alloc_string(buf);

            if (!first) {
                prev = nu;
                first = nu;
            } else {
                prev->next = nu;
                nu->prev = prev;
                prev = nu;
            }
    }

    fclose(f);
    return first;
}

/**
 * Write program text to disk
 *
 * @param first the head of the linked list of lines to write
 * @param i the dbref of the program to write
 */
void
write_program(struct line *first, dbref i)
{
    FILE *f;
    char fname[BUFFER_LEN];

    snprintf(fname, sizeof(fname), "muf/%d.m", (int) i);
    f = fopen(fname, "wb");

    if (!f) {
        log_status("Couldn't open file %s!", fname);
        return;
    }

    while (first) {
        if (!first->this_line)
            continue;

        if (fputs(first->this_line, f) == EOF) {
            abort();
        }

        if (fputc('\n', f) == EOF) {
            abort();
        }

        first = first->next;
    }

    fclose(f);
}

/**
 * Implementation of the @list command
 *
 * This includes all permission checking around whether someone can
 * list the program, and checking to make sure that 'name' is a program.
 *
 * linespec is surprisingly complex; you can have one of the following
 * prefixes:
 *
 * ! - outputs system messages as comments.  This comments anything that
 *     @list is injecting into the message stream, such as the line
 *     count at the end.
 *
 * @ - List program without line numbers overriding default
 *
 * # - List program with line numbers, overriding default
 *
 * You can then have a single line, or a line range with a hyphen.
 * By default the entire program is listed if no range is provided.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param name the program to list
 * @param linespec the arguments to list, as described above.
 */
void
do_list(int descr, dbref player, const char *name, char *linespec)
{
    dbref thing;
    const char *p;
    char *q;
    int range[2];
    int argc;
    int comment_sysmsg = 0;
    object_flag_type prevflags = FLAGS(player);
    struct match_data md;
    struct line *tmpline;

    init_match(descr, player, name, TYPE_PROGRAM, &md);
    match_neighbor(&md);
    match_possession(&md);
    match_registered(&md);
    match_absolute(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING)
        return;

    if (Typeof(thing) != TYPE_PROGRAM) {
        notify(player, "You can't list anything but a program.");
        return;
    }

    if (!(controls(player, thing) || (FLAGS(thing) & VEHICLE))) {
        notify(player,
               "Permission denied. (You don't control the program, and it's not set Viewable)");
        return;
    }

    while (*linespec && (*linespec == '!' || *linespec == '#' || *linespec == '@')) {
        prevflags = FLAGS(player);

        switch (*linespec) {
            case '!':
                comment_sysmsg = 1;
                break;
            case '#':
                FLAGS(player) |= INTERNAL;
                break;
            case '@':
                FLAGS(player) &= ~INTERNAL;
                break;
            default:
                break;
        }

        (void) *linespec++;
    }

    if (!*linespec) {
        range[0] = 1;
        range[1] = -1;
        argc = 2;
    } else {
        q = (char *)(p = linespec);

        while (*p) {
            while (*p && !isspace(*p))
                *q++ = *p++;

            skip_whitespace(&p);
        }

        *q = '\0';

        argc = 1;
        if (isdigit(*linespec)) {
            range[0] = atoi(linespec);

            while (*linespec && isdigit(*linespec))
                linespec++;
        } else {
            range[0] = 1;
        }

        if (*linespec) {
            argc = 2;

            if (*linespec == '-')
                linespec++;

            if (*linespec)
                range[1] = atoi(linespec);
            else
                range[1] = -1;
        }
    }

    tmpline = PROGRAM_FIRST(thing);
    PROGRAM_SET_FIRST(thing, read_program(thing));
    list_program(player, thing, range, argc, comment_sysmsg);
    free_prog_text(PROGRAM_FIRST(thing));
    PROGRAM_SET_FIRST(thing, tmpline);
    FLAGS(player) = prevflags;
    return;
}

/**
 * Determine if a player is editing or running an interactive command.
 *
 * If the player is flagged READMODE, then they are in an interactive
 * MUF program.  Otherwise, they are in the editor.
 *
 * This function is only called if the player is set INTERACTIVE, so
 * therefore if a player is only set INTERACTIVE but not READMODE then
 * they are in the editor.
 *
 * @param descr the descriptor of the player we are tracking interactivity for
 * @param player the player interacting
 * @param command the command they typed in
 */
void
interactive(int descr, dbref player, const char *command)
{
    if (FLAGS(player) & READMODE) {
        /*
         * process command, push onto stack, and return control to forth
         * program
         */
        handle_read_event(descr, player, command);
    } else {
        editor(descr, player, command);
    }
}

/**
 * Recursively dump MUF macros to a file handle
 *
 * Macros are written in a format that macroload can read
 *
 * Specifically, one macro per line in alphabetical order.  Each
 * macro takes 3 lines.
 *
 * The first line is the name of the macro.
 * The second line is the macro definition.
 * The third line is the dbref of the MUCK creator with no leading #
 *
 * @param node the macrotable to dump.
 * @param f the file handle to write to.
 */
void
macrodump(struct macrotable *node, FILE * f)
{
    if (!node)
        return;

    /*
     * @TODO This is a potential attack vector - we can avoid crashing
     *       the MUCK with too many macro's by limiting how deep our
     *       recursion can go (to say about 10,000 macro's)
     *
     *       Or a loop could be used, but it would be a much more messy
     *       mash of code.
     */

    macrodump(node->left, f);
    putstring(f, node->name);
    putstring(f, node->definition);
    putref(f, node->implementor);
    macrodump(node->right, f);
}

/**
 * Fetch the next line from a given file handle
 *
 * This cleans off trailing \n and \r if applicable.  Returns an allocated
 * string copy -- the caller must free the memory.
 *
 * @private
 * @param f the file handle to read from
 * @return a copy of the file line
 */
static char *
file_line(FILE * f)
{
    char buf[BUFFER_LEN];
    int len;

    if (!fgets(buf, BUFFER_LEN, f))
        return NULL;

    len = strlen(buf);

    if (buf[len - 1] == '\n') {
        buf[--len] = '\0';
    }

    if (buf[len - 1] == '\r') {
        buf[--len] = '\0';
    }

    return alloc_string(buf);
}

/**
 * Load the macros from a file
 *
 * The file format is described in the save function.
 *
 * @see macrodump
 *
 * @param f the file handle to load from
 */
void
macroload(FILE * f)
{
    char *line, *code;
    dbref ref;

    while ((line = file_line(f))) {
        /*
         * SO!  As it turns out, C compilers are not required to execute
         * your function parameters in any particular order.  Thus,
         * if you do a simple line such as:
         *
         * (void)insert_macro(line, file_line(f), getref(f));
         *
         * you will actually get different results on different compilers.
         * We found this out the hard way :)  This may look clunky but
         * it is safe.
         */
        code = file_line(f);
        ref = getref(f);

        (void)insert_macro(line, code, ref);
        free(line);
        free(code);
    }
}

/**
 * Starts editing a program
 *
 * This does controls permission checking and makes sure nobody else
 * is editing the program (INTERNAL flag set on program object).  Does
 * not check if player is a MUCKER.
 *
 * @param player the player doing the editing
 * @param program the program to edit
 */
void
edit_program(dbref player, dbref program)
{
    char unparse_buf[BUFFER_LEN];

    if ((Typeof(program) != TYPE_PROGRAM) || !controls(player, program)) {
        notify(player, "Permission denied!");
        return;
    }

    if (FLAGS(program) & INTERNAL) {
        notify(player,
               "Sorry, this program is currently being edited by someone else.  Try again later.");
        return;
    }

    PROGRAM_SET_FIRST(program, read_program(program));
    FLAGS(program) |= INTERNAL;
    DBDIRTY(program);

    PLAYER_SET_CURR_PROG(player, program);
    FLAGS(player) |= INTERACTIVE;
    DBDIRTY(player);

    unparse_object(player, program, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Entering editor for %s.", unparse_buf);

    list_program(player, program, NULL, 0, 0);
}
