/** @file boolexp.c
 *
 * Source for handling "boolean expressions", which is to say, the lock
 * type in Fuzzball.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "boolexp.h"
#include "db.h"
#include "fbstrings.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "match.h"
#include "mpi.h"
#include "props.h"

/**
 * Allocate a new boolexp struct
 *
 * @private
 * @return allocated memory for boolexp -- it is NOT zero'd out.
 */
static inline struct boolexp *
alloc_boolnode(void)
{
    return (malloc(sizeof(struct boolexp)));
}

/**
 * Free memory for a single boolexp node.  Does NOT recursively free children.
 *
 * @private
 * @param ptr the node to free.
 */
static inline void
free_boolnode(struct boolexp *ptr)
{
    free(ptr);
}

/**
 * Make a copy of a boolexp structure
 *
 * This is pretty straight forward; copies the entire boolexp structure
 * 'old', recursively.
 *
 * @param old the structure to copy
 * @return the copied structure -- be sure to use free_boolexp on it.
 */
struct boolexp *
copy_bool(struct boolexp *old)
{
    struct boolexp *o;

    if (old == TRUE_BOOLEXP)
        return TRUE_BOOLEXP;

    o = alloc_boolnode();

    if (!o)
        return 0;

    o->type = old->type;

    switch (old->type) {
        case BOOLEXP_AND:
        case BOOLEXP_OR:
            o->sub1 = copy_bool(old->sub1);
            o->sub2 = copy_bool(old->sub2);
            break;
        case BOOLEXP_NOT:
            o->sub1 = copy_bool(old->sub1);
            break;
        case BOOLEXP_CONST:
            o->data.thing = old->data.thing;
            break;
        case BOOLEXP_PROP:
            if (!old->data.prop_check) {
                free_boolnode(o);
                return 0;
            }

            o->data.prop_check = alloc_propnode(PropName(old->data.prop_check));
            SetPFlagsRaw(o->data.prop_check, PropFlagsRaw(old->data.prop_check));

            switch (PropType(old->data.prop_check)) {
                case PROP_STRTYP:
                    SetPDataStr(o->data.prop_check,
                                alloc_string(PropDataStr(old->data.prop_check)));
                    break;
                default:
                    SetPDataVal(o->data.prop_check, PropDataVal(old->data.prop_check));
                    break;
            }

            break;
        case BOOLEXP_MPI:
            o->data.mpi = old->data.mpi ? alloc_string(old->data.mpi) : NULL;
            break;
        default:
            panic("copy_bool(): Error in boolexp !");
    }

    return o;
}

/**
 * Recursively evaluate a boolean expression.
 *
 * This is used to actually navigate a boolexp tree of nodes and
 * do the logical evaluations.  The publically facing side of this is
 * eval_boolexp
 *
 * @see eval_boolexp
 *
 * How property checking works is somewhat complicated;
 * has_property_strict is called with the player and the 'thing' first,
 * then has_property is called with the player and the player second.
 * Only string props are supported.
 *
 * @see has_property_strict
 * @see has_property
 *
 * If a BOOLEXP_CONST is a dbref that belongs to a program, that program
 * is evaluated and as longa s it returns something other '0' or nothing
 * it will be counted as true.  Otherwise it looks for that DBREF in
 * a variety of ways (checks to see if its the player, checks to see if
 * its an object the player has, checks to see if its the player's location)
 *
 * Zombies are understood by this call and the owner of the zombie is
 * used in most places for locking purposes.
 *
 * @private
 * @param descr the descriptor that initiated the lock evaluation
 * @param player the person or thing that triggered the lock evaluation
 * @param b the boolexp node we are working on
 * @param thing the thing the lock is on.
 * @return boolean - 1 if lock checks out, 0 if it does not
 */
