#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#include "channel_manager_if.h"
#include "etime.h"
#include "group.h"
#include "psisi_manager_if.h"
#include "stream_handler.h"
#include "viewing_manager.h"

#include "utils.h"
#include "viewing_manager_if.h"
#include "modules.h"

#include "aribstr.h"


namespace arec {
namespace viewing {


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

manager::manager (std::string name, uint8_t que_max)
	:coseq::module_base (name, que_max)
	,tuner_resource_max_ (0)
{
	const uint8_t _max = static_cast<uint8_t>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_up_by_group_id(iface);}, std::move("on_module_up_by_group_id")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){manager::on_check_loop(iface);}, std::move("on_check_loop")},
		{[&](coseq::iface *iface){manager::on_notice_by_stream_handler(iface);}, std::move("on_notice_by_stream_handler")},
		{[&](coseq::iface *iface){manager::on_start_viewing(iface);}, std::move("on_start_viewing_by_physical_channel")},
		{[&](coseq::iface *iface){manager::on_start_viewing(iface);}, std::move("on_start_viewing_by_service_id")},
		{[&](coseq::iface *iface){manager::on_stop_viewing(iface);}, std::move("on_stop_viewing")},
		{[&](coseq::iface *iface){manager::on_get_viewing(iface);}, std::move("on_get_viewing")},
		{[&](coseq::iface *iface){manager::on_set_option(iface);}, std::move("on_set_option")},
		{[&](coseq::iface *iface){manager::on_dump_viewing(iface);}, std::move("on_dump_viewing")},
		{[&](coseq::iface *iface){manager::on_event_changed(iface);}, std::move("on_event_changed")},
	};
	set_sequences (seqs, _max);

	settings_ = arec::settings::get_instance();

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		viewings_[_gr].clear();
	}

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		forker_[_gr].set_log_cb(forker_log_print);
	}

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		tuner_notify_client_id_ [_gr] = 0xff;
		pat_detect_notify_client_id_ [_gr] = 0xff;
		event_change_notify_client_id_ [_gr] = 0xff;
	}
}

manager::~manager (void)
{
	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		viewings_[_gr].clear();
	}

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		forker_[_gr].destroy_pipes();
	}

	for (int _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		tuner_notify_client_id_ [_gr] = 0xff;
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

	std::string stream_path = arec::settings::get_instance()->get_params().get_viewing_stream_data_path();
	for (int i = 0; i < tuner_resource_max_; ++ i) {
		std::string path = stream_path + "/";
		path += std::to_string(i);
		arec::common::utils::makedir(path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	}

	// group ごとに module_up_by_groupid を順に実行
	for (uint8_t gr = 0; gr < arec::group::GROUP_MAX; ++ gr) {
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::viewing_manager),
			static_cast<uint8_t>(client_iface::sequence::module_up_by_groupid),
			reinterpret_cast<uint8_t*>(&gr),
			sizeof(gr)
		);
		if (r.get_result() != coseq::result::success) {
			iface->reply (coseq::result::error);
			return;
		}
	}

	// 監視ループ起動(check_loop は先に reply してループに入る)
	iface->request (static_cast<uint8_t>(modules::module_id::viewing_manager),
					static_cast<uint8_t>(client_iface::sequence::check_loop));

	iface->reply (coseq::result::success);
}

