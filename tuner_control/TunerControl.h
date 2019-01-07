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

#define TUNER_CALLBACK_NUM		(10)

class CTunerControl : public CThreadMgrBase
{
public:
	class ITunerCallbacks {
	public:
		virtual ~ITunerCallbacks (void) {};
		virtual bool onPreReceive (void *p_shared_data) = 0;
		virtual void onPostReceive (void *p_shared_data) = 0;
		virtual bool onCheckReceiveLoop (void *p_shared_data) = 0;
		virtual bool onReceiveFromTuner (void *p_shared_data, void *p_recv_data, int length) = 0;
	};

public:
	CTunerControl (char *pszName, uint8_t nQueNum);
	virtual ~CTunerControl (void);


	void start (CThreadMgrIf *pIf);
	void tuneStart (CThreadMgrIf *pIf);
	void tuneEnd (CThreadMgrIf *pIf);


	static bool open (void);
	static void close (void);
	static int tune (uint32_t freq);
	static void forceTuneEnd (void);
	static void *tuneThread (void *args);

	uint32_t mFreq;
	pthread_t mTuneThreadId;

private:
	// it9175 callbacks
	static bool onPreReceive (void *p_shared_data);
	static void onPostReceive (void *p_shared_data);
	static bool onCheckReceiveLoop (void *p_shared_data);
	static bool onReceiveFromTuner (void *p_shared_data, void *p_recv_data, int length);
	static void * mpSharedData;
	ST_IT9175_SETUP_INFO m_it9175_setupInfo;

	ITunerCallbacks *mpCallbacks [TUNER_CALLBACK_NUM];

	ST_SEQ_BASE mSeqs [EN_SEQ_TUNER_CONTROL_NUM]; // entity
};

#endif