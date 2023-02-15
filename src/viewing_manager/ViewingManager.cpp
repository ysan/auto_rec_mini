#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "ChannelManagerIf.h"
#include "Group.h"
#include "PsisiManagerIf.h"
#include "StreamHandler.h"
#include "ViewingManager.h"

#include "Utils.h"
#include "ViewingManagerIf.h"
#include "modules.h"

#include "aribstr.h"
#include "threadmgr_if.h"


static int forker_log_print (FILE *fp, const char* format, ...) {
	char buff [128] = {0};
	va_list va;
	va_start (va, format);
	vsnprintf (buff, sizeof(buff), format, va);
	if (fp == stderr) {
		_UTL_LOG_E (buff);
	} else {
		_UTL_LOG_I (buff);
	}
	va_end (va);
	return 0;
}

CViewingManager::CViewingManager (std::string name, uint8_t que_max)
	:threadmgr::CThreadMgrBase (name, que_max)
	,m_tuner_resource_max (0)
{
	const uint8_t _max = static_cast<uint8_t>(CViewingManagerIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_module_up(p_if);}, std::move("on_module_up")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_module_up_by_group_id(p_if);}, std::move("on_module_up_by_group_id")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_module_down(p_if);}, std::move("on_module_down")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_check_loop(p_if);}, std::move("on_check_loop")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_notice_by_stream_handler(p_if);}, std::move("on_notice_by_stream_handler")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_start_viewing_by_physical_channel(p_if);}, std::move("on_start_viewing_by_physical_channel")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_start_viewing_by_service_id(p_if);}, std::move("on_start_viewing_by_service_id")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_stop_viewing(p_if);}, std::move("on_stop_viewing")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_dump_viewing(p_if);}, std::move("on_dump_viewing")},
	};
	set_sequences (seqs, _max);

	mp_settings = CSettings::getInstance();

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_viewings[_gr].clear();
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_forker[_gr].set_log_cb(forker_log_print);
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_tuner_notify_client_id [_gr] = 0xff;
		m_pat_detect_notify_client_id [_gr] = 0xff;
		m_event_change_notify_client_id [_gr] = 0xff;
	}
}

CViewingManager::~CViewingManager (void)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_viewings[_gr].clear();
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_forker[_gr].destroy_pipes();
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_tuner_notify_client_id [_gr] = 0xff;
		m_pat_detect_notify_client_id [_gr] = 0xff;
		m_event_change_notify_client_id [_gr] = 0xff;
	}

	reset_sequences();
}


