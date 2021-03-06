#ifndef _REC_MANAGER_H_
#define _REC_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "Group.h"
#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Settings.h"
#include "RecManagerIf.h"
#include "RecInstance.h"
#include "RecAribB25.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"
#include "TunerServiceIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"
#include "EventScheduleManagerIf.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"


using namespace ThreadManager;


#define RESERVE_NUM_MAX		(50)
#define RESULT_NUM_MAX		(20)

#define RESERVE_STATE__INIT								(0x00000000)
#define RESERVE_STATE__REMOVED							(0x00000001)
#define RESERVE_STATE__RESCHEDULED						(0x00000002)
#define RESERVE_STATE__START_TIME_PASSED				(0x00000004)
#define RESERVE_STATE__REQ_START_RECORDING				(0x00000100)
#define RESERVE_STATE__NOW_RECORDING					(0x00000200)
#define RESERVE_STATE__FORCE_STOPPED					(0x00000400)
#define RESERVE_STATE__END_ERROR__ALREADY_PASSED		(0x00010000)
#define RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW	(0x00020000)
#define RESERVE_STATE__END_ERROR__TUNE_ERR				(0x00040000)
#define RESERVE_STATE__END_ERROR__EVENT_NOT_FOUND		(0x00080000)
#define RESERVE_STATE__END_ERROR__INTERNAL_ERR			(0x00100000)
#define RESERVE_STATE__END_SUCCESS						(0x80000000)


typedef enum {
	EN_REC_PROGRESS__INIT = 0,
	EN_REC_PROGRESS__PRE_PROCESS,
	EN_REC_PROGRESS__NOW_RECORDING,
	EN_REC_PROGRESS__END_SUCCESS,
	EN_REC_PROGRESS__END_ERROR,
	EN_REC_PROGRESS__POST_PROCESS,
} EN_REC_PROGRESS;


struct reserve_state_pair {
	uint32_t state;
	const char *psz_reserveState;
};

const char * getReserveStateString (uint32_t state);


class CRecReserve {
public:
	CRecReserve (void) {
		clear ();
	}
	virtual ~CRecReserve (void) {
		clear ();
	}

