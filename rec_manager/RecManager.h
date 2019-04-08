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


// notify category
//#define NOTIFY_CAT__PAT_DETECT		((uint8_t)0) 
//#define NOTIFY_CAT__EVENT_CHANGE	((uint8_t)1) 


class CRecManager
	:public CThreadMgrBase
	,public CTunerControlIf::ITsReceiveHandler
{
public:
	CRecManager (char *pszName, uint8_t nQueNum);
	virtual ~CRecManager (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);


private:
	void onReceiveNotify (CThreadMgrIf *pIf) override;


	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;



	ST_SEQ_BASE mSeqs [EN_SEQ_REC_MANAGER_NUM]; // entity

	uint8_t m_tunerNotify_clientId;
	int m_ts_receive_handler_id;
	uint8_t m_eventChangeNotify_clientId;

};

#endif
