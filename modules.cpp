#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CommandServer.h"
#include "TuneThread.h"
#include "TunerControl.h"
#include "PsisiManager.h"
#include "RecManager.h"

#include "modules.h"


static CCommandServer g_commandServer ((char*)"CommandServer", 10);
static CTunerControl  g_tunerControl  ((char*)"TunerControl",  10);
static CTuneThread    g_tuneThread    ((char*)"TuneThread",    10);
static CPsisiManager  g_psisiManager  ((char*)"PsisiManager",  50);
static CRecManager    g_recManager    ((char*)"RecManager",    10);


static CThreadMgrBase *gp_modules [] = {
	&g_commandServer,
	&g_tuneThread,
	&g_tunerControl,
	&g_psisiManager,
	&g_recManager,

};


CThreadMgrBase **getModules (void)
{
	return gp_modules;
}

CThreadMgrBase *getModule (EN_MODULE enModule)
{
	if (enModule < EN_MODULE_NUM) {
		return gp_modules [enModule];
	} else {
		return NULL;
	}
}
