#include "config.h"

#include "boolexp.h"
#include "commands.h"
#include "compile.h"
#include "db.h"
#include "edit.h"
#include "fbstrings.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#include "match.h"
#include "timequeue.h"
#include "tune.h"

static void
free_line(struct line *l)
{
    if (l->this_line)
	free((void *) l->this_line);
    free((void *) l);
}

void
free_prog_text(struct line *l)
{
    struct line *next;

    while (l) {
	next = l->next;
	free_line(l);
	l = next;
    }
}

char *
macro_expansion(struct macrotable *node, const char *match)
{
    if (!node)
	return NULL;
    else {
	register int value = strcasecmp(match, node->name);

	if (value < 0)
	    return macro_expansion(node->left, match);
	else if (value > 0)
	    return macro_expansion(node->right, match);
	else
	    return alloc_string(node->definition);
    }
}

static struct macrotable *
new_macro(const char *name, const char *definition, dbref player)
{
    struct macrotable *newmacro = (struct macrotable *) malloc(sizeof(struct macrotable));
    char buf[BUFFER_LEN];
    int i;

    for (i = 0; name[i]; i++)
	buf[i] = tolower(name[i]);
    buf[i] = '\0';
    newmacro->name = alloc_string(buf);
    newmacro->definition = alloc_string(definition);
    newmacro->implementor = player;
    newmacro->left = NULL;
    newmacro->right = NULL;
    return (newmacro);
}

static int
grow_macro_tree(struct macrotable *node, struct macrotable *newmacro)
{
    register int value = strcmp(newmacro->name, node->name);

    if (!value)
	return 0;
    else if (value < 0) {
	if (node->left)
	    return grow_macro_tree(node->left, newmacro);
	else {
	    node->left = newmacro;
	    return 1;
	}
    } else if (node->right)
	return grow_macro_tree(node->right, newmacro);
    else {
	node->right = newmacro;
	return 1;
    }
}

static int
insert_macro(const char *macroname, const char *macrodef,
	     dbref player, struct macrotable **node)
{
    struct macrotable *newmacro;

    newmacro = new_macro(macroname, macrodef, player);
    if (!(*node)) {
	*node = newmacro;
	return 1;
    } else
	return (grow_macro_tree((*node), newmacro));
}

static void
do_list_tree(struct macrotable *node, const char *first, const char *last,
	     int length, dbref player)
{
    static char buf[BUFFER_LEN];

    if (!node)
	return;
    else {
	if (strncmp(node->name, first, strlen(first)) >= 0)
	    do_list_tree(node->left, first, last, length, player);
	if ((strncmp(node->name, first, strlen(first)) >= 0) &&
	    (strncmp(node->name, last, strlen(last)) <= 0)) {
	    if (length) {
		notifyf(player, "%-16s %-16s  %s", node->name,
			NAME(node->implementor), node->definition);
		buf[0] = '\0';
	    } else {
		int blen = strlen(buf);
		snprintf(buf + blen, sizeof(buf) - blen, "%-16s", node->name);
		buf[sizeof(buf) - 1] = '\0';
		if (strlen(buf) > 70) {
		    notify(player, buf);
		    buf[0] = '\0';
		}
	    }
	}
	if (strncmp(last, node->name, strlen(last)) >= 0)
	    do_list_tree(node->right, first, last, length, player);
	if ((node == macrotop) && !length) {
	    notify(player, buf);
	    buf[0] = '\0';
	}
    }
}

static void
list_macros(const char *word[], int k, dbref player, int length)
{
    if (!k--) {
	do_list_tree(macrotop, "\001", "\377", length, player);
    } else {
	do_list_tree(macrotop, word[0], word[k], length, player);
    }
    notify(player, "End of list.");
    return;
}

static void
purge_macro_tree(struct macrotable *node)
{
    if (!node)
	return;
    purge_macro_tree(node->left);
    purge_macro_tree(node->right);
    if (node->name)
	free(node->name);
    if (node->definition)
	free(node->definition);
    free(node);
}

