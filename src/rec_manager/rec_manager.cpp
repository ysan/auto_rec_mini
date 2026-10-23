#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "event_schedule_manager_if.h"
#include "group.h"
#include "psisi_manager_if.h"
#include "rec_instance.h"
#include "rec_manager.h"
#include "rec_manager_if.h"

#include "utils.h"
#include "modules.h"

#include "aribstr.h"



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


namespace arec {
namespace rec {

manager::manager (std::string name, uint8_t que_max)
	:coseq::module_base (name, que_max)
////	,tuner_notify_client_id_ (0xff)
////	,ts_receive_handler_id_ (-1)
////	,pat_detect_notify_client_id_ (0xff)
////	,event_change_notify_client_id_ (0xff)
////	,rec_progress_ (EN_REC_PROGRESS__INIT)
	,tuner_resource_max_ (0)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_up_by_group_id(iface);}, std::move("on_module_up_by_group_id")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){manager::on_check_loop(iface);}, std::move("on_check_loop")},
		{[&](coseq::iface *iface){manager::on_check_reserves_event_loop(iface);}, std::move("on_check_reserves_event_loop")},
		{[&](coseq::iface *iface){manager::on_check_recordings_event_loop(iface);}, std::move("on_check_recordings_event_loop")},
		{[&](coseq::iface *iface){manager::on_recording_notice(iface);}, std::move("on_recording_notice")},
		{[&](coseq::iface *iface){manager::on_start_recording(iface);}, std::move("on_start_recording")},
		{[&](coseq::iface *iface){manager::on_add_reserve_current_event(iface);}, std::move("on_add_reserve_current_event")},
		{[&](coseq::iface *iface){manager::on_add_reserve_event(iface);}, std::move("on_add_reserve_event")},
		{[&](coseq::iface *iface){manager::on_add_reserve_event_helper(iface);}, std::move("on_add_reserve_event_helper")},
		{[&](coseq::iface *iface){manager::on_add_reserve_manual(iface);}, std::move("on_add_reserve_manual")},
		{[&](coseq::iface *iface){manager::on_remove_reserve(iface);}, std::move("on_remove_reserve")},
		{[&](coseq::iface *iface){manager::on_remove_reserve_by_index(iface);}, std::move("on_remove_reserve_by_index")},
		{[&](coseq::iface *iface){manager::on_get_reserves(iface);}, std::move("on_get_reserves")},
		{[&](coseq::iface *iface){manager::on_stop_recording(iface);}, std::move("on_stop_recording")},
		{[&](coseq::iface *iface){manager::on_dump_reserves(iface);}, std::move("on_dump_reserves")},
	};
	set_sequences (seqs, _max);


	settings_ = arec::settings::get_instance();

	clear_reserves ();
	clear_results ();
////	recordings_[0].clear();
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		recordings_[_gr].clear();
	}

////	memset (recording_tmpfile_, 0x00, sizeof (recording_tmpfile_));
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		memset (recording_tmpfiles_[_gr], 0x00, PATH_MAX);
	}

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		tuner_notify_client_id_ [_gr] = 0xff;
		ts_receive_handler_id_ [_gr] = -1;
		pat_detect_notify_client_id_ [_gr] = 0xff;
		event_change_notify_client_id_ [_gr] = 0xff;
	}
}

manager::~manager (void)
{
	clear_reserves ();
	clear_results ();
////	recordings_[0].clear();
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		recordings_[_gr].clear();
	}

////	memset (recording_tmpfile_, 0x00, sizeof (recording_tmpfile_));
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		memset (recording_tmpfiles_[_gr], 0x00, PATH_MAX);
	}

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		tuner_notify_client_id_ [_gr] = 0xff;
		ts_receive_handler_id_ [_gr] = -1;
		pat_detect_notify_client_id_ [_gr] = 0xff;
		event_change_notify_client_id_ [_gr] = 0xff;
	}

	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// settingsを使って初期化する場合はmodule upで
	tuner_resource_max_ = arec::settings::get_instance()->get_params().get_tuner_hal_allocates().size();

	std::string ts_path = settings_->get_params().get_rec_ts_path();
	arec::common::utils::makedir(ts_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

	std::string reserve_path = settings_->get_params().get_rec_reserves_json_path();
	arec::common::utils::makedir(reserve_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	std::string result_path = settings_->get_params().get_rec_results_json_path();
	arec::common::utils::makedir(result_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	load_reserves ();
	load_results ();

	// rec_instance グループ数分の生成(外部 manager を使うのでコンストラクタ不可)
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		std::unique_ptr <instance> _r(new instance(modules::get_manager(), _gr));
		rec_instances_[_gr].swap(_r);
		ts_handlers_[_gr] = rec_instances_[_gr].get();
	}

	// group ごとに module_up_by_groupid を順に実行
	for (uint8_t gr = 0; gr < arec::group::GROUP_MAX; ++ gr) {
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::rec_manager),
			static_cast<uint8_t>(client_iface::sequence::module_up_by_groupid),
			(uint8_t*)&gr, sizeof(gr));
		if (r.get_result() != coseq::result::success) {
			iface->reply (coseq::result::error);
			return;
		}
	}

	// 各監視ループ起動(いずれも先に reply してループに入る)
	iface->request (static_cast<uint8_t>(modules::module_id::rec_manager), static_cast<uint8_t>(client_iface::sequence::check_loop));
	iface->request (static_cast<uint8_t>(modules::module_id::rec_manager), static_cast<uint8_t>(client_iface::sequence::check_reserves_event_loop));
	iface->request (static_cast<uint8_t>(modules::module_id::rec_manager), static_cast<uint8_t>(client_iface::sequence::check_recordings_event_loop));

	iface->reply (coseq::result::success);
}

void manager::on_module_up_by_group_id (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// request msg から group_id を取得します
	uint8_t group_id = *(uint8_t*)(iface->get_source().get_message().data());
	_UTL_LOG_I("(%s) group_id:[%d]", iface->get_sequence_name(), group_id);

	// register tuner notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + group_id,
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::reg_tuner_notify));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_tuner_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		tuner_notify_client_id_ [group_id] = *(uint8_t*)(r.get_message().data());
	}

	// register ts receive handler
	{
		_UTL_LOG_I ("arec::tuner_control::client_iface::i_ts_receive_handler %p", rec_instances_[group_id].get());
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + group_id,
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::reg_ts_receive_handler),
			(uint8_t*)&(ts_handlers_[group_id]), sizeof(ts_handlers_[group_id]));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_ts_receive_handler is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		ts_receive_handler_id_ [group_id] = *(int*)(r.get_message().data());
	}

	// register pat detect notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::reg_pat_detect_notify));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_pat_detect_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		pat_detect_notify_client_id_ [group_id] = *(uint8_t*)(r.get_message().data());
	}

	// register event change notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::reg_event_change_notify));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_event_change_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		event_change_notify_client_id_ [group_id] = *(uint8_t*)(r.get_message().data());
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

