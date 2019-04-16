#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TunerControl.h"
#include "TuneThread.h"
#include "modules.h"



CTunerControl::CTunerControl (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,mFreq (0)
	,mState (EN_TUNER_STATE__TUNE_STOP)
{
	mSeqs [EN_SEQ_TUNER_CONTROL_MODULE_UP]   = {(PFN_SEQ_BASE)&CTunerControl::moduleUp,   (char*)"moduleUp"};
	mSeqs [EN_SEQ_TUNER_CONTROL_MODULE_DOWN] = {(PFN_SEQ_BASE)&CTunerControl::moduleDown, (char*)"moduleDown"};
	mSeqs [EN_SEQ_TUNER_CONTROL_TUNE]        = {(PFN_SEQ_BASE)&CTunerControl::tune,       (char*)"tune"};
	mSeqs [EN_SEQ_TUNER_CONTROL_TUNE_START]  = {(PFN_SEQ_BASE)&CTunerControl::tuneStart,  (char*)"tuneStart"};
	mSeqs [EN_SEQ_TUNER_CONTROL_TUNE_STOP]   = {(PFN_SEQ_BASE)&CTunerControl::tuneStop,   (char*)"tuneStop"};
	mSeqs [EN_SEQ_TUNER_CONTROL_REG_TUNER_NOTIFY] =
		{(PFN_SEQ_BASE)&CTunerControl::registerTunerNotify, (char*)"registerTunerNotify"};
	mSeqs [EN_SEQ_TUNER_CONTROL_UNREG_TUNER_NOTIFY] = 
		{(PFN_SEQ_BASE)&CTunerControl::unregisterTunerNotify, (char*)"unregisterTunerNotify"};
	mSeqs [EN_SEQ_TUNER_CONTROL_REG_TS_RECEIVE_HANDLER] =
		{(PFN_SEQ_BASE)&CTunerControl::registerTsReceiveHandler, (char*)"registerTsReceiveHandler"};
	mSeqs [EN_SEQ_TUNER_CONTROL_UNREG_TS_RECEIVE_HANDLER] = 
		{(PFN_SEQ_BASE)&CTunerControl::unregisterTsReceiveHandler, (char*)"unregisterTsReceiveHandler"};
	mSeqs [EN_SEQ_TUNER_CONTROL_GET_STATE] = 
		{(PFN_SEQ_BASE)&CTunerControl::getState, (char*)"getState"};
	setSeqs (mSeqs, EN_SEQ_TUNER_CONTROL_NUM);


	// it9175 ts callbacks
	memset (&m_it9175_ts_callbacks, 0x00, sizeof(m_it9175_ts_callbacks));
	m_it9175_ts_callbacks.pcb_pre_ts_receive = this->onPreTsReceive;
	m_it9175_ts_callbacks.pcb_post_ts_receive = this->onPostTsReceive;
	m_it9175_ts_callbacks.pcb_check_ts_receive_loop = this->onCheckTsReceiveLoop;
	m_it9175_ts_callbacks.pcb_ts_received = this->onTsReceived;
	m_it9175_ts_callbacks.p_shared_data = this;
	it9175_extern_setup_ts_callbacks (&m_it9175_ts_callbacks);

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
		requestAsync (EN_MODULE_TUNE_THREAD, EN_SEQ_TUNE_THREAD_MODULE_UP);

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

	static uint32_t freq = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY:
		freq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("freq [%d]kHz\n", freq);
		if (mFreq == freq) {
			_UTL_LOG_I ("already freq [%d]kHz\n", freq);
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_REQ_TUNE_STOP;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE_STOP:
		requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE_STOP);
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
		requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE_START, (uint8_t*)&freq, sizeof(freq));
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

	static uint32_t s_freq = 0;
	static int chkcnt = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY: {
		// lock
		pIf->lock();

		setState (EN_TUNER_STATE__TUNING_BEGIN);

		// fire notify
		EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNING_BEGIN;
		pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));

		s_freq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);

		sectId = SECTID_REQ_TUNE_THREAD_TUNE;
		enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE_THREAD_TUNE:
		requestAsync (EN_MODULE_TUNE_THREAD, EN_SEQ_TUNE_THREAD_TUNE, (uint8_t*)&s_freq, sizeof(s_freq));

		sectId = SECTID_WAIT_TUNE_THREAD_TUNE;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_TUNE_THREAD_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_CHECK_TUNED;
			chkcnt = 0;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_CHECK_TUNED:
		if (chkcnt > 40) {
			pIf->clearTimeout ();
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;

		} else {

			if (it9175_get_state() != EN_IT9175_STATE__TUNED) {
				++ chkcnt ;
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

		mFreq = s_freq;
		chkcnt = 0;
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

		chkcnt = 0;
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

	static int chkcnt = 0;

	if (it9175_get_state() == EN_IT9175_STATE__TUNED) {
		it9175_force_tune_end ();

		while (chkcnt < 20) {
			if (it9175_get_state() == EN_IT9175_STATE__TUNE_ENDED) {
				break;
			}

			usleep (200000);
			++ chkcnt;
		}

		if (chkcnt < 20) {
			mFreq = 0;
			it9175_close ();
			pIf->reply (EN_THM_RSLT_SUCCESS);
		} else {
			_UTL_LOG_E ("transition failure. (EN_IT9175_STATE__TUNED -> EN_IT9175_STATE__TUNE_ENDED)");
			pIf->reply (EN_THM_RSLT_ERROR);
		}

	} else {
		it9175_close ();
		pIf->reply (EN_THM_RSLT_SUCCESS);
	}

	chkcnt = 0;

	setState (EN_TUNER_STATE__TUNE_STOP);

	// fire notify
	EN_TUNER_STATE enNotify = EN_TUNER_STATE__TUNE_STOP;
	pIf->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));

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



