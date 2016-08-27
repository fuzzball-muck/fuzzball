#ifndef _P_REGEX_H
#define _P_REGEX_H

#define MUF_RE_ICASE	1
#define MUF_RE_ALL	2
#define MUF_RE_EXTENDED	4

void prim_regexp(PRIM_PROTOTYPE);
void prim_regsub(PRIM_PROTOTYPE);
void prim_regsplit(PRIM_PROTOTYPE);
void prim_regsplit_noempty(PRIM_PROTOTYPE);

#define PRIMS_REGEX_FUNCS prim_regexp, prim_regsub, prim_regsplit,	\
	prim_regsplit_noempty

#define PRIMS_REGEX_NAMES "REGEXP", "REGSUB", "REGSPLIT", "REGSPLIT_NOEMPTY"

#endif				/* _P_REGEX_H */
