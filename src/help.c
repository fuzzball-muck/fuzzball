/** @file help.c
 *
 * Implementation of the various commands that load text files and display
 * them to users.  Some of these files have special formatting to enable
 * retreiving information on specific keywords.  Help, man, mpi, info, and
 * motd are all examples.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "config.h"

#include "commands.h"
#include "db.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "log.h"
#include "tune.h"

/**
 * Read a file from a directory with support for partial match
 *
 * Outputs the file to a player.  'topic' is a file name in
 * directory 'd'.  If partial is true, then a partial match also
 * works.  If the file is found we hand off to spit_file_segment.
 *
 * @see spit_file_segment
 *
 * @private
 * @param player the player to output the file to
 * @param dir the directory to look for topics in
 * @param topic the file or partial file to search for
 * @param seg the segment -- @see spit_file_segment
 * @param partial boolean if true, allow partial file name match
 * @return boolean true if we handed off to spit_file_segment, false otherwise
 */
static int
show_subfile(dbref player, const char *dir, const char *topic, const char *seg,
             int partial)
{
    char buf[BUFFER_LEN-256];
    struct stat st;

#ifdef DIR_AVAILABLE
    DIR *df;
    struct dirent *dp;
#endif

#ifdef WIN32
    char *dirname;
    int dirnamelen = 0;
    HANDLE hFind;
    BOOL bMore;
    WIN32_FIND_DATA finddata;
#endif

    if (!topic || !*topic)
        return 0;

    if ((*topic == '.') || (*topic == '~') || (strchr(topic, '/'))) {
        return 0;
    }

    if (strlen(topic) > 63)
        return 0;

#ifdef DIR_AVAILABLE
    /* Method of operation: (1) exact match, or (2) partial match, but unique */
    *buf = 0;

    if ((df = (DIR *) opendir(dir))) {
        while ((dp = readdir(df))) {
            if ((partial && string_prefix(dp->d_name, topic)) ||
                (!partial && !strcasecmp(dp->d_name, topic))) {
                snprintf(buf, sizeof(buf), "%s/%s", dir, dp->d_name);
                break;
            }
        }

        closedir(df);
    }

    if (!*buf) {
        return 0; /* no such file or directory */
    }

#elif WIN32
    /* Method of operation: (1) exact match, or (2) partial match, but unique */
    *buf = 0;

    dirnamelen = strlen(dir) + 5;
    dirname = malloc(dirnamelen);
    strcpyn(dirname, dirnamelen, dir);
    strcatn(dirname, dirnamelen, "/*.*");
    hFind = FindFirstFile(dirname, &finddata);
    bMore = (hFind != (HANDLE) - 1);

    free(dirname);

    while (bMore) {
        if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if ((partial && string_prefix(finddata.cFileName, topic)) ||
                (!partial && !strcasecmp(finddata.cFileName, topic))) {
                snprintf(buf, sizeof(buf), "%s/%s", dir, finddata.cFileName);
                break;
            }
        }

        bMore = FindNextFile(hFind, &finddata);
    }
#else /* !DIR_AVAILABLE && !WIN32 */
    snprintf(buf, sizeof(buf), "%s/%s", dir, topic);
#endif

    if (stat(buf, &st)) {
        return 0;
    } else {
        spit_file_segment(player, buf, seg);
        return 1;
    }
}

/**
 * This takes a file in "index file" format and reads a given topic.
 *
 * If no topic is provided, the first entry of the index file will be
 * displayed.  If the file isn't present, the user is notified that
 * the file is missing.  If the topic isn't found or there is an error
 * reading, it will say the topic is not available.
 *
 * This seems like the old way of handling help files -- the new way
 * seems to be show_subfile
 *
 * @see show_subfile
 *
 * The file format is a series of data blocks delimited by lines that
 * start with ~.  The first line of each entry is a set of topics
 * that are delimited with | which are aliases for the given data
 * block.  The rest of the data block is what is is displayed to the
 * user up to EOF or a ~.
 *
 * Hopefully that makes sense -- the "help.txt" and related files are
 * good examples of this.
 *
 * @private
 * @param player the player querying the index file
 * @param onwhat the topic we are looking up, or ""
 * @param file the index file to load.
 * @return 0 on failure, 1 on success
 */
