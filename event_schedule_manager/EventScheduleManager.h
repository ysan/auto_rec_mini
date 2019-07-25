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


using namespace ThreadManager;


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

	~_service_key (void) {
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
	class CGenre {
	public:
		CGenre (void) {
			lvl1.clear ();
			lvl2.clear ();
		}
		~CGenre (void) {
			lvl1.clear ();
			lvl2.clear ();
		}
		std::string lvl1;
		std::string lvl2;
	};

public:
	CEvent (void) {
		clear ();
	}
	~CEvent (void) {
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


	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t section_number;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string event_name;
	std::string text;

	std::string video_component_type;
	std::string video_ratio;
	uint8_t component_tag;

	std::vector <CGenre> genres;


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
		video_component_type.clear();
		video_ratio.clear();
		component_tag = 0;
		genres.clear();
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
		_UTL_LOG_I (
			"vode_component:[%s][%s] component_tag:[0x%02x]",
			video_component_type.c_str(),
			video_ratio.c_str(),
			component_tag
		);
		std::vector<CGenre>::const_iterator iter = genres.begin();
		for (; iter != genres.end(); ++ iter) {
			_UTL_LOG_I ("genre:[%s][%s]", iter->lvl1.c_str(), iter->lvl2.c_str());
		}
	}

};


class CEventScheduleManager
	:public CThreadMgrBase
{
public:
	CEventScheduleManager (char *pszName, uint8_t nQueNum);
	virtual ~CEventScheduleManager (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_checkLoop (CThreadMgrIf *pIf);
	void onReq_parserNotice (CThreadMgrIf *pIf);
	void onReq_startCacheSchedule (CThreadMgrIf *pIf);
	void onReq_cacheSchedule_currentService (CThreadMgrIf *pIf);
	void onReq_getEvent (CThreadMgrIf *pIf);
	void onReq_getEvent_latestDumpedSchedule (CThreadMgrIf *pIf);
	void onReq_dumpScheduleMap (CThreadMgrIf *pIf);
	void onReq_dumpSchedule (CThreadMgrIf *pIf);


	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:
	void cacheSchedule (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		std::vector <CEvent*> *p_out_sched
	);
	void clearSchedule (std::vector <CEvent*> *p_sched);
	void dumpSchedule (const std::vector <CEvent*> *p_sched) const;

	bool addScheduleMap (const SERVICE_KEY_t &key, std::vector <CEvent*> *p_sched);
	void deleteScheduleMap (const SERVICE_KEY_t &key);
	bool hasScheduleMap (const SERVICE_KEY_t &key) const;
	void dumpScheduleMap (void) const;
	void dumpScheduleMap (const SERVICE_KEY_t &key) const;

	const CEvent *getEvent (const CEventScheduleManagerIf::EVENT_KEY_t &key) const;
	const CEvent *getEvent (const SERVICE_KEY_t &key, int index) const;



	ST_SEQ_BASE mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM]; // entity

	CSettings *mp_settings;

	uint8_t m_tunerNotify_clientId;
	uint8_t m_eventChangeNotify_clientId;

	CEventInformationTable_sched *mp_EIT_H_sched;
	CEventInformationTable_sched::CReference mEIT_H_sched_ref;


	CEtime m_lastUpdate_EITSched;
	CEtime m_startTime_EITSched;

	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> m_sched_map;

	SERVICE_KEY_t m_latest_dumped_key;


	CEtime m_schedule_cache_next_day;
	CEtime m_schedule_cache_current_plan;
};

#endif
