#ifndef _P_DB_H
#define _P_DB_H

extern void prim_addpennies(PRIM_PROTOTYPE);
extern void prim_moveto(PRIM_PROTOTYPE);
extern void prim_pennies(PRIM_PROTOTYPE);
extern void prim_dbcomp(PRIM_PROTOTYPE);
extern void prim_dbref(PRIM_PROTOTYPE);
extern void prim_contents(PRIM_PROTOTYPE);
extern void prim_exits(PRIM_PROTOTYPE);
extern void prim_next(PRIM_PROTOTYPE);
extern void prim_name(PRIM_PROTOTYPE);
extern void prim_setname(PRIM_PROTOTYPE);
extern void prim_pmatch(PRIM_PROTOTYPE);
extern void prim_match(PRIM_PROTOTYPE);
extern void prim_rmatch(PRIM_PROTOTYPE);
extern void prim_copyobj(PRIM_PROTOTYPE);
extern void prim_set(PRIM_PROTOTYPE);
extern void prim_mlevel(PRIM_PROTOTYPE);
extern void prim_flagp(PRIM_PROTOTYPE);
extern void prim_playerp(PRIM_PROTOTYPE);
extern void prim_thingp(PRIM_PROTOTYPE);
extern void prim_roomp(PRIM_PROTOTYPE);
extern void prim_programp(PRIM_PROTOTYPE);
extern void prim_exitp(PRIM_PROTOTYPE);
extern void prim_okp(PRIM_PROTOTYPE);
extern void prim_location(PRIM_PROTOTYPE);
extern void prim_owner(PRIM_PROTOTYPE);
extern void prim_controls(PRIM_PROTOTYPE);
extern void prim_getlink(PRIM_PROTOTYPE);
extern void prim_getlinks(PRIM_PROTOTYPE);
extern void prim_setlink(PRIM_PROTOTYPE);
extern void prim_setown(PRIM_PROTOTYPE);
extern void prim_newobject(PRIM_PROTOTYPE);
extern void prim_newroom(PRIM_PROTOTYPE);
extern void prim_newexit(PRIM_PROTOTYPE);
extern void prim_lockedp(PRIM_PROTOTYPE);
extern void prim_recycle(PRIM_PROTOTYPE);
extern void prim_setlockstr(PRIM_PROTOTYPE);
extern void prim_getlockstr(PRIM_PROTOTYPE);
extern void prim_part_pmatch(PRIM_PROTOTYPE);
extern void prim_checkpassword(PRIM_PROTOTYPE);
extern void prim_nextowned(PRIM_PROTOTYPE);
extern void prim_movepennies(PRIM_PROTOTYPE);
extern void prim_findnext(PRIM_PROTOTYPE);
extern void prim_setlinks_array(PRIM_PROTOTYPE);

/* new protomuck prims */
extern void prim_nextentrance(PRIM_PROTOTYPE);
extern void prim_newplayer(PRIM_PROTOTYPE);
extern void prim_copyplayer(PRIM_PROTOTYPE);
extern void prim_objmem(PRIM_PROTOTYPE);
extern void prim_instances(PRIM_PROTOTYPE);
extern void prim_compiledp(PRIM_PROTOTYPE);
extern void prim_newprogram(PRIM_PROTOTYPE);
extern void prim_contents_array(PRIM_PROTOTYPE);
extern void prim_exits_array(PRIM_PROTOTYPE);
extern void prim_getlinks_array(PRIM_PROTOTYPE);
extern void prim_entrances_array(PRIM_PROTOTYPE);
extern void prim_newpassword(PRIM_PROTOTYPE);
extern void prim_compile(PRIM_PROTOTYPE);
extern void prim_uncompile(PRIM_PROTOTYPE);
extern void prim_getpids(PRIM_PROTOTYPE);
extern void prim_getpidinfo(PRIM_PROTOTYPE);

extern void prim_program_getlines(PRIM_PROTOTYPE);
extern void prim_program_setlines(PRIM_PROTOTYPE);

/* WORK: Add these prims */
extern void prim_toadplayer(PRIM_PROTOTYPE);

#define PRIMS_DB_FUNCS1 prim_addpennies, prim_moveto, prim_pennies,      \
    prim_dbcomp, prim_dbref, prim_contents, prim_exits, prim_next,       \
    prim_name, prim_setname, prim_match, prim_rmatch, prim_copyobj,      \
    prim_set, prim_mlevel, prim_flagp, prim_playerp, prim_thingp,        \
    prim_roomp, prim_programp, prim_exitp, prim_okp, prim_location,      \
    prim_owner, prim_getlink, prim_setlink, prim_setown, prim_newobject, \
    prim_newroom, prim_newexit, prim_lockedp, prim_recycle,              \
    prim_setlockstr, prim_getlockstr, prim_part_pmatch, prim_controls,   \
    prim_checkpassword, prim_nextowned, prim_getlinks,                   \
    prim_pmatch, prim_movepennies, prim_findnext, prim_nextentrance,     \
    prim_newplayer, prim_copyplayer, prim_objmem, prim_instances,        \
    prim_compiledp, prim_newprogram, prim_contents_array,                \
    prim_exits_array, prim_getlinks_array, prim_entrances_array,         \
    prim_compile, prim_uncompile, prim_newpassword, prim_getpids,        \
    prim_program_getlines, prim_getpidinfo, prim_program_setlines,	 \
    prim_setlinks_array



#define PRIMS_DB_NAMES1 "ADDPENNIES", "MOVETO", "PENNIES", \
    "DBCMP", "DBREF", "CONTENTS", "EXITS", "NEXT",         \
    "NAME", "SETNAME", "MATCH", "RMATCH", "COPYOBJ",       \
    "SET", "MLEVEL", "FLAG?", "PLAYER?", "THING?",         \
    "ROOM?", "PROGRAM?", "EXIT?", "OK?", "LOCATION",       \
    "OWNER", "GETLINK", "SETLINK", "SETOWN", "NEWOBJECT",  \
    "NEWROOM", "NEWEXIT", "LOCKED?", "RECYCLE",            \
    "SETLOCKSTR", "GETLOCKSTR", "PART_PMATCH", "CONTROLS", \
    "CHECKPASSWORD", "NEXTOWNED", "GETLINKS",              \
    "PMATCH", "MOVEPENNIES", "FINDNEXT", "NEXTENTRANCE",   \
    "NEWPLAYER", "COPYPLAYER", "OBJMEM", "INSTANCES",      \
    "COMPILED?", "NEWPROGRAM", "CONTENTS_ARRAY",           \
    "EXITS_ARRAY", "GETLINKS_ARRAY", "ENTRANCES_ARRAY",    \
    "COMPILE", "UNCOMPILE", "NEWPASSWORD", "GETPIDS",      \
    "PROGRAM_GETLINES", "GETPIDINFO", "PROGRAM_SETLINES",  \
    "SETLINKS_ARRAY"

#define PRIMS_DB_CNT1 61


#ifdef SCARY_MUF_PRIMS

 /* These add dangerous, but possibly useful prims. */
# define PRIMS_DB_FUNCS PRIMS_DB_FUNCS1, prim_toadplayer
# define PRIMS_DB_NAMES PRIMS_DB_NAMES1, "TOADPLAYER"
# define PRIMS_DB_CNT (PRIMS_DB_CNT1 + 1)

#else
# define PRIMS_DB_FUNCS PRIMS_DB_FUNCS1
# define PRIMS_DB_NAMES PRIMS_DB_NAMES1
# define PRIMS_DB_CNT PRIMS_DB_CNT1
#endif

#endif /* _P_DB_H */
