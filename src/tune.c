#include "config.h"

#include "array.h"
#include "commands.h"
#include "db.h"
#include "defaults.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "match.h"
#include "log.h"
#include "tune.h"
#include "tunelist.h"

#define TP_SEND_ENTRY_INFO(tune_entry) \
{ \
	notifyf(player, "%-27s %.4096s", tune_entry->group, tune_entry->label); \
}

/* NOTE:  Do NOT use these values as the name of a parameter.  Reserve them as a preprocessor define so
   it's a bit easier to change if needed.  If changed, don't forget to update the help files, too! */
#define TP_INFO_CMD "info"
#define TP_LOAD_CMD "load"
#define TP_SAVE_CMD "save"

int
tune_count_parms(void)
{
    return ARRAYSIZE(tune_str_list) + ARRAYSIZE(tune_time_list) + ARRAYSIZE(tune_val_list)
         + ARRAYSIZE(tune_ref_list) + ARRAYSIZE(tune_bool_list) - 5;
}

static void
tune_display_parms(dbref player, char *name, int mlev, int show_extended)
{
    char buf[BUFFER_LEN + 50];
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    /* Show a helpful message at the end if nothing was found */
    int parms_found = 0;

    if (name == NULL) {
	return;
    }

    /* Treat default parameters as normal */
    TP_CLEAR_FLAG_DEFAULT(name);

    while (tstr->name) {
	if (tstr->readmlev > mlev) {
	    tstr++;
	    continue;
	}
	strcpyn(buf, sizeof(buf), tstr->name);
	if (!*name || equalstr(name, buf)) {
	    notifyf(player, "(str)  %-20s = %.4096s%s%s",
		    tstr->name, *tstr->str, MOD_ENABLED(tstr->module) ? "" : " [inactive]",
		    (tstr->isdefault) ? " [default]" : "");
	    if (show_extended)
		TP_SEND_ENTRY_INFO(tstr);
	    if (!parms_found)
		parms_found = 1;
	}
	tstr++;
    }

    while (ttim->name) {
	if (ttim->readmlev > mlev) {
	    ttim++;
	    continue;
	}
	strcpyn(buf, sizeof(buf), ttim->name);
	if (!*name || equalstr(name, buf)) {
	    notifyf(player, "(time) %-20s = %s%s%s",
		    ttim->name, timestr_full(*ttim->tim),
		    MOD_ENABLED(ttim->module) ? "" : " [inactive]",
		    (ttim->isdefault) ? " [default]" : "");
	    if (show_extended)
		TP_SEND_ENTRY_INFO(ttim);
	    if (!parms_found)
		parms_found = 1;
	}
	ttim++;
    }

    while (tval->name) {
	if (tval->readmlev > mlev) {
	    tval++;
	    continue;
	}
	strcpyn(buf, sizeof(buf), tval->name);
	if (!*name || equalstr(name, buf)) {
	    notifyf(player, "(int)  %-20s = %d%s%s",
		    tval->name, *tval->val, MOD_ENABLED(tval->module) ? "" : " [inactive]",
		    (tval->isdefault) ? " [default]" : "");
	    if (show_extended)
		TP_SEND_ENTRY_INFO(tval);
	    if (!parms_found)
		parms_found = 1;
	}
	tval++;
    }

    while (tref->name) {
	if (tref->readmlev > mlev) {
	    tref++;
	    continue;
	}
	strcpyn(buf, sizeof(buf), tref->name);
	if (!*name || equalstr(name, buf)) {
            char unparse_buf[BUFFER_LEN];
            unparse_object(player, *tref->ref, unparse_buf, sizeof(unparse_buf));
	    notifyf(player, "(ref)  %-20s = %s%s%s",
		    tref->name, unparse_buf,
		    MOD_ENABLED(tref->module) ? "" : " [inactive]",
		    (tref->isdefault) ? " [default]" : "");
	    if (show_extended)
		TP_SEND_ENTRY_INFO(tref);
	    if (!parms_found)
		parms_found = 1;
	}
	tref++;
    }

    while (tbool->name) {
	if (tbool->readmlev > mlev) {
	    tbool++;
	    continue;
	}
	strcpyn(buf, sizeof(buf), tbool->name);
	if (!*name || equalstr(name, buf)) {
	    notifyf(player, "(bool) %-20s = %s%s%s",
		    tbool->name, ((*tbool->boolval) ? "yes" : "no"),
		    MOD_ENABLED(tbool->module) ? "" : " [inactive]",
		    (tbool->isdefault) ? " [default]" : "");
	    if (show_extended)
		TP_SEND_ENTRY_INFO(tbool);
	    if (!parms_found)
		parms_found = 1;
	}
	tbool++;
    }

    if (!parms_found)
	notify(player, "No matching parameters.");
    notify(player, "*done*");
}

