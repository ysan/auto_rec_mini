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


#define SERVICE_INFOS_MAX	(64)
#define EVENT_PF_INFOS_MAX	(32)

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

typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	char event_name_char [1024];

	EN_EVENT_PF_STATE state;

	CEventInformationTable::CTable* p_org_table_addr;

	bool is_used;

	void dump (void) {
		p_org_table_addr->header.dump();
		_UTL_LOG_I (
			"  tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"  p/f:[%s] time:[%s - %s] [%s]",
			gpszEventPfState [state],
			start_time.toString(),
			end_time.toString(),
			event_name_char
		);
	}

} _EVENT_PF_INFO;

typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint8_t service_type;
	char service_name_char [64];

	// clear each tuning
	bool is_tune_target;
	_EVENT_PF_INFO *p_event_present;
	_EVENT_PF_INFO *p_event_follow;

	CEtime last_update;

	bool is_used;

	void dump (void) {
		_UTL_LOG_I (
			"tblid:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] svctype:[0x%02x] %s [%s] [%s]",
			table_id,
			transport_stream_id,
			original_network_id,
			service_id,
			service_type,
			is_tune_target ? "*" : " ",
			service_name_char,
			last_update.toString()
		);
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
	void dumpCaches (CThreadMgrIf *pIf);
	void dumpTables (CThreadMgrIf *pIf);


private:
	void onReceiveNotify (CThreadMgrIf *pIf) override;

	void cacheServiceInfos (bool is_atTuning);
	_SERVICE_INFO* findEmptyServiceInfo (void);
	void dumpServiceInfos (void);
	void clearServiceInfos (bool is_atTuning);

	void cacheEventPfInfos (void);
	void cacheEventPfInfos (uint16_t _service_id);
	_EVENT_PF_INFO* findEmptyEventPfInfo (void);
	void checkEventPfInfos (void);
	void refreshEventPfInfos (void);
	void dumpEventPfInfos (void);
	void clearEventPfInfo (_EVENT_PF_INFO *pInfo);
	void clearEventPfInfos (void);



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

	_SERVICE_INFO m_serviceInfos [SERVICE_INFOS_MAX];
	_EVENT_PF_INFO m_eventPfInfos [EVENT_PF_INFOS_MAX];

};

#endif