void manager::on_check_loop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 先にreplyしておく(以降はスレッド占有の監視ループ)
	iface->reply (coseq::result::success);

	// 1秒周期の監視ループ(app 終了要求で抜ける)
	while (!modules::is_shutdown_requested()) {

		iface->wait_timeout (1000); // 1sec

		check_reserves ();
		check_recording_end ();
		check_disk_free (iface);

		uint8_t group_id = 0;
		if (is_exist_req_start_recording_reserve ()) {
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::tuner_service),
				static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::open));
			if (r.get_result() != coseq::result::success) {
				continue;
			}
			group_id = *(uint8_t*)(r.get_message().data());
			_UTL_LOG_I ("request_open group_id:[0x%02x]", group_id);

			if (recordings_[group_id].is_used) {
				// recordings_ の idx は request_open で取った tuner_id で決まる
				// is_used になってるはずない...
				_UTL_LOG_E ("??? recordings_[group_id].is_used ???  group_id:[0x%02x]", group_id);
//TODO should be close
				continue;
			}
		}

		if (pick_req_start_recording_reserve (group_id)) {
			// request start recording (結果は見ない)
			iface->request (static_cast<uint8_t>(modules::module_id::rec_manager),
							static_cast<uint8_t>(client_iface::sequence::start_recording),
							(uint8_t*)&group_id, sizeof(uint8_t));
		}
	}
}

//TODO  event_sched_mgrの state_notify契機でチェックする方がいいかも
void manager::on_check_reserves_event_loop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 先にreplyしておく(以降はスレッド占有の監視ループ)
	iface->reply (coseq::result::success);

	// 10分周期の監視ループ(app 終了要求で抜ける)
	while (!modules::is_shutdown_requested()) {

		iface->wait_timeout (60*1000*10); // 10min

		// EPG取得が完了しているか確認します
		{
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<uint8_t>(arec::event_schedule::client_iface::sequence::get_cache_schedule_state));
			arec::event_schedule::client_iface::cache_schedule_state _s =
				*(arec::event_schedule::client_iface::cache_schedule_state*)(r.get_message().data());
			if (_s != arec::event_schedule::client_iface::cache_schedule_state::ready) {
				// readyでないので以下の処理は行いません
				continue;
			}
		}

		// 録画予約に対応するイベント内容をチェックします
		for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {

			if (!reserves_ [i].is_used) {
				continue;
			}
			if (reserves_ [i].state == RESERVE_STATE__INIT) {
				continue;
			}
			if (!reserves_ [i].is_event_type) {
				continue;
			}

			// スケジュールを確認して start_time, end_time, event_name が変わってないかチェック
			arec::event_schedule::client_iface::event_t event;
			arec::event_schedule::client_iface::request_event_param_t param = {
				{
					reserves_ [i].transport_stream_id,
					reserves_ [i].original_network_id,
					reserves_ [i].service_id,
					reserves_ [i].event_id
				},
				&event
			};

			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<uint8_t>(arec::event_schedule::client_iface::sequence::get_event),
				(uint8_t*)&param, sizeof(param));

			if (r.get_result() == coseq::result::success) {
				if ( reserves_ [i].start_time != param.p_out_event-> start_time) {
					_UTL_LOG_I (
						"check_event  reserve start_time is update.  [%s] -> [%s]",
						reserves_ [i].start_time.to_string(),
						param.p_out_event->start_time.to_string()
					);
					reserves_ [i].start_time = param.p_out_event-> start_time;
				}

				if (reserves_ [i].end_time != param.p_out_event-> end_time) {
					_UTL_LOG_I (
						"check_event  reserve end_time is update.  [%s] -> [%s]",
						reserves_ [i].end_time.to_string(),
						param.p_out_event->end_time.to_string()
					);
					reserves_ [i].end_time = param.p_out_event-> end_time;
				}

				if (reserves_ [i].title_name != *param.p_out_event-> p_event_name) {
					_UTL_LOG_I (
						"check_event  reserve title_name is update.  [%s] -> [%s]",
						reserves_ [i].title_name.c_str(),
						param.p_out_event->p_event_name->c_str()
					);
					reserves_ [i].title_name = param.p_out_event-> p_event_name->c_str();
				}

			} else {
				// 予約に対応するイベントがなかった
				_UTL_LOG_W ("no event corresponding to the reservation...");
				reserves_ [i].dump();
			}
		}
	}
}

void manager::on_check_recordings_event_loop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 先にreplyしておく(以降はスレッド占有の監視ループ)
	iface->reply (coseq::result::success);

	// group を巡回しながら録画中イベントを監視(app 終了要求で抜ける)
	uint8_t group_id = 0;
	while (!modules::is_shutdown_requested()) {

		if (group_id >= arec::group::GROUP_MAX) {
			group_id = 0;
		}

		iface->wait_timeout (1000); // 1sec

		if (!(recordings_[group_id].state & RESERVE_STATE__NOW_RECORDING)) {
			++ group_id;
			continue;
		}

		// get present event info
		psisi_structs::event_info_t present_event_info;
		present_event_info.clear();

		psisi_structs::service_info_t _svc_info;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = recordings_[group_id].transport_stream_id;
		_svc_info.original_network_id = recordings_[group_id].original_network_id;
		_svc_info.service_id = recordings_[group_id].service_id;

		psisi_structs::request_event_info_param_t eparam = { _svc_info, &present_event_info };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_present_event_info),
			(uint8_t*)&eparam, sizeof(eparam));

		if (r.get_result() == coseq::result::success) {
			if (recordings_[group_id].state & RESERVE_STATE__NOW_RECORDING) {

				if (recordings_[group_id].is_event_type) {
					// event_type の録画は event_id と end_time の確認
					if (recordings_[group_id].event_id == present_event_info.event_id) {
						if (recordings_[group_id].end_time != present_event_info.end_time) {
							_UTL_LOG_I (
								"#####  recording end_time is update.  [%s] -> [%s]  #####",
								recordings_[group_id].end_time.to_string (),
								present_event_info.end_time.to_string ()
							);
							recordings_[group_id].end_time = present_event_info.end_time;
							recordings_[group_id].dump();
						}

					} else {
//TODO 録画止めてもいいかも
						// event_id が変わった とりあえずログだしとく
						_UTL_LOG_W (
							"#####  recording event_id changed.  [0x%04x] -> [0x%04x]  #####",
							present_event_info.event_id,
							recordings_[group_id].event_id
						);
						recordings_[group_id].dump();
						present_event_info.dump();
					}

				} else {
					// manual 録画は event_name_char を代入
					recordings_[group_id].event_id = present_event_info.event_id;
					recordings_[group_id].title_name = present_event_info.event_name_char;
				}
			}

		} else {
			_UTL_LOG_E ("(%s) request_get_present_event_info err", iface->get_sequence_name());
		}

		++ group_id;
	}
}

