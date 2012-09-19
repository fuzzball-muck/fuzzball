
#include "config.h"
#include <math.h>
#include <ctype.h>
#include "params.h"

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "mcp.h"
#include "externs.h"
#include "props.h"
#include "match.h"
#include "interp.h"
#include "interface.h"
#include "msgparse.h"


struct line *get_new_line(void);

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
		notify(mcpframe_to_user(mfr), text);
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
	if (!string_compare(msg->mesgname, "set")) {
		dbref obj = NOTHING;
		char *reference;
		char *valtype;
		char category[BUFFER_LEN];
		char *ptr;
		int lines;
		dbref player;
		char buf[BUFFER_LEN];
		char *content;
		int line;
		int descr;

		reference = mcp_mesg_arg_getline(msg, "reference", 0);
		valtype = mcp_mesg_arg_getline(msg, "type", 0);
		lines = mcp_mesg_arg_linecount(msg, "content");
		player = mcpframe_to_user(mfr);
		descr = mcpframe_to_descr(mfr);

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
		if (!string_compare(category, "prop")) {
			if (obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE) {
				show_mcp_error(mfr, "simpleedit-set", "Bad reference object.");
				return;
			}
			if (!controls(player, obj)) {
				show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
				return;
			}
			for (ptr = reference; *ptr; ptr++) {
				if (*ptr == ':') {
					show_mcp_error(mfr, "simpleedit-set", "Bad property name.");
					return;
				}
			}
			if (Prop_System(reference) || (!Wizard(player) && (Prop_SeeOnly(reference) || Prop_Hidden(reference)))) {
				show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
				return;
			}
			if (!string_compare(valtype, "string-list") || !string_compare(valtype, "string")) {
				int left = BUFFER_LEN - 1;
				int len;

				buf[0] = '\0';
				for (line = 0; line < lines; line++) {
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
						left = 0;
						break;
					} else {
						strcatn(buf, sizeof(buf), content);
						left -= len;
					}
				}
				buf[BUFFER_LEN - 1] = '\0';
				add_property(obj, reference, buf, 0);

			} else if (!string_compare(valtype, "integer")) {
				if (lines != 1) {
					show_mcp_error(mfr, "simpleedit-set", "Bad integer value.");
					return;
				}
				content = mcp_mesg_arg_getline(msg, "content", 0);
				add_property(obj, reference, NULL, atoi(content));
			}

		} else if (!string_compare(category, "proplist")) {
			if (obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE) {
				show_mcp_error(mfr, "simpleedit-set", "Bad reference object.");
				return;
			}
			if (!controls(player, obj)) {
				show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
				return;
			}
			for (ptr = reference; *ptr; ptr++) {
				if (*ptr == ':') {
					show_mcp_error(mfr, "simpleedit-set", "Bad property name.");
					return;
				}
			}
			if (Prop_System(reference) || (!Wizard(player) && (Prop_SeeOnly(reference) || Prop_Hidden(reference)))) {
				show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
				return;
			}
			if (!string_compare(valtype, "string-list")) {

				if (lines == 0) {
					snprintf(buf, sizeof(buf), "%s#", reference);
					remove_property(obj, buf);
				} else {
					snprintf(buf, sizeof(buf), "%s#", reference);
					remove_property(obj, buf);
					add_property(obj, buf, "", lines);
					for (line = 0; line < lines; line++) {
						content = mcp_mesg_arg_getline(msg, "content", line);
						if (!content || !*content) {
							content = " ";
						}
						snprintf(buf, sizeof(buf), "%s#/%d", reference, line + 1);
						add_property(obj, buf, content, 0);
					}
				}
			} else if (!string_compare(valtype, "string") ||
					   !string_compare(valtype, "integer")) {
				show_mcp_error(mfr, "simpleedit-set", "Bad value type for proplist.");
				return;
			}

		} else if (!string_compare(category, "prog")) {
			struct line *tmpline;
			struct line *curr = NULL;
			struct line *new_line;

			if (obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE) {
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

			for (line = 0; line < lines; line++) {
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

			log_status("PROGRAM SAVED: %s by %s(%d)",
					   unparse_object(player, obj), NAME(player), player);

			write_program(PROGRAM_FIRST(obj), obj);

			if (tp_log_programs)
				log_program_text(PROGRAM_FIRST(obj), player, obj);

			do_compile(descr, player, obj, 1);

			free_prog_text(PROGRAM_FIRST(obj));

			PROGRAM_SET_FIRST(obj, tmpline);

			DBDIRTY(player);
			DBDIRTY(obj);

		} else if (!string_compare(category, "sysparm")) {
			if (!Wizard(player)) {
				show_mcp_error(mfr, "simpleedit-set", "Permission denied.");
				return;
			}
			if (lines != 1) {
				show_mcp_error(mfr, "simpleedit-set", "Bad @tune value.");
				return;
			}
			content = mcp_mesg_arg_getline(msg, "content", 0);
			tune_setparm(reference, content);

		} else if (!string_compare(category, "user")) {
		} else {
			show_mcp_error(mfr, "simpleedit-set", "Unknown reference category.");
			return;
		}
	}
}
static const char *mcppkgs_c_version = "$RCSfile: mcppkgs.c,v $ $Revision: 1.11 $";
const char *get_mcppkgs_c_version(void) { return mcppkgs_c_version; }
