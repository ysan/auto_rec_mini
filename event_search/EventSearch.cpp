#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "EventSearch.h"
#include "modules.h"

#include "Settings.h"


CEventSearch::CEventSearch (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_cache_sched_state_notify_client_id (0xff)
{
	mSeqs [EN_SEQ_EVENT_SEARCH__MODULE_UP] =
		{(PFN_SEQ_BASE)&CEventSearch::onReq_moduleUp,                    (char*)"onReq_moduleUp"};
	mSeqs [EN_SEQ_EVENT_SEARCH__MODULE_DOWN] =
		{(PFN_SEQ_BASE)&CEventSearch::onReq_moduleDown,                  (char*)"onReq_moduleDown"};
	mSeqs [EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH] =
		{(PFN_SEQ_BASE)&CEventSearch::onReq_addRecReserve_keywordSearch, (char*)"onReq_addRecReserve_keywordSearch"};
	mSeqs [EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH_EX] =
		{(PFN_SEQ_BASE)&CEventSearch::onReq_addRecReserve_keywordSearch, (char*)"onReq_addRecReserve_keywordSearch(ex)"};
	setSeqs (mSeqs, EN_SEQ_EVENT_SEARCH__NUM);


	m_event_name_keywords.clear();
	m_extended_event_keywords.clear();

}

CEventSearch::~CEventSearch (void)
{
}


void CEventSearch::onReq_moduleUp (CThreadMgrIf *pIf)
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

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:
		sectId = SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY: {
		CEventScheduleManagerIf _if (getExternalIf());
		_if.reqRegisterCacheScheduleStateNotify ();

		sectId = SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_cache_sched_state_notify_client_id = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
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

void CEventSearch::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CEventSearch::onReq_addRecReserve_keywordSearch (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_EVENTS,
		SECTID_WAIT_GET_EVENTS,
		SECTID_REQ_ADD_RESERVE,
		SECTID_WAIT_ADD_RESERVE,
		SECTID_CHECK_LOOP,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static std::vector<std::string>::const_iterator s_iter ;
	static std::vector<std::string>::const_iterator s_iter_end ;
	static CEventScheduleManagerIf::EVENT_t s_events [30];
	static int s_events_idx = 0;
	static int s_get_events_num = 0;


	switch (sectId) {
	case SECTID_ENTRY:

		// seqIdxが別々で 関数を共有する場合 EN_THM_ACT_WAITしたときに
		// キューもそれぞれseqIdxに対応した別々になるため sectId関係なく関数に入ってきてしまうので
		// lockで対応します
		pIf->lock();

		if (pIf->getSeqIdx() == EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH) {
			loadEventNameKeywords ();
			dumpEventNameKeywords ();

			s_iter = m_event_name_keywords.begin();
			s_iter_end = m_event_name_keywords.end();

		} else if (pIf->getSeqIdx() == EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH_EX) {
			loadExtendedEventKeywords ();
			dumpExtendedEventKeywords ();

			s_iter = m_extended_event_keywords.begin();
			s_iter_end = m_extended_event_keywords.end();

		} else {
			_UTL_LOG_E ("unexpected seqIdx [%d]", pIf->getSeqIdx());
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
			break;
		}


		if (s_iter == s_iter_end) {
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
			break;
		}

		sectId = SECTID_REQ_GET_EVENTS;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_EVENTS: {

		memset (s_events, 0x00, sizeof(s_events));
		s_events_idx = 0;
		s_get_events_num = 0;


		CEventScheduleManagerIf::REQ_EVENT_PARAM_t _param;
		_param.arg.p_keyword = s_iter->c_str();
		_param.p_out_event = s_events;
		_param.array_max_num = 30;

		if (pIf->getSeqIdx() == EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH) {
			CEventScheduleManagerIf _if(getExternalIf());
			_if.reqGetEvent_keyword (&_param);
		} else {
			// EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH_EX
			CEventScheduleManagerIf _if(getExternalIf());
			_if.reqGetEvent_keyword_ex (&_param);
		}

		sectId = SECTID_WAIT_GET_EVENTS;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_EVENTS:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			s_get_events_num = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			if (s_get_events_num > 0) {
				if (s_get_events_num > 30) {
					_UTL_LOG_W ("trancate s_get_events_num");
					s_get_events_num = 30;
				}

				sectId = SECTID_REQ_ADD_RESERVE;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				sectId = SECTID_CHECK_LOOP;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			s_get_events_num = 0;
			sectId = SECTID_CHECK_LOOP;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_ADD_RESERVE: {

		CRecManagerIf::ADD_RESERVE_PARAM_t _param;
		_param.transport_stream_id = s_events [s_events_idx].transport_stream_id;
		_param.original_network_id = s_events [s_events_idx].original_network_id;
		_param.service_id = s_events [s_events_idx].service_id;
		_param.event_id = s_events [s_events_idx].event_id;
		_param.repeatablity = EN_RESERVE_REPEATABILITY__NONE;

		CRecManagerIf _if(getExternalIf());
		_if.reqAddReserve_event (&_param);

		sectId = SECTID_WAIT_ADD_RESERVE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_ADD_RESERVE:
//TODO 暫定 結果見ない

		sectId = SECTID_CHECK_LOOP;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_LOOP:

		++ s_events_idx;

		if (s_events_idx < s_get_events_num) {
			// getEventの残りがあるので予約を入れます
			sectId = SECTID_REQ_ADD_RESERVE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			s_events_idx = 0;
			++ s_iter;

			if (s_iter == s_iter_end) {
				// キーワドリストすべて見終わりました
				sectId = SECTID_END;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				// キーワドリスト残りがあります
				sectId = SECTID_REQ_GET_EVENTS;
				enAct = EN_THM_ACT_CONTINUE;
			}
		}

		break;

	case SECTID_END:

		pIf->unlock();

		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CEventSearch::onReceiveNotify (CThreadMgrIf *pIf)
{
	if (pIf->getSrcInfo()->nClientId != m_cache_sched_state_notify_client_id) {
		return ;
	}

	EN_CACHE_SCHEDULE_STATE_t enState = *(EN_CACHE_SCHEDULE_STATE_t*)(pIf->getSrcInfo()->msg.pMsg);
	switch (enState) {
	case EN_CACHE_SCHEDULE_STATE__BUSY:
		break;

	case EN_CACHE_SCHEDULE_STATE__READY: {
		// EPG取得が終わったら キーワード予約をかけます

		uint32_t opt = getRequestOption ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		setRequestOption (opt);

		requestAsync (EN_MODULE_EVENT_SEARCH, EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH);
		requestAsync (EN_MODULE_EVENT_SEARCH, EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH_EX);

		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		setRequestOption (opt);

		}
		break;

	default:
		break;
	}


}


void CEventSearch::dumpEventNameKeywords (void) const
{
	_UTL_LOG_I ("----- event name keywords -----");
	std::vector<std::string>::const_iterator iter = m_event_name_keywords.begin();
	for (; iter != m_event_name_keywords.end(); ++ iter) {
		_UTL_LOG_I ("  [%s]", iter->c_str());
	}
}

void CEventSearch::dumpExtendedEventKeywords (void) const
{
	_UTL_LOG_I ("----- extended event keywords -----");
	std::vector<std::string>::const_iterator iter = m_extended_event_keywords.begin();
	for (; iter != m_extended_event_keywords.end(); ++ iter) {
		_UTL_LOG_I ("  [%s]", iter->c_str());
	}
}



//--------------------------------------------------------------------------------

void CEventSearch::saveEventNameKeywords (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_event_name_keywords));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getEventNameKeywordsJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventSearch::loadEventNameKeywords (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getEventNameKeywordsJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("event_name_keywords.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_event_name_keywords));

	ifs.close();
	ss.clear();
}

void CEventSearch::saveExtendedEventKeywords (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_extended_event_keywords));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getExtendedEventKeywordsJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventSearch::loadExtendedEventKeywords (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getExtendedEventKeywordsJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("extended_event_keywords.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_extended_event_keywords));

	ifs.close();
	ss.clear();
}
