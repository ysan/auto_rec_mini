#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <algorithm>

#include "EventInformationTable_sched.h"
#include "EventScheduleManager.h"
#include "EventScheduleManagerIf.h"

#include "PsisiManagerIf.h"
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


CEventScheduleManager::CEventScheduleManager (std::string name, uint8_t que_max)
	:threadmgr::CThreadMgrBase (name, que_max)
	,mp_settings (NULL)
	,m_state (CEventScheduleManagerIf::cache_schedule_state::init)
	,m_tuner_notify_client_id (0xff)
	,m_event_change_notify_client_id (0xff)
	,mp_EIT_H_sched (NULL)
	,m_is_need_stop (false)
	,m_force_cache_group_id (0xff)
	,m_tuner_resource_max (0)
{
	const int _max = static_cast<int>(CEventScheduleManagerIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_module_up(p_if);}, std::move("on_module_up")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_module_down(p_if);}, std::move("on_module_down")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_register_cache_schedule_state_notify(p_if);}, std::move("on_register_cache_schedule_state_notify")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_unregister_cache_schedule_state_notify(p_if);}, std::move("on_unregister_cache_schedule_state_notify")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_get_cache_schedule_state(p_if);}, std::move("on_get_cache_schedule_state")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_check_loop(p_if);}, std::move("on_check_loop")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_parser_notice(p_if);}, std::move("on_parser_notice")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_exec_cache_schedule(p_if);}, std::move("on_exec_cache_schedule")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_cache_schedule(p_if);}, std::move("on_cache_schedule")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_cache_schedule_force_current_service(p_if);}, std::move("on_cache_schedule_force_current_service")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_stop_cache_schedule(p_if);}, std::move("on_stop_cache_schedule")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_get_event(p_if);}, std::move("on_get_event")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_get_event_latest_dumped_schedule(p_if);}, std::move("on_get_event_latest_dumped_schedule")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_dump_event_latest_dumped_schedule(p_if);}, std::move("on_dump_event_latest_dumped_schedule")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_get_events_keyword_search(p_if);}, std::move("on_get_events_keyword_search")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_get_events_keyword_search(p_if);}, std::move("on_get_events_keyword_search(ex)")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_add_reserves(p_if);}, std::move("on_add_reserves")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_dump_schedule_map(p_if);}, std::move("on_dump_schedule_map")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_dump_schedule(p_if);}, std::move("on_dump_schedule")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_dump_reserves(p_if);}, std::move("on_dump_reserves")},
		{[&](threadmgr::CThreadMgrIf *p_if){CEventScheduleManager::on_dump_histories(p_if);}, std::move("on_dump_histories")},
	};
	set_sequences (seqs, _max);


	mp_settings = CSettings::get_instance();

	m_last_update_EIT_sched.clear();
	m_start_time_EIT_sched.clear();

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
	m_last_update_EIT_sched.clear();
	m_start_time_EIT_sched.clear();

	m_latest_dumped_key.clear();

	m_schedule_cache_next_day.clear();
	m_schedule_cache_current_plan.clear();

	m_reserves.clear();
	m_executing_reserve.clear();

	m_current_history.clear();
	m_current_history_stream.clear();
	m_histories.clear();

	m_container.clear();

	reset_sequences();
}


