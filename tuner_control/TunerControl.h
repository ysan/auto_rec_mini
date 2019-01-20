#ifndef _TUNER_CONTROL_H_
#define _TUNER_CONTROL_H_

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
#include "TunerControlIf.h"
#include "it9175_extern.h"


using namespace ThreadManager;

#define TS_CALLBACKS_REGISTER_NUM_MAX		(10)

class CTunerControl : public CThreadMgrBase
{
public:
	class ITsCallbacks {
	public:
		virtual ~ITsCallbacks (void) {};
		virtual bool onPreTsReceive (void *p_shared_data) = 0;
		virtual void onPostTsReceive (void *p_shared_data) = 0;
		virtual bool onCheckTsReceiveLoop (void *p_shared_data) = 0;
		virtual bool onTsReceived (void *p_shared_data, void *p_ts_data, int length) = 0;
	};

public:
	CTunerControl (char *pszName, uint8_t nQueNum);
	virtual ~CTunerControl (void);


	void start (CThreadMgrIf *pIf);
	void tune (CThreadMgrIf *pIf);
	void tuneStart (CThreadMgrIf *pIf);
	void tuneStop (CThreadMgrIf *pIf);


	int registerTsCallbacks (ITsCallbacks *pCallbacks);
	void unregisterTsCallbacks (int id);


private:
	// it9175 ts callbacks
	static bool onPreTsReceive (void *p_shared_data);
	static void onPostTsReceive (void *p_shared_data);
	static bool onCheckTsReceiveLoop (void *p_shared_data);
	static bool onTsReceived (void *p_shared_data, void *p_ts_data, int length);
	ST_IT9175_TS_RECEIVE_CALLBACKS m_it9175_ts_callbacks;

	uint32_t mFreq;

	ITsCallbacks *mpRegTsCallbacks [TS_CALLBACKS_REGISTER_NUM_MAX];


	ST_SEQ_BASE mSeqs [EN_SEQ_TUNER_CONTROL_NUM]; // entity
};

#endif
