/** @file fbsignal.c
 *
 * Definitions for Fuzzball's signal handling system.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#include "autoconf.h"
#include "fbsignal.h"
#include "db.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "tune.h"

#ifndef WIN32

/*
 * Function prototypes
 *
 * @TODO remove set_signals(void) because it is declared in fbsignal.h
 *       I believe the rest of these are needed because they are signal
 *       callbacks?  Normally you don't need prototypes for local functions.
 *       That's probably why these aren't static as well.
 *
 * See definitions for doc blocks.
 */
void set_signals(void);
void bailout(int);
void sig_dump_status(int i);
void sig_shutdown(int i);
void sig_reconfigure(int i);
#ifdef SIGEMERG
void sig_emerg(int i);
#endif

#ifdef SPAWN_HOST_RESOLVER
void sig_reap(int i);
#endif

#ifdef HAVE_PSELECT
/**
 * @private
 * @var boolean true if we have set the pselect_signal_mask, starts as false.
 */
static int set_pselect_signal_mask = 0;

/**
 * @var the signal mask for pselect - this is set by set_signals
 */
sigset_t pselect_signal_mask;
#endif

/**
 * Set signals for the forked dump processes
 *
 * These signals are not necessarily the same as the signals for the main
 * program, but are tailored towards the needs of the dump process.
 */
void
set_dumper_signals(void)
{
    signal(SIGPIPE, SIG_IGN);   /* Ignore Blocked Pipe */
    signal(SIGHUP, SIG_IGN);    /* Ignore Terminal Hangup */
    signal(SIGCHLD, SIG_IGN);   /* Ignore Child termination */
    signal(SIGFPE, SIG_IGN);    /* Ignore FP exceptions */
    signal(SIGUSR1, SIG_IGN);   /* Ignore SIGUSR1 */
    signal(SIGUSR2, SIG_IGN);   /* Ignore SIGUSR2 */
    signal(SIGINT, SIG_DFL);    /* Take Interrupt signal and die! */
    signal(SIGTERM, SIG_DFL);   /* Take Terminate signal and die! */
    signal(SIGSEGV, SIG_DFL);   /* Take Segfault and die! */
#ifdef SIGTRAP
    signal(SIGTRAP, SIG_DFL);
#endif
#ifdef SIGIOT
    signal(SIGIOT, SIG_DFL);
#endif
#ifdef SIGEMT
    signal(SIGEMT, SIG_DFL);
#endif
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif
#ifdef SIGSYS
    signal(SIGSYS, SIG_DFL);
#endif
#ifdef SIGXCPU
    signal(SIGXCPU, SIG_IGN);   /* CPU usage limit exceeded */
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ, SIG_IGN);   /* Exceeded file size limit */
#endif
#ifdef SIGVTALRM
    signal(SIGVTALRM, SIG_DFL);
#endif
}

/* Helper defines for set_sigs_intern */
#define SET_BAIL (bail ? SIG_DFL : bailout)
#define SET_IGN  (bail ? SIG_DFL : SIG_IGN)

/**
 * Internal signal settings
 *
 * Traps a bunch of signals and reroutes them to various handlers. Mostly
 * 'bailout'.  @see bailout
 *
 * If called from bailout, then reset all to default.  'bail' is true
 * when called from bailout.
 *
 * Called from main() and bailout()
 *
 * @param bail boolean true if called from 'bailout' function.
 */
static void
set_sigs_intern(int bail)
{
    /* we don't care about SIGPIPE, we notice it in select() and write() */
    signal(SIGPIPE, SET_IGN);

    /* didn't manage to lose that control tty, did we? Ignore it anyway. */
    signal(SIGHUP, sig_reconfigure);

#ifdef SPAWN_HOST_RESOLVER
    /* resolver's exited. Better clean up the mess our child leaves */
    signal(SIGCHLD, bail ? SIG_DFL : sig_reap);
#endif
    /* standard termination signals */
    signal(SIGINT, SET_BAIL);
    signal(SIGTERM, SET_BAIL);

    /* catch these because we might as well */
#ifdef SIGTRAP
    signal(SIGTRAP, SET_IGN);
#endif
#ifdef SIGIOT
    signal(SIGIOT, SET_BAIL);
#endif
#ifdef SIGEMT
    signal(SIGEMT, SET_BAIL);
#endif
#ifdef SIGBUS
    signal(SIGBUS, SET_BAIL);
#endif
#ifdef SIGSYS
    signal(SIGSYS, SET_BAIL);
#endif
    signal(SIGFPE, SET_BAIL);
    signal(SIGTERM, bail ? SET_BAIL : sig_shutdown);
#ifdef SIGXCPU
    signal(SIGXCPU, SET_BAIL);
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ, SET_BAIL);
#endif
#ifdef SIGVTALRM
    signal(SIGVTALRM, SET_BAIL);
