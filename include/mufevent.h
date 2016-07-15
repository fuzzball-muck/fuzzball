#ifndef _MUFEVENT_H
#define _MUFEVENT_H

#include "array.h"

struct mufevent {
    struct mufevent *next;
    char *event;
    struct inst data;
};

static struct mufevent_process {
    struct mufevent_process *prev, *next;
    dbref player;
    dbref prog;
    short filtercount;
    short deleted;
    char **filters;
    struct frame *fr;
} *mufevent_processes;

#define MUFEVENT_ALL	-1
#define MUFEVENT_FIRST	-2
#define MUFEVENT_LAST	-3

int muf_event_dequeue(dbref prog, int sleeponly);
int muf_event_dequeue_pid(int pid);
struct frame *muf_event_pid_frame(int pid);
int muf_event_controls(dbref player, int pid);
void muf_event_register(dbref player, dbref prog, struct frame *fr);
void muf_event_register_specific(dbref player, dbref prog, struct frame *fr, int eventcount,
				 char **eventids);
int muf_event_read_notify(int descr, dbref player, const char *cmd);
int muf_event_list(dbref player, const char *pat);
void muf_event_remove(struct frame *fr, char *event, int which);
void muf_event_purge(struct frame *fr);
void muf_event_process(void);
stk_array *get_mufevent_pids(stk_array * nw, dbref ref);
stk_array *get_mufevent_pidinfo(stk_array * nw, int pid);

void muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive);
int muf_event_count(struct frame *fr);
int muf_event_exists(struct frame *fr, const char *eventid);

#endif				/* _MUFEVENT_H */
