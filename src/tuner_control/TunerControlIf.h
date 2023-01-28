#ifndef _TUNER_CONTROL_IF_H_
#define _TUNER_CONTROL_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "Group.h"


#define TS_RECEIVE_HANDLER_REGISTER_NUM_MAX		(10)


class CTunerControlIf : public threadmgr::CThreadMgrExternalIf, public CGroup
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
	explicit CTunerControlIf (threadmgr::CThreadMgrExternalIf *pIf, uint8_t groupId=0)
		:threadmgr::CThreadMgrExternalIf (pIf)
		,CGroup (groupId)
	{
	};

	virtual ~CTunerControlIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

	bool request_tune (uint32_t frequest_kHz) {
		uint32_t f = frequest_kHz;
		int sequence = static_cast<int>(sequence::tune);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence, (uint8_t*)&f, sizeof(f));
	};

	bool request_tune_sync (uint32_t frequest_kHz) {
		uint32_t f = frequest_kHz;
		int sequence = static_cast<int>(sequence::tune);
		return request_sync (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence, (uint8_t*)&f, sizeof(f));
	};

	bool request_tune_stop (void) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

	bool request_tune_stop_sync (void) {
		int sequence = static_cast<int>(sequence::tune_stop);
		return request_sync (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

	bool request_register_tuner_notify (void) {
		int sequence = static_cast<int>(sequence::reg_tuner_notify);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

	bool request_unregister_tuner_notify (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_tuner_notify);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence, (uint8_t*)&_id, sizeof(_id));
	};

	bool request_register_ts_receive_handler (ITsReceiveHandler **p_handler) {
		ITsReceiveHandler **p = p_handler;
		int sequence = static_cast<int>(sequence::reg_ts_receive_handler);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence, (uint8_t*)p, sizeof(p));
	};

	bool request_unregister_ts_receive_handler (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_ts_receive_handler);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence, (uint8_t*)&_id, sizeof(_id));
	};

	bool request_get_state (void) {
		int sequence = static_cast<int>(sequence::get_state);
		return request_async (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

	bool request_get_state_sync (void) {
		int sequence = static_cast<int>(sequence::get_state);
		return request_sync (EN_MODULE_TUNER_CONTROL + getGroupId(), sequence);
	};

};

#endif
