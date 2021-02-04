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

enum {
	EN_SEQ_TUNER_SERVICE__MODULE_UP = 0,
	EN_SEQ_TUNER_SERVICE__MODULE_DOWN,
	EN_SEQ_TUNER_SERVICE__OPEN,
	EN_SEQ_TUNER_SERVICE__CLOSE,

	EN_SEQ_TUNER_SERVICE__NUM,
};


class CTunerServiceIf : public CThreadMgrExternalIf
{
public:
	enum class client : int {
		OTHER = 0,
		LIVE_VIEW,
		CACHE_EVENT_SCHEDULE,
		RECORDING,
		CHANNEL_SCAN,
	};

public:
	explicit CTunerServiceIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CTunerServiceIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, EN_SEQ_TUNER_SERVICE__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, EN_SEQ_TUNER_SERVICE__MODULE_DOWN);
	};

	bool reqOpen (client _client) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, EN_SEQ_TUNER_SERVICE__OPEN, (uint8_t*)&_client, sizeof(_client));
	};

	bool reqClose (uint8_t tuner_id) {
		return requestAsync (EN_MODULE_TUNER_SERVICE, EN_SEQ_TUNER_SERVICE__CLOSE, (uint8_t*)&tuner_id, sizeof(tuner_id));
	};
};

#endif
