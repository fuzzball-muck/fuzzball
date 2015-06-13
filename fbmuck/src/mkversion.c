/*
* Copyright (c) 2015 by Fuzzball Software
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
#ifndef WIN32
#include <dirent.h>
#endif
#include "config.h"
#include "sha1.h"

/* Define if you want the timezone to show UTC. Windows doesn't do
   abbreviations for timezone, so this is the only way to get it shortened. */
#undef USE_UTC

/* Maximum size of a line in the template and version files */
#define LINE_SIZE 1024

/* Strings to search for in tpl/version */
#define GENLINE "$generation"
#define CRELINE "$creation"
#define GITAVAIL "$gitavail"
#define GENDEF  "#define generation"
#define HASHARRAY "$hasharray"
#define HASH_ARRAY_TPL "\t{ \"%s\", \"%s\", \"%s\" },\n"
#define DEF_GITYES "#define GIT_AVAILABLE"
#define DEF_GITNO  "#undef GIT_AVAILABLE"

/* Filenames & paths */
#define TPL_FILE "version.tpl"
#define VERSION_FILE "version.c"
#define DIR_SOURCE "."
#define DIR_INCLUDE "../include"
#define EXT_SRC_FILE ".c"
#define EXT_INC_FILE ".h"

/* Git commands */
#define GIT_CHECK_CMD "git --version 2>&1"
#define GIT_BLOB_CMD "git hash-object %s 2>&1"

// Prototypes
unsigned int getgeneration();
void getnow(char *timestr, size_t len);
int test_git();
void print_hash_array(FILE *out);
char *get_git_hash(const char *name, char *hash, size_t hash_len);
char *get_sha1_hash(const char *name, char *hash, size_t hash_len);
int endswith(const char *str, const char *suf);
int qcmp(const void *arg1, const void *arg2);

int git_available = 0;

typedef struct file_entry {
	char *filepath;
	char *filename;
} file_entry;

int main(int argc, char* argv[])
{
	unsigned int generation = 0;
	FILE *ver, *tpl;
	char line[LINE_SIZE], creation[LINE_SIZE];
	char *pos;

	generation = getgeneration();
	getnow(creation, LINE_SIZE);
	git_available = test_git();

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

		/* fgets gives us \n, and we don't want them */
		pos = line;
		while (*pos) {
			if (*pos == '\r' || *pos == '\n')
				*pos = '\0';
			pos++;
		}

		if ((pos = strstr(line, GENLINE))) {
			*pos = '\0';
			fprintf(ver, "%s%d%s\n", line, generation, pos + strlen(GENLINE));
		} else if ((pos = strstr(line, CRELINE))) {
			*pos = '\0';
			fprintf(ver, "%s%s%s\n", line, creation, pos + strlen(CRELINE));
		} else if ((pos = strstr(line, HASHARRAY))) {
			print_hash_array(ver);
		} else if ((pos = strstr(line, GITAVAIL))) {
			fprintf(ver, "%s\n", (git_available ? DEF_GITYES : DEF_GITNO));
		} else {
			fprintf(ver, "%s\n", line);
		}
	}

	fclose(ver);
	fclose(tpl);

	return 0;
}