void manager::on_module_up_by_group_id (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// request msg から group_id を取得します
	uint8_t group_id = *(reinterpret_cast<uint8_t*>(iface->get_source().get_message().data()));
	_UTL_LOG_I("(%s) group_id:[%d]", iface->get_sequence_name(), group_id);

	// register tuner notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + group_id,
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::reg_tuner_notify)
		);
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_tuner_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		tuner_notify_client_id_ [group_id] = *(reinterpret_cast<uint8_t*>(r.get_message().data()));
	}

	// register pat detect notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::reg_pat_detect_notify)
		);
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_pat_detect_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		pat_detect_notify_client_id_ [group_id] = *(reinterpret_cast<uint8_t*>(r.get_message().data()));
	}

	// register event change notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
			static_cast<uint8_t>(arec::psisi::client_iface::sequence::reg_event_change_notify)
		);
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_event_change_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		event_change_notify_client_id_ [group_id] = *(reinterpret_cast<uint8_t*>(r.get_message().data()));
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

		// check auto stop
		for (int i = 0; i < arec::group::GROUP_MAX; ++ i) {
			if (viewings_[i].is_used && viewings_[i].auto_stop) {
				arec::common::etime current_time ;
				current_time.set_current_time();
				if (viewings_[i].auto_stop_time <= current_time) {
					// auto stopの時刻を過ぎたので 視聴を止めます
					_UTL_LOG_I("### auto stop - group:[%d] ###", viewings_[i].group_id);
					viewings_[i].dump();

					// 投げっぱなし(自リクエスト)
					iface->send (
						static_cast<uint8_t>(modules::module_id::viewing_manager),
						static_cast<uint8_t>(client_iface::sequence::stop_viewing),
						(uint8_t*)&viewings_[i].group_id,
						sizeof(uint8_t)
					);
				}
			}
		}

		// check child
		for (int i = 0; i < arec::group::GROUP_MAX; ++ i) {
			if (!forker_[i].has_child()) {
				continue;
			}

			arec::common::forker::child_status cs = forker_[i].wait_child(-1, true);
			if (cs.get_status() == 0) {
				// alive
				if (viewings_[i].is_used) {
					logging_stream_command(i);
				}

			} else if (cs.get_status() == 1 || cs.get_status() == 2) {
				// not alive
				// ffmpeg が落ちしまったので stream側も強制的に止めます
				// ######################################### //
				stream_handler_funcs::get_instance(i)->set_next_progress(arec::stream_handler::handler::progress::post_process_forcibly);
				// ######################################### //
			}
		}
	}
}

void manager::on_notice_by_stream_handler (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	arec::stream_handler::handler::notice_t _notice = *(reinterpret_cast<arec::stream_handler::handler::notice_t*>(iface->get_source().get_message().data()));

	switch (_notice.progress) {
	case arec::stream_handler::handler::progress::init:
		break;

	case arec::stream_handler::handler::progress::pre_process:
		_UTL_LOG_I ("arec::stream_handler::handler::progress::pre_process");
		break;

	case arec::stream_handler::handler::progress::processing:
		break;

	case arec::stream_handler::handler::progress::process_end_success:
		_UTL_LOG_I ("arec::stream_handler::handler::progress::process_end_success");
		break;

	case arec::stream_handler::handler::progress::process_end_error:
		_UTL_LOG_I ("arec::stream_handler::handler::progress::process_end_error");
		break;

	case arec::stream_handler::handler::progress::post_process:
	case arec::stream_handler::handler::progress::post_process_forcibly:
		_UTL_LOG_I ("arec::stream_handler::handler::progress::post_process");

		forker_[_notice.group_id].wait_child(SIGINT);
		forker_[_notice.group_id].destroy_pipes();

//TODO この位置でclearはどうなのか  tuner notifyで終了したらclearの方がいいかも
		viewings_[_notice.group_id].clear ();

		{
			std::string stream_path = arec::settings::get_instance()->get_params().get_viewing_stream_data_path();
			std::string path = stream_path + "/";
			path += std::to_string(_notice.group_id);
			clean_dir(path.c_str());
		}

		_UTL_LOG_I ("viewing end...");


		//-----------------------------//
		{
			// 選局を停止・クローズしときます(投げっぱなし)
			uint8_t gr = _notice.group_id;
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop),
							(uint8_t*)&gr, sizeof(gr));
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::close),
							(uint8_t*)&gr, sizeof(gr));
		}
		//-----------------------------//

		break;

	default:
		break;
	};

	iface->reply (coseq::result::success);
}

