/** @file fbstrings.h
 *
 * Header for string handling operations, string memory allocations, and
 * similar helpers.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef FBSTRINGS_H
#define FBSTRINGS_H

#include <stddef.h>

#include "config.h"
#include "fbmuck.h"

/**
 * Turn 's' into an empty string if it is NULL.
 *
 * @param s The string to manipulate.
 */
#define DoNull(s) ((s) ? (s) : "")

#ifndef MALLOC_PROFILING
/**
 * Allocate and initialize a shared string structure.  The new structure
 * is returned, or NULL is returned if the string is null or empty.
 *
 * This will abort() if malloc fails.  The string is copied so the caller
 * still 'owns' the original string memory after calling this.
 *
 * Note that if MALLOC_PROFILING is defined, the version of this call in
 * fbstrings.h is not used.
 *
 * @param s The string to initialze our shared string with
 * @return a properly initialized struct shared_string.
 */
struct shared_string *alloc_prog_string(const char *);

/**
 * Create a copy of the given string.  If the string is NULL or empty,
 * return NULL.
 *
 * The string is copied so the caller still 'owns' the orignal string
 * memory after calling this.
 *
 * Note that if MALLOC_PROFILING is defined, the version of this call in
 * fbstrings.h is not used.
 *
 * @param string The string to copy
 * @return a copy of the string
 */
char *alloc_string(const char *);
#endif

/**
 * This is a method that works similar to strcasecmp except it
 * sorts alphabetically or numerically as appropriate.  For instance*
 *
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 *
 * @param s1 The first string to compare
 * @param s2 The second string to compare
 * @return Negative is s1 is 'less than' s2, 0 if they are equal, positive if
 *         s1 is greater than s2.
 */
int alphanum_compare(const char *s1, const char *s2);

/**
 * Returns true if 's' is blank (just spaces or an empty string)
 *
 * @param s the string to check
 *
 * @return integer as described above.
 */
int blank(const char *s);

/**
 * This method does a variety of string escaping:
 *
 * - Carriage return is escaped to \r
 * - The ESCAPE control is escaped to \[
 * - ` is escaped to \`
 * - \ is escaped to \\
 *
 * This is used by the MPI parser.
 *
 * @param buf The buffer that will be output to.
 * @param buflen the length of that buffer
 * @param in The input string we are escaping
 *
 * @return This returns 'buf', which is mutaed by this call so the return
 *         value can be ignored.
 */
char *cr2slash(char *buf, int buflen, const char *in);

/**
 * This function takes string which contains an exit format string
 * (delimited with EXIT_DELIMITER which is usually ; ... so a string
 * like str1;str2;str3).  It searches for 'prefix' as one of those
 * strings, and returns 0 if its not found or a pointer to the location
 * in 'string' where 'prefix' is first found.
 *
 * @param string The string to search
 * @param prefix The prefix to search for
 *
 * @return See description - string or NULL
 */
const char *exit_prefix(register const char *string, register const char *prefix);

