/* MPI msgparse.c header file. */

#ifndef _MPI_H
#define _MPI_H

#define MPI_ISPUBLIC	0x00	/* never test for this one */
#define MPI_ISPRIVATE	0x01
#define MPI_ISLISTENER	0x02
#define MPI_ISLOCK		0x04
#define MPI_ISDEBUG		0x08
#define MPI_ISBLESSED	0x10
#define MPI_NOHOW		0x20

extern char *mesg_parse(int descr, dbref player, dbref what, dbref perms, const char *inbuf,
			char *outbuf, int maxchars, int mesgtyp);

extern char *do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf,
			   const char *abuf, char *outbuf, int buflen, int mesgtyp);

extern char *do_parse_prop(int descr, dbref player, dbref what, const char *propname,
			   const char *abuf, char *outbuf, int buflen, int mesgtyp);

#endif				/* _MPI_H */
