#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "RecManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void setReserve_currentEvent (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqSetReserve_currentEvent ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void setReserve_manual (int argc, char* argv[], CThreadMgrBase *pBase)
{
/*
	if (argc != 5) {
		_UTL_LOG_W ("invalid arguments.\n");
	}

	std::regex regex_ts_id ("[0-9]+");
	if (!std::regex_match (argv[0], regex_ts_id)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_n_id ("[0-9]+");
	if (!std::regex_match (argv[1], regex_n_id)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_service_id("[0-9]+");
	if (!std::regex_match (argv[2], regex_service_id)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_start_time("[0-9]+");
	if (!std::regex_match (argv[3], regex_start_time)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_end_time("[0-9]+");
	if (!std::regex_match (argv[4], regex_end_time)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}
*/


	_MANUAL_RESERVE_PARAM _param;

CEtime t;
t.setCurrentTime();
t.addMin (1);
_param.start_time = t;
t.addSec (1);
_param.end_time = t;
_param.dump();

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqSetReserve_manual (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void stop (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqStopRecording ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_reserves (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpReserves (0);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_results (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpReserves (1);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}



ST_COMMAND_INFO g_recManagerCommands [] = { // extern
	{
		"re",
		"set reserve - CurrentEvent",
		setReserve_currentEvent,
		NULL,
	},
	{
		"rm",
		"set reserve - Manual (usage: rm {ts_id} {n_id} {service_id} {start_time} {end_time}\n\
                                                         start_time, end_time format is \"yyyyMMddHHmmss\")",
		setReserve_manual,
		NULL,
	},
	{
		"stop",
		"force stop recording",
		stop,
		NULL,
	},
	{
		"dr",
		"dump reserves",
		dump_reserves,
		NULL,
	},
	{
		"drr",
		"dump results",
		dump_results,
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

