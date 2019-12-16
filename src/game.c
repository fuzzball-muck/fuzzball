/** @file game.c
 *
 * Implementation for game.c -- game.c is one of the "core" files that handles
 * most of the main game handling.  Stuff like handling the input from
 * users.  There's a lot of miscellaneous stuff here, so it is a little
 * hard to quantify in this little quip.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "commands.h"
#include "compile.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "edit.h"
#include "events.h"
#include "fbsignal.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "mpi.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

/**
 * Check to see if 'string' prefixes the 'command' variable.
 *
 * Uses 'goto' to jump to 'bad' if no match -- doesn't return anything.
 *
 * @see string_prefix
 *
 * @private
 * @param string the string to match against 'command'
 */
#define Matched(string) { if(!string_prefix((string), command)) goto bad; }

/**
 * @var a variable to keep track of the force level.  This is incremented
 *      when a 'force' command is used to avoid recursive forcing.
 */
int force_level = 0;

/**
 * @var A string variable that is basically a string delimited list of
 *      compile options formed by a series of ifdefs.  The string will
 *      have a trailing space.  Use the MOD_DEFINED(...) define to
 *      figure out if a given option is enabled.
 *
 *      @see MOD_DEFINED
 */
const char *compile_options =
#ifdef DEBUG
    "DEBUG "
#endif
#ifdef DISKBASE
    "DISKBASE "
#endif
#ifdef GOD_PRIV
    "GODPRIV "
#endif
#ifdef IP_FORWARDING
    "IPFWD "
#endif
#ifdef MALLOC_PROFILING
    "MEMPROF "
#endif
#ifdef MEMORY_CLEANUP
    "MEMCLEANUP "
#endif
#ifdef MCPGUI_SUPPORT
    "MCPGUI "
#endif
#ifdef SPAWN_HOST_RESOLVER
    "RESOLVER "
#endif
#ifdef HAVE_LIBSSL
    "SSL "
#endif
    "";

/**
 * @var the dump file path
 */
const char *dumpfile = 0;

/**
 * @private
 * @var a count of the number of times the DB is dumped
 */
static int epoch = 0;

/**
 * @var boolean value - if true, we are the dump child process.  This is
 *      set immediately after the fork and should be false for the parent
 *      (actual MUCK) process.
 */
int forked_dump_process_flag = 0;

/**
 * @var file handle for the input database file
 */
FILE *input_file;

/**
 * @private
 * @var the input database file path
 */
static char *in_filename = NULL;

/**
 * Implementation of the @dump command
 *
 * This does NOT do any permission checking, except for a GOD_PRIV check
 * if a newfile is provided.
 *
 * If newfile is provided, all future dumps will go to that new file
 *
 * @param player the player taking a dump
 * @param newfile if provided, will use newfile as the dump target.  Can be ""
 */
