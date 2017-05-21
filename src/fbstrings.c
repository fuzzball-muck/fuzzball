#include "config.h"

#include "db.h"
#include "fbstrings.h"
#include "game.h"
#include "mpi.h"
#include "props.h"
#include "tune.h"

/*
 * routine to be used instead of strcasecmp() in a sorting routine
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 */
int
alphanum_compare(const char *t1, const char *s2)
{
    int n1, n2, cnt1, cnt2;
    const char *u1, *u2;
    register const char *s1 = t1;

    while (*s1 && tolower(*s1) == tolower(*s2))
	s1++, s2++;

    /* if at a digit, compare number values instead of letters. */
    if (isdigit(*s1) && isdigit(*s2)) {
	u1 = s1;
	u2 = s2;
	n1 = n2 = 0;		/* clear number values */
	cnt1 = cnt2 = 0;

	/* back up before zeros */
	if (s1 > t1 && *s2 == '0')
	    s1--, s2--;		/* curr chars are diff */
	while (s1 > t1 && *s1 == '0')
	    s1--, s2--;		/* prev chars are same */
	if (!isdigit(*s1))
	    s1++, s2++;

	/* calculate number values */
	while (isdigit(*s1))
	    cnt1++, n1 = (n1 * 10) + (*s1++ - '0');
	while (isdigit(*s2))
	    cnt2++, n2 = (n2 * 10) + (*s2++ - '0');

	/* if more digits than int can handle... */
	if (cnt1 > 8 || cnt2 > 8) {
	    if (cnt1 == cnt2)
		return (*u1 - *u2);	/* cmp chars if mag same */
	    return (cnt1 - cnt2);	/* compare magnitudes */
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

int
string_prefix(register const char *string, register const char *prefix)
{
    while (*string && *prefix && tolower(*string) == tolower(*prefix))
	string++, prefix++;
    return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
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

#define GENDER_UNASSIGNED   0x0	/* unassigned - the default */
#define GENDER_NEUTER       0x1	/* neuter */
#define GENDER_FEMALE       0x2	/* for women */
#define GENDER_MALE         0x3	/* for men */
#define GENDER_HERM         0x4	/* for hermaphrodites */

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/hirs/its, His/Hers/Hirs/Its)
 * %s/%S for subjective pronouns (he/she/sie/it, He/She/Sie/It)
 * %o/%O for objective pronouns (him/her/hir/it, Him/Her/Hir/It)
 * %p/%P for possessive pronouns (his/her/hir/its, His/Her/Hir/Its)
 * %r/%R for reflexive pronouns (himself/herself/hirself/itself,
 *                                Himself/Herself/Hirself/Itself)
 * %n    for the player's name.
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
    const char *self_sub;	/* self substitution code */
    const char *temp_sub;
    dbref mywhere = player;
    int sex;

    static const char *subjective[5] = { "", "it", "she", "he", "sie" };
    static const char *possessive[5] = { "", "its", "her", "his", "hir" };
    static const char *objective[5] = { "", "it", "her", "him", "hir" };
    static const char *reflexive[5] = { "", "itself", "herself", "himself", "hirself" };
    static const char *absolute[5] = { "", "its", "hers", "his", "hirs" };

    prn[0] = '%';
    prn[2] = '\0';

    strcpyn(orig, sizeof(orig), str);
    str = orig;

    sexstr = get_property_class(player, tp_gender_prop);
    if (sexstr) {
	sexstr = do_parse_mesg(descr, player, player, sexstr, "(Lock)", sexbuf, sizeof(sexbuf),
			       (MPI_ISPRIVATE | MPI_ISLOCK |
				(Prop_Blessed(player, tp_gender_prop) ? MPI_ISBLESSED : 0)));
    }

    if (sexstr)
	skip_whitespace(&sexstr);

    sex = GENDER_UNASSIGNED;

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

    result = buf;
    while (*str) {
	if (*str == '%') {
	    *result = '\0';
	    prn[1] = c = *(++str);
	    if (!c) {
		*(result++) = '%';
		continue;
	    } else if (c == '%') {
		*(result++) = '%';
		str++;
	    } else {
		mywhere = player;
		d = (isupper(c)) ? c : toupper(c);

		snprintf(globprop, sizeof(globprop), "%s/%.64s/%s", PRONOUNS_PROPDIR, sexstr, prn);
		if (d == 'A' || d == 'S' || d == 'O' || d == 'P' || d == 'R' || d == 'N') {
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
		    snprintf(globprop, sizeof(globprop), "%s/_default/%s", PRONOUNS_PROPDIR, prn);

		    if (!(self_sub = get_property_class(player, globprop)))
			self_sub = get_property_class(0, globprop);
		}

		if (self_sub) {
		    temp_sub = NULL;
		    if (self_sub[0] == '%' && toupper(self_sub[1]) == 'N') {
			temp_sub = self_sub;
			self_sub = NAME(player);
		    }
		    if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
			return buf;
		    strcatn(result, sizeof(buf) - (result - buf), self_sub);
		    if (isupper(prn[1]) && islower(*result))
			*result = toupper(*result);
		    result += strlen(result);
		    str++;
		    if (temp_sub) {
			if (((result - buf) + strlen(temp_sub + 2)) > (BUFFER_LEN - 2))
			    return buf;
			strcatn(result, sizeof(buf) - (result - buf), temp_sub + 2);
			result += strlen(result);
		    }
		} else if (sex == GENDER_UNASSIGNED) {
		    switch (c) {
		    case 'n':
		    case 'N':
		    case 'o':
		    case 'O':
		    case 's':
		    case 'S':
		    case 'r':
		    case 'R':
			strcatn(result, sizeof(buf) - (result - buf), NAME(player));
			break;
		    case 'a':
		    case 'A':
		    case 'p':
		    case 'P':
			strcatn(result, sizeof(buf) - (result - buf), NAME(player));
			strcatn(result, sizeof(buf) - (result - buf), "'s");
			break;
		    default:
			result[0] = *str;
			result[1] = 0;
			break;
		    }
		    str++;
		    result += strlen(result);
		    if ((result - buf) > (BUFFER_LEN - 2)) {
			buf[BUFFER_LEN - 1] = '\0';
			return buf;
		    }
		} else {
		    switch (c) {
		    case 'a':
		    case 'A':
			strcatn(result, sizeof(buf) - (result - buf), absolute[sex]);
			break;
		    case 's':
		    case 'S':
			strcatn(result, sizeof(buf) - (result - buf), subjective[sex]);
			break;
		    case 'p':
		    case 'P':
			strcatn(result, sizeof(buf) - (result - buf), possessive[sex]);
			break;
		    case 'o':
		    case 'O':
			strcatn(result, sizeof(buf) - (result - buf), objective[sex]);
			break;
		    case 'r':
		    case 'R':
			strcatn(result, sizeof(buf) - (result - buf), reflexive[sex]);
			break;
		    case 'n':
		    case 'N':
			strcatn(result, sizeof(buf) - (result - buf), NAME(player));
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
		    if ((result - buf) > (BUFFER_LEN - 2)) {
			buf[BUFFER_LEN - 1] = '\0';
			return buf;
		    }
		}
	    }
	} else {
	    if ((result - buf) > (BUFFER_LEN - 2)) {
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

char *
alloc_string(const char *string)
{
    char *s;

    /* NULL, "" -> NULL */
    if (!string || !*string)
	return 0;

    if ((s = (char *) malloc(strlen(string) + 1)) == 0) {
	abort();
    }
    strcpy(s, string);		/* Guaranteed enough space. */
    return s;
}

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
    bcopy(s, ss->data, ss->length + 1);
    return (ss);
}
#endif

char *
intostr(int i)
{
    static char num[16];
    int j, k;
    char *ptr2;

    k = i;
    ptr2 = num + 14;
    num[15] = '\0';
    if (i < 0)
	i = -i;
    while (i) {
	j = i % 10;
	*ptr2-- = '0' + j;
	i /= 10;
    }
    if (!k)
	*ptr2-- = '0';
    if (k < 0)
	*ptr2-- = '-';
    return (++ptr2);
}

/*
 * Encrypt one string with another one.
 */

#define CHARCOUNT 97

static unsigned char enarr[256];
static int charset_count[] = { 96, 97, 0 };

static int initialized_crypt = 0;

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

    seed = 0;
    for (cp = key; *cp; cp++)
	seed = ((((*cp) ^ seed) + 170) % 192);

    seed2 = 0;
    for (cp = data; *cp; cp++)
	seed2 = ((((*cp) ^ seed2) + 21) & 0xff);
    seed3 = seed2 = ((seed2 ^ (seed ^ (RANDOM() >> 24))) & 0x3f);

    count = seed + 11;
    for (upt = data, ups = buf, cp = key; *upt; upt++) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	seed2 = ((seed2 + 1) & 0x3f);
	if (!*(++cp))
	    cp = key;
	result = (enarr[(unsigned char) *upt] - (32 - (CHARCOUNT - 96))) + count + seed;
	*ups = enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
	count = (((*upt) ^ count) + seed) & 0xff;
	ups++;
    }
    *ups++ = '\0';

    ptr = linebuf;

    linelen = strlen(data);
    *ptr++ = (' ' + 2);
    *ptr++ = (' ' + seed3);
    limit--;
    limit--;

    for (cp = buf; cp < &buf[linelen]; cp++) {
	limit--;
	if (limit < 0)
	    break;
	*ptr++ = *cp;
    }
    *ptr++ = '\0';
    return linebuf;
}



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

    if (data[0] == '\0' || data[1] == '\0') {
        return "";
    }

    if ((data[0] - ' ') < 1 || (data[0] - ' ') > 2) {
	return "";
    }

    chrcnt = charset_count[(data[0] - ' ') - 1];
    seed2 = (data[1] - ' ');

    strcpyn(linebuf, sizeof(linebuf), data + 2);

    seed = 0;
    for (cp = key; *cp; cp++) {
	seed = (((*cp) ^ seed) + 170) % 192;
    }

    count = seed + 11;
    outlen = strlen(linebuf);
    upt = linebuf;
    ups = buf;
    cp = key;
    while ((const char *) upt < &linebuf[outlen]) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	if (!*(++cp))
	    cp = key;
	seed2 = ((seed2 + 1) & 0x3f);

	result = (enarr[(unsigned char) *upt] - (32 - (chrcnt - 96))) - (count + seed);
	while (result < 0)
	    result += chrcnt;
	*ups = enarr[result + (32 - (chrcnt - 96))];

	count = (((*ups) ^ count) + seed) & 0xff;
	ups++;
	upt++;
    }
    *ups++ = '\0';

    return buf;
}


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
		*os++ = *is++;	/* esc */
		*os++ = *is++;	/*  [  */
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

/* Prepends what before before, granted it doesn't come
 * before start in which case it returns 0.
 * Otherwise it modifies *before to point to that new location,
 * and it returns the number of chars prepended.
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

int
is_valid_pose_separator(char ch)
{
    return (ch == '\'') || (ch == ' ') || (ch == ',') || (ch == '-');
}

void
prefix_message(char *Dest, const char *Src, const char *Prefix, int BufferLength,
	       int SuppressIfPresent)
{
    int PrefixLength = strlen(Prefix);
    int CheckForHangingEnter = 0;

    while ((BufferLength > PrefixLength) && (*Src != '\0')) {
	if (*Src == '\r') {
	    Src++;
	    continue;
	}

	if (!SuppressIfPresent || strncmp(Src, Prefix, PrefixLength)
	    || (!is_valid_pose_separator(Src[PrefixLength]) && (Src[PrefixLength] != '\r')
		&& (Src[PrefixLength] != '\0')
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

    if (CheckForHangingEnter && (Dest[-1] == '\r'))
	Dest--;

    *Dest = '\0';
}

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

/*
 * Like strncpy, except it guarantees null termination of the result string.
 * It also has a more sensible argument ordering.
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


/*
 * Like strncat, except it takes the buffer size instead of the number
 * of characters to catenate.  It also has a more sensible argument order.
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

int
blank(const char *s)
{
    skip_whitespace(&s);

    return !(*s);
}

/* returns true for numbers of form [ + | - ] <series of digits> */
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

/* returns true for floats of form  [+|-]<digits>.<digits>[E[+|-]<digits>] */
int
ifloat(const char *s)
{
    const char *hold;

    if (!s)
	return 0;
    skip_whitespace(&s);
    if (*s == '+' || *s == '-')
	s++;
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

#define test(x) if (tolower(x) == c1) return truthval
/* Watch if-else constructions */

static int
cmatch(char *s1, char c1)
{
    int truthval = 0;

    if (c1 == '\0')
        return 1;

    c1 = tolower(c1);
    if (*s1 == '^') {
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

static int smatch(char *s1, char *s2);

static int
wmatch(char *wlist, char **s2)
	/* char   *wlist;          word list                      */
	/* char  **s2;         buffer to match from           */
{
    char *matchstr,		/* which word to find             */
    *strend,			/* end of current word from wlist */
    *matchbuf,			/* where to find from             */
    *bufend;			/* end of match buffer            */
    int result = 1;		/* intermediate result            */

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

int
equalstr(char *pattern, char *str)
{
    return !smatch(pattern, str);
}

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

char *
stripspaces(char *buf, int buflen, char *in)
{
    char *ptr;

    for (ptr = in; *ptr == ' '; ptr++) ;
    strcpyn(buf, buflen, ptr);
    if (!*buf)
        return buf;
    ptr = strlen(buf) + buf - 1;
    while (*ptr == ' ' && ptr > buf)
        *(ptr--) = '\0';
    return buf;
}


char *
string_substitute(const char *str, const char *oldstr, const char *newstr,
                  char *buf, int maxlen)
{
    const char *ptr = str;
    char *ptr2 = buf;
    int len = strlen(oldstr);
    int clen = 0;

    if (len == 0) {
        strcpyn(buf, maxlen, str);
        return buf;
    }
    while (*ptr && clen < (maxlen + 2)) {
        if (!strncmp(ptr, oldstr, len)) {
            for (const char *ptr3 = newstr; ((ptr2 - buf) < (maxlen - 2)) && *ptr3;)
                *(ptr2++) = *(ptr3++);
            ptr += len;
            clen += len;
        } else {
            *(ptr2++) = *(ptr++);
            clen++;
        }
    }
    *ptr2 = '\0';
    return buf;
}

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


int
truestr(const char *buf)
{
    skip_whitespace(&buf);
    if (!*buf || (number(buf) && !atoi(buf)))
        return 0;
    return 1;
}

void
skip_whitespace(const char **parsebuf)
{
    while (**parsebuf && isspace(**parsebuf))
        (*parsebuf)++;
}
