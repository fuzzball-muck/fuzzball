#ifndef _GAME_H
#define _GAME_H

extern const char *compile_options;
extern short db_conversion_flag;
extern int force_level;
extern FILE *input_file;

void cleanup_game();
void fork_and_dump(void);
int init_game(const char *infile, const char *outfile);
void panic(const char *);
void process_command(int descr, dbref player, char *command);

#endif				/* _GAME_H */
