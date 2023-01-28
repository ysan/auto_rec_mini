#ifndef _EVENT_SCHEDULE_MANAGER_H_
#define _EVENT_SCHEDULE_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <map>
#include <string>
#include <queue>

#include "ThreadMgrpp.h"

#include "Settings.h"
#include "Utils.h"
#include "EventScheduleManagerIf.h"
#include "TsAribCommon.h"

#include "TunerServiceIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"

#include "PsisiManagerStructsAddition.h"
#include "EventInformationTable_sched.h"

#include "Event.h"
#include "EventScheduleContainer.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/vector.hpp"


// notify category
#define NOTIFY_CAT__CACHE_SCHEDULE			((uint8_t)0)


class CEventScheduleManager
	:public threadmgr::CThreadMgrBase
{
public:
	class CReserve {
	public:
		typedef enum {
			INIT      = 0x0000,
			NORMAL    = 0x0001,
			FORCE     = 0x0002,

			TYPE_MASK = 0x00ff,

			S_FLG     = 0x0100, // start flag
			E_FLG     = 0x0200, // end flag
			N_FLG     = 0x0400, // need notify flag
			R_FLG     = 0x0800, // need retry flag
		} type_t;

		CReserve (void) {
			clear ();
		}

		CReserve (
			uint16_t _transport_stream_id,
			uint16_t _original_network_id,
			uint16_t _service_id,
			CEtime *p_start_time,
			type_t _type
		) {
			clear ();
			transport_stream_id = _transport_stream_id;
			original_network_id = _original_network_id;
			service_id = _service_id;
			if (p_start_time) {
				start_time = *p_start_time;
			}
			type = _type;
		}

		virtual ~CReserve (void) {
			clear ();
		}

		bool operator == (const CReserve &obj) const {
			if (
				this->transport_stream_id == obj.transport_stream_id &&
				this->original_network_id == obj.original_network_id &&
				this->service_id == obj.service_id &&
				this->start_time == obj.start_time &&
				this->type == obj.type
			) {
				return true;
			} else {
				return false;
			}
		}

		bool operator != (const CReserve &obj) const {
			if (
				this->transport_stream_id != obj.transport_stream_id ||
				this->original_network_id != obj.original_network_id ||
				this->service_id != obj.service_id ||
				this->start_time != obj.start_time ||
				this->type != obj.type
			) {
				return true;
			} else {
				return false;
			}
		}

		bool is_valid (void) {
			if (transport_stream_id && original_network_id && service_id) {
				return true;
			} else {
				return false;
			}
		}

		void clear (void) {
			transport_stream_id = 0;
			original_network_id = 0;
			service_id = 0;
			start_time.clear ();
			type = INIT;
		}

		void dump (void) const {
			_UTL_LOG_I ("tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] start_time:[%s] type:[%s%s%s%s%s]",
				transport_stream_id,
				original_network_id,
				service_id,
				start_time.toString(),
				(type & TYPE_MASK) == INIT ? "INIT" :
					(type & TYPE_MASK) == NORMAL ? "NORMAL" :
						(type & TYPE_MASK) == FORCE ? "FORCE" : "???",
				type & S_FLG ? ",S" : "",
				type & E_FLG ? ",E" : "",
				type & N_FLG ? ",N" : "",
				type & R_FLG ? ",R" : ""
			);
		}

		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
		CEtime start_time;
		type_t type;
	};

	class CHistory {
	public:
		enum state {
			EN_STATE__INIT = 0,
			EN_STATE__COMPLETE,
			EN_STATE__TIMEOUT,
			EN_STATE__CANCEL,
			EN_STATE__ERROR,
		};

		struct service {
			service (void) {
				clear();
			}
			virtual ~service (void) {
				clear();
			}

			void clear (void) {
				key.clear();
				item_num = 0;
			}

			void dump (void) const {
//				key.dump();
				_UTL_LOG_I (
					"#_service_key#  0x%04x 0x%04x 0x%04x  (svctype:0x%02x,%s)  item_num=[%d]",
					key.transport_stream_id,
					key.original_network_id,
					key.service_id,
					key.service_type,
					key.service_name.c_str(),
					item_num
				);
			}

			service_key_t key;
			int item_num;
		};

		struct stream {
			stream (void) {
				clear();
			}
			virtual ~stream (void) {
				clear();
			}

			void clear (void) {
				stream_name.clear();
				_state = EN_STATE__INIT;
				services.clear();
//				services.shrink_to_fit();
			}

			void dump (void) const {
				_UTL_LOG_I ("transport_stream_name:[%s] ==> [%s]",
					stream_name.c_str(),
					_state == EN_STATE__INIT ? "INIT" :
						_state == EN_STATE__COMPLETE ? "COMPLETE" :
							_state == EN_STATE__TIMEOUT ? "TIMEOUT" :
								_state == EN_STATE__CANCEL ? "CANCEL" :
									_state == EN_STATE__ERROR ? "ERROR" : "???"
				);
				std::vector<struct service>::const_iterator iter = services.begin();
				for (; iter != services.end(); ++ iter) {
					iter->dump();
				}
			}

			std::string stream_name;
			state _state;
			std::vector <struct service> services;
		};


		CHistory (void) {
			clear();
		}
		virtual ~CHistory (void) {
			clear();
		}

		void add (struct stream &s) {
			streams.push_back (s);
		}

		void clear (void) {
			streams.clear ();
//			streams.shrink_to_fit ();
			start_time.clear();
			end_time.clear();
		}

		void set_start_time (void) {
			start_time.setCurrentTime();
		}

		void set_end_time (void) {
			end_time.setCurrentTime();
		}

		void dump (void) const {
			std::vector<struct stream>::const_iterator iter = streams.begin();
			for (; iter != streams.end(); ++ iter) {
				iter->dump();
			}
			_UTL_LOG_I (
				"\n *** start - end: [%s - %s] ***\n\n",
				start_time.toString(),
				end_time.toString()
			);
		}

//	private:
	// cereal 非侵入型対応のため やむなくpublicに
		std::vector <struct stream> streams;
		CEtime start_time;
		CEtime end_time;
	};

public:
	CEventScheduleManager (std::string name, uint8_t que_max);
	virtual ~CEventScheduleManager (void);


	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);
	void on_register_cache_schedule_state_notify (threadmgr::CThreadMgrIf *p_if);
	void on_unregister_cache_schedule_state_notify (threadmgr::CThreadMgrIf *p_if);
	void on_get_cache_schedule_state (threadmgr::CThreadMgrIf *p_if);
	void on_check_loop (threadmgr::CThreadMgrIf *p_if);
	void on_parser_notice (threadmgr::CThreadMgrIf *p_if);
	void on_exec_cache_schedule (threadmgr::CThreadMgrIf *p_if);
	void on_cache_schedule (threadmgr::CThreadMgrIf *p_if);
	void on_cache_schedule_force_current_service (threadmgr::CThreadMgrIf *p_if);
	void on_stop_cache_schedule (threadmgr::CThreadMgrIf *p_if);
	void on_get_event (threadmgr::CThreadMgrIf *p_if);
	void on_get_event_latest_dumped_schedule (threadmgr::CThreadMgrIf *p_if);
	void on_dump_event_latest_dumped_schedule (threadmgr::CThreadMgrIf *p_if);
	void on_get_events_keyword_search (threadmgr::CThreadMgrIf *p_if);
	void on_add_reserves (threadmgr::CThreadMgrIf *p_if);
	void on_dump_schedule_map (threadmgr::CThreadMgrIf *p_if);
	void on_dump_schedule (threadmgr::CThreadMgrIf *p_if);
	void on_dump_reserves (threadmgr::CThreadMgrIf *p_if);
	void on_dump_histories (threadmgr::CThreadMgrIf *p_if);


	void on_receive_notify (threadmgr::CThreadMgrIf *p_if) override;


