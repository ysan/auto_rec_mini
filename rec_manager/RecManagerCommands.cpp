#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "RecManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void setReserveCurrentEvent (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqSetReserveCurrentEvent ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}



ST_COMMAND_INFO g_recManagerCommands [] = { // extern
	{
		"rce",
		"set reserve CurrentEvent",
		setReserveCurrentEvent,
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

