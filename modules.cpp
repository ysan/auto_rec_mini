#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CommandServer.h"
#include "TuneThread.h"
#include "TunerControl.h"

#include "modules.h"


CCommandServer g_commandServer ((char*)"CommandServer", 10);
CTunerControl g_tunerControl ((char*)"TunerControl", 10);
CTuneThread g_tuneThread ((char*)"TuneThread", 10);


CThreadMgrBase *gp_modules [] = {
	&g_commandServer,
	&g_tuneThread,
	&g_tunerControl,


};


