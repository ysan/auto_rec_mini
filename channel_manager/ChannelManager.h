#ifndef _CHANNEL_MANAGER_H_
#define _CHANNEL_MANAGER_H_

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
#include "TsAribCommon.h"
#include "ChannelManagerIf.h"

#include "TunerControlIf.h"
#include "PsisiManagerIf.h"


using namespace ThreadManager;


class CChannelManager : public CThreadMgrBase
{
public:
	CChannelManager (char *pszName, uint8_t nQueNum);
	virtual ~CChannelManager (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_channelScan (CThreadMgrIf *pIf);



private:
	ST_SEQ_BASE mSeqs [EN_SEQ_CHANNEL_MANAGER__NUM]; // entity


	uint8_t m_tunerNotify_clientId;

};

#endif
