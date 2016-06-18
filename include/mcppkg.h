#include "config.h"
#ifdef MCP_SUPPORT
#ifndef _MCPPKG_H
#define _MCPPKG_H

void mcppkg_simpleedit(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);
void mcppkg_help_request(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);
void show_mcp_error(McpFrame * mfr, char *topic, char *text);

#endif				/* _MCPPKG_H */
#endif
