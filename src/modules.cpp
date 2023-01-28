#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CommandServer.h"
#include "TuneThread.h"
#include "TunerControl.h"
#include "PsisiManager.h"
#include "TunerService.h"
#include "RecManager.h"
#include "ChannelManager.h"
#include "EventScheduleManager.h"
#include "EventSearch.h"

#include "modules.h"


static CTuneThread           s_tuneThread_gr0    ((char*)"TuneTh_0"            , 10, 0);
static CTuneThread           s_tuneThread_gr1    ((char*)"TuneTh_1"            , 10, 1);
static CTuneThread           s_tuneThread_gr2    ((char*)"TuneTh_2"            , 10, 2);
static CTuneThread           s_tuneThread_gr3    ((char*)"TuneTh_3"            , 10, 3);

static CTunerControl         s_tunerControl_gr0  ((char*)"TunerCtl_0"          , 10, 0);
static CTunerControl         s_tunerControl_gr1  ((char*)"TunerCtl_1"          , 10, 1);
static CTunerControl         s_tunerControl_gr2  ((char*)"TunerCtl_2"          , 10, 2);
static CTunerControl         s_tunerControl_gr3  ((char*)"TunerCtl_3"          , 10, 3);

static CPsisiManager         s_psisiManager_gr0  ((char*)"PsisiMgr_0"          , 100, 0);
static CPsisiManager         s_psisiManager_gr1  ((char*)"PsisiMgr_1"          , 100, 1);
static CPsisiManager         s_psisiManager_gr2  ((char*)"PsisiMgr_2"          , 100, 2);
static CPsisiManager         s_psisiManager_gr3  ((char*)"PsisiMgr_3"          , 100, 3);

static CTunerService         s_tunerService      ((char*)"TunerService"        , 10);
static CRecManager           s_recManager        ((char*)"RecManager"          , 10);
static CChannelManager       s_chennelManager    ((char*)"ChannelManager"      , 10);
static CEventScheduleManager s_eventSchedManager ((char*)"EventScheduleManager", 50);
static CEventSearch          s_eventSearch       ((char*)"EventSearch"         , 10);
static CCommandServer        s_commandServer     ((char*)"CommandServer"       , 10);


static threadmgr::CThreadMgrBase *gp_modules [] = {
	&s_tuneThread_gr0,		// group0
	&s_tuneThread_gr1,		// group1
	&s_tuneThread_gr2,		// group2
	&s_tuneThread_gr3,		// group3

	&s_tunerControl_gr0,	// group0
	&s_tunerControl_gr1,	// group1
	&s_tunerControl_gr2,	// group2
	&s_tunerControl_gr3,	// group3

	&s_psisiManager_gr0,	// group0
	&s_psisiManager_gr1,	// group1
	&s_psisiManager_gr2,	// group2
	&s_psisiManager_gr3,	// group3

	&s_tunerService,
	&s_recManager,
	&s_chennelManager,
	&s_eventSchedManager,
	&s_eventSearch,
	&s_commandServer,

};


threadmgr::CThreadMgrBase **getModules (void)
{
	return gp_modules;
}

threadmgr::CThreadMgrBase *getModule (EN_MODULE enModule)
{
	if (enModule < EN_MODULE_NUM) {
		return gp_modules [enModule];
	} else {
		return NULL;
	}
}
