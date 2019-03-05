/** @file rob.c
 *
 * Source for some of the MUCK's more questionable features; robbing and
 * killing.  And also, randomly, giving.
 *
 * @TODO Might be nice to put 'do_score' here as well, then maybe make
 *       this 'pennies.c' instead of 'rob.c'.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

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

/**
 * Implementation of 'rob' command
 *
 * Steal pennies from a player if they don't have their lock set up
 * properly.  This is one of the most dubious commands in the MUCK.
 *
 * Uses can_doit for lock and messaging handling.
 *
 * @see can_doit
 *
 * @param descr the descriptor of the robber
 * @param player the robber
 * @param what the target of the robber's kleptomania.
 */
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

    /* @TODO The documentation (help robbery) suggests you can rob
     *       yourself to test your messages.  This is not the case;
     *       robbing yourself will always result in this failure message.
     *
     *       Either remove this check or fix the documentation.  I would
     *       probably prefer to fix the documentation
     */
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

    /* @TODO Consider making the 'rob' command tunable; perhaps tuning
     *       rob to 'off' flips the behavior so that unlocked players
     *       cannot be robbed, or perhaps robbing is just removed.
     *
     *       In my experience, 99% of the time this is used by jerks
     *       that find the command and go around harassing people with it.
     *       I don't think I've honestly ever seen a good and useful
     *       purpose for this command; it is easily implemented in MUF
     *       with stuff like skill or random checks instead of this
     *       weird lock behavior.
     *
     *       Maybe all this is an argument to just remove this stupid
     *       command altogether :)
     */
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

/**
 * Implementation of the kill command
 *
 * Rivalling 'rob' for most questionable command on the MUCK, this
 * command's sole purpose seems to be to cause newbie admins hassle until
 * they figure out how to turn it off.
 *
 * There is a shocking amount of logic around this command -- tune parameters,
 * its own flag, etc -- all for something incredibly pointless.  Anyway,
 * value judgements aside, this is how it works.
 *
 * Attempts to kill player 'what' for 'cost' which must be at least
 * tp_kill_min_cost amount.  The room must not be HAVEN, and the
 * players involved must be set KILL_OK if tp_restrict_kill is true.
 *
 * A random number between 0 and tp_kill_base_cost is selected, and if
 * cost is greater than that number, the target is killed.
 *
 * Killing someone results in some bland messaging and the killed person
 * being sent home.  The victim is paid tp_kill_bonus unless they are
 * already at tp_max_pennies or above.
 *
 * Drop and odrop messages are run on the victim on a successful kill.
 *
 * @param descr the descriptor of the killer
 * @param player the killer
 * @param what the target of killer's ire
 * @param cost how much the killer is willing to spend on this attempt.
 */
void
do_kill(int descr, dbref player, const char *what, int cost)
{
    dbref victim;
    char buf[BUFFER_LEN];
    struct match_data md;

    /* @TODO Consider removing this call.  Seriously, does any MUCK anywhere
     *       use this?  It is easily emulated in MUF if a MUCK wants it.
     *       We could even reclaim the 'K' flag for other uses, though
     *       practically speaking that is unlikely.  Practically speaking,
     *       I doubt this call will get removed, but I'll just put it out
     *       there as particularly useless.
     */

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
        notifyf(player, "You killed %s!", NAME(victim)); /* You bastard! */
    }

    if (GETODROP(victim)) {
        snprintf(buf, sizeof(buf), "%s killed %s! ", NAME(player), NAME(victim));
        parse_oprop(descr, player, LOCATION(player), victim, MESGPROP_ODROP, buf, "(@Odrop)");
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
