#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define HRULE_TEXT "----------------------------------------------------------------------------"

#define HTML_PAGE_HEAD     	"<html><head><title>%s</title></head>\n<body><div align=\"center\"><h1>%s</h1>\n<h3>by %s</h3></div>\n<ul><li><a href=\"#AlphaList\">Alphabetical List of Topics</a></li>\n<li><a href=\"#SectList\">List of Topics by Category</a></li></ul>\n"
#define HTML_PAGE_FOOT     	"</body></html>\n"

#define HTML_SECTION     	"<p><hr size=6>\n<h3><a name=\"%s\">%s</a></h3>\n"
#define HTML_SECTLIST_HEAD  "<p><hr size=\"6\"><h3><a name=\"SectList\">List of Topics by Category</a></h3>\n<h4>You can get more help on the following topics:</h4>\n<ul>"
#define HTML_SECTLIST_ENTRY "  <li><a href=\"#%s\">%s</a></li>\n"
#define HTML_SECTLIST_FOOT  "</ul>\n\n"

#define HTML_SECTIDX_BEGIN	"<blockquote><table border=0>\n  <tr>\n"
#define HTML_SECTIDX_ENTRY	"    <td width=\"%d%%\"> &nbsp; <a href=\"#%s\">%s</a> &nbsp; </td>\n"
#define HTML_SECTIDX_NEWROW	"  </tr>\n  <tr>\n"
#define HTML_SECTIDX_END	"  </tr>\n</table></blockquote>\n\n"

#define HTML_INDEX_BEGIN	"<p><hr size=\"6\"><h3><a name=\"AlphaList\">Alphabetical List of Topics</a></h3>\n"
#define HTML_IDXGROUP_BEGIN	"<h4>%s</h4><blockquote><table border=0>\n  <tr>\n"
#define HTML_IDXGROUP_ENTRY	"    <td nowrap> &nbsp; <a href=\"#%s\">%s</a> &nbsp; </td>\n"
#define HTML_IDXGROUP_NEWROW	"  </tr>\n  <tr>\n"
#define HTML_IDXGROUP_END		"  </tr>\n</table></blockquote>\n\n"
#define HTML_INDEX_END		""

#define HTML_TOPICHEAD		"<hr><h4><a name=\"%s\">"
#define HTML_TOPICHEAD_BREAK	"<br>\n"
#define HTML_TOPICBODY		"</a></h4>\n"
#define HTML_TOPICEND		"<p>\n"
#define HTML_PARAGRAPH		"<p>\n"

#define HTML_CODEBEGIN		"<pre>\n"
#define HTML_CODEEND		"</pre>\n"

#define HTML_ALSOSEE_BEGIN      "<p><h5>Also see:\n"
#define HTML_ALSOSEE_ENTRY      "    <a href=\"#%s\">%s</a>"
#define HTML_ALSOSEE_END        "\n</h5>\n"


const char* title = "";
const char* author = "";
const char* doccmd = "";

char *
string_dup(const char *s)
{
	char *p;

	p = (char *) malloc(strlen(s) + 1);
	if (p)
		strcpy(p, s);  /* Guaranteed enough space. */
	return p;
}

/* from stringutil.c */
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



char sect[256] = "";

struct topiclist {
	struct topiclist *next;
	char *topic;
	char *section;
	int printed;
};

struct topiclist *topichead;
struct topiclist *secthead;


void
add_section(const char *str)
{
	struct topiclist *ptr, *top;

	if (!str || !*str)
		return;
	top = (struct topiclist *) malloc(sizeof(struct topiclist));

	top->topic = NULL;
	top->section = (char *) string_dup(sect);
	top->printed = 0;
	top->next = NULL;

	if (!secthead) {
		secthead = top;
		return;
	}
	for (ptr = secthead; ptr->next; ptr = ptr->next) ;
	ptr->next = top;
}

