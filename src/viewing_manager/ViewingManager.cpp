#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#include "ChannelManagerIf.h"
#include "Etime.h"
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
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_start_viewing(p_if);}, std::move("on_start_viewing_by_physical_channel")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_start_viewing(p_if);}, std::move("on_start_viewing_by_service_id")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_stop_viewing(p_if);}, std::move("on_stop_viewing")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_get_viewing(p_if);}, std::move("on_get_viewing")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_set_option(p_if);}, std::move("on_set_option")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_dump_viewing(p_if);}, std::move("on_dump_viewing")},
		{[&](threadmgr::CThreadMgrIf *p_if){CViewingManager::on_event_changed(p_if);}, std::move("on_event_changed")},
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
			static_cast<uint8_t>(modules::module_id::viewing_manager),
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
		request_async (static_cast<uint8_t>(modules::module_id::viewing_manager), static_cast<uint8_t>(CViewingManagerIf::sequence::check_loop));

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

		// check auto stop
		for (int i = 0; i < CGroup::GROUP_MAX; ++ i) {
			if (m_viewings[i].is_used && m_viewings[i].auto_stop) {
				CEtime current_time ;
				current_time.setCurrentTime();
				if (m_viewings[i].auto_stop_time <= current_time) {
					// auto stopの時刻を過ぎたので 視聴を止めます
					_UTL_LOG_I("### auto stop - group:[%d] ###", m_viewings[i].group_id);
					m_viewings[i].dump();

					uint32_t opt = get_request_option ();
					opt |= REQUEST_OPTION__WITHOUT_REPLY;
					set_request_option (opt);
					request_async (
						static_cast<uint8_t>(modules::module_id::viewing_manager),
						static_cast<uint8_t>(CViewingManagerIf::sequence::stop_viewing),
						(uint8_t*)&m_viewings[i].group_id,
						sizeof(uint8_t)
					);
					opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
					set_request_option (opt);
				}
			}
		}

		// check child
		for (int i = 0; i < CGroup::GROUP_MAX; ++ i) {
			if (!m_forker[i].has_child()) {
				continue;
			}

			CForker::CChildStatus cs = m_forker[i].wait_child(-1, true);
			if (cs.get_status() == 0) {
				// alive
				if (m_viewings[i].is_used) {
					logging_stream_command(i);
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
		_UTL_LOG_I ("CStreamHandler::progress::process_end_error");
		break;

	case CStreamHandler::progress::post_process:
	case CStreamHandler::progress::post_process_forcibly:
		_UTL_LOG_I ("CStreamHandler::progress::post_process");

		m_forker[_notice.group_id].wait_child(SIGINT);
		m_forker[_notice.group_id].destroy_pipes();

//TODO この位置でclearはどうなのか  tuner notifyで終了したらclearの方がいいかも
		m_viewings[_notice.group_id].clear ();

		{
			std::string *stream_path = CSettings::getInstance()->getParams()->getViewingStreamDataPath();
			std::string path = *stream_path + "/";
			path += std::to_string(_notice.group_id);
			clean_dir(path.c_str());
		}

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

void CViewingManager::on_start_viewing (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_SERVICE_ID_BY_PHYSICAL_CH,
		SECTID_WAIT_GET_SERVICE_ID_BY_PHYSICAL_CH,
		SECTID_REQ_GET_PHYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PHYSICAL_CH_BY_SERVICE_ID,
		SECTID_REQ_OPEN,
		SECTID_WAIT_OPEN,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
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
	static CViewingManagerIf::service_id_param_t s_service_id_param;
	static psisi_structs::service_info_t s_service_info;
	static psisi_structs::event_info_t s_present_event_info;
	static int s_retry_get_event_info;
	static uint8_t s_group_id = 0xff;
	static uint16_t s_ch = 0;


	switch (section_id) {
	case SECTID_ENTRY:
		p_if->lock();

		_UTL_LOG_I ("(%s) entry", p_if->get_sequence_name());

		if (p_if->get_sequence_idx() == static_cast<uint8_t>(CViewingManagerIf::sequence::start_viewing_by_physical_channel)) {
			s_physical_channel_param = *(reinterpret_cast<CViewingManagerIf::physical_channel_param_t*>(p_if->get_source().get_message().data()));
			_UTL_LOG_I("phy_ch: %d", s_physical_channel_param.physical_channel);
			_UTL_LOG_I("svc_idx: %d", s_physical_channel_param.service_idx);

			s_ch = s_physical_channel_param.physical_channel;

			section_id = SECTID_REQ_GET_SERVICE_ID_BY_PHYSICAL_CH;
			act = threadmgr::action::continue_;
			
		} else if (p_if->get_sequence_idx() == static_cast<uint8_t>(CViewingManagerIf::sequence::start_viewing_by_service_id)) {
			s_service_id_param = *(reinterpret_cast<CViewingManagerIf::service_id_param_t*>(p_if->get_source().get_message().data()));
			_UTL_LOG_I("ts_id: %d", s_service_id_param.transport_stream_id);
			_UTL_LOG_I("org_nid: %d", s_service_id_param.original_network_id);
			_UTL_LOG_I("svc_id: %d", s_service_id_param.service_id);

			s_service_info.transport_stream_id = s_service_id_param.transport_stream_id;
			s_service_info.original_network_id = s_service_id_param.original_network_id;
			s_service_info.service_id = s_service_id_param.service_id;

			section_id = SECTID_REQ_GET_PHYSICAL_CH_BY_SERVICE_ID;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("unexpected seq idx [%d]", p_if->get_sequence_idx());
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
			break;
		}

		break;

	case SECTID_REQ_GET_SERVICE_ID_BY_PHYSICAL_CH: {
		CChannelManagerIf::request_service_id_param_t param = {
			s_physical_channel_param.physical_channel,
			s_physical_channel_param.service_idx,
		};
		CChannelManagerIf _if (get_external_if());
		_if.request_get_service_id_by_physical_channel(&param);

		section_id = SECTID_WAIT_GET_SERVICE_ID_BY_PHYSICAL_CH;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_SERVICE_ID_BY_PHYSICAL_CH:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			CChannelManagerIf::service_id_param_t *param = reinterpret_cast<CChannelManagerIf::service_id_param_t*>(p_if->get_source().get_message().data());
			s_service_info.transport_stream_id = param->transport_stream_id;
			s_service_info.original_network_id = param->original_network_id;
			s_service_info.service_id = param->service_id;

			section_id = SECTID_REQ_OPEN;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_get_service_id_by_physical_channel is failure.");

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_PHYSICAL_CH_BY_SERVICE_ID: {
		CChannelManagerIf::service_id_param_t param = {
			s_service_info.transport_stream_id,
			s_service_info.original_network_id,
			s_service_info.service_id
		};
		CChannelManagerIf _if (get_external_if());
		_if.request_get_physical_channel_by_service_id(&param);

		section_id = SECTID_WAIT_GET_PHYSICAL_CH_BY_SERVICE_ID;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PHYSICAL_CH_BY_SERVICE_ID:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_ch = *(reinterpret_cast<uint16_t*>(p_if->get_source().get_message().data()));

			section_id = SECTID_REQ_OPEN;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("request_get_service_id_by_physical_channel is failure.");

			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
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
			s_ch,
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
		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_present_event_info (&s_service_info, &s_present_event_info);

		section_id = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
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

	case SECTID_START_VIEWING: {
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
			s_service_info.transport_stream_id,
			s_service_info.original_network_id,
			s_service_info.service_id,
			s_present_event_info.event_id,
			&s_present_event_info.start_time,
			&s_present_event_info.end_time,
			s_present_event_info.event_name_char,
			svc_name,
			s_group_id
		);

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

			std::string *stream_path = CSettings::getInstance()->getParams()->getViewingStreamDataPath();
			std::string *fmt = CSettings::getInstance()->getParams()->getViewingStreamCommandFormat();
			char command[1024] = {0};
			std::string gr = std::to_string(s_group_id);
			snprintf(command, sizeof(command), fmt->c_str(), stream_path->c_str(), gr.c_str(), stream_path->c_str(), gr.c_str());
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

		}
		break;

	case SECTID_END_SUCCESS:

		p_if->reply (threadmgr::result::success, reinterpret_cast<uint8_t*>(&s_group_id), sizeof(s_group_id));

		memset (&s_physical_channel_param, 0x00, sizeof(s_physical_channel_param));
		memset (&s_service_id_param, 0x00, sizeof(s_service_id_param));
		memset (&s_service_info, 0x00, sizeof(s_service_info));
		s_present_event_info.clear();
		s_retry_get_event_info = 0;
		s_group_id = 0xff;
		s_ch = 0;

		p_if->unlock();

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:

		if (s_group_id < CGroup::GROUP_MAX) {
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
		}

		p_if->reply (threadmgr::result::error);

		memset (&s_physical_channel_param, 0x00, sizeof(s_physical_channel_param));
		memset (&s_service_id_param, 0x00, sizeof(s_service_id_param));
		memset (&s_service_info, 0x00, sizeof(s_service_info));
		s_present_event_info.clear();
		s_retry_get_event_info = 0;
		s_group_id = 0xff;
		s_ch = 0;

		p_if->unlock();

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
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

void CViewingManager::on_get_viewing (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);




	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CViewingManager::on_set_option (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	CViewingManagerIf::option_t opt = *(reinterpret_cast<CViewingManagerIf::option_t*>(p_if->get_source().get_message().data()));
	if (opt.group_id >= m_tuner_resource_max) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", opt.group_id);
		p_if->reply (threadmgr::result::error);

	} else {
		if (m_viewings[opt.group_id].is_used) {
			if (opt.auto_stop_grace_period_sec > 0) {
				CEtime auto_stop_target_time;
				auto_stop_target_time.setCurrentTime();
				auto_stop_target_time.addSec(opt.auto_stop_grace_period_sec);

				m_viewings[opt.group_id].auto_stop = true;
				m_viewings[opt.group_id].auto_stop_time = auto_stop_target_time;
				_UTL_LOG_I("set auto_stop_time:[%s] group:[%d]", m_viewings[opt.group_id].auto_stop_time.toString(), m_viewings[opt.group_id].group_id);
			}
		}

		p_if->reply (threadmgr::result::success);
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

void CViewingManager::on_event_changed (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;

	static psisi_structs::event_info_t s_present_event_info;
	static uint8_t s_group_id = 0xff;


	switch (section_id) {
	case SECTID_ENTRY:
		_UTL_LOG_I ("(%s) entry", p_if->get_sequence_name());

		s_group_id = *(reinterpret_cast<uint8_t*>(p_if->get_source().get_message().data()));

		if (m_viewings[s_group_id].is_used) {
			section_id = SECTID_REQ_GET_PRESENT_EVENT_INFO;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_END;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {
		psisi_structs::service_info_t info;
		info.transport_stream_id = m_viewings->transport_stream_id;
		info.original_network_id = m_viewings->original_network_id;
		info.service_id = m_viewings->service_id;

		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_present_event_info (&info, &s_present_event_info);

		section_id = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			if (m_viewings[s_group_id].is_used) {
				m_viewings[s_group_id].event_id = s_present_event_info.event_id;
				m_viewings[s_group_id].start_time = s_present_event_info.start_time;
				m_viewings[s_group_id].end_time = s_present_event_info.end_time;
				m_viewings[s_group_id].title_name = s_present_event_info.event_name_char;
				m_viewings[s_group_id].dump();
			}
		}

		section_id = SECTID_END;
		act = threadmgr::action::continue_;
		break;
		
	case SECTID_END:
		s_present_event_info.clear();
		s_group_id = 0xff;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

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

			uint32_t opt = get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			request_async(
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(CViewingManagerIf::sequence::event_changed),
				reinterpret_cast<uint8_t*>(&_gr),
				sizeof(_gr)
			);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);
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
		if (m_forker[i].has_child()) {
			_UTL_LOG_I("ffmpeg:[%s]", m_forker[i].get_child_commandline().c_str());
			_UTL_LOG_I("pid:[%d]", m_forker[i].get_child_pid());
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

void CViewingManager::clean_dir (const char* path) const
{
	DIR *dp = opendir (path);
	if (dp == NULL) {
		return ;
	}

	dirent* entry = readdir(dp);
	while (entry != NULL){
		if (strlen(entry->d_name) == 1 && entry->d_name[0] == '.') {
			;;
		} else if (strlen(entry->d_name) == 2 && strncmp (entry->d_name, "..", strlen(entry->d_name)) == 0) {
			;;
		} else {
			std::string s = path ;
			s += "/";
			s += entry->d_name;
			if (remove (s.c_str()) == 0) {
				_UTL_LOG_I("%s removed", s.c_str());
			} else {
				_UTL_LOG_E("%s remove failed", s.c_str());
			}
		}
		entry = readdir(dp);
	}
}

//TODO ffmpegのログ出力
void CViewingManager::logging_stream_command (uint8_t group_id) const
{
	fd_set _fds;
	struct timeval _timeout;
	FD_ZERO (&_fds);
	FD_SET (m_forker[group_id].get_child_stderr_fd(), &_fds);
	_timeout.tv_sec = 1;
	_timeout.tv_usec = 0;
	int sr = select (m_forker[group_id].get_child_stderr_fd() +1, &_fds, NULL, NULL, &_timeout);
	if (sr < 0) {
		;;
	} else if (sr == 0) {
		// timeout
		;;
	} else {
		char buff [1024*4] = {0};
		size_t r = read (m_forker[group_id].get_child_stderr_fd(), buff, sizeof(buff));
		if (r > 0) {
			_UTL_LOG_I(buff);
		}
	}
}
