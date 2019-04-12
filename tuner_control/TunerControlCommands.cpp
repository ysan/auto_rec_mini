#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "TunerControlIf.h"
#include "CommandTables.h"
#include "Utils.h"
#include "TsAribCommon.h"
#include "it9175_extern.h"


static void tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments. (usage: t {frequesncy[kHz]} )");
		return;
	}

	std::regex regex("[0-9]+");
	if (!std::regex_match (argv[0], regex)) {
		_UTL_LOG_E ("invalid arguments. (usage: t {frequesncy[kHz]} )");
		return;
	}

	uint32_t freq = atoi (argv[0]);
	_UTL_LOG_I ("freq=[%d]kHz\n", freq);

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqTuneSync (freq);

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_UTL_LOG_I ("tune success");
	} else {
		_UTL_LOG_I ("tune error");
	}
}

static void ch_tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_UTL_LOG_E ("invalid arguments. (usage: ch {physical channel} )");
		return;
	}

	std::regex regex("[0-9]+");
	if (!std::regex_match (argv[0], regex)) {
		_UTL_LOG_E ("invalid arguments. (usage: ch {physical channel} )");
		return;
	}

	uint32_t ch = atoi (argv[0]);
	if (ch < UHF_PHYSICAL_CHANNEL_MIN || ch > UHF_PHYSICAL_CHANNEL_MAX) {
		_UTL_LOG_E ("out of range. (physical channel is %d~%d)",
						UHF_PHYSICAL_CHANNEL_MIN, UHF_PHYSICAL_CHANNEL_MAX);
		return;
	} 

	_UTL_LOG_I ("ch=[%d]\n", ch);

	uint32_t freq = CTsAribCommon::ch2freqKHz (ch);
	_UTL_LOG_I ("freq=[%d]kHz\n", freq);

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
		_UTL_LOG_W ("ignore arguments.\n");
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

static void getState (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	CTunerControlIf ctl(pBase->getExternalIf());
	ctl.reqGetStateSync ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_UTL_LOG_I ("get state success");
	} else {
		_UTL_LOG_I ("get state error");
	}

	EN_TUNER_STATE state = *(EN_TUNER_STATE*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
	switch (state) {
	case EN_TUNER_STATE__TUNING_BEGIN:
		_UTL_LOG_I ("EN_TUNER_STATE__TUNING_BEGIN");
		break;
	case EN_TUNER_STATE__TUNING_SUCCESS:
		_UTL_LOG_I ("EN_TUNER_STATE__TUNING_SUCCESS");
		break;
	case EN_TUNER_STATE__TUNING_ERROR_STOP:
		_UTL_LOG_I ("EN_TUNER_STATE__TUNING_ERROR_STOP");
		break;
	case EN_TUNER_STATE__TUNE_STOP:
		_UTL_LOG_I ("EN_TUNER_STATE__TUNE_STOP");
		break;
	default:
		break;
	}
}

static void getState_it9175 (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_UTL_LOG_W ("ignore arguments.\n");
	}

	EN_IT9175_STATE state = it9175_get_state();
	switch (state) {
	case EN_IT9175_STATE__CLOSED:
		_UTL_LOG_I ("EN_IT9175_STATE__CLOSED");
		break;
	case EN_IT9175_STATE__OPENED:
		_UTL_LOG_I ("EN_IT9175_STATE__OPENED");
		break;
	case EN_IT9175_STATE__TUNE_BEGINED:
		_UTL_LOG_I ("EN_IT9175_STATE__TUNE_BEGINED");
		break;
	case EN_IT9175_STATE__TUNED:
		_UTL_LOG_I ("EN_IT9175_STATE__TUNED");
		break;
	case EN_IT9175_STATE__TUNE_ENDED:
		_UTL_LOG_I ("EN_IT9175_STATE__TUNE_ENDED");
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
	{
		"ss",
		"get tuner state (it9175 state)",
		getState_it9175,
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

