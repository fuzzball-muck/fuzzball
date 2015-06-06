/*
 * signal.c -- Curently included into interface.c
 *
 * Seperates the signal handlers and such into a seperate file
 * for maintainability.
 *
 * Broken off from interface.c, and restructured for POSIX
 * compatible systems by Peter A. Torkelson, aka WhiteFire.
 */

#ifdef SOLARIS
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE		/* Solaris needs this */
#  endif
#endif

#include "config.h"
#include "interface.h"
#include "externs.h"
#include "version.h"

#ifndef WIN32
#include <signal.h>
#include <sys/wait.h>

/*
 * SunOS can't include signal.h and sys/signal.h, stupid broken OS.
 */
#if defined(HAVE_SYS_SIGNAL_H) && !defined(SUN_OS)
# include <sys/signal.h>
#endif

#if defined(ULTRIX) || defined(_POSIX_VERSION)
#undef RETSIGTYPE
#define RETSIGTYPE void
#endif

/*
 * Function prototypes
 */
void set_signals(void);
RETSIGTYPE bailout(int);
RETSIGTYPE sig_dump_status(int i);
RETSIGTYPE sig_shutdown(int i);
#ifdef SIGEMERG
RETSIGTYPE sig_emerg(int i);
#endif

#ifdef SPAWN_HOST_RESOLVER
RETSIGTYPE sig_reap(int i);
#endif

#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler) (int));
#else
# define our_signal(s,f) signal((s),(f))
#endif

/*
 * our_signal(signo, sighandler)
 *
 * signo      - Signal #, see defines in signal.h
 * sighandler - The handler function desired.
 *
 * Calls sigaction() to set a signal, if we are posix.
 */
#ifdef _POSIX_VERSION
void
our_signal(int signo, void (*sighandler) (int))
{
	struct sigaction act, oact;

	act.sa_handler = sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	/* Restart long system calls if a signal is caught. */
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	/* Make it so */
	sigaction(signo, &act, &oact);
}

#endif							/* _POSIX_VERSION */

void
set_dumper_signals(void)
{
	our_signal(SIGPIPE, SIG_IGN); /* Ignore Blocked Pipe */
	our_signal(SIGHUP,  SIG_IGN); /* Ignore Terminal Hangup */
	our_signal(SIGCHLD, SIG_IGN); /* Ignore Child termination */
	our_signal(SIGFPE,  SIG_IGN); /* Ignore FP exceptions */
	our_signal(SIGUSR1, SIG_IGN); /* Ignore SIGUSR1 */
	our_signal(SIGUSR2, SIG_IGN); /* Ignore SIGUSR2 */
	our_signal(SIGINT,  SIG_DFL); /* Take Interrupt signal and die! */
	our_signal(SIGTERM, SIG_DFL); /* Take Terminate signal and die! */
	our_signal(SIGSEGV, SIG_DFL); /* Take Segfault and die! */
#ifdef SIGTRAP
	our_signal(SIGTRAP, SIG_DFL);
#endif
#ifdef SIGIOT
	our_signal(SIGIOT, SIG_DFL);
#endif
#ifdef SIGEMT
	our_signal(SIGEMT, SIG_DFL);
#endif
#ifdef SIGBUS
	our_signal(SIGBUS, SIG_DFL);
#endif
#ifdef SIGSYS
	our_signal(SIGSYS, SIG_DFL);
#endif
#ifdef SIGXCPU
	our_signal(SIGXCPU, SIG_IGN);  /* CPU usage limit exceeded */
#endif
#ifdef SIGXFSZ
	our_signal(SIGXFSZ, SIG_IGN);  /* Exceeded file size limit */
#endif
#ifdef SIGVTALRM
	our_signal(SIGVTALRM, SIG_DFL);
#endif
}


/*
 * set_signals()
 * set_sigs_intern(bail)
 *
 * Traps a bunch of signals and reroutes them to various
 * handlers. Mostly bailout.
 *
 * If called from bailout, then reset all to default.
 *
 * Called from main() and bailout()
 */
#define SET_BAIL (bail ? SIG_DFL : bailout)
#define SET_IGN  (bail ? SIG_DFL : SIG_IGN)

static void
set_sigs_intern(int bail)
{
	/* we don't care about SIGPIPE, we notice it in select() and write() */
	our_signal(SIGPIPE, SET_IGN);

	/* didn't manage to lose that control tty, did we? Ignore it anyway. */
	our_signal(SIGHUP, SET_IGN);

#ifdef SPAWN_HOST_RESOLVER
	/* resolver's exited. Better clean up the mess our child leaves */
	our_signal(SIGCHLD, bail ? SIG_DFL : sig_reap);
#endif
	/* standard termination signals */
	our_signal(SIGINT, SET_BAIL);
	our_signal(SIGTERM, SET_BAIL);

	/* catch these because we might as well */
/*  our_signal(SIGQUIT, SET_BAIL);  */
#ifdef SIGTRAP
	our_signal(SIGTRAP, SET_IGN);
#endif
#ifdef SIGIOT
	our_signal(SIGIOT, SET_BAIL);
#endif
#ifdef SIGEMT
	our_signal(SIGEMT, SET_BAIL);
#endif
#ifdef SIGBUS
	our_signal(SIGBUS, SET_BAIL);
#endif
#ifdef SIGSYS
	our_signal(SIGSYS, SET_BAIL);
#endif
	our_signal(SIGFPE, SET_BAIL);
	our_signal(SIGSEGV, SET_BAIL);
	our_signal(SIGTERM, bail ? SET_BAIL : sig_shutdown);
#ifdef SIGXCPU
	our_signal(SIGXCPU, SET_BAIL);
#endif
#ifdef SIGXFSZ
	our_signal(SIGXFSZ, SET_BAIL);
#endif
#ifdef SIGVTALRM
	our_signal(SIGVTALRM, SET_BAIL);
#endif
#ifdef SIGEMERG
	/* Clean shutdown signal (may be used with UPS software when power goes down...) */
	our_signal(SIGUSR2, sig_emerg);
#else
	our_signal(SIGUSR2, SET_BAIL);
#endif

	/* status dumper (predates "WHO" command) */
	our_signal(SIGUSR1, bail ? SIG_DFL : sig_dump_status);
}