void manager::on_recording_notice (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


////	RECORDING_NOTICE_t _notice = *(RECORDING_NOTICE_t*)(iface->get_source().get_message().data());
	instance::RECORDING_NOTICE_t _notice = *(instance::RECORDING_NOTICE_t*)(iface->get_source().get_message().data());
	switch (_notice.rec_progress) {
////	case EN_REC_PROGRESS__INIT:
	case instance::progress::INIT:
		break;

////	case EN_REC_PROGRESS__PRE_PROCESS:
	case instance::progress::PRE_PROCESS:
////		// ここでは エラーが起きることがあるのでRESERVE_STATE をチェックします
////		if (_notice.reserve_state != RESERVE_STATE__INIT) {
////			recordings_[0].state |= _notice.reserve_state;
////		}
		break;

////	case EN_REC_PROGRESS__NOW_RECORDING:
	case instance::progress::NOW_RECORDING:
		break;

////	case EN_REC_PROGRESS__END_SUCCESS:
	case instance::progress::END_SUCCESS:
		recordings_[_notice.group_id].state |= RESERVE_STATE__END_SUCCESS;
		break;

////	case EN_REC_PROGRESS__END_ERROR:
	case instance::progress::END_ERROR:
		break;

////	case EN_REC_PROGRESS__POST_PROCESS: {
	case instance::progress::POST_PROCESS: {

		// NOW_RECORDINGフラグは落としておきます
		recordings_[_notice.group_id].state &= ~RESERVE_STATE__NOW_RECORDING;

		recordings_[_notice.group_id].recording_end_time.set_current_time();

		set_result (&recordings_[_notice.group_id]);


		// rename
		std::string path = settings_->get_params().get_rec_ts_path();

		char newfile [PATH_MAX] = {0};
		char *p_name = (char*)"rec";
		if (recordings_[_notice.group_id].title_name.c_str()) {
			p_name = (char*)recordings_[_notice.group_id].title_name.c_str();
		}
		arec::common::etime t_end;
		t_end.set_current_time();
		snprintf (
			newfile,
			sizeof(newfile),
			"%s/%s_0x%08x_%d_%s.m2ts",
			path.c_str(),
			p_name,
			recordings_[_notice.group_id].state,
			_notice.group_id,
			t_end.to_string()
		);

		rename (recording_tmpfiles_[_notice.group_id], newfile) ;

		recordings_[_notice.group_id].clear ();
		_UTL_LOG_I ("recording end...");


		//-----------------------------//
		{
			// 選局を停止・クローズ(投げっぱなし)
			uint8_t gr = _notice.group_id;
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop), (uint8_t*)&gr, sizeof(gr));
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::close), (uint8_t*)&gr, sizeof(gr));
		}
		//-----------------------------//


		}
		break;

	default:
		break;
	};

	iface->reply (coseq::result::success);

}

