#ifndef P_CONNECTS_H
#define P_CONNECTS_H

#include "interp.h"

void prim_awakep(PRIM_PROTOTYPE);
void prim_online(PRIM_PROTOTYPE);
void prim_online_array(PRIM_PROTOTYPE);
void prim_concount(PRIM_PROTOTYPE);
void prim_condbref(PRIM_PROTOTYPE);
void prim_conidle(PRIM_PROTOTYPE);
void prim_contime(PRIM_PROTOTYPE);
void prim_conhost(PRIM_PROTOTYPE);
void prim_conuser(PRIM_PROTOTYPE);
void prim_conboot(PRIM_PROTOTYPE);
void prim_connotify(PRIM_PROTOTYPE);
void prim_condescr(PRIM_PROTOTYPE);
void prim_descrcon(PRIM_PROTOTYPE);
void prim_nextdescr(PRIM_PROTOTYPE);
void prim_descriptors(PRIM_PROTOTYPE);
void prim_descr_array(PRIM_PROTOTYPE);
void prim_descr_setuser(PRIM_PROTOTYPE);
void prim_descrflush(PRIM_PROTOTYPE);
void prim_descr(PRIM_PROTOTYPE);
void prim_firstdescr(PRIM_PROTOTYPE);
void prim_lastdescr(PRIM_PROTOTYPE);
void prim_descr_time(PRIM_PROTOTYPE);
void prim_descr_boot(PRIM_PROTOTYPE);
void prim_descr_host(PRIM_PROTOTYPE);
void prim_descr_user(PRIM_PROTOTYPE);
void prim_descr_idle(PRIM_PROTOTYPE);
void prim_descr_notify(PRIM_PROTOTYPE);
void prim_descr_dbref(PRIM_PROTOTYPE);
void prim_descr_least_idle(PRIM_PROTOTYPE);
void prim_descr_most_idle(PRIM_PROTOTYPE);
void prim_descr_securep(PRIM_PROTOTYPE);
void prim_descr_bufsize(PRIM_PROTOTYPE);

#define PRIMS_CONNECTS_FUNCS prim_awakep, prim_online, prim_concount,      \
    prim_condbref, prim_conidle, prim_contime, prim_conhost, prim_conuser, \
    prim_conboot, prim_connotify, prim_condescr, prim_descrcon,            \
    prim_nextdescr, prim_descriptors, prim_descr_setuser, prim_descrflush, \
    prim_descr, prim_online_array, prim_descr_array, prim_firstdescr,      \
	prim_lastdescr, prim_descr_time, prim_descr_host, prim_descr_user,     \
	prim_descr_boot, prim_descr_idle, prim_descr_notify, prim_descr_dbref, \
	prim_descr_least_idle, prim_descr_most_idle, prim_descr_securep,       \
	prim_descr_bufsize

#define PRIMS_CONNECTS_NAMES "AWAKE?", "ONLINE", "CONCOUNT",  \
    "CONDBREF", "CONIDLE", "CONTIME", "CONHOST", "CONUSER",   \
    "CONBOOT", "CONNOTIFY", "CONDESCR", "DESCRCON",           \
    "NEXTDESCR", "DESCRIPTORS", "DESCR_SETUSER", "DESCRFLUSH",\
    "DESCR", "ONLINE_ARRAY", "DESCR_ARRAY", "FIRSTDESCR",     \
	"LASTDESCR", "DESCRTIME", "DESCRHOST", "DESCRUSER",       \
	"DESCRBOOT", "DESCRIDLE", "DESCRNOTIFY", "DESCRDBREF",    \
	"DESCRLEASTIDLE", "DESCRMOSTIDLE", "DESCRSECURE?",        \
	"DESCRBUFSIZE"

#endif /* !P_CONNECTS_H */
