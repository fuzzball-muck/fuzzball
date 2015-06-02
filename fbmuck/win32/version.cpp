/*
 * Copyright (c) 1991-2000 by Fuzzball Software
 * under the GNU Public License
 *
 * Some parts of this code Copyright (c) 1990 Chelsea Dyerman
 * University of California, Berkeley (XCF)
 */

#include "fb.h"
#include "config.h"
#include "patchlevel.h"
#include "params.h"
#include "externs.h"

#define generation "1"
#define creation "Mon June 1 2015 at 16:41:17 EDT"
const char *version = PATCHLEVEL;
#ifdef DEBUG
#define debug "Debug Version, assertions enabled"
#else
#define debug ""
#endif

void
do_version(dbref player)
{
	char s[BUFFER_LEN];

	snprintf(s,BUFFER_LEN,"Version: %s Compiled on: %s %s",VERSION,creation,debug);
	notify(player, s);
	return;
}

