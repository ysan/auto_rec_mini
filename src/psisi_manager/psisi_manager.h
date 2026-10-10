#ifndef _PSISI_MANAGER_H_
#define _PSISI_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <memory>
#include <mutex>

#include "coseqpp.h"

#include "utils.h"
#include "group.h"
#include "settings.h"
#include "psisi_manager_if.h"

#include "tuner_control_if.h"
#include "ts_parser.h"
#include "event_schedule_manager_if.h"

#include "program_association_table.h"
#include "program_map_table.h"
#include "event_information_table.h"
#include "event_information_table_sched.h"
#include "network_information_table.h"
#include "service_description_table.h"
#include "running_status_table.h"
#include "broadcaster_information_table.h"
#include "time_offset_table.h"
#include "conditional_access_table.h"
#include "common_data_table.h"


// notify category
#define NOTIFY_CAT__PAT_DETECT			((uint8_t)0)
#define NOTIFY_CAT__EVENT_CHANGE		((uint8_t)1)
#define NOTIFY_CAT__PSISI_STATE		 	((uint8_t)2)


#define PROGRAM_INFOS_MAX			(32)
#define SERVICE_INFOS_MAX			(64)
#define EVENT_PF_INFOS_MAX			(32)

#define TMP_PROGRAM_MAPS_MAX		(16)


namespace arec {
namespace psisi {

enum class event_pf_state : int {
	init = 0,
	present,
	follow,
	already_passed,
	max,
};

static const char *gpsz_event_pf_state [static_cast<int>(event_pf_state::max)] = {
	// for debug log
	"-",
	"P",
	"F",
	"X",
};


class program_map {
public:
	program_map (void) {
		clear();
	}
	virtual ~program_map (void) {
		clear();
	}

	std::shared_ptr <parser::program_map_table> get (uint16_t pid) {
		// parsers_は確定しているので排他いれません

		const auto it = parsers_.find(pid);
		if (it != parsers_.end()) {
			return it->second;
		} else {
			return nullptr;
		}
	}

	void add (uint16_t pid) {
		// tuner threadとの排他します
		std::lock_guard <std::mutex> lock(mutex_);

		parsers_ [pid] = std::make_shared<parser::program_map_table>();
	}

	parser::EN_CHECK_SECTION parse (parser::TS_HEADER *ts_header, uint8_t* p_payload, size_t payload_size) {
		// parsers_は確定しているので排他いれません

		const auto it = parsers_.find(ts_header->pid);
		if (it != parsers_.end()) {
			 return it->second->check_section (ts_header, p_payload, payload_size);
		} else {
			return parser::EN_CHECK_SECTION__INVALID;
		}
	}

	bool has (uint16_t pid) {
		// tuner thread側で使うので排他します
		std::lock_guard <std::mutex> lock(mutex_);

		const auto &it = parsers_.find(pid);
		if (it != parsers_.end()) {
			return true;
		} else {
			return false;
		}
	}

	void dump (void) {
		for (const auto &parser : parsers_) {
			parser.second->dump_tables ();
		}
	}

	void clear (void) {
		// tuner threadとの排他します
		std::lock_guard <std::mutex> lock(mutex_);

		for (const auto &parser : parsers_) {
			parser.second->force_clear ();
		}
		parsers_.clear();
	}

private:
	std::unordered_map<uint16_t, std::shared_ptr<parser::program_map_table>> parsers_; // <pid, pmt parser>
	std::mutex mutex_; // tuner thread との排他のため
};

typedef struct _program_info {
public:
	struct _stream {
		uint8_t type;
		uint16_t pid;
	};

public:
	_program_info (void) {
		clear ();
	}
	virtual ~_program_info (void) {
		clear ();
	}

	uint8_t table_id;
	uint16_t transport_stream_id;

	uint16_t program_number;
	uint16_t program_map_PID;

	parser::program_association_table::table* p_org_table;

	std::vector<std::shared_ptr<_stream>> streams;

	bool is_used;

	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		program_number = 0;
		program_map_PID = 0;
		p_org_table = NULL;
		streams.clear();
		is_used = false;
	}

	void dump (void) {
		if (p_org_table) {
			p_org_table->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] pgm_num:[0x%04x] pmt_pid:[0x%04x]",
			table_id,
			transport_stream_id,
			program_number,
			program_map_PID
		);
		for (const auto& stream : streams) {
			_UTL_LOG_I (
				"    stream type:[0x%02x] pid:[0x%04x]",
				stream->type,
				stream->pid
			);
		}
	}

} _program_info_t;

