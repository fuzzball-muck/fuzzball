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

/**
 * Implementation of MUF NOTIFY_EXCLUDE
 *
 * Takes a room ref and stack range of dbrefs (dbrefs then integer count),
 * and finally a string to send to that room except for the given refs.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_notify_exclude(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INTOSTR
 *
 * Converts whatever is on the stack to a string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_intostr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXPLODE
 *
 * Consumes two strings, splitting the first by the second, and making
 * a stack range of string pieces.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_explode(PRIM_PROTOTYPE);

/**
 * Implementation of MUF EXPLODE_ARRAY
 *
 * Consumes two strings, splitting the first by the second, and making
 * a list array of string components.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_explode_array(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SUBST
 *
 * Consumes three strings.  The subject, the string to find, and the string
 * to substitute it with.  Returns the first string with substitutions made.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_subst(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INSTR
 *
 * Consumes two strings and returns an integer representing the first
 * position the second string is found in the first string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_instr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF RINSTR
 *
 * Consumes two strings and returns an integer representing the last
 * position the second string is found in the first string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rinstr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF PRONOUN_SUB
 *
 * Takes a ref and a string to do pronoun substitutions in.  This is a thin
 * wrapper around pronoun_substitute.  @see pronoun_substitute
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_pronoun_sub(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TOUPPER
 *
 * Consumes a string and puts an all-upper-case version of it back on the
 * stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_toupper(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TOLOWER
 *
 * Consumes a string and puts an all-lower-case version of it back on the
 * stack.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_tolower(PRIM_PROTOTYPE);

/**
 * Implementation of MUF UNPARSEOBJ
 *
 * Consumes a dbref and puts a string on the stack.  The string is the name
 * with the ref and flags appended to it like "One(#1PW)"
 *
 * @see unparse_flags
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_unparseobj(PRIM_PROTOTYPE);

/**
 * Implementation of MUF SMATCH
 *
 * Consumes two strings; a string and a match pattern.  Returns boolean
 * true if the string matches the patterns.  Uses Fuzzball's home brew
 * patterns.
 *
 * @see equalstr
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_smatch(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRIPLEAD
 *
 * Consumes a string and returns that string with leading strings stripped.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_striplead(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRIPTAIL
 *
 * Consumes a string and returns that string with ending strings stripped.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_striptail(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRINGPFX
 *
 * Consumes two strings.  Returns boolean true if the second string is a
 * prefix of the first string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_stringpfx(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRENCRYPT
 *
 * Consumes two strings; a string and an encryption key.  Returns
 * encryped string.
 *
 * @see strencrypt
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strencrypt(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRDECRYPT
 *
 * Consumes two strings; a string and an encryption key.  Returns
 * decryped string.
 *
 * @see strencrypt
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_strdecrypt(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TEXTATTR
 *
 * Consumes two strings; a plain text string and as tring containing
 * ANSI attributs.  Returns a string with the proper attributes.  The
 * ANSI attributes are strings, comma separated.  Look at the man
 * page to see the list of attributes, it will be more up to date
 * than this comment.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_textattr(PRIM_PROTOTYPE);

/**
 * Implementation of MUF TOKENSPLIT
 *
 * Consumes three strings; a string to split, the delimiter, and an
 * escape character.  Instead of splitting on the whole delimiter string,
 * any single character in the delimiter string can be used for a split.
 *
 * Returns the unescaped string before the found character, the raw
 * string (escaped) after that character, and the character that
 * was found.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_tokensplit(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ANSI_STRLEN
 *
 * Consumes a string and returns its length after stripping all ANSI escapes
 * within it.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ansi_strlen(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ANSI_STRCUT
 *
 * Consumes a string and an integer, and splits the string at the
 * integer number of characters.  Pushes two strings on the stack.
 * The first string will be empty if the integer is 0, and the second
 * string will be empty if the integer is greater than the source string's
 * length.
 *
 * This version differs from prim_strcut in that it ignores ANSI escapes.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ansi_strcut(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ANSI_STRIP
 *
 * Consumes a string and returns a string with all ANSI escapes removed.
 *
 * @see strip_ansi
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_ansi_strip(PRIM_PROTOTYPE);

/**
 * Implementation of MUF ANSI_MIDSTR
 *
 * Consumes a string and two integers.  Returns a substring starting
 * with the first integer and with a length of the second string.
 *
 * The difference between this and prim_midstr is this one ignores
 * ANSI escapes.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
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

/**
 * Implementation of MUF NOTIFY_SECURE
 *
 * Takes a ref and two strings.  The first string is sent over the ref's
 * secure descriptors, and the second string is sent over the ref's
 * insecure descriptors.  Listen prop queues are only triggered with the
 * second string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_notify_secure(PRIM_PROTOTYPE);

/**
 * Implementation of MUF INSTRING
 *
 * Consumes two strings and returns an integer representing the first
 * position the second string is found in the first string.  This is
 * case insensitive.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_instring(PRIM_PROTOTYPE);

/**
 * Implementation of MUF RINSTRING
 *
 * Consumes two strings and returns an integer representing the last
 * position the second string is found in the first string.  This is
 * case insensitive.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_rinstring(PRIM_PROTOTYPE);

/**
 * Implementation of MUF MD5HASH
 *
 * Consumes a string and returns a string containing the hash of the
 * provided string.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_md5hash(PRIM_PROTOTYPE);

/**
 * Implementation of MUF STRIP
 *
 * Consumes a string and returns the string without any leading or
 * trailing whitespace.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
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

/**
 * Implementation of MUF OTELL
 *
 * Consumes a string and broadcasts it to the current room, except for
 * the calling player.
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
void prim_otell(PRIM_PROTOTYPE);

/**
 * Implementation of MUF POSE-SEPARATOR?
 *
 * Consumes a string and returns true if the first character is
 * a valid pose separator.
 *
 * @see is_valid_pose_separator
 *
 * @param player the player running the MUF program
 * @param program the program being run
 * @param mlev the effective MUCKER level
 * @param pc the program counter pointer
 * @param arg the argument stack
 * @param top the top-most item of the stack
 * @param fr the program frame
 */
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
    prim_array_fmtstrings, prim_notify_nolisten, prim_notify_secure,    \
    prim_instring, prim_rinstring, prim_md5hash, prim_strip,    \
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
    "ARRAY_FMTSTRINGS", "NOTIFY_NOLISTEN", "NOTIFY_SECURE",    \
    "INSTRING", "RINSTRING", "MD5HASH", "STRIP",   \
    "TELL", "OTELL", "POSE-SEPARATOR?"

#endif /* !P_STRINGS_H */
