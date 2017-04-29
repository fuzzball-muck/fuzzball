#include "config.h"

#include "boolexp.h"
#include "compile.h"
#include "db.h"
#include "debugger.h"
#include "edit.h"
#include "fbmath.h"
#include "fbstrings.h"
#include "fbtime.h"
#include "game.h"
#include "inst.h"
#include "interface.h"
#include "interp.h"
#include "log.h"
#ifdef MCP_SUPPORT
#include "mcpgui.h"
#endif
#include "mufevent.h"
#include "predicates.h"
#include "props.h"
#include "timequeue.h"
#include "tune.h"

#define ERROR_DIE_NOW -1

void
p_null(PRIM_PROTOTYPE)
{
    return;
}

static void (*prim_func[]) (PRIM_PROTOTYPE) = {
    p_null, p_null, p_null, p_null, p_null, p_null,
            /* JMP, READ,   SLEEP,  CALL,   EXECUTE, RETURN, */
            p_null, p_null, p_null,
            /* EVENT_WAITFOR, CATCH,  CATCH_DETAILED */
            PRIMS_CONNECTS_FUNCS,
            PRIMS_DB_FUNCS,
            PRIMS_MATH_FUNCS,
            PRIMS_MISC_FUNCS,
            PRIMS_PROPS_FUNCS,
            PRIMS_STACK_FUNCS,
            PRIMS_STRINGS_FUNCS, PRIMS_ARRAY_FUNCS, PRIMS_FLOAT_FUNCS, PRIMS_ERROR_FUNCS,
#ifdef MCP_SUPPORT
            PRIMS_MCP_FUNCS,
#endif
PRIMS_REGEX_FUNCS, PRIMS_INTERNAL_FUNCS, NULL};

struct localvars *
localvars_get(struct frame *fr, dbref prog)
{
    if (!fr) {
	panic("localvars_get(): NULL frame passed !");
    }

    struct localvars *tmp = fr->lvars;

    while (tmp && tmp->prog != prog)
	tmp = tmp->next;
    if (tmp) {
	/* Pull this out of the middle of the stack. */
	*tmp->prev = tmp->next;
	if (tmp->next)
	    tmp->next->prev = tmp->prev;

    } else {
	/* Create a new var frame. */
	int count = MAX_VAR;
	tmp = (struct localvars *) malloc(sizeof(struct localvars));
	tmp->prog = prog;
	while (count-- > 0) {
	    tmp->lvars[count].type = PROG_INTEGER;
	    tmp->lvars[count].data.number = 0;
	}
    }

    /* Add this to the head of the stack. */
    tmp->next = fr->lvars;
    tmp->prev = &fr->lvars;
    fr->lvars = tmp;
    if (tmp->next)
	tmp->next->prev = &tmp->next;

    return tmp;
}

void
localvar_dupall(struct frame *fr, struct frame *oldfr)
{
    if (!fr || !oldfr) {
	panic("localvar_dupall(): NULL frame passed !");
    }

    struct localvars *orig = oldfr->lvars;
    struct localvars **targ = &fr->lvars;

    while (orig) {
	int count = MAX_VAR;
	*targ = (struct localvars *) malloc(sizeof(struct localvars));
	while (count-- > 0)
	    copyinst(&orig->lvars[count], &(*targ)->lvars[count]);
	(*targ)->prog = orig->prog;
	(*targ)->next = NULL;
	(*targ)->prev = targ;
	targ = &((*targ)->next);
	orig = orig->next;
    }
}

static void
localvar_freeall(struct frame *fr)
{
    if (!fr) {
	panic("localvar_freeall(): NULL frame passed !");
    }

    struct localvars *ptr = fr->lvars;
    struct localvars *nxt;

    while (ptr) {
	int count = MAX_VAR;
	nxt = ptr->next;
	while (count-- > 0)
	    CLEAR(&ptr->lvars[count]);
	ptr->next = NULL;
	ptr->prev = NULL;
	ptr->prog = NOTHING;
	free((void *) ptr);
	ptr = nxt;
    }
    fr->lvars = NULL;
}

static void
scopedvar_addlevel(struct frame *fr, struct inst *pc, int count)
{
    if (!fr) {
	panic("scopedvar_addlevel(): NULL frame passed !");
    }

    struct scopedvar_t *tmp;
    int siz;
    siz = sizeof(struct scopedvar_t) + (sizeof(struct inst) * (count - 1));

    tmp = (struct scopedvar_t *) malloc(siz);
    tmp->count = count;
    tmp->varnames = pc->data.mufproc->varnames;
    tmp->next = fr->svars;
    fr->svars = tmp;
    while (count-- > 0) {
	tmp->vars[count].type = PROG_INTEGER;
	tmp->vars[count].data.number = 0;
    }
}

void
scopedvar_dupall(struct frame *fr, struct frame *oldfr)
{
    if (!fr || !oldfr) {
	panic("scopedvar_dupall(): NULL frame passed !");
    }

    struct scopedvar_t *newsv;
    struct scopedvar_t **prev;
    int siz, count;

    prev = &fr->svars;
    *prev = NULL;
    for (struct scopedvar_t *cur = oldfr->svars; cur; cur = cur->next) {
	count = cur->count;
	siz = sizeof(struct scopedvar_t) + (sizeof(struct inst) * (count - 1));

	newsv = (struct scopedvar_t *) malloc(siz);
	newsv->count = count;
	newsv->varnames = cur->varnames;
	newsv->next = NULL;
	while (count-- > 0) {
	    copyinst(&cur->vars[count], &newsv->vars[count]);
	}
	*prev = newsv;
	prev = &newsv->next;
    }
}

static int
scopedvar_poplevel(struct frame *fr)
{
    if (!fr || !fr->svars) {
	return 0;
    }

    struct scopedvar_t *tmp = fr->svars;
    fr->svars = fr->svars->next;
    while (tmp->count-- > 0) {
	CLEAR(&tmp->vars[tmp->count]);
    }
    free(tmp);
    return 1;
}

static void
scopedvar_freeall(struct frame *fr)
{
    while (scopedvar_poplevel(fr)) ;
}

struct inst *
scopedvar_get(struct frame *fr, int level, int varnum)
{
    struct scopedvar_t *svinfo = fr ? fr->svars : NULL;
    while (svinfo && level-- > 0)
	svinfo = svinfo->next;
    if (!svinfo) {
	return NULL;
    }
    if (varnum < 0 || varnum >= svinfo->count) {
	return NULL;
    }
    return (&svinfo->vars[varnum]);
}

const char *
scopedvar_getname_byinst(struct inst *pc, int varnum)
{
    while (pc && pc->type != PROG_FUNCTION)
	pc--;
    if (!pc || !pc->data.mufproc) {
	return NULL;
    }
    if (varnum < 0 || varnum >= pc->data.mufproc->vars) {
	return NULL;
    }
    if (!pc->data.mufproc->varnames) {
	return NULL;
    }
    return pc->data.mufproc->varnames[varnum];
}

const char *
scopedvar_getname(struct frame *fr, int level, int varnum)
{
    struct scopedvar_t *svinfo = fr ? fr->svars : NULL;

    while (svinfo && level-- > 0)
	svinfo = svinfo->next;
    if (!svinfo) {
	return NULL;
    }
    if (varnum < 0 || varnum >= svinfo->count) {
	return NULL;
    }
    if (!svinfo->varnames) {
	return NULL;
    }
    return svinfo->varnames[varnum];
}