/**
 * This is a POSIX-style regex implementation.  I'm not sure why a standard
 * implementation isn't used as this is a custom build under the hood, though
 * this supports most of the POSIX regex features.  The MUF man page for
 * smatch describes what this supports:
 *
 * *  A '?' matches any single character.
 *
 * *  A '*' matches any number of any characters.
 *
 * *  '{word1|word2|etc}' will match a single word, if it is one of those
 *       given, separated by | characters, between the {}s.  A word ends with
 *       a space or at the end of the string.  The given example would match
 *       either the words "word1", "word2", or "etc".
 *       {} word patterns will only match complete words: "{foo}*" and "{foo}p"
 *       do not match "foop" and "*{foo}" and "p{foo}" do not match "pfoo".
 *       {} word patterns can be easily meaningless; they will match nothing
 *       if they:
 *         (a) contains spaces,
 *         (b) do not follow a wildcard, space or beginning of string,
 *         (c) are not followed by a wildcard, space or end of string.
 *
 * *  If the first char of a {} word set is a '^', then it will match a single
 *       word if it is NOT one of those contained within the {}s.  Example:
 * *  '[aeiou]' will match a single character as long as it is one of those
 *       contained between the []s.  In this case, it matches any vowel.
 *
 * *  If the first char of a [] char set is a '^', then it will match a single
 *       character if it is NOT one of those contained within the []s.  Example:
 *       '[^aeiou]' will match any single character EXCEPT for a vowel.
 *
 * *  If a [] char set contains two characters separated by a '-', then it will
 *       match any single character that is between those two given characters.
 *       Example:  '[a-z0-9_]' would match any single character between 'a' and
 *       'z', inclusive, any character between '0' and '9', inclusive, or a '_'.
 *
 * *  The '\' character will disable the special meaning of the character that
 *       follows it, matching it literally.
 *
 * Example patterns:
 *   "d*g" matches "dg", "dog", "doog", "dorfg", etc.
 *   "d?g" matches "dog", "dig" and "dug" but not "dg" or "drug".
 *   "M[rs]." matches "Mr." and "Ms."
 *   "M[a-z]" matches "Ma", "Mb", etc.
 *   "[^a-z]" matches anything but an alphabetical character.
 *   "{Moira|Chupchup}*" matches "Moira snores" and "Chupchup arghs."
 *   "{Moira|Chupchup}*" does NOT match "Moira' snores".
 *   "{Foxen|Lynx|Fier[ao]} *t[iy]ckle*\?"  Will match any string starting
 *     with 'Foxen', 'Lynx', 'Fiera', or 'Fiero', that contains either 'tickle'
 *     or 'tyckle' and ends with a '?'.
 *
 * @param pattern The pattern
 * @param str The string to match to the pattern
 * @return 1 if matched, 0 if not.
 */
int equalstr(char *s, char *t);

/**
 * Checks to see if string s contains a float of format:
 *
 * [+|-]<digits>.<digits>[E[+|-]<digits>]
 *
 * Returns true is matches, false if not.
 *
 * @param s string to check
 * @return boolean, true if matches format mentioned above.
 */
int ifloat(const char *s);

/**
 * Checks to see if a given character is a valid "pose separator"
 *
 * What this specifically means is, this answers the question "Do we need
 * to prefix this pose string with a space?"
 *
 * Currently it checks for these characters: ', space, comma, and -
 *
 * So, this means :'s pose will show up as MyName's Pose instead of
 * MyName 's pose with an awkward space.
 *
 * @param ch character to check
 * @return boolean as described above.
 */
int is_valid_pose_separator(char ch);

/**
 * Converts an integer to a string.
 *
 * Please note that this uses a static internal buffer so this is most
 * absolutely not thread-safe (not that fuzzball is in general) and you
 * should not free the memory that is returned by this.  There is no
 * promise your buffer won't mutate out from under you in subsequent
 * runs of the FuzzBall main loop, so you will want to copy this buffer
 * if you need to keep it.
 *
 * @param i integer to convert to string
 * @return string version of 'i'
 */
char *intostr(int i);

/**
 * This checks to see if a given property path is prefixed with a given string
 *
 * Returns true if Property is prefixed with Prefix.  This will not count
 * extra leading /'s.  If Property == Prefix, that still counts as true.
 *
 * @param Property the property path to check
 * @param Prefix the prefix to check for
 * @return boolean as described above.
 */
int is_prop_prefix(const char *Property, const char *Prefix);

/**
 * Check to see if a string contains a number
 *
 * This allows a prefix of both '+' and '-'.  Leading whitespace is ignored.
 * This does not check for floats; only digits with an optional leading + or -
 *
 * @param s String to check
 * @return boolean as described above
 */
int number(const char *s);

/**
 * Prefix one string with another.
 *
 * This is used primarily for injecting player name at the start of a
 * string to support low MUCKER level (or MPI) notify's that have a requirement
 * to have notifications start with a user name.
 *
 * However, any similar case would work.  Dest is the target buffer,
 * with BufferLength, and 'Src' is the thing to be prefixed with 'Prefix'.
 *
 * Here's the odd bits: First off, if SuppressifPresent is 1, then we
 * will not inject 'prefix' if it is already there.
 *
 * Secondly, this uses is_valid_pose_separator; if is_valid_pose_separator
 * is false on 'Src', then a space will be injected between 'Prefix' and
 * 'Dest'.
 *
 * Thirdly, it looks like if 'Dest' has multiple lines (delimited by \r),
 * then each line in 'Dest' will be prefixed with Prefix.
 *
 * @param Dest The destination buffer
 * @param Src the source that will be getting prefixed.
 * @param Prefix the prefix to put before Src
 * @param BufferLength the length of the 'Dest' buffer.
 * @param SupressIfPresent if true, do not prefix if prefix is already there
 */