void
tune_save_parms_to_file(FILE * f)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    /* Notes on parameter saving

       Comment out default values saved to the output.  This allows distinguishing between
       available parameters and customized parameters while still creating a list of new
       parameters in fresh or updated databases.

       If value is specified, save it as normal.  It won't be marked as default again until
       it's removed from the database (including commenting it out).

       Example:
       > Custom
       ssl_min_protocol_version=TLSv1.2
       > Default
       %ssl_min_protocol_version=None */

    while (tstr->name) {
	if (tstr->isdefault) {
	    fprintf(f, "%c%s=%.4096s\n", TP_FLAG_DEFAULT, tstr->name, *tstr->str);
	} else {
	    fprintf(f, "%s=%.4096s\n", tstr->name, *tstr->str);
	}
	tstr++;
    }

    while (ttim->name) {
	if (ttim->isdefault) {
	    fprintf(f, "%c%s=%s\n", TP_FLAG_DEFAULT, ttim->name, timestr_full(*ttim->tim));
	} else {
	    fprintf(f, "%s=%s\n", ttim->name, timestr_full(*ttim->tim));
	}
	ttim++;
    }

    while (tval->name) {
	if (tval->isdefault) {
	    fprintf(f, "%c%s=%d\n", TP_FLAG_DEFAULT, tval->name, *tval->val);
	} else {
	    fprintf(f, "%s=%d\n", tval->name, *tval->val);
	}
	tval++;
    }

    while (tref->name) {
	if (tref->isdefault) {
	    fprintf(f, "%c%s=#%d\n", TP_FLAG_DEFAULT, tref->name, *tref->ref);
	} else {
	    fprintf(f, "%s=#%d\n", tref->name, *tref->ref);
	}
	tref++;
    }

    while (tbool->name) {
	if (tbool->isdefault) {
	    fprintf(f, "%c%s=%s\n", TP_FLAG_DEFAULT, tbool->name,
		    (*tbool->boolval) ? "yes" : "no");
	} else {
	    fprintf(f, "%s=%s\n", tbool->name, (*tbool->boolval) ? "yes" : "no");
	}
	tbool++;
    }
}

