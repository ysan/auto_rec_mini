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

#include "coseqpp.h"

#include "settings.h"
#include "utils.h"
#include "event_schedule_manager_if.h"
#include "ts_arib_common.h"

#include "tuner_service_if.h"
#include "psisi_manager_if.h"
#include "channel_manager_if.h"

#include "psisi_manager_structs_addition.h"
#include "event_information_table_sched.h"

#include "event.h"
#include "event_schedule_container.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/vector.hpp"


// notify category
#define NOTIFY_CAT__CACHE_SCHEDULE			((uint8_t)0)


namespace arec {
namespace event_schedule {

class reserve {
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

		reserve (void) {
			clear ();
		}

		reserve (
			uint16_t _transport_stream_id,
			uint16_t _original_network_id,
			uint16_t _service_id,
			arec::common::etime *p_start_time,
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

		virtual ~reserve (void) {
			clear ();
		}

		bool operator == (const reserve &obj) const {
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

		bool operator != (const reserve &obj) const {
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
				start_time.to_string(),
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
		arec::common::etime start_time;
		type_t type;
	};

	class history {
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


		history (void) {
			clear();
		}
		virtual ~history (void) {
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
			start_time.set_current_time();
		}

		void set_end_time (void) {
			end_time.set_current_time();
		}

		void dump (void) const {
			std::vector<struct stream>::const_iterator iter = streams.begin();
			for (; iter != streams.end(); ++ iter) {
				iter->dump();
			}
			_UTL_LOG_I (
				"\n *** start - end: [%s - %s] ***\n\n",
				start_time.to_string(),
				end_time.to_string()
			);
		}

//	private:
	// cereal 非侵入型対応のため やむなくpublicに
		std::vector <struct stream> streams;
		arec::common::etime start_time;
		arec::common::etime end_time;
	};


class manager
	:public coseq::module_base
{
public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_register_cache_schedule_state_notify (coseq::iface *iface);
	void on_unregister_cache_schedule_state_notify (coseq::iface *iface);
	void on_get_cache_schedule_state (coseq::iface *iface);
	void on_check_loop (coseq::iface *iface);
	void on_parser_notice (coseq::iface *iface);
	void on_exec_cache_schedule (coseq::iface *iface);
	void on_cache_schedule (coseq::iface *iface);
	void on_cache_schedule_force_current_service (coseq::iface *iface);
	void on_stop_cache_schedule (coseq::iface *iface);
	void on_get_event (coseq::iface *iface);
	void on_get_event_latest_dumped_schedule (coseq::iface *iface);
	void on_dump_event_latest_dumped_schedule (coseq::iface *iface);
	void on_get_events_keyword_search (coseq::iface *iface);
	void on_add_reserves (coseq::iface *iface);
	void on_dump_schedule_map (coseq::iface *iface);
	void on_dump_schedule (coseq::iface *iface);
	void on_dump_reserves (coseq::iface *iface);
	void on_dump_histories (coseq::iface *iface);


	void on_receive_notify (coseq::iface *iface) override;


private:
	void cache_schedule (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		std::vector <event*> *p_out_sched
	);
	void cache_schedule_extended (std::vector <event*> *p_out_sched);
	void cache_schedule_extended (event* p_out_event);


	bool add_reserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		arec::common::etime * p_start_time,
		reserve::type_t _type
	);
	bool add_reserve (const reserve &rsv);
	bool remove_reserve (const reserve &rsv);
	bool is_duplicate_reserve (const reserve& rsv) const;
	void check2execute_reserves (coseq::iface *iface) ;
	bool is_exist_reserve (const reserve& rsv) const;
	void dump_reserves (void) const;


	void push_histories (const history *p_history);
	void dump_histories (void) const;

	void save_histories (void) ;
	void load_histories (void) ;


	arec::settings *settings_;

	client_iface::cache_schedule_state state_;

	uint8_t tuner_notify_client_id_;
	uint8_t event_change_notify_client_id_;

	parser::event_information_table_sched *EIT_H_sched_;
	parser::event_information_table_sched::reference EIT_H_sched_ref_;


	arec::common::etime last_update_EIT_sched_;
	arec::common::etime start_time_EIT_sched_;


	service_key_t latest_dumped_key_;


	arec::common::etime schedule_cache_next_day_;
	arec::common::etime schedule_cache_current_plan_;


	std::vector <reserve> reserves_;
	std::vector <reserve> retry_reserves_;
	reserve executing_reserve_;

	bool is_need_stop_;

	history current_history_;
	history::stream current_history_stream_;
	std::vector <history> histories_;

	container container_;

	uint8_t force_cache_group_id_;

	int tuner_resource_max_;
};

} // namespace event_schedule
} // namespace arec

#endif
