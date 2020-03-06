/** @file p_misc.c
 *
 * Implementation for handling MUF primitives that didn't fit anywhere else.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "array.h"
#include "boolexp.h"
#include "compile.h"
#include "db.h"
#include "edit.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "mufevent.h"
#include "player.h"
#include "timequeue.h"
#include "tune.h"

/*
 * TODO: These globals really probably shouldn't be globals.  I can only guess
 *       this is either some kind of primitive code re-use because all the
 *       functions use them, or it's some kind of an optimization to avoid
 *       local variables.  But it kills the thread safety and (IMO) makes the
 *       code harder to read/follow.
 */

/**
 * @private
 * @var used to store the parameters passed into the primitives
 *      This seems to be used for conveinance, but makes this probably
 *      not threadsafe.
 */
static struct inst *oper1, *oper2, *oper3, *oper4;

/**
 * @private
 * @var to store the result which is not a local variable for some reason
 *      This makes things very not threadsafe.
 */
static int result;
static dbref ref;
static struct tm *time_tm;

/**
 * @private
 * @var This breaks thread safety for fun.
 */
static char buf[BUFFER_LEN];

/**
 * Implementation of MUF TIME
 *
 * Returns the seconds, minutes, and hours of the current time.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_time(PRIM_PROTOTYPE)
{
    time_t lt;
    struct tm *tm;

    CHECKOP(0);
    CHECKOFLOW(3);

    lt = time(NULL);
    tm = localtime(&lt);

    result = tm->tm_sec;
    PushInt(result);
    result = tm->tm_min;
    PushInt(result);
    result = tm->tm_hour;
    PushInt(result);
}

/**
 * Implementation of MUF DATE
 *
 * Returns the month day, month number, and 4-digit year of the current date.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_date(PRIM_PROTOTYPE)
{
    time_t lt;
    struct tm *tm;

    CHECKOP(0);
    CHECKOFLOW(3);

    lt = time(NULL);
    tm = localtime(&lt);

    result = tm->tm_mday;
    PushInt(result);
    result = tm->tm_mon + 1;
    PushInt(result);
    result = tm->tm_year + 1900;
    PushInt(result);
}

/**
 * Implementation of MUF GMTOFFSET
 *
 * Returns the time zone offset from GMT.
 *
 * @see get_tz_offset
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_gmtoffset(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = get_tz_offset();
    PushInt(result);
}

/**
 * Implementation of MUF SYSTIME
 *
 * Returns the current Unix time as an integer.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_systime(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = (int)time(NULL);
    PushInt(result);
}

/**
 * Implementation of MUF SYSTIME_PRECISE
 *
 * Returns the current Unix time as an floating point number.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_systime_precise(PRIM_PROTOTYPE)
{
    struct timeval fulltime;
    double dbltime;

    CHECKOP(0);
    CHECKOFLOW(1);

    gettimeofday(&fulltime, (struct timezone *) 0);

    dbltime = fulltime.tv_sec + (((double) fulltime.tv_usec) / 1.0e6);
    PushFloat(dbltime);
}

/**
 * Implementation of MUF TIMESPLIT
 *
 * Consumes an integer representing a Unix time, and returns the
 * corresponsing seconds, minutes, hours, month day, month number, 4-digit
 * year, weekday number, the day of the year.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_timesplit(PRIM_PROTOTYPE)
{
    time_t lt = 0;

    CHECKOP(1);
    oper1 = POP();              /* integer: time */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid argument");

    lt = (time_t) oper1->data.number;
    time_tm = localtime(&lt);

    if (!time_tm)
        abort_interp("Out of range time argument");

    CHECKOFLOW(8);
    CLEAR(oper1);

    result = time_tm->tm_sec;
    PushInt(result);
    result = time_tm->tm_min;
    PushInt(result);
    result = time_tm->tm_hour;
    PushInt(result);
    result = time_tm->tm_mday;
    PushInt(result);
    result = time_tm->tm_mon + 1;
    PushInt(result);
    result = time_tm->tm_year + 1900;
    PushInt(result);
    result = time_tm->tm_wday + 1;
    PushInt(result);
    result = time_tm->tm_yday + 1;
    PushInt(result);
}

/**
 * Implementation of MUF TIMEFMT
 *
 * Consumes a string and a Unix time, and returns a string formatted with
 * the time, via the system's strftime function.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_timefmt(PRIM_PROTOTYPE)
{
    time_t lt = 0;

    CHECKOP(2);
    oper2 = POP();              /* integer: time */
    oper1 = POP();              /* string: format */

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument (1)");

    if (!oper1->data.string)
        abort_interp("Illegal NULL string (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Invalid argument (2)");

    lt = (time_t) oper2->data.number;
    time_tm = localtime(&lt);

    if (!time_tm)
        abort_interp("Out of range time argument");

    if (!strftime(buf, BUFFER_LEN, oper1->data.string->data, time_tm))
        abort_interp("Operation would result in overflow.");

    CHECKOFLOW(1);
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(buf);
}

/**
 * Implementation of MUF CONVTIME
 *
 * Consumes a time string in the format "HH:MM:SS MO/DY/YR" into a Unix time.
 *
 * @see time_string_to_seconds
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_convtime(PRIM_PROTOTYPE)
{
    /**
     * @TODO Change this to use the cross-platform timelib? (GitHub #472)
     */
#ifdef WIN32
    abort_interp(CURRENTLY_UNAVAILABLE);