void manager::on_start_viewing (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	iface->lock();
	_UTL_LOG_I ("(%s) entry", iface->get_sequence_name());

	// 実行専用スクラッチはローカルへ(service_info/present_event_info は psisi へポインタ渡し)
	psisi_structs::service_info_t service_info;
	memset (&service_info, 0x00, sizeof(service_info));
	psisi_structs::event_info_t present_event_info;
	uint8_t group_id = 0xff;
	uint16_t ch = 0;

	// 失敗時後始末(v1 SECTID_END_ERROR 相当)
	auto do_error = [&]() {
		if (group_id < arec::group::GROUP_MAX) {
			viewings_[group_id].clear();
			// 選局を停止・クローズ(投げっぱなし)
			uint8_t gr = group_id;
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop), (uint8_t*)&gr, sizeof(gr));
			iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
							static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::close), (uint8_t*)&gr, sizeof(gr));
		}
		iface->reply (coseq::result::error);
		iface->unlock();
	};

	uint8_t seq_idx = iface->get_source().get_sequence_idx();
	if (seq_idx == static_cast<uint8_t>(client_iface::sequence::start_viewing_by_physical_channel)) {
		client_iface::physical_channel_param_t pcp = *(reinterpret_cast<client_iface::physical_channel_param_t*>(iface->get_source().get_message().data()));
		_UTL_LOG_I("phy_ch: %d", pcp.physical_channel);
		_UTL_LOG_I("svc_idx: %d", pcp.service_idx);
		ch = pcp.physical_channel;

		// get service_id by physical channel
		arec::channel::client_iface::request_service_id_param_t param = { pcp.physical_channel, pcp.service_idx };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::channel_manager),
			static_cast<uint8_t>(arec::channel::client_iface::sequence::get_service_id_by_physical_channel),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_get_service_id_by_physical_channel is failure.");
			do_error();
			return;
		}
		arec::channel::client_iface::service_id_param_t *sp = reinterpret_cast<arec::channel::client_iface::service_id_param_t*>(r.get_message().data());
		service_info.transport_stream_id = sp->transport_stream_id;
		service_info.original_network_id = sp->original_network_id;
		service_info.service_id = sp->service_id;

	} else if (seq_idx == static_cast<uint8_t>(client_iface::sequence::start_viewing_by_service_id)) {
		client_iface::service_id_param_t sip = *(reinterpret_cast<client_iface::service_id_param_t*>(iface->get_source().get_message().data()));
		_UTL_LOG_I("ts_id: %d", sip.transport_stream_id);
		_UTL_LOG_I("org_nid: %d", sip.original_network_id);
		_UTL_LOG_I("svc_id: %d", sip.service_id);

		service_info.transport_stream_id = sip.transport_stream_id;
		service_info.original_network_id = sip.original_network_id;
		service_info.service_id = sip.service_id;

		// get physical channel by service_id
		arec::channel::client_iface::service_id_param_t param = {
			service_info.transport_stream_id, service_info.original_network_id, service_info.service_id
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
		ch = *(reinterpret_cast<uint16_t*>(r.get_message().data()));

	} else {
		_UTL_LOG_E ("unexpected seq idx [%d]", seq_idx);
		do_error();
		return;
	}

	// open tuner
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_service),
			static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::open));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_open is failure.");
			do_error();
			return;
		}
		group_id = *(reinterpret_cast<uint8_t*>(r.get_message().data()));
		_UTL_LOG_I ("request_open group_id:[0x%02x]", group_id);

		if (viewings_[group_id].is_used) {
			_UTL_LOG_E ("??? viewings_[group_id].is_used ???  group_id:[0x%02x]", group_id);
			viewings_[group_id].clear();
		}
	}

	// tune (with retry)
	{
		arec::tuner_service::client_iface::tune_param_t param = { ch, group_id };
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_service),
			static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_with_retry),
			(uint8_t*)&param, sizeof(param));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_tune is failure. -> not start viewing");
			do_error();
			return;
		}
	}

	// get present event info (たまに失敗するので 200ms 待ちで最大10回リトライ)
	{
		bool got = false;
		for (int retry = 0; retry <= 10; ++ retry) {
			psisi_structs::request_event_info_param_t eparam = { service_info, &present_event_info };
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
			do_error();
			return;
		}
	}

	// start viewing
	{
		char *svc_name = nullptr;
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
			// この後の viewings_[group_id].set 内で実態コピーする前提です
			svc_name = reinterpret_cast<char*>(r.get_message().data());
		}
		viewings_[group_id].set (
			service_info.transport_stream_id,
			service_info.original_network_id,
			service_info.service_id,
			present_event_info.event_id,
			&present_event_info.start_time,
			&present_event_info.end_time,
			present_event_info.event_name_char,
			svc_name,
			group_id
		);

		// 視聴開始 (streamhandler::progress で判断)
		if (stream_handler_funcs::get_instance(group_id)->get_current_progress() != arec::stream_handler::handler::progress::init) {
			_UTL_LOG_E ("progress != init ???  -> not start viewing");
			do_error();
			return;
		}

		if (!forker_[group_id].create_pipes()) {
			_UTL_LOG_E("create_pipes failure.");
			forker_[group_id].destroy_pipes();
			do_error();
			return;
		}

		std::string stream_path = arec::settings::get_instance()->get_params().get_viewing_stream_data_path();
		std::string fmt = arec::settings::get_instance()->get_params().get_viewing_stream_command_format();
		char command[1024] = {0};
		std::string gr = std::to_string(group_id);
		snprintf(command, sizeof(command), fmt.c_str(), stream_path.c_str(), gr.c_str(), stream_path.c_str(), gr.c_str());
		std::string _c = command;
		if (!forker_[group_id].do_fork(std::move(_c))) {
			_UTL_LOG_E("do_fork failure.");
			forker_[group_id].destroy_pipes();
			do_error();
			return;
		}

		int fd = forker_[group_id].get_child_stdin_fd();
