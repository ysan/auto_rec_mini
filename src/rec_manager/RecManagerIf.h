#ifndef _REC_MANAGER_IF_H_
#define _REC_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "Utils.h"


class CRecManagerIf : public threadmgr::CThreadMgrExternalIf
{
public:
	enum class sequence : int {
		module_up = 0,
		module_up_by_groupid,			// inner
		module_down,
		check_loop,						// inner
		check_reserves_event_loop,		// inner
		check_recordings_event_loop,	// inner
		recording_notice,				// inner
		start_recording,				// inner
		add_reserve_current_event,
		add_reserve_event,
		add_reserve_event_helper,
		add_reserve_manual,
		remove_reserve,
		remove_reserve_by_index,
		get_reserves,
		stop_recording,
		dump_reserves,
		max,
	};

	enum class reserve_repeatability : int {
		none = 0,
		daily,
		weekly,
		auto_,		// for event_type
	};

	typedef struct {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;

		uint16_t event_id;
		CEtime start_time;
		CEtime end_time;

		reserve_repeatability repeatablity;

		void clear (void) {
			transport_stream_id = 0;
			original_network_id = 0;
			service_id = 0;
			event_id = 0;
			start_time.clear();
			end_time.clear();
			repeatablity = reserve_repeatability::none;
		}

		void dump (void) const {
			_UTL_LOG_I (
				"reserve_param_t - tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x] time:[%s - %s] repeat:[%d]",
				transport_stream_id,
				original_network_id,
				service_id,
				event_id,
				start_time.toString(),
				end_time.toString(),
				repeatablity
			);
		}

	} add_reserve_param_t;

	typedef struct {
		int index;
		reserve_repeatability repeatablity;
	} add_reserve_helper_param_t;

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

		bool is_consider_repeatability;
		bool is_apply_result;

	} remove_reserve_param_t;

	typedef struct _reserve {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;

		uint16_t event_id;
		CEtime start_time;
		CEtime end_time;

		std::string *p_title_name;
		std::string *p_service_name;

		bool is_event_type ;
		reserve_repeatability repeatability;

	} reserve_t;

	typedef struct _get_reserves_param {
		reserve_t *p_out_reserves;
		int array_max_num;
	} get_reserves_param_t;


public:
	explicit CRecManagerIf (CThreadMgrExternalIf *p_if) : CThreadMgrExternalIf (p_if) {
	};

	virtual ~CRecManagerIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (EN_MODULE_REC_MANAGER, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (EN_MODULE_REC_MANAGER, sequence);
	};

	bool request_add_reserve_current_event (uint8_t group_id) {
		int sequence = static_cast<int>(sequence::add_reserve_current_event);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)&group_id,
					sizeof(uint8_t)
				);
	};

	bool request_add_reserve_event (add_reserve_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::add_reserve_event);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (add_reserve_param_t)
				);
	};

	bool request_add_reserve_event_helper (add_reserve_helper_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::add_reserve_event_helper);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (add_reserve_helper_param_t)
				);
	};

	bool request_add_reserve_manual (add_reserve_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::add_reserve_manual);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (add_reserve_param_t)
				);
	};

	bool request_remove_reserve (remove_reserve_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::remove_reserve);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (remove_reserve_param_t)
				);
	};

	bool request_remove_reserve_by_index (remove_reserve_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::remove_reserve_by_index);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (remove_reserve_param_t)
				);
	};

	bool request_get_reserves (get_reserves_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_reserves);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (get_reserves_param_t)
				);
	};

	bool request_get_reserves_sync (get_reserves_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_reserves);
		return request_sync (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)p_param,
					sizeof (get_reserves_param_t)
				);
	};

	bool request_stop_recording (uint8_t group_id) {
		int sequence = static_cast<int>(sequence::stop_recording);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)&group_id,
					sizeof(uint8_t)
				);
	};

	bool request_dump_reserves (int type) {
		int _type = type;
		int sequence = static_cast<int>(sequence::dump_reserves);
		return request_async (
					EN_MODULE_REC_MANAGER,
					sequence,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

};

#endif
