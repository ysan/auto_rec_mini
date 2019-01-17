#ifndef _THREADMGR_BASE_HH_
#define _THREADMGR_BASE_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"


namespace ThreadManager {

class CThreadMgr;
class CThreadMgrBase;
typedef void (CThreadMgrBase:: *PFN_SEQ_BASE) (CThreadMgrIf *pIf);

typedef struct _seq_base {
	PFN_SEQ_BASE pfnSeqBase;
	char *pszName;
} ST_SEQ_BASE;


class CThreadMgrBase
{
public:
	friend class CThreadMgr;


	CThreadMgrBase (const char *pszName, uint8_t nQueNum);
	virtual ~CThreadMgrBase (void);


	void exec (EN_THM_DISPATCH_TYPE enType, uint8_t nSeqIdx, ST_THM_IF *pIf);

	CThreadMgrExternalIf *getExternalIf (void);
	CThreadMgrIf *getIf (void);

protected:
	void setSeqs (ST_SEQ_BASE pstSeqs [], uint8_t seqNum);

	virtual void onCreate (void);
	virtual void onDestroy (void);
	virtual void onReceiveNotify (CThreadMgrIf *pIf);


	bool requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx);
	bool requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);

	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint32_t *pOutReqId);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize, uint32_t *pOutReqId);


private:
	ST_SEQ_BASE *mpSeqsBase ;

	char mName [16];
	uint8_t mQueNum;
	uint8_t mSeqNum;
	
	CThreadMgrExternalIf **mpExtIf;
	CThreadMgrIf *mpThmIf;
};

} // namespace ThreadManager

#endif