#else
    char *error = 0;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid time string");

    time_t seconds = time_string_to_seconds(oper1->data.string->data, "%T%t%D",
            &error);

    if (error)
        abort_interp(error);

    PushInt(seconds);
#endif
}

/**
 * Implementation of MUF FMTTIME
 *
 * Consumes two strings (input and format), and returns the Unix time
 * resulting from applying the format to the input.
 *
 * @see time_string_to_seconds
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_fmttime(PRIM_PROTOTYPE)
{
    /**
     * @TODO Change this to use the cross-platform timelib? (GitHub #472)
     */
#ifdef WIN32
    abort_interp(CURRENTLY_UNAVAILABLE);
#else
    char *error = 0;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid format string");

    if (oper2->type != PROG_STRING)
        abort_interp("Invalid time string");

    time_t seconds = time_string_to_seconds(oper2->data.string->data,
            oper1->data.string->data, &error);

    if (error)
        abort_interp(error);

    PushInt(seconds);
#endif
}

/**
 * Implementation of MUF USERLOG
 *
 * Consumes a string, and formats it into a message to write to the user log,
 * which is by default 'logs/user'.  Requires MUCKER level tp_userlog_mlev.
 *
 * Format is:
 * player(#) [program.muf(#)] timestamp: message
 *
 * @see log_user
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_userlog(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument.");

    if (mlev < tp_userlog_mlev)
        abort_interp("Permission Denied (mlev < tp_userlog_mlev)");

    /**
     * @TODO Would this work in place of this construct?
     *           buf = DoNullInd(oper1->data.string);
     */
    if (oper1->data.string) {
        snprintf(buf, BUFFER_LEN, "%s", oper1->data.string->data);
    } else {
        *buf = '\0';
    }

    log_user(player, program, buf);

    CLEAR(oper1);
}

/**
 * Implementation of MUF QUEUE
 *
 * Consumes an integer, a program dbref, and a string - and executes the
 * program after the given number of seconds, with the string as a parameter.
 * Requires MUCKER level 3.
 *
 * @see add_muf_delayq_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_queue(PRIM_PROTOTYPE)
{
    dbref temproom;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker level 3 or better.");

    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1).");

    /**
     * @TODO Combine these three checks.
     */
    if (oper2->type != PROG_OBJECT)
        abort_interp("Argument must be a dbref (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid dbref (2)");

    if (Typeof(oper2->data.objref) != TYPE_PROGRAM)
        abort_interp("Object must be a program. (2)");

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (3).");

    /**
     * @TODO Just use LOCATION(player) as the trigger?  Not sure why
     *       the 'loc' variable (which can be overwritten) is checked?
     *       I may be misunderstanding (wyld-sw).
     */
    if ((oper4 = fr->variables + 1)->type != PROG_OBJECT)
        temproom = LOCATION(player);
    else
        temproom = oper4->data.objref;

    result = add_muf_delayq_event(oper3->data.number, fr->descr, player,
            temproom, NOTHING, oper2->data.objref,
            DoNullInd(oper1->data.string), "Queued Event.", 0);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

/**
 * Implementation of MUF KILL
 *
 * Consumes an integer, and tries to kill the associated process - returning
 * 1 if successful and 0 if not.  Requires MUCKER level 3 to bypass ownership
 * checks.
 *
 * @see control_process
 * @see dequeue_process
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_kill(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1).");

    if (oper1->data.number == fr->pid) {
        do_abort_silent();
    } else {
        if (mlev < 3) {
            if (!control_process(ProgUID, oper1->data.number)) {
                abort_interp("Permission Denied.");
            }
        }

        result = dequeue_process(oper1->data.number);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF FORCE
 *
 * Consumes a player or thing dbref and a non-null command string, and forces
 * the object to perform the command.  Requires MUCKER level 4.
 *
 * God cannot be forced by programs not owned by God, if GOD_PRIV is enabled.
 *
 * @see process_command
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_force(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */

    CHECKOP(2);
    oper1 = POP();              /* string to @force */
    oper2 = POP();              /* player dbref */

    if (mlev < 4)
        abort_interp("Wizbit only primitive.");

    if (fr->level > 8)
        abort_interp("Interp call loops not allowed.");

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2).");

    if (oper2->type != PROG_OBJECT)
        abort_interp("Non-object argument (1).");

    ref = oper2->data.objref;
    if (!ObjExists(ref))
        abort_interp("Invalid object to force. (1)");

    if (Typeof(ref) != TYPE_PLAYER && Typeof(ref) != TYPE_THING)
        abort_interp("Object to force not a thing or player. (1)");

    if (0 == strcmp(DoNullInd(oper1->data.string), ""))
        abort_interp("Empty command argument (2).");

    if (strchr(oper1->data.string->data, '\r'))
        abort_interp("Carriage returns not allowed in command string. (2).");

#ifdef GOD_PRIV
    if (God(oper2->data.objref) && !God(OWNER(program)))
        abort_interp("Cannot force god (1).");
