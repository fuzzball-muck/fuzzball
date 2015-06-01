#include "config.h"

#include "db.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "props.h"

const char *
unparse_flags(dbref thing)
{
	static char buf[BUFFER_LEN];
	char *p;
	const char *type_codes = "R-EPFG";

	p = buf;
	if (Typeof(thing) != TYPE_THING)
		*p++ = type_codes[Typeof(thing)];
	if (FLAGS(thing) & ~TYPE_MASK) {
		/* print flags */
		if (FLAGS(thing) & WIZARD)
			*p++ = 'W';
		if (FLAGS(thing) & LINK_OK)
			*p++ = 'L';

		if (FLAGS(thing) & KILL_OK)
			*p++ = 'K';

		if (FLAGS(thing) & DARK)
			*p++ = 'D';
		if (FLAGS(thing) & STICKY)
			*p++ = 'S';
		if (FLAGS(thing) & QUELL)
			*p++ = 'Q';
		if (FLAGS(thing) & BUILDER)
			*p++ = 'B';
		if (FLAGS(thing) & CHOWN_OK)
			*p++ = 'C';
		if (FLAGS(thing) & JUMP_OK)
			*p++ = 'J';
		if (FLAGS(thing) & HAVEN)
			*p++ = 'H';
		if (FLAGS(thing) & ABODE)
			*p++ = 'A';
		if (FLAGS(thing) & VEHICLE)
			*p++ = 'V';
		if (FLAGS(thing) & XFORCIBLE)
			*p++ = 'X';
		if (FLAGS(thing) & ZOMBIE)
			*p++ = 'Z';
                if (FLAGS(thing) & YIELD && tp_enable_match_yield)
                        *p++ = 'Y';
                if (FLAGS(thing) & OVERT && tp_enable_match_yield)
                        *p++ = 'O';
		if (MLevRaw(thing)) {
			*p++ = 'M';
			switch (MLevRaw(thing)) {
			case 1:
				*p++ = '1';
				break;
			case 2:
				*p++ = '2';
				break;
			case 3:
				*p++ = '3';
				break;
			}
		}
	}
	*p = '\0';
	return buf;
}

const char *
unparse_object(dbref player, dbref loc)
{
	static char buf[BUFFER_LEN];
	if (player == NOTHING)
		goto islog;
	if (Typeof(player) != TYPE_PLAYER)
		player = OWNER(player);
islog:
	switch (loc) {
	case NOTHING:
		return "*NOTHING*";
	case AMBIGUOUS:
		return "*AMBIGUOUS*";
	case HOME:
		return "*HOME*";
	default:
		if (loc < 0 || loc >= db_top)
			return "*INVALID*";

		if ((player == NOTHING) || (!(FLAGS(player) & STICKY) &&
			(can_link_to(player, NOTYPE, loc) ||
			 ((Typeof(loc) != TYPE_PLAYER) &&
			  (controls_link(player, loc) || (FLAGS(loc) & CHOWN_OK)))))) {
			/* show everything */
			snprintf(buf, sizeof(buf), "%.*s(#%d%s)", (BUFFER_LEN / 2), NAME(loc), loc, unparse_flags(loc));
			return buf;
		} else {
			/* show only the name */
			return NAME(loc);
		}
	}
}

static char boolexp_buf[BUFFER_LEN];
static char *buftop;

static void
unparse_boolexp1(dbref player, struct boolexp *b, boolexp_type outer_type, int fullname)
{
	if ((buftop - boolexp_buf) > (BUFFER_LEN / 2))
		return;
	if (b == TRUE_BOOLEXP) {
		strcpyn(buftop, sizeof(boolexp_buf) - (buftop - boolexp_buf), "*UNLOCKED*");
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
				strcpyn(buftop, sizeof(boolexp_buf) - (buftop - boolexp_buf), unparse_object(player, b->thing));
			} else {
				snprintf(buftop, sizeof(boolexp_buf) - (buftop - boolexp_buf), "#%d", b->thing);
			}
			buftop += strlen(buftop);
			break;
		case BOOLEXP_PROP:
			strcpyn(buftop, sizeof(boolexp_buf) - (buftop - boolexp_buf), PropName(b->prop_check));
			strcatn(buftop, sizeof(boolexp_buf) - (buftop - boolexp_buf), ":");
			if (PropType(b->prop_check) == PROP_STRTYP)
				strcatn(buftop, sizeof(boolexp_buf) - (buftop - boolexp_buf), PropDataStr(b->prop_check));
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
