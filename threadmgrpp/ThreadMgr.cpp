#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgr.h"


namespace ThreadManager {

/*
 * Constant define
 */
// ここは threadmgr.c の宣言と同じ値にする
#define THREAD_IDX_MAX					(0x20) // 32  uint8の為 255以下の値で THREAD_IDX_BLANKより小さい値
#define SEQ_IDX_MAX						(0x40) // 64  uint8の為 255以下の値で SEQ_IDX_BLANKより小さい値
#define QUE_WORKER_MIN					(5)
#define QUE_WORKER_MAX					(100)


static void dispatcher (
	EN_THM_DISPATCH_TYPE enType,
	uint8_t nThreadIdx,
	uint8_t nSeqIdx,
	ST_THM_IF *pIf
);


CThreadMgrBase *gpThreads [THREAD_IDX_MAX];


CThreadMgr::CThreadMgr (void)
	:mThreadNum (0)
	,mpRegTbl (NULL)
	,mpExtIf (NULL)
{
}

CThreadMgr:: ~CThreadMgr (void)
{
}


CThreadMgr* CThreadMgr::getInstance (void)
{
	static CThreadMgr singleton;
	return &singleton;
}

bool CThreadMgr::registerThreads (CThreadMgrBase *pThreads[], int threadNum)
{
	if ((!pThreads) || (threadNum <= 0)) {
		THM_LOG_E ("invalid argument.\n");
		return false;
	}

	int i = 0;
	while (i < threadNum) {
		if (i == THREAD_IDX_MAX) {
			// over flow
			THM_LOG_E ("thread idx is over. (inner thread table)\n");
			return false;
		}

		if ((pThreads [i]->mQueNum < QUE_WORKER_MIN) && (pThreads [i]->mQueNum > QUE_WORKER_MAX)) {
			THM_LOG_E ("que num is invalid. (inner thread table)\n");
			return false;
		}

		if ((pThreads [i]->mSeqNum <= 0) && (pThreads [i]->mSeqNum > SEQ_IDX_MAX)) {
			THM_LOG_E ("func idx is invalid. (inner thread table)\n");
			return false;
		}

		gpThreads [i] = pThreads [i];
		++ i;
	}

	return true;
}

void CThreadMgr::unRegisterThreads (void)
{
	int i = 0;
	while (i < THREAD_IDX_MAX) {
		gpThreads [i] = NULL;
		++ i;
	}
}

bool CThreadMgr::setup (CThreadMgrBase *pThreads[], int threadNum)
{
	if (!registerThreads (pThreads, threadNum)) {
		THM_LOG_E ("registerThreads() is failure.\n");
		return false;
	}

	mThreadNum = threadNum;

	setupDispatcher (dispatcher);


	mpRegTbl = (ST_THM_REG_TBL*) malloc (sizeof(ST_THM_REG_TBL) * mThreadNum);
	if (!mpRegTbl) {
		THM_PERROR ("malloc");
		return false;
	}
	for (int i = 0; i < mThreadNum; ++ i) {
		ST_THM_REG_TBL *p = mpRegTbl + i;

		p->pszName = gpThreads[i]->mName;
		const_cast <PCB_CREATE&> (p->pcbCreate) = NULL;				// not use
		const_cast <PCB_DESTROY&> (p->pcbDestroy) = NULL;			// not use
		p->nQueNum = gpThreads[i]->mQueNum;
		p->pstSeqArray = NULL;										// set after
		p->nSeqNum = gpThreads[i]->mSeqNum;
		const_cast <PCB_RECV_NOTIFY&> (p->pcbRecvNotify) = NULL;	// not use


		// set seq name
		ST_THM_SEQ *pThmSeq = (ST_THM_SEQ*) malloc (sizeof(ST_THM_SEQ) * gpThreads[i]->mSeqNum);
		if (!pThmSeq) {
			THM_PERROR ("malloc");
			return false;
		}
		for (int j = 0; j < gpThreads[i]->mSeqNum; ++ j) {
			ST_THM_SEQ *p = pThmSeq + j;
			const_cast <PCB_THM_SEQ&> (p->pcbSeq) = NULL;
			p->pszName = ((gpThreads[i]->mpSeqsBase) + j)->pszName;
		}
		p->pstSeqArray = pThmSeq;


		// threadMgrExternalIf
		gpThreads[i]->mpExtIf = &mpExtIf;
	}


	ST_THM_EXTERNAL_IF* pExtIf = setupThreadMgr (mpRegTbl, (uint32_t)mThreadNum);
	if (!pExtIf) {
		THM_LOG_E ("setupThreadMgr() is failure.\n");
		return false;
	}

	mpExtIf = new CThreadMgrExternalIf (pExtIf);
	if (!mpExtIf) {
		THM_LOG_E ("mpExtIf is null.\n");
		return false;
	}

	return true;
}

void CThreadMgr::teardown (void) {
	if (mpExtIf) {
		delete mpExtIf;
		mpExtIf = NULL;
	}

	unRegisterThreads ();

	if (mpRegTbl) {
		for (int i = 0; i < mThreadNum; ++ i) {
			ST_THM_REG_TBL *p = mpRegTbl + i;
			ST_THM_SEQ* s = const_cast <ST_THM_SEQ*> (p->pstSeqArray);
			free (s);
		}

		free (mpRegTbl);
		mpRegTbl = NULL;
	}

	mThreadNum = 0;

	teardownThreadMgr ();
}

CThreadMgrExternalIf * CThreadMgr::getExternalIf (void) const
{
	return mpExtIf;
}

static void dispatcher (
	EN_THM_DISPATCH_TYPE enType,
	uint8_t nThreadIdx,
	uint8_t nSeqIdx,
	ST_THM_IF *pIf
) {
	CThreadMgrBase *pBase = gpThreads [nThreadIdx];
	if (!pBase) {
		THM_LOG_E ("gpThreads [%d] is null.", nThreadIdx);
		return ;
	}

	///////////////////////////////

	pBase->exec (enType, nSeqIdx, pIf);

	///////////////////////////////
}

} // namespace ThreadManager
