/* $Header: /cvsroot/fbmuck/fbmuck/include/patchlevel.h,v 1.6 2005/07/04 20:25:43 winged Exp $ */

#ifndef _PATCHLEVEL_H
#define _PATCHLEVEL_H

# define PATCHLEVEL "1"

#endif /* _PATCHLEVEL_H */

#ifdef DEFINE_HEADER_VERSIONS

#ifndef patchlevelh_version
#define patchlevelh_version
const char *patchlevel_h_version = "$RCSfile: patchlevel.h,v $ $Revision: 1.6 $";
#endif
#else
extern const char *patchlevel_h_version;
#endif

