#ifndef _COMPILE_H
#define _COMPILE_H

void clear_primitives(void);
void do_compile(int descr, dbref in_player, dbref in_program, int force_err_disp);
void free_unused_programs(void);
int get_primitive(const char *);
void init_primitives(void);
long size_prog(dbref prog);
void uncompile_program(dbref i);

#endif				/* _COMPILE_H */