typedef struct _event_pf_info {
public:
	_event_pf_info (void) {
		clear();
	}
	virtual ~_event_pf_info (void) {
		clear();
	}

	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	arec::common::etime start_time;
	arec::common::etime end_time;

	char event_name_char [MAXSECLEN];

	event_pf_state state;

	parser::event_information_table::table* p_org_table;

	bool is_used;

	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();
		memset (event_name_char, 0x00, sizeof(event_name_char));
		state = event_pf_state::init;
		p_org_table = NULL;
		is_used = false;
	}

	void dump (void) const{
		if (p_org_table) {
			p_org_table->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"  p/f:[%s] time:[%s - %s]",
			gpsz_event_pf_state [static_cast<int>(state)],
			start_time.to_string(),
			end_time.to_string()
		);
		_UTL_LOG_I ("  event_name:[%s]", event_name_char);
	}

} _event_pf_info_t;

typedef struct _service_info {
public:
	_service_info (void) {
		clear ();
	}
	virtual ~_service_info (void) {
		clear ();
	}

	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t service_type;
	char service_name_char [64];

	// -- clear each tuning
	bool is_tune_target;
	_event_pf_info_t event_follow_info;
	// --

	arec::common::etime last_update;

	parser::service_description_table::table* p_org_table;

	bool is_used;

	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		service_type = 0;
		memset (service_name_char, 0x00, sizeof(service_name_char))	;
		is_tune_target = false;
		event_follow_info.clear();
		last_update.clear();
		p_org_table = NULL;
		is_used = false;
	}

	void clear_at_tuning (void) {
		is_tune_target = false;
		event_follow_info.clear();
	}

	void dump (void) {
		if (p_org_table) {
			p_org_table->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] svctype:[0x%02x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			service_type
		);
		_UTL_LOG_I (
			"  %s service_name:[%s] last_update:[%s]",
			is_tune_target ? "*" : " ",
			service_name_char,
			last_update.to_string()
		);
		if (event_follow_info.is_used) {
			event_follow_info.dump();
		}
	}

} _service_info_t;

typedef struct _network_info {
public:
	_network_info (void) {
		clear ();
	}
	virtual ~_network_info (void) {
		clear ();
	}

	uint8_t table_id;
	uint16_t network_id;

	// parser::network_name_descriptor
	char network_name_char [64];


	//----- transport_stream_loop -----

	uint16_t transport_stream_id;
	uint16_t original_network_id;

	// parser::service_list_descriptor
	struct service {
		uint16_t service_id;
		uint8_t service_type;
	} services [16];
	int services_num;

	// parser::terrestrial_delivery_system_descriptor
	uint16_t area_code;
	uint8_t guard_interval;
	uint8_t transmission_mode;

	// parser::ts_information_descriptor
	uint8_t remote_control_key_id;
	char ts_name_char  [64];

	//----- transport_stream_loop ------


	parser::network_information_table::table* p_org_table;

	bool is_used;

	void clear (void) {
		table_id = 0;
		network_id = 0;
		memset (network_name_char, 0x00, sizeof(network_name_char));
		transport_stream_id = 0;
		original_network_id = 0;
		memset (services, 0x00, sizeof(services));
		services_num = 0;
		area_code = 0;
		guard_interval = 0;
		transmission_mode = 0;
		remote_control_key_id = 0;
		memset (ts_name_char, 0x00, sizeof(ts_name_char));
		p_org_table = NULL;
		is_used = false;
	}

	void dump (void) {
		if (p_org_table) {
			p_org_table->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id
		);
		_UTL_LOG_I ("  network_name:[%s]", network_name_char);
		_UTL_LOG_I ("  ----------");
		for (int i = 0; i < services_num; ++ i) {
			_UTL_LOG_I (
				"  svcid:[0x%04x] svctype:[0x%02x]",
				services[i].service_id,
				services[i].service_type
			);
		}
		_UTL_LOG_I ("  ----------");
		_UTL_LOG_I (
			"  area_code:[0x%04x] guard:[0x%02x] trans_mode:[0x%02x]",
			area_code,
			guard_interval,
			transmission_mode
		);
		_UTL_LOG_I (
			"  remote_control_key_id:[0x%04x] ts_name:[%s]",
			remote_control_key_id,
			ts_name_char
		);
	}

} _network_info_t;

typedef struct _section_comp_flag {
public:
	_section_comp_flag (void) {
		clear ();
	}
	virtual ~_section_comp_flag (void) {
		clear ();
	}

	void check_update (bool is_new_section) {
		if (is_new_section) {
			if (!flag_) {
				flag_ = 1;
			}
		} else {
			if (flag_ == 1) {
				flag_ = 2;
			}
		}
	}

	bool is_completed (void) {
		return (flag_ == 2) ;
	}

	void clear (void) {
		flag_ = 0;
	}

private:
	int flag_;

} section_comp_flag_t ;