#endif

    objnode_push(&forcelist, player);

    if (player != program) objnode_push(&forcelist, program);

    force_level++;
    process_command(dbref_first_descr(oper2->data.objref), oper2->data.objref,
                    oper1->data.string->data);
    force_level--;

    objnode_pop(&forcelist);
    if (player != program) objnode_pop(&forcelist);

    for (int i = 1; i <= fr->caller.top; i++) {
        if (Typeof(fr->caller.st[i]) != TYPE_PROGRAM) {
#ifdef DEBUG
            notifyf_nolisten(player, "[debug] prim_force: fr->caller.st[%d] isn't a program.", i);
#endif                          /* DEBUG */
            do_abort_silent();
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF TIMESTAMPS
 *
 * Consumes a dbref, and returns the created date, last modified date, last
 * used date, and use count.  Requires MUCKER level 2 to bypass basic
 * ownership checks.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_timestamps(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    /**
     * @TODO Combine these checks.
     */
    if (oper1->type != PROG_OBJECT)
        abort_interp("Non-object argument (1).");

    if (!valid_object(oper1))
        abort_interp("Invalid object.");

    CHECKREMOTE(oper1->data.objref);
    CHECKOFLOW(4);

    ref = oper1->data.objref;

    CLEAR(oper1);

    result = (int)DBFETCH(ref)->ts_created;
    PushInt(result);
    result = (int)DBFETCH(ref)->ts_modified;
    PushInt(result);
    result = (int)DBFETCH(ref)->ts_lastused;
    PushInt(result);
    result = DBFETCH(ref)->ts_usecount;
    PushInt(result);
}

/**
 * Implementation of MUF FORK
 *
 * Spawns a copy of the current program in the background.  A child process
 * created in this way has an added 0 on its stack, and this primitive
 * returns the child's PID (or -1 if unsuccessful) to the parent.   Requires
 * MUCKER level 3.
 *
 * @see add_muf_delay_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_fork(PRIM_PROTOTYPE)
{
    struct frame *tmpfr;

    CHECKOP(0);
    CHECKOFLOW(1);

    if (mlev < 3)
        abort_interp("Permission Denied.");

    fr->pc = pc;

    tmpfr = calloc(1, sizeof(struct frame));
    tmpfr->next = NULL;

    array_init_active_list(&tmpfr->array_active_list);
    stk_array_active_list = &tmpfr->array_active_list;

    tmpfr->system.top = fr->system.top;
    for (int i = 0; i < fr->system.top; i++)
        tmpfr->system.st[i] = fr->system.st[i];

    tmpfr->argument.top = fr->argument.top;
    for (int i = 0; i < fr->argument.top; i++)
        deep_copyinst(&fr->argument.st[i], &tmpfr->argument.st[i], -1);

    tmpfr->caller.top = fr->caller.top;
    for (int i = 0; i <= fr->caller.top; i++) {
        tmpfr->caller.st[i] = fr->caller.st[i];

        if (i > 0)
            PROGRAM_INC_INSTANCES(fr->caller.st[i]);
    }

    tmpfr->trys.top = fr->trys.top;
    tmpfr->trys.st = copy_trys(fr->trys.st);

    tmpfr->fors.top = fr->fors.top;
    tmpfr->fors.st = copy_fors(fr->fors.st);

    for (int i = 0; i < MAX_VAR; i++)
        deep_copyinst(&fr->variables[i], &tmpfr->variables[i], -1);

    localvar_dupall(tmpfr, fr);
    scopedvar_dupall(tmpfr, fr);

    tmpfr->error.is_flags = fr->error.is_flags;

    if (fr->rndbuf) {
        tmpfr->rndbuf = init_seed(fr->rndbuf);
    } else {
        tmpfr->rndbuf = NULL;
    }

    tmpfr->pc = pc;
    tmpfr->pc++;
    tmpfr->level = fr->level;
    tmpfr->already_created = fr->already_created;
    tmpfr->trig = fr->trig;

    /**
     * @TODO Aren't the assignments to 0 and NULL redundant after the calloc?
     */
    tmpfr->brkpt.debugging = 0;
    tmpfr->brkpt.bypass = 0;
    tmpfr->brkpt.isread = 0;
    tmpfr->brkpt.showstack = 0;
    tmpfr->brkpt.lastline = 0;
    tmpfr->brkpt.lastpc = 0;
    tmpfr->brkpt.lastlisted = 0;
    tmpfr->brkpt.lastcmd = NULL;
    tmpfr->brkpt.breaknum = -1;

    tmpfr->brkpt.lastproglisted = NOTHING;
    tmpfr->brkpt.proglines = NULL;

    tmpfr->brkpt.count = 1;
    tmpfr->brkpt.temp[0] = 1;
    tmpfr->brkpt.level[0] = -1;
    tmpfr->brkpt.line[0] = -1;
    tmpfr->brkpt.linecount[0] = -2;
    tmpfr->brkpt.pc[0] = NULL;
    tmpfr->brkpt.pccount[0] = -2;
    tmpfr->brkpt.prog[0] = program;

    tmpfr->proftime.tv_sec = 0;
    tmpfr->proftime.tv_usec = 0;
    tmpfr->totaltime.tv_sec = 0;
    tmpfr->totaltime.tv_usec = 0;

    tmpfr->pid = top_pid++;
    tmpfr->multitask = BACKGROUND;
    tmpfr->been_background = 1;
    tmpfr->writeonly = 1;
    tmpfr->started = time(NULL);
    tmpfr->instcnt = 0;
    tmpfr->skip_declare = fr->skip_declare;
    tmpfr->wantsblanks = fr->wantsblanks;
    tmpfr->perms = fr->perms;
    tmpfr->descr = fr->descr;
    tmpfr->supplicant = fr->supplicant;
    tmpfr->events = NULL;
    tmpfr->waiters = NULL;
    tmpfr->waitees = NULL;
    tmpfr->dlogids = NULL;
    tmpfr->timercount = 0;

    /* child process gets a 0 returned on the stack */
    result = 0;
    push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_INTEGER, &result);

    result = add_muf_delay_event(0, fr->descr, player, NOTHING, NOTHING, program,
                                 tmpfr, "BACKGROUND");

    stk_array_active_list = &fr->array_active_list;

    /* parent process gets the child's pid returned on the stack */
    if (!result)
        result = -1;

    PushInt(result);
}