#endif
#ifdef SIGEMERG
    /*
     * Clean shutdown signal (may be used with UPS software when power goes
     * down...)
     */
    signal(SIGUSR2, sig_emerg);
#else
    signal(SIGUSR2, SET_BAIL);
#endif

    /* status dumper (predates "WHO" command) */
    signal(SIGUSR1, bail ? SIG_DFL : sig_dump_status);

#ifdef HAVE_PSELECT
    /* This is only run once */
    if (!set_pselect_signal_mask) {
        sigset_t signals_to_block;

        sigemptyset(&signals_to_block);

        /*
         * block signals we expect during normal operation from that it
         * should be okay to wait for the current MUCK event to finish
         * before processing
         */
        sigaddset(&signals_to_block, SIGCHLD);
        sigaddset(&signals_to_block, SIGUSR1);

        /* block signals and record current signal mask */
        sigprocmask(SIG_BLOCK, &signals_to_block, &pselect_signal_mask);
        set_pselect_signal_mask = 1;
    }
#endif
}

/**
 * This does the initial signal setup.
 *
 * @TODO We could just rename set_sigs_intern to set_signals and take
 *       the boolean parameter rather than have this dinky little method.
 *       set_signals is only called in interface.c so making it pass
 *       0/false is no big deal.  If you change it, remember to update
 *       the header documentation.
 */
void
set_signals(void)
{
    set_sigs_intern(0);
}

/*
 * Signal handlers
 */

/**
 * Handle a situation where the MUCK must exit due to a signal.
 *
 * This results in the MUCK exiting with code 7.
 *
 * @param sig the signal number called.
 */
void
bailout(int sig)
{
    char message[1024];

    /*
     * Turn off signals
     *
     * @TODO Do we really need to do this?  This complicates set_sigs_intern
     *       and I don't really get why we need to do it unless the signal
     *       settings are interfering with exit(...) on some OSes?  I would
     *       suggest commenting this out and throwing some signals at the
     *       MUCK, see if it comes down.  If we don't need to do this, then
     *       we can get rid of the parameter and all the weird 'bail' if
     *       statements.
     */
    set_sigs_intern(1);

    snprintf(message, sizeof(message), "BAILOUT: caught signal %d", sig);

    panic(message);

    exit(7);
}

/**
 * Spew WHO to file
 *
 * @param i the signal number (not used)
 */
void
sig_dump_status(int i)
{
    dump_status();
}

#ifdef SIGEMERG
/**
 * Handle SIGEMERG
 *
 * This will shut down the MUCK and send a wall message to the MUCK.
 * Database will be dumped as well.
 *
 * @param i signal number (ignored)
 */
void
sig_emerg(int i)
{
    /*
     * @TODO This should probably log_status as well ?
     */
    wall_and_flush
        ("\nEmergency signal received ! (power failure ?)\nThe database will be saved.\n");
    dump_database();
    shutdown_flag = 1;
    restart_flag = 0;
}
#endif

/**
 * Send status to wizards and log file
 *
 * @private
 * @param s the message to send
 */
static void
wall_status(char *s)
{
    char buf[BUFFER_LEN];

    log_status(s);
    snprintf(buf, sizeof(buf), "## %s", s);
    wall_wizards(buf);
}

/**
 * Reload SSL configuration based on signal
 *
 * @param i the signal number (ignored)
 */
void
sig_reconfigure(int i)
{
    wall_status("Configuration reload requested remotely.");

#ifdef USE_SSL
    if (reconfigure_ssl()) {
        wall_status("Certificate reload was successful.");
    } else {
        wall_status("Certificate reload failed!");
    }
#endif

    wall_status("Configuration reload complete.");
}

/**
 * Gracefully shut the server down.
 *
 * @param i the signal number (ignored)
 */
void
sig_shutdown(int i)
{
    log_status("SHUTDOWN: via SIGNAL");
    shutdown_flag = 1;
    restart_flag = 0;
}

#ifdef SPAWN_HOST_RESOLVER
/*
 * Clean out Zombie Resolver Process.
 */

