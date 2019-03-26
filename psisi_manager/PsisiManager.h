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
	EN_EVENT_PF__FINISHED = 0,
	EN_EVENT_PF__PRESENT,
	EN_EVENT_PF__FOLLOW,

} EN_EVENT_PF;


typedef struct {
	uint8_t table_id;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	char event_name_char [1024];

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
	void dumpTables (CThreadMgrIf *pIf);


private:
	void onReceiveNotify (CThreadMgrIf *pIf) override;

	void cacheServiceInfos (bool is_atTuning);
	void clearServiceInfos (bool is_atTuning);
	_SERVICE_INFO* findEmptyServiceInfo (void);

	void cacheEventPfInfos (void);
	void clearEventPfInfos (void);
	_EVENT_PF_INFO* findEmptyEventPfInfo (void);
	bool getEventPFbyServiceId (uint16_t service_id, _EVENT_PF_INFO *p_out_event_pf_info);



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

	bool m_isTuned ;


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


	_SERVICE_INFO m_serviceInfos [SERVICE_INFOS_MAX];
	_EVENT_PF_INFO m_eventPfInfos [EVENT_PF_INFOS_MAX];
};

#endif
