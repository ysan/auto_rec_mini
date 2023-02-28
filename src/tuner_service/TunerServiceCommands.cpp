#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "TunerServiceIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "CommandServer.h"
#include "Utils.h"


static void _open (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CTunerServiceIf _if(base->get_external_if());
	_if.request_open_sync ();

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		uint8_t tuner_id = *(uint8_t*)(base->get_if()->get_source().get_message().data());
		_COM_SVR_PRINT ("open: tuner_id=[%d]\n", tuner_id);
	} else {
		_COM_SVR_PRINT ("open: failure.\n");
	}
}

static void _close (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: cl {tuner_id})\n");
		return;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
			_COM_SVR_PRINT ("invalid arguments. (tuner_id)\n");
			return;
	}
	uint8_t tuner_id = atoi(argv[0]);

	CTunerServiceIf _if(base->get_external_if());
	_if.request_close_sync (tuner_id);

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		_COM_SVR_PRINT ("close: success.\n");
	} else {
		_COM_SVR_PRINT ("close: error.\n");
	}
}

static void _tune (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 2) {
		_COM_SVR_PRINT ("invalid arguments. (usage: t {physical channel} {tuner id})\n");
		return;
	}

	std::regex regex_ch ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_ch)) {
		_COM_SVR_PRINT ("invalid arguments. (physical channel)\n");
		return;
	}

	std::regex regex_id ("^[0-9]+$");
	if (!std::regex_match (argv[1], regex_id)) {
		_COM_SVR_PRINT ("invalid arguments. (tuner id)\n");
		return;
	}

	uint16_t ch = atoi(argv[0]);
	uint8_t id = atoi(argv[1]);
	CTunerServiceIf::tune_param_t param = {ch, id};

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CTunerServiceIf _if(base->get_external_if());
	_if.request_tune_with_retry (&param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _tune_stop (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: stop {tuner_id})\n");
		return;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (tuner_id)\n");
		return;
	}
	uint8_t tuner_id = atoi(argv[0]);

	CTunerServiceIf _if(base->get_external_if());
	_if.request_tune_stop_sync (tuner_id);

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		_COM_SVR_PRINT ("tune stop success\n");
	} else {
		_COM_SVR_PRINT ("tune stop error\n");
	}
}

static void _dump (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CTunerServiceIf _if(base->get_external_if());
	_if.request_dump_allocates ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

command_info_t g_tuner_service_commands [] = { // extern
	{
		"op",
		"open tuner resource",
		_open,
		NULL,
	},
	{
		"cl",
		"close tuner resource (usage: cl {tuner_id})",
		_close,
		NULL,
	},
	{
		"t",
		"tune by physical channel (usage: t {physical channel} {tuner id})",
		_tune,
		NULL,
	},
	{
		"stop",
		"tune stop (usage: stop {tuner_id})",
		_tune_stop,
		NULL,
	},
	{
		"d",
		"dump tuner resources",
		_dump,
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

