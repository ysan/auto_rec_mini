#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "channel_manager_if.h"
#include "tuner_service_if.h"
#include "command_tables.h"
#include "command_server_log.h"
#include "command_server.h"
#include "utils.h"
#include "modules.h"


static void _scan (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}


	arec::channel::client_iface _if(modules::get_manager());
	_if.request_channel_scan ();

}

static void _tune_interactive (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint8_t group_id = 0xff;
	{
		arec::tuner_service::client_iface _if(modules::get_manager());
		arec::client_reply rep = _if.request_open_sync();
		if (rep.get_result() == coseq::result::success) {
			group_id = *(uint8_t*)(rep.get_message().data());
			_COM_SVR_PRINT ("open: group_id=[%d]\n", group_id);
		} else {
			_COM_SVR_PRINT ("open: failure.\n");
			return;
		}
	}

	arec::command_server::manager* pcs = dynamic_cast <arec::command_server::manager*> (base);
	int fd = pcs->get_client_fd();

	char buf[32] = {0};


	arec::channel::client_iface::channel_t channels[20] = {0};
	arec::channel::client_iface::request_channels_param_t param = {channels, 20};
	arec::channel::client_iface _if(modules::get_manager());
	arec::client_reply rep = _if.request_get_channels_sync (&param);
	if (rep.get_result() == coseq::result::error) {
		_COM_SVR_PRINT ("request_get_channels_sync is failure.\n");
		return ;
	}

	int ch_num = *(int*)(rep.get_message().data());
	_COM_SVR_PRINT ("request_get_channels_sync ch_num:[%d]\n", ch_num);

	if (ch_num == 0) {
		_COM_SVR_PRINT ("request_get_channels_sync is 0\n");
		return ;
	}


	// list ts channel
	for (int i = 0; i < 20; ++ i) {
		arec::channel::client_iface::service_id_param_t param = {
			channels[i].transport_stream_id,
			channels[i].original_network_id,
			0 // no need service_id
		};
		arec::channel::client_iface _if(modules::get_manager());
		arec::client_reply rep = _if.request_get_transport_stream_name_sync (&param);
		if (rep.get_result() == coseq::result::error) {
			continue ;
		}
		char* ts_name = (char*)(rep.get_message().data());
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, ts_name);
	}
	_COM_SVR_PRINT ("  'q': exit\n");

	// select ts channel
	int sel_ch_num = 0;
	while (1) {
		memset (buf, 0x00, sizeof(buf));
		_COM_SVR_PRINT ("select channel. # ");

		int rSize = arec::common::utils::recv_data (fd, (uint8_t*)buf, sizeof(buf), NULL);
		if (rSize <= 0) {
			continue;
		} else {
			arec::common::utils::delete_lf (buf);
			arec::common::utils::delete_head_sp (buf);
			arec::common::utils::delete_tail_sp (buf);

			if (buf[0] == 'q' && strlen(buf) == 1) {
				arec::tuner_service::client_iface _if(modules::get_manager());
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
		arec::channel::client_iface::service_id_param_t param = {
			channels[sel_ch_num].transport_stream_id,
			channels[sel_ch_num].original_network_id,
			channels[sel_ch_num].service_ids[i]
		};
		arec::client_reply rep = _if.request_get_service_name_sync (&param);
		if (rep.get_result() == coseq::result::error) {
			continue ;
		}
		char* svc_name = (char*)(rep.get_message().data());
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, svc_name);
	}
	_COM_SVR_PRINT ("  'q': exit\n");

	// select service
	int sel_svc_num = 0;
	while (1) {
		memset (buf, 0x00, sizeof(buf));
		_COM_SVR_PRINT ("select service. # ");

		int rSize = arec::common::utils::recv_data (fd, (uint8_t*)buf, sizeof(buf), NULL);
		if (rSize <= 0) {
			continue;
		} else {
			arec::common::utils::delete_lf (buf);
			arec::common::utils::delete_head_sp (buf);
			arec::common::utils::delete_tail_sp (buf);

			if (buf[0] == 'q' && strlen(buf) == 1) {
				arec::tuner_service::client_iface _if(modules::get_manager());
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


	arec::tuner_service::client_iface::tune_param_t tune_param = {
		channels[sel_ch_num].physical_channel,
		group_id,
	};


	
	arec::tuner_service::client_iface _ts_if(modules::get_manager());
	_ts_if.request_tune (&tune_param);
	
}

static void _tune_stop (int argc, char* argv[], coseq::module_base *base)
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
		arec::tuner_service::client_iface _if(modules::get_manager());
		arec::client_reply rep = _if.request_tune_stop_sync (group_id);
		if (rep.get_result() == coseq::result::success) {
			_COM_SVR_PRINT ("tune stop success\n");
		} else {
			_COM_SVR_PRINT ("tune stop error\n");
		}
	}
	{
		arec::tuner_service::client_iface _if(modules::get_manager());
		arec::client_reply rep = _if.request_close_sync (group_id);
		if (rep.get_result() == coseq::result::success) {
			_COM_SVR_PRINT ("close success\n");
		} else {
			_COM_SVR_PRINT ("close error\n");
		}
	}
}

static void _dump_channels (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}


	arec::channel::client_iface _if(modules::get_manager());
	_if.request_dump_channels ();

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

