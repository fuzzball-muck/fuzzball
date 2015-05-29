#ifndef _SPEECH_H_
#define _SPEECH_H_

void
do_say(dbref player, const char *message)
;

void
do_whisper(int descr, dbref player, const char *arg1, const char *arg2)
;

void
do_pose(dbref player, const char *message)
;

void
do_wall(dbref player, const char *message)
;

void
do_gripe(dbref player, const char *message)
;

void
do_page(dbref player, const char *arg1, const char *arg2)
;

void
notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg, int isprivate)
;

void
notify_except(dbref first, dbref exception, const char *msg, dbref who)
;

void
parse_oprop(int descr, dbref player, dbref dest, dbref exit, const char *propname,
			   const char *prefix, const char *whatcalled)
;

void
parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg,
			   const char *prefix, const char *whatcalled, int mpiflags)
;


int
blank(const char *s)
;

#endif /* !defined _SPEECH_H_ */
