#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TunerControl.h"
#include "TuneThread.h"
#include "TunerControlIf.h"
#include "modules.h"


CTunerControl::CTunerControl (std::string name, uint8_t que_max, uint8_t group_id)
	:threadmgr::CThreadMgrBase (name, que_max)
	,CGroup (group_id)
	,m_freq (0)
	,m_wk_freq (0)
	,m_start_freq (0)
	,m_chkcnt (0)
	,m_state (CTunerControlIf::tuner_state::tune_stop)
	,m_is_req_stop (false)
{
	const int _max = static_cast<int>(CTunerControlIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_module_up(p_if);}, std::move("on_module_up")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_module_down(p_if);}, std::move("on_module_down")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_tune(p_if);}, std::move("on_tune")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_tune_start(p_if);}, std::move("on_tune_start")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_tune_stop(p_if);}, std::move("on_tune_stop")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_register_tuner_notify(p_if);}, std::move("on_register_tuner_notify")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_unregister_tuner_notify(p_if);}, std::move("on_unregister_tuner_notify")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_register_ts_receive_handler(p_if);}, std::move("on_register_ts_receive_handler")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_unregister_ts_receive_handler(p_if);}, std::move("on_unregister_ts_receive_handler")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerControl::on_get_state(p_if);}, std::move("on_get_state")},
	};
	set_sequences (seqs, _max);

	for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
		mp_reg_ts_receive_handlers [i] = NULL;
	}
}

CTunerControl::~CTunerControl (void)
{
	reset_sequences();
}


