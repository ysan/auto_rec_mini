#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "event_search_if.h"
#include "command_tables.h"
#include "command_server_log.h"
#include "utils.h"
#include "modules.h"


static void _add_rec_reserve_keyword_search (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}


	arec::event_search::client_iface _if(modules::get_manager());
	_if.request_add_rec_reserve_keyword_search ();

}

static void _add_rec_reserve_keyword_search_ex (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}


	arec::event_search::client_iface _if(modules::get_manager());
	_if.request_add_rec_reserve_keyword_search_ex ();

}

static void _dump_histories (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}


	arec::event_search::client_iface _if(modules::get_manager());
	_if.request_dump_histories ();

}

static void _dump_histories_ex (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}


	arec::event_search::client_iface _if(modules::get_manager());
	_if.request_dump_histories_ex ();

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

