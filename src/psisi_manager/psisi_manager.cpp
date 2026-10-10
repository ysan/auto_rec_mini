#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include<iostream>
#include<fstream>
#include<sstream>

#include "psisi_manager.h"
#include "psisi_manager_if.h"
#include "psisi_manager_structs_addition.h"

#include "tuner_control_if.h"
#include "modules.h"
#include "forker.h"

#include "aribstr.h"
#include "pngcrc.h"


namespace arec {
namespace psisi {


typedef struct _parser_notice {
	_parser_notice (client_iface::psisi_type type, parser::TS_HEADER* p_ts_header, uint8_t* p_payload, size_t payload_size) {
		this->type = type;
		this->payload_size = payload_size;
		this->ts_header = *p_ts_header;
		memcpy (this->payload, p_payload, payload_size);
	};

	client_iface::psisi_type type;
	parser::TS_HEADER ts_header;
	uint8_t payload [TS_PACKET_LEN];
	size_t payload_size;
} _parser_notice_t;


manager::manager (std::string name, uint8_t que_max, uint8_t group_id)
	:coseq::module_base (name, que_max)
	,arec::group (group_id)
	,ts_handler_ (this)
	,parser_ (this)
	,tuner_notify_client_id_ (0xff)
	,ts_receive_handler_id_ (-1)
	,tuner_is_tuned_ (false)
	,is_detected_PAT_ (false)
	,state_ (client_iface::psisi_state::not_ready)
	,mPAT (16)
	,mEIT_H (4096*100, 100)
	,mCDT ((size_t)4096*100)
	,mEIT_H_sched (4096*100, 100, this)
	,is_enable_EIT_sched_ (false)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::move("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::move("on_module_down")},
		{[&](coseq::iface *iface){manager::on_get_state(iface);}, std::move("on_get_state")},
		{[&](coseq::iface *iface){manager::on_check_loop(iface);}, std::move("on_check_loop")},
		{[&](coseq::iface *iface){manager::on_parser_notice(iface);}, std::move("on_parser_notice")},
		{[&](coseq::iface *iface){manager::on_stabilization_after_tuning(iface);}, std::move("on_stabilization_after_tuning")},
		{[&](coseq::iface *iface){manager::on_register_pat_detect_notify(iface);}, std::move("on_register_pat_detect_notify")},
		{[&](coseq::iface *iface){manager::on_unregister_pat_detect_notify(iface);}, std::move("on_unregister_pat_detect_notify")},
		{[&](coseq::iface *iface){manager::on_register_event_change_notify(iface);}, std::move("on_register_event_change_notify")},
		{[&](coseq::iface *iface){manager::on_unregister_event_change_notify(iface);}, std::move("on_unregister_event_change_notify")},
		{[&](coseq::iface *iface){manager::on_register_psisi_state_notify(iface);}, std::move("on_register_psisi_state_notify")},
		{[&](coseq::iface *iface){manager::on_unregister_psisi_state_notify(iface);}, std::move("on_unregister_psisi_state_notify")},
		{[&](coseq::iface *iface){manager::on_get_pat_detect_state(iface);}, std::move("on_get_pat_detect_state")},
		{[&](coseq::iface *iface){manager::on_get_stream_infos(iface);}, std::move("on_get_stream_infos")},
		{[&](coseq::iface *iface){manager::on_get_current_service_infos(iface);}, std::move("on_get_current_service_infos")},
		{[&](coseq::iface *iface){manager::on_get_present_event_info(iface);}, std::move("on_get_present_event_info")},
		{[&](coseq::iface *iface){manager::on_get_follow_event_info(iface);}, std::move("on_get_follow_event_info")},
		{[&](coseq::iface *iface){manager::on_get_current_network_info(iface);}, std::move("on_get_current_network_info")},
		{[&](coseq::iface *iface){manager::on_enable_parse_eit_sched(iface);}, std::move("on_enable_parse_eit_sched")},
		{[&](coseq::iface *iface){manager::on_disable_parse_eit_sched(iface);}, std::move("on_disable_parse_eit_sched")},
		{[&](coseq::iface *iface){manager::on_dump_caches(iface);}, std::move("on_dump_caches")},
		{[&](coseq::iface *iface){manager::on_dump_tables(iface);}, std::move("on_dump_tables")},
	};
	set_sequences(seqs, _max);


	settings_ = arec::settings::get_instance();

	// references
//	mPAT_ref = mPAT.reference();
//	mEIT_H_ref = mEIT_H.reference();
//	mNIT_ref =  mNIT.reference();
//	mSDT_ref = mSDT.reference();


	pat_recv_time_.clear();

	clear_program_infos ();
	clear_service_infos ();
	clear_event_pf_infos ();
	clear_network_info ();

