/** @file fbstrings.c
 *
 * Source for string handling operations, string memory allocations, and
 * similar helpers.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "db.h"
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "mpi.h"
#include "props.h"
#include "tune.h"

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
int
alphanum_compare(const char *t1, const char *s2)
{
    int n1, n2, cnt1, cnt2;
    const char *u1, *u2;
    register const char *s1 = t1;

    while (*s1 && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
    }

    /* if at a digit, compare number values instead of letters. */
    if (isdigit(*s1) && isdigit(*s2)) {
        u1 = s1;
        u2 = s2;
        n1 = n2 = 0;    /* clear number values */
        cnt1 = cnt2 = 0;

        /* back up before zeros */
        if (s1 > t1 && *s2 == '0') {
            s1--;
            s2--;   /* curr chars are diff */
        }

        while (s1 > t1 && *s1 == '0') {
            s1--;
            s2--;   /* prev chars are same */
        }

        if (!isdigit(*s1)) {
            s1++;
            s2++;
        }

        /* calculate number values */
        while (isdigit(*s1)) {
            cnt1++;

            n1 = (n1 * 10) + (*s1++ - '0');
        }

        while (isdigit(*s2)) {
            cnt2++;
            n2 = (n2 * 10) + (*s2++ - '0');
        }

        /* if more digits than int can handle... */
        if (cnt1 > 8 || cnt2 > 8) {
            if (cnt1 == cnt2)
                return (*u1 - *u2);    /* cmp chars if mag same */

            return (cnt1 - cnt2);    /* compare magnitudes */
        }

        /* if number not same, return count difference */
        if (n1 && n2 && n1 != n2)
            return (n1 - n2);

        /* else, return difference of characters */
        return (*u1 - *u2);
    }

    /* if characters not digits, and not the same, return difference */
    return (tolower(*s1) - tolower(*s2));
}

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
const char *
exit_prefix(register const char *string, register const char *prefix)
{
    const char *p;
    const char *s = string;

    while (*s) {
        p = prefix;
        string = s;

        while (*s && *p && tolower(*s) == tolower(*p)) {
            s++;
            p++;
        }

        skip_whitespace(&s);

        if (!*p && (!*s || *s == EXIT_DELIMITER)) {
            return string;
        }

        while (*s && (*s != EXIT_DELIMITER))
            s++;

        if (*s)
            s++;

        skip_whitespace(&s);
    }

    return 0;
}

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
int
string_prefix(register const char *string, register const char *prefix)
{
    while (*string && *prefix && tolower(*string) == tolower(*prefix)) {
        string++;
        prefix++;
    }

    return *prefix == '\0';
}

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
 * @param src the source string
 * @param sub the substring to match
 * @return NULL if no match; otherwise it returns a pointer to the
 *         location in 'src' where 'sub' starts.
 */
const char *
string_match(register const char *src, register const char *sub)
{
    if (*sub != '\0') {
        while (*src) {
            if (string_prefix(src, sub))
                return src;

            /* else scan to beginning of next word */
            while (*src && isalnum(*src))
                src++;

            while (*src && !isalnum(*src))
                src++;
        }
    }
    return 0;
}