static int
erase_node(struct macrotable *oldnode, struct macrotable *node,
	   const char *killname, struct macrotable *mtop)
{
    if (!node)
	return 0;
    else if (strcmp(killname, node->name) < 0)
	return erase_node(node, node->left, killname, mtop);
    else if (strcmp(killname, node->name))
	return erase_node(node, node->right, killname, mtop);
    else {
	if (node == oldnode->left) {
	    oldnode->left = node->left;
	    if (node->right)
		grow_macro_tree(mtop, node->right);
	    if (node->name)
		free(node->name);
	    if (node->definition)
		free(node->definition);
	    free((void *) node);
	    return 1;
	} else {
	    oldnode->right = node->right;
	    if (node->left)
		grow_macro_tree(mtop, node->left);
	    if (node->name)
		free(node->name);
	    if (node->definition)
		free(node->definition);
	    free((void *) node);
	    return 1;
	}
    }
}

static int
kill_macro(const char *macroname, dbref player, struct macrotable **mtop)
{
    if (!(*mtop) || player == NOTHING) {
	return (0);
    } else if (!strcasecmp(macroname, (*mtop)->name)) {
	struct macrotable *macrotemp = (*mtop);
	int whichway = ((*mtop)->left) ? 1 : 0;

	*mtop = whichway ? (*mtop)->left : (*mtop)->right;
	if ((*mtop) && (whichway ? macrotemp->right : macrotemp->left))
	    grow_macro_tree((*mtop), whichway ? macrotemp->right : macrotemp->left);
	if (macrotemp->name)
	    free(macrotemp->name);
	if (macrotemp->definition)
	    free(macrotemp->definition);
	free((void *) macrotemp);
	return (1);
    } else if (erase_node((*mtop), (*mtop), macroname, (*mtop)))
	return (1);
    else
	return (0);
}

#ifdef MEMORY_CLEANUP
void
free_old_macros(void)
{
    purge_macro_tree(macrotop);
}
#endif

static void
chown_macros_rec(struct macrotable *node, dbref from, dbref to)
{
    if (!node)
	return;

    chown_macros_rec(node->left, from, to);

    if (node->implementor == from)
	node->implementor = to;

    chown_macros_rec(node->right, from, to);
}

void
chown_macros(dbref from, dbref to)
{
    chown_macros_rec(macrotop, from, to);
}