static int
eval_boolexp_rec(int descr, dbref player, struct boolexp *b, dbref thing)
{
    if (b == TRUE_BOOLEXP) {
        return 1;
    } else {
        switch (b->type) {
            /* AND, OR, and NOT are never leaf nodes, they must be
             * recursively evaluated.
             */
            case BOOLEXP_AND:
                return (eval_boolexp_rec(descr, player, b->sub1, thing)
                        && eval_boolexp_rec(descr, player, b->sub2, thing));
            case BOOLEXP_OR:
                return (eval_boolexp_rec(descr, player, b->sub1, thing)
                        || eval_boolexp_rec(descr, player, b->sub2, thing));
            case BOOLEXP_NOT:
                return !eval_boolexp_rec(descr, player, b->sub1, thing);

            /* CONST and PROP are both leaf nodes that will evaluate to
             * either true or false.
             */
            case BOOLEXP_CONST:
                if (b->data.thing == NOTHING)
                    return 0;

                /* Programs are evaluated */
                if (Typeof(b->data.thing) == TYPE_PROGRAM) {
                    struct inst *rv;
                    struct frame *tmpfr;
                    dbref real_player;

                    if (Typeof(player) == TYPE_PLAYER ||
                        Typeof(player) == TYPE_THING)
                        real_player = player;
                    else
                        real_player = OWNER(player);

                    tmpfr = interp(descr, real_player, LOCATION(player),
                                   b->data.thing, thing, PREEMPT, STD_HARDUID, 0);

                    if (!tmpfr)
                        return (0);

                    tmpfr->supplicant = player;
                    tmpfr->argument.top--;
                    push(tmpfr->argument.st, &(tmpfr->argument.top),
                         PROG_STRING, 0);
                    rv = interp_loop(real_player, b->data.thing, tmpfr, 0);

                    return (rv != NULL);
                }

                /* Fall back to checking to see if the dbref has
                 * any 'relationship' with the calling player.
                 */
                return (b->data.thing == player || b->data.thing == OWNER(player)
                        || member(b->data.thing, CONTENTS(player))
                        || b->data.thing == LOCATION(player));
            case BOOLEXP_PROP:
                /* Its for the best that only string props are supported,
                 * because in my code review of has_property I found
                 * out that there is a security leak if a prop is
                 * set to an integer type when the lock itself is
                 * expecting a string.
                 */
                if (PropType(b->data.prop_check) == PROP_STRTYP) {
                    if (OkObj(thing) &&
                        has_property_strict(descr, player, thing,
                                            PropName(b->data.prop_check),
                                            PropDataStr(b->data.prop_check), 0))
                        return 1;

                    if (has_property(descr, player, player,
                                     PropName(b->data.prop_check),
                                     PropDataStr(b->data.prop_check), 0))
                        return 1;
                }

                return 0;
            case BOOLEXP_MPI:
                if (b->data.mpi) {
                    char buf[BUFFER_LEN];
                    dbref real_player = (Typeof(player) == TYPE_PLAYER ||
                            Typeof(player) == TYPE_THING) ? player :
                            OWNER(player);
                    const char *result = do_parse_mesg(descr, real_player,
                            thing, b->data.mpi, "(Lock)", buf, sizeof(buf),
                            MPI_ISPRIVATE | MPI_ISLOCK);
                    return (result && *result && strcmp(result, "0") != 0);
                }
                return 0;
            default:
                /* This should never happen */
                panic("eval_boolexp_rec(): bad type !");
        }
    }

    return 0;
}

/**
 * Evaluate a boolean expression.
 *
 * How property checking works is somewhat complicated;
 * has_property_strict is called with the player and the 'thing' first,
 * then has_property is called with the player and the player second.
 * Only string props are supported.
 *
 * @see has_property_strict
 * @see has_property
 *
 * If a BOOLEXP_CONST is a dbref that belongs to a program, that program
 * is evaluated and as longa s it returns something other '0' or nothing
 * it will be counted as true.  Otherwise it looks for that DBREF in
 * a variety of ways (checks to see if its the player, checks to see if
 * its an object the player has, checks to see if its the player's location)
 *
 * Zombies are understood by this call and the owner of the zombie is
 * used in most places for locking purposes.
 *
 * This call temporarily copies the b boolexp structure to ensure the
 * original is not mutated.  I'm not sure if this is strictly necessary,
 * but that is how it is presently written.
 *
 * @param descr the descriptor that initiated the lock evaluation
 * @param player the person or thing that triggered the lock evaluation
 * @param b the boolexp node we are working on
 * @param thing the thing the lock is on.
 * @return boolean - 1 if lock checks out, 0 if it does not
 */
int
eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing)
{
    int result;

    b = copy_bool(b);
    result = eval_boolexp_rec(descr, player, b, thing);
    free_boolexp(b);
    return (result);
}

/* See comment for this below at definition */
static struct boolexp *parse_boolexp_E(int descr, const char **parsebuf,
                                       dbref player, int dbloadp, int depth);