stk_array *
tune_parms_array(const char *pattern, int mlev, int pinned)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;
    stk_array *nu = new_array_packed(0, pinned);
    struct inst temp1;
    char pat[BUFFER_LEN];
    char buf[BUFFER_LEN];
    int i = 0;

    strcpyn(pat, sizeof(pat), pattern);
    while (tbool->name) {
	if (tbool->readmlev <= mlev) {
	    strcpyn(buf, sizeof(buf), tbool->name);
	    if (!*pattern || equalstr(pat, buf)) {
		stk_array *item = new_array_dictionary(pinned);
		array_set_strkey_strval(&item, "type", "boolean");
		array_set_strkey_strval(&item, "group", tbool->group);
		array_set_strkey_strval(&item, "name", tbool->name);
		array_set_strkey_intval(&item, "value", *tbool->boolval ? 1 : 0);
		array_set_strkey_intval(&item, "mlev", tbool->readmlev);
		array_set_strkey_intval(&item, "readmlev", tbool->readmlev);
		array_set_strkey_intval(&item, "writemlev", tbool->writemlev);
		array_set_strkey_strval(&item, "label", tbool->label);
		array_set_strkey_intval(&item, "nullable", 0);
		array_set_strkey_intval(&item, "active", MOD_ENABLED(tbool->module));
		array_set_strkey_intval(&item, "default", tbool->isdefault);

		temp1.type = PROG_ARRAY;
		temp1.data.array = item;
		array_set_intkey(&nu, i++, &temp1);
		CLEAR(&temp1);
	    }
	}
	tbool++;
    }

    while (ttim->name) {
	if (ttim->readmlev <= mlev) {
	    strcpyn(buf, sizeof(buf), ttim->name);
	    if (!*pattern || equalstr(pat, buf)) {
		stk_array *item = new_array_dictionary(pinned);
		array_set_strkey_strval(&item, "type", "timespan");
		array_set_strkey_strval(&item, "group", ttim->group);
		array_set_strkey_strval(&item, "name", ttim->name);
		array_set_strkey_intval(&item, "value", *ttim->tim);
		array_set_strkey_intval(&item, "mlev", ttim->readmlev);
		array_set_strkey_intval(&item, "readmlev", ttim->readmlev);
		array_set_strkey_intval(&item, "writemlev", ttim->writemlev);
		array_set_strkey_strval(&item, "label", ttim->label);
		array_set_strkey_intval(&item, "nullable", 0);
		array_set_strkey_intval(&item, "active", MOD_ENABLED(ttim->module));
		array_set_strkey_intval(&item, "default", ttim->isdefault);

		temp1.type = PROG_ARRAY;
		temp1.data.array = item;
		array_set_intkey(&nu, i++, &temp1);
		CLEAR(&temp1);
	    }
	}
	ttim++;
    }

    while (tval->name) {
	if (tval->readmlev <= mlev) {
	    strcpyn(buf, sizeof(buf), tval->name);
	    if (!*pattern || equalstr(pat, buf)) {
		stk_array *item = new_array_dictionary(pinned);
		array_set_strkey_strval(&item, "type", "integer");
		array_set_strkey_strval(&item, "group", tval->group);
		array_set_strkey_strval(&item, "name", tval->name);
		array_set_strkey_intval(&item, "value", *tval->val);
		array_set_strkey_intval(&item, "mlev", tval->readmlev);
		array_set_strkey_intval(&item, "readmlev", tval->readmlev);
		array_set_strkey_intval(&item, "writemlev", tval->writemlev);
		array_set_strkey_strval(&item, "label", tval->label);
		array_set_strkey_intval(&item, "nullable", 0);
		array_set_strkey_intval(&item, "active", MOD_ENABLED(tval->module));
		array_set_strkey_intval(&item, "default", tval->isdefault);

		temp1.type = PROG_ARRAY;
		temp1.data.array = item;
		array_set_intkey(&nu, i++, &temp1);
		CLEAR(&temp1);
	    }
	}
	tval++;
    }

    while (tref->name) {
	if (tref->readmlev <= mlev) {
	    strcpyn(buf, sizeof(buf), tref->name);
	    if (!*pattern || equalstr(pat, buf)) {
		stk_array *item = new_array_dictionary(pinned);
		array_set_strkey_strval(&item, "type", "dbref");
		array_set_strkey_strval(&item, "group", tref->group);
		array_set_strkey_strval(&item, "name", tref->name);
		array_set_strkey_refval(&item, "value", *tref->ref);
		array_set_strkey_intval(&item, "mlev", tref->readmlev);
		array_set_strkey_intval(&item, "readmlev", tref->readmlev);
		array_set_strkey_intval(&item, "writemlev", tref->writemlev);
		array_set_strkey_strval(&item, "label", tref->label);
		array_set_strkey_intval(&item, "nullable", 0);
		array_set_strkey_intval(&item, "active", MOD_ENABLED(tref->module));
		array_set_strkey_intval(&item, "default", tref->isdefault);

		switch (tref->typ) {
		case NOTYPE:
		    array_set_strkey_strval(&item, "objtype", "any");
		    break;
		case TYPE_PLAYER:
		    array_set_strkey_strval(&item, "objtype", "player");
		    break;
		case TYPE_THING:
		    array_set_strkey_strval(&item, "objtype", "thing");
		    break;
		case TYPE_ROOM:
		    array_set_strkey_strval(&item, "objtype", "room");
		    break;
		case TYPE_EXIT:
		    array_set_strkey_strval(&item, "objtype", "exit");
		    break;
		case TYPE_PROGRAM:
		    array_set_strkey_strval(&item, "objtype", "program");
		    break;
		case TYPE_GARBAGE:
		    array_set_strkey_strval(&item, "objtype", "garbage");
		    break;
		default:
		    array_set_strkey_strval(&item, "objtype", "unknown");
		    break;
		}

		temp1.type = PROG_ARRAY;
		temp1.data.array = item;
		array_set_intkey(&nu, i++, &temp1);
		CLEAR(&temp1);
	    }
	}
	tref++;
    }

    while (tstr->name) {
	if (tstr->readmlev <= mlev) {
	    strcpyn(buf, sizeof(buf), tstr->name);
	    if (!*pattern || equalstr(pat, buf)) {
		stk_array *item = new_array_dictionary(pinned);
		array_set_strkey_strval(&item, "type", "string");
		array_set_strkey_strval(&item, "group", tstr->group);
		array_set_strkey_strval(&item, "name", tstr->name);
		array_set_strkey_strval(&item, "value", *tstr->str);
		array_set_strkey_intval(&item, "mlev", tstr->readmlev);
		array_set_strkey_intval(&item, "readmlev", tstr->readmlev);
		array_set_strkey_intval(&item, "writemlev", tstr->writemlev);
		array_set_strkey_strval(&item, "label", tstr->label);
		array_set_strkey_intval(&item, "nullable", tstr->isnullable);
		array_set_strkey_intval(&item, "active", MOD_ENABLED(tstr->module));
		array_set_strkey_intval(&item, "default", tstr->isdefault);

		temp1.type = PROG_ARRAY;
		temp1.data.array = item;
		array_set_intkey(&nu, i++, &temp1);
		CLEAR(&temp1);
	    }
	}
	tstr++;
    }

    return nu;
}

