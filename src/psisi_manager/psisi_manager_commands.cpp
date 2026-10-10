#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "psisi_manager_if.h"
#include "command_tables.h"
#include "command_server_log.h"
#include "utils.h"
#include "modules.h"


static uint8_t s_tuner_id = 0;

static void set_target_tuner_id (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: st {tuner id})\n");
		return ;
	}

	std::regex regex("[0-9]+");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (usage: st {tuner id})\n");
		return ;
	}

	s_tuner_id = atoi (argv[0]);
	if (s_tuner_id >= arec::group::GROUP_MAX) {
		_COM_SVR_PRINT ("invalid id:[%d]  -> force tuner id=0\n", s_tuner_id);
		s_tuner_id = 0;
	}
	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");
}

static void dump_programInfos (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_caches (0);

}

static void dump_serviceInfos (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_caches (1);

}

static void dump_eventPfInfos (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_caches (2);

}

static void dump_networkInfo (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_caches (3);

}

static void dump_pat (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::PAT);

}

static void dump_pmt (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::PMT);

}

static void dump_eit_pf (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::EIT_H_PF);

}

static void dump_eit_pf_simple (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::EIT_H_PF_simple);

}

static void dump_eit_sch (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::EIT_H_SCHED);

}

static void dump_eit_sched_event (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::EIT_H_SCHED_event);

}

static void dump_eit_sched_simple (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::EIT_H_SCHED_simple);

}

static void dump_nit (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::NIT);

}

static void dump_sdt (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::SDT);

}

static void dump_cat (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::CAT);

}

static void dump_cdt (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::CDT);

}

static void dump_rst (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::RST);

}

static void dump_bit (int argc, char* argv[], coseq::module_base *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");


	arec::psisi::client_iface _if(modules::get_manager(), s_tuner_id);
	_if.request_dump_tables (arec::psisi::client_iface::psisi_type::BIT);

}


command_info_t g_psisi_manager_dump_tables [] = { // extern
	{
		"pat",
		"dump PAT",
		dump_pat,
		NULL,
	},
	{
		"pmt",
		"dump PMT",
		dump_pmt,
		NULL,
	},
	{
		"eit_pf",
		"dump EIT (p/f)",
		dump_eit_pf,
		NULL,
	},
	{
		"eit_pf_s",
		"dump EIT (p/f) simple",
		dump_eit_pf_simple,
		NULL,
	},
	{
		"eit_sch",
		"dump EIT (schedule)",
		dump_eit_sch,
		NULL,
	},
	{
		"eit_sched_e",
		"dump EIT (schedule) event",
		dump_eit_sched_event,
		NULL,
	},
	{
		"eit_sched_s",
		"dump EIT (schedule) simple",
		dump_eit_sched_simple,
		NULL,
	},
	{
		"nit",
		"dump NIT",
		dump_nit,
		NULL,
	},
	{
		"sdt",
		"dump SDT",
		dump_sdt,
		NULL,
	},
	{
		"cat",
		"dump CAT",
		dump_cat,
		NULL,
	},
	{
		"cdt",
		"dump CDT",
		dump_cdt,
		NULL,
	},
	{
		"rst",
		"dump RST",
		dump_rst,
		NULL,
	},
	{
		"bit",
		"dump BIT",
		dump_bit,
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

command_info_t g_psisi_manager_commands [] = { // extern
	{
		"st",
		"set target tuner id (usage: st {tuner id})",
		set_target_tuner_id,
		NULL,
	},
	{
		"dp",
		"dump program infos",
		dump_programInfos,
		NULL,
	},
	{
		"ds",
		"dump service infos",
		dump_serviceInfos,
		NULL,
	},
	{
		"de",
		"dump event p/f infos",
		dump_eventPfInfos,
		NULL,
	},
	{
		"dn",
		"dump network info",
		dump_networkInfo,
		NULL,
	},
	{
		"t",
		"dump psisi-tables",
		NULL,
		g_psisi_manager_dump_tables,
	},
	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};

