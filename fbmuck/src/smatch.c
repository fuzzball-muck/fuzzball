/***********************************
 *                                 *
 * smatch string compare utility   *
 * Written by Explorer_Bob.        *
 * modified by Foxen               *
 *                                 *
 ***********************************/

#include "config.h"
/* #include <pwd.h> */

static int cmatch(char *s1, char c1);
static int wmatch(char *wlist, char **s2);
static int smatch(char *s1, char *s2);

char *cstrchr(char *s, char c);
char *estrchr(char *s, char c, char e);
int cstrcmp(char *s, char *t);
int cstrncmp(char *s, char *t, int n);

#ifdef STRSTR
char *strstr(char *s1, char *s2);

#endif
int equalstr(char *s, char *t);

#define DOWNCASE(x) (tolower(x))

/* String handlers
 * Some of these are already present in most C libraries, but go by
 * different names or are not always there.  Since they're small, TF
 * simply uses its own routines with non-standard but consistant naming.
 */

char *
cstrchr(char *s, char c)
{
	c = DOWNCASE(c);
	while (*s && DOWNCASE(*s) != c)
		s++;
	if (*s || !c)
		return s;
	else
		return NULL;
}

char *
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

#ifdef STRSTR
char *
strstr(char *s1, char *s2)
{
	register char *temp = s1;
	int len = strlen(s2);

	while (temp = strchr(temp, *s2)) {
		if (!strncmp(temp, s2, len))
			return temp;
		else
			temp++;
	}
	return NULL;
}

#endif

int
cstrcmp(char *s, char *t)
{
	while (*s && *t && DOWNCASE(*s) == DOWNCASE(*t)) {
		s++;
		t++;
	}
	return (DOWNCASE(*s) - DOWNCASE(*t));
}

int
cstrncmp(char *s, char *t, int n)
{
	for (; n && *s && *t && DOWNCASE(*s) == DOWNCASE(*t); s++, t++, n--) ;
	if (n <= 0)
		return 0;
	else
		return (DOWNCASE(*s) - DOWNCASE(*t));
}

#define test(x) if (DOWNCASE(x) == c1) return truthval
/* Watch if-else constructions */

static int
cmatch(char *s1, char c1)
{
	int truthval = FALSE;

	c1 = DOWNCASE(c1);
	if (*s1 == '^') {
		s1++;
		truthval = TRUE;
	}
	if (*s1 == '-')
		test(*s1++);
	while (*s1) {
		if (*s1 == '\\' && *(s1 + 1))
			s1++;
		if (*s1 == '-') {
			char c, start = *(s1 - 1), end = *(s1 + 1);

			if (start > end) {
				test(*s1++);
			} else {
				for (c = start; c <= end; c++)
					test(c);
				s1 += 2;
			}
		} else
			test(*s1++);
	}
	return !truthval;
}

static int
wmatch(char *wlist, char **s2)
	/* char   *wlist;          word list                      */
	/* char  **s2;         buffer to match from           */
{
	char *matchstr,				/* which word to find             */
	*strend,					/* end of current word from wlist */
	*matchbuf,					/* where to find from             */
	*bufend;					/* end of match buffer            */
	int result = 1;				/* intermediate result            */

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
				if (DOWNCASE(*s1++) != DOWNCASE(*s2++))
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
			if (DOWNCASE(*s1++) != DOWNCASE(*s2++))
				return 1;
			break;
		}
	}
	return DOWNCASE(*s1) - DOWNCASE(*s2);
}

int
equalstr(char *pattern, char *str)
{
	return !smatch(pattern, str);
}