/**
 * Implementation of MUF PID
 *
 * Returns the current program's PID.
 *
 * @see add_muf_delay_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_pid(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = fr->pid;
    PushInt(result);
}

/**
 * Implementation of MUF STATS
 *
 * Consumes a player dbref, and returns the number of owned objects as a
 * series of integers: total, rooms, exits, things, programs, players, and
 * garbage.  Requires MUCKER level 3.
 *
 * If the dbref is NOTHING, the integers represent all objects MUCK-wide.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_stats(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    /**
     * @TODO Should we allow anyone to summarize their own objects?
     */
    if (mlev < 3)
        abort_interp("Requires Mucker Level 3.");

    if (!valid_player(oper1) && (oper1->data.objref != NOTHING))
        abort_interp("non-player argument (1)");

    ref = oper1->data.objref;

    CLEAR(oper1);

    /**
     * @TODO Why?
     */
    nargs = 0;

    /**
     * @TODO Maybe use an array indexed by object type.  Too bad they aren't
     *       defined in exactly this order.
     */
    {
        int rooms, exits, things, players, programs, garbage;

        /* tmp, ref */
        rooms = exits = things = players = programs = garbage = 0;

        for (dbref i = 0; i < db_top; i++) {
            if (ref == NOTHING || OWNER(i) == ref) {
                switch (Typeof(i)) {
                    case TYPE_ROOM:
                        rooms++;
                        break;

                    case TYPE_EXIT:
                        exits++;
                        break;

                    case TYPE_THING:
                        things++;
                        break;

                    case TYPE_PLAYER:
                        players++;
                        break;

                    case TYPE_PROGRAM:
                        programs++;
                        break;

                    case TYPE_GARBAGE:
                        garbage++;
                        break;
                }
            }
        }

        ref = rooms + exits + things + players + programs + garbage;

        CHECKOFLOW(7);
        PushInt(ref);
        PushInt(rooms);
        PushInt(exits);
        PushInt(things);
        PushInt(programs);
        PushInt(players);
        PushInt(garbage);
    }
}

/**
 * Implementation of MUF STATS_ARRAY
 *
 * Consumes a player dbref, and returns an array containing the number of
 * owned objects as a series of integers: garbage, players, programs, things,
 * exits, rooms, and total.  Requires MUCKER level 3.
 *
 * If the dbref is NOTHING, the integers represent all objects MUCK-wide.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_stats_array(PRIM_PROTOTYPE)
{
    /**
     * @TODO The same comments for prim_stats apply here.
     */
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3.");

    if (!valid_player(oper1) && (oper1->data.objref != NOTHING))
        abort_interp("non-player argument (1)");
    ref = oper1->data.objref;

    CLEAR(oper1);

    nargs = 0;

    int rooms = 0, exits = 0, things = 0, programs = 0, players = 0, garbage = 0;
    stk_array *nu;
    int count = 0;

    for (dbref i = 0; i < db_top; i++) {
        if (ref == NOTHING || OWNER(i) == ref) {
            switch (Typeof(i)) {
                case TYPE_ROOM:
                    rooms++;
                    break;

                case TYPE_EXIT:
                    exits++;
                    break;

                case TYPE_THING:
                    things++;
                    break;

                case TYPE_PROGRAM:
                    programs++;
                    break;

                case TYPE_PLAYER:
                    players++;
                    break;

                case TYPE_GARBAGE:
                    garbage++;
                    break;
            }
        }
    }

    ref = rooms + exits + things + programs + players + garbage;

    CHECKOFLOW(1);

    nu = new_array_packed(0, fr->pinning);

    array_set_intkey_intval(&nu, count++, garbage);
    array_set_intkey_intval(&nu, count++, players);
    array_set_intkey_intval(&nu, count++, programs);
    array_set_intkey_intval(&nu, count++, things);
    array_set_intkey_intval(&nu, count++, exits);
    array_set_intkey_intval(&nu, count++, rooms);
    array_set_intkey_intval(&nu, count++, rooms+exits+things+programs+players
            +garbage);

    PushArrayRaw(nu);
}

/**
 * Implementation of MUF ABORT
 *
 * Consumes a string, and aborts the current program with that message.
 *
 * @see abort_interp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_abort(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument");

    strcpyn(buf, sizeof(buf), DoNullInd(oper1->data.string));

    abort_interp(buf);
}

/**
 * Implementation of MUF ISPID?
 *
 * Consumes an integer, and returns a boolean that represents if it is a
 * valid PID (either in the timequeue or the current PID)
 *
 * @see in_timequeue
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_ispidp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (1).");

    if (oper1->data.number == fr->pid) {
        result = 1;
    } else {
        result = in_timequeue(oper1->data.number);
    }

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF PARSELOCK
 *
 * Consumes an string, and attempts to parse it into a lock.  Returns
 * the special lock [TRUE_BOOLEXP] if not successful.
 *
 * Match failure messages are emitted during this process.
 *
 * @see parselock
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_parselock(PRIM_PROTOTYPE)
{
    struct boolexp *lok;

    CHECKOP(1);
    oper1 = POP();              /* string: lock string */
    CHECKOFLOW(1);

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument.");

    if (oper1->data.string != (struct shared_string *) NULL) {
        lok = parse_boolexp(fr->descr, ProgUID, oper1->data.string->data, 0);
    } else {
        lok = TRUE_BOOLEXP;
    }

    CLEAR(oper1);
    PushLock(lok);
    free_boolexp(lok);
}

