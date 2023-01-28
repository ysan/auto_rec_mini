#ifndef _REC_MANAGER_H_
#define _REC_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "Group.h"
#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Settings.h"
#include "RecManagerIf.h"
#include "RecInstance.h"
#include "RecAribB25.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"
#include "TunerServiceIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"
#include "EventScheduleManagerIf.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"


#define RESERVE_NUM_MAX		(50)
#define RESULT_NUM_MAX		(20)

#define RESERVE_STATE__INIT								(0x00000000)
#define RESERVE_STATE__REMOVED							(0x00000001)
#define RESERVE_STATE__RESCHEDULED						(0x00000002)
#define RESERVE_STATE__START_TIME_PASSED				(0x00000004)
#define RESERVE_STATE__REQ_START_RECORDING				(0x00000100)
#define RESERVE_STATE__NOW_RECORDING					(0x00000200)
#define RESERVE_STATE__FORCE_STOPPED					(0x00000400)
#define RESERVE_STATE__END_ERROR__ALREADY_PASSED		(0x00010000)
#define RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW	(0x00020000)
#define RESERVE_STATE__END_ERROR__TUNE_ERR				(0x00040000)
#define RESERVE_STATE__END_ERROR__EVENT_NOT_FOUND		(0x00080000)
#define RESERVE_STATE__END_ERROR__INTERNAL_ERR			(0x00100000)
#define RESERVE_STATE__END_SUCCESS						(0x80000000)


struct reserve_state_pair {
	uint32_t state;
	const char *psz_reserve_state;
};

const char * get_reserve_state_string (uint32_t state);


class CRecReserve {
public:
	CRecReserve (void) {
		clear ();
	}
	virtual ~CRecReserve (void) {
		clear ();
	}

	// event_type only
	bool operator == (const CRecReserve &obj) const {
		if (
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id &&
			this->service_id == obj.service_id &&
			this->event_id == obj.event_id
		) {
			return true;
		} else {
			return false;
		}
	}

	// event_type only
	bool operator != (const CRecReserve &obj) const {
		if (
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id ||
			this->service_id != obj.service_id ||
			this->event_id != obj.event_id
		) {
			return true;
		} else {
			return false;
		}
	}

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string title_name;
	std::string service_name;

	bool is_event_type ;
	CRecManagerIf::reserve_repeatability repeatability;

	uint32_t state;

	CEtime recording_start_time;
	CEtime recording_end_time;

	uint8_t group_id;

	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		bool _is_event_type,
		CRecManagerIf::reserve_repeatability _repeatability
	)
	{
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->event_id = _event_id;
		if (p_start_time) {
			this->start_time = *p_start_time;
		}
		if (p_end_time) {
			this->end_time = *p_end_time;
		}
		if (psz_title_name) {
			this->title_name = psz_title_name ;
		}
		if (psz_service_name) {
			this->service_name = psz_service_name ;
		}
		this->is_event_type = _is_event_type;
		this->repeatability = _repeatability;
		this->state = RESERVE_STATE__INIT;
		this->is_used = true;
	}

	void clear (void) {
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();	
		title_name.clear();
		title_name.shrink_to_fit();
		service_name.clear();
		service_name.shrink_to_fit();
		is_event_type = false;
		repeatability = CRecManagerIf::reserve_repeatability::none;
		state = RESERVE_STATE__INIT;
		recording_start_time.clear();
		recording_end_time.clear();	
		group_id = 0xff;
		is_used = false;
	}

	void dump (void) const {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s] event_type:[%d] repeat:[%s] state:[%s]",
			start_time.toString(),
			end_time.toString(),
			is_event_type,
			repeatability == CRecManagerIf::reserve_repeatability::none ? "none" :
				repeatability == CRecManagerIf::reserve_repeatability::daily ? "daily" :
					repeatability == CRecManagerIf::reserve_repeatability::weekly ? "weekly" :
						repeatability == CRecManagerIf::reserve_repeatability::auto_ ? "auto" : "???",
			get_reserve_state_string (state)
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
		_UTL_LOG_I ("service_name:[%s]", service_name.c_str());

		if (state != RESERVE_STATE__INIT) {
			if (state & RESERVE_STATE__NOW_RECORDING) {
				CEtime _cur;
				_cur.setCurrentTime ();
				CEtime::CABS diff = _cur - recording_start_time;
				_UTL_LOG_I ("recording_time:[%s]", diff.toString());
				_UTL_LOG_I ("group_id:[0x%02x]", group_id);

			} else {
				CEtime::CABS diff = recording_end_time - recording_start_time;
				_UTL_LOG_I ("recording_time:[%s]", diff.toString());
				_UTL_LOG_I ("group_id:[0x%02x]", group_id);
			}
		}
	}
};


