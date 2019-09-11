/** @file interp.c
 *
 * Source file that supports the MUF interpreter.  This has a lot of the
 * fundamentals for the whole MUF system.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "compile.h"
#include "db.h"
#include "debugger.h"
#include "edit.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#ifdef MCPGUI_SUPPORT
#include "mcpgui.h"
#endif
#include "mufevent.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

#define ERROR_DIE_NOW -1

/**
 * Placeholder "null" primitive implementation for pseudo-primitives.
 *
 * JUMP, READ, SLEEP, and a few others aren't actually primitives but are
 * more like constructs.  We use these to pad the primitives array so that
 * they have an instruction number, etc.
 *
 * @private
 * @param PRIM_PROTOTYPE the standard primitive parameters
 */
static void
p_null(PRIM_PROTOTYPE)
{
    return;
}

/**
 * Array of primitive implementations.  To add a new primitive,
 * you actually need to add to one of the #define's used below.
 *
 * The defines live in the different p_*.h in include.
 */
static void (*prim_func[]) (PRIM_PROTOTYPE) = {
    p_null, p_null, p_null, p_null, p_null, p_null,
            /* JMP, READ,   SLEEP,  CALL,   EXECUTE, RETURN, */
            p_null, p_null, p_null,
            /* EVENT_WAITFOR, CATCH,  CATCH_DETAILED */
            PRIMS_CONNECTS_FUNCS,
            PRIMS_DB_FUNCS,
            PRIMS_MATH_FUNCS,
            PRIMS_MISC_FUNCS,
            PRIMS_PROPS_FUNCS,
            PRIMS_STACK_FUNCS,
            PRIMS_STRINGS_FUNCS,
            PRIMS_ARRAY_FUNCS,
            PRIMS_FLOAT_FUNCS,
            PRIMS_ERROR_FUNCS,
            PRIMS_MCP_FUNCS,
#ifdef MCPGUI_SUPPORT
            PRIMS_MCPGUI_FUNCS,
#endif
            PRIMS_REGEX_FUNCS,
            PRIMS_INTERNAL_FUNCS,
            NULL
};

/**
 * Get the local (lvar) variables for a given frame and program combination.
 *
 * @param fr the frame structure
 * @param prog the program DB ref
 * @return localvars structure
 */
struct localvars *
localvars_get(struct frame *fr, dbref prog)
{
    if (!fr) {
        panic("localvars_get(): NULL frame passed !");
    }

    struct localvars *tmp = fr->lvars;

    while (tmp && tmp->prog != prog)
        tmp = tmp->next;

    if (tmp) {
        /* Pull this out of the middle of the stack. */
        *tmp->prev = tmp->next;

        if (tmp->next)
            tmp->next->prev = tmp->prev;
    } else {
        /* Create a new var frame. */
        int count = MAX_VAR;

        tmp = malloc(sizeof(struct localvars));
        tmp->prog = prog;

        while (count-- > 0) {
            tmp->lvars[count].type = PROG_INTEGER;
            tmp->lvars[count].data.number = 0;
        }
    }

    /* Add this to the head of the stack. */
    tmp->next = fr->lvars;
    tmp->prev = &fr->lvars;
    fr->lvars = tmp;

    if (tmp->next)
        tmp->next->prev = &tmp->next;

    return tmp;
}

/**
 * Copy local vars from one frame to another
 *
 * This is, so far, just used by the fork primitive to copy lvar's from
 * one program to another.
 *
 * @param fr the new frame
 * @param oldfr the source frame
 */
void
localvar_dupall(struct frame *fr, struct frame *oldfr)
{
    if (!fr || !oldfr) {
        panic("localvar_dupall(): NULL frame passed !");
    }

    struct localvars *orig = oldfr->lvars;
    struct localvars **targ = &fr->lvars;

    while (orig) {
        int count = MAX_VAR;
        *targ = malloc(sizeof(struct localvars));

        while (count-- > 0)
            deep_copyinst(&orig->lvars[count], &(*targ)->lvars[count], -1);

        (*targ)->prog = orig->prog;
        (*targ)->next = NULL;
        (*targ)->prev = targ;
        targ = &((*targ)->next);
        orig = orig->next;
    }
}

/**
 * Free memory for all local variables associated with the given frame.
 *
 * @private
 * @param fr the frame to free local variables from
 */
static void
localvar_freeall(struct frame *fr)
{
    if (!fr) {
        panic("localvar_freeall(): NULL frame passed !");
    }

    struct localvars *ptr = fr->lvars;
    struct localvars *nxt;

    while (ptr) {
        int count = MAX_VAR;
        nxt = ptr->next;

        while (count-- > 0)
            CLEAR(&ptr->lvars[count]);

        ptr->next = NULL;
        ptr->prev = NULL;
        ptr->prog = NOTHING;
        free(ptr);
        ptr = nxt;
    }

    fr->lvars = NULL;
}

/**
 * Add a scoped (function level) variable "level".
 *
 * The level refers to function level, each function level having its own
 * set of scoped variables.
 *
 * @private
 * @param fr the frame
 * @param pc the pointer to current instruction
 * @param count the function depth
 */
static void
scopedvar_addlevel(struct frame *fr, struct inst *pc, int count)
{
    if (!fr) {
        panic("scopedvar_addlevel(): NULL frame passed !");
    }

    struct scopedvar_t *tmp;
    size_t siz = sizeof(struct scopedvar_t) + (sizeof(struct inst) * ((size_t)count - 1));

    tmp = malloc(siz);
    tmp->count = (size_t)count;
    tmp->varnames = pc->data.mufproc->varnames;
    tmp->next = fr->svars;
    fr->svars = tmp;

    while (count-- > 0) {
        tmp->vars[count].type = PROG_INTEGER;
        tmp->vars[count].data.number = 0;
    }
}

/**
 * Duplicate all scoped variables from one frame to another.
 *
 * This is currently only used for fork.
 *
 * @param fr the destination frame
 * @param oldfr the source frame
 */
void
scopedvar_dupall(struct frame *fr, struct frame *oldfr)
{
    if (!fr || !oldfr) {
        panic("scopedvar_dupall(): NULL frame passed !");
    }

    struct scopedvar_t *newsv;
    struct scopedvar_t **prev;
    size_t siz, count;

    prev = &fr->svars;
    *prev = NULL;

    for (struct scopedvar_t *cur = oldfr->svars; cur; cur = cur->next) {
        count = (size_t)cur->count;
        siz = sizeof(struct scopedvar_t) + (sizeof(struct inst) * (count - 1));

        newsv = malloc(siz);
        newsv->count = count;
        newsv->varnames = cur->varnames;
        newsv->next = NULL;

        while (count-- > 0) {
            deep_copyinst(&cur->vars[count], &newsv->vars[count], -1);
        }

        *prev = newsv;
        prev = &newsv->next;
    }
}

/**
 * Pop a level of scoped variables off
 *
 * This is used, for instance, when returning from a function.  Frees up
 * associated memory.
 *
 * @private
 * @param fr the frame to pop a level of scoped variables from
 * @return boolean true if something was popped, false if not.
 */
static int
scopedvar_poplevel(struct frame *fr)
{
    if (!fr || !fr->svars) {
        return 0;
    }

    struct scopedvar_t *tmp = fr->svars;
    fr->svars = fr->svars->next;

    while (tmp->count-- > 0) {
        CLEAR(&tmp->vars[tmp->count]);
    }

    free(tmp);
    return 1;
}

/**
 * Free all scoped variables associated with a frame.
 *
 * Normally, I'm opposed to these little single line functions.  But this
 * is a mirror to localvar_freeall so we'll keep it for consistency.
 *
 * @private
 * @param fr the frame to free scoped variables from.
 */
static inline void
scopedvar_freeall(struct frame *fr)
{
    while (scopedvar_poplevel(fr)) ;
}

/**
 * Get a scoped variable for the given number, with the given number
 *
 * @param fr the frame to get the variable from
 * @param level the function level to get from
 * @param varnum the number "id" of the variable to get
 * @return the instruction associated with the requested variable or NULL
 */
struct inst *
scopedvar_get(struct frame *fr, int level, int varnum)
{
    struct scopedvar_t *svinfo = fr ? fr->svars : NULL;

    while (svinfo && level-- > 0)
        svinfo = svinfo->next;

    if (!svinfo) {
        return NULL;
    }

    if (varnum < 0 || varnum >= svinfo->count) {
        return NULL;
    }

    return (&svinfo->vars[varnum]);
}

/**
 * For a given program location and var number, get the var name
 *
 * This is for function-scoped variables.  This will find the variable
 * in the current scope of 'pc'.
 *
 * @param pc the current program location
 * @param varnum the variable number to fetch
 * @return constant variable name - do not free this memory
 */
const char *
scopedvar_getname_byinst(struct inst *pc, int varnum)
{
    while (pc && pc->type != PROG_FUNCTION)
        pc--;

    if (!pc || !pc->data.mufproc) {
        return NULL;
    }

    if (varnum < 0 || varnum >= pc->data.mufproc->vars) {
        return NULL;
    }

    if (!pc->data.mufproc->varnames) {
        return NULL;
    }

    return pc->data.mufproc->varnames[varnum];
}

/**
 * Get a function scoped variable name by frame, function level, and number
 *
 * @param fr the frame to look up
 * @param level the function level
 * @param varnum the variable number
 * @return constant variable name - do not free this memory
 */
const char *
scopedvar_getname(struct frame *fr, int level, int varnum)
{
    struct scopedvar_t *svinfo = fr ? fr->svars : NULL;

    while (svinfo && level-- > 0)
        svinfo = svinfo->next;

    if (!svinfo) {
        return NULL;
    }

    if (varnum < 0 || varnum >= svinfo->count) {
        return NULL;
    }

    if (!svinfo->varnames) {
        return NULL;
    }

    return svinfo->varnames[varnum];
}

/**
 * This is used to free memory for the given instruction.
 *
 * It underpins the CLEAR macro which injects file and line automatically.
 * The memory freed is the data associated with the instruction rather than
 * the instruction itself, which remains forever as long as the program
 * is in memory.
 *
 * The instruction type is set to PROG_CLEARED after it has been cleared,
 * the 'line' field of the struct set to 'line', and the data set to
 * 'file' so that if we try to clear something that is already cleared,
 * we can produce a nice error message.
 *
 * @param oper the instruction who's data we are clearing.
 * @param file the file from which this function was called.
 * @param line the line in the file from which this function was called.
 */
void
RCLEAR(struct inst *oper, char *file, int line)
{
    int varcnt;

    assert(oper != NULL);
    assert(file != NULL);
    assert(line > 0);

    switch (oper->type) {
        case PROG_CLEARED:
            log_status(
                "WARNING: attempt to re-CLEAR() instruction from %s:%d "
                " previously CLEAR()ed at %s:%d", file, line,
                (char *) oper->data.addr, oper->line);

            /*
             * If debugging, we want to figure out just what
             * is going on, and dump core at this point.  This
             * will at least give us some idea of what went wrong.
             */
            assert(0);
            return;

        case PROG_ADD:
            PROGRAM_DEC_INSTANCES(oper->data.addr->progref);
            oper->data.addr->links--;
            break;

        case PROG_STRING:
            if (oper->data.string && --oper->data.string->links == 0)
                free(oper->data.string);
            break;

        case PROG_FUNCTION:
            if (oper->data.mufproc) {
                free(oper->data.mufproc->procname);
                varcnt = oper->data.mufproc->vars;

                if (oper->data.mufproc->varnames) {
                    for (int j = 0; j < varcnt; j++) {
                        free((void *) oper->data.mufproc->varnames[j]);
                    }

                    free((void *)oper->data.mufproc->varnames);
                }

                free(oper->data.mufproc);
            }

            break;

        case PROG_ARRAY:
            array_free(oper->data.array);
            break;

        case PROG_LOCK:
            if (oper->data.lock != TRUE_BOOLEXP)
                free_boolexp(oper->data.lock);
            break;
    }

    oper->line = line;
    oper->data.addr = (struct prog_addr *)file;
    oper->type = PROG_CLEARED;
}

