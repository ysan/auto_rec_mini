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

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"

#include "ThreadMgrBase.h"
#include "ThreadMgr.h"

#include "Settings.h"
#include "Utils.h"
#include "EventScheduleManagerIf.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"

#include "PsisiManagerStructsAddition.h"
#include "EventInformationTable_sched.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/vector.hpp"


using namespace ThreadManager;


// notify category
#define NOTIFY_CAT__CACHE_SCHEDULE			((uint8_t)0)


typedef struct _service_key {
public:
	_service_key (void) {
		clear ();
    }

	_service_key (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	) {
		clear ();
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
    }

	_service_key (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint8_t _service_type,
		const char* _p_service_name_char
	) {
		clear ();
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->service_type = _service_type;
		this->service_name = _p_service_name_char;
    }

	explicit _service_key (CEventScheduleManagerIf::SERVICE_KEY_t &_key) {
		clear ();
		this->transport_stream_id = _key.transport_stream_id;
		this->original_network_id = _key.original_network_id;
		this->service_id = _key.service_id;
    }

	virtual ~_service_key (void) {
		clear ();
	}


	bool operator == (const _service_key &obj) const {
		if (
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id &&
			this->service_id == obj.service_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator != (const _service_key &obj) const {
		if (
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id ||
			this->service_id != obj.service_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator < (const _service_key &obj) const {
		// map の二分木探査用
		// networkが一番優先高く 次に内包されるts_id
		// 更に内包される serviceという観点で比較します

		if (this->original_network_id < obj.original_network_id) {
			return true;
		} else if (this->original_network_id == obj.original_network_id) {
			if (this->transport_stream_id < obj.transport_stream_id) {
				return true;
			} else if (this->transport_stream_id == obj.transport_stream_id) {
				if (this->service_id < obj.service_id) {
					return true;
				} else if (this->service_id == obj.service_id) {
					return false;
				} else {
					return false;
				}
			} else {
				return false;
			}
		} else {
			return false;
		}
	}


	//// keys ////
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
	//////////////


	// additional
	uint8_t service_type;
	std::string service_name;


	void clear (void) {
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		service_type = 0;
		service_name.clear();
	}

	void dump (void) const {
		_UTL_LOG_I (
			"#_service_key#  0x%04x 0x%04x 0x%04x  (svctype:0x%02x,%s)",
			transport_stream_id,
			original_network_id,
			service_id,
			service_type,
			service_name.c_str()
		);
	}

} SERVICE_KEY_t;


class CEvent {
public:
	class CExtendedInfo {
	public:
		CExtendedInfo (void) {
			item_description.clear();
			item.clear();
		}
		virtual ~CExtendedInfo (void) {
			item_description.clear();
			item.clear();
		}
		std::string item_description;
		std::string item;
	};

	class CGenre {
	public:
		CGenre (void) {
		}
		virtual ~CGenre (void) {
		}
		uint8_t content_nibble_level_1;
		uint8_t content_nibble_level_2;
	};

public:
	CEvent (void) {
		clear ();
	}
	virtual ~CEvent (void) {
		clear ();
	}

	bool operator == (const CEvent &obj) const {
		if (
//			this->table_id == obj.table_id &&
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

	bool operator != (const CEvent &obj) const {
		if (
//			this->table_id != obj.table_id ||
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


	uint8_t table_id; // sortするために必要
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t section_number; // sortするために必要

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string event_name;
	std::string text;

	uint8_t component_type;
	uint8_t component_tag;

	uint8_t audio_component_type;
	uint8_t audio_component_tag;
	uint8_t ES_multi_lingual_flag;
	uint8_t main_component_flag;
	uint8_t quality_indicator;
	uint8_t sampling_rate;

	std::vector <CGenre> genres;

	// table_id 0x58〜0x5fから取得します
	std::vector <CExtendedInfo> extendedInfos;



	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		section_number = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();	
		event_name.clear();
		text.clear();
		component_type = 0;
		component_tag = 0;
		audio_component_type = 0;
		audio_component_tag = 0;
		ES_multi_lingual_flag = 0;
		main_component_flag = 0;
		quality_indicator = 0;
		sampling_rate = 0;
		genres.clear();
		extendedInfos.clear();
	}

	void dump (void) const {
		_UTL_LOG_I (
			"tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] num:[0x%02x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			section_number,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s]",
			start_time.toString(),
			end_time.toString()
		);

		_UTL_LOG_I ("event_name:[%s]", event_name.c_str());
		_UTL_LOG_I ("text:[%s]", text.c_str());

	}

	void dump_detail (void) const {
		std::vector<CExtendedInfo>::const_iterator iter_ex = extendedInfos.begin();
		for (; iter_ex != extendedInfos.end(); ++ iter_ex) {
			_UTL_LOG_I ("item_description:[%s]", iter_ex->item_description.c_str());
			_UTL_LOG_I ("item:[%s]", iter_ex->item.c_str());
		}

		_UTL_LOG_I (
			"component_type:[%s][%s]",
			CTsAribCommon::getVideoComponentType(component_type),
			CTsAribCommon::getVideoRatio(component_type)
		);

		_UTL_LOG_I ("audio_component_type:[%s]", CTsAribCommon::getAudioComponentType(audio_component_type));
		_UTL_LOG_I ("ES_multi_lingual_flag:[%s]", ES_multi_lingual_flag ? "二ヶ国語" : "-");
		_UTL_LOG_I ("main_component_flag:[%s]", main_component_flag ? "主" : "副");
		_UTL_LOG_I ("quality_indicator:[%s]", CTsAribCommon::getAudioQuality(quality_indicator));
		_UTL_LOG_I ("sampling_rate:[%s]\n", CTsAribCommon::getAudioSamplingRate(sampling_rate));

		std::vector<CGenre>::const_iterator iter_gr = genres.begin();
		for (; iter_gr != genres.end(); ++ iter_gr) {
			_UTL_LOG_I (
				"genre:[%s][%s]",
				CTsAribCommon::getGenre_lvl1(iter_gr->content_nibble_level_1),
				CTsAribCommon::getGenre_lvl1(iter_gr->content_nibble_level_2)
			);
		}
	}

};


class CEventScheduleManager
	:public CThreadMgrBase
{
public:
	class CHistory {
	public:
		enum state {
			EN_STATE__INIT = 0,
			EN_STATE__COMPLETE,
			EN_STATE__TIMEOUT,
			EN_STATE__ERROR,
		};


		struct element {
			element (void) {
				clear();
			}
			virtual ~element (void) {
				clear();
			}

			void clear (void) {
				key.clear();
				item_num = 0;
				stt = EN_STATE__INIT;
			}

			void set_state (state _s) {
				stt = _s;
			}

			void dump (void) const {
				key.dump();
				_UTL_LOG_I (
					"  item_num=[%d] [%s]",
					item_num,
					stt == EN_STATE__INIT ? "INIT" :
						stt == EN_STATE__COMPLETE ? "COMPLETE" :
							stt == EN_STATE__TIMEOUT ? "TIMEOUT" :
								stt == EN_STATE__ERROR ? "ERROR" : "---"
				);
			}

			SERVICE_KEY_t key;
			int item_num;
			state stt;
		};


		CHistory (void) {
			clear();
		}
		virtual ~CHistory (void) {
			clear();
		}

		void add (struct element &e) {
			elements.push_back (e);
		}

		void clear (void) {
			elements.clear ();
			elements.shrink_to_fit ();
			start_time.clear();
			end_time.clear();
		}

		void set_start (void) {
			start_time.setCurrentTime();
		}

		void set_end (void) {
			end_time.setCurrentTime();
		}

		void dump (void) const {
			std::vector<struct element>::const_iterator iter = elements.begin();
			for (; iter != elements.end(); ++ iter) {
				iter->dump();
			}
			_UTL_LOG_I (
				"[%s - %s]",
				start_time.toString(),
				end_time.toString()
			);
		}

//	private:
	// cereal 非侵入型対応のため やむなくpublicに
		std::vector <struct element> elements;
		CEtime start_time;
		CEtime end_time;
	};

public:
	CEventScheduleManager (char *pszName, uint8_t nQueNum);
	virtual ~CEventScheduleManager (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_registerCacheScheduleStateNotify (CThreadMgrIf *pIf);
	void onReq_unregisterCacheScheduleStateNotify (CThreadMgrIf *pIf);
	void onReq_getCacheScheduleState (CThreadMgrIf *pIf);
	void onReq_checkLoop (CThreadMgrIf *pIf);
	void onReq_parserNotice (CThreadMgrIf *pIf);
	void onReq_startCacheSchedule (CThreadMgrIf *pIf);
	void onReq_cacheSchedule (CThreadMgrIf *pIf);
	void onReq_cacheSchedule_currentService (CThreadMgrIf *pIf);
	void onReq_getEvent (CThreadMgrIf *pIf);
	void onReq_getEvent_latestDumpedSchedule (CThreadMgrIf *pIf);
	void onReq_dumpEvent_latestDumpedSchedule (CThreadMgrIf *pIf);
	void onReq_getEvents_keywordSearch (CThreadMgrIf *pIf);
	void onReq_dumpScheduleMap (CThreadMgrIf *pIf);
	void onReq_dumpSchedule (CThreadMgrIf *pIf);
	void onReq_dumpHistories (CThreadMgrIf *pIf);


	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:
	void cacheSchedule (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		std::vector <CEvent*> *p_out_sched
	);
	void cacheSchedule_extended (std::vector <CEvent*> *p_out_sched);
	void cacheSchedule_extended (CEvent* p_out_event);
	void clearSchedule (std::vector <CEvent*> *p_sched);
	void dumpSchedule (const std::vector <CEvent*> *p_sched) const;

	bool addScheduleMap (const SERVICE_KEY_t &key, std::vector <CEvent*> *p_sched);
	void deleteScheduleMap (const SERVICE_KEY_t &key);
	bool hasScheduleMap (const SERVICE_KEY_t &key) const;
	void dumpScheduleMap (void) const;
	void dumpScheduleMap (const SERVICE_KEY_t &key) const;

	const CEvent *getEvent (const CEventScheduleManagerIf::EVENT_KEY_t &key) const;
	const CEvent *getEvent (const SERVICE_KEY_t &key, int index) const;
	int getEvents (
		const char *p_keyword,
		CEventScheduleManagerIf::EVENT_t *p_out_event,
		int out_array_num,
		bool is_include_extendedEvent
	) const;

	void pushHistories (const CHistory *p_history);
	void dumpHistories (void) const;


	void saveHistories (void) ;
	void loadHistories (void) ;



	ST_SEQ_BASE mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM]; // entity

	CSettings *mp_settings;

	EN_CACHE_SCHEDULE_STATE_t m_state;

	uint8_t m_tunerNotify_clientId;
	uint8_t m_eventChangeNotify_clientId;

	CEventInformationTable_sched *mp_EIT_H_sched;
	CEventInformationTable_sched::CReference mEIT_H_sched_ref;


	CEtime m_lastUpdate_EITSched;
	CEtime m_startTime_EITSched;

	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> m_sched_map;

	CHistory m_current_history;
	std::vector <CHistory> m_histories;

	SERVICE_KEY_t m_latest_dumped_key;


	CEtime m_schedule_cache_next_day;
	CEtime m_schedule_cache_current_plan;
};

#endif
