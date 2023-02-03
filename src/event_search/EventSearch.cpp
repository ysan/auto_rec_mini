#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>

#include "CommandServer.h"
#include "EventScheduleManagerIf.h"
#include "EventSearch.h"
#include "EventSearchIf.h"
#include "RecManagerIf.h"
#include "modules.h"

#include "Settings.h"


CEventSearch::CEventSearch (std::string name, uint8_t que_max)
	:CThreadMgrBase (name, que_max)
	,m_cache_sched_state_notify_client_id (0xff)
{
	const int _max = static_cast<int>(CEventSearchIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){on_module_up(p_if);}, "on_module_up"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_module_down(p_if);}, "on_module_up"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_add_rec_reserve_keyword_search(p_if);}, "on_add_rec_reserve_keyword_search"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_add_rec_reserve_keyword_search(p_if);}, "on_add_rec_reserve_keyword_search(ex)"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_dump_histories(p_if);}, "on_dump_histories"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_dump_histories(p_if);}, "on_dump_histories(ex)"},
	};
	set_sequences (seqs, _max);


	m_event_name_keywords.clear();
	m_extended_event_keywords.clear();

	m_event_name_search_histories.clear();
	m_extended_event_search_histories.clear();
	m_current_history.clear();
	m_current_history_keyword.clear();
}

CEventSearch::~CEventSearch (void)
{
	reset_sequences();
}


void CEventSearch::on_destroy (void)
{
	// CCommandServer::on_server_loop を落とします
	// 暫定でここに置きます
	CCommandServer::need_destroy();
}

void CEventSearch::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;


	switch (section_id) {
	case SECTID_ENTRY:

		load_event_name_search_histories ();
		load_extended_event_search_histories ();


		section_id = SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY: {
		CEventScheduleManagerIf _if (get_external_if());
		_if.request_register_cache_schedule_state_notify ();

		section_id = SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_cache_sched_state_notify_client_id = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_END_SUCCESS:
		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:
		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}


	p_if->set_section_id (section_id, act);
}

