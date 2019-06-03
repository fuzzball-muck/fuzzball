#ifndef TIMEQUEUE_H
#define TIMEQUEUE_H

#include <time.h>

#include "array.h"
#include "config.h"
#include "interp.h"

int add_mpi_event(int delay, int descr, dbref player, dbref loc, dbref trig,
                         const char *mpi, const char *cmdstr, const char *argstr, int listen_p,
                         int omesg_p, int bless_p);
int add_muf_read_event(int descr, dbref player, dbref prog, struct frame *fr);
int add_muf_delay_event(int delay, int descr, dbref player, dbref loc, dbref trig,
                               dbref prog, struct frame *fr, const char *mode);
int add_muf_delayq_event(int delay, int descr, dbref player, dbref loc, dbref trig,
                                dbref prog, const char *argstr, const char *cmdstr,
                                int listen_p);
int add_muf_timer_event(int descr, dbref player, dbref prog, struct frame *fr,
                               int delay, char *id);
int add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig, dbref prog,
                               const char *argstr, const char *cmdstr, int listen_p);
int control_process(dbref player, int procnum);
int dequeue_process(int procnum);
int dequeue_prog_real(dbref, int, const char *, const int);
#define dequeue_prog(x,i) dequeue_prog_real(x,i,__FILE__,__LINE__)
int dequeue_timers(int procnum, char *timerid);
void envpropqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
                         dbref xclude, const char *propname, const char *toparg, int mlev,
                         int mt);
stk_array *get_pidinfo(int pid, int pin);
stk_array *get_pids(dbref ref, int pin);
void handle_read_event(int descr, dbref player, const char *command);
void listenqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
                        dbref xclude, const char *propname, const char *toparg, int mlev,
                        int mt, int mpi_p);
int in_timequeue(int pid);
time_t next_event_time(time_t now);
void next_timequeue_event(time_t now);
void propqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
                      dbref xclude, const char *propname, const char *toparg, int mlev,
                      int mt);
void purge_timenode_free_pool(void);
int read_event_notify(int descr, dbref player, const char *cmd);
int scan_instances(dbref program);
struct frame *timequeue_pid_frame(int pid);

#endif /* !TIMEQUEUE_H */
