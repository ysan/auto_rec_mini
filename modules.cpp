#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CommandServer.h"

#include "modules.h"


CCommandServer g_commandServer ((char*)"CommandServer", 10);


CThreadMgrBase *gp_modules [] = {
	&g_commandServer,

};


