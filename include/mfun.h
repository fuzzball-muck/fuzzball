#ifndef _MFUN_H
#define _MFUN_H

#define MFUNARGS int descr, dbref player, dbref what, dbref perms, int argc, \
        char **argv, char *buf, size_t buflen, int mesgtyp

const char *mfn_abs(MFUNARGS);
const char *mfn_add(MFUNARGS);
const char *mfn_and(MFUNARGS);
const char *mfn_attr(MFUNARGS);
const char *mfn_awake(MFUNARGS);
const char *mfn_bless(MFUNARGS);
const char *mfn_center(MFUNARGS);
const char *mfn_commas(MFUNARGS);
const char *mfn_concat(MFUNARGS);
const char *mfn_contains(MFUNARGS);
const char *mfn_contents(MFUNARGS);
const char *mfn_controls(MFUNARGS);
const char *mfn_convsecs(MFUNARGS);
const char *mfn_convtime(MFUNARGS);
const char *mfn_count(MFUNARGS);
const char *mfn_created(MFUNARGS);
const char *mfn_date(MFUNARGS);
const char *mfn_dbeq(MFUNARGS);
const char *mfn_debug(MFUNARGS);
const char *mfn_debugif(MFUNARGS);
const char *mfn_dec(MFUNARGS);
const char *mfn_default(MFUNARGS);
const char *mfn_delay(MFUNARGS);
const char *mfn_delprop(MFUNARGS);
const char *mfn_descr(MFUNARGS);
const char *mfn_dice(MFUNARGS);
const char *mfn_dist(MFUNARGS);
const char *mfn_div(MFUNARGS);
const char *mfn_eq(MFUNARGS);
const char *mfn_escape(MFUNARGS);
const char *mfn_eval(MFUNARGS);
const char *mfn_evalbang(MFUNARGS);
const char *mfn_exec(MFUNARGS);
const char *mfn_execbang(MFUNARGS);
const char *mfn_exits(MFUNARGS);
const char *mfn_filter(MFUNARGS);
const char *mfn_flags(MFUNARGS);
const char *mfn_fold(MFUNARGS);
const char *mfn_for(MFUNARGS);
const char *mfn_force(MFUNARGS);
const char *mfn_foreach(MFUNARGS);
const char *mfn_fox(MFUNARGS);
const char *mfn_ftime(MFUNARGS);
const char *mfn_fullname(MFUNARGS);
const char *mfn_func(MFUNARGS);
const char *mfn_ge(MFUNARGS);
const char *mfn_gt(MFUNARGS);
const char *mfn_holds(MFUNARGS);
const char *mfn_idle(MFUNARGS);
const char *mfn_if(MFUNARGS);
const char *mfn_inc(MFUNARGS);
const char *mfn_index(MFUNARGS);
const char *mfn_indexbang(MFUNARGS);
const char *mfn_instr(MFUNARGS);
const char *mfn_isdbref(MFUNARGS);
const char *mfn_isnum(MFUNARGS);
const char *mfn_istype(MFUNARGS);
const char *mfn_kill(MFUNARGS);
const char *mfn_lastused(MFUNARGS);
const char *mfn_lcommon(MFUNARGS);
const char *mfn_le(MFUNARGS);
const char *mfn_left(MFUNARGS);
const char *mfn_lexec(MFUNARGS);
const char *mfn_links(MFUNARGS);
const char *mfn_list(MFUNARGS);
const char *mfn_listprops(MFUNARGS);
const char *mfn_lit(MFUNARGS);
const char *mfn_lmember(MFUNARGS);
const char *mfn_loc(MFUNARGS);
const char *mfn_locked(MFUNARGS);
const char *mfn_log(MFUNARGS);
const char *mfn_lrand(MFUNARGS);
const char *mfn_lremove(MFUNARGS);
const char *mfn_lsort(MFUNARGS);
const char *mfn_lt(MFUNARGS);
const char *mfn_ltimestr(MFUNARGS);
const char *mfn_lunion(MFUNARGS);
const char *mfn_lunique(MFUNARGS);
const char *mfn_max(MFUNARGS);
const char *mfn_midstr(MFUNARGS);
const char *mfn_min(MFUNARGS);
const char *mfn_mklist(MFUNARGS);
const char *mfn_mod(MFUNARGS);
const char *mfn_modified(MFUNARGS);
const char *mfn_money(MFUNARGS);
const char *mfn_muckname(MFUNARGS);
const char *mfn_muf(MFUNARGS);
const char *mfn_mult(MFUNARGS);
const char *mfn_name(MFUNARGS);
const char *mfn_ne(MFUNARGS);
const char *mfn_nearby(MFUNARGS);
const char *mfn_nl(MFUNARGS);
const char *mfn_not(MFUNARGS);
const char *mfn_null(MFUNARGS);
const char *mfn_online(MFUNARGS);
const char *mfn_ontime(MFUNARGS);
const char *mfn_or(MFUNARGS);
const char *mfn_otell(MFUNARGS);
const char *mfn_owner(MFUNARGS);
const char *mfn_parse(MFUNARGS);
const char *mfn_pronouns(MFUNARGS);
const char *mfn_prop(MFUNARGS);
const char *mfn_propbang(MFUNARGS);
const char *mfn_propdir(MFUNARGS);
const char *mfn_rand(MFUNARGS);
const char *mfn_ref(MFUNARGS);
const char *mfn_revoke(MFUNARGS);
const char *mfn_right(MFUNARGS);
const char *mfn_secs(MFUNARGS);
const char *mfn_select(MFUNARGS);
const char *mfn_set(MFUNARGS);
const char *mfn_sign(MFUNARGS);
const char *mfn_smatch(MFUNARGS);
const char *mfn_stimestr(MFUNARGS);
const char *mfn_store(MFUNARGS);
const char *mfn_strip(MFUNARGS);
const char *mfn_strlen(MFUNARGS);
const char *mfn_sysparm(MFUNARGS);
const char *mfn_sublist(MFUNARGS);
const char *mfn_subst(MFUNARGS);
const char *mfn_subt(MFUNARGS);
const char *mfn_tab(MFUNARGS);
const char *mfn_tell(MFUNARGS);
const char *mfn_testlock(MFUNARGS);
const char *mfn_time(MFUNARGS);
const char *mfn_timing(MFUNARGS);
const char *mfn_timestr(MFUNARGS);
const char *mfn_timesub(MFUNARGS);
const char *mfn_tolower(MFUNARGS);
const char *mfn_toupper(MFUNARGS);
const char *mfn_type(MFUNARGS);
const char *mfn_tzoffset(MFUNARGS);
const char *mfn_unbless(MFUNARGS);
const char *mfn_usecount(MFUNARGS);
const char *mfn_v(MFUNARGS);
const char *mfn_version(MFUNARGS);
const char *mfn_while(MFUNARGS);
const char *mfn_with(MFUNARGS);
const char *mfn_xor(MFUNARGS);

