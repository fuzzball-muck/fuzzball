/** @file prochelp.c
 *
 * Implementation of the standalone process that parses a raw help file and
 * creates a text and HTML representation of it.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "db.h"
#include "tune.h"
#include "tunelist.h"

#undef malloc
#undef strdup

/**
 * These defines exist to make it easy to alter the structure of the output
 * files.  Other than the first, they are used in HTML generation.
 */

#define HRULE_TEXT "----------------------------------------------------------------------------"

#define HTML_PAGE_HEAD          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n<html lang=\"en\">\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n<meta name=\"generator\" content=\"prochelp\">\n<title>%s</title>\n</head>\n<body>\n<h1>%s</h1>\n<address>by %s</address>\n<ul>\n<li><a href=\"#AlphaList\">Alphabetical List of Topics</a></li>\n<li><a href=\"#SectList\">List of Topics by Category</a></li>\n</ul>\n"
#define HTML_PAGE_FOOT          "</body>\n</html>\n"

#define HTML_SECTION            "<h2 id=\"%s\">%s</h2>\n"
#define HTML_SECTLIST_HEAD      "<h2 id=\"SectList\">List of Topics by Category</h2>\n<p>You can get more help on the following topics:</p>\n<ul>\n"
#define HTML_SECTLIST_ENTRY     "  <li><a href=\"#%s\">%s</a></li>\n"
#define HTML_SECTLIST_FOOT      "</ul>\n\n"

#define HTML_SECTIDX_BEGIN      "<ul>\n"
#define HTML_SECTIDX_ENTRY      "    <li><a href=\"#%s\">%s</a></li>\n"
#define HTML_SECTIDX_END        "</ul>\n\n"

#define HTML_INDEX_BEGIN        "<h2 id=\"AlphaList\">Alphabetical List of Topics</h2>\n"
#define HTML_IDXGROUP_BEGIN     "<h3>%s</h3>\n<ul>\n"
#define HTML_IDXGROUP_ENTRY     "    <li><a href=\"#%s\">%s</a></li>\n"
#define HTML_IDXGROUP_END               "</ul>\n\n"

#define HTML_TOPICHEAD          "<h3 id=\"%s\">"
#define HTML_TOPICHEAD_BREAK    "<br>\n"
#define HTML_TOPICBODY          "</h3>\n"
#define HTML_TOPICEND           "<!-- HTML_TOPICEND -->\n"
#define HTML_PARAGRAPH          "<p>\n"

#define HTML_CODEBEGIN          "<pre>\n"
#define HTML_CODEEND            "</pre>\n"

#define HTML_ALSOSEE_BEGIN      "<p>Also see:\n"
#define HTML_ALSOSEE_ENTRY      "    <a href=\"#%s\">%s</a>"
#define HTML_ALSOSEE_END        "\n</p>\n"

/**
 * Defines a token replacement function for help entries.  When the givrn
 * token is present in a line, the entire line is replaced with the "output"
 * of the associated function.  Does not escape HTML.
 */
typedef struct {
    char *token;
    void (*func)(FILE *);
} replacement;

/**
 * Sends the entire list of system parameters to the given file.
 *
 * @private
 * @param f the file being processed.
 */
static void
man_sysparm_list(FILE *f)
{
    for (struct tune_entry *tent = tune_list; tent->name; tent++) {
        fprintf(f, " %-6s %-25s - %s\n", str_tunetype[tent->type], tent->name, tent->label);
    }
}

/**
 * List of recognized token replacements.
 */
static replacement replacements[] = {
    { "%%SYSPARM_LIST%%", man_sysparm_list },
    { 0 }
};

/**
 * @private
 * @var Help file metadata.
 */
static const char *title = "";
static const char *author = "";
static const char *doccmd = "";

