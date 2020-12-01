#ifndef _THREADMGR_HH_
#define _THREADMGR_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"
#include "ThreadMgrBase.h"


namespace ThreadManager {

class CThreadMgr
{
public:
	static CThreadMgr *getInstance (void);

	bool setup (CThreadMgrBase *pThreads[], int threadNum);
	bool setup (std::vector<CThreadMgrBase*> &threads);
	void wait (void);
	void teardown (void);

	CThreadMgrExternalIf * getExternalIf (void) const;


private:
	// singleton
	CThreadMgr (void);
	virtual ~CThreadMgr (void);

	bool registerThreads (CThreadMgrBase *pThreads[], int threadNum);
	void unRegisterThreads (void);

	bool setup (void);


	std::vector <CThreadMgrBase*> mThreads;
	int mThreadNum;

	ST_THM_REG_TBL *mpRegTbl;
	CThreadMgrExternalIf *mpExtIf;
};

} // namespace ThreadManager

#endif