/**
 * @var the current highest PID.  The next MUF program will get this PID.
 */
int top_pid = 1;

/**
 * @var Number of arguments a primitive uses, for cleanup purposes.
 *      This number is used by abort_interp to clean up if something fails.
 */
int nargs = 0;

/**
 * @var the total number of primitives defined
 */
int prim_count = 0;

/**
 * @private
 * @var Keep track of free frames.  We can re-use frames for performance
 *      reasons.
 */
static struct frame *free_frames_list = NULL;

/**
 * @private
 * @var pool of reusable forvars structures, again for performance reasons.
 */
static struct forvars *for_pool = NULL;

/**
 * @private
 * @var pointer to a pointer which is the last item on the for_pool list.
 */
static struct forvars **last_for = &for_pool;

/**
 * @private
 * @var pool of reusable tryvars structures, again for performance reasons.
 */
static struct tryvars *try_pool = NULL;

/**
 * @private
 * @var pointer to a pointer which is the last item on the try_pool list.
 */
static struct tryvars **last_try = &try_pool;

/**
 * Clean up extra free frames
 *
 * The MUCK keeps a set of allocated frames in memory to re-use for
 * performance reasons.  There's a tune parameter, tp_free_frames_pool,
 * which indicates the maximum size of this pool.  This call will shrink
 * the pool to tp_free_frames_pool in size.
 */
void
purge_free_frames(void)
{
    struct frame *ptr, *ptr2;
    int count = tp_free_frames_pool;

    /*
     * This moves us past the frames we are not going to delete.
     */
    for (ptr = free_frames_list; ptr && --count > 0; ptr = ptr->next) ;

    while (ptr && ptr->next) {
        ptr2 = ptr->next;
        ptr->next = ptr->next->next;
        free(ptr2);
    }
}

#ifdef MEMORY_CLEANUP
/**
 * Clean up the entire free frames pool
 *
 * Like purge_free_frames, except it deletes all of them.  Only defined if
 * MEMORY_CLEANUP is defined.  This is used when shutting down the MUCK.
 */
void
purge_all_free_frames(void)
{
    struct frame *ptr;

    while (free_frames_list) {
        ptr = free_frames_list;
        free_frames_list = ptr->next;
        free(ptr);
    }
}
#endif

/**
 * Clean up the for memory pool
 *
 * This only purges up to the most recently used.  Run a second time to
 * purge everything.
 */
void
purge_for_pool(void)
{
    struct forvars *cur, *next;

    cur = *last_for;
    *last_for = NULL;
    last_for = &for_pool;

    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}

/**
 * Clean up the try memory pool
 *
 * This only purges up to the most recently used.  Run a second time to
 * purge everything.
 */
void
purge_try_pool(void)
{
    /* This only purges up to the most recently used. */
    /* Purge this a second time to purge all. */
    struct tryvars *cur, *next;

    cur = *last_try;
    *last_try = NULL;
    last_try = &try_pool;

    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}

/**
 * Set up a frame for MUF program interpretation
 *
 * The frame structure is the information for a running program.  This
 * function gets everything set up, the program counter in the right place,
 * and makes the program ready to run.
 *
 * Frames are re-used to improve allocation speed.  Up to tp_free_frames_pool
 * may be held in reserve at a time.
 *
 * @param descr the descriptor of the person calling the program
 * @param player the dbref of the person calling the program
 * @param location the dbref of the triggering location
 * @param program the dbref of the program to call
 * @param source the dbref of the triggering object
 * @param nosleeps PREEMPT, FOREGROUND, or BACKGROUND for initial program state
 * @param whichperms STD_REGUID, STD_SETUID, or STD_HARDUID
 * @param forced_pid if 0, make a new pid - otherwise use this number as pid
 *
 * @return constructed frame structure
 */
struct frame *
interp(int descr, dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int forced_pid)
{
    struct frame *fr;

    /* Check basic permissions to make sure we're allowed to do the call. */
    if (!MLevel(program) || !MLevel(OWNER(program)) ||
        ((source != NOTHING) && !TrueWizard(OWNER(source)) &&
         !can_link_to(OWNER(source), TYPE_EXIT, program))) {
        notify_nolisten(player, "Program call: Permission denied.", 1);
        return 0;
    }

    /* Grab a pre-allocated frame if we've got one, or allocate a fresh one */
    if (free_frames_list) {
        fr = free_frames_list;
        free_frames_list = fr->next;
    } else {
        fr = malloc(sizeof(struct frame));
    }

    memset(fr, 0, sizeof(struct frame));
    fr->pid = forced_pid ? forced_pid : top_pid++;
    fr->descr = descr;
    fr->supplicant = NOTHING;
    fr->multitask = nosleeps;
    fr->perms = whichperms;
    fr->been_background = (nosleeps == 2);
    fr->trig = source;
    fr->cmd = (!*match_cmdname) ? 0 : alloc_prog_string(match_cmdname);
    fr->started = time(NULL);
    fr->caller.top = 1;
    fr->caller.st[0] = source;
    fr->caller.st[1] = program;

    fr->system.top = 1;

    fr->errorprog = NOTHING;
    fr->pc = PROGRAM_START(program);
    fr->writeonly = ((source == -1) || (Typeof(source) == TYPE_ROOM) ||
		     ((Typeof(source) == TYPE_PLAYER) && (!PLAYER_DESCRCOUNT(source))) ||
                     (FLAGS(player) & READMODE));

    fr->brkpt.breaknum = -1;
    fr->brkpt.lastproglisted = NOTHING;
    fr->brkpt.count = 1;
    fr->brkpt.temp[0] = 1;
    fr->brkpt.level[0] = -1;
    fr->brkpt.line[0] = -1;
    fr->brkpt.linecount[0] = -2;
    fr->brkpt.pc[0] = NULL;
    fr->brkpt.pccount[0] = -2;
    fr->brkpt.prog[0] = program;

    for (int i = 0; i < MAX_VAR; i++) {
        fr->variables[i].type = PROG_INTEGER;
        fr->variables[i].data.number = 0;
    }

    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = player;
    fr->variables[1].type = PROG_OBJECT;
    fr->variables[1].data.objref = location;
    fr->variables[2].type = PROG_OBJECT;
    fr->variables[2].data.objref = source;
    fr->variables[3].type = PROG_STRING;
    fr->variables[3].data.string = fr->cmd;

    if (fr->cmd)
        fr->cmd->links++;

    array_init_active_list(&fr->array_active_list);
    fr->prev_array_active_list = NULL;

    if (PROGRAM_CODE(program)) {
        PROGRAM_INC_PROF_USES(program);
    }

    PROGRAM_INC_INSTANCES(program);
    push(fr->argument.st, &(fr->argument.top), PROG_STRING, *match_args ?
         alloc_prog_string(match_args) : 0);

    return fr;
}

/**
 * @var this is used to count errors in whatever MUF is currently being
 *      interpreted.  Setting this to non-0 will result in the MUF program
 *      aborting.
 */
static int err;

/**
 * Make a copy of a given stack of forvars structures
 *
 * This is used by the MUF fork primitive and has very little utility
 * otherwise at this time.
 *
 * @param forstack the stack of forvars to copy
 * @return a copy of forstack
 */
struct forvars *
copy_fors(struct forvars *forstack)
{
    struct forvars *out = NULL;
    struct forvars *nu;
    struct forvars *last = NULL;

    for (struct forvars *in = forstack; in; in = in->next) {
        if (!for_pool) {
            nu = malloc(sizeof(struct forvars));
        } else {
            nu = for_pool;

            if (*last_for == for_pool->next) {
                last_for = &for_pool;
            }

            for_pool = nu->next;
        }

        nu->didfirst = in->didfirst;
        deep_copyinst(&in->cur, &nu->cur, -1);
        deep_copyinst(&in->end, &nu->end, -1);
        nu->step = in->step;
        nu->next = NULL;

        if (!out) {
            last = out = nu;
        } else {
            last->next = nu;
            last = nu;
        }
    }

    return out;
}

/**
 * 'Push' an empty forvars struct into forstack and return it.
 *
 * Push is in quotes because it doesn't really push it -- you need to
 * do a call structure like this:
 *
 * stack = push_for(stack)
 *
 * This call may recycle previously allocated forstack structs.
 *
 * @param forstack the current top of the forstack
 * @return the new top of the forstack
 */
struct forvars *
push_for(struct forvars *forstack)
{
    struct forvars *nu;

    if (!for_pool) {
        nu = malloc(sizeof(struct forvars));
    } else {
        nu = for_pool;

        if (*last_for == for_pool->next) {
            last_for = &for_pool;
        }

        for_pool = nu->next;
    }

    memset(nu, 0, sizeof(struct forvars));
    nu->next = forstack;
    return nu;
}

/**
 * Remove a forvars struct from a forstack
 *
 * The removed forvars struct is put at the head of the for_pool list.
 * It should be considered deleted at that point and no longer referenced.
 *
 * The calling pattern for this should be thus:
 *
 * stack = pop_for(stack)
 *
 * as this will return what should be the new top of the forstack.
 * The result may be NULL if all items are popped off.
 *
 * @param forstack the stack to remove from
 * @return the new head of the forstack with the top item removed.
 */
struct forvars *
pop_for(struct forvars *forstack)
{
    struct forvars *newstack;

    if (!forstack) {
        return NULL;
    }

    newstack = forstack->next;
    forstack->next = for_pool;
    for_pool = forstack;

    if (last_for == &for_pool) {
        last_for = &(for_pool->next);
    }

    return newstack;
}

/**
 * Make a copy of a given stack of tryvars structures
 *
 * This is used by the MUF fork primitive and has very little utility
 * otherwise at this time.
 *
 * @param trystack the stack of tryvars to copy
 * @return a copy of trystack
 */
struct tryvars *
copy_trys(struct tryvars *trystack)
{
    struct tryvars *out = NULL;
    struct tryvars *nu;
    struct tryvars *last = NULL;

    for (struct tryvars *in = trystack; in; in = in->next) {
        if (!try_pool) {
            nu = malloc(sizeof(struct tryvars));
        } else {
            nu = try_pool;

            if (*last_try == try_pool->next) {
                last_try = &try_pool;
            }

            try_pool = nu->next;
        }

        nu->depth = in->depth;
        nu->call_level = in->call_level;
        nu->for_count = in->for_count;
        nu->addr = in->addr;
        nu->next = NULL;

        if (!out) {
            last = out = nu;
        } else {
            last->next = nu;
            last = nu;
        }
    }

    return out;
}

/**
 * 'Push' an empty tryvars struct into trystack and return it.
 *
 * Push is in quotes because it doesn't really push it -- you need to
 * do a call structure like this:
 *
 * stack = push_try(stack)
 *
 * This call may recycle previously allocated trystack structs.
 *
 * @param trystack the current top of the trystack
 * @return the new top of the trystack
 */
struct tryvars *
push_try(struct tryvars *trystack)
{
    struct tryvars *nu;

    if (!try_pool) {
        nu = malloc(sizeof(struct tryvars));
    } else {
        nu = try_pool;

        if (*last_try == try_pool->next) {
            last_try = &try_pool;
        }

        try_pool = nu->next;
    }

    nu->next = trystack;
    return nu;
}

/**
 * Remove a tryvars struct from a trystack
 *
 * The removed tryvars struct is put at the head of the try_pool list.
 * It should be considered deleted at that point and no longer referenced.
 *
 * The calling pattern for this should be thus:
 *
 * stack = pop_try(stack)
 *
 * as this will return what should be the new top of the trystack.
 * The result may be NULL if all items are popped off.
 *
 * @param trystack the stack to remove from
 * @return the new head of the trystack with the top item removed.
 */
struct tryvars *
pop_try(struct tryvars *trystack)
{
    struct tryvars *newstack;

    if (!trystack) {
        return NULL;
    }

    newstack = trystack->next;
    trystack->next = try_pool;
    try_pool = trystack;

    if (last_try == &try_pool) {
        last_try = &(try_pool->next);
    }

    return newstack;
}