static void
disassemble(dbref player, dbref program)
{
    struct inst *curr;
    struct inst *codestart;
    char buf[BUFFER_LEN];

    codestart = curr = PROGRAM_CODE(program);
    if (!PROGRAM_SIZ(program)) {
	notify(player, "Nothing to disassemble!");
	return;
    }
    for (int i = 0; i < PROGRAM_SIZ(program); i++, curr++) {
	switch (curr->type) {
	case PROG_PRIMITIVE:
	    if (curr->data.number >= 1 && curr->data.number <= prim_count)
		snprintf(buf, sizeof(buf), "%d: (line %d) PRIMITIVE: %s", i,
			 curr->line, base_inst[curr->data.number - 1]);
	    else
		snprintf(buf, sizeof(buf), "%d: (line %d) PRIMITIVE: %d", i, curr->line,
			 curr->data.number);
	    break;
	case PROG_MARK:
	    snprintf(buf, sizeof(buf), "%d: (line %d) MARK", i, curr->line);
	    break;
	case PROG_STRING:
	    snprintf(buf, sizeof(buf), "%d: (line %d) STRING: \"%s\"", i, curr->line,
		     DoNullInd(curr->data.string));
	    break;
	case PROG_ARRAY:
	    snprintf(buf, sizeof(buf), "%d: (line %d) ARRAY: %d items", i, curr->line,
		     curr->data.array ? curr->data.array->items : 0);
	    break;
	case PROG_FUNCTION:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FUNCTION: %s, VARS: %d, ARGS: %d", i,
		     curr->line, DoNull(curr->data.mufproc->procname),
		     curr->data.mufproc->vars, curr->data.mufproc->args);
	    break;
	case PROG_LOCK:
	    snprintf(buf, sizeof(buf), "%d: (line %d) LOCK: [%s]", i, curr->line,
		     curr->data.lock == TRUE_BOOLEXP ? "TRUE_BOOLEXP" :
		     unparse_boolexp(0, curr->data.lock, 0));
	    break;
	case PROG_INTEGER:
	    snprintf(buf, sizeof(buf), "%d: (line %d) INTEGER: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_FLOAT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FLOAT: %.17g", i, curr->line,
		     curr->data.fnumber);
	    break;
	case PROG_ADD:
	    snprintf(buf, sizeof(buf), "%d: (line %d) ADDRESS: %d", i,
		     curr->line, (int) (curr->data.addr->data - codestart));
	    break;
	case PROG_TRY:
	    snprintf(buf, sizeof(buf), "%d: (line %d) TRY: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_IF:
	    snprintf(buf, sizeof(buf), "%d: (line %d) IF: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_JMP:
	    snprintf(buf, sizeof(buf), "%d: (line %d) JMP: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_EXEC:
	    snprintf(buf, sizeof(buf), "%d: (line %d) EXEC: %d", i, curr->line,
		     (int) (curr->data.call - codestart));
	    break;
	case PROG_OBJECT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) OBJECT REF: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_VAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) VARIABLE: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_SVAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) SCOPEDVAR: %d (%s)", i, curr->line,
		     curr->data.number, scopedvar_getname_byinst(curr, curr->data.number));
	    break;
	case PROG_SVAR_AT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH SCOPEDVAR: %d (%s)", i, curr->line,
		     curr->data.number, scopedvar_getname_byinst(curr, curr->data.number));
	    break;
	case PROG_SVAR_AT_CLEAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH SCOPEDVAR (clear optim): %d (%s)",
		     i, curr->line, curr->data.number, scopedvar_getname_byinst(curr,
										curr->data.
										number));
	    break;
	case PROG_SVAR_BANG:
	    snprintf(buf, sizeof(buf), "%d: (line %d) SET SCOPEDVAR: %d (%s)", i, curr->line,
		     curr->data.number, scopedvar_getname_byinst(curr, curr->data.number));
	    break;
	case PROG_LVAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) LOCALVAR: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_LVAR_AT:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH LOCALVAR: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_LVAR_AT_CLEAR:
	    snprintf(buf, sizeof(buf), "%d: (line %d) FETCH LOCALVAR (clear optim): %d", i,
		     curr->line, curr->data.number);
	    break;
	case PROG_LVAR_BANG:
	    snprintf(buf, sizeof(buf), "%d: (line %d) SET LOCALVAR: %d", i, curr->line,
		     curr->data.number);
	    break;
	case PROG_CLEARED:
	    snprintf(buf, sizeof(buf), "%d: (line ?) CLEARED INST AT %s:%d", i,
		     (char *) curr->data.addr, curr->line);
	default:
	    snprintf(buf, sizeof(buf), "%d: (line ?) UNKNOWN INST", i);
	}
	notify(player, buf);
    }
}

/* puts program into insert mode */
static void
do_insert(dbref player, dbref program, int arg[], int argc)
{
    PLAYER_SET_INSERT_MODE(player, PLAYER_INSERT_MODE(player) + 1);
    /* DBDIRTY(player); */
    if (argc)
	PROGRAM_SET_CURR_LINE(program, arg[0] - 1);
    /* set current line to something else */
}

/* deletes line n if one argument,
   lines arg1 -- arg2 if two arguments
   current line if no argument */
static void
do_delete(dbref player, dbref program, int arg[], int argc)
{
    struct line *curr, *temp;
    int i;

    switch (argc) {
    case 0:
	arg[0] = PROGRAM_CURR_LINE(program);
    case 1:
	arg[1] = arg[0];
    case 2:
	/* delete from line 1 to line 2 */
	/* first, check for conflict */
	if (arg[0] > arg[1]) {
	    notify(player, "Nonsensical arguments.");
	    return;
	}
	i = arg[0] - 1;
	for (curr = PROGRAM_FIRST(program); curr && i; i--)
	    curr = curr->next;
	if (curr) {
	    int n = 0;
	    PROGRAM_SET_CURR_LINE(program, arg[0]);
	    i = arg[1] - arg[0] + 1;
	    /* delete n lines */
	    while (i && curr) {
		temp = curr;
		if (curr->prev)
		    curr->prev->next = curr->next;
		else
		    PROGRAM_SET_FIRST(program, curr->next);
		if (curr->next)
		    curr->next->prev = curr->prev;
		curr = curr->next;
		free_line(temp);
		i--;
	    }
	    n = arg[1] - arg[0] - i + 1;
	    notifyf(player, "%d line%s deleted", n, n != 1 ? "s" : "");
	} else
	    notify(player, "No line to delete!");
	break;
    default:
	notify(player, "Too many arguments!");
	break;
    }
}