void CEventSearch::on_module_down (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

//
// do something
//

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventSearch::on_add_rec_reserve_keyword_search (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_EVENTS,
		SECTID_WAIT_GET_EVENTS,
		SECTID_CHECK_EVENT_TIME,
		SECTID_REQ_REMOVE_RESERVE,
		SECTID_WAIT_REMOVE_RESERVE,
		SECTID_REQ_ADD_RESERVE,
		SECTID_WAIT_ADD_RESERVE,
		SECTID_CHECK_LOOP,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static std::vector<std::string>::const_iterator s_iter_keyword ;
	static std::vector<std::string>::const_iterator s_iter_keyword_end ;
	static CEventScheduleManagerIf::event_t s_events [30];
	static int s_events_idx = 0;
	static int s_get_events_num = 0;


	switch (section_id) {
	case SECTID_ENTRY:

		// seq idxが別々で 関数を共有する場合 キューもそれぞれseq idxに対応した別々になるため
		// threadmgr::action::waitしたときに section_id関係なく関数に入ってきてしまうので
		// lockで対応します
		p_if->lock();


		// history ----------------
		m_current_history.clear();

		if (p_if->get_sequence_idx() == static_cast<int>(CEventSearchIf::sequence::add_rec_reserve_keyword_search)) {
			load_event_name_keywords ();
			dump_event_name_keywords ();

			s_iter_keyword     = m_event_name_keywords.begin();
			s_iter_keyword_end = m_event_name_keywords.end();

		} else if (p_if->get_sequence_idx() == static_cast<int>(CEventSearchIf::sequence::add_rec_reserve_keyword_search_ex)) {
			load_extended_event_keywords ();
			dump_extended_event_keywords ();

			s_iter_keyword     = m_extended_event_keywords.begin();
			s_iter_keyword_end = m_extended_event_keywords.end();

		} else {
			_UTL_LOG_E ("unexpected seq idx [%d]", p_if->get_sequence_idx());
			section_id = SECTID_END;
			act = threadmgr::action::continue_;
			break;
		}


		if (s_iter_keyword == s_iter_keyword_end) {
			section_id = SECTID_END;
			act = threadmgr::action::continue_;
			break;
		}

		section_id = SECTID_REQ_GET_EVENTS;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_GET_EVENTS: {

		memset (s_events, 0x00, sizeof(s_events));
		s_events_idx = 0;
		s_get_events_num = 0;


		// history ----------------
		m_current_history_keyword.clear();
		m_current_history_keyword.keyword_string = *s_iter_keyword;


		CEventScheduleManagerIf::request_event_param_t _param;
		_param.arg.p_keyword = s_iter_keyword->c_str();
		_param.p_out_event = s_events;
		_param.array_max_num = 30;

		if (p_if->get_sequence_idx() == static_cast<int>(CEventSearchIf::sequence::add_rec_reserve_keyword_search)) {
			CEventScheduleManagerIf _if(get_external_if());
			_if.request_get_events_keyword (&_param);
		} else {
			// CEventSearchIf::sequence::add_rec_reserve_keyword_search_ex
			CEventScheduleManagerIf _if(get_external_if());
			_if.request_get_events_keyword_ex (&_param);
		}

		section_id = SECTID_WAIT_GET_EVENTS;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_EVENTS:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_get_events_num = *(int*)(p_if->get_source().get_message().data());
			if (s_get_events_num > 0) {
				if (s_get_events_num > 30) {
					_UTL_LOG_W ("trancate s_get_events_num 30");
					s_get_events_num = 30;
				}

				section_id = SECTID_CHECK_EVENT_TIME;
				act = threadmgr::action::continue_;

			} else {
				section_id = SECTID_CHECK_LOOP;
				act = threadmgr::action::continue_;
			}

		} else {
			s_get_events_num = 0;
			section_id = SECTID_CHECK_LOOP;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_CHECK_EVENT_TIME: {

		CEtime cur;
		cur.setCurrentTime();
		if (cur > s_events [s_events_idx].end_time) {
			// 終了時間が過ぎていたら録画予約入れません
			section_id = SECTID_CHECK_LOOP;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_REQ_REMOVE_RESERVE;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_REQ_REMOVE_RESERVE: {
		// 一度予約消してから入れ直します
		// （既に入っていれば消すことになります）

		CRecManagerIf::remove_reserve_param_t _param;
		_param.arg.key.transport_stream_id = s_events [s_events_idx].transport_stream_id;
		_param.arg.key.original_network_id = s_events [s_events_idx].original_network_id;
		_param.arg.key.service_id = s_events [s_events_idx].service_id;
		_param.arg.key.event_id = s_events [s_events_idx].event_id;
		_param.is_consider_repeatability = false;
		_param.is_apply_result = false; // result に記録しない

		CRecManagerIf _if(get_external_if());
		_if.request_remove_reserve (&_param);

		section_id = SECTID_WAIT_REMOVE_RESERVE;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_REMOVE_RESERVE:
//TODO 暫定 結果見ない

		section_id = SECTID_REQ_ADD_RESERVE;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_ADD_RESERVE: {

		// history ----------------
		CHistory::event _event;
		_event.transport_stream_id = s_events [s_events_idx].transport_stream_id;
		_event.original_network_id = s_events [s_events_idx].original_network_id;
		_event.service_id = s_events [s_events_idx].service_id;
		_event.event_id = s_events [s_events_idx].event_id;
		_event.start_time = s_events [s_events_idx].start_time;
		_event.end_time = s_events [s_events_idx].end_time;
		_event.event_name = *(s_events [s_events_idx].p_event_name);
		m_current_history_keyword.events.push_back(_event);


		CRecManagerIf::add_reserve_param_t _param;
		_param.transport_stream_id = s_events [s_events_idx].transport_stream_id;
		_param.original_network_id = s_events [s_events_idx].original_network_id;
		_param.service_id = s_events [s_events_idx].service_id;
		_param.event_id = s_events [s_events_idx].event_id;
		_param.repeatablity = CRecManagerIf::reserve_repeatability::none;

		CRecManagerIf _if(get_external_if());
		_if.request_add_reserve_event (&_param);

		section_id = SECTID_WAIT_ADD_RESERVE;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_ADD_RESERVE:
//TODO 暫定 結果見ない

		section_id = SECTID_CHECK_LOOP;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK_LOOP:

		++ s_events_idx;

		if (s_events_idx < s_get_events_num) {
			// getEventsで取得したリストの残りがあります
			section_id = SECTID_CHECK_EVENT_TIME;
			act = threadmgr::action::continue_;

		} else {
			// getEventsで取得したリストを全て見終わりました
			s_events_idx = 0;
			++ s_iter_keyword;

			if (s_iter_keyword == s_iter_keyword_end) {
				// キーワドリストすべて見終わりました
				section_id = SECTID_END;
				act = threadmgr::action::continue_;

			} else {
				// キーワドリスト残りがあります
				section_id = SECTID_REQ_GET_EVENTS;
				act = threadmgr::action::continue_;

				
				// history ----------------
				m_current_history.keywords.push_back(m_current_history_keyword);
				CEtime _t;
				_t.setCurrentTime();
				m_current_history.timestamp = _t;
			}
		}

		break;

	case SECTID_END:

		if (p_if->get_sequence_idx() == static_cast<int>(CEventSearchIf::sequence::add_rec_reserve_keyword_search)) {
			push_histories (m_current_history, m_event_name_search_histories);
			save_event_name_search_histories();
		} else {
			// CEventSearchIf::sequence::add_rec_reserve_keyword_search_ex
			push_histories (m_current_history, m_extended_event_search_histories);
			save_extended_event_search_histories();
		}

		p_if->unlock();

		p_if->reply (threadmgr::result::success);

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CEventSearch::on_dump_histories (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	if (p_if->get_sequence_idx() == static_cast<int>(CEventSearchIf::sequence::dump_histories)) {
		dump_histories (m_event_name_search_histories);
	} else {
		// CEventSearchIf::sequence::dump_histories_ex
		dump_histories (m_extended_event_search_histories);
	}


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventSearch::on_receive_notify (threadmgr::CThreadMgrIf *p_if)
{
	if (p_if->get_source().get_client_id() != m_cache_sched_state_notify_client_id) {
		return ;
	}

	CEventScheduleManagerIf::cache_schedule_state state = *(CEventScheduleManagerIf::cache_schedule_state*)(p_if->get_source().get_message().data());
	switch (state) {
		case CEventScheduleManagerIf::cache_schedule_state::busy:
		break;

		case CEventScheduleManagerIf::cache_schedule_state::ready: {
		// EPG取得が終わったら キーワード予約をかけます

		uint32_t opt = get_request_option ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		set_request_option (opt);

		request_async (static_cast<uint8_t>(module::module_id::event_search), static_cast<int>(CEventSearchIf::sequence::add_rec_reserve_keyword_search));
		request_async (static_cast<uint8_t>(module::module_id::event_search), static_cast<int>(CEventSearchIf::sequence::add_rec_reserve_keyword_search_ex));

		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		set_request_option (opt);

		}
		break;

	default:
		break;
	}


}


void CEventSearch::dump_event_name_keywords (void) const
{
	_UTL_LOG_I ("----- event name keywords -----");
	std::vector<std::string>::const_iterator iter = m_event_name_keywords.begin();
	for (; iter != m_event_name_keywords.end(); ++ iter) {
		_UTL_LOG_I ("  [%s]", iter->c_str());
	}
}

void CEventSearch::dump_extended_event_keywords (void) const
{
	_UTL_LOG_I ("----- extended event keywords -----");
	std::vector<std::string>::const_iterator iter = m_extended_event_keywords.begin();
	for (; iter != m_extended_event_keywords.end(); ++ iter) {
		_UTL_LOG_I ("  [%s]", iter->c_str());
	}
}

void CEventSearch::push_histories (const CHistory &history, std::vector<CHistory> &histories)
{
	// fifo 30
	// pop -> delete
	while (histories.size() >= 30) {
		histories.erase (histories.begin());
	}

	// push
	histories.push_back (history);

	_UTL_LOG_I ("push_histories  size=[%d]", histories.size());
}

void CEventSearch::dump_histories (const std::vector<CHistory> &histories) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (const auto & history : histories) {
		_UTL_LOG_I ("---------------------------------------");
		history.dump();
	}
}

void CEventSearch::save_event_name_keywords (void)
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

void CEventSearch::load_event_name_keywords (void)
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

void CEventSearch::save_extended_event_keywords (void)
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

void CEventSearch::load_extended_event_keywords (void)
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

void CEventSearch::save_event_name_search_histories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_event_name_search_histories));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getEventNameSearchHistoriesJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventSearch::load_event_name_search_histories (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getEventNameSearchHistoriesJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("event_name_search_histories.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_event_name_search_histories));

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので toString用の文字はここで作ります
	for (auto &history : m_event_name_search_histories) {
		history.timestamp.updateStrings();
		for (auto &keyword : history.keywords) {
			for (auto &event: keyword.events) {
				event.start_time.updateStrings();
				event.end_time.updateStrings();
			}
		}
	}
}

void CEventSearch::save_extended_event_search_histories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_extended_event_search_histories));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getExtendedEventSearchHistoriesJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventSearch::load_extended_event_search_histories (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getExtendedEventSearchHistoriesJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("extended_event_search_histories.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_extended_event_search_histories));

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので toString用の文字はここで作ります
	for (auto &history : m_extended_event_search_histories) {
		history.timestamp.updateStrings();
		for (auto &keyword : history.keywords) {
			for (auto &event: keyword.events) {
				event.start_time.updateStrings();
				event.end_time.updateStrings();
			}
		}
	}
}


//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (
		cereal::make_nvp("tv_sec", t.tv_sec)
		,cereal::make_nvp("tv_nsec", t.tv_nsec)
	);
}

template <class Archive>
void serialize (Archive &archive, CEtime &t)
{
	archive (cereal::make_nvp("m_time", t.m_time));
}

template <class Archive>
void serialize (Archive &archive, CEventSearch::CHistory::event &e)
{
	archive (
		cereal::make_nvp("transport_stream_id", e.transport_stream_id)
		,cereal::make_nvp("original_network_id", e.original_network_id)
		,cereal::make_nvp("service_id", e.service_id)
		,cereal::make_nvp("event_id", e.event_id)
		,cereal::make_nvp("start_time", e.start_time)
		,cereal::make_nvp("end_time", e.end_time)
		,cereal::make_nvp("event_name", e.event_name)
	);
}

template <class Archive>
void serialize (Archive &archive, CEventSearch::CHistory::keyword &k)
{
	archive (
		cereal::make_nvp("keyword_string", k.keyword_string)
		,cereal::make_nvp("events", k.events)
	);
}

template <class Archive>
void serialize (Archive &archive, CEventSearch::CHistory &h)
{
	archive (
		cereal::make_nvp("keywords", h.keywords)
		,cereal::make_nvp("timestamp", h.timestamp)
	);
}
