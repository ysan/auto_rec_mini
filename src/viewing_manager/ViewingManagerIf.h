#ifndef _VIEWING_MANAGER_IF_H_
#define _VIEWING_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "Utils.h"


class CViewingManagerIf : public threadmgr::CThreadMgrExternalIf
{
public:
	enum class sequence : uint8_t {
		module_up = 0,
		module_up_by_groupid,			// inner
		module_down,
		check_loop,						// inner
		notice_by_stream_handler,		// from stream handler
		start_viewing_by_physical_channel,
		start_viewing_by_service_id,
		stop_viewing,
		get_viewing,
		set_option,
		dump_viewing,					// from command server
		event_changed,					// inner
		max,
	};

	typedef struct _physical_channel_param {
		uint16_t physical_channel;
		int service_idx;
	} physical_channel_param_t;

	typedef struct _service_id_param {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
	} service_id_param_t;

	typedef struct _option {
		uint8_t group_id; // target group_id
		int auto_stop_grace_period_sec; // 0 is disable auto stop
	} option_t;

public:
	explicit CViewingManagerIf (CThreadMgrExternalIf *p_if)
		: CThreadMgrExternalIf (p_if)
		, m_module_id (static_cast<uint8_t>(modules::module_id::viewing_manager))
	{
	};

	virtual ~CViewingManagerIf (void) {
	};


	bool request_module_up (void) {
		uint8_t sequence = static_cast<uint8_t>(sequence::module_up);
		return request_async (m_module_id, sequence);
	};

	bool request_module_down (void) {
		uint8_t sequence = static_cast<uint8_t>(sequence::module_down);
		return request_async (m_module_id, sequence);
	};

	bool request_start_viewing_by_physical_channel (const physical_channel_param_t *param, bool async=true) {
		uint8_t sequence = static_cast<uint8_t>(sequence::start_viewing_by_physical_channel);
		if (async) {
			return request_async (m_module_id, sequence, (uint8_t*)param, sizeof(physical_channel_param_t));
		} else {
			return request_sync (m_module_id, sequence, (uint8_t*)param, sizeof(physical_channel_param_t));
		}
	};

	bool request_start_viewing_by_service_id (const service_id_param_t *param, bool async=true) {
		uint8_t sequence = static_cast<uint8_t>(sequence::start_viewing_by_service_id);
		if (async) {
			return request_async (m_module_id, sequence, (uint8_t*)param, sizeof(service_id_param_t));
		} else {
			return request_sync (m_module_id, sequence, (uint8_t*)param, sizeof(service_id_param_t));
		}
	};

	bool request_stop_viewing (uint8_t group_id, bool async=true) {
		uint8_t sequence = static_cast<uint8_t>(sequence::stop_viewing);
		if (async) {
			return request_async (m_module_id, sequence, (uint8_t*)&group_id, sizeof(group_id));
		} else {
			return request_sync (m_module_id, sequence, (uint8_t*)&group_id, sizeof(group_id));
		}
	};

	bool request_set_option (const option_t *opt, bool async=true) {
		uint8_t sequence = static_cast<uint8_t>(sequence::set_option);
		if (async) {
			return request_async (m_module_id, sequence, (uint8_t*)opt, sizeof(option_t));
		} else {
			return request_sync (m_module_id, sequence, (uint8_t*)opt, sizeof(option_t));
		}
	};

private:
	uint8_t m_module_id;
};

#endif
