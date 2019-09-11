/** @file debugger.c
 *
 * This is the source file that defines the different functions that
 * support the MUF debugger.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "commands.h"
#include "compile.h"
#include "db.h"
#include "debugger.h"
#include "edit.h"
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "timequeue.h"
#include "tune.h"

/**
 * Resolve a (scoped or local-to-function) variable name to a variable number
 *
 * It is unclear what "level" is for because it is always passed as 0.
 * The svars can be in a linked list, and level determines how many nodes
 * down that list we will traverse before searching for 'varname'.
 *
 * @private
 * @param fr the current frame
 * @param level number of nodes in svar linked list to travel before searching
 * @param varname the variable name to look for
 * @return the variable number for this varname or -1 if not found
 */
static int
scopedvar_getnum(struct frame *fr, int level, const char *varname)
{
    struct scopedvar_t *svinfo = NULL;

    assert(varname != NULL);
    assert(*varname != '\0');

    svinfo = fr ? fr->svars : NULL;

    while (svinfo && level-- > 0)
        svinfo = svinfo->next;

    if (!svinfo) {
        return -1;
    }

    if (!svinfo->varnames) {
        return -1;
    }

    for (int varnum = 0; varnum < svinfo->count; varnum++) {
        assert(svinfo->varnames[varnum] != NULL);
        if (!strcasecmp(svinfo->varnames[varnum], varname)) {
            return varnum;
        }
    }

    return -1;
}

/**
 * List program lines to a player
 *
 * This is used by the debugger for listing lines and for backtraces.
 * Uses the editor's list_program under the hood.
 *
 * @see list_program
 *
 * @param player the player to send the listing to
 * @param program the program to list
 * @param fr the running program frame
 * @param start the start line
 * @param end the end line
 */
void
list_proglines(dbref player, dbref program, struct frame *fr, int start,
               int end)
{
    int range[2];
    int argc;

    if (start == end || end == 0) {
        range[0] = start;
        range[1] = start;
        argc = 1;
    } else {
        range[0] = start;
        range[1] = end;
        argc = 2;
    }

    /* Make sure we have the lines of code loaded for display */
    if (!fr->brkpt.proglines || program != fr->brkpt.lastproglisted) {
        free_prog_text(fr->brkpt.proglines);
        fr->brkpt.proglines = (struct line *) read_program(program);
        fr->brkpt.lastproglisted = program;
    }

    {
        /*
         * We have to change the program position in order to do the
         * listing, so we use this tmpline so we can reset it when
         * we are done.
         */
        struct line *tmpline = PROGRAM_FIRST(program);

        PROGRAM_SET_FIRST(program, fr->brkpt.proglines);

        {
            int tmpflg = (FLAGS(player) & INTERNAL);

            FLAGS(player) |= INTERNAL;

            list_program(player, program, range, argc, 0);

            if (!tmpflg) {
                FLAGS(player) &= ~INTERNAL;
            }
        }

        PROGRAM_SET_FIRST(program, tmpline);
    }

    return;
}

/**
 * This is for showing line listings in "instruction" format.
 *
 * This returns a single line of code's primitives.  It uses a static buffer,
 * so be careful with it.  Not multi-threaded.
 *
 * @param program the program to show listings for
 * @param pc the program counter where the listing starts
 * @param maxprims the maximum primitives
 * @param markpc do a special mark when listing the 'pc' line
 * @return pointer to static buffer containing primitives
 */
char *
show_line_prims(dbref program, struct inst *pc, int maxprims, int markpc)
{
    static char buf[BUFFER_LEN];
    static char buf2[BUFFER_LEN];
    int maxback;
    int thisline = pc->line;
    struct inst *code, *end, *linestart, *lineend;

    code = PROGRAM_CODE(program);
    end = code + PROGRAM_SIZ(program);
    buf[0] = '\0';

    /*
     * This code is to determine the end of the line.  There can be multiple
     * primitives on the same line.  This series of loops finds the end
     * of the line, so we can iterate over the primitives between start and
     * end to form our return string.
     */
    for (linestart = pc, maxback = maxprims; linestart > code &&
         linestart->line == thisline && linestart->type != PROG_FUNCTION &&
         --maxback; --linestart) ;

    if (linestart->line < thisline)
        ++linestart;

    for (lineend = pc + 1, maxback = maxprims; lineend < end &&
         lineend->line == thisline && lineend->type != PROG_FUNCTION &&
         --maxback; ++lineend) ;

    if (lineend >= end || lineend->line > thisline || lineend->type == PROG_FUNCTION)
        --lineend;

    if (lineend - linestart >= maxprims) {
        if (pc - (maxprims - 1) / 2 > linestart)
            linestart = pc - (maxprims - 1) / 2;

        if (linestart + maxprims - 1 < lineend)
            lineend = linestart + maxprims - 1;
    }

    if (linestart > code && (linestart - 1)->line == thisline)
        strcpyn(buf, sizeof(buf), "...");

    /*
     * We're in the right position; loop from linestart til lineend,
     * putting together text versions of the instructions.
     */ 
    while (linestart <= lineend) {
        if (strlen(buf) < BUFFER_LEN / 2) {
            if (*buf)
                strcatn(buf, sizeof(buf), " ");

            if (pc == linestart && markpc) {
                strcatn(buf, sizeof(buf), " {{");
                strcatn(buf, sizeof(buf),
                        insttotext(NULL, 0, linestart, buf2, sizeof(buf2), 30,
                                   program, 1));
                strcatn(buf, sizeof(buf), "}} ");
            } else {
                strcatn(buf, sizeof(buf),
                        insttotext(NULL, 0, linestart, buf2, sizeof(buf2), 30,
                                   program, 1));
            }
        } else {
            break;
        }

        linestart++;
    }

    if (lineend < end && (lineend + 1)->line == thisline)
        strcatn(buf, sizeof(buf), " ...");

    return buf;
}

