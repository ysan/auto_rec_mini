#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <algorithm>

#include "EventScheduleManager.h"
#include "EventScheduleManagerIf.h"

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

bool _comp__reserve_start_time (const CEventScheduleManager::CReserve& l, const CEventScheduleManager::CReserve& r) {
	if (r.start_time > l.start_time) {
		return true;
	}

	return false;
}


CEventScheduleManager::CEventScheduleManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,mp_settings (NULL)
	,m_state (EN_CACHE_SCHEDULE_STATE__INIT)
	,m_tunerNotify_clientId (0xff)
	,m_eventChangeNotify_clientId (0xff)
	,mp_EIT_H_sched (NULL)
	,m_is_need_stop (false)
{
	SEQ_BASE_t seqs [EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM] = {
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_moduleUp, (char*)"onReq_moduleUp"},                                                     // EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_UP
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_moduleDown, (char*)"onReq_moduleDown"},                                                 // EN_SEQ_EVENT_SCHEDULE_MANAGER__MODULE_DOWN
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_registerCacheScheduleStateNotify, (char*)"onReq_registerCacheScheduleStateNotify"},     // EN_SEQ_EVENT_SCHEDULE_MANAGER__REG_CACHE_SCHEDULE_STATE_NOTIFY
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_unregisterCacheScheduleStateNotify, (char*)"onReq_unregisterCacheScheduleStateNotify"}, // EN_SEQ_EVENT_SCHEDULE_MANAGER__UNREG_CACHE_SCHEDULE_STATE_NOTIFY
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getCacheScheduleState, (char*)"onReq_getCacheScheduleState"},                           // EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_CACHE_SCHEDULE_STATE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_checkLoop, (char*)"onReq_checkLoop"},                                                   // EN_SEQ_EVENT_SCHEDULE_MANAGER__CHECK_LOOP
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_parserNotice, (char*)"onReq_parserNotice"},                                             // EN_SEQ_EVENT_SCHEDULE_MANAGER__PARSER_NOTICE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_execCacheSchedule, (char*)"onReq_execCacheSchedule"},                                   // EN_SEQ_EVENT_SCHEDULE_MANAGER__EXEC_CACHE_SCHEDULE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_cacheSchedule, (char*)"onReq_cacheSchedule"},                                           // EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_cacheSchedule_forceCurrentService, (char*)"onReq_cacheSchedule_forceCurrentService"},   // EN_SEQ_EVENT_SCHEDULE_MANAGER__CACHE_SCHEDULE_FORCE_CURRENT_SERVICE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_stopCacheSchedule, (char*)"onReq_stopCacheSchedule"},                                   // EN_SEQ_EVENT_SCHEDULE_MANAGER__STOP_CACHE_SCHEDULE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getEvent, (char*)"onReq_getEvent"},                                                     // EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getEvent_latestDumpedSchedule, (char*)"onReq_getEvent_latestDumpedSchedule"},           // EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENT__LATEST_DUMPED_SCHEDULE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpEvent_latestDumpedSchedule, (char*)"onReq_dumpEvent_latestDumpedSchedule"},         // EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_EVENT__LATEST_DUMPED_SCHEDULE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getEvents_keywordSearch, (char*)"onReq_getEvents_keywordSearch"},                       // EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_getEvents_keywordSearch, (char*)"onReq_getEvents_keywordSearch(ex)"},                   // EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH_EX
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_addReserves, (char*)"onReq_addReserves"},                                               // EN_SEQ_EVENT_SCHEDULE_MANAGER__ADD_RESERVES
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpScheduleMap, (char*)"onReq_dumpScheduleMap"},                                       // EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE_MAP
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpSchedule, (char*)"onReq_dumpSchedule"},                                             // EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_SCHEDULE
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpReserves, (char*)"onReq_dumpReserves"},                                             // EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_RESERVES
		{(PFN_SEQ_BASE)&CEventScheduleManager::onReq_dumpHistories, (char*)"onReq_dumpHistories"},                                           // EN_SEQ_EVENT_SCHEDULE_MANAGER__DUMP_HISTORIES
	};
	setSeqs (seqs, EN_SEQ_EVENT_SCHEDULE_MANAGER__NUM);


	mp_settings = CSettings::getInstance();

	m_lastUpdate_EITSched.clear();
	m_startTime_EITSched.clear();

	m_latest_dumped_key.clear();

	m_schedule_cache_next_day.clear();
	m_schedule_cache_current_plan.clear();

	m_reserves.clear();
	m_retry_reserves.clear();
	m_executing_reserve.clear();

	m_current_history.clear();
	m_current_history_stream.clear();
	m_histories.clear();

	m_container.clear();
}

