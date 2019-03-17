#ifndef _MATCH_H
#define _MATCH_H

#include "config.h"

struct match_data {
	dbref exact_match;       /* holds result of exact match */
	int check_keys;          /* if non-zero, check for keys */
	dbref last_match;        /* holds result of last match */
	int match_count;         /* holds total number of inexact matches */
	dbref match_who;         /* player used for me, here, and messages */
	dbref match_from;        /* object which is being matched around */
	int match_descr;         /* descriptor initiating the match */
	const char *match_name;  /* name to match */
	int preferred_type;      /* preferred type */
	int longest_match;       /* longest matched string */
	int match_level;         /* the highest priority level so far */
	int block_equals;        /* block matching of same name exits */
	int partial_exits;       /* if non-zero, allow exits to match partially */
};

dbref find_registered_obj(dbref player, const char *name);
void init_match(int descr, dbref player, const char *name, int type, struct match_data *md);
void init_match_check_keys(int descr, dbref player, const char *name, int type, struct match_data *md);
void init_match_remote(int descr, dbref player, dbref what, const char *name, int type, struct match_data *md);
dbref last_match_result(struct match_data *md);
void match_absolute(struct match_data *md);
void match_all_exits(struct match_data *md);
dbref match_controlled(int descr, dbref player, const char *name);
void match_everything(struct match_data *md);
void match_here(struct match_data *md);
void match_home(struct match_data *md);
void match_nil(struct match_data *md);
char *match_msg_ambiguous(const char *s, unsigned short types);
char *match_msg_nomatch(const char *s, unsigned short types);
void match_me(struct match_data *md);
void match_neighbor(struct match_data *md);
void match_player(struct match_data *md);
void match_possession(struct match_data *md);
void match_registered(struct match_data *md);
dbref match_result(struct match_data *md);	
void match_rmatch(dbref, struct match_data *md);
dbref noisy_match_result(struct match_data *md);

#endif				/* _MATCH_H */