/**
 * Clean up lists from watchpid and sends event.
 *
 * This is called on program cleanup, during exit, to send events to the
 * waiting processes.  Anyone this process is waiting for will be cleaned
 * up as well.
 *
 * @private
 * @param fr the frame of the program that is exiting
 */
static void
watchpid_process(struct frame *fr)
{
    if (!fr) {
        log_status("WARNING: watchpid_process(): NULL frame passed !  Ignored.");
        return;
    }

    struct frame *frame;
    struct mufwatchpidlist *cur;
    struct inst temp1;
    temp1.type = PROG_INTEGER;
    temp1.data.number = fr->pid;

    /* Clean up any pids we're waiting for */
    while (fr->waitees) {
        cur = fr->waitees;
        fr->waitees = cur->next;

        frame = timequeue_pid_frame(cur->pid);
        free(cur);

        if (frame) {
            for (struct mufwatchpidlist **curptr = &frame->waiters;
                 *curptr; curptr = &(*curptr)->next) {
                if ((*curptr)->pid == fr->pid) {
                    cur = *curptr;
                    *curptr = (*curptr)->next;
                    free(cur);
                    break;
                }
            }
        }
    }

    /* Notify processes that are waiting for us */
    while (fr->waiters) {
        char buf[64];

        snprintf(buf, sizeof(buf), "PROC.EXIT.%d", fr->pid);

        cur = fr->waiters;
        fr->waiters = cur->next;

        frame = timequeue_pid_frame(cur->pid);
        free(cur);

        if (frame) {
            muf_event_add(frame, buf, &temp1, 0);

            /* Remove us from their watch list after delivering the event */
            for (struct mufwatchpidlist **curptr = &frame->waitees;
                 *curptr; curptr = &(*curptr)->next) {
                if ((*curptr)->pid == fr->pid) {
                    cur = *curptr;
                    *curptr = (*curptr)->next;
                    free(cur);
                    break;
                }
            }
        }
    }
}

/**
 * Clean up a given frame, and return it to the free frames list.
 *
 * This does the heavy lifting of cleaning up a program.  This
 * includes stuff like freeing the program text, cleaning up variables,
 * etc. etc.
 *
 * This does NOT dequeue a running process and should NOT be used as
 * a "kill" command.
 *
 * It will handle the watchpid stuff.
 *
 * @param fr the frame to clean up
 */
void
prog_clean(struct frame *fr)
{
    if (!fr) {
        log_status("WARNING: prog_clean(): Tried to free a NULL frame !  "
                   "Ignored.");
        return;
    }

    for (struct frame *ptr = free_frames_list; ptr; ptr = ptr->next) {
        if (ptr == fr) {
            log_status("WARNING: prog_clean(): tried to free an already "
                       "freed program frame !  Ignored.");
            return;
        }
    }

    watchpid_process(fr);

    fr->system.top = 0;

    for (int i = 0; i < fr->argument.top; i++) {
        CLEAR(&fr->argument.st[i]);
    }

    DEBUGPRINT("prog_clean: fr->caller.top=%d\n", fr->caller.top);

    for (int i = 1; i <= fr->caller.top; i++) {
        DEBUGPRINT("Decreasing instances of fr->caller.st[%d](#%d)\n", i, fr->caller.st[i]);
        PROGRAM_DEC_INSTANCES(fr->caller.st[i]);
    }

    for (int i = 0; i < MAX_VAR; i++)
        CLEAR(&fr->variables[i]);

    if (fr->cmd && --fr->cmd->links == 0)
        free(fr->cmd);

    localvar_freeall(fr);
    scopedvar_freeall(fr);

    if (fr->fors.st) {
        struct forvars **loop = &(fr->fors.st);

        while (*loop) {
            CLEAR(&((*loop)->cur));
            CLEAR(&((*loop)->end));
            loop = &((*loop)->next);
        }

        *loop = for_pool;

        if (last_for == &for_pool) {
            last_for = loop;
        }

        for_pool = fr->fors.st;
        fr->fors.st = NULL;
        fr->fors.top = 0;
    }

    if (fr->trys.st) {
        struct tryvars **loop = &(fr->trys.st);

        while (*loop) {
            loop = &((*loop)->next);
        }

        *loop = try_pool;

        if (last_try == &try_pool) {
            last_try = loop;
        }

        try_pool = fr->trys.st;
        fr->trys.st = NULL;
        fr->trys.top = 0;
    }

    fr->argument.top = 0;
    fr->pc = 0;

    free(fr->brkpt.lastcmd);
    free_prog_text(fr->brkpt.proglines);

    fr->brkpt.proglines = NULL;

    if (fr->rndbuf)
        free(fr->rndbuf);

#ifdef MCPGUI_SUPPORT
    muf_dlog_purge(fr);
#endif

    dequeue_timers(fr->pid, NULL);

    muf_event_purge(fr);
    array_free_all_on_list(&fr->array_active_list);
    fr->next = free_frames_list;
    free_frames_list = fr;
    err = 0;
}

/**
 * This resets a program's argument and system stacks to the given locations.
 *
 * At the time of this writing, I'm not honestly sure what the purpose of
 * this is.  But it is used all over the place and in many circumstances,
 * so it is probably important.
 *
 * If someone else has a better idea of why this is done, please update
 * this documentation.
 *
 * @private
 * @param fr the frame to reset
 * @param atop the argument stack reload point
 * @param stop the system stack reload point
 */
static void
reload(struct frame *fr, int atop, int stop)
{
    assert(fr);
    fr->argument.top = atop;
    fr->system.top = stop;
}

/**
 * Does the given instruction evaluate to false?
 *
 * This returns true, oddly enough, if 'p' evaluates to false.
 *
 * Empty strings, markers, empty arrays, locks that equal TRUE_BOOLEXP,
 * integers equal to 0, floats equal to 0.0, and dbrefs equal to NOTHING
 * all evaluate to false.
 *
 * @param p the instruction to evaluate
 * @return boolean true if 'p' is considered a false value, false otherwise.
 */
int
false_inst(struct inst *p)
{
    return (
        (p->type == PROG_STRING && (!p->data.string || !(*p->data.string->data)))
        || (p->type == PROG_MARK)
        || (p->type == PROG_ARRAY && (!p->data.array || !p->data.array->items))
        || (p->type == PROG_LOCK && p->data.lock == TRUE_BOOLEXP)
        || (p->type == PROG_INTEGER && p->data.number == 0)
        || (p->type == PROG_FLOAT && p->data.fnumber == 0.0)
        || (p->type == PROG_OBJECT && p->data.objref == NOTHING)
    );
}

/**
 * Copy an instruction from instruction 'from' to instruction 'to'
 *
 * This does a 'shallow copy', meaning, in most cases it just increases
 * the reference counter for stuff like arrays and strings.
 *
 * This is used for stuff like 'dup' to copy an instruction on the stack
 * to another entry in the stack (because the stack is fully pre-allocated)
 * It is truly used all over the place, though.
 *
 * Locks and functions get a deeper copy due to the nature of them.
 *
 * @see deep_copyinst for a deep copy.
 *
 * @param from the source instruction
 * @param to the destination instruction
 */
void
copyinst(struct inst *from, struct inst *to)
{
    assert(from && to);
    int varcnt;
    *to = *from;

    switch (from->type) {
        case PROG_FUNCTION:
            if (from->data.mufproc) {
                to->data.mufproc = malloc(sizeof(struct muf_proc_data));
                to->data.mufproc->procname = strdup(from->data.mufproc->procname);
                to->data.mufproc->vars = varcnt = from->data.mufproc->vars;
                to->data.mufproc->args = from->data.mufproc->args;
                to->data.mufproc->varnames = calloc((size_t)varcnt, sizeof(const char *));

                for (int j = 0; j < varcnt; j++) {
                    to->data.mufproc->varnames[j] = strdup(from->data.mufproc->varnames[j]);
                }
            }

            break;

        case PROG_STRING:
            if (from->data.string) {
                from->data.string->links++;
            }

            break;

        case PROG_ARRAY:
            if (from->data.array) {
                from->data.array->links++;
            }

            break;

        case PROG_ADD:
            from->data.addr->links++;
            PROGRAM_INC_INSTANCES(from->data.addr->progref);
            break;

        case PROG_LOCK:
            if (from->data.lock != TRUE_BOOLEXP) {
                to->data.lock = copy_bool(from->data.lock);
            }

            break;
    }
}

/**
 * Deep copy an instruction from instruction 'from' to instruction 'to'
 *
 * Does a deep copy of 'in' to 'out'.  This iterates into arrays and
 * dictionaries to copy all the instructions within them in a recursive
 * fashion.  Otherwise it operates similar to copyinst, this really only
 * impacts arrays/dictionaries.
 *
 * @see copyinst
 *
 * @param from the source instruction
 * @param to the destination instruction
 * @param pinned boolean passed to any new arrays made.  -1 will use default
 */
void
deep_copyinst(struct inst *in, struct inst *out, int pinned)
{
    stk_array *nu = NULL, *arr;
    struct inst temp1;

    if (in->type != PROG_ARRAY) {
        copyinst(in, out);
        return;
    }

    arr = in->data.array;

    if (arr == NULL) {
        copyinst(in, out);
        return;
    }

    *out = *in;

    switch (arr->type) {
        case ARRAY_PACKED:{
            nu = new_array_packed(arr->items, pinned == -1 ?
                                  arr->pinned : pinned);

            for (int i = arr->items; i-- > 0;) {
                deep_copyinst(&arr->data.packed[i], &nu->data.packed[i],
                              pinned);
            }
            break;
        }

        case ARRAY_DICTIONARY:{
            array_iter idx;
            array_data *val;

            nu = new_array_dictionary(pinned == -1 ? arr->pinned : pinned);

            if (array_first(arr, &idx)) {
                do {
                    val = array_getitem(arr, &idx);
                    deep_copyinst(val, &temp1, pinned);
                    array_setitem(&nu, &idx, &temp1);
                } while (array_next(arr, &idx));
            }
            break;
        }
    }

    out->data.array = nu;
}

/**
 * Calculate profile timing for a given program ref and frame
 *
 * Updates the execution time and total execution time for the given program.
 *
 * @private
 * @param prog the program to update timings for
 * @param fr the frame associated with the program
 */
static void
calc_profile_timing(dbref prog, struct frame *fr)
{
    assert(fr);

    struct timeval tv;
    struct timeval tv2;

    gettimeofday(&tv, NULL);

    if (tv.tv_usec < fr->proftime.tv_usec) {
        tv.tv_usec += 1000000;
        tv.tv_sec -= 1;
    }

    tv.tv_usec -= fr->proftime.tv_usec;
    tv.tv_sec -= fr->proftime.tv_sec;
    tv2 = PROGRAM_PROFTIME(prog);
    tv2.tv_sec += tv.tv_sec;
    tv2.tv_usec += tv.tv_usec;

    if (tv2.tv_usec >= 1000000) {
        tv2.tv_usec -= 1000000;
        tv2.tv_sec += 1;
    }

    PROGRAM_SET_PROFTIME(prog, tv2.tv_sec, tv2.tv_usec);
    fr->totaltime.tv_sec += tv.tv_sec;
    fr->totaltime.tv_usec += tv.tv_usec;

    if (fr->totaltime.tv_usec > 1000000) {
        fr->totaltime.tv_usec -= 1000000;
        fr->totaltime.tv_sec += 1;
    }
}

/**
 * @private
 * @var this is incremented every time we enter the interpreter loop, and
 *      decremented when we leave it.  It is used to detect interpreter
 *      nesting.
 */
static int interp_depth = 0;

/**
 * @private
 * @var this is incremented every time we enter the interpreter loop, but
 *      is never decremented.  It resets to 0 when interp_depth is 0.
 *      This is used for the interpreter loop count tune variables to
 *      kill long running programs.
 */
static int nested_interp_loop_count = 0;

