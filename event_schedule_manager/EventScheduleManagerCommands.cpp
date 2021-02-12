#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "EventScheduleManagerIf.h"
#include "ChannelManagerIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "CommandServer.h"
#include "Utils.h"


static void _cacheSchedule (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqCacheSchedule ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _cacheSchedule_forceCurrentService (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("ignore arguments. (usage: csc {groupId} )\n");
		return ;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (group_id)\n");
		return;
	}
	uint8_t group_id = atoi(argv[0]);

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqCacheSchedule_forceCurrentService (group_id);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _stopCacheSchedule (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.syncStopCacheSchedule ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("syncStopCacheSchedule is success.\n");
	} else {
		_COM_SVR_PRINT ("syncStopCacheSchedule is failure.\n");
	}
}

static void _dump_scheduleMap (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqDumpScheduleMap ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_schedule (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 3) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	uint16_t _tsid = 0;
	std::regex regex_tsid ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_tsid)) {
		std::regex regex_tsid ("^0x([0-9]|[a-f]|[A-F])+$");
		if (!std::regex_match (argv[0], regex_tsid)) {
			_COM_SVR_PRINT ("invalid arguments. (tsid)\n");
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
			_COM_SVR_PRINT ("invalid arguments. (org_nid)\n");
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
			_COM_SVR_PRINT ("invalid arguments. (svc_id)\n");
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

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqDumpSchedule (&key);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_schedule_interactive (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CCommandServer* pcs = dynamic_cast <CCommandServer*> (pBase);
	int fd = pcs->getClientFd();

	char buf[32] = {0};


	CChannelManagerIf::CHANNEL_t channels[20] = {0};
	CChannelManagerIf::REQ_CHANNELS_PARAM_t param = {channels, 20};
	CChannelManagerIf _if (pBase->getExternalIf());

	_if.syncGetChannels (&param);
	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_ERROR) {
		_COM_SVR_PRINT ("syncGetChannels is failure.\n");
		return ;
	}

	int ch_num = *(int*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
	_COM_SVR_PRINT ("syncGetChannels ch_num:[%d]\n", ch_num);

	if (ch_num == 0) {
		_COM_SVR_PRINT ("syncGetChannels is 0\n");
		return ;
	}


	// list ts channel
	for (int i = 0; i < 20; ++ i) {
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			channels[i].transport_stream_id,
			channels[i].original_network_id,
			0 // no need service_id
		};
		_if.syncGetTransportStreamName (&param);
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_ERROR) {
			continue ;
		}
		char* ts_name = (char*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, ts_name);
	}

	// select ts channel
	int sel_ch_num = 0;
	while (1) {
		memset (buf, 0x00, sizeof(buf));
		_COM_SVR_PRINT ("select channel. # ");

		int rSize = CUtils::recvData (fd, (uint8_t*)buf, sizeof(buf), NULL);
		if (rSize <= 0) {
			continue;
		} else {
			CUtils::deleteLF (buf);
			CUtils::deleteHeadSp (buf);
			CUtils::deleteTailSp (buf);
			std::regex regex_num("^[0-9]+$");
			if (!std::regex_match (buf, regex_num)) {
				continue;
			}

			sel_ch_num = atoi(buf);
			if (sel_ch_num < ch_num) {
				break;
			}
		}
	}

	// list service
	for (int i = 0; i < channels[sel_ch_num].service_num; ++ i) {
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			channels[sel_ch_num].transport_stream_id,
			channels[sel_ch_num].original_network_id,
			channels[sel_ch_num].service_ids[i]
		};
		_if.syncGetServiceName (&param);
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_ERROR) {
			continue ;
		}
		char* svc_name = (char*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, svc_name);
	}

	// select service
	int sel_svc_num = 0;
	while (1) {
		memset (buf, 0x00, sizeof(buf));
		_COM_SVR_PRINT ("select service. # ");

		int rSize = CUtils::recvData (fd, (uint8_t*)buf, sizeof(buf), NULL);
		if (rSize <= 0) {
			continue;
		} else {
			CUtils::deleteLF (buf);
			CUtils::deleteHeadSp (buf);
			CUtils::deleteTailSp (buf);
			std::regex regex_num("^[0-9]+$");
			if (!std::regex_match (buf, regex_num)) {
				continue;
			}

			sel_svc_num = atoi(buf);
			if (sel_svc_num < channels[sel_ch_num].service_num) {
				break;
			}
		}
	}


	CEventScheduleManagerIf::SERVICE_KEY_t key = {
		channels[sel_ch_num].transport_stream_id,
		channels[sel_ch_num].original_network_id,
		channels[sel_ch_num].service_ids[sel_svc_num]
	};

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf em(pBase->getExternalIf());
	em.reqDumpSchedule (&key);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dumpEvent_detail (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	std::regex regex_idx ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_idx)) {
		_COM_SVR_PRINT ("invalid arguments. (index)\n");
		return;
	}
	uint16_t _idx = atoi (argv[0]);


	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
	_param.arg.index = _idx ;


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqDumpEvent_latestDumpedSchedule (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _getEvents_keywordSearch (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	CEventScheduleManagerIf::EVENT_t _ev [10] = {0};
	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
	_param.arg.p_keyword = argv[0];
	_param.p_out_event = _ev;
	_param.array_max_num = 10;

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.syncGetEvents_keyword (&_param);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		int n =  *(int*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("syncGetEvents_keyword success\n");
		_COM_SVR_PRINT ("num of searches is [%d]\n", n);
		for (int i = 0; i < n; ++ i) {
			_COM_SVR_PRINT (
				"%d: tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]\n",
				i,
				_ev[i].transport_stream_id,
				_ev[i].original_network_id,
				_ev[i].service_id,
				_ev[i].event_id
			);
			_COM_SVR_PRINT (
				"    [%s - %s][%s]\n",
				_ev[i].start_time.toString(),
				_ev[i].end_time.toString(),
				_ev[i].p_event_name->c_str()
			);
		}

	} else {
		_COM_SVR_PRINT ("syncGetEvents_keyword error\n");
	}
}

static void _getEvents_keywordSearch_ex (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	CEventScheduleManagerIf::EVENT_t _ev [10] = {0};
	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
	_param.arg.p_keyword = argv[0];
	_param.p_out_event = _ev;
	_param.array_max_num = 10;

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.syncGetEvents_keyword_ex (&_param);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		int n =  *(int*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("syncGetEvents_keyword_ex success\n");
		_COM_SVR_PRINT ("num of searches is [%d]\n", n);
		for (int i = 0; i < n; ++ i) {
			_COM_SVR_PRINT (
				"%d: tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]\n",
				i,
				_ev[i].transport_stream_id,
				_ev[i].original_network_id,
				_ev[i].service_id,
				_ev[i].event_id
			);
			_COM_SVR_PRINT (
				"    [%s - %s][%s]\n",
				_ev[i].start_time.toString(),
				_ev[i].end_time.toString(),
				_ev[i].p_event_name->c_str()
			);
		}

	} else {
		_COM_SVR_PRINT ("syncGetEvents_keyword_ex error\n");
	}
}

static void _dump_reserves (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqDumpReserves ();

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

	CEventScheduleManagerIf _if (pBase->getExternalIf());
	_if.reqDumpHistories ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_eventScheduleManagerCommands [] = { // extern
	{
		"cs",
		"cache schedule",
		_cacheSchedule,
		NULL,
	},
	{
		"csc",
		"cache schedule --force current service-- (usage: csc {group_id} )",
		_cacheSchedule_forceCurrentService,
		NULL,
	},
	{
		"stop",
		"stop cache schedule",
		_stopCacheSchedule,
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
		"di",
		"dump schedule (interactive mode)",
		_dump_schedule_interactive,
		NULL,
	},
	{
		"ded",
		"dump event detail --from last dumped schedule-- (usage: ded {index})",
		_dumpEvent_detail,
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
		"dr",
		"dump reserves",
		_dump_reserves,
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