	EIT_H_comp_flag_.clear();
	NIT_comp_flag_.clear();
	SDT_comp_flag_.clear();
}

manager::~manager (void)
{
	pat_recv_time_.clear();

	clear_program_infos ();
	clear_service_infos ();
	clear_event_pf_infos ();
	clear_network_info ();

	EIT_H_comp_flag_.clear();
	NIT_comp_flag_.clear();
	SDT_comp_flag_.clear();

	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// (v1 の on_create 相当) module スレッド上で SCHED_FIFO に優先度を上げる
	// 確認コマンド: $ ps -em -o pid,tid,policy,pri,ni,rtprio,comm
	{
		int policy = SCHED_FIFO;
		struct sched_param param;
		param.sched_priority = 80;
		if (pthread_setschedparam(pthread_self(), policy, &param) != 0) {
			_UTL_PERROR ("pthread_setschedparam");
		}
	}

	// register tuner notify
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + get_group_id(),
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::reg_tuner_notify));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_tuner_notify is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		tuner_notify_client_id_ = *(uint8_t*)(r.get_message().data());
	}

	// register ts receive handler
	{
		_UTL_LOG_I ("arec::tuner_control::client_iface::ITsReceiveHandler %p", ts_handler_);
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_control) + get_group_id(),
			static_cast<uint8_t>(arec::tuner_control::client_iface::sequence::reg_ts_receive_handler),
			(uint8_t*)&ts_handler_, sizeof(ts_handler_));
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("request_register_ts_receive_handler is failure.");
			iface->reply (coseq::result::error);
			return;
		}
		ts_receive_handler_id_ = *(int*)(r.get_message().data());
	}

	// 監視ループ起動(check_loop は先に reply してループに入る)
	iface->request (static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
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

void manager::on_get_state (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	iface->reply (coseq::result::success, (uint8_t*)&state_, sizeof(state_));


}

void manager::on_check_loop (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 先にreplyしておく(以降はスレッド占有の監視ループ)
	iface->reply (coseq::result::success);

	// 監視ループ(app 終了要求で抜ける)
	while (!modules::is_shutdown_requested()) {

		// --- PAT 途絶チェック ---
		iface->wait_timeout (1000); // 1sec

		if (tuner_is_tuned_) {
			// 最後にPATを受けとった時刻から再び受け取るまで30秒以上経過していたら異常
			arec::common::etime tcur;
			tcur.set_current_time();
			arec::common::etime ttmp = pat_recv_time_;
			ttmp.add_sec (30);
			if (tcur > ttmp) {
				_UTL_LOG_E ("PAT was not detected. probably stream is broken...");

				if (is_detected_PAT_) {
					// true -> false  fire notify
					client_iface::pat_detection_state _state = client_iface::pat_detection_state::not_detected;
					iface->notify (NOTIFY_CAT__PAT_DETECT, (uint8_t*)&_state, sizeof(client_iface::pat_detection_state));
				}

				is_detected_PAT_ = false;

			} else {

				if (!is_detected_PAT_) {
					// false -> true  fire notify
					client_iface::pat_detection_state _state = client_iface::pat_detection_state::detected;
					iface->notify (NOTIFY_CAT__PAT_DETECT, (uint8_t*)&_state, sizeof(client_iface::pat_detection_state));
				}

				is_detected_PAT_ = true;
			}
		}

		// --- follow event チェック ---
		iface->wait_timeout (1000); // 1sec

		assign_follow_event_to_service_infos ();
		check_follow_event_at_service_infos (iface);
	}
}

void manager::on_parser_notice (coseq::iface *iface)
{
	parser::EN_CHECK_SECTION r = parser::EN_CHECK_SECTION__COMPLETED;

	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	mPAT.check_async_delete ();
	mEIT_H.check_async_delete ();
	mEIT_H_sched.check_async_delete ();
	mNIT.check_async_delete ();
	mSDT.check_async_delete ();
	mCAT.check_async_delete ();
	mCDT.check_async_delete ();


	_parser_notice_t *p_notice = (_parser_notice_t*)(iface->get_source().get_message().data());
	switch (p_notice->type) {
		case client_iface::psisi_type::PAT:

		r = mPAT.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED ) {
			_UTL_LOG_I ("notice new PAT");
			clear_program_infos();
			cache_program_infos();

			// 新しいPATが取れたら PMT parserを準備します

			// PATは一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
			std::vector<parser::program_association_table::table*>::const_iterator iter = mPAT.get_tables ()->end();
			parser::program_association_table::table* latest = *(-- iter);

			// 一度クリアします
			program_map_.clear();

			for (const auto& program : latest->programs) {
				if (program.program_number == 0) {
					continue;
				}
				program_map_.add(program.program_map_PID);
			}
		}

		// PAT途絶チェック用
		pat_recv_time_.set_current_time ();

		break;

	case client_iface::psisi_type::PMT: {
		r = program_map_.parse(&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new PMT");
			clear_program_infos();
			cache_program_infos();
		}

		}
		break;

	case client_iface::psisi_type::CAT:

		r = mCAT.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new CAT");
//TODO
		}

		break;

	case client_iface::psisi_type::CDT:

		r = mCDT.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new CDT");

			store_logo();
		}

		break;

	case client_iface::psisi_type::NIT:

		r = mNIT.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			NIT_comp_flag_.check_update (true);

			_UTL_LOG_I ("notice new NIT");

			// ここにくるってことは選局したとゆうこと
			clear_network_info();
			cache_network_info();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			NIT_comp_flag_.check_update (false);
		}

		break;

	case client_iface::psisi_type::SDT:

		r = mSDT.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			SDT_comp_flag_.check_update (true);

			_UTL_LOG_I ("notice new SDT");

			// ここにくるってことは選局したとゆうこと
			clear_service_infos (true);
			cache_service_infos (true);

			// event_info もcacheし直し
			clear_event_pf_infos();
			cache_event_pf_infos();
			check_event_pf_infos();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			SDT_comp_flag_.check_update (false);
		}

		break;

	case client_iface::psisi_type::EIT_H_PF:

		r = mEIT_H.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == parser::EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			EIT_H_comp_flag_.check_update (true);

			_UTL_LOG_I ("notice new EIT p/f");
			clear_event_pf_infos();
			cache_event_pf_infos ();
			check_event_pf_infos();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			EIT_H_comp_flag_.check_update (false);
		}


		{
			// checkSection 内の onScheduleUpdate は coseq では send(fire-and-forget) になるため
			// request option 操作は不要
#ifdef _DUMMY_TUNER // デバッグ中は is_enable_EIT_sched_ 関係なしに
			mEIT_H_sched.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
#else
			if (is_enable_EIT_sched_) {
				mEIT_H_sched.check_section (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
			}
#endif
		}

		break;

	default:
		break;
	}

	iface->reply (coseq::result::success);

}

void manager::on_stabilization_after_tuning (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	// 先にreplyしておく
	iface->reply (coseq::result::success);

	// clear
	iface->wait_timeout (200);

	mPAT.clear();
	mEIT_H.clear();
	mNIT.clear();
	mSDT.clear();
	mCAT.clear();
	mCDT.clear();
//	mRST.clear();
//	mBIT.clear();

	EIT_H_comp_flag_.clear();
	NIT_comp_flag_.clear();
	SDT_comp_flag_.clear();

	// EIT,NIT,SDT のparserが完了したか確認(100ms周期, 上限30秒)
	// 少なくともこの3つが完了していれば選局完了(psisi_state::ready)とする
	//
	// ※v1 の enable_overwrite workaround は coseq では不要。
	//   原因だった「set_timeout の返しキュー溢れ」は wait_timeout では起きない。
	//   未完了のまま打ち切った場合は tuner_service 側の tune タイムアウト→リトライで
	//   再度 tuning_success → 本シーケンスが起動する。
	for (int i = 0; i < 300 && !modules::is_shutdown_requested(); ++ i) {

		iface->wait_timeout (100);

		if (
			EIT_H_comp_flag_.is_completed() &&
			NIT_comp_flag_.is_completed() &&
			SDT_comp_flag_.is_completed()
		) {
			_UTL_LOG_I ("PSI/SI ready.");

			tuner_is_tuned_ = true;

			// fire notify
			state_ = client_iface::psisi_state::ready;
			iface->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&state_, sizeof(state_));

			break;
		}
	}
}

void manager::on_register_pat_detect_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	uint8_t client_id = 0;
	coseq::result rslt;
	bool r = iface->reg_notify (NOTIFY_CAT__PAT_DETECT, &client_id);
	if (r) {
		rslt = coseq::result::success;
	} else {
		rslt = coseq::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	iface->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


}

void manager::on_unregister_pat_detect_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	coseq::result rslt;
	// msgからclient_idを取得
	uint8_t client_id = *(iface->get_source().get_message().data());
	bool r = iface->unreg_notify (NOTIFY_CAT__PAT_DETECT, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::error;
	}

	iface->reply (rslt);


}

void manager::on_register_event_change_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	uint8_t client_id = 0;
	coseq::result rslt;
	bool r = iface->reg_notify (NOTIFY_CAT__EVENT_CHANGE, &client_id);
	if (r) {
		rslt = coseq::result::success;
	} else {
		rslt = coseq::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	iface->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


}

void manager::on_unregister_event_change_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	coseq::result rslt;
	// msgからclient_idを取得
	uint8_t client_id = *(iface->get_source().get_message().data());
	bool r = iface->unreg_notify (NOTIFY_CAT__EVENT_CHANGE, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::error;
	}

	iface->reply (rslt);


}