CEventScheduleManager::~CEventScheduleManager (void)
{
	m_lastUpdate_EITSched.clear();
	m_startTime_EITSched.clear();

	m_latest_dumped_key.clear();

	m_schedule_cache_next_day.clear();
	m_schedule_cache_current_plan.clear();

	m_reserves.clear();
	m_executing_reserve.clear();

	m_current_history.clear();
	m_current_history_stream.clear();
	m_histories.clear();

	m_container.clear();
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

		m_schedule_cache_next_day.setCurrentDay();

		loadHistories ();

		m_container.setScheduleMapJsonPath(*mp_settings->getParams()->getEventScheduleCacheDataJsonPath());
		m_container.loadScheduleMap();

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

void CEventScheduleManager::onReq_registerCacheScheduleStateNotify (CThreadMgrIf *pIf)
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
	bool rslt = pIf->regNotify (NOTIFY_CAT__CACHE_SCHEDULE, &clientId);
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

void CEventScheduleManager::onReq_unregisterCacheScheduleStateNotify (CThreadMgrIf *pIf)
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
	bool rslt = pIf->unregNotify (NOTIFY_CAT__CACHE_SCHEDULE, clientId);
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

void CEventScheduleManager::onReq_getCacheScheduleState (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	// reply msgにm_stateを乗せます
	pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&m_state, sizeof(m_state));


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


	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK:

		pIf->setTimeout (10*1000); // 10sec

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT: {

		// 実行予定日をチェックします
		CEtime t;
		t.setCurrentDay();
		if (t == m_schedule_cache_next_day) {
			// 実行予定日なので予約を入れます
			_UTL_LOG_I ("add reserves base_time => [%s]", m_schedule_cache_next_day.toString());

			int start_hour = mp_settings->getParams()->getEventScheduleCacheStartHour();
			int start_min = mp_settings->getParams()->getEventScheduleCacheStartMin();
			CEtime _base_time = m_schedule_cache_next_day;
			_base_time.addHour (start_hour);
			_base_time.addMin (start_min);


			uint32_t opt = getRequestOption ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);

			requestAsync (
				EN_MODULE_EVENT_SCHEDULE_MANAGER,
				EN_SEQ_EVENT_SCHEDULE_MANAGER__ADD_RESERVES,
				(uint8_t*)&_base_time,
				sizeof(_base_time)
			);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);


			// 次回の予定日セットしておきます
			int interval_day = mp_settings->getParams()->getEventScheduleCacheStartIntervalDay();
			if (interval_day <= 1) {
				// interval_dayは最低でも１日にします
				interval_day = 1;
			}
			m_schedule_cache_next_day.addDay(interval_day);
		}


		if (mp_settings->getParams()->isEnableEventScheduleCache()) {
			// 予約をチェックします
			check2executeReserves ();
		}


		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;

		}
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