void CEventScheduleManager::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
//		SECTID_REQ_REG_TUNER_NOTIFY,
//		SECTID_WAIT_REG_TUNER_NOTIFY,
//		SECTID_REQ_REG_EVENT_CHANGE_NOTIFY,
//		SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;


	switch (section_id) {
	case SECTID_ENTRY: {

		// settingsを使って初期化する場合はmodule upで
		m_tuner_resource_max = CSettings::get_instance()->get_params().get_tuner_hal_allocates().size();

		std::string cache_data_path = CSettings::get_instance()->get_params().get_event_schedule_cache_data_json_path();
		CUtils::makedir(cache_data_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

		std::string cache_history_path = CSettings::get_instance()->get_params().get_event_schedule_cache_histories_json_path();
		CUtils::makedir(cache_history_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

		m_schedule_cache_next_day.setCurrentDay();

		load_histories ();

		m_container.set_schedule_map_json_path(mp_settings->get_params().get_event_schedule_cache_data_json_path());
		m_container.load_schedule_map();

//		section_id = SECTID_REQ_REG_TUNER_NOTIFY;
		section_id = SECTID_REQ_CHECK_LOOP;
		act = threadmgr::action::continue_;
		}
		break;
/***
	case SECTID_REQ_REG_TUNER_NOTIFY: {
		CTuner_control_if _if (get_external_if());
		_if.request_register_tuner_notify ();

		section_id = SECTID_WAIT_REG_TUNER_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_TUNER_NOTIFY:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_tuner_notify_client_id = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_register_tuner_notify is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_REG_EVENT_CHANGE_NOTIFY: {
		CPsisiManagerIf _if (get_external_if());
		_if.request_register_event_change_notify ();

		section_id = SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_event_change_notify_client_id = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_CHECK_LOOP;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_register_event_change_notify is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;
***/
	case SECTID_REQ_CHECK_LOOP:
		request_async (static_cast<uint8_t>(modules::module_id::event_schedule_manager), static_cast<int>(CEventScheduleManagerIf::sequence::check_loop));

		section_id = SECTID_WAIT_CHECK_LOOP;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_CHECK_LOOP:
//		rslt = p_if->get_source().get_result();
//		if (rslt == threadmgr::result::success) {
//
//		} else {
//
//		}
// threadmgr::result::successのみ

		section_id = SECTID_END_SUCCESS;
		act = threadmgr::action::continue_;
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

void CEventScheduleManager::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CEventScheduleManager::on_register_cache_schedule_state_notify (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	uint8_t client_id = 0;
	threadmgr::result rslt;
	bool r = p_if->reg_notify (NOTIFY_CAT__CACHE_SCHEDULE, &client_id);
	if (r) {
		rslt = threadmgr::result::success;
	} else {
		rslt = threadmgr::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	p_if->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_unregister_cache_schedule_state_notify (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	threadmgr::result rslt;
	// msgからclient_idを取得
	uint8_t client_id = *(p_if->get_source().get_message().data());
	bool r = p_if->unreg_notify (NOTIFY_CAT__CACHE_SCHEDULE, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::error;
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_get_cache_schedule_state (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	// reply msgにm_stateを乗せます
	p_if->reply (threadmgr::result::success, (uint8_t*)&m_state, sizeof(m_state));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_check_loop (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	switch (section_id) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		p_if->reply (threadmgr::result::success);

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK:

		p_if->set_timeout (10*1000); // 10sec

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_WAIT: {

		// 実行予定日をチェックします
		CEtime t;
		t.setCurrentDay();
		if (t == m_schedule_cache_next_day) {
			// 実行予定日なので予約を入れます
			_UTL_LOG_I ("add reserves base_time => [%s]", m_schedule_cache_next_day.toString());

			int start_hour = mp_settings->get_params().get_event_schedule_cache_start_hour();
			int start_min = mp_settings->get_params().get_event_schedule_cache_start_min();
			CEtime _base_time = m_schedule_cache_next_day;
			_base_time.addHour (start_hour);
			_base_time.addMin (start_min);


			uint32_t opt = get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			request_async (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<int>(CEventScheduleManagerIf::sequence::add_reserves),
				(uint8_t*)&_base_time,
				sizeof(_base_time)
			);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);


			// 次回の予定日セットしておきます
			int interval_day = mp_settings->get_params().get_event_schedule_cache_start_interval_day();
			if (interval_day <= 1) {
				// interval_dayは最低でも１日にします
				interval_day = 1;
			}
			m_schedule_cache_next_day.addDay(interval_day);
		}


		if (mp_settings->get_params().is_enable_event_schedule_cache()) {
			// 予約をチェックします
			check2execute_reserves ();
		}


		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;

		}
		break;

	case SECTID_END:
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_parser_notice (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CPsisiManagerIf::psisi_type _type = *(CPsisiManagerIf::psisi_type*)(p_if->get_source().get_message().data());
	switch (_type) {
	case CPsisiManagerIf::psisi_type::EIT_H_SCHED:

		// EITのshceduleの更新時間保存
		// 受け取り中はかなりの頻度でたたかれるはず
		m_last_update_EIT_sched.setCurrentTime ();

		break;

	default:
		break;
	};

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_exec_cache_schedule (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_OPEN,
		SECTID_WAIT_OPEN,
		SECTID_REQ_GET_PHYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PHYSICAL_CH_BY_SERVICE_ID,
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
		SECTID_EXIT,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static CEventScheduleManagerIf::service_key_t s_service_key;
	static psisi_structs::service_info_t s_service_infos[10];
	static int s_num = 0;
	static bool s_is_timeouted = false;
	static bool s_is_already_using_tuner = false;
	static uint8_t s_group_id = 0xff;
	static uint16_t s_ch = 0;


	switch (section_id) {
	case SECTID_ENTRY:

		s_service_key = *(CEventScheduleManagerIf::service_key_t*)(p_if->get_source().get_message().data());

		if (m_executing_reserve.type & CReserve::type_t::S_FLG) {
			// update m_state
			m_state = CEventScheduleManagerIf::cache_schedule_state::busy;

			// history ----------------
			m_current_history.clear();
			m_current_history.set_start_time();

			m_retry_reserves.clear();
		}

		if (m_executing_reserve.type & CReserve::type_t::N_FLG) {
			// fire notify
			p_if->notify (NOTIFY_CAT__CACHE_SCHEDULE, (uint8_t*)&m_state, sizeof(m_state));
		}


		// history ----------------
		{
			CChannelManagerIf::service_id_param_t param = {
				s_service_key.transport_stream_id,
				s_service_key.original_network_id,
				0 // no need service_id
			};
			CChannelManagerIf _if (get_external_if());
			_if.request_get_transport_stream_name_sync (&param);
			char* ts_name = (char*)(p_if->get_source().get_message().data());
			m_current_history_stream.clear();
			m_current_history_stream.stream_name = ts_name;
		}


		if ((m_executing_reserve.type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::FORCE) {
			section_id = SECTID_REQ_GET_SERVICE_INFOS;
			act = threadmgr::action::continue_;
		} else {
			// NORMAL
			section_id = SECTID_REQ_OPEN;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_OPEN: {

		CTunerServiceIf _if (get_external_if());
		_if.request_open ();

		section_id = SECTID_WAIT_OPEN;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_OPEN: {
		rslt = get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_group_id = *(uint8_t*)(get_if()->get_source().get_message().data());
			_UTL_LOG_I ("requst_open group_id:[0x%02x]", s_group_id);
			section_id = SECTID_REQ_GET_PHYSICAL_CH_BY_SERVICE_ID;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("someone is using a tuner.");
			s_is_already_using_tuner = true;
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_REQ_GET_PHYSICAL_CH_BY_SERVICE_ID: {
		CChannelManagerIf::service_id_param_t param = {
			s_service_key.transport_stream_id,
			s_service_key.original_network_id,
			s_service_key.service_id
		};

		CChannelManagerIf _if (get_external_if());
		_if.request_get_physical_channel_by_service_id (&param);

		section_id = SECTID_WAIT_GET_PHYSICAL_CH_BY_SERVICE_ID;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PHYSICAL_CH_BY_SERVICE_ID:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {

			s_ch = *(uint16_t*)(p_if->get_source().get_message().data());

			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_get_physical_channel_by_service_id is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE: {
		CTunerServiceIf::tune_param_t param = {
			s_ch,
			s_group_id
		};

		CTunerServiceIf _if (get_external_if());
		_if.request_tune_with_retry (&param);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_REQ_GET_SERVICE_INFOS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("CTunerServiceIf::request_tune is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		uint8_t _group_id = 0xff;
		if ((m_executing_reserve.type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::FORCE) {
			_group_id = m_force_cache_group_id;
		} else {
			// NORMAL
			_group_id = s_group_id;
		}

		CPsisiManagerIf _if (get_external_if(), _group_id);
		if (_if.request_get_current_service_infos (s_service_infos, 10)) {
			section_id = SECTID_WAIT_GET_SERVICE_INFOS;
			act = threadmgr::action::wait;
		} else {
			_UTL_LOG_E ("CPsisiManagerIf::request_get_current_service_infos is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_num = *(int*)(p_if->get_source().get_message().data());
			if (s_num > 0) {
//s_service_infos[0].dump();
				section_id = SECTID_REQ_ENABLE_PARSE_EIT_SCHED;
				act = threadmgr::action::continue_;

			} else {
				_UTL_LOG_E ("request_get_current_service_infos  num is 0");
				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_E ("request_get_current_service_infos err");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_ENABLE_PARSE_EIT_SCHED: {
		uint8_t _group_id = 0xff;
		if ((m_executing_reserve.type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::FORCE) {
			_group_id = m_force_cache_group_id;
		} else {
			// NORMAL
			_group_id = s_group_id;
		}

		CPsisiManagerIf _if (get_external_if(), _group_id);
		if (_if.request_enable_parse_eit_sched ()) {
			section_id = SECTID_WAIT_ENABLE_PARSE_EIT_SCHED;
			act = threadmgr::action::wait;
		} else {
			_UTL_LOG_E ("CPsisiManagerIf::request_enable_parse_eit_sched is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_WAIT_ENABLE_PARSE_EIT_SCHED: {
//		rslt = p_if->get_source().get_result();
//		if (rslt == threadmgr::result::success) {
//
//		} else {
//
//		}
// threadmgr::result::successのみ

		enable_parse_eit_sched_reply_param_t param =
					*(enable_parse_eit_sched_reply_param_t*)(p_if->get_source().get_message().data());

		// parserのインスタンス取得して
		// ここでparser側で積んでるデータは一度クリアします
		mp_EIT_H_sched = param.p_parser;
		m_EIT_H_sched_ref = param.p_parser->reference();
		mp_EIT_H_sched->clear();


		m_start_time_EIT_sched.setCurrentTime();

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;

		}
		break;

	case SECTID_CHECK:

		p_if->set_timeout (5000); // 5sec

		if (m_is_need_stop) {
			_UTL_LOG_W ("parser EIT schedule : cancel");
			section_id = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_CHECK_WAIT;
			act = threadmgr::action::wait;
		}

		break;

	case SECTID_CHECK_WAIT: {

		CEtime tcur;
		tcur.setCurrentTime();
		CEtime ttmp = m_last_update_EIT_sched;
		ttmp.addSec (10);

		if (tcur > ttmp) {
			// 約10秒間更新がなかったら完了とします
			_UTL_LOG_I ("parse EIT schedule : complete");
			section_id = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			act = threadmgr::action::continue_;

		} else {
			ttmp = m_start_time_EIT_sched;
			ttmp.addMin (mp_settings->get_params().get_event_schedule_cache_timeout_min());
			if (tcur > ttmp) {
				// get_event_schedule_cache_timeout_min 分経過していたらタイムアウトします
				_UTL_LOG_W ("parser EIT schedule : timeout");
				s_is_timeouted = true;
				section_id = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
				act = threadmgr::action::continue_;

			} else {
				_UTL_LOG_I ("parse EIT schedule : m_last_update_EIT_sched %s", m_last_update_EIT_sched.toString());
				section_id = SECTID_CHECK;
				act = threadmgr::action::continue_;
			}
		}

		}
		break;

	case SECTID_REQ_DISABLE_PARSE_EIT_SCHED: {
		uint8_t _group_id = 0xff;
		if ((m_executing_reserve.type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::FORCE) {
			_group_id = m_force_cache_group_id;
		} else {
			// NORMAL
			_group_id = s_group_id;
		}

		CPsisiManagerIf _if (get_external_if(), _group_id);
		if (_if.request_disable_parse_eit_sched ()) {
			section_id = SECTID_WAIT_DISABLE_PARSE_EIT_SCHED;
			act = threadmgr::action::wait;
		} else {
			// キューがいっぱいでした
			// disable は必ず行いたいので 少し待ってリトライします（無限）
			_UTL_LOG_E ("CPsisiManagerIf::request_disable_parse_eit_sched is failure. --> retry");
			p_if->set_timeout (500); // 500m_s
			section_id = SECTID_REQ_DISABLE_PARSE_EIT_SCHED;
			act = threadmgr::action::wait;
		}

		}
		break;

	case SECTID_WAIT_DISABLE_PARSE_EIT_SCHED:
//		rslt = p_if->get_source().get_result();
//		if (rslt == threadmgr::result::success) {
//
//		} else {
//
//		}
// threadmgr::result::successのみ

		section_id = SECTID_CACHE;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CACHE:

		for (int i = 0; i < s_num; ++ i) {
			s_service_infos[i].dump ();

			std::vector <CEvent*> *p_sched = new std::vector <CEvent*>;
			cache_schedule (
				s_service_infos[i].transport_stream_id,
				s_service_infos[i].original_network_id,
				s_service_infos[i].service_id,
				p_sched
			);

			if (p_sched->size() > 0) {
				cache_schedule_extended (p_sched);


				service_key_t key (
					s_service_infos[i].transport_stream_id,
					s_service_infos[i].original_network_id,
					s_service_infos[i].service_id,
					s_service_infos[i].service_type,			// additional
					s_service_infos[i].p_service_name_char	// additional
				);

				if (m_container.has_schedule_map (key)) {
					_UTL_LOG_I ("has_schedule_map -> delete_schedule_map");
					m_container.delete_schedule_map (key);
				}

				m_container.add_schedule_map (key, p_sched);
				key.dump();
				_UTL_LOG_I ("add_schedule_map -> %d items", p_sched->size());


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
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_END_SUCCESS:

		p_if->reply (threadmgr::result::success);


		// history ----------------
		m_current_history_stream._state = CHistory::EN_STATE__COMPLETE;
		m_current_history.add (m_current_history_stream);


		section_id = SECTID_EXIT;
		act = threadmgr::action::continue_;
		break;

	case SECTID_END_ERROR:

		p_if->reply (threadmgr::result::error);


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
			int interval = mp_settings->get_params().get_event_schedule_cache_retry_interval_min();
			start_time.addMin(interval);

			retry.transport_stream_id = s_service_key.transport_stream_id;
			retry.original_network_id = s_service_key.original_network_id;
			retry.service_id = s_service_key.service_id;
			retry.start_time = start_time;
			retry.type = CReserve::type_t::NORMAL;
			m_retry_reserves.push_back(retry);
		}


		section_id = SECTID_EXIT;
		act = threadmgr::action::continue_;
		break;

	case SECTID_EXIT:

		if (
			!s_is_already_using_tuner &&
			!((m_executing_reserve.type & CReserve::type_t::TYPE_MASK) == CReserve::type_t::FORCE)
		) {
			//-----------------------------//
			{
				uint32_t opt = get_request_option ();
				opt |= REQUEST_OPTION__WITHOUT_REPLY;
				set_request_option (opt);

				// 選局を停止しときます tune stop
				// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
				CTunerServiceIf _if (get_external_if());
				_if.request_tune_stop (s_group_id);
				_if.request_close (s_group_id);

				opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
				set_request_option (opt);
			}
			//-----------------------------//
		}

		if (m_executing_reserve.type & CReserve::type_t::E_FLG) {

			// history ----------------
			m_current_history.set_end_time();

			push_histories (&m_current_history);
			save_histories ();
			m_current_history.clear();

			// update m_state
			m_state = CEventScheduleManagerIf::cache_schedule_state::ready;

			// retry reserve
			int _rs = (int)m_retry_reserves.size();
			_UTL_LOG_I ("m_retry_reserves.size %d", _rs);
			if (_rs == 1) {
				m_retry_reserves[0].type =
					(CReserve::type_t)(CReserve::type_t::NORMAL | CReserve::type_t::S_FLG |
										CReserve::type_t::E_FLG | CReserve::type_t::N_FLG);
				add_reserve(m_retry_reserves[0]);
				dump_reserves();
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
					add_reserve(m_retry_reserves[i]);
				}
				dump_reserves();
				m_retry_reserves.clear();
			}

			m_container.save_schedule_map();
		}

		if (m_executing_reserve.type & CReserve::type_t::N_FLG) {
			// fire notify
			p_if->notify (NOTIFY_CAT__CACHE_SCHEDULE, (uint8_t*)&m_state, sizeof(m_state));
		}

		memset (&s_service_key, 0x00, sizeof(s_service_key));
		memset (s_service_infos, 0x00, sizeof(s_service_infos));
		s_num = 0;
		s_is_timeouted = false;
		s_is_already_using_tuner = false;
		mp_EIT_H_sched = NULL;
		m_executing_reserve.clear();
		s_group_id = 0xff;
		s_ch = 0;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;

		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_cache_schedule (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_ADD_RESERVES,
		SECTID_WAIT_ADD_RESERVES,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;

	switch (section_id) {
	case SECTID_ENTRY:

		section_id = SECTID_REQ_ADD_RESERVES;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_ADD_RESERVES: {

		CEtime _base_time ;
		_base_time.setCurrentTime();

		request_async (
			static_cast<uint8_t>(modules::module_id::event_schedule_manager),
			static_cast<int>(CEventScheduleManagerIf::sequence::add_reserves),
			(uint8_t*)&_base_time,
			sizeof(_base_time)
		);

		section_id = SECTID_WAIT_ADD_RESERVES;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_ADD_RESERVES:

		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("add reserve failure.");
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

void CEventScheduleManager::on_cache_schedule_force_current_service (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
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

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static psisi_structs::service_info_t s_service_infos[10];
	static int s_num = 0;
	static CReserve s_tmp_reserve ;


	switch (section_id) {
	case SECTID_ENTRY:

		m_force_cache_group_id = *(uint8_t*)(p_if->get_source().get_message().data());
		if (m_force_cache_group_id >= m_tuner_resource_max) {
			_UTL_LOG_E ("invalid group_id:[0x%02x]", m_force_cache_group_id);
			 section_id = SECTID_END_ERROR;
			 act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_REQ_GET_SERVICE_INFOS;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		CPsisiManagerIf _if (get_external_if(), m_force_cache_group_id);
		if (_if.request_get_current_service_infos (s_service_infos, 10)) {
			section_id = SECTID_WAIT_GET_SERVICE_INFOS;
			act = threadmgr::action::wait;
		} else {
			_UTL_LOG_E ("CPsisiManagerIf::request_get_current_service_infos is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_num = *(int*)(p_if->get_source().get_message().data());
			if (s_num > 0) {
//s_service_infos[0].dump();
				section_id = SECTID_ADD_RESERVE;
				act = threadmgr::action::continue_;

			} else {
				_UTL_LOG_E ("request_get_current_service_infos is 0");
				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_E ("request_get_current_service_infos err");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_ADD_RESERVE: {

		CEtime start_time;
		start_time.setCurrentTime();

		CReserve::type_t type =
			(CReserve::type_t)(CReserve::type_t::FORCE | CReserve::type_t::S_FLG | CReserve::type_t::E_FLG);

		// ここは添字0 でも何でも良いです
		// (現状ではすべてのチャンネル編成分のスケジュールを取得します)
		s_tmp_reserve.transport_stream_id = s_service_infos[0].transport_stream_id;
		s_tmp_reserve.original_network_id = s_service_infos[0].original_network_id;
		s_tmp_reserve.service_id = s_service_infos[0].service_id;
		s_tmp_reserve.start_time = start_time;
		s_tmp_reserve.type = type;
		add_reserve (s_tmp_reserve);


		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;

		}
		break;

	case SECTID_CHECK: {

		p_if->set_timeout (100); // 100ms

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_CHECK_WAIT: {

		if (is_exist_reserve(s_tmp_reserve)) {
			// まだ予約に入ってます
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;

		} else if (m_executing_reserve == s_tmp_reserve && m_executing_reserve.is_valid()) {
			// 只今実行中です
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;

		} else {
			// 実行終了しました
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_END_SUCCESS:

		p_if->reply (threadmgr::result::success);

		memset (s_service_infos, 0x00, sizeof(s_service_infos));
		s_num = 0;
		s_tmp_reserve.clear();
		m_force_cache_group_id = 0xff;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

		p_if->reply (threadmgr::result::error);

		memset (s_service_infos, 0x00, sizeof(s_service_infos));
		s_num = 0;
		s_tmp_reserve.clear();
		m_force_cache_group_id = 0xff;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_stop_cache_schedule (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	switch (section_id) {
	case SECTID_ENTRY:

		m_is_need_stop = true;

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK: {

		p_if->set_timeout (100); // 100ms

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_CHECK_WAIT: {

		if (m_executing_reserve.is_valid()) {
			// 只今実行中です
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;

		} else {
			// 実行終了しました
			section_id = SECTID_END;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_END:

		p_if->reply (threadmgr::result::success);

		m_is_need_stop = false;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_get_event (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CEventScheduleManagerIf::request_event_param_t _param =
			*(CEventScheduleManagerIf::request_event_param_t*)(p_if->get_source().get_message().data());

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		p_if->reply (threadmgr::result::error);

	} else {

		service_key_t _service_key = {_param.arg.key.transport_stream_id, _param.arg.key.original_network_id, _param.arg.key.service_id};
		const CEvent *p = m_container.get_event (_service_key, _param.arg.key.event_id);
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

			p_if->reply (threadmgr::result::success);

		} else {

			_UTL_LOG_E ("get_event is null.");
			p_if->reply (threadmgr::result::error);

		}
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_get_event_latest_dumped_schedule (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CEventScheduleManagerIf::request_event_param_t _param =
			*(CEventScheduleManagerIf::request_event_param_t*)(p_if->get_source().get_message().data());

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		p_if->reply (threadmgr::result::error);

	} else {

		const CEvent *p = m_container.get_event (m_latest_dumped_key, _param.arg.index);
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

			p_if->reply (threadmgr::result::success);

		} else {

			_UTL_LOG_E ("get_event is null.");
			p_if->reply (threadmgr::result::error);

		}
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_dump_event_latest_dumped_schedule (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CEventScheduleManagerIf::request_event_param_t _param =
			*(CEventScheduleManagerIf::request_event_param_t*)(p_if->get_source().get_message().data());

//_param.p_out_eventはnullで来ます

//	if (!_param.p_out_event) {
//		_UTL_LOG_E ("_param.p_out_event is null.");
//		p_if->reply (threadmgr::result::error);

//	} else {

		const CEvent *p = m_container.get_event (m_latest_dumped_key, _param.arg.index);
		CEvent* p_event = const_cast <CEvent*> (p);
		if (p_event) {

			p_event->dump();
			p_event->dump_detail ();

			p_if->reply (threadmgr::result::success);

		} else {

			_UTL_LOG_E ("get_event is null.");
			p_if->reply (threadmgr::result::error);

		}
//	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_get_events_keyword_search (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CEventScheduleManagerIf::request_event_param_t _param =
			*(CEventScheduleManagerIf::request_event_param_t*)(p_if->get_source().get_message().data());

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		p_if->reply (threadmgr::result::error);

	} else if (_param.array_max_num <= 0) {
		_UTL_LOG_E ("_param.array_max_num is invalid.");
		p_if->reply (threadmgr::result::error);

	} else {

		bool is_check_extended_event = false;
		if (p_if->get_sequence_idx() == static_cast<int>(CEventScheduleManagerIf::sequence::get_events__keyword_search)) {
			is_check_extended_event = false;
		} else if (p_if->get_sequence_idx() == static_cast<int>(CEventScheduleManagerIf::sequence::get_events__keyword_search_ex)) {
			is_check_extended_event = true;
		}

		int n = m_container.get_events (_param.arg.p_keyword, _param.p_out_event, _param.array_max_num, is_check_extended_event);
		if (n < 0) {
			_UTL_LOG_E ("get_events is invalid.");
			p_if->reply (threadmgr::result::error);
		} else {
			// 検索数をreply msgで返します
			p_if->reply (threadmgr::result::success, (uint8_t*)&n, sizeof(n));
		}
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_add_reserves (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_CHANNELS,
		SECTID_WAIT_GET_CHANNELS,
		SECTID_ADD_RESERVES,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static CChannelManagerIf::channel_t s_channels[20];
	static int s_ch_num = 0;
	static CEtime _base_time;


	switch (section_id) {
	case SECTID_ENTRY:

		_base_time = *(CEtime*)(p_if->get_source().get_message().data());

		section_id = SECTID_REQ_GET_CHANNELS;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_GET_CHANNELS: {

		memset (s_channels, 0x00, sizeof(s_channels));
		s_ch_num = 0;

		CChannelManagerIf::request_channels_param_t param = {s_channels, 20};

		CChannelManagerIf _if (get_external_if());
		_if.request_get_channels (&param);

		section_id = SECTID_WAIT_GET_CHANNELS;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_CHANNELS:

		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_ch_num = *(int*)(p_if->get_source().get_message().data());
			_UTL_LOG_I ("request_get_channels s_ch_num:[%d]", s_ch_num);

			if (s_ch_num > 0) {
				section_id = SECTID_ADD_RESERVES;
				act = threadmgr::action::continue_;
			} else {
				_UTL_LOG_E ("request_get_channels is 0");
				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_E ("request_get_channels is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
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

			add_reserve (
				s_channels[i].transport_stream_id,
				s_channels[i].original_network_id,
				s_channels[i].service_ids[0],	// ここは添字0 でも何でも良いです
												// (現状ではすべてのチャンネル編成分のスケジュールを取得します)
				&_start_time,
				type
			);
		}

		section_id = SECTID_END_SUCCESS;
		act = threadmgr::action::continue_;

		}
		break;

	case SECTID_END_SUCCESS:

		dump_reserves();

		p_if->reply (threadmgr::result::success);

		memset (s_channels, 0x00, sizeof(s_channels));
		s_ch_num = 0;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

		p_if->reply (threadmgr::result::error);

		memset (s_channels, 0x00, sizeof(s_channels));
		s_ch_num = 0;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_dump_schedule_map (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	m_container.dump_schedule_map ();


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_dump_schedule (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CEventScheduleManagerIf::service_key_t _key =
					*(CEventScheduleManagerIf::service_key_t*)(p_if->get_source().get_message().data());
	service_key_t key (_key);
	m_latest_dumped_key = key;

	m_container.dump_schedule_map (key);


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_dump_reserves (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	dump_reserves ();


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_dump_histories (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	dump_histories ();


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CEventScheduleManager::on_receive_notify (threadmgr::CThreadMgrIf *p_if)
{
/***
	if (p_if->get_source().get_client_id() == m_tuner_notify_client_id) {

		EN_TUNER_STATE en_state = *(EN_TUNER_STATE*)(p_if->get_source().get_message().data());
		switch (en_state) {
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

	} else if (p_if->get_source().get_client_id() == m_event_change_notify_client_id) {

		PSISI_NOTIFY_EVENT_INFO _info = *(PSISI_NOTIFY_EVENT_INFO*)(p_if->get_source().get_message().data());
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
void CEventScheduleManager::cache_schedule (
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


	std::lock_guard<std::recursive_mutex> lock (*m_EIT_H_sched_ref.mpMutex);

	std::vector<CEventInformationTable_sched::CTable*>::const_iterator iter = m_EIT_H_sched_ref.mpTables->begin();
	for (; iter != m_EIT_H_sched_ref.mpTables->end(); ++ iter) {

		CEventInformationTable_sched::CTable *p_table = *iter;

		// キーが一致したテーブルをみます
		if (
			(_transport_stream_id != p_table->transport_stream_id) ||
			(_original_network_id != p_table->original_network_id) ||
			(_service_id != p_table->header.table_id_extension)
		) {
			continue;
		}

		// table_id=0x50,0x51 が対象です
		if (
			(p_table->header.table_id != TBLID_EIT_SCHED_A) &&
			(p_table->header.table_id != TBLID_EIT_SCHED_A +1)
		) {
			continue;
		}

		// loop events
		std::vector<CEventInformationTable_sched::CTable::CEvent>::const_iterator iter_event = p_table->events.begin();
		for (; iter_event != p_table->events.end(); ++ iter_event) {
			if (iter_event->event_id == 0) {
				// event情報が付いてない
				continue;
			}

			CEvent *p_event = new CEvent ();

			p_event->table_id = p_table->header.table_id;
			p_event->transport_stream_id = p_table->transport_stream_id;
			p_event->original_network_id = p_table->original_network_id;
			p_event->service_id = p_table->header.table_id_extension;

			p_event->section_number = p_table->header.section_number;

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
						_UTL_LOG_W ("invalid Short_event_descriptor");
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
						_UTL_LOG_W ("invalid Component_descriptor");
						continue;
					}

					p_event->component_type = cd.component_type;
					p_event->component_tag = cd.component_tag;

				} else if (iter_desc->tag == DESC_TAG__AUDIO_COMPONENT_DESCRIPTOR) {
					CAudioComponentDescriptor acd (*iter_desc);
					if (!acd.isValid) {
						_UTL_LOG_W ("invalid Audio_component_descriptor\n");
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
						_UTL_LOG_W ("invalid Content_descriptor");
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

void CEventScheduleManager::cache_schedule_extended (std::vector <CEvent*> *p_out_sched)
{
	std::vector<CEvent*>::iterator iter = p_out_sched->begin();
	for (; iter != p_out_sched->end(); ++ iter) {
		CEvent* p = *iter;
		if (p) {
			cache_schedule_extended (p);
		}
	}
}

/**
 * 引数p_out_eventをキーにテーブルのextended event descripotrの内容を保存します
 * p_out_eventは キーとなる ts_id, org_netwkid, scv_id, evt_id はinvalidなデータなこと前提です
 * table_id=0x58〜0x5f が対象です
 */
void CEventScheduleManager::cache_schedule_extended (CEvent* p_out_event)
{
	if (!p_out_event) {
		return ;
	}


	std::lock_guard<std::recursive_mutex> lock (*m_EIT_H_sched_ref.mpMutex);

	std::vector<CEventInformationTable_sched::CTable*>::const_iterator iter = m_EIT_H_sched_ref.mpTables->begin();
	for (; iter != m_EIT_H_sched_ref.mpTables->end(); ++ iter) {

		CEventInformationTable_sched::CTable *p_table = *iter;

		// キーが一致したテーブルをみます
		if (
			(p_out_event->transport_stream_id != p_table->transport_stream_id) ||
			(p_out_event->original_network_id != p_table->original_network_id) ||
			(p_out_event->service_id != p_table->header.table_id_extension)
		) {
			continue;
		}

		// table_id=0x58〜0x5f が対象です
		if (
			(p_table->header.table_id < TBLID_EIT_SCHED_A_EXT) ||
			(p_table->header.table_id > TBLID_EIT_SCHED_A_EXT +7)
		) {
			continue;
		}


		// loop events
		std::vector<CEventInformationTable_sched::CTable::CEvent>::const_iterator iter_event = p_table->events.begin();
		for (; iter_event != p_table->events.end(); ++ iter_event) {
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
						_UTL_LOG_W ("invalid Extended_event_descriptor\n");
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

						p_out_event->extended_infos.push_back (info);
					}
				}

			} // loop descriptors

		} // loop events
	} // loop tables
}

bool CEventScheduleManager::add_reserve (
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

	return add_reserve (r);
}

bool CEventScheduleManager::add_reserve (const CReserve &reserve)
{
	if (is_duplicate_reserve (reserve)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	m_reserves.push_back (reserve);

	std::stable_sort (m_reserves.begin(), m_reserves.end(), _comp__reserve_start_time);

	return true;
}

bool CEventScheduleManager::remove_reserve (const CReserve &reserve)
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

bool CEventScheduleManager::is_duplicate_reserve (const CReserve& reserve) const
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

void CEventScheduleManager::check2execute_reserves (void)
{
	if (m_executing_reserve.is_valid()) {
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

			CEventScheduleManagerIf::service_key_t _key = {
				iter->transport_stream_id,
				iter->original_network_id,
				iter->service_id
			};

			uint32_t opt = get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			bool r = request_async (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<int>(CEventScheduleManagerIf::sequence::exec_cache_schedule),
				(uint8_t*)&_key,
				sizeof(_key)
			);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			if (r) {
				_UTL_LOG_I ("request_async exec_cache_schedule");
				iter->dump();
				m_executing_reserve = *iter;
			} else {
				_UTL_LOG_E ("request_async exec_cache_schedule failure!");
			}

			m_reserves.erase (iter);
			break;
		}

	}
}

bool CEventScheduleManager::is_exist_reserve (const CReserve& reserve) const
{
	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		if (*iter == reserve) {
			return true;
		}
	}

	return false;
}

void CEventScheduleManager::dump_reserves (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<CReserve>::const_iterator iter = m_reserves.begin();
	for (; iter != m_reserves.end(); ++ iter) {
		iter->dump();
	}
}

void CEventScheduleManager::push_histories (const CHistory *p_history)
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

	_UTL_LOG_I ("push_histories  size=[%d]", m_histories.size());
}

void CEventScheduleManager::dump_histories (void) const
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
void serialize (Archive &archive, service_key_t &k)
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

void CEventScheduleManager::save_histories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_histories));
	}

	std::string path = CSettings::get_instance()->get_params().get_event_schedule_cache_histories_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CEventScheduleManager::load_histories (void)
{
	std::string path = CSettings::get_instance()->get_params().get_event_schedule_cache_histories_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
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


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので to_string用の文字はここで作ります
	std::vector<CHistory>::iterator iter = m_histories.begin();
	for (; iter != m_histories.end(); ++ iter) {
		iter->start_time.updateStrings();
		iter->end_time.updateStrings();
	}
}