/**
 * Implementation of MUF UNPARSELOCK
 *
 * Consumes a lock, and returns its raw string representation.
 *
 * @see unparse_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_unparselock(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(1);
    oper1 = POP();              /* lock: lock */

    if (oper1->type != PROG_LOCK)
        abort_interp("Invalid argument.");

    if (oper1->data.lock != (struct boolexp *) TRUE_BOOLEXP) {
        ptr = unparse_boolexp(ProgUID, oper1->data.lock, 0);
    } else {
        ptr = NULL;
    }

    CHECKOFLOW(1);
    CLEAR(oper1);

    if (ptr) {
        PushString(ptr);
    } else {
        PushNullStr;
    }
}

/**
 * Implementation of MUF PRETTYLOCK
 *
 * Consumes a lock, and returns it as a human-readable string.
 *
 * @see unparse_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_prettylock(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(1);
    oper1 = POP();              /* lock: lock */

    if (oper1->type != PROG_LOCK)
        abort_interp("Invalid argument.");

    ptr = unparse_boolexp(ProgUID, oper1->data.lock, 1);

    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(ptr);
}

/**
 * Implementation of MUF TESTLOCK
 *
 * Consumes a lock and a player or thing dbref, and returns a boolean that
 * represents the evaluation of the object against the lock.  Requires
 * MUCKER level 2 to bypass basic ownership checks.
 *
 * If the sysparm consistent_lock_source is on, the source object is the
 * trigger stored in the program frame, otherwise it is the player.
 *
 * @see eval_boolexp
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_testlock(PRIM_PROTOTYPE)
{
    struct inst *oper1 = NULL;  /* prevents re-entrancy issues! */
    struct inst *oper2 = NULL;  /* prevents re-entrancy issues! */

    CHECKOP(2);
    oper1 = POP();              /* boolexp lock */
    oper2 = POP();              /* player dbref */

    if (fr->level > 8)
        abort_interp("Interp call loops not allowed.");

    /**
     * @TODO Combine these checks.
     */
    if (!valid_object(oper2))
        abort_interp("Invalid argument (1).");

    if (Typeof(oper2->data.objref) != TYPE_PLAYER && Typeof(oper2->data.objref) != TYPE_THING) {
        abort_interp("Invalid object type (1).");
    }

    CHECKREMOTE(oper2->data.objref);

    if (oper1->type != PROG_LOCK)
        abort_interp("Invalid argument (2).");

    result = eval_boolexp(fr->descr, oper2->data.objref, oper1->data.lock,
                          tp_consistent_lock_source ? fr->trig : player);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF SYSPARM
 *
 * Consumes a string, and returns the associated sysparm value, if the
 * current MUCKER level allows it.
 *
 * @see tune_get_parmstring
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_sysparm(PRIM_PROTOTYPE)
{
    const char *ptr;

    CHECKOP(1);
    oper1 = POP();              /* string: system parm name */

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument.");

    /**
     * @TODO Another use case for DoNullInd().
     */
    if (oper1->data.string) {
        ptr = tune_get_parmstring(oper1->data.string->data, TUNE_MLEV(player));
    } else {
        ptr = "";
    }

    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(ptr);
}

