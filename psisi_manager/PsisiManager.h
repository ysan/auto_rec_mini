#ifndef _PSISI_MANAGER_H_
#define _PSISI_MANAGER_H_

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
#include "PsisiManagerIf.h"

#include "TunerControlIf.h"
#include "TsParser.h"

#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "EventInformationTable.h"
#include "NetworkInformationTable.h"
#include "ServiceDescriptionTable.h"
#include "RunningStatusTable.h"
#include "BroadcasterInformationTable.h"
#include "TimeOffsetTable.h"


using namespace ThreadManager;


// notify category
#define NOTIFY_CAT__PAT_DETECT		((uint8_t)0) 
#define NOTIFY_CAT__EVENT_CHANGE	((uint8_t)1) 


#define PROGRAM_INFOS_MAX			(32)
#define SERVICE_INFOS_MAX			(64)
#define EVENT_PF_INFOS_MAX			(32)


typedef enum {
	EN_EVENT_PF_STATE__INIT = 0,
	EN_EVENT_PF_STATE__PRESENT,
	EN_EVENT_PF_STATE__FOLLOW,
	EN_EVENT_PF_STATE__ALREADY_PASSED,

	EN_EVENT_PF_STATE__MAX,
} EN_EVENT_PF_STATE;

static const char *gpszEventPfState [EN_EVENT_PF_STATE__MAX] = {
	// for debug log
	"-",
	"P",
	"F",
	"X",
};

typedef struct _program_info {
public:
	_program_info (void) {
		clear ();
	}
	~_program_info (void) {
		clear ();
	}

	uint8_t table_id;
	uint16_t transport_stream_id;

	uint16_t program_number;
	uint16_t program_map_PID;

	CProgramAssociationTable::CTable* p_orgTable;

	bool is_used;

	void clear (void) {
//TODO 適当クリア
		memset (this, 0x00, sizeof(struct _program_info));
		is_used = false;
	}

	void dump (void) {
		if (p_orgTable) {
			p_orgTable->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] pgm_num:[0x%04x] pmt_pid:[0x%04x]",
			table_id,
			transport_stream_id,
			program_number,
			program_map_PID
		);
	}

} _PROGRAM_INFO;

typedef struct _event_pf_info {
public:
	_event_pf_info (void) {
		clear();
	}
	~_event_pf_info (void) {
		clear();
	}

	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	char event_name_char [1024];

	EN_EVENT_PF_STATE state;

	CEventInformationTable::CTable* p_orgTable;

	bool is_used;

	void clear () {
//TODO 適当クリア
		// clear all
		memset (this, 0x00, sizeof(struct _event_pf_info));
		start_time.clear();
		end_time.clear();
		state = EN_EVENT_PF_STATE__INIT;
		is_used = false;
	}

	void dump (void) {
		if (p_orgTable) {
			p_orgTable->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"  p/f:[%s] time:[%s - %s]",
			gpszEventPfState [state],
			start_time.toString(),
			end_time.toString()
		);
		_UTL_LOG_I ("  [%s]", event_name_char);
	}

} _EVENT_PF_INFO;

typedef struct _service_info {
public:
	_service_info (void) {
		clear ();
	}
	~_service_info (void) {
		clear ();
	}

	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t service_type;
	char service_name_char [64];

	// -- clear each tuning
	bool is_tune_target;
	_EVENT_PF_INFO eventFollowInfo;
	// --

	CEtime last_update;

	CServiceDescriptionTable::CTable* p_orgTable;

	bool is_used;

	void clear (void) {
//TODO 適当クリア
		// clear all
		memset (this, 0x00, sizeof(struct _event_pf_info));
		is_tune_target = false;
		eventFollowInfo.clear();
		last_update.clear();
		is_used = false;
	}

	void clear_atTuning (void) {
		is_tune_target = false;
		eventFollowInfo.clear();
	}

	void dump (void) {
		if (p_orgTable) {
			p_orgTable->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] svctype:[0x%02x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			service_type
		);
		_UTL_LOG_I (
			"  %s [%s] last_update:[%s]",
			is_tune_target ? "*" : " ",
			service_name_char,
			last_update.toString()
		);
		if (eventFollowInfo.is_used) {
			eventFollowInfo.dump();
		}
	}

} _SERVICE_INFO;



