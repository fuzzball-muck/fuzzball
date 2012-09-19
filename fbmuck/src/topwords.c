
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "externs.h"
#include "db.h"

#define WORD_HASH_SIZE (100000)
#define SIZE_HASH_SIZE (100000)

struct queue_node {
	struct queue_node *next;
	struct queue_node *prev;
	int count;
	int spcount;
	int val;
	int len;
	char word[32];
};

struct queue_node *head = NULL;
struct queue_node *tail = NULL;

int total_words = 0;
int counted_words = 0;
hash_tab wordhash[WORD_HASH_SIZE];

struct queue_node *sizehash[100000];


/*
 * Like strncpy, except it guarentees null termination of the result string.
 * It also has a more sensible argument ordering.
 */
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


int
notify(int player, const char *msg)
{
	return printf("%s\n", msg);
}


int
string_compare(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

#ifndef MALLOC_PROFILING
char *
string_dup(const char *s)
{
	char *p = (char *) malloc(strlen(s) + 1);

	if (p)
	    strcpy(p, s);  /* Guaranteed enough space. */
	return p;
}
#endif


void
queue_remove_node(struct queue_node *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		head = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	} else {
		tail = node->prev;
	}
}

void
queue_insert_after(struct queue_node *where, struct queue_node *node)
{
	if (where) {
		node->next = where->next;
		where->next = node;
	} else {
		node->next = head;
		head = node;
	}
	node->prev = where;
	if (node->next) {
		node->next->prev = node;
	} else {
		tail = node;
	}
}


void
queue_promote(struct queue_node *node)
{
	struct queue_node *prev;
	struct queue_node *p;
	int oldval;

	oldval = node->val;
	node->val = (node->count * (node->len - 2)) + (node->spcount * (node->len - 1));

	/* remove node from sizehash table if necessary */
	if (oldval && oldval < SIZE_HASH_SIZE) {
		p = sizehash[oldval];
		if (p) {
			if (p == node) {
				if (node->prev) {
					if (node->prev->val == oldval) {
						sizehash[oldval] = node->prev;
					} else {
						sizehash[oldval] = NULL;
					}
				} else {
					sizehash[oldval] = NULL;
				}
			}
		}
	}

	if (node->val < SIZE_HASH_SIZE) {
		int limit;
		int i;

		limit = 16;
		i = node->val;
		do {
			prev = sizehash[i];
		} while (!prev && ++i < SIZE_HASH_SIZE && --limit);
		if (i >= SIZE_HASH_SIZE || !limit) {
			prev = node->prev;
			while (prev && prev->val < node->val) {
				prev = prev->prev;
			}
		}
	} else {
		prev = node->prev;
		while (prev && prev->val < node->val) {
			prev = prev->prev;
		}
	}

	if (prev != node->prev) {
		queue_remove_node(node);
		queue_insert_after(prev, node);
	}

	/* put entry in size hash */
	if (node->val < SIZE_HASH_SIZE) {
		sizehash[node->val] = node;
	}
}


void
list_top_4k_words(void)
{
	int count = 4096;
	int lastval;
	struct queue_node *node;

	lastval = 0x3fffffff;
	for (node = head; node && count--; node = node->next) {
		/* printf("%-32s %5d %5d\n", node->word, node->count, node->spcount); */
		printf("%s\n", node->word);
		fflush(stdout);
		if (lastval < node->val)
			abort();
		lastval = node->val;
	}
}

struct queue_node *
queue_add_node(const char *word, int pri)
{
	struct queue_node *nu;
	hash_data hd;

	nu = (struct queue_node *) malloc(sizeof(struct queue_node));

	if (!nu)
		abort();

	if (!head) {
		head = nu;
		nu->prev = NULL;
	}
	nu->next = NULL;
	nu->prev = tail;
	if (tail) {
		tail->next = nu;
	}
	tail = nu;
	nu->count = 0;
	nu->spcount = 0;
	strcpyn(nu->word, sizeof(nu->word), word);
	nu->len = strlen(nu->word);
	if (nu->word[nu->len - 1] == ' ') {
		nu->word[nu->len - 1] = '\0';
		nu->len--;
		nu->spcount += pri;
	} else {
		nu->count += pri;
	}
	nu->val = 0;
	queue_promote(nu);

	hd.pval = (void *) nu;
	(void) add_hash(nu->word, hd, wordhash, WORD_HASH_SIZE);

	return nu;
}