void
RCLEAR(struct inst *oper, char *file, int line)
{
    int varcnt;

    assert(oper != NULL);
    assert(file != NULL);
    assert(line > 0);

    switch (oper->type) {
    case PROG_CLEARED:{
	    log_status
		    ("WARNING: attempt to re-CLEAR() instruction from %s:%d  previously CLEAR()ed at %s:%d",
		     file, line, (char *) oper->data.addr, oper->line);
	    assert(0);		/* If debugging, we want to figure out just what
				   is going on, and dump core at this point.  This
				   will at least give us some idea of what's going on. */
	    return;
	}
    case PROG_ADD:
	PROGRAM_DEC_INSTANCES(oper->data.addr->progref);
	oper->data.addr->links--;
	break;
    case PROG_STRING:
	if (oper->data.string && --oper->data.string->links == 0)
	    free((void *) oper->data.string);
	break;
    case PROG_FUNCTION:
	if (oper->data.mufproc) {
	    free((void *) oper->data.mufproc->procname);
	    varcnt = oper->data.mufproc->vars;
	    if (oper->data.mufproc->varnames) {
		for (int j = 0; j < varcnt; j++) {
		    free((void *) oper->data.mufproc->varnames[j]);
		}
		free((void *) oper->data.mufproc->varnames);
	    }
	    free((void *) oper->data.mufproc);
	}
	break;
    case PROG_ARRAY:
	array_free(oper->data.array);
	break;
    case PROG_LOCK:
	if (oper->data.lock != TRUE_BOOLEXP)
	    free_boolexp(oper->data.lock);
	break;
    }
    oper->line = line;
    oper->data.addr = (struct prog_addr *) file;
    oper->type = PROG_CLEARED;
}

int top_pid = 1;
int nargs = 0;
#ifdef DEBUG
int expect_pop = 0;
int actual_pop = 0;
int expect_push_to = 0;
#endif
int prim_count = 0;

static struct frame *free_frames_list = NULL;

static struct forvars *for_pool = NULL;
static struct forvars **last_for = &for_pool;
static struct tryvars *try_pool = NULL;
static struct tryvars **last_try = &try_pool;

void
purge_free_frames(void)
{
    struct frame *ptr, *ptr2;
    int count = tp_free_frames_pool;

    for (ptr = free_frames_list; ptr && --count > 0; ptr = ptr->next) ;
    while (ptr && ptr->next) {
	ptr2 = ptr->next;
	ptr->next = ptr->next->next;
	free(ptr2);
    }
}

#ifdef MEMORY_CLEANUP
void
purge_all_free_frames(void)
{
    struct frame *ptr;

    while (free_frames_list) {
	ptr = free_frames_list;
	free_frames_list = ptr->next;
	free(ptr);
    }
}
#endif

void
purge_for_pool(void)
{
    /* This only purges up to the most recently used. */
    /* Purge this a second time to purge all. */
    struct forvars *cur, *next;

    cur = *last_for;
    *last_for = NULL;
    last_for = &for_pool;

    while (cur) {
	next = cur->next;
	free(cur);
	cur = next;
    }
}

void
purge_try_pool(void)
{
    /* This only purges up to the most recently used. */
    /* Purge this a second time to purge all. */
    struct tryvars *cur, *next;

    cur = *last_try;
    *last_try = NULL;
    last_try = &try_pool;

    while (cur) {
	next = cur->next;
	free(cur);
	cur = next;
    }
}

struct frame *
interp(int descr, dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms, int forced_pid)
{
    struct frame *fr;

    if (!MLevel(program) || !MLevel(OWNER(program)) ||
	((source != NOTHING) && !TrueWizard(OWNER(source)) &&
	 !can_link_to(OWNER(source), TYPE_EXIT, program))) {
	notify_nolisten(player, "Program call: Permission denied.", 1);
	return 0;
    }
    if (free_frames_list) {
	fr = free_frames_list;
	free_frames_list = fr->next;
    } else {
	fr = (struct frame *) malloc(sizeof(struct frame));
    }
    fr->next = NULL;
    fr->pid = forced_pid ? forced_pid : top_pid++;
    fr->descr = descr;
    fr->supplicant = NOTHING;
    fr->multitask = nosleeps;
    fr->perms = whichperms;
    fr->already_created = 0;
    fr->been_background = (nosleeps == 2);
    fr->trig = source;
    fr->cmd = (!*match_cmdname) ? 0 : alloc_prog_string(match_cmdname);
    fr->events = NULL;
    fr->timercount = 0;
    fr->started = time(NULL);
    fr->instcnt = 0;
    fr->skip_declare = 0;
    fr->wantsblanks = 0;
    fr->caller.top = 1;
    fr->caller.st[0] = source;
    fr->caller.st[1] = program;

    fr->system.top = 1;
    fr->system.st[0].progref = 0;
    fr->system.st[0].offset = 0;

    fr->waitees = NULL;
    fr->waiters = NULL;

    fr->fors.top = 0;
    fr->fors.st = NULL;
    fr->trys.top = 0;
    fr->trys.st = NULL;

    fr->errorstr = NULL;
    fr->errorinst = NULL;
    fr->errorprog = NOTHING;
    fr->errorline = 0;

    fr->rndbuf = NULL;
    fr->dlogids = NULL;

    fr->argument.top = 0;
    fr->pc = PROGRAM_START(program);
    fr->writeonly = ((source == -1) || (Typeof(source) == TYPE_ROOM) ||
		     ((Typeof(source) == TYPE_PLAYER) && (!online(source))) ||
		     (FLAGS(player) & READMODE));
    fr->level = 0;
    fr->error.is_flags = 0;

    /* set basic local variables */

    fr->svars = NULL;
    fr->lvars = NULL;
    for (int i = 0; i < MAX_VAR; i++) {
	fr->variables[i].type = PROG_INTEGER;
	fr->variables[i].data.number = 0;
    }

    fr->brkpt.force_debugging = 0;
    fr->brkpt.debugging = 0;
    fr->brkpt.bypass = 0;
    fr->brkpt.isread = 0;
    fr->brkpt.showstack = 0;
    fr->brkpt.dosyspop = 0;
    fr->brkpt.lastline = 0;
    fr->brkpt.lastpc = 0;
    fr->brkpt.lastlisted = 0;
    fr->brkpt.lastcmd = NULL;
    fr->brkpt.breaknum = -1;

    fr->brkpt.lastproglisted = NOTHING;
    fr->brkpt.proglines = NULL;

    fr->brkpt.count = 1;
    fr->brkpt.temp[0] = 1;
    fr->brkpt.level[0] = -1;
    fr->brkpt.line[0] = -1;
    fr->brkpt.linecount[0] = -2;
    fr->brkpt.pc[0] = NULL;
    fr->brkpt.pccount[0] = -2;
    fr->brkpt.prog[0] = program;

    fr->proftime.tv_sec = 0;
    fr->proftime.tv_usec = 0;
    fr->totaltime.tv_sec = 0;
    fr->totaltime.tv_usec = 0;

    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = player;
    fr->variables[1].type = PROG_OBJECT;
    fr->variables[1].data.objref = location;
    fr->variables[2].type = PROG_OBJECT;
    fr->variables[2].data.objref = source;
    fr->variables[3].type = PROG_STRING;
    fr->variables[3].data.string = fr->cmd;
    if (fr->cmd)
	fr->cmd->links++;

    if (PROGRAM_CODE(program)) {
	PROGRAM_INC_PROF_USES(program);
    }
    PROGRAM_INC_INSTANCES(program);
    push(fr->argument.st, &(fr->argument.top), PROG_STRING, *match_args ?
	 MIPSCAST alloc_prog_string(match_args) : 0);
    return fr;
}

static int err;
static int already_created;

struct forvars *
copy_fors(struct forvars *forstack)
{
    struct forvars *out = NULL;
    struct forvars *nu;
    struct forvars *last = NULL;

    for (struct forvars *in = forstack; in; in = in->next) {
	if (!for_pool) {
	    nu = (struct forvars *) malloc(sizeof(struct forvars));
	} else {
	    nu = for_pool;
	    if (*last_for == for_pool->next) {
		last_for = &for_pool;
	    }
	    for_pool = nu->next;
	}

	nu->didfirst = in->didfirst;
	copyinst(&in->cur, &nu->cur);
	copyinst(&in->end, &nu->end);
	nu->step = in->step;
	nu->next = NULL;

	if (!out) {
	    last = out = nu;
	} else {
	    last->next = nu;
	    last = nu;
	}
    }
    return out;
}

