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

#include "group.h"
#include "tune_thread.h"

#include "utils.h"
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

namespace arec {
namespace tuner_control {

tune_thread::tune_thread (std::string name, uint8_t que_max, uint8_t group_id)
	:coseq::module_base (name, que_max)
	,arec::group (group_id)
	,state_ (state::closed)
{
	const int _max = static_cast<int>(tune_thread::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){tune_thread::on_module_up(iface);},std::move("on_module_up")},
		{[&](coseq::iface *iface){tune_thread::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){tune_thread::on_tune(iface);}, std::move("tune")},
		{[&](coseq::iface *iface){tune_thread::on_force_kill(iface);}, std::move("on_force_kill")},
	};
	set_sequences (seqs, _max);

	settings_ = arec::settings::get_instance();
	forker_.set_log_cb (forker_log_print);
}

tune_thread::~tune_thread (void)
{
	reset_sequences();
}


void tune_thread::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

////
// do something
//

	iface->reply (coseq::result::success);
}

void tune_thread::on_module_down (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

//
// do something
//

	iface->reply (coseq::result::success);
}

void tune_thread::on_tune (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	tune_thread::tune_param_t param = *(tune_thread::tune_param_t*)(iface->get_source().get_message().data());

	if (state_ != state::closed) {
		_UTL_LOG_E ("state_ != closed");
		iface->reply (coseq::result::error);
		return;
	}

	// child process command
	std::vector<std::string> tuner_hal_allocates = settings_->get_params().get_tuner_hal_allocates();
	if (tuner_hal_allocates.size() <= get_group_id()) {
		_UTL_LOG_E ("not allocated tuner hal command... (settings.json ->tuner_hal_allocates)");
		iface->reply (coseq::result::error);
		return;
	}
	std::string com_form = tuner_hal_allocates [get_group_id()];
	char com_str [128] = {0};
	snprintf (com_str, sizeof(com_str), com_form.c_str(), parser::ts_arib_common::freq_khz2physical_ch (param.freq));
	std::string command = com_str;

	if (!forker_.create_pipes()) {
		_UTL_LOG_E("create_pipes failure.");
		iface->reply (coseq::result::error);
		forker_.destroy_pipes();
		return;
	}

	if (!forker_.do_fork(command)) {
		_UTL_LOG_E("do_fork failure.");
		iface->reply (coseq::result::error);
		forker_.destroy_pipes();
		return;
	}

	_UTL_LOG_I ("state_ => opened");
	state_ = state::opened;

	// ここ以下はループが回りスレッド占有するので
	// 先にリプライしときます
	iface->reply (coseq::result::success);

	_UTL_LOG_I ("state_ => tune_begined");
	state_ = state::tune_begined;

	{ //---------------------------
		_UTL_LOG_I ("on_pre_ts_receive");
		std::lock_guard<std::mutex> lock (*param.p_mutex_ts_receive_handlers);
		client_iface::i_ts_receive_handler **p_handlers = param.p_ts_receive_handlers;
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->on_pre_ts_receive ();
			}
		}
	} //---------------------------

	int r = 0;
//	int fd = pipeC2P[0];
//	int fd_err = pipeC2P_err[0];
	int fd = forker_.get_child_stdout_fd();
	int fd_err = forker_.get_child_stderr_fd();
	uint8_t buff [1024] = {0};
	uint64_t _total_size = 0;

	fd_set _fds;
	struct timeval _timeout;

	Bps _Bps(60*2);
	_Bps.start();

	while (1) {
		_Bps.update(_total_size, [](float Bps){
			_UTL_LOG_I("receiving stream... %.3f MiB/sec", Bps/1024/1024);
		});

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
			continue;
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
				if (state_ == state::tune_begined) {
					_UTL_LOG_I ("first ts read _size=[%d]", _size);
					_UTL_LOG_I ("state_ => tuned");
					state_ = state::tuned;
				}

				{ //---------------------------
					std::lock_guard<std::mutex> lock (*param.p_mutex_ts_receive_handlers);
					client_iface::i_ts_receive_handler **p_handlers = param.p_ts_receive_handlers;
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
	_UTL_LOG_I ("state_ => tune_ended");
	state_ = state::tune_ended;

	_UTL_LOG_I ("ts read total_size=[%llu]", _total_size);

	{ //---------------------------
		_UTL_LOG_I ("on_post_ts_receive");
		std::lock_guard<std::mutex> lock (*param.p_mutex_ts_receive_handlers);
		client_iface::i_ts_receive_handler **p_handlers = param.p_ts_receive_handlers;
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->on_post_ts_receive ();
			}
		}
	} //---------------------------

	forker_.wait_child(SIGUSR1);
	forker_.destroy_pipes();

	_UTL_LOG_I ("state_ => clsoed");
	state_ = state::closed;

	// ※ reply はループ手前で送信済み(スレッド占有のため)。ここでは何もしない。
}

void tune_thread::on_force_kill (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	forker_.wait_child(SIGUSR1);
	forker_.destroy_pipes();

	iface->reply (coseq::result::success);
}

} // namespace tuner_control
} // namespace arec
