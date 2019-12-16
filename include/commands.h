/** @file commands.h
 *
 * Header for declaring the different commands people can run on the MUCK.
 * The definitions for these are scattered all over the source code.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "config.h"
#include "fbmuck.h"

/*
 * This file is grouped alphabetically just to make it slightly easier to find
 * things.  Please keep that ordering.
 */
 
/*
 * A
 */

/**
 * This routine attaches a new existing action to a source object, if possible.
 *
 * Defined in: create.c
 *
 * The action will not do anything until it is LINKed.  This is the 
 * implementation of @action.
 *
 * Action names must pass ok_object_name.
 *
 * Calls create_action under the hood.  @see create_action
 *
 * Deducts money from player based on tuned configuration.  Supports
 * registration name if source_name contains an = character in it.
 * Supports autolink action if configured in tune.
 *
 * This does a lot of permission checking, but does not check for BUILDER.
 *
 * @param descr the descriptor
 * @param player the player creating the action
 * @param action_name the action name to make
 * @param source_name the name of the thing to put the action on.
 */
void do_action(int descr, dbref player, const char *action_name,
               const char *source_name);

/**
 * Handles the @armageddon command, which hopefully you'll never have to use
 *
 * This is defined in interface.c
 *
 * This closes all player connections, kills the host resolver if applicable,
 * deletes the PID file, then exits with the ARMAGEDDON_EXIT_CODE.
 *
 * The intention of this is to not save the database to avoid corruption.
 *
 * It will always broadcast a message saying:
 *
 * \r\nImmediate shutdown initiated by {name}\r\n
 *
 * 'msg', if provided, will be appended after this message.
 *
 * Only wizards can use this command and player permissions are checked.
 *
 * @param player the player attempting to do the armageddon call.
 * @param msg the message to append to the default message, or NULL if you wish
 */
void do_armageddon(dbref player, const char *msg);

/**
 * This routine attaches a previously existing action to a source object.
 * The action will not do anything unless it is LINKed.  This is the
 * implementation of @attach
 *
 * This is defined in create.c
 *
 * Does all the permission checking associated except for the builder bit.
 * Will reset the priority level if there is is a priority set.
 *
 * Won't let you attach to a program or something that doesn't exist.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player doing the attaching.
 * @param action_name the action we are operating on
 * @param source_name what we are attaching to.
 */
void do_attach(int descr, dbref player, const char *action_name, const char *source_name);

/*
 * B
 */

/**
 * Implementation of the @bless command
 *
 * This does not handle basic permission checking.  The propname can have
 * wildcards in it.
 *
 * This is defined in wiz.c
 *
 * Two kinds of wildcards are understood; * and **.  ** is a recursive
 * wildcard, * is just a 'local' match.
 *
 * @param descr the descriptor of the user doing the command
 * @param player the player doing the command
 * @param what the object being operated on
 * @param propname the name of the prop to bless.
 */
void do_bless(int descr, dbref player, const char *target, const char *propname);

/**
 * Implementation of the @boot command
 *
 * Defined in wiz.c
 *
 * WARNING: This call does NOT check to see if 'player' is a wizard.
 *
 * @param player the player doing the @boot command
 * @param name the string name that will be matched for the player to boot.
 */ 
void do_boot(dbref player, const char *name);

/*
 * C
 */

/**
 * Implementation of the @chown command
 *
 * This is defined in set.c
 *
 * This handles all the security aspects of the @chown command, including
 * the various weird details.  Such as THINGs must be held by the person
 * doing the @chowning, only wizards can transfer things from one player
 * to another, players can only @chown the room they are in, and players
 * can @chown any exit that links to something they control.
 *
 * Does not check the builder bit.
 *
 * @param descr the descriptor of the player doing the action
 * @param player the player doing the action
 * @param name the object having its ownership changed
 * @param newowner the new owner for the object, which may be empty string
 *                 to default to 'me'
 */
void do_chown(int descr, dbref player, const char *name, const char *newobj);

/**
 * Use this to clone an object.  Implementation for the @clone command.
 *
 * This is defined in create.c
 *
 * This does most of the permission checks, but does not check for player
 * BUILDER bit.  Supports prop registration if rname is provided.  You
 * have no control over the cloned object's name, it uses the name of
 * the original object.
 *
 * @param descr the cloner's descriptor
 * @param player the player doing the cloning
 * @param name the object name to clone
 * @param rname the registration name, or "" if not registering.
 */
void do_clone(int descr, dbref player, const char *name, const char *rname);

/**
 * Implementation of @contents
 *
 * This is defined in look.c
 *
 * This searches the contents of a given object, similar to the way @find
 * works except confined to a certain object.  It supports a similar
 * syntax.  If 'name' is not provided, defaults to here.  'flags' can have
 * types and also an output type.
 *
 * This does do appropriate permission checking.
 *
 * @see do_find for more syntatical details.
 * @see checkflags
 *
 * @param descr the descriptor of the person running the command
 * @param player the person running the command
 * @param name the name of the object to check or "" for here
 * @param flags optional "find" syntax flags, or "" for none.
 */
void do_contents(int descr, dbref player, const char *name, const char *flags);

/**
 * Implementation of @create
 *
 * The definition of this is in create.c
 *
 * Use this to create an object.  This does some checking; for instance,
 * the name must pass ok_object_name.  Does NOT check builder bit.
 * Validates that the cost you want to set on the object is a valid amount.
 * Note that if acost has an = in it, it will have a registration done.
 *
 * @param player the player doing the create
 * @param name the name of the object
 * @param acost the cost you want to assign, or "" for default.
 */
