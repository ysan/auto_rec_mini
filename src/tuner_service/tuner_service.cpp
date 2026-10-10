#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "psisi_manager_if.h"
#include "tuner_service.h"
#include "tuner_service_if.h"
#include "modules.h"

#include "settings.h"


namespace arec {
namespace tuner_service {

manager::manager (std::string name, uint8_t que_max)
	:coseq::module_base (name, que_max)
	,tuner_resource_max_ (0)
	,next_allocate_tuner_id_ (0)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){manager::on_open(iface);}, std::move("on_open")},
		{[&](coseq::iface *iface){manager::on_close(iface);}, std::move("on_close")},
		{[&](coseq::iface *iface){manager::on_tune(iface);}, std::move("on_tune")},
		{[&](coseq::iface *iface){manager::on_tune_with_retry(iface);}, std::move("on_tune_with_retry")},
		{[&](coseq::iface *iface){manager::on_tune_stop(iface);}, std::move("on_tune_stop")},
		{[&](coseq::iface *iface){manager::on_dump_allocates(iface);}, std::move("on_dump_allocates")},
	};
	set_sequences (seqs, _max);

}

manager::~manager (void)
{
	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// settingsを使って初期化する場合はmodule upで
	tuner_resource_max_ = arec::settings::get_instance()->get_params().get_tuner_hal_allocates().size();
	_UTL_LOG_I ("tuner_resource_max_:[%d]", tuner_resource_max_);
	resource_allcates_.clear();
	resource_allcates_.resize(tuner_resource_max_);
	for (auto &a : resource_allcates_) {
		a = std::make_shared<resource_t>();
	}


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

void manager::on_open (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	modules::module_id caller_module_id = caller_of(iface);
//	uint8_t allcated_tuner_id = allocate_linear (caller_module_id);
	uint8_t allcated_tuner_id = allocate_round_robin (caller_module_id);
	if (allcated_tuner_id != 0xff) {
		iface->reply (coseq::result::success, (uint8_t*)&allcated_tuner_id, sizeof(allcated_tuner_id));
	} else {
//TODO 割り当てできなかった時はpriorityに応じた処理をする  notifyしたり...
		iface->reply (coseq::result::error);
	}

	dump_allocates ();
}

void manager::on_close (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	uint8_t tuner_id = *(uint8_t*)(iface->get_source().get_message().data());
	if (release (tuner_id)) {
		iface->reply (coseq::result::success);
	} else {
		iface->reply (coseq::result::error);
	}

	dump_allocates ();
}

void manager::on_tune (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	iface->lock();

	// 1実行専用の状態はローカルへ(coseq は多重インスタンスのため static/member は不可)
	client_iface::tune_param_t param;
	memset (&param, 0x00, sizeof(param));

	modules::module_id caller_module_id = caller_of(iface);
	if (caller_module_id == modules::module_id::tuner_service) {
		// tune_with_retry, tuneAdvance から呼ばれました
		_tune_param *p = (_tune_param*)(iface->get_source().get_message().data());
		param.physical_channel = p->physical_channel;
		param.tuner_id = p->tuner_id;
		caller_module_id = p->caller_module_id;
	} else {
		// 外部モジュールから呼ばれました
		param = *(client_iface::tune_param_t*)(iface->get_source().get_message().data());
	}

	if (!pre_check (param.tuner_id, caller_module_id, true)) {
		dump_allocates ();
		iface->unlock();
		iface->reply (coseq::result::error);
		return;
	}
	dump_allocates ();

	// --- REQ_TUNE ---
	uint32_t freq = parser::ts_arib_common::physical_ch2freq_khz (param.physical_channel);
	_UTL_LOG_I ("freq=[%d]kHz\n", freq);
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + param.tuner_id,
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::tune),
			(uint8_t*)&freq, sizeof(freq)
		);
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_W ("tune is failure");
			iface->unlock();
			iface->reply (coseq::result::error);
			return;
		}
	}

	// --- CHECK PSI/SI STATE (poll) ---
	// 選局直後の psisi mgr はキュー溢れの可能性あり。200ms * 75 = 約15秒でタイムアウト
	bool ready = false;
	for (int retry = 0; retry <= 75; ++ retry) {
		iface->wait_timeout (200);

		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + param.tuner_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_state)
		);
		if (r.get_result() != coseq::result::success) {
			// キュー溢れ等。リトライ
			continue;
		}

		arec::psisi::client_iface::psisi_state _psisiState = *(arec::psisi::client_iface::psisi_state*)(r.get_message().data());
		if (_psisiState == arec::psisi::client_iface::psisi_state::ready) {
			ready = true;
			break;
		}
	}

	if (!ready) {
		_UTL_LOG_E ("psi/si state invalid. (never transitioned EN_PSISI_STATE__READY)");
		// 念の為停止しておきます(結果は見ません)
		iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + param.tuner_id,
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::tune_stop)
		);
		iface->unlock();
		iface->reply (coseq::result::error);
		return;
	}

	// --- END SUCCESS ---
	iface->unlock();

	resource_allcates_[param.tuner_id]->is_now_tuned = true;
	dump_allocates ();

	iface->reply (coseq::result::success);
}

