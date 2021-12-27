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
	if (argc != 2) {
		_COM_SVR_PRINT ("invalid arguments. (usage: t {frequesncy[kHz]} {tuner id})\n");
		return;
	}

	{
		std::regex regex("[0-9]+");
		if (!std::regex_match (argv[0], regex)) {
			_COM_SVR_PRINT ("invalid arguments. (usage: t {frequesncy[kHz]} {tuner id})\n");
			return;
		}
	}
	{
		std::regex regex("[0-9]+");
		if (!std::regex_match (argv[1], regex)) {
			_COM_SVR_PRINT ("invalid arguments. (usage: t {frequesncy[kHz]} {tuner id})\n");
		return;
		}
	}

	uint32_t freq = atoi (argv[0]);
	uint8_t id = atoi (argv[1]);
	_COM_SVR_PRINT ("freq=[%d]kHz id=[%d]\n", freq, id);

	CTunerControlIf ctl(pBase->getExternalIf(), id);
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
	if (argc != 2) {
		_COM_SVR_PRINT ("invalid arguments. (usage: ch {physical channel} {tuner id})\n");
		return;
	}

	{
		std::regex regex("[0-9]+");
		if (!std::regex_match (argv[0], regex)) {
			_COM_SVR_PRINT ("invalid arguments. (usage: ch {physical channel} {tuner id})\n");
			return;
		}
	}
	{
		std::regex regex("[0-9]+");
		if (!std::regex_match (argv[1], regex)) {
			_COM_SVR_PRINT ("invalid arguments. (usage: ch {physical channel} {tuner id})\n");
			return;
		}
	}

	uint16_t ch = atoi (argv[0]);
	if (ch < UHF_PHYSICAL_CHANNEL_MIN || ch > UHF_PHYSICAL_CHANNEL_MAX) {
		_COM_SVR_PRINT ("out of range. (physical channel is %d~%d)\n",
						UHF_PHYSICAL_CHANNEL_MIN, UHF_PHYSICAL_CHANNEL_MAX);
		return;
	} 
	uint8_t id = atoi (argv[1]);
	_COM_SVR_PRINT ("ch=[%d] id=[%d]\n", ch, id);

	uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (ch);
	_COM_SVR_PRINT ("freq=[%d]kHz\n", freq);

	CTunerControlIf ctl(pBase->getExternalIf(), id);
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
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: stop {tuner id})\n");
		return;
	}

	std::regex regex("[0-9]+");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (usage: stop {tuner id})\n");
		return;
	}

	uint8_t id = atoi (argv[0]);
	_COM_SVR_PRINT ("id=[%d]\n", id);

	CTunerControlIf ctl(pBase->getExternalIf(), id);
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

	for (uint8_t id = 0; id < CGroup::GROUP_MAX; ++ id) {
		_COM_SVR_PRINT ("tuner id=[%d]\n", id);

		CTunerControlIf ctl(pBase->getExternalIf(), id);
		ctl.reqGetStateSync ();

		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			_COM_SVR_PRINT ("  get state success\n");
		} else {
			_COM_SVR_PRINT ("  get state error\n");
			return;
		}

		EN_TUNER_STATE state = *(EN_TUNER_STATE*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		switch (state) {
		case EN_TUNER_STATE__TUNING_BEGIN:
			_COM_SVR_PRINT ("  EN_TUNER_STATE__TUNING_BEGIN\n");
			break;
		case EN_TUNER_STATE__TUNING_SUCCESS:
			_COM_SVR_PRINT ("  EN_TUNER_STATE__TUNING_SUCCESS\n");
			break;
		case EN_TUNER_STATE__TUNING_ERROR_STOP:
			_COM_SVR_PRINT ("  EN_TUNER_STATE__TUNING_ERROR_STOP\n");
			break;
		case EN_TUNER_STATE__TUNE_STOP:
			_COM_SVR_PRINT ("  EN_TUNER_STATE__TUNE_STOP\n");
			break;
		default:
			break;
		}
	}
}

ST_COMMAND_INFO g_tunerControlCommands [] = { // extern
	{
		"t",
		"tune by frequency (usage: t {frequesncy[kHz]} {tuner id})",
		tune,
		NULL,
	},
	{
		"ch",
		"tune by physical channel (usage: ch {physical channel} {tuner id})",
		ch_tune,
		NULL,
	},
	{
		"stop",
		"tune stop (usage: stop {tuner id})",
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