void manager::on_register_psisi_state_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	uint8_t client_id = 0;
	coseq::result rslt;
	bool r = iface->reg_notify (NOTIFY_CAT__PSISI_STATE, &client_id);
	if (r) {
		rslt = coseq::result::success;
	} else {
		rslt = coseq::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	iface->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


}

void manager::on_unregister_psisi_state_notify (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	coseq::result rslt;
	// msgからclient_idを取得
	uint8_t client_id = *(iface->get_source().get_message().data());
	bool r = iface->unreg_notify (NOTIFY_CAT__PSISI_STATE, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = coseq::result::error;
	}

	iface->reply (rslt);


}

void manager::on_get_pat_detect_state (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::pat_detection_state state =
			is_detected_PAT_ ? client_iface::pat_detection_state::detected: client_iface::pat_detection_state::not_detected;

	iface->reply (coseq::result::success, (uint8_t*)&state, sizeof(state));


}

void manager::on_get_stream_infos (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	psisi_structs::request_stream_infos_param_t param = *(psisi_structs::request_stream_infos_param_t*)(iface->get_source().get_message().data());
	if (!param.p_out_stream_infos || param.array_max_num == 0) {
		_UTL_LOG_E ("psisi_structs::request_stream_infos_param_t is invalid.\n");
		iface->reply (coseq::result::error);

	} else {
		int get_num = get_stream_infos (param.program_number, param.type, param.p_out_stream_infos, param.array_max_num);

		// reply msgで格納数を渡します
		iface->reply (coseq::result::success, (uint8_t*)&get_num, sizeof(get_num));
	}


}

void manager::on_get_current_service_infos (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	psisi_structs::request_service_infos_param_t param = *(psisi_structs::request_service_infos_param_t*)(iface->get_source().get_message().data());
	if (!param.p_out_service_infos || param.array_max_num == 0) {
		_UTL_LOG_E ("psisi_structs::request_service_infos_param_t is invalid.\n");
		iface->reply (coseq::result::error);

	} else {
		int get_num = get_current_service_infos (param.p_out_service_infos, param.array_max_num);

		// reply msgで格納数を渡します
		iface->reply (coseq::result::success, (uint8_t*)&get_num, sizeof(get_num));
	}


}

void manager::on_get_present_event_info (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	coseq::result rslt;

	psisi_structs::request_event_info_param_t param = *(psisi_structs::request_event_info_param_t*)(iface->get_source().get_message().data());
	if (!param.p_out_event_info) {
		_UTL_LOG_E ("psisi_structs::request_event_info_param_t is invalid.\n");
		rslt = coseq::result::error;

	} else {
		_event_pf_info_t *p_info = find_event_pf_info (
										TBLID_EIT_PF_A,
										param.key.transport_stream_id,
										param.key.original_network_id,
										param.key.service_id,
										event_pf_state::present
									);
		if (p_info) {
			param.p_out_event_info->table_id = p_info->table_id;
			param.p_out_event_info->transport_stream_id = p_info->transport_stream_id;
			param.p_out_event_info->original_network_id = p_info->original_network_id;
			param.p_out_event_info->service_id = p_info->service_id;
			param.p_out_event_info->event_id = p_info->event_id;
			param.p_out_event_info->start_time = p_info->start_time;
			param.p_out_event_info->end_time = p_info->end_time;
			strncpy (
				param.p_out_event_info->event_name_char,
				p_info->event_name_char,
				strlen(p_info->event_name_char)
			);

			rslt = coseq::result::success;

		} else {
			_UTL_LOG_E ("not found PresentEventInfo");
			rslt = coseq::result::error;
mEIT_H.dumpTables_simple();
mEIT_H.dump_section_list ();
		}
	}

	iface->reply (rslt);


}

void manager::on_get_follow_event_info (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	coseq::result rslt;

	psisi_structs::request_event_info_param_t param = *(psisi_structs::request_event_info_param_t*)(iface->get_source().get_message().data());
	if (!param.p_out_event_info) {
		_UTL_LOG_E ("psisi_structs::request_event_info_param_t is invalid.\n");
		rslt = coseq::result::error;

	} else {
		_event_pf_info_t *p_info = find_event_pf_info (
										TBLID_EIT_PF_A,
										param.key.transport_stream_id,
										param.key.original_network_id,
										param.key.service_id,
										event_pf_state::follow
									);
		if (p_info) {
			param.p_out_event_info->table_id = p_info->table_id;
			param.p_out_event_info->transport_stream_id = p_info->transport_stream_id;
			param.p_out_event_info->original_network_id = p_info->original_network_id;
			param.p_out_event_info->service_id = p_info->service_id;
			param.p_out_event_info->event_id = p_info->event_id;
			param.p_out_event_info->start_time = p_info->start_time;
			param.p_out_event_info->end_time = p_info->end_time;
			strncpy (
				param.p_out_event_info->event_name_char,
				p_info->event_name_char,
				strlen(p_info->event_name_char)
			);

			rslt = coseq::result::success;

		} else {
			_UTL_LOG_E ("not found FollowEventInfo");
			rslt = coseq::result::error;
		}
	}

	iface->reply (rslt);


}

void manager::on_get_current_network_info (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	coseq::result rslt;

	psisi_structs::request_network_info_param_t param = *(psisi_structs::request_network_info_param_t*)(iface->get_source().get_message().data());
	if (!param.p_out_network_info) {
		_UTL_LOG_E ("psisi_structs::request_network_info_param_t is invalid.\n");
		rslt = coseq::result::error;

	} else {
		if (network_info_.is_used) {
			param.p_out_network_info->table_id = network_info_.table_id;
			param.p_out_network_info->network_id = network_info_.network_id;

			strncpy (
				param.p_out_network_info->network_name_char,
				network_info_.network_name_char,
				strlen(network_info_.network_name_char)
			);

			param.p_out_network_info->transport_stream_id = network_info_.transport_stream_id;
			param.p_out_network_info->original_network_id = network_info_.original_network_id;

			for (int i = 0; i < network_info_.services_num; ++ i) {
				param.p_out_network_info->services[i].service_id = network_info_.services[i].service_id;
				param.p_out_network_info->services[i].service_type = network_info_.services[i].service_type;
			}
			param.p_out_network_info->services_num = network_info_.services_num;

			param.p_out_network_info->area_code = network_info_.area_code;
			param.p_out_network_info->guard_interval = network_info_.guard_interval;
			param.p_out_network_info->transmission_mode = network_info_.transmission_mode;
			param.p_out_network_info->remote_control_key_id = network_info_.remote_control_key_id;
			strncpy (
				param.p_out_network_info->ts_name_char,
				network_info_.ts_name_char,
				strlen(network_info_.ts_name_char)
			);

			rslt = coseq::result::success;

		} else {
			_UTL_LOG_E ("network_info_ is not availale...");
			rslt = coseq::result::error;
		}
	}

	iface->reply (rslt);


}