void manager::on_start_recording (coseq::iface *iface)
{
	_UTL_LOG_I ("(%s) entry", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ
	psisi_structs::event_info_t present_event_info;
	uint8_t group_id = *(uint8_t*)(iface->get_source().get_message().data());
	uint16_t ch = 0;

	// 選局停止・クローズ(END_ERROR 時)
	auto do_tuner_stop_close = [&]() {
		uint8_t gr = group_id;
		iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
						static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop), (uint8_t*)&gr, sizeof(gr));
		iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
						static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::close), (uint8_t*)&gr, sizeof(gr));
	};

	if (group_id >= tuner_resource_max_) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", group_id);
		// group_id 不正: tuner stop/close はしない(開いていない)が END_ERROR と同じ reply
		iface->reply (coseq::result::error);
		return;
	}
	recordings_[group_id].dump();

	// disk free space check
	{
		std::string path = settings_->get_params().get_rec_ts_path();
		int limit = settings_->get_params().get_rec_disk_space_low_limit_MB();
		int df = arec::common::utils::get_disk_free_mb(path.c_str());
		_UTL_LOG_I ("(%s) disk free space [%d] Mbytes.", iface->get_sequence_name(), df);
		if (df < limit) {
			// HDD残量たりないので 録画実行しません
			recordings_[group_id].state |= RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW;
			set_result (&recordings_[group_id]);
			recordings_[group_id].clear ();
			do_tuner_stop_close();
			iface->reply (coseq::result::error);
			return;
		}
	}

	// get physical channel by service id
	{
		arec::channel::client_iface::service_id_param_t param = {
			recordings_[group_id].transport_stream_id,
			recordings_[group_id].original_network_id,
			recordings_[group_id].service_id
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_physical_channel_by_service_id),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_physical_channel_by_service_id is failure.");
			do_tuner_stop_close();
			iface->reply (coseq::result::error);
			return;
		}
		ch = *(uint16_t*)(r.get_message().data());
	}

	// tune
	{
		arec::tuner_service::client_iface::tune_param_t param = { ch, group_id };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_service),
			static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_with_retry),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_tune is failure. -> not start recording");
			recordings_[group_id].state |= RESERVE_STATE__END_ERROR__TUNE_ERR;
			set_result (&recordings_[group_id]);
			recordings_[group_id].clear();
			do_tuner_stop_close();
			iface->reply (coseq::result::error);
			return;
		}
	}

	// event_type の録画はイベント開始時間を確認(manual は即録画)
	bool go_start = true;
	if (recordings_[group_id].is_event_type) {
		go_start = false;

		// get present event info (200ms×最大10リトライ)
		bool got = false;
		for (int retry = 0; retry <= 10; ++ retry) {
			psisi_structs::service_info_t _svc_info;
			_svc_info.transport_stream_id = recordings_[group_id].transport_stream_id;
			_svc_info.original_network_id = recordings_[group_id].original_network_id;
			_svc_info.service_id = recordings_[group_id].service_id;

			psisi_structs::request_event_info_param_t eparam = { _svc_info, &present_event_info };
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
				static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_present_event_info),
				(uint8_t*)&eparam, sizeof(eparam));
			if (r.get_result() == coseq::result::success) {
				got = true;
				break;
			}
			_UTL_LOG_W ("(%s) request_get_present_event_info retry", iface->get_sequence_name());
			iface->wait_timeout (200); // 200mS
		}

		if (!got) {
			_UTL_LOG_E ("(%s) request_get_present_event_info err", iface->get_sequence_name());
			recordings_[group_id].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			set_result (&recordings_[group_id]);
			recordings_[group_id].clear();
			do_tuner_stop_close();
			iface->reply (coseq::result::error);
			return;
		}

		if (present_event_info.event_id == recordings_[group_id].event_id) {
			// イベント一致 → 録画開始
			go_start = true;

		} else {
			// イベント不一致: 前番組延長等で開始が遅れたと予想 → EIT schedule 取得して予約入れ直し
			_UTL_LOG_E ("(%s) event tracking...", iface->get_sequence_name());

			coseq::source &rc = iface->request (
				static_cast<uint8_t>(modules::module_id::event_schedule_manager),
				static_cast<uint8_t>(arec::event_schedule::client_iface::sequence::cache_schedule_force_current_service),
				(uint8_t*)&group_id, sizeof(group_id));
			if (rc.get_result() != coseq::result::success) {
				_UTL_LOG_E ("(%s) request_cache_schedule_force_current_service err", iface->get_sequence_name());
				recordings_[group_id].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
				set_result (&recordings_[group_id]);
				recordings_[group_id].clear();
				do_tuner_stop_close();
				iface->reply (coseq::result::error);
				return;
			}

			// 予約入れ直し
			_UTL_LOG_E ("(%s) try reserve reschedule", iface->get_sequence_name());
			client_iface::add_reserve_param_t _param;
			_param.transport_stream_id = recordings_[group_id].transport_stream_id;
			_param.original_network_id = recordings_[group_id].original_network_id;
			_param.service_id = recordings_[group_id].service_id;
			_param.event_id = recordings_[group_id].event_id;
			_param.repeatablity = recordings_[group_id].repeatability;
			_param.dump();

			coseq::source &ra = iface->request (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<uint8_t>(client_iface::sequence::add_reserve_event),
				(uint8_t*)&_param, sizeof(_param));
			if (ra.get_result() == coseq::result::success) {
				_UTL_LOG_I ("(%s) add reserve reschedule ok.", iface->get_sequence_name());
				recordings_[group_id].clear();
			} else {
				_UTL_LOG_E ("(%s) add reserve reschedule err...", iface->get_sequence_name());
				recordings_[group_id].state |= RESERVE_STATE__END_ERROR__EVENT_NOT_FOUND;
				set_result (&recordings_[group_id]);
				recordings_[group_id].clear();
			}

			// 今回は録画せず選局止める(END_ERROR 扱い)
			do_tuner_stop_close();
			iface->reply (coseq::result::error);
			return;
		}
	}

	// START RECORDING
	if (go_start) {
		if (rec_instances_[group_id]->get_current_progress() != instance::progress::INIT) {
			_UTL_LOG_E ("progress != INIT ???  -> not start recording");
			recordings_[group_id].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			set_result (&recordings_[group_id]);
			recordings_[group_id].clear();
			do_tuner_stop_close();
			iface->reply (coseq::result::error);
			return;
		}

		_UTL_LOG_I ("start recording (on tune thread)");

		memset (recording_tmpfiles_[group_id], 0x00, PATH_MAX);
		std::string path = settings_->get_params().get_rec_ts_path();
		snprintf (
			recording_tmpfiles_[group_id],
			PATH_MAX,
			"%s/tmp.m2ts.%lu.%d",
			path.c_str(),
			pthread_self(),
			group_id
		);

		// ######################################### //
		std::string s = recording_tmpfiles_[group_id];
		rec_instances_[group_id]->set_rec_filename(s);
		rec_instances_[group_id]->set_service_id(recordings_[group_id].service_id);
		rec_instances_[group_id]->set_use_splitter(arec::settings::get_instance()->get_params().is_rec_use_splitter());
		rec_instances_[group_id]->set_next_progress(instance::progress::PRE_PROCESS);
		// ######################################### //

		recordings_[group_id].state |= RESERVE_STATE__NOW_RECORDING;
		recordings_[group_id].recording_start_time.set_current_time();
	}

	// END SUCCESS
	iface->reply (coseq::result::success);
}

void manager::on_add_reserve_current_event (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ(psisi へポインタ渡しするバッファ含む)
	psisi_structs::service_info_t service_infos[10];
	memset (service_infos, 0x00, sizeof(service_infos));
	psisi_structs::event_info_t present_event_info;

	uint8_t group_id = *(uint8_t*)(iface->get_source().get_message().data());
	if (group_id >= tuner_resource_max_) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", group_id);
		iface->reply (coseq::result::error);
		return;
	}

	// get tuner state
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + group_id,
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::get_state));
		if (r.get_result() == coseq::result::success) {
			arec::tuner_control::client_iface::tuner_state _state = *(arec::tuner_control::client_iface::tuner_state*)(r.get_message().data());
			if (_state != arec::tuner_control::client_iface::tuner_state::tuning_success) {
				_UTL_LOG_E ("not arec::tuner_control::client_iface::tuner_state::tuning_success %d", _state);
#ifndef _DUMMY_TUNER
				iface->reply (coseq::result::error);
				return;
#endif
			}
		}
	}

	// get pat detect state
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_pat_detect_state));
		if (r.get_result() == coseq::result::success) {
			arec::psisi::client_iface::pat_detection_state _state = *(arec::psisi::client_iface::pat_detection_state*)(r.get_message().data());
			if (_state != arec::psisi::client_iface::pat_detection_state::detected) {
				_UTL_LOG_E ("pat_detection_state::not_detected %d", _state);
#ifndef _DUMMY_TUNER
				iface->reply (coseq::result::error);
				return;
#endif
			}
		}
	}

	// get current service infos
	{
		psisi_structs::request_service_infos_param_t sparam = { service_infos, 10 };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_current_service_infos),
			(uint8_t*)&sparam, sizeof(sparam));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_current_service_infos err");
			iface->reply (coseq::result::error);
			return;
		}
		int num = *(int*)(r.get_message().data());
		if (num <= 0) {
			_UTL_LOG_E ("request_get_current_service_infos is 0");
			iface->reply (coseq::result::error);
			return;
		}
		service_infos[0].dump();
	}

	// get present event info
	// TODO service_infos[0] 暫定0番目を使用(大概 service_type=0x01 で HD画質だろうか)
	{
		psisi_structs::request_event_info_param_t eparam = { service_infos[0], &present_event_info };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_present_event_info),
			(uint8_t*)&eparam, sizeof(eparam));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("(%s) request_get_present_event_info err", iface->get_sequence_name());
			iface->reply (coseq::result::error);
			return;
		}
	}
	present_event_info.dump();

	// get service name (sync)
	char *p_svc_name = NULL;
	{
		arec::channel::client_iface::service_id_param_t param = {
			present_event_info.transport_stream_id,
			present_event_info.original_network_id,
			present_event_info.service_id
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_service_name),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() == coseq::result::success) {
			// この後の add_reserve 内で実態コピーする前提です
			p_svc_name = (char*)(r.get_message().data());
		}
	}

	bool r = add_reserve (
					present_event_info.transport_stream_id,
					present_event_info.original_network_id,
					present_event_info.service_id,
					present_event_info.event_id,
					&present_event_info.start_time,
					&present_event_info.end_time,
					present_event_info.event_name_char,
					p_svc_name,
					true, // is_event_type true
					client_iface::reserve_repeatability::none
				);

	iface->reply (r ? coseq::result::success : coseq::result::error);
}

