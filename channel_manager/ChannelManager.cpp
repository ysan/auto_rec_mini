#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ChannelManager.h"
#include "modules.h"



CChannelManager::CChannelManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tunerNotify_clientId (0xff)
{
	mSeqs [EN_SEQ_CHANNEL_MANAGER__MODULE_UP] =
		{(PFN_SEQ_BASE)&CChannelManager::onReq_moduleUp,    (char*)"onReq_moduleUp"};
	mSeqs [EN_SEQ_CHANNEL_MANAGER__MODULE_DOWN] =
		{(PFN_SEQ_BASE)&CChannelManager::onReq_moduleDown,  (char*)"onReq_moduleDown"};
	mSeqs [EN_SEQ_CHANNEL_MANAGER__CHANNEL_SCAN] =
		{(PFN_SEQ_BASE)&CChannelManager::onReq_channelScan, (char*)"onReq_channelScan"};
	setSeqs (mSeqs, EN_SEQ_CHANNEL_MANAGER__NUM);

}

CChannelManager::~CChannelManager (void)
{
}


void CChannelManager::onReq_moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:
		sectId = SECTID_REQ_REG_TUNER_NOTIFY;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_REG_TUNER_NOTIFY: {
		CTunerControlIf _if (getExternalIf());
		_if.reqRegisterTunerNotify ();

		sectId = SECTID_WAIT_REG_TUNER_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_TUNER_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_tunerNotify_clientId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_END_SUCCESS:
		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CChannelManager::onReq_channelScan (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_SET_FREQ,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_WAIT_AFTER_TUNE,
		SECTID_CHECK_SERVICE,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	static uint16_t s_ch = UHF_PHYSICAL_CHANNEL_MIN;	
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:

		// 先にreplyしとく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_SET_FREQ;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_SET_FREQ:

		if (s_ch >= UHF_PHYSICAL_CHANNEL_MIN && s_ch <= UHF_PHYSICAL_CHANNEL_MAX) {
			sectId = SECTID_REQ_TUNE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE: {

		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_ch);
		_UTL_LOG_I ("(%s) ##########  pysical channel:[%d] -> freq:[%d]kHz", pIf->getSeqName(), s_ch, freq);

		CTunerControlIf _if (getExternalIf());
		_if.reqTune (freq);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			pIf->setTimeout (7500);
			sectId = SECTID_WAIT_AFTER_TUNE;
			enAct = EN_THM_ACT_WAIT;

		} else {
			_UTL_LOG_W ("skip");
			sectId = SECTID_SET_FREQ;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_WAIT_AFTER_TUNE:
		sectId = SECTID_CHECK_SERVICE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_SERVICE:



		// inc pysical ch
		++ s_ch;

		sectId = SECTID_SET_FREQ;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:
		// reset pysical ch
		s_ch = UHF_PHYSICAL_CHANNEL_MIN;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}
