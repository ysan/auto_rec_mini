#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "CommandTables.h"

#include "TunerControlCommands.h"
#include "PsisiManagerCommands.h"



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
