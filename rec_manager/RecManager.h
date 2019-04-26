#ifndef _REC_MANAGER_H_
#define _REC_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>

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


#define RESERVE_NUM_MAX		(4)
#define RESULT_NUM_MAX		(4)


typedef enum {
	EN_REC_PROGRESS__INIT = 0,
	EN_REC_PROGRESS__PRE_PROCESS,
	EN_REC_PROGRESS__NOW_RECORDING,
	EN_REC_PROGRESS__END_SUCCESS,
	EN_REC_PROGRESS__END_ERROR,
	EN_REC_PROGRESS__POST_PROCESS,
} EN_REC_PROGRESS;

typedef enum {
	EN_RESERVE_STATE__INIT = 0,
	EN_RESERVE_STATE__REQ_START_RECORDING,
	EN_RESERVE_STATE__NOW_RECORDING,
	EN_RESERVE_STATE__END_SUCCESS,
	EN_RESERVE_STATE__END_ERROR__FORCE_STOP,
	EN_RESERVE_STATE__END_ERROR__ALREADY_PASSED,
	EN_RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW,
	EN_RESERVE_STATE__END_ERROR__INTERNAL_ERR,
} EN_RESERVE_STATE;

const char *g_reserveState [] = {
	"INIT",
	"REQ_START_RECORDING",
	"NOW_RECORDING",
	"END_SUCCESS",
	"END_ERROR__FORCE_STOP",
	"END_ERROR__ALREADY_PASSED",
	"END_ERROR__HDD_FREE_SPACE_LOW",
	"END_ERROR__INTERNAL_ERR",
};

const char *g_repeatability [] = {
	"NONE",
	"DAYLY",
	"WEEKLY",
};


class CRecReserve {
public:
	CRecReserve (void) {
		clear ();
	}
	~CRecReserve (void) {
		clear ();
	}

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

	std::string title_name;

	EN_RESERVE_REPEATABILITY repeatability;

	EN_RESERVE_STATE state;
	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name,
		EN_RESERVE_REPEATABILITY _repeatability
	)
	{
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->event_id = _event_id;
		this->start_time = *p_start_time;
		this->end_time = *p_end_time;
		if (psz_title_name) {
			this->title_name = psz_title_name ;
		}
		this->repeatability = _repeatability;
		
		this->state = EN_RESERVE_STATE__INIT;
		this->is_used = true;
	}

	void clear (void) {
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();	
		title_name.clear();
		repeatability = EN_RESERVE_REPEATABILITY__NONE;
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
			"time:[%s - %s] repeat:[%s] state:[%s]",
			start_time.toString(),
			end_time.toString(),
			g_repeatability [repeatability],
			g_reserveState [state]
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
	}

};


class CRecManager
	:public CThreadMgrBase
	,public CTunerControlIf::ITsReceiveHandler
{
public:
	CRecManager (char *pszName, uint8_t nQueNum);
	virtual ~CRecManager (void);


	void onModuleUp (CThreadMgrIf *pIf);
	void onModuleDown (CThreadMgrIf *pIf);
	void onCheckLoop (CThreadMgrIf *pIf);
	void onRecordingNotice (CThreadMgrIf *pIf);
	void onStartRecording (CThreadMgrIf *pIf);
	void onAddReserve_currentEvent (CThreadMgrIf *pIf);
	void onAddReserve_manual (CThreadMgrIf *pIf);
	void onRemoveReserve (CThreadMgrIf *pIf);
	void onStopRecording (CThreadMgrIf *pIf);
	void onDumpReserves (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:
	bool addReserve (PSISI_EVENT_INFO *p_info);
	bool addReserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name,
		EN_RESERVE_REPEATABILITY repeatability=EN_RESERVE_REPEATABILITY__NONE
	);
	bool removeReserve (int index);
	CRecReserve* searchAscendingOrderReserve (CEtime *p_start_time_rf);
	bool isExistEmptyReserve (void) const;
	CRecReserve *findEmptyReserve (void);
	bool isDuplicateReserve (const CRecReserve* p_reserve) const;
	bool isOverrapTimeReserve (const CRecReserve* p_reserve) const;
	void checkReserves (void);
	void refreshReserves (void);
	bool pickReqStartRecordingReserve (void);
	void setResult (CRecReserve *p);
	void checkRecordingEnd (void);
	void checkRepeatability (const CRecReserve *p_reserve);
	void dumpReserves (void);
	void dumpResults (void);
	void clearReserves (void);
	void clearResults (void);



	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;



	ST_SEQ_BASE mSeqs [EN_SEQ_REC_MANAGER_NUM]; // entity


	uint8_t m_tunerNotify_clientId;
	int m_tsReceive_handlerId;
	uint8_t m_patDetectNotify_clientId;
	uint8_t m_eventChangeNotify_clientId;

	EN_REC_PROGRESS m_recProgress; // tuneThreadと共有する とりあえず排他はいれません

	CRecReserve m_reserves [RESERVE_NUM_MAX];
	CRecReserve m_results [RESULT_NUM_MAX];
	CRecReserve m_recording;

	PSISI_SERVICE_INFO m_serviceInfos [10];
	PSISI_EVENT_INFO m_presentEventInfo;


	struct OutputBuffer *mp_outputBuffer;

};

#endif