void CTunerControl::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_TUNE_THREAD_MODULE_UP,
		SECTID_WAIT_TUNE_THREAD_MODULE_UP,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	switch (section_id) {
	case SECTID_ENTRY:
		section_id = SECTID_REQ_TUNE_THREAD_MODULE_UP;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_TUNE_THREAD_MODULE_UP:
		request_async (static_cast<uint8_t>(modules::module_id::tune_thread) + getGroupId(),
						static_cast<int>(CTuneThread::sequence::module_up));

		section_id = SECTID_WAIT_TUNE_THREAD_MODULE_UP;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_TUNE_THREAD_MODULE_UP:
		section_id = SECTID_END;
		act = threadmgr::action::continue_;
		break;

	case SECTID_END:
		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CTunerControl::on_tune (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_REQ_TUNE_START,
		SECTID_WAIT_TUNE_START,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;

	switch (section_id) {
	case SECTID_ENTRY:
		m_wk_freq = *(uint32_t*)(p_if->get_source().get_message().data());
		_UTL_LOG_I ("freq [%d]kHz\n", m_wk_freq);
		if (m_freq == m_wk_freq) {
			_UTL_LOG_I ("already freq [%d]kHz\n", m_wk_freq);
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_REQ_TUNE_STOP;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE_STOP:
		request_async (static_cast<uint8_t>(modules::module_id::tuner_control) + getGroupId(),
						static_cast<int>(CTunerControlIf::sequence::tune_stop));
		section_id = SECTID_WAIT_TUNE_STOP;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_TUNE_STOP:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_REQ_TUNE_START;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE_START:
		request_async (static_cast<uint8_t>(modules::module_id::tuner_control) + getGroupId(),
						static_cast<int>(CTunerControlIf::sequence::tune_start), (uint8_t*)&m_wk_freq, sizeof(m_wk_freq));
		section_id = SECTID_WAIT_TUNE_START;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_TUNE_START:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_END_SUCCESS:
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->reply (threadmgr::result::success);
		break;

	case SECTID_END_ERROR:
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;

		//-----------------------------//
		{
			uint32_t opt = get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			// 選局を停止しときます tune stop
			// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
			request_async (static_cast<uint8_t>(modules::module_id::tuner_control) + getGroupId(),
							static_cast<int>(CTunerControlIf::sequence::tune_stop));

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);
		}
		//-----------------------------//

		p_if->reply (threadmgr::result::error);
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_tune_start (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_TUNE_THREAD_TUNE,
		SECTID_WAIT_TUNE_THREAD_TUNE,
		SECTID_CHECK_TUNED,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;

	switch (section_id) {
	case SECTID_ENTRY: {
		// lock
		p_if->lock();

		set_state (CTunerControlIf::tuner_state::tuning_begin);

		// fire notify
		CTunerControlIf::tuner_state notify = CTunerControlIf::tuner_state::tuning_begin;
		p_if->notify (_TUNER_NOTIFY, (uint8_t*)&notify, sizeof(CTunerControlIf::tuner_state));

		m_start_freq = *(uint32_t*)(p_if->get_source().get_message().data());

		section_id = SECTID_REQ_TUNE_THREAD_TUNE;
		act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE_THREAD_TUNE: {
		CTuneThread::tune_param_t _param = {
			m_start_freq,
			&m_mutex_ts_receive_handlers,
			mp_reg_ts_receive_handlers,
			&m_is_req_stop
		};
		request_async (static_cast<uint8_t>(modules::module_id::tune_thread) + getGroupId(),
						static_cast<int>(CTuneThread::sequence::tune), (uint8_t*)&_param, sizeof(_param));

		section_id = SECTID_WAIT_TUNE_THREAD_TUNE;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE_THREAD_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_CHECK_TUNED;
			m_chkcnt = 0;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_CHECK_TUNED:
		if (m_chkcnt > 40) {
			p_if->clear_timeout ();
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;

		} else {
			uint8_t _id = static_cast<uint8_t>(modules::module_id::tune_thread) + getGroupId();
			CTuneThread *p_tuneThread = (CTuneThread*)(modules::get_module(static_cast<modules::module_id>(_id)));
			if (p_tuneThread->get_state() != CTuneThread::state::tuned) {
					++ m_chkcnt ;
				p_if->set_timeout (200);
				section_id = SECTID_CHECK_TUNED;
				act = threadmgr::action::wait;
			} else {
				p_if->clear_timeout ();
				section_id = SECTID_END_SUCCESS;
				act = threadmgr::action::continue_;
			}
		}
		break;

	case SECTID_END_SUCCESS: {
		// unlock
		p_if->unlock();

		set_state (CTunerControlIf::tuner_state::tuning_success);

		// fire notify
		CTunerControlIf::tuner_state enNotify = CTunerControlIf::tuner_state::tuning_success;
		p_if->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(CTunerControlIf::tuner_state));

		m_freq = m_start_freq;
		m_chkcnt = 0;
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->reply (threadmgr::result::success);
		}
		break;

	case SECTID_END_ERROR: {
		// unlock
		p_if->unlock();

		set_state (CTunerControlIf::tuner_state::tuning_error_stop);

		// fire notify
		CTunerControlIf::tuner_state notify = CTunerControlIf::tuner_state::tuning_error_stop;
		p_if->notify (_TUNER_NOTIFY, (uint8_t*)&notify, sizeof(CTunerControlIf::tuner_state));

		m_chkcnt = 0;
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->reply (threadmgr::result::error);
		}
		break;

	default:
		break;
	};

	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_tune_stop (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	int chkcnt = 0;

	uint8_t _id = static_cast<uint8_t>(modules::module_id::tune_thread) + getGroupId();
	CTuneThread *p_tuneThread = (CTuneThread*)(modules::get_module(static_cast<modules::module_id>(_id)));
	if (p_tuneThread->get_state() == CTuneThread::state::tuned ||
			p_tuneThread->get_state() == CTuneThread::state::tune_begined) {
		m_is_req_stop = true;

		while (chkcnt < 300) {
			if (p_tuneThread->get_state() == CTuneThread::state::closed) {
				break;
			}

			usleep (100000);
			++ chkcnt;

			if (chkcnt == 150) {
				_UTL_LOG_W ("request force kill");

				uint32_t opt = get_request_option ();
				opt |= REQUEST_OPTION__WITHOUT_REPLY;
				set_request_option (opt);

				request_async (static_cast<uint8_t>(modules::module_id::tune_thread) + getGroupId(),
								static_cast<int>(CTuneThread::sequence::force_kill));
				opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
				set_request_option (opt);
			}
		}

		if (chkcnt < 300) {
			_UTL_LOG_I ("success tune stop.");
			_UTL_LOG_I ("chkcnt=[%d]", chkcnt);
			m_freq = 0;
			set_state (CTunerControlIf::tuner_state::tune_stop);

			// fire notify
			CTunerControlIf::tuner_state enNotify = CTunerControlIf::tuner_state::tune_stop;
			p_if->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(CTunerControlIf::tuner_state));

			p_if->reply (threadmgr::result::success);

		} else {
			// 100mS * 300
			_UTL_LOG_E ("tune stop transition failure. (tuned -> tune_ended)");
			p_if->reply (threadmgr::result::error);
		}

	} else {
		_UTL_LOG_I ("already tune stoped.");
		set_state (CTunerControlIf::tuner_state::tune_stop);

#ifdef _DUMMY_TUNER
		// fire notify
		EN_TUNER_STATE enNotify = CTunerControlIf::tuner_state::tune_stop;
		p_if->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));
#endif

		p_if->reply (threadmgr::result::success);
	}

	chkcnt = 0;
	m_is_req_stop = false;

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_register_tuner_notify (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	uint8_t clientId = 0;
	threadmgr::result rslt;
	bool r = p_if->reg_notify (_TUNER_NOTIFY, &clientId);
	if (r) {
		rslt = threadmgr::result::success;
	} else {
		rslt = threadmgr::result::error;
	}

	_UTL_LOG_I ("registerd clientId=[0x%02x]\n", clientId);

	// clientIdをreply msgで返す 
	p_if->reply (rslt, (uint8_t*)&clientId, sizeof(clientId));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_unregister_tuner_notify (threadmgr::CThreadMgrIf *p_if)
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
	// msgからclientIdを取得
	uint8_t clientId = *(p_if->get_source().get_message().data());
	bool r = p_if->unreg_notify (_TUNER_NOTIFY, clientId);
	if (r) {
		_UTL_LOG_I ("unregisterd clientId=[0x%02x]\n", clientId);
		rslt = threadmgr::result::success;
	} else {
		_UTL_LOG_E ("failure unregister clientId=[0x%02x]\n", clientId);
		rslt = threadmgr::result::error;
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_register_ts_receive_handler (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CTunerControlIf::ITsReceiveHandler *p_handler = *(CTunerControlIf::ITsReceiveHandler**)(p_if->get_source().get_message().data());
	_UTL_LOG_I ("p_handler %p\n", p_handler);

	int r = register_ts_receive_handler (p_handler);
	if (r >= 0) {
		p_if->reply (threadmgr::result::success, (uint8_t*)&r, sizeof(r));
	} else {
		p_if->reply (threadmgr::result::error);
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_unregister_ts_receive_handler (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	int client_id = *(int*)(p_if->get_source().get_message().data());
	unregister_ts_receive_handler (client_id);


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerControl::on_get_state (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	// reply msgに乗せます
	p_if->reply (threadmgr::result::success, (uint8_t*)&m_state, sizeof(m_state));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

int CTunerControl::register_ts_receive_handler (CTunerControlIf::ITsReceiveHandler *p_handler)
{
	if (!p_handler) {
		_UTL_LOG_E ("p_handler is null.");
		return -1;
	}

	std::lock_guard<std::mutex> lock (m_mutex_ts_receive_handlers);

	int i = 0;
	for (i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
		if (!mp_reg_ts_receive_handlers [i]) {
			mp_reg_ts_receive_handlers [i] = p_handler;
			break;
		}
	}

	if (i == TS_RECEIVE_HANDLER_REGISTER_NUM_MAX) {
		_UTL_LOG_E ("mp_reg_ts_receive_handlers is full.");
		return -1;
	} else {
		return i;
	}
}

void CTunerControl::unregister_ts_receive_handler (int id)
{
	if (id < 0 || id >= TS_RECEIVE_HANDLER_REGISTER_NUM_MAX) {
		return ;
	}

	std::lock_guard<std::mutex> lock (m_mutex_ts_receive_handlers);

	mp_reg_ts_receive_handlers [id] = NULL;
}

void CTunerControl::set_state (CTunerControlIf::tuner_state s)
{
	m_state = s;
}
