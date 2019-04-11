#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "utils.h"
#include "verstr.h"
#include "message.h"
#ifdef __cplusplus
};
#endif

#include "it9175_extern.h"

#include "RecManager.h"
#include "RecManagerIf.h"

#include "modules.h"

#include "aribstr.h"



CRecManager::CRecManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tunerNotify_clientId (0xff)
	,m_tsReceive_handlerId (-1)
	,m_eventChangeNotify_clientId (0xff)
	,m_recState (EN_REC_STATE__INIT)
{
	mSeqs [EN_SEQ_REC_MANAGER_MODULE_UP]   = {(PFN_SEQ_BASE)&CRecManager::moduleUp,   (char*)"moduleUp"};
	mSeqs [EN_SEQ_REC_MANAGER_MODULE_DOWN] = {(PFN_SEQ_BASE)&CRecManager::moduleDown, (char*)"moduleDown"};
	mSeqs [EN_SEQ_REC_MANAGER_CHECK_LOOP]  = {(PFN_SEQ_BASE)&CRecManager::checkLoop,  (char*)"checkLoop"};
	mSeqs [EN_SEQ_REC_MANAGER_START_REC] =
		{(PFN_SEQ_BASE)&CRecManager::startRecording, (char*)"startRecording"};
	mSeqs [EN_SEQ_REC_MANAGER_START_CURRENT_EVENT_REC] =
		{(PFN_SEQ_BASE)&CRecManager::startCurrentEventRecording, (char*)"startCurrentEventRecording"};
	setSeqs (mSeqs, EN_SEQ_REC_MANAGER_NUM);

}

CRecManager::~CRecManager (void)
{
}


void CRecManager::moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
		SECTID_REQ_REG_HANDLER,
		SECTID_WAIT_REG_HANDLER,
		SECTID_REQ_REG_EVENT_CHANGE_NOTIFY,
		SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
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
			sectId = SECTID_REQ_REG_HANDLER;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_HANDLER: {

		CTunerControlIf::ITsReceiveHandler *p = this;
		CTunerControlIf _if (getExternalIf());
		_if.reqRegisterTsReceiveHandler (&p);

		sectId = SECTID_WAIT_REG_HANDLER;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_HANDLER:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_tsReceive_handlerId = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTsReceiveHandler is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_EVENT_CHANGE_NOTIFY: {
		CPsisiManagerIf _if (getExternalIf());
		_if.reqRegisterEventChangeNotify ();

		sectId = SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_eventChangeNotify_clientId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_CHECK_LOOP;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterEventChangeNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_CHECK_LOOP:
		requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER_CHECK_LOOP);

		sectId = SECTID_WAIT_CHECK_LOOP;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_CHECK_LOOP:
//		enRslt = pIf->getSrcInfo()->enRslt;
//		if (enRslt == EN_THM_RSLT_SUCCESS) {
//
//		} else {
//
//		}
// EN_THM_RSLT_SUCCESSのみ

		sectId = SECTID_END_SUCCESS;
		enAct = EN_THM_ACT_CONTINUE;
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

void CRecManager::moduleDown (CThreadMgrIf *pIf)
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

void CRecManager::checkLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK_RESERVE,
		SECTID_CHECK_RESERVE_WAIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK_RESERVE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_RESERVE:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_RESERVE_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_RESERVE_WAIT:

		// reserve check




		sectId = SECTID_CHECK_RESERVE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CRecManager::startRecording (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);






	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::startCurrentEventRecording (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);






	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReceiveNotify (CThreadMgrIf *pIf)
{
	if (pIf->getSrcInfo()->nClientId == m_tunerNotify_clientId) {

		EN_TUNER_STATE enState = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
		switch (enState) {
		case EN_TUNER_STATE__TUNING_BEGIN:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNING_BEGIN");
			break;

		case EN_TUNER_STATE__TUNING_SUCCESS:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNING_SUCCESS");
			break;

		case EN_TUNER_STATE__TUNING_ERROR_STOP:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNING_ERROR_STOP");
			break;

		case EN_TUNER_STATE__TUNE_STOP:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNE_STOP");
			break;

		default:
			break;
		}


	} else if (pIf->getSrcInfo()->nClientId == m_eventChangeNotify_clientId) {

		PSISI_NOTIFY_EVENT_INFO _info = *(PSISI_NOTIFY_EVENT_INFO*)(pIf->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("!!! event chenged !!!");
		_info.dump ();

	}

}

bool CRecManager::setReserve (PSISI_EVENT_INFO *p_info)
{
	if (!p_info) {
		return false;
	}

	bool r = setReserve (
					p_info->transport_stream_id,
					p_info->original_network_id,
					p_info->service_id,
					p_info->event_id,
					&(p_info->start_time),
					&(p_info->end_time),
					p_info->event_name_char
				);

	return r;
}

bool CRecManager::setReserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id,
	CEtime* p_start_time,
	CEtime* p_end_time,
	char *psz_title_name
)
{
	if (!p_start_time || !p_end_time) {
		return false;
	}


	CRecReserve tmp;
	tmp.set (
		_transport_stream_id,
		_original_network_id,
		_service_id,
		_event_id,
		p_start_time,
		p_end_time,
		psz_title_name
	);


	if (!isExistEmptyReserve ()) {
		_UTL_LOG_E ("reserve full.");
		return false;
	}

	if (isDuplicateReserve (&tmp)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	if (isOverrapTimeReserve (&tmp)) {
		_UTL_LOG_E ("reserve time is Overrap.");
		return false;
	}


	CRecReserve* p_reserve = searchAscendingOrderReserve (p_start_time);
	if (!p_reserve) {
		_UTL_LOG_E ("reserve full.");
		return false;
	}

	p_reserve->set (
				_transport_stream_id,
				_original_network_id,
				_service_id,
				_event_id,
				p_start_time,
				p_end_time,
				psz_title_name
			);

	return true;
}

/**
 * 開始時間を基準に降順で空きをさがします
 */
CRecReserve* CRecManager::searchAscendingOrderReserve (CEtime *p_start_time_ref)
{
	if (!p_start_time_ref) {
		return NULL;
	}


	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {

		// 基準時間より後ろの時間を探します
		if (m_reserves [i].start_time > *p_start_time_ref) {

			// 後ろから見てずらします
			for (int j = RESERVE_NUM_MAX; j > i; -- j) {
				m_reserves [j] = m_reserves [j-1] ;
			}

			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		// 見つからなかったので最後尾にします
		return findEmptyReserve ();

	} else {

		m_reserves [i].clear ();
		return &m_reserves [i];
	}
}

bool CRecManager::isExistEmptyReserve (void)
{
	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		_UTL_LOG_W ("m_reserves full.");
		return false;
	}

	return true;
}

CRecReserve* CRecManager::findEmptyReserve (void)
{
	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		_UTL_LOG_W ("m_reserves full.");
		return NULL;
	}

	return &m_reserves [i];
}

bool CRecManager::isDuplicateReserve (CRecReserve* p_reserve)
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves [i] == *p_reserve) {
			// duplicate
			return true;
		}
	}

	return false;
}

