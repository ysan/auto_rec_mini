#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "ChannelManagerIf.h"
#include "TunerServiceIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "CommandServer.h"
#include "Utils.h"


static void _scan (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CChannelManagerIf _if(base->get_external_if());
	_if.request_channel_scan ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _tune_interactive (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint8_t group_id = 0xff;
	{
		CTunerServiceIf _if(base->get_external_if());
		_if.request_open_sync();
		threadmgr::result rslt = base->get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			group_id = *(uint8_t*)(base->get_if()->get_source().get_message().data());
			_COM_SVR_PRINT ("open: group_id=[%d]\n", group_id);
		} else {
			_COM_SVR_PRINT ("open: failure.\n");
			return;
		}
	}

	CCommandServer* pcs = dynamic_cast <CCommandServer*> (base);
	int fd = pcs->get_client_fd();

	char buf[32] = {0};


	CChannelManagerIf::channel_t channels[20] = {0};
	CChannelManagerIf::request_channels_param_t param = {channels, 20};
	CChannelManagerIf _if(base->get_external_if());
	_if.request_get_channels_sync (&param);
	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::error) {
		_COM_SVR_PRINT ("request_get_channels_sync is failure.\n");
		return ;
	}

	int ch_num = *(int*)(base->get_if()->get_source().get_message().data());
	_COM_SVR_PRINT ("request_get_channels_sync ch_num:[%d]\n", ch_num);

	if (ch_num == 0) {
		_COM_SVR_PRINT ("request_get_channels_sync is 0\n");
		return ;
	}


	// list ts channel
	for (int i = 0; i < 20; ++ i) {
		CChannelManagerIf::service_id_param_t param = {
			channels[i].transport_stream_id,
			channels[i].original_network_id,
			0 // no need service_id
		};
		CChannelManagerIf _if(base->get_external_if());
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
				CTunerServiceIf _if(base->get_external_if());
				_if.request_close_sync (group_id);
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
				CTunerServiceIf _if(base->get_external_if());
				_if.request_close_sync (group_id);
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


	CTunerServiceIf::tune_param_t tune_param = {
		channels[sel_ch_num].pysical_channel,
		group_id,
	};


	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
	
	CTunerServiceIf _ts_if(base->get_external_if());
	_ts_if.request_tune (&tune_param);
	
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _tune_stop (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: stop {group_id})\n");
		return;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (group_id)\n");
		return;
	}
	uint8_t group_id = atoi(argv[0]);

	{
		CTunerServiceIf _if(base->get_external_if());
		_if.request_tune_stop_sync (group_id);
	
		threadmgr::result rslt = base->get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			_COM_SVR_PRINT ("tune stop success\n");
		} else {
			_COM_SVR_PRINT ("tune stop error\n");
		}
	}
	{
		CTunerServiceIf _if(base->get_external_if());
		_if.request_close_sync (group_id);
	
		threadmgr::result rslt = base->get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			_COM_SVR_PRINT ("close success\n");
		} else {
			_COM_SVR_PRINT ("close error\n");
		}
	}
}

static void _dump_channels (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CChannelManagerIf _if(base->get_external_if());
	_if.request_dump_channels ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}


command_info_t g_channel_manager_commands [] = { // extern
	{
		"scan",
		"channel scan",
		_scan,
		NULL,
	},
	{
		"t",
		"tune (interactive mode)",
		_tune_interactive,
		NULL,
	},
	{
		"stop",
		"tune stop (usage: stop {group_id})",
		_tune_stop,
		NULL,
	},
	{
		"d",
		"dump channels",
		_dump_channels,
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