void do_create(dbref player, char *name, char *cost);

/**
 * Implementation of @credits
 *
 * Just spits a credits file (tp_file_credits) out to the player.
 *
 * @see spit_file
 *
 * @param player the player to show the credits to.
 */
void do_credits(dbref player);

/*
 * D
 */

/**
 * Implementation of @debug command
 *
 * This is defined in wiz.c
 *
 * This only applies to DISKBASE (at the moment), and only works if
 * the argument "display propcache" is displayed.  Under the hood,
 * it just calls display_propcache.  Clearly the idea is that this could
 * do more in the future
 *
 * This does NO permission checking.
 *
 * @see display_propcache
 *
 * @param player the player doing the call
 * @param args the arguments provided.
 */
void do_debug(dbref player, const char *args);

/**
 * Implementation of @dig
 *
 * This is in create.c
 *
 * Use this to create a room.  This does a lot of checks; rooms must pass
 * ok_object_name.  It handles finding the parent, if no parent is
 * provided.  Otherwise, it tries to link the parent and does appropriate
 * permission checks.
 *
 * This does NOT check builder bit.
 *
 * It supports registrtration if pname has an = in it.
 *
 * @param descr the descriptor of the person making the call
 * @param player the player doing the call
 * @param name the name of the room to make
 * @param pname the parent room and possibly registration info
 */
void do_dig(int descr, dbref player, const char *name, const char *pname);

/**
 * Implemenation of @doing command
 *
 * This is defined in set.c
 *
 * This, at present, only allows you to to set a doing message on
 * 'me' or you can leave 'name' as an empty string.
 *
 * @param descr the descriptor of the person doing the action
 * @param player the player doing the action
 * @param name the name of the person to set the doing on or "" for 'me'
 * @param mesg the doing message to set.
 */
void do_doing(int descr, dbref player, const char *name, const char *mesg);

/**
 * Implementation of drop, put, and throw commands
 *
 * Implemented in move.c
 *
 * All three commands are completley identical under the hood.  The
 * only difference is their help files.
 *
 * The logic here is somewhat complicated as a result.  If just
 * 'name' is provided, this command acts like drop and seeks to
 * drop the given object in the caller's posession into the room.
 *
 * If 'obj' is also provided, then the 'throw'/'put' behavior is
 * in play.
 *
 * For the put/throw logic, if the target object is not a room, it must
 * have a container lock.  The lock is "@conlock" or "_/clk".  If there is
 * no lock it will default to false.
 *
 * This handles all the nuances of dropping things, such as drop/odrop
 * messages and room STICKY flags and drop-to's.
 *
 * @param descr the descriptor of the dropping player
 * @param player the dropping player
 * @param name the name of the object to drop
 * @param obj "" if dropping to current room, target of throw/put otherwise
 */
void do_drop(int descr, dbref player, const char *name, const char *obj);

/**
 * Implementation of the @dump command
 *
 * Defined in game.c
 *
 * This does NOT do any permission checking, except for a GOD_PRIV check
 * if a newfile is provided.
 *
 * If newfile is provided, all future dumps will go to that new file
 *
 * @param player the player taking a dump
 * @param newfile if provided, will use newfile as the dump target.  Can be ""
 */
void do_dump(dbref player, const char *newfile);

/*
 * E
 */

/**
 * This is the implementation of @edit
 *
 * This is defined in create.c
 *
 * This does not do any permission checking, however the underlying call
 * (edit_program) does do some minimal checking to make sure the player
 * controls the program being edited.
 *
 * This wrapper handles the matching of the program name then hands off
 * to edit_program.
 *
 * @see edit_program
 *
 * @param descr this is descriptor of the player entering edit
 * @param player the player doing the editing
 * @param name the name of the program to edit.
 */
void do_edit(int descr, dbref player, const char *name);

/**
 * Implementation of the @entrances command
 *
 * This is defined in look.c
 *
 * This supports the same sort of flag searches that @find does,
 * except it searches for exits on the given object (which defaults to here)
 *
 * Under the hood, this uses the checkflags series of methods.
 *
 * @see init_checkflags
 * @see checkflags
 * @see do_find
 *
 * See documentation for checkflags for the details of what flags are
 * supported.
 *
 * This does the necessary permission checking.
 *
 * @param descr the descriptor of the player making the call
 * @param player the player making the call
 * @param name the name of the object to search entrances on, or "" for here.
 * @param flags the flags to pass into checkflags
 */
void do_entrances(int descr, dbref player, const char *name, const char *flags);

/**
 * Implementation of the examine command
 *
 * This is a pretty huge infodump of the object 'name'.  Optionally, if
 * 'dir' is provided, we'll dump a listing of props instead.  'dir' can
 * have * or ** (recursive) wildcards.
 *
 * So it is tons of little if <has data> then <show data> types of
 * things, but is overall pretty simple.
 *
 * This does permission checking to make sure the user can examine
 * the given object.
 *
 * @param descr the descriptor of the examining player.
 * @param player the player doing the examining
 * @param name the name of the thing to be examined - defaults to 'here'
 * @param dir the prop directory to look for, or "" to not show props.
 */
void do_examine(int descr, dbref player, const char *name, const char *dir);