static int
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
        return 0;
    } else {
        if (*topic) {
            arglen = strlen(topic);

            /* If we have a topic .... */
            do {
                /* Skip over the data block we don't care about
                 * Or if we run out of file, stop.
                 */
                do {
                    if (!(fgets(buf, sizeof buf, f))) {
                        notifyf(player, "Sorry, no help available on topic \"%s\"", onwhat);
                        fclose(f);
                        return 1;
                    }
                } while (*buf != '~');

                /* Skip delimiters.  Again, stop if we run out of file. */
                do {
                    if (!(fgets(buf, sizeof buf, f))) {
                        notifyf(player, "Sorry, no help available on topic \"%s\"", onwhat);
                        fclose(f);
                        return 1;
                    }
                } while (*buf == '~');

                p = buf;
                found = 0;
                buf[strlen(buf) - 1] = '|';

                /* We should now have a list of topic keywords with |
                 * delimiters.  Let's iterate and see if we have a match.
                 */
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

        /* We either found our topic or don't have one -- read the file
         * til the next ~ line and output it to the player.
         */
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

    return 1;
}

/**
 * Implementation of mpi, help, man, and news
 *
 * This is the common command for dealing with different times of file
 * reads.  It understands "sub files" and "index files".  For
 * details about subfiles, see...
 *
 * @see show_subfile
 *
 * Index files are documented in the private function:
 *
 * @see index_file
 *
 * Basically, index files are a series of entries delimited by lines that
 * start with ~.  Each entry, except for the first one, starts with a
 * line of keywords delimited by | that can match the entry.
 *
 * Note: This function will also attempt to load file information from
 *       the system path defined in FBMUCK_GLOBAL_DATA_PATH ... for
 *       security reasons, we do not try to load "subfile" style files
 *       from the global path as that seems like a good way to probe
 *       random directories on the system.  This feature does not work
 *       on windows.
 *
 * @param player the player asking for help
 * @param dir the directory for subfiles
 * @param file the file for index files
 * @param topic the topic the player is looking up or ""
 * @param segment the range of lines to view - only works for subfiles
 */
void
do_helpfile(dbref player, const char *dir, const char *file, const char *topic, const char *segment)
{
    if ((!*topic || !*segment) && strchr(match_args, ARG_DELIMITER))
        topic = match_args;

    if (show_subfile(player, dir, topic, segment, 0))
        return;

    if (!index_file(player, topic, file)) {
#ifdef FBMUCK_GLOBAL_DATA_PATH
        /*
         * Usually, 'file' is going to have some path fragment on it.
         * Such as 'data/mufman.txt' or whatever.  Thus, we need to clear
         * off that fragment.
         *
         * 'file' should never end in /.  If it does, this will kind of
         * produce a nonsense (but harmless) result since it will try
         * to open FBMUCK_GLOBAL_DATA_PATH as a string and fail.
         */
        char  pathbuf[BUFFER_LEN];
        const char* name = file;
        int   i;

        /*
         * Iterate over 'file', finding /'s til we find the last one.
         */
        for (i = 0; file[i] != '\0'; i++) {
            if (file[i] == '/') {
                name = &file[i+1];
            }
        }

        if (i >= (BUFFER_LEN - (sizeof(FBMUCK_GLOBAL_DATA_PATH)+2))) {
            notifyf(
                player,
                "Sorry, %s is broken.  Management has been notified.",
                file
            );
            fprintf(stderr, "help: file %s is unreasonably long!\n", file);
            return;
        }

        /* Form the path */
        strncpy(pathbuf, FBMUCK_GLOBAL_DATA_PATH, BUFFER_LEN-1);
        strncat(pathbuf, "/", BUFFER_LEN-1);
        strncat(pathbuf, name, BUFFER_LEN-1);

        /* Try to load it. */
        if (!index_file(player, topic, pathbuf)) {
#else
        if (1) {
#endif

            notifyf(
                player,
                "Sorry, %s is missing.  Management has been notified.",
                file
            );
            fprintf(stderr, "help: No file %s!\n", file);
        }
    }
}

/**
 * Write MOTD text to the MOTD file
 *
 * This writes 'text' out to the file, doing line wrapping at between
 * 68 and 76 characters.
 *
 * @private
 * @param text to write to the file.
 */
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

/**
 * Implemenetation of the motd command
 *
 * If 'text' is not providd, or the player is not a wizard, then the
 * contents of the motd file will be displayed to the player.
 *
 * If the player is a wizard, 'text' can either be the string "clear"
 * to delete the MOTD, or the text you want to put in the MOTD file.
 *
 * Time is automatically added to the start of the entry, and a dashed
 * line added to the end.  Dashed line will be written to the file
 * if cleared.
 *
 * @param player the player doing the call
 * @param text either "", "clear", or a new MOTD message.
 */
void
do_motd(dbref player, char *text)
{
    time_t lt;
    char buf[BUFFER_LEN];

    if (!*text || !Wizard(OWNER(player))) {
        spit_file_segment(player, tp_file_motd, "");
        return;
    }

    if (!strcasecmp(text, "clear")) {
        unlink(tp_file_motd);
        log2file(tp_file_motd, "%s %s",
                 "- - - - - - - - - - - - - - - - - - -",
                 "- - - - - - - - - - - - - - - - - - -");
        notify(player, "MOTD cleared.");
        return;
    }

    lt = time(NULL);
    strftime(buf, sizeof(buf), "%a %b %d %T %Z %Y", localtime(&lt));
    log2file(tp_file_motd, "%s", buf);
    add_motd_text_fmt(text);
    log2file(tp_file_motd, "%s %s",
             "- - - - - - - - - - - - - - - - - - -",
             "- - - - - - - - - - - - - - - - - - -");
    notify(player, "MOTD updated.");
}

/**
 * Implementation of the info command
 *
 * This one works a little differently from help and its ilk.  If
 * no 'topic' is provided, then a directory listing is done to find
 * available info files in tp_file_info_dir unless directory listing
 * support isn't compiled in (DIR_AVAILABLE unset).
 *
 * Only subfiles are supported.  @see show_subfile
 *
 * @param player the player doing the info call
 * @param topic to look up or ""
 * @param seg the segment of the file - a range of lines to display.
 */
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

        /* @TODO This is a security hole.  A malicious wizard could set
         *       tp_file_info_dir to any directory on the system and
         *       then use 'info' to browse the files in it.
         *
         *       Albeit something of a minor security hole; wizards should
         *       be trusted.  This could be fixed by limiting the paths
         *       we can open here, but, that limits installation layouts.
         *
         *       May not be worth doing anything about.
         */
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
#else           /* !DIR_AVAILABLE && !WIN32 */
        notify(player, "Index not available on this system.");
#endif          /* !DIR_AVAILABLE && !WIN32 */
    }
}

/**
 * Implementation of \@credits
 *
 * Just spits a credits file (tp_file_credits) out to the player.
 *
 * @see spit_file
 *
 * @param player the player to show the credits to.
 */
void
do_credits(dbref player)
{
    spit_file_segment(player, tp_file_credits, "");
}

/**
 * Implementation of do_version
 *
 * Displays version information to player.
 *
 * @param player
 */
void
do_version(dbref player)
{
    notifyf(player, "Version: %s", VERSION);
    notifyf(player, "Options: %s", compile_options);
}