#define GENDER_UNASSIGNED   0x0    /* unassigned - the default */
#define GENDER_NEUTER       0x1    /* neuter */
#define GENDER_FEMALE       0x2    /* for women */
#define GENDER_MALE         0x3    /* for men */
#define GENDER_HERM         0x4    /* for hermaphrodites */

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
 * reflexive based on the understood strings above.
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
char *
pronoun_substitute(int descr, dbref player, const char *str)
{
    char c;
    char d;
    char prn[3];
    char globprop[128];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char sexbuf[BUFFER_LEN];
    char *result;
    const char *sexstr;
    const char *self_sub;    /* self substitution code */
    const char *temp_sub;
    dbref mywhere = player;
    int sex;

    /* If you ever have to add a new gender, this is where you set the
     * defaults.
     */
    static const char *subjective[5] = { "", "it", "she", "he", "sie" };
    static const char *possessive[5] = { "", "its", "her", "his", "hir" };
    static const char *objective[5] = { "", "it", "her", "him", "hir" };
    static const char *reflexive[5] = { "", "itself", "herself", "himself", "hirself" };
    static const char *absolute[5] = { "", "its", "hers", "his", "hirs" };

    /* All replacements will be kept in this variable as a tiny string;
     * the string always starts with % and only the middle character
     * will change (prn[1])
     */
    prn[0] = '%';
    prn[2] = '\0';

    strcpyn(orig, sizeof(orig), str);
    str = orig;

    /* Get our gender property value */
    sexstr = get_property_class(player, tp_gender_prop);

    if (sexstr) {
        /* Parse MPI in the gender property */
        sexstr = do_parse_mesg(descr, player, player, sexstr, "(Lock)", sexbuf,
                               sizeof(sexbuf),
                               (MPI_ISPRIVATE | MPI_ISLOCK |
                                (Prop_Blessed(player, tp_gender_prop) ? 
                                 MPI_ISBLESSED : 0)));
    }

    if (sexstr)
        skip_whitespace(&sexstr);

    sex = GENDER_UNASSIGNED;

    /* Try to determine what sex to use.  This is mostly used by the
     * default case as 'sex' will be an index into the arrays above.
     */
    if (!sexstr || !*sexstr) {
        sexstr = "_default";
    } else {
        char *last_non_space = sexbuf;
        char *ptr = sexbuf;

        for (; *ptr; ptr++)
            if (!isspace(*ptr))
                last_non_space = ptr;

        if (*last_non_space)
            *(last_non_space + 1) = '\0';

        if (strcasecmp(sexstr, "male") == 0)
            sex = GENDER_MALE;
        else if (strcasecmp(sexstr, "female") == 0)
            sex = GENDER_FEMALE;
        else if (strcasecmp(sexstr, "hermaphrodite") == 0)
            sex = GENDER_HERM;
        else if (strcasecmp(sexstr, "herm") == 0)
            sex = GENDER_HERM;
        else if (strcasecmp(sexstr, "neuter") == 0)
            sex = GENDER_NEUTER;
    }

    /* Now process the string */
    result = buf;
    while (*str) {
        if (*str == '%') { /* We found a % */
            *result = '\0';
            prn[1] = c = *(++str);

            if (!c) { /* But there's no character after it -- leave it be */
                *(result++) = '%';
                continue;
            } else if (c == '%') { /* But it is escaped, turn it into 1 % */
                *(result++) = '%';
                str++;
            } else { /* Process the gender */
                mywhere = player;
                d = (isupper(c)) ? c : toupper(c);

                /* Compute the global property name */
                snprintf(globprop, sizeof(globprop), "%s/%.64s/%s",
                         PRONOUNS_PROPDIR, sexstr, prn);

                /* Check a variety of properties to see if we're overriding
                 * the default.
                 */
                if (d == 'A' || d == 'S' || d == 'O' || d == 'P' ||
                    d == 'R' || d == 'N') {
                    self_sub = get_property_class(mywhere, prn);
                } else {
                    self_sub = envpropstr(&mywhere, prn);
                }

                if (!self_sub) {
                    self_sub = get_property_class(player, globprop);
                }

                if (!self_sub) {
                    self_sub = get_property_class(0, globprop);
                }

                if (!self_sub && (sex == GENDER_UNASSIGNED)) {
                    snprintf(globprop, sizeof(globprop), "%s/_default/%s",
                             PRONOUNS_PROPDIR, prn);

                    if (!(self_sub = get_property_class(player, globprop)))
                        self_sub = get_property_class(0, globprop);
                }

                /* If we found a "user defined" substition, inject it
                 * into the string.
                 */
                if (self_sub) {
                    temp_sub = NULL;

                    if (self_sub[0] == '%' && toupper(self_sub[1]) == 'N') {
                        temp_sub = self_sub;
                        self_sub = NAME(player);
                    }

                    if (((size_t)(result - buf) + strlen(self_sub)) >
                        (BUFFER_LEN - 2))
                        return buf;

                    strcatn(result, sizeof(buf) - (size_t)(result - buf),
                            self_sub);

                    if (isupper(prn[1]) && islower(*result))
                        *result = toupper(*result);

                    result += strlen(result);
                    str++;

                    if (temp_sub) {
                        if (((size_t)(result - buf) + strlen(temp_sub + 2)) >
                             (BUFFER_LEN - 2))
                            return buf;

                        strcatn(result, sizeof(buf) - (size_t)(result - buf), temp_sub + 2);
                        result += strlen(result);
                    }
                } else if (sex == GENDER_UNASSIGNED) {
                    /* Unassigned has some special rules */
                    switch (c) {
                        case 'n':
                        case 'N':
                        case 'o':
                        case 'O':
                        case 's':
                        case 'S':
                        case 'r':
                        case 'R':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    NAME(player));
                            break;
                        case 'a':
                        case 'A':
                        case 'p':
                        case 'P':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    NAME(player));
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf), "'s");
                            break;
                        default:
                            result[0] = *str;
                            result[1] = 0;
                            break;
                    }

                    str++;
                    result += strlen(result);

                    if ((size_t)(result - buf) > (BUFFER_LEN - 2)) {
                        buf[BUFFER_LEN - 1] = '\0';
                        return buf;
                    }
                } else {
                    /* Use the defaults in our various arrays */
                    switch (c) {
                        case 'a':
                        case 'A':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    absolute[sex]);
                            break;
                        case 's':
                        case 'S':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    subjective[sex]);
                            break;
                        case 'p':
                        case 'P':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    possessive[sex]);
                            break;
                        case 'o':
                        case 'O':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    objective[sex]);
                            break;
                        case 'r':
                        case 'R':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    reflexive[sex]);
                            break;
                        case 'n':
                        case 'N':
                            strcatn(result,
                                    sizeof(buf) - (size_t)(result - buf),
                                    NAME(player));
                            break;
                        default:
                            *result = *str;
                            result[1] = '\0';
                            break;
                    }

                    if (isupper(c) && islower(*result)) {
                        *result = toupper(*result);
                    }

                    result += strlen(result);
                    str++;

                    if ((size_t)(result - buf) > (BUFFER_LEN - 2)) {
                        buf[BUFFER_LEN - 1] = '\0';
                        return buf;
                    }
                }
            }
        } else {
            /* Did we run out of space? */
            if ((size_t)(result - buf) > (BUFFER_LEN - 2)) {
                buf[BUFFER_LEN - 1] = '\0';
                return buf;
            }

            *result++ = *str++;
        }
    }

    *result = '\0';
    return buf;
}

