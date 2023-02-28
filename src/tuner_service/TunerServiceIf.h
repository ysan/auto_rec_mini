#ifndef _TUNER_SERVICE_IF_H_
#define _TUNER_SERVICE_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"


class CTunerServiceIf : public threadmgr::CThreadMgrExternalIf
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		open,
		close,
		tune,
		tune_with_retry,
		tune_stop,
		dump_allocates,
		max
	};

	typedef struct _tune_param {
		uint16_t physical_channel;
		uint8_t tuner_id;
	} tune_param_t;

public:
	explicit CTunerServiceIf (CThreadMgrExternalIf *p_if)
		: CThreadMgrExternalIf (p_if)
		, m_module_id (static_cast<uint8_t>(modules::module_id::tuner_service))
	{
	};

	virtual ~CTunerServiceIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (m_module_id, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (m_module_id, sequence);
	};

	bool request_open (void) {
		int sequence = static_cast<int>(sequence::open);
		return request_async (m_module_id, sequence);
	};

	bool request_open_sync (void) {
		int sequence = static_cast<int>(sequence::open);
		return request_sync (m_module_id, sequence);
	};

	bool request_close (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::close);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	bool request_close_sync (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::close);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	bool request_tune (const tune_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::tune);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof(tune_param_t)
				);
	};

	bool request_tune_with_retry (const tune_param_t *p_param) {
		if (!p_param) {
			return false;
		}

		int sequence = static_cast<int>(sequence::tune_with_retry);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)p_param,
					sizeof(tune_param_t)
				);
	};

	bool request_tune_stop (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_async (
					m_module_id,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	bool request_tune_stop_sync (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_sync (
					m_module_id,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	bool request_dump_allocates (void) {
		int sequence = static_cast<int>(sequence::dump_allocates);
		return request_async (m_module_id, sequence);
	};

private:
	uint8_t m_module_id;
};

#endif
