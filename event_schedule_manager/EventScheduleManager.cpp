#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "EventScheduleManager.h"
#include "EventScheduleManagerIf.h"

#include "Settings.h"

#include "modules.h"

#include "aribstr.h"


CEventScheduleManager::CEventScheduleManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tunerNotify_clientId (0xff)
	,m_eventChangeNotify_clientId (0xff)
	,mp_EIT_H_sched (NULL)
{
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_UP] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_moduleUp,                  (char*)"onReq_moduleUp"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_DOWN] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_moduleDown,                (char*)"onReq_moduleDown"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__CHECK_LOOP] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_checkLoop,                 (char*)"onReq_checkLoop"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__PARSER_NOTICE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_parserNotice,              (char*)"onReq_parserNotice"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__START_CACHE_CURRENT_SERVICE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_startCache_currentService, (char*)"onReq_startCache_currentService"};
	setSeqs (mSeqs, EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM);


	m_lastUpdate_EITSched.clear();
	m_startTime_EITSched.clear();

	m_event_map.clear ();
}

CEventScheduleManager::~CEventScheduleManager (void)
{
	m_lastUpdate_EITSched.clear();
	m_startTime_EITSched.clear();

	m_event_map.clear ();
}


void CEventScheduleManager::onReq_moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
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
			sectId = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
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
		requestAsync (EN_MODULE_EVENT_SCHEDULE_MANAGER, EN_SEQ_EVENT_SCHEDULE_MANAGER__CHECK_LOOP);

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

void CEventScheduleManager::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CEventScheduleManager::onReq_checkLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_EVENT_INFO s_presentEventInfo;


	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT:

		sectId = SECTID_CHECK;
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

void CEventScheduleManager::onReq_parserNotice (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_PSISI_TYPE _type = *(EN_PSISI_TYPE*)(pIf->getSrcInfo()->msg.pMsg);
	switch (_type) {
	case EN_PSISI_TYPE__EIT_H_SCHED:

		// EITのshceduleの更新時間保存
		// 受け取り中はかなりの頻度でたたかれるはず
		m_lastUpdate_EITSched.setCurrentTime ();

		break;

	default:
		break;
	};

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_startCache_currentService (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_TUNER_STATE,
		SECTID_WAIT_GET_TUNER_STATE,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_ENABLE_PARSE_EIT_SCHED,
		SECTID_WAIT_ENABLE_PARSE_EIT_SCHED,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_REQ_DISABLE_PARSE_EIT_SCHED,
		SECTID_WAIT_DISABLE_PARSE_EIT_SCHED,
		SECTID_CACHE,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_SERVICE_INFO s_serviceInfos[10];


	switch (sectId) {
	case SECTID_ENTRY:

		// 時間がかかるはずなので先にリプライしときます
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_REQ_GET_TUNER_STATE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_TUNER_STATE: {
		CTunerControlIf _if (getExternalIf());
		_if.reqGetState ();

		sectId = SECTID_WAIT_GET_TUNER_STATE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_TUNER_STATE: {

		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			EN_TUNER_STATE _state = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
			if (_state == EN_TUNER_STATE__TUNING_SUCCESS) {
				sectId = SECTID_REQ_GET_SERVICE_INFOS;
				enAct = EN_THM_ACT_CONTINUE;
			} else {
				_UTL_LOG_E ("not EN_TUNER_STATE__TUNING_SUCCESS %d", _state);
#ifdef _DUMMY_TUNER
				sectId = SECTID_REQ_GET_SERVICE_INFOS;
#else
//TODO 暫定エラー処理なし
				sectId = SECTID_END;
#endif
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			// success only
		}

		}
		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		CPsisiManagerIf _if (getExternalIf());
		_if.reqGetCurrentServiceInfos (s_serviceInfos, 10);

		sectId = SECTID_WAIT_GET_SERVICE_INFOS;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			int num = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			if (num > 0) {
//s_serviceInfos[0].dump();
				sectId = SECTID_REQ_ENABLE_PARSE_EIT_SCHED;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
//TODO 暫定エラー処理なし
				_UTL_LOG_E ("reqGetCurrentServiceInfos is 0");
				sectId = SECTID_END;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
//TODO 暫定エラー処理なし
			_UTL_LOG_E ("reqGetCurrentServiceInfos err");
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_ENABLE_PARSE_EIT_SCHED: {
		CPsisiManagerIf _if (getExternalIf());
		_if.reqEnableParseEITSched ();

		sectId = SECTID_WAIT_ENABLE_PARSE_EIT_SCHED;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_ENABLE_PARSE_EIT_SCHED: {
//		enRslt = pIf->getSrcInfo()->enRslt;
//		if (enRslt == EN_THM_RSLT_SUCCESS) {
//
//		} else {
//
//		}
// EN_THM_RSLT_SUCCESSのみ

		CEventInformationTable_sched *p = (CEventInformationTable_sched*)(pIf->getSrcInfo()->msg.pMsg);
//p->dumpTables_simple();

		m_startTime_EITSched.setCurrentTime();

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;

		}
		break;

	case SECTID_CHECK:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_WAIT: {

		// 20秒間更新がなかったら完了とします
		CEtime tcur;
		tcur.setCurrentTime();
		CEtime ttmp = m_lastUpdate_EITSched;
		ttmp.addSec (20);

		if (tcur > ttmp) {
			_UTL_LOG_I ("EIT schedule complete");
			sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			// 開始から10分経過していたらタイムアウトします
			ttmp = m_startTime_EITSched;
			ttmp.addMin (10);
			if (tcur > ttmp) {
				_UTL_LOG_E ("EIT schedule timeout");
//TODO 暫定エラー処理なし
				sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				sectId = SECTID_CHECK;
				enAct = EN_THM_ACT_CONTINUE;
			}
		}

		}
		break;

	case SECTID_REQ_DISABLE_PARSE_EIT_SCHED: {
		CPsisiManagerIf _if (getExternalIf());
		_if.reqDisableParseEITSched ();

		sectId = SECTID_WAIT_DISABLE_PARSE_EIT_SCHED;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_DISABLE_PARSE_EIT_SCHED:
//		enRslt = pIf->getSrcInfo()->enRslt;
//		if (enRslt == EN_THM_RSLT_SUCCESS) {
//
//		} else {
//
//		}
// EN_THM_RSLT_SUCCESSのみ

		sectId = SECTID_CACHE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CACHE:







		sectId = SECTID_END;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReceiveNotify (CThreadMgrIf *pIf)
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

//		PSISI_NOTIFY_EVENT_INFO _info = *(PSISI_NOTIFY_EVENT_INFO*)(pIf->getSrcInfo()->msg.pMsg);
//		_UTL_LOG_I ("!!! event chenged !!!");
//		_info.dump ();

	}

}