/**
 * Convert a function name to an instruction pointer
 *
 * @private
 * @param program the program dbref
 * @param name the procedure name to look for
 * @return pointer to start of procedure
 */
static struct inst *
funcname_to_pc(dbref program, const char *name)
{
    int siz;
    struct inst *code;

    code = PROGRAM_CODE(program);
    siz = PROGRAM_SIZ(program);

    for (int i = 0; i < siz; i++) {
        if ((code[i].type == PROG_FUNCTION) &&
            !strcasecmp(name, code[i].data.mufproc->procname)) {
            return (code + i);
        }
    }

    return (NULL);
}

/**
 * Convert a line number to an instruction pointer
 *
 * @private
 * @param program dbref
 * @param whatline the line to find an instruction pointer for
 * @return pointer to starting instruction for the given line
 */
static struct inst *
linenum_to_pc(dbref program, int whatline)
{
    int siz;
    struct inst *code;

    code = PROGRAM_CODE(program);
    siz = PROGRAM_SIZ(program);

    for (int i = 0; i < siz; i++) {
        if (code[i].line == whatline) {
            return (code + i);
        }
    }

    return (NULL);
}

/**
 * Figures out the name of the current function for the given program counter
 *
 * It is functions like this why the documentation project is a thing.
 * Kind of undescriptive (even misleading) name, somewhat cryptic code.
 *
 * This 'rewinds' the code from the given pc location until it figures
 * out which function 'pc' lives in and returns a static buffer containing
 * pc's line number and containing function name information.
 *
 * Note that because this is a static buffer, you must be careful with
 * it; it is not thread-safe and it can theoretically mutate if you
 * hold onto the pointer for some reason.
 *
 * @private
 * @param program the dbref of the program
 * @param pc the program counter
 * @return a descriptive string with pc line number and function name
 */
static char *
unparse_sysreturn(dbref * program, struct inst *pc)
{
    static char buf[BUFFER_LEN];
    struct inst *ptr;
    char *fname;

    buf[0] = '\0';
    for (ptr = pc; ptr >= PROGRAM_CODE(*program); ptr--) {
        if (ptr->type == PROG_FUNCTION) {
            break;
        }
    }

    if (ptr->type == PROG_FUNCTION) {
        fname = ptr->data.mufproc->procname;
    } else {
        fname = "\033[1m???\033[0m";
    }

    snprintf(buf, sizeof(buf), "line \033[1m%d\033[0m, in \033[1m%s\033[0m", pc->line, fname);
    return buf;
}

/**
 * Return a string containing information about a given break point number
 *
 * Breakpoints are numbered from 0 onwards, but are displayed to the user
 * as starting from 1.
 *
 * @private
 * @param fr the current frame
 * @param brk the break point number to get information for
 * @return the string containing break point information
 */
