#include "config.h"

#include "commands.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "move.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

void
do_rob(int descr, dbref player, const char *what)
{
    dbref thing;
    struct match_data md;

    init_match(descr, player, what, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(OWNER(player))) {
	match_absolute(&md);
	match_player(&md);
    }

    if ((thing = noisy_match_result(&md)) == NOTHING) {
	return;
    }

    if (thing == player) {
	notify(player, "That seems unnecessary.");
	return;
    }

    if (Typeof(thing) != TYPE_PLAYER) {
	notify(player, "Sorry, you can only rob other players.");
	return;
    }


    if (GETVALUE(thing) < 1) {
	notifyf(player, "%s has no %s.", NAME(thing), tp_pennies);
	notifyf(thing, "%s tried to rob you, but you have no %s to take.",
		NAME(player), tp_pennies);
	return;
    }

    if (can_doit(descr, player, thing, "Your conscience tells you not to.")) {
	/* steal a penny */
	SETVALUE(OWNER(player), GETVALUE(OWNER(player)) + 1);
	DBDIRTY(player);
	SETVALUE(thing, GETVALUE(thing) - 1);
	DBDIRTY(thing);
	notifyf(player, "You stole a %s.", tp_penny);
	notifyf(thing, "%s stole one of your %s!", NAME(player), tp_pennies);
    }
}

void
do_kill(int descr, dbref player, const char *what, int cost)
{
    dbref victim;
    char buf[BUFFER_LEN];
    struct match_data md;

    if (FLAGS(LOCATION(player)) & HAVEN) {
	notify(player, "You can't kill anyone here!");
	return;
    }

    if (tp_restrict_kill && !(FLAGS(player) & KILL_OK)) {
	notify(player, "You have to be set Kill_OK to kill someone.");
	return;
    }

    init_match(descr, player, what, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(OWNER(player))) {
	match_player(&md);
	match_absolute(&md);
    }

    if ((victim = noisy_match_result(&md)) == NOTHING) {
	return;
    }

    if (victim == player || Typeof(victim) != TYPE_PLAYER) {
	notify(player, "Sorry, you can only kill other players.");
	return;
    }

    if (tp_restrict_kill && !(FLAGS(victim) & KILL_OK)) {
	notify(player, "They don't want to be killed.");
	return;
    }


    if (cost < tp_kill_min_cost) {
	cost = tp_kill_min_cost;
    }

    if (!payfor(player, cost)) {
	notifyf(player, "You don't have enough %s.", tp_pennies);
	return;
    }

    if ((RANDOM() % tp_kill_base_cost) >= cost || Wizard(OWNER(victim))) {
	notify(player, "Your murder attempt failed.");
	notifyf(victim, "%s tried to kill you!", NAME(player));
	return;
    }

    if (GETDROP(victim)) {
	notify(player, GETDROP(victim));
    } else {
	notifyf(player, "You killed %s!", NAME(victim));
    }

    if (GETODROP(victim)) {
	snprintf(buf, sizeof(buf), "%s killed %s! ", NAME(player), NAME(victim));
		parse_oprop(descr, player, LOCATION(player), victim,
		MESGPROP_ODROP, buf, "(@Odrop)");
    } else {
	snprintf(buf, sizeof(buf), "%s killed %s!", NAME(player), NAME(victim));
    }

    notify_except(CONTENTS(LOCATION(player)), player, buf, player);

    if (GETVALUE(victim) < tp_max_pennies) {
	notifyf(victim, "Your insurance policy pays %d %s.",
		tp_kill_bonus, tp_pennies);
	SETVALUE(victim, GETVALUE(victim) + tp_kill_bonus);
	DBDIRTY(victim);
    } else {
	notify(victim, "Your insurance policy has been revoked.");
    }

    send_home(descr, victim, 1);
}

void
do_give(int descr, dbref player, const char *recipient, int amount)
{
    dbref who;
    struct match_data md;


    if (amount < 0 && !Wizard(OWNER(player))) {
	notify(player, "Try using the \"rob\" command.");
	return;
    } else if (amount == 0) {
	notifyf(player, "You must specify a positive number of %s.", tp_pennies);
	return;
    }

    init_match(descr, player, recipient, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Wizard(OWNER(player))) {
	match_player(&md);
	match_absolute(&md);
    }

    if ((who = noisy_match_result(&md)) == NOTHING) {
	return;
    }

    if (who == player) {
	notify(player, "That seems unnecessary.");
	return;
    }

    if (!Wizard(OWNER(player))) {
	if (Typeof(who) != TYPE_PLAYER) {
	    notify(player, "You can only give to other players.");
	    return;
	} else if (GETVALUE(who) + amount > tp_max_pennies) {
	    notifyf(player, "That player doesn't need that many %s!", tp_pennies);
	    return;
	}
    }

    if (!payfor(player, amount)) {
	notifyf(player, "You don't have that many %s to give!", tp_pennies);
	return;
    }

    switch (Typeof(who)) {
    case TYPE_PLAYER:
	SETVALUE(who, GETVALUE(who) + amount);
	if (amount >= 0) {
	    notifyf(player, "You give %d %s to %s.",
		    amount, amount == 1 ? tp_penny : tp_pennies, NAME(who));
	    notifyf(who, "%s gives you %d %s.",
		    NAME(player), amount, amount == 1 ? tp_penny : tp_pennies);
	} else {
	    notifyf(player, "You take %d %s from %s.",
		    -amount, amount == -1 ? tp_penny : tp_pennies, NAME(who));
	    notifyf(who, "%s takes %d %s from you!",
		    NAME(player), -amount, -amount == 1 ? tp_penny : tp_pennies);
	}
	break;
    case TYPE_THING:
	SETVALUE(who, (GETVALUE(who) + amount));
	notifyf(player, "You change the value of %s to %d %s.",
		NAME(who), GETVALUE(who), GETVALUE(who) == 1 ? tp_penny : tp_pennies);
	break;
    default:
	notifyf(player, "You can't give %s to that!", tp_pennies);
	break;
    }

    DBDIRTY(who);
}