void
set_signals(void)
{
	set_sigs_intern(FALSE);
}

/*
 * Signal handlers
 */

/*
 * BAIL!
 */
RETSIGTYPE bailout(int sig)
{
	char message[1024];

	/* turn off signals */
	set_sigs_intern(TRUE);

	snprintf(message, sizeof(message), "BAILOUT: caught signal %d", sig);

	panic(message);
	exit(7);

#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}

/*
 * Spew WHO to file
 */
RETSIGTYPE sig_dump_status(int i)
{
	dump_status();
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}

#ifdef SIGEMERG
RETSIGTYPE sig_emerg(int i)
{
	wall_and_flush("\nEmergency signal received ! (power failure ?)\nThe database will be saved.\n");
	dump_database();
	shutdown_flag = 1;
	restart_flag = 0;
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}
#endif

/*
 * Gracefully shut the server down.
 */
RETSIGTYPE sig_shutdown(int i)
{
	log_status("SHUTDOWN: via SIGNAL");
	shutdown_flag = 1;
	restart_flag = 0;
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}

#ifdef SPAWN_HOST_RESOLVER
/*
 * Clean out Zombie Resolver Process.
 */
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
#define RETSIGVAL 0
#else
#define RETSIGVAL 
#endif

RETSIGTYPE sig_reap(int i)
{
	/* If DISKBASE is not defined, then there are two types of
	 * children that can die.  First is the nameservice resolver.
	 * Second is the database dumper.  If resolver exits, we should
	 * note it in the log -- at least give the admin the option of
	 * knowing about it, and dealing with it as necessary. */

	/* The fix for SSL connections getting closed when databases were
	 * saved with DISKBASE disabled required closing all sockets 
	 * when the server fork()ed.  This made it impossible for that
	 * process to spit out the "save done" message.  However, because
	 * that process dies as soon as it finishes dumping the database,
	 * can detect that the child died, and broadcast the "save done"
	 * message anyway. */

	int status = 0;
	int reapedpid = 0;

	reapedpid = waitpid(-1, &status, WNOHANG);
	if(!reapedpid)
	{
#ifdef DETACH
		log2file(LOG_ERR_FILE,"SIG_CHILD signal handler called with no pid!");
#else
		fprintf(stderr, "SIG_CHILD signal handler called with no pid!\n");
#endif
	} else {
		if (reapedpid == global_resolver_pid) {
			log_status("resolver exited with status %d", status);
			if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
				/* If the resolver exited with an error, respawn it. */
				spawn_resolver();
			} else if (WIFSIGNALED(status)) {
				/* If the resolver exited due to a signal, respawn it. */
				spawn_resolver();
			}
#ifndef DISKBASE
		} else if(reapedpid == global_dumper_pid) {
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
				wall_wizards("# WARNING: The forked DB save process crashed while saving the database.");
				wall_wizards("# This is probably due to memory corruption, which can crash this server.");
				wall_wizards("# Unless you have a REALLY good unix programmer around who can try to fix");
				wall_wizards("# this process live with a debugger, you should try to restart this Muck");
				wall_wizards("# as soon as possible, and accept the data lost since the previous DB save.");
			}
			global_dumpdone = 1;
			global_dumper_pid = 0;
#endif
		} else {
			fprintf(stderr, "unknown child process (pid %d) exited with status %d\n", reapedpid, status);
		}
	}
	return RETSIGVAL;
}
#endif

#else /* WIN32 */

#include <wincon.h>
#include <windows.h>
#include <signal.h>
#define VK_C         0x43
#define CONTROL_KEY (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED) 


#ifdef SPAWN_HOST_RESOLVER
void sig_reap(int i) {}
#endif

void sig_shutdown(int i) {}
void sig_dumpstatus(int i) {}
void set_sigs_intern(int bail) {}
void set_signals() {}

void bailout(int sig) {
	char message[1024];
	snprintf(message, sizeof(message), "BAILOUT: caught signal %d", sig);
	panic(message);
	exit(7);
}


BOOL WINAPI HandleConsole(DWORD mesg) {
   switch(mesg) {
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
         return FALSE;
   }

   return TRUE;
}

void set_console() {
	HANDLE InputHandle;

    SetConsoleCtrlHandler(HandleConsole, TRUE);
	SetConsoleTitle(VERSION);
}

#endif /* WIN32 */