//		set_nonblocking_io(fd);
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
		stream_handler_funcs::get_instance(group_id)->set_service_id(viewings_[group_id].service_id);
		stream_handler_funcs::get_instance(group_id)->set_use_splitter(arec::settings::get_instance()->get_params().is_viewing_use_splitter());
		stream_handler_funcs::get_instance(group_id)->set_process_handler(std::move(handler));
		stream_handler_funcs::get_instance(group_id)->set_next_progress(arec::stream_handler::handler::progress::pre_process);
		// ######################################### //
	}

	// END SUCCESS
	iface->reply (coseq::result::success, reinterpret_cast<uint8_t*>(&group_id), sizeof(group_id));
	iface->unlock();
}

void manager::on_stop_viewing (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	int group_id = *(reinterpret_cast<uint8_t*>(iface->get_source().get_message().data()));
	if (group_id >= tuner_resource_max_) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", group_id);
		iface->reply (coseq::result::error);

	} else {
		if (viewings_[group_id].is_used) {

			_UTL_LOG_I ("progress = end_success");


			// ######################################### //
			stream_handler_funcs::get_instance(group_id)->set_next_progress(arec::stream_handler::handler::progress::process_end_success);
			// ######################################### //


			iface->reply (coseq::result::success);

		} else {
			_UTL_LOG_E ("not viewing now...");
			iface->reply (coseq::result::error);
		}
	}

}

void manager::on_get_viewing (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());




	iface->reply (coseq::result::success);

}

void manager::on_set_option (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	client_iface::option_t opt = *(reinterpret_cast<client_iface::option_t*>(iface->get_source().get_message().data()));
	if (opt.group_id >= tuner_resource_max_) {
		_UTL_LOG_E ("invalid group_id:[0x%02x]", opt.group_id);
		iface->reply (coseq::result::error);

	} else {
		if (viewings_[opt.group_id].is_used) {
			if (opt.auto_stop_grace_period_sec > 0) {
				arec::common::etime auto_stop_target_time;
				auto_stop_target_time.set_current_time();
				auto_stop_target_time.add_sec(opt.auto_stop_grace_period_sec);

				viewings_[opt.group_id].auto_stop = true;
				viewings_[opt.group_id].auto_stop_time = auto_stop_target_time;
				_UTL_LOG_I("set auto_stop_time:[%s] group:[%d]", viewings_[opt.group_id].auto_stop_time.to_string(), viewings_[opt.group_id].group_id);
			}
		}

		iface->reply (coseq::result::success);
	}

}

