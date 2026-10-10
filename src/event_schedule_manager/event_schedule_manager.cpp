#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <algorithm>

#include "event_information_table_sched.h"
#include "event_schedule_manager.h"
#include "event_schedule_manager_if.h"

#include "psisi_manager_if.h"
#include "modules.h"

#include "aribstr.h"


namespace arec {
namespace event_schedule {


bool _comp__table_id (const event* l, const event* r) {
	if (r->table_id > l->table_id) {
		return true;
	}

	return false;
}

bool _comp__section_number (const event* l, const event* r) {
	if (r->table_id == l->table_id) {
		if (r->section_number > l->section_number) {
			return true;
		}
	}

	return false;
}

bool _comp__reserve_start_time (const reserve& l, const reserve& r) {
	if (r.start_time > l.start_time) {
		return true;
	}

	return false;
}


manager::manager (std::string name, uint8_t que_max)
	:coseq::module_base (name, que_max)
	,settings_ (NULL)
	,state_ (client_iface::cache_schedule_state::init)
	,tuner_notify_client_id_ (0xff)
	,event_change_notify_client_id_ (0xff)
	,EIT_H_sched_ (NULL)
	,is_need_stop_ (false)
	,force_cache_group_id_ (0xff)
	,tuner_resource_max_ (0)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){manager::on_register_cache_schedule_state_notify(iface);}, std::move("on_register_cache_schedule_state_notify")},
		{[&](coseq::iface *iface){manager::on_unregister_cache_schedule_state_notify(iface);}, std::move("on_unregister_cache_schedule_state_notify")},
		{[&](coseq::iface *iface){manager::on_get_cache_schedule_state(iface);}, std::move("on_get_cache_schedule_state")},
		{[&](coseq::iface *iface){manager::on_check_loop(iface);}, std::move("on_check_loop")},
		{[&](coseq::iface *iface){manager::on_parser_notice(iface);}, std::move("on_parser_notice")},
		{[&](coseq::iface *iface){manager::on_exec_cache_schedule(iface);}, std::move("on_exec_cache_schedule")},
		{[&](coseq::iface *iface){manager::on_cache_schedule(iface);}, std::move("on_cache_schedule")},
		{[&](coseq::iface *iface){manager::on_cache_schedule_force_current_service(iface);}, std::move("on_cache_schedule_force_current_service")},
		{[&](coseq::iface *iface){manager::on_stop_cache_schedule(iface);}, std::move("on_stop_cache_schedule")},
		{[&](coseq::iface *iface){manager::on_get_event(iface);}, std::move("on_get_event")},
		{[&](coseq::iface *iface){manager::on_get_event_latest_dumped_schedule(iface);}, std::move("on_get_event_latest_dumped_schedule")},
		{[&](coseq::iface *iface){manager::on_dump_event_latest_dumped_schedule(iface);}, std::move("on_dump_event_latest_dumped_schedule")},
		{[&](coseq::iface *iface){manager::on_get_events_keyword_search(iface);}, std::move("on_get_events_keyword_search")},
		{[&](coseq::iface *iface){manager::on_get_events_keyword_search(iface);}, std::move("on_get_events_keyword_search(ex)")},
		{[&](coseq::iface *iface){manager::on_add_reserves(iface);}, std::move("on_add_reserves")},
		{[&](coseq::iface *iface){manager::on_dump_schedule_map(iface);}, std::move("on_dump_schedule_map")},
		{[&](coseq::iface *iface){manager::on_dump_schedule(iface);}, std::move("on_dump_schedule")},
		{[&](coseq::iface *iface){manager::on_dump_reserves(iface);}, std::move("on_dump_reserves")},
		{[&](coseq::iface *iface){manager::on_dump_histories(iface);}, std::move("on_dump_histories")},
	};
	set_sequences (seqs, _max);


	settings_ = arec::settings::get_instance();

	last_update_EIT_sched_.clear();
	start_time_EIT_sched_.clear();

	latest_dumped_key_.clear();

	schedule_cache_next_day_.clear();
	schedule_cache_current_plan_.clear();

	reserves_.clear();
	retry_reserves_.clear();
	executing_reserve_.clear();

	current_history_.clear();
	current_history_stream_.clear();
	histories_.clear();

	container_.clear();
}

