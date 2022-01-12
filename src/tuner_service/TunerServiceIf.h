#ifndef _TUNER_SERVICE_IF_H_
#define _TUNER_SERVICE_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"


using namespace ThreadManager;

class CTunerServiceIf : public CThreadMgrExternalIf
{
public:
	enum sequence {
		MODULE_UP = 0,
		MODULE_DOWN,
		OPEN,
		CLOSE,
		TUNE,
		TUNE_WITH_RETRY,
		TUNE_ADVANCE,
		TUNE_STOP,
		DUMP_ALLOCATES,

		NUM,
	};

	typedef struct _tune_param {
		uint16_t physical_channel;
		uint8_t tuner_id;
	} tune_param_t;

	typedef struct _tune_advance_param {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
		uint8_t tuner_id;
		bool is_need_retry;
	} tune_advance_param_t;

public:
	explicit CTunerServiceIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CTunerServiceIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, sequence::MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, sequence::MODULE_DOWN);
	};

	bool reqOpen (void) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, sequence::OPEN);
	};

	bool reqOpenSync (void) {
		return requestSync (EN_MODULE_TUNER_SERVICE, sequence::OPEN);
	};

	bool reqClose (uint8_t tuner_id) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, sequence::CLOSE, (uint8_t*)&tuner_id, sizeof(tuner_id));
	};

	bool reqCloseSync (uint8_t tuner_id) {
		return requestSync (EN_MODULE_TUNER_SERVICE, sequence::CLOSE, (uint8_t*)&tuner_id, sizeof(tuner_id));
	};

	bool reqTune (const tune_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_TUNER_SERVICE,
					sequence::TUNE,
					(uint8_t*)p_param,
					sizeof(tune_param_t)
				);
	};

	bool reqTune_withRetry (const tune_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_TUNER_SERVICE,
					sequence::TUNE_WITH_RETRY,
					(uint8_t*)p_param,
					sizeof(tune_param_t)
				);
	};

	bool reqTuneAdvance (const tune_advance_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_TUNER_SERVICE,
					sequence::TUNE_ADVANCE,
					(uint8_t*)p_param,
					sizeof(tune_advance_param_t)
				);
	};

	bool reqTuneStop (uint8_t tuner_id) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, sequence::TUNE_STOP, (uint8_t*)&tuner_id, sizeof(tuner_id));
	};

	bool reqTuneStopSync (uint8_t tuner_id) {
		return requestSync (EN_MODULE_TUNER_SERVICE, sequence::TUNE_STOP, (uint8_t*)&tuner_id, sizeof(tuner_id));
	};

	bool reqDumpAllocates (void) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, sequence::DUMP_ALLOCATES);
	};
};

#endif