struct forvars *
push_for(struct forvars *forstack)
{
    struct forvars *nu;

    if (!for_pool) {
	nu = (struct forvars *) malloc(sizeof(struct forvars));
    } else {
	nu = for_pool;
	if (*last_for == for_pool->next) {
	    last_for = &for_pool;
	}
	for_pool = nu->next;
    }
    nu->next = forstack;
    return nu;
}

struct forvars *
pop_for(struct forvars *forstack)
{
    struct forvars *newstack;

    if (!forstack) {
	return NULL;
    }
    newstack = forstack->next;
    forstack->next = for_pool;
    for_pool = forstack;
    if (last_for == &for_pool) {
	last_for = &(for_pool->next);
    }
    return newstack;
}

struct tryvars *
copy_trys(struct tryvars *trystack)
{
    struct tryvars *out = NULL;
    struct tryvars *nu;
    struct tryvars *last = NULL;

    for (struct tryvars *in = trystack; in; in = in->next) {
	if (!try_pool) {
	    nu = (struct tryvars *) malloc(sizeof(struct tryvars));
	} else {
	    nu = try_pool;
	    if (*last_try == try_pool->next) {
		last_try = &try_pool;
	    }
	    try_pool = nu->next;
	}

	nu->depth = in->depth;
	nu->call_level = in->call_level;
	nu->for_count = in->for_count;
	nu->addr = in->addr;
	nu->next = NULL;

	if (!out) {
	    last = out = nu;
	} else {
	    last->next = nu;
	    last = nu;
	}
    }
    return out;
}

struct tryvars *
push_try(struct tryvars *trystack)
{
    struct tryvars *nu;

    if (!try_pool) {
	nu = (struct tryvars *) malloc(sizeof(struct tryvars));
    } else {
	nu = try_pool;
	if (*last_try == try_pool->next) {
	    last_try = &try_pool;
	}
	try_pool = nu->next;
    }
    nu->next = trystack;
    return nu;
}

struct tryvars *
pop_try(struct tryvars *trystack)
{
    struct tryvars *newstack;

    if (!trystack) {
	return NULL;
    }
    newstack = trystack->next;
    trystack->next = try_pool;
    try_pool = trystack;
    if (last_try == &try_pool) {
	last_try = &(try_pool->next);
    }
    return newstack;
}

/* clean up lists from watchpid and sends event */
static void
watchpid_process(struct frame *fr)
{
    if (!fr) {
	log_status("WARNING: watchpid_process(): NULL frame passed !  Ignored.");
	return;
    }

    struct frame *frame;
    struct mufwatchpidlist *cur;
    struct inst temp1;
    temp1.type = PROG_INTEGER;
    temp1.data.number = fr->pid;

    while (fr->waitees) {
	cur = fr->waitees;
	fr->waitees = cur->next;

	frame = timequeue_pid_frame(cur->pid);
	free(cur);
	if (frame) {
	    for (struct mufwatchpidlist **curptr = &frame->waiters; *curptr; curptr = &(*curptr)->next) {
		if ((*curptr)->pid == fr->pid) {
		    cur = *curptr;
		    *curptr = (*curptr)->next;
		    free(cur);
		    break;
		}
	    }
	}
    }

    while (fr->waiters) {
	char buf[64];

	snprintf(buf, sizeof(buf), "PROC.EXIT.%d", fr->pid);

	cur = fr->waiters;
	fr->waiters = cur->next;

	frame = timequeue_pid_frame(cur->pid);
	free(cur);
	if (frame) {
	    muf_event_add(frame, buf, &temp1, 0);
	    for (struct mufwatchpidlist **curptr = &frame->waitees; *curptr; curptr = &(*curptr)->next) {
		if ((*curptr)->pid == fr->pid) {
		    cur = *curptr;
		    *curptr = (*curptr)->next;
		    free(cur);
		    break;
		}
	    }
	}
    }
}

/* clean up the stack. */
void
prog_clean(struct frame *fr)
{
    if (!fr) {
	log_status("WARNING: prog_clean(): Tried to free a NULL frame !  Ignored.");
	return;
    }

    for (struct frame *ptr = free_frames_list; ptr; ptr = ptr->next) {
	if (ptr == fr) {
	    log_status
		    ("WARNING: prog_clean(): tried to free an already freed program frame !  Ignored.");
	    return;
	}
    }

    watchpid_process(fr);

    fr->system.top = 0;
    for (int i = 0; i < fr->argument.top; i++) {
	CLEAR(&fr->argument.st[i]);
    }

    DEBUGPRINT("prog_clean: fr->caller.top=%d\n", fr->caller.top);
    for (int i = 1; i <= fr->caller.top; i++) {
	DEBUGPRINT("Decreasing instances of fr->caller.st[%d](#%d)\n", i, fr->caller.st[i]);
	PROGRAM_DEC_INSTANCES(fr->caller.st[i]);
    }

    for (int i = 0; i < MAX_VAR; i++)
	CLEAR(&fr->variables[i]);

    if (fr->cmd && --fr->cmd->links == 0)
	free((void *) fr->cmd);

    localvar_freeall(fr);
    scopedvar_freeall(fr);

    if (fr->fors.st) {
	struct forvars **loop = &(fr->fors.st);

	while (*loop) {
	    CLEAR(&((*loop)->cur));
	    CLEAR(&((*loop)->end));
	    loop = &((*loop)->next);
	}
	*loop = for_pool;
	if (last_for == &for_pool) {
	    last_for = loop;
	}
	for_pool = fr->fors.st;
	fr->fors.st = NULL;
	fr->fors.top = 0;
    }

    if (fr->trys.st) {
	struct tryvars **loop = &(fr->trys.st);

	while (*loop) {
	    loop = &((*loop)->next);
	}
	*loop = try_pool;
	if (last_try == &try_pool) {
	    last_try = loop;
	}
	try_pool = fr->trys.st;
	fr->trys.st = NULL;
	fr->trys.top = 0;
    }

    fr->argument.top = 0;
    fr->pc = 0;
    if (fr->brkpt.lastcmd)
	free(fr->brkpt.lastcmd);
    if (fr->brkpt.proglines) {
	free_prog_text(fr->brkpt.proglines);
	fr->brkpt.proglines = NULL;
    }

    if (fr->rndbuf)
	delete_seed(fr->rndbuf);
#ifdef MCP_SUPPORT
    muf_dlog_purge(fr);
#endif
    dequeue_timers(fr->pid, NULL);

    muf_event_purge(fr);
    fr->next = free_frames_list;
    free_frames_list = fr;
    err = 0;
}

static void
reload(struct frame *fr, int atop, int stop)
{
    assert(fr);
    fr->argument.top = atop;
    fr->system.top = stop;
}

int
false_inst(struct inst *p)
{
    return ((p->type == PROG_STRING && (!p->data.string || !(*p->data.string->data)))
	    || (p->type == PROG_MARK)
	    || (p->type == PROG_ARRAY && (!p->data.array || !p->data.array->items))
	    || (p->type == PROG_LOCK && p->data.lock == TRUE_BOOLEXP)
	    || (p->type == PROG_INTEGER && p->data.number == 0)
	    || (p->type == PROG_FLOAT && p->data.fnumber == 0.0)
	    || (p->type == PROG_OBJECT && p->data.objref == NOTHING));
}

