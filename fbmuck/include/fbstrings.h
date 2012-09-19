/* $Header: /cvsroot/fbmuck/fbmuck/include/fbstrings.h,v 1.6 2005/07/12 17:33:33 revar Exp $ */

#ifndef _FBSTRINGS_H
#define _FBSTRINGS_H

const char *strencrypt(const char *, const char *);
const char *strdecrypt(const char *, const char *);


#ifdef DEFINE_HEADER_VERSIONS

#ifndef fbstringsh_version
#define fbstringsh_version
const char *fbstrings_h_version = "$RCSfile: fbstrings.h,v $ $Revision: 1.6 $";
#endif

#else
extern const char *fbstrings_h_version;
#endif

#endif /* _FBSTRINGS_H */

