#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/wait.h>     

#include "Group.h"
#include "TuneThread.h"

#include "modules.h"


int forker_log_print (FILE *fp, const char* format, ...)
{
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

CTuneThread::CTuneThread (std::string name, uint8_t que_max, uint8_t group_id)
	:threadmgr::CThreadMgrBase (name, que_max)
	,CGroup (group_id)
	,m_state (state::closed)
{
	const int _max = static_cast<int>(CTuneThread::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CTuneThread::on_module_up(p_if);},std::move("on_module_up")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTuneThread::on_module_down(p_if);}, std::move("on_module_down")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTuneThread::on_tune(p_if);}, std::move("tune")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTuneThread::on_force_kill(p_if);}, std::move("on_force_kill")},
	};
	set_sequences (seqs, _max);

	mp_settings = CSettings::getInstance();
	m_forker.set_log_cb (forker_log_print);
}

CTuneThread::~CTuneThread (void)
{
	reset_sequences();
}


void CTuneThread::on_module_up (threadmgr::CThreadMgrIf *p_if)
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

void CTuneThread::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CTuneThread::on_tune (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CTuneThread::tune_param_t param = *(CTuneThread::tune_param_t*)(p_if->get_source().get_message().data());

	if (m_state != state::closed) {
		_UTL_LOG_E ("m_state != closed");
		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->set_section_id (section_id, act);
		return;
	}

	// child process command
	std::vector<std::string> *p_tuner_hal_allocates = mp_settings->getParams()->getTunerHalAllocates();
	if (p_tuner_hal_allocates->size() <= getGroupId()) {
		_UTL_LOG_E ("not allocated tuner hal command... (settings.json ->tuner_hal_allocates)");
		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->set_section_id (section_id, act);
		return;
	}
	std::string com_form = (*p_tuner_hal_allocates) [getGroupId()];
	char com_str [128] = {0};
	snprintf (com_str, sizeof(com_str), com_form.c_str(), CTsAribCommon::freqKHz2physicalCh(param.freq));
	std::string command = com_str;

	if (!m_forker.create_pipes()) {
		_UTL_LOG_E("create_pipes failure.");
		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->set_section_id (section_id, act);
		m_forker.destroy_pipes();
		return;
	}

	if (!m_forker.do_fork(command)) {
		_UTL_LOG_E("do_fork failure.");
		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		p_if->set_section_id (section_id, act);
		m_forker.destroy_pipes();
		return;
	}

	_UTL_LOG_I ("m_state => opened");
	m_state = state::opened;

	// ここ以下はループが回りスレッド占有するので
	// 先にリプライしときます
	p_if->reply (threadmgr::result::success);

	_UTL_LOG_I ("m_state => tune_begined");
	m_state = state::tune_begined;

	{ //---------------------------
		_UTL_LOG_I ("on_pre_ts_receive");
		std::lock_guard<std::mutex> lock (*param.p_mutex_ts_receive_handlers);
		CTunerControlIf::ITsReceiveHandler **p_handlers = param.p_ts_receive_handlers;
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->on_pre_ts_receive ();
			}
		}
	} //---------------------------

	int r = 0;
//	int fd = pipeC2P[0];
//	int fd_err = pipeC2P_err[0];
	int fd = m_forker.get_child_stdout_fd();
	int fd_err = m_forker.get_child_stderr_fd();
	uint8_t buff [1024] = {0};
	uint64_t _total_size = 0;

	fd_set _fds;
	struct timeval _timeout;

	while (1) {
		FD_ZERO (&_fds);
		FD_SET (fd, &_fds);
		FD_SET (fd_err, &_fds);
		_timeout.tv_sec = 1;
		_timeout.tv_usec = 0;
		r = select (fd > fd_err ? fd + 1 : fd_err + 1, &_fds, NULL, NULL, &_timeout);
		if (r < 0) {
			_UTL_PERROR ("select");
			continue;

		} else if (r == 0) {
			// timeout
			// check tuner stop request
			if (*(param.p_is_req_stop)) {
				_UTL_LOG_I ("req stoped. (timeout check)");
				break;
			}
		}

		// read stdout
		if (FD_ISSET (fd, &_fds)) {
			int _size = read (fd, buff, sizeof(buff));
			if (_size < 0) {
				_UTL_PERROR ("read");
				break;

			} else if (_size == 0) {
				// file end
				_UTL_LOG_I ("ts read end");
				break;

			} else {
				if (m_state == state::tune_begined) {
					_UTL_LOG_I ("first ts read _size=[%d]", _size);
					_UTL_LOG_I ("m_state => tuned");
					m_state = state::tuned;
				}

				{ //---------------------------
					std::lock_guard<std::mutex> lock (*param.p_mutex_ts_receive_handlers);
					CTunerControlIf::ITsReceiveHandler **p_handlers = param.p_ts_receive_handlers;
					for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
						if (p_handlers[i]) {
							(p_handlers[i])->on_ts_received (buff, _size);
						}
					}
				} //---------------------------

				_total_size += _size;

				// check tuner stop request
				if (*(param.p_is_req_stop)) {
					_UTL_LOG_I ("req stoped.");
					break;
				}
			}
		}

		// read stderr
		if (FD_ISSET (fd_err, &_fds)) {
			memset (buff, 0x00, sizeof(buff));
			int _size = read (fd_err, buff, sizeof(buff));
			if (_size < 0) {
				_UTL_PERROR ("read");
				break;

			} else if (_size == 0) {
				// file end
				_UTL_LOG_I ("ts read end");
				break;

			} else {
				_UTL_LOG_I ("%s", buff);

				// check tuner stop request
				if (*(param.p_is_req_stop)) {
					_UTL_LOG_I ("req stoped.");
					break;
				}
			}
		}
	}
	_UTL_LOG_I ("m_state => tune_ended");
	m_state = state::tune_ended;

	_UTL_LOG_I ("ts read total_size=[%llu]", _total_size);

	{ //---------------------------
		_UTL_LOG_I ("on_post_ts_receive");
		std::lock_guard<std::mutex> lock (*param.p_mutex_ts_receive_handlers);
		CTunerControlIf::ITsReceiveHandler **p_handlers = param.p_ts_receive_handlers;
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->on_post_ts_receive ();
			}
		}
	} //---------------------------

	m_forker.wait_child(SIGUSR1);
	m_forker.destroy_pipes();

	_UTL_LOG_I ("m_state => clsoed");
	m_state = state::closed;


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTuneThread::on_force_kill (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	m_forker.wait_child(SIGUSR1);
	m_forker.destroy_pipes();

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}