/**
 * This is the implementation of the @examine command
 *
 * Specifically, this command examines the "sanity" or data integrity of
 * the given object or "here" if no object is provided.  There is NO
 * permission checking here, so the caller is responsible to check
 * permissions.
 *
 * @param descr the descriptor of the calling player
 * @param player the calling player
 * @param arg the target of the examine call.
 */
void do_examine_sanity(int descr, dbref player, const char *arg);

/*
 * F
 */

/**
 * Implementation of @find command
 *
 * This implements the find command, which is powered by
 * checkflags.  See that function for full details of how flags work.
 *
 * @see init_checkflags
 *
 * Takes a search string to look for, and iterates over the entire
 * database to find it.  There is an option to charge players for
 * @find's, probably since this is kind of nasty on the DB.
 *
 * @param player the player doing the find
 * @param name the search criteria
 * @param flags the flags for init_checkflags
 */
void do_find(dbref player, const char *name, const char *flags);

/**
 * The implementation of the @force command
 *
 * Defined in wiz.c
 *
 * This uses the global 'force_level' to keep track of how many
 * layers of force calls are in play.  That makes this not threadsafe,
 * and its very critical force_level is maintained correctly through the
 * call.
 *
 * Does a number of permission checks -- checks to see if zombies are
 * allowed if trying to force a THING.  Permissions are checked, and
 * F-Lock is checked as well.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param what the target to be forced
 * @param command the command to run.
 */
void do_force(int descr, dbref player, const char *what, char *command);

/*
 * G
 */

/**
 * Implementation of get command
 *
 * Defined in move.c
 *
 * This implements the 'get' command, including the ability to get
 * objects from a container.
 *
 * This does all the necessary permission checking.  If 'obj' is
 * provided, we will try to get 'what' out of container 'obj'.
 *
 * If 'obj' is empty, it will get it from the current room or get
 * it by DBREF if you're a wizard.
 *
 * A lot of the permission and messaging handling is delegated to common
 * calls could_doit and can_doit.
 *
 * @see could_doit
 * @see can_doit
 *
 * @param descr the descriptor of the person getting
 * @param player the person getting
 * @param what what to pick up
 * @param obj the container, if applicable, or ""
 */
void do_get(int descr, dbref player, const char *what, const char *obj);

/**
 * Implementation of give command
 *
 * Defined in rob.c
 *
 * This transfers "pennies" or value from one player to another.
 * Or, if you are a wizard, you can use this to take pennies or set the
 * value of an object.
 *
 * This does all necessary permission checks and works pretty much how
 * you would expect.  Players cannot cause another player to exceed
 * tp_max_pennies
 *
 * @param descr the descriptor of the giving player
 * @param player the giving player
 * @param recipient the person receiving the gift
 * @param amount the amount to transfer.
 */
void do_give(int descr, dbref player, const char *recipient, int amount);

/**
 * Implementation of the gripe command
 *
 * Defined in speech.c
 *
 * If the message is empty or NULL, if the player is a wizard, it will
 * display the gripe file.  Otherwise it will show an error message.
 *
 * If a message is provided, it will be logged to the gripe file.
 *
 * @param player the player running the gripe
 * @param message the message to gripe, or ""
 */
void do_gripe(dbref player, const char *message);

/*
 * H
 */

/**
 * Implementation of mpi, help, man, and news
 *
 * Defined in help.c
 *
 * This is the common command for dealing with different times of file
 * reads.  It understands "sub files" and "index files".  For
 * details about subfiles, see...
 *
 * @see show_subfile
 *
 * Index files are documented in the private function:
 *
 * @see index_file
 *
 * Basically, index files are a series of entries delimited by lines that
 * start with ~.  Each entry, except for the first one, starts with a
 * line of keywords delimited by | that can match the entry.
 *
 * @param player the player asking for help
 * @param dir the directory for subfiles
 * @param file the file for index files
 * @param topic the topic the player is looking up or ""
 * @param segment the range of lines to view - only works for subfiles
 */
void do_helpfile(dbref player, const char *dir, const char *file, char *topic, char *segment);

/*
 * I
 */

/**
 * Implementation of the info command
 *
 * This one works a little differently from help and its ilk.  If
 * no 'topic' is provided, then a directory listing is done to find
 * available info files in tp_file_info_dir unless directory listing
 * support isn't compiled in (DIR_AVAILABLE unset).
 *
 * Only subfiles are supported.  @see show_subfile
 *
 * @param player the player doing the info call
 * @param topic to look up or ""
 * @param seg the segment of the file - a range of lines to display.
 */
void do_info(dbref player, const char *topic, const char *seg);

/**
 * Implementation of inventory command
 *
 * Defined in look.c
 *
 * It is important to note that this is different from the look_contents
 * call because it is simpler.  A player can see everything in their inventory
 * regardless of darkness or player flags.
 *
 * @param player the player doing the call.
 */
void do_inventory(dbref player);

/*
 * K
 */

/**
 * Implementation of @kill command
 *
 * Defined in timequeue.c
 *
 * Kills a process or set of processes off the time queue.  "arg1" can
 * be either a process ID, a player name to kill all of a player's processs,
 * a program DBREF to kill all of a program's processes, or 'all' to kill
 * everything.
 *
 * This command does do permission checks.
 *
 * @see dequeue_process
 * @see free_timenode
 * @see dequeue_prog
 *
 * @param descr the descriptor of the calling player
 * @param player the player doing the call
 * @param arg1 what to kill
 */
void do_kill_process(int descr, dbref player, const char *arg1);

/*
 * L
 */

