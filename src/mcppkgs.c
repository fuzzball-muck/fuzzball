#include "config.h"

#ifdef MCP_SUPPORT

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"
#include "db.h"
#include "edit.h"
#include "fbstrings.h"
#include "inst.h"
#include "interface.h"
#include "log.h"
#include "mcp.h"
#include "mcppkg.h"
#include "props.h"
#include "tune.h"

void
show_mcp_error(McpFrame * mfr, char *topic, char *text)
{
    McpMesg msg;
    McpVer supp = mcp_frame_package_supported(mfr, "org-fuzzball-notify");

    if (supp.verminor != 0 || supp.vermajor != 0) {
	mcp_mesg_init(&msg, "org-fuzzball-notify", "error");
	mcp_mesg_arg_append(&msg, "text", text);
	mcp_mesg_arg_append(&msg, "topic", topic);
	mcp_frame_output_mesg(mfr, &msg);
	mcp_mesg_clear(&msg);
    } else {
	notify(MCPFRAME_PLAYER(mfr), text);
    }
}

/*
 * reference is in the format objnum.category.misc where objnum is the
 * object reference, and category can be one of the following:
 *    prop     to set a property named by misc.
 *    proplist to store a string proplist named by misc.
 *    prog     to set the program text of the given object.  Ignores misc.
 *    sysparm  to set an @tune value.  Ignores objnum.
 *    user     to return data to a muf program.
 *
 * If the category is prop, then it accepts the following types:
 *    string        to set the property to a string value.
 *    string-list   to set the property to a multi-line string value.
 *    integer       to set the property to an integer value.
 *
 * Any other values are ignored.
 */
