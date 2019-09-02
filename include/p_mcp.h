#ifndef P_MCP_H
#define P_MCP_H

#include "interp.h"

void prim_mcp_register(PRIM_PROTOTYPE);
void prim_mcp_register_event(PRIM_PROTOTYPE);
void prim_mcp_bind(PRIM_PROTOTYPE);
void prim_mcp_supports(PRIM_PROTOTYPE);
void prim_mcp_send(PRIM_PROTOTYPE);

#define PRIMS_MCP_FUNCS prim_mcp_register, prim_mcp_bind, prim_mcp_supports, \
		prim_mcp_send, prim_mcp_register_event

#define PRIMS_MCP_NAMES "MCP_REGISTER", "MCP_BIND", "MCP_SUPPORTS", \
		"MCP_SEND", "MCP_REGISTER_EVENT"

#endif /* !P_MCP_H */
