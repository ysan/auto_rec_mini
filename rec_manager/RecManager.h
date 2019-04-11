#ifndef _REC_MANAGER_H_
#define _REC_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"

#include "ThreadMgrBase.h"
#include "ThreadMgr.h"

#include "Utils.h"
#include "RecManagerIf.h"

#include "TunerControlIf.h"
#include "PsisiManagerIf.h"


using namespace ThreadManager;


#define RESERVE_NUM_MAX		(60)

typedef enum {
	EN_REC_STATE__INIT = 0,
	EN_REC_STATE__PRE_PROCESS,
	EN_REC_STATE__NOW_RECORDING,
	EN_REC_STATE__POST_PROCESS,
} EN_REC_STATE;

typedef enum {
	EN_RESERVE_STATE__INIT = 0,
	EN_RESERVE_STATE__PRE_PROCESS,
	EN_RESERVE_STATE__NOW_RECORDING,
	EN_RESERVE_STATE__POST_PROCESS,
	EN_RESERVE_STATE__END_SUCCESS,
	EN_RESERVE_STATE__END_ERROR,
} EN_RESERVE_STATE;


class CRecReserve {
public:
	CRecReserve (void) {
		clear ();
	}
	~CRecReserve (void) {}

	bool operator == (const CRecReserve &obj) const {
		if (
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

	bool operator != (const CRecReserve &obj) const {
		if (
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

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	char title_name [1024];

	EN_RESERVE_STATE state;
	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		char *psz_title_name
	)
	{
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->event_id = _event_id;
		this->start_time = *p_start_time;
		this->end_time = *p_end_time;
		if (psz_title_name) {
			strncpy (this->title_name, psz_title_name, strlen(psz_title_name));
		}
		this->state = EN_RESERVE_STATE__INIT;
		this->is_used = true;
	}

	void clear (void) {
//TODO 適当クリア
		// clear all
		memset (this, 0x00, sizeof(CRecReserve));
		state = EN_RESERVE_STATE__INIT;
		is_used = false;
	}

	void dump (void) {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
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
		_UTL_LOG_I ("[%s]", title_name);
	}

};


class CRecManager
	:public CThreadMgrBase
	,public CTunerControlIf::ITsReceiveHandler
{
public:
	CRecManager (char *pszName, uint8_t nQueNum);
	virtual ~CRecManager (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);
	void checkLoop (CThreadMgrIf *pIf);
	void startRecording (CThreadMgrIf *pIf);
	void startCurrentEventRecording (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;

private:
	bool setReserve (PSISI_EVENT_INFO *p_info);
	bool setReserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		char *psz_title_name
	);
	CRecReserve* searchAscendingOrderReserve (CEtime *p_start_time_rf);
	bool isExistEmptyReserve (void);
	CRecReserve *findEmptyReserve (void);
	bool isDuplicateReserve (CRecReserve* p_reserve);
	bool isOverrapTimeReserve (CRecReserve* p_reserve);



	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;



	ST_SEQ_BASE mSeqs [EN_SEQ_REC_MANAGER_NUM]; // entity

	uint8_t m_tunerNotify_clientId;
	int m_tsReceive_handlerId;
	uint8_t m_eventChangeNotify_clientId;

	EN_REC_STATE m_recState;

	CRecReserve m_reserves [RESERVE_NUM_MAX];

};

#endif
