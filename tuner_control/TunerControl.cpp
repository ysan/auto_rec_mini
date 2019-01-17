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
{
	mSeqs [EN_SEQ_TUNER_CONTROL_START] = {(PFN_SEQ_BASE)&CTunerControl::start, (char*)"start"};
	mSeqs [EN_SEQ_TUNER_CONTROL_TUNE] = {(PFN_SEQ_BASE)&CTunerControl::tune, (char*)"tune"};
	mSeqs [EN_SEQ_TUNER_CONTROL_TUNE_START] = {(PFN_SEQ_BASE)&CTunerControl::tuneStart, (char*)"tuneStart"};
	mSeqs [EN_SEQ_TUNER_CONTROL_TUNE_STOP] = {(PFN_SEQ_BASE)&CTunerControl::tuneStop, (char*)"tuneStop"};
	setSeqs (mSeqs, EN_SEQ_TUNER_CONTROL_NUM);


	// it9175 callbacks
	memset (&m_it9175_setupInfo, 0x00, sizeof(m_it9175_setupInfo));
	m_it9175_setupInfo.pcb_pre_receive = this->onPreReceive;
	m_it9175_setupInfo.pcb_post_receive = this->onPostReceive;
	m_it9175_setupInfo.pcb_check_receive_loop = this->onCheckReceiveLoop;
	m_it9175_setupInfo.pcb_receive_from_tuner = this->onReceiveFromTuner;
	m_it9175_setupInfo.p_shared_data = NULL;
	it9175_extern_setup (&m_it9175_setupInfo);

	for (int i = 0; i < TUNER_CALLBACK_NUM; ++ i) {
		mpCallbacks [i] = NULL;
	}
}

CTunerControl::~CTunerControl (void)
{
}


void CTunerControl::start (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	pIf->reply (EN_THM_RSLT_SUCCESS);

//
// do nothing
//

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
		SECTID_WAIT_TUNE_STOP,
		SECTID_WAIT_TUNE_START,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	uint32_t freq = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY:
		freq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("freq [%d]\n", freq);
		if (mFreq == freq) {
			_UTL_LOG_I ("already freq [%d]\n", freq);
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE_STOP);
			sectId = SECTID_WAIT_TUNE_STOP;
			enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			requestAsync (EN_MODULE_TUNER_CONTROL, EN_SEQ_TUNER_CONTROL_TUNE_START, (uint8_t*)&freq, sizeof(freq));
			sectId = SECTID_WAIT_TUNE_START;
			enAct = EN_THM_ACT_WAIT;

		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
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
		SECTID_WAIT_TUNE_THREAD_TUNE,
		SECTID_CHECK_TUNED,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	uint32_t freq = 0;
	static int chkcnt = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY:
		freq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);

		requestAsync (EN_MODULE_TUNE_THREAD, EN_SEQ_TUNE_THREAD_TUNE, (uint8_t*)&freq, sizeof(freq));

		sectId = SECTID_CHECK_TUNED;
		enAct = EN_THM_ACT_CONTINUE;
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
		if (chkcnt > 20) {
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

	case SECTID_END_SUCCESS:
		mFreq = freq;
		chkcnt = 0;
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->reply (EN_THM_RSLT_SUCCESS);
		break;

	case SECTID_END_ERROR:
		chkcnt = 0;
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->reply (EN_THM_RSLT_ERROR);
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
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

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

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

bool CTunerControl::onPreReceive (void *p_shared_data)
{
	return true;
}

void CTunerControl::onPostReceive (void *p_shared_data)
{
	return;
}

bool CTunerControl::onCheckReceiveLoop (void *p_shared_data)
{
	return true;
}

bool CTunerControl::onReceiveFromTuner (void *p_shared_data, void *p_recv_data, int length)
{
	return true;
}
