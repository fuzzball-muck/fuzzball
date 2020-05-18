/** @file p_regex.h
 *
 * Declaration of regex related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_REGEX_H
#define P_REGEX_H

#include "interp.h"

/*
 * The following defines can be bitwise OR'd together to use multple flags
 */

/**
 * @var MUF_RE_ICASE
 *      Case insensitive search
 */
#define MUF_RE_ICASE    1

/**
 * @var MUF_RE_ALL
 *      Substitute ALL on a regex substitution call
 */
#define MUF_RE_ALL      2

/**
 * @var MUF_RE_EXTENDED
 *      Enable PCRE extended features
 */
#define MUF_RE_EXTENDED 4

/**
 * Implementation of MUF REGEXP
 *
 * Consumes some text, a Perl-style pattern, and an integer flags.
 * Returns a list of submatch values and a list of submatch indexes.
 *
 * The flags can be a bitwise OR of the MUF_RE_* defines.  See the comments
 * for those defines to see what they do exactly.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_regexp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF REGSUB
 *
 * Consumes some text, a Perl-style pattern, a replacement string,
 * and an integer flags.  Returns a string that has replaced the
 * pattern with the replacement string.
 *
 * The flags can be a bitwise OR of the MUF_RE_* defines.  See the comments
 * for those defines to see what they do exactly.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_regsub(PRIM_PROTOTYPE);

/**
 * Implementation of MUF REGSPLIT
 *
 * Consumes a string, a pattern, and an integer which is the addition
 * of MUF_RE_* defines.  Returns a list which is the string split by
 * the pattern.  It will push empty pieces on the stack for consecutive
 * matches.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_regsplit(PRIM_PROTOTYPE);

/**
 * Implementation of MUF REGSPLIT_NOEMPTY
 *
 * Consumes a string, a pattern, and an integer which is the addition
 * of MUF_RE_* defines.  Returns a list which is the string split by
 * the pattern.  It will not put empty pieces on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_regsplit_noempty(PRIM_PROTOTYPE);

/**
 * Primitive callback functions
 */
#define PRIMS_REGEX_FUNCS prim_regexp, prim_regsub, prim_regsplit, \
        prim_regsplit_noempty

/**
 * Primitive names - must be in same order as the callback functions
 */
#define PRIMS_REGEX_NAMES "REGEXP", "REGSUB", "REGSPLIT", "REGSPLIT_NOEMPTY"

#endif /* !P_REGEX_H */