/**
 * Implementation of MUF CANCALL?
 *
 * Consumes a program dbref and a string, and returns a boolean that
 * represents if the current program has permissions to call the function
 * in the given program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_cancallp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();              /* string: public function name */
    oper1 = POP();              /* dbref: Program dbref to check */

    /**
     * @TODO Combine these three checks.
     */
    if (oper1->type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");

    if (!valid_object(oper1))
        abort_interp("Invalid dbref (1)");

    if (Typeof(oper1->data.objref) != TYPE_PROGRAM)
        abort_interp("Object is not a MUF Program. (1)");

    /**
     * @TODO ...and the next two.
     */
    if (oper2->type != PROG_STRING)
        abort_interp("Expected string argument. (2)");

    if (!oper2->data.string)
        abort_interp("Invalid Null string argument. (2)");

    if (!(PROGRAM_CODE(oper1->data.objref))) {
        struct line *tmpline;

        tmpline = PROGRAM_FIRST(oper1->data.objref);
        PROGRAM_SET_FIRST(oper1->data.objref,
                          (struct line *) read_program(oper1->data.objref));

        do_compile(-1, OWNER(oper1->data.objref), oper1->data.objref, 0);

        free_prog_text(PROGRAM_FIRST(oper1->data.objref));
        PROGRAM_SET_FIRST(oper1->data.objref, tmpline);
    }

    result = 0;

    if (ProgMLevel(oper1->data.objref) > 0 &&
            (mlev >= 4 || OWNER(oper1->data.objref) == ProgUID
            || Linkable(oper1->data.objref))) {
        struct publics *pbs;

        pbs = PROGRAM_PUBS(oper1->data.objref);

        while (pbs) {
            if (!strcasecmp(oper2->data.string->data, pbs->subname))
                break;
            pbs = pbs->next;
        }

        if (pbs && mlev >= pbs->mlev)
            result = 1;
    }

    CHECKOFLOW(1);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF SETSYSPARM
 *
 * Consumes two strings, and sets the value of the sysparm represented by
 * the first to the second.  Requires MUCKER level 4 and cannot be forced.
 *
 * @see tune_setparm
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_setsysparm(PRIM_PROTOTYPE)
{
    const char *parmname, *newvalue;
    char *oldvalue;
    int security = TUNE_MLEV(player);

    CHECKOP(2);
    oper1 = POP();              /* string: new parameter value */
    oper2 = POP();              /* string: parameter to tune */

    if (mlev < 4)
        abort_interp("Wizbit only primitive.");

    if (force_level)
        abort_interp("Cannot be forced.");

    if (oper2->type != PROG_STRING)
        abort_interp("Invalid argument. (1)");

    if (!oper2->data.string)
        abort_interp("Null string argument. (1)");

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument. (2)");

    parmname = oper2->data.string->data;
    /* Duplicate the string, otherwise the oldvalue pointer will be overridden to the new value
       when tune_setparm() is called. */
    oldvalue = strdup(tune_get_parmstring(oper2->data.string->data, security));
    newvalue = DoNullInd(oper1->data.string);

    result = tune_setparm(player, parmname, newvalue, security);

    /* Note: free(oldvalue) BEFORE calling abort_interp, or it will leak. */
    switch (result) {
        case TUNESET_SUCCESS:
            log_status("TUNED (MUF): %s(%d) tuned %s from '%s' to '%s'",
                    NAME(player), player, parmname, oldvalue, newvalue);
            free(oldvalue);
            break;

        case TUNESET_SUCCESS_DEFAULT:
            /* No need to show the flag in output */
            TP_CLEAR_FLAG_DEFAULT(parmname);
            log_status("TUNED (MUF): %s(%d) tuned %s from '%s' to default",
                    NAME(player), player, parmname, oldvalue);
            free(oldvalue);
             break;

        case TUNESET_UNKNOWN:
            free(oldvalue);
            abort_interp("Unknown parameter. (1)");

        case TUNESET_SYNTAX:
            free(oldvalue);
            abort_interp("Bad parameter syntax. (2)");

        case TUNESET_BADVAL:
            free(oldvalue);
            abort_interp("Bad parameter value. (2)");

        case TUNESET_DENIED:
            free(oldvalue);
            abort_interp("Permission denied. (1)");

        default:
            free(oldvalue);
            break;
    }

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF SYSPARM_ARRAY
 *
 * Consumes a string, and returns the associated sysparm details, if the
 * current MUCKER level allows it.
 *
 * @see tune_parms_array
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_sysparm_array(PRIM_PROTOTYPE)
{
    stk_array *nu;
    int security = TUNE_MLEV(player);

    CHECKOP(1);
    oper1 = POP();              /* string: match pattern */

    if (oper1->type != PROG_STRING)
        abort_interp("Expected a string smatch pattern.");

    nu = tune_parms_array(DoNullInd(oper1->data.string), security, fr->pinning);

    CLEAR(oper1);
    PushArrayRaw(nu);
}

/**
 * Implementation of MUF TIMER_START
 *
 * Consumes an integer and a string, and starts a timer event with the given
 * name to come due after the given number of seconds.
 *
 * Each MUF process is limited to process_timer_limit timers.
 *
 * @see add_muf_timer_event
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_timer_start(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();              /* string: timer id */
    oper1 = POP();              /* int: delay length in seconds */

    if (fr->timercount > tp_process_timer_limit)
        abort_interp("Too many timers!");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Expected an integer delay time. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Expected a string timer id. (2)");

    dequeue_timers(fr->pid, DoNullInd(oper2->data.string));

    add_muf_timer_event(fr->descr, player, program, fr, oper1->data.number,
                        DoNullInd(oper2->data.string));

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF TIMER_STOP
 *
 * Consumes a string, and stops the timer with the given name.
 *
 * @see dequeue_timers
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_timer_stop(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();              /* string: timer id */

    if (oper1->type != PROG_STRING)
        abort_interp("Expected a string timer id. (2)");

    dequeue_timers(fr->pid, DoNullInd(oper1->data.string));

    CLEAR(oper1);
}

/**
 * Implementation of MUF EVENT_EXISTS
 *
 * Consumes a string, and returns the number of events with the given type
 * that exist for the frame.
 *
 * @see event_exists
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_event_exists(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();              /* str: eventID to look for */

    if (oper1->type != PROG_STRING || !oper1->data.string)
        abort_interp("Expected a non-null string eventid to search for.");

    result = muf_event_exists(fr, oper1->data.string->data);

    CLEAR(oper1);
    PushInt(result);
}

/**
 * Implementation of MUF EVENT_COUNT
 *
 * Returns the number of events that exist for the frame.
 *
 * @see event_count
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_event_count(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    result = muf_event_count(fr);
    PushInt(result);
}

/**
 * Implementation of MUF EVENT_SEND
 *
 * Consumes a process ID, event name, and a data item - and sends a USER
 * event to the given proess with the given data.  Requires MUCKER level 3.
 *.
 * @see muf_event_add
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_event_send(PRIM_PROTOTYPE)
{
    struct frame *destfr;
    stk_array *arr;
    struct inst temp1;

    CHECKOP(3);
    oper3 = POP();              /* any: data to pass */
    oper2 = POP();              /* string: event id */
    oper1 = POP();              /* int: process id to send to */

    if (mlev < 3)
        abort_interp("Requires Mucker level 3 or better.");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Expected an integer process id. (1)");

    if (oper2->type != PROG_STRING)
        abort_interp("Expected a string event id. (2)");

    if (oper1->data.number == fr->pid)
        destfr = fr;
    else
        destfr = timequeue_pid_frame(oper1->data.number);

    if (destfr) {
        stk_array_active_list = &destfr->array_active_list;
        struct inst data_copy;

        deep_copyinst(oper3, &data_copy, destfr->pinning);

        arr = new_array_dictionary(destfr->pinning);
        array_set_strkey(&arr, "data", &data_copy);
        array_set_strkey_intval(&arr, "caller_pid", fr->pid);
        array_set_strkey_intval(&arr, "descr", fr->descr);
        array_set_strkey_refval(&arr, "caller_prog", program);
        array_set_strkey_refval(&arr, "trigger", fr->trig);
        array_set_strkey_refval(&arr, "prog_uid", ProgUID);
        array_set_strkey_refval(&arr, "player", player);

        temp1.type = PROG_ARRAY;
        temp1.data.array = arr;

        snprintf(buf, sizeof(buf), "USER.%.32s", DoNullInd(oper2->data.string));

        muf_event_add(destfr, buf, &temp1, 0);

        stk_array_active_list = &fr->array_active_list;

        CLEAR(&temp1);
        CLEAR(&data_copy);
    }

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}

