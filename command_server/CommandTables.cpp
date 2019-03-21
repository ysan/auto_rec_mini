#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "CommandTables.h"

#include "TunerControlCommands.h"
#include "PsisiManagerCommands.h"



static void _echo (int argc, char* argv[], CThreadMgrBase *pBase)
{
	_UTL_LOG_I ("%s\n", __func__);
}

static void _moni (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.");
	}

	kill (0, SIGQUIT);
}

static void _log_level (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments.");
		return;
	}

	EN_LOG_LEVEL lvl = (EN_LOG_LEVEL) atoi (argv[0]);
	CUtils::setLogLevel (lvl);
}


static ST_COMMAND_INFO g_systemCommands [] = {
	{
		"echo",
		"echo from commandServer",
		_echo,
		NULL,
	},
	{
		"moni",
		"threadmgr debug monitor",
		_moni,
		NULL,
	},
	{
		"sl",
		"set log level (usage: sl <0|1|2|3|4>)",
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



	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};