/* See comment for this below at definition */
static struct boolexp *parse_boolprop(char *buf);

/**
 * Lowest level of the parse_boolexp family
 *
 * The parse_boolexp_* family are a series of calls that, together,
 * 'compile' a string into a set of lock nodes.
 *
 * The purpose of splitting this up is several-fold; firstly, it
 * implements an order of operations.  Secondly, it separates the
 * two-operand operations (AND and OR) from the single-operand expressions
 * (Parentesis, NOT, and constants).  Lastly, it allows easy recursion as
 * any of these elements may appear in any order.
 *
 * This level handles single-operand expressions and is the most atomic
 * part of the lock process.  It is called by step _T, though it may
 * call step _E to handle expressions in parenthesis.
 *
 * @see parse_boolexp
 * @see parse_boolexp_T
 * @see parse_boolexp_E
 *
 * @private
 * @param descr the descriptor belonging to the person compiling the lock
 * @param parsebuf the lock string
 * @param player the person or thing compiling the lock
 * @param dbloadp 1 if being called by the property disk loader, 0 otherwise
 * @param depth the current recursion depth
 * @return struct boolexp whatever node was created or TRUE_BOOLEXP on failure
 */
static struct boolexp *
parse_boolexp_F(int descr, const char **parsebuf, dbref player, int dbloadp, int depth)
{
    struct boolexp *b;
    char *p;
    struct match_data md;
    char buf[BUFFER_LEN];

    if (depth > MAX_LOCK_DEPTH) {
        notifyf_nolisten(player, "Lock expression too deeply nested.");
        return TRUE_BOOLEXP;
    }

    skip_whitespace(parsebuf);

    switch (**parsebuf) {
        case '(':
            /* If we find a (, then we have to recurse into it and parse
             * its interior expression.
             */
            (*parsebuf)++;
            b = parse_boolexp_E(descr, parsebuf, player, dbloadp, depth+1);
            skip_whitespace(parsebuf);

            /* If the sub expression had a problem, or we're not presently
             * at a terminating ), then return TRUE_BOOLEXP as an error.
             */
            if (b == TRUE_BOOLEXP || *(*parsebuf)++ != ')') {
                free_boolexp(b);
                return TRUE_BOOLEXP;
            } else {
                return b;
            }

            /* break; */
        case NOT_TOKEN:
            /* Because ! is a unary expression, we will recurse back into
             * _F to get the operand.
             */
            (*parsebuf)++;
            b = alloc_boolnode();
            b->type = BOOLEXP_NOT;
            b->sub1 = parse_boolexp_F(descr, parsebuf, player, dbloadp, depth+1);

            if (b->sub1 == TRUE_BOOLEXP) {
                free_boolnode(b);
                return TRUE_BOOLEXP;
            } else {
                return b;
            }

            /* break */
        case MFUN_LEADCHAR:
            {
                int brace_count = 1;
                const char *mpi_start = *parsebuf;
                (*parsebuf)++;

                while (**parsebuf != '\0' && brace_count > 0) {
                    if (**parsebuf == MFUN_LEADCHAR) {
                        brace_count++;
                    } else if (**parsebuf == MFUN_ARGEND) {
                        brace_count--;
                    }
                    (*parsebuf)++;
                }

                (*parsebuf)--;

                if (brace_count > 0) {
                    notifyf_nolisten(player, "Unclosed MPI expression in lock.");
                    return TRUE_BOOLEXP;
                }

                size_t mpi_len = *parsebuf - mpi_start + 1;
                if (mpi_len >= BUFFER_LEN) {
                    notifyf_nolisten(player, "MPI expression too long in lock.");
                    return TRUE_BOOLEXP;
                }

                strncpy(buf, mpi_start, mpi_len);
                buf[mpi_len] = '\0';

                b = alloc_boolnode();
                b->type = BOOLEXP_MPI;
                b->sub1 = b->sub2 = NULL;
                b->data.thing = NOTHING;
                b->data.prop_check = NULL;
                b->data.mpi = alloc_string(buf);

                (*parsebuf)++;
                return b;
            }
        default:
            /* must have hit an object ref */
            /* load the name into our buffer */
            p = buf;

            while (**parsebuf
                   && **parsebuf != AND_TOKEN && **parsebuf != OR_TOKEN
                   && **parsebuf != ')') {
                *p++ = *(*parsebuf)++;
            }

            *p-- = '\0';

            p = buf;
            remove_ending_whitespace(&p);

            /* check to see if this is a property expression */
            if (strchr(buf, PROP_DELIMITER)) {
                if (!dbloadp) {
                    if (Prop_System(buf) || (!Wizard(OWNER(player)) &&
                        Prop_Hidden(buf))) {
                        notify(player,
                               "Permission denied. (You cannot use a hidden property in a lock.)"
                        );

                        return TRUE_BOOLEXP;
                    }
                }

                return parse_boolprop(buf);
            }

            b = alloc_boolnode();
            b->type = BOOLEXP_CONST;

            /* do the match
             *
             * We only match if we're not loading from the DB.  If we
             * are loading from the DB, we're going to assume we are
             * getting a ref that starts with # and we are going to
             * trust that it is right.
             */
            if (!dbloadp) {
                init_match(descr, player, buf, TYPE_THING, &md);
                match_neighbor(&md);
                match_possession(&md);
                match_me(&md);
                match_here(&md);
                match_absolute(&md);
                match_registered(&md);
                match_player(&md);
                b->data.thing = match_result(&md);

                if (b->data.thing == NOTHING) {
                    notifyf(player, "I don't see %s here.", buf);
                    free_boolnode(b);
                    return TRUE_BOOLEXP;
                } else if (b->data.thing == AMBIGUOUS) {
                    notifyf(player, "I don't know which %s you mean!", buf);
                    free_boolnode(b);
                    return TRUE_BOOLEXP;
                } else {
                    return b;
                }
            } else {
                if (*buf != NUMBER_TOKEN || !number(buf + 1)) {
                    free_boolnode(b);
                    return TRUE_BOOLEXP;
                }

                b->data.thing = (dbref) atoi(buf + 1);
                return b;
            }

            /* break */
    }
}

