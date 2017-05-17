#ifndef _MSGPARSE_H
#define _MSGPARSE_H

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

struct mpivar {
    char name[MAX_MFUN_NAME_LEN + 1];
    char *buf;
};

struct mpifunc {
    char name[MAX_MFUN_NAME_LEN + 1];
    char *buf;
};

#define CHECKRETURN(vari,funam,num) if (!vari) { notifyf_nolisten(player, "%s %c%s%c (%s)", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, num); return NULL; }

#define ABORT_MPI(funam,mesg) { notifyf_nolisten(player, "%s %c%s%c: %s", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, mesg); return NULL; }

#define MesgParse(in,out,outlen) mesg_parse(descr, player, what, perms, (in), (out), (outlen), mesgtyp)

extern time_t mpi_prof_start_time;
extern int varc;

int check_mvar_overflow(int count);
int free_top_mvar(void);
char *get_concat_list(dbref player, dbref what, dbref perms, dbref obj, char *listname,
		      char *buf, int maxchars, int mode, int mesgtyp, int *blessed);
int get_list_count(dbref trig, dbref what, dbref perms, char *listname, int mesgtyp,
		   int *blessed);
const char *get_list_item(dbref trig, dbref what, dbref perms, char *listname, int itemnum,
			  int mesgtyp, int *blessed);
char *get_mvar(const char *varname);
int isneighbor(dbref d1, dbref d2);
dbref mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf, int mesgtyp);
dbref mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf,
		       int mesgtyp);
dbref mesg_dbref_raw(int descr, dbref player, dbref what, dbref perms, const char *buf);
dbref mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf,
			int mesgtyp);
void mesg_init(void);
int new_mfunc(const char *funcname, const char *buf);
int new_mvar(const char *varname, char *buf);
void purge_mfns(void);
int safeblessprop(dbref obj, dbref perms, char *buf, int mesgtyp, int set_p);
const char *safegetprop(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int *blessed);
const char *safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf, int mesgtyp, int *blessed);
int safeputprop(dbref obj, dbref perms, char *buf, char *val, int mesgtyp);

#endif				/* _MSGPARSE_H */
