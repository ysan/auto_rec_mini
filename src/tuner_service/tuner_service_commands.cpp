#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "tuner_service_if.h"
#include "command_tables.h"
#include "command_server_log.h"
#include "command_server.h"
#include "utils.h"
#include "modules.h"


static void _open (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	arec::tuner_service::client_iface _if(modules::get_manager());
	arec::client_reply rep = _if.request_open_sync ();

	if (rep.get_result() == coseq::result::success) {
		uint8_t tuner_id = *(uint8_t*)(rep.get_message().data());
		_COM_SVR_PRINT ("open: tuner_id=[%d]\n", tuner_id);
	} else {
		_COM_SVR_PRINT ("open: failure.\n");
	}
}

static void _close (int argc, char* argv[], coseq::module_base *base)
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

	arec::tuner_service::client_iface _if(modules::get_manager());
	arec::client_reply rep = _if.request_close_sync (tuner_id);

	if (rep.get_result() == coseq::result::success) {
		_COM_SVR_PRINT ("close: success.\n");
	} else {
		_COM_SVR_PRINT ("close: error.\n");
	}
}

static void _tune (int argc, char* argv[], coseq::module_base *base)
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
	arec::tuner_service::client_iface::tune_param_t param = {ch, id};

	// 返信不要(fire-and-forget)
	arec::tuner_service::client_iface _if(modules::get_manager());
	_if.request_tune_with_retry (&param);
}

static void _tune_stop (int argc, char* argv[], coseq::module_base *base)
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

	arec::tuner_service::client_iface _if(modules::get_manager());
	arec::client_reply rep = _if.request_tune_stop_sync (tuner_id);

	if (rep.get_result() == coseq::result::success) {
		_COM_SVR_PRINT ("tune stop success\n");
	} else {
		_COM_SVR_PRINT ("tune stop error\n");
	}
}

static void _dump (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	// 返信不要(fire-and-forget)
	arec::tuner_service::client_iface _if(modules::get_manager());
	_if.request_dump_allocates ();
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