#ifndef MALLOC_PROFILING

/**
 * Create a copy of the given string.  If the string is NULL or empty,
 * return NULL.
 *
 * This will abort() if malloc fails.  The string is copied so the caller
 * still 'owns' the orignal string memory after calling this.
 *
 * Note that if MALLOC_PROFILING is defined, the version of this call in
 * fbstrings.h is not used.
 *
 * @param string The string to copy
 * @return a copy of the string
 */
char *
alloc_string(const char *string)
{
    if (!string || !*string)
        return 0;

    return strdup(string);
}

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
struct shared_string *
alloc_prog_string(const char *s)
{
    struct shared_string *ss;
    int length;

    if (s == NULL || *s == '\0')
        return (NULL);

    length = strlen(s);
    if ((ss = (struct shared_string *)
        malloc(sizeof(struct shared_string) + length)) == NULL)
        abort();

    ss->links = 1;
    ss->length = length;
    memmove(ss->data, s, ss->length + 1);
    return (ss);
}
#endif

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
char *
intostr(int i)
{
    static char num[16];
    snprintf(num, sizeof(num), "%d", i);
    return num;
}

/*
 * Encrypt one string with another one - the various static variables used
 * to initialize the system.
 *
 * @TODO: Consider "replacing" this with OpenSSL.  We'd still need to
 *        ensure backwards compatibility, but if we're built with OpenSSL
 *        there's no reason we can't use a slightly more serious algorythm.
 */

/* Encrypt uses this value as part of its entropy -- I think its to keep
 * all the resulting string data in the 'visible' ASCII range.  Weirdly,
 * this is treated as a constant in encrypt but is a variable in decrypt (see
 * charset_count) below.
 */
#define CHARCOUNT 97

/* enarr is used as a static character mapping that is used by the
 * encrypt / decrypt function as a basis for scrambling up a string.
 * Its part of the overall entropy alogn with the key and chrcnt.
 */
static unsigned char enarr[256];

/* When encrypting, the first character of the returned buffer is
 * used as an index into this array (its the character minus ' ').
 * This is part of the crypt / decrypt entropy.
 *
 * However, on encrypt, this is hardcoded to CHARCOUNT (97) on the
 * encryption side and the first character is always ' ' + 2 so
 * it seems like this is an unused encryption feature.  While I approve
 * of the first byte being essentially a validation, I'm not sure why
 * we do things a little different in encrypt vs. decrypt.
 */
static int charset_count[] = { 96, 97, 0 };

/* Have we initialized the crypt system yet? */
static int initialized_crypt = 0;