void manager::on_tune_with_retry (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 1実行専用の状態はローカルへ
	struct _tune_param param;
	memset (&param, 0x00, sizeof(param));

	modules::module_id caller_module_id = caller_of(iface);
	if (caller_module_id == modules::module_id::tuner_service) {
		// tuneAdvance_withRetry から呼ばれました
		_tune_param *p = (_tune_param*)(iface->get_source().get_message().data());
		param.physical_channel = p->physical_channel;
		param.tuner_id = p->tuner_id;
		param.caller_module_id = p->caller_module_id; // 引き継ぎます
	} else {
		// 外部モジュールから呼ばれました
		client_iface::tune_param_t *p = (client_iface::tune_param_t*)(iface->get_source().get_message().data());
		param.physical_channel = p->physical_channel;
		param.tuner_id = p->tuner_id;
		param.caller_module_id = caller_of(iface);
	}

	if (!pre_check (param.tuner_id, param.caller_module_id, true)) {
		iface->reply (coseq::result::error);
		return;
	}

	int retry = 3;
	while (true) {
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_service),
			static_cast<uint8_t>(client_iface::sequence::tune),
			(uint8_t*)&param, sizeof (param)
		);
		if (r.get_result() == coseq::result::success) {
			iface->reply (coseq::result::success);
			return;
		}

		if (retry > 0) {
			_UTL_LOG_W ("retry tune");
			-- retry;
			continue;
		}

		_UTL_LOG_E ("retry over...");
		iface->reply (coseq::result::error);
		return;
	}
}

void manager::on_tune_stop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	iface->lock();

	uint8_t tuner_id = *(uint8_t*)(iface->get_source().get_message().data());
	modules::module_id caller_module_id = caller_of(iface);
	if (!pre_check (tuner_id, caller_module_id, false)) {
		dump_allocates ();
		iface->unlock();
		iface->reply (coseq::result::error);
		return;
	}
	dump_allocates ();

	coseq::source &r = iface->request (
		static_cast<uint8_t>(modules::module_id::tuner_control) + tuner_id,
		static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::tune_stop)
	);

	if (r.get_result() == coseq::result::success) {
		iface->unlock();

		resource_allcates_[tuner_id]->is_now_tuned = false;
		dump_allocates ();

		iface->reply (coseq::result::success);
	} else {
		iface->unlock();

		iface->reply (coseq::result::error);
	}
}

void manager::on_dump_allocates (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	dump_allocates ();


	iface->reply (coseq::result::success);
}

