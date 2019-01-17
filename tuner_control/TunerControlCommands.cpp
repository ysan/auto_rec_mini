#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TunerControlIf.h"
#include "CommandTables.h"
#include "Utils.h"


static void tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid argument\n");
		return;
	}

	uint32_t freq = atoi (argv[0]);
	_UTL_LOG_I ("freq=[%d]\n", freq);

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqTuneSync (freq);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_UTL_LOG_I ("tune success");
	} else {
		_UTL_LOG_I ("tune error");
	}
}

static void tuneStop (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_E ("invalid argument\n");
		return;
	}

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqTuneStopSync ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_UTL_LOG_I ("tune stop success");
	} else {
		_UTL_LOG_I ("tune stop error");
	}
}

ST_COMMAND_INFO g_tunerControlCommands [] = { // extern
	{
		"tune",
		"tune by freq",
		tune,
		NULL,
	},
	{
		"stop",
		"tune stop",
		tuneStop,
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

