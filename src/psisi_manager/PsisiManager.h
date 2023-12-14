#ifndef _PSISI_MANAGER_H_
#define _PSISI_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <memory>
#include <mutex>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Group.h"
#include "Settings.h"
#include "PsisiManagerIf.h"

#include "TunerControlIf.h"
#include "TsParser.h"
#include "EventScheduleManagerIf.h"

#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "EventInformationTable.h"
#include "EventInformationTable_sched.h"
#include "NetworkInformationTable.h"
#include "ServiceDescriptionTable.h"
#include "RunningStatusTable.h"
#include "BroadcasterInformationTable.h"
#include "TimeOffsetTable.h"
#include "ConditionalAccessTable.h"
#include "CommonDataTable.h"


// notify category
#define NOTIFY_CAT__PAT_DETECT			((uint8_t)0)
#define NOTIFY_CAT__EVENT_CHANGE		((uint8_t)1)
#define NOTIFY_CAT__PSISI_STATE		 	((uint8_t)2)


#define PROGRAM_INFOS_MAX			(32)
#define SERVICE_INFOS_MAX			(64)
#define EVENT_PF_INFOS_MAX			(32)

#define TMP_PROGRAM_MAPS_MAX		(16)


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


class CProgramMap {
public:
	CProgramMap (void) {
		clear();
	}
	virtual ~CProgramMap (void) {
		clear();
	}

	std::shared_ptr <CProgramMapTable> get (uint16_t pid) {
		// m_parsersは確定しているので排他いれません

		const auto it = m_parsers.find(pid);
		if (it != m_parsers.end()) {
			return it->second;
		} else {
			return nullptr;
		}
	}

	void add (uint16_t pid) {
		// tuner threadとの排他します
		std::lock_guard <std::mutex> lock(m_mutex);

		m_parsers [pid] = std::make_shared<CProgramMapTable>();
	}

	EN_CHECK_SECTION parse (TS_HEADER *ts_header, uint8_t* p_payload, size_t payload_size) {
		// m_parsersは確定しているので排他いれません

		const auto it = m_parsers.find(ts_header->pid);
		if (it != m_parsers.end()) {
			 return it->second->checkSection (ts_header, p_payload, payload_size);
		} else {
			return EN_CHECK_SECTION__INVALID;
		}
	}

	bool has (uint16_t pid) {
		// tuner thread側で使うので排他します
		std::lock_guard <std::mutex> lock(m_mutex);

		const auto &it = m_parsers.find(pid);
		if (it != m_parsers.end()) {
			return true;
		} else {
			return false;
		}
	}

	void dump (void) {
		for (const auto &parser : m_parsers) {
			parser.second->dumpTables();
		}
	}

	void clear (void) {
		// tuner threadとの排他します
		std::lock_guard <std::mutex> lock(m_mutex);

		for (const auto &parser : m_parsers) {
			parser.second->forceClear();
		}
		m_parsers.clear();
	}

private:
	std::unordered_map<uint16_t, std::shared_ptr<CProgramMapTable>> m_parsers; // <pid, pmt parser>
	std::mutex m_mutex; // tuner thread との排他のため
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

	CProgramAssociationTable::CTable* p_org_table;

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
	CEtime start_time;
	CEtime end_time;

	char event_name_char [MAXSECLEN];

	event_pf_state state;

	CEventInformationTable::CTable* p_org_table;

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
			start_time.toString(),
			end_time.toString()
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

	CEtime last_update;

	CServiceDescriptionTable::CTable* p_org_table;

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
			last_update.toString()
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

	// CNetworkNameDescriptor
	char network_name_char [64];


	//----- transport_stream_loop -----

	uint16_t transport_stream_id;
	uint16_t original_network_id;

	// CServiceListDescriptor
	struct service {
		uint16_t service_id;
		uint8_t service_type;
	} services [16];
	int services_num;

	// CTerrestrialDeliverySystemDescriptor
	uint16_t area_code;
	uint8_t guard_interval;
	uint8_t transmission_mode;

	// CTSInformationDescriptor
	uint8_t remote_control_key_id;
	char ts_name_char  [64];

	//----- transport_stream_loop ------


	CNetworkInformationTable::CTable* p_org_table;

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
			if (!m_flag) {
				m_flag = 1;
			}
		} else {
			if (m_flag == 1) {
				m_flag = 2;
			}
		}
	}

	bool is_completed (void) {
		return (m_flag == 2) ;
	}

	void clear (void) {
		m_flag = 0;
	}

private:
	int m_flag;

} section_comp_flag_t ;


