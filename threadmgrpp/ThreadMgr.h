#ifndef _THREADMGR_HH_
#define _THREADMGR_HH_

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


namespace ThreadManager {

class CThreadMgr
{
public:
	CThreadMgr (void);
	virtual ~CThreadMgr (void);


	static CThreadMgr *getInstance (void);

	bool setup (CThreadMgrBase *pThreads[], int threadNum);
	void teardown (void);

	CThreadMgrExternalIf * getExternalIf (void) const;


private:
	bool registerThreads (CThreadMgrBase *pThreads[], int threadNum);
	void unRegisterThreads (void);


	int mThreadNum;

	ST_THM_REG_TBL *mpRegTbl;
	CThreadMgrExternalIf *mpExtIf;

};

} // namespace ThreadManager

#endif