/**
 * Middle level of the parse_boolexp family
 *
 * The parse_boolexp_* family are a series of calls that, together,
 * 'compile' a string into a set of lock nodes.
 *
 * The purpose of splitting this up is several-fold; firstly, it
 * implements an order of operations.  Secondly, it separates the
 * two-operand operations (AND and OR) from the single-operand expressions
 * (Parentesis, NOT, and constants).  Lastly, it allows easy recursion as
 * any of these elements may appear in any order.
 *
 * This level handles the '&' expression and calls the _F level to
 * compile the more atomic unary operations.
 *
 * @see parse_boolexp
 * @see parse_boolexp_F
 * @see parse_boolexp_E
 *
 * @private
 * @param descr the descriptor belonging to the person compiling the lock
 * @param parsebuf the lock string
 * @param player the person or thing compiling the lock
 * @param dbloadp 1 if being called by the property disk loader, 0 otherwise
 * @param depth the current recursion depth
 * @return struct boolexp whatever node was created or TRUE_BOOLEXP on failure
 */
static struct boolexp *
parse_boolexp_T(int descr, const char **parsebuf, dbref player, int dbloadp, int depth)
{
    struct boolexp *b;
    struct boolexp *b2;

    if (depth > MAX_LOCK_DEPTH) {
        notifyf_nolisten(player, "Lock expression too deeply nested.");
        return TRUE_BOOLEXP;
    }

    /* First process unary expressions if applicable */
    if ((b = parse_boolexp_F(descr, parsebuf, player, dbloadp, depth+1)) == TRUE_BOOLEXP) {
        /* This means there was an error */
        return b;
    } else {
        skip_whitespace(parsebuf);

        /* If there is an & expression, then process it and recurse back
         * into _T to process the second operand.
         */
        if (**parsebuf == AND_TOKEN) {
            (*parsebuf)++;

            b2 = alloc_boolnode();
            b2->type = BOOLEXP_AND;
            b2->sub1 = b;

            if ((b2->sub2 =
                 parse_boolexp_T(descr, parsebuf, player, dbloadp, depth+1)) ==
                 TRUE_BOOLEXP) {
                free_boolexp(b2);
                return TRUE_BOOLEXP;
            } else {
                return b2;
            }
        } else {
            return b;
        }
    }
}

