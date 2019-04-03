#ifndef FBSIGNAL_H
#define FBSIGNAL_H

void set_dumper_signals(void);
void set_signals(void);

#ifdef HAVE_PSELECT
extern sigset_t pselect_signal_mask;
#endif

#endif /* !FBSIGNAL_H */