manager::~manager (void)
{
	last_update_EIT_sched_.clear();
	start_time_EIT_sched_.clear();

	latest_dumped_key_.clear();

	schedule_cache_next_day_.clear();
	schedule_cache_current_plan_.clear();

	reserves_.clear();
	executing_reserve_.clear();

	current_history_.clear();
	current_history_stream_.clear();
	histories_.clear();

	container_.clear();

	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// settingsを使って初期化する場合はmodule upで
	tuner_resource_max_ = arec::settings::get_instance()->get_params().get_tuner_hal_allocates().size();

	std::string cache_data_path = arec::settings::get_instance()->get_params().get_event_schedule_cache_data_json_path();
	arec::common::utils::makedir(cache_data_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	std::string cache_history_path = arec::settings::get_instance()->get_params().get_event_schedule_cache_histories_json_path();
	arec::common::utils::makedir(cache_history_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	schedule_cache_next_day_.set_current_day();

	load_histories ();

	container_.set_schedule_map_json_path(settings_->get_params().get_event_schedule_cache_data_json_path());
	container_.load_schedule_map();

	// 監視ループ起動(check_loop は先に reply してループに入る)
	iface->request (static_cast<uint8_t>(modules::module_id::event_schedule_manager),
					static_cast<uint8_t>(client_iface::sequence::check_loop));

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

void manager::on_register_cache_schedule_state_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	uint8_t client_id = 0;
	coseq::result rslt;
	bool r = iface->reg_notify (NOTIFY_CAT__CACHE_SCHEDULE, &client_id);
	if (r) {
		rslt = coseq::result::success;
	} else {
		rslt = coseq::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	iface->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


}

void manager::on_unregister_cache_schedule_state_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	coseq::result rslt;
	// msgからclient_idを取得
	uint8_t client_id = *(iface->get_source().get_message().data());
	bool r = iface->unreg_notify (NOTIFY_CAT__CACHE_SCHEDULE, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::error;
	}

	iface->reply (rslt);


}

void manager::on_get_cache_schedule_state (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	// reply msgにm_stateを乗せます
	iface->reply (coseq::result::success, (uint8_t*)&state_, sizeof(state_));


}

void manager::on_check_loop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 先にreplyしておく(以降はスレッド占有の監視ループ)
	iface->reply (coseq::result::success);

	// 10秒周期の監視ループ(app 終了要求で抜ける)
	while (!modules::is_shutdown_requested()) {

		iface->wait_timeout (10*1000); // 10sec

		// 実行予定日をチェックします
		arec::common::etime t;
		t.set_current_day();
		if (t == schedule_cache_next_day_) {
			// 実行予定日なので予約を入れます
			_UTL_LOG_I ("add reserves base_time => [%s]", schedule_cache_next_day_.to_string());

			int start_hour = settings_->get_params().get_event_schedule_cache_start_hour();
			int start_min = settings_->get_params().get_event_schedule_cache_start_min();
			arec::common::etime _base_time = schedule_cache_next_day_;
			_base_time.add_hour (start_hour);
			_base_time.add_min (start_min);

			// 投げっぱなしで add_reserves を自リクエスト
			iface->send (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<uint8_t>(client_iface::sequence::add_reserves),
				(uint8_t*)&_base_time,
				sizeof(_base_time)
			);

			// 次回の予定日セットしておきます
			int interval_day = settings_->get_params().get_event_schedule_cache_start_interval_day();
			if (interval_day <= 1) {
				// interval_dayは最低でも１日にします
				interval_day = 1;
			}
			schedule_cache_next_day_.add_day(interval_day);
		}


		if (settings_->get_params().is_enable_event_schedule_cache()) {
			// 予約をチェックします
			check2execute_reserves (iface);
		}
	}
}

void manager::on_parser_notice (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	arec::psisi::client_iface::psisi_type _type = *(arec::psisi::client_iface::psisi_type*)(iface->get_source().get_message().data());
	switch (_type) {
	case arec::psisi::client_iface::psisi_type::EIT_H_SCHED:

		// EITのshceduleの更新時間保存
		// 受け取り中はかなりの頻度でたたかれるはず
		last_update_EIT_sched_.set_current_time ();

		break;

	default:
		break;
	};

	iface->reply (coseq::result::success);

}

void manager::on_exec_cache_schedule (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ(service_infos は psisi へポインタ渡し)
	client_iface::service_key_t service_key =
		*(client_iface::service_key_t*)(iface->get_source().get_message().data());
	psisi_structs::service_info_t service_infos[10];
	memset (service_infos, 0x00, sizeof(service_infos));
	int num = 0;
	bool is_timeouted = false;
	bool is_already_using_tuner = false;
	uint8_t group_id = 0xff;

	const bool is_force = ((executing_reserve_.type & reserve::type_t::TYPE_MASK) == reserve::type_t::FORCE);

	// 終了処理(v1 SECTID_EXIT 相当)
	auto do_exit = [&]() {
		if (!is_already_using_tuner && !is_force) {
			// 選局を停止・クローズ(投げっぱなし)
			uint8_t gr = group_id;
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop), (uint8_t*)&gr, sizeof(gr));
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::close), (uint8_t*)&gr, sizeof(gr));
		}

		if (executing_reserve_.type & reserve::type_t::E_FLG) {
			// history ----------------
			current_history_.set_end_time();
			push_histories (&current_history_);
			save_histories ();
			current_history_.clear();

			// update state_
			state_ = client_iface::cache_schedule_state::ready;

			// retry reserve
			int _rs = (int)retry_reserves_.size();
			_UTL_LOG_I ("retry_reserves_.size %d", _rs);
			if (_rs == 1) {
				retry_reserves_[0].type =
					(reserve::type_t)(reserve::type_t::NORMAL | reserve::type_t::S_FLG |
										reserve::type_t::E_FLG | reserve::type_t::N_FLG);
				add_reserve(retry_reserves_[0]);
				dump_reserves();
				retry_reserves_.clear();

			} else if (_rs >= 2) {
				for (int i = 0; i < _rs; ++ i) {
					if (i == 0) {
						retry_reserves_[i].type =
							(reserve::type_t)(reserve::type_t::NORMAL | reserve::type_t::S_FLG | reserve::type_t::N_FLG);
					} else if (i == (_rs - 1)) {
						retry_reserves_[i].type =
							(reserve::type_t)(reserve::type_t::NORMAL | reserve::type_t::E_FLG | reserve::type_t::N_FLG);
					}
					add_reserve(retry_reserves_[i]);
				}
				dump_reserves();
				retry_reserves_.clear();
			}

			container_.save_schedule_map();
		}

		if (executing_reserve_.type & reserve::type_t::N_FLG) {
			// fire notify
			iface->notify (NOTIFY_CAT__CACHE_SCHEDULE, (uint8_t*)&state_, sizeof(state_));
		}

		EIT_H_sched_ = NULL;
		executing_reserve_.clear();
	};

	// エラー終了(v1 SECTID_END_ERROR 相当)
	auto do_error = [&]() {
		iface->reply (coseq::result::error);

		// history ----------------
		if (is_need_stop_) {
			current_history_stream_._state = history::EN_STATE__CANCEL;
		} else if (is_timeouted) {
			current_history_stream_._state = history::EN_STATE__TIMEOUT;
		} else {
			current_history_stream_._state = history::EN_STATE__ERROR;
		}
		current_history_.add (current_history_stream_);

		if (executing_reserve_.type & reserve::type_t::R_FLG) {
			// retry reserve
			reserve retry;
			arec::common::etime start_time;
			start_time.set_current_time();
			int interval = settings_->get_params().get_event_schedule_cache_retry_interval_min();
			start_time.add_min(interval);

			retry.transport_stream_id = service_key.transport_stream_id;
			retry.original_network_id = service_key.original_network_id;
			retry.service_id = service_key.service_id;
			retry.start_time = start_time;
			retry.type = reserve::type_t::NORMAL;
			retry_reserves_.push_back(retry);
		}

		do_exit();
	};

	// ---- ENTRY ----
	if (executing_reserve_.type & reserve::type_t::S_FLG) {
		state_ = client_iface::cache_schedule_state::busy;
		current_history_.clear();
		current_history_.set_start_time();
		retry_reserves_.clear();
	}
	if (executing_reserve_.type & reserve::type_t::N_FLG) {
		iface->notify (NOTIFY_CAT__CACHE_SCHEDULE, (uint8_t*)&state_, sizeof(state_));
	}

	// history: transport stream name
	{
		arec::channel::client_iface::service_id_param_t param = {
			service_key.transport_stream_id, service_key.original_network_id, 0 /* no need service_id */
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_transport_stream_name),
			(uint8_t*)&param, sizeof(param));
		char* ts_name = (char*)(r.get_message().data());
		current_history_stream_.clear();
		current_history_stream_.stream_name = ts_name;
	}

	// ---- NORMAL のみ: open / get physical ch / tune ----
	if (!is_force) {
		// open
		{
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::tuner_service),
				static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::open));
			if (r.get_result() != coseq::result::success) {
				_UTL_LOG_E ("someone is using a tuner.");
				is_already_using_tuner = true;
				do_error();
				return;
			}
			group_id = *(uint8_t*)(r.get_message().data());
			_UTL_LOG_I ("requst_open group_id:[0x%02x]", group_id);
		}

		// get physical channel by service id
		{
			arec::channel::client_iface::service_id_param_t param = {
				service_key.transport_stream_id, service_key.original_network_id, service_key.service_id
			};
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::channel_manager),
				static_cast<uint8_t>(arec::channel::client_iface::sequence::get_physical_channel_by_service_id),
				(uint8_t*)&param, sizeof(param));
			if (r.get_result() != coseq::result::success) {
				_UTL_LOG_E ("request_get_physical_channel_by_service_id is failure.");
				do_error();
				return;
			}
			uint16_t ch = *(uint16_t*)(r.get_message().data());

			// tune
			arec::tuner_service::client_iface::tune_param_t tparam = { ch, group_id };
			coseq::source &rt = iface->request (
				static_cast<uint8_t>(modules::module_id::tuner_service),
				static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_with_retry),
				(uint8_t*)&tparam, sizeof(tparam));
			if (rt.get_result() != coseq::result::success) {
				_UTL_LOG_E ("arec::tuner_service::client_iface::request_tune is failure.");
				do_error();
				return;
			}
		}
	}

	const uint8_t work_group_id = is_force ? force_cache_group_id_ : group_id;

	// ---- get current service infos ----
	{
		psisi_structs::request_service_infos_param_t sparam = { service_infos, 10 };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + work_group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_current_service_infos),
			(uint8_t*)&sparam, sizeof(sparam));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_current_service_infos err");
			do_error();
			return;
		}
		num = *(int*)(r.get_message().data());
		if (num <= 0) {
			_UTL_LOG_E ("request_get_current_service_infos  num is 0");
			do_error();
			return;
		}
	}

	// ---- enable parse EIT schedule ----
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + work_group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::enable_parse_eit_sched));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("arec::psisi::client_iface::request_enable_parse_eit_sched is failure.");
			do_error();
			return;
		}

		enable_parse_eit_sched_reply_param_t param =
			*(enable_parse_eit_sched_reply_param_t*)(r.get_message().data());

		// parser のインスタンス取得。積んでるデータは一度クリア
		EIT_H_sched_ = param.p_parser;
		EIT_H_sched_ref_ = param.p_parser->get_reference();
		EIT_H_sched_->clear();

		start_time_EIT_sched_.set_current_time();
	}

	// ---- EIT schedule の取得完了を待つ(5秒周期ポーリング) ----
	while (true) {
		if (is_need_stop_) {
			_UTL_LOG_W ("parser EIT schedule : cancel");
			break;
		}

		iface->wait_timeout (5000); // 5sec

		arec::common::etime tcur;
		tcur.set_current_time();
		arec::common::etime ttmp = last_update_EIT_sched_;
		ttmp.add_sec (10);
		if (tcur > ttmp) {
			// 約10秒間更新がなかったら完了
			_UTL_LOG_I ("parse EIT schedule : complete");
			break;
		}

		ttmp = start_time_EIT_sched_;
		ttmp.add_min (settings_->get_params().get_event_schedule_cache_timeout_min());
		if (tcur > ttmp) {
			_UTL_LOG_W ("parser EIT schedule : timeout");
			is_timeouted = true;
			break;
		}

		_UTL_LOG_I ("parse EIT schedule : last_update_EIT_sched_ %s", last_update_EIT_sched_.to_string());
	}

	// ---- disable parse EIT schedule (必ず行いたいので成功までリトライ) ----
	for (int i = 0; i < 120; ++ i) {
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + work_group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::disable_parse_eit_sched));
		if (r.get_result() == coseq::result::success) {
			break;
		}
		_UTL_LOG_E ("arec::psisi::client_iface::request_disable_parse_eit_sched is failure. --> retry");
		iface->wait_timeout (500); // 500ms
	}

	// ---- cache ----
	for (int i = 0; i < num; ++ i) {
		service_infos[i].dump ();

		std::vector <event*> *p_sched = new std::vector <event*>;
		cache_schedule (
			service_infos[i].transport_stream_id,
			service_infos[i].original_network_id,
			service_infos[i].service_id,
			p_sched
		);

		if (p_sched->size() > 0) {
			cache_schedule_extended (p_sched);

			service_key_t key (
				service_infos[i].transport_stream_id,
				service_infos[i].original_network_id,
				service_infos[i].service_id,
				service_infos[i].service_type,			// additional
				service_infos[i].p_service_name_char	// additional
			);

			if (container_.has_schedule_map (key)) {
				_UTL_LOG_I ("has_schedule_map -> delete_schedule_map");
				container_.delete_schedule_map (key);
			}

			container_.add_schedule_map (key, p_sched);
			key.dump();
			_UTL_LOG_I ("add_schedule_map -> %d items", p_sched->size());

			// history ----------------
			history::service svc;
			svc.key = key;
			svc.item_num = p_sched->size();
			current_history_stream_.services.push_back (svc);

		} else {
			_UTL_LOG_I ("schedule none.");
			delete p_sched;
			p_sched = NULL;
		}
	}

	if (is_timeouted || is_need_stop_) {
		do_error();
		return;
	}

	// ---- END SUCCESS ----
	iface->reply (coseq::result::success);

	current_history_stream_._state = history::EN_STATE__COMPLETE;
	current_history_.add (current_history_stream_);

	do_exit();
}

