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
	EN_SEQ_REC_MANAGER_MODULE_UP = 0,
	EN_SEQ_REC_MANAGER_MODULE_DOWN,
	EN_SEQ_REC_MANAGER_CHECK_LOOP,				// inner
	EN_SEQ_REC_MANAGER_RECORDING_NOTICE,		// inner
	EN_SEQ_REC_MANAGER_START_RECORDING,			// inner
	EN_SEQ_REC_MANAGER_ADD_RESERVE_CURRENT_EVENT,
	EN_SEQ_REC_MANAGER_ADD_RESERVE_MANUAL,
	EN_SEQ_REC_MANAGER_REMOVE_RESERVE,
	EN_SEQ_REC_MANAGER_STOP_RECORDING,
	EN_SEQ_REC_MANAGER_DUMP_RESERVES,

	EN_SEQ_REC_MANAGER_NUM,
};

typedef enum {
	EN_RESERVE_REPEATABILITY__NONE,
	EN_RESERVE_REPEATABILITY__DAYLY,
	EN_RESERVE_REPEATABILITY__WEEKLY,
} EN_RESERVE_REPEATABILITY;

typedef struct {
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	CEtime start_time;
	CEtime end_time;

	EN_RESERVE_REPEATABILITY repeatablity;

	void dump (void) {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] time:[%s - %s] repeat:[%d]",
			transport_stream_id,
			original_network_id,
			service_id,
			start_time.toString(),
			end_time.toString(),
			repeatablity
		);
	}

} _MANUAL_RESERVE_PARAM;


class CRecManagerIf : public CThreadMgrExternalIf
{
public:
	explicit CRecManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CRecManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER_MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER_MODULE_DOWN);
	};

	bool reqAddReserve_currentEvent (void) {
		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER_ADD_RESERVE_CURRENT_EVENT
				);
	};

	bool reqAddReserve_manual (_MANUAL_RESERVE_PARAM *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER_ADD_RESERVE_MANUAL,
					(uint8_t*)p_param,
					sizeof (_MANUAL_RESERVE_PARAM)
				);
	};

	bool reqRemoveReserve (int index) {
		int _idx = index;
		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER_REMOVE_RESERVE,
					(uint8_t*)&_idx,
					sizeof (int)
				);
	};

	bool reqStopRecording (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER_STOP_RECORDING);
	};

	bool reqDumpReserves (int type) {
		int _type = type;
		return requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER_DUMP_RESERVES,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

};

#endif