/**
 * Implementation of the leave command
 *
 * Defined in move.c
 *
 * This is for a player to leave a vehicle.  Does all the flag checking
 * to make sure the player is able to leave.
 *
 * @param descr the descriptor of the leaving player
 * @param player the player leaving
 */
void do_leave(int descr, dbref player);

/**
 * Implementation of @link command.
 *
 * This is in create.c
 *
 * Use this to link to a room that you own.  It also sets home for objects and
 * things, and drop-to's for rooms.  It seizes ownership of an unlinked exit,
 * and costs 1 penny plus a penny transferred to the exit owner if they aren't
 * you
 *
 * All destinations must either be owned by you, or be LINK_OK.  This does
 * check all permissions and, in fact, even the BUILDER bit.
 *
 * @param descr the descriptor of the calling user
 * @param player the calling player
 * @param thing_name the thing to link
 * @param dest_name the destination to link to.
 */
void do_link(int descr, dbref player, const char *name, const char *room_name);

/**
 * Implementation of the @list command
 *
 * This is defined in edit.c
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
void do_list(int descr, dbref player, const char *name, char *linespec);

/**
 * Implementation for 'look' command (aka read)
 *
 * This is implemented in look.c
 *
 * If name (the thing we're looking at) is "" or "here", this will be
 * routed to look_room.
 *
 * @see look_room
 *
 * Otherwise, match and look.  This does have a concept of look-traps
 * (prop-based pseudo-objects) which are checked if either nothing
 * is found (looktrap searched for on current room) or if the 'detail'
 * paramter is searched.  Look trap props are under DETAILS_PROPDIR,
 * which is usually _details.
 *
 * Look-traps support exit-style aliasing and will run MPI or @-style
 * MUF calls.
 *
 * @see exec_or_notify
 *
 * Permissions checking is done.
 *
 * @param descr the descriptor of the person running the call
 * @param player the player running the call
 * @param name The name of what is being looked at or "" for here
 * @param detail the "look trap" or detail prop to examine, or ""
 */
void do_look_at(int descr, dbref player, const char *name, const char *detail);

/*
 * M
 */

/**
 * Implementation of @mcpedit command
 *
 * Defined in mcp.c
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
void do_mcpedit(int descr, dbref player, const char *name);

/**
 * Implementation of @mcpprogram command
 *
 * Defined in mcp.c
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
void do_mcpprogram(int descr, dbref player, const char *name, const char *rname);

#ifndef NO_MEMORY_COMMAND
/**
 * Implementation of @memory command
 *
 * Defined in wiz.c
 *
 * This displays memory information to the calling user.  Note that it
 * does not do any permission checkings.
 *
 * Much like @usage, this uses a set of #define's to determine what
 * is actually displayed due to different features that operating
 * systems support.
 *
 * @param who the person running the command
 */
void do_memory(dbref who);
#endif

/**
 * Implemenetation of the motd command
 *
 * Defined in help.c
 *
 * If 'text' is not providd, or the player is not a wizard, then the
 * contents of the motd file will be displayed to the player.
 *
 * If the player is a wizard, 'text' can either be the string "clear"
 * to delete the MOTD, or the text you want to put in the MOTD file.
 *
 * Time is automatically added to teh start of the entry, and a dashed
 * line added to the end.  Dashed line will be written to the file
 * if cleared.
 *
 * @param player the player doing the call
 * @param text either "", "clear", or a new MOTD message.
 */
void do_motd(dbref player, char *text);

/**
 * Implementation of the move command
 *
 * The move command is kind of like the 'start' command in Windows or the
 * 'open' command in Mac ... it makes the player go through an exit, which
 * can be any exit that matches, even a MUF program.
 *
 * lev should usually be 0 - @see match_all_exits
 *
 * @param descr the descriptor of the moving player
 * @param player the player moving
 * @param direction the exit to search for
 * @param lev the match level, passed to match_all_exits via match_data md
 */
void do_move(int descr, dbref player, const char *direction, int lev);

/**
 * Implementation of the @mpitops command
 *
 * Defined in wiz.c
 *
 * This shows statistics about programs that have been running.  These
 * statistics are shown for top 'arg1' number of programs.  The default
 * is '10'.  'reset' can also be passed to reset the statistic numbers.
 *
 * This iterates over the entire DB and puts the programs in a
 * 'struct profnode' linked list.  For large numbers of 'arg1', this is
 * really inefficient since its sorting a linked list.
 *
 * This does not do any permission checking.
 *
 * @param player the player doing the call
 * @param arg1 Either a string containing a number, the word "reset", or ""
 */
void do_mpi_topprofs(dbref player, char *arg1);

/**
 * Implementation of the @muftops command
 *
 * Defined in wiz.c
 *
 * This shows statistics about programs that have been running.  These
 * statistics are shown for top 'arg1' number of programs.  The default
 * is '10'.  'reset' can also be passed to reset the statistic numbers.
 *
 * This iterates over the entire DB and puts the programs in a
 * 'struct profnode' linked list.  For large numbers of 'arg1', this is
 * really inefficient since its sorting a linked list.
 *
 * This does not do any permission checking.
 *
 * @param player the player doing the call
 * @param arg1 Either a string containing a number, the word "reset", or ""
 */
void do_muf_topprofs(dbref player, char *arg1);

/*
 * N
 */