/* quit from edit mode.  Put player back into the regular game mode */
static void
do_quit(dbref player, dbref program)
{
    char unparse_buf[BUFFER_LEN];
    unparse_object(player, program, unparse_buf, sizeof(unparse_buf));
    log_status("PROGRAM SAVED: %s by %s(%d)", unparse_buf,
	       NAME(player), player);
    write_program(PROGRAM_FIRST(program), program);

    if (tp_log_programs)
	log_program_text(PROGRAM_FIRST(program), player, program);

    free_prog_text(PROGRAM_FIRST(program));
    PROGRAM_SET_FIRST(program, NULL);
    FLAGS(program) &= ~INTERNAL;
    FLAGS(player) &= ~INTERACTIVE;
    PLAYER_SET_CURR_PROG(player, NOTHING);
    DBDIRTY(player);
    DBDIRTY(program);
}

/* quit from edit mode, with no changes written */
static void
do_cancel(dbref player, dbref program)
{
    uncompile_program(program);
    free_prog_text(PROGRAM_FIRST(program));
    PROGRAM_SET_FIRST(program, NULL);
    FLAGS(program) &= ~INTERNAL;
    FLAGS(player) &= ~INTERACTIVE;
    PLAYER_SET_CURR_PROG(player, NOTHING);
    DBDIRTY(player);
}

/* list --- if no argument, redisplay the current line
   if 1 argument, display that line
   if 2 arguments, display all in between   */
void
list_program(dbref player, dbref program, int *oarg, int argc, int comment_sysmsg)
{
    struct line *curr;
    int i, count;
    int arg[2];
    char *msg_start = comment_sysmsg ? "( " : "";
    char *msg_end = comment_sysmsg ? " )" : "";

    if (oarg) {
	arg[0] = oarg[0];
	if (argc > 1) arg[1] = oarg[1];
    } else
	arg[0] = arg[1] = 0;
    switch (argc) {
    case 0:
	arg[0] = PROGRAM_CURR_LINE(program);
    case 1:
	arg[1] = arg[0];
    case 2:
	if ((arg[0] > arg[1]) && (arg[1] != -1)) {
	    notifyf_nolisten(player, "%sArguments don't make sense!%s", msg_start, msg_end);
	    return;
	}
	i = arg[0] - 1;
	for (curr = PROGRAM_FIRST(program); i && curr; i--)
	    curr = curr->next;
	if (curr) {
	    i = arg[1] - arg[0] + 1;
	    /* display n lines */
	    for (count = arg[0]; curr && (i || (arg[1] == -1)); i--) {
		if (FLAGS(player) & INTERNAL)
		    notifyf_nolisten(player, "%3d: %s", count, DoNull(curr->this_line));
		else
		    notifyf_nolisten(player, "%s", DoNull(curr->this_line));
		count++;
		curr = curr->next;
	    }
	    if (count - arg[0] > 1) {
		notifyf_nolisten(player, "%s%d lines displayed.%s", msg_start, count - arg[0], msg_end);
	    }
	} else
	    notifyf_nolisten(player, "%sLine not available for display.%s", msg_start, msg_end);
	break;
    default:
	notifyf_nolisten(player, "%sToo many arguments!%s", msg_start, msg_end);
	break;
    }
}

static void
do_list_header(dbref player, dbref program)
{
    struct line *curr = read_program(program);

    while (curr && (curr->this_line)[0] == '(') {
	notify(player, curr->this_line);
	curr = curr->next;
    }
    if (!(FLAGS(program) & INTERNAL)) {
	free_prog_text(curr);
    }
    notify(player, "Done.");
}

