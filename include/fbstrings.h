#ifndef _FBSTRINGS_H
#define _FBSTRINGS_H

#define DoNull(s) ((s) ? (s) : "")

#ifndef MALLOC_PROFILING
struct shared_string *alloc_prog_string(const char *);
char *alloc_string(const char *);
#endif
int alphanum_compare(const char *s1, const char *s2);
int blank(const char *s);
char *cr2slash(char *buf, int buflen, const char *in);
const char *exit_prefix(register const char *string, register const char *prefix);
int equalstr(char *s, char *t);
int ifloat(const char *s);
int is_valid_pose_separator(char ch);
char *intostr(int i);
int is_prop_prefix(const char *Property, const char *Prefix);
int number(const char *s);
void prefix_message(char *Dest, const char *Src, const char *Prefix, int BufferLength,
                           int SuppressIfPresent);
int prepend_string(char **before, char *start, const char *what);
char *pronoun_substitute(int descr, dbref player, const char *str);
char *ref2str(dbref obj, char *buf, size_t buflen);
void remove_ending_whitespace(char **s);
void skip_whitespace(const char **parsebuf);
char *strcatn(char *buf, size_t bufsize, const char *src);
char *strcpyn(char *buf, size_t bufsize, const char *src);
const char *strencrypt(const char *, const char *);
const char *strdecrypt(const char *, const char *);
const char *string_match(const char *src, const char *sub);
int string_prefix(const char *string, const char *prefix);
char *string_substitute(const char *str, const char *oldstr, const char *newstr, char *buf,
                        int maxlen);
char *strip_ansi(char *buf, const char *input);
char *strip_bad_ansi(char *buf, const char *input);
char *stripspaces(char *s);
void tolower_string(char **s);
void toupper_string(char **s);
int truestr(const char *buf);
#endif				/* _FBSTRINGS_H */
