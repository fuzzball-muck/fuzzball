#include "config.h"

#include "boolexp.h"
#include "db.h"
#include "fbstrings.h"
#include "game.h"
#include "interface.h"
#include "interp.h"
#include "match.h"
#include "props.h"

static struct boolexp *
alloc_boolnode(void)
{
    return ((struct boolexp *) malloc(sizeof(struct boolexp)));
}

static void
free_boolnode(struct boolexp *ptr)
{
    free(ptr);
}

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
	o->thing = old->thing;
	break;
    case BOOLEXP_PROP:
	if (!old->prop_check) {
	    free_boolnode(o);
	    return 0;
	}
	o->prop_check = alloc_propnode(PropName(old->prop_check));
	SetPFlagsRaw(o->prop_check, PropFlagsRaw(old->prop_check));
	switch (PropType(old->prop_check)) {
	case PROP_STRTYP:
	    SetPDataStr(o->prop_check, alloc_string(PropDataStr(old->prop_check)));
	    break;
	default:
	    SetPDataVal(o->prop_check, PropDataVal(old->prop_check));
	    break;
	}
	break;
    default:
	panic("copy_bool(): Error in boolexp !");
    }
    return o;
}

static int
eval_boolexp_rec(int descr, dbref player, struct boolexp *b, dbref thing)
{
    if (b == TRUE_BOOLEXP) {
	return 1;
    } else {
	switch (b->type) {
	case BOOLEXP_AND:
	    return (eval_boolexp_rec(descr, player, b->sub1, thing)
		    && eval_boolexp_rec(descr, player, b->sub2, thing));
	case BOOLEXP_OR:
	    return (eval_boolexp_rec(descr, player, b->sub1, thing)
		    || eval_boolexp_rec(descr, player, b->sub2, thing));
	case BOOLEXP_NOT:
	    return !eval_boolexp_rec(descr, player, b->sub1, thing);
	case BOOLEXP_CONST:
	    if (b->thing == NOTHING)
		return 0;
	    if (Typeof(b->thing) == TYPE_PROGRAM) {
		struct inst *rv;
		struct frame *tmpfr;
		dbref real_player;

		if (Typeof(player) == TYPE_PLAYER || Typeof(player) == TYPE_THING)
		    real_player = player;
		else
		    real_player = OWNER(player);

		tmpfr = interp(descr, real_player, LOCATION(player),
			       b->thing, thing, PREEMPT, STD_HARDUID, 0);

		if (!tmpfr)
		    return (0);

		tmpfr->supplicant = player;
		tmpfr->argument.top--;
		push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_STRING, 0);
		rv = interp_loop(real_player, b->thing, tmpfr, 0);

		return (rv != NULL);
	    }
	    return (b->thing == player || b->thing == OWNER(player)
		    || member(b->thing, CONTENTS(player))
		    || b->thing == LOCATION(player));
	case BOOLEXP_PROP:
	    if (PropType(b->prop_check) == PROP_STRTYP) {
		if (OkObj(thing) &&
                    has_property_strict(descr, player, thing,
					PropName(b->prop_check),
					PropDataStr(b->prop_check), 0))
		    return 1;
		if (has_property(descr, player, player,
				 PropName(b->prop_check), PropDataStr(b->prop_check), 0))
		    return 1;
	    }
	    return 0;
	default:
	    panic("eval_boolexp_rec(): bad type !");
	}
    }
    return 0;
}

int
eval_boolexp(int descr, dbref player, struct boolexp *b, dbref thing)
{
    int result;

    b = copy_bool(b);
    result = eval_boolexp_rec(descr, player, b, thing);
    free_boolexp(b);
    return (result);
}

static struct boolexp *parse_boolexp_E(int descr, const char **parsebuf, dbref player, int dbloadp);	/* defined below */
static struct boolexp *parse_boolprop(char *buf);	/* defined below */

