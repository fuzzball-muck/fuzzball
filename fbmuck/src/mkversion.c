/*
* Copyright (c) 2014 by Fuzzball Software
* under the GNU Public License
*
* This program reads in version.tpl and creates a version.c(pp) file as output.
* It scans for $generation and $creation tags in the source and replaces them
* provided they are on DIFFERENT lines. The matcher below only does one match
* per line as C has no native growing string functionality. It could be
* rewritten to use malloc and unlimited line lengths, but it seems a bit
* overkill for such a simple substituion.
*/

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "config.h"

/* Define if you want the timezone to show UTC. Windows doesn't do
   abbreviations for timezone, so this is the only way to get it shortened. */
#undef USE_UTC

/* Maximum size of a line in the template and version files */
#define LINE_SIZE 1024

#define GENLINE "$generation"
#define CRELINE "$creation"
#define GENDEF  "#define generation"

// Filenames
#define TPL_FILE "version.tpl"
#ifdef WIN32
#define VERSION_FILE "version.c"
#else
#define VERSION_FILE "version.c"
#endif

// Prototypes
unsigned int getgeneration();
void getnow(char *timestr, size_t len);

int main(int argc, char* argv[])
{
	unsigned int generation = 0;
	FILE *ver, *tpl;
	char line[LINE_SIZE], creation[LINE_SIZE];
	char *pos;

	generation = getgeneration();
	getnow(creation, LINE_SIZE);

	/* Read this like it were linux with rb */
	if (!(tpl = fopen(TPL_FILE, "rb"))) {
		fprintf(stderr, "error: could not open %s\n", TPL_FILE);
		return -1;
	}

	/* Specify wb so windows doesn't add \r */
	if (!(ver = fopen(VERSION_FILE, "wb"))) {
		fclose(tpl);
		fprintf(stderr, "error: could not open %s\n", VERSION_FILE);
		return -1;
	}

	while (!feof(tpl)) {
		if (!fgets(line, LINE_SIZE, tpl))
			break;

		if (pos = strstr(line, GENLINE)) {
			*pos = '\0';
			fprintf(ver, "%s%d%s\n", line, generation, pos + strlen(GENLINE));
		} else if (pos = strstr(line, CRELINE)) {
			*pos = '\0';
			fprintf(ver, "%s%s%s\n", line, creation, pos + strlen(CRELINE));
		} else {
			fprintf(ver, "%s\n", line);
		}
	}

	// TODO: Generate the do_showextver function instead of using TPL
	fclose(ver);
	fclose(tpl);

	return 0;
}

// Return a time in format of "Mon Apr 14 2008 at 12:01:17 EDT"
void getnow(char *timestr, size_t len) {
	time_t t = time(NULL);
	struct tm info;

#ifdef USE_UTC
	gmtime_s(&info, &t);
	strftime(timestr, len, "%a %b %d %Y at %H:%M:%S UTC", &info);
#else
	localtime_s(&info, &t);
	strftime(timestr, len, "%a %b %d %Y at %H:%M:%S %Z", &info);
#endif
}

// Get the generation value from the existing cpp file.
unsigned int getgeneration() {
	char line[LINE_SIZE];
	char *pos, *number;
	FILE *v;

	if (!(v = fopen(VERSION_FILE, "rb")))
		return 0;

	while (!feof(v)) {
		if (!fgets(line, LINE_SIZE, v))
			break;

		if (!(pos = strstr(line, GENDEF))) 
			continue;
		
		/* Skip over the text */
		pos += strlen(GENDEF);

		/* Find the start of the digit */
		while (*pos && !isdigit(*pos))
			pos++;

		/* Find the end of the digit */
		number = pos;
		while (*pos && isdigit(*pos))
			pos++;
		*pos = '\0';

		fclose(v);
		return atoi(number) + 1;
	}

	fclose(v);
	return 0;
}

