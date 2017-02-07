#ifndef _FBSIGNAL_H
#define _FBSIGNAL_H

#ifndef WIN32
#ifdef HAVE_ERRNO_H
#include <errno.h>
#else /* !defined(HAVE_ERRNO_H) */
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#else /* !defined(HAVE_SYS_ERRNO_H) */
extern int errno;
#endif /* defined(HAVE_SYS_ERRNO_H) */
#endif /* defined(HAVE_ERRNO_H) */
#endif /* !defined(WIN32) */

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