/**
 * Implementation of the @name command
 *
 * Defined in set.c
 *
 * This performs a name change for a given 'name' to 'newname'
 *
 * In the case of players, "newname" should also have the player's
 * password in it after a space.
 *
 * This does all permission checking.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param name the name of object to alter
 * @param newname the new name (Which may also have a password)
 */
void do_name(int descr, dbref player, const char *name, char *newname);

/**
 * Implements the @newpassword command
 *
 * Defined in wiz.c
 *
 * Changes the password to 'password' on a given player 'name'.
 *
 * This does NOT do permission checks -- it does a limited set around
 * changing wizard passwords, but it doesn't check permissions for
 * regular players.
 *
 * @param player the player doing the command
 * @param name the player who's password we are changing
 * @param password the new password
 */
void do_newpassword(dbref player, const char *name, const char *password);

/*
 * O
 */

/**
 * Implementation of @open command
 *
 * This is defined in create.c
 *
 * This creates an exit in the current room and optionally links it
 * in one step.
 *
 * Supports registering if linkto has a = in it.  Exits are checked so
 * that they are only 7-bit ASCII.  Does permission and money checking,
 * but not builder bit checking.
 *
 * @param descr descriptor of person running command
 * @param player the player running the command
 * @param direction the exit name to make
 * @param linkto what to link to
 */
void do_open(int descr, dbref player, const char *direction, const char *linkto);

/**
 * Implementation of @owned command
 *
 * Defined in look.c
 *
 * Like do_find, this is underpinned by the checkflags system.
 * For details of how the flags work, see init_checkflags
 *
 * This does do permission checks.  Like @find, it iterates over the
 * entire database and supports a lookup cost.
 *
 * @see init_checkflags
 * @see do_find
 *
 * @param player the player doing the call
 * @param name the name of the player to check ownership of - or "" for me
 * @param flags flags passed into init_checkflags
 */
void do_owned(dbref player, const char *name, const char *flags);

/*
 * P
 */

/**
 * Implementation of page command
 *
 * Defined in speech.c
 *
 * This is the very basic page that comes with the MUCK.  First off,
 * it costs the 'tp_lookup_cost' to page.  It takes this money before
 * checking to see if the target player exists or is haven which is
 * a little rude but nobody uses the built-in page anyway.
 *
 * Supports HAVEN check on target.  Does not support multiple page targets.
 * Also supports a blank arg2 (blank message) which results in page sending
 * player's location instead of a message.
 *
 * @param player the player doing the page
 * @param arg1 the target player to page
 * @param arg2 the message to send to that player
 */
void do_page(dbref player, const char *arg1, const char *arg2);

/**
 * Implementation of @password command
 *
 * Defined in player.c
 *
 * This changes the player's password after doing some validation checks.
 * Old password must be valid and new password must not have spaces in it.
 *
 * @param player the player doing the password change
 * @param old the player's old password
 * @param newobj the player's new password.
 */
void do_password(dbref player, const char *old, const char *newobj);

/**
 * Implements the @pcreate command
 *
 * Defined in wiz.c
 *
 * Creates a player.  This is an incredibly thin wrapper around create_player;
 * in fact, this doesn no permission checks or any other kind of check.
 *
 * @see create_player
 *
 * @param player the player running the command
 * @param user the user to create
 * @param password the password to set on the new user.
 */
void do_pcreate(dbref player, const char *arg1, const char *arg2);

/**
 * Implementation of pose command
 *
 * Defined in speech.c
 *
 * The built-in pose command is somewhat basic, but it does smartly
 * determine if there should be a space after the player's name or
 * not based on is_valid_pose_separator
 *
 * @see is_valid_pose_separator
 *
 * @param player the player doing the pose
 * @param message the message being posed.
 */
void do_pose(dbref player, const char *message);

/**
 * Implementation of the @ps command
 *
 * This is defined in timequeue.c
 *
 * Shows running timequeue / process information to the given player.
 * Permission checking is done; wizards can see all processes, but players
 * can only see their own processes or processes running programs they own.
 *
 * @param player the player running the process list command
 */
void do_process_status(dbref player);

/**
 * Implementation of @program
 *
 * Defined in create.c
 *
 * First, find a program that matches that name.  If there's one, then we 
 * put the player into edit mode and do it.  Otherwise, we create a new object
 * for the player, and call it a program.
 *
 * The name must pass ok_object_name, but no permissions checks are done.
 *
 * @param descr the descriptor of the programmer
 * @param player the player trying to create the program
 * @param name the name of the program
 * @param rname the registration name, which may be "" if not registering.
 */
void do_program(int descr, dbref player, const char *name, const char *rname);

/**
 * Implementation of @propset
 *
 * Defined in set.c
 *
 * Propset can set props along with types.  'prop' can be of the
 * format: <type>:<property>:<value> or erase:<property>
 *
 * Type can be omitted and will default to string,  but there still
 * must be two ':'s in order to set a property.  Type can
 * be: string, integer, float, lock, dbref.  Types can be shortened
 * to just a prefix (s, i, f, str, lo, etc.)  Erase can also be
 * prefixed (e, er, etc.)
 *
 * This does do permission checking.
 *
 * @param descr the descriptor of the person setting
 * @param player the person setting
 * @param name the object we are setting a prop on
 * @param prop the prop specification as described above.
 */
void do_propset(int descr, dbref player, const char *name, const char *prop);

/*
 * R
 */

#ifdef USE_SSL
/**
 * Implementation of @reconfiguressl
 *
 * Defined in game.c
 *
 * This is a simple wrapper around the reconfigure_ssl call.  This does
 * not do any permission checking.
 *
 * @see reconfigure_ssl
 *
 * @param player the player doing the call
 */