class CPsisiManager
	:public CThreadMgrBase
	,public CTunerControlIf::ITsReceiveHandler
	,public CTsParser::IParserListener
{
public:
	CPsisiManager (char *pszName, uint8_t nQueNum);
	virtual ~CPsisiManager (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);
	void checkLoop (CThreadMgrIf *pIf);
	void parserNotice (CThreadMgrIf *pIf);
	void stabilizationAfterTuning (CThreadMgrIf *pIf);
	void registerPatDetectNotify (CThreadMgrIf *pIf);
	void unregisterPatDetectNotify (CThreadMgrIf *pIf);
	void registerEventChangeNotify (CThreadMgrIf *pIf);
	void unregisterEventChangeNotify (CThreadMgrIf *pIf);
	void getPatDetectState (CThreadMgrIf *pIf);
	void getCurrentServiceInfos (CThreadMgrIf *pIf);
	void getPresentEventInfo (CThreadMgrIf *pIf);
	void getFollowEventInfo (CThreadMgrIf *pIf);
	void dumpCaches (CThreadMgrIf *pIf);
	void dumpTables (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;

private:
	// programInfo
	void cacheProgramInfos (void);
	void dumpProgramInfos (void);
	void clearProgramInfos (void);


	// serviceInfo
	void cacheServiceInfos (bool is_atTuning);
	_SERVICE_INFO* findServiceInfo (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	_SERVICE_INFO* findEmptyServiceInfo (void);
	bool isExistServiceTable (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	void assignFollowEventToServiceInfos (void);
	void checkFollowEventAtServiceInfos (CThreadMgrIf *pIf);
	void dumpServiceInfos (void);
	void clearServiceInfos (void);
	void clearServiceInfos (bool is_atTuning);

	// serviceInfo for request
	int getCurrentServiceInfos (PSISI_SERVICE_INFO *p_out_serviceInfos, int num);


	// eventPfInfo
	void cacheEventPfInfos (void);
	bool cacheEventPfInfos (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	_EVENT_PF_INFO* findEmptyEventPfInfo (void);
//	bool checkEventPfInfos (CThreadMgrIf *pIf);
	void checkEventPfInfos (void);
	void refreshEventPfInfos (void);
	void dumpEventPfInfos (void);
	void clearEventPfInfo (_EVENT_PF_INFO *pInfo);
	void clearEventPfInfos (void);

	// eventPfInfo for request
	_EVENT_PF_INFO* findEventPfInfo (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		EN_EVENT_PF_STATE state
	);



	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;

	// CTsParser::IParserListener
	bool onTsPacketAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size) override;



	ST_SEQ_BASE mSeqs [EN_SEQ_PSISI_MANAGER_NUM]; // entity

	CTsParser m_parser;

	uint8_t m_tuner_notify_client_id;
	int m_ts_receive_handler_id;

	// tuner is tuned 
	bool m_tunerIsTuned ;

	bool m_isDetectedPAT;


	CProgramAssociationTable mPAT;
	CEventInformationTable mEIT_H;
	CNetworkInformationTable mNIT;
	CServiceDescriptionTable mSDT;
	CRunningStatusTable mRST;
	CBroadcasterInformationTable mBIT;

	CProgramAssociationTable::CReference mPAT_ref;
	CEventInformationTable::CReference mEIT_H_pf_ref;
	CEventInformationTable::CReference mEIT_H_sch_ref;
	CNetworkInformationTable::CReference mNIT_ref;
	CServiceDescriptionTable::CReference mSDT_ref;

	CEtime m_patRecvTime;

	_PROGRAM_INFO m_programInfos [PROGRAM_INFOS_MAX];
	_SERVICE_INFO m_serviceInfos [SERVICE_INFOS_MAX];
	_EVENT_PF_INFO m_eventPfInfos [EVENT_PF_INFOS_MAX];

};

#endif
