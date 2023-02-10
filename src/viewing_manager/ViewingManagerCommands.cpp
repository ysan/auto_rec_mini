#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "ViewingManagerIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Utils.h"


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

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CViewingManagerIf _if (base->get_external_if());
	_if.request_start_viewing_by_physical_channel (&param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
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

	CViewingManagerIf _if (base->get_external_if());
	_if.request_dump_viewing ();

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

