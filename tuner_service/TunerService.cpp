#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TunerService.h"
#include "modules.h"

#include "Settings.h"


CTunerService::CTunerService (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tuner_resource_max (0)
	,m_next_allocate_tuner_id (0)
{
	SEQ_BASE_t seqs [CTunerServiceIf::sequence::NUM] = {
		{(PFN_SEQ_BASE)&CTunerService::onReq_moduleUp, (char*)"onReq_moduleUp"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_moduleDown, (char*)"onReq_moduleDown"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_open, (char*)"onReq_open"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_close, (char*)"onReq_close"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_tune, (char*)"onReq_tune"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_tune_withRetry, (char*)"onReq_tune_withRetry"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_tuneStop, (char*)"onReq_tuneStop"},
		{(PFN_SEQ_BASE)&CTunerService::onReq_dumpAllocates, (char*)"onReq_dumpAllocates"},
	};
	setSeqs (seqs, CTunerServiceIf::sequence::NUM);

}

CTunerService::~CTunerService (void)
{
}


void CTunerService::onReq_moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	// settingsを使って初期化する場合はmodule upで
	m_tuner_resource_max = CSettings::getInstance()->getParams()->getTunerHalAllocates()->size();
	_UTL_LOG_I ("m_tuner_resource_max:[%d]", m_tuner_resource_max);
	m_resource_allcates.clear();
	m_resource_allcates.resize(m_tuner_resource_max);
	for (auto &a : m_resource_allcates) {
		a = make_shared<resource_t>();
	}


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CTunerService::onReq_open (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	uint8_t caller_module = pIf->getSrcInfo()->nThreadIdx;
//	uint8_t allcated_tuner_id = allocateLinear ((EN_MODULE)caller_module);
	uint8_t allcated_tuner_id = allocateRoundRobin ((EN_MODULE)caller_module);
	if (allcated_tuner_id != 0xff) {
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&allcated_tuner_id, sizeof(allcated_tuner_id));
	} else {
//TODO 割り当てできなかった時はpriorityに応じた処理をする  notifyしたり...
		pIf->reply (EN_THM_RSLT_ERROR);
	}

	dumpAllocates ();


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_close (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	uint8_t tuner_id = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
	if (release (tuner_id)) {
		pIf->reply (EN_THM_RSLT_SUCCESS);
	} else {
		pIf->reply (EN_THM_RSLT_ERROR);
	}

	dumpAllocates ();


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_tune (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_CHECK_PSISI_STATE,
		SECTID_CHECK_WAIT_PSISI_STATE,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	static CTunerServiceIf::tune_param_t s_param;
	static int s_retry = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY: {
		pIf->lock();

		memset (&s_param, 0x00, sizeof(s_param));
		s_retry = 0;

		uint8_t caller_module = pIf->getSrcInfo()->nThreadIdx;
		if (caller_module == EN_MODULE_TUNER_SERVICE) {
			// tune_withRetryから呼ばれました
			_tune_param *p = (_tune_param*)(pIf->getSrcInfo()->msg.pMsg);
			s_param.physical_channel = p->physical_channel;
			s_param.tuner_id = p->tuner_id;
			caller_module = p->caller_module;
		} else {
			s_param = *(CTunerServiceIf::tune_param_t*)(pIf->getSrcInfo()->msg.pMsg);
		}

		if (preCheck (s_param.tuner_id, (EN_MODULE)caller_module, true)) {
			sectId = SECTID_REQ_TUNE;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		dumpAllocates ();

		}
		break;

	case SECTID_REQ_TUNE: {

		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_param.physical_channel);
		_UTL_LOG_I ("freq=[%d]kHz\n", freq);

		CTunerControlIf _if (getExternalIf(), s_param.tuner_id);
		_if.reqTune (freq);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_CHECK_PSISI_STATE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_W ("tune is failure");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_CHECK_PSISI_STATE:

		pIf->setTimeout(200);
		sectId = SECTID_CHECK_WAIT_PSISI_STATE;
		enAct = EN_THM_ACT_WAIT;

		break;

	case SECTID_CHECK_WAIT_PSISI_STATE: {

		CPsisiManagerIf _if(getExternalIf(), s_param.tuner_id);
		_if.reqGetStateSync ();

		EN_PSISI_STATE _psisiState = *(EN_PSISI_STATE*)(pIf->getSrcInfo()->msg.pMsg);
		if (_psisiState == EN_PSISI_STATE__READY) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {

			if (s_retry > 75) {
				// 200ms * 75 = 約15秒 でタイムアウトします
				_UTL_LOG_E ("psi/si state invalid. (never transitioned EN_PSISI_STATE__READY)");
				sectId = SECTID_REQ_TUNE_STOP;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				++ s_retry ;
				sectId = SECTID_CHECK_PSISI_STATE;
				enAct = EN_THM_ACT_CONTINUE;
			}
		}

		}
		break;

	case SECTID_REQ_TUNE_STOP: {
		// 念の為停止しておきます
		CTunerControlIf _if (getExternalIf(), s_param.tuner_id);
		_if.reqTuneStop ();

		sectId = SECTID_WAIT_TUNE_STOP;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
		// とくに結果はみません
		sectId = SECTID_END_ERROR;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END_SUCCESS:
		pIf->unlock();

		m_resource_allcates[s_param.tuner_id]->is_now_tuned = true;
		dumpAllocates ();

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:
		pIf->unlock();

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_tune_withRetry (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//	static CTunerServiceIf::tune_param_t s_param;
	static struct _tune_param s_param;
	static int s_retry = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY: {

		memset (&s_param, 0x00, sizeof(s_param));
		s_retry = 3;

		CTunerServiceIf::tune_param_t *p = (CTunerServiceIf::tune_param_t*)(pIf->getSrcInfo()->msg.pMsg);
		s_param.physical_channel = p->physical_channel;
		s_param.tuner_id = p->tuner_id;
		s_param.caller_module = pIf->getSrcInfo()->nThreadIdx;


		sectId = SECTID_REQ_TUNE;
		enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE:

		requestAsync (
			EN_MODULE_TUNER_SERVICE,
			CTunerServiceIf::sequence::TUNE,
			(uint8_t*)&s_param,
			sizeof (s_param)
		);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			if (s_retry > 0) {
				_UTL_LOG_W ("retry tune");
				sectId = SECTID_REQ_TUNE;
				enAct = EN_THM_ACT_CONTINUE;
				-- s_retry;
			} else {
				_UTL_LOG_E ("retry over...");
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}
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

void CTunerService::onReq_tuneStop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static uint8_t s_tuner_id = 0;

	switch (sectId) {
	case SECTID_ENTRY: {
		pIf->lock();

		s_tuner_id = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
		uint8_t caller_module = pIf->getSrcInfo()->nThreadIdx;
		if (preCheck (s_tuner_id, (EN_MODULE)caller_module, false)) {
			sectId = SECTID_REQ_TUNE_STOP;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		dumpAllocates ();

		}
		break;

	case SECTID_REQ_TUNE_STOP: {
		CTunerControlIf _if (getExternalIf(), s_tuner_id);
		_if.reqTuneStop ();

		sectId = SECTID_WAIT_TUNE_STOP;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
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
		pIf->unlock();

		m_resource_allcates[s_tuner_id]->is_now_tuned = false;
		dumpAllocates ();

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:
		pIf->unlock();

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_dumpAllocates (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	dumpAllocates ();


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

uint8_t CTunerService::allocateLinear (EN_MODULE module)
{
	uint8_t _id = 0;
	// 未割り当てを探します
	for (const auto &a : m_resource_allcates) {
		if (a->tuner_id == 0xff) {
			break;
		}
		++ _id;
	}

	if (_id < m_tuner_resource_max) {
		m_resource_allcates[_id]->tuner_id = _id;
		m_resource_allcates[_id]->module = module;
		m_resource_allcates[_id]->priority = getPriority (module);
		_UTL_LOG_I ("allocated. [0x%02x]", _id);
		return _id;

	} else {
		_UTL_LOG_E ("resources full...");
		return 0xff;
	}
}

uint8_t CTunerService::allocateRoundRobin (EN_MODULE module)
{
	uint8_t _init_id = m_next_allocate_tuner_id;
	int n = 0;

	while (1) {
		if (_init_id == m_next_allocate_tuner_id && n != 0) {
			break;
		}

		uint8_t _id = m_next_allocate_tuner_id;

		if (m_resource_allcates[_id]->tuner_id == 0xff) {
			m_resource_allcates[_id]->tuner_id = _id;
			m_resource_allcates[_id]->module = module;
			m_resource_allcates[_id]->priority = getPriority (module);
			_UTL_LOG_I ("allocated. [0x%02x]", _id);

			//set next
			++ m_next_allocate_tuner_id;
			if (m_next_allocate_tuner_id == m_tuner_resource_max) {
				m_next_allocate_tuner_id = 0;
			}

			return _id;

		} else {
			//set next
			++ m_next_allocate_tuner_id;
			if (m_next_allocate_tuner_id == m_tuner_resource_max) {
				m_next_allocate_tuner_id = 0;
			}
		}

		++ n;
	}

	_UTL_LOG_E ("resources full...");
	return 0xff;
}

bool CTunerService::release (uint8_t tuner_id)
{
	if (tuner_id < m_tuner_resource_max) {
		if (m_resource_allcates[tuner_id]->is_now_tuned) {
			_UTL_LOG_E ("not yet tuned... [0x%02x]", tuner_id);
			return false;

		} else {
			m_resource_allcates[tuner_id]->clear();
			_UTL_LOG_I ("released. [0x%02x]", tuner_id);
			return true;
		}

	} else {
		_UTL_LOG_E ("invalid tuner_id. [0x%02x]", tuner_id);
		return false;
	}
}

bool CTunerService::preCheck (uint8_t tuner_id, EN_MODULE module, bool is_req_tune) const
{
	if (tuner_id < m_tuner_resource_max) {
		if (
			(m_resource_allcates[tuner_id]->tuner_id == tuner_id) &&
			(m_resource_allcates[tuner_id]->module == module)
		) {
			if (is_req_tune) {
				if (!m_resource_allcates[tuner_id]->is_now_tuned) {
					return true;
				} else {
					_UTL_LOG_E ("already tuned... [0x%02x]", tuner_id);
					return false;
				}
			} else {
				// req tune stop
				if (m_resource_allcates[tuner_id]->is_now_tuned) {
					return true;
				} else {
					_UTL_LOG_E ("not tuned... [0x%02x]", tuner_id);
					return false;
				}
			}

		} else {
			_UTL_LOG_E ("not allocated tuner_id. [0x%02x]", tuner_id);
			return false;
		}

	} else {
		_UTL_LOG_E ("invalid tuner_id. [0x%02x]", tuner_id);
		return false;
	}
}

CTunerService::client_priority CTunerService::getPriority (EN_MODULE module) const
{
	switch (module) {
	case EN_MODULE_REC_MANAGER:
		return client_priority::RECORDING;

	case EN_MODULE_EVENT_SCHEDULE_MANAGER:
		return client_priority::EVENT_SCHEDULE;

	case EN_MODULE_CHANNEL_MANAGER:
		return client_priority::CHANNEL_SCAN;

	case EN_MODULE_COMMAND_SERVER:
		return client_priority::COMMAND;

	default:
		return client_priority::OTHER;
	}
}

void CTunerService::dumpAllocates (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	for (const auto &a : m_resource_allcates) {
		a->dump();
	}
}
