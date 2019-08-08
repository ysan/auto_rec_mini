#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "EventScheduleManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void _cacheSchedule_currentService (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.reqCacheSchedule_currentService ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_scheduleMap (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpScheduleMap ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_schedule (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 3) {
		_UTL_LOG_W ("invalid arguments.\n");
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


	CEventScheduleManagerIf::SERVICE_KEY_t key = {_tsid, _org_nid, _svc_id};


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpSchedule (&key);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _getEvent (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	std::regex regex_idx ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_idx)) {
		_UTL_LOG_E ("invalid arguments. (index)");
		return;
	}
	uint16_t _idx = atoi (argv[0]);


	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
	_param.arg.index = _idx ;


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpEvent_latestDumpedSchedule (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _getEvents_keywordSearch (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_W ("invalid arguments.\n");
		return;
	}

	CEventScheduleManagerIf::EVENT_t _ev [10] = {0};
	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
	_param.arg.p_keyword = argv[0];
	_param.p_out_event = _ev;
	_param.array_max_num = 10;

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.syncGetEvent_keyword (&_param);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		int n =  *(int*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("syncGetEvent_keyword success");
		_UTL_LOG_I ("num of searches is [%d]", n);
		for (int i = 0; i < n; ++ i) {
			_UTL_LOG_I (
				"%d: tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
				i,
				_ev[i].transport_stream_id,
				_ev[i].original_network_id,
				_ev[i].service_id,
				_ev[i].event_id
			);
			_UTL_LOG_I (
				"    [%s - %s][%s]",
				_ev[i].start_time.toString(),
				_ev[i].end_time.toString(),
				_ev[i].p_event_name->c_str()
			);
		}

	} else {
		_UTL_LOG_I ("syncGetEvent_keyword error");
	}
}

static void _getEvents_keywordSearch_ex (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_W ("invalid arguments.\n");
		return;
	}

	CEventScheduleManagerIf::EVENT_t _ev [10] = {0};
	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
	_param.arg.p_keyword = argv[0];
	_param.p_out_event = _ev;
	_param.array_max_num = 10;

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.syncGetEvent_keyword_ex (&_param);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		int n =  *(int*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("syncGetEvent_keyword_ex success");
		_UTL_LOG_I ("num of searches is [%d]", n);
		for (int i = 0; i < n; ++ i) {
			_UTL_LOG_I (
				"%d: tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
				i,
				_ev[i].transport_stream_id,
				_ev[i].original_network_id,
				_ev[i].service_id,
				_ev[i].event_id
			);
			_UTL_LOG_I (
				"    [%s - %s][%s]",
				_ev[i].start_time.toString(),
				_ev[i].end_time.toString(),
				_ev[i].p_event_name->c_str()
			);
		}

	} else {
		_UTL_LOG_I ("syncGetEvent_keyword_ex error");
	}
}

static void _dump_histories (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpHistories ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_eventScheduleManagerCommands [] = { // extern
	{
		"cs",
		"cache schedule --current service--",
		_cacheSchedule_currentService,
		NULL,
	},
	{
		"dm",
		"dump schedule map",
		_dump_scheduleMap,
		NULL,
	},
	{
		"d",
		"dump schedule (usage: d {tsid} {org_nid} {svcid} )",
		_dump_schedule,
		NULL,
	},
	{
		"g",
		"get event --from last dumped schedule-- (usage: g {index})",
		_getEvent,
		NULL,
	},
	{
		"grep",
		"grep events --search event name by keyword-- (usage: grep {keyword})",
		_getEvents_keywordSearch,
		NULL,
	},
	{
		"grepx",
		"grep events --search event name and extended event by keyword-- (usage: grepx {keyword})",
		_getEvents_keywordSearch_ex,
		NULL,
	},
	{
		"h",
		"dump histories",
		_dump_histories,
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

