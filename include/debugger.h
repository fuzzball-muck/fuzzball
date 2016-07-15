#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "db.h"
#include "interp.h"

void list_proglines(dbref player, dbref program, struct frame *fr, int start, int end);
void muf_backtrace(dbref player, dbref program, int count, struct frame *fr);
int muf_debugger(int descr, dbref player, dbref program, const char *text, struct frame *fr);
char *show_line_prims(dbref program, struct inst *pc, int maxprims, int markpc);

#endif				/* _DEBUGGER_H */
