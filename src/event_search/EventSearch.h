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

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "TsAribCommon.h"
#include "EventSearchIf.h"

#include "RecManagerIf.h"
#include "EventScheduleManagerIf.h"

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"
#include "threadmgr_if.h"


class CEventSearch : public threadmgr::CThreadMgrBase
{
public:
	class CHistory {
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
			CEtime start_time;
			CEtime end_time;
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
							event.start_time.toString(),
							event.end_time.toString(),
							event.event_name.c_str()
						);
					}
				}
			}

			std::string keyword_string;
			std::vector <struct event> events;
		};


		CHistory (void) {
			clear();
		}
		virtual ~CHistory (void) {
			clear();
		}

		void clear (void) {
			keywords.clear ();
			timestamp.clear();
		}

		void dump (void) const {
			_UTL_LOG_I (" timestamp:[%s]", timestamp.toString());
			for (const auto & keyword : keywords) {
				keyword.dump();
			}
		}

//	private:
	// cereal 非侵入型対応のため やむなくpublicに
		std::vector <struct keyword> keywords;
		CEtime timestamp;
	};

public:
	CEventSearch (std::string name, uint8_t que_max);
	virtual ~CEventSearch (void);


	void on_destroy (void) override;

	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);
	void on_add_rec_reserve_keyword_search (threadmgr::CThreadMgrIf *p_if);
	void on_dump_histories (threadmgr::CThreadMgrIf *p_if);

	void on_receive_notify (threadmgr::CThreadMgrIf *p_if) override;


private:
	void dump_event_name_keywords (void) const;
	void dump_extended_event_keywords (void) const;

	void push_histories (const CHistory &history, std::vector<CHistory> &histories);
	void dump_histories (const std::vector<CHistory> &histories) const;

	void save_event_name_keywords (void);
	void load_event_name_keywords (void);

	void save_extended_event_keywords (void);
	void load_extended_event_keywords (void);

	void save_event_name_search_histories (void);
	void load_event_name_search_histories (void);

	void save_extended_event_search_histories (void);
	void load_extended_event_search_histories (void);


	uint8_t m_cache_sched_state_notify_client_id;
	std::vector <std::string> m_event_name_keywords;
	std::vector <std::string> m_extended_event_keywords;

	std::vector <CHistory> m_event_name_search_histories;
	std::vector <CHistory> m_extended_event_search_histories;
	CHistory m_current_history;
	CHistory::keyword m_current_history_keyword;
};

#endif