void manager::on_cache_schedule (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	arec::common::etime _base_time;
	_base_time.set_current_time();

	coseq::source &r = iface->request (
		static_cast<uint8_t>(modules::module_id::event_schedule_manager),
		static_cast<uint8_t>(client_iface::sequence::add_reserves),
		(uint8_t*)&_base_time,
		sizeof(_base_time)
	);
	if (r.get_result() == coseq::result::success) {
		iface->reply (coseq::result::success);
	} else {
		_UTL_LOG_E ("add reserve failure.");
		iface->reply (coseq::result::error);
	}
}

void manager::on_cache_schedule_force_current_service (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ
	psisi_structs::service_info_t service_infos[10];
	memset (service_infos, 0x00, sizeof(service_infos));

	force_cache_group_id_ = *(uint8_t*)(iface->get_source().get_message().data());
	if (force_cache_group_id_ >= tuner_resource_max_) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", force_cache_group_id_);
		iface->reply (coseq::result::error);
		force_cache_group_id_ = 0xff;
		return;
	}

	// get current service infos
	int num = 0;
	{
		psisi_structs::request_service_infos_param_t sparam = { service_infos, 10 };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + force_cache_group_id_,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_current_service_infos),
			(uint8_t*)&sparam, sizeof(sparam));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_current_service_infos err");
			iface->reply (coseq::result::error);
			force_cache_group_id_ = 0xff;
			return;
		}
		num = *(int*)(r.get_message().data());
		if (num <= 0) {
			_UTL_LOG_E ("request_get_current_service_infos is 0");
			iface->reply (coseq::result::error);
			force_cache_group_id_ = 0xff;
			return;
		}
	}

	// add reserve (FORCE)
	reserve tmp_reserve;
	{
		arec::common::etime start_time;
		start_time.set_current_time();
		reserve::type_t type =
			(reserve::type_t)(reserve::type_t::FORCE | reserve::type_t::S_FLG | reserve::type_t::E_FLG);

		// 添字0 でも何でも良い(全チャンネル編成分のスケジュールを取得)
		tmp_reserve.transport_stream_id = service_infos[0].transport_stream_id;
		tmp_reserve.original_network_id = service_infos[0].original_network_id;
		tmp_reserve.service_id = service_infos[0].service_id;
		tmp_reserve.start_time = start_time;
		tmp_reserve.type = type;
		add_reserve (tmp_reserve);
	}

	// 予約消化(実行完了)を待つ
	while (!modules::is_shutdown_requested()) {
		iface->wait_timeout (100); // 100ms

		if (is_exist_reserve(tmp_reserve)) {
			continue; // まだ予約に入っている
		}
		if (executing_reserve_ == tmp_reserve && executing_reserve_.is_valid()) {
			continue; // 只今実行中
		}
		break; // 実行終了
	}

	iface->reply (coseq::result::success);

	force_cache_group_id_ = 0xff;
}