class manager
	:public coseq::module_base
	,public arec::group
	,public arec::tuner_control::client_iface::ITsReceiveHandler
	,public parser::ts_parser::IParserListener
	,public parser::event_information_table_sched::IEventScheduleHandler
{
public:
	manager (std::string name, uint8_t que_max, uint8_t group_id);
	virtual ~manager (void);



	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_get_state (coseq::iface *iface);
	void on_check_loop (coseq::iface *iface);
	void on_parser_notice (coseq::iface *iface);
	void on_stabilization_after_tuning (coseq::iface *iface);
	void on_register_pat_detect_notify (coseq::iface *iface);
	void on_unregister_pat_detect_notify (coseq::iface *iface);
	void on_register_event_change_notify (coseq::iface *iface);
	void on_unregister_event_change_notify (coseq::iface *iface);
	void on_register_psisi_state_notify (coseq::iface *iface);
	void on_unregister_psisi_state_notify (coseq::iface *iface);
	void on_get_pat_detect_state (coseq::iface *iface);
	void on_get_stream_infos (coseq::iface *iface);
	void on_get_current_service_infos (coseq::iface *iface);
	void on_get_present_event_info (coseq::iface *iface);
	void on_get_follow_event_info (coseq::iface *iface);
	void on_get_current_network_info (coseq::iface *iface);
	void on_enable_parse_eit_sched (coseq::iface *iface);
	void on_disable_parse_eit_sched (coseq::iface *iface);
	void on_dump_caches (coseq::iface *iface);
	void on_dump_tables (coseq::iface *iface);

	void on_receive_notify (coseq::iface *iface) override;


private:
	//-- program_info --
	void cache_program_infos (void);
	void dump_program_infos (void);
	void clear_program_infos (void);

	// for request
	int get_stream_infos (uint16_t program_map_PID, parser::EN_STREAM_TYPE type, psisi_structs::stream_info_t *p_out_stream_infos, int num);


	//-- service_info --
	void cache_service_infos (bool is_at_tuning);
	_service_info_t* find_service_info (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	_service_info_t* find_empty_service_info (void);
	bool is_exist_service_table (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	void assign_follow_event_to_service_infos (void);
	void check_follow_event_at_service_infos (coseq::iface *iface);
	void dump_service_infos (void);
	void clear_service_infos (void);
	void clear_service_infos (bool is_at_tuning);

	// for request
	int get_current_service_infos (psisi_structs::service_info_t *p_out_service_infos, int num);


	//-- event_pf_info --
	void cache_event_pf_infos (void);
	bool cache_event_pf_infos (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	_event_pf_info_t* find_empty_event_pf_info (void);
	void check_event_pf_infos (void);
	void refresh_event_pf_infos (void);
	void dump_event_pf_infos (void);
	void clear_event_pf_info (_event_pf_info_t *p_info);
	void clear_event_pf_infos (void);

	// for request
	_event_pf_info_t* find_event_pf_info (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		event_pf_state state
	);


	//-- network_info --
	void cache_network_info (void);
	void dump_network_info (void);
	void clear_network_info (void);


	// -- logo --
	void store_logo (void);


	// arec::tuner_control::client_iface::ITsReceiveHandler
	bool on_pre_ts_receive (void) override;
	void on_post_ts_receive (void) override;
	bool on_check_ts_receive_loop (void) override;
	bool on_ts_received (void *p_ts_data, int length) override;

	// parser::ts_parser::IParserListener
	bool on_ts_packet_available (parser::TS_HEADER *p_header, uint8_t *p_payload, size_t payload_size) override;

	// parser::event_information_table_sched::IEventSchedleHandler
	void on_schedule_update (void) override;


	arec::settings *settings_;

	arec::tuner_control::client_iface::ITsReceiveHandler *ts_handler_;

	parser::ts_parser parser_;

	uint8_t tuner_notify_client_id_;
	int ts_receive_handler_id_;

	// tuner is tuned 
	bool tuner_is_tuned_ ;

	bool is_detected_PAT_;

	client_iface::psisi_state state_;

	parser::program_association_table mPAT;
	parser::event_information_table mEIT_H;
	parser::network_information_table mNIT;
	parser::service_description_table mSDT;
	parser::conditional_access_table mCAT;
	parser::common_data_table mCDT;

	program_map program_map_;

//	parser::program_association_table::reference mPAT_ref;
//	parser::event_information_table::reference mEIT_H_ref;
//	parser::network_information_table::reference mNIT_ref;
//	parser::service_description_table::reference mSDT_ref;

	arec::common::etime pat_recv_time_;

	_program_info_t program_infos_ [PROGRAM_INFOS_MAX];
	_service_info_t service_infos_ [SERVICE_INFOS_MAX];
	_event_pf_info_t event_pf_infos_ [EVENT_PF_INFOS_MAX];
	_network_info_t network_info_ ;

	section_comp_flag_t EIT_H_comp_flag_;
	section_comp_flag_t SDT_comp_flag_;
	section_comp_flag_t NIT_comp_flag_;


	// for EIT schedule
	parser::event_information_table_sched mEIT_H_sched;
	bool is_enable_EIT_sched_;

};

} // namespace psisi
} // namespace arec

#endif
