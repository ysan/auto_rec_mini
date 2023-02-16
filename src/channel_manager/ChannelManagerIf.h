#ifndef _CHANNEL_MANAGER_IF_H_
#define _CHANNEL_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"


class CChannelManagerIf : public threadmgr::CThreadMgrExternalIf
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		channel_scan,
		get_pysical_channel_by_service_id,
		get_pysical_channel_by_remote_control_key_id,
		get_service_id_by_pysical_channel,
		get_channels,
		get_transport_stream_name,
		get_service_name,
		dump_scan_results,
		max,
	};

	typedef struct _service_id_param {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
	} service_id_param_t;

	typedef struct _remote_control_id_param {
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint8_t remote_control_key_id;
	} remote_control_id_param_t;

	typedef struct _request_service_id_param {
		uint16_t pysical_channel;
		int service_idx;
	} request_service_id_param_t;

	typedef struct _channel {
		uint16_t pysical_channel;
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint8_t remote_control_key_id;
		uint16_t service_ids[10];
		int service_num;
	} channel_t;

	typedef struct _request_channels_param {
		channel_t *p_out_channels;
		int array_max_num;
	} request_channels_param_t;


public:
	explicit CChannelManagerIf (CThreadMgrExternalIf *p_if)
		: CThreadMgrExternalIf (p_if)
		, m_module_id (static_cast<uint8_t>(modules::module_id::channel_manager))
	{
	};

	virtual ~CChannelManagerIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (m_module_id, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (m_module_id, sequence);
	};

	bool request_channel_scan (void) {
		int sequence = static_cast<int>(sequence::channel_scan);
		return request_async (m_module_id, sequence);
	};

	bool request_get_pysical_channel_by_service_id (const service_id_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_pysical_channel_by_service_id);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (service_id_param_t)
				);
	};

	bool request_get_pysical_channel_by_remote_control_key_id (const remote_control_id_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_pysical_channel_by_remote_control_key_id);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (remote_control_id_param_t)
				);
	};

	bool request_get_service_id_by_pysical_channel (const request_service_id_param_t *param) {
		int sequence = static_cast<int>(sequence::get_service_id_by_pysical_channel);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)param,
					sizeof (request_service_id_param_t)
				);
	};

	bool request_get_channels (request_channels_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_channels);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_channels_param_t)
				);
	};

	bool request_get_channels_sync (request_channels_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_channels);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (request_channels_param_t)
				);
	};

	bool request_get_transport_stream_name (const service_id_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_transport_stream_name);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (service_id_param_t)
				);
	};

	bool request_get_transport_stream_name_sync (const service_id_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_transport_stream_name);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (service_id_param_t)
				);
	};

	bool request_get_service_name (const service_id_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_service_name);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (service_id_param_t)
				);
	};

	bool request_get_service_name_sync (const service_id_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::get_service_name);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof (service_id_param_t)
				);
	};

	bool request_dump_channels (void) {
		int sequence = static_cast<int>(sequence::dump_scan_results);
		return request_async (m_module_id, sequence);
	};

private:
	uint8_t m_module_id;
};

#endif
