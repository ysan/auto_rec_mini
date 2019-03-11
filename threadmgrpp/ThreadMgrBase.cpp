#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrBase.h"


namespace ThreadManager {

CThreadMgrBase::CThreadMgrBase (const char *pszName, uint8_t nQueNum)
	:mpSeqsBase (NULL)
	,mQueNum (0)
	,mSeqNum (0)
	,mpExtIf (NULL)
	,mpThmIf (NULL)
{
	if (pszName && (strlen(pszName) > 0)) {
		memset (mName, 0x00, sizeof(mName));
		strncpy (mName, pszName, sizeof(mName));
	} else {
		memset (mName, 0x00, sizeof(mName));
		strncpy (mName, "-----", sizeof(mName));
	}

	mQueNum = nQueNum;
}

CThreadMgrBase::~CThreadMgrBase (void)
{
}


void CThreadMgrBase:: exec (EN_THM_DISPATCH_TYPE enType, uint8_t nSeqIdx, ST_THM_IF *pIf) {

	switch (enType) {
	case EN_THM_DISPATCH_TYPE_CREATE:

		onCreate ();

		break;

	case EN_THM_DISPATCH_TYPE_DESTROY:

		onDestroy ();

		break;

	case EN_THM_DISPATCH_TYPE_REQ_REPLY:
		{
		CThreadMgrIf thmIf (pIf);
		setIf (&thmIf);

		(void) (this->*((mpSeqsBase + nSeqIdx)->pfnSeqBase)) (&thmIf);

		setIf (NULL);
		break;
		}

	case EN_THM_DISPATCH_TYPE_NOTIFY:
		{
		CThreadMgrIf thmIf (pIf);
		setIf (&thmIf);

		onReceiveNotify (&thmIf);

		setIf (NULL);
		break;
		}

	default:
		THM_LOG_E ("BUG: enType is unknown. (CThreadMgrBase::exec)");
		break;
	}
}

void CThreadMgrBase::setSeqs (ST_SEQ_BASE pstSeqs [], uint8_t seqNum)
{
	if (pstSeqs && seqNum > 0) {
		mpSeqsBase = pstSeqs;
		mSeqNum = seqNum;
	}
}

void CThreadMgrBase::onCreate (void)
{
}

void CThreadMgrBase::onDestroy (void)
{
}

void CThreadMgrBase::onReceiveNotify (CThreadMgrIf *pIf)
{
}


bool CThreadMgrBase::requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return false;
	}

	return (*mpExtIf)->requestSync (nThreadIdx, nSeqIdx);
}

bool CThreadMgrBase::requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return false;
	}

	return (*mpExtIf)->requestSync (nThreadIdx, nSeqIdx, pMsg, msgSize);
}

bool CThreadMgrBase::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return false;
	}

	return (*mpExtIf)->requestAsync (nThreadIdx, nSeqIdx);
}

bool CThreadMgrBase::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint32_t *pOutReqId)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return false;
	}

	return (*mpExtIf)->requestAsync (nThreadIdx, nSeqIdx, pOutReqId);
}

bool CThreadMgrBase::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return false;
	}

	return (*mpExtIf)->requestAsync (nThreadIdx, nSeqIdx, pMsg, msgSize);
}

bool CThreadMgrBase::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize, uint32_t *pOutReqId)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return false;
	}

	return (*mpExtIf)->requestAsync (nThreadIdx, nSeqIdx, pMsg, msgSize, pOutReqId);
}

void CThreadMgrBase::setRequestOption (uint32_t option)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return ;
	}

	(*mpExtIf)->setRequestOption (option);
}

uint32_t CThreadMgrBase::getRequestOption (void)
{
	if (!(*mpExtIf)) {
		THM_LOG_E ("BUG: mpExtIf is null. (CThreadMgrBase)");
		return 0;
	}

	return (*mpExtIf)->getRequestOption ();
}

CThreadMgrExternalIf * CThreadMgrBase::getExternalIf (void) const
{
	return (*mpExtIf);
}

void CThreadMgrBase::setExternalIf (CThreadMgrExternalIf **pExtIf)
{
	mpExtIf = pExtIf;
}

CThreadMgrIf * CThreadMgrBase::getIf (void) const
{
	return mpThmIf;
}

void CThreadMgrBase::setIf (CThreadMgrIf *pIf)
{
	mpThmIf = pIf;
}

} // namespace ThreadManager
