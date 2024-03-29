/** @file compile.h
 *
 * Header for the various functions that handles the MUF compiler.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef COMPILE_H
#define COMPILE_H

#include <stddef.h>

#include "config.h"

#define MUF_AUTHOR_PROP         "_author"   /**< Author name string location */
#define MUF_DOCCMD_PROP         "_docs"     /**< Command to list docs */
#define MUF_ERRCOUNT_PROP       ".debug/errcount"   /**< error count */
#define MUF_LASTCRASH_PROP      ".debug/lastcrash"  /**< last crash timestamp */
#define MUF_LASTCRASHTIME_PROP  ".debug/lastcrashtime"  /**< last crash time str */
#define MUF_LASTERR_PROP        ".debug/lasterr"    /**< last error */
#define MUF_LIB_VERSION_PROP    "_lib-version"      /**< Library version */
#define MUF_NOTE_PROP           "_note"             /**< Notes */
#define MUF_VERSION_PROP        "_version"          /**< Version */

/**
 * @var IN_FOR
 *      integer primitive ID for FOR primitive
 */
extern int IN_FOR;

/**
 * @var IN_FOREACH
 *      integer primitive ID for FOREACH primitive
 */
extern int IN_FOREACH;

/**
 * @var IN_FORITER
 *      integer primitive ID for FORITER primitive
 */
extern int IN_FORITER;

/**
 * @var IN_FORPOP
 *      integer primitive ID for FORPOP primitive
 */
extern int IN_FORPOP;

/**
 * @var IN_TRYPOP
 *      integer primitive ID for TRYPROP primitive
 */
extern int IN_TRYPOP;

/**
 * Free the memory used by the primitive hash
 */
void clear_primitives(void);

/**
 * Compile MUF code associated with a given dbref
 *
 * Live the dream, compile some MUF.  According to the original docs, this
 * does piece-meal tokenization parsing and backward checking.
 *
 * This takes some program text associated with 'program_in', converts it
 * to INTERMEDIATE's, and then does some optimizations before turning it
 * into an array of struct inst's.
 *
 * Once compiled, the program is ready to run.
 *
 * A few notes:
 *
 * - When compile starts, it kills any running copies of the program.
 * - If the program is both set A (ABODE/AUTOSTART) and owned by a wizard,
 *   it will be run automatically if successfully compiled.
 * - force_err_display when true will display all compile errors to the
 *   player.  Error messages are still displayed anyway if the player
 *   is INTERACTIVE but not READMODE (which I believe means they are in the
 *   \@program/\@edit editor).
 *
 * @param descr the descriptor of the person compiling
 * @param in_player the player compiling
 * @param in_program the program to compile
 * @param force_err_disp boolean - true to always show compile errors
 */
void do_compile(int descr, dbref in_player, dbref in_program,
                int force_err_disp);

/**
 * Free ("uncompile") unused programs
 *
 * An unused program is one who's program object has a ts_lastused time
 * that is older than tp_clean_interval seconds and also is not set
 * ABODE (Autostart) or INTERNAL.
 *
 * This scans over the entire database and is thus kind of expensive
 */
void free_unused_programs(void);

/**
 * Return primitive instruction number
 *
 * @param token the primitive token name such as "GETPROPSTR", etc.
 * @return integer token instruction number
 */
int get_primitive(const char * token);

/**
 * Initialize the hash of primitives
 *
 * This must run before the compile will work.  It first clears anything
 * currently in the primitive hash, then loads all the primitives from
 * base_inst into the hash.
 *
 * This also loads the value of variables IN_FORPOP, IN_FORITER, IN_FOR,
 * IN_FOREACH, and IN_TRYPOP.  The number of primitives is logged via
 * log_status
 *
 * @see log_status
 */
void init_primitives(void);

/**
 * See if 'token' is a primitive
 *
 * @param token the token to check
 * @return boolean true if 'token' is a primitive, false otherwise
 */
int primitive(const char *token);

/**
 * Calculate the in-memory size of a program
 *
 * This may be 0 if the program is not loaded (PROGRAM_CODE(prog) is NULL)
 * Otherwise it is a (hopefully) accurate representation of a program's
 * memory usage.
 *
 * @param prog the dbref of the program we are size checking
 * @return the size in bytes of consumed memory
 */
size_t size_prog(dbref prog);

/**
 * Delete a program from memory
 *
 * "Uncompile" is a really awkward way to put this, because (to me) it
 * sounds a lot like "decompile".  It has absolutely nothing to do with
 * decompiling.  "free_program" is probably more accurate.
 *
 * @param i the program ref to decompile
 */
void uncompile_program(dbref i);

#endif /* !COMPILE_H */