/**
 * Initialize the encrypt / decrypt system
 *
 * Primarily this configures the 'enarr' array which acts as a mapping
 * of ASCII characters where some of the characters are mixed about and
 * some are ensured remain constant.
 *
 * Once initialized, it sets initialized_crypt to 1
 *
 * @private
 */
static void
init_crypt(void)
{
    for (int i = 0; i <= 255; i++)
        enarr[i] = (unsigned char) i;

    for (int i = 'A'; i <= 'M'; i++)
        enarr[i] = (unsigned char) enarr[i] + 13;

    for (int i = 'N'; i <= 'Z'; i++)
        enarr[i] = (unsigned char) enarr[i] - 13;

    enarr['\r'] = 127;
    enarr[127] = '\r';
    enarr[ESCAPE_CHAR] = 31;
    enarr[31] = ESCAPE_CHAR;
    initialized_crypt = 1;
}


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
const char *
strencrypt(const char *data, const char *key)
{
    static char linebuf[BUFFER_LEN];
    char buf[BUFFER_LEN + 8];
    const char *cp;
    char *ptr;
    char *ups;
    const char *upt;
    int linelen;
    int count;
    int seed, seed2, seed3;
    int limit = BUFFER_LEN;
    int result;

    if (!initialized_crypt)
        init_crypt();

    /* 'seed3' is the actual seed that that will be used by decrypting. */
    seed = 0;
    for (cp = key; *cp; cp++)
        seed = ((((*cp) ^ seed) + 170) % 192);

    seed2 = 0;
    for (cp = data; *cp; cp++)
        seed2 = ((((*cp) ^ seed2) + 21) & 0xff);

    seed3 = seed2 = ((seed2 ^ (seed ^ (RANDOM() >> 24))) & 0x3f);

    count = seed + 11;

    /* Use 'buf' as a temporary buffer for our encrypted string */
    for (upt = data, ups = buf, cp = key; *upt; upt++) {
        count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;

        seed2 = ((seed2 + 1) & 0x3f);

        if (!*(++cp))
            cp = key;

        result = (enarr[(unsigned char) *upt] - (32 - (CHARCOUNT - 96))) + 
                 count + seed;
        *ups = (char)enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
        count = (((*upt) ^ count) + seed) & 0xff;
        ups++;
    }

    *ups++ = '\0';

    /* Linebuf is what we will actually return */
    ptr = linebuf;

    linelen = strlen(data);

    /* Bake in the seed information required for decryption. */
    *ptr++ = (' ' + 2);
    *ptr++ = (' ' + seed3);
    limit--;
    limit--;

    /* This could just be a strcpy ... */
    for (cp = buf; cp < &buf[linelen]; cp++) {
        limit--;

        if (limit < 0)
            break;

        *ptr++ = *cp;
    }

    *ptr++ = '\0';
    return linebuf;
}


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
const char *
strdecrypt(const char *data, const char *key)
{
    char linebuf[BUFFER_LEN];
    static char buf[BUFFER_LEN];
    const char *cp;
    char *ups;
    const char *upt;
    int outlen;
    int count;
    int seed, seed2;
    int result;
    int chrcnt;

    if (!initialized_crypt)
        init_crypt();

    /* Null string -- string must be at least 2 bytes */
    if (data[0] == '\0' || data[1] == '\0') {
        return "";
    }

    /* Validate the first byte; this is static on the encrypt side,
     * so not sure why its dynamic here.
     */
    if ((data[0] - ' ') < 1 || (data[0] - ' ') > 2) {
        return "";
    }

    chrcnt = charset_count[(data[0] - ' ') - 1];

    /* The seed is the second byte */
    seed2 = (data[1] - ' ');

    strcpyn(linebuf, sizeof(linebuf), data + 2);

    /* Calculate the seed */
    seed = 0;
    for (cp = key; *cp; cp++) {
        seed = (((*cp) ^ seed) + 170) % 192;
    }

    count = seed + 11;
    outlen = strlen(linebuf);
    upt = linebuf;
    ups = buf;
    cp = key;

    /* Loop over and decrypt */
    while ((const char *) upt < &linebuf[outlen]) {
        count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;

        if (!*(++cp))
            cp = key;
        seed2 = ((seed2 + 1) & 0x3f);

        result = (enarr[(unsigned char) *upt] - (32 - (chrcnt - 96))) -
                 (count + seed);

        while (result < 0)
            result += chrcnt;

        *ups = (char)enarr[result + (32 - (chrcnt - 96))];

        count = (((*ups) ^ count) + seed) & 0xff;
        ups++;
        upt++;
    }

    *ups++ = '\0';

    return buf;
}


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
char *
strip_ansi(char *buf, const char *input)
{
    const char *is;
    char *os;

    buf[0] = '\0';
    os = buf;

    is = input;

    while (*is) {
        if (*is == ESCAPE_CHAR) {
            is++;

            if (*is == '[') {
                is++;

                while (isdigit(*is) || *is == ';')
                    is++;

                if (*is == 'm')
                    is++;
            } else if (*is != '\0') {
                is++;
            }
        } else {
            *os++ = *is++;
        }
    }

    *os = '\0';

    return buf;
}


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
char *
strip_bad_ansi(char *buf, const char *input)
{
    const char *is;
    char *os;
    int aflag = 0;
    int limit = BUFFER_LEN - 5;

    buf[0] = '\0';
    os = buf;

    is = input;

    while (*is && limit-- > 0) {
        if (*is == ESCAPE_CHAR) {
            if (is[1] == '\0') {
                is++;
            } else if (is[1] != '[') {
                is++;
                is++;
            } else {
                aflag = 1;
                *os++ = *is++;    /* esc */
                *os++ = *is++;    /*  [  */

                while (isdigit(*is) || *is == ';') {
                    *os++ = *is++;
                }

                if (*is != 'm') {
                    *os++ = 'm';
                }

                *os++ = *is++;
            }
        } else {
            *os++ = *is++;
        }
    }

    if (aflag) {
        int termrn = 0;

        if (*(os - 2) == '\r' && *(os - 1) == '\n') {
            termrn = 1;
            os -= 2;
        }

        *os++ = '\033';
        *os++ = '[';
        *os++ = '0';
        *os++ = 'm';

        if (termrn) {
            *os++ = '\r';
            *os++ = '\n';
        }
    }

    *os = '\0';

    return buf;
}

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
int
prepend_string(char **before, char *start, const char *what)
{
    char *ptr;
    size_t len;
    len = strlen(what);
    ptr = *before - len;

    if (ptr < start)
        return 0;

    memcpy((void *) ptr, (const void *) what, len);
    *before = ptr;
    return len;
}

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
int
is_valid_pose_separator(char ch)
{
    return (ch == '\'') || (ch == ' ') || (ch == ',') || (ch == '-');
}


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
void
prefix_message(char *Dest, const char *Src, const char *Prefix,
               size_t BufferLength, int SuppressIfPresent)
{
    size_t PrefixLength = strlen(Prefix);
    int CheckForHangingEnter = 0;

    while ((BufferLength > PrefixLength) && (*Src != '\0')) {
        if (*Src == '\r') {
            Src++;
            continue;
        }

        if (!SuppressIfPresent || strncmp(Src, Prefix, PrefixLength)
            || (!is_valid_pose_separator(Src[PrefixLength])
            && (Src[PrefixLength] != '\r') && (Src[PrefixLength] != '\0')
        )) {
            strcpyn(Dest, BufferLength, Prefix);

            Dest += PrefixLength;
            BufferLength -= PrefixLength;

            if (BufferLength > 1) {
                if (!is_valid_pose_separator(*Src)) {
                    *Dest++ = ' ';
                    BufferLength--;
                }
            }
        }

        while ((BufferLength > 1) && (*Src != '\0')) {
            *Dest++ = *Src;
            BufferLength--;

            if (*Src++ == '\r') {
                CheckForHangingEnter = 1;
                break;
            }
        }
    }

    /* I'm not entirely clear on the logic behind "CheckForHangingEnter",
     * though it seems the purpose is to avoid terminating 'Dest' with
     * a \r.
     */
    if (CheckForHangingEnter && (Dest[-1] == '\r'))
        Dest--;

    *Dest = '\0';
}

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
int
is_prop_prefix(const char *Property, const char *Prefix)
{
    while (*Property == PROPDIR_DELIMITER)
        Property++;

    while (*Prefix == PROPDIR_DELIMITER)
        Prefix++;

    while (*Prefix) {
        if (*Property == '\0')
            return 0;

        if (*Property++ != *Prefix++)
            return 0;
    }

    return (*Property == '\0') || (*Property == PROPDIR_DELIMITER);
}

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
char *
strcpyn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = 0;
    char *dest = buf;

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }

    *dest = '\0';
    return buf;
}


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
char *
strcatn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = strlen(buf);
    char *dest = &buf[pos];

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }

    if (pos <= bufsize) {
        *dest = '\0';
    }

    return buf;
}