void
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

	top = (struct topiclist *) malloc(sizeof(struct topiclist));

	top->topic = (char *) string_dup(buf);
	top->section = (char *) string_dup(sect);
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

char*
escape_html(char* buf, int buflen, const char* in)
{
	char* out = buf;
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

void
print_section_topics(FILE * f, FILE * hf, const char *whichsect)
{
	struct topiclist *ptr;
	struct topiclist *sptr;
	char sectname[256];
	char *sectptr;
	char *osectptr;
	char *divpos;
	char buf[256];
	char buf2[256];
	char buf3[256];
	int cnt;
	int width;
	int hcol;
	int cols;
	int longest;
	char *currsect;

	longest = 0;
	for (sptr = secthead; sptr; sptr = sptr->next) {
		if (!strncasecmp(whichsect, sptr->section, strlen(whichsect))) {
			currsect = sptr->section;
			for (ptr = topichead; ptr; ptr = ptr->next) {
				if (!strcasecmp(currsect, ptr->section)) {
					divpos = index(ptr->topic, '|');
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
	for (sptr = secthead; sptr; sptr = sptr->next) {
		if (!strncasecmp(whichsect, sptr->section, strlen(whichsect))) {
			currsect = sptr->section;
			cnt = 0;
			hcol = 0;
			buf[0] = '\0';
			strcpyn(sectname, sizeof(sectname), currsect);
			sectptr = index(sectname, '|');
			if (sectptr) {
				*sectptr++ = '\0';
				osectptr = sectptr;
				sectptr = rindex(sectptr, '|');
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

			fprintf(hf, HTML_SECTION, escape_html(buf2, sizeof(buf2), sectptr), escape_html(buf3, sizeof(buf3), sectname));
			fprintf(f, "~\n~\n%s\n%s\n\n", currsect, sectname);
			fprintf(hf, HTML_SECTIDX_BEGIN);
			for (ptr = topichead; ptr; ptr = ptr->next) {
				if (!strcasecmp(currsect, ptr->section)) {
					ptr->printed++;
					cnt++;
					hcol++;
					if (hcol > cols) {
						fprintf(hf, HTML_SECTIDX_NEWROW);
						hcol = 1;
					}
					escape_html(buf3, sizeof(buf3), ptr->topic);
					fprintf(hf, HTML_SECTIDX_ENTRY, (100 / cols), buf3, buf3);
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

void
print_all_section_topics(FILE * f, FILE * hf)
{
	struct topiclist *sptr;

	for (sptr = secthead; sptr; sptr = sptr->next) {
		print_section_topics(f, hf, sptr->section);
	}
}

void
print_sections(FILE * f, FILE * hf, int cols)
{
	struct topiclist *ptr;
	struct topiclist *sptr;
	char sectname[256];
	char *osectptr;
	char *sectptr;
	char buf[256];
	char buf2[256];
	char buf3[256];
	char buf4[256];
	int cnt;
	int width;
	int hcol;
	char *currsect;

	fprintf(f, "~\n");
	fprintf(f, "~%s\n", HRULE_TEXT);
	fprintf(f, "~\n");
	fprintf(f, "CATEGORY|CATEGORIES|TOPICS|SECTIONS\n");
	fprintf(f, "                   List of Topics by Category:\n \n");
	fprintf(f, "You can get more help on the following topics:\n \n");
	fprintf(hf, HTML_SECTLIST_HEAD);
	if (cols < 1) {
		cols = 1;
	}
	width = 78 / cols;
	for (sptr = secthead; sptr; sptr = sptr->next) {
		currsect = sptr->section;
		cnt = 0;
		hcol = 0;
		buf[0] = '\0';
		strcpyn(sectname, sizeof(sectname), currsect);
		sectptr = index(sectname, '|');
		if (sectptr) {
			*sectptr++ = '\0';
			osectptr = sectptr;
			sectptr = rindex(sectptr, '|');
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

		fprintf(hf, HTML_SECTLIST_ENTRY, escape_html(buf3, sizeof(buf3), sectptr), escape_html(buf4, sizeof(buf4), sectname));
		fprintf(f, "  %-40s (%s)\n", sectname, sectptr);
	}
	fprintf(hf, HTML_SECTLIST_FOOT);
	fprintf(f, " \nUse '%s <topicname>' to get more information on a topic.\n", doccmd);
}

void
print_topics(FILE * f, FILE * hf)
{
	struct topiclist *ptr;
	char buf[256];
	char buf2[256];
	char buf3[256];
	char alph;
	char firstletter;
	char *divpos;
	int cnt = 0;
	int width;
	int hcol = 0;
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
	for (alph = 'A' - 1; alph <= 'Z'; alph++) {
		cnt = 0;
		longest = 0;
		for (ptr = topichead; ptr; ptr = ptr->next) {
			firstletter = toupper(ptr->topic[0]);
			if (firstletter == alph || (!isalpha(alph) && !isalpha(firstletter))) {
				cnt++;
				divpos = index(ptr->topic, '|');
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
			hcol = 0;
			for (ptr = topichead; ptr; ptr = ptr->next) {
				firstletter = toupper(ptr->topic[0]);
				if (firstletter == alph || (!isalpha(alph) && !isalpha(firstletter))) {
					cnt++;
					hcol++;
					if (hcol > cols) {
						fprintf(hf, HTML_IDXGROUP_NEWROW);
						hcol = 1;
					}
					escape_html(buf3, sizeof(buf3), ptr->topic);
					fprintf(hf, HTML_IDXGROUP_ENTRY, /*(100 / cols),*/ buf3, buf3);
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
	fprintf(hf, "%s", HTML_INDEX_END);
	fprintf(f, " \nUse '%s <topicname>' to get more information on a topic.\n", doccmd);
}

int
find_topics(FILE * infile)
{
	char buf[4096];
	char *s, *p;
	int longest, lng;

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
					title = string_dup(buf+8);
				} else if (!strncmp(buf, "~~author ", 9)) {
					buf[strlen(buf) - 1] = '\0';
					author = string_dup(buf+9);
				} else if (!strncmp(buf, "~~doccmd ", 9)) {
					buf[strlen(buf) - 1] = '\0';
					doccmd = string_dup(buf+9);
				}
			}
		} while (!feof(infile) &&
				 (*buf != '~' || buf[1] == '@' || buf[1] == '~' || buf[1] == '<' ||
				  buf[1] == '!'));

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
					title = string_dup(buf+8);
				} else if (!strncmp(buf, "~~author ", 9)) {
					buf[strlen(buf) - 1] = '\0';
					author = string_dup(buf+9);
				} else if (!strncmp(buf, "~~doccmd ", 9)) {
					buf[strlen(buf) - 1] = '\0';
					doccmd = string_dup(buf+9);
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
				p = s;
				break;
			}
		}
	}
	return (longest);
}


void
process_lines(FILE * infile, FILE * outfile, FILE * htmlfile, int cols)
{
	FILE *docsfile;
	char *sectptr;
	char buf[4096];
	char buf2[4096];
	char buf3[4096];
	char buf4[4096];
	int nukenext = 0;
	int topichead = 0;
	int codeblock = 0;
	char *ptr;
	char *ptr2;
	char *ptr3;

	docsfile = stdout;
	escape_html(buf, sizeof(buf), title);
	escape_html(buf2, sizeof(buf2), author);
	fprintf(htmlfile, HTML_PAGE_HEAD, buf, buf, buf2);

	fprintf(outfile, "%*s%s\n", (int)(36-(strlen(title)/2)), "", title);
	fprintf(outfile, "%*sby %s\n\n", (int)(36-((strlen(author)+3)/2)), "", author);
	fprintf(outfile, "You may get a listing of topics that you can get help on, either sorted\n");
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
					if (!(docsfile = fopen(buf + 7, "wb")))
					{
						fprintf(stderr, "Error: can't write to %s", buf + 7);
						exit(1);
					}
					fprintf(docsfile,  "%*s%s\n", (int)(36-(strlen(title)/2)), "", title);
					fprintf(docsfile,  "%*sby %s\n\n", (int)(36-((strlen(author)+3)/2)), "", author);
				} else if (!strncmp(buf, "~~section ", 10)) {
					buf[strlen(buf) - 1] = '\0';
					sectptr = index(buf + 10, '|');
					if (sectptr) {
						*sectptr = '\0';
					}
					fprintf(outfile, "~\n~\n~%s\n", HRULE_TEXT);
					fprintf(docsfile, "\n\n%s\n", HRULE_TEXT);
					fprintf(docsfile, "%*s\n", (int)(38 + strlen(buf + 10) / 2), (buf + 10));
					print_section_topics(outfile, htmlfile, (buf + 10));
					fprintf(outfile, "~%s\n~\n~\n", HRULE_TEXT);
					fprintf(docsfile, "%s\n\n\n", HRULE_TEXT);
				} else if (!strncmp(buf, "~~alsosee ", 10)) {
					buf[strlen(buf) - 1] = '\0';
					fprintf(htmlfile, HTML_ALSOSEE_BEGIN);
					fprintf(outfile, "Also see: ");
					ptr = buf + 10;
					while (*ptr && isspace(*ptr))
						ptr++;
					while (ptr && *ptr) {
						ptr2 = ptr;
						ptr = index(ptr, ',');
						if (ptr) {
							*ptr++ = '\0';
							while (*ptr && isspace(*ptr))
								ptr++;
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
						for (ptr3 = buf4; *ptr3; ptr3++) {
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
					print_sections(outfile, htmlfile, cols);
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
			topichead = 1;
			fprintf(outfile, "%s", buf);
			for (ptr = buf; *ptr && *ptr != '|' && *ptr != '\n'; ptr++) {
				*ptr = tolower(*ptr);
			}
			*ptr = '\0';
			escape_html(buf3, sizeof(buf3), buf);
			fprintf(htmlfile, HTML_TOPICHEAD, buf3);
		} else if (buf[0] == ' ') {
			nukenext = 0;
			if (topichead) {
				topichead = 0;
				fprintf(htmlfile, HTML_TOPICBODY);
			} else if (!codeblock) {
				fprintf(htmlfile, HTML_PARAGRAPH);
			}
			fprintf(outfile, "%s", buf);
			fprintf(docsfile, "%s", buf);
			escape_html(buf3, sizeof(buf3), buf);
			fprintf(htmlfile, "%s", buf3);
		} else {
			fprintf(outfile, "%s", buf);
			fprintf(docsfile, "%s", buf);
			escape_html(buf3, sizeof(buf3), buf);
			fprintf(htmlfile, "%s", buf3);
			if (topichead) {
				fprintf(htmlfile, HTML_TOPICHEAD_BREAK);
			}
		}
	}
	fprintf(htmlfile, HTML_PAGE_FOOT);
	fclose(docsfile);
}


int
main(int argc, char **argv)
{
	FILE *infile, *outfile, *htmlfile;
	int cols;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s inputrawfile outputhelpfile outputhtmlfile\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], argv[2])) {
		fprintf(stderr, "%s: cannot use same file for input rawfile and output helpfile\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], argv[3])) {
		fprintf(stderr, "%s: cannot use same file for input rawfile and output htmlfile\n", argv[0]);
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

	cols = 78 / (find_topics(infile) + 1);
	fseek(infile, 0L, 0);

	process_lines(infile, outfile, htmlfile, cols);

	fclose(infile);
	fclose(outfile);
	fclose(htmlfile);
	return 0;
}
