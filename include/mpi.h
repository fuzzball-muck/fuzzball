/** @file mpi.h
 *
 * Header for some of the MPI message calls.  The rest are in msgparse.h
 * All implementation is in msgparse.c
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

/*
 * @TODO: Is there a particular reason why this file is separate from
 *        msgparse.h ?  There is no corresponding mpi.c; these calls are
 *        all implemented in msgparse.h
 *
 * The only thing I can figure is that msgparse.h has calls or definitions
 * that would conflict somewhere?  Is there any benefit to having them
 * separated?  If not, I would generally prefer to use mpi.h / mpi.c, and
 * rename msgparse.h to mpi.h (after moving this stuff over) and rename
 * msgparse.c to mpi.c ... that makes it immensely obvious what is going
 * on.
 */

#ifndef MPI_H
#define MPI_H

#include "config.h"

#define MPI_ISPUBLIC        0x00    /* never test for this one */
#define MPI_ISPRIVATE       0x01    /* Private calls don't do omesg I think */
#define MPI_ISLISTENER      0x02    /* Triggered by listener */
#define MPI_ISLOCK          0x04    /* Triggered by lock */
#define MPI_ISDEBUG         0x08    /* Display debugging */
#define MPI_ISBLESSED       0x10    /* Has 'super user' permissions */
#define MPI_NOHOW           0x20    /* Do not allocate {&how} variable */

/**
 * Parse an MPI message
 *
 * MPI can only recurse 25 deep.  This is hard coded.  Only BUFFER_LEN of
 * 'inbuf' will be processed even if inbuf is longer than BUFFER_LEN.
 *
 * @param descr the descriptor of the user running the parser
 * @param player the player running the parser
 * @param what the triggering object
 * @param perms the object that dictates the permission
 * @param inbuf the string to process
 * @param outbuf the output buffer
 * @param maxchars the maximum size of the output buffer
 * @param mesgtyp permission bitvector
 * @return NULL on failure, outbuf on success
 */
char *mesg_parse(int descr, dbref player, dbref what, dbref perms,
                 const char *inbuf, char *outbuf, int maxchars, int mesgtyp);

/**
 * Process an MPI message, handling variable allocations and cleanup
 *
 * The importance of this function is that it allocates certain standard
 * MPI variables: how (if MPI_NOHOW is not in mesgtyp), cmd, and arg
 *
 * Then it runs MesgParse to process the message after those variables
 * are set up.  And finally, it does cleanup.
 *
 * Statistics are kept as part of this call, to keep track of how much
 * time MPI is taking up to run.  This updates all the profile numbers.
 *
 * @param descr the descriptor of the calling player
 * @param player the player doing the MPI parsing call
 * @param what the object that triggered the MPI parse call
 * @param perms the object from which permissions are sourced
 * @param inbuf the input string to process
 * @param abuf identifier string for error messages
 * @param outbuf the buffer to store output results
 * @param outbuflen the length of outbuf
 * @param mesgtyp the bitvector of permissions
 * @return a pointer to outbuf
 */
char *do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf,
                    const char *abuf, char *outbuf, int buflen, int mesgtyp);

char *do_parse_prop(int descr, dbref player, dbref what, const char *propname,
                    const char *abuf, char *outbuf, int buflen, int mesgtyp);

#endif /* !MPI_H */