static char *
unparse_breakpoint(struct frame *fr, int brk)
{
    static char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    dbref ref;

    snprintf(buf, sizeof(buf), "%2d) break", brk + 1);

    if (fr->brkpt.line[brk] != -1) {
        snprintf(buf2, sizeof(buf2), " in line %d", fr->brkpt.line[brk]);
        strcatn(buf, sizeof(buf), buf2);
    }

    if (fr->brkpt.pc[brk] != NULL) {
        ref = fr->brkpt.prog[brk];
        snprintf(buf2, sizeof(buf2), " at %s",
                 unparse_sysreturn(&ref, fr->brkpt.pc[brk] + 1));
        strcatn(buf, sizeof(buf), buf2);
    }

    if (fr->brkpt.linecount[brk] != -2) {
        snprintf(buf2, sizeof(buf2),
                 " after %d line(s)", fr->brkpt.linecount[brk]);
        strcatn(buf, sizeof(buf), buf2);
    }

    if (fr->brkpt.pccount[brk] != -2) {
        snprintf(buf2, sizeof(buf2),
                 " after %d instruction(s)", fr->brkpt.pccount[brk]);
        strcatn(buf, sizeof(buf), buf2);
    }

    if (fr->brkpt.prog[brk] != NOTHING) {
        snprintf(buf2, sizeof(buf2), " in %s(#%d)", NAME(fr->brkpt.prog[brk]),
                 fr->brkpt.prog[brk]);
        strcatn(buf, sizeof(buf), buf2);
    }

    if (fr->brkpt.level[brk] != -1) {
        snprintf(buf2, sizeof(buf2), " on call level %d", fr->brkpt.level[brk]);
        strcatn(buf, sizeof(buf), buf2);
    }

    return buf;
}

/**
 * Displays a MUF Backtrace message.
 *
 * This is used when a MUF program is aborted, if someone who has 'controls'
 * permissions was running it, or if you type 'where' in the debugger.
 * Shows a fairly straight forward backtrace, going 'count' deep.  If
 * 'count' is 0, it will display up to STACK_SIZE depth.
 *
 * @param player the player getting the backtrace
 * @param program the program being backtraced
 * @param count the depth of the trace, or 0 for STACK_SIZE
 * @param fr the frame pointer
 */
void
muf_backtrace(dbref player, dbref program, int count, struct frame *fr)
{
    char buf2[BUFFER_LEN];
    char buf3[BUFFER_LEN];
    char *ptr;
    dbref ref;
    int i, j, cnt, flag;
    struct inst *pinst, *lastinst;
    int lev;

    notify_nolisten(player, "\033[1;33;40mSystem stack backtrace:\033[0m", 1);
    i = count;

    if (!i)
        i = STACK_SIZE;

    ref = program;
    pinst = NULL;
    j = fr->system.top + 1;

    /*
     * i is our depth; iterate until we've run out of depth or we have
     * broken out of the loop
     */
    while (j > 1 && i-- > 0) {
        cnt = 0;

        do {
            lastinst = pinst;

            if (--j == fr->system.top) {
                pinst = fr->pc;
            } else {
                ref = fr->system.st[j].progref;
                pinst = fr->system.st[j].offset;
            }

            ptr = unparse_sysreturn(&ref, pinst);
            cnt++;
        } while (pinst == lastinst && j > 1);

        if (cnt > 1) {
            notifyf_nolisten(player, "     [repeats %d times]", cnt);
        }

        lev = fr->system.top - j;

        if (ptr) {
            int snplen;
            char *bufend = buf2;
            struct inst *fntop = fr->pc;
            struct inst *varinst;

            while (fntop->type != PROG_FUNCTION)
                fntop--;

            snplen = snprintf(buf2, sizeof(buf2), "%.512s\033[1m(\033[0m", ptr);

            if (snplen == -1) {
                buf2[sizeof(buf2) - 1] = '\0';
                snplen = sizeof(buf2) - 1;
            }

            bufend += snplen;

            for (int k = 0; k < fntop->data.mufproc->args; k++) {
                const char *nam = scopedvar_getname(fr, lev, k);
                char *val;

                if (!nam) {
                    break;
                }

                varinst = scopedvar_get(fr, lev, k);
                val = insttotext(fr, lev, varinst, buf3, sizeof(buf3), 30, program, 1);

                if (k) {
                    bufend += snprintf(bufend, buf2 - bufend - 18, "\033[1m, %s=\033[0m%s", nam, val);
                } else {
                    bufend += snprintf(bufend, buf2 - bufend - 18, "\033[1m%s=\033[0m%s", nam, val);
                }
            }

            ptr = buf2;
        }

        if (pinst != lastinst) {
            notifyf_nolisten(player, "\033[1;33;40m%3d)\033[0m \033[1m%s(#%d)\033[0m %s:", lev,
                             NAME(ref), ref, ptr);
            flag = ((FLAGS(player) & INTERNAL) ? 1 : 0);
            FLAGS(player) &= ~INTERNAL;
            list_proglines(player, ref, fr, pinst->line, 0);

            if (flag) {
                FLAGS(player) |= INTERNAL;
            }
        }
    }

    notify_nolisten(player, "\033[1;33;40m*done*\033[0m", 1);
}

