#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifdef WIN32
void set_console(void);
#endif
void set_dumper_signals(void);
void set_signals(void);

#endif				/* _SIGNAL_H */
