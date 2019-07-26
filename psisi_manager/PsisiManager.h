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
#include "EventScheduleManagerIf.h"

#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "EventInformationTable.h"
#include "EventInformationTable_sched.h"
#include "NetworkInformationTable.h"
#include "ServiceDescriptionTable.h"
#include "RunningStatusTable.h"
#include "BroadcasterInformationTable.h"
#include "TimeOffsetTable.h"


using namespace ThreadManager;


// notify category
#define NOTIFY_CAT__PAT_DETECT			((uint8_t)0)
#define NOTIFY_CAT__EVENT_CHANGE		((uint8_t)1)
#define NOTIFY_CAT__TUNE_COMPLETE	 	((uint8_t)2)


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
		memset (this, 0x00, sizeof(struct _program_info));
		p_orgTable = NULL;
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

	char event_name_char [MAXSECLEN];

	EN_EVENT_PF_STATE state;

	CEventInformationTable::CTable* p_orgTable;

	bool is_used;

	void clear (void) {
		// clear all
		memset (this, 0x00, sizeof(struct _event_pf_info));
		start_time.clear();
		end_time.clear();
		state = EN_EVENT_PF_STATE__INIT;
		p_orgTable = NULL;
		is_used = false;
	}

	void dump (void) const{
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
		_UTL_LOG_I ("  event_name:[%s]", event_name_char);
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
		// clear all
		memset (this, 0x00, sizeof(struct _event_pf_info));
		is_tune_target = false;
		eventFollowInfo.clear();
		last_update.clear();
		p_orgTable = NULL;
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
			"  %s service_name:[%s] last_update:[%s]",
			is_tune_target ? "*" : " ",
			service_name_char,
			last_update.toString()
		);
		if (eventFollowInfo.is_used) {
			eventFollowInfo.dump();
		}
	}

} _SERVICE_INFO;

typedef struct _network_info {
public:
	_network_info (void) {
		clear ();
	}
	~_network_info (void) {
		clear ();
	}

	uint8_t table_id;
	uint16_t network_id;

	// CNetworkNameDescriptor
	char network_name_char [64];


	//----- transport_stream_loop -----

	uint16_t transport_stream_id;
	uint16_t original_network_id;

	// CServiceListDescriptor
	struct service {
		uint16_t service_id;
		uint8_t service_type;
	} services [16];
	int services_num;

	// CTerrestrialDeliverySystemDescriptor
	uint16_t area_code;
	uint8_t guard_interval;
	uint8_t transmission_mode;

	// CTSInformationDescriptor
	uint8_t remote_control_key_id;
	char ts_name_char  [64];

	//----- transport_stream_loop ------


	CNetworkInformationTable::CTable* p_orgTable;

	bool is_used;

	void clear (void) {
		// clear all
		memset (this, 0x00, sizeof(struct _network_info));
		p_orgTable = NULL;
		is_used = false;
	}

	void dump (void) {
		if (p_orgTable) {
			p_orgTable->header.dump();
		}
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id
		);
		_UTL_LOG_I ("  network_name:[%s]", network_name_char);
		_UTL_LOG_I ("  ----------");
		for (int i = 0; i < services_num; ++ i) {
			_UTL_LOG_I (
				"  svcid:[0x%04x] svctype:[0x%02x]",
				services[i].service_id,
				services[i].service_type
			);
		}
		_UTL_LOG_I ("  ----------");
		_UTL_LOG_I (
			"  area_code:[0x%04x] guard:[0x%02x] trans_mode:[0x%02x]",
			area_code,
			guard_interval,
			transmission_mode
		);
		_UTL_LOG_I (
			"  remote_control_key_id:[0x%04x] ts_name:[%s]",
			remote_control_key_id,
			ts_name_char
		);
	}

} _NETWORK_INFO;

typedef struct _section_comp_flag {
public:
	_section_comp_flag (void) {
		clear ();
	}
	~_section_comp_flag (void) {
		clear ();
	}

	void check_update (bool is_new_section) {
		if (is_new_section) {
			if (!m_flag) {
				m_flag = 1;
			}
		} else {
			if (m_flag == 1) {
				m_flag = 2;
			}
		}
	}

	bool is_completed (void) {
		return (m_flag == 2) ;
	}

	void clear (void) {
		m_flag = 0;
	}

private:
	int m_flag;

} SECTION_COMP_FLAG_t ;


