#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TunerControl.h"
#include "TuneThread.h"
#include "modules.h"


CTunerControl::CTunerControl (char *pszName, uint8_t nQueNum, uint8_t groupId)
	:CThreadMgrBase (pszName, nQueNum)
	,CGroup (groupId)
	,mFreq (0)
	,mWkFreq (0)
	,mStartFreq (0)
	,mChkcnt (0)
	,mState (EN_TUNER_STATE__TUNE_STOP)
	,mIsReqStop (false)
{
	SEQ_BASE_t seqs [EN_SEQ_TUNER_CONTROL_NUM] = {
		{(PFN_SEQ_BASE)&CTunerControl::moduleUp, (char*)"moduleUp"},                                     // EN_SEQ_TUNER_CONTROL_MODULE_UP
		{(PFN_SEQ_BASE)&CTunerControl::moduleDown, (char*)"moduleDown"},                                 // EN_SEQ_TUNER_CONTROL_MODULE_DOWN
		{(PFN_SEQ_BASE)&CTunerControl::tune, (char*)"tune"},                                             // EN_SEQ_TUNER_CONTROL_TUNE
		{(PFN_SEQ_BASE)&CTunerControl::tuneStart, (char*)"tuneStart"},                                   // EN_SEQ_TUNER_CONTROL_TUNE_START
		{(PFN_SEQ_BASE)&CTunerControl::tuneStop, (char*)"tuneStop"},                                     // EN_SEQ_TUNER_CONTROL_TUNE_STOP
		{(PFN_SEQ_BASE)&CTunerControl::registerTunerNotify, (char*)"registerTunerNotify"},               // EN_SEQ_TUNER_CONTROL_REG_TUNER_NOTIFY
		{(PFN_SEQ_BASE)&CTunerControl::unregisterTunerNotify, (char*)"unregisterTunerNotify"},           // EN_SEQ_TUNER_CONTROL_UNREG_TUNER_NOTIFY
		{(PFN_SEQ_BASE)&CTunerControl::registerTsReceiveHandler, (char*)"registerTsReceiveHandler"},     // EN_SEQ_TUNER_CONTROL_REG_TS_RECEIVE_HANDLER
		{(PFN_SEQ_BASE)&CTunerControl::unregisterTsReceiveHandler, (char*)"unregisterTsReceiveHandler"}, // EN_SEQ_TUNER_CONTROL_UNREG_TS_RECEIVE_HANDLER
		{(PFN_SEQ_BASE)&CTunerControl::getState, (char*)"getState"},                                     // EN_SEQ_TUNER_CONTROL_GET_STATE
	};
	setSeqs (seqs, EN_SEQ_TUNER_CONTROL_NUM);

	for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
		mpRegTsReceiveHandlers [i] = NULL;
	}
}

CTunerControl::~CTunerControl (void)
{
}


