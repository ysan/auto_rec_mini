#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrIf.h"


namespace ThreadManager {

CThreadMgrIf::CThreadMgrIf (ST_THM_IF *pIf)
{
	if (pIf) {
		mpIf = pIf;
	}
}

CThreadMgrIf::CThreadMgrIf (CThreadMgrIf *pIf)
{
	if (pIf) {
		if (pIf->mpIf) {
			mpIf = pIf->mpIf;
		}
	}
}

CThreadMgrIf::~CThreadMgrIf (void)
{
}


ST_THM_SRC_INFO * CThreadMgrIf::getSrcInfo (void) const
{
	if (mpIf) {
		return mpIf->pstSrcInfo;
	} else {
		return NULL;
	}
}

bool CThreadMgrIf::reply (EN_THM_RSLT enRslt)
{
	if (mpIf) {
		return mpIf->pfnReply (enRslt, NULL, 0);
	} else {
		return false;
	}
}

bool CThreadMgrIf::reply (EN_THM_RSLT enRslt, uint8_t *pMsg, size_t msgSize)
{
	if (mpIf) {
		return mpIf->pfnReply (enRslt, pMsg, msgSize);
	} else {
		return false;
	}
}

bool CThreadMgrIf::regNotify (uint8_t nCategory, uint8_t *pnClientId)
{
	if (mpIf) {
		return mpIf->pfnRegNotify (nCategory, pnClientId);
	} else {
		return false;
	}
}

bool CThreadMgrIf::unregNotify (uint8_t nCategory, uint8_t nClientId)
{
	if (mpIf) {
		return mpIf->pfnUnRegNotify (nCategory, nClientId);
	} else {
		return false;
	}
}

bool CThreadMgrIf::notify (uint8_t nCategory)
{
	if (mpIf) {
		return mpIf->pfnNotify (nCategory, NULL, 0);
	} else {
		return false;
	}
}

bool CThreadMgrIf::notify (uint8_t nCategory, uint8_t *pMsg, size_t msgSize)
{
	if (mpIf) {
		return mpIf->pfnNotify (nCategory, pMsg, msgSize);
	} else {
		return false;
	}
}

void CThreadMgrIf::setSectId (uint8_t nSectId, EN_THM_ACT enAct)
{
	if (mpIf) {
		mpIf->pfnSetSectId (nSectId, enAct);
	}
}

uint8_t CThreadMgrIf::getSectId (void) const
{
	if (mpIf) {
		return mpIf->pfnGetSectId ();
	} else {
		return THM_SECT_ID_INIT;
	}
}

void CThreadMgrIf::setTimeout (uint32_t nTimeoutMsec)
{
	if (mpIf) {
		mpIf->pfnSetTimeout (nTimeoutMsec);
	}
}

void CThreadMgrIf::clearTimeout (void)
{
	if (mpIf) {
		mpIf->pfnClearTimeout ();
	}
}

void CThreadMgrIf::enableOverwrite (void)
{
	if (mpIf) {
		mpIf->pfnEnableOverwrite ();
	}
}

void CThreadMgrIf::disableOverwrite (void)
{
	if (mpIf) {
		mpIf->pfnDisableOverwrite ();
	}
}

void CThreadMgrIf::lock (void)
{
	if (mpIf) {
		mpIf->pfnLock ();
	}
}

void CThreadMgrIf::unlock (void)
{
	if (mpIf) {
		mpIf->pfnUnlock ();
	}
}

uint8_t CThreadMgrIf::getSeqIdx (void) const
{
	if (mpIf) {
		return mpIf->pfnGetSeqIdx ();
	} else {
//TODO SEQ_IDX_BLANK
		return 0x80;
	}
}

const char* CThreadMgrIf::getSeqName (void) const
{
	if (mpIf) {
		return mpIf->pfnGetSeqName ();
	} else {
		return NULL;
	}
}

} // namespace ThreadManager
