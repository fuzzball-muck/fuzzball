#ifndef MCPPKG_H
#define MCPPKG_H

#include "mcp.h"

void mcppkg_simpleedit(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);
void mcppkg_languages(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);
void mcppkg_help_request(McpFrame * mfr, McpMesg * msg, McpVer ver, void *context);
void show_mcp_error(McpFrame * mfr, char *topic, char *text);

#endif /* !MCPPKG_H */
