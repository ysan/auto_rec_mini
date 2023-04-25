#ifndef _PSISI_MANAGER_IF_H_
#define _PSISI_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "Group.h"

#include "PsisiManagerStructs.h"
#include "threadmgr_if.h"


class CPsisiManagerIf : public threadmgr::CThreadMgrExternalIf, public CGroup
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		get_state,
		check_loop,					// inner
		parser_notice,				// inner
		stabilization_after_tuning,	// inner
		reg_pat_detect_notify,
		unreg_pat_detect_notify,
		reg_event_change_notify,
		unreg_event_change_notify,
		reg_psisi_state_notify,
		unreg_psisi_state_notify,
		get_pat_detect_state,
		get_stream_infos,
		get_current_service_infos,
		get_present_event_info,
		get_follow_event_info,
		get_current_network_info,
		enable_parse_eit_sched,
		disable_parse_eit_sched,
		dump_caches,
		dump_tables,
		max,
	};

	enum class psisi_type : int {
		PAT = 0,
		PMT,
		EIT_H,
		EIT_M,
		EIT_L,
		NIT,
		SDT,
		RST,
		BIT,
		CAT,
		CDT,

		// for dump command
		EIT_H_PF,
		EIT_H_PF_simple,
		EIT_H_SCHED,
		EIT_H_SCHED_event,
		EIT_H_SCHED_simple,

		max,
	};

	enum class pat_detection_state : int {
		detected = 0,
		not_detected,
	};

	enum class psisi_state : int {
		ready = 0,
		not_ready,
	};

public:
	explicit CPsisiManagerIf (CThreadMgrExternalIf *p_if, uint8_t groupId=0)
		:CThreadMgrExternalIf (p_if)
		,CGroup (groupId)
		,m_module_id (static_cast<uint8_t>(modules::module_id::psisi_manager))
	{
	};

	virtual ~CPsisiManagerIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (m_module_id + getGroupId(), sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (m_module_id + getGroupId(), sequence);
	};

	bool request_get_state (void) {
		int sequence = static_cast<int>(sequence::get_state);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_get_state_sync (void) {
		int sequence = static_cast<int>(sequence::get_state);
		return request_sync (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_register_pat_detect_notify (void) {
		int sequence = static_cast<int>(sequence::reg_pat_detect_notify);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_unregister_pat_detect_notify (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_pat_detect_notify);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool request_register_event_change_notify (void) {
		int sequence = static_cast<int>(sequence::reg_event_change_notify);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_unregister_event_change_notify (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_event_change_notify);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool request_register_psisi_state_notify (void) {
		int sequence = static_cast<int>(sequence::reg_psisi_state_notify);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_unregister_psisi_state_notify (int client_id) {
		int _id = client_id;
		int sequence = static_cast<int>(sequence::unreg_psisi_state_notify);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool request_get_pat_detect_state (void) {
		int sequence = static_cast<int>(sequence::get_pat_detect_state);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_get_stream_infos (uint16_t program_number, EN_STREAM_TYPE type, psisi_structs::stream_info_t *p_out_stream_infos, int array_max_num) {
		if (!p_out_stream_infos || array_max_num == 0) {
			return false;
		}

		psisi_structs::request_stream_infos_param_t param = {program_number, type, p_out_stream_infos, array_max_num};
		int sequence = static_cast<int>(sequence::get_stream_infos);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_stream_infos_sync (uint16_t program_number, EN_STREAM_TYPE type, psisi_structs::stream_info_t *p_out_stream_infos, int array_max_num) {
		if (!p_out_stream_infos || array_max_num == 0) {
			return false;
		}

		psisi_structs::request_stream_infos_param_t param = {program_number, type, p_out_stream_infos, array_max_num};
		int sequence = static_cast<int>(sequence::get_stream_infos);
		return request_sync (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_current_service_infos (psisi_structs::service_info_t *p_out_service_infos, int array_max_num) {
		if (!p_out_service_infos || array_max_num == 0) {
			return false;
		}

		psisi_structs::request_service_infos_param_t param = {p_out_service_infos, array_max_num};
		int sequence = static_cast<int>(sequence::get_current_service_infos);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_present_event_info (psisi_structs::service_info_t *p_key, psisi_structs::event_info_t *p_out_event_info) {
		if (!p_key || !p_out_event_info) {
			return false;
		}

		psisi_structs::request_event_info_param_t param = {*p_key, p_out_event_info};
		int sequence = static_cast<int>(sequence::get_present_event_info);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_present_event_info_sync (psisi_structs::service_info_t *p_key, psisi_structs::event_info_t *p_out_event_info) {
		if (!p_key || !p_out_event_info) {
			return false;
		}

		psisi_structs::request_event_info_param_t param = {*p_key, p_out_event_info};
		int sequence = static_cast<int>(sequence::get_present_event_info);
		return request_sync (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_follow_event_info (psisi_structs::service_info_t *p_key, psisi_structs::event_info_t *p_out_event_info) {
		if (!p_key || !p_out_event_info) {
			return false;
		}

		psisi_structs::request_event_info_param_t param = {*p_key, p_out_event_info};
		int sequence = static_cast<int>(sequence::get_follow_event_info);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_follow_event_info_sync (psisi_structs::service_info_t *p_key, psisi_structs::event_info_t *p_out_event_info) {
		if (!p_key || !p_out_event_info) {
			return false;
		}

		psisi_structs::request_event_info_param_t param = {*p_key, p_out_event_info};
		int sequence = static_cast<int>(sequence::get_follow_event_info);
		return request_sync (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_get_current_network_info (psisi_structs::network_info_t *p_out_network_info) {
		if (!p_out_network_info) {
			return false;
		}

		psisi_structs::request_network_info_param_t param = {p_out_network_info};
		int sequence = static_cast<int>(sequence::get_current_network_info);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool request_enable_parse_eit_sched (void) {
		int sequence = static_cast<int>(sequence::enable_parse_eit_sched);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_disable_parse_eit_sched (void) {
		int sequence = static_cast<int>(sequence::disable_parse_eit_sched);
		return request_async (
					m_module_id + getGroupId(),
					sequence
				);
	};

	bool request_dump_caches (int type) {
		int _type = type;
		int sequence = static_cast<int>(sequence::dump_caches);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

	bool request_dump_tables (psisi_type type) {
		psisi_type _type = type;
		int sequence = static_cast<int>(sequence::dump_tables);
		return request_async (
					m_module_id + getGroupId(),
					sequence,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

private:
	uint8_t m_module_id ;
};

#endif
