#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <regex>

#include "CommandTables.h"
#include "CommandServer.h"
#include "CommandServerLog.h"

#include "TunerControlCommands.h"
#include "PsisiManagerCommands.h"
#include "RecManagerCommands.h"
#include "ChannelManagerCommands.h"
#include "EventScheduleManagerCommands.h"
#include "EventSearchCommands.h"


const char *g_szLogLevels [] = {
	"LOG_LEVEL_D",
	"LOG_LEVEL_I",
	"LOG_LEVEL_W",
	"LOG_LEVEL_E",
	"LOG_LEVEL_PE",
};

static void _echo (int argc, char* argv[], CThreadMgrBase *pBase)
{
	_COM_SVR_PRINT ("%s\n", __func__);
}

static void _close (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

//	CCommandServer *p = (CCommandServer*)pBase;
	CCommandServer* p = dynamic_cast <CCommandServer*> (pBase);
	p->connectionClose ();
}

static void _threadmgr_status_dump (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	kill (0, SIGQUIT);
}

static void _log_level (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: lv {0|1|2|3|4} )\n");
		return;
	}

	std::regex regex("[0-4]");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (usage: lv {0|1|2|3|4} )\n");
		return;
	}

	EN_LOG_LEVEL lvl = (EN_LOG_LEVEL) atoi (argv[0]);
	CUtils::setLogLevel (lvl);
	_COM_SVR_PRINT ("set %s\n", g_szLogLevels [lvl]);
}


static ST_COMMAND_INFO g_systemCommands [] = {
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
		"log level (usage: lv {0|1|2|3|4} )",
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
ST_COMMAND_INFO g_rootCommandTable [] = { // extern
	{
		"s",
		"system commands",
		NULL,
		g_systemCommands,
	},
	{
		"tc",
		"tuner control",
		NULL,
		g_tunerControlCommands,
	},
	{
		"pm",
		"psisi manager",
		NULL,
		g_psisiManagerCommands,
	},
	{
		"rec",
		"rec manager",
		NULL,
		g_recManagerCommands,
	},
	{
		"cm",
		"channel manager",
		NULL,
		g_chManagerCommands,
	},
	{
		"em",
		"event schedule manager",
		NULL,
		g_eventScheduleManagerCommands,
	},
	{
		"es",
		"event search",
		NULL,
		g_eventSearchCommands,
	},


	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};