/**
 * Returns true if 's' is blank (just spaces or an empty string)
 *
 * @param s the string to check
 *
 * @return integer as described above.
 */
int
blank(const char *s)
{
    skip_whitespace(&s);

    return !(*s);
}

/**
 * Check to see if a string contains a number
 *
 * This allows a prefix of both '+' and '-'.  Leading whitespace is ignored.
 * This does not check for floats; only digits with an optional leading + or -
 *
 * @param s String to check
 * @return boolean as described above
 */
int
number(const char *s)
{
    if (!s)
        return 0;

    skip_whitespace(&s);

    if (*s == '+' || *s == '-')
        s++;

    if (!*s)
        return 0;

    for (; *s; s++)
        if (*s < '0' || *s > '9')
            return 0;

    return 1;
}

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
int
ifloat(const char *s)
{
    const char *hold;

    if (!s)
        return 0;

    skip_whitespace(&s);

    if (*s == '+' || *s == '-')
        s++;

    /**
     * @TODO: I see a number of places in the code where a reference to
     * improving float parsing is mentioned.  We should define what exactly
     * this means and make issues for them if there aren't already issues
     * out there.
     */

    /* WORK: for when float parsing is improved.
     * if (!strcasecmp(s, "inf")) {
     * return 1;
     * }
     * if (!strcasecmp(s, "nan")) {
     * return 1;
     * }
     */
    hold = s;

    while ((*s) && (*s >= '0' && *s <= '9'))
        s++;

    if ((!*s) || (s == hold))
        return 0;

    if (*s != '.')
        return 0;

    s++;
    hold = s;

    while ((*s) && (*s >= '0' && *s <= '9'))
        s++;

    if (hold == s)
        return 0;

    if (!*s)
        return 1;

    if ((*s != 'e') && (*s != 'E'))
        return 0;

    s++;

    if (*s == '+' || *s == '-')
        s++;

    hold = s;

    while ((*s) && (*s >= '0' && *s <= '9'))
        s++;

    if (s == hold)
        return 0;

    if (*s)
        return 0;

    return 1;
}

