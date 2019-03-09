#ifndef _THREADMGR_EXTERNAL_IF_HH_
#define _THREADMGR_EXTERNAL_IF_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "threadmgr_if.h"

using namespace std;


namespace ThreadManager {

class CThreadMgrExternalIf 
{
public:
	explicit CThreadMgrExternalIf (ST_THM_EXTERNAL_IF *pExtIf);
	explicit CThreadMgrExternalIf (CThreadMgrExternalIf *pExtIf);
	virtual ~CThreadMgrExternalIf (void);


	bool requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx);
	bool requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);

	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint32_t *pOutReqId);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);
	bool requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize, uint32_t *pOutReqId);

	void setRequestOption (uint32_t option);
	uint32_t getRequestOption (void);

	bool createExternalCp (void);
	void destroyExternalCp (void);

	ST_THM_SRC_INFO* receiveExternal (void);


private:
	ST_THM_EXTERNAL_IF *mpExtIf;

};

} // namespace ThreadManager

#endif