void manager::on_add_reserve_event (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ
	client_iface::add_reserve_param_t param_in =
		*(client_iface::add_reserve_param_t*)(iface->get_source().get_message().data());
	arec::event_schedule::client_iface::event_t event;

	// repeatablity のチェック
	if (!(
		param_in.repeatablity == client_iface::reserve_repeatability::none ||
		param_in.repeatablity == client_iface::reserve_repeatability::auto_
	)) {
		_UTL_LOG_E ("invalid repeatablity");
		iface->reply (coseq::result::error);
		return;
	}

	// reschedule かのチェック
	bool is_rescheduled = false;
	{
		uint8_t src_thread_idx = iface->get_source().get_thread_idx();
		uint8_t src_seq_idx = iface->get_source().get_sequence_idx();
		if ((src_thread_idx == static_cast<uint8_t>(modules::module_id::rec_manager)) && (src_seq_idx == static_cast<int>(client_iface::sequence::start_recording))) {
			_UTL_LOG_I ("(%s) requst from client_iface::sequence::start_recording", iface->get_sequence_name());
			is_rescheduled = true;
		}
	}

	// get event
	{
		arec::event_schedule::client_iface::request_event_param_t param = {
			{
				param_in.transport_stream_id,
				param_in.original_network_id,
				param_in.service_id,
				param_in.event_id
			},
			&event
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::event_schedule_manager),
			static_cast<uint8_t>(arec::event_schedule::client_iface::sequence::get_event),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			// 予約に対応するイベントがなかった あらら...
			_UTL_LOG_E ("on_add_reserve_event - request_get_event error");
			iface->reply (coseq::result::error);
			return;
		}
	}

	// get service name (sync)
	char *p_svc_name = NULL;
	{
		arec::channel::client_iface::service_id_param_t param = {
			param_in.transport_stream_id,
			param_in.original_network_id,
			param_in.service_id
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_service_name),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() == coseq::result::success) {
			p_svc_name = (char*)(r.get_message().data());
		}
	}

	bool r = add_reserve (
					event.transport_stream_id,
					event.original_network_id,
					event.service_id,
					event.event_id,
					&event.start_time,
					&event.end_time,
					event.p_event_name->c_str(),
					p_svc_name,
					true, // is_event_type true
					param_in.repeatablity,
					is_rescheduled
				);

	iface->reply (r ? coseq::result::success : coseq::result::error);
}

void manager::on_add_reserve_event_helper (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ
	client_iface::add_reserve_helper_param_t param_in =
		*(client_iface::add_reserve_helper_param_t*)(iface->get_source().get_message().data());
	arec::event_schedule::client_iface::event_t event;

	// repeatablity のチェック
	if (!(
		param_in.repeatablity == client_iface::reserve_repeatability::none ||
		param_in.repeatablity == client_iface::reserve_repeatability::auto_
	)) {
		iface->reply (coseq::result::error);
		return;
	}

	// get event (latest dumped schedule, index 指定)
	{
		arec::event_schedule::client_iface::request_event_param_t param;
		param.arg.index = param_in.index;
		param.p_out_event = &event;

		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::event_schedule_manager),
			static_cast<uint8_t>(arec::event_schedule::client_iface::sequence::get_event__latest_dumped_schedule),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			// 予約に対応するイベントがなかった あらら...
			_UTL_LOG_E ("request_get_event error");
			iface->reply (coseq::result::error);
			return;
		}
	}

	// get service name (sync)
	char *p_svc_name = NULL;
	{
		arec::channel::client_iface::service_id_param_t param = {
			event.transport_stream_id,
			event.original_network_id,
			event.service_id
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_service_name),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() == coseq::result::success) {
			p_svc_name = (char*)(r.get_message().data());
		}
	}

	bool r = add_reserve (
					event.transport_stream_id,
					event.original_network_id,
					event.service_id,
					event.event_id,
					&event.start_time,
					&event.end_time,
					event.p_event_name->c_str(),
					p_svc_name,
					true, // is_event_type true
					param_in.repeatablity
				);

	iface->reply (r ? coseq::result::success : coseq::result::error);
}

void manager::on_add_reserve_manual (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ
	client_iface::add_reserve_param_t param_in =
		*(client_iface::add_reserve_param_t*)(iface->get_source().get_message().data());

	// repeatablity のチェック
	if (!(
		param_in.repeatablity >= client_iface::reserve_repeatability::none &&
		param_in.repeatablity <= client_iface::reserve_repeatability::weekly
	)) {
		iface->reply (coseq::result::error);
		return;
	}

	// サービスの存在チェック(get physical channel by service id)
	{
		arec::channel::client_iface::service_id_param_t param = {
			param_in.transport_stream_id,
			param_in.original_network_id,
			param_in.service_id
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_physical_channel_by_service_id),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_physical_channel_by_service_id is failure.");
			iface->reply (coseq::result::error);
			return;
		}
	}

	// get service name (sync)
	char *p_svc_name = NULL;
	{
		arec::channel::client_iface::service_id_param_t param = {
			param_in.transport_stream_id,
			param_in.original_network_id,
			param_in.service_id
		};
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_service_name),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() == coseq::result::success) {
			p_svc_name = (char*)(r.get_message().data());
		}
	}

	bool r = add_reserve (
					param_in.transport_stream_id,
					param_in.original_network_id,
					param_in.service_id,
					0x00, // event_id はこの時点では不明
					&param_in.start_time,
					&param_in.end_time,
					NULL, // タイトル名も不明
					p_svc_name,
					false,
					param_in.repeatablity
				);

	iface->reply (r ? coseq::result::success : coseq::result::error);
}

void manager::on_remove_reserve (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::remove_reserve_param_t param =
			*(client_iface::remove_reserve_param_t*)(iface->get_source().get_message().data());

	int idx = get_reserve_index (
					param.arg.key.transport_stream_id,
					param.arg.key.original_network_id,
					param.arg.key.service_id,
					param.arg.key.event_id
				);

	if (idx < 0) {
		iface->reply (coseq::result::error);

	} else {
		if (remove_reserve (idx, param.is_consider_repeatability, param.is_apply_result)) {
			iface->reply (coseq::result::success);
		} else {
			iface->reply (coseq::result::error);
		}
	}

}

