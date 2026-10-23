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

#include "group.h"
#include "coseqpp.h"

#include "utils.h"
#include "settings.h"
#include "rec_manager_if.h"
#include "rec_instance.h"
#include "rec_arib_b25.h"
#include "ts_arib_common.h"

#include "tuner_control_if.h"
#include "tuner_service_if.h"
#include "psisi_manager_if.h"
#include "channel_manager_if.h"
#include "event_schedule_manager_if.h"

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


namespace arec {
namespace rec {

class reserve {
public:
	reserve (void) {
		clear ();
	}
	virtual ~reserve (void) {
		clear ();
	}

	// event_type only
	bool operator == (const reserve &obj) const {
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
	bool operator != (const reserve &obj) const {
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
	arec::common::etime start_time;
	arec::common::etime end_time;

	std::string title_name;
	std::string service_name;

	bool is_event_type ;
	client_iface::reserve_repeatability repeatability;

	uint32_t state;

	arec::common::etime recording_start_time;
	arec::common::etime recording_end_time;

	uint8_t group_id;

	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		arec::common::etime* p_start_time,
		arec::common::etime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		bool _is_event_type,
		client_iface::reserve_repeatability _repeatability
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
		repeatability = client_iface::reserve_repeatability::none;
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
			start_time.to_string(),
			end_time.to_string(),
			is_event_type,
			repeatability == client_iface::reserve_repeatability::none ? "none" :
				repeatability == client_iface::reserve_repeatability::daily ? "daily" :
					repeatability == client_iface::reserve_repeatability::weekly ? "weekly" :
						repeatability == client_iface::reserve_repeatability::auto_ ? "auto" : "???",
			get_reserve_state_string (state)
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
		_UTL_LOG_I ("service_name:[%s]", service_name.c_str());

		if (state != RESERVE_STATE__INIT) {
			if (state & RESERVE_STATE__NOW_RECORDING) {
				arec::common::etime _cur;
				_cur.set_current_time ();
				arec::common::etime::abs diff = _cur - recording_start_time;
				_UTL_LOG_I ("recording_time:[%s]", diff.to_string());
				_UTL_LOG_I ("group_id:[0x%02x]", group_id);

			} else {
				arec::common::etime::abs diff = recording_end_time - recording_start_time;
				_UTL_LOG_I ("recording_time:[%s]", diff.to_string());
				_UTL_LOG_I ("group_id:[0x%02x]", group_id);
			}
		}
	}
};


class manager
	:public coseq::module_base
////	,public arec::tuner_control::client_iface::i_ts_receive_handler
{
public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_module_up_by_group_id (coseq::iface *iface);
	void on_module_down_by_group_id (coseq::iface *iface);
	void on_check_loop (coseq::iface *iface);
	void on_check_reserves_event_loop (coseq::iface *iface);
	void on_check_recordings_event_loop (coseq::iface *iface);
	void on_recording_notice (coseq::iface *iface);
	void on_start_recording (coseq::iface *iface);
	void on_add_reserve_current_event (coseq::iface *iface);
	void on_add_reserve_event (coseq::iface *iface);
	void on_add_reserve_event_helper (coseq::iface *iface);
	void on_add_reserve_manual (coseq::iface *iface);
	void on_remove_reserve (coseq::iface *iface);
	void on_remove_reserve_by_index (coseq::iface *iface);
	void on_get_reserves (coseq::iface *iface);
	void on_stop_recording (coseq::iface *iface);
	void on_dump_reserves (coseq::iface *iface);

	void on_receive_notify (coseq::iface *iface) override;


private:
	bool add_reserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		arec::common::etime* p_start_time,
		arec::common::etime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		bool _is_event_type,
		client_iface::reserve_repeatability repeatability,
		bool _is_rescheduled=false
	);
	int get_reserve_index (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id
	);
	bool remove_reserve (int index, bool is_consider_repeatability, bool is_apply_result);
	reserve* search_ascending_order_reserve (arec::common::etime *p_start_time_rf);
	bool is_exist_empty_reserve (void) const;
	reserve *find_empty_reserve (void);
	bool is_duplicate_reserve (const reserve* p_reserve) const;
	bool is_overrap_time_reserve (const reserve* p_reserve) const;
	void check_reserves (void);
	void refresh_reserves (uint32_t state);
	bool is_exist_req_start_recording_reserve (void);
	bool pick_req_start_recording_reserve (uint8_t group_id);
	void set_result (reserve *p);
	void check_recording_end (void);
	void check_disk_free (coseq::iface *iface);
	void check_repeatability (const reserve *p_reserve);
	int get_reserves (client_iface::reserve_t *p_out_reserves, int out_array_num) const;
	void dump_reserves (void) const;
	void dump_results (void) const;
	void dump_recording (void) const;
	void clear_reserves (void);
	void clear_results (void);


	// arec::tuner_control::client_iface::i_ts_receive_handler
////	bool on_pre_ts_receive (void) override;
////	void on_post_ts_receive (void) override;
////	bool on_check_ts_receive_loop (void) override;
////	bool on_ts_received (void *p_ts_data, int length) override;


	void save_reserves (void);
	void load_reserves (void);

	void save_results (void);
	void load_results (void);


	arec::settings *settings_;
	
////	uint8_t tuner_notify_client_id_;
////	int ts_receive_handler_id_;
////	uint8_t pat_detect_notify_client_id_;
////	uint8_t event_change_notify_client_id_;
	uint8_t tuner_notify_client_id_ [arec::group::GROUP_MAX];
	int ts_receive_handler_id_ [arec::group::GROUP_MAX];
	uint8_t pat_detect_notify_client_id_ [arec::group::GROUP_MAX];
	uint8_t event_change_notify_client_id_ [arec::group::GROUP_MAX];

////	EN_REC_PROGRESS rec_progress_; // tune_threadと共有する とりあえず排他はいれません

	reserve reserves_ [RESERVE_NUM_MAX];
	reserve results_ [RESULT_NUM_MAX];
////	reserve recording_;
	reserve recordings_ [arec::group::GROUP_MAX];

////	char recording_tmpfile_ [PATH_MAX];
	char recording_tmpfiles_ [arec::group::GROUP_MAX][PATH_MAX];
////	unique_ptr<arib_b25> b25_;

	std::unique_ptr <instance> rec_instances_ [arec::group::GROUP_MAX];
	arec::tuner_control::client_iface::i_ts_receive_handler *ts_handlers_ [arec::group::GROUP_MAX];

	int tuner_resource_max_;
};

} // namespace rec
} // namespace arec

#endif
