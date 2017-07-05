#include "config.h"

#include "db.h"
#include "fbtime.h"
#include "fbstrings.h"
#include "inst.h"
#include "log.h"
#include "tune.h"

static void
vlog2file(int prepend_time, const char *filename, char *format, va_list args)
{
    FILE *fp;
    time_t lt;
    char buf[40];
    lt = time(NULL);
    *buf = '\0';

    if ((fp = fopen(filename, "ab")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", filename);
	if (prepend_time)
	    strftime(buf, 32, "%Y-%m-%dT%H:%M:%S", localtime(&lt));
	    fprintf(fp, "%.32s: ", buf);
	vfprintf(stderr, format, args);
    } else {
	if (prepend_time) {
	    strftime(buf, 32, "%Y-%m-%dT%H:%M:%S", localtime(&lt));
	    fprintf(fp, "%.32s: ", buf);
	}

	vfprintf(fp, format, args);
	fprintf(fp, "\n");

	fclose(fp);
    }
}

void
log2file(const char *filename, char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog2file(0, filename, format, args);
    va_end(args);
}

#define log_function(FILENAME) \
{ \
	va_list args; \
	va_start(args, format); \
	vlog2file(1, FILENAME, format, args); \
	va_end(args); \
}

void
log_sanity(char *format, ...)
log_function(tp_file_log_sanity)

void
log_sanfix(char *format, ...)
log_function(tp_file_log_sanfix)

void
log_status(char *format, ...)
log_function(tp_file_log_status)

void
log_muf(char *format, ...)
log_function(tp_file_log_muf_errors)

void
log_gripe(char *format, ...)
log_function(tp_file_log_gripes)

void
log_command(char *format, ...)
log_function(tp_file_log_commands)

static void
strip_evil_characters(char *badstring)
{
    unsigned char *s = (unsigned char *) badstring;

    if (!s)
	return;

    for (; *s; ++s) {
	*s &= 0x7f;		/* No high ascii */

	if (*s == 0x1b)
	    *s = '[';		/* No escape (Aieeee!) */

	if (!isprint(*s))
	    *s = '_';
    }
    return;
}

void
log_user(dbref player, dbref program, char *logmessage)
{
    char logformat[BUFFER_LEN];
    char buf[40];
    time_t lt = 0;
    int len = 0;

    *buf = '\0';
    *logformat = '\0';

    lt = time(NULL);
    strftime(buf, 32, "%Y-%m-%dT%H:%M:%S", localtime(&lt));

    snprintf(logformat, BUFFER_LEN, "%s: %s(#%d) [%s(#%d)]: ",
             buf, NAME(player), player, NAME(program), program);
    len = BUFFER_LEN - strlen(logformat) - 1;
    strncat(logformat, logmessage, len);
    strip_evil_characters(logformat);
    log2file(tp_file_log_user, "%s", logformat);
}

void
log_program_text(struct line *first, dbref player, dbref i)
{
    FILE *f;
    char fname[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];
    char tbuf[24];
    time_t lt = time(NULL);

    strcpyn(fname, sizeof(fname), tp_file_log_programs);
    f = fopen(fname, "ab");
    if (!f) {
        log_status("Couldn't open file %s!", fname);
        return;
    }

    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", localtime(&lt));
    fputs("#######################################", f);
    fputs("#######################################\n", f);
    unparse_object(player, i, unparse_buf, sizeof(unparse_buf));
    fprintf(f, "%s: %s SAVED BY %s(#%d)\n",
            tbuf, unparse_buf, NAME(player), player);
    fputs("#######################################", f);
    fputs("#######################################\n\n", f);

    while (first) {
        if (!first->this_line)
            continue;
        fputs(first->this_line, f);
        fputc('\n', f);
        first = first->next;
    }
    fputs("\n\n\n", f);
    fclose(f);
}

char *
whowhere(dbref who)
{
    char buf[BUFFER_LEN];

    snprintf(buf, sizeof(buf), "%s%s%s%s(#%d) in %s(#%d)",
             Wizard(OWNER(who)) ? "WIZ: " : "",
             (Typeof(who) != TYPE_PLAYER) ? NAME(who) : "",
             (Typeof(who) != TYPE_PLAYER) ? " owned by " : "",
             NAME(OWNER(who)), who, NAME(LOCATION(who)), LOCATION(who));

    return strdup(buf);
}