uint8_t manager::allocate_linear (modules::module_id module_id)
{
	uint8_t _id = 0;
	// 未割り当てを探します
	for (const auto &a : resource_allcates_) {
		if (a->tuner_id == 0xff) {
			break;
		}
		++ _id;
	}

	if (_id < tuner_resource_max_) {
		resource_allcates_[_id]->tuner_id = _id;
		resource_allcates_[_id]->module_id = module_id;
		resource_allcates_[_id]->priority = get_priority (module_id);
		_UTL_LOG_I ("allocated. [0x%02x]", _id);
		return _id;

	} else {
		_UTL_LOG_E ("resources full...");
		return 0xff;
	}
}

uint8_t manager::allocate_round_robin (modules::module_id module_id)
{
	uint8_t _init_id = next_allocate_tuner_id_;
	int n = 0;

	while (1) {
		if (_init_id == next_allocate_tuner_id_ && n != 0) {
			break;
		}

		uint8_t _id = next_allocate_tuner_id_;

		if (resource_allcates_[_id]->tuner_id == 0xff) {
			resource_allcates_[_id]->tuner_id = _id;
			resource_allcates_[_id]->module_id = module_id;
			resource_allcates_[_id]->priority = get_priority (module_id);
			_UTL_LOG_I ("allocated. [0x%02x]", _id);

			//set next
			++ next_allocate_tuner_id_;
			if (next_allocate_tuner_id_ == tuner_resource_max_) {
				next_allocate_tuner_id_ = 0;
			}

			return _id;

		} else {
			//set next
			++ next_allocate_tuner_id_;
			if (next_allocate_tuner_id_ == tuner_resource_max_) {
				next_allocate_tuner_id_ = 0;
			}
		}

		++ n;
	}

	_UTL_LOG_E ("resources full...");
	return 0xff;
}

bool manager::release (uint8_t tuner_id)
{
	if (tuner_id < tuner_resource_max_) {
		if (resource_allcates_[tuner_id]->is_now_tuned) {
			_UTL_LOG_E ("not yet tuned... [0x%02x]", tuner_id);
			return false;

		} else {
			resource_allcates_[tuner_id]->clear();
			_UTL_LOG_I ("released. [0x%02x]", tuner_id);
			return true;
		}

	} else {
		_UTL_LOG_E ("invalid tuner_id. [0x%02x]", tuner_id);
		return false;
	}
}

bool manager::pre_check (uint8_t tuner_id, modules::module_id module_id, bool is_req_tune) const
{
	if (tuner_id < tuner_resource_max_) {
		if (
			(resource_allcates_[tuner_id]->tuner_id == tuner_id) &&
			(resource_allcates_[tuner_id]->module_id == module_id)
		) {
			if (is_req_tune) {
				if (!resource_allcates_[tuner_id]->is_now_tuned) {
					return true;
				} else {
					_UTL_LOG_E ("already tuned... [0x%02x]", tuner_id);
					return false;
				}
			} else {
				// req tune stop
				if (resource_allcates_[tuner_id]->is_now_tuned) {
					return true;
				} else {
					_UTL_LOG_E ("not tuned... [0x%02x]", tuner_id);
					return false;
				}
			}

		} else {
			_UTL_LOG_E ("not allocated tuner_id. [0x%02x]", tuner_id);
			return false;
		}

	} else {
		_UTL_LOG_E ("invalid tuner_id. [0x%02x]", tuner_id);
		return false;
	}
}

manager::client_priority manager::get_priority (modules::module_id module_id) const
{
	switch (module_id) {
	case modules::module_id::rec_manager:
		return client_priority::RECORDING;

	case modules::module_id::event_schedule_manager:
		return client_priority::EVENT_SCHEDULE;

	case modules::module_id::channel_manager:
		return client_priority::CHANNEL_SCAN;

	case modules::module_id::command_server:
		return client_priority::COMMAND;

	default:
		return client_priority::OTHER;
	}
}

void manager::dump_allocates (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	for (const auto &a : resource_allcates_) {
		a->dump();
	}
}


} // namespace tuner_service
} // namespace arec