void do_reconfigure_ssl(dbref player);
#endif

/**
 * Implementation of @recycle command
 *
 * Defined in create.c
 *
 * This is a wrapper around 'recycle', but it does a lot of additional
 * permission checks.  For instance, in order to recycle an object or
 * room, you must actually own it even if you a are a wizard.  This
 * makes the requirement for a two-step '@chown' then '@recycle' if you
 * wish to delete something as a wizard that you do not know.
 *
 * This is on purpose, I would imagine, to prevent accidents.
 *
 * @see recycle
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param name the object to recycle
 */
void do_recycle(int descr, dbref player, const char *name);

/**
 * Implementation of @register command
 *
 * Defined in set.c
 *
 * Does property registration similar to the 'classic' MUF @register
 * command.  Thus, the parameters are a little complex.
 *
 * By default, this operates on #0 and propdir base _reg.  This
 * can be modified by a prefix that will show up in arg1.
 *
 * Prefix can be #me for operating on 'me', propdir base _reg
 * Or it can be #prop [<target>:]<propdir> to target a given
 * prop directory instead.  If <target> is not provided, #0 will be
 * the default.
 *
 * If arg2 is "", then arg1 will be either just a prefix or a prefix
 * plus a propdir to list registrations in either the base propdir or
 * a subdir.
 *
 * If arg2 is set, then arg1 is the prefix + object name or just
 * the prefix and arg2 is the target registration name.  If there
 * is no object name, then the registration target is cleared.  Otherwise,
 * object is registered to the given name.
 *
 * See help @register on the MUCK for a more consise explanation :)
 *
 * This does permission checking.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param arg1 The first argument as described above
 * @param arg2 the second argument as described above.
 */
void do_register(int descr, dbref player, char *arg1, const char *arg2);

/**
 * Implementation of @relink command
 *
 * Defined in set.c
 *
 * This is basically the same as doing an @unlink followed by an @link.
 * Thus the details of what that entails is best described in those functions.
 *
 * @see do_unlink
 * @see do_link
 *
 * This does some checking to make sure that the target linking will
 * work, and it does do permission checking.
 *
 * @param descr the descriptor of the person running the command
 * @param player the person running the command
 * @param thing_name the thing to relink
 * @param dest_name the destination to relink to.
 */
void do_relink(int descr, dbref player, const char *thing_name, const char *dest_name);

/**
 * Implementation of the @restart command
 *
 * Permissions are checked and non-wizard players are logged.  This will
 * trigger the MUCK to try and restart itself.  The implementation of this
 * is in interface.c
 *
 * @param player the player trying to do the restart
 */
void do_restart(dbref player);

/**
 * Implementation of @restrict command
 *
 * Defined in game.c
 *
 * This does not do any permission checking.  Arg can be either "on" to
 * turn on wiz-only mode, "off" to turn it off, or anything else (such
 * as "" or a typo) to display current restriction status.
 *
 * @param player the player doing the call
 * @param arg the argument as described above
 */
void do_restrict(dbref player, const char *arg);

/*
 * S
 */

/**
 * Implementation of @sanchange command
 *
 * Defined in sanity.c
 *
 * This enables the manual editing of database object fields in a very
 * raw fashion.  It is truly a bad idea to mess with as this can cause
 * the very sanity problems its designed to fix.
 *
 * What this can manipulate is limited to:
 *
 * - the 'next' linked list dbref
 * - the 'exits' linked list dbref
 * - the 'contents' linked list dbref
 * - the 'location' dbref
 * - the 'owner' dbref
 * - the 'home' dbref
 *
 * Commands are space delimited and look something like:
 *
 * #dbref command #dbref
 *
 * Where the first dbref is the object to modify and the second dbref
 * is what to set to.  For instance, "#12345 home #4321"
 *
 * No permission checking is done.
 *
 * @param player the player doing the call
 * @param command the command, as described above
 */
void do_sanchange(dbref player, const char *command);

/**
 * Implementation of @sanfix command
 *
 * Defined in sanity.c
 *
 * This does a sanity check, and tries to repair any problems it finds.
 * Theoretically there are problems this can't fix.  do_sanity does sort
 * of a read-only version of this.
 *
 * @see do_sanity
 *
 * The scope of problems it finds are objects that have invalid contents,
 * exits, missing names.  It is overall pretty basic.  Doesn't cover
 * props at all.
 *
 * This does no permission checking.
 *
 * @param player the player doing the sanfix call.
 */
void do_sanfix(dbref player);

/**
 * Implementation of the @sanity command
 *
 * Defined in sanity.c
 *
 * This does sanity checks on the entire database.  This is pretty compute
 * intensive as it crawls through everything.  No permission checks are done
 * by this command.
 *
 * The details of what a sanity check involves can be found in the
 * various static methods in sanity.c -- this mostly involves reference
 * checking and other somewhat basic checks.
 *
 * Prints status for every 10,000 refs.
 *
 * player can be NOTHING to output to log file, or AMBIGUOUS to output
 * to stderr.  Otherwise, output is sent to the indicated player dbref.
 *
 * @param player the player to notify, NOTHING, or AMBIGUOUS as noted above.
 */
void do_sanity(dbref player);

/**
 * Implementation of the say command.
 *
 * Defined in speech.c
 *
 * There is absolutely nothing fancy here.  The player and the room get
 * slightly different variants of the same message.
 *
 * @param player the player talking
 * @param message the message being said
 */
