#ifndef _TUNER_CONTROL_IF_H_
#define _TUNER_CONTROL_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_TUNER_CONTROL_START = 0,
	EN_SEQ_TUNER_CONTROL_TUNE,
	EN_SEQ_TUNER_CONTROL_TUNE_START,
	EN_SEQ_TUNER_CONTROL_TUNE_STOP,
	EN_SEQ_TUNER_CONTROL_NUM,
};


class CTunerControlIf : public CThreadMgrExternalIf
{
public:
	explicit CTunerControlIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CTunerControlIf (void) {
	};


	bool reqStart (void) {
		return requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_START);
	};

	bool reqTune (uint32_t freq) {
		uint32_t f = freq;
		return requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE, (uint8_t*)&f, sizeof(f));
	};

	bool reqTuneSync (uint32_t freq) {
		uint32_t f = freq;
		return requestSync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE, (uint8_t*)&f, sizeof(f));
	};

	bool reqTuneStop (void) {
		return requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE_STOP);
	};

	bool reqTuneStopSync (void) {
		return requestSync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE_STOP);
	};
};

#endif
