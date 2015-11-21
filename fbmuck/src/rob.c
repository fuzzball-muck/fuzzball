#include "config.h"

/* rob and kill */

#include "db.h"
#include "tune.h"
#include "match.h"
#include "externs.h"

void
do_rob(int descr, dbref player, const char *what)
{
	dbref thing;
	char buf[BUFFER_LEN];
	struct match_data md;

	init_match(descr, player, what, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(OWNER(player))) {
		match_absolute(&md);
		match_player(&md);
	}
	thing = match_result(&md);

	switch (thing) {
	case NOTHING:
		notify(player, "Rob whom?");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		if (Typeof(thing) != TYPE_PLAYER) {
			notify(player, "Sorry, you can only rob other players.");
		} else if (GETVALUE(thing) < 1) {
			snprintf(buf, sizeof(buf), "%s has no %s.", NAME(thing), tp_pennies);
			notify(player, buf);
			snprintf(buf, sizeof(buf),
					"%s tried to rob you, but you have no %s to take.",
					NAME(player), tp_pennies);
			notify(thing, buf);
		} else if (can_doit(descr, player, thing, "Your conscience tells you not to.")) {
			/* steal a penny */
			SETVALUE(OWNER(player), GETVALUE(OWNER(player)) + 1);
			DBDIRTY(player);
			SETVALUE(thing, GETVALUE(thing) - 1);
			DBDIRTY(thing);
			notify_fmt(player, "You stole a %s.", tp_penny);
			snprintf(buf, sizeof(buf), "%s stole one of your %s!", NAME(player), tp_pennies);
			notify(thing, buf);
		}
		break;
	}
}

void
do_kill(int descr, dbref player, const char *what, int cost)
{
	dbref victim;
	char buf[BUFFER_LEN];
	struct match_data md;

	init_match(descr, player, what, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(OWNER(player))) {
		match_player(&md);
		match_absolute(&md);
	}
	victim = match_result(&md);

	switch (victim) {
	case NOTHING:
		notify(player, "I don't see that player here.");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		if (Typeof(victim) != TYPE_PLAYER) {
			notify(player, "Sorry, you can only kill other players.");
		} else {
			/* go for it */
			/* set cost */
			if (cost < tp_kill_min_cost)
				cost = tp_kill_min_cost;

			if (FLAGS(DBFETCH(player)->location) & HAVEN) {
				notify(player, "You can't kill anyone here!");
				break;
			}

			if (tp_restrict_kill) {
				if (!(FLAGS(player) & KILL_OK)) {
					notify(player, "You have to be set Kill_OK to kill someone.");
					break;
				}
				if (!(FLAGS(victim) & KILL_OK)) {
					notify(player, "They don't want to be killed.");
					break;
				}
			}

			/* see if it works */
			if (!payfor(player, cost)) {
				notify_fmt(player, "You don't have enough %s.", tp_pennies);
			} else if ((RANDOM() % tp_kill_base_cost) < cost && !Wizard(OWNER(victim))) {
				/* you killed him */
				if (GETDROP(victim))
					/* give him the drop message */
					notify(player, GETDROP(victim));
				else {
					snprintf(buf, sizeof(buf), "You killed %s!", NAME(victim));
					notify(player, buf);
				}

				/* now notify everybody else */
				if (GETODROP(victim)) {
					snprintf(buf, sizeof(buf), "%s killed %s! ", NAME(player), NAME(victim));
					parse_oprop(descr, player, getloc(player), victim,
								   MESGPROP_ODROP, buf, "(@Odrop)");
				} else {
					snprintf(buf, sizeof(buf), "%s killed %s!", NAME(player), NAME(victim));
				}
				notify_except(DBFETCH(DBFETCH(player)->location)->contents, player, buf,
							  player);

				/* maybe pay off the bonus */
				if (GETVALUE(victim) < tp_max_pennies) {
					snprintf(buf, sizeof(buf), "Your insurance policy pays %d %s.",
							tp_kill_bonus, tp_pennies);
					notify(victim, buf);
					SETVALUE(victim, GETVALUE(victim) + tp_kill_bonus);
					DBDIRTY(victim);
				} else {
					notify(victim, "Your insurance policy has been revoked.");
				}
				/* send him home */
				send_home(descr, victim, 1);

			} else {
				/* notify player and victim only */
				notify(player, "Your murder attempt failed.");
				snprintf(buf, sizeof(buf), "%s tried to kill you!", NAME(player));
				notify(victim, buf);
			}
			break;
		}
	}
}

void
do_give(int descr, dbref player, const char *recipient, int amount)
{
	dbref who;
	char buf[BUFFER_LEN];
	struct match_data md;

	/* do amount consistency check */
	if (amount < 0 && !Wizard(OWNER(player))) {
		notify(player, "Try using the \"rob\" command.");
		return;
	} else if (amount == 0) {
		notify_fmt(player, "You must specify a positive number of %s.", tp_pennies);
		return;
	}
	/* check recipient */
	init_match(descr, player, recipient, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(OWNER(player))) {
		match_player(&md);
		match_absolute(&md);
	}
	switch (who = match_result(&md)) {
	case NOTHING:
		notify(player, "Give to whom?");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		return;
	default:
		if (!Wizard(OWNER(player))) {
			if (Typeof(who) != TYPE_PLAYER) {
				notify(player, "You can only give to other players.");
				return;
			} else if (GETVALUE(who) + amount > tp_max_pennies) {
				notify_fmt(player, "That player doesn't need that many %s!", tp_pennies);
				return;
			}
		}
		break;
	}

	/* try to do the give */
	if (!payfor(player, amount)) {
		notify_fmt(player, "You don't have that many %s to give!", tp_pennies);
	} else {
		/* he can do it */
		switch (Typeof(who)) {
		case TYPE_PLAYER:
			SETVALUE(who, GETVALUE(who) + amount);
			if(amount >= 0) {
				snprintf(buf, sizeof(buf), "You give %d %s to %s.",
						amount, amount == 1 ? tp_penny : tp_pennies, NAME(who));
				notify(player, buf);
				snprintf(buf, sizeof(buf), "%s gives you %d %s.",
						NAME(player), amount, amount == 1 ? tp_penny : tp_pennies);
				notify(who, buf);
			} else {
				snprintf(buf, sizeof(buf), "You take %d %s from %s.",
						-amount, amount == -1 ? tp_penny : tp_pennies, NAME(who));
				notify(player, buf);
				snprintf(buf, sizeof(buf), "%s takes %d %s from you!",
						NAME(player), -amount, -amount == 1 ? tp_penny : tp_pennies);
				notify(who, buf);
			}
			break;
		case TYPE_THING:
			SETVALUE(who, (GETVALUE(who) + amount));
			snprintf(buf, sizeof(buf), "You change the value of %s to %d %s.",
					NAME(who),
					GETVALUE(who), GETVALUE(who) == 1 ? tp_penny : tp_pennies);
			notify(player, buf);
			break;
		default:
			notify_fmt(player, "You can't give %s to that!", tp_pennies);
			break;
		}
		DBDIRTY(who);
	}
}
