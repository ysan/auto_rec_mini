#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include<iostream>
#include<fstream>
#include<sstream>

#include "PsisiManager.h"
#include "PsisiManagerIf.h"
#include "PsisiManagerStructsAddition.h"

#include "TunerControlIf.h"
#include "modules.h"
#include "Forker.h"

#include "aribstr.h"


typedef struct _parser_notice {
	_parser_notice (CPsisiManagerIf::psisi_type type, TS_HEADER* p_ts_header, uint8_t* p_payload, size_t payload_size) {
		this->type = type;
		this->payload_size = payload_size;
		this->ts_header = *p_ts_header;
		memcpy (this->payload, p_payload, payload_size);
	};

	CPsisiManagerIf::psisi_type type;
	TS_HEADER ts_header;
	uint8_t payload [TS_PACKET_LEN];
	size_t payload_size;
} _parser_notice_t;


CPsisiManager::CPsisiManager (std::string name, uint8_t que_max, uint8_t group_id)
	:threadmgr::CThreadMgrBase (name, que_max)
	,CGroup (group_id)
	,mp_ts_handler (this)
	,m_parser (this)
	,m_tuner_notify_client_id (0xff)
	,m_ts_receive_handler_id (-1)
	,m_tuner_is_tuned (false)
	,m_is_detected_PAT (false)
	,m_state (CPsisiManagerIf::psisi_state::not_ready)
	,mPAT (16)
	,mEIT_H (4096*100, 100)
	,mCDT (4096*100)
	,mEIT_H_sched (4096*100, 100, this)
	,m_is_enable_EIT_sched (false)
{
	const int _max = static_cast<int>(CPsisiManagerIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_module_up(p_if);}, "on_module_up"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_module_down(p_if);}, "on_module_down"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_state(p_if);}, "on_get_state"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_check_loop(p_if);}, "on_check_loop"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_parser_notice(p_if);}, "on_parser_notice"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_stabilization_after_tuning(p_if);}, "on_stabilization_after_tuning"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_register_pat_detect_notify(p_if);}, "on_register_pat_detect_notify"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_unregister_pat_detect_notify(p_if);}, "on_unregister_pat_detect_notify"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_register_event_change_notify(p_if);}, "on_register_event_change_notify"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_unregister_event_change_notify(p_if);}, "on_unregister_event_change_notify"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_register_psisi_state_notify(p_if);}, "on_register_psisi_state_notify"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_unregister_psisi_state_notify(p_if);}, "on_unregister_psisi_state_notify"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_pat_detect_state(p_if);}, "on_get_pat_detect_state"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_stream_infos(p_if);}, "on_get_stream_infos"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_current_service_infos(p_if);}, "on_get_current_service_infos"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_present_event_info(p_if);}, "on_get_present_event_info"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_follow_event_info(p_if);}, "on_get_follow_event_info"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_get_current_network_info(p_if);}, "on_get_current_network_info"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_enable_parse_eit_sched(p_if);}, "on_enable_parse_eit_sched"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_disable_parse_eit_sched(p_if);}, "on_disable_parse_eit_sched"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_dump_caches(p_if);}, "on_dump_caches"},
		{[&](threadmgr::CThreadMgrIf *p_if){CPsisiManager::on_dump_tables(p_if);}, "on_dump_tables"},
	};
	set_sequences(seqs, _max);


	mp_settings = CSettings::getInstance();

	// references
//	mPAT_ref = mPAT.reference();
//	mEIT_H_ref = mEIT_H.reference();
//	mNIT_ref =  mNIT.reference();
//	mSDT_ref = mSDT.reference();


	m_pat_recv_time.clear();

	clear_program_infos ();
	clear_service_infos ();
	clear_event_pf_infos ();
	clear_network_info ();

	m_EIT_H_comp_flag.clear();
	m_NIT_comp_flag.clear();
	m_SDT_comp_flag.clear();
}

CPsisiManager::~CPsisiManager (void)
{
	m_pat_recv_time.clear();

	clear_program_infos ();
	clear_service_infos ();
	clear_event_pf_infos ();
	clear_network_info ();

	m_EIT_H_comp_flag.clear();
	m_NIT_comp_flag.clear();
	m_SDT_comp_flag.clear();

	reset_sequences();
}


void CPsisiManager::on_create (void)
{
	// SCHED_FIFOにして優先度上げてみます
	// 確認コマンド (FFやRRで99のtaskとか存在する)
	// $ ps -em -o pid,tid,policy,pri,ni,rtprio,comm
	int policy = SCHED_FIFO;
	struct sched_param param;
	param.sched_priority = 80;
	if (pthread_setschedparam(pthread_self(), policy, &param) != 0) {
		_UTL_PERROR ("pthread_setschedparam");
	}
}

