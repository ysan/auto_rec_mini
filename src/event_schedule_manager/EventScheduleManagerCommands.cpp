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


static void _cache_schedule (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_cache_schedule ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _cache_schedule_force_current_service (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: csc {group_id})\n");
		return ;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (group_id)\n");
		return;
	}
	uint8_t group_id = atoi(argv[0]);

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_cache_schedule_force_current_service (group_id);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _stop_cache_schedule (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_stop_cache_schedule_sync ();

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		_COM_SVR_PRINT ("sync_stop_cache_schedule is success.\n");
	} else {
		_COM_SVR_PRINT ("sync_stop_cache_schedule is failure.\n");
	}
}

static void _dump_schedule_map (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_dump_schedule_map ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _dump_schedule (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
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


	CEventScheduleManagerIf::service_key_t key = {_tsid, _org_nid, _svc_id};


	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_dump_schedule (&key);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _dump_schedule_interactive (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CCommandServer* pcs = dynamic_cast <CCommandServer*> (base);
	int fd = pcs->get_client_fd();

	char buf[32] = {0};


	CChannelManagerIf::channel_t channels[20] = {0};
	CChannelManagerIf::request_channels_param_t param = {channels, 20};
	CChannelManagerIf _if (base->get_external_if());

	_if.request_get_channels_sync (&param);
	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::error) {
		_COM_SVR_PRINT ("syncGetChannels is failure.\n");
		return ;
	}

	int ch_num = *(int*)(base->get_if()->get_source().get_message().data());
	_COM_SVR_PRINT ("syncGetChannels ch_num:[%d]\n", ch_num);

	if (ch_num == 0) {
		_COM_SVR_PRINT ("syncGetChannels is 0\n");
		return ;
	}


	// list ts channel
	for (int i = 0; i < 20; ++ i) {
		CChannelManagerIf::service_id_param_t param = {
			channels[i].transport_stream_id,
			channels[i].original_network_id,
			0 // no need service_id
		};
		_if.request_get_transport_stream_name_sync (&param);
		threadmgr::result rslt = base->get_if()->get_source().get_result();
		if (rslt == threadmgr::result::error) {
			continue ;
		}
		char* ts_name = (char*)(base->get_if()->get_source().get_message().data());
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, ts_name);
	}
	_COM_SVR_PRINT ("  'q': exit\n");

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

			if (buf[0] == 'q' && strlen(buf) == 1) {
				return;
			}

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
		CChannelManagerIf::service_id_param_t param = {
			channels[sel_ch_num].transport_stream_id,
			channels[sel_ch_num].original_network_id,
			channels[sel_ch_num].service_ids[i]
		};
		_if.request_get_service_name_sync (&param);
		threadmgr::result rslt = base->get_if()->get_source().get_result();
		if (rslt == threadmgr::result::error) {
			continue ;
		}
		char* svc_name = (char*)(base->get_if()->get_source().get_message().data());
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, svc_name);
	}
	_COM_SVR_PRINT ("  'q': exit\n");

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

			if (buf[0] == 'q' && strlen(buf) == 1) {
				return;
			}

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


	CEventScheduleManagerIf::service_key_t key = {
		channels[sel_ch_num].transport_stream_id,
		channels[sel_ch_num].original_network_id,
		channels[sel_ch_num].service_ids[sel_svc_num]
	};

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf em(base->get_external_if());
	em.request_dump_schedule (&key);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _dump_event_detail (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
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


	CEventScheduleManagerIf::request_event_param_t _param;
	_param.arg.index = _idx ;


	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_dump_event_latest_dumped_schedule (&_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _get_events_keyword_search (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	CEventScheduleManagerIf::event_t _ev [50] = {0};
	CEventScheduleManagerIf::request_event_param_t _param;
	_param.arg.p_keyword = argv[0];
	_param.p_out_event = _ev;
	_param.array_max_num = 50;

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_get_events_keyword_sync (&_param);

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		int n =  *(int*)(base->get_if()->get_source().get_message().data());
		_COM_SVR_PRINT ("sync_get_events_keyword success\n");
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
		_COM_SVR_PRINT ("sync_get_events_keyword error\n");
	}
}

static void _get_events_keyword_search_ex (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	CEventScheduleManagerIf::event_t _ev [50] = {0};
	CEventScheduleManagerIf::request_event_param_t _param;
	_param.arg.p_keyword = argv[0];
	_param.p_out_event = _ev;
	_param.array_max_num = 50;

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_get_events_keyword_ex_sync (&_param);

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		int n =  *(int*)(base->get_if()->get_source().get_message().data());
		_COM_SVR_PRINT ("sync_get_events_keyword_ex success\n");
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
		_COM_SVR_PRINT ("sync_get_events_keyword_ex error\n");
	}
}

static void _dump_reserves (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_dump_reserves ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _dump_histories (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventScheduleManagerIf _if (base->get_external_if());
	_if.request_dump_histories ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}


command_info_t g_event_schedule_manager_commands [] = { // extern
	{
		"cs",
		"cache schedule",
		_cache_schedule,
		NULL,
	},
	{
		"csc",
		"cache schedule --force current service-- (usage: csc {group_id})",
		_cache_schedule_force_current_service,
		NULL,
	},
	{
		"stop",
		"stop cache schedule",
		_stop_cache_schedule,
		NULL,
	},
	{
		"dm",
		"dump schedule map",
		_dump_schedule_map,
		NULL,
	},
	{
		"d",
		"dump schedule (usage: d {tsid} {org_nid} {svcid})",
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
		_dump_event_detail,
		NULL,
	},
	{
		"grep",
		"grep events --search event name by keyword-- (usage: grep {keyword})",
		_get_events_keyword_search,
		NULL,
	},
	{
		"grepx",
		"grep events --search event name and extended event by keyword-- (usage: grepx {keyword})",
		_get_events_keyword_search_ex,
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

