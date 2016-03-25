#include "config.h"

/* commands for giving help */

#include "db.h"
#include "externs.h"
#if defined(MCP_SUPPORT) && !defined(STANDALONE_HELP)
#include "mcppkg.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdarg.h>

/*
 * Ok, directory stuff IS a bit ugly.
 */
#if defined(HAVE_DIRENT_H) || defined(_POSIX_VERSION)
# include <dirent.h>
# define NLENGTH(dirent) (strlen((dirent)->d_name))
#else							/* not (HAVE_DIRENT_H or _POSIX_VERSION) */
# define dirent direct
# define NLENGTH(dirent) ((dirent)->d_namlen)
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif							/* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif							/* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif							/* HAVE_NDIR_H */
#endif							/* not (HAVE_DIRENT_H or _POSIX_VERSION) */

#if defined(HAVE_DIRENT_H) || defined(_POSIX_VERSION) || defined(HAVE_SYS_NDIR_H) || defined(HAVE_SYS_DIR_H) || defined(HAVE_NDIR_H)
# define DIR_AVALIBLE
#endif

#if defined(STANDALONE_HELP)
# define dbref int

int
notify(dbref player, const char *msg)
{
	return printf("%s\n", msg);
}

void
notifyf(dbref player, char *format, ...)
{
        va_list args;
        char bufr[BUFFER_LEN];

        va_start(args, format);
        vsnprintf(bufr, sizeof(bufr), format, args);
        bufr[sizeof(bufr)-1] = '\0';
        notify(player, bufr);
        va_end(args);
}

int
string_prefix(register const char *string, register const char *prefix)
{
	while (*string && *prefix && tolower(*string) == tolower(*prefix))
				string++, prefix++;
		return *prefix == '\0';
}

int
string_compare(register const char *s1, register const char *s2)
{
	while (*s1 && tolower(*s1) == tolower(*s2))
		s1++, s2++;

	return (tolower(*s1) - tolower(*s2));
}

char*
strcpyn(char* buf, size_t bufsize, const char* src)
{
	int pos = 0;
	char* dest = buf;

	while (++pos < bufsize && *src) {
		*dest++ = *src++;
	}
	*dest = '\0';
	return buf;
}

char*
strcatn(char* buf, size_t bufsize, const char* src)
{
	int pos = strlen(buf);
	char* dest = &buf[pos];

	while (++pos < bufsize && *src) {
		*dest++ = *src++;
	}
	if (pos <= bufsize) {
		*dest = '\0';
	}
	return buf;
}

#endif

void
spit_file_segment(dbref player, const char *filename, const char *seg)
{
	FILE *f;
	char buf[BUFFER_LEN];
	char segbuf[BUFFER_LEN];
	char *p;
	int startline, endline, currline;

	startline = endline = currline = 0;
	if (seg && *seg) {
		strcpyn(segbuf, sizeof(segbuf), seg);
		for (p = segbuf; isdigit(*p); p++) ;
		if (*p) {
			*p++ = '\0';
			startline = atoi(segbuf);
			while (*p && !isdigit(*p))
				p++;
			if (*p)
				endline = atoi(p);
		} else {
			endline = startline = atoi(segbuf);
		}
	}
	if ((f = fopen(filename, "rb")) == NULL) {
		notifyf(player, "Sorry, %s is missing.  Management has been notified.", filename);
		fputs("spit_file:", stderr);
		perror(filename);
	} else {
		while (fgets(buf, sizeof buf, f)) {
			for (p = buf; *p; p++) {
				if (*p == '\n' || *p == '\r') {
					*p = '\0';
					break;
				}
			}
			currline++;
			if ((!startline || (currline >= startline)) && (!endline || (currline <= endline))) {
				if (*buf) {
					notify(player, buf);
				} else {
					notify(player, "  ");
				}
			}
		}
		fclose(f);
	}
}

void
spit_file(dbref player, const char *filename)
{
	spit_file_segment(player, filename, "");
}