void
sig_reap(int i)
{
    /*
     * If DISKBASE is not defined, then there are two types of
     * children that can die.  First is the nameservice resolver.
     * Second is the database dumper.  If resolver exits, we should
     * note it in the log -- at least give the admin the option of
     * knowing about it, and dealing with it as necessary.
     *
     * The fix for SSL connections getting closed when databases were
     * saved with DISKBASE disabled required closing all sockets 
     * when the server fork()ed.  This made it impossible for that
     * process to spit out the "save done" message.  However, because
     * that process dies as soon as it finishes dumping the database,
     * can detect that the child died, and broadcast the "save done"
     * message anyway.
     */

    int status = 0;
    int reapedpid = 0;
    int need_to_spawn_resolver = 0;

    /*
     * look for children to reap in a loop in case a resolver and dumper
     * process die at almost the same time.
     */
    do {
        reapedpid = waitpid(-1, &status, WNOHANG);

        if (reapedpid == global_resolver_pid) {
            log_status("resolver exited with status %d", status);

            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                /* If the resolver exited with an error, respawn it. */
                need_to_spawn_resolver = 1;
            } else if (WIFSIGNALED(status)) {
                /* If the resolver exited due to a signal, respawn it. */
                need_to_spawn_resolver = 1;
            }

#ifndef DISKBASE
        } else if (reapedpid == global_dumper_pid) {
            int warnflag = 0;

            log_status("forked DB dump task exited with status %d", status);

            if (WIFSIGNALED(status)) {
                warnflag = 1;
            } else if (WIFEXITED(status)) {
                /* In case NOCOREDUMP is defined, check for panic()s exit codes. */
                int statres = WEXITSTATUS(status);
                if (statres == 135 || statres == 136) {
                    warnflag = 1;
                }
            }
            if (warnflag) {
                wall_wizards
                        ("# WARNING: The forked DB save process crashed while saving the database.");
                wall_wizards
                        ("# This is probably due to memory corruption, which can crash this server.");
                wall_wizards
                        ("# Unless you have a REALLY good unix programmer around who can try to fix");
                wall_wizards
                        ("# this process live with a debugger, you should try to restart this Muck");
                wall_wizards
                        ("# as soon as possible, and accept the data lost since the previous DB save.");
            }
            global_dumpdone = 1;
            global_dumper_pid = 0;
#endif
        } else if (reapedpid == -1) {
            log_status("waitpid() call from SIGCHLD handler returned errno=%d", errno);
        } else {
            fprintf(stderr, "unknown child process (pid %d) exited with status %d\n",
                    reapedpid, status);
        }
    } while (reapedpid != -1 && reapedpid != 0);

    /*
     * spawn the resolver after the loop so if it exits immediately,
     * we still manage to exit this signal handler.
     */
    if (need_to_spawn_resolver) {
        spawn_resolver();
    }
}
#endif

#else /* WIN32 */

/*
 * In the case of windows, we actually ignore most of this stuff.
 *
 * I don't know how doxygen will handle these "duplicates" so I am not
 * using docblock tags. (tanabi)
 */

#ifdef SPAWN_HOST_RESOLVER
/*
 * FOR WINDOWS: reap does nothing.
 *
 * @param i the signal number (ignored)
 */
void
sig_reap(int i)
{
}
#endif

/*
 * FOR WINDOWS: shutdown does nothing.
 *
 * @param i the signal number (ignored)
 */
void
sig_shutdown(int i)
{
}

/*
 * FOR WINDOWS: set signal internal does nothing
 *
 * @param bail (ignored)
 */
void
set_sigs_intern(int bail)
{
}

/*
 * FOR WINDOWS: set signal does nothing
 */
void
set_signals()
{
}

/*
 * FOR WINDOWS: bailout is simplified
 *
 * @param sig the signal number
 */
void
bailout(int sig)
{
    char message[1024];
    snprintf(message, sizeof(message), "BAILOUT: caught signal %d", sig);
    panic(message);
    exit(7);
}

/**
 * Windows Console Control Handler
 *
 * This handles different "controls", such as CTRL-C, close events,
 * shutdown, etc.  Its basically an event handler for the console
 * window.
 *
 * @param mesg the event type
 * @return boolean true if event was processed/handled, false otherwise.
 */
BOOL WINAPI
HandleConsole(DWORD mesg)
{
    switch (mesg) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            break;

        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            shutdown_flag = 1;
            restart_flag = 0;
            log_status("SHUTDOWN: via SIGNAL");
            break;

        default:
            return 0;
    }

    return 1;
}

/**
 * Windows console setup
 *
 * Does windows-specific console setup items.
 */
void
set_console()
{
    SetConsoleCtrlHandler(HandleConsole, 1);
    SetConsoleTitle(VERSION);
}

#endif /* WIN32 */