void CPsisiManager::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
		SECTID_REQ_REG_HANDLER,
		SECTID_WAIT_REG_HANDLER,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;

	switch (section_id) {
	case SECTID_ENTRY:
		section_id = SECTID_REQ_REG_TUNER_NOTIFY;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_REG_TUNER_NOTIFY: {
		CTunerControlIf _if (get_external_if(), getGroupId());
		_if.request_register_tuner_notify ();

		section_id = SECTID_WAIT_REG_TUNER_NOTIFY;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_TUNER_NOTIFY:
		rslt = p_if->get_source().get_result();
        if (rslt == threadmgr::result::success) {
			m_tuner_notify_client_id = *(uint8_t*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_REG_HANDLER;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_REG_HANDLER: {

		_UTL_LOG_I ("CTunerControlIf::ITsReceiveHandler %p", mp_ts_handler);
		CTunerControlIf _if (get_external_if(), getGroupId());
		_if.request_register_ts_receive_handler (&mp_ts_handler);

		section_id = SECTID_WAIT_REG_HANDLER;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_REG_HANDLER:
		rslt = p_if->get_source().get_result();
        if (rslt == threadmgr::result::success) {
			m_ts_receive_handler_id = *(int*)(p_if->get_source().get_message().data());
			section_id = SECTID_REQ_CHECK_LOOP;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("reqRegisterTsReceiveHandler is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_CHECK_LOOP:
		request_async (EN_MODULE_PSISI_MANAGER + getGroupId(), static_cast<int>(CPsisiManagerIf::sequence::check_loop));

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

void CPsisiManager::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CPsisiManager::on_get_state (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	p_if->reply (threadmgr::result::success, (uint8_t*)&m_state, sizeof(m_state));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_check_loop (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CHECK_PAT,
		SECTID_CHECK_PAT_WAIT,
		SECTID_CHECK_EVENT_PF,
		SECTID_CHECK_EVENT_PF_WAIT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	switch (section_id) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		p_if->reply (threadmgr::result::success);

		section_id = SECTID_CHECK_PAT;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK_PAT:

		p_if->set_timeout (1000); // 1sec

		section_id = SECTID_CHECK_PAT_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_PAT_WAIT: {

		if (m_tuner_is_tuned) {
			// PAT途絶チェック
			// 最後にPATを受けとった時刻から再び受け取るまで30秒以上経過していたら異常	
			CEtime tcur;
			tcur.setCurrentTime();
			CEtime ttmp = m_pat_recv_time;
			ttmp.addSec (30); 
			if (tcur > ttmp) {
				_UTL_LOG_E ("PAT was not detected. probably stream is broken...");

				if (m_is_detected_PAT) {
					// true -> false
					// fire notify
					CPsisiManagerIf::pat_detection_state _state = CPsisiManagerIf::pat_detection_state::not_detected;
					p_if->notify (NOTIFY_CAT__PAT_DETECT, (uint8_t*)&_state, sizeof(CPsisiManagerIf::pat_detection_state));
				}

				m_is_detected_PAT = false;

			} else {

				if (!m_is_detected_PAT) {
					// false -> true
					// fire notify
					CPsisiManagerIf::pat_detection_state _state = CPsisiManagerIf::pat_detection_state::detected;
					p_if->notify (NOTIFY_CAT__PAT_DETECT, (uint8_t*)&_state, sizeof(CPsisiManagerIf::pat_detection_state));
				}

				m_is_detected_PAT = true;
			}
		}

		section_id = SECTID_CHECK_EVENT_PF;
		act = threadmgr::action::continue_;
		}
		break;

	case SECTID_CHECK_EVENT_PF:

		p_if->set_timeout (1000); // 1sec

		section_id = SECTID_CHECK_EVENT_PF_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_EVENT_PF_WAIT: {

//		if (check_event_pf_infos (p_if)) {
#ifndef _DUMMY_TUNER
// ローカルデバッグ中は消したくないので
//			refresh_event_pf_infos ();
#endif
//		}

		assign_follow_event_to_service_infos ();
		check_follow_event_at_service_infos (p_if);


		section_id = SECTID_CHECK_PAT;
		act = threadmgr::action::continue_;
		} break;

	case SECTID_END:
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_parser_notice (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	EN_CHECK_SECTION r = EN_CHECK_SECTION__COMPLETED;

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	mPAT.checkAsyncDelete ();
	mEIT_H.checkAsyncDelete ();
	mEIT_H_sched.checkAsyncDelete ();
	mNIT.checkAsyncDelete ();
	mSDT.checkAsyncDelete ();
	mCAT.checkAsyncDelete ();
	mCDT.checkAsyncDelete ();


	_parser_notice_t *p_notice = (_parser_notice_t*)(p_if->get_source().get_message().data());
	switch (p_notice->type) {
		case CPsisiManagerIf::psisi_type::PAT:

		r = mPAT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED ) {
			_UTL_LOG_I ("notice new PAT");
			clear_program_infos();
			cache_program_infos();

			// 新しいPATが取れたら PMT parserを準備します

			// PATは一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
			std::vector<CProgramAssociationTable::CTable*>::const_iterator iter = mPAT.getTables()->end();
			CProgramAssociationTable::CTable* latest = *(-- iter);

			// 一度クリアします
			m_program_map.clear();

			for (const auto& program : latest->programs) {
				if (program.program_number == 0) {
					continue;
				}
				m_program_map.add(program.program_map_PID);
			}
		}

		// PAT途絶チェック用
		m_pat_recv_time.setCurrentTime ();

		break;

	case CPsisiManagerIf::psisi_type::PMT: {
		r = m_program_map.parse(&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new PMT");
			clear_program_infos();
			cache_program_infos();
		}

		}
		break;

	case CPsisiManagerIf::psisi_type::CAT:

		r = mCAT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new CAT");
//TODO
		}

		break;

	case CPsisiManagerIf::psisi_type::CDT:

		r = mCDT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new CDT");

			store_logo();
		}

		break;

	case CPsisiManagerIf::psisi_type::NIT:

		r = mNIT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_NIT_comp_flag.check_update (true);

			_UTL_LOG_I ("notice new NIT");

			// ここにくるってことは選局したとゆうこと
			clear_network_info();
			cache_network_info();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_NIT_comp_flag.check_update (false);
		}

		break;

	case CPsisiManagerIf::psisi_type::SDT:

		r = mSDT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_SDT_comp_flag.check_update (true);

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
			m_SDT_comp_flag.check_update (false);
		}

		break;

	case CPsisiManagerIf::psisi_type::EIT_H_PF:

		r = mEIT_H.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_EIT_H_comp_flag.check_update (true);

			_UTL_LOG_I ("notice new EIT p/f");
			clear_event_pf_infos();
			cache_event_pf_infos ();
			check_event_pf_infos();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_EIT_H_comp_flag.check_update (false);
		}


		{
			uint32_t opt = get_external_if()->get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			get_external_if()->set_request_option (opt);

#ifdef _DUMMY_TUNER // デバッグ中は m_is_enable_EIT_sched 関係なしに
			mEIT_H_sched.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
#else
			if (m_is_enable_EIT_sched) {
				mEIT_H_sched.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
			}
#endif
			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);
		}

		break;

	default:
		break;
	}

	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_stabilization_after_tuning (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_CLEAR,
		SECTID_CLEAR_WAIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	switch (section_id) {
	case SECTID_ENTRY:

		// workaround
		// 選局とと同時にparserNoticeに多くのキューが入り あふれてしまうことがある
		// そのため 本シーケンスのset_timeoutの返しキューがうけとれず停止
		// -> tunerService的に選局失敗 -> リトライ
		// リトライ時に またリクエストを受け取れるように enable_overwriteを使います
		p_if->enable_overwrite();

		// 先にreplyしておく
		p_if->reply (threadmgr::result::success);

		section_id = SECTID_CLEAR;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CLEAR:

		p_if->set_timeout (200);

		section_id = SECTID_CLEAR_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CLEAR_WAIT:

		mPAT.clear();
		mEIT_H.clear();
		mNIT.clear();
		mSDT.clear();
		mCAT.clear();
		mCDT.clear();
//		mRST.clear();
//		mBIT.clear();

		m_EIT_H_comp_flag.clear();
		m_NIT_comp_flag.clear();
		m_SDT_comp_flag.clear();

		section_id = SECTID_CHECK;
		act = threadmgr::action::continue_;
		break;

	case SECTID_CHECK:

		p_if->set_timeout (100);

		section_id = SECTID_CHECK_WAIT;
		act = threadmgr::action::wait;
		break;

	case SECTID_CHECK_WAIT:

		// EIT,NIT,SDTのparserが完了したか確認します
		// 少なくともこの3つが完了していれば psisi manager的に選局完了とします (CPsisiManagerIf::psisi_state::READY)
		// ARIB規格上では送出頻度は以下のとおりになっている... PAT,PMTは上3つより頻度高いので除外しています
		//   PAT 1回以上/100mS
		//   PMT 1回以上/100mS
		//   EIT 1回以上/2s
		//   NIT 1回以上/10s
		//   SDT 1回以上/2s
		if (
			m_EIT_H_comp_flag.is_completed() &&
			m_NIT_comp_flag.is_completed() &&
			m_SDT_comp_flag.is_completed()
		) {
			_UTL_LOG_I ("PSI/SI ready.");

			m_tuner_is_tuned = true;

			// fire notify
			m_state = CPsisiManagerIf::psisi_state::ready;
			p_if->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));


			section_id = SECTID_END;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_CHECK;
			act = threadmgr::action::continue_;
		}

		break;

	case SECTID_END:
		// workaround
		p_if->disable_overwrite();

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_register_pat_detect_notify (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	uint8_t client_id = 0;
	threadmgr::result rslt;
	bool r = p_if->reg_notify (NOTIFY_CAT__PAT_DETECT, &client_id);
	if (r) {
		rslt = threadmgr::result::success;
	} else {
		rslt = threadmgr::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	p_if->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_unregister_pat_detect_notify (threadmgr::CThreadMgrIf *p_if)
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
	// msgからclient_idを取得
	uint8_t client_id = *(p_if->get_source().get_message().data());
	bool r = p_if->unreg_notify (NOTIFY_CAT__PAT_DETECT, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::error;
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_register_event_change_notify (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	uint8_t client_id = 0;
	threadmgr::result rslt;
	bool r = p_if->reg_notify (NOTIFY_CAT__EVENT_CHANGE, &client_id);
	if (r) {
		rslt = threadmgr::result::success;
	} else {
		rslt = threadmgr::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	p_if->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_unregister_event_change_notify (threadmgr::CThreadMgrIf *p_if)
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
	// msgからclient_idを取得
	uint8_t client_id = *(p_if->get_source().get_message().data());
	bool r = p_if->unreg_notify (NOTIFY_CAT__EVENT_CHANGE, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::error;
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_register_psisi_state_notify (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	uint8_t client_id = 0;
	threadmgr::result rslt;
	bool r = p_if->reg_notify (NOTIFY_CAT__PSISI_STATE, &client_id);
	if (r) {
		rslt = threadmgr::result::success;
	} else {
		rslt = threadmgr::result::error;
	}

	_UTL_LOG_I ("registerd client_id=[0x%02x]\n", client_id);

	// client_idをreply msgで返す 
	p_if->reply (rslt, (uint8_t*)&client_id, sizeof(client_id));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_unregister_psisi_state_notify (threadmgr::CThreadMgrIf *p_if)
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
	// msgからclient_idを取得
	uint8_t client_id = *(p_if->get_source().get_message().data());
	bool r = p_if->unreg_notify (NOTIFY_CAT__PSISI_STATE, client_id);
	if (r) {
		_UTL_LOG_I ("unregisterd client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::success;
	} else {
		_UTL_LOG_E ("failure unregister client_id=[0x%02x]\n", client_id);
		rslt = threadmgr::result::error;
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_get_pat_detect_state (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CPsisiManagerIf::pat_detection_state state =
			m_is_detected_PAT ? CPsisiManagerIf::pat_detection_state::detected: CPsisiManagerIf::pat_detection_state::not_detected;

	p_if->reply (threadmgr::result::success, (uint8_t*)&state, sizeof(state));


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_get_stream_infos (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	psisi_structs::request_stream_infos_param_t param = *(psisi_structs::request_stream_infos_param_t*)(p_if->get_source().get_message().data());
	if (!param.p_out_stream_infos || param.array_max_num == 0) {
		_UTL_LOG_E ("psisi_structs::request_stream_infos_param_t is invalid.\n");
		p_if->reply (threadmgr::result::error);

	} else {
		int get_num = get_stream_infos (param.program_number, param.type, param.p_out_stream_infos, param.array_max_num);

		// reply msgで格納数を渡します
		p_if->reply (threadmgr::result::success, (uint8_t*)&get_num, sizeof(get_num));
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_get_current_service_infos (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	psisi_structs::request_service_infos_param_t param = *(psisi_structs::request_service_infos_param_t*)(p_if->get_source().get_message().data());
	if (!param.p_out_service_infos || param.array_max_num == 0) {
		_UTL_LOG_E ("psisi_structs::request_service_infos_param_t is invalid.\n");
		p_if->reply (threadmgr::result::error);

	} else {
		int get_num = get_current_service_infos (param.p_out_service_infos, param.array_max_num);

		// reply msgで格納数を渡します
		p_if->reply (threadmgr::result::success, (uint8_t*)&get_num, sizeof(get_num));
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_get_present_event_info (threadmgr::CThreadMgrIf *p_if)
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

	psisi_structs::request_event_info_param_t param = *(psisi_structs::request_event_info_param_t*)(p_if->get_source().get_message().data());
	if (!param.p_out_event_info) {
		_UTL_LOG_E ("psisi_structs::request_event_info_param_t is invalid.\n");
		rslt = threadmgr::result::error;

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

			rslt = threadmgr::result::success;

		} else {
			_UTL_LOG_E ("not found PresentEventInfo");
			rslt = threadmgr::result::error;
mEIT_H.dumpTables_simple();
mEIT_H.dumpSectionList();
		}
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_get_follow_event_info (threadmgr::CThreadMgrIf *p_if)
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

	psisi_structs::request_event_info_param_t param = *(psisi_structs::request_event_info_param_t*)(p_if->get_source().get_message().data());
	if (!param.p_out_event_info) {
		_UTL_LOG_E ("psisi_structs::request_event_info_param_t is invalid.\n");
		rslt = threadmgr::result::error;

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

			rslt = threadmgr::result::success;

		} else {
			_UTL_LOG_E ("not found FollowEventInfo");
			rslt = threadmgr::result::error;
		}
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_get_current_network_info (threadmgr::CThreadMgrIf *p_if)
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

	psisi_structs::request_network_info_param_t param = *(psisi_structs::request_network_info_param_t*)(p_if->get_source().get_message().data());
	if (!param.p_out_network_info) {
		_UTL_LOG_E ("psisi_structs::request_network_info_param_t is invalid.\n");
		rslt = threadmgr::result::error;

	} else {
		if (m_network_info.is_used) {
			param.p_out_network_info->table_id = m_network_info.table_id;
			param.p_out_network_info->network_id = m_network_info.network_id;

			strncpy (
				param.p_out_network_info->network_name_char,
				m_network_info.network_name_char,
				strlen(m_network_info.network_name_char)
			);

			param.p_out_network_info->transport_stream_id = m_network_info.transport_stream_id;
			param.p_out_network_info->original_network_id = m_network_info.original_network_id;

			for (int i = 0; i < m_network_info.services_num; ++ i) {
				param.p_out_network_info->services[i].service_id = m_network_info.services[i].service_id;
				param.p_out_network_info->services[i].service_type = m_network_info.services[i].service_type;
			}
			param.p_out_network_info->services_num = m_network_info.services_num;

			param.p_out_network_info->area_code = m_network_info.area_code;
			param.p_out_network_info->guard_interval = m_network_info.guard_interval;
			param.p_out_network_info->transmission_mode = m_network_info.transmission_mode;
			param.p_out_network_info->remote_control_key_id = m_network_info.remote_control_key_id;
			strncpy (
				param.p_out_network_info->ts_name_char,
				m_network_info.ts_name_char,
				strlen(m_network_info.ts_name_char)
			);

			rslt = threadmgr::result::success;

		} else {
			_UTL_LOG_E ("m_network_info is not availale...");
			rslt = threadmgr::result::error;
		}
	}

	p_if->reply (rslt);


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_enable_parse_eit_sched (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	m_is_enable_EIT_sched = true;


	// reply msgにparserのアドレスを乗せます
	enable_parse_eit_sched_reply_param_t param = {&mEIT_H_sched};


	p_if->reply (threadmgr::result::success, (uint8_t*)&param, sizeof(param));

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_disable_parse_eit_sched (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	m_is_enable_EIT_sched = false;


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_dump_caches (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	int type = *(int*)(p_if->get_source().get_message().data());
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


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_dump_tables (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CPsisiManagerIf::psisi_type type = *(CPsisiManagerIf::psisi_type*)(p_if->get_source().get_message().data());
	switch (type) {
	case CPsisiManagerIf::psisi_type::PAT:
		mPAT.dumpTables();
		break;

	case CPsisiManagerIf::psisi_type::PMT:
		m_program_map.dump();
		break;

	case CPsisiManagerIf::psisi_type::EIT_H_PF:
		mEIT_H.dumpTables();
		break;

	case CPsisiManagerIf::psisi_type::EIT_H_PF_simple:
		mEIT_H.dumpTables_simple();
		break;

	case CPsisiManagerIf::psisi_type::EIT_H_SCHED:
		mEIT_H_sched.dumpTables();
		break;

	case CPsisiManagerIf::psisi_type::EIT_H_SCHED_event:
		mEIT_H_sched.dumpTables_event();
		break;

	case CPsisiManagerIf::psisi_type::EIT_H_SCHED_simple:
		mEIT_H_sched.dumpTables_simple();
		break;

	case CPsisiManagerIf::psisi_type::NIT:
		mNIT.dumpTables();
		break;

	case CPsisiManagerIf::psisi_type::SDT:
		mSDT.dumpTables();
		break;

	case CPsisiManagerIf::psisi_type::CAT:
		mCAT.dumpTables();
		break;

	case CPsisiManagerIf::psisi_type::CDT:
		mCDT.dumpTables();
		break;

//	case CPsisiManagerIf::psisi_type::RST:
//		mRST.dump_tables();
//		break;

//	case CPsisiManagerIf::psisi_type::BIT:
//		mBIT.dump_tables();
//		break;

	default:
		break;
	}


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CPsisiManager::on_receive_notify (threadmgr::CThreadMgrIf *p_if)
{
	if (!(p_if->get_source().get_client_id() == m_tuner_notify_client_id)) {
		return ;
	}

	CTunerControlIf::tuner_state state = *(CTunerControlIf::tuner_state*)(p_if->get_source().get_message().data());
	switch (state) {
	case CTunerControlIf::tuner_state::tuning_begin: {
		_UTL_LOG_I ("CTunerControlIf::tuner_state::tuning_begin");

		// fire notify
		m_state = CPsisiManagerIf::psisi_state::not_ready;
		p_if->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));

		m_tuner_is_tuned = false;

//#ifdef _DUMMY_TUNER 
//		// ダミーデバッグ中はここでクリア
//		mPAT.clear();
//		mEIT_H.clear();
//		mNIT.clear();
//		mSDT.clear();
//		mRST.clear();
//		mBIT.clear();
//
//		m_EIT_H_comp_flag.clear();
//		m_NIT_comp_flag.clear();
//		m_SDT_comp_flag.clear();
//#endif

		}
		break;

	case CTunerControlIf::tuner_state::tuning_success: {
		_UTL_LOG_I ("CTunerControlIf::tuner_state::tuning_success");

//#ifndef _DUMMY_TUNER
		// 選局後の安定化
		uint32_t opt = get_request_option ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		set_request_option (opt);

		request_async (EN_MODULE_PSISI_MANAGER + getGroupId(), static_cast<int>(CPsisiManagerIf::sequence::stabilization_after_tuning));

		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		set_request_option (opt);
//#endif

		}
		break;

	case CTunerControlIf::tuner_state::tuning_error_stop: {
		_UTL_LOG_I ("CTunerControlIf::tuner_state::tuning_error_stop");

		// fire notify
		m_state = CPsisiManagerIf::psisi_state::not_ready;
		p_if->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));


		// tune stopで一連の動作が終わった時の対策
		clear_program_infos();
		clear_network_info();
		clear_service_infos (true);
		clear_event_pf_infos();

		m_tuner_is_tuned = false;

		}
		break;

	case CTunerControlIf::tuner_state::tune_stop: {
		_UTL_LOG_I ("CTunerControlIf::tuner_state::tune_stop");

		// fire notify
		m_state = CPsisiManagerIf::psisi_state::not_ready;
		p_if->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));


		// tune stopで一連の動作が終わった時の対策
		clear_program_infos();
		clear_network_info();
		clear_service_infos (true);
		clear_event_pf_infos();

		m_tuner_is_tuned = false;

		}
		break;

	default:
		break;
	}
}

void CPsisiManager::cache_program_infos (void)
{
	int n = 0;

	if (mPAT.getTables()->size() == 0) {
		return;
	}

	// PATは一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
	std::vector<CProgramAssociationTable::CTable*>::const_iterator iter = mPAT.getTables()->end();
	CProgramAssociationTable::CTable* latest = *(-- iter);
	if (!latest) {
		return;
	}
	CProgramAssociationTable::CTable *p_table = latest;

	uint8_t tbl_id = p_table->header.table_id;
	uint16_t ts_id = p_table->header.table_id_extension;

	std::vector<CProgramAssociationTable::CTable::CProgram>::const_iterator iter_prog = p_table->programs.begin();
	for (; iter_prog != p_table->programs.end(); ++ iter_prog) {

		if (n >= PROGRAM_INFOS_MAX) {
			return ;
		}

		m_program_infos[n].table_id = tbl_id;
		m_program_infos[n].transport_stream_id = ts_id;

		m_program_infos[n].program_number = iter_prog->program_number;
		m_program_infos[n].program_map_PID = iter_prog->program_map_PID;

		m_program_infos[n].p_org_table = p_table;
		m_program_infos[n].is_used = true;

		// cache pmt streams
		std::shared_ptr<CProgramMapTable> pmt = m_program_map.get(iter_prog->program_map_PID);
		if (pmt != nullptr && pmt->getTables()->size() > 0) {
			// PMTも一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
			std::vector<CProgramMapTable::CTable*>::const_iterator iter = pmt->getTables()->end();
			CProgramMapTable::CTable* latest = *(-- iter);
			if (latest) {
				CProgramMapTable::CTable *p_table = latest;

				for (const auto & stream : p_table->streams) {
					std::shared_ptr<_program_info_t::_stream>s = std::make_shared<_program_info_t::_stream>();
					s->type = stream.stream_type;
					s->pid = stream.elementary_PID;
					m_program_infos[n].streams.push_back(s);
				}
			}
		}

		++ n;

	} // loop programs
}

void CPsisiManager::dump_program_infos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		if (m_program_infos [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_program_infos [i].dump();
		}
	}
}

void CPsisiManager::clear_program_infos (void)
{
	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		m_program_infos [i].clear();
	}
}

// streamInfo for request
int CPsisiManager::get_stream_infos (uint16_t program_number, EN_STREAM_TYPE type, psisi_structs::stream_info_t *p_out_stream_infos, int num)
{
	if (!p_out_stream_infos || num == 0) {
		return 0;
	}

	int r = 0;

	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		if (m_program_infos[i].program_number == program_number) {
			for (const auto& stream : m_program_infos[i].streams) {
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
void CPsisiManager::cache_service_infos (bool is_at_tuning)
{
	std::vector<CServiceDescriptionTable::CTable*>::const_iterator iter = mSDT.getTables()->begin();
	for (; iter != mSDT.getTables()->end(); ++ iter) {

		CServiceDescriptionTable::CTable *p_table = *iter;
		uint8_t tbl_id = p_table->header.table_id;
		uint16_t ts_id = p_table->header.table_id_extension;
		uint16_t network_id = p_table->original_network_id;

		// 自ストリームのみ
		if (tbl_id != TBLID_SDT_A) {
			continue;
		}

		std::vector<CServiceDescriptionTable::CTable::CService>::const_iterator iter_svc = p_table->services.begin();
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

			std::vector<CDescriptor>::const_iterator iter_desc = iter_svc->descriptors.begin();
			for (; iter_desc != iter_svc->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SERVICE_DESCRIPTOR) {
					CServiceDescriptor sd (*iter_desc);
					if (!sd.isValid) {
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
			p_info->last_update.setCurrentTime();

		} // loop services

	} // loop tables
}

_service_info_t* CPsisiManager::find_service_info (
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
		if (m_service_infos [i].is_used) {
			if (
				(_table_id == m_service_infos [i].table_id) &&
				(_transport_stream_id == m_service_infos [i].transport_stream_id) &&
				(_original_network_id == m_service_infos [i].original_network_id) &&
				(_service_id == m_service_infos [i].service_id)
			) {
				return &m_service_infos [i];
			}
		}
	}

	// not existed
	return NULL;
}

_service_info_t* CPsisiManager::find_empty_service_info (void)
{
	int i = 0;
	for (i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (!m_service_infos [i].is_used) {
			break;
		}
	}

	if (i == SERVICE_INFOS_MAX) {
		_UTL_LOG_W ("m_service_infos full.");
		return NULL;
	}

	return &m_service_infos [i];
}

bool CPsisiManager::is_exist_service_table (
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

	std::vector<CServiceDescriptionTable::CTable*>::const_iterator iter = mSDT.getTables()->begin();
	for (; iter != mSDT.getTables()->end(); ++ iter) {

		CServiceDescriptionTable::CTable *p_table = *iter;
		uint8_t tbl_id = p_table->header.table_id;
		uint16_t ts_id = p_table->header.table_id_extension;
		uint16_t network_id = p_table->original_network_id;

		std::vector<CServiceDescriptionTable::CTable::CService>::const_iterator iter_svc = p_table->services.begin();
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

void CPsisiManager::assign_follow_event_to_service_infos (void)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_service_infos [i].is_tune_target) {
			if (!m_service_infos [i].event_follow_info.is_used) {

				_event_pf_info_t* p_info = find_event_pf_info (
												TBLID_EIT_PF_A,
												m_service_infos [i].transport_stream_id,
												m_service_infos [i].original_network_id,
												m_service_infos [i].service_id,
												event_pf_state::follow
											);
				if (p_info) {
//_UTL_LOG_I ("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
//p_info->dump();
//					m_service_infos [i].event_follow_info = *p_info;
//					memcpy (&m_service_infos [i].event_follow_info, p_info, sizeof(_EVENT_PF_INFO));
					m_service_infos [i].event_follow_info.clear();
					m_service_infos [i].event_follow_info.start_time = p_info->start_time;
					m_service_infos [i].event_follow_info.end_time = p_info->end_time;
					m_service_infos [i].event_follow_info.is_used = true;
					m_service_infos [i].last_update.setCurrentTime();
//_UTL_LOG_I ("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
//m_service_infos [i].event_follow_info.dump();
				}
			}
		}
	}
}

void CPsisiManager::check_follow_event_at_service_infos (threadmgr::CThreadMgrIf *p_if)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_service_infos [i].is_tune_target) {
			if (m_service_infos [i].event_follow_info.is_used) {

				CEtime cur_time;
				cur_time.setCurrentTime();

				if (m_service_infos [i].event_follow_info.start_time <= cur_time) {
					// 次のイベント開始時刻到来
					// event changed

					if (p_if) {
						psisi_structs::notify_event_info_t _info;
						_info.table_id = m_service_infos [i].event_follow_info.table_id;
						_info.transport_stream_id = m_service_infos [i].event_follow_info.transport_stream_id;
						_info.original_network_id = m_service_infos [i].event_follow_info.original_network_id;
						_info.service_id = m_service_infos [i].event_follow_info.service_id;
						_info.event_id = m_service_infos [i].event_follow_info.event_id;

						// fire notify
						p_if->notify (NOTIFY_CAT__EVENT_CHANGE, (uint8_t*)&_info, sizeof(psisi_structs::notify_event_info_t));
					}


					CEtime t_tmp = m_service_infos [i].event_follow_info.start_time;
					t_tmp.addSec(15);
					// 次イベント開始から15秒以上経過していたら
					// cache_event_pf_infos 実行する
					if (t_tmp < cur_time) {
						_UTL_LOG_E ("more than 15 seconds have passed since the start of the next event.");
						clear_event_pf_infos ();
						cache_event_pf_infos ();
						check_event_pf_infos ();
					}

					// 用済みなのでクリアします
					m_service_infos [i].event_follow_info.clear();

				}

			}
		}
	}
}

void CPsisiManager::dump_service_infos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_service_infos [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_service_infos [i].dump();
		}
	}
}

void CPsisiManager::clear_service_infos (void)
{
	clear_service_infos (false);
}

void CPsisiManager::clear_service_infos (bool is_at_tuning)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (is_at_tuning) {
			m_service_infos [i].clear_at_tuning();

		} else {
			m_service_infos [i].clear();
		}
	}
}

// serviceInfo for request
int CPsisiManager::get_current_service_infos (psisi_structs::service_info_t *p_out_service_infos, int num)
{
	if (!p_out_service_infos || num == 0) {
		return 0;
	}

	int n = 0;

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (n >= num) {
			break;
		}

		if (!m_service_infos [i].is_used) {
			continue;
		}

		if (!m_service_infos [i].is_tune_target) {
			continue;
		}

		(p_out_service_infos + n)->table_id = m_service_infos [i].table_id;
		(p_out_service_infos + n)->transport_stream_id = m_service_infos [i].transport_stream_id;
		(p_out_service_infos + n)->original_network_id = m_service_infos [i].original_network_id;
		(p_out_service_infos + n)->service_id = m_service_infos [i].service_id;
		(p_out_service_infos + n)->service_type = m_service_infos [i].service_type;
		(p_out_service_infos + n)->p_service_name_char = m_service_infos [i].service_name_char;

		++ n;
	}
	
	return n;
}

void CPsisiManager::cache_event_pf_infos (void)
{
	bool r = false;

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		// 今選局中のserviceを探します
		if (!m_service_infos [i].is_tune_target) {
			continue;
		}

		uint8_t _table_id = TBLID_EIT_PF_A; // 自ストリームのみ
		uint16_t _transport_stream_id = m_service_infos [i].transport_stream_id;
		uint16_t _original_network_id = m_service_infos [i].original_network_id;
		uint16_t _service_id = m_service_infos [i].service_id;
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
bool CPsisiManager::cache_event_pf_infos (
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

//	std::lock_guard<std::recursive_mutex> lock (*mEIT_H_ref.mpMutex);

//	std::vector<CEventInformationTable::CTable*>::const_iterator iter = mEIT_H_ref.mp_tables->begin();
//	for (; iter != mEIT_H_ref.mp_tables->end(); ++ iter) {
	std::vector<CEventInformationTable::CTable*>::const_iterator iter = mEIT_H.getTables()->begin();
	for (; iter != mEIT_H.getTables()->end(); ++ iter) {
		// p/fでそれぞれ別のテーブル

		CEventInformationTable::CTable *p_table = *iter;

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
			_UTL_LOG_E ("getEventPfByServiceId failure.");
			return false;
		}

		p_info->table_id = p_table->header.table_id;
		p_info->transport_stream_id = p_table->transport_stream_id;
		p_info->original_network_id = p_table->original_network_id;
		p_info->service_id = p_table->header.table_id_extension;

		std::vector<CEventInformationTable::CTable::CEvent>::const_iterator iter_event = p_table->events.begin();
		for (; iter_event != p_table->events.end(); ++ iter_event) {
			// p/fのテーブルにつき eventは一つだと思われる...  いちおうforでまわしておく

			if (iter_event->event_id == 0) {
				// event情報が付いてない
				continue;
			}

			p_info->event_id = iter_event->event_id;

			time_t stime = CTsAribCommon::getEpochFromMJD (iter_event->start_time);
			CEtime wk (stime);
			p_info->start_time = wk;

			int dur_sec = CTsAribCommon::getSecFromBCD (iter_event->duration);
			wk.addSec (dur_sec);
			p_info->end_time = wk;

			std::vector<CDescriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SHORT_EVENT_DESCRIPTOR) {
					CShortEventDescriptor sd (*iter_desc);
					if (!sd.isValid) {
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

_event_pf_info_t* CPsisiManager::find_empty_event_pf_info (void)
{
	int i = 0;
	for (i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (!m_event_pf_infos [i].is_used) {
			break;
		}
	}

	if (i == EVENT_PF_INFOS_MAX) {
		_UTL_LOG_W ("m_event_pf_infos full.");
		return NULL;
	}

	return &m_event_pf_infos [i];
}

void CPsisiManager::check_event_pf_infos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_event_pf_infos [i].is_used) {

			CEtime cur_time;
			cur_time.setCurrentTime();

			if (m_event_pf_infos [i].start_time <= cur_time && m_event_pf_infos [i].end_time >= cur_time) {

				m_event_pf_infos [i].state = event_pf_state::present;

			} else if (m_event_pf_infos [i].start_time > cur_time) {

				m_event_pf_infos [i].state = event_pf_state::follow;

			} else if (m_event_pf_infos [i].end_time < cur_time) {

				m_event_pf_infos [i].state = event_pf_state::already_passed;

			} else {
				_UTL_LOG_E ("check_event_pf_infos ???");
			}
		}
	}
}

void CPsisiManager::refresh_event_pf_infos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_event_pf_infos [i].is_used) {
			if (m_event_pf_infos [i].state == event_pf_state::already_passed) {

//TODO parserがわで同等の処理しているはず
//				mEIT_H.clear_pf (m_event_pf_infos [i].p_org_table);

				m_event_pf_infos [i].clear();
			}
		}
	}
}

void CPsisiManager::dump_event_pf_infos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_event_pf_infos [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_event_pf_infos [i].dump();
		}
	}
}

void CPsisiManager::clear_event_pf_infos (void)
{
//	_UTL_LOG_I ("clear_event_pf_infos");
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		m_event_pf_infos [i].clear();
	}
}

// eventPfInfo for request
_event_pf_info_t* CPsisiManager::find_event_pf_info (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	event_pf_state state
)
{
	int i = 0;
	for (i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (!m_event_pf_infos [i].is_used) {
			continue;
		}

#ifndef _DUMMY_TUNER // デバッグ中はp/f関係なしに
		if (m_event_pf_infos [i].state != state) {
			continue;
		}
#endif

		if (
			(m_event_pf_infos [i].table_id == _table_id) &&
			(m_event_pf_infos [i].transport_stream_id == _transport_stream_id) &&
			(m_event_pf_infos [i].original_network_id == _original_network_id) &&
			(m_event_pf_infos [i].service_id == _service_id)
		) {
			break;
		}
	}

	if (i == EVENT_PF_INFOS_MAX) {
//		_UTL_LOG_W ("find_event_pf_info not found.");
		return NULL;
	}

	return &m_event_pf_infos[i];
}

void CPsisiManager::cache_network_info (void)
{
	std::vector<CNetworkInformationTable::CTable*>::const_iterator iter = mNIT.getTables()->begin();
	for (; iter != mNIT.getTables()->end(); ++ iter) {

		CNetworkInformationTable::CTable *p_table = *iter;
		uint8_t tbl_id = p_table->header.table_id;
		uint16_t network_id = p_table->header.table_id_extension;

		// 自ストリームのみ
		if (tbl_id != TBLID_NIT_A) {
			continue;
		}

		m_network_info.is_used = true;

		m_network_info.table_id = tbl_id;
		m_network_info.network_id = network_id;

		std::vector<CDescriptor>::const_iterator iter_desc = p_table->descriptors.begin();
		for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {

			if (iter_desc->tag == DESC_TAG__NETWORK_NAME_DESCRIPTOR) {
				CNetworkNameDescriptor nnd (*iter_desc);
				if (!nnd.isValid) {
					_UTL_LOG_W ("invalid NetworkNameDescriptor");
					continue;
				}

				char aribstr [MAXSECLEN];
				memset (aribstr, 0x00, MAXSECLEN);
				memset (m_network_info.network_name_char, 0x00, 64);
				AribToString (aribstr, (const char*)nnd.name_char, (int)nnd.length);
				strncpy (m_network_info.network_name_char, aribstr, 64);
			}
		} // loop descriptors

		std::vector<CNetworkInformationTable::CTable::CStream>::const_iterator iter_strm = p_table->streams.begin();
		for (; iter_strm != p_table->streams.end(); ++ iter_strm) {

			m_network_info.transport_stream_id = iter_strm->transport_stream_id;
			m_network_info.original_network_id = iter_strm->original_network_id;

			std::vector<CDescriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
			for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SERVICE_LIST_DESCRIPTOR) {
					CServiceListDescriptor sld (*iter_desc);
					if (!sld.isValid) {
						_UTL_LOG_W ("invalid ServiceListDescriptor");
						continue;
					}

					int n = 0;
					std::vector<CServiceListDescriptor::CService>::const_iterator iter_sv = sld.services.begin();
					for (; iter_sv != sld.services.end(); ++ iter_sv) {
						if (n >= 16) {
							break;
						}
						m_network_info.services[n].service_id = iter_sv->service_id;
						m_network_info.services[n].service_type = iter_sv->service_type;
						++ n;
					}
					m_network_info.services_num = n;

				} else if (iter_desc->tag == DESC_TAG__TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR) {
					CTerrestrialDeliverySystemDescriptor tdsd (*iter_desc);
					if (!tdsd.isValid) {
						_UTL_LOG_W ("invalid TerrestrialDeliverySystemDescriptor");
						continue;
					}

					m_network_info.area_code = tdsd.area_code;
					m_network_info.guard_interval = tdsd.guard_interval;
					m_network_info.transmission_mode = tdsd.transmission_mode;

				} else if (iter_desc->tag == DESC_TAG__TS_INFORMATION_DESCRIPTOR) {
					CTSInformationDescriptor tsid (*iter_desc);
					if (!tsid.isValid) {
						_UTL_LOG_W ("invalid TSInformationDescriptor");
						continue;
					}

					m_network_info.remote_control_key_id = tsid.remote_control_key_id;

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					memset (m_network_info.ts_name_char, 0x00, 64);
					AribToString (aribstr, (const char*)tsid.ts_name_char, (int)tsid.length_of_ts_name);
					strncpy (m_network_info.ts_name_char, aribstr, 64);
				}

			} // loop descriptors

			// streamは基本１つだろうことを期待
			break;

		} // loop streams

		// tableも基本１つだろうことを期待
		break;

	} // loop tables
}