private:
	void cache_schedule (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		std::vector <CEvent*> *p_out_sched
	);
	void cache_schedule_extended (std::vector <CEvent*> *p_out_sched);
	void cache_schedule_extended (CEvent* p_out_event);


	bool add_reserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		CEtime * p_start_time,
		CReserve::type_t _type
	);
	bool add_reserve (const CReserve &reserve);
	bool remove_reserve (const CReserve &reserve);
	bool is_duplicate_reserve (const CReserve& reserve) const;
	void check2execute_reserves (void) ;
	bool is_exist_reserve (const CReserve& reserve) const;
	void dump_reserves (void) const;


	void push_histories (const CHistory *p_history);
	void dump_histories (void) const;

	void save_histories (void) ;
	void load_histories (void) ;


	CSettings *mp_settings;

	CEventScheduleManagerIf::cache_schedule_state m_state;

	uint8_t m_tuner_notify_client_id;
	uint8_t m_event_change_notify_client_id;

	CEventInformationTable_sched *mp_EIT_H_sched;
	CEventInformationTable_sched::CReference m_EIT_H_sched_ref;


	CEtime m_last_update_EIT_sched;
	CEtime m_start_time_EIT_sched;


	service_key_t m_latest_dumped_key;


	CEtime m_schedule_cache_next_day;
	CEtime m_schedule_cache_current_plan;


	std::vector <CReserve> m_reserves;
	std::vector <CReserve> m_retry_reserves;
	CReserve m_executing_reserve;

	bool m_is_need_stop;

	CHistory m_current_history;
	CHistory::stream m_current_history_stream;
	std::vector <CHistory> m_histories;

	CEventScheduleContainer m_container;

	uint8_t m_force_cache_group_id;

	int m_tuner_resource_max;
};

#endif