/**
 * Display an interpreter error
 *
 * This also sets the MUF_ERRCOUNT_PROP, MUF_LASTERR_PROP, MUF_LASTCRASH_PROP,
 * and MUF_LASTCRASHTIME_PROP properties on origprog and program (if they
 * are different of course).
 *
 * Origprog would be the program the player ran, and program may be different
 * if it is a CALL'd library for instance.  
 *
 * @private
 * @param player the player that is running the program
 * @param program the program currently running
 * @param pc the program counter for the program (location in instructions)
 * @param arg not used
 * @param atop not used
 * @param origprog the original program that he player called.  May be the
 *                 same as program
 * @param msg1 the error message prefix
 * @param msg2 the bulk of the error message
 */
static void
interp_err(dbref player, dbref program, struct inst *pc,
           struct inst *arg, int atop, dbref origprog, const char *msg1,
           const char *msg2)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char tbuf[40];
    int errcount;
    time_t lt;

    err++;

    if (OWNER(origprog) == OWNER(player)) {
        notify_nolisten(player, 
                        "\033[1;31;40mProgram Error.  Your program just got "
                        "the following error.\033[0m", 1);
    } else {
        notifyf_nolisten(player,
                         "\033[1;31;40mProgrammer Error.  Please tell %s what "
                         "you typed, and the following message.\033[0m",
                         NAME(OWNER(origprog)));
    }

    notifyf_nolisten(player, "\033[1m%s(#%d), line %d; %s: %s\033[0m",
                     NAME(program), program, pc ? pc->line : -1, msg1, msg2);

    lt = time(NULL);
    strftime(tbuf, 32, "%c", localtime(&lt));

    strip_ansi(buf2, buf);
    errcount = get_property_value(origprog, MUF_ERRCOUNT_PROP);
    errcount++;
    add_property(origprog, MUF_ERRCOUNT_PROP, NULL, errcount);
    add_property(origprog, MUF_LASTERR_PROP, buf2, 0);
    add_property(origprog, MUF_LASTCRASH_PROP, NULL, (int) lt);
    add_property(origprog, MUF_LASTCRASHTIME_PROP, tbuf, 0);

    if (origprog != program) {
        errcount = get_property_value(program, MUF_ERRCOUNT_PROP);
        errcount++;
        add_property(program, MUF_ERRCOUNT_PROP, NULL, errcount);
        add_property(program, MUF_LASTERR_PROP, buf2, 0);
        add_property(program, MUF_LASTCRASH_PROP, NULL, (int) lt);
        add_property(program, MUF_LASTCRASHTIME_PROP, tbuf, 0);
    }
}

/**
 * Do book-keeping associated with leaving the interpreter loop
 *
 * This is called by several different points in the interp_oop
 *
 * Decrements the interpreter depth, does active list shuffling, and
 * updates the program timing.  Note that frame level is not decremented
 * by this, to prevent infinite or troublesome recursive loops.
 *
 * @private
 * @param program the program running
 * @param fr the frame of the program running
 */
static void
record_exit_interp(dbref program, struct frame *fr) {
    --interp_depth;

    if (stk_array_active_list == &fr->array_active_list) {
        stk_array_active_list = fr->prev_array_active_list;
        fr->prev_array_active_list = NULL;
    }

    calc_profile_timing(program, fr);
}

/**
 * The 'guts' behind aborting the interpreter loop, with proper book-keeping
 *
 * The 'right' way to call this is through defines 'abort_loop' and
 * 'abort_loop_hard'.  This function isn't called directly.
 *
 * @see abort_loop
 * @see abort_loop_hard
 *
 * This handles error notification, program cleanup, and clearing the
 * player-blocked state if applicable.
 *
 * @private
 * @param player the player running the program
 * @param program the program being run
 * @param msg the error message to show
 * @param fr the current program frame
 * @param pc the current program counter (instruction being run)
 * @param atop agrument top index
 * @param stop system top index
 * @param clinst1 if not null, this will be CLEAR'd
 * @param clinst2 if not null, this will be CLEAR'd
 *
 * @see CLEAR
 */
static void
do_abort_loop(dbref player, dbref program, const char *msg,
              struct frame *fr, struct inst *pc, int atop, int stop,
              struct inst *clinst1, struct inst *clinst2)
{
    if (!fr) {
        panic("localvars_get(): NULL frame passed !");
    }

    char buffer[128];

    /* If we are in a try/catch, let's copy over some error info */
    if (fr->trys.top) {
        fr->errorstr = strdup(msg);

        if (pc) {
            fr->errorinst =
                strdup(
                    insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program,
                               1)
                );
            fr->errorline = pc->line;
        } else {
            fr->errorinst = NULL;
            fr->errorline = -1;
        }

        fr->errorprog = program;
        err++;
    }

    if (clinst1)
        CLEAR(clinst1);

    if (clinst2)
        CLEAR(clinst2);

    reload(fr, atop, stop);
    fr->pc = pc;

    /* If we aren't in a try/catch, then display error and clean up. */
    if (!fr->trys.top) {
        if (pc) {
            interp_err(player, program, pc, fr->argument.st, fr->argument.top,
                       fr->caller.st[1], insttotext(fr, 0, pc, buffer,
                       sizeof(buffer), 30, program, 1), msg);

            if (controls(player, program))
                muf_backtrace(player, program, STACK_SIZE, fr);
        } else {
            notify_nolisten(player, msg, 1);
        }

        record_exit_interp(program, fr);
        prog_clean(fr);
        PLAYER_SET_BLOCK(player, 0);
    }
}

/**
 * Handles an abort / exception in the interpreter loop, supporting try/catch
 *
 * This will either issue a 'break' if we are in a try/catch, or it will
 * return 0
 *
 * @private
 * @param S the message for aborting
 * @param C1 instruction to clear or NULL
 * @param C2 instruction to clear or NULL
 */
#define abort_loop(S, C1, C2) \
{ \
    do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
    if (fr && fr->trys.top) \
        break; \
    else \
        return 0; \
}

/**
 * Handles an abort / exception in the interpreter loop, ignoring try/catch
 *
 * This will cause the function to return 0 regardless of try/catch.
 *
 * @private
 * @param S the message for aborting
 * @param C1 instruction to clear or NULL
 * @param C2 instruction to clear or NULL
 */
#define abort_loop_hard(S, C1, C2) \
{ \
    int tmptop = 0; \
    if (fr) { tmptop = fr->trys.top; fr->trys.top = 0; } \
    do_abort_loop(player, program, (S), fr, pc, atop, stop, (C1), (C2)); \
    if (fr) fr->trys.top = tmptop; \
    return 0; \
}

/**
 * The MUF interpreter loop - run a program until it completes or yields
 *
 * This is where the magic happens.  There's a lot of nuances here that are
 * difficult to sum up into a little sound bite.  Here's the highlights:
 *
 * A PREEMPT or BOUND program runs until a certain number of instructions
 * (at which point it hard stops), until it has gotten too many nested
 * interpreter calls, or until it finishes completely.
 *
 * Otherwise, a program runs for awhile until it either blocks for input
 * or sleep, or it gets forcibly "0 sleep" injected to make it yield.  This
 * is called a slice.  All these numbers are tunable with @tune but the
 * defaults are pretty much always used.
 *
 * This will parse the instructions using a godawful switch statement.
 *
 * @param player the player running the program
 * @param program the program being run
 * @param fr the frame for the current running program
 * @param rettyp boolean if true we will return an instruction rather than
 *               a simple true or NULL
 * @return an instruction return value or NULL
 */
