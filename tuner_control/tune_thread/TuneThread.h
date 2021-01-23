#ifndef _TUNE_THREAD_H_
#define _TUNE_THREAD_H_

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
#include "TsAribCommon.h"


using namespace ThreadManager;

enum {
	EN_SEQ_TUNE_THREAD_MODULE_UP = 0,
	EN_SEQ_TUNE_THREAD_MODULE_DOWN,
	EN_SEQ_TUNE_THREAD_TUNE,

	EN_SEQ_TUNE_THREAD_NUM,
};

class CTuneThread : public CThreadMgrBase, public CGroup
{
public:
	enum class state : int {
		CLOSED = 0,
		OPENED,
		TUNE_BEGINED,
		TUNED,
		TUNE_ENDED,
	};

	typedef struct tune_param {
		uint32_t freq;
		std::mutex * pMutexTsReceiveHandlers;
		CTunerControlIf::ITsReceiveHandler ** pTsReceiveHandlers;
		bool *pIsReqStop;
	} TUNE_PARAM_t;

public:
	CTuneThread (char *pszName, uint8_t nQueNum, uint8_t groupId);
	virtual ~CTuneThread (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);
	void tune (CThreadMgrIf *pIf);

	// direct getter
	state getState (void) {
		return mState;
	}

private:
	state mState;
};

#endif
