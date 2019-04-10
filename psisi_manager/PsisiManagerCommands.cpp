#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PsisiManagerIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void dump_programInfos (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpCaches (0);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_serviceInfos (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpCaches (1);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eventPfInfos (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpCaches (2);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_pat (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__PAT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_pf (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_PF);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_pf_simple (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_PF_simple);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_sch (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_SCH);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_eit_sch_simple (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__EIT_H_SCH_simple);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_nit (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__NIT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_sdt (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__SDT);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_rst (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpTables (EN_PSISI_TYPE__RST);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void dump_bit (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CPsisiManagerIf mgr(pBase->getExternalIf());
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
		"eit_sch_s",
		"dump EIT (schedule) simple",
		dump_eit_sch_simple,
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