void CPsisiManager::dump_network_info (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	if (m_network_info.is_used) {
		m_network_info.dump();
	}
}

void CPsisiManager::clear_network_info (void)
{
	m_network_info.clear();
}


#include "Pngcrc.h"
void CPsisiManager::store_logo (void)
{
	if (m_state == CPsisiManagerIf::psisi_state::not_ready) {
		return;
	}

	const auto *p_tables = mCDT.getTables();
	if (!p_tables) {
		return;
	}

	std::string *p_path = mp_settings->getParams()->getLogoPath();
	if (p_path->length() > 0) {
		struct stat _s;
		if (stat(p_path->c_str(), &_s) != 0) {
			CForker forker;
			if (!forker.create_pipes()) {
				_UTL_LOG_I ("forker.create_pipes failure\n");
				return;
			}
			std::string s = "/bin/mkdir -p " + *p_path;
			_UTL_LOG_I ("[%s]", s.c_str());
			if (!forker.do_fork(std::move(s))) {
				_UTL_LOG_I ("forker.do_fork failure\n");
				return;
			}

			CForker::CChildStatus cs = forker.wait_child();
			_UTL_LOG_I ("is_normal_end %d  get_return_code %d", cs.is_normal_end(), cs.get_return_code());
			forker.destroy_pipes();
			if (cs.is_normal_end() && cs.get_return_code() == 0) {
				// success
				_UTL_LOG_I ("mkdir -p %s\n", p_path->c_str());
			} else {
				_UTL_LOG_I ("mkdir failure [mkdir -p %s]\n", p_path->c_str());
				return;
			}
		}
	}

	CPngcrc pngcrc;

	for (auto it = p_tables->cbegin(); it != p_tables->cend(); ++ it) {
		if ((*it)->data_type != 0x01) {
			continue;
		}

		const size_t buff_plte_size = 4 + 4 + (COMMON_CLUT_SIZE * 3) + 4;
		uint8_t buff_plte [buff_plte_size] = {0};
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
				*p = CTsAribCommon::getCLUTPalette (i, 0);
				++ p;
				*p = CTsAribCommon::getCLUTPalette (i, 1);
				++ p;
				*p = CTsAribCommon::getCLUTPalette (i, 2);
				++ p;
			}
			uint32_t _crc = pngcrc.crc(&buff_plte[4], 4 + (COMMON_CLUT_SIZE * 3));
			*p = (_crc >> 24) & 0xff; ++ p;
			*p = (_crc >> 16) & 0xff; ++ p;
			*p = (_crc >> 8) & 0xff; ++ p;
			*p = _crc & 0xff;
		}

		const size_t buff_trns_size = 4 + 4 + COMMON_CLUT_SIZE + 4;
		uint8_t buff_trns [buff_trns_size] = {0};
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
				*p = CTsAribCommon::getCLUTPalette (i, 3);
				++ p;
			}
			uint32_t _crc = pngcrc.crc(&buff_trns[4], 4 + COMMON_CLUT_SIZE);
			*p = (_crc >> 24) & 0xff; ++ p;
			*p = (_crc >> 16) & 0xff; ++ p;
			*p = (_crc >> 8) & 0xff; ++ p;
			*p = _crc & 0xff;
		}

		size_t png_include_plte_size = buff_plte_size + buff_trns_size + (*it)->data.data_size;
		uint8_t png_include_plte [png_include_plte_size] = {0};
		{
			uint8_t *p = png_include_plte;
			// 元pngのヘッダをコピーします
			memcpy (p, (*it)->data.data_byte.get(), 33);
			p += 33;

			// PLTEチャンクをコピー(挿入)します
			memcpy (p, buff_plte, buff_plte_size); 
			p += buff_plte_size;

			// tRNSチャンクをコピー(挿入)します
			memcpy (p, buff_trns, buff_trns_size);
			p += buff_trns_size;

			// 元pngの残りをコピーします
			memcpy (p, (*it)->data.data_byte.get() + 33, (*it)->data.data_size - 33);
		}

		char _name[PATH_MAX] = {0};
		snprintf (
			_name,
			sizeof(_name),
			"%s/logo_%s_0x%04x_0x%04x_0x%04x_0x%02x%04x%04x.png",
			p_path->length() > 0 ? p_path->c_str() : ".",
			m_network_info.ts_name_char,
			m_network_info.transport_stream_id,
			m_network_info.original_network_id,
			(*it)->header.table_id_extension,
			(*it)->data.logo_type,
			(*it)->data.logo_id,
			(*it)->data.logo_version
		);
		std::ofstream ofs (_name, std::ios::out|std::ios::binary);
		ofs.write((char*)png_include_plte, png_include_plte_size);
		ofs.flush();
		ofs.close();
	}
}