void CViewingManager::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_MODULE_UP_BY_GROUPID,
		SECTID_WAIT_MODULE_UP_BY_GROUPID,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static uint8_t s_gr_cnt = 0;

	switch (section_id) {
	case SECTID_ENTRY: {

		// settingsを使って初期化する場合はmodule upで
		m_tuner_resource_max = CSettings::getInstance()->getParams()->getTunerHalAllocates()->size();

		std::string *stream_path = CSettings::getInstance()->getParams()->getViewingStreamDataPath();
		for (int i = 0; i < m_tuner_resource_max; ++ i) {
			std::string path = *stream_path + "/";
			path += std::to_string(i);
			CUtils::makedir(path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		}

		section_id = SECTID_REQ_MODULE_UP_BY_GROUPID;
		act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_MODULE_UP_BY_GROUPID:
		request_async (
			static_cast<uint8_t>(module::module_id::viewing_manager),
			static_cast<uint8_t>(CViewingManagerIf::sequence::module_up_by_groupid),
			reinterpret_cast<uint8_t*>(&s_gr_cnt),
			sizeof(s_gr_cnt)
		);

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
		request_async (static_cast<uint8_t>(module::module_id::viewing_manager), static_cast<uint8_t>(CViewingManagerIf::sequence::check_loop));

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

void CViewingManager::on_module_up_by_group_id (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
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
		s_group_id = *(reinterpret_cast<uint8_t*>(get_if()->get_source().get_message().data()));
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
			m_tuner_notify_client_id [s_group_id] = *(reinterpret_cast<uint8_t*>(p_if->get_source().get_message().data()));
			section_id = SECTID_REQ_REG_PAT_DETECT_NOTIFY;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_register_tuner_notify is failure.");
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
			m_pat_detect_notify_client_id [s_group_id] = *(reinterpret_cast<uint8_t*>(p_if->get_source().get_message().data()));
			section_id = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_register_pat_detect_notify is failure.");
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
			m_event_change_notify_client_id [s_group_id] = *(reinterpret_cast<uint8_t*>(p_if->get_source().get_message().data()));
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_register_event_change_notify is failure.");
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

void CViewingManager::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CViewingManager::on_check_loop (threadmgr::CThreadMgrIf *p_if)
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

		p_if->set_timeout (1000); // 1sec

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_WAIT: {

		for (int i = 0; i < CGroup::GROUP_MAX; ++ i) {
			if (!m_forker[i].has_child()) {
				continue;
			}

			CForker::CChildStatus cs = m_forker[i].wait_child(-1, true);
			if (cs.get_status() == 0) {
				// alive
				if (m_viewings[i].is_used) {
					//TODO ffmpegのログ出力
					char buff [1024*4] = {0};
					size_t r = read (m_forker[i].get_child_stderr_fd(), buff, sizeof(buff));
					if (r > 0) {
						_UTL_LOG_I(buff);
					}
				}

			} else if (cs.get_status() == 1 || cs.get_status() == 2) {
				// not alive
				// ffmpeg が落ちしまったので stream側も強制的に止めます
				// ######################################### //
				stream_handler_funcs::get_instance(i)->set_next_progress(CStreamHandler::progress::post_process_forcibly);
				// ######################################### //
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

void CViewingManager::on_notice_by_stream_handler (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CStreamHandler::notice_t _notice = *(reinterpret_cast<CStreamHandler::notice_t*>(p_if->get_source().get_message().data()));

	switch (_notice.progress) {
	case CStreamHandler::progress::init:
		break;

	case CStreamHandler::progress::pre_process:
		_UTL_LOG_I ("CStreamHandler::progress::pre_process");
		break;

	case CStreamHandler::progress::processing:
		break;

	case CStreamHandler::progress::process_end_success:
		_UTL_LOG_I ("CStreamHandler::progress::process_end_success");
		break;

	case CStreamHandler::progress::process_end_error:
		_UTL_LOG_I ("CStreamHandler::progress::process_end_success");
		break;

	case CStreamHandler::progress::post_process:
	case CStreamHandler::progress::post_process_forcibly:
		_UTL_LOG_I ("CStreamHandler::progress::post_process");

		m_forker[_notice.group_id].wait_child(SIGINT);
		m_forker[_notice.group_id].destroy_pipes();

//TODO この位置でclearはどうなのか  tuner notifyで終了したらclearの方がいいかも
		m_viewings[_notice.group_id].clear ();

		_UTL_LOG_I ("viewing end...");


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

		break;

	default:
		break;
	};

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CViewingManager::on_start_viewing_by_physical_channel (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_OPEN,
		SECTID_WAIT_OPEN,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_REQ_GET_SERVICE_ID_BY_PHY_CH,
		SECTID_WAIT_GET_SERVICE_ID_BY_PHY_CH,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_START_VIEWING,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;

	static CViewingManagerIf::physical_channel_param_t s_physical_channel_param;
	static CChannelManagerIf::service_id_param_t s_service_id_param;
	static psisi_structs::event_info_t s_present_event_info;
	static int s_retry_get_event_info;
	static uint8_t s_group_id = 0xff;


	switch (section_id) {
	case SECTID_ENTRY:

		_UTL_LOG_I ("(%s) entry", p_if->get_sequence_name());

		s_physical_channel_param = *(reinterpret_cast<CViewingManagerIf::physical_channel_param_t*>(p_if->get_source().get_message().data()));
		_UTL_LOG_I("phy_ch: %d", s_physical_channel_param.physical_channel);
		_UTL_LOG_I("svc_idx: %d", s_physical_channel_param.service_idx);

		section_id = SECTID_REQ_OPEN;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_OPEN: {
		CTunerServiceIf _if(get_external_if());
		_if.request_open ();

		section_id = SECTID_WAIT_OPEN;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_OPEN:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_group_id = *(reinterpret_cast<uint8_t*>(get_if()->get_source().get_message().data()));
			_UTL_LOG_I ("request_open group_id:[0x%02x]", s_group_id);

			if (m_viewings[s_group_id].is_used) {
				// m_recordingsのidxはrequest_openで取ったtuner_idで決まります
				// is_usedになってるはずない...
				_UTL_LOG_E ("??? m_viewings[group_id].is_used ???  group_id:[0x%02x]", s_group_id);
				m_viewings[s_group_id].clear();
			}

		} else {
			_UTL_LOG_E ("request_open is failure.");

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

	case SECTID_REQ_TUNE: {
		CTunerServiceIf::tune_param_t param = {
			s_physical_channel_param.physical_channel,
			s_group_id,
		};

		CTunerServiceIf _if(get_external_if());
		_if.request_tune_with_retry (&param);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_REQ_GET_SERVICE_ID_BY_PHY_CH;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_tune is failure.");
			_UTL_LOG_E ("tune is failure  -> not start viewing");

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_SERVICE_ID_BY_PHY_CH: {
		CChannelManagerIf::request_service_id_param_t param = {
			s_physical_channel_param.physical_channel,
			s_physical_channel_param.service_idx,
		};
		CChannelManagerIf _if (get_external_if());
		_if.request_get_service_id_by_pysical_channel(&param);

		section_id = SECTID_WAIT_GET_SERVICE_ID_BY_PHY_CH;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_SERVICE_ID_BY_PHY_CH:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_service_id_param = *(reinterpret_cast<CChannelManagerIf::service_id_param_t*>(p_if->get_source().get_message().data()));

			// イベント開始時間を確認します
			section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_tune is failure.");
			_UTL_LOG_E ("tune is failure  -> not start viewing");

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {
		psisi_structs::service_info_t _svc_info ;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = s_service_id_param.transport_stream_id;
		_svc_info.original_network_id = s_service_id_param.original_network_id;
		_svc_info.service_id = s_service_id_param.service_id;

		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_present_event_info (&_svc_info, &s_present_event_info);

		section_id = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			char *svc_name = nullptr;
			CChannelManagerIf::service_id_param_t param = {
				s_present_event_info.transport_stream_id,
				s_present_event_info.original_network_id,
				s_present_event_info.service_id
			};
			CChannelManagerIf _if (get_external_if());
			_if.request_get_service_name_sync (&param); // sync wait
			rslt = get_if()->get_source().get_result();
			if (rslt == threadmgr::result::success) {
				// この後のm_viewings[s_group_id].set内で実態コピーすること前提です
				svc_name = reinterpret_cast<char*>(get_if()->get_source().get_message().data());
			}

			m_viewings[s_group_id].set (
				s_service_id_param.transport_stream_id,
				s_service_id_param.original_network_id,
				s_service_id_param.service_id,
				s_present_event_info.event_id,
				&s_present_event_info.start_time,
				&s_present_event_info.end_time,
				s_present_event_info.event_name_char,
				svc_name,
				s_group_id
			);

			section_id = SECTID_START_VIEWING;
			act = threadmgr::action::continue_;

		} else {
			if (s_retry_get_event_info >= 10) {
				_UTL_LOG_E ("(%s) request_get_present_event_info err", p_if->get_sequence_name());

				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;

			} else {
				// workaround
				// たまにエラーになることがあるので 暫定対策として200mS待ってリトライしてみます
				// psi/siの選局完了時に確実にEIT p/fを取得できてないのが直接の原因だと思われます
				_UTL_LOG_W ("(%s) request_get_present_event_info retry", p_if->get_sequence_name());

				usleep (200000); // 200mS
				++ s_retry_get_event_info;

				section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				act = threadmgr::action::continue_;
			}
		}
		break;

	case SECTID_START_VIEWING:

		// 視聴開始
		// ここはstremahandler::progressで判断いれとく
		if (stream_handler_funcs::get_instance(s_group_id)->get_current_progress() == CStreamHandler::progress::init) {

			if (!m_forker[s_group_id].create_pipes()) {
				_UTL_LOG_E("create_pipes failure.");
				m_forker[s_group_id].destroy_pipes();

				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
				break;
			}

			std::string *path = CSettings::getInstance()->getParams()->getViewingStreamDataPath();
			std::string *fmt = CSettings::getInstance()->getParams()->getViewingStreamCommandFormat();
			char command[1024] = {0};
			std::string gr = std::to_string(s_group_id);
			snprintf(command, sizeof(command), fmt->c_str(), path->c_str(), gr.c_str(), path->c_str(), gr.c_str());
			std::string _c = command;
			if (!m_forker[s_group_id].do_fork(std::move(_c))) {
				_UTL_LOG_E("do_fork failure.");
				m_forker[s_group_id].destroy_pipes();

				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
				break;
			}

			int fd = m_forker[s_group_id].get_child_stdin_fd();
//			set_nonblocking_io(fd);
			auto handler = [fd](const void *buff, size_t size) -> int {
				int r = 0;
				while (size > 0) {
					r = write (fd, (uint8_t*)buff + r, size);
					if (r < 0) {
						_UTL_PERROR("write");
						return -1;
					} else if (r == 0) {
						_UTL_LOG_W("write return 0");
						break;
					} else {
						size -= r;
					}
				}
				return 0;
			};

			_UTL_LOG_I ("start viewing (on tune thread)");


			// ######################################### //
			stream_handler_funcs::get_instance(s_group_id)->set_service_id(m_viewings[s_group_id].service_id);
			stream_handler_funcs::get_instance(s_group_id)->set_use_splitter(CSettings::getInstance()->getParams()->isViewingUseSplitter());
			stream_handler_funcs::get_instance(s_group_id)->set_process_handler(std::move(handler));
			stream_handler_funcs::get_instance(s_group_id)->set_next_progress(CStreamHandler::progress::pre_process);
			// ######################################### //


			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("progress != init ???  -> not start viewing");

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_END_SUCCESS:

		memset (&s_physical_channel_param, 0x00, sizeof(s_physical_channel_param));
		memset (&s_service_id_param, 0x00, sizeof(s_service_id_param));
		s_present_event_info.clear();
		s_retry_get_event_info = 0;
		s_group_id = 0xff;

		p_if->reply (threadmgr::result::success, reinterpret_cast<uint8_t*>(&s_group_id), sizeof(s_group_id));
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

		m_viewings[s_group_id].clear();

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

		memset (&s_physical_channel_param, 0x00, sizeof(s_physical_channel_param));
		memset (&s_service_id_param, 0x00, sizeof(s_service_id_param));
		s_present_event_info.clear();
		s_retry_get_event_info = 0;
		s_group_id = 0xff;

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CViewingManager::on_start_viewing_by_service_id (threadmgr::CThreadMgrIf *p_if)
{

}

void CViewingManager::on_stop_viewing (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	int group_id = *(reinterpret_cast<uint8_t*>(p_if->get_source().get_message().data()));
	if (group_id >= m_tuner_resource_max) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", group_id);
		p_if->reply (threadmgr::result::error);

	} else {
		if (m_viewings[group_id].is_used) {

			_UTL_LOG_I ("progress = end_success");


			// ######################################### //
			stream_handler_funcs::get_instance(group_id)->set_next_progress(CStreamHandler::progress::process_end_success);
			// ######################################### //


			p_if->reply (threadmgr::result::success);

		} else {
			_UTL_LOG_E ("not viewing now...");
			p_if->reply (threadmgr::result::error);
		}
	}

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CViewingManager::on_dump_viewing (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	dump_viewing();


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CViewingManager::on_receive_notify (threadmgr::CThreadMgrIf *p_if)
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
				_UTL_LOG_I ("pat_detection_state::detected");

			} else if (_state == CPsisiManagerIf::pat_detection_state::not_detected) {
				_UTL_LOG_E ("pat_detection_state::not_detected");

				// PAT途絶したら視聴中止します
				if (m_viewings[_gr].is_used) {

					// ######################################### //
					stream_handler_funcs::get_instance(_gr)->set_next_progress(CStreamHandler::progress::post_process_forcibly);
					// ######################################### //

					uint32_t opt = get_request_option ();
					opt |= REQUEST_OPTION__WITHOUT_REPLY;
					set_request_option (opt);

					// 自ら呼び出します
					// 内部で自リクエストするので
					// REQUEST_OPTION__WITHOUT_REPLY を入れときます
					//
					// PAT途絶 -> ts_receive_handlerは動いていない前提
					stream_handler_funcs::get_instance(_gr)->on_ts_received (nullptr, 0);

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

void CViewingManager::dump_viewing (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < CGroup::GROUP_MAX; ++ i) {
		if (m_viewings[i].is_used) {
			m_viewings[i].dump();
		}
	}
}

bool CViewingManager::set_nonblocking_io (int fd) const
{
	int flags = 0;
	if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
		_UTL_PERROR("fcntl");
		return false;
	}

	flags |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) < 0) {
		_UTL_PERROR("fcntl");
		return false;
	}

	return true;
}
