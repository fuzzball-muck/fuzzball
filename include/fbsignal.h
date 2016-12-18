#ifndef _FBSIGNAL_H
#define _FBSIGNAL_H

#ifdef WIN32
void set_console(void);
#endif
void set_dumper_signals(void);
void set_signals(void);

#ifdef HAVE_PSELECT
#include <signal.h>
extern sigset_t pselect_signal_mask;
#endif

#endif				/* _FBSIGNAL_H */
