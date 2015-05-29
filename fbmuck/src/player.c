#include "copyright.h"
#include "config.h"

#include "db.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"

static hash_tab player_list[PLAYER_HASH_SIZE];

dbref
lookup_player(const char *name)
{
	hash_data *hd;

	if ((hd = find_hash(name, player_list, PLAYER_HASH_SIZE)) == NULL) {
		return NOTHING;
	} else {
		return (hd->dbval);
	}
}


int
check_password(dbref player, const char* password)
{
	char md5buf[64];
	const char *processed = password;
	const char *pword = PLAYER_PASSWORD(player);

	if (password == NULL) {
		MD5base64(md5buf, "", 0);
		processed = md5buf;
	} else {
		if (*password) {
			MD5base64(md5buf, password, strlen(password));
			processed = md5buf;
		}
	}

	if (!pword || !*pword)
		return 1;

	if (!strcmp(pword, processed))
		return 1;

	return 0;
}


void
set_password_raw(dbref player, const char* password)
{
	PLAYER_SET_PASSWORD(player, password);
	DBDIRTY(player);
}


void
set_password(dbref player, const char* password)
{
	char md5buf[64];
	const char *processed = password;

	if (*password) {
		MD5base64(md5buf, password, strlen(password));
		processed = md5buf;
	}
	
	if (PLAYER_PASSWORD(player))
		free((void *) PLAYER_PASSWORD(player));

	set_password_raw(player, alloc_string(processed));
}


dbref
connect_player(const char *name, const char *password)
{
	dbref player;

	if (*name == NUMBER_TOKEN && number(name + 1) && atoi(name + 1)) {
		player = (dbref) atoi(name + 1);
		if ((player < 0) || (player >= db_top) || (Typeof(player) != TYPE_PLAYER))
			player = NOTHING;
	} else {
		player = lookup_player(name);
	}
	if (player == NOTHING)
		return NOTHING;
	if (!check_password(player, password))
		return NOTHING;

	return player;
}

dbref
create_player(const char *name, const char *password)
{
	dbref player;

	if (!ok_player_name(name) || !ok_password(password))
		return NOTHING;

	/* else he doesn't already exist, create him */
	player = new_object();

	/* initialize everything */
	NAME(player) = alloc_string(name);
	DBFETCH(player)->location = tp_player_start;	/* home */
	FLAGS(player) = TYPE_PLAYER;
	OWNER(player) = player;
	ALLOC_PLAYER_SP(player);
	PLAYER_SET_HOME(player, tp_player_start);
	DBFETCH(player)->exits = NOTHING;

	SETVALUE(player, tp_start_pennies);
	set_password_raw(player, NULL);
	set_password(player, password);
	PLAYER_SET_CURR_PROG(player, NOTHING);
	PLAYER_SET_INSERT_MODE(player, 0);
	PLAYER_SET_IGNORE_CACHE(player, NULL);
	PLAYER_SET_IGNORE_COUNT(player, 0);
	PLAYER_SET_IGNORE_LAST(player, NOTHING);

	/* link him to tp_player_start */
	PUSH(player, DBFETCH(tp_player_start)->contents);
	add_player(player);
	DBDIRTY(player);
	DBDIRTY(tp_player_start);

	set_flags_from_tunestr(player, tp_pcreate_flags);

	return player;
}

void
do_password(dbref player, const char *old, const char *newobj)
{
	NOGUEST("@password",player);

	if (!PLAYER_PASSWORD(player) || !check_password(player, old)) {
		notify(player, "Sorry, old password did not match current password.");
	} else if (!ok_password(newobj)) {
		notify(player, "Bad new password (no spaces allowed).");
	} else {
		set_password(player, newobj);
		DBDIRTY(player);
		notify(player, "Password changed.");
	}
}

void
clear_players(void)
{
	kill_hash(player_list, PLAYER_HASH_SIZE, 0);
	return;
}

void
add_player(dbref who)
{
	hash_data hd;

	hd.dbval = who;
	if (add_hash(NAME(who), hd, player_list, PLAYER_HASH_SIZE) == NULL) {
		panic("Out of memory");
	} else {
		return;
	}
}


void
delete_player(dbref who)
{
	int result;
	char buf[BUFFER_LEN];
	char namebuf[BUFFER_LEN];
	int i, j;
	dbref found, ren;


	result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);

	if (result) {
		wall_wizards
				("## WARNING: Playername hashtable is inconsistent.  Rebuilding it.  Don't panic.");
		clear_players();
		for (i = 0; i < db_top; i++) {
			if (Typeof(i) == TYPE_PLAYER) {
				found = lookup_player(NAME(i));
				if (found != NOTHING) {
					ren = (i == who) ? found : i;
					j = 0;
					do {
						snprintf(namebuf, sizeof(namebuf), "%s%d", NAME(ren), ++j);
					} while (lookup_player(namebuf) != NOTHING);

					snprintf(buf, sizeof(buf),
							"## Renaming %s(#%d) to %s to prevent name collision.",
							NAME(ren), ren, namebuf);
					wall_wizards(buf);

					log_status("SANITY NAME CHANGE: %s(#%d) to %s", NAME(ren), ren, namebuf);

					if (ren == found) {
						free_hash(NAME(ren), player_list, PLAYER_HASH_SIZE);
					}
					if (NAME(ren)) {
						free((void *) NAME(ren));
					}
					ts_modifyobject(ren);
					NAME(ren) = alloc_string(namebuf);
					add_player(ren);
				} else {
					add_player(i);
				}
			}
		}
		result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);
		if (result) {
			wall_wizards
					("## WARNING: Playername hashtable still inconsistent.  Now you can panic.");
		}
	}

	return;
}
