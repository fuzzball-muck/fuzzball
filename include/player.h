#ifndef PLAYER_H
#define PLAYER_H

#include "config.h"

void add_player(dbref who);
int check_password(dbref player, const char *password);
void change_player_name(dbref player, const char *name);
void clear_players(void);
dbref create_player(const char *name, const char *password);
void delete_player(dbref who);
dbref lookup_player(const char *name);
int ok_password(const char *password);
int payfor(dbref who, int cost);
void toad_player(int descr, dbref player, dbref victim, dbref recipient);
void set_password(dbref player, const char *password);
void set_password_raw(dbref player, const char *password);

#endif /* !PLAYER_H */