/* String handlers
 * Some of these are already present in most C libraries, but go by
 * different names or are not always there.  Since they're small, TF
 * simply uses its own routines with non-standard but consistant naming.
 */

/**
 * This is the fuzzball version of strchr
 *
 * Returns a pointer to the first occurance of 'c' in 's' or returns NULL.
 * The difference between this strchr and stdlib's is this one is
 * CASE INSENSITIVE.  At least my libc doesn't have a case-insensitive
 * strchr.
 *
 * @private
 * @param s the string to scan
 * @param c the character to find
 * @return a pointer to the position of c in s, or NULL if not found.
 */
static char *
cstrchr(char *s, char c)
{
    c = tolower(c);
    while (*s && tolower(*s) != c)
        s++;

    if (*s || !c)
        return s;
    else
        return NULL;
}

/**
 * This is a strchr variant that can skip escape characters.
 *
 * Returns a pointer to the first occurance of 'c' in 's' or returns NULL.
 * It will skip over characters 'e' which are considered escape characters
 * typically (so typically \)
 *
 * @private
 * @param s the string to scan
 * @param c the character to find
 * @param e the escape character
 * @return a pointer to the position of c in s, or NULL if not found.
 */
static char *
estrchr(char *s, char c, char e)
{
    while (*s) {
        if (*s == c)
            break;

        if (*s == e)
            s++;

        if (*s)
            s++;
    }

    if (*s)
        return s;
    else
        return NULL;
}

/**
 * Match a character class
 *
 * This is part of the smatch call and is used to handle the character
 * class (square brackets) expressions.  Note if you read this code,
 * it uses a funky test(...) #define as shown below this comment.
 *
 * It probably has no utility outside of smatch.
 *
 * @private
 * @param s1 the start of the character class (first character after [)
 * @param c1 the character to match
 * @return 0 if matched, 1 if not matched.
 */
#define test(x) if (tolower(x) == c1) return truthval
static int
cmatch(char *s1, char c1)
{
    int truthval = 0;

    if (c1 == '\0')
        return 1;

    c1 = tolower(c1);

    if (*s1 == '^') { /* This inverts the character class */
        s1++;
        truthval = 1;
    }

    if (*s1 == '-')
        test(*s1++);

    while (*s1) {
        if (*s1 == '\\' && *(s1 + 1))
            s1++;

        if (*s1 == '-') {
            char start = *(s1 - 1), end = *(s1 + 1);

            if (start > end) {
                test(*s1++);
            } else {
                for (char c = start; c <= end; c++)
                    test(c);
                s1 += 2;
            }
        } else
            test(*s1++);
    }

    return !truthval;
}