void prefix_message(char *Dest, const char *Src, const char *Prefix,
                    size_t BufferLength, int SuppressIfPresent);

/**
 * This does a strange prepending logic that is used in just one place
 * (the MUF) debugger.  'before' is a buffer with 'start' being a pointer
 * into that buffer.  'what' will be copied into the buffer, overwriting
 * whatever is in there, based on the following logic:
 *
 * - subtract length of 'what' from 'before' and make a pointer there
 * - If the pointer is less than 'start', abort -- this is to prevent
 *   buffer underruns I believe.
 * - Otherwise, copy 'what' in' before the start of that 'before' pointer.
 *
 * Basically you're prepending into 'before' very literally.  'Before' is
 * relocated to the start of the prepended string (before - len)
 *
 * If this sounds kind of crazy, its because it is.  This seems to be only
 * used in one spot, and if you don't know what its for, you probably don't
 * need it.
 *
 * @param before Pointer to pointer to character buffer.  Will be mutated
 * @param start The 'start' of the before buffer, to avoid buffer underruns
 * @param what What to copy into the buffer as a prepend
 *
 * @return number of characters prepended (may be 0)
 */
int prepend_string(char **before, char *start, const char *what);

/**
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/hirs/its, His/Hers/Hirs/Its)
 * %s/%S for subjective pronouns (he/she/sie/it, He/She/Sie/It)
 * %o/%O for objective pronouns (him/her/hir/it, Him/Her/Hir/It)
 * %p/%P for possessive pronouns (his/her/hir/its, His/Her/Hir/Its)
 * %r/%R for reflexive pronouns (himself/herself/hirself/itself,
 *                                Himself/Herself/Hirself/Itself)
 * %n    for the player's name.
 *
 * This is based on props; the gender prop is configured with @tune
 * and defaults to 'sex'.  The contents of that prop are parsed for MPI.
 * it will, under the hood, default to '_default' if nothing is set.
 *
 * By default it understands the strings "male", "female", "hermaphrodite",
 * "herm", and "neuter".  The understanding can be altered with props
 * in a variety of kinda complicated ways.
 *
 * First it checks for the pronoun set on the player itself, named
 * after the pronoun, such as %o:override if it is a supported pronoun
 * as lisfted above.
 *
 * If the pronoun is not one of the defaults supported by the system,
 * then it is searched for in the environment starting with the player
 * using envpropstr - so a prop named like %w:whatever
 *
 * Finally, it searches both the player and room #0 for props of
 * the format: _pronouns/{sex}/{prn}   <-- _pronouns is controlled by
 * PRONOUNS_PROPDIR.  A sample of how this might work is like:
 *
 * _pronouns/male/%o:whatever
 *
 * Finally, if the gender is unknown, and no other props are set, it
 * checks _pronouns/_default/{prn}
 *
 * If all else fails, it uses the default baked in pronouns, which
 * are either the player's name (if the gender is unknown) or
 * based on the arrays absolute, subjective, posessive, objective,
 * reflexive in the function itself based on the understood strings above.
 *
 * So this thing is pretty complicated!  But also forward thinking
 * since it can basically support an unlimited number of genders.
 *
 * @param descr the descriptor of the triggering person
 * @param player the object we are checking pronouns on
 * @param str the string to process pronouns from
 *
 * @return the string with pronoun substitutions
 *
 * WARNING: This uses an internal static buffer, so the usual notes apply;
 * this is not threadsafe, and if you want to preserve the buffer you should
 * copy it because it will not last.  The maximum buffer size is BUFFER_LEN*2
 */
char *pronoun_substitute(int descr, dbref player, const char *str);