static int
tune_save_parmsfile(void)
{
    FILE *f;

    f = fopen(tp_file_sysparms, "wb");
    if (!f) {
	log_status("Couldn't open file %s!", tp_file_sysparms);
	return 0;
    }

    tune_save_parms_to_file(f);

    fclose(f);
    return 1;
}

const char *
tune_get_parmstring(const char *name, int mlev)
{
    static char buf[BUFFER_LEN + 50];
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    /* Treat default parameters as normal */
    TP_CLEAR_FLAG_DEFAULT(name);

    if (!name) {
	strcpyn(buf, sizeof(buf), "");
	return (buf);
    }

    while (tstr->name) {
	if (!strcasecmp(name, tstr->name)) {
	    if (tstr->readmlev > mlev)
		return "";
	    return (*tstr->str);
	}
	tstr++;
    }

    while (ttim->name) {
	if (!strcasecmp(name, ttim->name)) {
	    if (ttim->readmlev > mlev)
		return "";
	    snprintf(buf, sizeof(buf), "%d", *ttim->tim);
	    return (buf);
	}
	ttim++;
    }

    while (tval->name) {
	if (!strcasecmp(name, tval->name)) {
	    if (tval->readmlev > mlev)
		return "";
	    snprintf(buf, sizeof(buf), "%d", *tval->val);
	    return (buf);
	}
	tval++;
    }

    while (tref->name) {
	if (!strcasecmp(name, tref->name)) {
	    if (tref->readmlev > mlev)
		return "";
	    snprintf(buf, sizeof(buf), "#%d", *tref->ref);
	    return (buf);
	}
	tref++;
    }

    while (tbool->name) {
	if (!strcasecmp(name, tbool->name)) {
	    if (tbool->readmlev > mlev)
		return "";
	    snprintf(buf, sizeof(buf), "%s", ((*tbool->boolval) ? "yes" : "no"));
	    return (buf);
	}
	tbool++;
    }

    /* Parameter was not found.  Return null string. */
    strcpyn(buf, sizeof(buf), "");
    return (buf);
}

#ifdef MEMORY_CLEANUP
void
tune_freeparms()
{
    struct tune_str_entry *tstr = tune_str_list;
    while (tstr->name) {
	if (!tstr->isdefault) {
	    free((char *) *tstr->str);
	}
	tstr++;
    }
}
#endif

