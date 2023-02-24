#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "ThreadMgrIf.h"
#include "ViewingManagerIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Utils.h"
#include "modules.h"


static void start_by_phy_ch (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 2) {
		_COM_SVR_PRINT ("invalid arguments. (usage: ch {physical_channel, service_idx})\n");
		return ;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (physical_channel)\n");
		return;
	}
	if (!std::regex_match (argv[1], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (service_idx)\n");
		return;
	}
	_COM_SVR_PRINT ("physical_channel %s\n", argv[0]);
	_COM_SVR_PRINT ("service_idx %s\n", argv[1]);

	uint16_t ch = atoi(argv[0]);
	int svc_idx = atoi(argv[1]);
	CViewingManagerIf::physical_channel_param_t param = {ch, svc_idx};

//	uint32_t opt = base->get_external_if()->get_request_option ();
//	opt |= REQUEST_OPTION__WITHOUT_REPLY;
//	base->get_external_if()->set_request_option (opt);

	CViewingManagerIf _if (base->get_external_if());
	_if.request_start_viewing_by_physical_channel (&param, false);
	threadmgr::result r = base->get_if()->get_source().get_result();
	if (r == threadmgr::result::success) {
		uint8_t _gr = *(reinterpret_cast<uint8_t*>(base->get_if()->get_source().get_message().data()));
		_COM_SVR_PRINT ("success -> group %d\n", _gr);
	}

//	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
//	base->get_external_if()->set_request_option (opt);
}

static void start_by_svc_id (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 3) {
		_COM_SVR_PRINT ("invalid arguments. (usage: svc {tsid} {org_nid} {svcid})\n");
		return ;
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

	_COM_SVR_PRINT ("tsid %s\n", argv[0]);
	_COM_SVR_PRINT ("org_nid %s\n", argv[1]);
	_COM_SVR_PRINT ("svcid %s\n", argv[2]);

	CViewingManagerIf::service_id_param_t param = {_tsid, _org_nid, _svc_id};

//	uint32_t opt = base->get_external_if()->get_request_option ();
//	opt |= REQUEST_OPTION__WITHOUT_REPLY;
//	base->get_external_if()->set_request_option (opt);

	CViewingManagerIf _if (base->get_external_if());
	_if.request_start_viewing_by_service_id (&param, false);
	threadmgr::result r = base->get_if()->get_source().get_result();
	if (r == threadmgr::result::success) {
		uint8_t _gr = *(reinterpret_cast<uint8_t*>(base->get_if()->get_source().get_message().data()));
		_COM_SVR_PRINT ("success -> group %d\n", _gr);
	}

//	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
//	base->get_external_if()->set_request_option (opt);
}

static void stop (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: stop {group_id})\n");
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

	CViewingManagerIf _if (base->get_external_if());
	_if.request_stop_viewing (group_id);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void dump (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	base->get_external_if()->request_async(
		static_cast<uint8_t>(modules::module_id::viewing_manager),
		static_cast<uint8_t>(CViewingManagerIf::sequence::dump_viewing)
	);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}



command_info_t g_viewing_manager_commands [] = { // extern
	{
		"ch",
		"start viewing by physical channel (usage: ch {physical_channel, service_idx})",
		start_by_phy_ch,
		NULL,
	},
	{
		"svc",
		"start viewing by service id  (usage: svc {tsid} {org_nid} {svcid})",
		start_by_svc_id,
		NULL,
	},
	{
		"stop",
		"force stop viewing (usage: stop {group_id})",
		stop,
		NULL,
	},
	{
		"d",
		"dump ",
		dump,
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

