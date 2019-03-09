#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"


namespace ThreadManager {

CThreadMgrExternalIf::CThreadMgrExternalIf (ST_THM_EXTERNAL_IF *pExtIf)
{
	if (pExtIf) {
		mpExtIf = pExtIf;
	}
}

CThreadMgrExternalIf::CThreadMgrExternalIf (CThreadMgrExternalIf *pExtIf)
{
	if (pExtIf) {
		if (pExtIf->mpExtIf) {
			mpExtIf = pExtIf->mpExtIf;
		}
	}
}

CThreadMgrExternalIf::~CThreadMgrExternalIf (void)
{
}


bool CThreadMgrExternalIf::requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx)
{
	if (mpExtIf) {
		return mpExtIf->pfnRequestSync (nThreadIdx, nSeqIdx, NULL, 0);
	} else {
		return false;
	}
}

bool CThreadMgrExternalIf::requestSync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pszMsg, size_t msgSize)
{
	if (mpExtIf) {
		return mpExtIf->pfnRequestSync (nThreadIdx, nSeqIdx, pszMsg, msgSize);
	} else {
		return false;
	}
}

bool CThreadMgrExternalIf::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx)
{
	if (mpExtIf) {
		return mpExtIf->pfnRequestAsync (nThreadIdx, nSeqIdx, NULL, 0, NULL);
	} else {
		return false;
	}
}

bool CThreadMgrExternalIf::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint32_t *pOutReqId)
{
	if (mpExtIf) {
		return mpExtIf->pfnRequestAsync (nThreadIdx, nSeqIdx, NULL, 0, pOutReqId);
	} else {
		return false;
	}
}

bool CThreadMgrExternalIf::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pszMsg, size_t msgSize)
{
	if (mpExtIf) {
		return mpExtIf->pfnRequestAsync (nThreadIdx, nSeqIdx, pszMsg, msgSize, NULL);
	} else {
		return false;
	}
}

bool CThreadMgrExternalIf::requestAsync (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pszMsg, size_t msgSize, uint32_t *pOutReqId)
{
	if (mpExtIf) {
		return mpExtIf->pfnRequestAsync (nThreadIdx, nSeqIdx, pszMsg, msgSize, pOutReqId);
	} else {
		return false;
	}
}

void CThreadMgrExternalIf::setRequestOption (uint32_t option)
{
	if (mpExtIf) {
		mpExtIf->pfnSetRequestOption (option);
	}
}

uint32_t CThreadMgrExternalIf::getRequestOption (void)
{
	if (mpExtIf) {
		return mpExtIf->pfnGetRequestOption ();
	} else {
		return 0;
	}
}

bool CThreadMgrExternalIf::createExternalCp (void)
{
	if (mpExtIf) {
		return mpExtIf->pfnCreateExternalCp ();
	} else {
		return false;
	}
}

void CThreadMgrExternalIf::destroyExternalCp (void)
{
	if (mpExtIf) {
		return mpExtIf->pfnDestroyExternalCp ();
	}
}

ST_THM_SRC_INFO* CThreadMgrExternalIf::receiveExternal (void)
{
	if (mpExtIf) {
		return mpExtIf->pfnReceiveExternal ();
	} else {
		return NULL;
	}
}

} // namespace ThreadManager