/**
 * Implementation of MUF EXT-NAME-OK?
 *
 * Consumes a string and an object specification, and returns a boolean that
 * represents if an object can be created with the given name.
 *
 * The specification can be an object that is of the same type of the object,
 * or a string representing the type.  If the second is used, the values can
 * be one of these case-sensitive strings:
 * - "e", "exit"
 * - "r", "room"
 * - "t", "thing"
 * - "p", "player"
 * - "f", "muf", "program"
 *
 * @see ok_object_name
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_ext_name_okp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Object name string expected (1).");

    char *b = DoNullInd(oper1->data.string);

    if (oper2->type == PROG_STRING) {
        strcpyn(buf, sizeof(buf), DoNullInd(oper2->data.string));

        for (ref = 0; buf[ref]; ref++)
            buf[ref] = tolower(buf[ref]);

        /**
         * @TODO Maybe these should use case-insenstive?
         */
        if (!strcmp(buf, "e") || !strcmp(buf, "exit")) {
            result = ok_object_name(b, TYPE_EXIT);
        } else if (!strcmp(buf, "r") || !strcmp(buf, "room")) {
            result = ok_object_name(b, TYPE_ROOM);
        } else if (!strcmp(buf, "t") || !strcmp(buf, "thing")) {
            result = ok_object_name(b, TYPE_THING);
        } else if (!strcmp(buf, "p") || !strcmp(buf, "player")) {
            result = ok_object_name(b, TYPE_PLAYER);
        } else if (!strcmp(buf, "f") || !strcmp(buf, "muf")
                   || !strcmp(buf, "program")) {
            result = ok_object_name(b, TYPE_PROGRAM);
        } else {
            abort_interp("String must be a valid object type (2).");
        }
    } else if (oper2->type == PROG_OBJECT) {
        if (!valid_object(oper2))
            abort_interp("Invalid argument (2).");

        result = ok_object_name(b, Typeof(oper2->data.objref));
    } else {
        abort_interp("Dbref or object type name expected (2).");
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF FORCE_LEVEL
 *
 * Returns the force level of the current program.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_force_level(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    PushInt(force_level);
}

/**
 * Implementation of MUF FORCEDBY
 *
 * Returns the object that most recently forced the current program, or
 * NOTHING if no forcing has occurred.  Requires MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_forcedby(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    CHECKOFLOW(1);

    if (mlev < 4)
        abort_interp("Wizbit only primitive.");

    if (forcelist) {
        ref = forcelist->data;
    } else {
        ref = NOTHING;
    }

    PushObject(ref);
}

/**
 * Implementation of MUF FORCEDBY_ARRAY
 *
 * Returns an array of objects forcingthe current program.  Requires
 * MUCKER level 4.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_forcedby_array(PRIM_PROTOTYPE)
{
    stk_array *nu;
    int count = 0;

    CHECKOP(0);
    CHECKOFLOW(1);

    if (mlev < 4)
        abort_interp("Wizbit only primitive.");

    if (!forcelist) {
        PushArrayRaw(new_array_packed(0, fr->pinning));
        return;
    }

    nu = new_array_packed(force_level, fr->pinning);

    for (objnode *tmp = forcelist; tmp; tmp = tmp->next) {
        array_set_intkey_refval(&nu, count++, tmp->data);
    }

    PushArrayRaw(nu);
}

/**
 * Implementation of MUF WATCHPID
 *
 * Consumes a process ID, and schedules a PROC.EXIT event to be sent to the
 * current program when the process ends.  If the process does not exist,
 * the event is sent immediately.   Requires MUCKER level 3.
 *
 * @see muf_event_add
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_watchpid(PRIM_PROTOTYPE)
{
    struct frame *frame;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3) {
        abort_interp("Mucker level 3 required.");
    }

    /**
     * @TODO Combine these two checks.
     */
    if (oper1->type != PROG_INTEGER) {
        abort_interp("Process PID expected.");
    }

    if (oper1->data.number == fr->pid) {
        abort_interp("Narcissistic processes not allowed.");
    }

    frame = timequeue_pid_frame(oper1->data.number);

    if (frame) {
        struct mufwatchpidlist **cur;
        struct mufwatchpidlist *waitee;

        for (cur = &frame->waiters; *cur; cur = &(*cur)->next) {
            if ((*cur)->pid == oper1->data.number) {
                break;
            }
        }

        if (!*cur) {
            *cur = malloc(sizeof(**cur));

            if (!*cur) {
                abort_interp("Internal memory error.\n");
            }

            (*cur)->next = 0;
            (*cur)->pid = fr->pid;

            waitee = malloc(sizeof(*waitee));

            if (!waitee) {
                abort_interp("Internal memory error.\n");
            }

            waitee->next = fr->waitees;
            waitee->pid = oper1->data.number;
            fr->waitees = waitee;
        }
    } else {
        char buf[64];

        snprintf(buf, sizeof(buf), "PROC.EXIT.%d", oper1->data.number);
        muf_event_add(fr, buf, oper1, 0);
    }

    CLEAR(oper1);
}

