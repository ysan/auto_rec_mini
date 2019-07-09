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

#include "Utils.h"
#include "EventScheduleManagerIf.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"

#include "EventInformationTable_sched.h"


using namespace ThreadManager;


typedef struct _service_key {
public:
	_service_key (void) {
		clear ();
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


	// keys
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	void clear (void) {
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
	}

	void dump (void) {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x]",
			transport_stream_id,
			original_network_id,
			service_id
		);
	}

} _SERVICE_KEY;


class CEventSchedule {
public:
	CEventSchedule (void) {
		clear ();
	}
	~CEventSchedule (void) {
		clear ();
	}

	bool operator == (const CEventSchedule &obj) const {
		if (
			this->table_id == obj.table_id &&
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id &&
			this->service_id == obj.service_id &&
			this->event_id == obj.event_id &&
			this->start_time == obj.start_time &&
			this->end_time == obj.end_time
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator != (const CEventSchedule &obj) const {
		if (
			this->table_id != obj.table_id ||
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id ||
			this->service_id != obj.service_id ||
			this->event_id != obj.event_id ||
			this->start_time != obj.start_time ||
			this->end_time != obj.end_time
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

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string title_name;


	void set (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name
	)
	{
		this->table_id = _table_id;
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->event_id = _event_id;
		this->start_time = *p_start_time;
		this->end_time = *p_end_time;
		if (psz_title_name) {
			this->title_name = psz_title_name ;
		}
	}

	void clear (void) {
		table_id = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();	
		title_name.clear();
	}

	void dump (void) {
		_UTL_LOG_I (
			"tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s]",
			start_time.toString(),
			end_time.toString()
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
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
	void onReq_startCache_currentService (CThreadMgrIf *pIf);


	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:

	ST_SEQ_BASE mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM]; // entity

	uint8_t m_tunerNotify_clientId;
	uint8_t m_eventChangeNotify_clientId;

	CEventInformationTable_sched *mp_EIT_H_sched;


	CEtime m_lastUpdate_EITSched;
	CEtime m_startTime_EITSched;

	std::map <_SERVICE_KEY, std::vector <CEventSchedule*> > m_event_map;

};

#endif
