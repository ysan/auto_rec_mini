#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <regex>

#include "CommandTables.h"
#include "CommandServer.h"

#include "TunerControlCommands.h"
#include "PsisiManagerCommands.h"
#include "RecManagerCommands.h"
#include "ChannelManagerCommands.h"


const char *g_szLogLevels [] = {
	"LOG_LEVEL_D",
	"LOG_LEVEL_I",
	"LOG_LEVEL_W",
	"LOG_LEVEL_E",
	"LOG_LEVEL_PE",
};

static void _echo (int argc, char* argv[], CThreadMgrBase *pBase)
{
	_UTL_LOG_I ("%s\n", __func__);
}

static void _close (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.");
	}

	// このcastはどうなの
	CCommandServer *p = (CCommandServer*)pBase;
	p->connectionClose ();
}

static void _threadmgr_status_dump (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.");
	}

	kill (0, SIGQUIT);
}

static void _log_level (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments. (usage: lv {0|1|2|3|4} )");
		return;
	}

	std::regex regex("[0-4]");
	if (!std::regex_match (argv[0], regex)) {
		_UTL_LOG_E ("invalid arguments. (usage: lv {0|1|2|3|4} )");
		return;
	}

	EN_LOG_LEVEL lvl = (EN_LOG_LEVEL) atoi (argv[0]);
	CUtils::setLogLevel (lvl);
	_UTL_LOG_I ("set %s", g_szLogLevels [lvl]);
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
		"tuner control commands",
		NULL,
		g_tunerControlCommands,
	},
	{
		"pm",
		"psisi manager commands",
		NULL,
		g_psisiManagerCommands,
	},
	{
		"rec",
		"rec manager commands",
		NULL,
		g_recManagerCommands,
	},
	{
		"cm",
		"channel manager commands",
		NULL,
		g_chManagerCommands,
	},

	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};