static void
val_and_head(dbref player, int arg[], int argc)
{
    dbref program;

    if (argc != 1) {
	notify(player, "I don't understand which header you're trying to look at.");
	return;
    }
    program = arg[0];
    if (!ObjExists(program) || Typeof(program) != TYPE_PROGRAM) {
	notify(player, "That isn't a program.");
	return;
    }
    if (!(controls(player, program) || Linkable(program))) {
	notify(player, "That's not a public program.");
	return;
    }
    do_list_header(player, program);
}

static void
do_list_publics(dbref player, dbref program)
{
    notify(player, "PUBLIC funtions:");
    for (struct publics *ptr = PROGRAM_PUBS(program); ptr; ptr = ptr->next)
	notify(player, ptr->subname);
}

static void
list_publics(int descr, dbref player, int arg[], int argc)
{
    dbref program;

    if (argc > 1) {
	notify(player,
	       "I don't understand which program you want to list PUBLIC functions for.");
	return;
    }
    program = (argc == 0) ? PLAYER_CURR_PROG(player) : arg[0];
    if (!OkObj(program) || Typeof(program) != TYPE_PROGRAM) {
	notify(player, "That isn't a program.");
	return;
    }
    if (!(controls(player, program) || Linkable(program))) {
	notify(player, "That's not a public program.");
	return;
    }
    if (!(PROGRAM_CODE(program))) {
	if (program == PLAYER_CURR_PROG(player)) {
	    do_compile(descr, OWNER(program), program, 0);
	} else {
	    struct line *tmpline;

	    tmpline = PROGRAM_FIRST(program);
	    PROGRAM_SET_FIRST(program, (struct line *) read_program(program));
	    do_compile(descr, OWNER(program), program, 0);
	    free_prog_text(PROGRAM_FIRST(program));
	    PROGRAM_SET_FIRST(program, tmpline);
	}
	if (!PROGRAM_CODE(program)) {
	    notify(player, "Program not compilable.");
	    return;
	}
    }
    do_list_publics(player, program);
}

static void
toggle_numbers(dbref player, int arg[], int argc)
{
    if (argc) {
	switch (arg[0]) {
	case 0:
	    FLAGS(player) &= ~INTERNAL;
	    notify(player, "Line numbers off.");
	    break;

	default:
	    FLAGS(player) |= INTERNAL;
	    notify(player, "Line numbers on.");
	    break;
	}
    } else if (FLAGS(player) & INTERNAL) {
	FLAGS(player) &= ~INTERNAL;
	notify(player, "Line numbers off.");
    } else {
	FLAGS(player) |= INTERNAL;
	notify(player, "Line numbers on.");
    }
}

/* insert this line into program */
static void
insert_line(dbref player, const char *line)
{
    dbref program;
    int i;
    struct line *curr;
    struct line *new_line;

    program = PLAYER_CURR_PROG(player);
    if (!strcasecmp(line, EXIT_INSERT)) {
	PLAYER_SET_INSERT_MODE(player, 0);	/* turn off insert mode */
	notify_nolisten(player, "Exiting insert mode.", 1);
	return;
    }
    i = PROGRAM_CURR_LINE(program) - 1;
    for (curr = PROGRAM_FIRST(program); curr && i && i + 1; i--)
	curr = curr->next;
    new_line = get_new_line();	/* initialize line */
    if (!*line) {
	new_line->this_line = alloc_string(" ");
    } else {
	new_line->this_line = alloc_string(line);
    }
    if (!PROGRAM_FIRST(program)) {	/* nothing --- insert in front */
	PROGRAM_SET_FIRST(program, new_line);
	PROGRAM_SET_CURR_LINE(program, 2);	/* insert at the end */
	/* DBDIRTY(program); */
	return;
    }
    if (!curr) {		/* insert at the end */
	i = 1;
	for (curr = PROGRAM_FIRST(program); curr->next; curr = curr->next)
	    i++;		/* count lines */
	PROGRAM_SET_CURR_LINE(program, i + 2);
	new_line->prev = curr;
	curr->next = new_line;
	/* DBDIRTY(program); */
	return;
    }
    if (!PROGRAM_CURR_LINE(program)) {	/* insert at the
					 * beginning */
	PROGRAM_SET_CURR_LINE(program, 1);	/* insert after this new
						 * line */
	new_line->next = PROGRAM_FIRST(program);
	PROGRAM_SET_FIRST(program, new_line);
	/* DBDIRTY(program); */
	return;
    }
    /* inserting in the middle */
    PROGRAM_SET_CURR_LINE(program, PROGRAM_CURR_LINE(program) + 1);
    new_line->prev = curr;
    new_line->next = curr->next;
    if (new_line->next)
	new_line->next->prev = new_line;
    curr->next = new_line;
    /* DBDIRTY(program); */
}