void manager::on_enable_parse_eit_sched (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	is_enable_EIT_sched_ = true;


	// reply msgにparserのアドレスを乗せます
	enable_parse_eit_sched_reply_param_t param = {&mEIT_H_sched};


	iface->reply (coseq::result::success, (uint8_t*)&param, sizeof(param));

}

void manager::on_disable_parse_eit_sched (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	is_enable_EIT_sched_ = false;


	iface->reply (coseq::result::success);

}

void manager::on_dump_caches (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	int type = *(int*)(iface->get_source().get_message().data());
	switch (type) {
	case 0:
		dump_program_infos();
		break;

	case 1:
		dump_service_infos();
		break;

	case 2:
		dump_event_pf_infos();
		break;

	case 3:
		dump_network_info();
		break;

	default:
		break;
	}


	iface->reply (coseq::result::success);

}

void manager::on_dump_tables (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::psisi_type type = *(client_iface::psisi_type*)(iface->get_source().get_message().data());
	switch (type) {
	case client_iface::psisi_type::PAT:
		mPAT.dump_tables ();
		break;

	case client_iface::psisi_type::PMT:
		program_map_.dump();
		break;

	case client_iface::psisi_type::EIT_H_PF:
		mEIT_H.dump_tables ();
		break;

	case client_iface::psisi_type::EIT_H_PF_simple:
		mEIT_H.dumpTables_simple();
		break;

	case client_iface::psisi_type::EIT_H_SCHED:
		mEIT_H_sched.dump_tables ();
		break;

	case client_iface::psisi_type::EIT_H_SCHED_event:
		mEIT_H_sched.dumpTables_event();
		break;

	case client_iface::psisi_type::EIT_H_SCHED_simple:
		mEIT_H_sched.dumpTables_simple();
		break;

	case client_iface::psisi_type::NIT:
		mNIT.dump_tables ();
		break;

	case client_iface::psisi_type::SDT:
		mSDT.dump_tables ();
		break;

	case client_iface::psisi_type::CAT:
		mCAT.dump_tables ();
		break;

	case client_iface::psisi_type::CDT:
		mCDT.dump_tables ();
		break;

//	case client_iface::psisi_type::RST:
//		mRST.dump_tables();
//		break;

//	case client_iface::psisi_type::BIT:
//		mBIT.dump_tables();
//		break;

	default:
		break;
	}


	iface->reply (coseq::result::success);

}

void manager::on_receive_notify (coseq::iface *iface)
{
	if (!(iface->get_source().get_client_id() == tuner_notify_client_id_)) {
		return ;
	}

	arec::tuner_control::client_iface::tuner_state state = *(arec::tuner_control::client_iface::tuner_state*)(iface->get_source().get_message().data());
	switch (state) {
	case arec::tuner_control::client_iface::tuner_state::tuning_begin: {
		_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tuning_begin");

		// fire notify
		state_ = client_iface::psisi_state::not_ready;
		iface->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&state_, sizeof(state_));

		tuner_is_tuned_ = false;

//#ifdef _DUMMY_TUNER 
//		// ダミーデバッグ中はここでクリア
//		mPAT.clear();
//		mEIT_H.clear();
//		mNIT.clear();
//		mSDT.clear();
//		mRST.clear();
//		mBIT.clear();
//
//		EIT_H_comp_flag_.clear();
//		NIT_comp_flag_.clear();
//		SDT_comp_flag_.clear();
//#endif

		}
		break;

	case arec::tuner_control::client_iface::tuner_state::tuning_success: {
		_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tuning_success");

//#ifndef _DUMMY_TUNER
		// 選局後の安定化(投げっぱなしで自リクエスト)
		iface->send (static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
						static_cast<uint8_t>(client_iface::sequence::stabilization_after_tuning));
//#endif

		}
		break;

	case arec::tuner_control::client_iface::tuner_state::tuning_error_stop: {
		_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tuning_error_stop");

		// fire notify
		state_ = client_iface::psisi_state::not_ready;
		iface->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&state_, sizeof(state_));


		// tune stopで一連の動作が終わった時の対策
		clear_program_infos();
		clear_network_info();
		clear_service_infos (true);
		clear_event_pf_infos();

		tuner_is_tuned_ = false;
		is_detected_PAT_ = false;

		}
		break;

	case arec::tuner_control::client_iface::tuner_state::tune_stop: {
		_UTL_LOG_I ("arec::tuner_control::client_iface::tuner_state::tune_stop");

		// fire notify
		state_ = client_iface::psisi_state::not_ready;
		iface->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&state_, sizeof(state_));


		// tune stopで一連の動作が終わった時の対策
		clear_program_infos();
		clear_network_info();
		clear_service_infos (true);
		clear_event_pf_infos();

		tuner_is_tuned_ = false;
		is_detected_PAT_ = false;

		}
		break;

	default:
		break;
	}
}

void manager::cache_program_infos (void)
{
	int n = 0;

	if (mPAT.get_tables ()->size() == 0) {
		return;
	}

	// PATは一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
	std::vector<parser::program_association_table::table*>::const_iterator iter = mPAT.get_tables ()->end();
	parser::program_association_table::table* latest = *(-- iter);
	if (!latest) {
		return;
	}
	parser::program_association_table::table *p_table = latest;

	uint8_t tbl_id = p_table->header.table_id;
	uint16_t ts_id = p_table->header.table_id_extension;

	std::vector<parser::program_association_table::table::program>::const_iterator iter_prog = p_table->programs.begin();
	for (; iter_prog != p_table->programs.end(); ++ iter_prog) {

		if (n >= PROGRAM_INFOS_MAX) {
			return ;
		}

		program_infos_[n].table_id = tbl_id;
		program_infos_[n].transport_stream_id = ts_id;

		program_infos_[n].program_number = iter_prog->program_number;
		program_infos_[n].program_map_PID = iter_prog->program_map_PID;

		program_infos_[n].p_org_table = p_table;
		program_infos_[n].is_used = true;

		// cache pmt streams
		std::shared_ptr<parser::program_map_table> pmt = program_map_.get(iter_prog->program_map_PID);
		if (pmt != nullptr && pmt->get_tables ()->size() > 0) {
			// PMTも一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
			std::vector<parser::program_map_table::table*>::const_iterator iter = pmt->get_tables ()->end();
			parser::program_map_table::table* latest = *(-- iter);
			if (latest) {
				parser::program_map_table::table *p_table = latest;

				for (const auto & stream : p_table->streams) {
					std::shared_ptr<_program_info_t::_stream>s = std::make_shared<_program_info_t::_stream>();
					s->type = stream.stream_type;
					s->pid = stream.elementary_PID;
					program_infos_[n].streams.push_back(s);
				}
			}
		}

		++ n;

	} // loop programs
}

void manager::dump_program_infos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		if (program_infos_ [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			program_infos_ [i].dump();
		}
	}
}

void manager::clear_program_infos (void)
{
	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		program_infos_ [i].clear();
	}
}