/**
 * Copy a string from src to buf, taking into account the size of the buffer
 * and ensuring it is null terminated.
 *
 * @private
 * @param buf Target buffer
 * @param bufsize the size of that buffer
 * @param src the source to copy from
 *
 * @return buf -- because buf is mutated, the return value doesn't matter.
 */
static char *
strcpyn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = 0;
    char *dest = buf;

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }

    *dest = '\0';
    return buf;
}

/**
 * Skip over starting whitespaces in a string
 *
 * Note that this does not technically "trim" spaces; instead, it advances
 * the provided pointer to the first non-space (or to the end of the string
 * as the case may be).
 *
 * @private
 * @param parsebuf pointer to a string pointer.
 */
static void
skip_whitespace(const char **parsebuf)
{
    while (**parsebuf && isspace(**parsebuf))
        (*parsebuf)++;
}

/**
 * @private
 * @var List of section names.
 * @TODO Make the number a config parameter.
 */
static char sect[256] = "";

/**
 * Structure holding a list of topics.
 */

struct topiclist {
    struct topiclist *next;
    char *topic;
    char *section;
    int printed;
};

static struct topiclist *topichead;
static struct topiclist *secthead;

/**
 * Adds a section.
 *
 * @private
 * @param str the name of the section to add
 */
static void
add_section(const char *str)
{
    struct topiclist *ptr, *top;

    if (!str || !*str)
        return;

    top = malloc(sizeof(struct topiclist));

    top->topic = NULL;
    top->section = strdup(sect);
    top->printed = 0;
    top->next = NULL;

    if (!secthead) {
        secthead = top;
        return;
    }

    for (ptr = secthead; ptr->next; ptr = ptr->next) ;

    ptr->next = top;
}

/**
 * Adds a topic to the current section.
 *
 * @private
 * @param str the name of the topic to add
 */
static void
add_topic(const char *str)
{
    struct topiclist *ptr, *top;
    char buf[256];
    const char *p;
    char *s;

    if (!str || !*str)
        return;

    p = str;
    s = buf;

    do {
        *s++ = tolower(*p);
    } while (*p++);

    top = malloc(sizeof(struct topiclist));

    top->topic = strdup(buf);
    top->section = strdup(sect);
    top->printed = 0;

    if (!topichead) {
        topichead = top;
        top->next = NULL;
        return;
    }

    if (strcasecmp(str, topichead->topic) < 0) {
        top->next = topichead;
        topichead = top;
        return;
    }

    ptr = topichead;

    while (ptr->next && strcasecmp(str, ptr->next->topic) > 0) {
        ptr = ptr->next;
    }

    top->next = ptr->next;
    ptr->next = top;
}

/**
 * Changes < > & " in given buf to corresponding HTML entities.
 *
 * @private
 * @param buf Target buffer
 * @param buflen the size of that buffer
 * @param in the source to copy from
 */
static char *
escape_html(char *buf, size_t buflen, const char *in)
{
    char *out = buf;

    while (*in) {
        if (*in == '<') {
            strcpyn(out, buflen, "&lt;");
            out += strlen(out);
        } else if (*in == '>') {
            strcpyn(out, buflen, "&gt;");
            out += strlen(out);
        } else if (*in == '&') {
            strcpyn(out, buflen, "&amp;");
            out += strlen(out);
        } else if (*in == '"') {
            strcpyn(out, buflen, "&quot;");
            out += strlen(out);
        } else {
            *out++ = *in;
        }

        in++;
    }

    *out++ = '\0';
    return buf;
}

/**
 * Outputs a list of section topics.
 *
 * @private
 * @param f the text file being created
 * @param hf the html file being created
 * @param whichsect the name of the current section
 */
