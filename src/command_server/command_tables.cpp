#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <regex>

#include "command_tables.h"
#include "command_server.h"
#include "command_server_log.h"

#include "tuner_control_commands.h"
#include "psisi_manager_commands.h"
#include "tuner_service_commands.h"
#include "rec_manager_commands.h"
#include "channel_manager_commands.h"
#include "event_schedule_manager_commands.h"
#include "event_search_commands.h"
#include "viewing_manager_commands.h"


const char *g_sz_log_levels [] = {
	"debug",
	"info",
	"warning",
	"error",
	"perror",
};

static void _echo (int argc, char* argv[], coseq::module_base *pBase)
{
	_COM_SVR_PRINT ("%s\n", __func__);
}

static void _close (int argc, char* argv[], coseq::module_base *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

//	arec::command_server::manager *p = (arec::command_server::manager*)pBase;
	arec::command_server::manager* p = dynamic_cast <arec::command_server::manager*> (pBase);
	p->connection_close();
}

static void _threadmgr_status_dump (int argc, char* argv[], coseq::module_base *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("status dump is not supported on coseq.\n");
}

static void _log_level (int argc, char* argv[], coseq::module_base *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: lv {0|1|2|3|4})\n");
		return;
	}

	std::regex regex("[0-4]");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (usage: lv {0|1|2|3|4})\n");
		return;
	}

	arec::common::logger::level lvl = (arec::common::logger::level) atoi (argv[0]);
	arec::common::utils::get_logger()->set_log_level (lvl);
	_COM_SVR_PRINT ("set %s\n", g_sz_log_levels [static_cast<int>(lvl)]);
}


static command_info_t g_system_commands [] = {
	{
		"e",
		"echo from commandServer",
		_echo,
		NULL,
	},
	{
		"cl",
		"close connection",
		_close,
		NULL,
	},
	{
		"ts",
		"threadmgr status dump",
		_threadmgr_status_dump,
		NULL,
	},
	{
		"lv",
		"log level (usage: lv {0|1|2|3|4})",
		_log_level,
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


///////////  root command table  ///////////
command_info_t g_root_command_table [] = { // extern
	{
		"s",
		"system commands",
		NULL,
		g_system_commands,
	},
	{
		"tc",
		"tuner control",
		NULL,
		g_tuner_control_commands,
	},
	{
		"pm",
		"psisi manager",
		NULL,
		g_psisi_manager_commands,
	},
	{
		"ts",
		"tuner service",
		NULL,
		g_tuner_service_commands,
	},
	{
		"rec",
		"rec manager",
		NULL,
		g_rec_manager_commands,
	},
	{
		"cm",
		"channel manager",
		NULL,
		g_channel_manager_commands,
	},
	{
		"em",
		"event schedule manager",
		NULL,
		g_event_schedule_manager_commands,
	},
	{
		"es",
		"event search",
		NULL,
		g_event_search_commands,
	},
	{
		"vm",
		"viewing manager",
		NULL,
		g_viewing_manager_commands,
	},


	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};
