#ifndef _TUNER_CONTROL_H_
#define _TUNER_CONTROL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <mutex>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Group.h"
#include "TunerControlIf.h"


using namespace ThreadManager;


// notify category
#define _TUNER_NOTIFY							((uint8_t)0)

class CTunerControl : public CThreadMgrBase, public CGroup
{
public:
	CTunerControl (char *pszName, uint8_t nQueNum, uint8_t groupId);
	virtual ~CTunerControl (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);

	void tune (CThreadMgrIf *pIf);
	void tuneStart (CThreadMgrIf *pIf);
	void tuneStop (CThreadMgrIf *pIf);

	void registerTunerNotify (CThreadMgrIf *pIf);
	void unregisterTunerNotify (CThreadMgrIf *pIf);

	void registerTsReceiveHandler (CThreadMgrIf *pIf);
	void unregisterTsReceiveHandler (CThreadMgrIf *pIf);

	void getState (CThreadMgrIf *pIf);



	std::mutex* getMutexTsReceiveHandlers (void) {
		return &mMutexTsReceiveHandlers;
	}

	CTunerControlIf::ITsReceiveHandler** getTsReceiveHandlers (void) {
		return mpRegTsReceiveHandlers;
	}

private:
	int registerTsReceiveHandler (CTunerControlIf::ITsReceiveHandler *pHandler);
	void unregisterTsReceiveHandler (int id);
	void setState (EN_TUNER_STATE s);


	uint32_t mFreq;
	uint32_t mWkFreq;
	uint32_t mStartFreq;
	int mChkcnt;

	std::mutex mMutexTsReceiveHandlers;
	CTunerControlIf::ITsReceiveHandler *mpRegTsReceiveHandlers [TS_RECEIVE_HANDLER_REGISTER_NUM_MAX];

	EN_TUNER_STATE mState;
	bool mIsReqStop;
};

#endif