/* F -> (E); F -> !F; F -> object identifier */
static struct boolexp *
parse_boolexp_F(int descr, const char **parsebuf, dbref player, int dbloadp)
{
    struct boolexp *b;
    char *p;
    struct match_data md;
    char buf[BUFFER_LEN];

    skip_whitespace(parsebuf);
    switch (**parsebuf) {
    case '(':
	(*parsebuf)++;
	b = parse_boolexp_E(descr, parsebuf, player, dbloadp);
	skip_whitespace(parsebuf);
	if (b == TRUE_BOOLEXP || *(*parsebuf)++ != ')') {
	    free_boolexp(b);
	    return TRUE_BOOLEXP;
	} else {
	    return b;
	}
	/* break; */
    case NOT_TOKEN:
	(*parsebuf)++;
	b = alloc_boolnode();
	b->type = BOOLEXP_NOT;
	b->sub1 = parse_boolexp_F(descr, parsebuf, player, dbloadp);
	if (b->sub1 == TRUE_BOOLEXP) {
	    free_boolnode(b);
	    return TRUE_BOOLEXP;
	} else {
	    return b;
	}
	/* break */
    default:
	/* must have hit an object ref */
	/* load the name into our buffer */
	p = buf;
	while (**parsebuf
	       && **parsebuf != AND_TOKEN && **parsebuf != OR_TOKEN && **parsebuf != ')') {
	    *p++ = *(*parsebuf)++;
	}
	*p-- = '\0';

	p = buf;
	remove_ending_whitespace(&p);

	/* check to see if this is a property expression */
	if (index(buf, PROP_DELIMITER)) {
	    if (Prop_System(buf) || (!Wizard(OWNER(player)) && Prop_Hidden(buf))) {
		notify(player,
		       "Permission denied. (You cannot use a hidden property in a lock.)");
		return TRUE_BOOLEXP;
	    }
	    return parse_boolprop(buf);
	}
	b = alloc_boolnode();
	b->type = BOOLEXP_CONST;

	/* do the match */
	if (!dbloadp) {
	    init_match(descr, player, buf, TYPE_THING, &md);
	    match_neighbor(&md);
	    match_possession(&md);
	    match_me(&md);
	    match_here(&md);
	    match_absolute(&md);
	    match_registered(&md);
	    match_player(&md);
	    b->thing = match_result(&md);

	    if (b->thing == NOTHING) {
		notifyf(player, "I don't see %s here.", buf);
		free_boolnode(b);
		return TRUE_BOOLEXP;
	    } else if (b->thing == AMBIGUOUS) {
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
	    b->thing = (dbref) atoi(buf + 1);
            return b;
	}
	/* break */
    }
}

/* T -> F; T -> F & T */
static struct boolexp *
parse_boolexp_T(int descr, const char **parsebuf, dbref player, int dbloadp)
{
    struct boolexp *b;
    struct boolexp *b2;

    if ((b = parse_boolexp_F(descr, parsebuf, player, dbloadp)) == TRUE_BOOLEXP) {
	return b;
    } else {
	skip_whitespace(parsebuf);
	if (**parsebuf == AND_TOKEN) {
	    (*parsebuf)++;

	    b2 = alloc_boolnode();
	    b2->type = BOOLEXP_AND;
	    b2->sub1 = b;
	    if ((b2->sub2 = parse_boolexp_T(descr, parsebuf, player, dbloadp)) == TRUE_BOOLEXP) {
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

/* E -> T; E -> T | E */
static struct boolexp *
parse_boolexp_E(int descr, const char **parsebuf, dbref player, int dbloadp)
{
    struct boolexp *b;
    struct boolexp *b2;

    if ((b = parse_boolexp_T(descr, parsebuf, player, dbloadp)) == TRUE_BOOLEXP) {
	return b;
    } else {
	skip_whitespace(parsebuf);
	if (**parsebuf == OR_TOKEN) {
	    (*parsebuf)++;

	    b2 = alloc_boolnode();
	    b2->type = BOOLEXP_OR;
	    b2->sub1 = b;
	    if ((b2->sub2 = parse_boolexp_E(descr, parsebuf, player, dbloadp)) == TRUE_BOOLEXP) {
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

struct boolexp *
parse_boolexp(int descr, dbref player, const char *buf, int dbloadp)
{
    return parse_boolexp_E(descr, &buf, player, dbloadp);
}

/* parse a property expression
   If this gets changed, please also remember to modify set.c       */
static struct boolexp *
parse_boolprop(char *buf)
{
    const char *type = alloc_string(buf);
    char *strval = index(type, PROP_DELIMITER);
    const char *x;
    struct boolexp *b;
    PropPtr p;
    char *temp;

    x = type;
    b = alloc_boolnode();
    b->type = BOOLEXP_PROP;
    b->sub1 = b->sub2 = 0;
    b->thing = NOTHING;
    skip_whitespace(&type);

    if (*type == PROP_DELIMITER) {
	/* Oops!  Clean up and return a TRUE */
	free((void *) x);
	free_boolnode(b);
	return TRUE_BOOLEXP;
    }
    *strval = '\0';

    remove_ending_whitespace((char **)&type);

    strval++;
    skip_whitespace((const char **)&strval);
    if (!*strval) {
	/* Oops!  CLEAN UP AND RETURN A TRUE */
	free((void *) x);
	free_boolnode(b);
	return TRUE_BOOLEXP;
    }

    for (temp = (char *)strval; !isspace(*temp) && *temp; temp++) ;
    *temp = '\0';

    b->prop_check = p = alloc_propnode(type);
    SetPDataStr(p, alloc_string(strval));
    SetPType(p, PROP_STRTYP);
    free((void *) x);
    return b;
}


long
size_boolexp(struct boolexp *b)
{
    long result = 0L;

    if (b == TRUE_BOOLEXP) {
	return 0L;
    } else {
	result = sizeof(*b);
	switch (b->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
	    result += size_boolexp(b->sub2);
	case BOOLEXP_NOT:
	    result += size_boolexp(b->sub1);
	case BOOLEXP_CONST:
	    break;
	case BOOLEXP_PROP:
	    result += sizeof(*b->prop_check);
	    result += strlen(PropName(b->prop_check)) + 1;
	    if (PropDataStr(b->prop_check))
		result += strlen(PropDataStr(b->prop_check)) + 1;
	    break;
	default:
	    panic("size_boolexp(): bad type !");
	}
	return (result);
    }
}

void
free_boolexp(struct boolexp *b)
{
    if (b != TRUE_BOOLEXP) {
	switch (b->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
	    free_boolexp(b->sub1);
	    free_boolexp(b->sub2);
	    free_boolnode(b);
	    break;
	case BOOLEXP_NOT:
	    free_boolexp(b->sub1);
	    free_boolnode(b);
	    break;
	case BOOLEXP_CONST:
	    free_boolnode(b);
	    break;
	case BOOLEXP_PROP:
	    free_propnode(b->prop_check);
	    free_boolnode(b);
	    break;
	}
    }
}

static char boolexp_buf[BUFFER_LEN];
static char *buftop;

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
                unparse_object(player, b->thing, unparse_buf, sizeof(unparse_buf));
		strcpyn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf),
                        unparse_buf);
	    } else {
		snprintf(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf), "#%d",
			 b->thing);
	    }
	    buftop += strlen(buftop);
	    break;
	case BOOLEXP_PROP:
	    strcpyn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf),
		    PropName(b->prop_check));
	    strcatn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf), ":");
	    if (PropType(b->prop_check) == PROP_STRTYP)
		strcatn(buftop, sizeof(boolexp_buf) - (size_t)(buftop - boolexp_buf),
			PropDataStr(b->prop_check));
	    buftop += strlen(buftop);
	    break;
	default:
	    panic("unparse_boolexp1(): bad type !");
	}
    }
}

const char *
unparse_boolexp(dbref player, struct boolexp *b, int fullname)
{
    buftop = boolexp_buf;
    unparse_boolexp1(player, b, BOOLEXP_CONST, fullname);	/* no outer type */
    *buftop++ = '\0';

    return boolexp_buf;
}

int
test_lock(int descr, dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lokptr;

    lokptr = get_property_lock(thing, lockprop);
    return (eval_boolexp(descr, player, lokptr, thing));
}


int
test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lok;

    lok = get_property_lock(thing, lockprop);

    if (lok == TRUE_BOOLEXP)
        return 0;
    return (eval_boolexp(descr, player, lok, thing));
}
