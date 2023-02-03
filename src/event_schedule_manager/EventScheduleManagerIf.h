#ifndef _EVENT_SCHEDULE_MANAGER_IF_H_
#define _EVENT_SCHEDULE_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "Utils.h"


class CEventScheduleManagerIf : public threadmgr::CThreadMgrExternalIf
{
public:
	enum class cache_schedule_state: int {
		init = 0,
		busy,
		ready,
	};

	typedef struct _service_key {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
	} service_key_t;

	typedef struct _event_key {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
		uint16_t event_id;
	} event_key_t;

	typedef struct _event {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;

		uint16_t event_id;
		CEtime start_time;
		CEtime end_time;

		std::string *p_event_name;
		std::string *p_text;

		void clear () {
			transport_stream_id = 0;
			original_network_id = 0;
			service_id = 0;
			event_id = 0;
			start_time.clear();
			end_time.clear();
			p_event_name = NULL;
			p_text = NULL;
		}

	} event_t;

	typedef struct _request_event_param {
		union _arg {
			// key指定 or index指定 or キーワード
			event_key_t key;
			int index;
			const char *p_keyword;
		} arg;

		event_t *p_out_event;

		// キーワードで複数検索されたときのため
		// p_out_event元が配列になっている前提のもの
		int array_max_num;

	} request_event_param_t;

public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		reg_cache_schedule_state_notify,
		unreg_cache_schedule_state_notify,
		get_cache_schedule_state,
		check_loop, // inner
		parser_notice, // inner
		exec_cache_schedule, // inner
		cache_schedule,
		cache_schedule_force_current_service,
		stop_cache_schedule,
		get_event,
		get_event__latest_dumped_schedule,
		dump_event__latest_dumped_schedule,
		get_events__keyword_search,
		get_events__keyword_search_ex,
		add_reserves, // inner
		dump_schedule_map,
		dump_schedule,
		dump_reserves,
		dump_histories,
		max,
	};

	explicit CEventScheduleManagerIf (CThreadMgrExternalIf *p_if) 
		: CThreadMgrExternalIf (p_if)
		, m_module_id (static_cast<uint8_t>(module::module_id::event_schedule_manager))
	{
	};

	virtual ~CEventScheduleManagerIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (m_module_id, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (m_module_id, sequence);
	};

	bool request_register_cache_schedule_state_notify (void) {
		int sequence = static_cast<int>(sequence::reg_cache_schedule_state_notify);
		return request_async (m_module_id, sequence);
	};

	bool request_unregister_cache_schedule_state_notify (int client_id) {
		int sequence = static_cast<int>(sequence::unreg_cache_schedule_state_notify);
		int _id = client_id;
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool request_get_cache_schedule_state (void) {
		int sequence = static_cast<int>(sequence::get_cache_schedule_state);
		return request_async (m_module_id, sequence);
	};

	bool request_get_cache_schedule_state_sync (void) {
		int sequence = static_cast<int>(sequence::get_cache_schedule_state);
		return request_sync (m_module_id, sequence);
	};

	bool request_cache_schedule (void) {
		int sequence = static_cast<int>(sequence::cache_schedule);
		return request_async (m_module_id, sequence);
	};

	bool request_cache_schedule_force_current_service (uint8_t group_id) {
		int sequence = static_cast<int>(sequence::cache_schedule_force_current_service);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)&group_id,
					sizeof(uint8_t)
				);
	};

	bool request_stop_cache_schedule (void) {
		int sequence = static_cast<int>(sequence::stop_cache_schedule);
		return request_async (m_module_id, sequence);
	};

	bool request_stop_cache_schedule_sync (void) {
		int sequence = static_cast<int>(sequence::stop_cache_schedule);
		return request_sync (m_module_id, sequence);
	};

	bool request_get_event (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_event);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_event_sync (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_event);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_event_latest_dumped_schedule (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_event__latest_dumped_schedule);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_event_latest_dumped_schedule_sync (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_event__latest_dumped_schedule);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_dump_event_latest_dumped_schedule (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::dump_event__latest_dumped_schedule);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_events_keyword (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_events__keyword_search);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_events_keyword_sync (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_events__keyword_search);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_events_keyword_ex (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_events__keyword_search_ex);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_get_events_keyword_ex_sync (request_event_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_events__keyword_search_ex);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_event_param_t)
				);
	};

	bool request_dump_schedule_map (void) {
		int sequence = static_cast<int>(sequence::dump_schedule_map);
		return request_async (m_module_id, sequence);
	};

	bool request_dump_schedule (service_key_t *p_key) {
		if (!p_key) {
			return false;
		}

		int sequence = static_cast<int>(sequence::dump_schedule);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_key,
					sizeof (service_key_t)
				);
	};

	bool request_dump_reserves (void) {
		int sequence = static_cast<int>(sequence::dump_reserves);
		return request_async (m_module_id, sequence);
	};

	bool request_dump_histories (void) {
		int sequence = static_cast<int>(sequence::dump_histories);
		return request_async (m_module_id, sequence);
	};

private:
	uint8_t m_module_id;
};

#endif