/* The editor itself --- this gets called each time every time to
 * parse a command.
 */

static void
editor(int descr, dbref player, const char *command)
{
    dbref program;
    int arg[MAX_ARG + 1];
    char buf[BUFFER_LEN];
    const char *word[MAX_ARG + 1];
    int i, j;			/* loop variables */

    program = PLAYER_CURR_PROG(player);

    /* check to see if we are insert mode */
    if (PLAYER_INSERT_MODE(player)) {
	insert_line(player, command);	/* insert it! */
	return;
    }
    /* parse the commands */
    for (i = 0; i <= MAX_ARG && *command; i++) {
	skip_whitespace(&command);
	j = 0;
	while (*command && !isspace(*command)) {
	    buf[j] = *command;
	    command++, j++;
	}

	buf[j] = '\0';
	word[i] = alloc_string(buf);
	if ((i == 1) && !strcasecmp(word[0], "def")) {
	    if (word[1] && (word[1][0] == '.' || (word[1][0] >= '0' && word[1][0] <= '9'))) {
		notify(player, "Invalid macro name.");
		return;
	    }
	    skip_whitespace(&command);
	    word[2] = alloc_string(command);
	    if (!word[2])
		notify(player, "Invalid definition syntax.");
	    else {
		if (insert_macro(word[1], word[2], player, &macrotop)) {
		    notify(player, "Entry created.");
		} else {
		    notify(player, "That macro already exists!");
		}
	    }
	    for (; i >= 0; i--) {
		if (word[i])
		    free((void *) word[i]);
	    }
	    return;
	}
	arg[i] = atoi(buf);
	if (arg[i] < 0) {
	    notify(player, "Negative arguments not allowed!");
	    for (; i >= 0; i--) {
		if (word[i])
		    free((void *) word[i]);
	    }
	    return;
	}
    }
    i--;
    while ((i >= 0) && !word[i])
	i--;
    if (i < 0) {
	return;
    } else {
	switch (word[i][0]) {
	case KILL_COMMAND:
	    if (!Wizard(player)) {
		notify(player, "I'm sorry Dave, but I can't let you do that.");
	    } else {
		if (kill_macro(word[0], player, &macrotop))
		    notify(player, "Macro entry deleted.");
		else
		    notify(player, "Macro to delete not found.");
	    }
	    break;
	case SHOW_COMMAND:
	    list_macros(word, i, player, 1);
	    break;
	case SHORTSHOW_COMMAND:
	    list_macros(word, i, player, 0);
	    break;
	case INSERT_COMMAND:
	    do_insert(player, program, arg, i);
	    notify(player, "Entering insert mode.");
	    break;
	case DELETE_COMMAND:
	    do_delete(player, program, arg, i);
	    break;
	case QUIT_EDIT_COMMAND:
	    do_quit(player, program);
	    notify(player, "Editor exited.");
	    break;
	case CANCEL_EDIT_COMMAND:
	    do_cancel(player, program);
	    notify(player, "Changes cancelled.");
	    break;
	case COMPILE_COMMAND:
	    /* compile code belongs in compile.c, not in the editor */
	    do_compile(descr, player, program, 1);
	    notify(player, "Compiler done.");
	    break;
	case LIST_COMMAND:
	    list_program(player, program, arg, i, 0);
	    break;
	case EDITOR_HELP_COMMAND:
	    spit_file(player, tp_file_editor_help);
	    break;
	case VIEW_COMMAND:
	    val_and_head(player, arg, i);
	    break;
	case UNASSEMBLE_COMMAND:
	    disassemble(player, program);
	    break;
	case NUMBER_COMMAND:
	    toggle_numbers(player, arg, i);
	    break;
	case PUBLICS_COMMAND:
	    list_publics(descr, player, arg, i);
	    break;
	default:
	    notify(player, "Illegal editor command.");
	    break;
	}
    }
    for (; i >= 0; i--) {
	if (word[i])
	    free((void *) word[i]);
    }
}