/**
 * Top / entry point level of the parse_boolexp family
 *
 * The parse_boolexp_* family are a series of calls that, together,
 * 'compile' a string into a set of lock nodes.
 *
 * The purpose of splitting this up is several-fold; firstly, it
 * implements an order of operations.  Secondly, it separates the
 * two-operand operations (AND and OR) from the single-operand expressions
 * (Parentesis, NOT, and constants).  Lastly, it allows easy recursion as
 * any of these elements may appear in any order.
 *
 * This level is the entrypoint, and handles the processing of the OR
 * operator.  It calls the _T level.
 *
 * @see parse_boolexp
 * @see parse_boolexp_F
 * @see parse_boolexp_T
 *
 * @private
 * @param descr the descriptor belonging to the person compiling the lock
 * @param parsebuf the lock string
 * @param player the person or thing compiling the lock
 * @param dbloadp 1 if being called by the property disk loader, 0 otherwise
 * @param depth the current recursion depth
 * @return struct boolexp whatever node was created or TRUE_BOOLEXP on failure
 */
static struct boolexp *
parse_boolexp_E(int descr, const char **parsebuf, dbref player, int dbloadp, int depth)
{
    struct boolexp *b;
    struct boolexp *b2;

    if (depth > MAX_LOCK_DEPTH) {
        notifyf_nolisten(player, "Lock expression too deeply nested.");
        return TRUE_BOOLEXP;
    }

    if ((b = parse_boolexp_T(descr, parsebuf, player, dbloadp, depth+1)) ==
        TRUE_BOOLEXP) {
        /* This would have been an error */
        return b;
    } else {
        skip_whitespace(parsebuf);

        /* If we find an | operator, we need to call ourselves (_E) to get
         * the second operand.
         */
        if (**parsebuf == OR_TOKEN) {
            (*parsebuf)++;

            b2 = alloc_boolnode();
            b2->type = BOOLEXP_OR;
            b2->sub1 = b;

            if ((b2->sub2 =
                 parse_boolexp_E(descr, parsebuf, player, dbloadp, depth+1)) ==
                 TRUE_BOOLEXP) {
                free_boolexp(b2);
                return TRUE_BOOLEXP;
            } else {
                return b2;
            }
        } else {
            return b;
        }
    }
}

/**
 * Parse a boolean expression string into a boolexp object
 *
 * This essentially "compiles" a lock expression.  Some notes:
 *
 * * Ultimately, this only understands prop key-value pairs (values cannot
 *   have spaces), MUF return values, and DBREFs.  The syntax is very
 *   basic.
 *
 * * The expressions can have parenthesis grouping ( ) and it understands
 *   the logical operators & (and), | (or), and ! (not)
 *
 * * You should pass 0 to dbloadp unless you are a very special case.
 *   It is just used by the property disk loader.
 *
 * @param descr the descriptor belonging to the person compiling the lock
 * @param parsebuf the lock string
 * @param player the person or thing compiling the lock
 * @param dbloadp 1 if being called by the property disk loader, 0 otherwise
 * @return struct boolexp - the "head" of the boolean expression node tree.
 *         or TRUE_BOOLEXP on failure.
 */
struct boolexp *
parse_boolexp(int descr, dbref player, const char *buf, int dbloadp)
{
    return parse_boolexp_E(descr, &buf, player, dbloadp, 0);
}


/**
 * Parse a property expression in a boolean
 *
 * The original comment had a note:
 *
 *   If this gets changed, please also remember to modify set.c
 *
 * @TODO: Come back and document *why* set.c has to be modified once
 *        I'm done documenting set.c
 *
 * @TODO: There is no way to support a prop with spaces in the value.
 *        I don't imagine it comes up much, but we could probably come up
 *        with a way to support it.
 *
 * This is used internally by parse_boolexp to create a boolexp
 * structure (essentially a constant) from a property string
 * found in a lock expression.
 *
 * @private
 * @param buf the buffer being processed
 * @return a new boolexp structure or TRUE_BOOLEXP if there is any error.
 */