void manager::on_stop_cache_schedule (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	is_need_stop_ = true;

	// 実行中の予約が終わるのを待つ
	while (!modules::is_shutdown_requested()) {
		if (!executing_reserve_.is_valid()) {
			break; // 実行終了
		}
		iface->wait_timeout (100); // 100ms
	}

	iface->reply (coseq::result::success);

	is_need_stop_ = false;
}

void manager::on_get_event (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::request_event_param_t _param =
			*(client_iface::request_event_param_t*)(iface->get_source().get_message().data());

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		iface->reply (coseq::result::error);

	} else {

		service_key_t _service_key = {_param.arg.key.transport_stream_id, _param.arg.key.original_network_id, _param.arg.key.service_id};
		const event *p = container_.get_event (_service_key, _param.arg.key.event_id);
		event* p_event = const_cast <event*> (p);
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

			iface->reply (coseq::result::success);

		} else {

			_UTL_LOG_E ("get_event is null.");
			iface->reply (coseq::result::error);

		}
	}

}

void manager::on_get_event_latest_dumped_schedule (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::request_event_param_t _param =
			*(client_iface::request_event_param_t*)(iface->get_source().get_message().data());

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		iface->reply (coseq::result::error);

	} else {

		const event *p = container_.get_event (latest_dumped_key_, _param.arg.index);
		event* p_event = const_cast <event*> (p);
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

			iface->reply (coseq::result::success);

		} else {

			_UTL_LOG_E ("get_event is null.");
			iface->reply (coseq::result::error);

		}
	}

}