// streamInfo for request
int manager::get_stream_infos (uint16_t program_number, parser::EN_STREAM_TYPE type, psisi_structs::stream_info_t *p_out_stream_infos, int num)
{
	if (!p_out_stream_infos || num == 0) {
		return 0;
	}

	int r = 0;

	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		if (program_infos_[i].program_number == program_number) {
			for (const auto& stream : program_infos_[i].streams) {
				if (r >= num) {
					break;
				}
		
				if (stream->type == type) {
					(p_out_stream_infos + r)->type = stream->type;
					(p_out_stream_infos + r)->pid = stream->pid;
					++ r;
				}
			}

			break;
		}
	}
	
	return r;
}

/**
 * SDTテーブル内容を_SERVICE_INFOに保存します
 * 選局が発生しても継続して保持します
 *
 * 引数 is_at_tuning は選局契機で呼ばれたかどうか
 * SDTテーブルは選局ごとクリアしてる
 */
void manager::cache_service_infos (bool is_at_tuning)
{
	std::vector<parser::service_description_table::table*>::const_iterator iter = mSDT.get_tables ()->begin();
	for (; iter != mSDT.get_tables ()->end(); ++ iter) {

		parser::service_description_table::table *p_table = *iter;
		uint8_t tbl_id = p_table->header.table_id;
		uint16_t ts_id = p_table->header.table_id_extension;
		uint16_t network_id = p_table->original_network_id;

		// 自ストリームのみ
		if (tbl_id != TBLID_SDT_A) {
			continue;
		}

		std::vector<parser::service_description_table::table::service>::const_iterator iter_svc = p_table->services.begin();
		for (; iter_svc != p_table->services.end(); ++ iter_svc) {

			_service_info_t * p_info = find_service_info (tbl_id, ts_id, network_id, iter_svc->service_id);
			if (!p_info) {
				p_info = find_empty_service_info();
				if (!p_info) {
					_UTL_LOG_E ("cache_service_infos failure.");
					return;
				}
			}

			if (is_at_tuning) {
				p_info->is_tune_target = true;
			}
			p_info->table_id = tbl_id;
			p_info->transport_stream_id = ts_id;
			p_info->original_network_id = network_id;
			p_info->service_id = iter_svc->service_id;

			std::vector<parser::descriptor>::const_iterator iter_desc = iter_svc->descriptors.begin();
			for (; iter_desc != iter_svc->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SERVICE_DESCRIPTOR) {
					parser::service_descriptor sd (*iter_desc);
					if (!sd.is_valid) {
						_UTL_LOG_W ("invalid ServiceDescriptor");
						continue;
					}
					p_info->service_type = sd.service_type;

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					memset (p_info->service_name_char, 0x00, 64);
					AribToString (aribstr, (const char*)sd.service_name_char, (int)sd.service_name_length);
					strncpy (p_info->service_name_char, aribstr, 64);
				}

			} // loop descriptors

			p_info->p_org_table = p_table;
			p_info->is_used = true;
			p_info->last_update.set_current_time();

		} // loop services

	} // loop tables
}

_service_info_t* manager::find_service_info (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
)
{
	if (
		(_table_id == 0) &&
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return NULL;
	}

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (service_infos_ [i].is_used) {
			if (
				(_table_id == service_infos_ [i].table_id) &&
				(_transport_stream_id == service_infos_ [i].transport_stream_id) &&
				(_original_network_id == service_infos_ [i].original_network_id) &&
				(_service_id == service_infos_ [i].service_id)
			) {
				return &service_infos_ [i];
			}
		}
	}

	// not existed
	return NULL;
}

_service_info_t* manager::find_empty_service_info (void)
{
	int i = 0;
	for (i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (!service_infos_ [i].is_used) {
			break;
		}
	}

	if (i == SERVICE_INFOS_MAX) {
		_UTL_LOG_W ("service_infos_ full.");
		return NULL;
	}

	return &service_infos_ [i];
}

bool manager::is_exist_service_table (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
)
{
	if (
		(_table_id == 0) &&
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return false;
	}

	std::vector<parser::service_description_table::table*>::const_iterator iter = mSDT.get_tables ()->begin();
	for (; iter != mSDT.get_tables ()->end(); ++ iter) {

		parser::service_description_table::table *p_table = *iter;
		uint8_t tbl_id = p_table->header.table_id;
		uint16_t ts_id = p_table->header.table_id_extension;
		uint16_t network_id = p_table->original_network_id;

		std::vector<parser::service_description_table::table::service>::const_iterator iter_svc = p_table->services.begin();
		for (; iter_svc != p_table->services.end(); ++ iter_svc) {

			uint16_t service_id = iter_svc->service_id;

			if (
				(_table_id == tbl_id) &&
				(_transport_stream_id == ts_id) &&
				(_original_network_id == network_id) &&
				(_service_id == service_id)
			) {
				return true;
			}

		} // loop services

	} // loop tables


	// not existed
	return false;
}

void manager::assign_follow_event_to_service_infos (void)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (service_infos_ [i].is_tune_target) {
			if (!service_infos_ [i].event_follow_info.is_used) {

				_event_pf_info_t* p_info = find_event_pf_info (
												TBLID_EIT_PF_A,
												service_infos_ [i].transport_stream_id,
												service_infos_ [i].original_network_id,
												service_infos_ [i].service_id,
												event_pf_state::follow
											);
				if (p_info) {
//_UTL_LOG_I ("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
//p_info->dump();
//					service_infos_ [i].event_follow_info = *p_info;
//					memcpy (&service_infos_ [i].event_follow_info, p_info, sizeof(_EVENT_PF_INFO));
					service_infos_ [i].event_follow_info.clear();
					service_infos_ [i].event_follow_info.start_time = p_info->start_time;
					service_infos_ [i].event_follow_info.end_time = p_info->end_time;
					service_infos_ [i].event_follow_info.is_used = true;
					service_infos_ [i].last_update.set_current_time();
//_UTL_LOG_I ("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
//service_infos_ [i].event_follow_info.dump();
				}
			}
		}
	}
}

void manager::check_follow_event_at_service_infos (coseq::iface *iface)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (service_infos_ [i].is_tune_target) {
			if (service_infos_ [i].event_follow_info.is_used) {

				arec::common::etime cur_time;
				cur_time.set_current_time();

				if (service_infos_ [i].event_follow_info.start_time <= cur_time) {
					// 次のイベント開始時刻到来
					// event changed

					if (iface) {
						psisi_structs::notify_event_info_t _info;
						_info.table_id = service_infos_ [i].event_follow_info.table_id;
						_info.transport_stream_id = service_infos_ [i].event_follow_info.transport_stream_id;
						_info.original_network_id = service_infos_ [i].event_follow_info.original_network_id;
						_info.service_id = service_infos_ [i].event_follow_info.service_id;
						_info.event_id = service_infos_ [i].event_follow_info.event_id;

						// fire notify
						iface->notify (NOTIFY_CAT__EVENT_CHANGE, (uint8_t*)&_info, sizeof(psisi_structs::notify_event_info_t));
					}


					arec::common::etime t_tmp = service_infos_ [i].event_follow_info.start_time;
					t_tmp.add_sec(15);
					// 次イベント開始から15秒以上経過していたら
					// cache_event_pf_infos 実行する
					if (t_tmp < cur_time) {
						_UTL_LOG_E ("more than 15 seconds have passed since the start of the next event.");
						clear_event_pf_infos ();
						cache_event_pf_infos ();
						check_event_pf_infos ();
					}

					// 用済みなのでクリアします
					service_infos_ [i].event_follow_info.clear();

				}

			}
		}
	}
}

