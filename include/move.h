#ifndef _MOVE_H
#define _MOVE_H

#include "config.h"

void enter_room(int descr, dbref player, dbref loc, dbref exit);
void moveto(dbref what, dbref where);
void send_contents(int descr, dbref loc, dbref dest);
void send_home(int descr, dbref thing, int homepuppet);
void recycle(int descr, dbref player, dbref thing);

#endif				/* _MOVE_H */