void manager::on_dump_event_latest_dumped_schedule (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::request_event_param_t _param =
			*(client_iface::request_event_param_t*)(iface->get_source().get_message().data());

//_param.p_out_eventはnullで来ます

//	if (!_param.p_out_event) {
//		_UTL_LOG_E ("_param.p_out_event is null.");
//		iface->reply (coseq::result::error);

//	} else {

		const event *p = container_.get_event (latest_dumped_key_, _param.arg.index);
		event* p_event = const_cast <event*> (p);
		if (p_event) {

			p_event->dump();
			p_event->dump_detail ();

			iface->reply (coseq::result::success);

		} else {

			_UTL_LOG_E ("get_event is null.");
			iface->reply (coseq::result::error);

		}
//	}

}

void manager::on_get_events_keyword_search (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::request_event_param_t _param =
			*(client_iface::request_event_param_t*)(iface->get_source().get_message().data());

	if (!_param.p_out_event) {
		_UTL_LOG_E ("_param.p_out_event is null.");
		iface->reply (coseq::result::error);

	} else if (_param.array_max_num <= 0) {
		_UTL_LOG_E ("_param.array_max_num is invalid.");
		iface->reply (coseq::result::error);

	} else {

		bool is_check_extended_event = false;
		if (iface->get_source().get_sequence_idx() == static_cast<int>(client_iface::sequence::get_events__keyword_search)) {
			is_check_extended_event = false;
		} else if (iface->get_source().get_sequence_idx() == static_cast<int>(client_iface::sequence::get_events__keyword_search_ex)) {
			is_check_extended_event = true;
		}

		int n = container_.get_events (_param.arg.p_keyword, _param.p_out_event, _param.array_max_num, is_check_extended_event);
		if (n < 0) {
			_UTL_LOG_E ("get_events is invalid.");
			iface->reply (coseq::result::error);
		} else {
			// 検索数をreply msgで返します
			iface->reply (coseq::result::success, (uint8_t*)&n, sizeof(n));
		}
	}

}