void manager::on_remove_reserve_by_index (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::remove_reserve_param_t param =
			*(client_iface::remove_reserve_param_t*)(iface->get_source().get_message().data());

	if (remove_reserve (param.arg.index, param.is_consider_repeatability, param.is_apply_result)) {
		iface->reply (coseq::result::success);
	} else {
		iface->reply (coseq::result::error);
	}


}

void manager::on_get_reserves (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::get_reserves_param_t _param =
			*(client_iface::get_reserves_param_t*)(iface->get_source().get_message().data());

	if (!_param.p_out_reserves) {
		_UTL_LOG_E ("_param.p_out_reserves is null.");
		iface->reply (coseq::result::error);

	} else if (_param.array_max_num <= 0) {
		_UTL_LOG_E ("_param.array_max_num is invalid.");
		iface->reply (coseq::result::error);

	} else {

		int n = get_reserves (_param.p_out_reserves, _param.array_max_num);
		// 取得数をreply msgで返します
		iface->reply (coseq::result::success, (uint8_t*)&n, sizeof(n));
	}

}

void manager::on_stop_recording (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	int group_id = *(uint8_t*)(iface->get_source().get_message().data());
	if (group_id >= tuner_resource_max_) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", group_id);
		iface->reply (coseq::result::error);

	} else {
		if (recordings_[group_id].state & RESERVE_STATE__NOW_RECORDING) {

			// stop_recording が呼ばれたらエラー終了にしておきます
////		_UTL_LOG_W ("rec_progress_ = EN_REC_PROGRESS__END_ERROR");
			_UTL_LOG_W ("progress = END_ERROR");


			// ######################################### //
////		rec_progress_ = EN_REC_PROGRESS__END_ERROR;
			rec_instances_[group_id]->set_next_progress(instance::progress::END_ERROR);
			// ######################################### //


			recordings_[group_id].state |= RESERVE_STATE__FORCE_STOPPED;

			iface->reply (coseq::result::success);

		} else {
			_UTL_LOG_E ("invalid rec state (not recording now...)");
			iface->reply (coseq::result::error);
		}
	}

}

void manager::on_dump_reserves (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	int type = *(int*)(iface->get_source().get_message().data());
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


	iface->reply (coseq::result::success);

}

void manager::on_receive_notify (coseq::iface *iface)
{
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		if (iface->get_source().get_client_id() == tuner_notify_client_id_[_gr]) {

			arec::tuner_control::client_iface::tuner_state en_state = *(arec::tuner_control::client_iface::tuner_state*)(iface->get_source().get_message().data());
			switch (en_state) {
				case arec::tuner_control::client_iface::tuner_state::tuning_begin:
				_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tuning_begin group_id:[%d]", _gr);
				break;

				case arec::tuner_control::client_iface::tuner_state::tuning_success:
				_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tuning_success group_id:[%d]", _gr);
				break;

				case arec::tuner_control::client_iface::tuner_state::tuning_error_stop:
				_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tuning_error_stop group_id:[%d]", _gr);
				break;

				case arec::tuner_control::client_iface::tuner_state::tune_stop:
				_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tune_stop group_id:[%d]", _gr);
				break;

			default:
				break;
			}


		} else if (iface->get_source().get_client_id() == pat_detect_notify_client_id_[_gr]) {

			arec::psisi::client_iface::pat_detection_state _state = *(arec::psisi::client_iface::pat_detection_state*)(iface->get_source().get_message().data());
			if (_state == arec::psisi::client_iface::pat_detection_state::detected) {
				_UTL_LOG_I ("pat_detection_state::detected");

			} else if (_state == arec::psisi::client_iface::pat_detection_state::not_detected) {
				_UTL_LOG_E ("pat_detection_state::not_detected");

				// PAT途絶したら録画中止します
				if (recordings_[_gr].state & RESERVE_STATE__NOW_RECORDING) {

					// ######################################### //
////				rec_progress_ = EN_REC_PROGRESS__POST_PROCESS;
					rec_instances_[_gr]->set_next_progress(instance::progress::POST_PROCESS);
					// ######################################### //


					recordings_[_gr].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;

					// 自ら呼び出します(内部の自リクエストは coseq では send=fire-and-forget)
					// PAT途絶して i_ts_receive_handler は動いていない前提
					rec_instances_[_gr]->on_ts_received (NULL, 0);

				}
			}

		} else if (iface->get_source().get_client_id() == event_change_notify_client_id_[_gr]) {

			psisi_structs::notify_event_info_t _info = *(psisi_structs::notify_event_info_t*)(iface->get_source().get_message().data());
			_UTL_LOG_I ("!!! event changed !!! group_id:[%d]", _gr);
			_info.dump ();

		}
	}
}

bool manager::add_reserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id,
	arec::common::etime* p_start_time,
	arec::common::etime* p_end_time,
	const char *psz_title_name,
	const char *psz_service_name,
	bool _is_event_type,
	client_iface::reserve_repeatability repeatabilitiy,
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


	reserve tmp;
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
		for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
			if (recordings_[_gr].is_used) {
				if (recordings_[_gr] == tmp) {
					_UTL_LOG_E ("reserve is now recording.");
					return false;
				}
			}
		}
	}


	reserve* p_reserve = search_ascending_order_reserve (p_start_time);
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
int manager::get_reserve_index (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id
)
{
	reserve tmp;
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
		client_iface::reserve_repeatability::none
	);

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (reserves_ [i] == tmp) {
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
bool manager::remove_reserve (int index, bool is_consider_repeatability, bool is_apply_result)
{
	if (index >= RESERVE_NUM_MAX) {
		return false;
	}

	reserves_ [index].state |= RESERVE_STATE__REMOVED;
	if (is_apply_result) {
		set_result (&reserves_[index]);
	}

	_UTL_LOG_I ("##############################");
	_UTL_LOG_I ("####    remove reserve    ####");
	_UTL_LOG_I ("##############################");
	reserves_ [index].dump();

	if (is_consider_repeatability) {
		check_repeatability (&reserves_[index]);
	}

	// 間詰め
	for (int i = index; i < RESERVE_NUM_MAX -1; ++ i) {
		reserves_ [i] = reserves_ [i+1];
	}
	reserves_ [RESERVE_NUM_MAX -1].clear();


	// 毎回書き込み
	save_reserves ();


	return true;
}

/**
 * 開始時間を基準に降順で空きをさがします
 * 空きがある前提
 */
reserve* manager::search_ascending_order_reserve (arec::common::etime *p_start_time_ref)
{
	if (!p_start_time_ref) {
		return NULL;
	}


	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {

		// 先頭から詰まっているはず
//		if (!reserves_ [i].is_used) {
//			continue;
//		}

		// 基準時間より後ろの時間を探します
		if (reserves_ [i].start_time > *p_start_time_ref) {

			// 後ろから見てずらします
			for (int j = RESERVE_NUM_MAX -1; j > i; -- j) {
				reserves_ [j] = reserves_ [j-1] ;
			}

			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		// 見つからなかったので最後尾にします
		return find_empty_reserve ();

	} else {

		reserves_ [i].clear ();
		return &reserves_ [i];
	}
}

bool manager::is_exist_empty_reserve (void) const
{
	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		_UTL_LOG_W ("reserves_ full.");
		return false;
	}

	return true;
}

reserve* manager::find_empty_reserve (void)
{
	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		_UTL_LOG_W ("reserves_ full.");
		return NULL;
	}

	return &reserves_ [i];
}

