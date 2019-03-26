/** @file pennies.c
 *
 * Source for commands to do with money. Giving them, and viewing your "score".
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#include "commands.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "player.h"
#include "props.h"
#include "tune.h"

/**
 * Implementation of give command
 *
 * This transfers "pennies" or value from one player to another.
 * Or, if you are a wizard, you can use this to take pennies or set the
 * value of an object.
 *
 * This does all necessary permission checks and works pretty much how
 * you would expect.  Players cannot cause another player to exceed
 * tp_max_pennies
 *
 * @param descr the descriptor of the giving player
 * @param player the giving player
 * @param recipient the person receiving the gift
 * @param amount the amount to transfer.
 */
void
do_give(int descr, dbref player, const char *recipient, int amount)
{
    dbref who;
    struct match_data md;


    if (amount == 0 || (amount < 0 && !Wizard(OWNER(player)))) {
        notifyf(player, "You must specify a valid number of %s.", tp_pennies);
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

/**
 * Implementation of score command
 *
 * Which is how you see how many "pennies" you have.
 *
 * @param player the player doing the call
 */
void
do_score(dbref player)
{
    notifyf(player, "You have %d %s.", GETVALUE(player),
            GETVALUE(player) == 1 ? tp_penny : tp_pennies);
}
