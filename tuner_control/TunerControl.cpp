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
	uint8_t nSectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	nSectId = pIf->getSectId();
	_UTL_LOG_I ("nSectId %d\n", nSectId);

	pIf->reply (EN_THM_RSLT_SUCCESS);

//
// do nothing
//

	nSectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (nSectId, enAct);
}

void CTunerControl::tuneStart (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK_COMPLETE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("sectId %d\n", sectId);

	uint32_t freq = 0;
	static int chkcnt = 0;

	switch (sectId) {
	case SECTID_ENTRY:
		freq = *(pIf->getSrcInfo()->msg.pMsg);
		mFreq = freq;

		getExternalIf()->requestAsync (
			EN_MODULE_TUNE_THREAD, EN_SEQ_TUNE_THREAD_TUNE, (uint8_t*)&freq, sizeof(freq));

		sectId = SECTID_CHECK_COMPLETE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_COMPLETE:
		if (chkcnt > 10) {
			pIf->clearTimeout ();
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		if (it9175_get_state() != EN_IT9175_STATE__TUNED) {
			++ chkcnt ;
			pIf->setTimeout (500);
			sectId = SECTID_CHECK_COMPLETE;
			enAct = EN_THM_ACT_WAIT;
		} else {
			chkcnt = 0;
			pIf->clearTimeout ();
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_END_SUCCESS:
		pIf->reply (EN_THM_RSLT_SUCCESS);
		break;

	case SECTID_END_ERROR:
		pIf->reply (EN_THM_RSLT_ERROR);
		break;

	default:
		break;
	};

	pIf->setSectId (sectId, enAct);
}

void CTunerControl::tuneEnd (CThreadMgrIf *pIf)
{
	uint8_t nSectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	nSectId = pIf->getSectId();
	_UTL_LOG_I ("nSectId %d\n", nSectId);




	pIf->reply (EN_THM_RSLT_SUCCESS);

	nSectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (nSectId, enAct);
}

bool CTunerControl::open (void)
{
	return it9175_open ();
}

void CTunerControl::close (void)
{
	it9175_close ();
}

int CTunerControl::tune (uint32_t freq)
{
	return it9175_tune (freq);
}

void forceTuneEnd (void)
{
	it9175_force_tune_end ();
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
