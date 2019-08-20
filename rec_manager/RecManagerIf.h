#ifndef _REC_MANAGER_IF_H_
#define _REC_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"
#include "Utils.h"


using namespace ThreadManager;

enum {
	EN_SEQ_REC_MANAGER__MODULE_UP = 0,
	EN_SEQ_REC_MANAGER__MODULE_DOWN,
	EN_SEQ_REC_MANAGER__CHECK_LOOP,				// inner
	EN_SEQ_REC_MANAGER__CHECK_EVENT_LOOP,		// inner
	EN_SEQ_REC_MANAGER__RECORDING_NOTICE,		// inner
	EN_SEQ_REC_MANAGER__START_RECORDING,		// inner
	EN_SEQ_REC_MANAGER__ADD_RESERVE_CURRENT_EVENT,
	EN_SEQ_REC_MANAGER__ADD_RESERVE_EVENT,
	EN_SEQ_REC_MANAGER__ADD_RESERVE_EVENT_HELPER,
	EN_SEQ_REC_MANAGER__ADD_RESERVE_MANUAL,
	EN_SEQ_REC_MANAGER__REMOVE_RESERVE,
	EN_SEQ_REC_MANAGER__REMOVE_RESERVE_BY_INDEX,
	EN_SEQ_REC_MANAGER__STOP_RECORDING,
	EN_SEQ_REC_MANAGER__DUMP_RESERVES,

	EN_SEQ_REC_MANAGER__NUM,
};

typedef enum {
	EN_RESERVE_REPEATABILITY__NONE = 0,
	EN_RESERVE_REPEATABILITY__DAILY,
	EN_RESERVE_REPEATABILITY__WEEKLY,
	EN_RESERVE_REPEATABILITY__AUTO,		// for event_type
} EN_RESERVE_REPEATABILITY;


class CRecManagerIf : public CThreadMgrExternalIf
{
public:
	typedef struct {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;

		uint16_t event_id;
		CEtime start_time;
		CEtime end_time;

		EN_RESERVE_REPEATABILITY repeatablity;

		void dump (void) const {
		_UTL_LOG_I (
				"reserve_param_t - tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] svcid:[0x%04x] time:[%s - %s] repeat:[%d]",
				transport_stream_id,
				original_network_id,
				service_id,
				event_id,
				start_time.toString(),
				end_time.toString(),
				repeatablity
			);
		}

	} ADD_RESERVE_PARAM_t;

	typedef struct {
		int index;
		EN_RESERVE_REPEATABILITY repeatablity;
	} ADD_RESERVE_HELPER_PARAM_t;

	typedef struct {
		union _arg {
			// key指定 or index指定
			struct _key {
				uint16_t transport_stream_id;
				uint16_t original_network_id;
				uint16_t service_id;
				uint16_t event_id;
			} key;
			int index;
		} arg;

		bool isConsiderRepeatability;
		bool isApplyResult;

	} REMOVE_RESERVE_PARAM_t;

public:
	explicit CRecManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CRecManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__MODULE_DOWN);
	};

	bool reqAddReserve_currentEvent (void) {
		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__ADD_RESERVE_CURRENT_EVENT
				);
	};

	bool reqAddReserve_event (ADD_RESERVE_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__ADD_RESERVE_EVENT,
					(uint8_t*)p_param,
					sizeof (ADD_RESERVE_PARAM_t)
				);
	};

	bool reqAddReserve_eventHelper (ADD_RESERVE_HELPER_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__ADD_RESERVE_EVENT_HELPER,
					(uint8_t*)p_param,
					sizeof (ADD_RESERVE_HELPER_PARAM_t)
				);
	};

	bool reqAddReserve_manual (ADD_RESERVE_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__ADD_RESERVE_MANUAL,
					(uint8_t*)p_param,
					sizeof (ADD_RESERVE_PARAM_t)
				);
	};

	bool reqRemoveReserve (REMOVE_RESERVE_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__REMOVE_RESERVE,
					(uint8_t*)p_param,
					sizeof (REMOVE_RESERVE_PARAM_t)
				);
	};

	bool reqRemoveReserve_byIndex (REMOVE_RESERVE_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__REMOVE_RESERVE_BY_INDEX,
					(uint8_t*)p_param,
					sizeof (REMOVE_RESERVE_PARAM_t)
				);
	};

	bool reqStopRecording (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__STOP_RECORDING);
	};

	bool reqDumpReserves (int type) {
		int _type = type;
		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__DUMP_RESERVES,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

};

#endif