static void
print_section_topics(FILE * f, FILE * hf, const char *whichsect)
{
    char sectname[256];
    char *sectptr;
    char *osectptr;
    char *divpos;
    char buf[256];
    char buf2[256];
    char buf3[256];
    int cnt;
    int width;
    int cols;
    int longest;
    char *currsect;

    longest = 0;

    for (struct topiclist *sptr = secthead; sptr; sptr = sptr->next) {
        if (!strncasecmp(whichsect, sptr->section, strlen(whichsect))) {
            currsect = sptr->section;

            for (struct topiclist *ptr = topichead; ptr; ptr = ptr->next) {
                if (!strcasecmp(currsect, ptr->section)) {
                    divpos = strchr(ptr->topic, '|');

                    if (!divpos) {
                        cnt = strlen(ptr->topic);
                    } else {
                        cnt = divpos - ptr->topic;
                    }

                    if (cnt > longest) {
                        longest = cnt;
                    }
                }
            }
        }
    }

    cols = 78 / (longest + 2);

    if (cols < 1) {
        cols = 1;
    }

    width = 78 / cols;

    for (struct topiclist *sptr = secthead; sptr; sptr = sptr->next) {
        if (!strncasecmp(whichsect, sptr->section, strlen(whichsect))) {
            currsect = sptr->section;
            cnt = 0;
            buf[0] = '\0';

            strcpyn(sectname, sizeof(sectname), currsect);
            sectptr = strchr(sectname, '|');

            if (sectptr) {
                *sectptr++ = '\0';
                osectptr = sectptr;
                sectptr = strrchr(sectptr, '|');

                if (sectptr) {
                    sectptr++;
                }

                if (!sectptr) {
                    sectptr = osectptr;
                }
            }

            if (!sectptr) {
                sectptr = "";
            }

            fprintf(hf, HTML_SECTION, escape_html(buf2, sizeof(buf2), sectptr),
                    escape_html(buf3, sizeof(buf3), sectname));
            fprintf(f, "~\n~\n%s\n%s\n\n", currsect, sectname);
            fprintf(hf, HTML_SECTIDX_BEGIN);

            for (struct topiclist *ptr = topichead; ptr; ptr = ptr->next) {
                if (!strcasecmp(currsect, ptr->section)) {
                    ptr->printed++;
                    cnt++;

                    escape_html(buf3, sizeof(buf3), ptr->topic);
                    fprintf(hf, HTML_SECTIDX_ENTRY, buf3, buf3);

                    if (cnt == cols) {
                        snprintf(buf2, sizeof(buf2), "%-.*s", width - 1, ptr->topic);
                    } else {
                        snprintf(buf2, sizeof(buf2), "%-*.*s", width, width - 1, ptr->topic);
                    }

                    strcat(buf, buf2);

                    if (cnt >= cols) {
                        fprintf(f, "%s\n", buf);
                        buf[0] = '\0';
                        cnt = 0;
                    }
                }
            }

            fprintf(hf, HTML_SECTIDX_END);

            if (cnt)
                fprintf(f, "%s\n", buf);

            fprintf(f, "\n");
        }
    }
}

/**
 * Outputs a list of section names.
 *
 * @private
 * @param f the text file being created
 * @param hf the html file being created
 */
static void
print_sections(FILE * f, FILE * hf)
{
    char sectname[256];
    char *osectptr;
    char *sectptr;
    char buf[256];
    char buf3[256];
    char buf4[256];
    char *currsect;

    fprintf(f, "~\n");
    fprintf(f, "~%s\n", HRULE_TEXT);
    fprintf(f, "~\n");
    fprintf(f, "CATEGORY|CATEGORIES|TOPICS|SECTIONS\n");
    fprintf(f, "                   List of Topics by Category:\n \n");
    fprintf(f, "You can get more help on the following topics:\n \n");
    fprintf(hf, HTML_SECTLIST_HEAD);

    for (struct topiclist *sptr = secthead; sptr; sptr = sptr->next) {
        currsect = sptr->section;
        buf[0] = '\0';

        strcpyn(sectname, sizeof(sectname), currsect);
        sectptr = strchr(sectname, '|');

        if (sectptr) {
            *sectptr++ = '\0';
            osectptr = sectptr;
            sectptr = strrchr(sectptr, '|');

            if (sectptr) {
                sectptr++;
            }

            if (!sectptr) {
                sectptr = osectptr;
            }
        }

        if (!sectptr) {
            sectptr = "";
        }

        fprintf(hf, HTML_SECTLIST_ENTRY, escape_html(buf3, sizeof(buf3), sectptr),
                escape_html(buf4, sizeof(buf4), sectname));
        fprintf(f, "  %-40s (%s)\n", sectname, sectptr);
    }

    fprintf(hf, HTML_SECTLIST_FOOT);
    fprintf(f, " \nUse '%s <topicname>' to get more information on a topic.\n", doccmd);
}