void do_say(dbref player, const char *message);

/**
 * Implementation of score command
 *
 * Defined in pennies.c
 *
 * Which is how you see how many "pennies" you have.
 *
 * @param player the player doing the call
 */
void do_score(dbref player);

/**
 * Implementation of @set command
 *
 * Defined in set.c
 *
 * This can set flags or props.  The determining factor is if there is
 * a PROP_DELIMITER (:) character in the 'flag' string.  The special
 * case of ':clear' will clear all properties that the user controls
 * on teh object (i.e. players cannot unset wizprops with that).
 *
 * Otherwise, it will look at the flag and and set whatever desired flag
 * on the object in question.  Thisi s done with
 * string_prefix("FULL_NAME", flag) basically, so any fraction of a flag
 * name will match.  It supports "!FLAGNAME" as well to unset.
 *
 * Permission checking is done.
 *
 * @param descr the descriptor of the person doing the call.
 * @param player the player doing the call
 * @param name the name of the object to modify
 * @param flag the flag or property to set.
 */
void do_set(int descr, dbref player, const char *name, const char *flag);

/**
 * Implementation of @shutdown command
 *
 * Defined in game.c
 *
 * Permissions are checked and non-wizard players are logged.  This will
 * trigger the MUCK to shut itself down  The implementation of this
 * is in interface.c
 *
 * @param player the player trying to do the restart
 */
void do_shutdown(dbref player);

/**
 * Implementation of the @stats command
 *
 * Defined in wiz.c
 *
 * This displays stats on what is in the database, optionally showing
 * stats on what a given player owns.
 *
 * The stats returned are basic counts -- numbeer of rooms, objects, etc.
 * It loops over the entire DB to get this information.
 *
 * This does do permission checking
 *
 * @param player the player doing the call
 * @param name the name of the person to get stats for, or "" for global stats
 */
void do_stats(dbref player, const char *name);

/**
 * Implementation of the sweep command
 *
 * Defined in look.c
 *
 * Does a 'security sweep' of a given location, checking it for listening
 * players and objects.  The parameter given is either a room reference
 * or it can be "" to default for here.  This does do permission checking.
 *
 * Security sweeps are done up the environment chain, looking for anything
 * that might be listening or common communication commands that may be
 * overridden in the room and thus used for spying.
 *
 * This has nothing to do with the sweep that removes sleeping players.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param name the room to scan or "" to default to "here"
 */
void do_sweep(int descr, dbref player, const char *name);

/*
 * T
 */

/**
 * Implementation of @teledump command
 *
 * The definition resides in wiz.c
 *
 * @teledump does a base64 encoded dump of the entire database and the
 * associated macros and MUF in a fashion that can be consumed by an unpacker
 * script (a python unpacker will be provided as part of this).
 *
 * It is a way to preserve the vital data of a MUCK but it does not
 * send over irrelevant files.
 *
 * For simplicity sake, it dumps the DB on disk, so @dump before running
 * this command to get the latest.
 *
 * @param descr the player's descriptor
 * @param player the player doing the teledump
 */
void
do_teledump(int descr, dbref player);

/**
 * Implementation of the @teleport command
 *
 * The definition resides in wiz.c
 *
 * This is the MUCK's @teleport command.  It does do permission checking
 * and sometimes that gets pretty detailed -- the rules for teleporting
 * different things vary slightly.  However, in general, the player must
 * control both the object to be teleported and the destination -- or
 * the destination must be LINK_OK (things) or ABODE (rooms) based on what
 * is being teleported.
 *
 * The nuances beyond that are largely uninteresting as they have to do
 * with data integrity more than "business logic".  See the can_link_to
 * call as that is used to determine who can go where for THINGS, ROOMS,
 * and PROGRAMS.
 *
 * @see can_link_to
 *
 * Players cannot teleport other players if they are not a wizard.
 *
 * Garbage cannot be teleported.
 *
 * @param descr the descriptor of the person running the command
 * @param player the player running the command
 * @param arg1 The person being teleported, or the destination to teleport
 *             'me' to if arg2 is empty stirng
 * @param arg2 the destination to deleport to or "" as noted for arg1
 */
void do_teleport(int descr, dbref player, const char *arg1, const char *arg2);

/**
 * Implementation of the @toad command
 *
 * Defined in wiz.c
 *
 * Toading is the deletion of a player.  This is a wrapper around
 * toad_player, so the mechanics of toading are in that call.
 *
 * @see toad_player
 *
 * This does all the permission checking around toading and verifies
 * the target player is valid for toading.  This will not allow you to
 * toad yourself which is a protection against wiping all players out of
 * a DB.
 *
 * @param descr the descriptor of the person doing the toading
 * @param player the player doing the toading
 * @param name the name of the person being toaded
 * @param recip the person to receive the toaded player's owned things or ""
 *
 * If no toad recipient is provided, then it goes to the system default.
 */
void do_toad(int descr, dbref player, const char *name, const char *recip);

/**
 * Implementation of the @tops command
 *
 * Defined in wiz.c
 *
 * This shows statistics about programs that have been running.  These
 * statistics are shown for top 'arg1' number of programs.  The default
 * is '10'.  'reset' can also be passed to reset the statistic numbers.
 *
 * This iterates over the entire DB and puts the programs in a
 * 'struct profnode' linked list.  For large numbers of 'arg1', this is
 * really inefficient since its sorting a linked list.
 *
 * This does not do any permission checking.
 *
 * @param player the player doing the call
 * @param arg1 Either a string containing a number, the word "reset", or ""
 */