class CPsisiManager
	:public threadmgr::CThreadMgrBase
	,public CGroup
	,public CTunerControlIf::ITsReceiveHandler
	,public CTsParser::IParserListener
	,public CEventInformationTable_sched::IEventScheduleHandler
{
public:
	CPsisiManager (std::string name, uint8_t que_max, uint8_t group_id);
	virtual ~CPsisiManager (void);


	void on_create (void) override;

	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);
	void on_get_state (threadmgr::CThreadMgrIf *p_if);
	void on_check_loop (threadmgr::CThreadMgrIf *p_if);
	void on_parser_notice (threadmgr::CThreadMgrIf *p_if);
	void on_stabilization_after_tuning (threadmgr::CThreadMgrIf *p_if);
	void on_register_pat_detect_notify (threadmgr::CThreadMgrIf *p_if);
	void on_unregister_pat_detect_notify (threadmgr::CThreadMgrIf *p_if);
	void on_register_event_change_notify (threadmgr::CThreadMgrIf *p_if);
	void on_unregister_event_change_notify (threadmgr::CThreadMgrIf *p_if);
	void on_register_psisi_state_notify (threadmgr::CThreadMgrIf *p_if);
	void on_unregister_psisi_state_notify (threadmgr::CThreadMgrIf *p_if);
	void on_get_pat_detect_state (threadmgr::CThreadMgrIf *p_if);
	void on_get_stream_infos (threadmgr::CThreadMgrIf *p_if);
	void on_get_current_service_infos (threadmgr::CThreadMgrIf *p_if);
	void on_get_present_event_info (threadmgr::CThreadMgrIf *p_if);
	void on_get_follow_event_info (threadmgr::CThreadMgrIf *p_if);
	void on_get_current_network_info (threadmgr::CThreadMgrIf *p_if);
	void on_enable_parse_eit_sched (threadmgr::CThreadMgrIf *p_if);
	void on_disable_parse_eit_sched (threadmgr::CThreadMgrIf *p_if);
	void on_dump_caches (threadmgr::CThreadMgrIf *p_if);
	void on_dump_tables (threadmgr::CThreadMgrIf *p_if);

	void on_receive_notify (threadmgr::CThreadMgrIf *p_if) override;


private:
	//-- program_info --
	void cache_program_infos (void);
	void dump_program_infos (void);
	void clear_program_infos (void);

	// for request
	int get_stream_infos (uint16_t program_map_PID, EN_STREAM_TYPE type, psisi_structs::stream_info_t *p_out_stream_infos, int num);


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
	void check_follow_event_at_service_infos (threadmgr::CThreadMgrIf *p_if);
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


	// CTunerControlIf::ITsReceiveHandler
	bool on_pre_ts_receive (void) override;
	void on_post_ts_receive (void) override;
	bool on_check_ts_receive_loop (void) override;
	bool on_ts_received (void *p_ts_data, int length) override;

	// CTsParser::IParserListener
	bool on_ts_packet_available (TS_HEADER *p_header, uint8_t *p_payload, size_t payload_size) override;

	// CEventInformationTable_sched::IEventSchedleHandler
	void onScheduleUpdate (void) override;


	CSettings *mp_settings;

	CTunerControlIf::ITsReceiveHandler *mp_ts_handler;

	CTsParser m_parser;

	uint8_t m_tuner_notify_client_id;
	int m_ts_receive_handler_id;

	// tuner is tuned 
	bool m_tuner_is_tuned ;

	bool m_is_detected_PAT;

	CPsisiManagerIf::psisi_state m_state;

	CProgramAssociationTable mPAT;
	CEventInformationTable mEIT_H;
	CNetworkInformationTable mNIT;
	CServiceDescriptionTable mSDT;
	CConditionalAccessTable mCAT;
	CCommonDataTable mCDT;

	CProgramMap m_program_map;

//	CProgramAssociationTable::CReference mPAT_ref;
//	CEventInformationTable::CReference mEIT_H_ref;
//	CNetworkInformationTable::CReference mNIT_ref;
//	CServiceDescriptionTable::CReference mSDT_ref;

	CEtime m_pat_recv_time;

	_program_info_t m_program_infos [PROGRAM_INFOS_MAX];
	_service_info_t m_service_infos [SERVICE_INFOS_MAX];
	_event_pf_info_t m_event_pf_infos [EVENT_PF_INFOS_MAX];
	_network_info_t m_network_info ;

	section_comp_flag_t m_EIT_H_comp_flag;
	section_comp_flag_t m_SDT_comp_flag;
	section_comp_flag_t m_NIT_comp_flag;


	// for EIT schedule
	CEventInformationTable_sched mEIT_H_sched;
	bool m_is_enable_EIT_sched;

};

#endif
