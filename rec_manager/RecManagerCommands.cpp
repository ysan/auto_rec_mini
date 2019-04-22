#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "RecManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void addReserve_currentEvent (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqAddReserve_currentEvent ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

// yyyyMMddHHmmss to epoch time 
static time_t dateString2epoch (char *pszDate)
{
	if (!pszDate) {
		return 0;
	}

	std::regex regex("[0-9]{14}");
	if (!std::regex_match (pszDate, regex)) {
		return 0;
	}

	struct tm t;
	int off = 0;

	// year
	char sz_year [4+1] = {0};
	strncpy (sz_year, pszDate, 4);
	int y = atoi (sz_year);
	t.tm_year = y -1900;

	off += 4;

	// month
	char sz_month [2+1] = {0};
	strncpy (sz_month, pszDate + off, 2);
	int mon = atoi (sz_month);
	t.tm_mon = mon -1;
	
	off += 2;

	// day
	char sz_day [2+1] = {0};
	strncpy (sz_day, pszDate + off, 2);
	int d = atoi (sz_day);
	t.tm_mday = d;

	off += 2;

	// hour
	char sz_hour [2+1] = {0};
	strncpy (sz_hour, pszDate + off, 2);
	int h = atoi (sz_hour);
	t.tm_hour = h;

	off += 2;

	// min
	char sz_min [2+1] = {0};
	strncpy (sz_min, pszDate + off, 2);
	int min = atoi (sz_min);
	t.tm_min = min;

	off += 2;

	// sec
	char sz_sec [2+1] = {0};
	strncpy (sz_sec, pszDate + off, 2);
	int s = atoi (sz_sec);
	t.tm_sec = s;

	return mktime (&t);
}

static void addReserve_manual (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 5) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	uint16_t _ts_id = 0;
	std::regex regex_ts_id ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_ts_id)) {
		std::regex regex_ts_id ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[0], regex_ts_id)) {
			_UTL_LOG_E ("invalid arguments.");
			return;
		} else {
			_ts_id = strtol (argv[0], NULL, 16);
		}
	} else {
		_ts_id = atoi (argv[0]);
	}

	uint16_t _n_id = 0;
	std::regex regex_n_id ("^[0-9]+$");
	if (!std::regex_match (argv[1], regex_n_id)) {
		std::regex regex_n_id ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[1], regex_n_id)) {
			_UTL_LOG_E ("invalid arguments.");
			return;
		} else {
			_n_id = strtol (argv[1], NULL, 16);
		}
	} else {
		_n_id = atoi (argv[1]);
	}

	uint16_t _svc_id = 0;
	std::regex regex_svc_id("^[0-9]+$");
	if (!std::regex_match (argv[2], regex_svc_id)) {
		std::regex regex_svc_id ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[2], regex_svc_id)) {
			_UTL_LOG_E ("invalid arguments.");
			return;
		} else {
			_svc_id = strtol (argv[2], NULL, 16);
		}
	} else {
		_svc_id = atoi (argv[2]);
	}

	std::regex regex_start_time("^[0-9]{14}$");
	if (!std::regex_match (argv[3], regex_start_time)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_end_time("^[0-9]{14}$");
	if (!std::regex_match (argv[4], regex_end_time)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	time_t _start = dateString2epoch (argv[3]);
	if (_start == 0) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	time_t _end = dateString2epoch (argv[4]);
	if (_end == 0) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	if (_start >= _end) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	CEtime s (_start);
	CEtime e (_end);

	_MANUAL_RESERVE_PARAM _param;
	_param.transport_stream_id = _ts_id ;
	_param.original_network_id = _n_id;
	_param.service_id = _svc_id;
	_param.start_time = s;
	_param.end_time = e;

	_param.dump();

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqAddReserve_manual (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void removeReserve (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments.");
		return ;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	int idx = atoi(argv[0]);

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqRemoveReserve (idx);

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
		"e",
		"add reserve - CurrentEvent",
		addReserve_currentEvent,
		NULL,
	},
	{
		"m",
		"add reserve - Manual (usage: m {ts_id} {n_id} {service_id} {start_time} {end_time}\n\
                                                         start_time, end_time format is \"yyyyMMddHHmmss\")",
		addReserve_manual,
		NULL,
	},
	{
		"r",
		"remove reserve (udage: r {idx})",
		removeReserve,
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
		"drl",
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

