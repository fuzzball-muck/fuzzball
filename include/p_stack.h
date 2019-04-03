#ifndef P_STACK_H
#define P_STACK_H

#include "interp.h"

void prim_pop(PRIM_PROTOTYPE);	/*     ? --              */
void prim_dup(PRIM_PROTOTYPE);	/*     ? -- ? ?          */
void prim_pdup(PRIM_PROTOTYPE);	/*     a -- a a | ?	 */
void prim_nip(PRIM_PROTOTYPE);	/*     a b -- b		 */
void prim_tuck(PRIM_PROTOTYPE);	/*     a b -- b a b	 */
void prim_at(PRIM_PROTOTYPE);	/*   V|v -- ?            */
void prim_bang(PRIM_PROTOTYPE);	/* ? V|v --              */
void prim_var(PRIM_PROTOTYPE);	/*     i -- V            */
void prim_localvar(PRIM_PROTOTYPE);	/*     i -- v            */
void prim_swap(PRIM_PROTOTYPE);	/* ?1 ?2 -- ?2 ?1        */
void prim_over(PRIM_PROTOTYPE);	/* ?1 ?1 -- ?1 ?2 ?1     */
void prim_pick(PRIM_PROTOTYPE);	/*    ?n ... ?1 i -- ?n ... ?1 ?n */
void prim_put(PRIM_PROTOTYPE);	/* ?n ... ?1 ?x i -- ?x ... ?1    */
void prim_rot(PRIM_PROTOTYPE);	/*       ?1 ?2 ?3 -- ?2 ?3 ?1     */
void prim_rrot(PRIM_PROTOTYPE);	/*       ?1 ?2 ?3 -- ?3 ?1 ?2     */
void prim_rotate(PRIM_PROTOTYPE);	/* Rotates top N stack elements */
void prim_dbtop(PRIM_PROTOTYPE);	/*       -- i            */
void prim_depth(PRIM_PROTOTYPE);	/*       -- i            */
void prim_fulldepth(PRIM_PROTOTYPE);	/*       -- i            */
void prim_version(PRIM_PROTOTYPE);	/*       -- s            */
void prim_prog(PRIM_PROTOTYPE);	/*       -- d            */
void prim_trig(PRIM_PROTOTYPE);	/*       -- d            */
void prim_caller(PRIM_PROTOTYPE);	/*       -- d            */
void prim_intp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_arrayp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_dictionaryp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_floatp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_stringp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_dbrefp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_addressp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_lockp(PRIM_PROTOTYPE);	/*     ? -- i            */
void prim_checkargs(PRIM_PROTOTYPE);	/*     s --              */
void prim_mode(PRIM_PROTOTYPE);	/*       -- i            */
void prim_setmode(PRIM_PROTOTYPE);	/*     i --              */
void prim_interp(PRIM_PROTOTYPE);	/* d d s -- ?            */
void prim_reverse(PRIM_PROTOTYPE);	/* ?n ... ?1 i -- ?1 ... ?n    */
void prim_lreverse(PRIM_PROTOTYPE);	/* ?n ... ?1 i -- ?1 ... ?n i  */
void prim_dupn(PRIM_PROTOTYPE);
void prim_ldup(PRIM_PROTOTYPE);	/*   {?} -- {?} {?}      */
void prim_popn(PRIM_PROTOTYPE);	/*   {?} --              */
void prim_for(PRIM_PROTOTYPE);	/* i i i --              */
void prim_foreach(PRIM_PROTOTYPE);	/*     i --              */
void prim_secure_sysvars(PRIM_PROTOTYPE);	/* -- */
void prim_cmd(PRIM_PROTOTYPE);	/* -- s */
void prim_preempt(PRIM_PROTOTYPE);	/* -- */
void prim_foreground(PRIM_PROTOTYPE);	/* -- */
void prim_background(PRIM_PROTOTYPE);	/* -- */
void prim_shallow_copy(PRIM_PROTOTYPE);	/*     ? -- ? ?          */
void prim_deep_copy(PRIM_PROTOTYPE);	/*     ? -- ? ?          */
void prim_foriter(PRIM_PROTOTYPE);	/*       -- i  or  @ ?   */
void prim_forpop(PRIM_PROTOTYPE);	/*       --              */
void prim_mark(PRIM_PROTOTYPE);	/*       -- m            */
void prim_findmark(PRIM_PROTOTYPE);	/* m ?n ... ?1 -- ?n ... ?1 i    */
void prim_trypop(PRIM_PROTOTYPE);	/* -- */

#define PRIMS_STACK_FUNCS prim_pop, prim_dup, prim_pdup, prim_nip, prim_tuck, \
    prim_at, prim_bang, prim_var, prim_localvar, prim_swap, prim_over, \
    prim_pick, prim_put, prim_rot, prim_rrot, prim_rotate, prim_dbtop, \
    prim_depth, prim_version, prim_prog, prim_trig, prim_caller, prim_intp, \
    prim_stringp, prim_dbrefp, prim_addressp, prim_lockp, prim_checkargs, \
    prim_mode, prim_setmode, prim_interp, prim_for, prim_foreach, prim_floatp, \
    prim_reverse, prim_popn, prim_dupn, prim_ldup, prim_lreverse, \
    prim_arrayp, prim_dictionaryp, prim_mark, prim_findmark, prim_fulldepth, \
    prim_secure_sysvars, prim_cmd, prim_preempt, prim_foreground, \
    prim_background, prim_shallow_copy, prim_deep_copy

#define PRIMS_STACK_NAMES "POP", "DUP", "?DUP", "NIP", "TUCK",	\
    "@", "!", "VARIABLE", "LOCALVAR", "SWAP", "OVER", "PICK",	\
    "PUT", "ROT", "-ROT", "ROTATE", "DBTOP", "DEPTH", "VERSION",\
    "PROG", "TRIG", "CALLER", "INT?", "STRING?", "DBREF?",	\
    "ADDRESS?", "LOCK?", "CHECKARGS", "MODE", "SETMODE",	\
    "INTERP", " FOR", " FOREACH", "FLOAT?", "REVERSE", "POPN",	\
    "DUPN", "LDUP", "LREVERSE", "ARRAY?", "DICTIONARY?", "{",	\
    "}", "FULLDEPTH", "SECURE_SYSVARS", "CMD", "PREEMPT",	\
    "FOREGROUND", "BACKGROUND", "SHALLOW_COPY", "DEEP_COPY"

#define PRIMS_INTERNAL_FUNCS prim_foriter, prim_forpop, prim_trypop

#define PRIMS_INTERNAL_NAMES " FORITER", " FORPOP", " TRYPOP"

#endif /* !P_STACK_H */