bool manager::is_duplicate_reserve (const reserve* p_reserve) const
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (reserves_ [i] == *p_reserve) {
			// duplicate
			return true;
		}
	}

	return false;
}

bool manager::is_overrap_time_reserve (const reserve* p_reserve) const
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (reserves_ [i].start_time >= p_reserve->start_time && reserves_ [i].start_time >= p_reserve->end_time) {
			continue;
		}

		if (reserves_ [i].end_time <= p_reserve->start_time && reserves_ [i].end_time <= p_reserve->end_time) {
			continue;
		}

		// overrap
		return true;
	}

	return false;
}

void manager::check_reserves (void)
{
	arec::common::etime current_time;
	current_time.set_current_time();

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {

		if (!reserves_ [i].is_used) {
			continue;
		}

		// 開始時間/終了時間とも過ぎていたら resultsに入れときます
		if (reserves_ [i].start_time < current_time && reserves_ [i].end_time <= current_time) {
			reserves_ [i].state |= RESERVE_STATE__END_ERROR__ALREADY_PASSED;
			set_result (&reserves_[i]);
			check_repeatability (&reserves_[i]);
			continue;
		}

		// 開始時間をみて 時間が来たら 録画要求を立てます
		// start_time - end_time 範囲内
		if (reserves_ [i].start_time <= current_time && reserves_ [i].end_time > current_time) {

			// 開始時間から10秒以上経過していたら フラグたてときます
			arec::common::etime tmp = reserves_ [i].start_time;
			tmp.add_sec(10);
			if (tmp <= current_time) {
				reserves_ [i].state |= RESERVE_STATE__START_TIME_PASSED;
			}

//			if (!recordings_[0].is_used) {
//				// 録画中でなければ 録画開始フラグ立てます
// 録画してても関係なく録画開始フラグ立てます
				reserves_ [i].state |= RESERVE_STATE__REQ_START_RECORDING;

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
void manager::refresh_reserves (uint32_t state)
{
	// 逆から見ます
	for (int i = RESERVE_NUM_MAX -1; i >= 0; -- i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (!(reserves_[i].state & state)) {
			continue;
		}

		// 間詰め
		for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
			reserves_ [j] = reserves_ [j+1];
		}
		reserves_ [RESERVE_NUM_MAX -1].clear();

		save_reserves ();
	}
}

bool manager::is_exist_req_start_recording_reserve (void)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (reserves_[i].state & RESERVE_STATE__REQ_START_RECORDING) {
			return true;
		}
	}

	return false;
}

bool manager::pick_req_start_recording_reserve (uint8_t group_id)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (reserves_[i].state & RESERVE_STATE__REQ_START_RECORDING) {

			// 次に録画する予約を取り出します
			recordings_[group_id] = reserves_[i];

			// フラグは落としておきます
			recordings_[group_id].state &= ~RESERVE_STATE__REQ_START_RECORDING;

			recordings_[group_id].group_id = group_id;


			// 間詰め
			for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
				reserves_ [j] = reserves_ [j+1];
			}
			reserves_ [RESERVE_NUM_MAX -1].clear();

			check_repeatability (&recordings_[group_id]);

			save_reserves ();

			return true;
		}
	}

	return false;
}

/**
 * 録画結果をリストに保存します
 */
void manager::set_result (reserve *p)
{
	if (!p) {
		return ;
	}


	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (!results_ [i].is_used) {
			results_ [i] = *p;

			// 毎回書き込み
			save_results ();

			return ;
		}
	}

	// results_ full
	// 最古のものを消します
	for (int i = 0; i < RESULT_NUM_MAX -1; ++ i) {
		results_ [i] = results_ [i + 1] ;		
	}

	results_ [RESULT_NUM_MAX -1].clear ();
	results_ [RESULT_NUM_MAX -1] = *p;


	// 毎回書き込み
	save_results ();
}

void manager::check_recording_end (void)
{
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		if (!recordings_[_gr].is_used) {
			continue;
		}

		if (!(recordings_[_gr].state & RESERVE_STATE__NOW_RECORDING)) {
			continue;
		}

		arec::common::etime current_time ;
		current_time.set_current_time();

		if (recordings_[_gr].end_time <= current_time) {

			// 録画 正常終了します
			if (rec_instances_[_gr]->get_current_progress() == instance::progress::NOW_RECORDING) {
				_UTL_LOG_I ("progress = END_SUCCESS");


				// ######################################### //
				rec_instances_[_gr]->set_next_progress(instance::progress::END_SUCCESS);
				// ######################################### //


			}
		}
	}
}

void manager::check_disk_free (coseq::iface *iface)
{
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		if (!recordings_[_gr].is_used) {
			continue;
		}

		if (recordings_[_gr].state & RESERVE_STATE__NOW_RECORDING) {

			std::string path = settings_->get_params().get_rec_ts_path();
			int limit = settings_->get_params().get_rec_disk_space_low_limit_MB();
			int df = arec::common::utils::get_disk_free_mb(path.c_str());
			if (df < limit) {
				_UTL_LOG_E ("(check_recording_disk_free) disk free space [%d] Mbytes !!", df);
				// HDD残量たりない場合は 録画を止めます
				recordings_[_gr].state |= RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW;

				uint8_t group_id = _gr;

				// 投げっぱなしで stop_recording を自リクエスト
				iface->send (static_cast<uint8_t>(modules::module_id::rec_manager),
								static_cast<uint8_t>(client_iface::sequence::stop_recording),
								(uint8_t*)&group_id, sizeof(uint8_t));
			}
		}
	}
}

//TODO event_typeの場合はどうするか
/**
 * Repeatabilityの確認して
 * 予約入れるべきものは予約します
 */