void
mcppkg_simpleedit(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context)
{
    if (!strcasecmp(msg->mesgname, "set")) {
	dbref obj = NOTHING;
	char *reference;
	char *valtype;
	char category[BUFFER_LEN];
	char *ptr;
	int lines;
	dbref player;
	char buf[BUFFER_LEN];
	char *content;
	int descr;

	reference = mcp_mesg_arg_getline(msg, "reference", 0);
	valtype = mcp_mesg_arg_getline(msg, "type", 0);
	lines = mcp_mesg_arg_linecount(msg, "content");
	player = MCPFRAME_PLAYER(mfr);
	descr = MCPFRAME_DESCR(mfr);

	/* extract object number.  -1 for none.  */
	if (isdigit(*reference)) {
	    obj = 0;
	    while (isdigit(*reference)) {
		obj = (10 * obj) + (*reference++ - '0');
		if (obj >= 100000000) {
		    show_mcp_error(mfr, "simpleedit-set", "Bad reference object.");
		    return;
		}
	    }
	}
	if (*reference != '.') {
	    show_mcp_error(mfr, "simpleedit-set", "Bad reference value.");
	    return;
	}
	reference++;

	/* extract category string */
	ptr = category;
	while (*reference && *reference != '.') {
	    *ptr++ = *reference++;
	}
	*ptr = '\0';
	if (*reference != '.') {
	    show_mcp_error(mfr, "simpleedit-set", "Bad reference value.");
	    return;
	}
	reference++;

	/* the rest is category specific data. */
	if (!strcasecmp(category, "prop")) {
	    if (!OkObj(obj)) {
		show_mcp_error(mfr, "simpleedit-set", "Bad reference object.");
		return;
	    }
	    if (!controls(player, obj)) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    for (ptr = reference; *ptr; ptr++) {
		if (*ptr == PROP_DELIMITER) {
		    show_mcp_error(mfr, "simpleedit-set", "Bad property name.");
		    return;
		}
	    }
	    if (Prop_System(reference)
		|| (!Wizard(player) && (Prop_SeeOnly(reference) || Prop_Hidden(reference)))) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    if (!strcasecmp(valtype, "string-list") || !strcasecmp(valtype, "string")) {
		int left = BUFFER_LEN - 1;
		int len;

		buf[0] = '\0';
		for (int line = 0; line < lines; line++) {
		    content = mcp_mesg_arg_getline(msg, "content", line);
		    if (line > 0) {
			if (left >= 1) {
			    strcatn(buf, sizeof(buf), "\r");
			    left--;
			} else {
			    break;
			}
		    }
		    len = strlen(content);
		    if (len >= left - 1) {
			strncat(buf, content, left);
			break;
		    } else {
			strcatn(buf, sizeof(buf), content);
			left -= len;
		    }
		}
		buf[BUFFER_LEN - 1] = '\0';
		add_property(obj, reference, buf, 0);

	    } else if (!strcasecmp(valtype, "integer")) {
		if (lines != 1) {
		    show_mcp_error(mfr, "simpleedit-set", "Bad integer value.");
		    return;
		}
		content = mcp_mesg_arg_getline(msg, "content", 0);
		add_property(obj, reference, NULL, atoi(content));
	    }

	} else if (!strcasecmp(category, "proplist")) {
	    if (!OkObj(obj)) {
		show_mcp_error(mfr, "simpleedit-set", "Bad reference object.");
		return;
	    }
	    if (!controls(player, obj)) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    for (ptr = reference; *ptr; ptr++) {
		if (*ptr == PROP_DELIMITER) {
		    show_mcp_error(mfr, "simpleedit-set", "Bad property name.");
		    return;
		}
	    }
	    if (Prop_System(reference)
		|| (!Wizard(player) && (Prop_SeeOnly(reference) || Prop_Hidden(reference)))) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    if (!strcasecmp(valtype, "string-list")) {

		if (lines == 0) {
		    snprintf(buf, sizeof(buf), "%s#", reference);
		    remove_property(obj, buf, 0);
		} else {
		    snprintf(buf, sizeof(buf), "%s#", reference);
		    remove_property(obj, buf, 0);
		    add_property(obj, buf, "", lines);
		    for (int line = 0; line < lines; line++) {
			content = mcp_mesg_arg_getline(msg, "content", line);
			if (!content || !*content) {
			    content = " ";
			}
			snprintf(buf, sizeof(buf), "%s#/%d", reference, line + 1);
			add_property(obj, buf, content, 0);
		    }
		}
	    } else if (!strcasecmp(valtype, "string") ||
		       !strcasecmp(valtype, "integer")) {
		show_mcp_error(mfr, "simpleedit-set", "Bad value type for proplist.");
		return;
	    }

	} else if (!strcasecmp(category, "prog")) {
	    struct line *tmpline;
	    struct line *curr = NULL;
	    struct line *new_line;
            char unparse_buf[BUFFER_LEN];

	    if (!OkObj(obj)) {
		show_mcp_error(mfr, "simpleedit-set", "Bad reference object.");
		return;
	    }
	    if (Typeof(obj) != TYPE_PROGRAM || !controls(player, obj)) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    if (!Mucker(player)) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    if (FLAGS(obj) & INTERNAL) {
		show_mcp_error(mfr, "simpleedit-set",
			       "Sorry, this program is currently being edited.  Try again later.");
		return;
	    }
	    tmpline = PROGRAM_FIRST(obj);
	    PROGRAM_SET_FIRST(obj, NULL);

	    for (int line = 0; line < lines; line++) {
		new_line = get_new_line();
		content = mcp_mesg_arg_getline(msg, "content", line);
		if (!content || !*content) {
		    new_line->this_line = alloc_string(" ");
		} else {
		    new_line->this_line = alloc_string(content);
		}
		new_line->next = NULL;
		if (line == 0) {
		    PROGRAM_SET_FIRST(obj, new_line);
		} else {
		    curr->next = new_line;
		}
		curr = new_line;
	    }

            unparse_object(player, obj, unparse_buf, sizeof(unparse_buf));
	    log_status("PROGRAM SAVED: %s by %s(%d)",
		       unparse_buf, NAME(player), player);

	    write_program(PROGRAM_FIRST(obj), obj);

	    if (tp_log_programs)
		log_program_text(PROGRAM_FIRST(obj), player, obj);

	    do_compile(descr, player, obj, 1);

	    free_prog_text(PROGRAM_FIRST(obj));

	    PROGRAM_SET_FIRST(obj, tmpline);

	    DBDIRTY(player);
	    DBDIRTY(obj);

	} else if (!strcasecmp(category, "sysparm")) {
	    if (!Wizard(player)) {
		show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
		return;
	    }
	    if (lines != 1) {
		show_mcp_error(mfr, "simpleedit-set", "Bad @tune value.");
		return;
	    }
	    content = mcp_mesg_arg_getline(msg, "content", 0);
	    tune_setparm(player, reference, content, TUNE_MLEV(player));

	} else if (!strcasecmp(category, "user")) {
	} else {
	    show_mcp_error(mfr, "simpleedit-set", "Unknown reference category.");
	    return;
	}
    }
}

void
mcppkg_languages(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context)
{
    McpMesg omsg;
    McpVer supp = mcp_frame_package_supported(mfr, "org-fuzzball-languages");

    if (supp.verminor == 0 && supp.vermajor == 0) {
	notify(MCPFRAME_PLAYER(mfr), "MCP: org-fuzzball-languages not supported.");
	return;
    }

    if (!strcasecmp(msg->mesgname, "request")) {
	mcp_mesg_init(&omsg, "org-fuzzball-languages", "supported");
	mcp_mesg_arg_append(&omsg, "languages", "muf:7.0");
	/* mcp_mesg_arg_append(&omsg, "languages", "muv:2.0"); */
	mcp_frame_output_mesg(mfr, &omsg);
	mcp_mesg_clear(&omsg);
	return;
    }
}


