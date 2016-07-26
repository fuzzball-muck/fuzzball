#include "config.h"

#include "db.h"
#include "fbtime.h"
#include "fbstrings.h"
#include "inst.h"
#include "log.h"

#include <stdarg.h>

/* cks: these are varargs routines. We are assuming ANSI C. We could at least
   USE ANSI C varargs features, no? Sigh. */

void
vlog2file(int prepend_time, char *filename, char *format, va_list args)
{
    FILE *fp;
    time_t lt;
    char buf[40];
    lt = time(NULL);
    *buf = '\0';

    if ((fp = fopen(filename, "ab")) == NULL) {
	fprintf(stderr, "Unable to open %s!\n", filename);
	if (prepend_time)
	    format_time(buf, 32, "%Y-%m-%dT%H:%M:%S", MUCK_LOCALTIME(lt));
	    fprintf(fp, "%.32s: ", buf);
	vfprintf(stderr, format, args);
    } else {
	if (prepend_time) {
	    format_time(buf, 32, "%Y-%m-%dT%H:%M:%S", MUCK_LOCALTIME(lt));
	    fprintf(fp, "%.32s: ", buf);
	}

	vfprintf(fp, format, args);
	fprintf(fp, "\n");

	fclose(fp);
    }
}

void
log2file(char *filename, char *format, ...)
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
log_function(LOG_SANITY)

void
log_status(char *format, ...)
log_function(LOG_STATUS)

void
log_muf(char *format, ...)
log_function(LOG_MUF)

void
log_gripe(char *format, ...)
log_function(LOG_GRIPE)

void
log_command(char *format, ...)
log_function(COMMAND_LOG)

void
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
    format_time(buf, 32, "%Y-%m-%dT%H:%M:%S", MUCK_LOCALTIME(lt));

    snprintf(logformat, BUFFER_LEN, "%s: %s(#%d) [%s(#%d)]: ",
             buf, NAME(player), player, NAME(program), program);
    len = BUFFER_LEN - strlen(logformat) - 1;
    strncat(logformat, logmessage, len);
    strip_evil_characters(logformat);
    log2file(USER_LOG, "%s", logformat);
}

void
log_program_text(struct line *first, dbref player, dbref i)
{
    FILE *f;
    char fname[BUFFER_LEN];
    char tbuf[24];
    time_t lt = time(NULL);

    strcpyn(fname, sizeof(fname), PROGRAM_LOG);
    f = fopen(fname, "ab");
    if (!f) {
        log_status("Couldn't open file %s!", fname);
        return;
    }

    format_time(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", MUCK_LOCALTIME(lt));
    fputs("#######################################", f);
    fputs("#######################################\n", f);
    fprintf(f, "%s: %s SAVED BY %s(#%d)\n",
            tbuf, unparse_object(player, i), NAME(player), player);
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
