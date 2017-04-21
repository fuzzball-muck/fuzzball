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
#include "msgparse.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

#include <signal.h>

#ifndef WIN32
#include <sys/wait.h>
#else
#include <process.h>
#include <windows.h>
#endif

#define Matched(string) { if(!string_prefix((string), command)) goto bad; }

int force_level = 0;

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
#ifdef USE_IPV6
	"IPV6 "
#endif
#ifdef MALLOC_PROFILING
	"MEMPROF "
#endif
#ifdef MEMORY_CLEANUP
        "MEMCLEANUP "
#endif
#ifdef MCP_SUPPORT
	"MCP "
#endif
#ifdef SPAWN_HOST_RESOLVER
	"RESOLVER "
#endif
#ifdef HAVE_LIBSSL
	"SSL "
#endif
	"";

/* declarations */
static const char *dumpfile = 0;
static int epoch = 0;
static int forked_dump_process_flag = 0;
FILE *input_file;
static char *in_filename = NULL;

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
    if (*newfile
#ifdef GOD_PRIV
	&& God(player)
#endif				/* GOD_PRIV */
	    ) {
	if (dumpfile)
	    free((void *) dumpfile);
	dumpfile = alloc_string(newfile);
	snprintf(buf, sizeof(buf), "Dumping to file %s...", dumpfile);
    } else {
	snprintf(buf, sizeof(buf), "Dumping...");
    }
    notify(player, buf);
    dump_db_now();
}

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

static void
dump_database_internal(void)
{
    char tmpfile[2048];
    FILE *f;

    snprintf(tmpfile, sizeof(tmpfile), "%s.#%d#", dumpfile, epoch - 1);
    (void) unlink(tmpfile);	/* nuke our predecessor */

    snprintf(tmpfile, sizeof(tmpfile), "%s.#%d#", dumpfile, epoch);

    if ((f = fopen(tmpfile, "wb")) != NULL) {
	db_write(f);
	fclose(f);

#ifdef DISKBASE
	fclose(input_file);
#endif

#ifdef WIN32
	(void) unlink(dumpfile);	/* Delete old file before rename */
#endif

	if (rename(tmpfile, dumpfile) < 0)
	    perror(tmpfile);

#ifdef DISKBASE
	free((void *) in_filename);
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

void
panic(const char *message)
{
    char panicfile[2048];
    FILE *f;

    log_status("PANIC: %s", message);
    fprintf(stderr, "PANIC: %s\n", message);

    /* shut down interface */
    if (!forked_dump_process_flag) {
	emergency_shutdown();
    }

#ifdef SPAWN_HOST_RESOLVER
    if (global_resolver_pid != 0) {
        (void) kill(global_resolver_pid, SIGKILL);
    }
#endif

    /* dump panic file */
    snprintf(panicfile, sizeof(panicfile), "%s.PANIC", dumpfile);
    if ((f = fopen(panicfile, "wb")) == NULL) {
	perror("CANNOT OPEN PANIC FILE, YOU LOSE");
	sync();

#ifdef NOCOREDUMP
	exit(135);
#else				/* !NOCOREDUMP */
# ifdef SIGIOT
	signal(SIGIOT, SIG_DFL);
# endif
	abort();
#endif				/* NOCOREDUMP */
    } else {
	log_status("DUMPING: %s", panicfile);
	fprintf(stderr, "DUMPING: %s\n", panicfile);
	db_write(f);
	fclose(f);
	log_status("DUMPING: %s (done)", panicfile);
	fprintf(stderr, "DUMPING: %s (done)\n", panicfile);
    }

    /* Write out the macros */
    snprintf(panicfile, sizeof(panicfile), "%s.PANIC", MACRO_FILE);
    if ((f = fopen(panicfile, "wb")) != NULL) {
	macrodump(macrotop, f);
	fclose(f);
    } else {
	perror("CANNOT OPEN MACRO PANIC FILE, YOU LOSE");
	sync();
#ifdef NOCOREDUMP
	exit(135);
#else				/* !NOCOREDUMP */
#ifdef SIGIOT
	signal(SIGIOT, SIG_DFL);
#endif
	abort();
#endif				/* NOCOREDUMP */
    }

    sync();

    volatile char *x = 0;
    *x = 1;

#ifdef NOCOREDUMP
    exit(136);
#else				/* !NOCOREDUMP */
#ifdef SIGIOT
    signal(SIGIOT, SIG_DFL);
#endif
    abort();
#endif				/* NOCOREDUMP */
}

void
dump_database(void)
{
    epoch++;

    log_status("DUMPING: %s.#%d#", dumpfile, epoch);
    dump_database_internal();
    log_status("DUMPING: %s.#%d# (done)", dumpfile, epoch);
}

#ifdef WIN32
/* TODO: This is not thread safe - disabled for now... */
/*void fork_dump_thread(void *arg) {
	forked_dump_process_flag = 1;
	dump_database_internal();
        global_dumper_pid = 0;
	_endthread();
}*/
#endif


/*
 * Named "fork_and_dump()" mostly for historical reasons...
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

#ifdef DISKBASE
    dump_database_internal();
#else
# ifndef WIN32
    if ((global_dumper_pid = fork()) == 0) {
	/* We are the child. */
	forked_dump_process_flag = 1;
#  ifdef NICEVAL
	/* Requested by snout of SPR, reduce the priority of the
	 * dumper child. */
	nice(NICEVAL);
#  endif			/* NICEVAL */
	set_dumper_signals();
	dump_database_internal();
	_exit(0);
    }
    if (global_dumper_pid < 0) {
	global_dumper_pid = 0;
	wall_wizards("## Could not fork for database dumping.  Possibly out of memory.");
	wall_wizards("## Please restart the server when next convenient.");
    }
# else				/* !WIN32 */
    dump_database_internal();
    /* TODO: This is not thread safe - disabled for now... */
    /*global_dumper_pid = (long) _beginthread(fork_dump_thread, 0, 0);
       if (global_dumper_pid == -1L) {
       wall_wizards("## Could not create thread for database dumping");
       } */
# endif
#endif
}