struct inst *
interp_loop(dbref player, dbref program, struct frame *fr, int rettyp)
{
    register struct inst *pc;
    register int atop;
    register struct inst *arg;
    register struct inst *temp1;
    register struct inst *temp2;
    register struct stack_addr *sys;
    register int instr_count;
    register int stop;
    int i = 0, tmp, writeonly, mlev;
    static struct inst retval;
    char dbuf[BUFFER_LEN];
    int instno_debug_line = get_primitive("debug_line");

    /* Keep track of the depth */
    if (interp_depth == 0) {
        nested_interp_loop_count = 0;
    } else {
        ++nested_interp_loop_count;
    }

    /*
     * Increase frame level, while also incrementing the interpreter depth
     * count.
     *
     * Frame level is quite similar to nested_interp_loop_count and is used
     * to prevent runaway situations.
     */
    fr->level = ++interp_depth;

    /* Update active lists */
    fr->prev_array_active_list = stk_array_active_list;
    stk_array_active_list = &fr->array_active_list;

    /* load everything into local stuff */
    pc = fr->pc;
    atop = fr->argument.top;
    stop = fr->system.top;
    arg = fr->argument.st;
    sys = fr->system.st;
    writeonly = fr->writeonly;
    fr->brkpt.isread = 0;

    /* Try to compile it if we need to */
    if (!pc) {
        struct line *tmpline;

        tmpline = PROGRAM_FIRST(program);
        PROGRAM_SET_FIRST(program, (struct line *) read_program(program));
        do_compile(-1, OWNER(program), program, 0);
        free_prog_text(PROGRAM_FIRST(program));
        PROGRAM_SET_FIRST(program, tmpline);
        pc = fr->pc = PROGRAM_START(program);

        if (!pc) {
            abort_loop_hard("Program not compilable. Cannot run.", NULL, NULL);
        }

        PROGRAM_INC_PROF_USES(program);
        PROGRAM_INC_INSTANCES(program);
    }

    /* Mark the program as used */
    ts_useobject(program);
    err = 0;

    instr_count = 0;
    mlev = ProgMLevel(program);
    gettimeofday(&fr->proftime, NULL);

    /* This is the 'natural' way to exit a function */
    while (stop) {
        /* Abort program if player/thing running it is recycled */
        if (!OkObj(player)) {
            reload(fr, atop, stop);
            prog_clean(fr);
            interp_depth--;
            calc_profile_timing(program, fr);

            return NULL;
        }

        fr->instcnt++;
        instr_count++;

        /*
         * If it is pre-empt, check instruction count, nested loop count,
         * and all.
         */
        if ((fr->multitask == PREEMPT) || (FLAGS(program) & BUILDER)) {
            if (mlev == 4) {
                if (tp_max_ml4_preempt_count) {
                    if (instr_count >= tp_max_ml4_preempt_count)
                        abort_loop_hard("Maximum preempt instruction count "
                                        "exceeded", NULL, NULL);
                } else
                    instr_count = 0;

                if (tp_max_ml4_nested_interp_loop_count)
                    if (nested_interp_loop_count >= tp_max_ml4_nested_interp_loop_count)
                        abort_loop_hard("Maximum interp loop nested call count exceeded in preempt mode",
                                        NULL, NULL);
            } else {
                /* else make sure that the program doesn't run too long */
                if (instr_count >= tp_max_instr_count)
                    abort_loop_hard("Maximum preempt instruction count "
                                    "exceeded", NULL, NULL);

                if (nested_interp_loop_count >= tp_max_nested_interp_loop_count)
                    abort_loop_hard("Maximum interp loop nested call count exceeded in preempt mode",
                                    NULL, NULL);
            }
        } else {
            /* if in FOREGROUND or BACKGROUND mode, '0 sleep' every so often. */
            if (((fr->instcnt > tp_instr_slice * 4) && 
                 (instr_count >= tp_instr_slice)) ||
                 (nested_interp_loop_count > tp_max_nested_interp_loop_count)) {
                fr->pc = pc;
                reload(fr, atop, stop);
                PLAYER_SET_BLOCK(player, (!fr->been_background));
                add_muf_delay_event(0, fr->descr, player, NOTHING, NOTHING,
                                    program, fr,
                                    (fr->multitask ==
                                     FOREGROUND) ? "FOREGROUND" : "BACKGROUND");
                record_exit_interp(program, fr);
                return NULL;
            }
        }

        /* Handle enter debug mode or not */
        if (((FLAGS(program) & ZOMBIE) || fr->brkpt.force_debugging) &&
            !fr->been_background && controls(player, program)) {
            fr->brkpt.debugging = 1;
        } else {
            fr->brkpt.debugging = 0;
        }

        /* Handle debug (dump) mode */
        if (FLAGS(program) & DARK ||
            (fr->brkpt.debugging && fr->brkpt.showstack && !fr->brkpt.bypass)) {
            if ((pc->type != PROG_PRIMITIVE) || 
                (pc->data.number != instno_debug_line)) {
                char *m =
                    debug_inst(fr, 0, pc, fr->pid, arg, dbuf, sizeof(dbuf),
                               atop, program);
                notify_nolisten(player, m, 1);
            }
        }

        /* Breakpoint ? */
        if (fr->brkpt.debugging) {
            short breakflag = 0;

            if (stop == 1 &&
                !fr->brkpt.bypass && pc->type == PROG_PRIMITIVE &&
                pc->data.number == IN_RET) {
                /* Program is about to EXIT */
                notify_nolisten(player, "Program is about to EXIT.", 1);
                breakflag = 1;
            } else if (fr->brkpt.count) {
                for (i = 0; i < fr->brkpt.count; i++) {
                    if ((!fr->brkpt.pc[i] || pc == fr->brkpt.pc[i]) &&
                        /* pc matches */
                        (fr->brkpt.line[i] == -1 ||
                         (fr->brkpt.lastline != pc->line &&
                          fr->brkpt.line[i] == pc->line)) &&
                        /* line matches */
                        (fr->brkpt.level[i] == -1 || stop <= fr->brkpt.level[i])
                        &&
                        /* level matches */
                        (fr->brkpt.prog[i] == NOTHING ||
                         fr->brkpt.prog[i] == program) &&
                        /* program matches */
                        (fr->brkpt.linecount[i] == -2 ||
                         (fr->brkpt.lastline != pc->line &&
                          fr->brkpt.linecount[i]-- <= 0)) &&
                        /* line count matches */
                        (fr->brkpt.pccount[i] == -2 ||
                         (fr->brkpt.lastpc != pc && fr->brkpt.pccount[i]-- <= 0))
                        /* pc count matches */
                    ) {
                        if (fr->brkpt.bypass) {
                            if (fr->brkpt.pccount[i] == -1)
                                fr->brkpt.pccount[i] = 0;

                            if (fr->brkpt.linecount[i] == -1)
                                fr->brkpt.linecount[i] = 0;
                        } else {
                            breakflag = 1;
                            break;
                        }
                    }
                }
            }

            if (breakflag) {
                char *m;

                if (fr->brkpt.dosyspop) {
                    program = sys[--stop].progref;
                    pc = sys[stop].offset;
                }

                add_muf_read_event(fr->descr, player, program, fr);
                reload(fr, atop, stop);
                fr->pc = pc;
                fr->brkpt.isread = 0;
                fr->brkpt.breaknum = i;
                fr->brkpt.lastlisted = 0;
                fr->brkpt.bypass = 0;
                fr->brkpt.dosyspop = 0;
                PLAYER_SET_CURR_PROG(player, program);
                PLAYER_SET_BLOCK(player, 0);
                record_exit_interp(program, fr);

                if (!fr->brkpt.showstack) {
                    m = debug_inst(fr, 0, pc, fr->pid, arg, dbuf, sizeof(dbuf),
                                   atop, program);
                    notify_nolisten(player, m, 1);
                }

                if (pc <= PROGRAM_CODE(program) || (pc - 1)->line != pc->line) {
                    list_proglines(player, program, fr, pc->line, 0);
                } else {
                    m = show_line_prims(program, pc, 15, 1);
                    notifyf_nolisten(player, "     %s", m);
                }

                return NULL;
            }

            fr->brkpt.lastline = pc->line;
            fr->brkpt.lastpc = pc;
            fr->brkpt.bypass = 0;
        }

        /* Instruction count handling for MUCKER1 and MUCKER2 */
        if (mlev < 3) {
            if (fr->instcnt > (tp_max_instr_count * ((mlev == 2) ? 4 : 1)))
                abort_loop_hard("Maximum total instruction count exceeded.",
                                NULL, NULL);
        }

        /* The giant switch to handle instruction types */
        switch (pc->type) {
            case PROG_INTEGER: /* These all push something onto the stack */
            case PROG_FLOAT:
            case PROG_ADD:
            case PROG_OBJECT:
            case PROG_VAR:
            case PROG_LVAR:
            case PROG_SVAR:
            case PROG_STRING:
            case PROG_LOCK:
            case PROG_MARK:
            case PROG_ARRAY:
                if (atop >= STACK_SIZE)
                    abort_loop("Stack overflow.", NULL, NULL);

                copyinst(pc, arg + atop);
                pc++;
                atop++;
                break;

            case PROG_LVAR_AT: /* Push local variable content onto stack */
            case PROG_LVAR_AT_CLEAR:
                {
                    struct inst *tmpvar;
                    struct localvars *lv;

                    if (atop >= STACK_SIZE)
                        abort_loop("Stack overflow.", NULL, NULL);

                    if (pc->data.number >= MAX_VAR || pc->data.number < 0)
                        abort_loop("Scoped variable number out of range.", NULL,
                                   NULL);

                    lv = localvars_get(fr, program);
                    tmpvar = &(lv->lvars[pc->data.number]);

                    copyinst(tmpvar, arg + atop);

                    if (pc->type == PROG_LVAR_AT_CLEAR) {
                        CLEAR(tmpvar);
                        tmpvar->type = PROG_INTEGER;
                        tmpvar->data.number = 0;
                    }

                    pc++;
                    atop++;
                }

                break;

            case PROG_LVAR_BANG: /* Implementation ! for local variables */
                {
                    struct inst *the_var;
                    struct localvars *lv;

                    if (atop < 1)
                        abort_loop("Stack Underflow.", NULL, NULL);

                    if (fr->trys.top && atop - fr->trys.st->depth < 1)
                        abort_loop("Stack protection fault.", NULL, NULL);

                    if (pc->data.number >= MAX_VAR || pc->data.number < 0)
                        abort_loop("Scoped variable number out of range.", NULL,
                                   NULL);

                    lv = localvars_get(fr, program);
                    the_var = &(lv->lvars[pc->data.number]);

                    CLEAR(the_var);
                    temp1 = arg + --atop;
                    *the_var = *temp1;
                    pc++;
                }

                break;

            case PROG_SVAR_AT: /* Push scoped var onto the stack */
            case PROG_SVAR_AT_CLEAR:
                {
                    struct inst *tmpvar;

                    if (atop >= STACK_SIZE)
                        abort_loop("Stack overflow.", NULL, NULL);

                    tmpvar = scopedvar_get(fr, 0, pc->data.number);

                    if (!tmpvar)
                        abort_loop("Scoped variable number out of range.", NULL,
                                   NULL);

                    copyinst(tmpvar, arg + atop);

                    if (pc->type == PROG_SVAR_AT_CLEAR) {
                        CLEAR(tmpvar);

                        tmpvar->type = PROG_INTEGER;
                        tmpvar->data.number = 0;
                    }

                    pc++;
                    atop++;
                }

                break;

            case PROG_SVAR_BANG: /* ! for scoped variables */
                {
                    struct inst *the_var;

                    if (atop < 1)
                        abort_loop("Stack Underflow.", NULL, NULL);

                    if (fr->trys.top && atop - fr->trys.st->depth < 1)
                        abort_loop("Stack protection fault.", NULL, NULL);

                    the_var = scopedvar_get(fr, 0, pc->data.number);

                    if (!the_var)
                        abort_loop("Scoped variable number out of range.", NULL,
                                   NULL);

                    CLEAR(the_var);
                    temp1 = arg + --atop;
                    *the_var = *temp1;
                    pc++;
                }

                break;

            case PROG_FUNCTION: /* Call a function */
                {
                    int mufargs = pc->data.mufproc->args;

                    if (atop < mufargs)
                        abort_loop("Stack Underflow.", NULL, NULL);

                    if (fr->trys.top && atop - fr->trys.st->depth < mufargs)
                        abort_loop("Stack protection fault.", NULL, NULL);

                    if (fr->skip_declare)
                        fr->skip_declare = 0;
                    else
                        scopedvar_addlevel(fr, pc, pc->data.mufproc->vars);

                    while (mufargs-- > 0) {
                        struct inst *tmpvar;

                        temp1 = arg + --atop;
                        tmpvar = scopedvar_get(fr, 0, mufargs);

                        if (!tmpvar)
                            abort_loop_hard("Internal error: Scoped variable "
                                            "number out of range in FUNCTION "
                                            "init.", temp1, NULL);
                        CLEAR(tmpvar);
                        copyinst(temp1, tmpvar);
                        CLEAR(temp1);
                    }

                    pc++;
                }

                break;

            case PROG_IF: /* Handle if */
                if (atop < 1)
                    abort_loop("Stack Underflow.", NULL, NULL);

                if (fr->trys.top && atop - fr->trys.st->depth < 1)
                    abort_loop("Stack protection fault.", NULL, NULL);

                temp1 = arg + --atop;

                if (false_inst(temp1))
                    pc = pc->data.call;
                else
                    pc++;

                CLEAR(temp1);
                break;

            case PROG_EXEC: /* Call another program */
                if (stop >= STACK_SIZE)
                    abort_loop("System Stack Overflow", NULL, NULL);

                sys[stop].progref = program;
                sys[stop++].offset = pc + 1;
                pc = pc->data.call;
                fr->skip_declare = 0; /* Make sure we DON'T skip var decls */
                break;

            case PROG_JMP: /* JMP implementation */
                /* Don't need to worry about skipping scoped var decls here. */
                /* JMP to a function header can only happen in IN_JMP */
                pc = pc->data.call;
                break;

            case PROG_TRY: /* Start of a try block */
                if (atop < 1)
                    abort_loop("Stack Underflow.", NULL, NULL);

                if (fr->trys.top && atop - fr->trys.st->depth < 1)
                    abort_loop("Stack protection fault.", NULL, NULL);

                temp1 = arg + --atop;

                if (temp1->type != PROG_INTEGER || temp1->data.number < 0)
                    abort_loop("Argument is not a positive integer.", temp1,
                               NULL);

                if (fr->trys.top && atop - fr->trys.st->depth
                    < temp1->data.number)
                    abort_loop("Stack protection fault.", NULL, NULL);

                if (temp1->data.number > atop)
                    abort_loop("Stack Underflow.", temp1, NULL);

                fr->trys.top++;
                fr->trys.st = push_try(fr->trys.st);
                fr->trys.st->depth = atop - temp1->data.number;
                fr->trys.st->call_level = stop;
                fr->trys.st->for_count = 0;
                fr->trys.st->addr = pc->data.call;

                pc++;
                CLEAR(temp1);
                break;

            case PROG_PRIMITIVE: /*
                                  * It's a primitive -- call the associated
                                  * primitive function.
                                  */
                /*
                 * All pc modifiers and stuff like that should stay here,
                 * everything else call with an independent dispatcher.
                 */
                switch (pc->data.number) {
                    case IN_JMP: /* Doing a JMP */
                        if (atop < 1)
                            abort_loop("Stack underflow.  Missing address.",
                                       NULL, NULL);

                        if (fr->trys.top && atop - fr->trys.st->depth < 1)
                            abort_loop("Stack protection fault.", NULL, NULL);

                        temp1 = arg + --atop;

                        if (temp1->type != PROG_ADD)
                            abort_loop("Argument is not an address.", temp1,
                                       NULL);

                        if (!ObjExists(temp1->data.addr->progref) ||
                            Typeof(temp1->data.addr->progref) != TYPE_PROGRAM)
                            abort_loop_hard("Internal error.  Invalid address.",
                                            temp1, NULL);

                        if (program != temp1->data.addr->progref) {
                            abort_loop("Destination outside current program.",
                                       temp1, NULL);
                        }

                        if (temp1->data.addr->data->type == PROG_FUNCTION) {
                            fr->skip_declare = 1;
                        }

                        pc = temp1->data.addr->data;
                        CLEAR(temp1);
                        break;

                    case IN_EXECUTE: /* Doing a program execution */
                        if (atop < 1)
                            abort_loop("Stack Underflow. Missing address.",
                                       NULL, NULL);

                        if (fr->trys.top && atop - fr->trys.st->depth < 1)
                            abort_loop("Stack protection fault.", NULL, NULL);

                        temp1 = arg + --atop;

                        if (temp1->type != PROG_ADD)
                            abort_loop("Argument is not an address.", temp1,
                                       NULL);

                        if (!ObjExists(temp1->data.addr->progref) ||
                            Typeof(temp1->data.addr->progref) != TYPE_PROGRAM)
                            abort_loop_hard("Internal error.  Invalid address.",
                                            temp1, NULL);

                        if (stop >= STACK_SIZE)
                            abort_loop("System Stack Overflow", temp1, NULL);

                        sys[stop].progref = program;
                        sys[stop++].offset = pc + 1;

                        if (program != temp1->data.addr->progref) {
                            program = temp1->data.addr->progref;
                            fr->caller.st[++fr->caller.top] = program;
                            mlev = ProgMLevel(program);
                            PROGRAM_INC_INSTANCES(program);
                        }

                        pc = temp1->data.addr->data;
                        CLEAR(temp1);
                        break;

                    case IN_CALL: /* Doing a call */
                        if (atop < 1)
                            abort_loop("Stack Underflow. Missing dbref "
                                       "argument.", NULL, NULL);

                        if (fr->trys.top && atop - fr->trys.st->depth < 1)
                            abort_loop("Stack protection fault.", NULL, NULL);

                        temp1 = arg + --atop;
                        temp2 = 0;

                        if (temp1->type != PROG_OBJECT) {
                            temp2 = temp1;

                            if (atop < 1)
                                abort_loop("Stack Underflow. Missing dbref of "
                                           "func.", temp1, NULL);

                            if (fr->trys.top && atop - fr->trys.st->depth < 1)
                                abort_loop("Stack protection fault.", NULL,
                                           NULL);

                            temp1 = arg + --atop;

                            if (temp2->type != PROG_STRING)
                                abort_loop("Public Func. name string required. "
                                           "(2)", temp1, temp2);

                            if (!temp2->data.string)
                                abort_loop("Null string not allowed. (2)",
                                           temp1, temp2);
                        }

                        if (temp1->type != PROG_OBJECT)
                            abort_loop("Dbref required. (1)", temp1, temp2);

                        if (!valid_object(temp1)
                            || Typeof(temp1->data.objref) != TYPE_PROGRAM)
                            abort_loop("Invalid object.", temp1, temp2);

                        if (!(PROGRAM_CODE(temp1->data.objref))) {
                            struct line *tmpline;

                            tmpline = PROGRAM_FIRST(temp1->data.objref);
                            PROGRAM_SET_FIRST(temp1->data.objref,
                              (struct line *) read_program(temp1->data.objref));
                            do_compile(-1, OWNER(temp1->data.objref),
                                       temp1->data.objref, 0);
                            free_prog_text(PROGRAM_FIRST(temp1->data.objref));
                            PROGRAM_SET_FIRST(temp1->data.objref, tmpline);

                            if (!(PROGRAM_CODE(temp1->data.objref)))
                                abort_loop("Program not compilable.", temp1,
                                           temp2);
                        }

                        if (ProgMLevel(temp1->data.objref) == 0)
                            abort_loop("Permission denied", temp1, temp2);

                        if (mlev < 4 && OWNER(temp1->data.objref) != ProgUID
                            && !Linkable(temp1->data.objref))
                            abort_loop("Permission denied", temp1, temp2);

                        if (stop >= STACK_SIZE)
                            abort_loop("System Stack Overflow", temp1, temp2);

                        sys[stop].progref = program;
                        sys[stop].offset = pc + 1;

                        if (!temp2) {
                            pc = PROGRAM_START(temp1->data.objref);
                        } else {
                            struct publics *pbs;
                            int tmpint;

                            pbs = PROGRAM_PUBS(temp1->data.objref);

                            while (pbs) {
                                tmpint = 
                                        strcasecmp(temp2->data.string->data,
                                                   pbs->subname);

                                if (!tmpint)
                                    break;

                                pbs = pbs->next;
                            }

                            if (!pbs)
                                abort_loop("PUBLIC or WIZCALL function not "
                                           "found. (2)", temp1, temp2);

                            if (mlev < pbs->mlev)
                                abort_loop("Insufficient permissions to "
                                           "call WIZCALL function. (2)",
                                           temp1, temp2);

                            pc = pbs->addr.ptr;
                        }

                        stop++;

                        if (temp1->data.objref != program) {
                            calc_profile_timing(program, fr);
                            gettimeofday(&fr->proftime, NULL);
                            program = temp1->data.objref;
                            fr->caller.st[++fr->caller.top] = program;
                            PROGRAM_INC_INSTANCES(program);
                            mlev = ProgMLevel(program);
                        }

                        PROGRAM_INC_PROF_USES(program);
                        ts_useobject(program);
                        CLEAR(temp1);

                        if (temp2)
                            CLEAR(temp2);

                        break;

                    case IN_RET: /* Internal return prim */
                        if (stop > 1 && program != sys[stop - 1].progref) {
                            if (!ObjExists(sys[stop - 1].progref) ||
                                Typeof(sys[stop - 1].progref) != 
                                TYPE_PROGRAM)
                                abort_loop_hard("Internal error.  Invalid "
                                                "address.", NULL, NULL);

                            calc_profile_timing(program, fr);
                            gettimeofday(&fr->proftime, NULL);
                            PROGRAM_DEC_INSTANCES(program);
                            program = sys[stop - 1].progref;
                            mlev = ProgMLevel(program);
                            fr->caller.top--;
                        }

                        scopedvar_poplevel(fr);
                        pc = sys[--stop].offset;
                        break;

                    case IN_CATCH:
                    case IN_CATCH_DETAILED: /* Doing a catch */
                        {
                            int depth;

                            if (!(fr->trys.top))
                                abort_loop_hard("Internal error.  TRY "
                                                "stack underflow.", NULL,
                                                NULL);

                            depth = fr->trys.st->depth;

                            while (atop > depth) {
                                temp1 = arg + --atop;
                                CLEAR(temp1);
                            }

                            while (fr->trys.st->for_count-- > 0) {
                                CLEAR(&fr->fors.st->cur);
                                CLEAR(&fr->fors.st->end);
                                fr->fors.top--;
                                fr->fors.st = pop_for(fr->fors.st);
                            }

                            fr->trys.top--;
                            fr->trys.st = pop_try(fr->trys.st);

                            if (pc->data.number == IN_CATCH) {
                                /* IN_CATCH */
                                if (fr->errorstr) {
                                    arg[atop].type = PROG_STRING;
                                    arg[atop++].data.string = 
                                        alloc_prog_string(fr->errorstr);
                                    free(fr->errorstr);
                                    fr->errorstr = NULL;
                                } else {
                                    arg[atop].type = PROG_STRING;
                                    arg[atop++].data.string = NULL;
                                }

                                free(fr->errorinst);
                                fr->errorinst = NULL;
                            } else {
                                /* IN_CATCH_DETAILED */
                                stk_array *nu =
                                    new_array_dictionary(fr->pinning);

                                if (fr->errorstr) {
                                    array_set_strkey_strval(&nu, "error",
                                                            fr->errorstr);
                                    free(fr->errorstr);
                                    fr->errorstr = NULL;
                                }

                                if (fr->errorinst) {
                                    array_set_strkey_strval(&nu, "instr",
                                                            fr->errorinst);
                                    free(fr->errorinst);
                                    fr->errorinst = NULL;
                                }

                                array_set_strkey_intval(&nu, "line",
                                                        fr->errorline);
                                array_set_strkey_refval(&nu, "program",
                                                        fr->errorprog);
                                arg[atop].type = PROG_ARRAY;
                                arg[atop++].data.array = nu;
                            }

                            reload(fr, atop, stop);
                        }

                        pc++;
                        break;

                    case IN_EVENT_WAITFOR: /* Waiting for event */
                        if (atop < 1)
                            abort_loop("Stack Underflow. Missing eventID list "
                                       "array argument.", NULL, NULL);

                        if (fr->trys.top && atop - fr->trys.st->depth < 1)
                            abort_loop("Stack protection fault.", NULL, NULL);

                        temp1 = arg + --atop;

                        if (temp1->type != PROG_ARRAY)
                            abort_loop("EventID string list array expected.",
                                       temp1, NULL);

                        if (temp1->data.array &&
                            temp1->data.array->type != ARRAY_PACKED)
                            abort_loop("Argument must be a list array of "
                                       "eventid strings.", temp1, NULL);

                        if (!array_is_homogenous(temp1->data.array, PROG_STRING))
                            abort_loop("Argument must be a list array of "
                                       "eventid strings.", temp1, NULL);

                        fr->pc = pc + 1;
                        reload(fr, atop, stop);

                        {
                            size_t k, outcount;
                            size_t count =
                                (size_t)array_count(temp1->data.array);
                            char **events = malloc(count * sizeof(char **));

                            for (outcount = k = 0; k < count; k++) {
                                char *val =
                                    array_get_intkey_strval(temp1->data.array,
                                                            k);

                                if (val != NULL) {
                                    int found = 0;

                                    for (unsigned int j = 0; j < outcount;
                                         j++) {
                                        if (!strcmp(events[j], val)) {
                                            found = 1;
                                            break;
                                        }
                                    }

                                    if (!found) {
                                        events[outcount++] = val;
                                    }
                                }
                            }

                            muf_event_register_specific(player, program, fr,
                                                        outcount, events);
                            free(events);
                        }

                        PLAYER_SET_BLOCK(player, (!fr->been_background));
                        CLEAR(temp1);
                        record_exit_interp(program, fr);
                        return NULL;

                    case IN_READ: /* Waiting for read */
                        if (writeonly)
                            abort_loop("Program is write-only.", NULL, NULL);

                        if (fr->multitask == BACKGROUND)
                            abort_loop("BACKGROUND programs are write only.",
                                       NULL, NULL);

                        reload(fr, atop, stop);
                        fr->brkpt.isread = 1;
                        fr->pc = pc + 1;
                        PLAYER_SET_CURR_PROG(player, program);
                        PLAYER_SET_BLOCK(player, 0);
                        add_muf_read_event(fr->descr, player, program, fr);
                        record_exit_interp(program, fr);
                        return NULL;

                    case IN_SLEEP: /* Waiting for timer */
                        if (atop < 1)
                            abort_loop("Stack Underflow.", NULL, NULL);

                        if (fr->trys.top && atop - fr->trys.st->depth < 1)
                            abort_loop("Stack protection fault.", NULL, NULL);

                        temp1 = arg + --atop;

                        if (temp1->type != PROG_INTEGER)
                            abort_loop("Invalid argument type.", temp1, NULL);

                        fr->pc = pc + 1;
                        reload(fr, atop, stop);

                        if (temp1->data.number < 0)
                            abort_loop("Timetravel beyond scope of muf.",
                                       temp1, NULL);

                        add_muf_delay_event(temp1->data.number, fr->descr,
                                            player, NOTHING, NOTHING, program,
                                            fr, "SLEEPING");
                        PLAYER_SET_BLOCK(player, (!fr->been_background));
                        record_exit_interp(program, fr);
                        return NULL;

                    default: /* Call a program in the primitive vector */
                        nargs = 0;
#ifdef DEBUG
                        fr->expect_pop = fr->actual_pop = 0;
                        fr->expect_push_to = -1;
#endif
                        reload(fr, atop, stop);
                        tmp = atop;
                        PROGRAM_INC_INSTANCES_IN_PRIMITIVE(program);
                        prim_func[pc->data.number - 1] (player, program, mlev,
                                                        pc, arg, &tmp, fr);
                        PROGRAM_DEC_INSTANCES_IN_PRIMITIVE(program);
#ifdef DEBUG
                        assert(fr->expect_pop == fr->actual_pop || err);
                        assert(fr->expect_push_to == -1 ||
                               fr->expect_push_to == tmp || err);
                        assert(fr->expect_push_to != -1 || tmp <= atop);
#endif
                        atop = tmp;
                        pc++;
                        break;
                } /* switch */

                break;

            case PROG_CLEARED: /* Error condition */
                log_status("WARNING: attempt to execute instruction cleared "
                           "by %s:%hd in program %d", (char *) pc->data.addr,
                           pc->line, program);
                pc = NULL;
                abort_loop_hard("Program internal error. Program erroneously "
                                "freed from memory.", NULL, NULL);
            default: /* Unknown instruction type */
                pc = NULL;
                abort_loop_hard("Program internal error. Unknown instruction "
                                "type.", NULL, NULL);
        } /* switch */

        if (err) { /* Handle error if we need to */
            if (err != ERROR_DIE_NOW && fr->trys.top) {
                while (fr->trys.st->call_level < stop) {
                    if (stop > 1 && program != sys[stop - 1].progref) {
                        if (!ObjExists(sys[stop - 1].progref) ||
                            Typeof(sys[stop - 1].progref) != TYPE_PROGRAM)
                            abort_loop_hard("Internal error.  Invalid address.",
                                            NULL, NULL);

                        calc_profile_timing(program, fr);
                        gettimeofday(&fr->proftime, NULL);
                        PROGRAM_DEC_INSTANCES(program);
                        program = sys[stop - 1].progref;
                        mlev = ProgMLevel(program);
                        fr->caller.top--;
                    }

                    scopedvar_poplevel(fr);
                    stop--;
                }

                pc = fr->trys.st->addr;
                err = 0;
            } else {
                reload(fr, atop, stop);
                prog_clean(fr);
                PLAYER_SET_BLOCK(player, 0);
                record_exit_interp(program, fr);
                return NULL;
            }
        }
    } /* while */

    /* Cleanup and end program */
    PLAYER_SET_BLOCK(player, 0);

    /* Handle return value if available */
    if (atop) {
        struct inst *rv;

        if (rettyp) {
            copyinst(arg + atop - 1, &retval);
            rv = &retval;
        } else {
            if (!false_inst(arg + atop - 1)) {
                rv = (struct inst *) 1;
            } else {
                rv = NULL;
            }
        }

        reload(fr, atop, stop);
        prog_clean(fr);
        record_exit_interp(program, fr);
        return rv;
    }

    /* Clean and exit */
    reload(fr, atop, stop);
    prog_clean(fr);
    record_exit_interp(program, fr);
    return NULL;
}