void
index_file(dbref player, const char *onwhat, const char *file)
{
	FILE *f;
	char buf[BUFFER_LEN];
	char topic[BUFFER_LEN];
	char *p;
	int arglen, found;

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

int
show_subfile(dbref player, const char *dir, const char *topic, const char *seg, int partial)
{
	char buf[256];
	struct stat st;

#ifdef DIR_AVALIBLE
	DIR *df;
	struct dirent *dp;
#endif

#ifdef WIN32
	char   *dirname;
	int dirnamelen = 0;
	HANDLE  hFind;
	BOOL    bMore;
	WIN32_FIND_DATA finddata;
#endif

	if (!topic || !*topic)
		return 0;

	if ((*topic == '.') || (*topic == '~') || (index(topic, '/'))) {
		return 0;
	}
	if (strlen(topic) > 63)
		return 0;


#ifdef DIR_AVALIBLE
	/* TO DO: (1) exact match, or (2) partial match, but unique */
	*buf = 0;

	if ((df = (DIR *) opendir(dir))) {
		while ((dp = readdir(df))) {
			if ((partial && string_prefix(dp->d_name, topic)) ||
				(!partial && !string_compare(dp->d_name, topic))
					) {
				snprintf(buf, sizeof(buf), "%s/%s", dir, dp->d_name);
				break;
			}
		}
		closedir(df);
	}

	if (!*buf) {
		return 0;				/* no such file or directory */
	}
#elif WIN32
	/* TO DO: (1) exact match, or (2) partial match, but unique */
	*buf = 0;

	dirnamelen = strlen(dir) + 5;
	dirname = (char *) malloc(dirnamelen);
	strcpyn(dirname, dirnamelen, dir);
	strcatn(dirname, dirnamelen, "/*.*");
	hFind = FindFirstFile(dirname,&finddata);
	bMore = (hFind != (HANDLE) -1);

	free(dirname);

	while (bMore) {
		if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if ((partial && string_prefix(finddata.cFileName, topic)) ||
				(!partial && !string_compare(finddata.cFileName,topic))
				)
			{
				snprintf(buf, sizeof(buf), "%s/%s", dir, finddata.cFileName);
				break;
			}
		}
		bMore = FindNextFile(hFind, &finddata);
	}
#else                           /* !DIR_AVAILABLE && !WIN32 */
	snprintf(buf, sizeof(buf), "%s/%s", dir, topic);
#endif 

	if (stat(buf, &st)) {
		return 0;
	} else {
		spit_file_segment(player, buf, seg);
		return 1;
	}
}


#if !defined(STANDALONE_HELP)
void
do_man(dbref player, char *topic, char *seg)
{
	if (show_subfile(player, MAN_DIR, topic, seg, FALSE))
		return;
	index_file(player, topic, MAN_FILE);
}


void
do_mpihelp(dbref player, char *topic, char *seg)
{
	if (show_subfile(player, MPI_DIR, topic, seg, FALSE))
		return;
	index_file(player, topic, MPI_FILE);
}


void
do_help(dbref player, char *topic, char *seg)
{
	if (show_subfile(player, HELP_DIR, topic, seg, FALSE))
		return;
	index_file(player, topic, HELP_FILE);
}


void
do_news(dbref player, char *topic, char *seg)
{
	if (show_subfile(player, NEWS_DIR, topic, seg, FALSE))
		return;
	index_file(player, topic, NEWS_FILE);
}


void
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
		log2file(MOTD_FILE, "%s", buf);
		while (*p && isspace(*p))
			p++;
		count = 0;
	}
}


void
do_motd(dbref player, char *text)
{
	time_t lt;

	if (!*text || !Wizard(OWNER(player))) {
		spit_file(player, MOTD_FILE);
		return;
	}
	if (!string_compare(text, "clear")) {
		unlink(MOTD_FILE);
		log2file(MOTD_FILE, "%s %s", "- - - - - - - - - - - - - - - - - - -",
				 "- - - - - - - - - - - - - - - - - - -");
		notify(player, "MOTD cleared.");
		return;
	}
	lt = time(NULL);
	log2file(MOTD_FILE, "%.16s", ctime(&lt));
	add_motd_text_fmt(text);
	log2file(MOTD_FILE, "%s %s", "- - - - - - - - - - - - - - - - - - -",
			 "- - - - - - - - - - - - - - - - - - -");
	notify(player, "MOTD updated.");
}