//////////  CTunerControlIf::ITsReceiveHandler  //////////

bool CPsisiManager::on_pre_ts_receive (void)
{
	get_external_if()->create_external_cp();

	uint32_t opt = get_external_if()->get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	get_external_if()->set_request_option (opt);

	return true;
}

void CPsisiManager::on_post_ts_receive (void)
{
	uint32_t opt = get_external_if()->get_request_option ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	get_external_if()->set_request_option (opt);

	get_external_if()->destroy_external_cp();
}

bool CPsisiManager::on_check_ts_receive_loop (void)
{
	return true;
}

bool CPsisiManager::on_ts_received (void *p_ts_data, int length)
{

	// ts parser processing
	m_parser.run ((uint8_t*)p_ts_data, length);

	return true;
}



//////////  CTsParser::IParserListener  //////////

bool CPsisiManager::onTsPacketAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size)
{
	if (!p_ts_header || !p_payload || payload_size == 0) {
		// through
		return true;
	}

	bool r = true;

	switch (p_ts_header->pid) {
	case PID_PAT: {

		_parser_notice_t _notice (CPsisiManagerIf::psisi_type::PAT, p_ts_header, p_payload, payload_size);
		r = request_async (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_EIT_H: {

		_parser_notice_t _notice (CPsisiManagerIf::psisi_type::EIT_H_PF, p_ts_header, p_payload, payload_size);
		r = request_async (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_NIT: {

		_parser_notice_t _notice (CPsisiManagerIf::psisi_type::NIT, p_ts_header, p_payload, payload_size);
		r = request_async (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_SDT: {

		_parser_notice_t _notice (CPsisiManagerIf::psisi_type::SDT, p_ts_header, p_payload, payload_size);
		r = request_async (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_CAT: {

		_parser_notice_t _notice (CPsisiManagerIf::psisi_type::CAT, p_ts_header, p_payload, payload_size);
		r = request_async (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_CDT: {

		_parser_notice_t _notice (CPsisiManagerIf::psisi_type::CDT, p_ts_header, p_payload, payload_size);
		r = request_async (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

//	case PID_RST:
//		r = mRST.checkSection (p_ts_header, p_payload, payload_size);
//		break;

//	case PID_BIT:
//		r = mBIT.checkSection (p_ts_header, p_payload, payload_size);
//		break;

	default:
		// parse PMT
		if (m_program_map.has(p_ts_header->pid)) {
			_parser_notice_t _notice (CPsisiManagerIf::psisi_type::PMT, p_ts_header, p_payload, payload_size);
			r = request_async (
				EN_MODULE_PSISI_MANAGER + getGroupId(),
				static_cast<int>(CPsisiManagerIf::sequence::parser_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);
		}

		break;
	}

	// workaround
	// ts parse後に requestが集中するので 溢れた場合は100usまつことにします
	if (!r) {
		usleep(100);
	}

	return true;
}


//////////  CEventInformationTable_sched::IEventScheduleHandler  //////////

void CPsisiManager::onScheduleUpdate (void)
{
//	_UTL_LOG_I (__PRETTY_FUNCTION__);


	CPsisiManagerIf::psisi_type _type = CPsisiManagerIf::psisi_type::EIT_H_SCHED;

	request_async (
		EN_MODULE_EVENT_SCHEDULE_MANAGER,
		static_cast<int>(CEventScheduleManagerIf::sequence::parser_notice),
		(uint8_t*)&_type,
		sizeof(_type)
	);

}
