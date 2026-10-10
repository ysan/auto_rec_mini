#ifndef _EVENT_SEARCH_H_
#define _EVENT_SEARCH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "coseqpp.h"

#include "utils.h"
#include "ts_arib_common.h"
#include "event_search_if.h"

#include "rec_manager_if.h"
#include "event_schedule_manager_if.h"

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


namespace arec {
namespace event_search {

class history {
public:
	struct event {
		event (void) {
			clear();
		}
		virtual ~event (void) {
			clear();
		}

		void clear (void) {
			transport_stream_id = 0;
			original_network_id = 0;
			service_id = 0;
			event_id = 0;
			start_time.clear();
			end_time.clear();
			event_name.clear();
		}

		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;
		uint16_t event_id;
		arec::common::etime start_time;
		arec::common::etime end_time;
		std::string event_name;
	};

	struct keyword {
		keyword (void) {
			clear();
		}
		virtual ~keyword (void) {
			clear();
		}

		void clear (void) {
			keyword_string.clear();
			events.clear();
		}

		void dump (void) const {
			_UTL_LOG_I ("  keyword:[%s]", keyword_string.c_str());
			if (events.size() == 0) {
				_UTL_LOG_I ("    none.");
			} else {
				for (const auto & event : events) {
					_UTL_LOG_I (
						"    tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x] [%s - %s] [%s]",
						event.transport_stream_id,
						event.original_network_id,
						event.service_id,
						event.event_id,
						event.start_time.to_string(),
						event.end_time.to_string(),
						event.event_name.c_str()
					);
				}
			}
		}

		std::string keyword_string;
		std::vector <struct event> events;
	};


	history (void) {
		clear();
	}
	virtual ~history (void) {
		clear();
	}

	void clear (void) {
		keywords.clear ();
		timestamp.clear();
	}

	void dump (void) const {
		_UTL_LOG_I (" timestamp:[%s]", timestamp.to_string());
		for (const auto & keyword : keywords) {
			keyword.dump();
		}
	}

//	private:
	// cereal 非侵入型対応のため やむなくpublicに
	std::vector <struct keyword> keywords;
	arec::common::etime timestamp;
};


class manager : public coseq::module_base
{
public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_add_rec_reserve_keyword_search (coseq::iface *iface);
	void on_dump_histories (coseq::iface *iface);

	void on_receive_notify (coseq::iface *iface) override;


private:
	void dump_event_name_keywords (void) const;
	void dump_extended_event_keywords (void) const;

	void push_histories (const history &h, std::vector<history> &histories);
	void dump_histories (const std::vector<history> &histories) const;

	void save_event_name_keywords (void);
	void load_event_name_keywords (void);

	void save_extended_event_keywords (void);
	void load_extended_event_keywords (void);

	void save_event_name_search_histories (void);
	void load_event_name_search_histories (void);

	void save_extended_event_search_histories (void);
	void load_extended_event_search_histories (void);


	uint8_t cache_sched_state_notify_client_id_;
	std::vector <std::string> event_name_keywords_;
	std::vector <std::string> extended_event_keywords_;

	std::vector <history> event_name_search_histories_;
	std::vector <history> extended_event_search_histories_;
};

} // namespace event_search
} // namespace arec

#endif
