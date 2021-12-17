#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "TunerServiceIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "CommandServer.h"
#include "Utils.h"


static void _open (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CTunerServiceIf _if(pBase->getExternalIf());
	_if.reqOpenSync ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		uint8_t tuner_id = *(uint8_t*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("open: tuner_id=[%d]\n", tuner_id);
	} else {
		_COM_SVR_PRINT ("open: failure.\n");
	}
}

static void _close (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("ignore arguments. (usage: cl {tuner_id} )\n");
		return;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
			_COM_SVR_PRINT ("invalid arguments. (tuner_id)\n");
			return;
	}
	uint8_t tuner_id = atoi(argv[0]);

	CTunerServiceIf _if(pBase->getExternalIf());
	_if.reqCloseSync (tuner_id);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("close: success.\n");
	} else {
		_COM_SVR_PRINT ("close: error.\n");
	}
}

static void _tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 2) {
		_COM_SVR_PRINT ("invalid arguments. (usage: tune {physical channel} {tuner id} )\n");
		return;
	}

	std::regex regex_ch ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_ch)) {
		_COM_SVR_PRINT ("invalid arguments. (physical channel)\n");
		return;
	}

	std::regex regex_id ("^[0-9]+$");
	if (!std::regex_match (argv[1], regex_id)) {
		_COM_SVR_PRINT ("invalid arguments. (tuner id)\n");
		return;
	}

	uint16_t ch = atoi(argv[0]);
	uint8_t id = atoi(argv[1]);
	CTunerServiceIf::tune_param_t param = {ch, id};

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CTunerServiceIf _if(pBase->getExternalIf());
	_if.reqTune_withRetry (&param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune_stop (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("ignore arguments. (usage: stop {tuner_id} )\n");
		return;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (tuner_id)\n");
		return;
	}
	uint8_t tuner_id = atoi(argv[0]);

	CTunerServiceIf _if(pBase->getExternalIf());
	_if.reqTuneStopSync (tuner_id);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("tune stop success\n");
	} else {
		_COM_SVR_PRINT ("tune stop error\n");
	}
}

static void _dump (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CTunerServiceIf _if(pBase->getExternalIf());
	_if.reqDumpAllocates ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

ST_COMMAND_INFO g_tunerServiceCommands [] = { // extern
	{
		"op",
		"open tuner resource",
		_open,
		NULL,
	},
	{
		"cl",
		"close tuner resource (usage: cl {tuner_id} )",
		_close,
		NULL,
	},
	{
		"tune",
		"tune by physical channel (usage: tune {physical channel} {tuner id} )",
		_tune,
		NULL,
	},
	{
		"stop",
		"tune stop (usage: stop {tuner_id} )",
		_tune_stop,
		NULL,
	},
	{
		"d",
		"dump tuner resources",
		_dump,
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