struct mfun_dat {
    char *name;
    const char *(*mfn) (MFUNARGS);
    short parsep;
    short postp;
    short stripp;
    short minargs;
    short maxargs;
};

static struct mfun_dat mfun_list[] = {
    {"ABS", mfn_abs, 1, 0, 1, 1, 1},
    {"ADD", mfn_add, 1, 0, 1, 2, 9},
    {"AND", mfn_and, 0, 0, 1, 2, 9},
    {"ATTR", mfn_attr, 1, 0, 1, 2, 9},
    {"AWAKE", mfn_awake, 1, 0, 1, 1, 1},
    {"BLESS", mfn_bless, 1, 0, 1, 1, 2},
    {"CENTER", mfn_center, 1, 0, 0, 1, 3},
    {"COMMAS", mfn_commas, 0, 0, 0, 1, 4},
    {"CONCAT", mfn_concat, 1, 0, 1, 1, 2},
    {"CONTAINS", mfn_contains, 1, 0, 1, 1, 2},
    {"CONTENTS", mfn_contents, 1, 0, 1, 1, 2},
    {"CONTROLS", mfn_controls, 1, 0, 1, 1, 2},
    {"CONVSECS", mfn_convsecs, 1, 0, 1, 1, 1},
    {"CONVTIME", mfn_convtime, 1, 0, 1, 1, 1},
    {"COUNT", mfn_count, 1, 0, 0, 1, 2},
    {"CREATED", mfn_created, 1, 0, 1, 1, 1},
    {"DATE", mfn_date, 1, 0, 1, 0, 1},
    {"DBEQ", mfn_dbeq, 1, 0, 1, 2, 2},
    {"DEBUG", mfn_debug, 0, 0, 0, 1, 1},
    {"DEBUGIF", mfn_debugif, 0, 0, 0, 2, 2},
    {"DEC", mfn_dec, 1, 0, 1, 1, 2},
    {"DEFAULT", mfn_default, 0, 0, 0, 2, 2},
    {"DELAY", mfn_delay, 1, 0, 1, 2, 2},
    {"DELPROP", mfn_delprop, 1, 0, 1, 1, 2},
    {"DESCR", mfn_descr, 1, 0, 1, 0, 0},
    {"DICE", mfn_dice, 1, 0, 1, 1, 3},
    {"DIST", mfn_dist, 1, 0, 1, 2, 6},
    {"DIV", mfn_div, 1, 0, 1, 2, 9},
    {"ESCAPE", mfn_escape, 1, 0, 0, 1, 1},
    {"EQ", mfn_eq, 1, 0, 0, 2, 2},
    {"EVAL", mfn_eval, 1, 0, 0, 1, 1},
    {"EVAL!", mfn_evalbang, 1, 0, 0, 1, 1},
    {"EXEC", mfn_exec, 1, 0, 1, 1, 2},
    {"EXEC!", mfn_execbang, 1, 0, 1, 1, 2},
    {"EXITS", mfn_exits, 1, 0, 1, 1, 1},
    {"FILTER", mfn_filter, 0, 0, 0, 3, 5},
    {"FLAGS", mfn_flags, 1, 0, 1, 1, 1},
    {"FOLD", mfn_fold, 0, 0, 0, 4, 5},
    {"FOR", mfn_for, 0, 0, 0, 5, 5},
    {"FORCE", mfn_force, 1, 0, 1, 2, 2},
    {"FOREACH", mfn_foreach, 0, 0, 0, 3, 4},
    {"FOX", mfn_fox, 0, 0, 0, 0, 0},
    {"FTIME", mfn_ftime, 1, 0, 0, 1, 3},
    {"FULLNAME", mfn_fullname, 1, 0, 1, 1, 1},
    {"FUNC", mfn_func, 0, 0, 1, 2, 9},	/* define a function */
    {"GE", mfn_ge, 1, 0, 1, 2, 2},
    {"GT", mfn_gt, 1, 0, 1, 2, 2},
    {"HOLDS", mfn_holds, 1, 0, 1, 1, 2},
    {"IDLE", mfn_idle, 1, 0, 1, 1, 1},
    {"IF", mfn_if, 0, 0, 0, 2, 3},
    {"INC", mfn_inc, 1, 0, 1, 1, 2},
    {"INDEX", mfn_index, 1, 0, 1, 1, 2},
    {"INDEX!", mfn_indexbang, 1, 0, 1, 1, 2},
    {"INSTR", mfn_instr, 1, 0, 0, 2, 2},
    {"ISDBREF", mfn_isdbref, 1, 0, 1, 1, 1},
    {"ISNUM", mfn_isnum, 1, 0, 1, 1, 1},
    {"ISTYPE", mfn_istype, 1, 0, 1, 2, 2},
    {"KILL", mfn_kill, 1, 0, 1, 1, 1},
    {"LASTUSED", mfn_lastused, 1, 0, 1, 1, 1},
    {"LCOMMON", mfn_lcommon, 1, 0, 0, 2, 2},	/* items in both 1 & 2 */
    {"LE", mfn_le, 1, 0, 1, 2, 2},
    {"LEFT", mfn_left, 1, 0, 0, 1, 3},
    {"LEXEC", mfn_lexec, 1, 0, 1, 1, 2},
    {"LINKS", mfn_links, 1, 0, 1, 1, 1},
    {"LIST", mfn_list, 1, 0, 1, 1, 2},
    {"LISTPROPS", mfn_listprops, 1, 0, 1, 1, 3},
    {"LIT", mfn_lit, 0, 0, 0, 1, -1},
    {"LMEMBER", mfn_lmember, 1, 0, 0, 2, 3},
    {"LOC", mfn_loc, 1, 0, 1, 1, 1},
    {"LOCKED", mfn_locked, 1, 0, 1, 2, 2},
    {"LRAND", mfn_lrand, 1, 0, 0, 1, 2},	/* returns random list item */
    {"LREMOVE", mfn_lremove, 1, 0, 0, 2, 2},	/* items in 1 not in 2 */
    {"LSORT", mfn_lsort, 0, 0, 0, 1, 4},	/* sort list items */
    {"LT", mfn_lt, 1, 0, 1, 2, 2},
    {"LTIMESTR", mfn_ltimestr, 1, 0, 1, 1, 1},
    {"LUNION", mfn_lunion, 1, 0, 0, 2, 2},	/* items from both */
    {"LUNIQUE", mfn_lunique, 1, 0, 0, 1, 1},	/* make items unique */
    {"MAX", mfn_max, 1, 0, 1, 2, 2},
    {"MIDSTR", mfn_midstr, 1, 0, 0, 2, 3},
    {"MIN", mfn_min, 1, 0, 1, 2, 2},
    {"MKLIST", mfn_mklist, 1, 0, 0, 0, 9},
    {"MOD", mfn_mod, 1, 0, 1, 2, 9},
    {"MODIFIED", mfn_modified, 1, 0, 1, 1, 1},
    {"MONEY", mfn_money, 1, 0, 1, 1, 1},
    {"MUCKNAME", mfn_muckname, 0, 0, 0, 0, 0},
    {"MUF", mfn_muf, 1, 0, 0, 2, 2},
    {"MULT", mfn_mult, 1, 0, 1, 2, 9},
    {"NAME", mfn_name, 1, 0, 1, 1, 1},
    {"NE", mfn_ne, 1, 0, 0, 2, 2},
    {"NEARBY", mfn_nearby, 1, 0, 1, 1, 2},
    {"NL", mfn_nl, 0, 0, 0, 0, 0},
    {"NOT", mfn_not, 1, 0, 1, 1, 1},
    {"NULL", mfn_null, 1, 0, 0, 0, 9},
    {"ONLINE", mfn_online, 0, 0, 0, 0, 0},
    {"ONTIME", mfn_ontime, 1, 0, 1, 1, 1},
    {"OR", mfn_or, 0, 0, 1, 2, 9},
    {"OTELL", mfn_otell, 1, 0, 0, 1, 3},
    {"OWNER", mfn_owner, 1, 0, 1, 1, 1},
    {"PARSE", mfn_parse, 0, 0, 0, 3, 5},
    {"PRONOUNS", mfn_pronouns, 1, 0, 0, 1, 2},
    {"PROP", mfn_prop, 1, 0, 1, 1, 2},
    {"PROP!", mfn_propbang, 1, 0, 1, 1, 2},
    {"PROPDIR", mfn_propdir, 1, 0, 1, 1, 2},
    {"RAND", mfn_rand, 1, 0, 1, 1, 2},
    {"RIGHT", mfn_right, 1, 0, 0, 1, 3},
    {"REF", mfn_ref, 1, 0, 1, 1, 1},
    {"REVOKE", mfn_revoke, 0, 0, 0, 1, 1},
    {"SECS", mfn_secs, 0, 0, 0, 0, 0},
    {"SELECT", mfn_select, 1, 0, 1, 2, 3},
    {"SET", mfn_set, 1, 0, 0, 2, 2},
    {"SIGN", mfn_sign, 1, 0, 1, 1, 1},
    {"SMATCH", mfn_smatch, 1, 0, 0, 2, 2},
    {"STIMESTR", mfn_stimestr, 1, 0, 1, 1, 1},
    {"STORE", mfn_store, 1, 0, 1, 2, 3},
    {"STRIP", mfn_strip, 1, 0, 0, 1, -1},
    {"STRLEN", mfn_strlen, 1, 0, 0, 1, 1},
    {"SYSPARM", mfn_sysparm, 1, 0, 1, 1, 1},
    {"SUBLIST", mfn_sublist, 1, 0, 0, 1, 4},
    {"SUBST", mfn_subst, 1, 0, 0, 3, 3},
    {"SUBT", mfn_subt, 1, 0, 1, 2, 9},
    {"TAB", mfn_tab, 0, 0, 0, 0, 0},
    {"TESTLOCK", mfn_testlock, 1, 0, 1, 2, 4},
    {"TELL", mfn_tell, 1, 0, 0, 1, 2},
    {"TIME", mfn_time, 1, 0, 1, 0, 1},
    {"TIMING", mfn_timing, 0, 0, 0, 1, 1},
    {"TIMESTR", mfn_timestr, 1, 0, 1, 1, 1},
    {"TIMESUB", mfn_timesub, 1, 0, 1, 3, 4},
    {"TOLOWER", mfn_tolower, 1, 0, 0, 1, 1},
    {"TOUPPER", mfn_toupper, 1, 0, 0, 1, 1},
    {"TYPE", mfn_type, 1, 0, 1, 1, 1},
    {"TZOFFSET", mfn_tzoffset, 0, 0, 0, 0, 0},
    {"UNBLESS", mfn_unbless, 1, 0, 1, 1, 2},
    {"USECOUNT", mfn_usecount, 1, 0, 1, 1, 1},
    {"V", mfn_v, 1, 0, 1, 1, 1},	/* variable value */
    {"VERSION", mfn_version, 0, 0, 0, 0, 0},
    {"WHILE", mfn_while, 0, 0, 0, 2, 2},
    {"WITH", mfn_with, 0, 0, 0, 3, 9},	/* declares var & val */
    {"XOR", mfn_xor, 1, 0, 1, 2, 2},	/* logical XOR */

    {NULL, NULL, 0, 0, 0, 0, 0}	/* Ends the mfun list */
};

#endif				/* _MFUN_H */
