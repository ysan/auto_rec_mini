#ifndef _COMMAND_TABLES_H_
#define _COMMAND_TABLES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"

#include "Utils.h"


typedef struct command_info {
	const char *command;
	const char *desc;
	void (*cb_command) (int argc, char* argv[], threadmgr::CThreadMgrBase *base);
	struct command_info *next;
} command_info_t;


extern command_info_t g_root_command_table [] ;

#endif