void manager::check_repeatability (const reserve *p_reserve)
{
	if (!p_reserve) {
		return ;
	}

	arec::common::etime s;
	arec::common::etime e;
	s = p_reserve->start_time;
	e = p_reserve->end_time;

	switch (p_reserve->repeatability) {
	case client_iface::reserve_repeatability::none:
		return;
		break;

	case client_iface::reserve_repeatability::daily:
		s.add_day(1);
		e.add_day(1);
		break;

	case client_iface::reserve_repeatability::weekly:
		s.add_week(1);
		e.add_week(1);
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

int manager::get_reserves (client_iface::reserve_t *p_out_reserves, int out_array_num) const
{
	if (!p_out_reserves || out_array_num <= 0) {
		return -1;
	}

	int n = 0;

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!reserves_ [i].is_used) {
			continue;
		}

		if (out_array_num == 0) {
			break;
		}

		p_out_reserves->transport_stream_id = reserves_ [i].transport_stream_id;
		p_out_reserves->original_network_id = reserves_ [i].original_network_id;
		p_out_reserves->service_id = reserves_ [i].service_id;

		p_out_reserves->event_id = reserves_ [i].event_id;
		p_out_reserves->start_time = reserves_ [i].start_time;
		p_out_reserves->end_time = reserves_ [i].end_time;

		// アドレスで渡します
		p_out_reserves->p_title_name = const_cast<std::string*> (&reserves_ [i].title_name);
		p_out_reserves->p_service_name = const_cast<std::string*> (&reserves_ [i].service_name);

		p_out_reserves->is_event_type = reserves_ [i].is_event_type;
		p_out_reserves->repeatability = reserves_ [i].repeatability;

		++ p_out_reserves;
		-- out_array_num;
		++ n;
	}

	return n;
}

void manager::dump_reserves (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (reserves_ [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			_UTL_LOG_I ("-- index=[%d] --", i);
			reserves_ [i].dump();
		}
	}
}

void manager::dump_results (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (results_ [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			results_ [i].dump();
		}
	}
}

void manager::dump_recording (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	int i = 0;
	for (i = 0; i < arec::group::GROUP_MAX; ++ i) {
		if (recordings_[i].is_used) {
			_UTL_LOG_I ("-----------   now recording   -----------");
			recordings_[i].dump();
		}
	}
}

void manager::clear_reserves (void)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		reserves_ [i].clear();
	}
}

void manager::clear_results (void)
{
	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		results_ [i].clear();
	}
}


#if 0
//////////  arec::tuner_control::client_iface::i_ts_receive_handler  //////////

bool manager::on_pre_ts_receive (void)
{
	get_external_if()->create_external_cp();

	uint32_t opt = get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	get_external_if()->set_request_option (opt);

	return true;
}

void manager::on_post_ts_receive (void)
{
	uint32_t opt = get_external_if()->get_request_option ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	get_external_if()->set_request_option (opt);

	get_external_if()->destroy_external_cp();
}

bool manager::on_check_ts_receive_loop (void)
{
	return true;
}

bool manager::on_ts_received (void *p_ts_data, int length)
{
	switch (rec_progress_) {
	case EN_REC_PROGRESS__PRE_PROCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__PRE_PROCESS");

		std::unique_ptr<arib_b25> b25 (new arib_b25(8192, recording_tmpfile_));
		b25_.swap (b25);

		RECORDING_NOTICE_t _notice = {rec_progress_, RESERVE_STATE__INIT};
		request_async (
			static_cast<uint8_t>(modules::module_id::rec_manager),
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		rec_progress_ = EN_REC_PROGRESS__NOW_RECORDING;
		_UTL_LOG_I ("next  EN_REC_PROGRESS__NOW_RECORDING");

		}
		break;

	case EN_REC_PROGRESS__NOW_RECORDING: {

		// recording
		int r = b25_->put ((uint8_t*)p_ts_data, length);
		if (r < 0) {
			_UTL_LOG_W ("TS write failed");
		}

		}
		break;

	case EN_REC_PROGRESS__END_SUCCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_SUCCESS");

		RECORDING_NOTICE_t _notice = {rec_progress_, RESERVE_STATE__INIT};
		request_async (
			static_cast<uint8_t>(modules::module_id::rec_manager),
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		rec_progress_ = EN_REC_PROGRESS__POST_PROCESS;

		}
		break;

	case EN_REC_PROGRESS__END_ERROR: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_ERROR");

		RECORDING_NOTICE_t _notice = {rec_progress_, RESERVE_STATE__INIT};
		request_async (
			static_cast<uint8_t>(modules::module_id::rec_manager),
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		rec_progress_ = EN_REC_PROGRESS__POST_PROCESS;

		}
		break;

	case EN_REC_PROGRESS__POST_PROCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__POST_PROCESS");

		b25_->flush();
		b25_->release();

		RECORDING_NOTICE_t _notice = {rec_progress_, RESERVE_STATE__INIT};
		request_async (
			static_cast<uint8_t>(modules::module_id::rec_manager),
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		rec_progress_ = EN_REC_PROGRESS__INIT;

		}
		break;

	default:
		break;
	}

	return true;
}
#endif

void manager::save_reserves (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_reserves", reserves_), sizeof(reserve) * RESERVE_NUM_MAX);
	}

	std::string path = settings_->get_params().get_rec_reserves_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_reserves (void)
{
	std::string path = settings_->get_params().get_rec_reserves_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("rec_reserves.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_reserves", reserves_), sizeof(reserve) * RESERVE_NUM_MAX);

	ifs.close();
	ss.clear();


	// etimeの値は直接 tv_sec,tvnsecに書いてるので to_string用の文字はここで作ります
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		reserves_ [i].start_time.update_strings();
		reserves_ [i].end_time.update_strings();
		reserves_ [i].recording_start_time.update_strings();
		reserves_ [i].recording_end_time.update_strings();
	}
}

void manager::save_results (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_results", results_), sizeof(reserve) * RESULT_NUM_MAX);
	}

	std::string path = settings_->get_params().get_rec_results_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_results (void)
{
	std::string path = settings_->get_params().get_rec_results_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("rec_results.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_results", results_), sizeof(reserve) * RESULT_NUM_MAX);

	ifs.close();
	ss.clear();


	// etimeの値は直接 tv_sec,tv_nsecに書いてるので to_string用の文字はここで作ります
	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		results_ [i].start_time.update_strings();
		results_ [i].end_time.update_strings();
		results_ [i].recording_start_time.update_strings();
		results_ [i].recording_end_time.update_strings();
	}
}


} // namespace rec
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
namespace rec {

template <class Archive>
void serialize (Archive &archive, reserve &r)
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

} // namespace rec
} // namespace arec