void manager::dump_service_infos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (service_infos_ [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			service_infos_ [i].dump();
		}
	}
}

void manager::clear_service_infos (void)
{
	clear_service_infos (false);
}

void manager::clear_service_infos (bool is_at_tuning)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (is_at_tuning) {
			service_infos_ [i].clear_at_tuning();

		} else {
			service_infos_ [i].clear();
		}
	}
}

// serviceInfo for request
int manager::get_current_service_infos (psisi_structs::service_info_t *p_out_service_infos, int num)
{
	if (!p_out_service_infos || num == 0) {
		return 0;
	}

	int n = 0;

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (n >= num) {
			break;
		}

		if (!service_infos_ [i].is_used) {
			continue;
		}

		if (!service_infos_ [i].is_tune_target) {
			continue;
		}

		(p_out_service_infos + n)->table_id = service_infos_ [i].table_id;
		(p_out_service_infos + n)->transport_stream_id = service_infos_ [i].transport_stream_id;
		(p_out_service_infos + n)->original_network_id = service_infos_ [i].original_network_id;
		(p_out_service_infos + n)->service_id = service_infos_ [i].service_id;
		(p_out_service_infos + n)->service_type = service_infos_ [i].service_type;
		(p_out_service_infos + n)->p_service_name_char = service_infos_ [i].service_name_char;

		++ n;
	}
	
	return n;
}

void manager::cache_event_pf_infos (void)
{
	bool r = false;

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		// 今選局中のserviceを探します
		if (!service_infos_ [i].is_tune_target) {
			continue;
		}

		uint8_t _table_id = TBLID_EIT_PF_A; // 自ストリームのみ
		uint16_t _transport_stream_id = service_infos_ [i].transport_stream_id;
		uint16_t _original_network_id = service_infos_ [i].original_network_id;
		uint16_t _service_id = service_infos_ [i].service_id;
		if (
			(_table_id == 0) &&
			(_transport_stream_id == 0) &&
			(_original_network_id == 0) &&
			(_service_id == 0)
		) {
			continue;
		}

		r = cache_event_pf_infos (_table_id, _transport_stream_id, _original_network_id, _service_id);

	}

	if (!r) {
		_UTL_LOG_D ("(cache_event_pf_infos) not match");
	}
}

/**
 * 引数をキーにテーブル内容を_EVENT_PF_INFOに保存します
 * 少なくともp/fの２つ分は見つかるはず
 */
bool manager::cache_event_pf_infos (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
)
{
	if (
		(_table_id == 0) &&
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return false;
	}

	int m = 0;

//	std::lock_guard<std::recursive_mutex> lock (*mEIT_H_ref.mutex_);

//	std::vector<parser::event_information_table::table*>::const_iterator iter = mEIT_H_ref.tables_->begin();
//	for (; iter != mEIT_H_ref.tables_->end(); ++ iter) {
	std::vector<parser::event_information_table::table*>::const_iterator iter = mEIT_H.get_tables ()->begin();
	for (; iter != mEIT_H.get_tables ()->end(); ++ iter) {
		// p/fでそれぞれ別のテーブル

		parser::event_information_table::table *p_table = *iter;

		// キーが一致したテーブルをみます
		if (
			(_table_id != p_table->header.table_id) ||
			(_transport_stream_id != p_table->transport_stream_id) ||
			(_original_network_id != p_table->original_network_id) ||
			(_service_id != p_table->header.table_id_extension)
		) {
			continue;
		}

		_event_pf_info_t * p_info = find_empty_event_pf_info ();
		if (!p_info) {
			_UTL_LOG_E ("find_empty_event_pf_info failure.");
			return false;
		}

		p_info->table_id = p_table->header.table_id;
		p_info->transport_stream_id = p_table->transport_stream_id;
		p_info->original_network_id = p_table->original_network_id;
		p_info->service_id = p_table->header.table_id_extension;

		std::vector<parser::event_information_table::table::event>::const_iterator iter_event = p_table->events.begin();
		for (; iter_event != p_table->events.end(); ++ iter_event) {
			// p/fのテーブルにつき eventは一つだと思われる...  いちおうforでまわしておく

			if (iter_event->event_id == 0) {
				// event情報が付いてない
				continue;
			}

			p_info->event_id = iter_event->event_id;

			time_t stime = parser::ts_arib_common::get_epoch_from_mjd (iter_event->start_time);
			arec::common::etime wk (stime);
			p_info->start_time = wk;

			int dur_sec = parser::ts_arib_common::get_sec_from_bcd (iter_event->duration);
			wk.add_sec (dur_sec);
			p_info->end_time = wk;

			std::vector<parser::descriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SHORT_EVENT_DESCRIPTOR) {
					parser::short_event_descriptor sd (*iter_desc);
					if (!sd.is_valid) {
						_UTL_LOG_W ("invalid ShortEventDescriptor");
						continue;
					}

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					AribToString (aribstr, (const char*)sd.event_name_char, (int)sd.event_name_length);
					strncpy (p_info->event_name_char, aribstr, MAXSECLEN);
				}

			} // loop descriptors

		} // loop events


		if (p_info->event_id == 0) {
			// event情報が付いてない
			// cancel
			p_info->clear();
			continue;
		}

		p_info->p_org_table = p_table;
		p_info->is_used = true;
		++ m;

//		_UTL_LOG_I ("cache_event_pf_infos 0x%04x 0x%04x 0x%04x 0x%04x",
//			p_info->transport_stream_id,
//			p_info->original_network_id,
//			p_info->service_id,
//			p_info->event_id
//		);
	} // loop tables


	if (m == 0) {
		return false;
	} else {
		return true;
	}
}

_event_pf_info_t* manager::find_empty_event_pf_info (void)
{
	int i = 0;
	for (i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (!event_pf_infos_ [i].is_used) {
			break;
		}
	}

	if (i == EVENT_PF_INFOS_MAX) {
		_UTL_LOG_W ("event_pf_infos_ full.");
		return NULL;
	}

	return &event_pf_infos_ [i];
}

void manager::check_event_pf_infos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (event_pf_infos_ [i].is_used) {

			arec::common::etime cur_time;
			cur_time.set_current_time();

			if (event_pf_infos_ [i].start_time <= cur_time && event_pf_infos_ [i].end_time >= cur_time) {

				event_pf_infos_ [i].state = event_pf_state::present;

			} else if (event_pf_infos_ [i].start_time > cur_time) {

				event_pf_infos_ [i].state = event_pf_state::follow;

			} else if (event_pf_infos_ [i].end_time < cur_time) {

				event_pf_infos_ [i].state = event_pf_state::already_passed;

			} else {
				_UTL_LOG_E ("check_event_pf_infos ???");
			}
		}
	}
}