int
tune_setparm(dbref player, const char *parmname, const char *val, int mlev)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    char buf[BUFFER_LEN];
    char *parmval;
    strcpyn(buf, sizeof(buf), val);
    parmval = buf;

    int reset_default = 0;
    if (TP_HAS_FLAG_DEFAULT(parmname)) {
	/* Parameter name starts with the 'reset to default' flag.  Remove the flag and mark
	   the parameter for resetting to default. */
	TP_CLEAR_FLAG_DEFAULT(parmname);
	reset_default = 1;
    }

    if (!parmname) return TUNESET_UNKNOWN;

    while (tstr->name) {
	if (!strcasecmp(parmname, tstr->name)) {
	    if (tstr->writemlev > mlev)
		return TUNESET_DENIED;

	    if (reset_default) {
		/* Reset to default value */
		if (!tstr->isdefault)
		    free((char *) *tstr->str);
		tstr->isdefault = 1;
		*tstr->str = tstr->defaultstr;
		return TUNESET_SUCCESS_DEFAULT;
	    } else {
		/* Specify a new value */
		if (!tstr->isnullable && !*parmval)
		    return TUNESET_BADVAL;
		if (!tstr->isdefault)
		    free((char *) *tstr->str);
		tstr->isdefault = 0;
		*tstr->str = strdup(parmval);
		return TUNESET_SUCCESS;
	    }
	}
	tstr++;
    }

    while (ttim->name) {
	if (!strcasecmp(parmname, ttim->name)) {
	    if (ttim->writemlev > mlev)
		return TUNESET_DENIED;

	    if (reset_default) {
		/* Reset to default value */
		ttim->isdefault = 1;
		*ttim->tim = ttim->defaulttim;
		return TUNESET_SUCCESS_DEFAULT;
	    } else {
		/* Specify a new value */
		int days, hrs, mins, secs, result;
		char *end;
		days = hrs = mins = secs = 0;
		end = parmval + strlen(parmval) - 1;
		switch (*end) {
		case 's':
		case 'S':
		    *end = '\0';
		    if (!number(parmval))
			return TUNESET_SYNTAX;
		    secs = atoi(parmval);
		    break;
		case 'm':
		case 'M':
		    *end = '\0';
		    if (!number(parmval))
			return TUNESET_SYNTAX;
		    mins = atoi(parmval);
		    break;
		case 'h':
		case 'H':
		    *end = '\0';
		    if (!number(parmval))
			return TUNESET_SYNTAX;
		    hrs = atoi(parmval);
		    break;
		case 'd':
		case 'D':
		    *end = '\0';
		    if (!number(parmval))
			return TUNESET_SYNTAX;
		    days = atoi(parmval);
		    break;
		default:
		    result = sscanf(parmval, "%dd %2d:%2d:%2d", &days, &hrs, &mins, &secs);
		    if (result != 4)
			return TUNESET_SYNTAX;
		    break;
		}
		/* New value success, clear default flag */
		ttim->isdefault = 0;
		*ttim->tim = (days * 86400) + (3600 * hrs) + (60 * mins) + secs;
		return TUNESET_SUCCESS;
	    }
	}
	ttim++;
    }

    while (tval->name) {
	if (!strcasecmp(parmname, tval->name)) {
	    if (tval->writemlev > mlev)
		return TUNESET_DENIED;

	    if (reset_default) {
		/* Reset to default value */
		tval->isdefault = 1;
		*tval->val = tval->defaultval;
		return TUNESET_SUCCESS_DEFAULT;
	    } else {
		/* Specify a new value */
		if (!number(parmval))
		    return TUNESET_SYNTAX;

		/* New value success, clear default flag */
		tval->isdefault = 0;
		*tval->val = atoi(parmval);
		return TUNESET_SUCCESS;
	    }
	}
	tval++;
    }

    while (tref->name) {
	if (!strcasecmp(parmname, tref->name)) {
	    if (tref->writemlev > mlev)
		return TUNESET_DENIED;

	    if (reset_default) {
		/* Reset to default value */
		tref->isdefault = 1;
		*tref->ref = tref->defaultref;
		return TUNESET_SUCCESS_DEFAULT;
	    } else {
		/* Specify a new value */
		dbref obj;
		struct match_data md;
		init_match(NOTHING, player, parmval, NOTYPE, &md);
		match_absolute(&md);
		match_registered(&md);
		if ((obj = match_result(&md)) == NOTHING)
		    return TUNESET_SYNTAX;
		if (!ObjExists(obj))
		    return TUNESET_SYNTAX;
		if (tref->typ != NOTYPE && Typeof(obj) != tref->typ)
		    return TUNESET_BADVAL;

		/* New value success, clear default flag */
		tref->isdefault = 0;
		*tref->ref = obj;
		return TUNESET_SUCCESS;
	    }
	}
	tref++;
    }

    while (tbool->name) {
	if (!strcasecmp(parmname, tbool->name)) {
	    if (tbool->writemlev > mlev)
		return TUNESET_DENIED;

	    if (reset_default) {
		/* Reset to default value */
		tbool->isdefault = 1;
		*tbool->boolval = tbool->defaultbool;
		return TUNESET_SUCCESS_DEFAULT;
	    } else {
		/* Specify a new value */
		if (*parmval == 'y' || *parmval == 'Y') {
		    tbool->isdefault = 0;
		    *tbool->boolval = 1;
		} else if (*parmval == 'n' || *parmval == 'N') {
		    tbool->isdefault = 0;
		    *tbool->boolval = 0;
		} else {
		    return TUNESET_SYNTAX;
		}
		return TUNESET_SUCCESS;
	    }
	}
	tbool++;
    }

    return TUNESET_UNKNOWN;
}

