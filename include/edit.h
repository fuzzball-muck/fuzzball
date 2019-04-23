/** @file edit.h
 *
 * Header declaring the MUCK's code editing interface for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef EDIT_H
#define EDIT_H

#include <stdio.h>

#include "config.h"
#include "inst.h"

/* Structure defining macros */
struct macrotable {
    char *name;                 /* Name of macro              */
    char *definition;           /* Code for macro             */
    dbref implementor;          /* dbref of macro creator     */
    struct macrotable *left;    /* Binary tree implementation */
    struct macrotable *right;   /* Binary tree implementation */
};

/* Maximum arguments for editor commands */
#define MAX_ARG 2

/* Exit insert mode character */
#define EXIT_INSERT "."

/* Command strings -- the defines are self-descriptive enough */
#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
#define CANCEL_EDIT_COMMAND 'x'
#define COMPILE_COMMAND 'c'
#define LIST_COMMAND   'l'
#define EDITOR_HELP_COMMAND 'h'
#define KILL_COMMAND 'k'
#define SHOW_COMMAND 's'
#define SHORTSHOW_COMMAND 'a'
#define VIEW_COMMAND 'v'
#define UNASSEMBLE_COMMAND 'u'
#define NUMBER_COMMAND 'n'
#define PUBLICS_COMMAND 'p'

/**
 * @var The head of the macrotable binary tree
 */
extern struct macrotable *macrotop;

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
void chown_macros(struct macrotable *node, dbref from, dbref to);

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
void edit_program(dbref player, dbref program);

/**
 * Recursively free memory associated with the macro table
 *
 * @param node the macrotable node to free
 */
void purge_macro_tree(struct macrotable *node);

/**
 * Frees an entire linked list of struct line's
 *
 * @private
 * @param l the head of the list to delete
 */
void free_prog_text(struct line *l);

/**
 * Allocate a new line structure
 *
 * @return newly allocated struct line
 */
struct line *get_new_line(void);

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
void interactive(int descr, dbref player, const char *command);

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
void list_program(dbref player, dbref program, int *oarg, int argc, int comment_sysmsg);

/**
 * Locates a macro with a given name in the macrotable.
 *
 * Returns the macro definition or NULL if not found.  The returned memory
 * belongs to the caller -- remember to free it!
 *
 * @param match the string name to find, case insensitive
 * @return the macro definition or NULL if not found.
 */
char *macro_expansion(const char *match);

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
void macrodump(struct macrotable *node, FILE * f);

/**
 * Load the macros from a file
 *
 * The file format is described in the save function.
 *
 * @see macrodump
 *
 * @param f the file handle to load from
 */
void macroload(FILE * f);

/**
 * Read a program from the disk and return it as a linked list of struct line
 *
 * @param i the dbref of the program to load
 * @return the program text as a linked list of struct line
 */
struct line *read_program(dbref i);

/**
 * Write program text to disk
 *
 * @param first the head of the linked list of lines to write
 * @param i the dbref of the program to write
 */
void write_program(struct line *first, dbref i);


#endif /* !EDIT_H */
