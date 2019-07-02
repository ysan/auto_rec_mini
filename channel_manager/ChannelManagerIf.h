#ifndef _CHANNEL_MANAGER_IF_H_
#define _CHANNEL_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_CHANNEL_MANAGER__MODULE_UP = 0,
	EN_SEQ_CHANNEL_MANAGER__MODULE_DOWN,
	EN_SEQ_CHANNEL_MANAGER__CHANNEL_SCAN,
	EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_SERVICE_ID,
	EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_REMOTE_CONTROL_KEY_ID,
	EN_SEQ_CHANNEL_MANAGER__TUNE_BY_SERVICE_ID,
	EN_SEQ_CHANNEL_MANAGER__TUNE_BY_REMOTE_CONTROL_KEY_ID,
	EN_SEQ_CHANNEL_MANAGER__DUMP_SCAN_RESULTS,

	EN_SEQ_CHANNEL_MANAGER__NUM,
};

typedef struct {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
} _SERVICE_ID_PARAM;

typedef struct {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint8_t remote_control_key_id;
} _REMOTE_CONTROL_ID_PARAM;


class CChannelManagerIf : public CThreadMgrExternalIf
{
public:
	explicit CChannelManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CChannelManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__MODULE_DOWN);
	};

	bool reqChannelScan (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__CHANNEL_SCAN);
	};

	bool reqGetPysicalChannelByServiceId (_SERVICE_ID_PARAM *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_SERVICE_ID,
					(uint8_t*)p_param,
					sizeof (_SERVICE_ID_PARAM)
				);
	};

	bool reqGetPysicalChannelByRemoteControlKeyId (_REMOTE_CONTROL_ID_PARAM *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_REMOTE_CONTROL_KEY_ID,
					(uint8_t*)p_param,
					sizeof (_REMOTE_CONTROL_ID_PARAM)
				);
	};

	bool reqTuneByServiceId (_SERVICE_ID_PARAM *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__TUNE_BY_SERVICE_ID,
					(uint8_t*)p_param,
					sizeof (_SERVICE_ID_PARAM)
				);
	};

	bool reqTuneByRemoteControlKeyId (_REMOTE_CONTROL_ID_PARAM *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__TUNE_BY_REMOTE_CONTROL_KEY_ID,
					(uint8_t*)p_param,
					sizeof (_REMOTE_CONTROL_ID_PARAM)
				);
	};

	bool reqDumpScanResults (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__DUMP_SCAN_RESULTS);
	};

};

#endif
