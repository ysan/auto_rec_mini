#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PsisiManagerIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Utils.h"


static uint8_t s_tuner_id = 0;

static void set_target_tuner_id (int argc, char* argv[], CThreadMgrBase *pBase)
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
	if (s_tuner_id >= CGroup::GROUP_MAX) {
		_COM_SVR_PRINT ("invalid id:[%d]  -> force tuner id=0\n", s_tuner_id);
		s_tuner_id = 0;
	}
	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");
}

static void dump_programInfos (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpCaches (0);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_serviceInfos (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpCaches (1);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eventPfInfos (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpCaches (2);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_networkInfo (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpCaches (3);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_pat (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__PAT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_pmt (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__PMT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_pf (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_PF);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_pf_simple (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_PF_simple);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_sch (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_SCHED);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_sched_event (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_SCHED_event);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_sched_simple (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_SCHED_simple);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_nit (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__NIT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_sdt (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__SDT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_cat (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__CAT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_cdt (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__CDT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_rst (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__RST);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_bit (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	_COM_SVR_PRINT ("--------------------\n");
	_COM_SVR_PRINT ("target tuner id=[%d]\n", s_tuner_id);
	_COM_SVR_PRINT ("--------------------\n");

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf(), s_tuner_id);
	mgr.reqDumpTables (EN_PSISI_TYPE__BIT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_psisiManagerDumpTables [] = { // extern
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

ST_COMMAND_INFO g_psisiManagerCommands [] = { // extern
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
		g_psisiManagerDumpTables,
	},
	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};