void
tune_load_parms_defaults()
{
    for (struct tune_str_entry *tstr = tune_str_list; tstr->name; tstr++) {
	if (!tstr->isdefault) {
	    free((char *)*tstr->str);
	}
	tstr->isdefault = 1;
	*tstr->str = tstr->defaultstr;
    }
    for (struct tune_time_entry *ttim = tune_time_list; ttim->name; ttim++) {
	ttim->isdefault = 1;
	*ttim->tim = ttim->defaulttim;
    }
    for (struct tune_val_entry *tval = tune_val_list; tval->name; tval++) {
	tval->isdefault = 1;
	*tval->val = tval->defaultval;
    }
    for (struct tune_ref_entry *tref = tune_ref_list; tref->name; tref++) {
	tref->isdefault = 1;
	*tref->ref = tref->defaultref;
    }
    for (struct tune_bool_entry *tbool = tune_bool_list; tbool->name; tbool++) {
	tbool->isdefault = 1;
	*tbool->boolval = tbool->defaultbool;
    }
}

void
tune_load_parms_from_file(FILE * f, dbref player, int cnt)
{
    char buf[BUFFER_LEN];
    char *c, *p;
    int result = 0;

    while (!feof(f) && (cnt < 0 || cnt--)) {
	fgets(buf, sizeof(buf), f);
	if (*buf != '#') {
	    c = strchr(buf, ARG_DELIMITER);
	    if (c) {
		*c++ = '\0';
		p = buf;
		remove_ending_whitespace(&p);
		skip_whitespace((const char **)&c);
		for (p = c; *p && *p != '\n' && *p != '\r'; p++) ;
		*p = '\0';
		p = buf;
		skip_whitespace((const char **)&p);
		if (*p) {
		    result = tune_setparm((dbref)1, p, c, MLEV_GOD);
		}
		switch (result) {
		case TUNESET_SUCCESS:
		    strcatn(buf, sizeof(buf), ": Parameter set.");
		    break;
		case TUNESET_SUCCESS_DEFAULT:
		    strcatn(buf, sizeof(buf), ": Parameter reset to default.");
		    break;
		case TUNESET_UNKNOWN:
		    strcatn(buf, sizeof(buf), ": Unknown parameter.");
		    break;
		case TUNESET_SYNTAX:
		    strcatn(buf, sizeof(buf), ": Bad parameter syntax.");
		    break;
		case TUNESET_BADVAL:
		    strcatn(buf, sizeof(buf), ": Bad parameter value.");
		    break;
		case TUNESET_DENIED:
		    strcatn(buf, sizeof(buf), ": Permission denied.");
		    break;
		}
		if (result && player != NOTHING)
		    notify(player, p);
	    }
	}
    }
}

static int
tune_load_parmsfile(dbref player)
{
    FILE *f;

    f = fopen(tp_file_sysparms, "rb");
    if (!f) {
	log_status("Couldn't open file %s!", tp_file_sysparms);
	return 0;
    }

    tune_load_parms_from_file(f, player, -1);

    fclose(f);
    return 1;
}

