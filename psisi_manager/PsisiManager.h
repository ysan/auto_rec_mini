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
	void checkPAT (CThreadMgrIf *pIf);
	void dumpTables (CThreadMgrIf *pIf);


private:
	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;

	// CTsParser::IParserListener
	bool onTsPacketAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size) override;


	ST_SEQ_BASE mSeqs [EN_SEQ_PSISI_MANAGER_NUM]; // entity

	CTsParser m_parser;

	int m_ts_receive_handler_id;


	CProgramAssociationTable mPAT;
	CEventInformationTable mEIT_H;
	CNetworkInformationTable mNIT;
	CServiceDescriptionTable mSDT;
	CRunningStatusTable mRST;
	CBroadcasterInformationTable mBIT;

	CProgramAssociationTable::CReference mPAT_ref;
	CEventInformationTable::CReference mEIT_H_ref;

};

#endif
