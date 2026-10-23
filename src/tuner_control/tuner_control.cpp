#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "tuner_control.h"
#include "tune_thread.h"
#include "tuner_control_if.h"
#include "modules.h"


namespace arec {
namespace tuner_control {

manager::manager (std::string name, uint8_t que_max, uint8_t group_id)
	:coseq::module_base (name, que_max)
	,arec::group (group_id)
	,freq_ (0)
	,state_ (client_iface::tuner_state::tune_stop)
	,is_req_stop_ (false)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){manager::on_tune(iface);}, std::move("on_tune")},
		{[&](coseq::iface *iface){manager::on_tune_start(iface);}, std::move("on_tune_start")},
		{[&](coseq::iface *iface){manager::on_tune_stop(iface);}, std::move("on_tune_stop")},
		{[&](coseq::iface *iface){manager::on_register_tuner_notify(iface);}, std::move("on_register_tuner_notify")},
		{[&](coseq::iface *iface){manager::on_unregister_tuner_notify(iface);}, std::move("on_unregister_tuner_notify")},
		{[&](coseq::iface *iface){manager::on_register_ts_receive_handler(iface);}, std::move("on_register_ts_receive_handler")},
		{[&](coseq::iface *iface){manager::on_unregister_ts_receive_handler(iface);}, std::move("on_unregister_ts_receive_handler")},
		{[&](coseq::iface *iface){manager::on_get_state(iface);}, std::move("on_get_state")},
	};
	set_sequences (seqs, _max);

	for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
		reg_ts_receive_handlers_ [i] = NULL;
	}
}

manager::~manager (void)
{
	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// tune_thread の module_up を待つ(結果は見ない)
	iface->request (static_cast<uint8_t>(modules::module_id::tune_thread) + get_group_id(),
					static_cast<uint8_t>(tune_thread::sequence::module_up));

	iface->reply (coseq::result::success);
}

void manager::on_module_down (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

//
// do something
//

	iface->reply (coseq::result::success);
}

void manager::on_tune (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 1実行専用のスクラッチはローカルへ
	uint32_t wk_freq = *(uint32_t*)(iface->get_source().get_message().data());
	_UTL_LOG_I ("freq [%d]kHz\n", wk_freq);

	if (freq_ == wk_freq) {
		_UTL_LOG_I ("already freq [%d]kHz\n", wk_freq);
		iface->reply (coseq::result::success);
		return;
	}

	// tune stop
	{
		coseq::source &r = iface->request (static_cast<uint8_t>(modules::module_id::tuner_control) + get_group_id(),
						static_cast<uint8_t>(client_iface::sequence::tune_stop));
		if (r.get_result() != coseq::result::success) {
			// 選局を停止しときます(投げっぱなし)
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_control) + get_group_id(),
							static_cast<uint8_t>(client_iface::sequence::tune_stop));
			iface->reply (coseq::result::error);
			return;
		}
	}

	// tune start
	{
		coseq::source &r = iface->request (static_cast<uint8_t>(modules::module_id::tuner_control) + get_group_id(),
						static_cast<uint8_t>(client_iface::sequence::tune_start), (uint8_t*)&wk_freq, sizeof(wk_freq));
		if (r.get_result() != coseq::result::success) {
			// 選局を停止しときます(投げっぱなし)
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_control) + get_group_id(),
							static_cast<uint8_t>(client_iface::sequence::tune_stop));
			iface->reply (coseq::result::error);
			return;
		}
	}

	iface->reply (coseq::result::success);
}

void manager::on_tune_start (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// lock
	iface->lock();

	set_state (client_iface::tuner_state::tuning_begin);

	// fire notify
	{
		client_iface::tuner_state notify = client_iface::tuner_state::tuning_begin;
		iface->notify (_TUNER_NOTIFY, (uint8_t*)&notify, sizeof(client_iface::tuner_state));
	}

	// 1実行専用のスクラッチはローカルへ
	uint32_t start_freq = *(uint32_t*)(iface->get_source().get_message().data());

	// tune_thread へ tune 要求
	bool tuned = false;
	{
		tune_thread::tune_param_t _param = {
			start_freq,
			&mutex_ts_receive_handlers_,
			reg_ts_receive_handlers_,
			&is_req_stop_
		};
		coseq::source &r = iface->request (static_cast<uint8_t>(modules::module_id::tune_thread) + get_group_id(),
						static_cast<uint8_t>(tune_thread::sequence::tune), (uint8_t*)&_param, sizeof(_param));

		if (r.get_result() == coseq::result::success) {
			// tuned 遷移を待つ (200ms * 40 = 約8秒でタイムアウト)
			uint8_t _id = static_cast<uint8_t>(modules::module_id::tune_thread) + get_group_id();
			tune_thread *p_tuneThread = (tune_thread*)(modules::get_module(static_cast<modules::module_id>(_id)));
			for (int chkcnt = 0; chkcnt <= 40; ++ chkcnt) {
				if (p_tuneThread->get_state() == tune_thread::state::tuned) {
					tuned = true;
					break;
				}
				iface->wait_timeout (200);
			}
		}
	}

	// unlock
	iface->unlock();

	if (tuned) {
		set_state (client_iface::tuner_state::tuning_success);

		client_iface::tuner_state enNotify = client_iface::tuner_state::tuning_success;
		iface->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(client_iface::tuner_state));

		freq_ = start_freq;
		iface->reply (coseq::result::success);

	} else {
		set_state (client_iface::tuner_state::tuning_error_stop);

		client_iface::tuner_state notify = client_iface::tuner_state::tuning_error_stop;
		iface->notify (_TUNER_NOTIFY, (uint8_t*)&notify, sizeof(client_iface::tuner_state));

		iface->reply (coseq::result::error);
	}
}