static void
ensure_support()
{
    char buf[BUFFER_LEN];

    /* players need at least one pname_history entry */
    for (dbref i = 0; i < db_top; i++) {
	if (Typeof(i) == TYPE_PLAYER && !is_propdir(i, PNAME_HISTORY_PROPDIR)) {
	    snprintf(buf, sizeof(buf), "%s/0", PNAME_HISTORY_PROPDIR);
	    add_property(i, buf, NAME(i), 0);
	}
    }
}

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

    in_filename = (char *) strdup(infile);
    if ((input_file = fopen(infile, "rb")) == NULL)
	return -1;

    db_free();
    init_primitives();		/* init muf compiler */
    mesg_init();		/* init mpi interpreter */
    SRANDOM(getpid());		/* init random number generator */

    /* ok, read the db in */
    log_status("LOADING: %s", infile);
    fprintf(stderr, "LOADING: %s\n", infile);
    if (db_read(input_file) < 0)
	return -1;
    log_status("LOADING: %s (done)", infile);
    fprintf(stderr, "LOADING: %s (done)\n", infile);

    ensure_support();

    /* set up dumper */
    if (dumpfile)
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
void
cleanup_game()
{
    if (dumpfile)
        free((void *) dumpfile);
    free((void *) in_filename);
}
#endif

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
	if (!(FLAGS(player) & (INTERACTIVE | READMODE))) {
	    if (!*command) {
                free(log_name);
		return;
	    }
	    log_command("%s: %s", log_name, command);
	} else {
	    if (tp_log_interactive) {
		log_command("%s: %s%s", log_name,
			    (FLAGS(player) & (READMODE)) ? "[READ] " : "[INTERP] ", command);
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
	    do_move(descr, player, command, 0);	/* command is exact match for exit */
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
		    do_move(descr, player, command, 0);	/* command is exact match for exit */
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

	/* truncate arg1 */
	for (char *p = arg2 - 1; p >= arg1 && isspace(*p); p--)
	    *p = '\0';

	/* go past delimiter if present */
	if (*arg2)
	    *arg2++ = '\0';
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
/*
					WIZARDONLY("@armageddon", player);
					PLAYERONLY("@armageddon", player);
*/
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
		/* @chlock, @chown, @chown_lock, @clone,
		   @conlock, @contents, @create, @credits */
		switch (command[2]) {
		case 'h':
		case 'H':
		    switch (command[3]) {
		    case 'l':
		    case 'L':
			Matched("@chlock");
			NOGUEST("@chlock", player);
			set_standard_lock(descr, player, arg1, MESGPROP_CHLOCK, "Chown Lock",
					  arg2);
			break;
		    case 'o':
		    case 'O':
			if (strlen(command) < 7) {
			    Matched("@chown");
			    do_chown(descr, player, arg1, arg2);
			} else {
			    Matched("@chown_lock");
			    NOGUEST("@chown_lock", player);
			    set_standard_lock(descr, player, arg1, MESGPROP_CHLOCK,
					      "Chown Lock", arg2);
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
			set_standard_lock(descr, player, arg1, MESGPROP_CONLOCK,
					  "Container Lock", arg2);
			break;
		    case 't':
		    case 'T':
			Matched("@contents");
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
		/* @debug, @describe, @dig, @doing,
		   @drop, @dump */
		switch (command[2]) {
		case 'e':
		case 'E':
                    if (strcasecmp(command, "@debug")) {
			Matched("@describe");
			NOGUEST("@describe", player);
			set_standard_property(descr, player, arg1, MESGPROP_DESC,
					  "Object Description", arg2);
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
		    if (!tp_who_doing)
			goto bad;
		    NOGUEST("@doing", player);
		    set_standard_property(descr, player, arg1, MESGPROP_DOING, "Doing", arg2);
		    break;
		case 'r':
		case 'R':
		    Matched("@drop");
		    NOGUEST("@drop", player);
		    set_standard_property(descr, player, arg1, MESGPROP_DROP, "Drop Message",
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
		    set_standard_property(descr, player, arg1, MESGPROP_FAIL, "Fail Message",
					  arg2);
		    break;
		case 'i':
		case 'I':
		    Matched("@find");
		    do_find(player, arg1, arg2);
		    break;
		case 'l':
		case 'L':
		    Matched("@flock");
		    NOGUEST("@flock", player);
		    NOFORCE("@flock", player);
		    set_standard_lock(descr, player, arg1, MESGPROP_FLOCK, "Force Lock", arg2);
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
			set_standard_lock(descr, player, arg1, MESGPROP_FLOCK, "Force Lock",
					  arg2);
		    }
		    break;
		default:
		    goto bad;
		}
		break;
	    case 'h':
	    case 'H':
		/* @hashes */
		Matched("@hashes");
		do_hashes(player, arg1);
		break;
	    case 'i':
	    case 'I':
		/* @idescribe */
		Matched("@idescribe");
		NOGUEST("@idescribe", player);
		set_standard_property(descr, player, arg1, MESGPROP_IDESC,
				      "Inside Description", arg2);
		break;
	    case 'k':
	    case 'K':
		/* @kill */
		Matched("@kill");
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
			    set_standard_lock(descr, player, arg1, MESGPROP_LINKLOCK,
						"Link Lock", arg2);
			}
			break;
		    case 's':
		    case 'S':
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
		    set_standard_lock(descr, player, arg1, MESGPROP_LOCK, "Lock", arg2);
		    break;
		default:
		    goto bad;
		}
		break;
	    case 'm':
	    case 'M':
		/* @mcpedit, @mcpprogram, @memory, @mpitops,
		   @muftops */
		switch (command[2]) {
#ifdef MCP_SUPPORT
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
#endif
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
		/* @odrop, @oecho, @ofail, @open, @osuccess,
		   @owned, @ownlock */
		switch (command[2]) {
		case 'd':
		case 'D':
		    Matched("@odrop");
		    NOGUEST("@odrop", player);
		    set_standard_property(descr, player, arg1, MESGPROP_ODROP, "ODrop Message",
					  arg2);
		    break;
		case 'e':
		case 'E':
		    Matched("@oecho");
		    NOGUEST("@oecho", player);
		    set_standard_property(descr, player, arg1, MESGPROP_OECHO,
					  "Outside-echo Prefix", arg2);
		    break;
		case 'f':
		case 'F':
		    Matched("@ofail");
		    NOGUEST("@ofail", player);
		    set_standard_property(descr, player, arg1, MESGPROP_OFAIL, "OFail Message",
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
		    set_standard_property(descr, player, arg1, MESGPROP_OSUCC,
					  "OSuccess Message", arg2);
		    break;
		case 'w':
		case 'W':
		    if (string_prefix("@owned", command)) {
			Matched("@owned");
			do_owned(player, arg1, arg2);
			break;
		    } else {
			Matched("@ownlock");
			NOGUEST("@ownlock", player);
			NOFORCE("@ownlock", player);
			set_standard_lock(descr, player, arg1, MESGPROP_OWNLOCK, "Ownership Lock", arg2);
			break;
		    }
		default:
		    goto bad;
		}
		break;
	    case 'p':
	    case 'P':
		/* @password, @pcreate, @pecho, @program, 
		   @propset, @ps */
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
		    set_standard_property(descr, player, arg1, MESGPROP_PECHO,
					  "Puppet-echo Prefix", arg2);
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
		    do_process_status(player);
		    break;
		default:
		    goto bad;
		}
		break;
	    case 'r':
	    case 'R':
		/* @readlock, @recycle, @reconfiguressl, @relink, @restart, @restrict */
		switch (command[3]) {
		case 'a':
		case 'A':
		    Matched("@readlock");
		    NOGUEST("@readlock", player);
		    NOFORCE("@readlock", player);
		    set_standard_lock(descr, player, arg1, MESGPROP_READLOCK, "Read Lock", arg2);
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
		case 'l':
		case 'L':
		    Matched("@relink");
		    NOGUEST("@relink", player);
		    do_relink(descr, player, arg1, arg2);
		    break;
		case 's':
		case 'S':
		    if (!strcmp(command, "@restart")) {
/*
						WIZARDONLY("@restart", player);
						PLAYERONLY("@restart", player);
*/
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
		/* @sanity, @sanchange, @sanfix, @set,
		   @shutdown, @stats, @success, @sweep */
		switch (command[2]) {
		case 'a':
		case 'A':
		    if (!strcmp(command, "@sanity")) {
			GODONLY("@sanity", player);
			do_sanity(player);
		    } else if (!strcmp(command, "@sanchange")) {
			GODONLY("@sanchange", player);
			do_sanchange(player, full_command);
		    } else if (!strcmp(command, "@sanfix")) {
			GODONLY("@sanfix", player);
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
/*
					WIZARDONLY("@shutdown", player);
					PLAYERONLY("@shutdown", player);
*/
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
		    set_standard_property(descr, player, arg1, MESGPROP_SUCC,
					  "Success Message", arg2);
		    break;
		case 'w':
		case 'W':
		    Matched("@sweep");
		    do_sweep(descr, player, arg1);
		    break;
		default:
		    goto bad;
		}
		break;
	    case 't':
	    case 'T':
		/* @teleport, @toad, @trace, @tune */
		switch (command[2]) {
		case 'e':
		case 'E':
		    Matched("@teleport");
		    do_teleport(descr, player, arg1, arg2);
		    break;
		case 'o':
		case 'O':
		    if (!strcmp(command, "@toad")) {
			WIZARDONLY("@toad", player);
			PLAYERONLY("@toad", player);
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
		    do_trace(descr, player, arg1, atoi(arg2));
		    break;
		case 'u':
		case 'U':
		    Matched("@tune");
		    WIZARDONLY("@tune", player);
		    PLAYERONLY("@tune", player);
		    do_tune(player, arg1, arg2, !!strchr(full_command, ARG_DELIMITER));
		    break;
		default:
		    goto bad;
		}
		break;
	    case 'u':
	    case 'U':
		/* @unbless, @unlink, @unlock, @uncompile,
		   @usage */
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
			set_standard_lock(descr, player, arg1, MESGPROP_LOCK, "Lock", "");
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
	    /* help */
	    Matched("help");
	    do_help(player, arg1, arg2);
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
	case 'k':
	case 'K':
	    /* kill */
	    Matched("kill");
	    do_kill(descr, player, arg1, atoi(arg2));
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
		do_mpihelp(player, arg1, arg2);
		break;
	    } else {
		if (strcasecmp(command, "man"))
		    goto bad;
		do_man(player, full_command, arg2);
	    }
	    break;
	case 'n':
	case 'N':
	    /* news */
	    Matched("news");
	    do_news(player, arg1, arg2);
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
		Matched("read");	/* undocumented alias for look */
		do_look_at(descr, player, arg1, arg2);
		break;
	    case 'o':
	    case 'O':
		Matched("rob");
		do_rob(descr, player, arg1);
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
	    if (tp_log_failed_commands && !controls(player, LOCATION(player))) {
		log_status("HUH from %s(%d) in %s(%d)[%s]: %s %s",
			   NAME(player), player, NAME(LOCATION(player)),
			   LOCATION(player),
			   NAME(OWNER(LOCATION(player))), command, full_command);
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
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", MUCK_LOCALTIME(st));
        char *log_name = whowhere(player);
	log2file(tp_file_log_cmd_times, "%s: (%.3f) %s: %s",
		 tbuf, totaltime, log_name, command);
        free(log_name);
    }
}

#undef Matched