void manager::refresh_event_pf_infos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (event_pf_infos_ [i].is_used) {
			if (event_pf_infos_ [i].state == event_pf_state::already_passed) {

//TODO parserがわで同等の処理しているはず
//				mEIT_H.clear_pf (event_pf_infos_ [i].p_org_table);

				event_pf_infos_ [i].clear();
			}
		}
	}
}

void manager::dump_event_pf_infos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (event_pf_infos_ [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			event_pf_infos_ [i].dump();
		}
	}
}

void manager::clear_event_pf_infos (void)
{
//	_UTL_LOG_I ("clear_event_pf_infos");
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		event_pf_infos_ [i].clear();
	}
}

// eventPfInfo for request
_event_pf_info_t* manager::find_event_pf_info (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	event_pf_state state
)
{
	int i = 0;
	for (i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (!event_pf_infos_ [i].is_used) {
			continue;
		}

#ifndef _DUMMY_TUNER // デバッグ中はp/f関係なしに
		if (event_pf_infos_ [i].state != state) {
			continue;
		}
#endif

		if (
			(event_pf_infos_ [i].table_id == _table_id) &&
			(event_pf_infos_ [i].transport_stream_id == _transport_stream_id) &&
			(event_pf_infos_ [i].original_network_id == _original_network_id) &&
			(event_pf_infos_ [i].service_id == _service_id)
		) {
			break;
		}
	}

	if (i == EVENT_PF_INFOS_MAX) {
//		_UTL_LOG_W ("find_event_pf_info not found.");
		return NULL;
	}

	return &event_pf_infos_[i];
}

void manager::cache_network_info (void)
{
	std::vector<parser::network_information_table::table*>::const_iterator iter = mNIT.get_tables ()->begin();
	for (; iter != mNIT.get_tables ()->end(); ++ iter) {

		parser::network_information_table::table *p_table = *iter;
		uint8_t tbl_id = p_table->header.table_id;
		uint16_t network_id = p_table->header.table_id_extension;

		// 自ストリームのみ
		if (tbl_id != TBLID_NIT_A) {
			continue;
		}

		network_info_.is_used = true;

		network_info_.table_id = tbl_id;
		network_info_.network_id = network_id;

		std::vector<parser::descriptor>::const_iterator iter_desc = p_table->descriptors.begin();
		for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {

			if (iter_desc->tag == DESC_TAG__NETWORK_NAME_DESCRIPTOR) {
				parser::network_name_descriptor nnd (*iter_desc);
				if (!nnd.is_valid) {
					_UTL_LOG_W ("invalid NetworkNameDescriptor");
					continue;
				}

				char aribstr [MAXSECLEN];
				memset (aribstr, 0x00, MAXSECLEN);
				memset (network_info_.network_name_char, 0x00, 64);
				AribToString (aribstr, (const char*)nnd.name_char, (int)nnd.length);
				strncpy (network_info_.network_name_char, aribstr, 64);
			}
		} // loop descriptors

		std::vector<parser::network_information_table::table::stream>::const_iterator iter_strm = p_table->streams.begin();
		for (; iter_strm != p_table->streams.end(); ++ iter_strm) {

			network_info_.transport_stream_id = iter_strm->transport_stream_id;
			network_info_.original_network_id = iter_strm->original_network_id;

			std::vector<parser::descriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
			for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SERVICE_LIST_DESCRIPTOR) {
					parser::service_list_descriptor sld (*iter_desc);
					if (!sld.is_valid) {
						_UTL_LOG_W ("invalid ServiceListDescriptor");
						continue;
					}

					int n = 0;
					std::vector<parser::service_list_descriptor::service>::const_iterator iter_sv = sld.services.begin();
					for (; iter_sv != sld.services.end(); ++ iter_sv) {
						if (n >= 16) {
							break;
						}
						network_info_.services[n].service_id = iter_sv->service_id;
						network_info_.services[n].service_type = iter_sv->service_type;
						++ n;
					}
					network_info_.services_num = n;

				} else if (iter_desc->tag == DESC_TAG__TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR) {
					parser::terrestrial_delivery_system_descriptor tdsd (*iter_desc);
					if (!tdsd.is_valid) {
						_UTL_LOG_W ("invalid TerrestrialDeliverySystemDescriptor");
						continue;
					}

					network_info_.area_code = tdsd.area_code;
					network_info_.guard_interval = tdsd.guard_interval;
					network_info_.transmission_mode = tdsd.transmission_mode;

				} else if (iter_desc->tag == DESC_TAG__TS_INFORMATION_DESCRIPTOR) {
					parser::ts_information_descriptor tsid (*iter_desc);
					if (!tsid.is_valid) {
						_UTL_LOG_W ("invalid TSInformationDescriptor");
						continue;
					}

					network_info_.remote_control_key_id = tsid.remote_control_key_id;

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					memset (network_info_.ts_name_char, 0x00, 64);
					AribToString (aribstr, (const char*)tsid.ts_name_char, (int)tsid.length_of_ts_name);
					strncpy (network_info_.ts_name_char, aribstr, 64);
				}

			} // loop descriptors

			// streamは基本１つだろうことを期待
			break;

		} // loop streams

		// tableも基本１つだろうことを期待
		break;

	} // loop tables
}

void manager::dump_network_info (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	if (network_info_.is_used) {
		network_info_.dump();
	}
}

void manager::clear_network_info (void)
{
	network_info_.clear();
}