static struct boolexp *
parse_boolprop(char *buf)
{
    char *type = alloc_string(buf);
    char *strval = strchr(type, PROP_DELIMITER);
    char *x;
    struct boolexp *b;
    PropPtr p;
    char *temp;

    x = type;
    skip_whitespace_var(&type);

    /* This means there was no propname */
    if (*type == PROP_DELIMITER) {
        free(x);
        return TRUE_BOOLEXP;
    }

    /* By injecting this null, we are separating the prop name from
     * the prop value.
     */
    *strval = '\0';

    remove_ending_whitespace(&type);

    strval++;
    skip_whitespace_var(&strval);

    /* If there is no value, then we can't make a property node */
    if (!*strval) {
        free(x);
        return TRUE_BOOLEXP;
    }

    /* find the end of our prop value.  Note that it is impossible
     * to lock a prop to a value with spaces using this code.
     */
    for (temp = strval; !isspace(*temp) && *temp; temp++) ;
    *temp = '\0';

    /* Set up our propnode and boolnode, and then clean up our memory. */
    b = alloc_boolnode();
    b->type = BOOLEXP_PROP;
    b->sub1 = b->sub2 = 0;
    b->data.thing = NOTHING;
    b->data.prop_check = p = alloc_propnode(type);
    SetPDataStr(p, alloc_string(strval));
    SetPType(p, PROP_STRTYP);
    free(x);
    return b;
}

/**
 * Recursively compute the in-memory byte size of boolexp *b
 *
 * @param b the boolexp to compute the size on
 * @return the size in bytes
 */
size_t
size_boolexp(struct boolexp *b)
{
    size_t result = 0;

    if (b == TRUE_BOOLEXP) {
        return result;
    } else {
        result = sizeof(*b);

        switch (b->type) {
            case BOOLEXP_AND:
            case BOOLEXP_OR:
                result += size_boolexp(b->sub2);
                /* fall through */
            case BOOLEXP_NOT:
                result += size_boolexp(b->sub1);
                /* fall through */
            case BOOLEXP_CONST:
                break; /* CONST size is "baked into" sizeof *b */
            case BOOLEXP_PROP:
                result += sizeof(*b->data.prop_check);
                result += strlen(PropName(b->data.prop_check)) + 1;

                if (PropDataStr(b->data.prop_check))
                    result += strlen(PropDataStr(b->data.prop_check)) + 1;
                break;
            case BOOLEXP_MPI:
                if (b->data.mpi) {
                    result += strlen(b->data.mpi) + 1;
                }
                break;
            default:
                panic("size_boolexp(): bad type !");
        }

        return (result);
    }
}

/**
 * Recursively free a boolean expression structure
 *
 * This will clear all the child nodes, and any property node values
 * if necessary.
 *
 * @param b the structure to free
 */
void
free_boolexp(struct boolexp *b)
{
    if (b != TRUE_BOOLEXP) {
        switch (b->type) {
            case BOOLEXP_AND:
            case BOOLEXP_OR:
                free_boolexp(b->sub1);
                free_boolexp(b->sub2);
                break;
            case BOOLEXP_NOT:
                free_boolexp(b->sub1);
                break;
            case BOOLEXP_CONST:
                break;
            case BOOLEXP_PROP:
                free_propnode(b->data.prop_check);
                break;
            case BOOLEXP_MPI:
                if (b->data.mpi) {
                    free(b->data.mpi);
                }
                break;
        }
        free_boolnode(b);
    }
}

/* These statics are used by unparse_boolexp */
static char boolexp_buf[BUFFER_LEN];
static char *buftop;

/**
 * Interal method to translate a boolexp structure into something
 * human-readable
 *
 * Please note that this uses a static buffer of size BUFLEN, so you
 * must be aware that the buffer can mutate under you if another
 * unparse_boolexp happens.  Also, this means the call is not threadsafe.
 *
 * If fullname is true, then DBREFS will be run through unparse_object.
 * Otherwise, #12345 style DBREFs will be displayed.
 *
 * 'outer_type' is used to pass in the node-type of the parent to the
 * children.  It is used so the children can determine if there are
 * parens or not.  The top level node is given outer_type == BOOLEXP_CONST
 * as basically a non-typee.
 *
 * @see unparse_object
 *
 * @private
 * @param player the player who is going to see the output of this command
 * @param b the boolean expression to process
 * @param outer_type the type of the parent node
 * @param fullname Show names instead of DBREFs?  True or false.
 */ 