// Return a time in format of "Mon Apr 14 2008 at 12:01:17 EDT"
void getnow(char *timestr, size_t len) {
#ifdef WIN32
	time_t t = time(NULL);
	struct tm info;

#ifdef USE_UTC
	gmtime_s(&info, &t);
	strftime(timestr, len, "%a %b %d %Y at %H:%M:%S UTC", &info);
#else
	localtime_s(&info, &t);
	strftime(timestr, len, "%a %b %d %Y at %H:%M:%S %Z", &info);
#endif
#else /* !WIN32 */
	time_t t = time(NULL);
	struct tm *info;
#ifdef USE_UTC
	info = gmtime(&t);
	strftime(timestr, len, "%a %b %d %Y at %H:%M:%S UTC", info);
#else
	info = localtime(&t);
	strftime(timestr, len, "%a %b %d %Y at %H:%M:%S %Z", info);
#endif
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

int test_git() {
	FILE *gitpipe;
	char line[LINE_SIZE];

	if (!(gitpipe = popen(GIT_CHECK_CMD, "r"))) {
		printf("Error opening git command");
		return 0;
	}

	/* Read until eof */
	while (fgets(line, LINE_SIZE, gitpipe));

	if (pclose(gitpipe) == 0)
		return 1;

	return 0;
}

void print_hash_array(FILE *out) {
	char *filename = NULL;
	file_entry **files = NULL, **temp = NULL;
	size_t filecount = 0, i;
#ifdef WIN32
	HANDLE  hFind;
	BOOL    bMore;
	WIN32_FIND_DATA finddata;
	char githash[LINE_SIZE];
	char shahash[LINE_SIZE];

	/* C files in src dir */
	hFind = FindFirstFile(DIR_SOURCE "/*" EXT_SRC_FILE, &finddata);
	bMore = (hFind != (HANDLE)-1);

	while (bMore) {
		if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			&& strcmp(finddata.cFileName, VERSION_FILE)) {
			filename = finddata.cFileName;
#else
	DIR *dir;
	struct dirent *file;
	char shahash[LINE_SIZE];
	char githash[LINE_SIZE];

	/* If we can't open the source directory, quit */
	if (!(dir = opendir(DIR_SOURCE)))
		return;

	while ((file = readdir(dir))) {
		if (endswith(file->d_name, EXT_SRC_FILE) && strcmp(file->d_name, VERSION_FILE)) {
			filename = file->d_name;
#endif
			filecount++;
			/* Copy over the array, if it exists */
			if (files) {
				temp = (file_entry **) malloc(sizeof(file_entry *) * (filecount));
				memcpy(temp, files, (filecount - 1) * sizeof(file_entry *));
				free(files);
				files = temp;
			} else {
				files = (file_entry **) malloc(sizeof(file_entry *) * (filecount));
			}
			
			files[filecount - 1] = (file_entry *) malloc(sizeof(file_entry));
			i = strlen(filename) + strlen(DIR_SOURCE) + strlen("/");
			files[filecount - 1]->filepath = (char *) malloc(sizeof(char) * (i + 1));
			snprintf(files[filecount - 1]->filepath, i + 1, "%s/%s", DIR_SOURCE, filename);
			i = strlen(filename);
			files[filecount - 1]->filename = (char *) malloc(sizeof(char) * (i + 1));
			snprintf(files[filecount - 1]->filename, i + 1, "%s", filename);
#ifdef WIN32
		}
		bMore = FindNextFile(hFind, &finddata);
	}
	FindClose(hFind);
#else
		}
	}
	closedir(dir);
#endif


#ifdef WIN32
	/* H files in include dir */
	hFind = FindFirstFile(DIR_INCLUDE "/*" EXT_INC_FILE, &finddata);
	bMore = (hFind != (HANDLE)-1);

	while (bMore) {
		if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			&& strcmp(finddata.cFileName, VERSION_FILE)) {
			filename = finddata.cFileName;
#else

	/* If we can't open the source directory, quit */
	if (!(dir = opendir(DIR_INCLUDE)))
		return;

	while ((file = readdir(dir))) {
		if (endswith(file->d_name, EXT_INC_FILE) && strcmp(file->d_name, VERSION_FILE)) {
			filename = file->d_name;
#endif
			filecount++;
			/* Copy over the array, if it exists */
			if (files) {
				temp = (file_entry **) malloc(sizeof(file_entry *) * (filecount));
				memcpy(temp, files, (filecount - 1) * sizeof(file_entry *));
				free(files);
				files = temp;
			} else {
				files = (file_entry **) malloc(sizeof(file_entry *) * (filecount));
			}
			
			files[filecount - 1] = (file_entry *) malloc(sizeof(file_entry));
			i = strlen(filename) + strlen(DIR_INCLUDE) + strlen("/");
			files[filecount - 1]->filepath = (char *) malloc(sizeof(char) * (i + 1));
			snprintf(files[filecount - 1]->filepath, i + 1, "%s/%s", DIR_INCLUDE, filename);
			i = strlen(filename);
			files[filecount - 1]->filename = (char *) malloc(sizeof(char) * (i + 1));
			snprintf(files[filecount - 1]->filename, i + 1, "%s", filename);
#ifdef WIN32
		}
		bMore = FindNextFile(hFind, &finddata);
	}
	FindClose(hFind);
#else
		}
	}
	closedir(dir);
#endif

	/* Sort the array */
	qsort((void *)files, filecount, sizeof(file_entry *), qcmp);

	/* Output the array to the file */
	for (i = 0; i < filecount; i++) {
		fprintf(
			out,
			HASH_ARRAY_TPL,
			files[i]->filename,
			get_git_hash(files[i]->filepath, githash, LINE_SIZE),
			get_sha1_hash(files[i]->filepath, shahash, LINE_SIZE)
		);
		free(files[i]->filename);
		free(files[i]->filepath);
		free(files[i]);
	}
	free(files);
}

int qcmp( const void *arg1, const void *arg2 ) {
   return strcasecmp( (*(file_entry**)arg1)->filename, (*(file_entry**)arg2)->filename );
}


char *get_git_hash(const char *name, char *hash, size_t hash_len) {
	FILE *gitpipe;
	char line[LINE_SIZE];
	char *ptr;

	hash[0] = '\0';

	if (!git_available)
		return hash;

	snprintf(line, LINE_SIZE, GIT_BLOB_CMD, name);
	if (!(gitpipe = popen(line, "r")))
		return hash;

	/* Get a line of output */
	if (!fgets(line, LINE_SIZE, gitpipe)) {
		pclose(gitpipe);
		return hash;
	}

	/* Close the pipe and check return status */
	if (pclose(gitpipe) != 0) {
		return hash;
	}

	/* Success! Copy the hash and return */
	strncpy(hash, line, hash_len - 1);
	hash[hash_len - 1] = '\0';

	/* Strip out the newlines and such */
	ptr = hash;
	while (*ptr) {
		if (*ptr == '\r' || *ptr == '\n')
			*ptr = '\0';
		ptr++;
	}

	return hash;
}

char *get_sha1_hash(const char *name, char *hash, size_t hash_len) {
	FILE *f;
	int c;
	hash[0] = '\0';

	if (!(f = fopen(name, "rb")))
		return hash;

	sha1nfo info;
	sha1_init(&info);
	while ((c = fgetc(f)) != EOF) {
		sha1_writebyte(&info, (uint8_t)c);
	}
	hash2hex(sha1_result(&info), hash, hash_len);
	fclose(f);

	return hash;
}

int endswith(const char *str, const char *suf) {
	size_t str_len, suf_len;
	const char *ptr;

	if (!str || !suf)
		return 0;

	str_len = strlen(str);
	suf_len = strlen(suf);

	if (suf_len > str_len)
		return 0;

	ptr = str + str_len - suf_len;
	return (0 == strcmp(ptr, suf) ? 1 : 0);
}
