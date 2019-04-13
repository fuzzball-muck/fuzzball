/** @file fbsignal.h
 *
 * Header for the declaration of Fuzzball's signal handling system.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef FBSIGNAL_H
#define FBSIGNAL_H

/**
 * Set signals for the forked dump processes
 *
 * These signals are not necessarily the same as the signals for the main
 * program, but are tailored towards the needs of the dump process.
 */
void set_dumper_signals(void);

/**
 * This does the initial signal setup.
 */
void set_signals(void);

#ifdef HAVE_PSELECT
/**
 * @var the signal mask for pselect - this is set by set_signals
 */
extern sigset_t pselect_signal_mask;
#endif

#endif /* !FBSIGNAL_H */
