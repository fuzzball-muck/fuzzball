#ifndef _COMPILE_H
#define _COMPILE_H

#define free_prog(i) free_prog_real(i,__FILE__,__LINE__);

#define MUF_AUTHOR_PROP		"_author"
#define MUF_DOCCMD_PROP		"_docs"
#define MUF_ERRCOUNT_PROP	".debug/errcount"
#define MUF_LASTCRASH_PROP	".debug/lastcrash"
#define MUF_LASTCRASHTIME_PROP	".debug/lastcrashtime"
#define MUF_LASTERR_PROP	".debug/lasterr"
#define MUF_LIB_VERSION_PROP	"_lib-version"
#define MUF_NOTE_PROP		"_note"
#define MUF_VERSION_PROP	"_version"

extern int IN_FOR;
extern int IN_FOREACH;
extern int IN_FORITER;
extern int IN_FORPOP;
extern int IN_TRYPOP;

void clear_primitives(void);
void do_compile(int descr, dbref in_player, dbref in_program, int force_err_disp);
void free_unused_programs(void);
int get_primitive(const char *);
void init_primitives(void);
int primitive(const char *s);
long size_prog(dbref prog);
void uncompile_program(dbref i);

#endif				/* _COMPILE_H */