/*
 * See the definition for smatch for all the details.  It is a rather
 * lengthly comment so I don't want to repeat it here :)
 */
static int smatch(char *s1, char *s2);

/**
 * Does a word match as part of the smatch implementation
 *
 * This supports the { ... } operators.  Takes a word list and the
 * buffer to match from
 *
 * @param wlist the word list
 * @param s2 the buffer to match from
 * @return 0 if matched, 1 if not matched.
 */
static int
wmatch(char *wlist, char **s2)
{
    char *matchstr,     /* which word to find             */
    *strend,            /* end of current word from wlist */
    *matchbuf,          /* where to find from             */
    *bufend;            /* end of match buffer            */
    int result = 1;     /* intermediate result            */

    if (!wlist || !*s2)
        return 1;

    matchbuf = *s2;
    matchstr = wlist;
    bufend = strchr(matchbuf, ' ');

    if (bufend == NULL)
        *s2 += strlen(*s2);
    else {
        *s2 = bufend;
        *bufend = '\0';
    }

    do {
        if ((strend = estrchr(matchstr, '|', '\\')) != NULL)
            *strend = '\0';

        result = smatch(matchstr, matchbuf);

        if (strend != NULL)
            *strend++ = '|';

        if (!result)
            break;
    } while ((matchstr = strend) != NULL);

    if (bufend != NULL)
        *bufend = ' ';

    return result;
}

/**
 * This is a POSIX-style regex implementation.  I'm not sure why a standard
 * implementation isn't used, though this supports most of the POSIX regex
 * features.  The MUF man page for smatch describes what this supports:
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
 * @param s1 The pattern
 * @param s2 The string to match to the pattern
 * @return 0 if matched; otherwise it returns a non-0 number.
 */
static int
smatch(char *s1, char *s2)
{
    char ch, *start = s2;

    while (*s1) {
        switch (*s1) {
            case '\\':
                if (!*(s1 + 1)) {
                    return 1;
                } else {
                    s1++;

                    if (tolower(*s1++) != tolower(*s2++))
                        return 1;
                }

                break;
            case '?':
                if (!*s2++)
                    return 1;

                s1++;
                break;
            case '*':
                while (*s1 == '*' || (*s1 == '?' && *s2++))
                    s1++;

                if (*s1 == '?')
                    return 1;

                if (*s1 == '{') {
                    if (s2 == start)
                        if (!smatch(s1, s2))
                            return 0;

                    while ((s2 = strchr(s2, ' ')) != NULL)
                        if (!smatch(s1, ++s2))
                            return 0;

                    return 1;
                } else if (*s1 == '[') {
                    while (*s2)
                        if (!smatch(s1, s2++))
                            return 0;
                    return 1;
                }

                if (*s1 == '\\' && *(s1 + 1))
                    ch = *(s1 + 1);
                else
                    ch = *s1;

                while ((s2 = cstrchr(s2, ch)) != NULL) {
                    if (!smatch(s1, s2))
                        return 0;
                    s2++;
                }

                return 1;
            case '[':
                {
                    char *end;
                    int tmpflg;

                    if (!(end = estrchr(s1, ']', '\\'))) {
                        return 1;
                    }

                    *end = '\0';
                    tmpflg = cmatch(&s1[1], *s2++);
                    *end = ']';

                    if (tmpflg) {
                        return 1;
                    }

                    s1 = end + 1;
                }

                break;
            case '{':
                if (s2 != start && *(s2 - 1) != ' ')
                    return 1;

                {
                    char *end;
                    int tmpflg = 0;

                    if (s1[1] == '^')
                        tmpflg = 1;

                    if (!(end = estrchr(s1, '}', '\\'))) {
                        return 1;
                    }

                    *end = '\0';
                    tmpflg -= (wmatch(&s1[tmpflg + 1], &s2)) ? 1 : 0;
                    *end = '}';

                    if (tmpflg) {
                        return 1;
                    }

                    s1 = end + 1;
                }

                break;
            default:
                if (tolower(*s1++) != tolower(*s2++))
                    return 1;

                break;
        }
    }

    return tolower(*s1) - tolower(*s2);
}