/**
 * Method to convert a reference to a character string
 *
 * This has a few quirks.  If the ref is greater than or equal to
 * dbtop, or less than -4, then the string is "Bad"
 *
 * If the object ref is a player, it uses the player name prefixed with
 * *, such as *Tanabi
 *
 * Otherwise, it takes the number and prefixes it with #, such as #12345
 *
 * @param obj the ref to turn into a string
 * @param buf the buffer to write the string into
 * @param buflen the size of the buffer
 *
 * @return the 'buf' pointer.  You can pretty much ignore this since
 *         buf is muted by ref2str
 */
char *ref2str(dbref obj, char *buf, size_t buflen);

/**
 * Trim ending whitespace off a string
 *
 * Uses isspace to determine if something is a space.
 *
 * Not sure why this requires a pointer to a pointer; the pointer itself
 * is not mutated in any way (however the data is mutated, with trailing
 * spaces becoming \0).
 *
 * Its probably to keep it consistent with skip_whitespace which trims
 * off starting spaces.
 *
 * @param s Pointer to pointer to string buffer
 */
void remove_ending_whitespace(char **s);

/**
 * Skip over starting whitespaces in a string
 *
 * Note that this does not technically "trim" spaces; instead, it advances
 * the provided pointer to the first non-space (or to the end of the string
 * as the case may be).
 *
 * @param parsebuf pointer to a string pointer.
 */
void skip_whitespace(const char **parsebuf);

/**
 * This is an implementation of strncat that takes the buffer size instead
 * of the number of characters to concatinate.
 *
 * @param buf the target buffer to concatinate onto.
 * @param bufsize the size of the target buffer
 * @param src the string to concatinate onto buf
 *
 * @return buf -- because buf is mutated, the return value doesn't matter.
 */
char *strcatn(char *buf, size_t bufsize, const char *src);

/**
 * Copy a string from src to buf, taking into account the size of the buffer
 * and ensuring it is null terminated.
 *
 * @param buf Target buffer
 * @param bufsize the size of that buffer
 * @param src the source to copy from
 *
 * @return buf -- because buf is mutated, the return value doesn't matter.
 */
char *strcpyn(char *buf, size_t bufsize, const char *src);

/**
 * Encrypt a string of 'data' with key 'key'
 *
 * This is a very simple scramble style encryption.  The resulting string
 * has 'magic' in the first two bytes; the first byte defines a constant
 * that is used for decryption and the second byte is the seed.
 *
 * The encrypted string is returned via a static buffer.  Note that
 * the maximum size of the buffer is BUFFER_LEN, and as usual, this means
 * this method is not threadsafe and your buffer could theoretically
 * mutate out from under you if you do not copy the result elsewhere first.
 *
 * @param data The string to encrypt
 * @param key the key to use
 * @return static buffer containing encrypted string.
 */
const char *strencrypt(const char *data, const char *key);

/**
 * Decrypt a string of 'data' with key 'key'
 *
 * This is a very simple scramble style encryption.  The resulting string
 * has 'magic' in the first two bytes; the first byte defines a constant
 * that is used for decryption and the second byte is the seed.
 *
 * The encrypted string is returned via a static buffer.  Note that
 * the maximum size of the buffer is BUFFER_LEN, and as usual, this means
 * this method is not threadsafe and your buffer could theoretically
 * mutate out from under you if you do not copy the result elsewhere first.
 *
 * @param data The string to decrypt
 * @param key the key to use
 * @return static buffer containing encrypted string.
 */
const char *strdecrypt(const char *, const char *);

/**
 * Does a word-by-word match of 'sub' in 'src'
 *
 * For each word in string 'src' (a word being defined as a block of
 * alphanumeric text being separated by non-alphanumeric text), this
 * will check to see if 'sub' is a prefix of that word.
 *
 * For instance, a string: The quick brown fox yaps! loudly
 *
 * 'The', 'quick', 'brown', 'fox', 'yaps', 'loudly' are all words and
 * each word would be checked to see if it starts with the value in
 * 'sub'.  So, for instance, 'bro' would match but 'wn' would not.
 *
 * This is CASE INSENSITIVE.
 *
 * @param src the source string
 * @param sub the substring to match
 * @return NULL if no match; otherwise it returns a pointer to the
 *         location in 'src' where 'sub' starts.
 */
