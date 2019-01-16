#ifndef _COMMAND_TABLES_H_
#define _COMMAND_TABLES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"

#include "Utils.h"


using namespace ThreadManager;



typedef struct command_info {
	const char *pszCommand;
	const char *pszDesc;
	void (*pcbCommand) (int argc, char* argv[], CThreadMgrExternalIf *pIf);
	struct command_info *pNext;
} ST_COMMAND_INFO;




extern ST_COMMAND_INFO g_rootCommandTable [] ;

#endif