/**
 * Outputs a list of topics.
 *
 * @private
 * @param f the text file being created
 * @param hf the html file being created
 */
static void
print_topics(FILE * f, FILE * hf)
{
    char buf[256];
    char buf2[256];
    char buf3[256];
    char firstletter;
    char *divpos;
    int cnt = 0;
    int width;
    int cols;
    int len;
    int longest;

    fprintf(hf, HTML_INDEX_BEGIN);
    fprintf(f, "~\n");
    fprintf(f, "~%s\n", HRULE_TEXT);
    fprintf(f, "~\n");
    fprintf(f, "ALPHA|ALPHABETICAL|COMMANDS\n");
    fprintf(f, "                 Alphabetical List of Topics:\n");
    fprintf(f, " \n");
    fprintf(f, "You can get more help on the following topics:\n");

    for (char alph = 'A' - 1; alph <= 'Z'; alph++) {
        cnt = 0;
        longest = 0;

        for (struct topiclist *ptr = topichead; ptr; ptr = ptr->next) {
            firstletter = toupper(ptr->topic[0]);

            if (firstletter == alph || (!isalpha(alph) && !isalpha(firstletter))) {
                cnt++;
                divpos = strchr(ptr->topic, '|');

                if (!divpos) {
                    len = strlen(ptr->topic);
                } else {
                    len = divpos - ptr->topic;
                }

                if (len > longest) {
                    longest = len;
                }
            }
        }

        cols = 78 / (longest + 2);

        if (cols < 1) {
            cols = 1;
        }

        width = 78 / cols;

        if (cnt > 0) {
            if (!isalpha(alph)) {
                strcpyn(buf, sizeof(buf), "Symbols");
            } else {
                buf[0] = alph;
                buf[1] = '\'';
                buf[2] = 's';
                buf[3] = '\0';
            }

            fprintf(f, "\n%s\n", buf);
            fprintf(hf, HTML_IDXGROUP_BEGIN, buf);

            buf[0] = '\0';
            cnt = 0;

            for (struct topiclist *ptr = topichead; ptr; ptr = ptr->next) {
                firstletter = toupper(ptr->topic[0]);

                if (firstletter == alph || (!isalpha(alph) && !isalpha(firstletter))) {
                    cnt++;

                    escape_html(buf3, sizeof(buf3), ptr->topic);
                    fprintf(hf, HTML_IDXGROUP_ENTRY, /*(100 / cols), */ buf3, buf3);

                    if (cnt == cols) {
                        snprintf(buf2, sizeof(buf2), "%-.*s", width - 1, ptr->topic);
                    } else {
                        snprintf(buf2, sizeof(buf2), "%-*.*s", width, width - 1, ptr->topic);
                    }

                    strcat(buf, buf2);

                    if (cnt >= cols) {
                        fprintf(f, "  %s\n", buf);
                        buf[0] = '\0';
                        cnt = 0;
                    }
                }
            }

            if (cnt) {
                fprintf(f, "  %s\n", buf);
            }
            fprintf(hf, HTML_IDXGROUP_END);
        }
    }

    fprintf(f, " \nUse '%s <topicname>' to get more information on a topic.\n", doccmd);
}

