#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <algorithm>

#include "EventScheduleManager.h"
#include "EventScheduleManagerIf.h"

#include "Settings.h"

#include "modules.h"

#include "aribstr.h"


bool _comp__table_id (const CEvent* l, const CEvent* r) {
	if (r->table_id > l->table_id) {
		return true;
	}

	return false;
}

bool _comp__section_number (const CEvent* l, const CEvent* r) {
	if (r->table_id == l->table_id) {
		if (r->section_number > l->section_number) {
			return true;
		}
	}

	return false;
}


CEventScheduleManager::CEventScheduleManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tunerNotify_clientId (0xff)
	,m_eventChangeNotify_clientId (0xff)
	,mp_EIT_H_sched (NULL)
{
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_UP] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_moduleUp,                      (char*)"onReq_moduleUp"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_DOWN] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_moduleDown,                    (char*)"onReq_moduleDown"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__CHECK_LOOP] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_checkLoop,                     (char*)"onReq_checkLoop"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__PARSER_NOTICE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_parserNotice,                  (char*)"onReq_parserNotice"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__START_CACHE_SCHEDULE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_startCacheSchedule,            (char*)"onReq_startCacheSchedule"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE_CURRENT_SERVICE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_cacheSchedule_currentService,  (char*)"onReq_cacheSchedule_currentService"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getEvent,                      (char*)"onReq_getEvent"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT_LATEST_DUMPED_SCHEDULE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getEvent_latestDumpedSchedule, (char*)"onReq_getEvent_latestDumpedSchedule"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE_MAP] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpScheduleMap,               (char*)"onReq_dumpScheduleMap"};
	mSeqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE] =
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpSchedule,                  (char*)"onReq_dumpSchedule"};
	setSeqs (mSeqs, EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM);


	m_lastUpdate_EITSched.clear();
	m_startTime_EITSched.clear();

	m_sched_map.clear ();

	m_latest_dumped_key.clear();

}

