#ifndef _THREADMGR_BASE_HH_
#define _THREADMGR_BASE_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>

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
	std::string name;
} SEQ_BASE_t;


class CThreadMgrBase
{
public:
	friend class CThreadMgr;


	CThreadMgrBase (const char *pszName, uint8_t nQueNum);
	CThreadMgrBase (std::string name, uint8_t nQueNum);
	virtual ~CThreadMgrBase (void);


	void exec (EN_THM_DISPATCH_TYPE enType, uint8_t nSeqIdx, ST_THM_IF *pIf);

	CThreadMgrExternalIf *getExternalIf (void) const;
	CThreadMgrIf *getIf (void) const;

	void setIdx (uint8_t idx);
	uint8_t getIdx (void) const;
	const char* getName (void) const;


protected:
	void setSeqs (const SEQ_BASE_t pstSeqs [], uint8_t seqNum);
	void setSeqs (const std::vector<SEQ_BASE_t> &seqs);

	virtual void onCreate (void);
	virtual void onDestroy (void);
	virtual void onReceiveNotify (CThreadMgrIf *pIf);


	bool requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx);
	bool requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);

	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint32_t *pOutReqId);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize, uint32_t *pOutReqId);

	void setRequestOption (uint32_t option);
	uint32_t getRequestOption (void) const;


private:
	void setExternalIf (CThreadMgrExternalIf **pExtIf);
	void setIf (CThreadMgrIf *pIf);


	const SEQ_BASE_t *mpSeqsBase ;

	char mName [16];
	uint8_t mQueNum;

	std::vector<SEQ_BASE_t> mSeqs;
	uint8_t mSeqNum;
	
	CThreadMgrExternalIf **mpExtIf;
	CThreadMgrIf *mpThmIf;

	uint8_t mIdx;
};

} // namespace ThreadManager

#endif
