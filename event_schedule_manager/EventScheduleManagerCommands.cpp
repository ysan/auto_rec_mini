#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "EventScheduleManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void startCache_currentService (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.reqStartCache_currentService ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_eventScheduleManagerCommands [] = { // extern
	{
		"e",
		"start cache - CurrentService",
		startCache_currentService,
		NULL,
	},
	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};

