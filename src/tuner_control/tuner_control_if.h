#ifndef _TUNER_CONTROL_IF_H_
#define _TUNER_CONTROL_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "client_if.h"
#include "modules.h"
#include "group.h"


#define TS_RECEIVE_HANDLER_REGISTER_NUM_MAX		(10)


namespace arec {
namespace tuner_control {

class client_iface : public arec::client_iface_base, public arec::group
{
public:
	class ITsReceiveHandler {
	public:
		virtual ~ITsReceiveHandler (void) {};
		virtual bool on_pre_ts_receive (void) = 0;
		virtual void on_post_ts_receive (void) = 0;
		virtual bool on_check_ts_receive_loop (void) = 0;
		virtual bool on_ts_received (void *p_ts_data, int length) = 0;
	};

	enum class tuner_state : int {
		tuning_begin = 0,		// closed -> open -> tune
		tuning_success,			// tuning -> tuned
		tuning_error_stop,		// not tune, not close 
		tune_stop,				// closed
	};

	enum class sequence : int {
		module_up = 0,
		module_down,
		tune,
		tune_start,
		tune_stop,
		reg_tuner_notify,
		unreg_tuner_notify,
		reg_ts_receive_handler,
		unreg_ts_receive_handler,
		get_state,
		max,
	};

public:
	explicit client_iface (coseq::manager *p_mgr, uint8_t groupId=0)
		:arec::client_iface_base (p_mgr)
		,arec::group (groupId)
		,module_id_ (static_cast<uint8_t>(modules::module_id::tuner_control))
	{
	};

	virtual ~client_iface (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_sync (module_id_ + get_group_id(), sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (module_id_ + get_group_id(), sequence);
	};

	bool request_tune (uint32_t frequest_kHz) {
		uint32_t f = frequest_kHz;
		int sequence = static_cast<int>(sequence::tune);
		return request_async (module_id_ + get_group_id(), sequence, (uint8_t*)&f, sizeof(f));
	};

	arec::client_reply request_tune_sync (uint32_t frequest_kHz) {
		uint32_t f = frequest_kHz;
		int sequence = static_cast<int>(sequence::tune);
		return request_sync (module_id_ + get_group_id(), sequence, (uint8_t*)&f, sizeof(f));
	};

	bool request_tune_stop (void) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_async (module_id_ + get_group_id(), sequence);
	};

	arec::client_reply request_tune_stop_sync (void) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_sync (module_id_ + get_group_id(), sequence);
	};

	bool request_register_tuner_notify (void) {
		int sequence = static_cast<int>(sequence::reg_tuner_notify);
		return request_async (module_id_ + get_group_id(), sequence);
	};

	bool request_unregister_tuner_notify (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_tuner_notify);
		return request_async (module_id_ + get_group_id(), sequence, (uint8_t*)&_id, sizeof(_id));
	};

	arec::client_reply request_register_ts_receive_handler (ITsReceiveHandler **p_handler) {
		ITsReceiveHandler **p = p_handler;
		int sequence = static_cast<int>(sequence::reg_ts_receive_handler);
		return request_async (module_id_ + get_group_id(), sequence, (uint8_t*)p, sizeof(p));
	};

	bool request_unregister_ts_receive_handler (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_ts_receive_handler);
		return request_async (module_id_ + get_group_id(), sequence, (uint8_t*)&_id, sizeof(_id));
	};

	bool request_get_state (void) {
		int sequence = static_cast<int>(sequence::get_state);
		return request_async (module_id_ + get_group_id(), sequence);
	};

	arec::client_reply request_get_state_sync (void) {
		int sequence = static_cast<int>(sequence::get_state);
		return request_sync (module_id_ + get_group_id(), sequence);
	};

private:
	uint8_t module_id_ ;
};

} // namespace tuner_control
} // namespace arec

#endif