void manager::on_tune_stop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	int chkcnt = 0;

	uint8_t _id = static_cast<uint8_t>(modules::module_id::tune_thread) + get_group_id();
	tune_thread *p_tuneThread = (tune_thread*)(modules::get_module(static_cast<modules::module_id>(_id)));
	if (p_tuneThread->get_state() == tune_thread::state::tuned ||
			p_tuneThread->get_state() == tune_thread::state::tune_begined) {
		is_req_stop_ = true;

		while (chkcnt < 300) {
			if (p_tuneThread->get_state() == tune_thread::state::closed) {
				break;
			}

			iface->wait_timeout (100);
			++ chkcnt;

			if (chkcnt == 150) {
				_UTL_LOG_W ("request force kill");
				// 投げっぱなし
				iface->send (static_cast<uint8_t>(modules::module_id::tune_thread) + get_group_id(),
								static_cast<uint8_t>(tune_thread::sequence::force_kill));
			}
		}

		if (chkcnt < 300) {
			_UTL_LOG_I ("success tune stop.");
			_UTL_LOG_I ("chkcnt=[%d]", chkcnt);
			freq_ = 0;
			set_state (client_iface::tuner_state::tune_stop);

			// fire notify
			client_iface::tuner_state enNotify = client_iface::tuner_state::tune_stop;
			iface->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(client_iface::tuner_state));

			iface->reply (coseq::result::success);

		} else {
			// 100mS * 300
			_UTL_LOG_E ("tune stop transition failure. (tuned -> tune_ended)");
			iface->reply (coseq::result::error);
		}

	} else {
		_UTL_LOG_I ("already tune stoped.");
		set_state (client_iface::tuner_state::tune_stop);

#ifdef _DUMMY_TUNER
		// fire notify
		EN_TUNER_STATE enNotify = client_iface::tuner_state::tune_stop;
		iface->notify (_TUNER_NOTIFY, (uint8_t*)&enNotify, sizeof(EN_TUNER_STATE));
#endif

		iface->reply (coseq::result::success);
	}

	is_req_stop_ = false;
}

void manager::on_register_tuner_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	uint8_t clientId = 0;
	coseq::result rslt;
	bool r = iface->reg_notify (_TUNER_NOTIFY, &clientId);
	if (r) {
		rslt = coseq::result::success;
	} else {
		rslt = coseq::result::error;
	}

	_UTL_LOG_I ("registerd clientId=[0x%02x]\n", clientId);

	// clientIdをreply msgで返す
	iface->reply (rslt, (uint8_t*)&clientId, sizeof(clientId));
}

void manager::on_unregister_tuner_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	coseq::result rslt;
	// msgからclientIdを取得
	uint8_t clientId = *(iface->get_source().get_message().data());
	bool r = iface->unreg_notify (_TUNER_NOTIFY, clientId);
	if (r) {
		_UTL_LOG_I ("unregisterd clientId=[0x%02x]\n", clientId);
		rslt = coseq::result::success;
	} else {
		_UTL_LOG_E ("failure unregister clientId=[0x%02x]\n", clientId);
		rslt = coseq::result::error;
	}

	iface->reply (rslt);
}

void manager::on_register_ts_receive_handler (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::i_ts_receive_handler *p_handler = *(client_iface::i_ts_receive_handler**)(iface->get_source().get_message().data());
	_UTL_LOG_I ("p_handler %p\n", p_handler);

	int r = register_ts_receive_handler (p_handler);
	if (r >= 0) {
		iface->reply (coseq::result::success, (uint8_t*)&r, sizeof(r));
	} else {
		iface->reply (coseq::result::error);
	}
}

void manager::on_unregister_ts_receive_handler (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	int client_id = *(int*)(iface->get_source().get_message().data());
	unregister_ts_receive_handler (client_id);


	iface->reply (coseq::result::success);
}

void manager::on_get_state (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	// reply msgに乗せます
	iface->reply (coseq::result::success, (uint8_t*)&state_, sizeof(state_));
}

int manager::register_ts_receive_handler (client_iface::i_ts_receive_handler *p_handler)
{
	if (!p_handler) {
		_UTL_LOG_E ("p_handler is null.");
		return -1;
	}

	std::lock_guard<std::mutex> lock (mutex_ts_receive_handlers_);

	int i = 0;
	for (i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
		if (!reg_ts_receive_handlers_ [i]) {
			reg_ts_receive_handlers_ [i] = p_handler;
			break;
		}
	}

	if (i == TS_RECEIVE_HANDLER_REGISTER_NUM_MAX) {
		_UTL_LOG_E ("reg_ts_receive_handlers_ is full.");
		return -1;
	} else {
		return i;
	}
}

void manager::unregister_ts_receive_handler (int id)
{
	if (id < 0 || id >= TS_RECEIVE_HANDLER_REGISTER_NUM_MAX) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mutex_ts_receive_handlers_);

	reg_ts_receive_handlers_ [id] = NULL;
}

void manager::set_state (client_iface::tuner_state s)
{
	state_ = s;
}

} // namespace tuner_control
} // namespace arec
