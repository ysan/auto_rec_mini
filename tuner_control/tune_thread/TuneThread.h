#ifndef _TUNE_THREAD_H_
#define _TUNE_THREAD_H_

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
#include "it9175_extern.h"


using namespace ThreadManager;

enum {
	EN_SEQ_TUNE_THREAD_START = 0,
	EN_SEQ_TUNE_THREAD_TUNE,
	EN_SEQ_TUNE_THREAD_NUM,
};

class CTuneThread : public CThreadMgrBase
{
public:
	CTuneThread (char *pszName, uint8_t nQueNum);
	virtual ~CTuneThread (void);


	void start (CThreadMgrIf *pIf);
	void tune (CThreadMgrIf *pIf);

private:
	ST_SEQ_BASE mSeqs [EN_SEQ_TUNE_THREAD_NUM]; // entity
};

#endif
