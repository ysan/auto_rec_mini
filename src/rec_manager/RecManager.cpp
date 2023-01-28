#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "EventScheduleManagerIf.h"
#include "Group.h"
#include "PsisiManagerIf.h"
#include "RecInstance.h"
#include "RecManager.h"
#include "RecManagerIf.h"

#include "Utils.h"
#include "modules.h"

#include "aribstr.h"
#include "threadmgr_if.h"



static const struct reserve_state_pair g_reserve_state_pair [] = {
	{0x00000000, "INIT"},
	{0x00000001, "REMOVED"},
	{0x00000002, "RESCHEDULED"},
	{0x00000004, "START_TIME_PASSED"},
	{0x00000100, "REQ_START_RECORDING"},
	{0x00000200, "NOW_RECORDING"},
	{0x00000400, "FORCE_STOPPED"},
	{0x00010000, "END_ERROR__ALREADY_PASSED"},
	{0x00020000, "END_ERROR__HDD_FREE_SPACE_LOW"},
	{0x00040000, "END_ERROR__TUNE_ERR"},
	{0x00080000, "END_ERROR__EVENT_NOT_FOUND"},
	{0x00100000, "END_ERROR__INTERNAL_ERR"},
	{0x80000000, "END_SUCCESS"},

	{0x00000000, NULL}, // term
};

static char gsz_reserve_state [256];

const char * get_reserve_state_string (uint32_t state)
{
	memset (gsz_reserve_state, 0x00, sizeof(gsz_reserve_state));
	int n = 0;
	int s = 0;
	char *pos = gsz_reserve_state;
	int _size = sizeof(gsz_reserve_state);

	while (g_reserve_state_pair [n].psz_reserve_state != NULL) {
		if (state & g_reserve_state_pair [n].state) {
			s = snprintf (pos, _size, "%s,", g_reserve_state_pair [n].psz_reserve_state);
			pos += s;
			_size -= s;
		}
		++ n ;
	}

	// INIT
	if (pos == gsz_reserve_state) {
		strncpy (
			gsz_reserve_state,
			g_reserve_state_pair[0].psz_reserve_state,
			sizeof(gsz_reserve_state) -1
		);
	}

	return gsz_reserve_state;
}

const char *g_repeatability [] = {
	"NONE",
	"DAILY",
	"WEEKLY",
	"AUTO",
};


////typedef struct {
////	EN_REC_PROGRESS rec_progress;
////	uint32_t reserve_state;
////} RECORDING_NOTICE_t;


CRecManager::CRecManager (char *psz_name, uint8_t n_que_num)
	:threadmgr::CThreadMgrBase (psz_name, n_que_num)
////	,m_tuner_notify_client_id (0xff)
////	,m_ts_receive_handler_id (-1)
////	,m_pat_detect_notify_client_id (0xff)
////	,m_event_change_notify_client_id (0xff)
////	,m_rec_progress (EN_REC_PROGRESS__INIT)
	,m_tuner_resource_max (0)
{
	const int _max = static_cast<int>(CRecManagerIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_module_up(p_if);}, "on_module_up"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_module_up_by_group_id(p_if);}, "on_module_up_by_group_id"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_module_down(p_if);}, "on_module_down"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_check_loop(p_if);}, "on_check_loop"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_check_reserves_event_loop(p_if);}, "on_check_reserves_event_loop"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_check_recordings_event_loop(p_if);}, "on_check_recordings_event_loop"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_recording_notice(p_if);}, "on_recording_notice"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_start_recording(p_if);}, "on_start_recording"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_add_reserve_current_event(p_if);}, "on_add_reserve_current_event"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_add_reserve_event(p_if);}, "on_add_reserve_event"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_add_reserve_event_helper(p_if);}, "on_add_reserve_event_helper"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_add_reserve_manual(p_if);}, "on_add_reserve_manual"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_remove_reserve(p_if);}, "on_remove_reserve"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_remove_reserve_by_index(p_if);}, "on_remove_reserve_by_index"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_get_reserves(p_if);}, "on_get_reserves"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_stop_recording(p_if);}, "on_stop_recording"},
		{[&](threadmgr::CThreadMgrIf *p_if){CRecManager::on_dump_reserves(p_if);}, "on_dump_reserves"},
	};
	set_sequences (seqs, _max);


	mp_settings = CSettings::getInstance();

	clear_reserves ();
	clear_results ();
////	m_recordings[0].clear();
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_recordings[_gr].clear();
	}

////	memset (m_recording_tmpfile, 0x00, sizeof (m_recording_tmpfile));
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		memset (m_recording_tmpfiles[_gr], 0x00, PATH_MAX);
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_tuner_notify_client_id [_gr] = 0xff;
		m_ts_receive_handler_id [_gr] = -1;
		m_pat_detect_notify_client_id [_gr] = 0xff;
		m_event_change_notify_client_id [_gr] = 0xff;
	}
}

CRecManager::~CRecManager (void)
{
	clear_reserves ();
	clear_results ();
////	m_recordings[0].clear();
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_recordings[_gr].clear();
	}

////	memset (m_recording_tmpfile, 0x00, sizeof (m_recording_tmpfile));
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		memset (m_recording_tmpfiles[_gr], 0x00, PATH_MAX);
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_tuner_notify_client_id [_gr] = 0xff;
		m_ts_receive_handler_id [_gr] = -1;
		m_pat_detect_notify_client_id [_gr] = 0xff;
		m_event_change_notify_client_id [_gr] = 0xff;
	}

	reset_sequences();
}


void CRecManager::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_MODULE_UP_BY_GROUPID,
		SECTID_WAIT_MODULE_UP_BY_GROUPID,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
		SECTID_REQ_CHECK_RESERVES_EVENT_LOOP,
		SECTID_WAIT_CHECK_RESERVES_EVENT_LOOP,
		SECTID_REQ_CHECK_RECORDINGS_EVENT_LOOP,
		SECTID_WAIT_CHECK_RECORDINGS_EVENT_LOOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static uint8_t s_gr_cnt = 0;

	switch (section_id) {
	case SECTID_ENTRY:

		// settingsを使って初期化する場合はmodule upで
		m_tuner_resource_max = CSettings::getInstance()->getParams()->getTunerHalAllocates()->size(); 

		load_reserves ();
		load_results ();

		// rec_instance グループ数分の生成はここで
		// get_external_if()メンバ使ってるからコンストラクタ上では実行不可
		for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
			std::unique_ptr <CRecInstance> _r(new CRecInstance(get_external_if(), _gr));
			msp_rec_instances[_gr].swap(_r);
			mp_ts_handlers[_gr] = msp_rec_instances[_gr].get();
		}

		section_id = SECTID_REQ_MODULE_UP_BY_GROUPID;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_MODULE_UP_BY_GROUPID:
		request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::module_up_by_groupid), (uint8_t*)&s_gr_cnt, sizeof(s_gr_cnt));
		++ s_gr_cnt;

		section_id = SECTID_WAIT_MODULE_UP_BY_GROUPID;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_MODULE_UP_BY_GROUPID:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			if (s_gr_cnt < CGroup::GROUP_MAX) {
				section_id = SECTID_REQ_MODULE_UP_BY_GROUPID;
				act = threadmgr::action::continue_;
			} else {
				section_id = SECTID_REQ_CHECK_LOOP;
				act = threadmgr::action::continue_;
			}

		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_CHECK_LOOP:
		request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::check_loop));

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

		section_id = SECTID_REQ_CHECK_RESERVES_EVENT_LOOP;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_CHECK_RESERVES_EVENT_LOOP:
		request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::check_reserves_event_loop));

		section_id = SECTID_WAIT_CHECK_RESERVES_EVENT_LOOP;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_CHECK_RESERVES_EVENT_LOOP:
//		rslt = p_if->get_source().get_result();
//		if (rslt == threadmgr::result::success) {
//
//		} else {
//
//		}
// threadmgr::result::successのみ

		section_id = SECTID_REQ_CHECK_RECORDINGS_EVENT_LOOP;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_CHECK_RECORDINGS_EVENT_LOOP:
		request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::check_recordings_event_loop));

		section_id = SECTID_WAIT_CHECK_RECORDINGS_EVENT_LOOP;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_CHECK_RECORDINGS_EVENT_LOOP:
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
		s_gr_cnt = 0;
		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:
		s_gr_cnt = 0;
		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}


	p_if->set_section_id (section_id, act);
}

