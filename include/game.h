#ifndef _GAME_H
#define _GAME_H

void cleanup_game();
extern const char *compile_options;
extern short db_conversion_flag;
extern int force_level;
extern dbref force_prog;
void fork_and_dump(void);
extern FILE *input_file;

#endif				/* _GAME_H */