void
do_dump(dbref player, const char *newfile)
{
    char buf[BUFFER_LEN];

#ifndef DISKBASE
    if (global_dumper_pid != 0) {
        notify(player, "Sorry, there is already a dump currently in progress.");
        return;
    }
#endif

    /* My modifications are supposed to be just documentation only, but
     * the way this ifdef was structured was just too gross for me to
     * leave.
     */
#ifdef GOD_PRIV
    if (*newfile && God(player)) {
#else
    if (*newfile) {
#endif      /* GOD_PRIV */
        free((void *) dumpfile);
        dumpfile = alloc_string(newfile);
        snprintf(buf, sizeof(buf), "Dumping to file %s...", dumpfile);
    } else {
        snprintf(buf, sizeof(buf), "Dumping...");
    }

    notify(player, buf);
    dump_db_now();
}

/**
 * Implementation of @shutdown command
 *
 * Permissions are checked and non-wizard players are logged.  This will
 * trigger the MUCK to shut itself down  The implementation of this
 * is in interface.c
 *
 * @param player the player trying to do the restart
 */
void
do_shutdown(dbref player)
{
    char unparse_buf[BUFFER_LEN];
    unparse_object(player, player, unparse_buf, sizeof(unparse_buf));

    if (Wizard(player) && Typeof(player) == TYPE_PLAYER) {
        log_status("SHUTDOWN: by %s", unparse_buf);
        shutdown_flag = 1;
        restart_flag = 0;
    } else {
        notify(player, "Your delusions of grandeur have been duly noted.");
        log_status("ILLEGAL SHUTDOWN: tried by %s", unparse_buf);
    }
}

#ifdef USE_SSL
/**
 * Implementation of @reconfiguressl
 *
 * This is a simple wrapper around the reconfigure_ssl call.  This does
 * not do any permission checking.
 *
 * @see reconfigure_ssl
 *
 * @param player the player doing the call
 */
void
do_reconfigure_ssl(dbref player)
{
    if (reconfigure_ssl()) {
        notify(player, "Successfully reloaded SSL configuration.");
    } else {
        notify(player, "Failed to reconfigure SSL; check status log for info.");
    }
}
#endif

/**
 * Implementation of the @restart command
 *
 * Permissions are checked and non-wizard players are logged.  This will
 * trigger the MUCK to try and restart itself.  The implementation of this
 * is in interface.c
 *
 * @param player the player trying to do the restart
 */
void
do_restart(dbref player)
{
    char unparse_buf[BUFFER_LEN];
    unparse_object(player, player, unparse_buf, sizeof(unparse_buf));

    if (Wizard(player) && Typeof(player) == TYPE_PLAYER) {
        log_status("SHUTDOWN & RESTART: by %s", unparse_buf);
        shutdown_flag = 1;
        restart_flag = 1;
    } else {
        notify(player, "Your delusions of grandeur have been duly noted.");
        log_status("ILLEGAL RESTART: tried by %s", unparse_buf);
    }
}

/**
 * Common code path for handling database dumps.
 *
 * This is used by both the forked process and the non-forked process
 * for dumping (saving).  The dump file is the 'dumpfile' variable
 * suffixed with '.#X#' where X is the 'epoch' variable.  Epoch is
 * not set or incremented by this call, so it must be handled
 * by the caller.
 *
 * If the DB writes successfully, it will replace the 'dumpfile' with
 * the written epoch file, and the epoch file is removed.
 *
 * It also writes the macro file, using the same epoch suffix.  If
 * the macro file writes successfully, it replaces the original macro
 * file (MACRO_FILE)
 *
 * @private
 */
static void
dump_database_internal(void)
{
    char tmpfile[2048];
    FILE *f;

    snprintf(tmpfile, sizeof(tmpfile), "%s.#%d#", dumpfile, epoch - 1);
    (void) unlink(tmpfile); /* nuke our predecessor */

    snprintf(tmpfile, sizeof(tmpfile), "%s.#%d#", dumpfile, epoch);

    if ((f = fopen(tmpfile, "wb")) != NULL) {
        db_write(f);
        fclose(f);

#ifdef DISKBASE
        fclose(input_file);
#endif

#ifdef WIN32
        (void) unlink(dumpfile); /* Delete old file before rename */
#endif

        if (rename(tmpfile, dumpfile) < 0)
            perror(tmpfile);

#ifdef DISKBASE
            free(in_filename);
            in_filename = strdup(dumpfile);

            if ((input_file = fopen(in_filename, "rb")) == NULL)
                perror(dumpfile);
#endif
    } else {
        perror(tmpfile);
    }

    /* Write out the macros */

    snprintf(tmpfile, sizeof(tmpfile), "%s.#%d#", MACRO_FILE, epoch - 1);
    (void) unlink(tmpfile);

    snprintf(tmpfile, sizeof(tmpfile), "%s.#%d#", MACRO_FILE, epoch);

    if ((f = fopen(tmpfile, "wb")) != NULL) {
        macrodump(macrotop, f);
        fclose(f);

#ifdef WIN32
        unlink(MACRO_FILE);
#endif
        if (rename(tmpfile, MACRO_FILE) < 0)
            perror(tmpfile);
    } else {
        perror(tmpfile);
    }

    sync();

#ifdef DISKBASE
    /* Only show dumpdone mesg if not doing background saves. */
    if (tp_dbdump_warning && tp_dumpdone_warning)
        wall_and_flush(tp_dumpdone_mesg);

    propcache_hits = 0L;
    propcache_misses = 1L;
#endif
}

/**
 * Perform a database dump without forking.
 *
 * This is used in cases such as when shutting down the MUCK or whenever
 * you want everything to wait for the database to save.
 */
void
dump_database(void)
{
    epoch++;

    log_status("DUMPING: %s.#%d#", dumpfile, epoch);
    dump_database_internal();
    log_status("DUMPING: %s.#%d# (done)", dumpfile, epoch);
}

/**
 * Perform a dump operation, forking a new process if it is supported
 *
 * If DISKBASE is defined, or we are running windows, no process is
 * forked and the database is dumped "inline".
 *
 * Otherwise, a process is forked, its nice level is set to NICELEVEL
 * if defined, and the database is dumped to disk.
 */
void
fork_and_dump(void)
{
    epoch++;

#ifndef DISKBASE
    if (global_dumper_pid != 0) {
        wall_wizards("## Dump already in progress.  Skipping redundant scheduled dump.");
        return;
    }
#endif

    log_status("CHECKPOINTING: %s.#%d#", dumpfile, epoch);

    if (tp_dbdump_warning)
        wall_and_flush(tp_dumping_mesg);

    /*
     * Reviewer note: usually I do not modify code as part of the
     * documentation project, but this ifdef structure was such
     * a tangled mess I couldn't help but fix it. (tanabi)
     */
#if defined(DISKBASE) || defined(WIN32)
    dump_database_internal();
#else
    if ((global_dumper_pid = fork()) == 0) {
        /* We are the child. */
        forked_dump_process_flag = 1;

#  ifdef NICEVAL
        /* 
         * Requested by snout of SPR, reduce the priority of the
         * dumper child.
         */
        nice(NICEVAL);
#  endif /* NICEVAL */

        set_dumper_signals();
        dump_database_internal();
        _exit(0);
    }

    if (global_dumper_pid < 0) {
        global_dumper_pid = 0;
        wall_wizards("## Could not fork for database dumping.  Possibly out of memory.");
        wall_wizards("## Please restart the server when next convenient.");
    }
#endif
}

/**
 * Initialize the game
 *
 * This does a series of important stuff:
 *
 * - Loads the macro file
 * - Opens th input file
 * - Clears the DB in-memory structures (which initializes it)
 * - Initalize MUF primitives
 * - Initialize MPI
 * - Initialize random number generator
 * - Load DB
 * - Set the book-keeping ~sys properties on #0
 *
 * @param infile the path to the input database file
 * @param outfile the path to the output database file
 * @return returns 0 on success, -1 on any error condition
 */
int
init_game(const char *infile, const char *outfile)
{
    FILE *f;

    if ((f = fopen(MACRO_FILE, "rb")) == NULL)
        log_status("INIT: Macro storage file %s is tweaked.", MACRO_FILE);
    else {
        macroload(f);
        fclose(f);
    }

    in_filename = strdup(infile);

    if ((input_file = fopen(infile, "rb")) == NULL)
        return -1;

    db_free();
    init_primitives(); /* init muf compiler */
    mesg_init(); /* init mpi interpreter */
    SRANDOM((unsigned int)getpid()); /* init random number generator */

    /* ok, read the db in */
    log_status("LOADING: %s", infile);
    fprintf(stderr, "LOADING: %s\n", infile);

    if (db_read(input_file) < 0)
        return -1;

    log_status("LOADING: %s (done)", infile);
    fprintf(stderr, "LOADING: %s (done)\n", infile);

    /* set up dumper */
    free((void *) dumpfile);
    dumpfile = alloc_string(outfile);

    if (!db_conversion_flag) {
        add_property((dbref) 0, SYS_STARTUPTIME_PROP, NULL, (int) time((time_t *) NULL));
        add_property((dbref) 0, SYS_MAXPENNIES_PROP, NULL, tp_max_pennies);
        add_property((dbref) 0, SYS_DUMPINTERVAL_PROP, NULL, tp_dump_interval);
        add_property((dbref) 0, SYS_MAX_CONNECTS_PROP, NULL, 0);
    }

    return 0;
}

#ifdef MEMORY_CLEANUP
/**
 * Clean up certain global strings allocated by game.c
 *
 * This is only available if MEMORY_CLEANUP is defined.
 */
void
cleanup_game()
{
    free((void *) dumpfile);
    free(in_filename);
}
#endif

/**
 * Implementation of @restrict command
 *
 * This does not do any permission checking.  Arg can be either "on" to
 * turn on wiz-only mode, "off" to turn it off, or anything else (such
 * as "" or a typo) to display current restriction status.
 *
 * @param player the player doing the call
 * @param arg the argument as described above
 */
void
do_restrict(dbref player, const char *arg)
{
    if (!strcmp(arg, "on")) {
        wizonly_mode = 1;
        notify(player, "Login access is now restricted to wizards only.");
    } else if (!strcmp(arg, "off")) {
        wizonly_mode = 0;
        notify(player, "Login access is now unrestricted.");
    } else {
        notifyf(player, "Restricted connection mode is currently %s.",
                wizonly_mode ? "on" : "off");
    }
}

/**
 * Process command input from a given user
 *
 * Does all the processing of a given command and directs it to
 * the appropriate target.  This target could be an action/exit, or
 * it could be a command, or if the player is in interactive mode
 * it could be sent to whatever the player is engaging with.
 *
 * @param descr the descriptor of the player
 * @param player the player running the command
 * @param command the command input from the player
 */
void
process_command(int descr, dbref player, const char *command)
{
    char *arg1;
    char *arg2;
    char *full_command;
    char pbuf[BUFFER_LEN];
    char xbuf[BUFFER_LEN];
    char ybuf[BUFFER_LEN];
    struct timeval starttime;
    struct timeval endtime;
    double totaltime;

    if (command == 0)
        abort();

    /* robustify player */
    if (!ObjExists(player) ||
        (Typeof(player) != TYPE_PLAYER && Typeof(player) != TYPE_THING)) {
        log_status("process_command: bad player %d", player);
        return;
    }

    if ((tp_log_commands || Wizard(OWNER(player)))) {
        char *log_name = whowhere(player);
        char log_cmd[BUFFER_LEN];

        strcpy(log_cmd, command); /* OK */

        if (string_prefix(log_cmd, "@password")) {
            snprintf(log_cmd, 16, "%.9s [***]", command);
        } else if (string_prefix(log_cmd, "@newpassword")) {
            snprintf(log_cmd, 19, "%.12s [***]", command);
        } else if (string_prefix(log_cmd, "@pcreate")) {
            char *to_mask = strchr(log_cmd, ARG_DELIMITER);

            if (to_mask) {
                strcpyn(to_mask+1, 6, "[***]");
            }
        }

        if (!(FLAGS(player) & (INTERACTIVE | READMODE))) {
            if (!*command) {
                free(log_name);
                return;
            }

            log_command("%s: %s", log_name, log_cmd);
        } else {
            if (tp_log_interactive) {
                log_command("%s: %s%s", log_name,
                            (FLAGS(player) & (READMODE)) ? "[READ] " : "[INTERP] ", log_cmd);
            }
        }

        free(log_name);
    }

    if (FLAGS(player) & INTERACTIVE) {
        interactive(descr, player, command);
        return;
    }

    skip_whitespace(&command);

    /* Disable null command once past READ line */
    if (!*command)
        return;

    /* check for single-character commands */
    if (!tp_enable_prefix) {
        if (*command == SAY_TOKEN) {
            snprintf(pbuf, sizeof(pbuf), "say %s", command + 1);
            command = &pbuf[0];
        } else if (*command == POSE_TOKEN) {
            snprintf(pbuf, sizeof(pbuf), "pose %s", command + 1);
            command = &pbuf[0];
        } else if (*command == EXIT_DELIMITER) {
            snprintf(pbuf, sizeof(pbuf), "delimiter %s", command + 1);
            command = &pbuf[0];
        }
    }

    /* profile how long command takes. */
    gettimeofday(&starttime, NULL);

    /* if player is a wizard, and uses override token to start line... */
    /* ... then do NOT run actions, but run the command they specify. */
    if (!(TrueWizard(OWNER(player)) && (*command == OVERRIDE_TOKEN))) {
        if (can_move(descr, player, command, 0)) {
            /* command is exact match for exit */
            do_move(descr, player, command, 0);
            *match_args = 0;
            *match_cmdname = 0;
        } else {
            if (tp_enable_prefix) {
                if (*command == SAY_TOKEN) {
                    snprintf(pbuf, sizeof(pbuf), "say %s", command + 1);
                    command = &pbuf[0];
                } else if (*command == POSE_TOKEN) {
                    snprintf(pbuf, sizeof(pbuf), "pose %s", command + 1);
                    command = &pbuf[0];
                } else if (*command == EXIT_DELIMITER) {
                    snprintf(pbuf, sizeof(pbuf), "delimiter %s", command + 1);
                    command = &pbuf[0];
                } else {
                    goto bad_pre_command;
                }

                if (can_move(descr, player, command, 0)) {
                    /* command is exact match for exit */
                    do_move(descr, player, command, 0);
                    *match_args = 0;
                    *match_cmdname = 0;
                } else {
                    goto bad_pre_command;
                }
            } else {
                goto bad_pre_command;
            }
        }
    } else {
        bad_pre_command:

        if (TrueWizard(OWNER(player)) && (*command == OVERRIDE_TOKEN))
            command++;
        else if (tp_cmd_only_overrides)
            goto bad;

        full_command = strcpyn(xbuf, sizeof(xbuf), command);

        for (; *full_command && !isspace(*full_command); full_command++) ;

        if (*full_command)
            full_command++;

        /* find arg1 -- move over command word */
        command = strcpyn(ybuf, sizeof(ybuf), command);

        for (arg1 = (char *)command; *arg1 && !isspace(*arg1); arg1++) ;

        /* truncate command */
        if (*arg1)
            *arg1++ = '\0';

        /* remember command for programs */
        strcpyn(match_args, sizeof(match_args), full_command);
        strcpyn(match_cmdname, sizeof(match_cmdname), command);

        skip_whitespace((const char **)&arg1);

        /* find end of arg1, start of arg2 */
        for (arg2 = arg1; *arg2 && *arg2 != ARG_DELIMITER; arg2++) ;

        if (*arg2)
            *arg2++ = '\0';

        char *p = arg1;
        remove_ending_whitespace(&p);

        skip_whitespace((const char **)&arg2);

        switch (command[0]) {
            case '@':
                switch (command[1]) {
                    case 'a':
                    case 'A':
                        /* @action, @armageddon, @attach */
                        switch (command[2]) {
                            case 'c':
                            case 'C':
                                Matched("@action");
                                NOGUEST("@action", player);
                                BUILDERONLY("@action", player);
                                do_action(descr, player, arg1, arg2);
                                break;

                            case 'r':
                            case 'R':
                                if (strcmp(command, "@armageddon"))
                                    goto bad;

                                do_armageddon(player, full_command);
                                break;

                            case 't':
                            case 'T':
                                Matched("@attach");
                                NOGUEST("@attach", player);
                                BUILDERONLY("@attach", player);
                                do_attach(descr, player, arg1, arg2);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'b':
                    case 'B':
                        /* @bless, @boot */
                        switch (command[2]) {
                            case 'l':
                            case 'L':
                                Matched("@bless");
                                WIZARDONLY("@bless", player);
                                PLAYERONLY("@bless", player);
                                NOFORCE("@bless", player);
                                do_bless(descr, player, arg1, arg2);
                                break;

                            case 'o':
                            case 'O':
                                Matched("@boot");
                                WIZARDONLY("@boot", player);
                                PLAYERONLY("@boot", player);
                                do_boot(player, arg1);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'c':
                    case 'C':
                        /*
                         * @chlock, @chown, @chown_lock, @clone,
                         * @conlock, @contents, @create, @credits
                         */
                        switch (command[2]) {
                            case 'h':
                            case 'H':
                                switch (command[3]) {
                                    case 'l':
                                    case 'L':
                                        Matched("@chlock");
                                        NOGUEST("@chlock", player);
                                        set_standard_lock(descr, player, arg1,
                                                          MESGPROP_CHLOCK,
                                                          "Chown Lock", arg2);
                                        break;
                                    case 'o':
                                    case 'O':
                                        if (strlen(command) < 7) {
                                            Matched("@chown");
                                            do_chown(descr, player, arg1, arg2);
                                        } else {
                                            Matched("@chown_lock");
                                            NOGUEST("@chown_lock", player);
                                            set_standard_lock(descr, player,
                                                              arg1,
                                                              MESGPROP_CHLOCK,
                                                              "Chown Lock",
                                                              arg2);
                                        }
                                        break;

                                    default:
                                        goto bad;
                                }

                                break;

                            case 'l':
                            case 'L':
                                Matched("@clone");
                                NOGUEST("@clone", player);
                                BUILDERONLY("@clone", player);
                                do_clone(descr, player, arg1, arg2);
                                break;

                            case 'o':
                            case 'O':
                                switch (command[4]) {
                                    case 'l':
                                    case 'L':
                                        Matched("@conlock");
                                        NOGUEST("@conlock", player);
                                        set_standard_lock(descr, player, arg1,
                                                          MESGPROP_CONLOCK,
                                                          "Container Lock",
                                                          arg2);
                                        break;

                                    case 't':
                                    case 'T':
                                        Matched("@contents");
                                        NOGUEST("@contents", player);
                                        BUILDERONLY("@contents", player);
                                        do_contents(descr, player, arg1, arg2);
                                        break;

                                    default:
                                        goto bad;
                                }

                                break;

                            case 'r':
                            case 'R':
                                if (strcasecmp(command, "@credits")) {
                                    Matched("@create");
                                    NOGUEST("@create", player);
                                    BUILDERONLY("@create", player);
                                    do_create(player, arg1, arg2);
                                } else {
                                    do_credits(player);
                                }

                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'd':
                    case 'D':
                        /* @debug, @describe, @dig, @doing, @drop, @dump */
                        switch (command[2]) {
                            case 'e':
                            case 'E':
                                if (strcasecmp(command, "@debug")) {
                                    Matched("@describe");
                                    NOGUEST("@describe", player);
                                    set_standard_property(descr, player, arg1,
                                                          MESGPROP_DESC,
                                                          "Object Description",
                                                          arg2);
                                    break;
                                } else {
                                    Matched("@debug");
                                    WIZARDONLY("@debug", player);
                                    do_debug(player, arg1);
                                    break;
                                }

                            case 'i':
                            case 'I':
                                Matched("@dig");
                                NOGUEST("@dig", player);
                                BUILDERONLY("@dig", player);
                                do_dig(descr, player, arg1, arg2);
                                break;

                            case 'o':
                            case 'O':
                                Matched("@doing");
                                NOGUEST("@doing", player);
                                do_doing(descr, player, arg1, arg2);
                                break;

                            case 'r':
                            case 'R':
                                Matched("@drop");
                                NOGUEST("@drop", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_DROP,
                                                      "Drop Message",
                                                      arg2);
                                break;

                            case 'u':
                            case 'U':
                                Matched("@dump");
                                WIZARDONLY("@dump", player);
                                PLAYERONLY("@dump", player);
                                do_dump(player, full_command);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'e':
                    case 'E':
                        /* @edit, @entrances, @examine */
                        switch (command[2]) {
                            case 'd':
                            case 'D':
                                Matched("@edit");
                                NOGUEST("@edit", player);
                                PLAYERONLY("@edit", player);
                                MUCKERONLY("@edit", player);
                                do_edit(descr, player, arg1);
                                break;

                            case 'n':
                            case 'N':
                                Matched("@entrances");
                                NOGUEST("@entrances", player);
                                BUILDERONLY("@entrances", player);
                                do_entrances(descr, player, arg1, arg2);
                                break;

                            case 'x':
                            case 'X':
                                Matched("@examine");
                                GODONLY("@examine", player);
                                do_examine_sanity(descr, player, arg1);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'f':
                    case 'F':
                        /* @fail, @find, @flock, @force, @force_lock */
                        switch (command[2]) {
                            case 'a':
                            case 'A':
                                Matched("@fail");
                                NOGUEST("@fail", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_FAIL,
                                                      "Fail Message", arg2);
                                break;

                            case 'i':
                            case 'I':
                                Matched("@find");
                                NOGUEST("@find", player);
                                BUILDERONLY("@find", player);
                                do_find(player, arg1, arg2);
                                break;

                            case 'l':
                            case 'L':
                                Matched("@flock");
                                NOGUEST("@flock", player);
                                NOFORCE("@flock", player);
                                set_standard_lock(descr, player, arg1,
                                                  MESGPROP_FLOCK,
                                                  "Force Lock", arg2);
                                break;
                            case 'o':
                            case 'O':
                                if (strlen(command) < 7) {
                                    Matched("@force");
                                    do_force(descr, player, arg1, arg2);
                                } else {
                                    Matched("@force_lock");
                                    NOGUEST("@force_lock", player);
                                    NOFORCE("@force_lock", player);
                                    set_standard_lock(descr, player, arg1,
                                                      MESGPROP_FLOCK,
                                                      "Force Lock",
                                                      arg2);
                                }

                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'i':
                    case 'I':
                        /* @idescribe */
                        Matched("@idescribe");
                        NOGUEST("@idescribe", player);
                        set_standard_property(descr, player, arg1,
                                              MESGPROP_IDESC,
                                              "Inside Description", arg2);
                        break;

                    case 'k':
                    case 'K':
                        /* @kill */
                        Matched("@kill");
                        NOGUEST("@kill", player);
                        BUILDERONLY("@kill", player);
                        do_kill_process(descr, player, arg1);
                        break;

                    case 'l':
                    case 'L':
                        /* @link, @linklock, @list, @lock */
                        switch (command[2]) {
                            case 'i':
                            case 'I':
                                switch (command[3]) {
                                    case 'n':
                                    case 'N':
                                        if (strlen(command) < 7) {
                                            Matched("@link");
                                            NOGUEST("@link", player);
                                            do_link(descr, player, arg1, arg2);
                                        } else {
                                            Matched("@linklock");
                                            NOGUEST("@linklock", player);
                                            set_standard_lock(descr, player,
                                                              arg1,
                                                              MESGPROP_LINKLOCK,
                                                              "Link Lock",
                                                              arg2);
                                        }

                                        break;

                                    case 's':
                                    case 'S':
                                        NOGUEST("@list", player);
                                        PLAYERONLY("@list", player);
                                        Matched("@list");
                                        do_list(descr, player, arg1, arg2);
                                        break;

                                    default:
                                        goto bad;
                                }

                                break;

                            case 'o':
                            case 'O':
                                Matched("@lock");
                                NOGUEST("@lock", player);
                                set_standard_lock(descr, player, arg1,
                                                  MESGPROP_LOCK, "Lock", arg2);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'm':
                    case 'M':
                        /* @mcpedit, @mcpprogram, @memory, @mpitops, @muftops */
                        switch (command[2]) {
                            case 'c':
                            case 'C':
                                if (string_prefix("@mcpedit", command)) {
                                    Matched("@mcpedit");
                                    NOGUEST("@mcpedit", player);
                                    PLAYERONLY("@mcpedit", player);
                                    MUCKERONLY("@mcpedit", player);
                                    do_mcpedit(descr, player, arg1);
                                    break;
                                } else {
                                    Matched("@mcpprogram");
                                    NOGUEST("@mcpprogram", player);
                                    PLAYERONLY("@mcpprogram", player);
                                    MUCKERONLY("@mcpprogram", player);
                                    do_mcpprogram(descr, player, arg1, arg2);
                                    break;
                                }
#ifndef NO_MEMORY_COMMAND
                            case 'e':
                            case 'E':
                                Matched("@memory");
                                WIZARDONLY("@memory", player);
                                do_memory(player);
                                break;
#endif
                            case 'p':
                            case 'P':
                                Matched("@mpitops");
                                WIZARDONLY("@mpitops", player);
                                do_mpi_topprofs(player, arg1);
                                break;

                            case 'u':
                            case 'U':
                                Matched("@muftops");
                                WIZARDONLY("@muftops", player);
                                do_muf_topprofs(player, arg1);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'n':
                    case 'N':
                        /* @name, @newpassword */
                        switch (command[2]) {
                            case 'a':
                            case 'A':
                                Matched("@name");
                                NOGUEST("@name", player);
                                do_name(descr, player, arg1, arg2);
                                break;

                            case 'e':
                            case 'E':
                                if (strcmp(command, "@newpassword"))
                                    goto bad;

                                WIZARDONLY("@newpassword", player);
                                PLAYERONLY("@newpassword", player);
                                do_newpassword(player, arg1, arg2);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'o':
                    case 'O':
                        /*
                         * @odrop, @oecho, @ofail, @open, @osuccess,
                         * @owned, @ownlock
                         */
                        switch (command[2]) {
                            case 'd':
                            case 'D':
                                Matched("@odrop");
                                NOGUEST("@odrop", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_ODROP,
                                                      "ODrop Message", arg2);
                                break;

                            case 'e':
                            case 'E':
                                Matched("@oecho");
                                NOGUEST("@oecho", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_OECHO,
                                                      "Outside-echo Prefix",
                                                      arg2);
                                break;

                            case 'f':
                            case 'F':
                                Matched("@ofail");
                                NOGUEST("@ofail", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_OFAIL,
                                                      "OFail Message",
                                                      arg2);
                                break;

                            case 'p':
                            case 'P':
                                Matched("@open");
                                NOGUEST("@open", player);
                                BUILDERONLY("@open", player);
                                do_open(descr, player, arg1, arg2);
                                break;

                            case 's':
                            case 'S':
                                Matched("@osuccess");
                                NOGUEST("@osuccess", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_OSUCC,
                                                      "OSuccess Message", arg2);
                                break;

                            case 'w':
                            case 'W':
                                if (string_prefix("@owned", command)) {
                                    Matched("@owned");
                                    NOGUEST("@owned", player);
                                    BUILDERONLY("@owned", player);
                                    do_owned(player, arg1, arg2);
                                    break;
                                } else {
                                    Matched("@ownlock");
                                    NOGUEST("@ownlock", player);
                                    NOFORCE("@ownlock", player);
                                    set_standard_lock(descr, player, arg1,
                                                      MESGPROP_OWNLOCK,
                                                      "Ownership Lock", arg2);
                                    break;
                                }

                            default:
                                goto bad;
                        }

                        break;

                    case 'p':
                    case 'P':
                        /*
                         * @password, @pcreate, @pecho, @program, @propset,
                         * @ps
                         */
                        switch (command[2]) {
                            case 'a':
                            case 'A':
                                Matched("@password");
                                PLAYERONLY("@password", player);
                                NOGUEST("@password", player);
                                do_password(player, arg1, arg2);
                                break;

                            case 'c':
                            case 'C':
                                Matched("@pcreate");
                                WIZARDONLY("@pcreate", player);
                                PLAYERONLY("@pcreate", player);
                                do_pcreate(player, arg1, arg2);
                                break;

                            case 'e':
                            case 'E':
                                Matched("@pecho");
                                NOGUEST("@pecho", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_PECHO,
                                                      "Puppet-echo Prefix",
                                                      arg2);
                                break;

                            case 'r':
                            case 'R':
                                if (string_prefix("@program", command)) {
                                    Matched("@program");
                                    NOGUEST("@program", player);
                                    PLAYERONLY("@program", player);
                                    MUCKERONLY("@program", player);
                                    do_program(descr, player, arg1, arg2);
                                    break;
                                } else {
                                    Matched("@propset");
                                    NOGUEST("@propset", player);
                                    do_propset(descr, player, arg1, arg2);
                                    break;
                                }

                            case 's':
                            case 'S':
                                Matched("@ps");
                                NOGUEST("@ps", player);
                                BUILDERONLY("@ps", player);
                                do_process_status(player);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'r':
                    case 'R':
                        /*
                         * @readlock, @recycle, @reconfiguressl, @register,
                         * @relink, @restart, @restrict
                         */
                        switch (command[3]) {
                            case 'a':
                            case 'A':
                                Matched("@readlock");
                                NOGUEST("@readlock", player);
                                NOFORCE("@readlock", player);
                                set_standard_lock(descr, player, arg1,
                                                  MESGPROP_READLOCK,
                                                  "Read Lock", arg2);
                                break;

                            case 'c':
                            case 'C':
#ifdef USE_SSL
                                if (!strcmp(command, "@reconfiguressl")) {
                                    WIZARDONLY("@reconfiguressl", player);
                                    PLAYERONLY("@reconfiguressl", player);
                                    do_reconfigure_ssl(player);
                                    break;
                                }
#endif
                                Matched("@recycle");
                                NOGUEST("@recycle", player);
                                do_recycle(descr, player, arg1);
                                break;

                            case 'g':
                            case 'G':
                                Matched("@register");
                                NOGUEST("@register", player);
                                do_register(descr, player, arg1, arg2);
                                break;

                            case 'l':
                            case 'L':
                                Matched("@relink");
                                NOGUEST("@relink", player);
                                do_relink(descr, player, arg1, arg2);
                                break;

                            case 's':
                            case 'S':
                                if (!strcmp(command, "@restart")) {
                                    do_restart(player);
                                } else if (!strcmp(command, "@restrict")) {
                                    WIZARDONLY("@restrict", player);
                                    PLAYERONLY("@restrict", player);
                                    do_restrict(player, arg1);
                                } else {
                                    goto bad;
                                }

                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 's':
                    case 'S':
                        /*
                         * @sanity, @sanchange, @sanfix, @set, @shutdown,
                         * @stats, @success, @sweep
                         */
                        switch (command[2]) {
                            case 'a':
                            case 'A':
                                if (!strcmp(command, "@sanity")) {
                                    GODONLY("@sanity", player);
                                    do_sanity(player);
                                } else if (!strcmp(command, "@sanchange")) {
                                    GODONLY("@sanchange", player);
                                    NOFORCE("@sanchange", player);
                                    do_sanchange(player, full_command);
                                } else if (!strcmp(command, "@sanfix")) {
                                    GODONLY("@sanfix", player);
                                    NOFORCE("@sanfix", player);
                                    do_sanfix(player);
                                } else {
                                    goto bad;
                                }

                                break;

                            case 'e':
                            case 'E':
                                Matched("@set");
                                NOGUEST("@set", player);
                                do_set(descr, player, arg1, arg2);
                                break;

                            case 'h':
                            case 'H':
                                if (strcmp(command, "@shutdown"))
                                    goto bad;

                                do_shutdown(player);
                                break;

                            case 't':
                            case 'T':
                                Matched("@stats");
                                do_stats(player, arg1);
                                break;

                            case 'u':
                            case 'U':
                                Matched("@success");
                                NOGUEST("@success", player);
                                set_standard_property(descr, player, arg1,
                                                      MESGPROP_SUCC,
                                                      "Success Message", arg2);
                                break;

                            case 'w':
                            case 'W':
                                Matched("@sweep");
                                NOGUEST("@sweep", player);
                                BUILDERONLY("@sweep", player);
                                do_sweep(descr, player, arg1);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 't':
                    case 'T':
                        /* @teledump, @teleport, @toad, @trace, @tune */
                        switch (command[2]) {
                            case 'e':
                            case 'E':
                                /* Teledump is exact match only */
                                if (!strcmp(command, "@teledump")) {
                                    WIZARDONLY("@teledump", player);
                                    PLAYERONLY("@teledump", player);
                                    NOFORCE("@teledump", player);
                                    do_teledump(descr, player);
                                } else {
                                    NOGUEST("@teleport", player);
                                    Matched("@teleport");
                                    do_teleport(descr, player, arg1, arg2);
                                }

                                break;

                            case 'o':
                            case 'O':
                                if (!strcmp(command, "@toad")) {
                                    WIZARDONLY("@toad", player);
                                    PLAYERONLY("@toad", player);
                                    NOFORCE("@toad", player);
                                    do_toad(descr, player, arg1, arg2);
                                } else if (!strcmp(command, "@tops")) {
                                    WIZARDONLY("@tops", player);
                                    do_topprofs(player, arg1);
                                } else {
                                    goto bad;
                                }

                                break;

                            case 'r':
                            case 'R':
                                Matched("@trace");
                                NOGUEST("@trace", player);
                                BUILDERONLY("@trace", player);
                                do_trace(descr, player, arg1, atoi(arg2));
                                break;

                            case 'u':
                            case 'U':
                                Matched("@tune");
                                WIZARDONLY("@tune", player);
                                PLAYERONLY("@tune", player);
                                do_tune(player, arg1, arg2);
                                break;

                            default:
                                goto bad;
                        }

                        break;

                    case 'u':
                    case 'U':
                        /* @unbless, @unlink, @unlock, @uncompile, @usage */
                        switch (command[2]) {
                            case 'N':
                            case 'n':
                                if (string_prefix(command, "@unb")) {
                                    Matched("@unbless");
                                    WIZARDONLY("@unbless", player);
                                    PLAYERONLY("@unbless", player);
                                    NOFORCE("@unbless", player);
                                    do_unbless(descr, player, arg1, arg2);
                                } else if (string_prefix(command, "@unli")) {
                                    Matched("@unlink");
                                    NOGUEST("@unlink", player);
                                    do_unlink(descr, player, arg1);
                                } else if (string_prefix(command, "@unlo")) {
                                    Matched("@unlock");
                                    NOGUEST("@unlock", player);
                                    do_unlock(descr, player, arg1);
                                } else if (string_prefix(command, "@uncom")) {
                                    Matched("@uncompile");
                                    WIZARDONLY("@uncompile", player);
                                    PLAYERONLY("@uncompile", player);
                                    do_uncompile(player);
                                } else {
                                    goto bad;
                                }

                                break;

#ifndef NO_USAGE_COMMAND
                            case 'S':
                            case 's':
                                Matched("@usage");
                                WIZARDONLY("@usage", player);
                                do_usage(player);
                                break;
#endif
                            default:
                                goto bad;
                        }

                        break;

                    case 'v':
                    case 'V':
                        /* @version */
                        Matched("@version");
                        do_version(player);
                        break;

                    case 'w':
                    case 'W':
                        /* @wall */
                        if (strcmp(command, "@wall"))
                            goto bad;

                        WIZARDONLY("@wall", player);
                        PLAYERONLY("@wall", player);
                        do_wall(player, full_command);
                        break;

                    default:
                        goto bad;
                }

                break;

            case 'd':
            case 'D':
                /* disembark, drop */
                switch (command[1]) {
                    case 'i':
                    case 'I':
                        Matched("disembark");
                        do_leave(descr, player);
                        break;

                    case 'r':
                    case 'R':
                        Matched("drop");
                        do_drop(descr, player, arg1, arg2);
                        break;

                    default:
                        goto bad;
                }

                break;
            case 'e':
            case 'E':
                /* examine */
                Matched("examine");
                do_examine(descr, player, arg1, arg2);
                break;

            case 'g':
            case 'G':
                /* get, give, goto, gripe */
                switch (command[1]) {
                    case 'e':
                    case 'E':
                        Matched("get");
                        do_get(descr, player, arg1, arg2);
                        break;
                    case 'i':
                    case 'I':
                        Matched("give");
                        do_give(descr, player, arg1, atoi(arg2));
                        break;

                    case 'o':
                    case 'O':
                        Matched("goto");
                        do_move(descr, player, arg1, 0);
                        break;

                    case 'r':
                    case 'R':
                        if (strcasecmp(command, "gripe"))
                            goto bad;

                        do_gripe(player, full_command);
                        break;

                    default:
                        goto bad;
                }

                break;
            case 'h':
            case 'H':
                /* hand, help */
                if (!strcasecmp(command, "hand")) {
                    do_drop(descr, player, arg1, arg2);
                } else {
                    Matched("help");
                    do_helpfile(player, tp_file_help_dir, tp_file_help, arg1,
                                arg2);
                }

                break;

            case 'i':
            case 'I':
                /* inventory, info */
                if (strcasecmp(command, "info")) {
                    Matched("inventory");
                    do_inventory(player);
                } else {
                    Matched("info");
                    do_info(player, arg1, arg2);
                }

                break;

            case 'l':
            case 'L':
                /* leave, look */
                if (string_prefix("look", command)) {
                    Matched("look");
                    do_look_at(descr, player, arg1, arg2);
                    break;
                } else {
                    Matched("leave");
                    do_leave(descr, player);
                    break;
                }

            case 'm':
            case 'M':
                /* man, motd, move, mpi */
                if (string_prefix(command, "move")) {
                    do_move(descr, player, arg1, 0);
                    break;
                } else if (!strcasecmp(command, "motd")) {
                    do_motd(player, full_command);
                    break;
                } else if (!strcasecmp(command, "mpi")) {
                    do_helpfile(player, tp_file_mpihelp_dir, tp_file_mpihelp,
                                arg1, arg2);
                    break;
                } else {
                    if (strcasecmp(command, "man"))
                        goto bad;

                    do_helpfile(player, tp_file_man_dir, tp_file_man, arg1,
                                arg2);
                }

                break;

            case 'n':
            case 'N':
                /* news */
                Matched("news");
                do_helpfile(player, tp_file_news_dir, tp_file_news, arg1, arg2);
                break;

            case 'p':
            case 'P':
                /* page, pose, put */
                switch (command[1]) {
                    case 'a':
                    case 'A':
                        Matched("page");
                        do_page(player, arg1, arg2);
                        break;

                    case 'o':
                    case 'O':
                        Matched("pose");
                        do_pose(player, full_command);
                        break;

                    case 'u':
                    case 'U':
                        Matched("put");
                        do_drop(descr, player, arg1, arg2);
                        break;

                    default:
                        goto bad;
                }

                break;
            case 'r':
            case 'R':
                /* read, rob */
                switch (command[1]) {
                    case 'e':
                    case 'E':
                        Matched("read"); /* undocumented alias for look */
                        do_look_at(descr, player, arg1, arg2);
                        break;
                    default:
                        goto bad;
                }

                break;

            case 's':
            case 'S':
                /* say, score */
                switch (command[1]) {
                    case 'a':
                    case 'A':
                        Matched("say");
                        do_say(player, full_command);
                        break;

                    case 'c':
                    case 'C':
                        Matched("score");
                        do_score(player);
                        break;

                    default:
                    goto bad;
                }

                break;

            case 't':
            case 'T':
                /* take, throw */
                switch (command[1]) {
                    case 'a':
                    case 'A':
                        Matched("take");
                        do_get(descr, player, arg1, arg2);
                        break;

                    case 'h':
                    case 'H':
                        Matched("throw");
                        do_drop(descr, player, arg1, arg2);
                        break;

                    default:
                        goto bad;
                }

                break;

            case 'u':
            case 'U':
                Matched("uptime");
                do_uptime(player);
                break;

            case 'w':
            case 'W':
                /* whisper */
                Matched("whisper");
                do_whisper(descr, player, arg1, arg2);
                break;

            default:
                bad:

                if (tp_m3_huh != 0) {
                    char hbuf[BUFFER_LEN];
                    snprintf(hbuf, BUFFER_LEN, "HUH? %s", command);

                    if (can_move(descr, player, hbuf, 3)) {
                        do_move(descr, player, hbuf, 3);
                        *match_args = 0;
                        *match_cmdname = 0;
                        break;
                    }
                }

                notify(player, tp_huh_mesg);

                if (tp_log_failed_commands &&
                    !controls(player, LOCATION(player))) {
                    log_status("HUH from %s(%d) in %s(%d)[%s]: %s %s",
                               NAME(player), player, NAME(LOCATION(player)),
                               LOCATION(player),
                               NAME(OWNER(LOCATION(player))), command,
                               full_command);
                }

                break;
        }
    }

    /* calculate time command took. */
    gettimeofday(&endtime, NULL);

    if (starttime.tv_usec > endtime.tv_usec) {
        endtime.tv_usec += 1000000;
        endtime.tv_sec -= 1;
    }

    endtime.tv_usec -= starttime.tv_usec;
    endtime.tv_sec -= starttime.tv_sec;

    totaltime = endtime.tv_sec + (endtime.tv_usec * 1.0e-6);

    if (totaltime > (tp_cmd_log_threshold_msec / 1000.0)) {
        char tbuf[24];
        time_t st = (time_t)starttime.tv_sec;
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", localtime(&st));
        char *log_name = whowhere(player);
        log2file(tp_file_log_cmd_times, "%s: (%.3f) %s: %s",
                 tbuf, totaltime, log_name, command);
        free(log_name);
    }
}

#undef Matched