/**
 * Stores section, topic, and summary information for the given helpfile.
 *
 * @private
 * @param infile the sourc helpfile
 * @return the string length of the longest topic
 */
static int
find_topics(FILE * infile)
{
    char buf[4096];
    char *s, *p;
    int longest, lng;

    /**
     * @TODO Remove longest, and change function to return void.
     */
    longest = 0;

    while (!feof(infile)) {
        do {
            if (!fgets(buf, sizeof(buf), infile)) {
                *buf = '\0';
                break;
            } else {
                if (!strncmp(buf, "~~section ", 10)) {
                    buf[strlen(buf) - 1] = '\0';
                    strcpyn(sect, sizeof(sect), (buf + 10));

                    add_section(sect);
                } else if (!strncmp(buf, "~~title ", 8)) {
                    buf[strlen(buf) - 1] = '\0';
                    title = strdup(buf + 8);
                } else if (!strncmp(buf, "~~author ", 9)) {
                    buf[strlen(buf) - 1] = '\0';
                    author = strdup(buf + 9);
                } else if (!strncmp(buf, "~~doccmd ", 9)) {
                    buf[strlen(buf) - 1] = '\0';
                    doccmd = strdup(buf + 9);
                }
            }
        } while (!feof(infile) && (*buf != '~' || buf[1] == '@' ||
                buf[1] == '~' || buf[1] == '<' || buf[1] == '!'));

        do {
            if (!fgets(buf, sizeof(buf), infile)) {
                *buf = '\0';
                break;
            } else {
                if (!strncmp(buf, "~~section ", 10)) {
                    buf[strlen(buf) - 1] = '\0';
                    strcpyn(sect, sizeof(sect), (buf + 10));

                    add_section(sect);
                } else if (!strncmp(buf, "~~title ", 8)) {
                    buf[strlen(buf) - 1] = '\0';
                    title = strdup(buf + 8);
                } else if (!strncmp(buf, "~~author ", 9)) {
                    buf[strlen(buf) - 1] = '\0';
                    author = strdup(buf + 9);
                } else if (!strncmp(buf, "~~doccmd ", 9)) {
                    buf[strlen(buf) - 1] = '\0';
                    doccmd = strdup(buf + 9);
                }
            }
        } while (*buf == '~' && !feof(infile));

        for (s = p = buf; *s; s++) {
            if (*s == '|' || *s == '\n') {
                *s++ = '\0';

                add_topic(p);

                lng = strlen(p);

                if (lng > longest)
                    longest = lng;

                break;
            }
        }
    }

    return (longest);
}

/**
 * Re-processes raw helpfile and creates text and html versions.
 *
 * @private
 * @param infile the raw helpfile
 * @param outfile text file to create
 * @param htmlfile html file to create
 */