class CPsisiManager
	:public CThreadMgrBase
	,public CTunerControlIf::ITsReceiveHandler
	,public CTsParser::IParserListener
	,public CEventInformationTable_sched::IEventScheduleHandler
{
public:
	CPsisiManager (char *pszName, uint8_t nQueNum);
	virtual ~CPsisiManager (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_checkLoop (CThreadMgrIf *pIf);
	void onReq_parserNotice (CThreadMgrIf *pIf);
	void onReq_stabilizationAfterTuning (CThreadMgrIf *pIf);
	void onReq_registerPatDetectNotify (CThreadMgrIf *pIf);
	void onReq_unregisterPatDetectNotify (CThreadMgrIf *pIf);
	void onReq_registerEventChangeNotify (CThreadMgrIf *pIf);
	void onReq_unregisterEventChangeNotify (CThreadMgrIf *pIf);
	void onReq_registerTuneCompleteNotify (CThreadMgrIf *pIf);
	void onReq_unregisterTuneCompleteNotify (CThreadMgrIf *pIf);
	void onReq_getPatDetectState (CThreadMgrIf *pIf);
	void onReq_getCurrentServiceInfos (CThreadMgrIf *pIf);
	void onReq_getPresentEventInfo (CThreadMgrIf *pIf);
	void onReq_getFollowEventInfo (CThreadMgrIf *pIf);
	void onReq_getCurrentNetworkInfo (CThreadMgrIf *pIf);
	void onReq_enableParseEITSched (CThreadMgrIf *pIf);
	void onReq_disableParseEITSched (CThreadMgrIf *pIf);
	void onReq_dumpCaches (CThreadMgrIf *pIf);
	void onReq_dumpTables (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:
	//-- programInfo --
	void cacheProgramInfos (void);
	void dumpProgramInfos (void);
	void clearProgramInfos (void);


	//-- serviceInfo --
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

	// for request
	int getCurrentServiceInfos (PSISI_SERVICE_INFO *p_out_serviceInfos, int num);


	//-- eventPfInfo --
	void cacheEventPfInfos (void);
	bool cacheEventPfInfos (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	_EVENT_PF_INFO* findEmptyEventPfInfo (void);
	void checkEventPfInfos (void);
	void refreshEventPfInfos (void);
	void dumpEventPfInfos (void);
	void clearEventPfInfo (_EVENT_PF_INFO *pInfo);
	void clearEventPfInfos (void);

	// for request
	_EVENT_PF_INFO* findEventPfInfo (
		uint8_t _table_id,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		EN_EVENT_PF_STATE state
	);


	//-- networkInfo --
	void cacheNetworkInfo (void);
	void dumpNetworkInfo (void);
	void clearNetworkInfo (void);



	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;

	// CTsParser::IParserListener
	bool onTsPacketAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size) override;

	// CEventInformationTable_sched::IEventSchedleHandler
	void onScheduleUpdate (void) override;



	ST_SEQ_BASE mSeqs [EN_SEQ_PSISI_MANAGER__NUM]; // entity

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
	CEventInformationTable::CReference mEIT_H_ref;
	CNetworkInformationTable::CReference mNIT_ref;
	CServiceDescriptionTable::CReference mSDT_ref;

	CEtime m_patRecvTime;

	_PROGRAM_INFO m_programInfos [PROGRAM_INFOS_MAX];
	_SERVICE_INFO m_serviceInfos [SERVICE_INFOS_MAX];
	_EVENT_PF_INFO m_eventPfInfos [EVENT_PF_INFOS_MAX];
	_NETWORK_INFO m_networkInfo ;

	SECTION_COMP_FLAG_t m_EIT_H_comp_flag;
	SECTION_COMP_FLAG_t m_SDT_comp_flag;
	SECTION_COMP_FLAG_t m_NIT_comp_flag;


	// for EIT schedule
	CEventInformationTable_sched mEIT_H_sched;
	bool m_isEnableEITSched;

};

#endif