static void
unparse_boolexp1(dbref player, struct boolexp *b, short outer_type, int fullname)
{
    if ((size_t)(buftop - boolexp_buf) > (BUFFER_LEN / 2))
        return;

    if (b == TRUE_BOOLEXP) {
        strcpyn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf), PROP_UNLOCKED_VAL);
        buftop += strlen(buftop);
    } else {
        switch (b->type) {
            case BOOLEXP_AND:
                if (outer_type == BOOLEXP_NOT) {
                    *buftop++ = '(';
                }

                unparse_boolexp1(player, b->sub1, b->type, fullname);
                *buftop++ = AND_TOKEN;
                unparse_boolexp1(player, b->sub2, b->type, fullname);

                if (outer_type == BOOLEXP_NOT) {
                    *buftop++ = ')';
                }

                break;
            case BOOLEXP_OR:
                if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
                    *buftop++ = '(';
                }

                unparse_boolexp1(player, b->sub1, b->type, fullname);
                *buftop++ = OR_TOKEN;
                unparse_boolexp1(player, b->sub2, b->type, fullname);

                if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
                    *buftop++ = ')';
                }

                break;
            case BOOLEXP_NOT:
                *buftop++ = '!';
                unparse_boolexp1(player, b->sub1, b->type, fullname);
                break;
            case BOOLEXP_CONST:
                if (fullname) {
                    char unparse_buf[BUFFER_LEN];
                    unparse_object(player, b->data.thing, unparse_buf, sizeof(unparse_buf));
                    strcpyn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf),
                            unparse_buf);
                } else {
                    snprintf(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf), "#%d",
                             b->data.thing);
                }

                buftop += strlen(buftop);
                break;
            case BOOLEXP_PROP:
                strcpyn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf),
                        PropName(b->data.prop_check));
                strcatn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf), (char[]){PROP_DELIMITER,0});

                if (PropType(b->data.prop_check) == PROP_STRTYP)
                    strcatn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf),
                            PropDataStr(b->data.prop_check));

                buftop += strlen(buftop);
                break;
            case BOOLEXP_MPI:
                if (b->data.mpi) {
                    strcpyn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf), b->data.mpi);
                }
                buftop += strlen(buftop);
                break;
            default:
                panic("unparse_boolexp1(): bad type !");
        }
    }
}

/**
 * Translate a boolexp structure into something human-readable
 *
 * Please note that this uses a static buffer of size BUFLEN, so you
 * must be aware that the buffer can mutate under you if another
 * unparse_boolexp happens.  Also, this means the call is not threadsafe.
 *
 * If fullname is true, then DBREFS will be run through unparse_object.
 * Otherwise, #12345 style DBREFs will be displayed.
 *
 * @see unparse_object
 *
 * @param player the player who is going to see the output of this command
 * @param b the boolean expression to process
 * @param fullname Show names instead of DBREFs?  True or false.
 *
 * @return pointer to static buffer containing string for display.
 */ 
const char *
unparse_boolexp(dbref player, struct boolexp *b, int fullname)
{
    buftop = boolexp_buf;
    unparse_boolexp1(player, b, BOOLEXP_CONST, fullname);  /* no outer type */
    *buftop++ = '\0';

    return boolexp_buf;
}

/**
 * Test a lock - shortcut for a prop load and eval_boolexp
 *
 * This loads a prop 'lockprop' from 'thing' and tests it against 'player'.
 *
 * @see eval_boolexp
 *
 * @param descr the descriptor of the person triggering the lock
 * @param player the person or object triggering the lock
 * @param thing the thing the lock is on
 * @param lockprop the name of the lock property
 *
 * @return boolean result of eval_boolexp (1 for pass, 0 for fail)
 */
int
test_lock(int descr, dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lokptr;

    lokptr = GETLOCK(thing, lockprop);
    return (eval_boolexp(descr, player, lokptr, thing));
}

/**
 * Test a lock - if the lock does not exist, return failure.
 *
 * This loads a prop 'lockprop' from 'thing' and tests it against 'player'.
 * Unlike test_lock, if there is no lock property on the object, we will
 * return failure (false)
 *
 * @see eval_boolexp
 * @see test_lock
 *
 * @param descr the descriptor of the person triggering the lock
 * @param player the person or object triggering the lock
 * @param thing the thing the lock is on
 * @param lockprop the name of the lock property
 *
 * @return boolean result of eval_boolexp (1 for pass, 0 for fail)
 */
int
test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lok;

    lok = GETLOCK(thing, lockprop);

    if (lok == TRUE_BOOLEXP)
        return 0;

    return (eval_boolexp(descr, player, lok, thing));
}