/**
 * Implementation of MUF READ_WANTS_BLANKS
 *
 * Specifies that future READs in this frame can return a blank string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_read_wants_blanks(PRIM_PROTOTYPE)
{
    fr->wantsblanks = 1;
}

/**
 * Implementation of MUF READ_WANTS_NO_BLANKS
 *
 * Specifies that future READs in this frame cannot return a blank string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_read_wants_no_blanks(PRIM_PROTOTYPE)
{
    fr->wantsblanks = 0;
}

/**
 * Implementation of MUF DEBUGGER_BREAK
 *
 * Starts the MUF debugger.  There cannot be more than MAX_BREAKS breakpoints.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_debugger_break(PRIM_PROTOTYPE)
{
    int i = 0;

    if (fr->brkpt.count >= MAX_BREAKS)
        abort_interp("Too many breakpoints set.");

    fr->brkpt.force_debugging = 1;

    if (fr->brkpt.count != 1 ||
        fr->brkpt.temp[0] != 1 ||
        fr->brkpt.level[0] != -1 ||
        fr->brkpt.line[0] != -1 ||
        fr->brkpt.linecount[0] != -2 ||
        fr->brkpt.pc[0] != NULL ||
        fr->brkpt.pccount[0] != -2 || fr->brkpt.prog[0] != program) {
        /* No initial breakpoint.  Lets make one. */
        i = fr->brkpt.count++;
        fr->brkpt.temp[i] = 1;
        fr->brkpt.level[i] = -1;
        fr->brkpt.line[i] = -1;
        fr->brkpt.linecount[i] = -2;
        fr->brkpt.pc[i] = NULL;
        fr->brkpt.pccount[i] = 0;
        fr->brkpt.prog[i] = NOTHING;
    }
}

/**
 * Implementation of MUF IGNORING?
 *
 * Consumes two dbrefs, and returns a boolean that represents if the first
 * object's owner is ignoring the second's.  Requires MUCKER level 3.
 *
 * Returns 0 if the sysparm ignore_support is off.
 *
 * @see ignore_is_ignoring
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_ignoringp(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 3)
        abort_interp("Permission Denied.");

    if (!valid_object(oper1))
        abort_interp("Invalid object. (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid object. (1)");

    result = ignore_is_ignoring(oper2->data.objref, oper1->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

/**
 * Implementation of MUF IGNORE_ADD
 *
 * Consumes two dbrefs, and adds the second's owner to the first's owner's
 * ignore list.  Requires MUCKER level 3.
 *
 * Does not do anything if the sysparm ignore_support is off.
 *
 * @see ignore_add_player
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_ignore_add(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 3)
        abort_interp("Permission Denied.");

    if (!valid_object(oper1))
        abort_interp("Invalid object. (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid object. (1)");

    ignore_add_player(oper2->data.objref, oper1->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF IGNORE_DEL
 *
 * Consumes two dbrefs, and removes the second's owner to the first's owner's
 * ignore list.  Requires MUCKER level 3.
 *
 * Does not do anything if the sysparm ignore_support is off.
 *
 * @see ignore_removeplayer
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_ignore_del(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 3)
        abort_interp("Permission Denied.");

    if (!valid_object(oper1))
        abort_interp("Invalid object. (2)");

    if (!valid_object(oper2))
        abort_interp("Invalid object. (1)");

    ignore_remove_player(oper2->data.objref, oper1->data.objref);

    CLEAR(oper1);
    CLEAR(oper2);
}

/**
 * Implementation of MUF DEBUG_ON
 *
 * Sets the current program DEBUG, allowing the owner to see the stack trace.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_debug_on(PRIM_PROTOTYPE)
{
    FLAGS(program) |= DARK;
    DBDIRTY(program);
}

/**
 * Implementation of MUF DEBUG_OFF
 *
 * Sets the current program !DEBUG, turning off the stack trace.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_debug_off(PRIM_PROTOTYPE)
{
    FLAGS(program) &= ~DARK;
    DBDIRTY(program);
}

/**
 * Implementation of MUF DEBUG_LINE
 *
 * Shows the stack trace for the current line.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void
prim_debug_line(PRIM_PROTOTYPE)
{
    if (((FLAGS(program) & DARK) == 0) && controls(player, program)) {
        char *msg = debug_inst(fr, 0, pc, fr->pid, arg, buf, sizeof(buf),
                *top, program);
        notify_nolisten(player, msg, 1);
    }
}