void CRecManager::on_module_up_by_group_id (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
		SECTID_REQ_REG_HANDLER,
		SECTID_WAIT_REG_HANDLER,
		SECTID_REQ_REG_PAT_DETECT_NOTIFY,
		SECTID_WAIT_REG_PAT_DETECT_NOTIFY,
		SECTID_REQ_REG_EVENT_CHANGE_NOTIFY,
		SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static uint8_t s_group_id = 0;

	switch (section_id) {
	case SECTID_ENTRY:

		// request msg から group_id を取得します
		s_group_id = *(uint8_t*)(get_if()->get_source().get_message().data());
		_UTL_LOG_I("(%s) group_id:[%d]", p_if->get_sequence_name(), s_group_id);

		section_id = SECTID_REQ_REG_TUNER_NOTIFY;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_REG_TUNER_NOTIFY: {
		CTunerControlIf _if (get_external_if(), s_group_id);
		_if.request_register_tuner_notify ();

		section_id = SECTID_WAIT_REG_TUNER_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_TUNER_NOTIFY:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_tuner_notify_client_id [s_group_id] = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_REG_HANDLER;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("req_register_tuner_notify is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_REG_HANDLER: {

		_UTL_LOG_I ("CTunerControlIf::ITsReceiveHandler %p", msp_rec_instances[s_group_id].get());
		CTunerControlIf _if (get_external_if(), s_group_id);
		_if.request_register_ts_receive_handler (&(mp_ts_handlers[s_group_id]));

		section_id = SECTID_WAIT_REG_HANDLER;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_HANDLER:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_ts_receive_handler_id [s_group_id] = *(int*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_REG_PAT_DETECT_NOTIFY;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("req_register_ts_receive_handler is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_REG_PAT_DETECT_NOTIFY: {
		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_register_pat_detect_notify ();

		section_id = SECTID_WAIT_REG_PAT_DETECT_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_PAT_DETECT_NOTIFY:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_pat_detect_notify_client_id [s_group_id] = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("req_register_pat_detect_notify is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_REG_EVENT_CHANGE_NOTIFY: {
		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_register_event_change_notify ();

		section_id = SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			m_event_change_notify_client_id [s_group_id] = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("req_register_event_change_notify is failure.");
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

void CRecManager::on_module_down (threadmgr::CThreadMgrIf *p_if)
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
// do nothing
//

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_check_loop (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_WAIT_START_RECORDING,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;


	switch (section_id) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		p_if->reply (threadmgr::result::success);

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK:

		p_if->set_timeout (1000); // 1sec

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_WAIT: {

		check_reserves ();

		check_recording_end ();

		check_disk_free ();

		uint8_t group_id = 0;
		if (is_exist_req_start_recording_reserve ()) {
			CTunerServiceIf _if (get_external_if());
			_if.request_open_sync ();
			
			rslt = get_if()->get_source().get_result();
			if (rslt == threadmgr::result::success) {
				group_id = *(uint8_t*)(get_if()->get_source().get_message().data());
				_UTL_LOG_I ("req_open group_id:[0x%02x]", group_id);

				if (m_recordings[group_id].is_used) {
					// m_recordingsのidxはreq_openで取ったtuner_idで決まります
					// is_usedになってるはずない...
					_UTL_LOG_E ("??? m_recordings[group_id].is_used ???  group_id:[0x%02x]", group_id);
					section_id = SECTID_CHECK;
					act = threadmgr::action::continue_;
					break;
				}

			} else {
				section_id = SECTID_CHECK;
				act = threadmgr::action::continue_;
				break;
			}
		}

		if (pick_req_start_recording_reserve (group_id)) {
			// request start recording
			request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::start_recording), (uint8_t*)&group_id, sizeof(uint8_t));

			section_id = SECTID_WAIT_START_RECORDING;
			act = threadmgr::action::wait;

		} else {
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_WAIT_START_RECORDING:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
//TODO imple
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;

		} else {
//TODO imple
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

//TODO  event_sched_mgrの state_notify契機でチェックする方がいいかも
void CRecManager::on_check_reserves_event_loop (threadmgr::CThreadMgrIf *p_if)
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

	threadmgr::result rslt = threadmgr::result::success;


	switch (section_id) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		p_if->reply (threadmgr::result::success);

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK:

		p_if->set_timeout (60*1000*10); // 10min

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_WAIT: {

		// EPG取得が完了しているか確認します
		CEventScheduleManagerIf _if (get_external_if());
		_if.request_get_cache_schedule_state_sync ();
		CEventScheduleManagerIf::cache_schedule_state _s =  *(CEventScheduleManagerIf::cache_schedule_state*)(get_if()->get_source().get_message().data());
		if (_s != CEventScheduleManagerIf::cache_schedule_state::ready) {
			// readyでないので以下の処理は行いません
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;
			break;
		}


		// 録画予約に対応するイベント内容ををチェックします
		for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {

			if (!m_reserves [i].is_used) {
				continue;
			}

			if (m_reserves [i].state == RESERVE_STATE__INIT) {
				continue;
			}

			if (!m_reserves [i].is_event_type) {
				continue;
			}


			// スケジュールを確認してstart_time, end_time, event_name が変わってないかチェックします

			CEventScheduleManagerIf::event_t event;
			CEventScheduleManagerIf::request_event_param_t param = {
				{
					m_reserves [i].transport_stream_id,
					m_reserves [i].original_network_id,
					m_reserves [i].service_id,
					m_reserves [i].event_id
				},
				&event
			};

			CEventScheduleManagerIf _if (get_external_if());
			_if.request_get_event_sync (&param);
			
			rslt = get_if()->get_source().get_result();
			if (rslt == threadmgr::result::success) {
				if ( m_reserves [i].start_time != param.p_out_event-> start_time) {
					_UTL_LOG_I (
						"check_event  reserve start_time is update.  [%s] -> [%s]",
						m_reserves [i].start_time.toString(),
						param.p_out_event->start_time.toString()
					);
					m_reserves [i].start_time = param.p_out_event-> start_time;
				}

				if (m_reserves [i].end_time != param.p_out_event-> end_time) {
					_UTL_LOG_I (
						"check_event  reserve end_time is update.  [%s] -> [%s]",
						m_reserves [i].end_time.toString(),
						param.p_out_event->end_time.toString()
					);
					m_reserves [i].end_time = param.p_out_event-> end_time;
				}

				if (m_reserves [i].title_name != *param.p_out_event-> p_event_name) {
					_UTL_LOG_I (
						"check_event  reserve title_name is update.  [%s] -> [%s]",
						m_reserves [i].title_name.c_str(),
						param.p_out_event->p_event_name->c_str()
					);
					m_reserves [i].title_name = param.p_out_event-> p_event_name->c_str();
				}

			} else {
				// 予約に対応するイベントがなかった
				_UTL_LOG_W ("no event corresponding to the reservation...");
				m_reserves [i].dump();
			}

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

void CRecManager::on_check_recordings_event_loop (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static psisi_structs::event_info_t s_present_event_info;
	static uint8_t s_group_id = 0;

	switch (section_id) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		p_if->reply (threadmgr::result::success);

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK:

//		memset (&s_present_event_info, 0x00, sizeof(s_present_event_info));
		s_present_event_info.clear();
		if (s_group_id >= CGroup::GROUP_MAX) {
			s_group_id = 0;
		}

		p_if->set_timeout (1000); // 1sec

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_WAIT:

		if (m_recordings[s_group_id].state & RESERVE_STATE__NOW_RECORDING) {
			section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
			act = threadmgr::action::continue_;

		} else {

			++ s_group_id;

			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {

		psisi_structs::service_info_t _svc_info ;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = m_recordings[s_group_id].transport_stream_id;
		_svc_info.original_network_id = m_recordings[s_group_id].original_network_id;
		_svc_info.service_id = m_recordings[s_group_id].service_id;

		CPsisiManagerIf _if (get_external_if(), s_group_id);
		if (_if.request_get_present_event_info (&_svc_info, &s_present_event_info)) {
			section_id = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
			act = threadmgr::action::wait;
		} else {
			// キューが入らなかった時用

			++ s_group_id;

			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
//s_present_event_info.dump();
			if (m_recordings[s_group_id].state & RESERVE_STATE__NOW_RECORDING) {

				if (m_recordings[s_group_id].is_event_type) {
					// event_typeの録画は event_idとend_timeの確認
					if (m_recordings[s_group_id].event_id == s_present_event_info.event_id) {
						if (m_recordings[s_group_id].end_time != s_present_event_info.end_time) {
							_UTL_LOG_I (
								"#####  recording end_time is update.  [%s] -> [%s]  #####",
								m_recordings[s_group_id].end_time.toString (),
								s_present_event_info.end_time.toString ()
							);
							m_recordings[s_group_id].end_time = s_present_event_info.end_time;
							m_recordings[s_group_id].dump();
						}

					} else {
//TODO 録画止めてもいいかも
						// event_idが変わった とりあえずログだしとく
						_UTL_LOG_W (
							"#####  recording event_id changed.  [0x%04x] -> [0x%04x]  #####",
							s_present_event_info.event_id,
							m_recordings[s_group_id].event_id
						);
						m_recordings[s_group_id].dump();
						s_present_event_info.dump();
					}

				} else {
					// manual録画は event_name_charを代入しときます
					m_recordings[s_group_id].event_id = s_present_event_info.event_id;
					m_recordings[s_group_id].title_name = s_present_event_info.event_name_char;
				}
			}

		} else {
			_UTL_LOG_E ("(%s) req_get_present_event_info err", p_if->get_sequence_name());
		}

		++ s_group_id;

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
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

void CRecManager::on_recording_notice (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


////	RECORDING_NOTICE_t _notice = *(RECORDING_NOTICE_t*)(p_if->get_source().get_message().data());
	CRecInstance::RECORDING_NOTICE_t _notice = *(CRecInstance::RECORDING_NOTICE_t*)(p_if->get_source().get_message().data());
	switch (_notice.rec_progress) {
////	case EN_REC_PROGRESS__INIT:
	case CRecInstance::progress::INIT:
		break;

////	case EN_REC_PROGRESS__PRE_PROCESS:
	case CRecInstance::progress::PRE_PROCESS:
////		// ここでは エラーが起きることがあるのでRESERVE_STATE をチェックします
////		if (_notice.reserve_state != RESERVE_STATE__INIT) {
////			m_recordings[0].state |= _notice.reserve_state;
////		}
		break;

////	case EN_REC_PROGRESS__NOW_RECORDING:
	case CRecInstance::progress::NOW_RECORDING:
		break;

////	case EN_REC_PROGRESS__END_SUCCESS:
	case CRecInstance::progress::END_SUCCESS:
		m_recordings[_notice.group_id].state |= RESERVE_STATE__END_SUCCESS;
		break;

////	case EN_REC_PROGRESS__END_ERROR:
	case CRecInstance::progress::END_ERROR:
		break;

////	case EN_REC_PROGRESS__POST_PROCESS: {
	case CRecInstance::progress::POST_PROCESS: {

		// NOW_RECORDINGフラグは落としておきます
		m_recordings[_notice.group_id].state &= ~RESERVE_STATE__NOW_RECORDING;

		m_recordings[_notice.group_id].recording_end_time.setCurrentTime();

		set_result (&m_recordings[_notice.group_id]);


		// rename
		std::string *p_path = mp_settings->getParams()->getRecTsPath();

		char newfile [PATH_MAX] = {0};
		char *p_name = (char*)"rec";
		if (m_recordings[_notice.group_id].title_name.c_str()) {
			p_name = (char*)m_recordings[_notice.group_id].title_name.c_str();
		}
		CEtime t_end;
		t_end.setCurrentTime();
		snprintf (
			newfile,
			sizeof(newfile),
			"%s/%s_0x%08x_%d_%s.m2ts",
			p_path->c_str(),
			p_name,
			m_recordings[_notice.group_id].state,
			_notice.group_id,
			t_end.toString()
		);

		rename (m_recording_tmpfiles[_notice.group_id], newfile) ;

		m_recordings[_notice.group_id].clear ();
		_UTL_LOG_I ("recording end...");


		//-----------------------------//
		{
			uint32_t opt = get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			// 選局を停止しときます tune stop
			// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
			CTunerServiceIf _if (get_external_if());
			_if.request_tune_stop (_notice.group_id);
			_if.request_close (_notice.group_id);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);
		}
		//-----------------------------//


		}
		break;

	default:
		break;
	};

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_start_recording (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CHECK_DISK_FREE_SPACE,
//		SECTID_REQ_STOP_CACHE_SCHED,
//		SECTID_WAIT_STOP_CACHE_SCHED,
//		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
//		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_REQ_CACHE_SCHEDULE,
		SECTID_WAIT_CACHE_SCHEDULE,
		SECTID_REQ_ADD_RESERVE_RESCHEDULE,
		SECTID_WAIT_ADD_RESERVE_RESCHEDULE,
		SECTID_START_RECORDING,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static psisi_structs::event_info_t s_present_event_info;
	static int s_retry_get_event_info;
	static uint8_t s_group_id = 0xff;
//	static uint16_t s_ch = 0;


	switch (section_id) {
	case SECTID_ENTRY:

		_UTL_LOG_I ("(%s) entry", p_if->get_sequence_name());

		s_group_id = *(uint8_t*)(p_if->get_source().get_message().data());
		if (s_group_id >= m_tuner_resource_max) {
			_UTL_LOG_E ("invalid group_id:[0x%02x]", s_group_id);

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;

		} else {
			m_recordings[s_group_id].dump();

			section_id = SECTID_CHECK_DISK_FREE_SPACE;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_CHECK_DISK_FREE_SPACE: {

		std::string *p_path = mp_settings->getParams()->getRecTsPath();
		int limit = mp_settings->getParams()->getRecDiskSpaceLowLimitMB();
		int df = CUtils::getDiskFreeMB(p_path->c_str());
		_UTL_LOG_I ("(%s) disk free space [%d] Mbytes.", p_if->get_sequence_name(), df);
		if (df < limit) {
			// HDD残量たりないので 録画実行しません
			m_recordings[s_group_id].state |= RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW;
			set_result (&m_recordings[s_group_id]);
			m_recordings[s_group_id].clear ();

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;

		} else {
//TODO
//			section_id = SECTID_REQ_STOP_CACHE_SCHED;
//			section_id = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;
		}

		}
		break;
/***
	case SECTID_REQ_STOP_CACHE_SCHED: {

		// EPG取得中だったら止めてから録画開始します
		CEventScheduleManagerIf _if (get_external_if());
		_if.req_stop_cache_schedule ();

		section_id = SECTID_WAIT_STOP_CACHE_SCHED;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_STOP_CACHE_SCHED:
		// successのみ
		section_id = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
		act = threadmgr::action::continue_;
		break;
***/
/***
	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {

		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			m_recordings[s_group_id].transport_stream_id,
			m_recordings[s_group_id].original_network_id,
			m_recordings[s_group_id].service_id
		};

		CChannelManagerIf _if (get_external_if());
		_if.req_get_pysical_channel_by_service_id (&param);

		section_id = SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {

			s_ch = *(uint16_t*)(p_if->get_source().get_message().data());

			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("req_get_pysical_channel_by_service_id is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;
***/
	case SECTID_REQ_TUNE: {

/***
		CTunerServiceIf::tune_param_t param = {
			s_ch,
			s_group_id
		};

		CTunerServiceIf _if (get_external_if());
		_if.req_tune_with_retry (&param);
***/
		CTunerServiceIf::tune_advance_param_t param = {
			m_recordings[s_group_id].transport_stream_id,
			m_recordings[s_group_id].original_network_id,
			m_recordings[s_group_id].service_id,
			s_group_id,
			true // enable retry
		};

		CTunerServiceIf _if(get_external_if());
		_if.request_tune_advance (&param);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {

			if (m_recordings[s_group_id].is_event_type) {
				// イベント開始時間を確認します
				section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				act = threadmgr::action::continue_;

			} else {
				// manual録画は即 録画開始します
				section_id = SECTID_START_RECORDING;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_E ("req_tune is failure.");
			_UTL_LOG_E ("tune is failure  -> not start recording");

			m_recordings[s_group_id].state |= RESERVE_STATE__END_ERROR__TUNE_ERR;
			set_result (&m_recordings[s_group_id]);
			m_recordings[s_group_id].clear();

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {

		psisi_structs::service_info_t _svc_info ;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = m_recordings[s_group_id].transport_stream_id;
		_svc_info.original_network_id = m_recordings[s_group_id].original_network_id;
		_svc_info.service_id = m_recordings[s_group_id].service_id;

		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_present_event_info (&_svc_info, &s_present_event_info);

		section_id = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {

			if (s_present_event_info.event_id == m_recordings[s_group_id].event_id) {
				// 録画開始します
				section_id = SECTID_START_RECORDING;
				act = threadmgr::action::continue_;

			} else {
				// イベントが一致しないので 前番組の延長とかで開始時間が遅れたと予想します
				// --> EIT schedule を取得してみます
				_UTL_LOG_E ("(%s) event tracking...", p_if->get_sequence_name());
				section_id = SECTID_REQ_CACHE_SCHEDULE;
				act = threadmgr::action::continue_;
			}

		} else {
			if (s_retry_get_event_info >= 10) {
				_UTL_LOG_E ("(%s) req_get_present_event_info err", p_if->get_sequence_name());

				m_recordings[s_group_id].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
				set_result (&m_recordings[s_group_id]);
				m_recordings[s_group_id].clear();

				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;

			} else {
				// workaround
				// たまにエラーになることがあるので 暫定対策として200m_s待ってリトライしてみます
				// psi/siの選局完了時に確実にEIT p/fを取得できてないのが直接の原因だと思われます
				_UTL_LOG_W ("(%s) req_get_present_event_info retry", p_if->get_sequence_name());

				usleep (200000); // 200m_s
				++ s_retry_get_event_info;

				section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				act = threadmgr::action::continue_;
			}
		}
		break;

	case SECTID_REQ_CACHE_SCHEDULE: {

		CEventScheduleManagerIf _if (get_external_if());
		_if.request_cache_schedule_force_current_service (s_group_id);

		section_id = SECTID_WAIT_CACHE_SCHEDULE;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_CACHE_SCHEDULE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {

			// イベントを検索して予約を入れなおしてみます
			_UTL_LOG_E ("(%s) try reserve reschedule", p_if->get_sequence_name());
			section_id = SECTID_REQ_ADD_RESERVE_RESCHEDULE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("(%s) req_cache_schedule_force_current_service err", p_if->get_sequence_name());

			m_recordings[s_group_id].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			set_result (&m_recordings[s_group_id]);
			m_recordings[s_group_id].clear();

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_ADD_RESERVE_RESCHEDULE: {

		CRecManagerIf::add_reserve_param_t _param;
		_param.transport_stream_id = m_recordings[s_group_id].transport_stream_id ;
		_param.original_network_id = m_recordings[s_group_id].original_network_id;
		_param.service_id = m_recordings[s_group_id].service_id;
		_param.event_id = m_recordings[s_group_id].event_id;
		_param.repeatablity = m_recordings[s_group_id].repeatability;
		_param.dump();

		request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::add_reserve_event), (uint8_t*)&_param, sizeof(_param));

		section_id = SECTID_WAIT_ADD_RESERVE_RESCHEDULE;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_ADD_RESERVE_RESCHEDULE:

		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			_UTL_LOG_I ("(%s) add reserve reschedule ok.", p_if->get_sequence_name());

			// 今回は録画は行いません
			m_recordings[s_group_id].clear();

			// 選局止めたいのでエラーにしておきます
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("(%s) add reserve reschedule err...", p_if->get_sequence_name());

			m_recordings[s_group_id].state |= RESERVE_STATE__END_ERROR__EVENT_NOT_FOUND;
			set_result (&m_recordings[s_group_id]);
			m_recordings[s_group_id].clear();

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_START_RECORDING:

		// 録画開始
		// ここはm_rec_progressで判断いれとく
////		if (m_rec_progress == EN_REC_PROGRESS__INIT) {
		if (msp_rec_instances[s_group_id]->get_current_progress() == CRecInstance::progress::INIT) {

			_UTL_LOG_I ("start recording (on tune thread)");

			memset (m_recording_tmpfiles[s_group_id], 0x00, PATH_MAX);
			std::string *p_path = mp_settings->getParams()->getRecTsPath();
			snprintf (
				m_recording_tmpfiles[s_group_id],
				PATH_MAX,
				"%s/tmp.m2ts.%lu.%d",
				p_path->c_str(),
				pthread_self(),
				s_group_id
			);


			// ######################################### //
////			m_rec_progress = EN_REC_PROGRESS__PRE_PROCESS;
			std::string s = m_recording_tmpfiles[s_group_id];
			msp_rec_instances[s_group_id]->set_rec_filename(s);
			msp_rec_instances[s_group_id]->set_service_id(m_recordings[s_group_id].service_id);
			msp_rec_instances[s_group_id]->set_use_splitter(CSettings::getInstance()->getParams()->isRecUseSplitter());
			msp_rec_instances[s_group_id]->set_next_progress(CRecInstance::progress::PRE_PROCESS);
			// ######################################### //


			m_recordings[s_group_id].state |= RESERVE_STATE__NOW_RECORDING;
			m_recordings[s_group_id].recording_start_time.setCurrentTime();

			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
////			_UTL_LOG_E ("m_rec_progress != EN_REC_PROGRESS__INIT ???  -> not start recording");
			_UTL_LOG_E ("progress != INIT ???  -> not start recording");

			m_recordings[s_group_id].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			set_result (&m_recordings[s_group_id]);
			m_recordings[s_group_id].clear();

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_END_SUCCESS:

//		memset (&s_present_event_info, 0x00, sizeof(s_present_event_info));
		s_present_event_info.clear();
		s_retry_get_event_info = 0;
		s_group_id = 0xff;
//		s_ch = 0;

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

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

//		memset (&s_present_event_info, 0x00, sizeof(s_present_event_info));
		s_present_event_info.clear();
		s_retry_get_event_info = 0;
		s_group_id = 0xff;
//		s_ch = 0;

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CRecManager::on_add_reserve_current_event (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_TUNER_STATE,
		SECTID_WAIT_GET_TUNER_STATE,
		SECTID_REQ_GET_PAT_DETECT_STATE,
		SECTID_WAIT_GET_PAT_DETECT_STATE,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static psisi_structs::service_info_t s_service_infos[10];
	static psisi_structs::event_info_t s_present_event_info;
	static uint8_t s_group_id = 0;


	switch (section_id) {
	case SECTID_ENTRY:
		s_group_id = *(uint8_t*)(p_if->get_source().get_message().data());
		if (s_group_id >= m_tuner_resource_max) {
			_UTL_LOG_E ("invalid group_id:[0x%02x]", s_group_id);
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_REQ_GET_TUNER_STATE;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_TUNER_STATE: {
		CTunerControlIf _if (get_external_if(), s_group_id);
		_if.request_get_state ();

		section_id = SECTID_WAIT_GET_TUNER_STATE;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_TUNER_STATE: {

		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			CTunerControlIf::tuner_state _state = *(CTunerControlIf::tuner_state*)(p_if->get_source().get_message().data());
			if (_state == CTunerControlIf::CTunerControlIf::tuner_state::tuning_success) {
				section_id = SECTID_REQ_GET_PAT_DETECT_STATE;
				act = threadmgr::action::continue_;
			} else {
				_UTL_LOG_E ("not CTunerControlIf::tuner_state__TUNING_SUCCESS %d", _state);
#ifdef _DUMMY_TUNER
				section_id = SECTID_REQ_GET_PAT_DETECT_STATE;
#else
				section_id = SECTID_END_ERROR;
#endif
				act = threadmgr::action::continue_;
			}

		} else {
			// success only
		}

		}
		break;

	case SECTID_REQ_GET_PAT_DETECT_STATE: {
		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_pat_detect_state ();

		section_id = SECTID_WAIT_GET_PAT_DETECT_STATE;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_PAT_DETECT_STATE: {
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			CPsisiManagerIf::pat_detection_state _state = *(CPsisiManagerIf::pat_detection_state*)(p_if->get_source().get_message().data());
			if (_state == CPsisiManagerIf::pat_detection_state::detected) {
				section_id = SECTID_REQ_GET_SERVICE_INFOS;
				act = threadmgr::action::continue_;
			} else {
				_UTL_LOG_E ("not EN_PAT_DETECT_STATE__DETECTED %d", _state);
#ifdef _DUMMY_TUNER
				section_id = SECTID_REQ_GET_SERVICE_INFOS;
#else
				section_id = SECTID_END_ERROR;
#endif
				act = threadmgr::action::continue_;
			}

		} else {
			// success only
		}

		}
		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_current_service_infos (s_service_infos, 10);

		section_id = SECTID_WAIT_GET_SERVICE_INFOS;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			int num = *(int*)(p_if->get_source().get_message().data());
			if (num > 0) {
s_service_infos[0].dump();
				section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				act = threadmgr::action::continue_;

			} else {
				_UTL_LOG_E ("req_get_current_service_infos is 0");
				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_E ("req_get_current_service_infos err");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {
		CPsisiManagerIf _if (get_external_if(), s_group_id);
//TODO s_service_infos[0] 暫定0番目を使います 大概service_type=0x01でHD画質だろうか
		_if.request_get_present_event_info (&s_service_infos[0], &s_present_event_info);

		section_id = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		act = threadmgr::action::wait;

		} break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
s_present_event_info.dump();

			char *p_svc_name = NULL;
			CChannelManagerIf::service_id_param_t param = {
				s_present_event_info.transport_stream_id,
				s_present_event_info.original_network_id,
				s_present_event_info.service_id
			};
			CChannelManagerIf _if (get_external_if());
			_if.request_get_service_name_sync (&param); // sync wait
			rslt = get_if()->get_source().get_result();
			if (rslt == threadmgr::result::success) {
				p_svc_name = (char*)(get_if()->get_source().get_message().data());
			}

			bool r = add_reserve (
							s_present_event_info.transport_stream_id,
							s_present_event_info.original_network_id,
							s_present_event_info.service_id,
							s_present_event_info.event_id,
							&s_present_event_info.start_time,
							&s_present_event_info.end_time,
							s_present_event_info.event_name_char,
							p_svc_name,
							true, // is_event_type true
							CRecManagerIf::reserve_repeatability::none
						);
			if (r) {
				section_id = SECTID_END_SUCCESS;
				act = threadmgr::action::continue_;

			} else {
				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_E ("(%s) req_get_present_event_info err", p_if->get_sequence_name());
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_END_SUCCESS:

		memset (s_service_infos, 0x00, sizeof(s_service_infos));
//		memset (&s_present_event_info, 0x00, sizeof(s_present_event_info));
		s_present_event_info.clear();

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

		memset (s_service_infos, 0x00, sizeof(s_service_infos));
//		memset (&s_present_event_info, 0x00, sizeof(s_present_event_info));
		s_present_event_info.clear();

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CRecManager::on_add_reserve_event (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_EVENT,
		SECTID_WAIT_GET_EVENT,
		SECTID_ADD_RESERVE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static CRecManagerIf::add_reserve_param_t s_param;
	static CEventScheduleManagerIf::event_t s_event;
	static bool s_is_rescheduled = false;


	switch (section_id) {
	case SECTID_ENTRY: {

		s_param = *(CRecManagerIf::add_reserve_param_t*)(p_if->get_source().get_message().data());

		// repeatablityのチェック入れときます
		if (
			s_param.repeatablity == CRecManagerIf::reserve_repeatability::none ||
			s_param.repeatablity == CRecManagerIf::reserve_repeatability::auto_
		) {
			section_id = SECTID_REQ_GET_EVENT;
			act = threadmgr::action::continue_;
		} else {
			_UTL_LOG_E ("invalid repeatablity");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		// rescheduleかのチェックします
		uint8_t src_thread_idx = p_if->get_source().get_thread_idx();
		uint8_t src_seq_idx = p_if->get_source().get_sequence_idx();
		if ((src_thread_idx == EN_MODULE_REC_MANAGER) && (src_seq_idx == static_cast<int>(CRecManagerIf::sequence::start_recording))) {
			_UTL_LOG_I ("(%s) requst from CRecManagerIf::sequence::start_recording", p_if->get_sequence_name());
			s_is_rescheduled = true;
		}

		}
		break;

	case SECTID_REQ_GET_EVENT: {

		CEventScheduleManagerIf::request_event_param_t param = {
			{
				s_param.transport_stream_id,
				s_param.original_network_id,
				s_param.service_id,
				s_param.event_id
			},
			&s_event
		};

		CEventScheduleManagerIf _if (get_external_if());
		_if.request_get_event (&param);

		section_id = SECTID_WAIT_GET_EVENT;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_EVENT:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_ADD_RESERVE;
			act = threadmgr::action::continue_;

		} else {
			// 予約に対応するイベントがなかった あらら...
			_UTL_LOG_E ("on_add_reserve_event - req_get_event error");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_ADD_RESERVE: {

		char *p_svc_name = NULL;
		CChannelManagerIf::service_id_param_t param = {
			s_param.transport_stream_id,
			s_param.original_network_id,
			s_param.service_id
		};
		CChannelManagerIf _if (get_external_if());
		_if.request_get_service_name_sync (&param); // sync wait
		rslt = get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			p_svc_name = (char*)(get_if()->get_source().get_message().data());
		}


		bool r = add_reserve (
						s_event.transport_stream_id,
						s_event.original_network_id,
						s_event.service_id,
						s_event.event_id,
						&s_event.start_time,
						&s_event.end_time,
						s_event.p_event_name->c_str(),
						p_svc_name,
						true, // is_event_type true
						s_param.repeatablity,
						s_is_rescheduled
					);
		if (r) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_END_SUCCESS:

//		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_param.clear();
		s_event.clear();
		s_is_rescheduled = false;

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

//		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_param.clear();
		s_event.clear();
		s_is_rescheduled = false;

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}


	p_if->set_section_id (section_id, act);
}

void CRecManager::on_add_reserve_event_helper (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_EVENT,
		SECTID_WAIT_GET_EVENT,
		SECTID_ADD_RESERVE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static CRecManagerIf::add_reserve_helper_param_t s_param;
	static CEventScheduleManagerIf::event_t s_event;


	switch (section_id) {
	case SECTID_ENTRY:

		s_param = *(CRecManagerIf::add_reserve_helper_param_t*)(p_if->get_source().get_message().data());


		// repeatablityのチェック入れときます
		if (
			s_param.repeatablity == CRecManagerIf::reserve_repeatability::none ||
			s_param.repeatablity == CRecManagerIf::reserve_repeatability::auto_
		) {
			section_id = SECTID_REQ_GET_EVENT;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_GET_EVENT: {

		CEventScheduleManagerIf::request_event_param_t param ;
		param.arg.index = s_param.index;
		param.p_out_event = &s_event;

		CEventScheduleManagerIf _if (get_external_if());
		_if.request_get_event_latest_dumped_schedule (&param);

		section_id = SECTID_WAIT_GET_EVENT;
		act = threadmgr::action::wait;

		}
		break;

	case SECTID_WAIT_GET_EVENT:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_ADD_RESERVE;
			act = threadmgr::action::continue_;

		} else {
			// 予約に対応するイベントがなかった あらら...
			_UTL_LOG_E ("req_get_event error");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_ADD_RESERVE: {

		char *p_svc_name = NULL;
		CChannelManagerIf::service_id_param_t param = {
			s_event.transport_stream_id,
			s_event.original_network_id,
			s_event.service_id
		};
		CChannelManagerIf _if (get_external_if());
		_if.request_get_service_name_sync (&param); // sync wait
		rslt = get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			p_svc_name = (char*)(get_if()->get_source().get_message().data());
		}


		bool r = add_reserve (
						s_event.transport_stream_id,
						s_event.original_network_id,
						s_event.service_id,
						s_event.event_id,
						&s_event.start_time,
						&s_event.end_time,
						s_event.p_event_name->c_str(),
						p_svc_name,
						true, // is_event_type true
						s_param.repeatablity
					);
		if (r) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_END_SUCCESS:

		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_event.clear();

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_event.clear();

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}


	p_if->set_section_id (section_id, act);
}

void CRecManager::on_add_reserve_manual (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_ADD_RESERVE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	static CRecManagerIf::add_reserve_param_t s_param;
	threadmgr::result rslt = threadmgr::result::success;


	switch (section_id) {
	case SECTID_ENTRY:

		s_param = *(CRecManagerIf::add_reserve_param_t*)(p_if->get_source().get_message().data());


		// repeatablityのチェック入れときます
		if (
			s_param.repeatablity >= CRecManagerIf::reserve_repeatability::none &&
			s_param.repeatablity <= CRecManagerIf::reserve_repeatability::weekly
		) {
			section_id = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {
		// サービスの存在チェックします

		CChannelManagerIf::service_id_param_t param = {
			s_param.transport_stream_id,
			s_param.original_network_id,
			s_param.service_id
		};

		CChannelManagerIf _if (get_external_if());
		_if.request_get_pysical_channel_by_service_id (&param);

		section_id = SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_ADD_RESERVE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("req_get_pysical_channel_by_service_id is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_ADD_RESERVE: {

		char *p_svc_name = NULL;
		CChannelManagerIf::service_id_param_t param = {
			s_param.transport_stream_id,
			s_param.original_network_id,
			s_param.service_id
		};
		CChannelManagerIf _if (get_external_if());
		_if.request_get_service_name_sync (&param); // sync wait
		rslt = get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			p_svc_name = (char*)(get_if()->get_source().get_message().data());
		}


		bool r = add_reserve (
						s_param.transport_stream_id,
						s_param.original_network_id,
						s_param.service_id,
						0x00, // event_idはこの時点では不明
						&s_param.start_time,
						&s_param.end_time,
						NULL, // タイトル名も不明 (そもそもs_paramにない)
						p_svc_name,
						false,
						s_param.repeatablity
					);
		if (r) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_END_SUCCESS:

//		memset (&s_param, 0x00, sizeof(s_param));
		s_param.clear();

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

//		memset (&s_param, 0x00, sizeof(s_param));
		s_param.clear();

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}


	p_if->set_section_id (section_id, act);
}

void CRecManager::on_remove_reserve (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CRecManagerIf::remove_reserve_param_t param =
			*(CRecManagerIf::remove_reserve_param_t*)(p_if->get_source().get_message().data());

	int idx = get_reserve_index (
					param.arg.key.transport_stream_id,
					param.arg.key.original_network_id,
					param.arg.key.service_id,
					param.arg.key.event_id
				);

	if (idx < 0) {
		p_if->reply (threadmgr::result::error);

	} else {
		if (remove_reserve (idx, param.is_consider_repeatability, param.is_apply_result)) {
			p_if->reply (threadmgr::result::success);
		} else {
			p_if->reply (threadmgr::result::error);
		}
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_remove_reserve_by_index (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CRecManagerIf::remove_reserve_param_t param =
			*(CRecManagerIf::remove_reserve_param_t*)(p_if->get_source().get_message().data());

	if (remove_reserve (param.arg.index, param.is_consider_repeatability, param.is_apply_result)) {
		p_if->reply (threadmgr::result::success);
	} else {
		p_if->reply (threadmgr::result::error);
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_get_reserves (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CRecManagerIf::get_reserves_param_t _param =
			*(CRecManagerIf::get_reserves_param_t*)(p_if->get_source().get_message().data());

	if (!_param.p_out_reserves) {
		_UTL_LOG_E ("_param.p_out_reserves is null.");
		p_if->reply (threadmgr::result::error);

	} else if (_param.array_max_num <= 0) {
		_UTL_LOG_E ("_param.array_max_num is invalid.");
		p_if->reply (threadmgr::result::error);

	} else {

		int n = get_reserves (_param.p_out_reserves, _param.array_max_num);
		// 取得数をreply msgで返します
		p_if->reply (threadmgr::result::success, (uint8_t*)&n, sizeof(n));
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_stop_recording (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	int group_id = *(uint8_t*)(p_if->get_source().get_message().data());
	if (group_id >= m_tuner_resource_max) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", group_id);
		p_if->reply (threadmgr::result::error);

	} else {
		if (m_recordings[group_id].state & RESERVE_STATE__NOW_RECORDING) {

			// stop_recording が呼ばれたらエラー終了にしておきます
////		_UTL_LOG_W ("m_rec_progress = EN_REC_PROGRESS__END_ERROR");
			_UTL_LOG_W ("progress = END_ERROR");


			// ######################################### //
////		m_rec_progress = EN_REC_PROGRESS__END_ERROR;
			msp_rec_instances[group_id]->set_next_progress(CRecInstance::progress::END_ERROR);
			// ######################################### //


			m_recordings[group_id].state |= RESERVE_STATE__FORCE_STOPPED;

			p_if->reply (threadmgr::result::success);

		} else {
			_UTL_LOG_E ("invalid rec state (not recording now...)");
			p_if->reply (threadmgr::result::error);
		}
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_dump_reserves (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	int type = *(int*)(p_if->get_source().get_message().data());
	switch (type) {
	case 0:
		dump_reserves();
		break;

	case 1:
		dump_results();
		break;

	case 2:
		dump_recording();
		break;

	default:
		break;
	}


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CRecManager::on_receive_notify (threadmgr::CThreadMgrIf *p_if)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		if (p_if->get_source().get_client_id() == m_tuner_notify_client_id[_gr]) {

			CTunerControlIf::tuner_state en_state = *(CTunerControlIf::tuner_state*)(p_if->get_source().get_message().data());
			switch (en_state) {
				case CTunerControlIf::tuner_state::tuning_begin:
				_UTL_LOG_I ("CTunerControlIf::tuner_state::tuning_begin group_id:[%d]", _gr);
				break;

				case CTunerControlIf::tuner_state::tuning_success:
				_UTL_LOG_I ("CTunerControlIf::tuner_state::tuning_success group_id:[%d]", _gr);
				break;

				case CTunerControlIf::tuner_state::tuning_error_stop:
				_UTL_LOG_I ("CTunerControlIf::tuner_state::tuning_error_stop group_id:[%d]", _gr);
				break;

				case CTunerControlIf::tuner_state::tune_stop:
				_UTL_LOG_I ("CTunerControlIf::tuner_state::tune_stop group_id:[%d]", _gr);
				break;

			default:
				break;
			}


		} else if (p_if->get_source().get_client_id() == m_pat_detect_notify_client_id[_gr]) {

			CPsisiManagerIf::pat_detection_state _state = *(CPsisiManagerIf::pat_detection_state*)(p_if->get_source().get_message().data());
			if (_state == CPsisiManagerIf::pat_detection_state::detected) {
				_UTL_LOG_I ("EN_PAT_DETECT_STATE__DETECTED");

			} else if (_state == CPsisiManagerIf::pat_detection_state::not_detected) {
				_UTL_LOG_E ("EN_PAT_DETECT_STATE__NOT_DETECTED");

				// PAT途絶したら録画中止します
				if (m_recordings[_gr].state & RESERVE_STATE__NOW_RECORDING) {

					// ######################################### //
////				m_rec_progress = EN_REC_PROGRESS__POST_PROCESS;
					msp_rec_instances[_gr]->set_next_progress(CRecInstance::progress::POST_PROCESS);
					// ######################################### //


					m_recordings[_gr].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;


					uint32_t opt = get_request_option ();
					opt |= REQUEST_OPTION__WITHOUT_REPLY;
					set_request_option (opt);

					// 自ら呼び出します
					// 内部で自リクエストするので
					// REQUEST_OPTION__WITHOUT_REPLY を入れときます
					//
					// PAT途絶してTs_receive_handlerは動いていない前提
////					this->on_ts_received (NULL, 0);
					msp_rec_instances[_gr]->on_ts_received (NULL, 0);

					opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
					set_request_option (opt);

				}
			}

		} else if (p_if->get_source().get_client_id() == m_event_change_notify_client_id[_gr]) {

			psisi_structs::notify_event_info_t _info = *(psisi_structs::notify_event_info_t*)(p_if->get_source().get_message().data());
			_UTL_LOG_I ("!!! event changed !!! group_id:[%d]", _gr);
			_info.dump ();

		}
	}
}

bool CRecManager::add_reserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id,
	CEtime* p_start_time,
	CEtime* p_end_time,
	const char *psz_title_name,
	const char *psz_service_name,
	bool _is_event_type,
	CRecManagerIf::reserve_repeatability repeatabilitiy,
	bool _is_rescheduled
)
{
	if (!p_start_time || !p_end_time) {
		return false;
	}

	if (*p_start_time >= *p_end_time) {
		_UTL_LOG_E ("invalid reserve time");
		return false;
	}

	if (!is_exist_empty_reserve ()) {
		_UTL_LOG_E ("reserve full.");
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
		psz_title_name,
		psz_service_name,
		_is_event_type,
		repeatabilitiy
	);


	if (is_duplicate_reserve (&tmp)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	if (is_overrap_time_reserve (&tmp)) {
		_UTL_LOG_W ("reserve time is overrap.");
// 時間被ってるけど予約入れます
//		return false;
	}

	if (!_is_rescheduled) {
		for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
			if (m_recordings[_gr].is_used) {
				if (m_recordings[_gr] == tmp) {
					_UTL_LOG_E ("reserve is now recording.");
					return false;
				}
			}
		}
	}


	CRecReserve* p_reserve = search_ascending_order_reserve (p_start_time);
	if (!p_reserve) {
		_UTL_LOG_E ("search_ascending_order_reserve failure.");
		return false;
	}

	p_reserve->set (
		_transport_stream_id,
		_original_network_id,
		_service_id,
		_event_id,
		p_start_time,
		p_end_time,
		psz_title_name,
		psz_service_name,
		_is_event_type,
		repeatabilitiy
	);

	if (_is_rescheduled) {
		p_reserve->state |= RESERVE_STATE__RESCHEDULED;
	}

	_UTL_LOG_I ("###########################");
	_UTL_LOG_I ("####    add reserve    ####");
	_UTL_LOG_I ("###########################");
	p_reserve->dump();


	// 毎回書き込み
	save_reserves ();


	return true;
}

/**
 * 引数をキーと同一予約のindexを返します
 * 見つからない場合は -1 で返します
 */
int CRecManager::get_reserve_index (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id
)
{
	CRecReserve tmp;
	tmp.set (
		_transport_stream_id,
		_original_network_id,
		_service_id,
		_event_id,
		NULL,
		NULL,
		NULL,
		NULL,
		false,
		CRecManagerIf::reserve_repeatability::none
	);

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves [i] == tmp) {
			return i;
		}
	}

	return -1;
}

/**
 * indexで指定した予約を削除します
 * 0始まり
 * is_consider_repeatability == false で Repeatability関係なく削除します
 */
bool CRecManager::remove_reserve (int index, bool is_consider_repeatability, bool is_apply_result)
{
	if (index >= RESERVE_NUM_MAX) {
		return false;
	}

	m_reserves [index].state |= RESERVE_STATE__REMOVED;
	if (is_apply_result) {
		set_result (&m_reserves[index]);
	}

	_UTL_LOG_I ("##############################");
	_UTL_LOG_I ("####    remove reserve    ####");
	_UTL_LOG_I ("##############################");
	m_reserves [index].dump();

	if (is_consider_repeatability) {
		check_repeatability (&m_reserves[index]);
	}

	// 間詰め
	for (int i = index; i < RESERVE_NUM_MAX -1; ++ i) {
		m_reserves [i] = m_reserves [i+1];
	}
	m_reserves [RESERVE_NUM_MAX -1].clear();


	// 毎回書き込み
	save_reserves ();


	return true;
}

/**
 * 開始時間を基準に降順で空きをさがします
 * 空きがある前提
 */
CRecReserve* CRecManager::search_ascending_order_reserve (CEtime *p_start_time_ref)
{
	if (!p_start_time_ref) {
		return NULL;
	}


	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {

		// 先頭から詰まっているはず
//		if (!m_reserves [i].is_used) {
//			continue;
//		}

		// 基準時間より後ろの時間を探します
		if (m_reserves [i].start_time > *p_start_time_ref) {

			// 後ろから見てずらします
			for (int j = RESERVE_NUM_MAX -1; j > i; -- j) {
				m_reserves [j] = m_reserves [j-1] ;
			}

			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		// 見つからなかったので最後尾にします
		return find_empty_reserve ();

	} else {

		m_reserves [i].clear ();
		return &m_reserves [i];
	}
}

bool CRecManager::is_exist_empty_reserve (void) const
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

CRecReserve* CRecManager::find_empty_reserve (void)
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

bool CRecManager::is_duplicate_reserve (const CRecReserve* p_reserve) const
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

bool CRecManager::is_overrap_time_reserve (const CRecReserve* p_reserve) const
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves [i].start_time >= p_reserve->start_time && m_reserves [i].start_time >= p_reserve->end_time) {
			continue;
		}

		if (m_reserves [i].end_time <= p_reserve->start_time && m_reserves [i].end_time <= p_reserve->end_time) {
			continue;
		}

		// overrap
		return true;
	}

	return false;
}

void CRecManager::check_reserves (void)
{
	CEtime current_time;
	current_time.setCurrentTime();

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {

		if (!m_reserves [i].is_used) {
			continue;
		}

		// 開始時間/終了時間とも過ぎていたら resultsに入れときます
		if (m_reserves [i].start_time < current_time && m_reserves [i].end_time <= current_time) {
			m_reserves [i].state |= RESERVE_STATE__END_ERROR__ALREADY_PASSED;
			set_result (&m_reserves[i]);
			check_repeatability (&m_reserves[i]);
			continue;
		}

		// 開始時間をみて 時間が来たら 録画要求を立てます
		// start_time - end_time 範囲内
		if (m_reserves [i].start_time <= current_time && m_reserves [i].end_time > current_time) {

			// 開始時間から10秒以上経過していたら フラグたてときます
			CEtime tmp = m_reserves [i].start_time;
			tmp.addSec(10);
			if (tmp <= current_time) {
				m_reserves [i].state |= RESERVE_STATE__START_TIME_PASSED;
			}

//			if (!m_recordings[0].is_used) {
//				// 録画中でなければ 録画開始フラグ立てます
// 録画してても関係なく録画開始フラグ立てます
				m_reserves [i].state |= RESERVE_STATE__REQ_START_RECORDING;

				// 見つかったのでbreakします。
				// 同時刻で予約が入っていた場合 配列の並び順で先の方を録画開始します
				// 先行の録画が終わらないと後続は始まらないです
				break;
//			}
		}
	}

	refresh_reserves (RESERVE_STATE__END_ERROR__ALREADY_PASSED);
}

/**
 * 引数のstateが立っている予約を除去します
 */
void CRecManager::refresh_reserves (uint32_t state)
{
	// 逆から見ます
	for (int i = RESERVE_NUM_MAX -1; i >= 0; -- i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (!(m_reserves[i].state & state)) {
			continue;
		}

		// 間詰め
		for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
			m_reserves [j] = m_reserves [j+1];
		}
		m_reserves [RESERVE_NUM_MAX -1].clear();

		save_reserves ();
	}
}

bool CRecManager::is_exist_req_start_recording_reserve (void)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves[i].state & RESERVE_STATE__REQ_START_RECORDING) {
			return true;
		}
	}

	return false;
}

bool CRecManager::pick_req_start_recording_reserve (uint8_t group_id)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves[i].state & RESERVE_STATE__REQ_START_RECORDING) {

			// 次に録画する予約を取り出します
			m_recordings[group_id] = m_reserves[i];

			// フラグは落としておきます
			m_recordings[group_id].state &= ~RESERVE_STATE__REQ_START_RECORDING;

			m_recordings[group_id].group_id = group_id;


			// 間詰め
			for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
				m_reserves [j] = m_reserves [j+1];
			}
			m_reserves [RESERVE_NUM_MAX -1].clear();

			check_repeatability (&m_recordings[group_id]);

			save_reserves ();

			return true;
		}
	}

	return false;
}

/**
 * 録画結果をリストに保存します
 */
void CRecManager::set_result (CRecReserve *p)
{
	if (!p) {
		return ;
	}


	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (!m_results [i].is_used) {
			m_results [i] = *p;

			// 毎回書き込み
			save_results ();

			return ;
		}
	}

	// m_results full
	// 最古のものを消します
	for (int i = 0; i < RESULT_NUM_MAX -1; ++ i) {
		m_results [i] = m_results [i + 1] ;		
	}

	m_results [RESULT_NUM_MAX -1].clear ();
	m_results [RESULT_NUM_MAX -1] = *p;


	// 毎回書き込み
	save_results ();
}

void CRecManager::check_recording_end (void)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		if (!m_recordings[_gr].is_used) {
			continue;
		}

		if (!(m_recordings[_gr].state & RESERVE_STATE__NOW_RECORDING)) {
			continue;
		}

		CEtime current_time ;
		current_time.setCurrentTime();

		if (m_recordings[_gr].end_time <= current_time) {

			// 録画 正常終了します
			if (msp_rec_instances[_gr]->get_current_progress() == CRecInstance::progress::NOW_RECORDING) {
				_UTL_LOG_I ("progress = END_SUCCESS");


				// ######################################### //
				msp_rec_instances[_gr]->set_next_progress(CRecInstance::progress::END_SUCCESS);
				// ######################################### //


			}
		}
	}
}

void CRecManager::check_disk_free (void)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		if (!m_recordings[_gr].is_used) {
			continue;
		}

		if (m_recordings[_gr].state & RESERVE_STATE__NOW_RECORDING) {

			std::string *p_path = mp_settings->getParams()->getRecTsPath();
			int limit = mp_settings->getParams()->getRecDiskSpaceLowLimitMB();
			int df = CUtils::getDiskFreeMB(p_path->c_str());
			if (df < limit) {
				_UTL_LOG_E ("(check_recording_disk_free) disk free space [%d] Mbytes !!", df);
				// HDD残量たりない場合は 録画を止めます
				m_recordings[_gr].state |= RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW;

				uint8_t group_id = _gr;

				uint32_t opt = get_request_option ();
				opt |= REQUEST_OPTION__WITHOUT_REPLY;
				set_request_option (opt);
				request_async (EN_MODULE_REC_MANAGER, static_cast<int>(CRecManagerIf::sequence::stop_recording), (uint8_t*)&group_id, sizeof(uint8_t));
				opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
				set_request_option (opt);
			}
		}
	}
}

//TODO event_typeの場合はどうするか
/**
 * Repeatabilityの確認して
 * 予約入れるべきものは予約します
 */
void CRecManager::check_repeatability (const CRecReserve *p_reserve)
{
	if (!p_reserve) {
		return ;
	}

	CEtime s;
	CEtime e;
	s = p_reserve->start_time;
	e = p_reserve->end_time;

	switch (p_reserve->repeatability) {
	case CRecManagerIf::reserve_repeatability::none:
		return;
		break;

	case CRecManagerIf::reserve_repeatability::daily:
		s.addDay(1);
		e.addDay(1);
		break;

	case CRecManagerIf::reserve_repeatability::weekly:
		s.addWeek(1);
		e.addWeek(1);
		break;

	default:
		_UTL_LOG_W ("invalid repeatability");
		return ;
		break;
	}

	bool r = add_reserve (
				p_reserve->transport_stream_id,
				p_reserve->original_network_id,
				p_reserve->service_id,
				p_reserve->event_id,
				&s,
				&e,
				p_reserve->title_name.c_str(),
				p_reserve->service_name.c_str(),
				p_reserve->is_event_type,
				p_reserve->repeatability
			);

	if (r) {
		_UTL_LOG_I ("add_reserve by repeatability -- success.");
	} else {
		_UTL_LOG_W ("add_reserve by repeatability -- failure.");
	}
}

int CRecManager::get_reserves (CRecManagerIf::reserve_t *p_out_reserves, int out_array_num) const
{
	if (!p_out_reserves || out_array_num <= 0) {
		return -1;
	}

	int n = 0;

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (out_array_num == 0) {
			break;
		}

		p_out_reserves->transport_stream_id = m_reserves [i].transport_stream_id;
		p_out_reserves->original_network_id = m_reserves [i].original_network_id;
		p_out_reserves->service_id = m_reserves [i].service_id;

		p_out_reserves->event_id = m_reserves [i].event_id;
		p_out_reserves->start_time = m_reserves [i].start_time;
		p_out_reserves->end_time = m_reserves [i].end_time;

		// アドレスで渡します
		p_out_reserves->p_title_name = const_cast<std::string*> (&m_reserves [i].title_name);
		p_out_reserves->p_service_name = const_cast<std::string*> (&m_reserves [i].service_name);

		p_out_reserves->is_event_type = m_reserves [i].is_event_type;
		p_out_reserves->repeatability = m_reserves [i].repeatability;

		++ p_out_reserves;
		-- out_array_num;
		++ n;
	}

	return n;
}

void CRecManager::dump_reserves (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (m_reserves [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			_UTL_LOG_I ("-- index=[%d] --", i);
			m_reserves [i].dump();
		}
	}
}

void CRecManager::dump_results (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (m_results [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_results [i].dump();
		}
	}
}

void CRecManager::dump_recording (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	int i = 0;
	for (i = 0; i < CGroup::GROUP_MAX; ++ i) {
		if (m_recordings[i].is_used) {
			_UTL_LOG_I ("-----------   now recording   -----------");
			m_recordings[i].dump();
		}
	}
}

void CRecManager::clear_reserves (void)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		m_reserves [i].clear();
	}
}

void CRecManager::clear_results (void)
{
	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		m_results [i].clear();
	}
}


#if 0
//////////  CTunerControlIf::ITsReceiveHandler  //////////

bool CRecManager::on_pre_ts_receive (void)
{
	get_external_if()->create_external_cp();

	uint32_t opt = get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	get_external_if()->set_request_option (opt);

	return true;
}

void CRecManager::on_post_ts_receive (void)
{
	uint32_t opt = get_external_if()->get_request_option ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	get_external_if()->set_request_option (opt);

	get_external_if()->destroy_external_cp();
}

bool CRecManager::on_check_ts_receive_loop (void)
{
	return true;
}

bool CRecManager::on_ts_received (void *p_ts_data, int length)
{
	switch (m_rec_progress) {
	case EN_REC_PROGRESS__PRE_PROCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__PRE_PROCESS");

		std::unique_ptr<CRec_arib_b25> b25 (new CRec_arib_b25(8192, m_recording_tmpfile));
		msp_b25.swap (b25);

		RECORDING_NOTICE_t _notice = {m_rec_progress, RESERVE_STATE__INIT};
		request_async (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_rec_progress = EN_REC_PROGRESS__NOW_RECORDING;
		_UTL_LOG_I ("next  EN_REC_PROGRESS__NOW_RECORDING");

		}
		break;

	case EN_REC_PROGRESS__NOW_RECORDING: {

		// recording
		int r = msp_b25->put ((uint8_t*)p_ts_data, length);
		if (r < 0) {
			_UTL_LOG_W ("TS write failed");
		}

		}
		break;

	case EN_REC_PROGRESS__END_SUCCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_SUCCESS");

		RECORDING_NOTICE_t _notice = {m_rec_progress, RESERVE_STATE__INIT};
		request_async (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_rec_progress = EN_REC_PROGRESS__POST_PROCESS;

		}
		break;

	case EN_REC_PROGRESS__END_ERROR: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_ERROR");

		RECORDING_NOTICE_t _notice = {m_rec_progress, RESERVE_STATE__INIT};
		request_async (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_rec_progress = EN_REC_PROGRESS__POST_PROCESS;

		}
		break;

	case EN_REC_PROGRESS__POST_PROCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__POST_PROCESS");

		msp_b25->flush();
		msp_b25->release();

		RECORDING_NOTICE_t _notice = {m_rec_progress, RESERVE_STATE__INIT};
		request_async (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_rec_progress = EN_REC_PROGRESS__INIT;

		}
		break;

	default:
		break;
	}

	return true;
}
#endif

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
void serialize (Archive &archive, CRecReserve &r)
{
	archive (
		cereal::make_nvp("transport_stream_id", r.transport_stream_id),
		cereal::make_nvp("original_network_id", r.original_network_id),
		cereal::make_nvp("service_id", r.service_id),
		cereal::make_nvp("event_id", r.event_id),
		cereal::make_nvp("start_time", r.start_time),
		cereal::make_nvp("end_time", r.end_time),
		cereal::make_nvp("title_name", r.title_name),
		cereal::make_nvp("service_name", r.service_name),
		cereal::make_nvp("is_event_type", r.is_event_type),
		cereal::make_nvp("repeatability", r.repeatability),
		cereal::make_nvp("state", r.state),
		cereal::make_nvp("recording_start_time", r.recording_start_time),
		cereal::make_nvp("recording_end_time", r.recording_end_time),
		cereal::make_nvp("group_id", r.group_id),
		cereal::make_nvp("is_used", r.is_used)
	);
}

void CRecManager::save_reserves (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_reserves), sizeof(CRecReserve) * RESERVE_NUM_MAX);
	}

	std::string *p_path = mp_settings->getParams()->getRecReservesJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CRecManager::load_reserves (void)
{
	std::string *p_path = mp_settings->getParams()->getRecReservesJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("rec_reserves.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_reserves), sizeof(CRecReserve) * RESERVE_NUM_MAX);

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tvnsecに書いてるので to_string用の文字はここで作ります
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		m_reserves [i].start_time.updateStrings();
		m_reserves [i].end_time.updateStrings();
		m_reserves [i].recording_start_time.updateStrings();
		m_reserves [i].recording_end_time.updateStrings();
	}
}

void CRecManager::save_results (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_results), sizeof(CRecReserve) * RESULT_NUM_MAX);
	}

	std::string *p_path = mp_settings->getParams()->getRecResultsJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CRecManager::load_results (void)
{
	std::string *p_path = mp_settings->getParams()->getRecResultsJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("rec_results.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_results), sizeof(CRecReserve) * RESULT_NUM_MAX);

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので to_string用の文字はここで作ります
	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		m_results [i].start_time.updateStrings();
		m_results [i].end_time.updateStrings();
		m_results [i].recording_start_time.updateStrings();
		m_results [i].recording_end_time.updateStrings();
	}
}