struct line *
get_new_line(void)
{
    struct line *nu;

    nu = (struct line *) malloc(sizeof(struct line));

    if (!nu) {
	fprintf(stderr, "get_new_line(): Out of memory!\n");
	abort();
    }
    nu->this_line = NULL;
    nu->next = NULL;
    nu->prev = NULL;
    return nu;
}

struct line *
read_program(dbref i)
{
    char buf[BUFFER_LEN];
    struct line *first;
    struct line *prev = NULL;
    struct line *nu;
    FILE *f;
    int len;

    first = NULL;
    snprintf(buf, sizeof(buf), "muf/%d.m", (int) i);
    f = fopen(buf, "rb");
    if (!f)
	return 0;

    while (fgets(buf, BUFFER_LEN, f)) {
	nu = get_new_line();
	len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n') {
	    buf[len - 1] = '\0';
	    len--;
	}
	if (len > 0 && buf[len - 1] == '\r') {
	    buf[len - 1] = '\0';
	    len--;
	}
	if (!*buf)
	    strcpyn(buf, sizeof(buf), " ");
	nu->this_line = alloc_string(buf);
	if (!first) {
	    prev = nu;
	    first = nu;
	} else {
	    prev->next = nu;
	    nu->prev = prev;
	    prev = nu;
	}
    }

    fclose(f);
    return first;
}

void
write_program(struct line *first, dbref i)
{
    FILE *f;
    char fname[BUFFER_LEN];

    snprintf(fname, sizeof(fname), "muf/%d.m", (int) i);
    f = fopen(fname, "wb");
    if (!f) {
	log_status("Couldn't open file %s!", fname);
	return;
    }
    while (first) {
	if (!first->this_line)
	    continue;
	if (fputs(first->this_line, f) == EOF) {
	    abort();
	}
	if (fputc('\n', f) == EOF) {
	    abort();
	}
	first = first->next;
    }
    fclose(f);
}

void
do_list(int descr, dbref player, const char *name, char *linespec)
{
    dbref thing;
    const char *p;
    char *q;
    int range[2];
    int argc;
    int comment_sysmsg = 0;
    object_flag_type prevflags = FLAGS(player);
    struct match_data md;
    struct line *tmpline;

    init_match(descr, player, name, TYPE_PROGRAM, &md);
    match_neighbor(&md);
    match_possession(&md);
    match_registered(&md);
    match_absolute(&md);
    if ((thing = noisy_match_result(&md)) == NOTHING)
	return;
    if (Typeof(thing) != TYPE_PROGRAM) {
	notify(player, "You can't list anything but a program.");
	return;
    }
    if (!(controls(player, thing) || (FLAGS(thing) & VEHICLE))) {
	notify(player,
	       "Permission denied. (You don't control the program, and it's not set Viewable)");
	return;
    }

    while (*linespec && (*linespec == '!' || *linespec == '#' || *linespec == '@')) {
	prevflags = FLAGS(player);
	switch (*linespec) {
	case '!':
	    comment_sysmsg = 1;
	    break;
	case '#':
	    FLAGS(player) |= INTERNAL;
	    break;
	case '@':
	    FLAGS(player) &= ~INTERNAL;
	    break;
	default:
	    break;
	}
	(void) *linespec++;
    }

    if (!*linespec) {
	range[0] = 1;
	range[1] = -1;
	argc = 2;
    } else {
	q = (char *)(p = linespec);
	while (*p) {
	    while (*p && !isspace(*p))
		*q++ = *p++;
	    skip_whitespace(&p);
	}
	*q = '\0';

	argc = 1;
	if (isdigit(*linespec)) {
	    range[0] = atoi(linespec);
	    while (*linespec && isdigit(*linespec))
		linespec++;
	} else {
	    range[0] = 1;
	}
	if (*linespec) {
	    argc = 2;
	    while (*linespec && !isdigit(*linespec))
		linespec++;
	    if (*linespec)
		range[1] = atoi(linespec);
	    else
		range[1] = -1;
	}
    }
    tmpline = PROGRAM_FIRST(thing);
    PROGRAM_SET_FIRST(thing, read_program(thing));
    list_program(player, thing, range, argc, comment_sysmsg);
    free_prog_text(PROGRAM_FIRST(thing));
    PROGRAM_SET_FIRST(thing, tmpline);
    FLAGS(player) = prevflags;
    return;
}

