/** @file p_strings.h
 *
 * Declaration of string related primitives for MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef P_STRINGS_H
#define P_STRINGS_H

#include "interp.h"

/**
 * Implementation of MUF FMTSTRING
 *
 * This is the MUF equivalent of sprintf.
 *
 * The substitutions should be on the stack in reverse order -- the
 * first substitution will be the second from the top-most item on the
 * stack; the topmost item is the string with replacement tokens in it.
 *
 * All items are consumed, then the finished string is returned.  See
 * the MUF man page for the list of substitutions.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_fmtstring(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PRIM_SPLIT
 *
 * Consumes a string and a match string.  Splits the string at the first
 * occurance of the match and puts the two pieces on the stack.  If the
 * match string is not found, it pushes the original string and an empty
 * string on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_split(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PRIM_RSPLIT
 *
 * Consumes a string and a match string.  Splits the string at the last
 * occurance of the match and puts the two pieces on the stack.  If the
 * match string is not found, it pushes the original string and an empty
 * string on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rsplit(PRIM_PROTOTYPE);

/**
 * Implementation of MUF CTOI
 *
 * Consumes a string and converts the first character in the string to
 * its ASCII equivalent, putting that number on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ctoi(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ITOC
 *
 * Consumes an integer and returns a string ASCII character or an empty
 * string if i is not a valid printable ASCII character.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_itoc(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STOD
 *
 * Consumes a string and tries to convert it to a dbref type.  Both with
 * and without leading # are supported.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_stod(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MIDSTR
 *
 * Consumes a string and two integers.  Returns a substring starting
 * with the first integer and with a length of the second string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_midstr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NUMBER?
 *
 * Consumes a string and returns boolean true if the string contains a
 * number.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_numberp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRINGCMP
 *
 * Consumes two strings and compares them, case insensitive.  Returns
 * the same as strcasecmp, pushing result on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_stringcmp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRCMP
 *
 * Consumes two strings and compares them, case sensitive.  Returns
 * the same as strcmp, pushing result on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strcmp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRNCMP
 *
 * Consumes two strings plus an integer and compares the first characters
 * of both strings, up to the count provided as the integer parameter.
 * Case sensitive.  Returns the same as strncmp, pushing result on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strncmp(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRCUT
 *
 * Consumes a string and an integer, and splits the string at the
 * integer number of characters.  Pushes two strings on the stack.
 * The first string will be empty if the integer is 0, and the second
 * string will be empty if the integer is greater than the source string's
 * length.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strcut(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRLEN
 *
 * Consumes a string and pushes the length of it onto the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strlen(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRCAT
 *
 * Consumes two strings and puts the conjoined string onto the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strcat(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ATOI
 *
 * Convert string to integer.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_atoi(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NOTIFY
 *
 * Consumes a dbref and a string, and transmits that string to that ref.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_notify(PRIM_PROTOTYPE);
void prim_notify_exclude(PRIM_PROTOTYPE);
void prim_intostr(PRIM_PROTOTYPE);
void prim_explode(PRIM_PROTOTYPE);
void prim_explode_array(PRIM_PROTOTYPE);
void prim_subst(PRIM_PROTOTYPE);
void prim_instr(PRIM_PROTOTYPE);
void prim_rinstr(PRIM_PROTOTYPE);
void prim_pronoun_sub(PRIM_PROTOTYPE);
void prim_toupper(PRIM_PROTOTYPE);
void prim_tolower(PRIM_PROTOTYPE);
void prim_unparseobj(PRIM_PROTOTYPE);
void prim_smatch(PRIM_PROTOTYPE);
void prim_striplead(PRIM_PROTOTYPE);
void prim_striptail(PRIM_PROTOTYPE);
void prim_stringpfx(PRIM_PROTOTYPE);
void prim_strencrypt(PRIM_PROTOTYPE);
void prim_strdecrypt(PRIM_PROTOTYPE);
void prim_textattr(PRIM_PROTOTYPE);
void prim_tokensplit(PRIM_PROTOTYPE);
void prim_ansi_strlen(PRIM_PROTOTYPE);
void prim_ansi_strcut(PRIM_PROTOTYPE);
void prim_ansi_strip(PRIM_PROTOTYPE);
void prim_ansi_midstr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ARRAY_FMTSTRINGS
 *
 * This is the MUF equivalent of sprintf, except allowing *multiple*
 * substitution sets to run against the singular format string, returning
 * a list of results.
 *
 * This is actually very complex, more than can easily be explained in
 * a header; please see the MUF man page for the (extremely lengthly)
 * details.
 *
 * It consumes a list containing dictionaries and a string, and puts a
 * list of strings on the stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_array_fmtstrings(PRIM_PROTOTYPE);

/**
 * Implementation of MUF NOTIFY_NOLISTEN
 *
 * Consumes a dbref and a string, then sends the string to that ref.
 * _listen properties are not triggered.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_notify_nolisten(PRIM_PROTOTYPE);
void prim_notify_secure(PRIM_PROTOTYPE);
void prim_instring(PRIM_PROTOTYPE);
void prim_rinstring(PRIM_PROTOTYPE);
void prim_md5hash(PRIM_PROTOTYPE);
void prim_strip(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TELL
 *
 * Consumes a string and sends it to the calling user.  Functionally the same
 * as prim_notify with 'me' as the target, but under the hood, does not
 * use prim_notify's code at all.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_tell(PRIM_PROTOTYPE);
void prim_otell(PRIM_PROTOTYPE);
void prim_pose_separatorp(PRIM_PROTOTYPE);

#define PRIMS_STRINGS_FUNCS prim_numberp, prim_stringcmp, prim_strcmp,        \
    prim_strncmp, prim_strcut, prim_strlen, prim_strcat, prim_atoi,           \
    prim_notify, prim_notify_exclude, prim_intostr, prim_explode, prim_subst, \
	prim_instr, prim_rinstr, prim_pronoun_sub, prim_toupper, prim_tolower,    \
	prim_unparseobj, prim_smatch, prim_striplead, prim_striptail,             \
	prim_stringpfx, prim_strencrypt, prim_strdecrypt, prim_textattr,          \
	prim_midstr, prim_ctoi, prim_itoc, prim_stod, prim_split, prim_rsplit,    \
	prim_fmtstring, prim_tokensplit, prim_ansi_strlen, prim_ansi_strcut,      \
	prim_ansi_strip, prim_ansi_midstr, prim_explode_array,                    \
	prim_array_fmtstrings, prim_notify_nolisten, prim_notify_secure,	\
	prim_instring, prim_rinstring, prim_md5hash, prim_strip,	\
	prim_tell, prim_otell, prim_pose_separatorp

#define PRIMS_STRINGS_NAMES "NUMBER?", "STRINGCMP", "STRCMP",  \
    "STRNCMP", "STRCUT", "STRLEN", "STRCAT", "ATOI",           \
    "NOTIFY", "NOTIFY_EXCLUDE", "INTOSTR", "EXPLODE", "SUBST", \
    "INSTR", "RINSTR", "PRONOUN_SUB", "TOUPPER", "TOLOWER",    \
	"UNPARSEOBJ", "SMATCH", "STRIPLEAD", "STRIPTAIL",          \
	"STRINGPFX", "STRENCRYPT", "STRDECRYPT", "TEXTATTR",       \
	"MIDSTR", "CTOI", "ITOC", "STOD", "SPLIT", "RSPLIT",       \
	"FMTSTRING", "TOKENSPLIT", "ANSI_STRLEN", "ANSI_STRCUT",   \
	"ANSI_STRIP", "ANSI_MIDSTR", "EXPLODE_ARRAY",              \
	"ARRAY_FMTSTRINGS", "NOTIFY_NOLISTEN", "NOTIFY_SECURE",	   \
	"INSTRING", "RINSTRING", "MD5HASH", "STRIP",   \
	"TELL", "OTELL", "POSE-SEPARATOR?"

#endif /* !P_STRINGS_H */