void CEventScheduleManager::onReq_execCacheSchedule (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_TUNE_STATE,
		SECTID_WAIT_GET_TUNE_STATE,
		SECTID_REQ_TUNE_BY_SERVICE_ID,
		SECTID_WAIT_TUNE_BY_SERVICE_ID,
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
		SECTID_EXIT,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static CEventScheduleManagerIf::SERVICE_KEY_t s_service_key;
	static PSISI_SERVICE_INFO s_serviceInfos[10];
	static int s_num = 0;
	static bool s_is_timeouted = false;
	static bool s_is_already_using_tuner = false;


	switch (sectId) {
	case SECTID_ENTRY:

		s_service_key = *(CEventScheduleManagerIf::SERVICE_KEY_t*)(pIf->getSrcInfo()->msg.pMsg);

		if (m_executing_reserve.type & CReserve::type_t::S_FLG) {
			// update m_state
			m_state = EN_CACHE_SCHEDULE_STATE__BUSY;

			// history ----------------
			m_current_history.clear();
			m_current_history.set_startTime();

			m_retry_reserves.clear();
		}

		if (m_executing_reserve.type & CReserve::type_t::N_FLG) {
			// fire notify
			pIf->notify (NOTIFY_CAT__CACHE_SCHEDULE, (uint8_t*)&m_state, sizeof(m_state));
		}


		// history ----------------
		{
			CChannelManagerIf::SERVICE_ID_PARAM_t param = {
				s_service_key.transport_stream_id,
				s_service_key.original_network_id,
				0 // no need service_id
			};
			CChannelManagerIf _if (getExternalIf());
			_if.syncGetTransportStreamName (&param);
			char* ts_name = (char*)(pIf->getSrcInfo()->msg.pMsg);
			m_current_history_stream.clear();
			m_current_history_stream.stream_name = ts_name;
		}


		if ((m_executing_reserve.type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::FORCE) {
			sectId = SECTID_REQ_GET_SERVICE_INFOS;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			// NORMAL
			sectId = SECTID_REQ_GET_TUNE_STATE;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_GET_TUNE_STATE: {

		CTunerControlIf _if (getExternalIf());
		_if.reqGetState ();

		sectId = SECTID_WAIT_GET_TUNE_STATE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_GET_TUNE_STATE: {

		EN_TUNER_STATE state = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
		if (state != EN_TUNER_STATE__TUNE_STOP) {
			_UTL_LOG_E ("someone is using a tuner.");
			s_is_already_using_tuner = true;
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_REQ_TUNE_BY_SERVICE_ID;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_REQ_TUNE_BY_SERVICE_ID: {

		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			s_service_key.transport_stream_id,
			s_service_key.original_network_id,
			s_service_key.service_id
		};

		CChannelManagerIf _if (getExternalIf());
		_if.reqTuneByServiceId_withRetry (&param);

		sectId = SECTID_WAIT_TUNE_BY_SERVICE_ID;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE_BY_SERVICE_ID:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_REQ_GET_SERVICE_INFOS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("CChannelManagerIf::reqTuneByServiceId is failure.");
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
				_UTL_LOG_E ("reqGetCurrentServiceInfos  num is 0");
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

		Enable_PARSE_EIT_SCHED_REPLY_PARAM_t param =
					*(Enable_PARSE_EIT_SCHED_REPLY_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

		// parserのインスタンス取得して
		// ここでparser側で積んでるデータは一度クリアします
		mp_EIT_H_sched = param.p_parser;
		mEIT_H_sched_ref = param.p_parser->reference();
		mp_EIT_H_sched->clear();


		m_startTime_EITSched.setCurrentTime();

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;

		}
		break;

	case SECTID_CHECK:

		pIf->setTimeout (5000); // 5sec

		if (m_is_need_stop) {
			_UTL_LOG_W ("parser EIT schedule : cancel");
			sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_CHECK_WAIT;
			enAct = EN_THM_ACT_WAIT;
		}

		break;

	case SECTID_CHECK_WAIT: {

		CEtime tcur;
		tcur.setCurrentTime();
		CEtime ttmp = m_lastUpdate_EITSched;
		ttmp.addSec (10);

		if (tcur > ttmp) {
			// 約10秒間更新がなかったら完了とします
			_UTL_LOG_I ("parse EIT schedule : complete");
			sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			ttmp = m_startTime_EITSched;
			ttmp.addMin (mp_settings->getParams()->getEventScheduleCacheTimeoutMin());
			if (tcur > ttmp) {
				// getEventScheduleCacheTimeoutMin 分経過していたらタイムアウトします
				_UTL_LOG_W ("parser EIT schedule : timeout");
				s_is_timeouted = true;
				sectId = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				_UTL_LOG_I ("parse EIT schedule : m_lastUpdate_EITSched %s", m_lastUpdate_EITSched.toString());
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
				cacheSchedule_extended (p_sched);


				SERVICE_KEY_t key (
					s_serviceInfos[i].transport_stream_id,
					s_serviceInfos[i].original_network_id,
					s_serviceInfos[i].service_id,
					s_serviceInfos[i].service_type,			// additional
					s_serviceInfos[i].p_service_name_char	// additional
				);

				if (m_container.hasScheduleMap (key)) {
					_UTL_LOG_I ("hasScheduleMap -> deleteScheduleMap");
					m_container.deleteScheduleMap (key);
				}

				m_container.addScheduleMap (key, p_sched);
				key.dump();
				_UTL_LOG_I ("addScheduleMap -> %d items", p_sched->size());


				// history ----------------
				CHistory::service svc;
				svc.key = key;
				svc.item_num = p_sched->size();
				m_current_history_stream.services.push_back (svc);


			} else {
				_UTL_LOG_I ("schedule none.");
				delete p_sched;
				p_sched = NULL;
			}
		}

		if (s_is_timeouted || m_is_need_stop) {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_END_SUCCESS:

		pIf->reply (EN_THM_RSLT_SUCCESS);


		// history ----------------
		m_current_history_stream._state = CHistory::EN_STATE__COMPLETE;
		m_current_history.add (m_current_history_stream);


		sectId = SECTID_EXIT;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END_ERROR:

		pIf->reply (EN_THM_RSLT_ERROR);


		// history ----------------
		if (m_is_need_stop) {
			m_current_history_stream._state = CHistory::EN_STATE__CANCEL;
		} else if (s_is_timeouted) {
			m_current_history_stream._state = CHistory::EN_STATE__TIMEOUT;
		} else {
			m_current_history_stream._state = CHistory::EN_STATE__ERROR;
		}
		m_current_history.add (m_current_history_stream);


		if (m_executing_reserve.type & CReserve::type_t::R_FLG) {
			// retry reserve
			CReserve retry;

			CEtime start_time;
			start_time.setCurrentTime();
			int interval = mp_settings->getParams()->getEventScheduleCacheRetryIntervalMin();
			start_time.addMin(interval);

			retry.transport_stream_id = s_service_key.transport_stream_id;
			retry.original_network_id = s_service_key.original_network_id;
			retry.service_id = s_service_key.service_id;
			retry.start_time = start_time;
			retry.type = CReserve::type_t::NORMAL;
			m_retry_reserves.push_back(retry);
		}


		sectId = SECTID_EXIT;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_EXIT:

		if (!s_is_already_using_tuner) {
			//-----------------------------//
			{
				uint32_t opt = getRequestOption ();
				opt |= REQUEST_OPTION__WITHOUT_REPLY;
				setRequestOption (opt);

				// 選局を停止しときます tune stop
				// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
				CChannelManagerIf _if (getExternalIf());
				_if.reqTuneStop ();

				opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
				setRequestOption (opt);
			}
			//-----------------------------//
		}

		if (m_executing_reserve.type & CReserve::type_t::E_FLG) {

			// history ----------------
			m_current_history.set_endTime();

			pushHistories (&m_current_history);
			saveHistories ();
			m_current_history.clear();

			// update m_state
			m_state = EN_CACHE_SCHEDULE_STATE__READY;

			// retry reserve
			int _rs = (int)m_retry_reserves.size();
			_UTL_LOG_I ("m_retry_reserves.size %d", _rs);
			if (_rs == 1) {
				m_retry_reserves[0].type =
					(CReserve::type_t)(CReserve::type_t::NORMAL | CReserve::type_t::S_FLG |
										CReserve::type_t::E_FLG | CReserve::type_t::N_FLG);
				addReserve(m_retry_reserves[0]);
				dumpReserves();
				m_retry_reserves.clear();

			} else if (_rs >= 2) {
				for (int i = 0; i < _rs; ++ i) {
					if (i == 0) {
						m_retry_reserves[i].type =
							(CReserve::type_t)(CReserve::type_t::NORMAL | CReserve::type_t::S_FLG | CReserve::type_t::N_FLG);
					} else if (i == (_rs - 1)) {
						m_retry_reserves[i].type =
							(CReserve::type_t)(CReserve::type_t::NORMAL | CReserve::type_t::E_FLG | CReserve::type_t::N_FLG);
					}
					addReserve(m_retry_reserves[i]);
				}
				dumpReserves();
				m_retry_reserves.clear();
			}

			m_container.saveScheduleMap();
		}

		if (m_executing_reserve.type & CReserve::type_t::N_FLG) {
			// fire notify
			pIf->notify (NOTIFY_CAT__CACHE_SCHEDULE, (uint8_t*)&m_state, sizeof(m_state));
		}

		memset (&s_service_key, 0x00, sizeof(s_service_key));
		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		s_is_timeouted = false;
		s_is_already_using_tuner = false;
		mp_EIT_H_sched = NULL;
		m_executing_reserve.clear();

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;

		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_cacheSchedule (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_ADD_RESERVES,
		SECTID_WAIT_ADD_RESERVES,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;

	switch (sectId) {
	case SECTID_ENTRY:

		sectId = SECTID_REQ_ADD_RESERVES;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_ADD_RESERVES: {

		CEtime _base_time ;
		_base_time.setCurrentTime();

		requestAsync (
			EN_MODULE_EVENT_SCHEDULE_MANAGER,
			EN_SEQ_EVENT_SCHEDULE_MANAGER__ADD_RESERVES,
			(uint8_t*)&_base_time,
			sizeof(_base_time)
		);

		sectId = SECTID_WAIT_ADD_RESERVES;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_ADD_RESERVES:

		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("add reserve failure.");
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

void CEventScheduleManager::onReq_cacheSchedule_forceCurrentService (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_TUNER_STATE,
		SECTID_WAIT_GET_TUNER_STATE,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_ADD_RESERVE,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_SERVICE_INFO s_serviceInfos[10];
	static int s_num = 0;
	static CReserve s_tmp_reserve ;


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
				sectId = SECTID_ADD_RESERVE;
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

	case SECTID_ADD_RESERVE: {

		CEtime start_time;
		start_time.setCurrentTime();

		CReserve::type_t type =
			(CReserve::type_t)(CReserve::type_t::FORCE | CReserve::type_t::S_FLG | CReserve::type_t::E_FLG);

		// ここは添字0 でも何でも良いです
		// (現状ではすべてのチャンネル編成分のスケジュールを取得します)
		s_tmp_reserve.transport_stream_id = s_serviceInfos[0].transport_stream_id;
		s_tmp_reserve.original_network_id = s_serviceInfos[0].original_network_id;
		s_tmp_reserve.service_id = s_serviceInfos[0].service_id;
		s_tmp_reserve.start_time = start_time;
		s_tmp_reserve.type = type;
		addReserve (s_tmp_reserve);


		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;

		}
		break;

	case SECTID_CHECK: {

		pIf->setTimeout (100); // 100ms

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_CHECK_WAIT: {

		if (isExistReserve(s_tmp_reserve)) {
			// まだ予約に入ってます
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;

		} else if (m_executing_reserve == s_tmp_reserve && m_executing_reserve.isValid()) {
			// 只今実行中です
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			// 実行終了しました
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_END_SUCCESS:

		pIf->reply (EN_THM_RSLT_SUCCESS);

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		s_tmp_reserve.clear();

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		pIf->reply (EN_THM_RSLT_ERROR);

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		s_num = 0;
		s_tmp_reserve.clear();

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_stopCacheSchedule (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	switch (sectId) {
	case SECTID_ENTRY:

		m_is_need_stop = true;

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK: {

		pIf->setTimeout (100); // 100ms

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_CHECK_WAIT: {

		if (m_executing_reserve.isValid()) {
			// 只今実行中です
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			// 実行終了しました
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_END:

		pIf->reply (EN_THM_RSLT_SUCCESS);

		m_is_need_stop = false;

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

		SERVICE_KEY_t _service_key = {_param.arg.key.transport_stream_id, _param.arg.key.original_network_id, _param.arg.key.service_id};
		const CEvent *p = m_container.getEvent (_service_key, _param.arg.key.event_id);
		CEvent* p_event = const_cast <CEvent*> (p);
		if (p_event) {

			_param.p_out_event->transport_stream_id = p_event->transport_stream_id;
			_param.p_out_event->original_network_id = p_event->original_network_id;
			_param.p_out_event->service_id = p_event->service_id;

			_param.p_out_event->event_id = p_event->event_id;
			_param.p_out_event->start_time = p_event->start_time;
			_param.p_out_event->end_time = p_event->end_time;

			// アドレスで渡してますが 基本的には schedule casheが走らない限り
			// アドレスは変わらない前提です
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

		const CEvent *p = m_container.getEvent (m_latest_dumped_key, _param.arg.index);
		CEvent* p_event = const_cast <CEvent*> (p);
		if (p_event) {

			_param.p_out_event->transport_stream_id = p_event->transport_stream_id;
			_param.p_out_event->original_network_id = p_event->original_network_id;
			_param.p_out_event->service_id = p_event->service_id;

			_param.p_out_event->event_id = p_event->event_id;
			_param.p_out_event->start_time = p_event->start_time;
			_param.p_out_event->end_time = p_event->end_time;

			// アドレスで渡してますが 基本的には schedule casheが走らない限り
			// アドレスは変わらない前提です
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

void CEventScheduleManager::onReq_dumpEvent_latestDumpedSchedule (CThreadMgrIf *pIf)
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

//_param.p_out_eventはnullで来ます

//	if (!_param.p_out_event) {
//		_UTL_LOG_E ("_param.p_out_event is null.");
//		pIf->reply (EN_THM_RSLT_ERROR);

//	} else {

		const CEvent *p = m_container.getEvent (m_latest_dumped_key, _param.arg.index);
		CEvent* p_event = const_cast <CEvent*> (p);
		if (p_event) {

			p_event->dump();
			p_event->dump_detail ();

			pIf->reply (EN_THM_RSLT_SUCCESS);

		} else {

			_UTL_LOG_E ("getEvent is null.");
			pIf->reply (EN_THM_RSLT_ERROR);

		}
//	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_getEvents_keywordSearch (CThreadMgrIf *pIf)
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

	} else if (_param.array_max_num <= 0) {
		_UTL_LOG_E ("_param.array_max_num is invalid.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {

		bool is_check_extendedEvent = false;
		if (pIf->getSeqIdx() == EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH) {
			is_check_extendedEvent = false;
		} else if (pIf->getSeqIdx() == EN_SEQ_EVENT_SCHEDULE_MANAGER__GET_EVENTS__KEYWORD_SEARCH_EX) {
			is_check_extendedEvent = true;
		}

		int n = m_container.getEvents (_param.arg.p_keyword, _param.p_out_event, _param.array_max_num, is_check_extendedEvent);
		if (n < 0) {
			_UTL_LOG_E ("getEvents is invalid.");
			pIf->reply (EN_THM_RSLT_ERROR);
		} else {
			// 検索数をreply msgで返します
			pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&n, sizeof(n));
		}
	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_addReserves (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_CHANNELS,
		SECTID_WAIT_GET_CHANNELS,
		SECTID_ADD_RESERVES,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static CChannelManagerIf::CHANNEL_t s_channels[20];
	static int s_ch_num = 0;
	static CEtime _base_time;


	switch (sectId) {
	case SECTID_ENTRY:

		_base_time = *(CEtime*)(pIf->getSrcInfo()->msg.pMsg);

		sectId = SECTID_REQ_GET_CHANNELS;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_CHANNELS: {

		memset (s_channels, 0x00, sizeof(s_channels));
		s_ch_num = 0;

		CChannelManagerIf::REQ_CHANNELS_PARAM_t param = {s_channels, 20};

		CChannelManagerIf _if (getExternalIf());
		_if.reqGetChannels (&param);

		sectId = SECTID_WAIT_GET_CHANNELS;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_CHANNELS:

		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			s_ch_num = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			_UTL_LOG_I ("reqGetChannels s_ch_num:[%d]", s_ch_num);

			if (s_ch_num > 0) {
				sectId = SECTID_ADD_RESERVES;
				enAct = EN_THM_ACT_CONTINUE;
			} else {
				_UTL_LOG_E ("reqGetChannels is 0");
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("reqGetChannels is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
        }

		break;

	case SECTID_ADD_RESERVES: {

		// 取得したチャンネル分の予約を入れます
		for (int i = 0; i < s_ch_num; ++ i) {

			if (i != 0) {
				_base_time.addMin(1); // とりあえず 1分おきに
			}
			CEtime _start_time = _base_time;

			CReserve::type_t type = (CReserve::type_t)(CReserve::type_t::NORMAL | CReserve::type_t::R_FLG);
			if (i == 0) {
				type = (CReserve::type_t)(type | CReserve::type_t::S_FLG | CReserve::type_t::N_FLG);
			}
			if (i == s_ch_num -1) {
				type = (CReserve::type_t)(type | CReserve::type_t::E_FLG | CReserve::type_t::N_FLG);
			}

			addReserve (
				s_channels[i].transport_stream_id,
				s_channels[i].original_network_id,
				s_channels[i].service_ids[0],	// ここは添字0 でも何でも良いです
												// (現状ではすべてのチャンネル編成分のスケジュールを取得します)
				&_start_time,
				type
			);
		}

		sectId = SECTID_END_SUCCESS;
		enAct = EN_THM_ACT_CONTINUE;

		}
		break;

	case SECTID_END_SUCCESS:

		dumpReserves();

		pIf->reply (EN_THM_RSLT_SUCCESS);

		memset (s_channels, 0x00, sizeof(s_channels));
		s_ch_num = 0;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		pIf->reply (EN_THM_RSLT_ERROR);

		memset (s_channels, 0x00, sizeof(s_channels));
		s_ch_num = 0;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

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


	m_container.dumpScheduleMap ();


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


	CEventScheduleManagerIf::SERVICE_KEY_t _key =
					*(CEventScheduleManagerIf::SERVICE_KEY_t*)(pIf->getSrcInfo()->msg.pMsg);
	SERVICE_KEY_t key (_key);
	m_latest_dumped_key = key;

	m_container.dumpScheduleMap (key);


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_dumpReserves (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	dumpReserves ();


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReq_dumpHistories (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	dumpHistories ();


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CEventScheduleManager::onReceiveNotify (CThreadMgrIf *pIf)
{
/***
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
		_UTL_LOG_I ("!!! event changed !!!");
		_info.dump ();

	}
***/
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

				} else if (iter_desc->tag == DESC_TAG__COMPONENT_DESCRIPTOR) {
					CComponentDescriptor cd (*iter_desc);
					if (!cd.isValid) {
						_UTL_LOG_W ("invalid ComponentDescriptor");
						continue;
					}

					p_event->component_type = cd.component_type;
					p_event->component_tag = cd.component_tag;

				} else if (iter_desc->tag == DESC_TAG__AUDIO_COMPONENT_DESCRIPTOR) {
					CAudioComponentDescriptor acd (*iter_desc);
					if (!acd.isValid) {
						_UTL_LOG_W ("invalid AudioComponentDescriptor\n");
						continue;
					}

					p_event->audio_component_type = acd.component_type;
					p_event->audio_component_tag = acd.component_tag;
					p_event->ES_multi_lingual_flag = acd.ES_multi_lingual_flag;
					p_event->main_component_flag = acd.main_component_flag;
					p_event->quality_indicator = acd.quality_indicator;
					p_event->sampling_rate = acd.sampling_rate;

				} else if (iter_desc->tag == DESC_TAG__CONTENT_DESCRIPTOR) {
					CContentDescriptor cd (*iter_desc);
					if (!cd.isValid) {
						_UTL_LOG_W ("invalid ContentDescriptor");
						continue;
					}

					std::vector<CContentDescriptor::CContent>::const_iterator iter_con = cd.contents.begin();
					for (; iter_con != cd.contents.end(); ++ iter_con) {
						CEvent::CGenre genre;
						genre.content_nibble_level_1 = iter_con->content_nibble_level_1;
						genre.content_nibble_level_2 = iter_con->content_nibble_level_2;
						p_event->genres.push_back (genre);
					}

				}

			} // loop descriptors


			p_out_sched->push_back (p_event);


		} // loop events

	} // loop tables


	std::stable_sort (p_out_sched->begin(), p_out_sched->end(), _comp__table_id);
	std::stable_sort (p_out_sched->begin(), p_out_sched->end(), _comp__section_number);

}

void CEventScheduleManager::cacheSchedule_extended (std::vector <CEvent*> *p_out_sched)
{
	std::vector<CEvent*>::iterator iter = p_out_sched->begin();
	for (; iter != p_out_sched->end(); ++ iter) {
		CEvent* p = *iter;
		if (p) {
			cacheSchedule_extended (p);
		}
	}
}

/**
 * 引数p_out_eventをキーにテーブルのextended event descripotrの内容を保存します
 * p_out_eventは キーとなる ts_id, org_netwkid, scv_id, evt_id はinvalidなデータなこと前提です
 * table_id=0x58〜0x5f が対象です
 */
void CEventScheduleManager::cacheSchedule_extended (CEvent* p_out_event)
{
	if (!p_out_event) {
		return ;
	}


	std::lock_guard<std::recursive_mutex> lock (*mEIT_H_sched_ref.mpMutex);

	std::vector<CEventInformationTable_sched::CTable*>::const_iterator iter = mEIT_H_sched_ref.mpTables->begin();
	for (; iter != mEIT_H_sched_ref.mpTables->end(); ++ iter) {

		CEventInformationTable_sched::CTable *pTable = *iter;

		// キーが一致したテーブルをみます
		if (
			(p_out_event->transport_stream_id != pTable->transport_stream_id) ||
			(p_out_event->original_network_id != pTable->original_network_id) ||
			(p_out_event->service_id != pTable->header.table_id_extension)
		) {
			continue;
		}

		// table_id=0x58〜0x5f が対象です
		if (
			(pTable->header.table_id < TBLID_EIT_SCHED_A_EXT) ||
			(pTable->header.table_id > TBLID_EIT_SCHED_A_EXT +7)
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


			// キーが一致したテーブルをみます
			if (p_out_event->event_id != iter_event->event_id) {
				continue;
			}


			std::vector<CDescriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__EXTENDED_EVENT_DESCRIPTOR) {
					CExtendedEventDescriptor eed (*iter_desc);
					if (!eed.isValid) {
						_UTL_LOG_W ("invalid ExtendedEventDescriptor\n");
						continue;
					}

					char aribstr [MAXSECLEN];
					std::vector<CExtendedEventDescriptor::CItem>::const_iterator iter_item = eed.items.begin();
					for (; iter_item != eed.items.end(); ++ iter_item) {
						CEvent::CExtendedInfo info;

						memset (aribstr, 0x00, MAXSECLEN);
						AribToString (
							aribstr,
							(const char*)iter_item->item_description_char,
							(int)iter_item->item_description_length
						);
						info.item_description = aribstr;

						memset (aribstr, 0x00, MAXSECLEN);
						AribToString (
							aribstr,
							(const char*)iter_item->item_char,
							(int)iter_item->item_length
						);
						info.item = aribstr;

						p_out_event->extendedInfos.push_back (info);
					}
				}

			} // loop descriptors

		} // loop events
	} // loop tables
}

bool CEventScheduleManager::addReserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	CEtime * p_start_time,
	CReserve::type_t _type
)
{
	if (!p_start_time) {
		_UTL_LOG_E ("p_start_time is null");
		return false;
	}

	CReserve r (_transport_stream_id, _original_network_id, _service_id, p_start_time, _type);

	return addReserve (r);
}

bool CEventScheduleManager::addReserve (const CReserve &reserve)
{
	if (isDuplicateReserve (reserve)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	m_reserves.push_back (reserve);

	std::stable_sort (m_reserves.begin(), m_reserves.end(), _comp__reserve_start_time);

	return true;
}

bool CEventScheduleManager::removeReserve (const CReserve &reserve)
{
	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		if (*iter == reserve) {
			// match
			m_reserves.erase (iter);
			// 重複無い前提
			return true;
		}
	}

	return false;
}

bool CEventScheduleManager::isDuplicateReserve (const CReserve& reserve) const
{
	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		if (*iter == reserve) {
			// duplicate
			return true;
		}
	}

	return false;
}

void CEventScheduleManager::check2executeReserves (void)
{
	if (m_executing_reserve.isValid()) {
		return;
	}


	CEtime cur_time;
	cur_time.setCurrentTime ();

	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		if ((iter->type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::INIT) {
			continue;
		}

		CEtime chk_time = iter->start_time;
		chk_time.addHour(1);
		if (cur_time >= chk_time) {
			_UTL_LOG_I ("delete reserve because more time than the start time...");
			iter->dump();
			m_reserves.erase (iter);
			break;
		}

		if (cur_time >= iter->start_time) {

			CEventScheduleManagerIf::SERVICE_KEY_t _key = {
				iter->transport_stream_id,
				iter->original_network_id,
				iter->service_id
			};

			bool r = requestAsync (
				EN_MODULE_EVENT_SCHEDULE_MANAGER,
				EN_SEQ_EVENT_SCHEDULE_MANAGER__EXEC_CACHE_SCHEDULE,
				(uint8_t*)&_key,
				sizeof(_key)
			);

			if (r) {
				_UTL_LOG_I ("requestAsync EXEC_CACHE_SCHEDULE");
				iter->dump();
				m_executing_reserve = *iter;
			} else {
				_UTL_LOG_E ("requestAsync EXEC_CACHE_SCHEDULE failure!");
			}

			m_reserves.erase (iter);
			break;
		}

	}
}

bool CEventScheduleManager::isExistReserve (const CReserve& reserve) const
{
	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		if (*iter == reserve) {
			return true;
		}
	}

	return false;
}

void CEventScheduleManager::dumpReserves (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		iter->dump();
	}
}

void CEventScheduleManager::pushHistories (const CHistory *p_history)
{
	if (!p_history) {
		return;
	}

	// fifo 10
	// pop -> delete
	while (m_histories.size() >= 10) {
		m_histories.erase (m_histories.begin());
	}

	// push
	m_histories.push_back (*(CHistory*)p_history);

	_UTL_LOG_I ("pushHistories  size=[%d]", m_histories.size());
}

void CEventScheduleManager::dumpHistories (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<CHistory>::const_iterator iter = m_histories.begin();
	for (; iter != m_histories.end(); ++ iter) {
		_UTL_LOG_I ("---------------------------------------");
		iter->dump();
	}
}



//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (
		cereal::make_nvp("tv_sec", t.tv_sec),
		cereal::make_nvp("tv_nsec", t.tv_nsec)
	);
}

template <class Archive>
void serialize (Archive &archive, CEtime &t)
{
	archive (cereal::make_nvp("m_time", t.m_time));
}

template <class Archive>
void serialize (Archive &archive, SERVICE_KEY_t &k)
{
	archive (
		cereal::make_nvp("transport_stream_id", k.transport_stream_id),
		cereal::make_nvp("original_network_id", k.original_network_id),
		cereal::make_nvp("service_id", k.service_id),
		cereal::make_nvp("service_type", k.service_type),
		cereal::make_nvp("service_name", k.service_name)
	);
}

template <class Archive>
void serialize (Archive &archive, CEventScheduleManager::CHistory::service &s)
{
	archive (
		cereal::make_nvp("key", s.key),
		cereal::make_nvp("item_num", s.item_num)
	);
}

template <class Archive>
void serialize (Archive &archive, CEventScheduleManager::CHistory::stream &s)
{
	archive (
		cereal::make_nvp("stream_name", s.stream_name),
		cereal::make_nvp("_state", s._state),
		cereal::make_nvp("services", s.services)
	);
}

template <class Archive>
void serialize (Archive &archive, CEventScheduleManager::CHistory &h)
{
	archive (
		cereal::make_nvp("streams", h.streams),
		cereal::make_nvp("start_time", h.start_time),
		cereal::make_nvp("end_time", h.end_time)
	);
}

void CEventScheduleManager::saveHistories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_histories));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getEventScheduleCacheHistoriesJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventScheduleManager::loadHistories (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getEventScheduleCacheHistoriesJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("event_schedule_cache_histories.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_histories));

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので toString用の文字はここで作ります
	std::vector<CHistory>::iterator iter = m_histories.begin();
	for (; iter != m_histories.end(); ++ iter) {
		iter->start_time.updateStrings();
		iter->end_time.updateStrings();
	}
}