/* This routine determines if a player is editing or running an interactive
   command.  It does it by checking the frame pointer field of the player ---
   if the program counter is NULL, then the player is not running anything
   The reason we don't just check the pointer but check the pc too is because
   I plan to leave the frame always on to save the time required allocating
   space each time a program is run.
   */
void
interactive(int descr, dbref player, const char *command)
{
    if (FLAGS(player) & READMODE) {
	/*
	 * process command, push onto stack, and return control to forth
	 * program
	 */
	handle_read_event(descr, player, command);
    } else {
	editor(descr, player, command);
    }
}

void
macrodump(struct macrotable *node, FILE * f)
{
    if (!node)
	return;
    macrodump(node->left, f);
    putstring(f, node->name);
    putstring(f, node->definition);
    putref(f, node->implementor);
    macrodump(node->right, f);
}

static char *
file_line(FILE * f)
{
    char buf[BUFFER_LEN];
    int len;

    if (!fgets(buf, BUFFER_LEN, f))
	return NULL;
    len = strlen(buf);
    if (buf[len - 1] == '\n') {
	buf[--len] = '\0';
    }
    if (buf[len - 1] == '\r') {
	buf[--len] = '\0';
    }
    return alloc_string(buf);
}

static void
foldtree(struct macrotable *center)
{
    int count = 0;
    struct macrotable *nextcent = center;

    for (; nextcent; nextcent = nextcent->left)
	count++;
    if (count > 1) {
	for (nextcent = center, count /= 2; count--; nextcent = nextcent->left) ;
	if (center->left)
	    center->left->right = NULL;
	center->left = nextcent;
	foldtree(center->left);
    }
    for (count = 0, nextcent = center; nextcent; nextcent = nextcent->right)
	count++;
    if (count > 1) {
	for (nextcent = center, count /= 2; count--; nextcent = nextcent->right) ;
	if (center->right)
	    center->right->left = NULL;
	foldtree(center->right);
    }
}

static int
macrochain(struct macrotable *lastnode, FILE * f)
{
    char *line, *line2;
    struct macrotable *newmacro;

    if (!(line = file_line(f)))
	return 0;
    line2 = file_line(f);

    newmacro = (struct macrotable *) new_macro(line, line2, getref(f));
    free(line);
    free(line2);

    if (!macrotop)
	macrotop = (struct macrotable *) newmacro;
    else {
	newmacro->left = lastnode;
	if (lastnode) lastnode->right = newmacro;
    }
    return (1 + macrochain(newmacro, f));
}

void
macroload(FILE * f)
{
    int count = 0;

    macrotop = NULL;
    count = macrochain(macrotop, f);
    for (count /= 2; count--; macrotop = macrotop->right) ;
    foldtree(macrotop);
    return;
}

void
edit_program(dbref player, dbref program)
{
    char unparse_buf[BUFFER_LEN];

    if ((Typeof(program) != TYPE_PROGRAM) || !controls(player, program)) {
	notify(player, "Permission denied!");
	return;
    }

    if (FLAGS(program) & INTERNAL) {
	notify(player,
		"Sorry, this program is currently being edited by someone else.  Try again later.");
	return;
    }

    PROGRAM_SET_FIRST(program, read_program(program));
    FLAGS(program) |= INTERNAL;
    DBDIRTY(program);

    PLAYER_SET_CURR_PROG(player, program);
    FLAGS(player) |= INTERACTIVE;
    DBDIRTY(player);

    unparse_object(player, program, unparse_buf, sizeof(unparse_buf));
    notifyf(player, "Entering editor for %s.", unparse_buf);

    list_program(player, program, NULL, 0, 0);
}
