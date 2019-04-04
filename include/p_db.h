#ifndef P_DB_H
#define P_DB_H

#include "interp.h"

void prim_addpennies(PRIM_PROTOTYPE);
void prim_moveto(PRIM_PROTOTYPE);
void prim_pennies(PRIM_PROTOTYPE);
void prim_dbref(PRIM_PROTOTYPE);
void prim_contents(PRIM_PROTOTYPE);
void prim_exits(PRIM_PROTOTYPE);
void prim_next(PRIM_PROTOTYPE);
void prim_name(PRIM_PROTOTYPE);
void prim_setname(PRIM_PROTOTYPE);
void prim_pmatch(PRIM_PROTOTYPE);
void prim_match(PRIM_PROTOTYPE);
void prim_rmatch(PRIM_PROTOTYPE);
void prim_copyobj(PRIM_PROTOTYPE);
void prim_set(PRIM_PROTOTYPE);
void prim_mlevel(PRIM_PROTOTYPE);
void prim_flagp(PRIM_PROTOTYPE);
void prim_playerp(PRIM_PROTOTYPE);
void prim_thingp(PRIM_PROTOTYPE);
void prim_roomp(PRIM_PROTOTYPE);
void prim_programp(PRIM_PROTOTYPE);
void prim_exitp(PRIM_PROTOTYPE);
void prim_okp(PRIM_PROTOTYPE);
void prim_location(PRIM_PROTOTYPE);
void prim_owner(PRIM_PROTOTYPE);
void prim_controls(PRIM_PROTOTYPE);
void prim_getlink(PRIM_PROTOTYPE);
void prim_getlinks(PRIM_PROTOTYPE);
void prim_setlink(PRIM_PROTOTYPE);
void prim_setown(PRIM_PROTOTYPE);
void prim_newobject(PRIM_PROTOTYPE);
void prim_newroom(PRIM_PROTOTYPE);
void prim_newexit(PRIM_PROTOTYPE);
void prim_lockedp(PRIM_PROTOTYPE);
void prim_recycle(PRIM_PROTOTYPE);
void prim_setlockstr(PRIM_PROTOTYPE);
void prim_getlockstr(PRIM_PROTOTYPE);
void prim_part_pmatch(PRIM_PROTOTYPE);
void prim_checkpassword(PRIM_PROTOTYPE);
void prim_nextowned(PRIM_PROTOTYPE);
void prim_movepennies(PRIM_PROTOTYPE);
void prim_findnext(PRIM_PROTOTYPE);
void prim_setlinks_array(PRIM_PROTOTYPE);
void prim_nextentrance(PRIM_PROTOTYPE);
void prim_newplayer(PRIM_PROTOTYPE);
void prim_copyplayer(PRIM_PROTOTYPE);
void prim_objmem(PRIM_PROTOTYPE);
void prim_instances(PRIM_PROTOTYPE);
void prim_compiledp(PRIM_PROTOTYPE);
void prim_newprogram(PRIM_PROTOTYPE);
void prim_contents_array(PRIM_PROTOTYPE);
void prim_exits_array(PRIM_PROTOTYPE);
void prim_getlinks_array(PRIM_PROTOTYPE);
void prim_entrances_array(PRIM_PROTOTYPE);
void prim_newpassword(PRIM_PROTOTYPE);
void prim_compile(PRIM_PROTOTYPE);
void prim_uncompile(PRIM_PROTOTYPE);
void prim_getpids(PRIM_PROTOTYPE);
void prim_getpidinfo(PRIM_PROTOTYPE);
void prim_pname_history(PRIM_PROTOTYPE);
void prim_program_getlines(PRIM_PROTOTYPE);
void prim_program_setlines(PRIM_PROTOTYPE);
void prim_supplicant(PRIM_PROTOTYPE);
void prim_toadplayer(PRIM_PROTOTYPE);

#define PRIMS_DB_FUNCS prim_addpennies, prim_moveto, prim_pennies,     	\
    prim_dbref, prim_contents, prim_exits, prim_next, prim_name,	\
    prim_setname, prim_match, prim_rmatch, prim_copyobj, prim_set,	\
    prim_mlevel, prim_flagp, prim_playerp, prim_thingp, prim_roomp,	\
    prim_programp, prim_exitp, prim_okp, prim_location, prim_owner,	\
    prim_getlink, prim_setlink, prim_setown, prim_newobject,		\
    prim_newroom, prim_newexit, prim_lockedp, prim_recycle,             \
    prim_setlockstr, prim_getlockstr, prim_part_pmatch, prim_controls,  \
    prim_checkpassword, prim_nextowned, prim_getlinks,                  \
    prim_pmatch, prim_movepennies, prim_findnext, prim_nextentrance,    \
    prim_newplayer, prim_copyplayer, prim_objmem, prim_instances,       \
    prim_compiledp, prim_newprogram, prim_contents_array,               \
    prim_exits_array, prim_getlinks_array, prim_entrances_array,        \
    prim_compile, prim_uncompile, prim_newpassword, prim_getpids,       \
    prim_program_getlines, prim_getpidinfo, prim_program_setlines,	\
    prim_setlinks_array, prim_toadplayer, prim_supplicant,		\
    prim_pname_history

#define PRIMS_DB_NAMES "ADDPENNIES", "MOVETO", "PENNIES",  \
    "DBREF", "CONTENTS", "EXITS", "NEXT", "NAME",	   \
    "SETNAME", "MATCH", "RMATCH", "COPYOBJ", "SET",	   \
    "MLEVEL", "FLAG?", "PLAYER?", "THING?", "ROOM?",	   \
    "PROGRAM?", "EXIT?", "OK?", "LOCATION", "OWNER",	   \
    "GETLINK", "SETLINK", "SETOWN", "NEWOBJECT", "NEWROOM",\
    "NEWEXIT", "LOCKED?", "RECYCLE", "SETLOCKSTR",	   \
    "GETLOCKSTR", "PART_PMATCH", "CONTROLS",		   \
    "CHECKPASSWORD", "NEXTOWNED", "GETLINKS",              \
    "PMATCH", "MOVEPENNIES", "FINDNEXT", "NEXTENTRANCE",   \
    "NEWPLAYER", "COPYPLAYER", "OBJMEM", "INSTANCES",      \
    "COMPILED?", "NEWPROGRAM", "CONTENTS_ARRAY",           \
    "EXITS_ARRAY", "GETLINKS_ARRAY", "ENTRANCES_ARRAY",    \
    "COMPILE", "UNCOMPILE", "NEWPASSWORD", "GETPIDS",      \
    "PROGRAM_GETLINES", "GETPIDINFO", "PROGRAM_SETLINES",  \
    "SETLINKS_ARRAY", "TOADPLAYER", "SUPPLICANT",	   \
    "PNAME_HISTORY"

#endif /* !P_DB_H */