void manager::store_logo (void)
{
	if (state_ == client_iface::psisi_state::not_ready) {
		return;
	}

	const auto *p_tables = mCDT.get_tables ();
	if (!p_tables) {
		return;
	}

	std::string path = settings_->get_params().get_logo_path();
	if (path.length() > 0) {
		struct stat _s;
		if (stat(path.c_str(), &_s) != 0) {
			_UTL_LOG_I ("mkdir [%s]\n", path.c_str());
			int r = arec::common::utils::makedir (path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
			if (r == 0) {
				printf ("mkdir success [%s]\n", path.c_str());
			} else {
				printf ("mkdir failure [%s]\n", path.c_str());
				exit (EXIT_FAILURE);
			}
		}
	}

	arec::common::pngcrc pngcrc;

	for (auto it = p_tables->cbegin(); it != p_tables->cend(); ++ it) {
		if ((*it)->data_type != 0x01) {
			continue;
		}

		const size_t buff_plte_size = 4 + 4 + (COMMON_CLUT_SIZE * 3) + 4;
		std::vector<uint8_t> buff_plte (buff_plte_size, 0);
		{
			uint8_t *p = &buff_plte[0];
			*p = ((COMMON_CLUT_SIZE * 3) >> 24) & 0xff; ++ p;
			*p = ((COMMON_CLUT_SIZE * 3) >> 16) & 0xff; ++ p;
			*p = ((COMMON_CLUT_SIZE * 3) >> 8) & 0xff; ++ p;
			*p = (COMMON_CLUT_SIZE * 3) & 0xff; ++ p;
			*p = 'P'; ++ p;
			*p = 'L'; ++ p;
			*p = 'T'; ++ p;
			*p = 'E'; ++ p;
			for (int i = 0; i < COMMON_CLUT_SIZE; ++ i) {
				*p = parser::ts_arib_common::get_clut_palette (i, 0);
				++ p;
				*p = parser::ts_arib_common::get_clut_palette (i, 1);
				++ p;
				*p = parser::ts_arib_common::get_clut_palette (i, 2);
				++ p;
			}
			uint32_t _crc = pngcrc.crc(&buff_plte[4], 4 + (COMMON_CLUT_SIZE * 3));
			*p = (_crc >> 24) & 0xff; ++ p;
			*p = (_crc >> 16) & 0xff; ++ p;
			*p = (_crc >> 8) & 0xff; ++ p;
			*p = _crc & 0xff;
		}

		const size_t buff_trns_size = 4 + 4 + COMMON_CLUT_SIZE + 4;
		std::vector<uint8_t> buff_trns (buff_trns_size, 0);
		{
			uint8_t *p = &buff_trns[0];
			*p = (COMMON_CLUT_SIZE >> 24) & 0xff; ++ p;
			*p = (COMMON_CLUT_SIZE >> 16) & 0xff; ++ p;
			*p = (COMMON_CLUT_SIZE >> 8) & 0xff; ++ p;
			*p = COMMON_CLUT_SIZE & 0xff; ++ p;
			*p = 't'; ++ p;
			*p = 'R'; ++ p;
			*p = 'N'; ++ p;
			*p = 'S'; ++ p;
			for (int i = 0; i < COMMON_CLUT_SIZE; ++ i) {
				*p = parser::ts_arib_common::get_clut_palette (i, 3);
				++ p;
			}
			uint32_t _crc = pngcrc.crc(&buff_trns[4], 4 + COMMON_CLUT_SIZE);
			*p = (_crc >> 24) & 0xff; ++ p;
			*p = (_crc >> 16) & 0xff; ++ p;
			*p = (_crc >> 8) & 0xff; ++ p;
			*p = _crc & 0xff;
		}

		size_t png_include_plte_size = buff_plte_size + buff_trns_size + (*it)->data.data_size;
		std::vector<uint8_t> png_include_plte (png_include_plte_size, 0);
		{
			uint8_t *p = png_include_plte.data();
			// 元pngのヘッダをコピーします
			memcpy (p, (*it)->data.data_byte.get(), 33);
			p += 33;

			// PLTEチャンクをコピー(挿入)します
			memcpy (p, buff_plte.data(), buff_plte_size);
			p += buff_plte_size;

			// tRNSチャンクをコピー(挿入)します
			memcpy (p, buff_trns.data(), buff_trns_size);
			p += buff_trns_size;

			// 元pngの残りをコピーします
			memcpy (p, (*it)->data.data_byte.get() + 33, (*it)->data.data_size - 33);
		}

		char _name[PATH_MAX] = {0};
		snprintf (
			_name,
			sizeof(_name),
			"%s/logo_%s_0x%04x_0x%04x_0x%04x_0x%02x%04x%04x.png",
			path.length() > 0 ? path.c_str() : ".",
			network_info_.ts_name_char,
			network_info_.transport_stream_id,
			network_info_.original_network_id,
			(*it)->header.table_id_extension,
			(*it)->data.logo_type,
			(*it)->data.logo_id,
			(*it)->data.logo_version
		);
		std::ofstream ofs (_name, std::ios::out|std::ios::binary);
		ofs.write((char*)png_include_plte.data(), png_include_plte_size);
		ofs.flush();
		ofs.close();
	}
}



//////////  arec::tuner_control::client_iface::ITsReceiveHandler  //////////

bool manager::on_pre_ts_receive (void)
{
	// coseq: request_async は元来 fire-and-forget のため option/外部CP 操作は不要
	parser_.reset();

	return true;
}

void manager::on_post_ts_receive (void)
{
	// coseq: 上記同様に不要
}

bool manager::on_check_ts_receive_loop (void)
{
	return true;
}

bool manager::on_ts_received (void *p_ts_data, int length)
{

	// ts parser processing
	parser_.put ((uint8_t*)p_ts_data, length);

	return true;
}



//////////  parser::ts_parser::IParserListener  //////////

bool manager::on_ts_packet_available (parser::TS_HEADER *p_header, uint8_t *p_payload, size_t payload_size)
{
	if (!p_header || !p_payload || payload_size == 0) {
		// through
		return true;
	}

	switch (p_header->pid) {
	case PID_PAT: {

		_parser_notice_t _notice (client_iface::psisi_type::PAT, p_header, p_payload, payload_size);
		modules::get_manager()->request_async (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
			static_cast<int>(client_iface::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_EIT_H: {

		_parser_notice_t _notice (client_iface::psisi_type::EIT_H_PF, p_header, p_payload, payload_size);
		modules::get_manager()->request_async (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
			static_cast<int>(client_iface::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_NIT: {

		_parser_notice_t _notice (client_iface::psisi_type::NIT, p_header, p_payload, payload_size);
		modules::get_manager()->request_async (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
			static_cast<int>(client_iface::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_SDT: {

		_parser_notice_t _notice (client_iface::psisi_type::SDT, p_header, p_payload, payload_size);
		modules::get_manager()->request_async (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
			static_cast<int>(client_iface::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_CAT: {

		_parser_notice_t _notice (client_iface::psisi_type::CAT, p_header, p_payload, payload_size);
		modules::get_manager()->request_async (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
			static_cast<int>(client_iface::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_CDT: {

		_parser_notice_t _notice (client_iface::psisi_type::CDT, p_header, p_payload, payload_size);
		modules::get_manager()->request_async (
			static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
			static_cast<int>(client_iface::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

//	case PID_RST:
//		r = mRST.check_section (p_header, p_payload, payload_size);
//		break;

//	case PID_BIT:
//		r = mBIT.check_section (p_header, p_payload, payload_size);
//		break;

	default:
		// parse PMT
		if (program_map_.has(p_header->pid)) {
			_parser_notice_t _notice (client_iface::psisi_type::PMT, p_header, p_payload, payload_size);
			modules::get_manager()->request_async (
				static_cast<uint8_t>(modules::module_id::psisi_manager) + get_group_id(),
				static_cast<int>(client_iface::sequence::parser_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);
		}

		break;
	}

	// 注) v1 は request_async の戻り(キュー溢れ)で 100us の backpressure を入れていたが
	//     coseq の request_async(fire-and-forget) は戻り値が無いため省略

	return true;
}


//////////  parser::event_information_table_sched::IEventScheduleHandler  //////////

void manager::on_schedule_update (void)
{
//	_UTL_LOG_I (__PRETTY_FUNCTION__);


	client_iface::psisi_type _type = client_iface::psisi_type::EIT_H_SCHED;

	modules::get_manager()->request_async (
		static_cast<uint8_t>(modules::module_id::event_schedule_manager),
		static_cast<int>(arec::event_schedule::client_iface::sequence::parser_notice),
		(uint8_t*)&_type,
		sizeof(_type)
	);

}

} // namespace psisi
} // namespace arec
