#ifndef _EDIT_H
#define _EDIT_H

struct macrotable {
    char *name;
    char *definition;
    dbref implementor;
    struct macrotable *left;
    struct macrotable *right;
};

extern struct macrotable *macrotop;

void chown_macros(dbref from, dbref to);
void free_old_macros();
void free_prog_text(struct line *l);
struct line *get_new_line(void);
void interactive(int descr, dbref player, const char *command);
void list_program(dbref player, dbref program, int *oarg, int argc);
char *macro_expansion(struct macrotable *node, const char *match);
void macrodump(struct macrotable *node, FILE * f);
void macroload(FILE * f);
struct line *read_program(dbref i);
void write_program(struct line *first, dbref i);


#endif				/* _EDIT_H */
