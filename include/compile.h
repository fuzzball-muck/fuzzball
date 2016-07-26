#ifndef _COMPILE_H
#define _COMPILE_H

#define free_prog(i) free_prog_real(i,__FILE__,__LINE__);

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