/**
 * Push a value onto the given instruction stack
 *
 * If a string is pushed onto the stack, it is not copied; it is assumed
 * to be a struct shared_string.
 *
 * @param stack the stack to push to
 * @param top the top stack number -- this will be updated by this call
 * @param type the type, PROG_FLOAT, PROG_STRING, etc.
 * @param res the value to push
 */
void
push(struct inst *stack, int *top, int type, void *res)
{
    stack[*top].type = type;

    if (type == PROG_FLOAT)
        stack[*top].data.fnumber = *(double *) res;
    else if (type < PROG_STRING)
        stack[*top].data.number = *(int *) res;
    else
        stack[*top].data.string = (struct shared_string *) res;

    (*top)++;
}

/**
 * Is the given instruction a valid player ref?
 *
 * @param oper the instruction to check
 * @return boolean true if oper is a valid player ref
 */
int
valid_player(struct inst *oper)
{
    return (!(oper->type != PROG_OBJECT || !ObjExists(oper->data.objref)
            || (Typeof(oper->data.objref) != TYPE_PLAYER)));
}

/**
 * Is the given instruction a valid object ref?
 *
 * @param oper the instruction to check
 * @return boolean true if oper is a valid object ref
 */
int
valid_object(struct inst *oper)
{
    return (!(oper->type != PROG_OBJECT || !ObjExists(oper->data.objref)
            || Typeof(oper->data.objref) == TYPE_GARBAGE));
}

