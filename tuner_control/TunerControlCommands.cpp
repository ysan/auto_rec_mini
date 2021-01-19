#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "TunerControlIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Utils.h"
#include "TsAribCommon.h"


static void tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: t {frequesncy[kHz]} )\n");
		return;
	}

	std::regex regex("[0-9]+");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (usage: t {frequesncy[kHz]} )\n");
		return;
	}

	uint32_t freq = atoi (argv[0]);
	_COM_SVR_PRINT ("freq=[%d]kHz\n", freq);

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqTuneSync (freq);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("tune success\n");
	} else {
		_COM_SVR_PRINT ("tune error\n");
	}
}

static void ch_tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: ch {physical channel} )\n");
		return;
	}

	std::regex regex("[0-9]+");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (usage: ch {physical channel} )\n");
		return;
	}

	uint16_t ch = atoi (argv[0]);
	if (ch < UHF_PHYSICAL_CHANNEL_MIN || ch > UHF_PHYSICAL_CHANNEL_MAX) {
		_COM_SVR_PRINT ("out of range. (physical channel is %d~%d)\n",
						UHF_PHYSICAL_CHANNEL_MIN, UHF_PHYSICAL_CHANNEL_MAX);
		return;
	} 

	_COM_SVR_PRINT ("ch=[%d]\n", ch);

	uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (ch);
	_COM_SVR_PRINT ("freq=[%d]kHz\n", freq);

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqTuneSync (freq);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("tune success\n");
	} else {
		_COM_SVR_PRINT ("tune error\n");
	}
}

static void tuneStop (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqTuneStopSync ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("tune stop success\n");
	} else {
		_COM_SVR_PRINT ("tune stop error\n");
	}
}

static void getState (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqGetStateSync ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("get state success\n");
	} else {
		_COM_SVR_PRINT ("get state error\n");
	}

	EN_TUNER_STATE state = *(EN_TUNER_STATE*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
	switch (state) {
	case EN_TUNER_STATE__TUNING_BEGIN:
		_COM_SVR_PRINT ("EN_TUNER_STATE__TUNING_BEGIN\n");
		break;
	case EN_TUNER_STATE__TUNING_SUCCESS:
		_COM_SVR_PRINT ("EN_TUNER_STATE__TUNING_SUCCESS\n");
		break;
	case EN_TUNER_STATE__TUNING_ERROR_STOP:
		_COM_SVR_PRINT ("EN_TUNER_STATE__TUNING_ERROR_STOP\n");
		break;
	case EN_TUNER_STATE__TUNE_STOP:
		_COM_SVR_PRINT ("EN_TUNER_STATE__TUNE_STOP\n");
		break;
	default:
		break;
	}
}

ST_COMMAND_INFO g_tunerControlCommands [] = { // extern
	{
		"t",
		"tune by frequency (usage: t {frequesncy[kHz]} )",
		tune,
		NULL,
	},
	{
		"ch",
		"tune by physical channel (usage: ct {physical channel} )",
		ch_tune,
		NULL,
	},
	{
		"stop",
		"tune stop",
		tuneStop,
		NULL,
	},
	{
		"s",
		"get tuner state",
		getState,
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

