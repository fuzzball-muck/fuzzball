#include "config.h"

#ifdef MCPGUI_SUPPORT
#ifndef P_MCPGUI_H
#define P_MCPGUI_H

#include "interp.h"

void prim_gui_available(PRIM_PROTOTYPE);
void prim_gui_dlog_create(PRIM_PROTOTYPE);
void prim_gui_dlog_show(PRIM_PROTOTYPE);
void prim_gui_dlog_close(PRIM_PROTOTYPE);
void prim_gui_value_set(PRIM_PROTOTYPE);
void prim_gui_value_get(PRIM_PROTOTYPE);
void prim_gui_values_get(PRIM_PROTOTYPE);
void prim_gui_ctrl_create(PRIM_PROTOTYPE);
void prim_gui_ctrl_command(PRIM_PROTOTYPE);


#define PRIMS_MCPGUI_FUNCS prim_gui_available, prim_gui_dlog_show, \
		prim_gui_dlog_close, prim_gui_values_get, prim_gui_value_get, \
		prim_gui_value_set, prim_gui_ctrl_create, prim_gui_ctrl_command, \
		prim_gui_dlog_create

#define PRIMS_MCPGUI_NAMES "GUI_AVAILABLE", "GUI_DLOG_SHOW", \
		"GUI_DLOG_CLOSE", "GUI_VALUES_GET", "GUI_VALUE_GET", \
		"GUI_VALUE_SET", "GUI_CTRL_CREATE", "GUI_CTRL_COMMAND", \
		"GUI_DLOG_CREATE"

#endif /* !P_MCPGUI_H */
#endif /* MCPGUI_SUPPORT */