void manager::on_add_reserves (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ(channels は channel_manager へポインタ渡し)
	arec::common::etime _base_time = *(arec::common::etime*)(iface->get_source().get_message().data());

	arec::channel::client_iface::channel_t channels[20];
	memset (channels, 0x00, sizeof(channels));

	// get channels
	int ch_num = 0;
	{
		arec::channel::client_iface::request_channels_param_t param = {channels, 20};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_channels),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_channels is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		ch_num = *(int*)(r.get_message().data());
		_UTL_LOG_I ("request_get_channels ch_num:[%d]", ch_num);
		if (ch_num <= 0) {
			_UTL_LOG_E ("request_get_channels is 0");
			iface->reply (coseq::result::error);
			return;
		}
	}

	// 取得したチャンネル分の予約を入れます
	for (int i = 0; i < ch_num; ++ i) {

		if (i != 0) {
			_base_time.add_min(1); // とりあえず 1分おきに
		}
		arec::common::etime _start_time = _base_time;

		reserve::type_t type = (reserve::type_t)(reserve::type_t::NORMAL | reserve::type_t::R_FLG);
		if (i == 0) {
			type = (reserve::type_t)(type | reserve::type_t::S_FLG | reserve::type_t::N_FLG);
		}
		if (i == ch_num -1) {
			type = (reserve::type_t)(type | reserve::type_t::E_FLG | reserve::type_t::N_FLG);
		}

		add_reserve (
			channels[i].transport_stream_id,
			channels[i].original_network_id,
			channels[i].service_ids[0],	// 添字0 でも何でも良い(全チャンネル編成分取得)
			&_start_time,
			type
		);
	}

	dump_reserves();

	iface->reply (coseq::result::success);
}

void manager::on_dump_schedule_map (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	container_.dump_schedule_map ();


	iface->reply (coseq::result::success);

}

void manager::on_dump_schedule (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::service_key_t _key =
					*(client_iface::service_key_t*)(iface->get_source().get_message().data());
	service_key_t key (_key);
	latest_dumped_key_ = key;

	container_.dump_schedule_map (key);


	iface->reply (coseq::result::success);

}

void manager::on_dump_reserves (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	dump_reserves ();


	iface->reply (coseq::result::success);

}

void manager::on_dump_histories (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	dump_histories ();


	iface->reply (coseq::result::success);

}

