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
	if (argc != 6) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	uint16_t _tsid = 0;
	std::regex regex_tsid ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_tsid)) {
		std::regex regex_tsid ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[0], regex_tsid)) {
			_UTL_LOG_E ("invalid arguments. (tsid)");
			return;
		} else {
			_tsid = strtol (argv[0], NULL, 16);
		}
	} else {
		_tsid = atoi (argv[0]);
	}

	uint16_t _org_nid = 0;
	std::regex regex_org_nid ("^[0-9]+$");
	if (!std::regex_match (argv[1], regex_org_nid)) {
		std::regex regex_org_nid ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[1], regex_org_nid)) {
			_UTL_LOG_E ("invalid arguments. (org_nid)");
			return;
		} else {
			_org_nid = strtol (argv[1], NULL, 16);
		}
	} else {
		_org_nid = atoi (argv[1]);
	}

	uint16_t _svc_id = 0;
	std::regex regex_svc_id("^[0-9]+$");
	if (!std::regex_match (argv[2], regex_svc_id)) {
		std::regex regex_svc_id ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[2], regex_svc_id)) {
			_UTL_LOG_E ("invalid arguments. (svc_id)");
			return;
		} else {
			_svc_id = strtol (argv[2], NULL, 16);
		}
	} else {
		_svc_id = atoi (argv[2]);
	}

	std::regex regex_start_time("^[0-9]{14}$");
	if (!std::regex_match (argv[3], regex_start_time)) {
		_UTL_LOG_E ("invalid arguments. (start_time)");
		return;
	}

	std::regex regex_end_time("^[0-9]{14}$");
	if (!std::regex_match (argv[4], regex_end_time)) {
		_UTL_LOG_E ("invalid arguments. (end_time)");
		return;
	}

	time_t _start = dateString2epoch (argv[3]);
	if (_start == 0) {
		_UTL_LOG_E ("invalid arguments. (start_time:dateString2epoch)");
		return;
	}

	time_t _end = dateString2epoch (argv[4]);
	if (_end == 0) {
		_UTL_LOG_E ("invalid arguments. (end_time:dateString2epoch)");
		return;
	}

	if (_start >= _end) {
		_UTL_LOG_E ("invalid arguments. (start_time >= end_time)");
		return;
	}

	CEtime s (_start);
	CEtime e (_end);

	std::regex regex_repeat("^[0-2]$");
	if (!std::regex_match (argv[5], regex_repeat)) {
		_UTL_LOG_E ("invalid arguments. (repeat)");
		return;
	}
	EN_RESERVE_REPEATABILITY r = (EN_RESERVE_REPEATABILITY) atoi (argv[5]);


	CRecManagerIf::ADD_RESERVE_PARAM_t _param;
	_param.transport_stream_id = _tsid ;
	_param.original_network_id = _org_nid;
	_param.service_id = _svc_id;
	_param.start_time = s;
	_param.end_time = e;
	_param.repeatablity = r;

	_param.dump();


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqAddReserve_manual (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void addReserve_event (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 5) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	uint16_t _tsid = 0;
	std::regex regex_tsid ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_tsid)) {
		std::regex regex_tsid ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[0], regex_tsid)) {
			_UTL_LOG_E ("invalid arguments. (tsid)");
			return;
		} else {
			_tsid = strtol (argv[0], NULL, 16);
		}
	} else {
		_tsid = atoi (argv[0]);
	}

	uint16_t _org_nid = 0;
	std::regex regex_org_nid ("^[0-9]+$");
	if (!std::regex_match (argv[1], regex_org_nid)) {
		std::regex regex_org_nid ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[1], regex_org_nid)) {
			_UTL_LOG_E ("invalid arguments. (org_nid)");
			return;
		} else {
			_org_nid = strtol (argv[1], NULL, 16);
		}
	} else {
		_org_nid = atoi (argv[1]);
	}

	uint16_t _svc_id = 0;
	std::regex regex_svc_id("^[0-9]+$");
	if (!std::regex_match (argv[2], regex_svc_id)) {
		std::regex regex_svc_id ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[2], regex_svc_id)) {
			_UTL_LOG_E ("invalid arguments. (svc_id)");
			return;
		} else {
			_svc_id = strtol (argv[2], NULL, 16);
		}
	} else {
		_svc_id = atoi (argv[2]);
	}

	uint16_t _evt_id = 0;
	std::regex regex_evt_id("^[0-9]+$");
	if (!std::regex_match (argv[3], regex_evt_id)) {
		std::regex regex_evt_id ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[3], regex_evt_id)) {
			_UTL_LOG_E ("invalid arguments. (evt_id)");
			return;
		} else {
			_evt_id = strtol (argv[3], NULL, 16);
		}
	} else {
		_evt_id = atoi (argv[3]);
	}


	std::regex regex_repeat("^[0-1]$");
	if (!std::regex_match (argv[4], regex_repeat)) {
		_UTL_LOG_E ("invalid arguments. (repeat)");
		return;
	}
	int rpt = atoi (argv[4]);
	EN_RESERVE_REPEATABILITY r = rpt == 0 ? EN_RESERVE_REPEATABILITY__NONE: EN_RESERVE_REPEATABILITY__AUTO;


	CRecManagerIf::ADD_RESERVE_PARAM_t _param;
	_param.transport_stream_id = _tsid ;
	_param.original_network_id = _org_nid;
	_param.service_id = _svc_id;
	_param.event_id = _evt_id;
	_param.repeatablity = r;

	_param.dump();


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqAddReserve_event (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void addReserve_event_helper (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 2) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_idx ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_idx)) {
		_UTL_LOG_E ("invalid arguments. (index)");
		return;
	}
	uint16_t _idx = atoi (argv[0]);


	std::regex regex_repeat("^[0-1]$");
	if (!std::regex_match (argv[1], regex_repeat)) {
		_UTL_LOG_E ("invalid arguments. (repeat)");
		return;
	}
	int rpt = atoi (argv[1]);
	EN_RESERVE_REPEATABILITY r = rpt == 0 ? EN_RESERVE_REPEATABILITY__NONE: EN_RESERVE_REPEATABILITY__AUTO;


	CRecManagerIf::ADD_RESERVE_HELPER_PARAM_t _param;
	_param.index = _idx ;
	_param.repeatablity = r;


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqAddReserve_event_helper (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void removeReserve (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 2) {
		_UTL_LOG_E ("invalid arguments.");
		return ;
	}

	std::regex regex_idx ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_idx)) {
		_UTL_LOG_E ("invalid arguments. (index)");
		return;
	}

	std::regex regex_rep ("^[0-1]$");
	if (!std::regex_match (argv[1], regex_rep)) {
		_UTL_LOG_E ("invalid arguments. (consider repeatability)");
		return;
	}

	int index = atoi(argv[0]);
	bool isConsiderRepeatability = (atoi(argv[1]) == 0 ? false : true);

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CRecManagerIf::REMOVE_RESERVE_PARAM_t param = {index, isConsiderRepeatability};

	CRecManagerIf mgr(pBase->getExternalIf());
	mgr.reqRemoveReserve (&param);

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
		"ce",
		"add reserve - CurrentEvent",
		addReserve_currentEvent,
		NULL,
	},
	{
		"e",
		"add reserve - event\n\
                                (usage: e {tsid} {org_nid} {svcid} {evtid} {repeat} )\n\
                                           - repeat is 0 (none), 1 (auto)",
		addReserve_event,
		NULL,
	},
	{
		"eh",
		"add reserve - event (helper)\n\
                                (usage: eh {index} {repeat} )\n\
                                           - repeat is 0 (none), 1 (auto)",
		addReserve_event_helper,
		NULL,
	},
	{
		"m",
		"add reserve - Manual\n\
                                (usage: m {tsid} {org_nid} {svcid} {start_time} {end_time} {repeat} )\n\
                                           - start_time, end_time format is \"yyyyMMddHHmmss\"\n\
                                           - repeat is 0 (none), 1 (daily), 2 (weekly)",
		addReserve_manual,
		NULL,
	},
	{
		"r",
		"remove reserve (usage: r {index} {consider repeatability} )\n\
                                           - consider repeatability is 0 (false), 1 (true)",
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

