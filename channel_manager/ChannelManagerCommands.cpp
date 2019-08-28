#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "ChannelManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void _scan (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqChannelScan ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments.\n");
		return;
	}

	std::regex regex_idx ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_idx)) {
		_UTL_LOG_E ("invalid arguments. (id)");
		return;
	}

//TODO
// 暫定remote_control_key_idだけ渡す
	uint8_t id = atoi(argv[0]);
	CChannelManagerIf::REMOTE_CONTROL_ID_PARAM_t param = {0, 0, id};

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqTuneByRemoteControlKeyId (&param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _dump_channels (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpChannels ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_chManagerCommands [] = { // extern
	{
		"scan",
		"channel scan",
		_scan,
		NULL,
	},
	{
		"tr",
		"tune by remote_control_key_id (usage: tr {remote_control_key_id} )",
		_tune,
		NULL,
	},
	{
		"d",
		"dump channels",
		_dump_channels,
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

