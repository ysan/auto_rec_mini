#ifndef _TUNER_SERVICE_IF_H_
#define _TUNER_SERVICE_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "client_if.h"
#include "modules.h"


namespace arec {
namespace tuner_service {

class client_iface : public arec::client_iface_base
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
	explicit client_iface (coseq::manager *p_mgr)
		: arec::client_iface_base (p_mgr)
		, module_id_ (static_cast<uint8_t>(modules::module_id::tuner_service))
	{
	};

	virtual ~client_iface (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_sync (module_id_, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (module_id_, sequence);
	};

	bool request_open (void) {
		int sequence = static_cast<int>(sequence::open);
		return request_async (module_id_, sequence);
	};

	arec::client_reply request_open_sync (void) {
		int sequence = static_cast<int>(sequence::open);
		return request_sync (module_id_, sequence);
	};

	bool request_close (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::close);
		return request_async (
					module_id_,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	arec::client_reply request_close_sync (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::close);
		return request_sync (
					module_id_,
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
					module_id_,
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
					module_id_,
					sequence,
					(uint8_t*)p_param,
					sizeof(tune_param_t)
				);
	};

	bool request_tune_stop (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_async (
					module_id_,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	arec::client_reply request_tune_stop_sync (uint8_t tuner_id) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_sync (
					module_id_,
					sequence,
					(uint8_t*)&tuner_id,
					sizeof(tuner_id)
				);
	};

	bool request_dump_allocates (void) {
		int sequence = static_cast<int>(sequence::dump_allocates);
		return request_async (module_id_, sequence);
	};

private:
	uint8_t module_id_;
};

} // namespace tuner_service
} // namespace arec

#endif
