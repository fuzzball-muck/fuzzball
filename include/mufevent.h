#ifndef _MUFEVENT_H
#define _MUFEVENT_H

#include "array.h"

stk_array *get_mufevent_pids(stk_array * nw, dbref ref);
stk_array *get_mufevent_pidinfo(stk_array * nw, int pid);
void muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive);
int muf_event_controls(dbref player, int pid);
int muf_event_count(struct frame *fr);
int muf_event_dequeue(dbref prog, int sleeponly);
int muf_event_dequeue_pid(int pid);
int muf_event_exists(struct frame *fr, const char *eventid);
int muf_event_list(dbref player, const char *pat);
struct frame *muf_event_pid_frame(int pid);
void muf_event_process(void);
void muf_event_purge(struct frame *fr);
int muf_event_read_notify(int descr, dbref player, const char *cmd);
void muf_event_register_specific(dbref player, dbref prog, struct frame *fr, size_t eventcount,
				 char **eventids);

#endif				/* _MUFEVENT_H */
