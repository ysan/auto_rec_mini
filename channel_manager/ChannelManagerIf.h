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
	EN_SEQ_CHANNEL_MANAGER__TUNE_BY_SERVICE_ID_WITH_RETRY,
	EN_SEQ_CHANNEL_MANAGER__TUNE_BY_REMOTE_CONTROL_KEY_ID,
	EN_SEQ_CHANNEL_MANAGER__GET_CHANNELS,
	EN_SEQ_CHANNEL_MANAGER__DUMP_SCAN_RESULTS,

	EN_SEQ_CHANNEL_MANAGER__NUM,
};


class CChannelManagerIf : public CThreadMgrExternalIf
{
public:
	typedef struct _service_id_param {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
	} SERVICE_ID_PARAM_t;

	typedef struct _remote_control_id_param {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint8_t remote_control_key_id;
	} REMOTE_CONTROL_ID_PARAM_t;

	typedef struct _channel {
		uint16_t pysical_channel;
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint8_t remote_control_key_id;
		uint16_t service_ids[10];
		int service_num;
	} CHANNEL_t;

	typedef struct _req_channels_param {
		CHANNEL_t *p_out_channels;
		int array_max_num;
	} REQ_CHANNELS_PARAM_t;


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

	bool reqGetPysicalChannelByServiceId (SERVICE_ID_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_SERVICE_ID,
					(uint8_t*)p_param,
					sizeof (SERVICE_ID_PARAM_t)
				);
	};

	bool reqGetPysicalChannelByRemoteControlKeyId (REMOTE_CONTROL_ID_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_REMOTE_CONTROL_KEY_ID,
					(uint8_t*)p_param,
					sizeof (REMOTE_CONTROL_ID_PARAM_t)
				);
	};

	bool reqTuneByServiceId (SERVICE_ID_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__TUNE_BY_SERVICE_ID,
					(uint8_t*)p_param,
					sizeof (SERVICE_ID_PARAM_t)
				);
	};

	bool reqTuneByServiceId_withRetry (SERVICE_ID_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__TUNE_BY_SERVICE_ID_WITH_RETRY,
					(uint8_t*)p_param,
					sizeof (SERVICE_ID_PARAM_t)
				);
	};

	bool reqTuneByRemoteControlKeyId (REMOTE_CONTROL_ID_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__TUNE_BY_REMOTE_CONTROL_KEY_ID,
					(uint8_t*)p_param,
					sizeof (REMOTE_CONTROL_ID_PARAM_t)
				);
	};

	bool reqGetChannels (REQ_CHANNELS_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_CHANNEL_MANAGER,
					EN_SEQ_CHANNEL_MANAGER__GET_CHANNELS,
					(uint8_t*)p_param,
					sizeof (REQ_CHANNELS_PARAM_t)
				);
	};

	bool reqDumpChannels (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__DUMP_SCAN_RESULTS);
	};

};

#endif
