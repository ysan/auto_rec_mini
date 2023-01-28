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


static void tune (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
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

	CTunerControlIf ctl(base->get_external_if(), id);
	ctl.request_tune_sync (freq);

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		_COM_SVR_PRINT ("tune success\n");
	} else {
		_COM_SVR_PRINT ("tune error\n");
	}
}

static void ch_tune (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
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

	CTunerControlIf ctl(base->get_external_if(), id);
	ctl.request_tune_sync (freq);

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		_COM_SVR_PRINT ("tune success\n");
	} else {
		_COM_SVR_PRINT ("tune error\n");
	}
}

static void tune_stop (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
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

	CTunerControlIf ctl(base->get_external_if(), id);
	ctl.request_tune_stop_sync ();

	threadmgr::result rslt = base->get_if()->get_source().get_result();
	if (rslt == threadmgr::result::success) {
		_COM_SVR_PRINT ("tune stop success\n");
	} else {
		_COM_SVR_PRINT ("tune stop error\n");
	}
}

static void get_state (int argc, char* argv[], threadmgr::CThreadMgrBase *base)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	for (uint8_t id = 0; id < CGroup::GROUP_MAX; ++ id) {
		_COM_SVR_PRINT ("tuner id=[%d]\n", id);

		CTunerControlIf ctl(base->get_external_if(), id);
		ctl.request_get_state_sync ();

		threadmgr::result rslt = base->get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			_COM_SVR_PRINT ("  get state success\n");
		} else {
			_COM_SVR_PRINT ("  get state error\n");
			return;
		}

		CTunerControlIf::tuner_state state = *(CTunerControlIf::tuner_state*)(base->get_if()->get_source().get_message().data());
		switch (state) {
		case CTunerControlIf::tuner_state::tuning_begin:
			_COM_SVR_PRINT ("  CTunerControlIf::tuner_state::tuning_begin\n");
			break;
		case CTunerControlIf::tuner_state::tuning_success:
			_COM_SVR_PRINT ("  CTunerControlIf::tuner_state::tuning_success\n");
			break;
		case CTunerControlIf::tuner_state::tuning_error_stop:
			_COM_SVR_PRINT ("  CTunerControlIf::tuner_state::tuning_error_stop\n");
			break;
		case CTunerControlIf::tuner_state::tune_stop:
			_COM_SVR_PRINT ("  CTunerControlIf::tuner_state::tune_stop\n");
			break;
		default:
			break;
		}
	}
}

command_info_t g_tuner_control_commands [] = { // extern
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
		tune_stop,
		NULL,
	},
	{
		"s",
		"get tuner state",
		get_state,
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