void manager::on_dump_viewing (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	dump_viewing();


	iface->reply (coseq::result::success);

}

void manager::on_event_changed (coseq::iface *iface)
{
	_UTL_LOG_I ("(%s) entry", iface->get_sequence_name());

	// この seq は on_receive_notify から投げっぱなしで起動される(reply しない)
	uint8_t group_id = *(reinterpret_cast<uint8_t*>(iface->get_source().get_message().data()));

	if (!viewings_[group_id].is_used) {
		return;
	}

	psisi_structs::service_info_t info;
	info.transport_stream_id = viewings_->transport_stream_id;
	info.original_network_id = viewings_->original_network_id;
	info.service_id = viewings_->service_id;

	psisi_structs::event_info_t present_event_info;
	psisi_structs::request_event_info_param_t eparam = { info, &present_event_info };
	coseq::source &r = iface->request (
		static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
		static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_present_event_info),
		(uint8_t*)&eparam, sizeof(eparam));

	if (r.get_result() == coseq::result::success) {
		if (viewings_[group_id].is_used) {
			viewings_[group_id].event_id = present_event_info.event_id;
			viewings_[group_id].start_time = present_event_info.start_time;
			viewings_[group_id].end_time = present_event_info.end_time;
			viewings_[group_id].title_name = present_event_info.event_name_char;
			viewings_[group_id].dump();
		}
	}
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

				// PAT途絶したら視聴中止します
				if (viewings_[_gr].is_used) {

					// ######################################### //
					stream_handler_funcs::get_instance(_gr)->set_next_progress(arec::stream_handler::handler::progress::post_process_forcibly);
					// ######################################### //

					// 自ら呼び出します(内部の自リクエストは coseq では send=fire-and-forget)
					// PAT途絶 -> ts_receive_handlerは動いていない前提
					stream_handler_funcs::get_instance(_gr)->on_ts_received (nullptr, 0);
				}
			}

		} else if (iface->get_source().get_client_id() == event_change_notify_client_id_[_gr]) {

			psisi_structs::notify_event_info_t _info = *(psisi_structs::notify_event_info_t*)(iface->get_source().get_message().data());
			_UTL_LOG_I ("!!! event changed !!! group_id:[%d]", _gr);
			_info.dump ();

			// 投げっぱなしで event_changed を自リクエスト
			uint8_t gr = static_cast<uint8_t>(_gr);
			iface->send (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(client_iface::sequence::event_changed),
				reinterpret_cast<uint8_t*>(&gr),
				sizeof(gr)
			);
		}
	}
}

void manager::dump_viewing (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < arec::group::GROUP_MAX; ++ i) {
		if (viewings_[i].is_used) {
			viewings_[i].dump();
		}
		if (forker_[i].has_child()) {
			_UTL_LOG_I("ffmpeg:[%s]", forker_[i].get_child_commandline().c_str());
			_UTL_LOG_I("pid:[%d]", forker_[i].get_child_pid());
		}
	}
}

bool manager::set_nonblocking_io (int fd) const
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

void manager::clean_dir (const char* path) const
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

	if (dp != NULL) {
		closedir (dp);
		dp = NULL;
	}
}

//TODO ffmpegのログ出力
void manager::logging_stream_command (uint8_t group_id) const
{
	fd_set _fds;
	struct timeval _timeout;
	FD_ZERO (&_fds);
	FD_SET (forker_[group_id].get_child_stderr_fd(), &_fds);
	_timeout.tv_sec = 1;
	_timeout.tv_usec = 0;
	int sr = select (forker_[group_id].get_child_stderr_fd() +1, &_fds, NULL, NULL, &_timeout);
	if (sr < 0) {
		;;
	} else if (sr == 0) {
		// timeout
		;;
	} else {
		char buff [1024*4] = {0};
		size_t r = read (forker_[group_id].get_child_stderr_fd(), buff, sizeof(buff));
		if (r > 0) {
			_UTL_LOG_I(buff);
		}
	}
}


} // namespace viewing
} // namespace arec