static void
process_lines(FILE * infile, FILE * outfile, FILE * htmlfile)
{
    FILE *docsfile;
    char *sectptr;
    char buf[4096];
    char buf2[4096];
    char buf3[4096];
    char buf4[4096];
    int nukenext = 0;
    int istopichead = 0;
    int codeblock = 0;
    char *ptr;
    char *ptr2;
    char temp[BUFFER_LEN];

    docsfile = stdout;
    escape_html(buf, sizeof(buf), title);
    escape_html(buf2, sizeof(buf2), author);
    fprintf(htmlfile, HTML_PAGE_HEAD, buf, buf, buf2);

    snprintf(temp, sizeof(temp), "%s for %s", title, VERSION);

    fprintf(outfile, "%*s%s\n", (int) (36 - (strlen(temp) / 2)), "", temp);
    fprintf(outfile, "%*sby %s\n\n", (int) (36 - ((strlen(author) + 3) / 2)), "", author);
    fprintf(outfile,
            "You may get a listing of topics that you can get help on, either sorted\n");
    fprintf(outfile, "Alphabetically or sorted by Category.  To get these lists, type:\n");
    fprintf(outfile, "        %s alpha        or\n", doccmd);
    fprintf(outfile, "        %s category\n\n", doccmd);

    while (!feof(infile)) {
        if (!fgets(buf, sizeof(buf), infile)) {
            break;
        }

        if (buf[0] == '~') {
            if (buf[1] == '~') {
                if (!strncmp(buf, "~~file ", 7)) {
                    fclose(docsfile);
                    buf[strlen(buf) - 1] = '\0';

                    if (!(docsfile = fopen(buf + 7, "wb"))) {
                        fprintf(stderr, "Error: can't write to %s", buf + 7);
                        exit(1);
                    }

                    fprintf(docsfile, "%*s%s\n", (int) (36 - (strlen(title) / 2)), "", title);
                    fprintf(docsfile, "%*sby %s\n\n", (int) (36 - ((strlen(author) + 3) / 2)),
                            "", author);
                } else if (!strncmp(buf, "~~section ", 10)) {
                    buf[strlen(buf) - 1] = '\0';
                    sectptr = strchr(buf + 10, '|');

                    if (sectptr) {
                        *sectptr = '\0';
                    }

                    fprintf(outfile, "~\n~\n~%s\n", HRULE_TEXT);
                    fprintf(docsfile, "\n\n%s\n", HRULE_TEXT);
                    fprintf(docsfile, "%*s\n", (int) (38 + strlen(buf + 10) / 2), (buf + 10));

                    print_section_topics(outfile, htmlfile, (buf + 10));

                    fprintf(outfile, "~%s\n~\n~\n", HRULE_TEXT);
                    fprintf(docsfile, "%s\n\n\n", HRULE_TEXT);
                } else if (!strncmp(buf, "~~alsosee ", 10)) {
                    buf[strlen(buf) - 1] = '\0';

                    fprintf(htmlfile, HTML_ALSOSEE_BEGIN);
                    fprintf(outfile, "Also see: ");

                    ptr = buf + 10;
                    skip_whitespace((const char **)&ptr);

                    while (ptr && *ptr) {
                        ptr2 = ptr;
                        ptr = strchr(ptr, ',');

                        if (ptr) {
                            *ptr++ = '\0';
                            skip_whitespace((const char **)&ptr);
                        }

                        if (ptr2 > buf + 10) {
                            if (!ptr || !*ptr) {
                                fprintf(htmlfile, " and\n");
                                fprintf(outfile, " and ");
                            } else {
                                fprintf(htmlfile, ",\n");
                                fprintf(outfile, ", ");
                            }
                        }

                        escape_html(buf3, sizeof(buf3), ptr2);
                        strcpyn(buf4, sizeof(buf4), buf3);

                        for (char *ptr3 = buf4; *ptr3; ptr3++) {
                            *ptr3 = tolower(*ptr3);
                        }

                        fprintf(htmlfile, HTML_ALSOSEE_ENTRY, buf4, buf3);
                        fprintf(outfile, "%s", ptr2);
                    }

                    fprintf(htmlfile, HTML_ALSOSEE_END);
                    fprintf(outfile, "\n");
                } else if (!strcmp(buf, "~~code\n")) {
                    fprintf(htmlfile, HTML_CODEBEGIN);
                    codeblock = 1;
                } else if (!strcmp(buf, "~~endcode\n")) {
                    fprintf(htmlfile, HTML_CODEEND);
                    codeblock = 0;
                } else if (!strcmp(buf, "~~sectlist\n")) {
                    print_sections(outfile, htmlfile);
                } else if (!strcmp(buf, "~~secttopics\n")) {
                    /* print_all_section_topics(outfile, htmlfile); */
                } else if (!strcmp(buf, "~~index\n")) {
                    print_topics(outfile, htmlfile);
                }
            } else if (buf[1] == '!') {
                fprintf(outfile, "%s", buf + 2);
            } else if (buf[1] == '@') {
                escape_html(buf3, sizeof(buf3), buf + 2);
                fprintf(htmlfile, "%s", buf3);
            } else if (buf[1] == '<') {
                fprintf(outfile, "%s", buf + 2);
                fprintf(docsfile, "%s", buf + 2);
            } else if (buf[1] == '#') {
                fprintf(outfile, "~%s", buf + 2);
                fprintf(docsfile, "%s", buf + 2);
            } else {
                if (!nukenext) {
                    fprintf(htmlfile, HTML_TOPICEND);
                }

                nukenext = 1;

                fprintf(outfile, "%s", buf);
                fprintf(docsfile, "%s", buf + 1);
                escape_html(buf3, sizeof(buf3), buf + 1);
                fprintf(htmlfile, "%s", buf3);
            }
        } else if (nukenext) {
            nukenext = 0;
            istopichead = 1;
            fprintf(outfile, "%s", buf);

            for (ptr = buf; *ptr && *ptr != '|' && *ptr != '\n'; ptr++) {
                *ptr = tolower(*ptr);
            }

            *ptr = '\0';
            escape_html(buf3, sizeof(buf3), buf);
            fprintf(htmlfile, HTML_TOPICHEAD, buf3);
        } else if (buf[0] == ' ') {
            nukenext = 0;

            if (istopichead) {
                istopichead = 0;
                fprintf(htmlfile, HTML_TOPICBODY);
            } else if (!codeblock) {
                fprintf(htmlfile, HTML_PARAGRAPH);
            }

            fprintf(outfile, "%s", buf);
            fprintf(docsfile, "%s", buf);
            escape_html(buf3, sizeof(buf3), buf);
            fprintf(htmlfile, "%s", buf3);
        } else {
            int found = 0;

            for (replacement *rtemp = replacements; rtemp->token; rtemp++) {
                if (strstr(buf, rtemp->token)) {
                    found = 1;
                    (rtemp->func)(outfile);
                    (rtemp->func)(docsfile);
                    (rtemp->func)(htmlfile);
                    break;
                }
            }

            if (!found) {
                fprintf(outfile, "%s", buf);
                fprintf(docsfile, "%s", buf);
                escape_html(buf3, sizeof(buf3), buf);
                fprintf(htmlfile, "%s", buf3);
            }

            if (istopichead) {
                fprintf(htmlfile, HTML_TOPICHEAD_BREAK);
            }
        }
    }
    fprintf(htmlfile, HTML_PAGE_FOOT);
    fclose(docsfile);
}