void do_topprofs(dbref player, char *arg1);

/**
 * Implementation of @trace command
 *
 * Defined in look.c
 *
 * This takes a room ref or "" for here and goes up the environment
 * chain until the global environment room is reached or we've traversed
 * 'depth' number of parent rooms.  Each room in the chain is displayed
 * to the user.
 *
 * @param descr the descriptor of the person doing the call
 * @param player the player doing the call
 * @param name the room dbref or ""
 * @param depth the number of rooms to iterate over.
 */
void do_trace(int descr, dbref player, const char *name, int depth);

/**
 * Implementation of @tune command
 *
 * Defined in tune.c
 *
 * This does a lot of different things based on if 'parmname' and
 * 'parmval' are set and to what.
 *
 * This does SOME permission checking; it checks to see if the player is
 * being forced or not, and checks if their MUCKER level allows them to
 * read/write parameters as appropriate.  However it doesn't stop people
 * with no MUCKER/Wizard from running it.
 *
 * If parmname and parmval are both set, or parmname starts with the
 * TP_FLAG_DEFAULT symbol (usually %) then the parameter will be set.
 *
 * @see tune_setparm
 *
 * If the parmname is "save" or "load", it will save or load parameters
 * from file accordingly.
 *
 * Otherwise, parmname should be either "" to list all @tunes or
 * a simple regex can be used to show only certain parameters.
 *
 * The parmname "info" will display more detained tune information.
 *
 * @param player the player running the command
 * @param parmname command / parameter name - see description
 * @param parmval Parameter value to set, or ""
 */
void do_tune(dbref player, char *parmname, char *parmval);

/*
 * U
 */

/**
 * Implementation of the @unbless command
 *
 * Defined in wiz.c
 *
 * This does not handle basic permission checking.  The propname can have
 * wildcards in it.
 *
 * Two kinds of wildcards are understood; * and **.  ** is a recursive
 * wildcard, * is just a 'local' match.
 *
 * @param descr the descriptor of the user doing the command
 * @param player the player doing the command
 * @param what the object being operated on
 * @param propname the name of the prop to bless.
 */
void do_unbless(int descr, dbref player, const char *target, const char *propname);

/**
 * Implementation of @uncompile command
 *
 * Implemented in compile.c
 *
 * Removes all compiled programs from memory.  Does not do permission
 * checking.
 *
 * @param player the player to notify when the uncompile is done.
 */
void do_uncompile(dbref player);

/**
 * Implementation of @unlink command
 *
 * Defined in set.c
 *
 * This is a thin wrapper around _do_unlink (a private function) that does
 * all the logic.  As it is a private function, the documentation will
 * be duplicated here:
 *
 * If this is an exit, the exit will be unlinked.  If this is a room, the
 * dropto will be removed.  If this is a thing, its home will be reset
 * to its owner.  If this is a player, its home will be reset to player start.
 *
 * This does do permission checking.
 *
 * @see _do_unlink
 *
 * @param descr the descriptor of the calling player
 * @param player the calling player
 * @param name the name of the thing to unlink
 */
void do_unlink(int descr, dbref player, const char *name);

/**
 * Implementation of @unlock command
 *
 * Defined in set.c
 *
 * Unlocks a given object.  This does do permission checks.
 *
 * @param descr the descriptor of the person unlocking the object.
 * @param player the player unlocking the object.
 * @param name the name to find
 */
void do_unlock(int descr, dbref player, const char *name);

/**
 * Implementation of uptime command
 *
 * Defined in look.c
 *
 * Just outputs uptime information to player.
 *
 * @param player the player doing the call
 */
void do_uptime(dbref player);

#ifndef NO_USAGE_COMMAND
/**
 * Implementation of the @usage command
 *
 * Defined in wiz.c
 *
 * This gives a bunch of information about the FuzzBall MUCK process.
 * Back in "the day" a lot of this information was not available on
 * all operating systems, so the NO_USAGE_COMMAND and HAVE_RUSAGE
 * defines were born to disable this on non-participating systems.
 *
 * There are no permissions checks done; this is usually wizard only though.
 *
 * @param player the player doing the call.
 */
void do_usage(dbref player);
#endif

/*
 * V
 */

/**
 * Implementation of do_version
 *
 * Defined in help.c
 *
 * Displays version information to player.
 *
 * @param player
 */
void do_version(dbref player);

/*
 * W
 */

/**
 * Implementation of @wall command
 *
 * Defined in speech.c
 *
 * This broadcasts a message to everyone on the MUCK, as a "shout"
 * using a format similar to "say".  This does NOT do permission
 * checking.
 *
 * @param player the player shouting
 * @param message the message being sent
 */
void do_wall(dbref player, const char *message);

/**
 * Implementation of whisper
 *
 * Defined in speech.c
 *
 * The built-in whisper is incredibly basic.  Wizard players are able
 * to remote whisper with by prefixing player name with a *
 *
 * You can only whisper a single player at a time with this command,
 * unlike most MUF based whispers.
 *
 * @param descr the descriptor of the player whispering
 * @param player the player whispering
 * @param arg1 the target to whisper to
 * @param arg2 the whisper message to send
 */
void do_whisper(int descr, dbref player, const char *arg1, const char *arg2);


#endif /* !COMMANDS_H */