CEventScheduleManager::~CEventScheduleManager (void)
{
	m_lastUpdate_EITSched.clear();
	m_startTime_EITSched.clear();

	m_sched_map.clear ();
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

void CEventScheduleManager::onReq_startCacheSchedule (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_ENABLE_PARSE_EIT_SCHED,
		SECTID_WAIT_ENABLE_PARSE_EIT_SCHED,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_REQ_DISABLE_PARSE_EIT_SCHED,
		SECTID_WAIT_DISABLE_PARSE_EIT_SCHED,
		SECTID_CACHE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static CEventScheduleManagerIf::SERVICE_KEY_t s_service_key;
	static uint16_t s_ch = 0;
	static PSISI_SERVICE_INFO s_serviceInfos[10];
	static int s_num = 0;


	switch (sectId) {
	case SECTID_ENTRY:

		s_service_key = *(CEventScheduleManagerIf::SERVICE_KEY_t*)(pIf->getSrcInfo()->msg.pMsg);


		// 時間がかかるはずなので先にリプライしときます
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {

		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			s_service_key.transport_stream_id,
			s_service_key.original_network_id,
			s_service_key.service_id
		};

		CChannelManagerIf _if (getExternalIf());
		_if.reqGetPysicalChannelByServiceId (&param);

		sectId = SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			s_ch = *(uint16_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_TUNE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqGetPysicalChannelByServiceId is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE: {
		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_ch);
		_UTL_LOG_I ("freq=[%d]kHz\n", freq);

		CTunerControlIf _if (getExternalIf());
		_if.reqTune (freq);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_REQ_GET_SERVICE_INFOS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_W ("tune is failure  -> not start recording");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
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
			s_num = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			if (s_num > 0) {
//s_serviceInfos[0].dump();
				sectId = SECTID_REQ_ENABLE_PARSE_EIT_SCHED;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				_UTL_LOG_E ("reqGetCurrentServiceInfos is 0");
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("reqGetCurrentServiceInfos err");
			sectId = SECTID_END_ERROR;
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

		Enable_PARSE_EIT_SCHED_REPLY_PARAM_t param = *(Enable_PARSE_EIT_SCHED_REPLY_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);
		mp_EIT_H_sched = param.p_parser;
//mp_EIT_H_sched->dumpTables_simple();
		mEIT_H_sched_ref = mp_EIT_H_sched->reference();


		m_startTime_EITSched.setCurrentTime();

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;

		}
		break;

	case SECTID_CHECK:

		_UTL_LOG_I ("parse EIT schedule ...");
		pIf->setTimeout (2000); // 2sec

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT: {

		// 20秒間更新がなかったら完了とします
		CEtime tcur;
		tcur.setCurrentTime();
		CEtime ttmp = m_lastUpdate_EITSched;
		ttmp.addSec (20);

		if (tcur > ttmp) {
			_UTL_LOG_I ("parse EIT schedule : complete");
			sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			ttmp = m_startTime_EITSched;
			ttmp.addMin (5);
			if (tcur > ttmp) {
				// 開始から5分経過していたらタイムアウトします
				_UTL_LOG_E ("parser EIT schedule : timeout");
				sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				_UTL_LOG_I ("m_lastUpdate_EITSched %s", m_lastUpdate_EITSched.toString());
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

		for (int i = 0; i < s_num; ++ i) {
			s_serviceInfos[i].dump ();

			std::vector <CEvent*> *p_sched = new std::vector <CEvent*>;
			cacheSchedule (
				s_serviceInfos[i].transport_stream_id,
				s_serviceInfos[i].original_network_id,
				s_serviceInfos[i].service_id,
				p_sched
			);

			if (p_sched->size() > 0) {
				SERVICE_KEY_t key (
					s_serviceInfos[i].transport_stream_id,
					s_serviceInfos[i].original_network_id,
					s_serviceInfos[i].service_id
				);
				addScheduleMap (key, p_sched);
				key.dump();
				_UTL_LOG_I ("addScheduleMap -> %d items", p_sched->size());

			} else {
				_UTL_LOG_I ("schedule none.");
				delete p_sched;
			}
		}

		sectId = SECTID_END_SUCCESS;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END_SUCCESS:

		memset (&s_service_key, 0x00, sizeof(s_service_key));
		s_ch = 0;
		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		mp_EIT_H_sched = NULL;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		memset (&s_service_key, 0x00, sizeof(s_service_key));
		s_ch = 0;
		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		mp_EIT_H_sched = NULL;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_cacheSchedule_currentService (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_TUNER_STATE,
		SECTID_WAIT_GET_TUNER_STATE,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_START_CACHE_SCHEDULE,
		SECTID_WAIT_START_CACHE_SCHEDULE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_SERVICE_INFO s_serviceInfos[10];
	static int s_num = 0;


	switch (sectId) {
	case SECTID_ENTRY:


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
				sectId = SECTID_END_ERROR;
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
			s_num = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			if (s_num > 0) {
//s_serviceInfos[0].dump();
				sectId = SECTID_REQ_START_CACHE_SCHEDULE;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				_UTL_LOG_E ("reqGetCurrentServiceInfos is 0");
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("reqGetCurrentServiceInfos err");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_START_CACHE_SCHEDULE: {

		// ここは添字0 でも何でも良いです
		CEventScheduleManagerIf::SERVICE_KEY_t _key = {
			s_serviceInfos[0].transport_stream_id,
			s_serviceInfos[0].original_network_id,
			s_serviceInfos[0].service_id
		};
		requestAsync (
			EN_MODULE_EVENT_SCHEDULE_MANAGER,
			EN_SEQ_EVENT_SCHEDULE_MANAGER__START_CACHE_SCHEDULE,
			(uint8_t*)&_key,
			sizeof(_key)
		);

		sectId = SECTID_WAIT_START_CACHE_SCHEDULE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_START_CACHE_SCHEDULE:
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

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		mp_EIT_H_sched = NULL;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		pIf->reply (EN_THM_RSLT_ERROR);

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		mp_EIT_H_sched = NULL;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_getEvent (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param =
			*(CEventScheduleManagerIf::REQ_EVENT_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {

		const CEvent *p = getEvent (_param.arg.key);
		CEvent* p_event = const_cast <CEvent*> (p);
		if (p_event) {

			_param.p_out_event->table_id = p_event->table_id;
			_param.p_out_event->transport_stream_id = p_event->transport_stream_id;
			_param.p_out_event->original_network_id = p_event->original_network_id;
			_param.p_out_event->service_id = p_event->service_id;

			_param.p_out_event->event_id = p_event->event_id;
			_param.p_out_event->start_time = p_event->start_time;
			_param.p_out_event->end_time = p_event->end_time;

			_param.p_out_event->p_event_name = &p_event->event_name;
			_param.p_out_event->p_text = &p_event->text;

			pIf->reply (EN_THM_RSLT_SUCCESS);

		} else {

			_UTL_LOG_E ("getEvent is null.");
			pIf->reply (EN_THM_RSLT_ERROR);

		}
	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_getEvent_latestDumpedSchedule (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param =
			*(CEventScheduleManagerIf::REQ_EVENT_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {

		const CEvent *p = getEvent (m_latest_dumped_key, _param.arg.index);
		CEvent* p_event = const_cast <CEvent*> (p);
		if (p_event) {

			_param.p_out_event->table_id = p_event->table_id;
			_param.p_out_event->transport_stream_id = p_event->transport_stream_id;
			_param.p_out_event->original_network_id = p_event->original_network_id;
			_param.p_out_event->service_id = p_event->service_id;

			_param.p_out_event->event_id = p_event->event_id;
			_param.p_out_event->start_time = p_event->start_time;
			_param.p_out_event->end_time = p_event->end_time;

			_param.p_out_event->p_event_name = &p_event->event_name;
			_param.p_out_event->p_text = &p_event->text;

			pIf->reply (EN_THM_RSLT_SUCCESS);

		} else {

			_UTL_LOG_E ("getEvent is null.");
			pIf->reply (EN_THM_RSLT_ERROR);

		}
	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_dumpScheduleMap (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	dumpScheduleMap ();


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_dumpSchedule (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CEventScheduleManagerIf::SERVICE_KEY_t _key = *(CEventScheduleManagerIf::SERVICE_KEY_t*)(pIf->getSrcInfo()->msg.pMsg);
	SERVICE_KEY_t key (_key);
	m_latest_dumped_key = key;

	dumpScheduleMap (key);


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
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
//		_UTL_LOG_I ("!!! event changed !!!");
//		_info.dump ();

	}
}

/**
 * 引数をキーにテーブル内容を保存します
 * table_id=0x50,0x51 が対象です
 * 0x50 本日 〜4日目
 * 0x51 5日目〜8日目
 */
void CEventScheduleManager::cacheSchedule (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	std::vector <CEvent*> *p_out_sched
)
{
	if (
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return ;
	}

	if (!p_out_sched) {
		return ;
	}


	std::lock_guard<std::recursive_mutex> lock (*mEIT_H_sched_ref.mpMutex);

	std::vector<CEventInformationTable_sched::CTable*>::const_iterator iter = mEIT_H_sched_ref.mpTables->begin();
	for (; iter != mEIT_H_sched_ref.mpTables->end(); ++ iter) {

		CEventInformationTable_sched::CTable *pTable = *iter;

		// キーが一致したテーブルをみます
		if (
			(_transport_stream_id != pTable->transport_stream_id) ||
			(_original_network_id != pTable->original_network_id) ||
			(_service_id != pTable->header.table_id_extension)
		) {
			continue;
		}

		// table_id=0x50,0x51 が対象です
		if (
			(pTable->header.table_id != TBLID_EIT_SCHED_A) &&
			(pTable->header.table_id != TBLID_EIT_SCHED_A +1)
		) {
			continue;
		}

		// loop events
		std::vector<CEventInformationTable_sched::CTable::CEvent>::const_iterator iter_event = pTable->events.begin();
		for (; iter_event != pTable->events.end(); ++ iter_event) {
			if (iter_event->event_id == 0) {
				// event情報が付いてない
				continue;
			}

			CEvent *p_event = new CEvent ();

			p_event->table_id = pTable->header.table_id;
			p_event->transport_stream_id = pTable->transport_stream_id;
			p_event->original_network_id = pTable->original_network_id;
			p_event->service_id = pTable->header.table_id_extension;

			p_event->section_number = pTable->header.section_number;

			p_event->event_id = iter_event->event_id;

			time_t stime = CTsAribCommon::getEpochFromMJD (iter_event->start_time);
			CEtime wk (stime);
			p_event->start_time = wk;

			int dur_sec = CTsAribCommon::getSecFromBCD (iter_event->duration);
			wk.addSec (dur_sec);
			p_event->end_time = wk;


			std::vector<CDescriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SHORT_EVENT_DESCRIPTOR) {
					CShortEventDescriptor sd (*iter_desc);
					if (!sd.isValid) {
						_UTL_LOG_W ("invalid ShortEventDescriptor");
						continue;
					}

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					AribToString (aribstr, (const char*)sd.event_name_char, (int)sd.event_name_length);
					p_event->event_name = aribstr;

					if (sd.text_length > 0) {
						memset (aribstr, 0x00, MAXSECLEN);
						AribToString (aribstr, (const char*)sd.text_char, (int)sd.text_length);
						p_event->text = aribstr;
					}

				}

			} // loop descriptors


			p_out_sched->push_back (p_event);


		} // loop events

	} // loop tables


	std::stable_sort (p_out_sched->begin(), p_out_sched->end(), _comp__table_id);
	std::stable_sort (p_out_sched->begin(), p_out_sched->end(), _comp__section_number);

}

void CEventScheduleManager::clearSchedule (std::vector <CEvent*> *p_sched)
{
	if (!p_sched || p_sched->size() == 0) {
		return;
	}

	std::vector<CEvent*>::const_iterator iter = p_sched->begin();
	for (; iter != p_sched->end(); ++ iter) {
		CEvent* p = *iter;
		if (p) {
			p->clear();
			delete p;
		}
	}

	p_sched->clear();
}

void CEventScheduleManager::dumpSchedule (const std::vector <CEvent*> *p_sched) const
{
	if (!p_sched || p_sched->size() == 0) {
		return;
	}

	int i = 0;
	std::vector<CEvent*>::const_iterator iter = p_sched->begin();
	for (; iter != p_sched->end(); ++ iter) {
		CEvent* p = *iter;
		if (p) {
			_UTL_LOG_I ("-----------------------------------");
			_UTL_LOG_I ("[[[ %d ]]]", i);
			_UTL_LOG_I ("-----------------------------------");

			p->dump();

			++ i;
		}
	}
}

bool CEventScheduleManager::addScheduleMap (const SERVICE_KEY_t &key, std::vector <CEvent*> *p_sched)
{
	if (!p_sched || p_sched->size() == 0) {
		return false;
	}

	if (hasScheduleMap (key)) {
		_UTL_LOG_I ("hasScheduleMap -> deleteScheduleMap");
		deleteScheduleMap (key);
	}

	m_sched_map.insert (pair<SERVICE_KEY_t, std::vector <CEvent*> *>(key, p_sched));

	return true;
}

void CEventScheduleManager::deleteScheduleMap (const SERVICE_KEY_t &key)
{
	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return ;
	}

	std::vector <CEvent*> *p_sched = iter->second;

	clearSchedule (p_sched);

	delete p_sched;

	m_sched_map.erase (iter);
}

bool CEventScheduleManager::hasScheduleMap (const SERVICE_KEY_t &key) const
{
	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return false;

	} else {
		std::vector <CEvent*> *p_sched = iter->second;
		if (p_sched->size() == 0) {
			return false;
		}
	}

	return true;
}

void CEventScheduleManager::dumpScheduleMap (void) const
{
	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.begin ();

	if (iter == m_sched_map.end()) {
		return ;
	}

	for (; iter != m_sched_map.end(); ++ iter) {
		SERVICE_KEY_t key = iter->first;
		key.dump();

		std::vector <CEvent*> *p_sched = iter->second;
		if (p_sched) {
			_UTL_LOG_I ("  [%d] items", p_sched->size());
		}
	}
}

void CEventScheduleManager::dumpScheduleMap (const SERVICE_KEY_t &key) const
{
	if (!hasScheduleMap (key)) {
		_UTL_LOG_I ("not hasScheduleMap...");
		return ;
	}

	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return ;

	} else {
		std::vector <CEvent*> *p_sched = iter->second;
		if (p_sched->size() == 0) {
			return ;

		} else {
			dumpSchedule (p_sched);
		}
	}
}

const CEvent *CEventScheduleManager::getEvent (const CEventScheduleManagerIf::EVENT_KEY_t &key) const
{
	SERVICE_KEY_t _service_key = {key.transport_stream_id, key.original_network_id, key.service_id};

	if (!hasScheduleMap (_service_key)) {
		_UTL_LOG_I ("not hasScheduleMap...");
		return NULL;
	}


	CEvent *r = NULL;
	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (_service_key);

	if (iter == m_sched_map.end()) {
		return NULL;

	} else {
		std::vector <CEvent*> *p_sched = iter->second;
		if (p_sched->size() == 0) {
			return NULL;

		} else {
			std::vector<CEvent*>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				CEvent* p = *iter_event;
				if (
					(p->transport_stream_id == key.transport_stream_id) &&
					(p->original_network_id == key.original_network_id) &&
					(p->service_id == key.service_id) &&
					(p->event_id == key.event_id)
				) {
					r = p;
					break;
				}
			}
		}
	}

	return r;
}

const CEvent *CEventScheduleManager::getEvent (const SERVICE_KEY_t &key, int index) const
{
	if (!hasScheduleMap (key)) {
		_UTL_LOG_I ("not hasScheduleMap...");
		return NULL;
	}


	CEvent *r = NULL;
	std::map <SERVICE_KEY_t, std::vector <CEvent*> *> ::const_iterator iter = m_sched_map.find (key);

	if (iter == m_sched_map.end()) {
		return NULL;

	} else {
		std::vector <CEvent*> *p_sched = iter->second;
		if (p_sched->size() == 0) {
			return NULL;

		} else {
			int _idx = 0;
			std::vector<CEvent*>::const_iterator iter_event = p_sched->begin();
			for (; iter_event != p_sched->end(); ++ iter_event) {
				if (index == _idx) {
					CEvent* p = *iter_event;
					r = p;
					break;
				}

				++ _idx;
			}
		}
	}

	return r;
}
