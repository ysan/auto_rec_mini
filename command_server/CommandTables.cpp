#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "CommandTables.h"

#include "TunerControlCommands.h"



static void __echo (int argc, char* argv[], CThreadMgrBase *pBase)
{
	_UTL_LOG_I ("%s echo.\n", __func__);
}

static void moni (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_E ("invalid arg.");
	}

	kill (0, SIGQUIT);
}

static ST_COMMAND_INFO g_commandServer2 [] = {
	{
		"echo",
		"echo from commandServer2",
		__echo,
		NULL,
	},
	{
		"moni",
		"debug monitor2",
		moni,
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

static ST_COMMAND_INFO g_commandServer [] = {
	{
		"echo",
		"echo from commandServer",
		__echo,
		NULL,
	},
	{
		"moni",
		"debug monitor",
		moni,
		NULL,
	},
	{
		"cs2",
		"command server2",
		NULL,
		g_commandServer2,
	},
	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};


//////////////////////////////////////
ST_COMMAND_INFO g_rootCommandTable [] = { // extern
	{
		"cs",
		"command server",
		NULL,
		g_commandServer,
	},
	{
		"tc",
		"tuner control",
		NULL,
		g_tunerControlCommands,
	},


	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};
//////////////////////////////////////