void
copyinst(struct inst *from, struct inst *to)
{
    assert(from && to);
    int varcnt;
    *to = *from;
    switch (from->type) {
    case PROG_FUNCTION:
	if (from->data.mufproc) {
	    to->data.mufproc = (struct muf_proc_data *) malloc(sizeof(struct muf_proc_data));
	    to->data.mufproc->procname = strdup(from->data.mufproc->procname);
	    to->data.mufproc->vars = varcnt = from->data.mufproc->vars;
	    to->data.mufproc->args = from->data.mufproc->args;
	    to->data.mufproc->varnames = (const char **) calloc(varcnt, sizeof(const char *));
	    for (int j = 0; j < varcnt; j++) {
		to->data.mufproc->varnames[j] = strdup(from->data.mufproc->varnames[j]);
	    }
	}
	break;
    case PROG_STRING:
	if (from->data.string) {
	    from->data.string->links++;
	}
	break;
    case PROG_ARRAY:
	if (from->data.array) {
	    from->data.array->links++;
	}
	break;
    case PROG_ADD:
	from->data.addr->links++;
	PROGRAM_INC_INSTANCES(from->data.addr->progref);
	break;
    case PROG_LOCK:
	if (from->data.lock != TRUE_BOOLEXP) {
	    to->data.lock = copy_bool(from->data.lock);
	}
	break;
    }
}

static void
copyvars(vars * from, vars * to)
{
    assert(from && to);

    for (int i = 0; i < MAX_VAR; i++) {
	copyinst(&(*from)[i], &(*to)[i]);
    }
}

static void
calc_profile_timing(dbref prog, struct frame *fr)
{
    assert(fr);

    struct timeval tv;
    struct timeval tv2;

    gettimeofday(&tv, NULL);
    if (tv.tv_usec < fr->proftime.tv_usec) {
	tv.tv_usec += 1000000;
	tv.tv_sec -= 1;
    }
    tv.tv_usec -= fr->proftime.tv_usec;
    tv.tv_sec -= fr->proftime.tv_sec;
    tv2 = PROGRAM_PROFTIME(prog);
    tv2.tv_sec += tv.tv_sec;
    tv2.tv_usec += tv.tv_usec;
    if (tv2.tv_usec >= 1000000) {
	tv2.tv_usec -= 1000000;
	tv2.tv_sec += 1;
    }
    PROGRAM_SET_PROFTIME(prog, tv2.tv_sec, tv2.tv_usec);
    fr->totaltime.tv_sec += tv.tv_sec;
    fr->totaltime.tv_usec += tv.tv_usec;
    if (fr->totaltime.tv_usec > 1000000) {
	fr->totaltime.tv_usec -= 1000000;
	fr->totaltime.tv_sec += 1;
    }
}

static int interp_depth = 0;

static void
interp_err(dbref player, dbref program, struct inst *pc,
	   struct inst *arg, int atop, dbref origprog, const char *msg1, const char *msg2)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char tbuf[40];
    int errcount;
    time_t lt;

    err++;

    if (OWNER(origprog) == OWNER(player)) {
	notify_nolisten(player, "\033[1;31;40mProgram Error.  Your program just got the following error.\033[0m", 1);
    } else {
	notifyf_nolisten(player,
		 "\033[1;31;40mProgrammer Error.  Please tell %s what you typed, and the following message.\033[0m",
		 NAME(OWNER(origprog)));
    }

    notifyf_nolisten(player, "\033[1m%s(#%d), line %d; %s: %s\033[0m",
		     NAME(program), program, pc ? pc->line : -1, msg1, msg2);

    lt = time(NULL);
    strftime(tbuf, 32, "%c", MUCK_LOCALTIME(lt));

    strip_ansi(buf2, buf);
    errcount = get_property_value(origprog, MUF_ERRCOUNT_PROP);
    errcount++;
    add_property(origprog, MUF_ERRCOUNT_PROP, NULL, errcount);
    add_property(origprog, MUF_LASTERR_PROP, buf2, 0);
    add_property(origprog, MUF_LASTCRASH_PROP, NULL, (int) lt);
    add_property(origprog, MUF_LASTCRASHTIME_PROP, tbuf, 0);

    if (origprog != program) {
	errcount = get_property_value(program, MUF_ERRCOUNT_PROP);
	errcount++;
	add_property(program, MUF_ERRCOUNT_PROP, NULL, errcount);
	add_property(program, MUF_LASTERR_PROP, buf2, 0);
	add_property(program, MUF_LASTCRASH_PROP, NULL, (int) lt);
	add_property(program, MUF_LASTCRASHTIME_PROP, tbuf, 0);
    }
}

static void
do_abort_loop(dbref player, dbref program, const char *msg,
	      struct frame *fr, struct inst *pc, int atop, int stop,
	      struct inst *clinst1, struct inst *clinst2)
{
    if (!fr) {
	panic("localvars_get(): NULL frame passed !");
    }

    char buffer[128];

    if (fr->trys.top) {
	fr->errorstr = strdup(msg);
	if (pc) {
	    fr->errorinst =
		    strdup(insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program, 1));
	    fr->errorline = pc->line;
	} else {
	    fr->errorinst = NULL;
	    fr->errorline = -1;
	}
	fr->errorprog = program;
	err++;
    } else {
	if (pc) {
	    calc_profile_timing(program, fr);
	}
    }
    if (clinst1)
	CLEAR(clinst1);
    if (clinst2)
	CLEAR(clinst2);
    reload(fr, atop, stop);
    fr->pc = pc;
    if (!fr->trys.top) {
	if (pc) {
	    interp_err(player, program, pc, fr->argument.st, fr->argument.top,
		       fr->caller.st[1], insttotext(fr, 0, pc, buffer, sizeof(buffer), 30,
						    program, 1), msg);
	    if (controls(player, program))
		muf_backtrace(player, program, STACK_SIZE, fr);
	} else {
	    notify_nolisten(player, msg, 1);
	}
	interp_depth--;
	prog_clean(fr);
	PLAYER_SET_BLOCK(player, 0);
    }
}