class CRecManager
	:public threadmgr::CThreadMgrBase
////	,public CTunerControlIf::ITsReceiveHandler
{
public:
	CRecManager (char *pszName, uint8_t nQueNum);
	virtual ~CRecManager (void);


	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);
	void on_module_up_by_group_id (threadmgr::CThreadMgrIf *p_if);
	void on_module_down_by_group_id (threadmgr::CThreadMgrIf *p_if);
	void on_check_loop (threadmgr::CThreadMgrIf *p_if);
	void on_check_reserves_event_loop (threadmgr::CThreadMgrIf *p_if);
	void on_check_recordings_event_loop (threadmgr::CThreadMgrIf *p_if);
	void on_recording_notice (threadmgr::CThreadMgrIf *p_if);
	void on_start_recording (threadmgr::CThreadMgrIf *p_if);
	void on_add_reserve_current_event (threadmgr::CThreadMgrIf *p_if);
	void on_add_reserve_event (threadmgr::CThreadMgrIf *p_if);
	void on_add_reserve_event_helper (threadmgr::CThreadMgrIf *p_if);
	void on_add_reserve_manual (threadmgr::CThreadMgrIf *p_if);
	void on_remove_reserve (threadmgr::CThreadMgrIf *p_if);
	void on_remove_reserve_by_index (threadmgr::CThreadMgrIf *p_if);
	void on_get_reserves (threadmgr::CThreadMgrIf *p_if);
	void on_stop_recording (threadmgr::CThreadMgrIf *p_if);
	void on_dump_reserves (threadmgr::CThreadMgrIf *p_if);

	void on_receive_notify (threadmgr::CThreadMgrIf *p_if) override;


private:
	bool add_reserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		bool _is_event_type,
		CRecManagerIf::reserve_repeatability repeatability,
		bool _is_rescheduled=false
	);
	int get_reserve_index (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id
	);
	bool remove_reserve (int index, bool is_consider_repeatability, bool is_apply_result);
	CRecReserve* search_ascending_order_reserve (CEtime *p_start_time_rf);
	bool is_exist_empty_reserve (void) const;
	CRecReserve *find_empty_reserve (void);
	bool is_duplicate_reserve (const CRecReserve* p_reserve) const;
	bool is_overrap_time_reserve (const CRecReserve* p_reserve) const;
	void check_reserves (void);
	void refresh_reserves (uint32_t state);
	bool is_exist_req_start_recording_reserve (void);
	bool pick_req_start_recording_reserve (uint8_t group_id);
	void set_result (CRecReserve *p);
	void check_recording_end (void);
	void check_disk_free (void);
	void check_repeatability (const CRecReserve *p_reserve);
	int get_reserves (CRecManagerIf::reserve_t *p_out_reserves, int out_array_num) const;
	void dump_reserves (void) const;
	void dump_results (void) const;
	void dump_recording (void) const;
	void clear_reserves (void);
	void clear_results (void);


	// CTuner_control_if::ITsReceiveHandler
////	bool on_pre_ts_receive (void) override;
////	void on_post_ts_receive (void) override;
////	bool on_check_ts_receive_loop (void) override;
////	bool on_ts_received (void *p_ts_data, int length) override;


	void save_reserves (void);
	void load_reserves (void);

	void save_results (void);
	void load_results (void);


	CSettings *mp_settings;
	
////	uint8_t m_tuner_notify_client_id;
////	int m_ts_receive_handler_id;
////	uint8_t m_pat_detect_notify_client_id;
////	uint8_t m_event_change_notify_client_id;
	uint8_t m_tuner_notify_client_id [CGroup::GROUP_MAX];
	int m_ts_receive_handler_id [CGroup::GROUP_MAX];
	uint8_t m_pat_detect_notify_client_id [CGroup::GROUP_MAX];
	uint8_t m_event_change_notify_client_id [CGroup::GROUP_MAX];

////	EN_REC_PROGRESS m_rec_progress; // tune_threadと共有する とりあえず排他はいれません

	CRecReserve m_reserves [RESERVE_NUM_MAX];
	CRecReserve m_results [RESULT_NUM_MAX];
////	CRecReserve m_recording;
	CRecReserve m_recordings [CGroup::GROUP_MAX];

////	char m_recording_tmpfile [PATH_MAX];
	char m_recording_tmpfiles [CGroup::GROUP_MAX][PATH_MAX];
////	unique_ptr<CRec_arib_b25> msp_b25;

	std::unique_ptr <CRecInstance> msp_rec_instances [CGroup::GROUP_MAX];
	CTunerControlIf::ITsReceiveHandler *mp_ts_handlers [CGroup::GROUP_MAX];

	int m_tuner_resource_max;
};

#endif