bool CRecManager::isOverrapTimeReserve (CRecReserve* p_reserve)
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (
			m_reserves [i].start_time <= p_reserve->start_time &&
			m_reserves [i].end_time >= p_reserve->end_time
		) {
			// start        end
			// |------------|
			//   |-------|
			return true;
		}

		if (
			m_reserves [i].start_time >= p_reserve->start_time &&
			m_reserves [i].end_time <= p_reserve->end_time
		) {
			//   start   end
			//   |-------|
			// |-----------|
			return true;
		}

		if (
			m_reserves [i].start_time <= p_reserve->start_time &&
			m_reserves [i].end_time <= p_reserve->end_time
		) {
			// start        end
			// |------------|
			//   |-------------|
			return true;
		}

		if (
			m_reserves [i].start_time >= p_reserve->start_time &&
			m_reserves [i].end_time >= p_reserve->end_time
		) {
			//    start        end
			//    |------------|
			// |-----------|
			return true;
		}
	}

	return false;
}



//////////  CTunerControlIf::ITsReceiveHandler  //////////

bool CRecManager::onPreTsReceive (void)
{
	getExternalIf()->createExternalCp();

	uint32_t opt = getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	return true;
}

void CRecManager::onPostTsReceive (void)
{
	uint32_t opt = getExternalIf()->getRequestOption ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	getExternalIf()->destroyExternalCp();
}

bool CRecManager::onCheckTsReceiveLoop (void)
{
	return true;
}

bool CRecManager::onTsReceived (void *p_ts_data, int length)
{
	switch (m_recState) {
	case EN_REC_STATE__PRE_PROCESS:
		break;

	case EN_REC_STATE__NOW_RECORDING:

		OutputBuffer_put (NULL, p_ts_data, length);

		break;

	case EN_REC_STATE__POST_PROCESS:
		break;

	default:
		break;
	}


	return true;
}