struct inst *
interp_loop(dbref player, dbref program, struct frame *fr, int rettyp)
{
    register struct inst *pc;
    register int atop;
    register struct inst *arg;
    register struct inst *temp1;
    register struct inst *temp2;
    register struct stack_addr *sys;
    register int instr_count;
    register int stop;
    int i = 0, tmp, writeonly, mlev;
    static struct inst retval;
    char dbuf[BUFFER_LEN];
    int instno_debug_line = get_primitive("debug_line");


    fr->level = ++interp_depth;	/* increment interp level */

    /* load everything into local stuff */
    pc = fr->pc;
    atop = fr->argument.top;
    stop = fr->system.top;
    arg = fr->argument.st;
    sys = fr->system.st;
    writeonly = fr->writeonly;
    already_created = 0;
    fr->brkpt.isread = 0;

    if (!pc) {
	struct line *tmpline;

	tmpline = PROGRAM_FIRST(program);
	PROGRAM_SET_FIRST(program, (struct line *) read_program(program));
	do_compile(-1, OWNER(program), program, 0);
	free_prog_text(PROGRAM_FIRST(program));
	PROGRAM_SET_FIRST(program, tmpline);
	pc = fr->pc = PROGRAM_START(program);
	if (!pc) {
	    abort_loop_hard("Program not compilable. Cannot run.", NULL, NULL);
	}
	PROGRAM_INC_PROF_USES(program);
	PROGRAM_INC_INSTANCES(program);
    }
    ts_useobject(program);
    err = 0;

    instr_count = 0;
    mlev = ProgMLevel(program);
    gettimeofday(&fr->proftime, NULL);

    /* This is the 'natural' way to exit a function */
    while (stop) {

	/* Abort program if player/thing running it is recycled */
	if (!OkObj(player)) {
	    reload(fr, atop, stop);
	    prog_clean(fr);
	    interp_depth--;
	    calc_profile_timing(program, fr);

	    return NULL;
	}

	fr->instcnt++;
	instr_count++;

	if ((fr->multitask == PREEMPT) || (FLAGS(program) & BUILDER)) {
	    if (mlev == 4) {
		if (tp_max_ml4_preempt_count) {
		    if (instr_count >= tp_max_ml4_preempt_count)
			abort_loop_hard("Maximum preempt instruction count exceeded", NULL,
					NULL);
		} else
		    instr_count = 0;
	    } else {
		/* else make sure that the program doesn't run too long */
		if (instr_count >= tp_max_instr_count)
		    abort_loop_hard("Maximum preempt instruction count exceeded", NULL, NULL);
	    }
	} else {
	    /* if in FOREGROUND or BACKGROUND mode, '0 sleep' every so often. */
	    if ((fr->instcnt > tp_instr_slice * 4) && (instr_count >= tp_instr_slice)) {
		fr->pc = pc;
		reload(fr, atop, stop);
		PLAYER_SET_BLOCK(player, (!fr->been_background));
		add_muf_delay_event(0, fr->descr, player, NOTHING, NOTHING, program, fr,
				    (fr->multitask ==
				     FOREGROUND) ? "FOREGROUND" : "BACKGROUND");
		interp_depth--;
		calc_profile_timing(program, fr);
		return NULL;
	    }
	}
	if (((FLAGS(program) & ZMUF_DEBUGGER) || fr->brkpt.force_debugging) &&
	    !fr->been_background && controls(player, program)
		) {
	    fr->brkpt.debugging = 1;
	} else {
	    fr->brkpt.debugging = 0;
	}
	if (FLAGS(program) & DARK ||
	    (fr->brkpt.debugging && fr->brkpt.showstack && !fr->brkpt.bypass)) {

	    if ((pc->type != PROG_PRIMITIVE) || (pc->data.number != instno_debug_line)) {
		char *m =
			debug_inst(fr, 0, pc, fr->pid, arg, dbuf, sizeof(dbuf), atop, program);

		notify_nolisten(player, m, 1);
	    }
	}
	if (fr->brkpt.debugging) {
	    short breakflag = 0;
	    if (stop == 1 &&
		!fr->brkpt.bypass && pc->type == PROG_PRIMITIVE && pc->data.number == IN_RET) {
		/* Program is about to EXIT */
		notify_nolisten(player, "Program is about to EXIT.", 1);
		breakflag = 1;
	    } else if (fr->brkpt.count) {
		for (i = 0; i < fr->brkpt.count; i++) {
		    if ((!fr->brkpt.pc[i] || pc == fr->brkpt.pc[i]) &&
			/* pc matches */
			(fr->brkpt.line[i] == -1 ||
			 (fr->brkpt.lastline != pc->line && fr->brkpt.line[i] == pc->line)) &&
			/* line matches */
			(fr->brkpt.level[i] == -1 || stop <= fr->brkpt.level[i]) &&
			/* level matches */
			(fr->brkpt.prog[i] == NOTHING || fr->brkpt.prog[i] == program) &&
			/* program matches */
			(fr->brkpt.linecount[i] == -2 ||
			 (fr->brkpt.lastline != pc->line && fr->brkpt.linecount[i]-- <= 0)) &&
			/* line count matches */
			(fr->brkpt.pccount[i] == -2 ||
			 (fr->brkpt.lastpc != pc && fr->brkpt.pccount[i]-- <= 0))
			/* pc count matches */
			    ) {
			if (fr->brkpt.bypass) {
			    if (fr->brkpt.pccount[i] == -1)
				fr->brkpt.pccount[i] = 0;
			    if (fr->brkpt.linecount[i] == -1)
				fr->brkpt.linecount[i] = 0;
			} else {
			    breakflag = 1;
			    break;
			}
		    }
		}
	    }
	    if (breakflag) {
		char *m;

		if (fr->brkpt.dosyspop) {
		    program = sys[--stop].progref;
		    pc = sys[stop].offset;
		}
		add_muf_read_event(fr->descr, player, program, fr);
		reload(fr, atop, stop);
		fr->pc = pc;
		fr->brkpt.isread = 0;
		fr->brkpt.breaknum = i;
		fr->brkpt.lastlisted = 0;
		fr->brkpt.bypass = 0;
		fr->brkpt.dosyspop = 0;
		PLAYER_SET_CURR_PROG(player, program);
		PLAYER_SET_BLOCK(player, 0);
		interp_depth--;
		if (!fr->brkpt.showstack) {
		    m = debug_inst(fr, 0, pc, fr->pid, arg, dbuf, sizeof(dbuf), atop, program);
		    notify_nolisten(player, m, 1);
		}
		if (pc <= PROGRAM_CODE(program) || (pc - 1)->line != pc->line) {
		    list_proglines(player, program, fr, pc->line, 0);
		} else {
		    m = show_line_prims(program, pc, 15, 1);
		    notifyf_nolisten(player, "     %s", m);
		}
		calc_profile_timing(program, fr);
		return NULL;
	    }
	    fr->brkpt.lastline = pc->line;
	    fr->brkpt.lastpc = pc;
	    fr->brkpt.bypass = 0;
	}
	if (mlev < 3) {
	    if (fr->instcnt > (tp_max_instr_count * ((mlev == 2) ? 4 : 1)))
		abort_loop_hard("Maximum total instruction count exceeded.", NULL, NULL);
	}
	switch (pc->type) {
	case PROG_INTEGER:
	case PROG_FLOAT:
	case PROG_ADD:
	case PROG_OBJECT:
	case PROG_VAR:
	case PROG_LVAR:
	case PROG_SVAR:
	case PROG_STRING:
	case PROG_LOCK:
	case PROG_MARK:
	case PROG_ARRAY:
	    if (atop >= STACK_SIZE)
		abort_loop("Stack overflow.", NULL, NULL);
	    copyinst(pc, arg + atop);
	    pc++;
	    atop++;
	    break;

	case PROG_LVAR_AT:
	case PROG_LVAR_AT_CLEAR:
	    {
		struct inst *tmp;
		struct localvars *lv;

		if (atop >= STACK_SIZE)
		    abort_loop("Stack overflow.", NULL, NULL);

		if (pc->data.number >= MAX_VAR || pc->data.number < 0)
		    abort_loop("Scoped variable number out of range.", NULL, NULL);

		lv = localvars_get(fr, program);
		tmp = &(lv->lvars[pc->data.number]);

		copyinst(tmp, arg + atop);

		if (pc->type == PROG_LVAR_AT_CLEAR) {
		    CLEAR(tmp);
		    tmp->type = PROG_INTEGER;
		    tmp->data.number = 0;
		}

		pc++;
		atop++;
	    }
	    break;

	case PROG_LVAR_BANG:
	    {
		struct inst *the_var;
		struct localvars *lv;
		if (atop < 1)
		    abort_loop("Stack Underflow.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);

		if (pc->data.number >= MAX_VAR || pc->data.number < 0)
		    abort_loop("Scoped variable number out of range.", NULL, NULL);

		lv = localvars_get(fr, program);
		the_var = &(lv->lvars[pc->data.number]);

		CLEAR(the_var);
		temp1 = arg + --atop;
		*the_var = *temp1;
		pc++;
	    }
	    break;

	case PROG_SVAR_AT:
	case PROG_SVAR_AT_CLEAR:
	    {
		struct inst *tmp;

		if (atop >= STACK_SIZE)
		    abort_loop("Stack overflow.", NULL, NULL);

		tmp = scopedvar_get(fr, 0, pc->data.number);
		if (!tmp)
		    abort_loop("Scoped variable number out of range.", NULL, NULL);

		copyinst(tmp, arg + atop);

		if (pc->type == PROG_SVAR_AT_CLEAR) {
		    CLEAR(tmp);

		    tmp->type = PROG_INTEGER;
		    tmp->data.number = 0;
		}

		pc++;
		atop++;
	    }
	    break;

	case PROG_SVAR_BANG:
	    {
		struct inst *the_var;
		if (atop < 1)
		    abort_loop("Stack Underflow.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);

		the_var = scopedvar_get(fr, 0, pc->data.number);
		if (!the_var)
		    abort_loop("Scoped variable number out of range.", NULL, NULL);

		CLEAR(the_var);
		temp1 = arg + --atop;
		*the_var = *temp1;
		pc++;
	    }
	    break;

	case PROG_FUNCTION:
	    {
		int i = pc->data.mufproc->args;
		if (atop < i)
		    abort_loop("Stack Underflow.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < i)
		    abort_loop("Stack protection fault.", NULL, NULL);
		if (fr->skip_declare)
		    fr->skip_declare = 0;
		else
		    scopedvar_addlevel(fr, pc, pc->data.mufproc->vars);
		while (i-- > 0) {
		    struct inst *tmp;
		    temp1 = arg + --atop;
		    tmp = scopedvar_get(fr, 0, i);
		    if (!tmp)
			abort_loop_hard
				("Internal error: Scoped variable number out of range in FUNCTION init.",
				 temp1, NULL);
		    CLEAR(tmp);
		    copyinst(temp1, tmp);
		    CLEAR(temp1);
		}
		pc++;
	    }
	    break;

	case PROG_IF:
	    if (atop < 1)
		abort_loop("Stack Underflow.", NULL, NULL);
	    if (fr->trys.top && atop - fr->trys.st->depth < 1)
		abort_loop("Stack protection fault.", NULL, NULL);
	    temp1 = arg + --atop;
	    if (false_inst(temp1))
		pc = pc->data.call;
	    else
		pc++;
	    CLEAR(temp1);
	    break;

	case PROG_EXEC:
	    if (stop >= STACK_SIZE)
		abort_loop("System Stack Overflow", NULL, NULL);
	    sys[stop].progref = program;
	    sys[stop++].offset = pc + 1;
	    pc = pc->data.call;
	    fr->skip_declare = 0;	/* Make sure we DON'T skip var decls */
	    break;

	case PROG_JMP:
	    /* Don't need to worry about skipping scoped var decls here. */
	    /* JMP to a function header can only happen in IN_JMP */
	    pc = pc->data.call;
	    break;

	case PROG_TRY:
	    if (atop < 1)
		abort_loop("Stack Underflow.", NULL, NULL);
	    if (fr->trys.top && atop - fr->trys.st->depth < 1)
		abort_loop("Stack protection fault.", NULL, NULL);
	    temp1 = arg + --atop;
	    if (temp1->type != PROG_INTEGER || temp1->data.number < 0)
		abort_loop("Argument is not a positive integer.", temp1, NULL);
	    if (fr->trys.top && atop - fr->trys.st->depth < temp1->data.number)
		abort_loop("Stack protection fault.", NULL, NULL);
	    if (temp1->data.number > atop)
		abort_loop("Stack Underflow.", temp1, NULL);

	    fr->trys.top++;
	    fr->trys.st = push_try(fr->trys.st);
	    fr->trys.st->depth = atop - temp1->data.number;
	    fr->trys.st->call_level = stop;
	    fr->trys.st->for_count = 0;
	    fr->trys.st->addr = pc->data.call;

	    pc++;
	    CLEAR(temp1);
	    break;

	case PROG_PRIMITIVE:
	    /*
	     * All pc modifiers and stuff like that should stay here,
	     * everything else call with an independent dispatcher.
	     */
	    switch (pc->data.number) {
	    case IN_JMP:
		if (atop < 1)
		    abort_loop("Stack underflow.  Missing address.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);
		temp1 = arg + --atop;
		if (temp1->type != PROG_ADD)
		    abort_loop("Argument is not an address.", temp1, NULL);
		if (!ObjExists(temp1->data.addr->progref) ||
		    Typeof(temp1->data.addr->progref) != TYPE_PROGRAM)
		    abort_loop_hard("Internal error.  Invalid address.", temp1, NULL);
		if (program != temp1->data.addr->progref) {
		    abort_loop("Destination outside current program.", temp1, NULL);
		}
		if (temp1->data.addr->data->type == PROG_FUNCTION) {
		    fr->skip_declare = 1;
		}
		pc = temp1->data.addr->data;
		CLEAR(temp1);
		break;

	    case IN_EXECUTE:
		if (atop < 1)
		    abort_loop("Stack Underflow. Missing address.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);
		temp1 = arg + --atop;
		if (temp1->type != PROG_ADD)
		    abort_loop("Argument is not an address.", temp1, NULL);
		if (!ObjExists(temp1->data.addr->progref) ||
		    Typeof(temp1->data.addr->progref) != TYPE_PROGRAM)
		    abort_loop_hard("Internal error.  Invalid address.", temp1, NULL);
		if (stop >= STACK_SIZE)
		    abort_loop("System Stack Overflow", temp1, NULL);
		sys[stop].progref = program;
		sys[stop++].offset = pc + 1;
		if (program != temp1->data.addr->progref) {
		    program = temp1->data.addr->progref;
		    fr->caller.st[++fr->caller.top] = program;
		    mlev = ProgMLevel(program);
		    PROGRAM_INC_INSTANCES(program);
		}
		pc = temp1->data.addr->data;
		CLEAR(temp1);
		break;

	    case IN_CALL:
		if (atop < 1)
		    abort_loop("Stack Underflow. Missing dbref argument.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);
		temp1 = arg + --atop;
		temp2 = 0;
		if (temp1->type != PROG_OBJECT) {
		    temp2 = temp1;
		    if (atop < 1)
			abort_loop("Stack Underflow. Missing dbref of func.", temp1, NULL);
		    if (fr->trys.top && atop - fr->trys.st->depth < 1)
			abort_loop("Stack protection fault.", NULL, NULL);
		    temp1 = arg + --atop;
		    if (temp2->type != PROG_STRING)
			abort_loop("Public Func. name string required. (2)", temp1, temp2);
		    if (!temp2->data.string)
			abort_loop("Null string not allowed. (2)", temp1, temp2);
		}
		if (temp1->type != PROG_OBJECT)
		    abort_loop("Dbref required. (1)", temp1, temp2);
		if (!valid_object(temp1)
		    || Typeof(temp1->data.objref) != TYPE_PROGRAM)
		    abort_loop("Invalid object.", temp1, temp2);
		if (!(PROGRAM_CODE(temp1->data.objref))) {
		    struct line *tmpline;

		    tmpline = PROGRAM_FIRST(temp1->data.objref);
		    PROGRAM_SET_FIRST(temp1->data.objref,
				      (struct line *) read_program(temp1->data.objref));
		    do_compile(-1, OWNER(temp1->data.objref), temp1->data.objref, 0);
		    free_prog_text(PROGRAM_FIRST(temp1->data.objref));
		    PROGRAM_SET_FIRST(temp1->data.objref, tmpline);
		    if (!(PROGRAM_CODE(temp1->data.objref)))
			abort_loop("Program not compilable.", temp1, temp2);
		}
		if (ProgMLevel(temp1->data.objref) == 0)
		    abort_loop("Permission denied", temp1, temp2);
		if (mlev < 4 && OWNER(temp1->data.objref) != ProgUID
		    && !Linkable(temp1->data.objref))
		    abort_loop("Permission denied", temp1, temp2);
		if (stop >= STACK_SIZE)
		    abort_loop("System Stack Overflow", temp1, temp2);
		sys[stop].progref = program;
		sys[stop].offset = pc + 1;
		if (!temp2) {
		    pc = PROGRAM_START(temp1->data.objref);
		} else {
		    struct publics *pbs;
		    int tmpint;

		    pbs = PROGRAM_PUBS(temp1->data.objref);
		    while (pbs) {
			tmpint = strcasecmp(temp2->data.string->data, pbs->subname);
			if (!tmpint)
			    break;
			pbs = pbs->next;
		    }
		    if (!pbs)
			abort_loop("PUBLIC or WIZCALL function not found. (2)", temp2, temp2);
		    if (mlev < pbs->mlev)
			abort_loop("Insufficient permissions to call WIZCALL function. (2)",
				   temp2, temp2);
		    pc = pbs->addr.ptr;
		}
		stop++;
		if (temp1->data.objref != program) {
		    calc_profile_timing(program, fr);
		    gettimeofday(&fr->proftime, NULL);
		    program = temp1->data.objref;
		    fr->caller.st[++fr->caller.top] = program;
		    PROGRAM_INC_INSTANCES(program);
		    mlev = ProgMLevel(program);
		}
		PROGRAM_INC_PROF_USES(program);
		ts_useobject(program);
		CLEAR(temp1);
		if (temp2)
		    CLEAR(temp2);
		break;

	    case IN_RET:
		if (stop > 1 && program != sys[stop - 1].progref) {
		    if (!ObjExists(sys[stop - 1].progref) ||
			Typeof(sys[stop - 1].progref) != TYPE_PROGRAM)
			abort_loop_hard("Internal error.  Invalid address.", NULL, NULL);
		    calc_profile_timing(program, fr);
		    gettimeofday(&fr->proftime, NULL);
		    PROGRAM_DEC_INSTANCES(program);
		    program = sys[stop - 1].progref;
		    mlev = ProgMLevel(program);
		    fr->caller.top--;
		}
		scopedvar_poplevel(fr);
		pc = sys[--stop].offset;
		break;

	    case IN_CATCH:
	    case IN_CATCH_DETAILED:
		{
		    int depth;

		    if (!(fr->trys.top))
			abort_loop_hard("Internal error.  TRY stack underflow.", NULL, NULL);

		    depth = fr->trys.st->depth;
		    while (atop > depth) {
			temp1 = arg + --atop;
			CLEAR(temp1);
		    }

		    while (fr->trys.st->for_count-- > 0) {
			CLEAR(&fr->fors.st->cur);
			CLEAR(&fr->fors.st->end);
			fr->fors.top--;
			fr->fors.st = pop_for(fr->fors.st);
		    }

		    fr->trys.top--;
		    fr->trys.st = pop_try(fr->trys.st);

		    if (pc->data.number == IN_CATCH) {
			/* IN_CATCH */
			if (fr->errorstr) {
			    arg[atop].type = PROG_STRING;
			    arg[atop++].data.string = alloc_prog_string(fr->errorstr);
			    free(fr->errorstr);
			    fr->errorstr = NULL;
			} else {
			    arg[atop].type = PROG_STRING;
			    arg[atop++].data.string = NULL;
			}
			if (fr->errorinst) {
			    free(fr->errorinst);
			    fr->errorinst = NULL;
			}
		    } else {
			/* IN_CATCH_DETAILED */
			stk_array *nu = new_array_dictionary();
			if (fr->errorstr) {
			    array_set_strkey_strval(&nu, "error", fr->errorstr);
			    free(fr->errorstr);
			    fr->errorstr = NULL;
			}
			if (fr->errorinst) {
			    array_set_strkey_strval(&nu, "instr", fr->errorinst);
			    free(fr->errorinst);
			    fr->errorinst = NULL;
			}
			array_set_strkey_intval(&nu, "line", fr->errorline);
			array_set_strkey_refval(&nu, "program", fr->errorprog);
			arg[atop].type = PROG_ARRAY;
			arg[atop++].data.array = nu;
		    }
		    reload(fr, atop, stop);
		}
		pc++;
		break;

	    case IN_EVENT_WAITFOR:
		if (atop < 1)
		    abort_loop("Stack Underflow. Missing eventID list array argument.", NULL,
			       NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);
		temp1 = arg + --atop;
		if (temp1->type != PROG_ARRAY)
		    abort_loop("EventID string list array expected.", temp1, NULL);
		if (temp1->data.array && temp1->data.array->type != ARRAY_PACKED)
		    abort_loop("Argument must be a list array of eventid strings.", temp1,
			       NULL);
		if (!array_is_homogenous(temp1->data.array, PROG_STRING))
		    abort_loop("Argument must be a list array of eventid strings.", temp1,
			       NULL);
		fr->pc = pc + 1;
		reload(fr, atop, stop);

		{
		    int i, outcount;
		    int count = array_count(temp1->data.array);
		    char **events = (char **) malloc(count * sizeof(char **));
		    for (outcount = i = 0; i < count; i++) {
			char *val = array_get_intkey_strval(temp1->data.array, i);
			if (val != NULL) {
			    int found = 0;
			    for (int j = 0; j < outcount; j++) {
				if (!strcmp(events[j], val)) {
				    found = 1;
				    break;
				}
			    }
			    if (!found) {
				events[outcount++] = val;
			    }
			}
		    }
		    muf_event_register_specific(player, program, fr, outcount, events);
		    free(events);
		}

		PLAYER_SET_BLOCK(player, (!fr->been_background));
		CLEAR(temp1);
		interp_depth--;
		calc_profile_timing(program, fr);
		return NULL;

	    case IN_READ:
		if (writeonly)
		    abort_loop("Program is write-only.", NULL, NULL);
		if (fr->multitask == BACKGROUND)
		    abort_loop("BACKGROUND programs are write only.", NULL, NULL);
		reload(fr, atop, stop);
		fr->brkpt.isread = 1;
		fr->pc = pc + 1;
		PLAYER_SET_CURR_PROG(player, program);
		PLAYER_SET_BLOCK(player, 0);
		add_muf_read_event(fr->descr, player, program, fr);
		interp_depth--;
		calc_profile_timing(program, fr);
		return NULL;

	    case IN_SLEEP:
		if (atop < 1)
		    abort_loop("Stack Underflow.", NULL, NULL);
		if (fr->trys.top && atop - fr->trys.st->depth < 1)
		    abort_loop("Stack protection fault.", NULL, NULL);
		temp1 = arg + --atop;
		if (temp1->type != PROG_INTEGER)
		    abort_loop("Invalid argument type.", temp1, NULL);
		fr->pc = pc + 1;
		reload(fr, atop, stop);
		if (temp1->data.number < 0)
		    abort_loop("Timetravel beyond scope of muf.", temp1, NULL);
		add_muf_delay_event(temp1->data.number, fr->descr, player,
				    NOTHING, NOTHING, program, fr, "SLEEPING");
		PLAYER_SET_BLOCK(player, (!fr->been_background));
		interp_depth--;
		calc_profile_timing(program, fr);
		return NULL;

	    default:
		nargs = 0;
                fprintf(stderr, "about to run %s\n", base_inst[pc->data.number - 1]);
#ifdef DEBUG
                expect_pop = actual_pop = 0;
                expect_push_to = -1;
#endif
		reload(fr, atop, stop);
		tmp = atop;
                PROGRAM_INC_INSTANCES_IN_PRIMITIVE(program);
		prim_func[pc->data.number - 1] (player, program, mlev, pc, arg, &tmp, fr);
                PROGRAM_DEC_INSTANCES_IN_PRIMITIVE(program);
#ifdef DEBUG
                assert(expect_pop == actual_pop || err);
                assert(expect_push_to == -1 || expect_push_to == tmp || err);
                assert(expect_push_to != -1 || tmp <= atop);
#endif
		atop = tmp;
		pc++;
		break;
	    }			/* switch */
	    break;
	case PROG_CLEARED:
	    log_status
		    ("WARNING: attempt to execute instruction cleared by %s:%hd in program %d",
		     (char *) pc->data.addr, pc->line, program);
	    pc = NULL;
	    abort_loop_hard("Program internal error. Program erroneously freed from memory.",
			    NULL, NULL);
	default:
	    pc = NULL;
	    abort_loop_hard("Program internal error. Unknown instruction type.", NULL, NULL);
	}			/* switch */
	if (err) {
	    if (err != ERROR_DIE_NOW && fr->trys.top) {
		while (fr->trys.st->call_level < stop) {
		    if (stop > 1 && program != sys[stop - 1].progref) {
			if (!ObjExists(sys[stop - 1].progref) ||
			    Typeof(sys[stop - 1].progref) != TYPE_PROGRAM)
			    abort_loop_hard("Internal error.  Invalid address.", NULL, NULL);
			calc_profile_timing(program, fr);
			gettimeofday(&fr->proftime, NULL);
			PROGRAM_DEC_INSTANCES(program);
			program = sys[stop - 1].progref;
			mlev = ProgMLevel(program);
			fr->caller.top--;
		    }
		    scopedvar_poplevel(fr);
		    stop--;
		}

		pc = fr->trys.st->addr;
		err = 0;
	    } else {
		reload(fr, atop, stop);
		prog_clean(fr);
		PLAYER_SET_BLOCK(player, 0);
		interp_depth--;
		calc_profile_timing(program, fr);
		return NULL;
	    }
	}
    }				/* while */

    PLAYER_SET_BLOCK(player, 0);
    if (atop) {
	struct inst *rv;

	if (rettyp) {
	    copyinst(arg + atop - 1, &retval);
	    rv = &retval;
	} else {
	    if (!false_inst(arg + atop - 1)) {
		rv = (struct inst *) 1;
	    } else {
		rv = NULL;
	    }
	}
	reload(fr, atop, stop);
	prog_clean(fr);
	interp_depth--;
	calc_profile_timing(program, fr);
	return rv;
    }
    reload(fr, atop, stop);
    prog_clean(fr);
    interp_depth--;
    calc_profile_timing(program, fr);
    return NULL;
}

void
push(struct inst *stack, int *top, int type, voidptr res)
{
    stack[*top].type = type;
    if (type == PROG_FLOAT)
	stack[*top].data.fnumber = *(double *) res;
    else if (type < PROG_STRING)
	stack[*top].data.number = *(int *) res;
    else
	stack[*top].data.string = (struct shared_string *) res;
    (*top)++;
}

int
valid_player(struct inst *oper)
{
    return (!(oper->type != PROG_OBJECT || !ObjExists(oper->data.objref)
	      || (Typeof(oper->data.objref) != TYPE_PLAYER)));
}

int
valid_object(struct inst *oper)
{
    return (!(oper->type != PROG_OBJECT || !ObjExists(oper->data.objref)
	      || Typeof(oper->data.objref) == TYPE_GARBAGE));
}

int
is_home(struct inst *oper)
{
    return (oper->type == PROG_OBJECT && oper->data.objref == HOME);
}

int
permissions(dbref player, dbref thing)
{

    if (thing == player || thing == HOME)
	return 1;

    switch (Typeof(thing)) {
    case TYPE_PLAYER:
	return 0;
    case TYPE_EXIT:
	return (OWNER(thing) == OWNER(player) || OWNER(thing) == NOTHING);
    case TYPE_ROOM:
    case TYPE_THING:
    case TYPE_PROGRAM:
	return (OWNER(thing) == OWNER(player));
    }

    return 0;
}

dbref
find_mlev(dbref prog, struct frame * fr, int st)
{
    if ((FLAGS(prog) & STICKY) && (FLAGS(prog) & HAVEN)) {
	if ((st > 1) && (TrueWizard(OWNER(prog))))
	    return (find_mlev(fr->caller.st[st - 1], fr, st - 1));
    }
    if (MLevel(prog) < MLevel(OWNER(prog))) {
	return (MLevel(prog));
    } else {
	return (MLevel(OWNER(prog)));
    }
}

dbref
find_uid(dbref player, struct frame * fr, int st, dbref program)
{
    if ((FLAGS(program) & STICKY) || (fr->perms == STD_SETUID)) {
	if (FLAGS(program) & HAVEN) {
	    if ((st > 1) && (TrueWizard(OWNER(program))))
		return (find_uid(player, fr, st - 1, fr->caller.st[st - 1]));
	    return (OWNER(program));
	}
	return (OWNER(program));
    }
    if (ProgMLevel(program) < 2)
	return (OWNER(program));
    if ((FLAGS(program) & HAVEN) || (fr->perms == STD_HARDUID)) {
	if (fr->trig == NOTHING)
	    return (OWNER(program));
	return (OWNER(fr->trig));
    }
    return (OWNER(player));
}

void
do_abort_interp(dbref player, const char *msg, struct inst *pc,
		struct inst *arg, int atop, struct frame *fr,
		struct inst *oper1, struct inst *oper2,
		struct inst *oper3, struct inst *oper4, int nargs,
		dbref program, char *file, int line)
{
    char buffer[128];

    if (fr->trys.top) {
	fr->errorstr = strdup(msg);
	if (pc) {
	    fr->errorinst =
		    strdup(insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program, 1));
	    fr->errorline = pc->line;
	} else {
	    fr->errorinst = NULL;
	    fr->errorline = -1;
	}
	fr->errorprog = program;
	err++;
    } else {
	fr->pc = pc;
	calc_profile_timing(program, fr);
	interp_err(player, program, pc, arg, atop, fr->caller.st[1],
		   insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program, 1), msg);
	if (controls(player, program))
	    muf_backtrace(player, program, STACK_SIZE, fr);
    }
    switch (nargs) {
    case 4:
	RCLEAR(oper4, file, line);
    case 3:
	RCLEAR(oper3, file, line);
    case 2:
	RCLEAR(oper2, file, line);
    case 1:
	RCLEAR(oper1, file, line);
    }
    return;
}

/*
 * Errors set with this will not be caught.
 *
 * This will always result in program termination the next time
 * interp_loop() checks for this.
 */
void
do_abort_silent(void)
{
    err = ERROR_DIE_NOW;
}

char *
debug_inst(struct frame *fr, int lev, struct inst *pc, int pid, struct inst *stack,
           char *buffer, int buflen, int sp, dbref program)
{
    char *bend;
    char *bstart;
    char *ptr;
    int length;

    char buf2[BUFFER_LEN];
    /* To hold Debug> ... at the beginning */
    char buf3[64];
    int count;

    assert(buflen > 1);

    buffer[buflen - 1] = '\0';

    length = snprintf(buf3, sizeof(buf3), "Debug> Pid %d: #%d %d (", pid, program, pc->line);

    if (length == -1) {
        length = sizeof(buf3) - 1;
    }
    bstart = buffer + length;   /* start far enough away so we can fit Debug> #xxx xxx ( thingy. */
    length = buflen - length - 1;       /* - 1 for the '\0' */
    bend = buffer + (buflen - 1);       /* - 1 for the '\0' */

    /* + 10 because we must at least be able to store " ... ) ..." after that. */
    if (bstart + 10 > bend) {   /* we have no room. Eeek! */
        /*                                  123456789012345678 */
        memcpy((void *) buffer, (const void *) "Need buffer space!",
               (buflen - 1 > 18) ? 18 : buflen - 1);
        return buffer;
    }


    /* We use this if-else structure to handle errors and such nicely. */
    /* We use length - 7 so we KNOW we'll have room for " ... ) " */

    ptr = insttotext(fr, lev, pc, buf2, length - 7, 30, program, 1);
    if (*ptr) {
        length -= prepend_string(&bend, bstart, ptr);
    } else {
        strcpyn(buffer, buflen, buf3);
        strcatn(buffer, buflen, " ... ) ...");
        return buffer;
    }

    length -= prepend_string(&bend, bstart, ") ");

    count = sp - 1;
    if (count >= 0) {
        for (;;) {
            if (count && length <= 5) {
                prepend_string(&bend, bstart, "...");
                break;
            }
            /* we use length - 5 to leave room for "..., "
             * ... except if we're outputing the last item (count == 0) */
            ptr = insttotext(fr, lev, stack + count, buf2, (count) ? length - 5 : length, 30,
                             program, 1);
            if (*ptr) {
                length -= prepend_string(&bend, bstart, ptr);
            } else {
                break;          /* done because we couldn't display all that */
            }
            if (count > 0 && count > sp - 8) {
                length -= prepend_string(&bend, bstart, ", ");
            } else {
                break;          /* all done! */
            }
            count--;
        }
    }

    /* we don't use bstart, because it's compensated for the length of this. */
    prepend_string(&bend, buffer, buf3);
    /* and return the pointer to the beginning of our backwards grown string. */
    return bend;
}