const char *string_match(const char *src, const char *sub);

/**
 * Case insensitive check to see if 'string' starts with 'prefix'.
 *
 * This is basically strncasecmp, but it avoids having to do a
 * strlen on 'prefix' so it might be faster.
 *
 * @param string the string to check
 * @param prefix the prefix to check string for
 * @return boolean - true if string starts with prefix.
 */
int string_prefix(const char *string, const char *prefix);

/**
 * Do a string substitution
 *
 * This is the underpinning of the SUBST MUF primitive, though
 * also used elsewhere.  Finds every occurance of 'oldstr' in 'str',
 * and replaces it with 'newstr'.  The result is put in 'buf'.
 *
 * @param str The string to do substitutions in.
 * @param oldstr The substring to find
 * @param newstr Replace oldstr with this.
 * @param buf The buffer to put the result into
 * @param maxlen the size of the result buffer
 *
 * @return 'buf' - the return value can safely be ignored since buf is
 *         mutated by this call.
 */
char *string_substitute(const char *str, const char *oldstr, const char *newstr,
                        char *buf, size_t maxlen);

/**
 * Strip the ANSI control characters out of a given string and puts
 * the result into 'buf'
 *
 * WARNING: This does NOT validate the length of 'buf' and that makes
 *          this a potentially dangerous call.  It should be safe 
 *          if 'buf' is the same size as 'input' or larger.
 *
 * This call effectively removes color from a string, as well as makes
 * it easier to string compare and other such things.
 *
 * @param buf The buffer to put 'input' into.
 * @param input The source string
 * @return pointer to buf which can be ignored since buf is mutated by this
 *         call
 */
char *strip_ansi(char *buf, const char *input);

/**
 * This is used to sanitize ANSI before displaying it to the user
 *
 * This is just used by the message queue methods.  It makes sure all
 * ANSI codes are 'SGR' type (graphics rendition) and it also makes sure
 * all strings are terminated with a SGR reset (to reset formatting).
 *
 * WARNING: This *does* limit the size of buf, but it assumes buf is
 *          BUFFER_LEN, which is theoretically a bad assumption.  Its
 *          fine for its one purpose, but may not be fine for other
 *          purposes.
 *
 * @param buf the buffer to put 'input' into - must be AT LEAST BUFFER_LEN
 * @param input the source to sanitize
 * @return pointer to buf which can be ignored since buf is mutated by this
 *         call
 */
char *strip_bad_ansi(char *buf, const char *input);

/**
 * Return a string that is 's' without beginning or ending strings.
 *
 * So this one is a weird mix of mutating and non-mutating.  stripspaces
 * does *not* remove the leading spaces from 's' but it *does* remove
 * the trailing spaces.
 *
 * The pointer returned is a pointer into 's' which points to the first
 * non-space character in 's'.  It is, effectively, a stripped spaces
 * version of 's'.
 *
 * @param s the string to strip spaces from
 * @return a pointer to the first non-space character within 's'
 */
char *stripspaces(char *s);

/**
 * Converts all characters in s to lower case.
 *
 * This call mutates the buffer pointed to by 's' to be all lower
 * case.  There is no actual reason for this to be a pointer to
 * a pointer; the pointer isn't mutated, just what it points to.
 *
 * @param s the string to lowercase.
 */
void tolower_string(char **s);

/**
 * Converts all characters in s to upper case.
 *
 * This call mutates the buffer pointed to by 's' to be all upper
 * case.  There is no actual reason for this to be a pointer to
 * a pointer; the pointer isn't mutated, just what it points to.
 *
 * @param s the string to uppercase.
 */
void toupper_string(char **s);

/**
 * Figure out if a string should be considered a true value
 *
 * This is used by MPI for instance to determine if a string
 * should be considered binary true or not.  Empty string is
 * false, as is a string containing "0".  Anything else is true.
 *
 * @param s the string to consider
 * @return boolean as described above.
 */
int truestr(const char *buf);

#endif /* !FBSTRINGS_H */