void
do_tune(dbref player, char *parmname, char *parmval)
{
    char *oldvalue;
    int result;
    int security = TUNE_MLEV(player);

    /* If parmname exists, and either has parmvalue or the reset to default flag, try to set the
       value.  Otherwise, fall back to displaying it. */
    if (*parmname && (strchr(match_args, ARG_DELIMITER) || TP_HAS_FLAG_DEFAULT(parmname))) {
	if (force_level) {
	    notify(player, "You cannot force setting a @tune.");
	    return;
	}

	/* Duplicate the string, otherwise the oldvalue pointer will be overridden to the new value
	   when tune_setparm() is called. */
	oldvalue = strdup(tune_get_parmstring(parmname, security));
	result = tune_setparm(player, parmname, parmval, security);
	switch (result) {
	case TUNESET_SUCCESS:
	case TUNESET_SUCCESS_DEFAULT:
	    if (result == TUNESET_SUCCESS_DEFAULT) {
		TP_CLEAR_FLAG_DEFAULT(parmname);
		log_status("TUNED: %s(%d) tuned %s from '%s' to default",
			   NAME(player), player, parmname, oldvalue);
		notify(player, "Parameter reset to default.");
	    } else {
		log_status("TUNED: %s(%d) tuned %s from '%s' to '%s'",
			   NAME(player), player, parmname, oldvalue, DoNull(parmval));
		notify(player, "Parameter set.");
	    }
	    tune_display_parms(player, parmname, security, 0);
	    break;
	case TUNESET_UNKNOWN:
	    notify(player, "Unknown parameter.");
	    break;
	case TUNESET_SYNTAX:
	    notify(player, "Bad parameter syntax.");
	    break;
	case TUNESET_BADVAL:
	    notify(player, "Bad parameter value.");
	    break;
	case TUNESET_DENIED:
	    notify(player, "Permission denied.");
	    break;
	default:
	    break;
	}
	/* Don't let the oldvalue hang around */
	if (oldvalue)
	    free(oldvalue);
    } else if (*parmname && string_prefix(parmname, TP_INFO_CMD)) {
	/* Space-separated parameters are all in parmname.  Trim out the 'info' command and
	   any extra spaces */
	const char *p_trim = strchr(parmname, ' ');
	if (p_trim != NULL) {
	    skip_whitespace(&p_trim);
	    if (*p_trim) {
		tune_display_parms(player, (char *)p_trim, security, 1);
	    } else {
		notify(player, "Usage is @tune " TP_INFO_CMD " [optional: <parameter>]");
	    }
	} else {
	    /* Show expanded information on all parameters */
	    tune_display_parms(player, "", security, 1);
	}
    } else if (*parmname && !strcasecmp(parmname, TP_SAVE_CMD)) {
	if (tune_save_parmsfile()) {
	    notify(player, "Saved parameters to configuration file.");
	} else {
	    notify(player, "Unable to save to configuration file.");
	}
    } else if (*parmname && !strcasecmp(parmname, TP_LOAD_CMD)) {
	if (tune_load_parmsfile(player)) {
	    notify(player, "Restored parameters from configuration file.");
	} else {
	    notify(player, "Unable to load from configuration file.");
	}
    } else {
	tune_display_parms(player, parmname, security, 0);
    }
}

void
set_flags_from_tunestr(dbref obj, const char *tunestr)
{
    const char *p = tunestr;
    object_flag_type f = 0;

    for (;;) {
        char pcc = toupper(*p);
        if (pcc == '\0' || pcc == '\n' || pcc == '\r') {
            break;
        } else if (pcc == '0') {
            SetMLevel(obj, 0);
        } else if (pcc == '1') {
            SetMLevel(obj, 1);
        } else if (pcc == '2') {
            SetMLevel(obj, 2);
        } else if (pcc == '3') {
            SetMLevel(obj, 3);
        } else if (pcc == 'A') {
            f = ABODE;
        } else if (pcc == 'B') {
            f = BUILDER;
        } else if (pcc == 'C') {
            f = CHOWN_OK;
        } else if (pcc == 'D') {
            f = DARK;
        } else if (pcc == 'G') {
            f = GUEST;
        } else if (pcc == 'H') {
            f = HAVEN;
        } else if (pcc == 'J') {
            f = JUMP_OK;
        } else if (pcc == 'K') {
            f = KILL_OK;
        } else if (pcc == 'L') {
            f = LINK_OK;
        } else if (pcc == 'M') {
            SetMLevel(obj, 2);
        } else if (pcc == 'Q') {
            f = QUELL;
        } else if (pcc == 'S') {
            f = STICKY;
        } else if (pcc == 'V') {
            f = VEHICLE;
        } else if (pcc == 'W') {
            /* f = WIZARD;     This is very bad to auto-set. */
        } else if (pcc == 'X') {
            f = XFORCIBLE;
        } else if (tp_enable_match_yield && pcc == 'Y') {
            f = YIELD;
        } else if (tp_enable_match_yield && pcc == 'O') {
            f = OVERT;
        } else if (pcc == 'Z') {
            f = ZOMBIE;
        }
        FLAGS(obj) |= f;
        p++;
    }
    ts_modifyobject(obj);
    DBDIRTY(obj);
}
