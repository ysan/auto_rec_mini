#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrBase.h"


namespace ThreadManager {

CThreadMgrBase::CThreadMgrBase (const char *pszName, uint8_t nQueNum)
//	:mpfnSeqsBase (NULL)
	:mpSeqsBase (NULL)
	,mQueNum (0)
	,mSeqNum (0)
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
//		(void) (this->*mpfnSeqsBase [nSeqIdx]) (&thmIf);
		(void) (this->*((mpSeqsBase + nSeqIdx)->pfnSeqBase)) (&thmIf);
		break;
		}

	case EN_THM_DISPATCH_TYPE_NOTIFY:
		{
		CThreadMgrIf thmIf (pIf);
		onReceiveNotify (&thmIf);
		break;
		}

	default:
		THM_LOG_E ("BUG: enType is unknown. (CThreadMgrBase::exec)");
		break;
	}
}

//void CThreadMgrBase::setSeqs (PFN_SEQ_BASE pfnSeqs [], uint8_t seqNum)
//{
//	if (pfnSeqs && seqNum > 0) {
//		mpfnSeqsBase = pfnSeqs;
//		mSeqNum = seqNum;
//	}
//}

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

CThreadMgrExternalIf * CThreadMgrBase::getExternalIf (void)
{
	return (*mpExtIf);
}


} // namespace ThreadManager