/**
 * List the functions in a program
 *
 * Optionally provide a filter string that will match functions using
 * equalstr
 *
 * @see equalstr
 *
 * You cannot pass NULL into arg, you can pass empty string if you wish to
 * show all functions.
 *
 * @private
 * @param player the person to show the list of functions to
 * @param program the program to list functions from
 * @param arg a filter string or ""
 */
static void
list_program_functions(dbref player, dbref program, char *arg)
{
    struct inst *ptr;
    int count;

    ptr = PROGRAM_CODE(program);
    count = PROGRAM_SIZ(program);
    notify_nolisten(player, "*function words*", 1);

    while (count-- > 0) {
        if (ptr->type == PROG_FUNCTION) {
            if (ptr->data.mufproc) {
                if (!*arg || equalstr(arg, ptr->data.mufproc->procname)) {
                    notify_nolisten(player, ptr->data.mufproc->procname, 1);
                }
            }
        }

        ptr++;
    }

    notify_nolisten(player, "*done*", 1);
}

/**
 * Print variable information
 *
 * Takes an argument string which is parsed to determine what to display.
 * Argument can look like:
 *
 * * a variable name
 * * L<num> or LV<num>
 * * L<name> or LV<name>
 * * S<num> or SV<num>
 * * V<num>
 *
 * L / LV is for local variables.  Just a name, or S / SV searches scoped
 * variables.  And finally V is for global variables.
 *
 * @private
 * @param player the player to show variable information
 * @param program the program that is running
 * @param fr the frame pointer
 * @param arg the argument as described above
 */
static void
debug_printvar(dbref player, dbref program, struct frame *fr, const char *arg)
{
    int i;
    int lflag = 0;
    int sflag = 0;
    int varnum = -1;
    char buf[BUFFER_LEN];

    if (!arg || !*arg) {
        notify_nolisten(player, "I don't know which variable you mean.", 1);
        return;
    }

    /*
     * If 'sflag' is set, we will be looking up a scoped variable
     * If 'lflag' is set, we will be looking up a local variable
     * If neither is set, then we will be looking up a global variable.
     *
     * This code figures out the variable number (varnum) we will look up.
     */
    varnum = scopedvar_getnum(fr, 0, arg);
    if (varnum != -1) {
        sflag = 1;
    } else {
        if (*arg == 'L' || *arg == 'l') {
            arg++;

            if (*arg == 'V' || *arg == 'v') {
                arg++;
            }

            lflag = 1;
            varnum = scopedvar_getnum(fr, 0, arg);
        } else if (*arg == 'S' || *arg == 's') {
            arg++;

            if (*arg == 'V' || *arg == 'v') {
                arg++;
            }

            sflag = 1;
        } else if (*arg == 'V' || *arg == 'v') {
            arg++;
        }
    }

    /*
     * If varnum > -1, then we have found it and we need not do more
     * searching.
     *
     * If it is -1, we'll try to turn the remainder of 'arg' into a number
     * and see if that matches.
     *
     * Otherwise, error.
     */
    if (varnum > -1) {
        i = varnum;
    } else if (number(arg)) {
        i = atoi(arg);
    } else {
        notify_nolisten(player, "I don't know which variable you mean.", 1);
        return;
    }

    /* Out of bounds */
    if (i >= MAX_VAR || i < 0) {
        notify_nolisten(player, "Variable number out of range.", 1);
        return;
    }

    if (sflag) {
        struct inst *tmp = scopedvar_get(fr, 0, i);

        if (!tmp) {
            notify_nolisten(player, "Scoped variable number out of range.", 1);
            return;
        }

        notify_nolisten(player, insttotext(fr, 0, tmp, buf, sizeof(buf), 4000, -1, 1), 1);
    } else if (lflag) {
        struct localvars *lvars = localvars_get(fr, program);
        notify_nolisten(player,
                        insttotext(fr, 0, &(lvars->lvars[i]), buf, sizeof(buf), 4000, -1, 1),
                        1);
    } else {
        notify_nolisten(player,
                        insttotext(fr, 0, &(fr->variables[i]), buf, sizeof(buf), 4000, -1, 1),
                        1);
    }
}

/**
 * Push something into the stack from the debugger
 *
 * This can be a number, float, string, dbref, or variable.
 *
 * Variables are parsed almost the same as debug_printvar.  In fact, it
 * looks pretty copy-pasty.  For variable formating rules,
 * @see debug_printvar
 *
 * @private
 * @param player the player doing the pushing
 * @param fr the frame pointer
 * @param arg the argument which is a string containing something to push
 */