	// event_type only
	bool operator == (const CRecReserve &obj) const {
		if (
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

	// event_type only
	bool operator != (const CRecReserve &obj) const {
		if (
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

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string title_name;
	std::string service_name;

	bool is_event_type ;
	EN_RESERVE_REPEATABILITY repeatability;

	uint32_t state;

	CEtime recording_start_time;
	CEtime recording_end_time;

	uint8_t group_id;

	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		bool _is_event_type,
		EN_RESERVE_REPEATABILITY _repeatability
	)
	{
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->event_id = _event_id;
		if (p_start_time) {
			this->start_time = *p_start_time;
		}
		if (p_end_time) {
			this->end_time = *p_end_time;
		}
		if (psz_title_name) {
			this->title_name = psz_title_name ;
		}
		if (psz_service_name) {
			this->service_name = psz_service_name ;
		}
		this->is_event_type = _is_event_type;
		this->repeatability = _repeatability;
		this->state = RESERVE_STATE__INIT;
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
		title_name.shrink_to_fit();
		service_name.clear();
		service_name.shrink_to_fit();
		is_event_type = false;
		repeatability = EN_RESERVE_REPEATABILITY__NONE;
		state = RESERVE_STATE__INIT;
		recording_start_time.clear();
		recording_end_time.clear();	
		group_id = 0xff;
		is_used = false;
	}

	void dump (void) const {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s] event_type:[%d] repeat:[%s] state:[%s]",
			start_time.toString(),
			end_time.toString(),
			is_event_type,
			repeatability == 0 ? "NONE" :
				repeatability == 1 ? "DAILY" :
					repeatability == 2 ? "WEEKLY" :
						repeatability == 3 ? "AUTO" : "???",
			getReserveStateString (state)
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
		_UTL_LOG_I ("service_name:[%s]", service_name.c_str());

		if (state != RESERVE_STATE__INIT) {
			if (state & RESERVE_STATE__NOW_RECORDING) {
				CEtime _cur;
				_cur.setCurrentTime ();
				CEtime::CABS diff = _cur - recording_start_time;
				_UTL_LOG_I ("recording_time:[%s]", diff.toString());
				_UTL_LOG_I ("group_id:[0x%02x]", group_id);

			} else {
				CEtime::CABS diff = recording_end_time - recording_start_time;
				_UTL_LOG_I ("recording_time:[%s]", diff.toString());
				_UTL_LOG_I ("group_id:[0x%02x]", group_id);
			}
		}
	}
};


class CRecManager
	:public CThreadMgrBase
////	,public CTunerControlIf::ITsReceiveHandler
{
public:
	CRecManager (char *pszName, uint8_t nQueNum);
	virtual ~CRecManager (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_moduleUpByGroupId (CThreadMgrIf *pIf);
	void onReq_moduleDownByGroupId (CThreadMgrIf *pIf);
	void onReq_checkLoop (CThreadMgrIf *pIf);
	void onReq_checkReservesEventLoop (CThreadMgrIf *pIf);
	void onReq_checkRecordingsEventLoop (CThreadMgrIf *pIf);
	void onReq_recordingNotice (CThreadMgrIf *pIf);
	void onReq_startRecording (CThreadMgrIf *pIf);
	void onReq_addReserve_currentEvent (CThreadMgrIf *pIf);
	void onReq_addReserve_event (CThreadMgrIf *pIf);
	void onReq_addReserve_eventHelper (CThreadMgrIf *pIf);
	void onReq_addReserve_manual (CThreadMgrIf *pIf);
	void onReq_removeReserve (CThreadMgrIf *pIf);
	void onReq_removeReserve_byIndex (CThreadMgrIf *pIf);
	void onReq_getReserves (CThreadMgrIf *pIf);
	void onReq_stopRecording (CThreadMgrIf *pIf);
	void onReq_dumpReserves (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:
	bool addReserve (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		bool _is_event_type,
		EN_RESERVE_REPEATABILITY repeatability,
		bool _is_rescheduled=false
	);
	int getReserveIndex (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id
	);
	bool removeReserve (int index, bool isConsiderRepeatability, bool isApplyResult);
	CRecReserve* searchAscendingOrderReserve (CEtime *p_start_time_rf);
	bool isExistEmptyReserve (void) const;
	CRecReserve *findEmptyReserve (void);
	bool isDuplicateReserve (const CRecReserve* p_reserve) const;
	bool isOverrapTimeReserve (const CRecReserve* p_reserve) const;
	void checkReserves (void);
	void refreshReserves (uint32_t state);
	bool isExistReqStartRecordingReserve (void);
	bool pickReqStartRecordingReserve (uint8_t groupId);
	void setResult (CRecReserve *p);
	void checkRecordingEnd (void);
	void checkDiskFree (void);
	void checkRepeatability (const CRecReserve *p_reserve);
	int getReserves (CRecManagerIf::RESERVE_t *p_out_reserves, int out_array_num) const;
	void dumpReserves (void) const;
	void dumpResults (void) const;
	void dumpRecording (void) const;
	void clearReserves (void);
	void clearResults (void);


	// CTunerControlIf::ITsReceiveHandler
////	bool onPreTsReceive (void) override;
////	void onPostTsReceive (void) override;
////	bool onCheckTsReceiveLoop (void) override;
////	bool onTsReceived (void *p_ts_data, int length) override;


	void saveReserves (void);
	void loadReserves (void);

	void saveResults (void);
	void loadResults (void);


	CSettings *mp_settings;
	
////	uint8_t m_tunerNotify_clientId;
////	int m_tsReceive_handlerId;
////	uint8_t m_patDetectNotify_clientId;
////	uint8_t m_eventChangeNotify_clientId;
	uint8_t m_tunerNotify_clientId [CGroup::GROUP_MAX];
	int m_tsReceive_handlerId [CGroup::GROUP_MAX];
	uint8_t m_patDetectNotify_clientId [CGroup::GROUP_MAX];
	uint8_t m_eventChangeNotify_clientId [CGroup::GROUP_MAX];

////	EN_REC_PROGRESS m_recProgress; // tuneThreadと共有する とりあえず排他はいれません

	CRecReserve m_reserves [RESERVE_NUM_MAX];
	CRecReserve m_results [RESULT_NUM_MAX];
////	CRecReserve m_recording;
	CRecReserve m_recordings [CGroup::GROUP_MAX];

////	char m_recording_tmpfile [PATH_MAX];
	char m_recording_tmpfiles [CGroup::GROUP_MAX][PATH_MAX];
////	unique_ptr<CRecAribB25> msp_b25;

	std::unique_ptr <CRecInstance> msp_rec_instances [CGroup::GROUP_MAX];
	CTunerControlIf::ITsReceiveHandler *mp_ts_handlers [CGroup::GROUP_MAX];

	int m_tuner_resource_max;
};

#endif