void manager::on_receive_notify (coseq::iface *iface)
{
/***
	if (iface->get_source().get_client_id() == tuner_notify_client_id_) {

		EN_TUNER_STATE en_state = *(EN_TUNER_STATE*)(iface->get_source().get_message().data());
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

	} else if (iface->get_source().get_client_id() == event_change_notify_client_id_) {

		PSISI_NOTIFY_EVENT_INFO _info = *(PSISI_NOTIFY_EVENT_INFO*)(iface->get_source().get_message().data());
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
void manager::cache_schedule (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	std::vector <event*> *p_out_sched
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


	std::lock_guard<std::recursive_mutex> lock (*EIT_H_sched_ref_.mutex_);

	std::vector<parser::event_information_table_sched::table*>::const_iterator iter = EIT_H_sched_ref_.tables_->begin();
	for (; iter != EIT_H_sched_ref_.tables_->end(); ++ iter) {

		parser::event_information_table_sched::table *p_table = *iter;

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
		std::vector<parser::event_information_table_sched::table::event>::const_iterator iter_event = p_table->events.begin();
		for (; iter_event != p_table->events.end(); ++ iter_event) {
			if (iter_event->event_id == 0) {
				// event情報が付いてない
				continue;
			}

			event *p_event = new event ();

			p_event->table_id = p_table->header.table_id;
			p_event->transport_stream_id = p_table->transport_stream_id;
			p_event->original_network_id = p_table->original_network_id;
			p_event->service_id = p_table->header.table_id_extension;

			p_event->section_number = p_table->header.section_number;

			p_event->event_id = iter_event->event_id;

			time_t stime = parser::ts_arib_common::get_epoch_from_mjd (iter_event->start_time);
			arec::common::etime wk (stime);
			p_event->start_time = wk;

			int dur_sec = parser::ts_arib_common::get_sec_from_bcd (iter_event->duration);
			wk.add_sec (dur_sec);
			p_event->end_time = wk;


			std::vector<parser::descriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SHORT_EVENT_DESCRIPTOR) {
					parser::short_event_descriptor sd (*iter_desc);
					if (!sd.is_valid) {
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
					parser::component_descriptor cd (*iter_desc);
					if (!cd.is_valid) {
						_UTL_LOG_W ("invalid Component_descriptor");
						continue;
					}

					p_event->component_type = cd.component_type;
					p_event->component_tag = cd.component_tag;

				} else if (iter_desc->tag == DESC_TAG__AUDIO_COMPONENT_DESCRIPTOR) {
					parser::audio_component_descriptor acd (*iter_desc);
					if (!acd.is_valid) {
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
					parser::content_descriptor cd (*iter_desc);
					if (!cd.is_valid) {
						_UTL_LOG_W ("invalid Content_descriptor");
						continue;
					}

					std::vector<parser::content_descriptor::content>::const_iterator iter_con = cd.contents.begin();
					for (; iter_con != cd.contents.end(); ++ iter_con) {
						event::genre genre;
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

void manager::cache_schedule_extended (std::vector <event*> *p_out_sched)
{
	std::vector<event*>::iterator iter = p_out_sched->begin();
	for (; iter != p_out_sched->end(); ++ iter) {
		event* p = *iter;
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
void manager::cache_schedule_extended (event* p_out_event)
{
	if (!p_out_event) {
		return ;
	}


	std::lock_guard<std::recursive_mutex> lock (*EIT_H_sched_ref_.mutex_);

	std::vector<parser::event_information_table_sched::table*>::const_iterator iter = EIT_H_sched_ref_.tables_->begin();
	for (; iter != EIT_H_sched_ref_.tables_->end(); ++ iter) {

		parser::event_information_table_sched::table *p_table = *iter;

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
		std::vector<parser::event_information_table_sched::table::event>::const_iterator iter_event = p_table->events.begin();
		for (; iter_event != p_table->events.end(); ++ iter_event) {
			if (iter_event->event_id == 0) {
				// event情報が付いてない
				continue;
			}


			// キーが一致したテーブルをみます
			if (p_out_event->event_id != iter_event->event_id) {
				continue;
			}


			std::vector<parser::descriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__EXTENDED_EVENT_DESCRIPTOR) {
					parser::extended_event_descriptor eed (*iter_desc);
					if (!eed.is_valid) {
						_UTL_LOG_W ("invalid Extended_event_descriptor\n");
						continue;
					}

					char aribstr [MAXSECLEN];
					std::vector<parser::extended_event_descriptor::item>::const_iterator iter_item = eed.items.begin();
					for (; iter_item != eed.items.end(); ++ iter_item) {
						event::extended_info info;

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

bool manager::add_reserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	arec::common::etime * p_start_time,
	reserve::type_t _type
)
{
	if (!p_start_time) {
		_UTL_LOG_E ("p_start_time is null");
		return false;
	}

	reserve r (_transport_stream_id, _original_network_id, _service_id, p_start_time, _type);

	return add_reserve (r);
}

bool manager::add_reserve (const reserve &rsv)
{
	if (is_duplicate_reserve (rsv)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	reserves_.push_back (rsv);

	std::stable_sort (reserves_.begin(), reserves_.end(), _comp__reserve_start_time);

	return true;
}

bool manager::remove_reserve (const reserve &rsv)
{
	std::vector<reserve>::const_iterator iter = reserves_.begin();
	for (; iter != reserves_.end(); ++ iter) {
		if (*iter == rsv) {
			// match
			reserves_.erase (iter);
			// 重複無い前提
			return true;
		}
	}

	return false;
}

bool manager::is_duplicate_reserve (const reserve& rsv) const
{
	std::vector<reserve>::const_iterator iter = reserves_.begin();
	for (; iter != reserves_.end(); ++ iter) {
		if (*iter == rsv) {
			// duplicate
			return true;
		}
	}

	return false;
}

void manager::check2execute_reserves (coseq::iface *iface)
{
	if (executing_reserve_.is_valid()) {
		return;
	}


	arec::common::etime cur_time;
	cur_time.set_current_time ();

	std::vector<reserve>::const_iterator iter = reserves_.begin();
	for (; iter != reserves_.end(); ++ iter) {
		if ((iter->type & reserve::type_t::TYPE_MASK) == reserve::type_t::INIT) {
			continue;
		}

		arec::common::etime chk_time = iter->start_time;
		chk_time.add_hour(1);
		if (cur_time >= chk_time) {
			_UTL_LOG_I ("delete reserve because more time than the start time...");
			iter->dump();
			reserves_.erase (iter);
			break;
		}

		if (cur_time >= iter->start_time) {

			client_iface::service_key_t _key = {
				iter->transport_stream_id,
				iter->original_network_id,
				iter->service_id
			};

			// 投げっぱなしで exec_cache_schedule を自リクエスト
			iface->send (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<uint8_t>(client_iface::sequence::exec_cache_schedule),
				(uint8_t*)&_key,
				sizeof(_key)
			);

			_UTL_LOG_I ("send exec_cache_schedule");
			iter->dump();
			executing_reserve_ = *iter;

			reserves_.erase (iter);
			break;
		}

	}
}

bool manager::is_exist_reserve (const reserve& rsv) const
{
	std::vector<reserve>::const_iterator iter = reserves_.begin();
	for (; iter != reserves_.end(); ++ iter) {
		if (*iter == rsv) {
			return true;
		}
	}

	return false;
}

void manager::dump_reserves (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<reserve>::const_iterator iter = reserves_.begin();
	for (; iter != reserves_.end(); ++ iter) {
		iter->dump();
	}
}

void manager::push_histories (const history *p_history)
{
	if (!p_history) {
		return;
	}

	// fifo 10
	// pop -> delete
	while (histories_.size() >= 10) {
		histories_.erase (histories_.begin());
	}

	// push
	histories_.push_back (*(history*)p_history);

	_UTL_LOG_I ("push_histories  size=[%d]", histories_.size());
}

void manager::dump_histories (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<history>::const_iterator iter = histories_.begin();
	for (; iter != histories_.end(); ++ iter) {
		_UTL_LOG_I ("---------------------------------------");
		iter->dump();
	}
}



} // namespace event_schedule
} // namespace arec

//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (
		cereal::make_nvp("tv_sec", t.tv_sec),
		cereal::make_nvp("tv_nsec", t.tv_nsec)
	);
}

namespace arec { namespace common {
template <class Archive>
void serialize (Archive &archive, arec::common::etime &t)
{
	archive (cereal::make_nvp("m_time", t.time_));
}
}} // namespace arec::common

namespace arec {
namespace event_schedule {

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
void serialize (Archive &archive, history::service &s)
{
	archive (
		cereal::make_nvp("key", s.key),
		cereal::make_nvp("item_num", s.item_num)
	);
}

template <class Archive>
void serialize (Archive &archive, history::stream &s)
{
	archive (
		cereal::make_nvp("stream_name", s.stream_name),
		cereal::make_nvp("_state", s._state),
		cereal::make_nvp("services", s.services)
	);
}

template <class Archive>
void serialize (Archive &archive, history &h)
{
	archive (
		cereal::make_nvp("streams", h.streams),
		cereal::make_nvp("start_time", h.start_time),
		cereal::make_nvp("end_time", h.end_time)
	);
}

void manager::save_histories (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_histories", histories_));
	}

	std::string path = arec::settings::get_instance()->get_params().get_event_schedule_cache_histories_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_histories (void)
{
	std::string path = arec::settings::get_instance()->get_params().get_event_schedule_cache_histories_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("event_schedule_cache_histories.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_histories", histories_));

	ifs.close();
	ss.clear();


	// etimeの値は直接 tv_sec,tv_nsecに書いてるので to_string用の文字はここで作ります
	std::vector<history>::iterator iter = histories_.begin();
	for (; iter != histories_.end(); ++ iter) {
		iter->start_time.update_strings();
		iter->end_time.update_strings();
	}
}

} // namespace event_schedule
} // namespace arec