static void
push_arg(dbref player, struct frame *fr, const char *arg)
{
    int num, lflag = 0;
    int sflag = 0;
    double inum;

    if (fr->argument.top >= STACK_SIZE) {
        notify_nolisten(player, "That would overflow the stack.", 1);
        return;
    }

    if (number(arg)) {
        /* push a number */
        num = atoi(arg);
        push(fr->argument.st, &fr->argument.top, PROG_INTEGER, &num);
        notify_nolisten(player, "Integer pushed.", 1);
    } else if (ifloat(arg)) {
        /* push a float */
        inum = atof(arg);
        push(fr->argument.st, &fr->argument.top, PROG_FLOAT, &inum);
        notify_nolisten(player, "Float pushed.", 1);
    } else if (*arg == NUMBER_TOKEN) {
        /* push a dbref */
        if (!number(arg + 1)) {
            notify_nolisten(player, "I don't understand that dbref.", 1);
            return;
        }

        num = atoi(arg + 1);
        push(fr->argument.st, &fr->argument.top, PROG_OBJECT, &num);
        notify_nolisten(player, "Dbref pushed.", 1);
    } else if (*arg == '"') {
        /* push a string */
        char buf[BUFFER_LEN];
        char *ptr;
        const char *ptr2;

        for (ptr = buf, ptr2 = arg + 1; *ptr2; ptr2++) {
            if (*ptr2 == '\\') {
                if (!*(++ptr2))
                    break;

                *ptr++ = *ptr2;
            } else if (*ptr2 == '"') {
                break;
            } else {
                *ptr++ = *ptr2;
            }
        }

        *ptr = '\0';
        push(fr->argument.st, &fr->argument.top, PROG_STRING, alloc_prog_string(buf));
        notify_nolisten(player, "String pushed.", 1);
    } else {
        /*
         * @TODO This is basically copy/pasted from debug_printvar.
         *       The variable resolution should probably be a common
         *       function.
         */
        int varnum = scopedvar_getnum(fr, 0, arg);

        if (varnum != -1) {
            sflag = 1;
        } else {
            if (*arg == 'S' || *arg == 's') {
                arg++;

                if (*arg == 'V' || *arg == 'v') {
                    arg++;
                }

                sflag = 1;
                varnum = scopedvar_getnum(fr, 0, arg);
            } else if (*arg == 'L' || *arg == 'l') {
                arg++;

                if (*arg == 'V' || *arg == 'v') {
                    arg++;
                }

                lflag = 1;
            } else if (*arg == 'V' || *arg == 'v') {
                arg++;
            }
        }

        if (varnum > -1) {
            num = varnum;
        } else if (number(arg)) {
            num = atoi(arg);
        } else {
            notify_nolisten(player, "I don't understand what you want to push.", 1);
            return;
        }

        if (lflag) {
            push(fr->argument.st, &fr->argument.top, PROG_LVAR, &num);
            notify_nolisten(player, "Local variable pushed.", 1);
        } else if (sflag) {
            push(fr->argument.st, &fr->argument.top, PROG_SVAR, &num);
            notify_nolisten(player, "Scoped variable pushed.", 1);
        } else {
            push(fr->argument.st, &fr->argument.top, PROG_VAR, &num);
            notify_nolisten(player, "Global variable pushed.", 1);
        }
    }
}

/**
 * Implementation of the MUF debugger
 *
 * This implements the command parsing for the MUF debugger.  It also clears
 * temporary bookmarks if this was triggered from a temporary one.
 *
 * This relies on some static globals, so it is not threadsafe.  If the
 * 'prim' debugger command is ever used to trigger the MUF debugger somehow
 * in a recursive fashion, and then you call 'prim' again, it will probably
 * cause havock.
 *
 * @param descr the descriptor of the debugging player
 * @param player the debugging player
 * @param program the program we are debugging
 * @param text the input text from the user
 * @param fr the current frame pointer
 * @return boolean true if the program should exit, false if not
 */