void CTunerControl::moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_TUNE_THREAD_MODULE_UP,
		SECTID_WAIT_TUNE_THREAD_MODULE_UP,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	switch (sectId) {
	case SECTID_ENTRY:
		sectId = SECTID_REQ_TUNE_THREAD_MODULE_UP;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_TUNE_THREAD_MODULE_UP:
		requestAsync (EN_MODULE_TUNE_THREAD + getGroupId(), EN_SEQ_TUNE_THREAD_MODULE_UP);

		sectId = SECTID_WAIT_TUNE_THREAD_MODULE_UP;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_TUNE_THREAD_MODULE_UP:
		sectId = SECTID_END;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:
		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CTunerControl::moduleDown (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerControl::tune (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_REQ_TUNE_START,
		SECTID_WAIT_TUNE_START,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY:
		mWkFreq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("freq [%d]kHz\n", mWkFreq);
		if (mFreq == mWkFreq) {
			_UTL_LOG_I ("already freq [%d]kHz\n", mWkFreq);
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_REQ_TUNE_STOP;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE_STOP:
		requestAsync (EN_MODULE_TUNER_CONTROL + getGroupId(), EN_SEQ_TUNER_CONTROL_TUNE_STOP);
		sectId = SECTID_WAIT_TUNE_STOP;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_TUNE_STOP:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_REQ_TUNE_START;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE_START:
		requestAsync (EN_MODULE_TUNER_CONTROL + getGroupId(), EN_SEQ_TUNER_CONTROL_TUNE_START, (uint8_t*)&mWkFreq, sizeof(mWkFreq));
		sectId = SECTID_WAIT_TUNE_START;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_TUNE_START:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_END_SUCCESS:
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->reply (EN_THM_RSLT_SUCCESS);
		break;

	case SECTID_END_ERROR:
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->reply (EN_THM_RSLT_ERROR);
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CTunerControl::tuneStart (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_TUNE_THREAD_TUNE,
		SECTID_WAIT_TUNE_THREAD_TUNE,
		SECTID_CHECK_TUNED,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY: {
		// lock
		pIf->lock();

		setState (EN_TUNER_STATE__TUNING_BEGIN);

		// fire notify
		EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNING_BEGIN;
		pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));

		mStartFreq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);

		sectId = SECTID_REQ_TUNE_THREAD_TUNE;
		enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE_THREAD_TUNE: {
		CTuneThread::TUNE_PARAM_t _param = {
			mStartFreq,
			&mMutexTsReceiveHandlers,
			mpRegTsReceiveHandlers,
			&mIsReqStop
		};
		requestAsync (EN_MODULE_TUNE_THREAD + getGroupId(),
						EN_SEQ_TUNE_THREAD_TUNE, (uint8_t*)&_param, sizeof(_param));

		sectId = SECTID_WAIT_TUNE_THREAD_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE_THREAD_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_CHECK_TUNED;
			mChkcnt = 0;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_CHECK_TUNED:
		if (mChkcnt > 40) {
			pIf->clearTimeout ();
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			EN_MODULE _id = (EN_MODULE)(EN_MODULE_TUNE_THREAD + getGroupId());
			CTuneThread *p_tuneThread = (CTuneThread*)(getModule(_id));
			if (p_tuneThread->getState() != CTuneThread::state::TUNED) {
					++ mChkcnt ;
				pIf->setTimeout (200);
				sectId = SECTID_CHECK_TUNED;
				enAct = EN_THM_ACT_WAIT;
			} else {
				pIf->clearTimeout ();
				sectId = SECTID_END_SUCCESS;
				enAct = EN_THM_ACT_CONTINUE;
			}
		}
		break;

	case SECTID_END_SUCCESS: {
		// unlock
		pIf->unlock();

		setState (EN_TUNER_STATE__TUNING_SUCCESS);

		// fire notify
		EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNING_SUCCESS;
		pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));

		mFreq = mStartFreq;
		mChkcnt = 0;
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->reply (EN_THM_RSLT_SUCCESS);
		}
		break;

	case SECTID_END_ERROR: {
		// unlock
		pIf->unlock();

		setState (EN_TUNER_STATE__TUNING_ERROR_STOP);

		// fire notify
		EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNING_ERROR_STOP;
		pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));

		mChkcnt = 0;
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->reply (EN_THM_RSLT_ERROR);
		}
		break;

	default:
		break;
	};

	pIf->setSectId (sectId, enAct);
}

