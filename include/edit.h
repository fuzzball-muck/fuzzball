#ifndef _EDIT_H
#define _EDIT_H

struct macrotable {
    char *name;
    char *definition;
    dbref implementor;
    struct macrotable *left;
    struct macrotable *right;
};

#define MAX_ARG 2
#define EXIT_INSERT "."
#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
#define CANCEL_EDIT_COMMAND 'x'
#define COMPILE_COMMAND 'c'
#define LIST_COMMAND   'l'
#define EDITOR_HELP_COMMAND 'h'
#define KILL_COMMAND 'k'
#define SHOW_COMMAND 's'
#define SHORTSHOW_COMMAND 'a'
#define VIEW_COMMAND 'v'
#define UNASSEMBLE_COMMAND 'u'
#define NUMBER_COMMAND 'n'
#define PUBLICS_COMMAND 'p'

extern struct macrotable *macrotop;

void chown_macros(dbref from, dbref to);
void edit_program(dbref player, dbref program);
void free_old_macros();
void free_prog_text(struct line *l);
struct line *get_new_line(void);
void interactive(int descr, dbref player, const char *command);
void list_program(dbref player, dbref program, int *oarg, int argc, int comment_sysmsg);
char *macro_expansion(struct macrotable *node, const char *match);
void macrodump(struct macrotable *node, FILE * f);
void macroload(FILE * f);
struct line *read_program(dbref i);
void write_program(struct line *first, dbref i);


#endif				/* _EDIT_H */
