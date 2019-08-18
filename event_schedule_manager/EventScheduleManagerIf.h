#ifndef _EVENT_SCHEDULE_MANAGER_IF_H_
#define _EVENT_SCHEDULE_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>

#include "ThreadMgrExternalIf.h"
#include "modules.h"
#include "Utils.h"


using namespace ThreadManager;

enum {
	EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_UP = 0,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_DOWN,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__REG_CACHE_SCHEDULE_STATE_NOTIFY,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__UNREG_CACHE_SCHEDULE_STATE_NOTIFY,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_CACHE_SCHEDULE_STATE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__CHECK_LOOP,				// inner
	EN_SEQ_EVENT_SCHEDULE_MANAGER__PARSER_NOTICE,           // inner
	EN_SEQ_EVENT_SCHEDULE_MANAGER__START_CACHE_SCHEDULE,	// inner
	EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE_CURRENT_SERVICE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT__LATEST_DUMPED_SCHEDULE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_EVENT__LATEST_DUMPED_SCHEDULE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH_EX,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE_MAP,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE,
	EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_HISTORIES,


	EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM,
};

typedef enum {
	EN_CACHE_SCHEDULE_STATE__INIT = 0,
	EN_CACHE_SCHEDULE_STATE__BUSY,
	EN_CACHE_SCHEDULE_STATE__READY,

} EN_CACHE_SCHEDULE_STATE_t;


class CEventScheduleManagerIf : public CThreadMgrExternalIf
{
public:
	typedef struct _service_key {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
	} SERVICE_KEY_t;

	typedef struct _event_key {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
		uint16_t event_id;
	} EVENT_KEY_t;

	typedef struct _event {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;

		uint16_t event_id;
		CEtime start_time;
		CEtime end_time;

		std::string *p_event_name;
		std::string *p_text;

	} EVENT_t;

	typedef struct _req_event_param {
		union _arg {
			// key指定 or index指定 or キーワード
			EVENT_KEY_t key;
			int index;
			const char *p_keyword;
		} arg;

		EVENT_t *p_out_event;

		// キーワードで複数検索されたときのため
		// p_out_event元が配列になっている前提のもの
		int array_max_num;

	} REQ_EVENT_PARAM_t;

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

	bool reqRegisterCacheScheduleStateNotify (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__REG_CACHE_SCHEDULE_STATE_NOTIFY
				);
	};

	bool reqUnregisterCacheScheduleStateNotify (int client_id) {
		int _id = client_id;
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__UNREG_CACHE_SCHEDULE_STATE_NOTIFY,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool reqGetCacheScheduleState (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_CACHE_SCHEDULE_STATE
				);
	};

	bool syncGetCacheScheduleState (void) {
		return requestSync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_CACHE_SCHEDULE_STATE
				);
	};

	bool reqCacheSchedule (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE
				);
	};

	bool reqCacheSchedule_currentService (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE_CURRENT_SERVICE
				);
	};

	bool reqGetEvent (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool syncGetEvent (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestSync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool reqGetEvent_latestDumpedSchedule (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT__LATEST_DUMPED_SCHEDULE,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool syncGetEvent_latestDumpedSchedule (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestSync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT__LATEST_DUMPED_SCHEDULE,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool reqDumpEvent_latestDumpedSchedule (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_EVENT__LATEST_DUMPED_SCHEDULE,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool reqGetEvent_keyword (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool syncGetEvent_keyword (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestSync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool reqGetEvent_keyword_ex (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH_EX,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool syncGetEvent_keyword_ex (REQ_EVENT_PARAM_t *p_param) {
		if (!p_param) {
			return false;
		}

		return requestSync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH_EX,
					(uint8_t*)p_param,
					sizeof (REQ_EVENT_PARAM_t)
				);
	};

	bool reqDumpScheduleMap (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE_MAP
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

	bool reqDumpHistories (void) {
		return requestAsync (
					EN_MODULE_EVENT_SCHEDULE_MANAGER,
					EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_HISTORIES
				);
	};

};

#endif
