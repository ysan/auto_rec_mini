#ifndef _THREADMGR_IF_HH_
#define _THREADMGR_IF_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "threadmgr_if.h"

using namespace std;


namespace ThreadManager {

class CThreadMgrIf 
{
public:
	explicit CThreadMgrIf (ST_THM_IF *pIf);
	explicit CThreadMgrIf (CThreadMgrIf *pIf);
	virtual ~CThreadMgrIf (void);


	ST_THM_SRC_INFO * getSrcInfo (void) const;

	bool reply (EN_THM_RSLT enRslt);
	bool reply (EN_THM_RSLT enRslt, uint8_t *pMsg, size_t msgSize);

	bool regNotify (uint8_t nCateory, uint8_t *pnClientId);
	bool unregNotify (uint8_t nCateory, uint8_t nClientId);

	bool notify (uint8_t nCateory);
	bool notify (uint8_t nCateory, uint8_t *pMsg, size_t msgSize);

	void setSectId (uint8_t nSectId, EN_THM_ACT enAct);
	uint8_t getSectId (void) const;

	void setTimeout (uint32_t nTimeoutMsec);
	void clearTimeout (void);

	void enableOverwrite (void);
	void disableOverwrite (void);

	void lock (void);
	void unlock (void);

	uint8_t getSeqIdx (void) const;
	const char* getSeqName (void) const;


private:
	ST_THM_IF *mpIf;
};

} // namespace ThreadManager

#endif
