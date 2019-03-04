#include "config.h"

#include "commands.h"
#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "tune.h"

static void
index_file(dbref player, const char *onwhat, const char *file)
{
    FILE *f;
    char buf[BUFFER_LEN];
    char topic[BUFFER_LEN];
    char *p;
    size_t arglen;
    int found;

    *topic = '\0';
    strcpyn(topic, sizeof(topic), onwhat);
    if (*onwhat) {
	strcatn(topic, sizeof(topic), "|");
    }

    if ((f = fopen(file, "rb")) == NULL) {
	notifyf(player, "Sorry, %s is missing.  Management has been notified.", file);
	fprintf(stderr, "help: No file %s!\n", file);
    } else {
	if (*topic) {
	    arglen = strlen(topic);
	    do {
		do {
		    if (!(fgets(buf, sizeof buf, f))) {
			notifyf(player, "Sorry, no help available on topic \"%s\"", onwhat);
			fclose(f);
			return;
		    }
		} while (*buf != '~');
		do {
		    if (!(fgets(buf, sizeof buf, f))) {
			notifyf(player, "Sorry, no help available on topic \"%s\"", onwhat);
			fclose(f);
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
	while (fgets(buf, sizeof buf, f)) {
	    if (*buf == '~')
		break;
	    for (p = buf; *p; p++) {
		if (*p == '\n' || *p == '\r') {
		    *p = '\0';
		    break;
		}
	    }
	    if (*buf) {
		notify(player, buf);
	    } else {
		notify(player, "  ");
	    }
	}
	fclose(f);
    }
}

void
do_helpfile(dbref player, const char *dir, const char *file, char *topic, char *segment)
{
    if ((!*topic || !*segment) && strchr(match_args, ARG_DELIMITER))
        topic = match_args;

    if (show_subfile(player, dir, topic, segment, 0))
	return;
    index_file(player, topic, file);
}

static void
add_motd_text_fmt(const char *text)
{
    char buf[80];
    const char *p = text;
    int count = 4;

    buf[0] = buf[1] = buf[2] = buf[3] = ' ';
    while (*p) {
	while (*p && (count < 68))
	    buf[count++] = *p++;
	while (*p && !isspace(*p) && (count < 76))
	    buf[count++] = *p++;
	buf[count] = '\0';
	log2file(tp_file_motd, "%s", buf);
	skip_whitespace(&p);
	count = 0;
    }
}

void
do_motd(dbref player, char *text)
{
    time_t lt;
    char buf[BUFFER_LEN];

    if (!*text || !Wizard(OWNER(player))) {
	spit_file(player, tp_file_motd);
	return;
    }
    if (!strcasecmp(text, "clear")) {
	unlink(tp_file_motd);
	log2file(tp_file_motd, "%s %s", "- - - - - - - - - - - - - - - - - - -",
		 "- - - - - - - - - - - - - - - - - - -");
	notify(player, "MOTD cleared.");
	return;
    }

    lt = time(NULL);
    strftime(buf, sizeof(buf), "%a %b %d %T %Z %Y", localtime(&lt));
    log2file(tp_file_motd, "%s", buf);
    add_motd_text_fmt(text);
    log2file(tp_file_motd, "%s %s", "- - - - - - - - - - - - - - - - - - -",
	     "- - - - - - - - - - - - - - - - - - -");
    notify(player, "MOTD updated.");
}

void
do_info(dbref player, const char *topic, const char *seg)
{
    char *buf;
    int f;
    int cols;
    size_t buflen = 80;

#ifdef DIR_AVAILABLE
    DIR *df;
    struct dirent *dp;
#endif
#ifdef WIN32
    HANDLE hFind;
    BOOL bMore;
    WIN32_FIND_DATA finddata;
    char *dirname;
    int dirnamelen = 0;
#endif

    if (*topic) {
	if (!show_subfile(player, tp_file_info_dir, topic, seg, 1)) {
	    notify(player, NO_INFO_MSG);
	}
    } else {
#ifdef DIR_AVAILABLE
	buf = calloc(1, buflen);
	(void) strcpyn(buf, buflen, "    ");
	f = 0;
	cols = 0;
	if ((df = (DIR *) opendir(tp_file_info_dir))) {
	    while ((dp = readdir(df))) {

		if (*(dp->d_name) != '.') {
		    if (!f)
			notify(player, "Available information files are:");
		    if ((cols++ > 2) || ((strlen(buf) + strlen(dp->d_name)) > 63)) {
			notify(player, buf);
			strcpyn(buf, buflen, "    ");
			cols = 0;
		    }
		    strcatn(buf, buflen, dp->d_name);
		    strcatn(buf, buflen, " ");
		    f = strlen(buf);
		    while ((f % 20) != 4)
			buf[f++] = ' ';
		    buf[f] = '\0';
		}
	    }
	    closedir(df);
	}
	if (f)
	    notify(player, buf);
	else
	    notify(player, "No information files are available.");
	free(buf);
#elif WIN32
	buf = calloc(1, buflen);
	(void) strcpyn(buf, buflen, "    ");
	f = 0;
	cols = 0;

	dirnamelen = strlen(tp_file_info_dir) + 4;
	dirname = malloc(dirnamelen);
	strcpyn(dirname, dirnamelen, tp_file_info_dir);
	strcatn(dirname, dirnamelen, "*.*");
	hFind = FindFirstFile(dirname, &finddata);
	bMore = (hFind != (HANDLE) - 1);

	free(dirname);

	while (bMore) {
	    if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		if (!f)
		    notify(player, "Available information files are:");
		if ((cols++ > 2) || ((strlen(buf) + strlen(finddata.cFileName)) > 63)) {
		    notify(player, buf);
		    (void) strcpyn(buf, buflen, "    ");
		    cols = 0;
		}
		strcatn(buf, buflen, finddata.cFileName);
		strcatn(buf, buflen, " ");
		f = strlen(buf);
		while ((f % 20) != 4)
		    buf[f++] = ' ';
		buf[f] = '\0';
	    }
	    bMore = FindNextFile(hFind, &finddata);
	}

	if (f)
	    notify(player, buf);
	else
	    notify(player, "There are no information files available.");

	free(buf);
#else				/* !DIR_AVAILABLE && !WIN32 */
	notify(player, "Index not available on this system.");
#endif				/* !DIR_AVAILABLE && !WIN32 */
    }
}

void
do_credits(dbref player)
{
    spit_file(player, tp_file_credits);
}
