#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "EventSearchIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Utils.h"


static void _addRecReserve_keywordSearch (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventSearchIf mgr(pBase->getExternalIf());
	mgr.reqAddRecReserve_keywordSearch ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _addRecReserve_keywordSearch_ex (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventSearchIf mgr(pBase->getExternalIf());
	mgr.reqAddRecReserve_keywordSearch_ex ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_histories (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventSearchIf mgr(pBase->getExternalIf());
	mgr.reqDumpHistories ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_histories_ex (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventSearchIf mgr(pBase->getExternalIf());
	mgr.reqDumpHistories_ex ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_eventSearchCommands [] = { // extern
	{
		"r",
		"add rec reserve (event name keyword search)",
		_addRecReserve_keywordSearch,
		NULL,
	},
	{
		"rx",
		"add rec reserve (extended event keyword search)",
		_addRecReserve_keywordSearch_ex,
		NULL,
	},
	{
		"h",
		"dump histories (event name keyword search)",
		_dump_histories,
		NULL,
	},
	{
		"hx",
		"dump histories (extended event keyword search)",
		_dump_histories_ex,
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