void CTunerControl::tuneStop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	int chkcnt = 0;

	EN_MODULE _id = (EN_MODULE)(EN_MODULE_TUNE_THREAD + getGroupId());
	CTuneThread *p_tuneThread = (CTuneThread*)(getModule(_id));
	if (p_tuneThread->getState() == CTuneThread::state::TUNED ||
			p_tuneThread->getState() == CTuneThread::state::TUNE_BEGINED) {
		mIsReqStop = true;

		while (chkcnt < 300) {
			if (p_tuneThread->getState() == CTuneThread::state::CLOSED) {
				break;
			}

			usleep (100000);
			++ chkcnt;

			if (chkcnt == 150) {
				_UTL_LOG_W ("request force kill");
				requestAsync (EN_MODULE_TUNE_THREAD + getGroupId(), EN_SEQ_TUNE_THREAD_FORCE_KILL);
			}
		}

		if (chkcnt < 300) {
			_UTL_LOG_I ("success tune stop.");
			_UTL_LOG_I ("chkcnt=[%d]", chkcnt);
			mFreq = 0;
			setState (EN_TUNER_STATE__TUNE_STOP);

			// fire notify
			EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNE_STOP;
			pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));

			pIf->reply (EN_THM_RSLT_SUCCESS);

		} else {
			// 100mS * 300
			_UTL_LOG_E ("tune stop transition failure. (TUNED -> TUNE_ENDED)");
			pIf->reply (EN_THM_RSLT_ERROR);
		}

	} else {
		_UTL_LOG_I ("already tune stoped.");
		setState (EN_TUNER_STATE__TUNE_STOP);

#ifdef _DUMMY_TUNER
		// fire notify
		EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNE_STOP;
		pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));
#endif

		pIf->reply (EN_THM_RSLT_SUCCESS);
	}

	chkcnt = 0;
	mIsReqStop = false;

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerControl::registerTunerNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	uint8_t clientId = 0;
	EN_THM_RSLT enRslt;
	bool rslt = pIf->regNotify (_TUNER_NOTIFY, &clientId);
	if (rslt) {
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		enRslt = EN_THM_RSLT_ERROR;
	}

	_UTL_LOG_I ("registerd clientId=[0x%02x]\n", clientId);

	// clientIdをreply msgで返す 
	pIf->reply (enRslt, (uint8_t*)&clientId, sizeof(clientId));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerControl::unregisterTunerNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_THM_RSLT enRslt;
	// msgからclientIdを取得
	uint8_t clientId = *(pIf->getSrcInfo()->msg.pMsg);
	bool rslt = pIf->unregNotify (_TUNER_NOTIFY, clientId);
	if (rslt) {
		_UTL_LOG_I ("unregisterd clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		_UTL_LOG_E ("failure unregister clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_ERROR;
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerControl::registerTsReceiveHandler (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CTunerControlIf::ITsReceiveHandler *pHandler = *(CTunerControlIf::ITsReceiveHandler**)(pIf->getSrcInfo()->msg.pMsg);
	_UTL_LOG_I ("pHandler %p\n", pHandler);

	int r = registerTsReceiveHandler (pHandler);
	if (r >= 0) {
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&r, sizeof(r));
	} else {
		pIf->reply (EN_THM_RSLT_ERROR);
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerControl::unregisterTsReceiveHandler (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	int client_id = *(int*)(pIf->getSrcInfo()->msg.pMsg);
	unregisterTsReceiveHandler (client_id);


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerControl::getState (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	// reply msgに乗せます
	pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&mState, sizeof(mState));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

int CTunerControl::registerTsReceiveHandler (CTunerControlIf::ITsReceiveHandler *pHandler)
{
	if (!pHandler) {
		_UTL_LOG_E ("pHandler is null.");
		return -1;
	}

	std::lock_guard<std::mutex> lock (mMutexTsReceiveHandlers);

	int i = 0;
	for (i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
		if (!mpRegTsReceiveHandlers [i]) {
			mpRegTsReceiveHandlers [i] = pHandler;
			break;
		}
	}

	if (i == TS_RECEIVE_HANDLER_REGISTER_NUM_MAX) {
		_UTL_LOG_E ("mpRegTsReceiveHandlers is full.");
		return -1;
	} else {
		return i;
	}
}

void CTunerControl::unregisterTsReceiveHandler (int id)
{
	if (id < 0 || id >= TS_RECEIVE_HANDLER_REGISTER_NUM_MAX) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTsReceiveHandlers);

	mpRegTsReceiveHandlers [id] = NULL;
}

void CTunerControl::setState (EN_TUNER_STATE s)
{
	mState = s;
}