/**
 * Is the given instruction a ref to the special 'HOME' ref?
 *
 * @param oper the instruction to check
 * @return boolean true if oper is HOME, false otherwise.
 */
int
is_home(struct inst *oper)
{
    return (oper->type == PROG_OBJECT && oper->data.objref == HOME);
}

/**
 * Check to see if 'player' has ownership of 'thing'
 *
 * This is like a lame version of 'controls'.  It doesn't check Wizard
 * permissions; it just checks some basic things.
 *
 * If player == thing, thing == HOME, or the owner of player owns 'thing',
 * then this returns true.  If owner thing is NOTHING, that's true as well.
 *
 * This will always be false if player != thing and thing is a player.
 *
 * @param player the player to check permissions for
 * @param thing the thing to check player's permissions of.
 * @return boolean as described above
 */
int
permissions(dbref player, dbref thing)
{

    if (thing == player || thing == HOME)
        return 1;

    switch (Typeof(thing)) {
        case TYPE_PLAYER:
            return 0;

        case TYPE_EXIT:
            return (OWNER(thing) == OWNER(player) || OWNER(thing) == NOTHING);

        case TYPE_ROOM:
        case TYPE_THING:
        case TYPE_PROGRAM:
            return (OWNER(thing) == OWNER(player));
    }

    return 0;
}

/**
 * Determine the effective MUCKER level of the program
 *
 * This is complicated by the STICKY / HAVEN flags and relative MUCKER levels
 * of program vs. owner.  But the result of this will be the MUCKER level the
 * program should run at.
 *
 * Usually you would use ProgMLevel instead of this function, as that define
 * is a shortcut wrapper around this.  @see ProgMLevel
 *
 * @param prog the ref of the program running
 * @param fr the frame of the running program
 * @param st the stack position of the program if one program called another
 * @return the effective MUCKER level as an integer
 */
dbref
find_mlev(dbref prog, struct frame * fr, int st)
{
    if ((FLAGS(prog) & STICKY) && (FLAGS(prog) & HAVEN)) {
        if ((st > 1) && (TrueWizard(OWNER(prog))))
            return (find_mlev(fr->caller.st[st - 1], fr, st - 1));
    }

    if (MLevel(prog) < MLevel(OWNER(prog))) {
        return (MLevel(prog));
    } else {
        return (MLevel(OWNER(prog)));
    }
}

/**
 * Figure out effective dbref of running program's user
 *
 * This handles various nuances such as if the frame is SETUID, the
 * program is STICKY or HAVEN.  There's a lot of little complexities
 * here.
 *
 * If the program is sticky, or the frame is SETUID, and the program is
 * haven and owned by a wizard,, then the UID will be the UID Of the calling
 * program.
 *
 * If the program is sticky but not HAVEN, then the owner of the program
 * will be used as the UID.
 *
 * If the MUCKER level is less than 2, then it will be the UID of the
 * owner of the program.
 *
 * IF the program is HAVEN, then the owner of the trigger is used if possible.
 * Otherwise, return owner of player.
 *
 * @param player the player running the program
 * @param fr the frame of the running program
 * @param st the calling stack level of the running program
 * @param program the dbref of the program
 * @return the effective program runner dbref.
 */
dbref
find_uid(dbref player, struct frame * fr, int st, dbref program)
{
    if ((FLAGS(program) & STICKY) || (fr->perms == STD_SETUID)) {
        if (FLAGS(program) & HAVEN) {
            if ((st > 1) && (TrueWizard(OWNER(program))))
                return (find_uid(player, fr, st - 1, fr->caller.st[st - 1]));

            return (OWNER(program));
        }

        return (OWNER(program));
    }

    if (ProgMLevel(program) < 2)
        return (OWNER(program));

    if ((FLAGS(program) & HAVEN) || (fr->perms == STD_HARDUID)) {
        if (fr->trig == NOTHING)
            return (OWNER(program));

        return (OWNER(fr->trig));
    }

    return (OWNER(player));
}

/**
 * Abort the interpreter.  Do error notifications and stack traces as needed.
 *
 * This is rarely, if ever, called directly; the define abort_interp is
 * used instead.  This function only makes sense when called within the
 * interpreter loop.
 *
 * @see abort_interp
 *
 * @param player the player running teh program
 * @param msg the message to display to the player
 * @param pc the current running instruction (program counter)
 * @param arg the argument stack
 * @param atop the top of the argument stack
 * @param fr the running frame
 * @param oper1 if provided, this will be RCLEAR'd
 * @param oper2 if provided, this will be RCLEAR'd
 * @param oper3 if provided, this will be RCLEAR'd
 * @param oper4 if provided, this will be RCLEAR'd
 * @param nargs the number of operands provided, 0 through 4 are valid
 * @param program the program dbref
 * @param file the file name
 * @param line the line number
 */
void
do_abort_interp(dbref player, const char *msg, struct inst *pc,
                struct inst *arg, int atop, struct frame *fr,
                struct inst *oper1, struct inst *oper2,
                struct inst *oper3, struct inst *oper4, int nargs,
                dbref program, char *file, int line)
{
    char buffer[128];

    if (fr->trys.top) {
        fr->errorstr = strdup(msg);

        if (pc) {
            fr->errorinst =
                strdup(insttotext(fr, 0, pc, buffer, sizeof(buffer), 30,
                       program, 1));
            fr->errorline = pc->line;
        } else {
            fr->errorinst = NULL;
            fr->errorline = -1;
        }

        fr->errorprog = program;
        err++;
    } else {
        fr->pc = pc;
        calc_profile_timing(program, fr);
        interp_err(player, program, pc, arg, atop, fr->caller.st[1],
                   insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program, 1), msg);

        if (controls(player, program))
            muf_backtrace(player, program, STACK_SIZE, fr);
    }

    switch (nargs) {
        case 4:
            RCLEAR(oper4, file, line);
        case 3:
            RCLEAR(oper3, file, line);
        case 2:
            RCLEAR(oper2, file, line);
        case 1:
            RCLEAR(oper1, file, line);
    }

    return;
}

/**
 * Silently abort the interpreter loop.
 *
 * Errors set with this will not be caught.
 *
 * This will always result in program termination the next time
 * interp_loop() checks for this.
 */
void
do_abort_silent(void)
{
    err = ERROR_DIE_NOW;
}

/**
 * Dump debugging data about an instruction
 *
 * This is used by DARK mode programs to dump data in a nicely formatted,
 * buffer-friendly kind of way.  It is only called in the interpreter loop.
 *
 * @param fr the program frame
 * @param lev this is passed into insttotext but is 0 everywhere this function
 *            is called so I'm not sure why we pass it.
 * @param pc the current instruction "program counter"
 * @param pid the running PID
 * @param stack the stack
 * @param buffer our output buffer
 * @param buflen the size of the buffer
 * @param sp the top of the stack
 * @param program the program dbref
 * @return the start of the debug string
 */