//////////  it9175 ts callbacks  //////////

// static 
bool CTunerControl::onPreTsReceive (void *p_shared_data)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	if (p_shared_data) {
		CTunerControl *p_ctl = static_cast<CTunerControl*> (p_shared_data);

		std::lock_guard<std::mutex> lock (*p_ctl->getMutexTsReceiveHandlers ());

		CTunerControlIf::ITsReceiveHandler **p_handlers = p_ctl->getTsReceiveHandlers ();
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->onPreTsReceive ();
			}
		}
	}

	return true;
}

// static 
void CTunerControl::onPostTsReceive (void *p_shared_data)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	if (p_shared_data) {
		CTunerControl *p_ctl = static_cast<CTunerControl*> (p_shared_data);

		std::lock_guard<std::mutex> lock (*p_ctl->getMutexTsReceiveHandlers ());

		CTunerControlIf::ITsReceiveHandler **p_handlers = p_ctl->getTsReceiveHandlers ();
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->onPostTsReceive ();
			}
		}
	}
}

// static 
bool CTunerControl::onCheckTsReceiveLoop (void *p_shared_data)
{
	if (p_shared_data) {
		CTunerControl *p_ctl = static_cast<CTunerControl*> (p_shared_data);

		std::lock_guard<std::mutex> lock (*p_ctl->getMutexTsReceiveHandlers ());

		CTunerControlIf::ITsReceiveHandler **p_handlers = p_ctl->getTsReceiveHandlers ();
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				if (!(p_handlers[i])->onCheckTsReceiveLoop()) {
					return false;
				}
			}	
		}
	}

	return true;
}

// static 
bool CTunerControl::onTsReceived (void *p_shared_data, void *p_ts_data, int length)
{
	if (p_shared_data) {
		CTunerControl *p_ctl = static_cast<CTunerControl*> (p_shared_data);

		std::lock_guard<std::mutex> lock (*p_ctl->getMutexTsReceiveHandlers ());

		CTunerControlIf::ITsReceiveHandler **p_handlers = p_ctl->getTsReceiveHandlers ();
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				if (!((p_handlers[i])->onTsReceived (p_ts_data, length))) {
					return false;
				}
			}
		}
	}

	return true;
}
