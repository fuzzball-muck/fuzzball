
/*
#ifndef BSD43
# include <string.h>
#else
# ifdef __GNUC__
extern char *strchr(), *strrchr();
# else
#  include <strings.h>
#  ifndef __STDC__
#   define strchr(x,y) index((x),(y))
#   define strrchr(x,y) rindex((x),(y))
#  endif
# endif
#endif
*/

#ifdef DEFINE_HEADER_VERSIONS

#ifndef smatchh_version
#define smatchh_version
const char *smatch_h_version = "$RCSfile: smatch.h,v $ $Revision: 1.5 $";
#endif
#else
extern const char *smatch_h_version;
#endif