/**
 * This is a POSIX-style regex implementation.  I'm not sure why a standard
 * implementation isn't used, though this supports most of the POSIX regex
 * features.  The MUF man page for smatch describes what this supports:
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
int
equalstr(char *pattern, char *str)
{
    return !smatch(pattern, str);
}

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
char *
cr2slash(char *buf, int buflen, const char *in)
{
    char *ptr = buf;
    const char *ptr2 = in;

    for (ptr = buf, ptr2 = in; *ptr2 && ptr - buf < buflen - 3; ptr2++) {
        if (*ptr2 == '\r') {
            *(ptr++) = '\\';
            *(ptr++) = 'r';
        } else if (*ptr2 == ESCAPE_CHAR) {
            *(ptr++) = '\\';
            *(ptr++) = '[';
        } else if (*ptr2 == '`') {
            *(ptr++) = '\\';
            *(ptr++) = '`';
        } else if (*ptr2 == '\\') {
            *(ptr++) = '\\';
            *(ptr++) = '\\';
        } else {
            *(ptr++) = *ptr2;
        }
    }

    *(ptr++) = '\0';
    return buf;
}

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
char *
stripspaces(char *s)
{
    char *ptr = s;
    skip_whitespace((const char **)&ptr);
    remove_ending_whitespace(&ptr);
    return ptr;
}


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
char *
string_substitute(const char *str, const char *oldstr, const char *newstr,
                  char *buf, size_t maxlen)
{
    const char *ptr = str;
    char *ptr2 = buf;
    size_t len = strlen(oldstr);
    size_t clen = 0;

    /* If the original string is empty, just copy the buffer over.  */
    if (len == 0) {
        strcpyn(buf, maxlen, str);
        return buf;
    }

    /* Iterate over the incoming string */
    while (*ptr && clen < (maxlen + 2)) {
        /* If the oldstring is found, we need to stuff the newstring
         * into the output buffer in place.
         */
        if (!strncmp(ptr, oldstr, len)) {
            for (const char *ptr3 = newstr;
                 ((ptr2 - buf) < (int)(maxlen - 2)) && *ptr3;)
                *(ptr2++) = *(ptr3++);
            ptr += len;
            clen += len;
        } else { /* Otherwise, advance our pointers. */
            *(ptr2++) = *(ptr++);
            clen++;
        }
    }

    /* Set the null and return */
    *ptr2 = '\0';
    return buf;
}

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
char *
ref2str(dbref obj, char *buf, size_t buflen)
{
    if (obj < -4 || obj >= db_top) {
        snprintf(buf, buflen, "Bad");
        return buf;
    }

    if (obj >= 0 && Typeof(obj) == TYPE_PLAYER) {
        snprintf(buf, buflen, "*%s", NAME(obj));
    } else {
        snprintf(buf, buflen, "#%d", obj);
    }

    return buf;
}


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
int
truestr(const char *buf)
{
    skip_whitespace(&buf);
    if (!*buf || (number(buf) && !atoi(buf)))
        return 0;
    return 1;
}

/**
 * Skip over starting whitespaces in a string
 *
 * Note that this does not technically "trim" spaces; instead, it advances
 * the provided pointer to the first non-space (or to the end of the string
 * as the case may be).
 *
 * @param parsebuf pointer to a string pointer.
 */
void
skip_whitespace(const char **parsebuf)
{
    while (**parsebuf && isspace(**parsebuf))
        (*parsebuf)++;
}

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
void
remove_ending_whitespace(char **s)
{
    char *p = *s + strlen(*s) - 1;

    while (p > *s && isspace(*p))
        *(p--) = '\0';
}

/**
 * Converts all characters in s to lower case.
 *
 * This call mutates the buffer pointed to by 's' to be all lower
 * case.  There is no actual reason for this to be a pointer to
 * a pointer; the pointer isn't mutated, just what it points to.
 *
 * @param s the string to lowercase.
 */
void
tolower_string(char **s)
{
    for (char *p = *s; *p; p++)
        *p = tolower(*p);
}

/**
 * Converts all characters in s to upper case.
 *
 * This call mutates the buffer pointed to by 's' to be all upper
 * case.  There is no actual reason for this to be a pointer to
 * a pointer; the pointer isn't mutated, just what it points to.
 *
 * @param s the string to uppercase.
 */
void
toupper_string(char **s)
{
   for (char *p = *s; *p; p++)
        *p = toupper(*p);
}