int
muf_debugger(int descr, dbref player, dbref program, const char *text, struct frame *fr)
{
    char cmd[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2, *arg;
    struct inst *pinst;
    int i, j, cnt;
    static struct inst primset[5];
    static struct muf_proc_data temp_muf_proc_data = {
        "__Temp_Debugger_Proc",
        0,
        0,
        NULL
    };

    /*
     * Basic massaging of the input - clearing spaces, finding the
     * argument.
     */
    skip_whitespace(&text);
    strcpyn(cmd, sizeof(cmd), text);

    ptr = cmd;
    remove_ending_whitespace(&ptr);

    for (arg = cmd; *arg && !isspace(*arg); arg++) ;

    if (*arg)
        *arg++ = '\0';

    /* Empty command means repeat last command, if available */
    if (!*cmd && fr->brkpt.lastcmd) {
        strcpyn(cmd, sizeof(cmd), fr->brkpt.lastcmd);
    } else {
        free(fr->brkpt.lastcmd);

        if (*cmd)
            fr->brkpt.lastcmd = strdup(cmd);
    }

    /* delete triggering breakpoint, if it's only temp. */
    j = fr->brkpt.breaknum;

    if (j >= 0 && fr->brkpt.temp[j]) {
        for (j++; j < fr->brkpt.count; j++) {
            fr->brkpt.temp[j - 1] = fr->brkpt.temp[j];
            fr->brkpt.level[j - 1] = fr->brkpt.level[j];
            fr->brkpt.line[j - 1] = fr->brkpt.line[j];
            fr->brkpt.linecount[j - 1] = fr->brkpt.linecount[j];
            fr->brkpt.pc[j - 1] = fr->brkpt.pc[j];
            fr->brkpt.pccount[j - 1] = fr->brkpt.pccount[j];
            fr->brkpt.prog[j - 1] = fr->brkpt.prog[j];
        }

        fr->brkpt.count--;
    }

    fr->brkpt.breaknum = -1;

    /**
     * @TODO This giant if statement is pretty gnarly; we'd be better
     *       of looping over an array of callbacks to break this up
     *       nicer and make it more extensible.
     */

    if (!strcasecmp(cmd, "cont")) {
        /* Nothing to do -- this will continue to next breakpoint */
    } else if (!strcasecmp(cmd, "finish")) {
        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player,
                            "Cannot finish because there are too many breakpoints set.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = fr->system.top - 1;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = -2;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = -2;
        fr->brkpt.prog[j] = program;
        fr->brkpt.bypass = 1;

        return 0;
    } else if (!strcasecmp(cmd, "stepi")) {
        i = atoi(arg);

        if (!i)
            i = 1;

        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player, "Cannot stepi because there are too many breakpoints set.",
                            1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = -1;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = -2;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = i;
        fr->brkpt.prog[j] = NOTHING;
        fr->brkpt.bypass = 1;

        return 0;
    } else if (!strcasecmp(cmd, "step")) {
        i = atoi(arg);

        if (!i)
            i = 1;

        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player, "Cannot step because there are too many breakpoints set.",
                            1);

            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = -1;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = i;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = -2;
        fr->brkpt.prog[j] = NOTHING;
        fr->brkpt.bypass = 1;

        return 0;
    } else if (!strcasecmp(cmd, "nexti")) {
        i = atoi(arg);

        if (!i)
            i = 1;

        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player, "Cannot nexti because there are too many breakpoints set.",
                            1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = fr->system.top;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = -2;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = i;
        fr->brkpt.prog[j] = program;
        fr->brkpt.bypass = 1;

        return 0;
    } else if (!strcasecmp(cmd, "next")) {
        i = atoi(arg);

        if (!i)
            i = 1;

        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player, "Cannot next because there are too many breakpoints set.",
                            1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = fr->system.top;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = i;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = -2;
        fr->brkpt.prog[j] = program;
        fr->brkpt.bypass = 1;

        return 0;
    } else if (!strcasecmp(cmd, "exec")) {
        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player,
                            "Cannot finish because there are too many breakpoints set.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        if (!(pinst = funcname_to_pc(program, arg))) {
            notify_nolisten(player, "I don't know a function by that name.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        if (fr->system.top >= STACK_SIZE) {
            notify_nolisten(player,
                            "That would exceed the system stack size for this program.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        fr->system.st[fr->system.top].progref = program;
        fr->system.st[fr->system.top++].offset = fr->pc;
        fr->pc = pinst;
        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = fr->system.top - 1;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = -2;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = -2;
        fr->brkpt.prog[j] = program;
        fr->brkpt.bypass = 1;

        return 0;
    } else if (!strcasecmp(cmd, "prim")) {
        /*
         * @TODO The way this works is a little funky.  It looks like
         *       it would be possible to cause some weird havoc if we
         *       manage to run a primitive that in turn triggers muf_debugger
         *
         *       I am uncertain if this is possible; looks like the only
         *       way it could happen is by typing 'prim debugger_break'
         *       but I don't know much about about how muf_debugger is
         *       triggered.  Some digging should be done to make this
         *       safe.
         *
         *       Even better would be to not use statics for this somehow
         *       without introducing a memory leak. (tanabi)
         */
        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player,
                            "Cannot finish because there are too many breakpoints set.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        if (!primitive(arg)) {
            notify_nolisten(player, "I don't recognize that primitive.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        if (fr->system.top >= STACK_SIZE) {
            notify_nolisten(player,
                            "That would exceed the system stack size for this program.", 1);
            add_muf_read_event(descr, player, program, fr);
            return 0;
        }

        primset[0].type = PROG_FUNCTION;
        primset[0].line = 0;
        primset[0].data.mufproc = &temp_muf_proc_data;
        primset[0].data.mufproc->vars = 0;
        primset[0].data.mufproc->args = 0;
        primset[0].data.mufproc->varnames = NULL;

        primset[1].type = PROG_PRIMITIVE;
        primset[1].line = 0;
        primset[1].data.number = get_primitive(arg);

        primset[2].type = PROG_PRIMITIVE;
        primset[2].line = 0;
        primset[2].data.number = IN_RET;

        /* primset[3].data.number = primitive("EXIT"); */

        fr->system.st[fr->system.top].progref = program;
        fr->system.st[fr->system.top++].offset = fr->pc;
        fr->pc = &primset[1];
        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 1;
        fr->brkpt.level[j] = -1;
        fr->brkpt.line[j] = -1;
        fr->brkpt.linecount[j] = -2;
        fr->brkpt.pc[j] = &primset[2];
        fr->brkpt.pccount[j] = -2;
        fr->brkpt.prog[j] = program;
        fr->brkpt.bypass = 1;
        fr->brkpt.dosyspop = 1;

        return 0;
    } else if (!strcasecmp(cmd, "break")) {
        add_muf_read_event(descr, player, program, fr);

        if (fr->brkpt.count >= MAX_BREAKS) {
            notify_nolisten(player, "Too many breakpoints set.", 1);
            return 0;
        }

        if (number(arg)) {
            i = atoi(arg);
        } else {
            if (!(pinst = funcname_to_pc(program, arg))) {
                notify_nolisten(player, "I don't know a function by that name.", 1);
                return 0;
            } else {
                i = pinst->line;
            }
        }

        if (!i)
            i = fr->pc->line;

        j = fr->brkpt.count++;
        fr->brkpt.temp[j] = 0;
        fr->brkpt.level[j] = -1;
        fr->brkpt.line[j] = i;
        fr->brkpt.linecount[j] = -2;
        fr->brkpt.pc[j] = NULL;
        fr->brkpt.pccount[j] = -2;
        fr->brkpt.prog[j] = program;
        notify_nolisten(player, "Breakpoint set.", 1);

        return 0;
    } else if (!strcasecmp(cmd, "delete")) {
        add_muf_read_event(descr, player, program, fr);
        i = atoi(arg);

        if (!i) {
            notify_nolisten(player, "Which breakpoint did you want to delete?", 1);
            return 0;
        }

        if (i < 1 || i > fr->brkpt.count) {
            notify_nolisten(player, "No such breakpoint.", 1);
            return 0;
        }

        j = i - 1;

        for (j++; j < fr->brkpt.count; j++) {
            fr->brkpt.temp[j - 1] = fr->brkpt.temp[j];
            fr->brkpt.level[j - 1] = fr->brkpt.level[j];
            fr->brkpt.line[j - 1] = fr->brkpt.line[j];
            fr->brkpt.linecount[j - 1] = fr->brkpt.linecount[j];
            fr->brkpt.pc[j - 1] = fr->brkpt.pc[j];
            fr->brkpt.pccount[j - 1] = fr->brkpt.pccount[j];
            fr->brkpt.prog[j - 1] = fr->brkpt.prog[j];
        }

        fr->brkpt.count--;
        notify_nolisten(player, "Breakpoint deleted.", 1);

        return 0;
    } else if (!strcasecmp(cmd, "breaks")) {
        notify_nolisten(player, "Breakpoints:", 1);

        for (i = 0; i < fr->brkpt.count; i++) {
            ptr = unparse_breakpoint(fr, i);
            notify_nolisten(player, ptr, 1);
        }

        notify_nolisten(player, "*done*", 1);
        add_muf_read_event(descr, player, program, fr);

        return 0;
    } else if (!strcasecmp(cmd, "where")) {
        i = atoi(arg);
        muf_backtrace(player, program, i, fr);
        add_muf_read_event(descr, player, program, fr);
        return 0;
    } else if (!strcasecmp(cmd, "stack")) {
        notify_nolisten(player, "*Argument stack top*", 1);
        i = atoi(arg);

        if (!i)
            i = STACK_SIZE;

        ptr = "";

        for (j = fr->argument.top; j > 0 && i-- > 0;) {
            cnt = 0;

            do {
                strcpyn(buf, sizeof(buf), ptr);
                ptr = insttotext(NULL, 0, &fr->argument.st[--j], buf2, sizeof(buf2), 4000,
                                 program, 1);
                cnt++;
            } while (!strcasecmp(ptr, buf) && j > 0);

            if (cnt > 1)
                notifyf(player, "     [repeats %d times]", cnt);

            if (strcasecmp(ptr, buf))
                notifyf(player, "%3d) %s", j + 1, ptr);
        }

        notify_nolisten(player, "*done*", 1);
        add_muf_read_event(descr, player, program, fr);

        return 0;
    } else if (!strcasecmp(cmd, "list") || !strcasecmp(cmd, "listi")) {
        int startline, endline;

        add_muf_read_event(descr, player, program, fr);

        if ((ptr2 = (char *) strchr(arg, ','))) {
            *ptr2++ = '\0';
        } else {
            ptr2 = "";
        }

        if (!*arg) {
            if (fr->brkpt.lastlisted) {
                startline = fr->brkpt.lastlisted + 1;
            } else {
                startline = fr->pc->line;
            }

            endline = startline + 15;
        } else {
            if (!number(arg)) {
                if (!(pinst = funcname_to_pc(program, arg))) {
                    notify_nolisten(player,
                                    "I don't know a function by that name. (starting arg, 1)",
                                    1);
                    return 0;
                } else {
                    startline = pinst->line;
                    endline = startline + 15;
                }
            } else {
                if (*ptr2) {
                    endline = startline = atoi(arg);
                } else {
                    startline = atoi(arg) - 7;
                    endline = startline + 15;
                }
            }
        }

        if (*ptr2) {
            if (!number(ptr2)) {
                if (!(pinst = funcname_to_pc(program, ptr2))) {
                    notify_nolisten(player,
                                    "I don't know a function by that name. (ending arg, 1)",
                                    1);
                    return 0;
                } else {
                    endline = pinst->line;
                }
            } else {
                endline = atoi(ptr2);
            }
        }

        i = (PROGRAM_CODE(program) + PROGRAM_SIZ(program) - 1)->line;

        if (startline > i) {
            notify_nolisten(player, "Starting line is beyond end of program.", 1);
            return 0;
        }

        if (startline < 1)
            startline = 1;

        if (endline > i)
            endline = i;

        if (endline < startline)
            endline = startline;

        notify_nolisten(player, "Listing:", 1);

        if (!strcasecmp(cmd, "listi")) {
            for (i = startline; i <= endline; i++) {
                pinst = linenum_to_pc(program, i);

                if (pinst) {
                    notifyf_nolisten(player, "line %d: %s", i, (i == fr->pc->line) ?
                                     show_line_prims(program, fr->pc, STACK_SIZE, 1) :
                                     show_line_prims(program, pinst, STACK_SIZE, 0));
                }
            }
        } else {
            list_proglines(player, program, fr, startline, endline);
        }

        fr->brkpt.lastlisted = endline;
        notify_nolisten(player, "*done*", 1);

        return 0;
    } else if (!strcasecmp(cmd, "quit")) {
        notify_nolisten(player, "Halting execution.", 1);
        return 1;
    } else if (!strcasecmp(cmd, "trace")) {
        add_muf_read_event(descr, player, program, fr);

        if (!strcasecmp(arg, "on")) {
            fr->brkpt.showstack = 1;
            notify_nolisten(player, "Trace turned on.", 1);
        } else if (!strcasecmp(arg, "off")) {
            fr->brkpt.showstack = 0;
            notify_nolisten(player, "Trace turned off.", 1);
        } else {
            notifyf_nolisten(player, "Trace is currently %s.",
                             fr->brkpt.showstack ? "on" : "off");
        }

        return 0;
    } else if (!strcasecmp(cmd, "words")) {
        list_program_functions(player, program, arg);
        add_muf_read_event(descr, player, program, fr);
        return 0;
    } else if (!strcasecmp(cmd, "print")) {
        debug_printvar(player, program, fr, arg);
        add_muf_read_event(descr, player, program, fr);
        return 0;
    } else if (!strcasecmp(cmd, "push")) {
        push_arg(player, fr, arg);
        add_muf_read_event(descr, player, program, fr);
        return 0;
    } else if (!strcasecmp(cmd, "pop")) {
        add_muf_read_event(descr, player, program, fr);

        if (fr->argument.top < 1) {
            notify_nolisten(player, "Nothing to pop.", 1);
            return 0;
        }

        fr->argument.top--;
        CLEAR(fr->argument.st + fr->argument.top);
        notify_nolisten(player, "Stack item popped.", 1);

        return 0;
    } else if (!strcasecmp(cmd, "help")) {
        do_helpfile(player, tp_file_man_dir, tp_file_man, "debugger_commands", "");

        return 0;
    } else {
        notify_nolisten(player,
                        "I don't understand that debugger command. Type 'help' for help.", 1);
        add_muf_read_event(descr, player, program, fr);
        return 0;
    }

    return 0;
}