/**
 * This is the main function for the helpfile converter.
 */
int
main(int argc, char **argv)
{
    FILE *infile, *outfile, *htmlfile;

    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s inputrawfile outputhelpfile outputhtmlfile\n",
                argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], argv[2])) {
        fprintf(stderr,
                "%s: cannot use same file for input rawfile and output helpfile\n",
                argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], argv[3])) {
        fprintf(stderr,
                "%s: cannot use same file for input rawfile and output htmlfile\n",
                argv[0]);
        return 1;
    }

    if (!strcmp(argv[3], argv[2])) {
        fprintf(stderr, "%s: cannot use same file for htmlfile and helpfile\n", argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], "-")) {
        infile = stdin;
    } else {
        if (!(infile = fopen(argv[1], "rb"))) {
            fprintf(stderr, "%s: cannot read %s\n", argv[0], argv[1]);
            return 1;
        }
    }

    if (!(outfile = fopen(argv[2], "wb"))) {
        fprintf(stderr, "%s: cannot write to %s\n", argv[0], argv[2]);
        return 1;
    }

    if (!(htmlfile = fopen(argv[3], "wb"))) {
        fprintf(stderr, "%s: cannot write to %s\n", argv[0], argv[3]);
        return 1;
    }

    find_topics(infile);
    fseek(infile, 0L, SEEK_SET);

    process_lines(infile, outfile, htmlfile);

    fclose(infile);
    fclose(outfile);
    fclose(htmlfile);

    return 0;
}