void
do_info(dbref player, const char *topic, const char *seg)
{
	char *buf;
	int f;
	int cols;
	int buflen = 80;

#ifdef DIR_AVALIBLE
	DIR *df;
	struct dirent *dp;
#endif
#ifdef WIN32
	HANDLE  hFind;
	BOOL    bMore;
	WIN32_FIND_DATA finddata;
	char    *dirname;
	int dirnamelen = 0;
#endif

	if (*topic) {
		if (!show_subfile(player, INFO_DIR, topic, seg, TRUE)) {
			notify(player, NO_INFO_MSG);
		}
	} else {
#ifdef DIR_AVALIBLE
		buf = (char *) calloc(1, buflen);
		(void) strcpyn(buf, buflen, "    ");
		f = 0;
		cols = 0;
		if ((df = (DIR *) opendir(INFO_DIR))) {
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
		buf = (char *) calloc(1,buflen);
		(void) strcpyn(buf, buflen, "    ");
		f = 0;
		cols = 0;

		dirnamelen = strlen(INFO_DIR) + 4;
		dirname = (char *) malloc(dirnamelen);
		strcpyn(dirname, dirnamelen, INFO_DIR);
		strcatn(dirname, dirnamelen, "*.*");
		hFind = FindFirstFile(dirname,&finddata);
		bMore = (hFind != (HANDLE) -1);

		free(dirname);

		while (bMore) {
			if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if (!f)
					notify(player, "Available information files are:");
				if ((cols++ > 2) || ((strlen(buf) + strlen(finddata.cFileName)) > 63)) {
					notify(player,buf);
					(void) strcpyn(buf, buflen, "    ");
					cols = 0;
				}
			    strcatn(buf, buflen, finddata.cFileName);
			    strcatn(buf, buflen, " ");
				f = strlen(buf);
				while((f %20) != 4)
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
#else							/* !DIR_AVALIBLE && !WIN32 */
		notify(player, "Index not available on this system.");
#endif							/* !DIR_AVALIBLE && !WIN32 */
	}
}

void
do_credits(dbref player)
{
	spit_file(player, CREDITS_FILE);
}

#else /* STANDALONE_HELP */

int
main(int argc, char**argv)
{
	char* helpfile = NULL;
	char* topic = NULL;
	char buf[BUFFER_LEN];
	if (argc < 2) {
			fprintf(stderr, "Usage: %s muf|mpi|help [topic]\n", argv[0]);
		exit(-1);
	} else if (argc == 2 || argc == 3) {
		if (argc == 2) {
			topic = "";
		} else {
			topic = argv[2];
		}
		if (!strcmp(argv[1], "man")) {
			helpfile = MAN_FILE;
		} else if (!strcmp(argv[1], "muf")) {
			helpfile = MAN_FILE;
		} else if (!strcmp(argv[1], "mpi")) {
			helpfile = MPI_FILE;
		} else if (!strcmp(argv[1], "help")) {
			helpfile = HELP_FILE;
		} else {
			fprintf(stderr, "Usage: %s muf|mpi|help [topic]\n", argv[0]);
			exit(-2);
		}

		helpfile = rindex(helpfile, '/');
		helpfile++;
#ifdef HELPFILE_DIR
		snprintf(buf, sizeof(buf), "%s/%s", HELPFILE_DIR, helpfile);
#else
		snprintf(buf, sizeof(buf), "%s/%s", "/usr/local/fbmuck/help", helpfile);
#endif

		index_file(1, topic, buf);
		exit(0);
	} else if (argc > 3) {
		fprintf(stderr, "Usage: %s muf|mpi|help [topic]\n", argv[0]);
		exit(-1);
	}
	return 0;
}

#endif /* STANDALONE_HELP */
