#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "EventSearchIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Utils.h"


static void _add_rec_reserve_keyword_search (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventSearchIf _if(base->get_external_if());
	_if.request_add_rec_reserve_keyword_search ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _add_rec_reserve_keyword_search_ex (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventSearchIf _if(base->get_external_if());
	_if.request_add_rec_reserve_keyword_search_ex ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _dump_histories (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventSearchIf _if(base->get_external_if());
	_if.request_dump_histories ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}

static void _dump_histories_ex (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = base->get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);

	CEventSearchIf _if(base->get_external_if());
	_if.request_dump_histories_ex ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	base->get_external_if()->set_request_option (opt);
}


command_info_t g_event_search_commands [] = { // extern
	{
		"r",
		"add rec reserve (event name keyword search)",
		_add_rec_reserve_keyword_search,
		NULL,
	},
	{
		"rx",
		"add rec reserve (extended event keyword search)",
		_add_rec_reserve_keyword_search_ex,
		NULL,
	},
	{
		"h",
		"dump histories (event name keyword search)",
		_dump_histories,
		NULL,
	},
	{
		"hx",
		"dump histories (extended event keyword search)",
		_dump_histories_ex,
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

