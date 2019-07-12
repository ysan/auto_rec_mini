#ifndef _EVENT_SCHEDULE_MANAGER_IF_H_
#define _EVENT_SCHEDULE_MANAGER_IF_H_

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
	EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_UP = 0,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_DOWN,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__CHECK_LOOP,				// inner
	EN_SEQ_EVENT_SCHEDULE_MANAGER__PARSER_NOTICE,           // inner
	EN_SEQ_EVENT_SCHEDULE_MANAGER__START_CACHE_CURRENT_SERVICE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE,


	EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM,
};


class CEventScheduleManagerIf : public CThreadMgrExternalIf
{
public:
	typedef struct _service_key {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
	} SERVICE_KEY_t;

public:
	explicit CEventScheduleManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CEventScheduleManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_EVENT_SCHEDULE_MANAGER, EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_EVENT_SCHEDULE_MANAGER, EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_DOWN);
	};

	bool reqStartCache_currentService (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__START_CACHE_CURRENT_SERVICE
				);
	};

	bool reqDumpSchedule (SERVICE_KEY_t *p_key) {
		if (!p_key) {
			return false;
		}

		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE,
					(uint8_t*)p_key,
					sizeof (SERVICE_KEY_t)
				);
	};


};

#endif
