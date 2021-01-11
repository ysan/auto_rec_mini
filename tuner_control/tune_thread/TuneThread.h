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
#include "TunerControlIf.h"
#if 0
#include "it9175_extern.h"
#endif


using namespace ThreadManager;

enum {
	EN_SEQ_TUNE_THREAD_MODULE_UP = 0,
	EN_SEQ_TUNE_THREAD_MODULE_DOWN,
	EN_SEQ_TUNE_THREAD_TUNE,

	EN_SEQ_TUNE_THREAD_NUM,
};

class CTuneThread : public CThreadMgrBase
{
public:
	typedef enum {
		CLOSED = 0,
		OPENED,
		TUNE_BEGINED,
		TUNED,
		TUNE_ENDED,
	} STATE_t;

	typedef struct tune_param {
		uint32_t freq;
		std::mutex * pMutexTsReceiveHandlers;
		CTunerControlIf::ITsReceiveHandler ** pTsReceiveHandlers;
		bool *pIsReqStop;
	} TUNE_PARAM_t;

public:
	CTuneThread (char *pszName, uint8_t nQueNum);
	virtual ~CTuneThread (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);
	void tune (CThreadMgrIf *pIf);

	// direct getter
	STATE_t getState (void) {
		return mState;
	}

private:
	STATE_t mState;
};

#endif