char *
debug_inst(struct frame *fr, int lev, struct inst *pc, int pid,
           struct inst *stack, char *buffer, size_t buflen, int sp,
           dbref program)
{
    char *bend;
    char *bstart;
    char *ptr;
    int length;

    char buf2[BUFFER_LEN];

    /* To hold Debug> ... at the beginning */
    char buf3[64];
    int count;


    assert(buflen > 1);

    buffer[buflen - 1] = '\0';

    length = snprintf(buf3, sizeof(buf3), "Debug> Pid %d: #%d %d (", pid,
                      program, pc->line);

    if (length == -1) {
        length = sizeof(buf3) - 1;
    }

    bstart = buffer + length;   /* start far enough away so we can fit Debug> #xxx xxx ( thingy. */
    length = buflen - (size_t)length - 1;       /* - 1 for the '\0' */
    bend = buffer + (buflen - 1);       /* - 1 for the '\0' */

    /* + 10 because we must at least be able to store " ... ) ..." after that. */
    if (bstart + 10 > bend) {   /* we have no room. Eeek! */
        /*                                  123456789012345678 */
        memcpy((void *) buffer, (const void *) "Need buffer space!",
               (buflen - 1 > 18) ? 18 : buflen - 1);
        return buffer;
    }


    /*
     * We use this if-else structure to handle errors and such nicely.
     * We use length - 7 so we KNOW we'll have room for " ... ) "
     */

    ptr = insttotext(fr, lev, pc, buf2, length - 7, 30, program, 1);

    if (*ptr) {
        length -= prepend_string(&bend, bstart, ptr);
    } else {
        strcpyn(buffer, buflen, buf3);
        strcatn(buffer, buflen, " ... ) ...");
        return buffer;
    }

    length -= prepend_string(&bend, bstart, ") ");

    count = sp - 1;

    if (count >= 0) {
        for (;;) {
            if (count && length <= 5) {
                prepend_string(&bend, bstart, "...");
                break;
            }
            /*
             * we use length - 5 to leave room for "..., "
             * ... except if we're outputing the last item (count == 0)
             */
            ptr = insttotext(fr, lev, stack + count, buf2,
                             (size_t)((count) ? length - 5 : length), 30,
                             program, 1);

            if (*ptr) {
                length -= prepend_string(&bend, bstart, ptr);
            } else {
                break;          /* done because we couldn't display all that */
            }

            if (count > 0 && count > sp - 8) {
                length -= prepend_string(&bend, bstart, ", ");
            } else {
                break;          /* all done! */
            }

            count--;
        }
    }

    /* we don't use bstart, because it's compensated for the length of this. */
    prepend_string(&bend, buffer, buf3);

    /* and return the pointer to the beginning of our backwards grown string. */
    return bend;
}

/**
 * Convert an instruction to a displayable string
 *
 * Converts an instruction into a printable string, stores the string in
 * buffer and returns a pointer to it.
 *
 * The first byte of the return value will be NULL if a buffer overflow
 * would have occured.  There's tons of logic here, though what it does is
 * basically straight forward.
 *
 * @param fr the program frame
 * @param lev the function level -- this is passed in to look up scoped args
 * @param theinst the instruction to convert
 * @param buffer the buffer to use
 * @param buflen the length of the buffer
 * @param strmax the maximum string length to display -- longer strings will
 *               truncate the string.  This only applies to string stack items
 * @param program the program ref
 * @param expandarrs boolean if true show items in array
 * @return a pointer to 'buffer'
 */
char *
insttotext(struct frame *fr, int lev, struct inst *theinst, char *buffer,
           int buflen, int strmax, dbref program, int expandarrs)
{
    const char *ptr;
    char buf2[BUFFER_LEN];
    struct inst temp1;
    struct inst *oper2;

    /*
     * unset mark. We don't use -1 since some snprintf() version will
     * return that if we would've overflowed.
     */
    int length = -2;
    int firstflag = 1;
    int arrcount = 0;

    assert(buflen > 0);

    strmax = (strmax > buflen - 3) ? buflen - 3 : strmax;

    switch (theinst->type) {
        case PROG_PRIMITIVE:
            if (theinst->data.number >= 1 && theinst->data.number <= prim_count) {
                ptr = base_inst[theinst->data.number - 1];

                if (strlen(ptr) >= (size_t) buflen)
                    *buffer = '\0';
                else
                    strcpyn(buffer, buflen, ptr);
            } else {
                if (buflen > 3)
                    strcpyn(buffer, buflen, "???");
                else
                    *buffer = '\0';
            }

            break;

        case PROG_STRING:
            if (!theinst->data.string) {
                if (buflen > 2)
                    strcpyn(buffer, buflen, "\"\"");
                else
                    *buffer = '\0';

                break;
            }

            if (strmax <= 0) {
                *buffer = '\0';
                break;
            }

            /* we know we won't overflow, so don't set length */
            snprintf(buffer, buflen, "\"%1.*s\"", (strmax - 1),
                     theinst->data.string->data);

            if ((int)theinst->data.string->length > strmax)
                strcatn(buffer, buflen, "_");

            break;

        case PROG_MARK:
            if (buflen > 4)
                strcpyn(buffer, buflen, "MARK");
            else
                *buffer = '\0';

            break;

        case PROG_ARRAY:
            if (!theinst->data.array) {
                if (buflen > 3)
                    strcpyn(buffer, buflen, "0{}");
                else
                    *buffer = '\0';

                break;
            }

            if (tp_expanded_debug_trace && expandarrs) {
                length = snprintf(buffer, buflen, "%d{",
                                  theinst->data.array->items);

                if (length >= buflen || length == -1) {
                    /*
                     * >= because we need room for one more charctor at the
                     * end
                     */
                    *buffer = '\0';
                    break;
                }

                /* - 1 for the "\0" at the end. */
                length = buflen - (size_t)length - 1;
                firstflag = 1;
                arrcount = 0;

                if (array_first(theinst->data.array, &temp1)) {
                    int truncated_array = 1;

                    while (1) {
                        char *inststr;

                        if (arrcount++ >= 8) {
                            strcatn(buffer, buflen, "_");
                            break;
                        }

                        if (!firstflag) {
                            strcatn(buffer, buflen, " ");
                            length--;
                        }

                        firstflag = 0;
                        oper2 = array_getitem(theinst->data.array, &temp1);

                        if (length <= 2) {
                            /* no space left, let's not pass a buflen of 0 */
                            strcatn(buffer, buflen, "_");
                            break;
                        }

                        /* length - 2 so we have room for the ":_" */
                        inststr =
                            insttotext(fr, lev, &temp1, buf2,
                                      (size_t)length - 2, strmax, program, 0);

                        if (!*inststr) {
                            /* overflow problem. */
                            strcatn(buffer, buflen, "_");
                            break;
                        }

                        length -= strlen(inststr) + 1;
                        strcatn(buffer, buflen, inststr);
                        strcatn(buffer, buflen, ":");

                        if (length <= 2) {
                            /* no space left, let's not pass a buflen of 0 */
                            strcatn(buffer, buflen, "_");
                            break;
                        }

                        inststr = insttotext(fr, lev, oper2, buf2,
                                             (size_t)length, strmax, program,
                                             0);

                        if (!*inststr) {
                            /*
                             * we'd overflow if we did that
                             * as before add a "_" and let it be.
                             */
                            strcatn(buffer, buflen, "_");
                            break;
                        }

                        length -= strlen(inststr);
                        strcatn(buffer, buflen, inststr);

                        if (length < 2) {
                            /*
                             * we should have a length of exactly 1, if we get
                             * here.  So we just have enough room for a '_' now.
                             * Just append the "_" and stop this madness.
                             */
                            strcatn(buffer, buflen, "_");
                            length--;
                            break;
                        }

                        if (!array_next(theinst->data.array, &temp1)) {
                            truncated_array = 0;
                            break;
                        }
                    }

                    if (truncated_array) {
                        CLEAR(&temp1);
                    }
                }

                strcatn(buffer, buflen, "}");
            } else {
                length = snprintf(buffer, buflen, "%d{...}",
                                  theinst->data.array->items);
            }

            break;

        case PROG_INTEGER:
            length = snprintf(buffer, buflen, "%d", theinst->data.number);
            break;

        case PROG_FLOAT:
            length = snprintf(buffer, buflen, "%.16g", theinst->data.fnumber);

            if (!strchr(buffer, '.') && !strchr(buffer, 'n')
                && !strchr(buffer, 'e')) {
                strcatn(buffer, buflen, ".0");
            }

            break;

        case PROG_ADD:
            if (theinst->data.addr->data->type == PROG_FUNCTION &&
                theinst->data.addr->data->data.mufproc != NULL) {
                if (theinst->data.addr->progref != program)
                    length = snprintf(
                            buffer, buflen, "'#%d'%s",
                            theinst->data.addr->progref,
                            theinst->data.addr->data->data.mufproc->procname
                    );
                else
                    length = snprintf(
                            buffer, buflen, "'%s",
                            theinst->data.addr->data->data.mufproc->procname
                    );
            } else {
                if (theinst->data.addr->progref != program)
                    length = snprintf(buffer, buflen, "'#%d'line%d?",
                                      theinst->data.addr->progref,
                                      theinst->data.addr->data->line);
                else
                    length = snprintf(buffer, buflen, "'line%d?",
                                      theinst->data.addr->data->line);
            }

            break;

        case PROG_TRY:
            length = snprintf(buffer, buflen, "TRY->line%d",
                              theinst->data.call->line);
            break;

        case PROG_IF:
            length = snprintf(buffer, buflen, "IF->line%d",
                              theinst->data.call->line);
            break;

        case PROG_EXEC:
            if (theinst->data.call->type == PROG_FUNCTION) {
                length = snprintf(buffer, buflen, "EXEC->%s",
                                  theinst->data.call->data.mufproc->procname);
            } else {
                length = snprintf(buffer, buflen, "EXEC->line%d",
                                  theinst->data.call->line);
            }

            break;

        case PROG_JMP:
            if (theinst->data.call->type == PROG_FUNCTION) {
                length = snprintf(buffer, buflen, "JMP->%s",
                                  theinst->data.call->data.mufproc->procname);
            } else {
                length = snprintf(buffer, buflen, "JMP->line%d",
                                  theinst->data.call->line);
            }

            break;

        case PROG_OBJECT:
            length = snprintf(buffer, buflen, "#%d", theinst->data.objref);
            break;

        case PROG_VAR:
            length = snprintf(buffer, buflen, "V%d", theinst->data.number);
            break;

        case PROG_SVAR:
            if (fr) {
                length = snprintf(
                            buffer, buflen, "SV%d:%s", theinst->data.number,
                            scopedvar_getname(fr, lev, theinst->data.number)
                );
            } else {
                length = snprintf(buffer, buflen, "SV%d", theinst->data.number);
            }

            break;

        case PROG_SVAR_AT:
        case PROG_SVAR_AT_CLEAR:
            if (fr) {
                length = snprintf(
                              buffer, buflen, "SV%d:%s @",
                              theinst->data.number,
                              scopedvar_getname(fr, lev, theinst->data.number));
            } else {
                length = snprintf(buffer, buflen, "SV%d @",
                                  theinst->data.number);
            }

            break;

        case PROG_SVAR_BANG:
            if (fr) {
                length = snprintf(
                          buffer, buflen, "SV%d:%s !",
                          theinst->data.number,
                          scopedvar_getname(fr, lev, theinst->data.number));
            } else {
                length = snprintf(buffer, buflen, "SV%d !",
                                  theinst->data.number);
            }

            break;

        case PROG_LVAR:
            length = snprintf(buffer, buflen, "LV%d", theinst->data.number);
            break;

        case PROG_LVAR_AT:
        case PROG_LVAR_AT_CLEAR:
            length = snprintf(buffer, buflen, "LV%d @", theinst->data.number);
            break;

        case PROG_LVAR_BANG:
            length = snprintf(buffer, buflen, "LV%d !", theinst->data.number);
            break;

        case PROG_FUNCTION:
            length = snprintf(buffer, buflen, "INIT FUNC: %s (%d arg%s)",
                              theinst->data.mufproc->procname,
                              theinst->data.mufproc->args,
                              theinst->data.mufproc->args == 1 ? "" : "s");
            break;

        case PROG_LOCK:
            if (theinst->data.lock == TRUE_BOOLEXP) {
                /*
                 *                  12345678901234
                 * 14
                 */
                if (buflen > 14)
                    strcpyn(buffer, buflen, "[TRUE_BOOLEXP]");
                else
                    *buffer = '\0';

                break;
            }

            length = snprintf(buffer, buflen, "[%1.*s]",
                              (strmax - 1),
                              unparse_boolexp(0, theinst->data.lock, 0));
            break;

        case PROG_CLEARED:
            length = snprintf(buffer, buflen, "?<%s:%d>",
                              (char *) theinst->data.addr,
                              theinst->line);
            break;

        default:
            if (buflen > 3)
                strcpyn(buffer, buflen, "?");
            else
                *buffer = '\0';

            break;
    }

    if (length == -1 || length > buflen)
        *buffer = '\0';

    return buffer;
}
