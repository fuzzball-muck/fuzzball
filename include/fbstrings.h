#ifndef _FBSTRINGS_H
#define _FBSTRINGS_H

int alphanum_compare(const char *s1, const char *s2);
int blank(const char *s);
const char *exit_prefix(register const char *string, register const char *prefix);
int equalstr(char *s, char *t);
char *intostr(int i);
int ifloat(const char *s);
int is_prop_prefix(const char *Property, const char *Prefix);
int number(const char *s);
void prefix_message(char *Dest, const char *Src, const char *Prefix, int BufferLength,
                           int SuppressIfPresent);
int prepend_string(char **before, char *start, const char *what);
char *pronoun_substitute(int descr, dbref player, const char *str);
char *strcatn(char *buf, size_t bufsize, const char *src);
char *strcpyn(char *buf, size_t bufsize, const char *src);
const char *strencrypt(const char *, const char *);
const char *strdecrypt(const char *, const char *);
int string_compare(const char *s1, const char *s2);
#if !defined(MALLOC_PROFILING)
char *string_dup(const char *s);
#endif
const char *string_match(const char *src, const char *sub);
int string_prefix(const char *string, const char *prefix);
char *strip_ansi(char *buf, const char *input);
char *strip_bad_ansi(char *buf, const char *input);

#endif				/* _FBSTRINGS_H */