void
mcppkg_help_request(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context)
{
    FILE *f;
    const char *file;
    char buf[BUFFER_LEN];
    char topic[BUFFER_LEN];
    char *p;
    size_t arglen;
    int found;
    McpVer supp = mcp_frame_package_supported(mfr, "org-fuzzball-help");
    McpMesg omsg;

    if (supp.verminor == 0 && supp.vermajor == 0) {
	notify(MCPFRAME_PLAYER(mfr), "MCP: org-fuzzball-help not supported.");
	return;
    }

    if (!strcasecmp(msg->mesgname, "request")) {
	char *onwhat;
	char *valtype;

	onwhat = mcp_mesg_arg_getline(msg, "topic", 0);
	valtype = mcp_mesg_arg_getline(msg, "type", 0);

	*topic = '\0';
	strcpyn(topic, sizeof(topic), onwhat);
	if (*onwhat) {
	    strcatn(topic, sizeof(topic), "|");
	}

	if (!strcasecmp(valtype, "man")) {
	    file = tp_file_man;
	} else if (!strcasecmp(valtype, "mpi")) {
	    file = tp_file_mpihelp;
	} else if (!strcasecmp(valtype, "help")) {
	    file = tp_file_help;
	} else if (!strcasecmp(valtype, "news")) {
	    file = tp_file_news;
	} else {
	    snprintf(buf, sizeof(buf), "Sorry, %s is not a valid help type.", valtype);
	    mcp_mesg_init(&omsg, "org-fuzzball-help", "error");
	    mcp_mesg_arg_append(&omsg, "text", buf);
	    mcp_mesg_arg_append(&omsg, "topic", onwhat);
	    mcp_frame_output_mesg(mfr, &omsg);
	    mcp_mesg_clear(&omsg);
	    return;
	}

	if ((f = fopen(file, "rb")) == NULL) {
	    snprintf(buf, sizeof(buf), "Sorry, %s is missing.  Management has been notified.",
		     file);
	    fprintf(stderr, "help: No file %s!\n", file);
	    mcp_mesg_init(&omsg, "org-fuzzball-help", "error");
	    mcp_mesg_arg_append(&omsg, "text", buf);
	    mcp_mesg_arg_append(&omsg, "topic", onwhat);
	    mcp_frame_output_mesg(mfr, &omsg);
	    mcp_mesg_clear(&omsg);
	} else {
	    if (*topic) {
		arglen = strlen(topic);
		do {
		    do {
			if (!(fgets(buf, sizeof buf, f))) {
			    snprintf(buf, sizeof(buf),
				     "Sorry, no help available on topic \"%s\"", onwhat);
			    fclose(f);
			    mcp_mesg_init(&omsg, "org-fuzzball-help", "error");
			    mcp_mesg_arg_append(&omsg, "text", buf);
			    mcp_mesg_arg_append(&omsg, "topic", onwhat);
			    mcp_frame_output_mesg(mfr, &omsg);
			    mcp_mesg_clear(&omsg);
			    return;
			}
		    } while (*buf != '~');
		    do {
			if (!(fgets(buf, sizeof buf, f))) {
			    snprintf(buf, sizeof(buf),
				     "Sorry, no help available on topic \"%s\"", onwhat);
			    fclose(f);
			    mcp_mesg_init(&omsg, "org-fuzzball-help", "error");
			    mcp_mesg_arg_append(&omsg, "text", buf);
			    mcp_mesg_arg_append(&omsg, "topic", onwhat);
			    mcp_frame_output_mesg(mfr, &omsg);
			    mcp_mesg_clear(&omsg);
			    return;
			}
		    } while (*buf == '~');
		    p = buf;
		    found = 0;
		    buf[strlen(buf) - 1] = '|';
		    while (*p && !found) {
			if (strncasecmp(p, topic, arglen)) {
			    while (*p && (*p != '|'))
				p++;
			    if (*p)
				p++;
			} else {
			    found = 1;
			}
		    }
		} while (!found);
	    }
	    mcp_mesg_init(&omsg, "org-fuzzball-help", "entry");
	    mcp_mesg_arg_append(&omsg, "topic", onwhat);
	    while (fgets(buf, sizeof buf, f)) {
		if (*buf == '~')
		    break;
		for (p = buf; *p; p++) {
		    if (*p == '\n' || *p == '\r') {
			*p = '\0';
			break;
		    }
		}
		if (!*buf) {
		    strcpyn(buf, sizeof(buf), "  ");
		}
		mcp_mesg_arg_append(&omsg, "text", buf);
	    }
	    fclose(f);
	    mcp_frame_output_mesg(mfr, &omsg);
	    mcp_mesg_clear(&omsg);
	}
    }
}
#endif