void
add_to_list(const char *word)
{
	struct queue_node *node;
	hash_data *exp;
	char buf[32];
	short spcflag = 0;

	if (strlen(word) < 3) {
		return;
	}

	strcpyn(buf, sizeof(buf), word);
	if (buf[strlen(buf) - 1] == ' ') {
		buf[strlen(buf) - 1] = '\0';
		spcflag++;
	}
	exp = find_hash(buf, wordhash, WORD_HASH_SIZE);
	if (exp) {
		node = (struct queue_node *) exp->pval;
		if (spcflag) {
			node->spcount++;
		} else {
			node->count++;
		}
		queue_promote(node);
		counted_words++;
	} else {
		/* printf("%s\n", buf); */
		queue_add_node(word, 1);
		total_words++;
		counted_words++;
	}
}

void
remember_word_variants(const char *in)
{
	char word[32];

	strcpyn(word, sizeof(word), in);
	add_to_list(word);
	/*
	   t = word + strlen(word);
	   while (t > word) {
	   h = word;
	   while (h < t) {
	   add_to_list(h);
	   h++;
	   }
	   t--;
	   if (*t == ' ') {
	   t--;
	   }
	   *t = '\0';
	   }
	 */
}

void
remember_words_in_string(const char *in)
{
	char buf[32];
	const char *h;
	char *o;

	h = in;
	while (*h) {
		o = buf;
		while (*h && !isalnum(*h)) {
			h++;
		}
		while (*h && isalnum(*h) && (o - buf < 30)) {
			if (isupper(*h)) {
				*o++ = tolower(*h++);
			} else {
				*o++ = *h++;
			}
		}
		if (*h == ' ') {
			*o++ = ' ';
		}
		*o++ = '\0';
		if (strlen(buf) > 3) {
			remember_word_variants(buf);
		}
	}
}


int
main(int argc, char **argv)
{
	char buf[16384];
	char *p;

	queue_add_node("felorin", 99999999);
	queue_add_node("anthro", 99999999);
	queue_add_node("phile ", 99999999);
	queue_add_node("morph", 99999999);
	queue_add_node("revar", 99999999);
	queue_add_node("sion ", 99999999);
	queue_add_node("tion ", 99999999);
	queue_add_node("post", 99999999);
	queue_add_node("ing ", 99999999);
	queue_add_node("ion ", 99999999);
	queue_add_node("est ", 99999999);
	queue_add_node("ies ", 99999999);
	queue_add_node("ism ", 99999999);
	queue_add_node("ish ", 99999999);
	queue_add_node("ary ", 99999999);
	queue_add_node("ous ", 99999999);
	queue_add_node("dis", 99999999);
	queue_add_node("non", 99999999);
	queue_add_node("pre", 99999999);
	queue_add_node("sub", 99999999);
	queue_add_node("al ", 99999999);
	queue_add_node("ic ", 99999999);
	queue_add_node("ly ", 99999999);
	queue_add_node("le ", 99999999);
	queue_add_node("es ", 99999999);
	queue_add_node("ed ", 99999999);
	queue_add_node("er ", 99999999);

	while (!feof(stdin)) {
		fgets(buf, sizeof(buf), stdin);
		p = strchr(buf, ':');
		if (p) {
			p = strchr(p + 1, ':');
			if (p) {
				remember_words_in_string(p + 1);
			}
		}
	}
	list_top_4k_words();
	/* printf("%d unique words found.\n", total_words); */
	/* printf("%d counted words.\n", counted_words); */
	return(0);
}
static const char *topwords_c_version = "$RCSfile: topwords.c,v $ $Revision: 1.11 $";
const char *get_topwords_c_version(void) { return topwords_c_version; }
