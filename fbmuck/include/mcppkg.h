#ifndef _MCPPKG_H
#define _MCPPKG_H

void show_mcp_error(McpFrame * mfr, char *topic, char *text);
void mcppkg_simpleedit(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);
void mcppkg_help_request(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);

#endif /* _MCPPKG_H */

#ifdef DEFINE_HEADER_VERSIONS

#ifndef mcppkgh_version
#define mcppkgh_version
const char *mcppkg_h_version = "$RCSfile: mcppkg.h,v $ $Revision: 1.7 $";
#endif
#else
extern const char *mcppkg_h_version;
#endif

