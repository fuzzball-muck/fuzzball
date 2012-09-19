#ifndef _MESGPARSE_H
#define _MESGPARSE_H

#define MAX_MFUN_NAME_LEN 16
#define MAX_MFUN_LIST_LEN 512
#define MPI_MAX_VARIABLES 32
#define MPI_MAX_FUNCTIONS 32

#define MFUN_LITCHAR '`'
#define MFUN_LEADCHAR '{'
#define MFUN_ARGSTART ':'
#define MFUN_ARGSEP ','
#define MFUN_ARGEND '}'

#define UNKNOWN ((dbref)-88)
#define PERMDENIED ((dbref)-89)

#undef WIZZED_DELAY


int Wizperms(dbref what);

int safeputprop(dbref obj, dbref perms, char *buf, char *val, int mesgtyp);
const char *safegetprop(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int* blessed);
const char *safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int* blessed);
int safeblessprop(dbref obj, dbref perms, char *buf, int mesgtyp, int set_p);

char *stripspaces(char *buf, int buflen, char *in);
char *string_substitute(const char *str, const char *oldstr, const char *newstr, char *buf,

						int maxlen);
char *cr2slash(char *buf, int buflen, const char *in);

int get_list_count(dbref trig, dbref what, dbref perms, const char *listname, int mesgtyp, int* blessed);
const char *get_list_item(dbref trig, dbref what, dbref perms, const char *listname, int itemnum, int mesgtyp, int* blessed);
char *get_concat_list(dbref player, dbref what, dbref perms, dbref obj, const char *listname,
					  char *buf, int maxchars, int mode, int mesgtyp, int* blessed);

int isneighbor(dbref d1, dbref d2);
int mesg_read_perms(dbref player, dbref perms, dbref obj, int mesgtyp);
int mesg_local_perms(dbref player, dbref perms, dbref obj, int mesgtyp);

dbref mesg_dbref_raw(int descr, dbref player, dbref what, dbref perms, const char *buf);
dbref mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp);
dbref mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp);
dbref mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp);

char *ref2str(dbref obj, char *buf, size_t buflen);
int truestr(char *buf);

int check_mvar_overflow(int count);
int new_mvar(const char *varname, char *buf);
char *get_mvar(const char *varname);
int free_top_mvar(void);

int new_mfunc(const char *funcname, const char *buf);
const char *get_mfunc(const char *funcname);
int free_mfuncs(int downto);



#define MFUNARGS int descr, dbref player, dbref what, dbref perms, int argc, \
                argv_typ argv, char *buf, int buflen, int mesgtyp

#define CHECKRETURN(vari,funam,num) if (!vari) { snprintf(buf, BUFFER_LEN, "%s %c%s%c (%s)", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, num);  notify_nolisten(player, buf, 1);  return NULL; }

#define ABORT_MPI(funam,mesg) { snprintf(buf, BUFFER_LEN, "%s %c%s%c: %s", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, mesg);  notify_nolisten(player, buf, 1);  return NULL; }

typedef char **argv_typ;

#define MesgParse(in,out,outlen) mesg_parse(descr, player, what, perms, (in), (out), (outlen), mesgtyp)

#endif /* _MESGPARSE_H */

#ifdef DEFINE_HEADER_VERSIONS

#ifndef msgparseh_version
#define msgparseh_version
const char *msgparse_h_version = "$RCSfile: msgparse.h,v $ $Revision: 1.15 $";
#endif
#else
extern const char *msgparse_h_version;
#endif

