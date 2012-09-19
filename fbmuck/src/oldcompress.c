/* $Header: /cvsroot/fbmuck/fbmuck/src/oldcompress.c,v 1.4 2005/07/04 12:04:23 winged Exp $ */

/*
 * $Log: oldcompress.c,v $
 * Revision 1.4  2005/07/04 12:04:23  winged
 * Initial revisions for everything.
 *
 * Revision 1.3  2000/07/19 01:33:18  revar
 * Compiling cleanup for -Wall -Wstrict-prototypes -Wno-format.
 * Changed the mcpgui package to use 'const char*'s instead of 'char *'s
 *
 * Revision 1.2  2000/03/29 12:21:02  revar
 * Reformatted all code into consistent format.
 * 	Tabs are 4 spaces.
 * 	Indents are one tab.
 * 	Braces are generally K&R style.
 * Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
 * Rewrote restart script as a bourne shell script.
 *
 * Revision 1.1.1.1  1999/12/16 03:23:29  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.1.1.1  1999/12/12 07:27:44  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/12 02:43:50  foxen
 * Initial revision
 *
 * Revision 5.3  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.3  90/09/10  02:19:44  rearl
 * Introduced string compression of properties, for the
 * COMPRESS compiler option.
 *
 * Revision 1.2  90/08/11  03:51:33  rearl
 * *** empty log message ***
 *
 * Revision 1.1  90/07/19  23:02:36  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

/*
 * Compression routines
 *
 * These use a pathetically simple encoding that takes advantage of the
 * eighth bit on a char; if you are using an international character set,
 * they may need substantial patching.
 *
 */

#define BUFFER_LEN 16384		/* nice big buffer */

#define TOKEN_BIT 0x80			/* if on, it's a token */
#define TOKEN_MASK 0x7f			/* for stripping out token value */
#define NUM_TOKENS (128)
#define MAX_CHAR (128)

/* Top 128 bigrams in the CMU TinyMUD database as of 2/13/90 */
static const char *old_tokens[NUM_TOKENS] = {
	"e ", " t", "th", "he", "s ", " a", "ou", "in",
	"t ", " s", "er", "d ", "re", "an", "n ", " i",
	" o", "es", "st", "to", "or", "nd", "o ", "ar",
	"r ", ", ", "on", " b", "ea", "it", "u ", " w",
	"ng", "le", "is", "te", "en", "at", " c", "y ",
	"ro", " f", "oo", "al", ". ", "a ", " d", "ut",
	" h", "se", "nt", "ll", "g ", "yo", " l", " y",
	" p", "ve", "f ", "as", "om", "of", "ha", "ed",
	"h ", "hi", " r", "lo", "Yo", " m", "ne", "l ",
	"li", "de", "el", "ta", "wa", "ri", "ee", "ti",
	"no", "do", "Th", " e", "ck", "ur", "ow", "la",
	"ac", "et", "me", "il", " g", "ra", "co", "ch",
	"ma", "un", "so", "rt", "ai", "ce", "ic", "be",
	" n", "k ", "ge", "ot", "si", "pe", "tr", "wi",
	"e.", "ca", "rs", "ly", "ad", "we", "bo", "ho",
	"ir", "fo", "ke", "us", "m ", " T", "di", ".."
};

static char old_token_table[MAX_CHAR][MAX_CHAR];
static int old_table_initialized = 0;

static void
old_init_compress(void)
{
	int i;
	int j;

	for (i = 0; i < MAX_CHAR; i++) {
		for (j = 0; j < MAX_CHAR; j++) {
			old_token_table[i][j] = 0;
		}
	}

	for (i = 0; i < NUM_TOKENS; i++) {
		old_token_table[(int)old_tokens[i][0]][(int)old_tokens[i][1]] = i | TOKEN_BIT;
	}

	old_table_initialized = 1;
}

static int
compressed(const char *s)
{
	if (!s)
		return 0;
	while (*s) {
		if (*s++ & TOKEN_BIT)
			return 1;
	}
	return 0;
}

const char *
old_compress(const char *s)
{
	static char buf[BUFFER_LEN];
	char *to;
	char token;

	if (!old_table_initialized)
		old_init_compress();

	if (!s || compressed(s))
		return s;				/* already compressed */

	/* tokenize the first characters */
	for (to = buf; s[0] && s[1]; to++) {
		if ((token = old_token_table[(int)s[0]][(int)s[1]])) {
			*to = token;
			s += 2;
		} else {
			*to = s[0];
			s++;
		}
	}

	/* copy the last character (if any) and null */
	while ((*to++ = *s++)) ;

	return buf;
}

const char *
old_uncompress(const char *s)
{
	static char buf[BUFFER_LEN];
	char *to;
	const char *token;

	if (!old_table_initialized)
		old_init_compress();

	if (!s || !compressed(s))
		return s;				/* already uncompressed */

	for (to = buf; *s; s++) {
		if (*s & TOKEN_BIT) {
			token = old_tokens[*s & TOKEN_MASK];
			*to++ = *token++;
			*to++ = *token;
		} else {
			*to++ = *s;
		}
	}

	*to++ = *s;

	return buf;
}
static const char *oldcompress_c_version = "$RCSfile: oldcompress.c,v $ $Revision: 1.4 $";
const char *get_oldcompress_c_version(void) { return oldcompress_c_version; }
