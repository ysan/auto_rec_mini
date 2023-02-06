#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PsisiManagerIf.h"
#include "TunerService.h"
#include "TunerServiceIf.h"
#include "modules.h"

#include "Settings.h"


CTunerService::CTunerService (std::string name, uint8_t que_max)
	:threadmgr::CThreadMgrBase (name, que_max)
	,m_tuner_resource_max (0)
	,m_next_allocate_tuner_id (0)
{
	const int _max = static_cast<int>(CTunerServiceIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_module_up(p_if);}, std::move("on_module_up")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_module_down(p_if);}, std::move("on_module_down")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_open(p_if);}, std::move("on_open")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_close(p_if);}, std::move("on_close")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_tune(p_if);}, std::move("on_tune")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_tune_with_retry(p_if);}, std::move("on_tune_with_retry")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_tune_advance(p_if);}, std::move("on_tune_advance")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_tune_stop(p_if);}, std::move("on_tune_stop")},
		{[&](threadmgr::CThreadMgrIf *p_if){CTunerService::on_dump_allocates(p_if);}, std::move("on_dump_allocates")},
	};
	set_sequences (seqs, _max);

}

CTunerService::~CTunerService (void)
{
	reset_sequences();
}


void CTunerService::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	// settingsを使って初期化する場合はmodule upで
	m_tuner_resource_max = CSettings::getInstance()->getParams()->getTunerHalAllocates()->size();
	_UTL_LOG_I ("m_tuner_resource_max:[%d]", m_tuner_resource_max);
	m_resource_allcates.clear();
	m_resource_allcates.resize(m_tuner_resource_max);
	for (auto &a : m_resource_allcates) {
		a = std::make_shared<resource_t>();
	}


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerService::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CTunerService::on_open (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	module::module_id caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
//	uint8_t allcated_tuner_id = allocate_linear (caller_module_id);
	uint8_t allcated_tuner_id = allocate_round_robin (caller_module_id);
	if (allcated_tuner_id != 0xff) {
		p_if->reply (threadmgr::result::success, (uint8_t*)&allcated_tuner_id, sizeof(allcated_tuner_id));
	} else {
//TODO 割り当てできなかった時はpriorityに応じた処理をする  notifyしたり...
		p_if->reply (threadmgr::result::error);
	}

	dump_allocates ();


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerService::on_close (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	uint8_t tuner_id = *(uint8_t*)(p_if->get_source().get_message().data());
	if (release (tuner_id)) {
		p_if->reply (threadmgr::result::success);
	} else {
		p_if->reply (threadmgr::result::error);
	}

	dump_allocates ();


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CTunerService::on_tune (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_CHECK_PSISI_STATE,
		SECTID_CHECK_WAIT_PSISI_STATE,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	static CTunerServiceIf::tune_param_t s_param;
	static int s_retry = 0;
	threadmgr::result rslt = threadmgr::result::success;


	switch (section_id) {
	case SECTID_ENTRY: {
		p_if->lock();

		memset (&s_param, 0x00, sizeof(s_param));
		s_retry = 0;

		module::module_id caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
		if (caller_module_id == module::module_id::tuner_service) {
			// tune_withRetry, tuneAdvance から呼ばれました
			_tune_param *p = (_tune_param*)(p_if->get_source().get_message().data());
			s_param.physical_channel = p->physical_channel;
			s_param.tuner_id = p->tuner_id;
			caller_module_id = p->caller_module_id;
		} else {
			// 外部モジュールから呼ばれました
			s_param = *(CTunerServiceIf::tune_param_t*)(p_if->get_source().get_message().data());
		}

		if (pre_check (s_param.tuner_id, caller_module_id, true)) {
			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		dump_allocates ();

		}
		break;

	case SECTID_REQ_TUNE: {

		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_param.physical_channel);
		_UTL_LOG_I ("freq=[%d]kHz\n", freq);

		CTunerControlIf _if (get_external_if(), s_param.tuner_id);
		_if.request_tune (freq);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_CHECK_PSISI_STATE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_W ("tune is failure");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_CHECK_PSISI_STATE:

		p_if->set_timeout(200);
		section_id = SECTID_CHECK_WAIT_PSISI_STATE;
		act = threadmgr::action::wait;

		break;

	case SECTID_CHECK_WAIT_PSISI_STATE: {

		CPsisiManagerIf _if(get_external_if(), s_param.tuner_id);
		if (!_if.request_get_state_sync ()) {
			// 選局直後のpsisi mgrはキュー溢れが起きる可能性あるので チェックします
			++ s_retry ;
			section_id = SECTID_CHECK_PSISI_STATE;
			act = threadmgr::action::continue_;
			break;
		}

		CPsisiManagerIf::psisi_state _psisiState = *(CPsisiManagerIf::psisi_state*)(p_if->get_source().get_message().data());
		if (_psisiState == CPsisiManagerIf::psisi_state::ready) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {

			if (s_retry > 75) {
				// 200ms * 75 = 約15秒 でタイムアウトします
				_UTL_LOG_E ("psi/si state invalid. (never transitioned EN_PSISI_STATE__READY)");
				section_id = SECTID_REQ_TUNE_STOP;
				act = threadmgr::action::continue_;

			} else {
				++ s_retry ;
				section_id = SECTID_CHECK_PSISI_STATE;
				act = threadmgr::action::continue_;
			}
		}

		}
		break;

	case SECTID_REQ_TUNE_STOP: {
		// 念の為停止しておきます
		CTunerControlIf _if (get_external_if(), s_param.tuner_id);
		_if.request_tune_stop ();

		section_id = SECTID_WAIT_TUNE_STOP;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
		// とくに結果はみません
		section_id = SECTID_END_ERROR;
		act = threadmgr::action::continue_;
		break;

	case SECTID_END_SUCCESS:
		p_if->unlock();

		m_resource_allcates[s_param.tuner_id]->is_now_tuned = true;
		dump_allocates ();

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:
		p_if->unlock();

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CTunerService::on_tune_with_retry (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

//	static CTunerServiceIf::tune_param_t s_param;
	static struct _tune_param s_param;
	static int s_retry = 0;
	threadmgr::result rslt = threadmgr::result::success;

	switch (section_id) {
	case SECTID_ENTRY: {

		memset (&s_param, 0x00, sizeof(s_param));
		s_retry = 3;

//		CTunerServiceIf::tune_param_t *p = (CTunerServiceIf::tune_param_t*)(p_if->get_source().get_message().data());
//		s_param.physical_channel = p->physical_channel;
//		s_param.tuner_id = p->tuner_id;
//		s_param.caller_module_id = p_if->get_source().get_thread_idx();

		module::module_id caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
		if (caller_module_id == module::module_id::tuner_service) {
			// tuneAdvance_withRetry から呼ばれました
			_tune_param *p = (_tune_param*)(p_if->get_source().get_message().data());
			s_param.physical_channel = p->physical_channel;
			s_param.tuner_id = p->tuner_id;
			s_param.caller_module_id = p->caller_module_id; // 引き継ぎます
		} else {
			// 外部モジュールから呼ばれました
			CTunerServiceIf::tune_param_t *p = (CTunerServiceIf::tune_param_t*)(p_if->get_source().get_message().data());
			s_param.physical_channel = p->physical_channel;
			s_param.tuner_id = p->tuner_id;
			s_param.caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
		}

		if (pre_check (s_param.tuner_id, s_param.caller_module_id, true)) {
			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

//		section_id = SECTID_REQ_TUNE;
//		act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE:

		request_async (
			static_cast<uint8_t>(module::module_id::tuner_service),
			static_cast<int>(CTunerServiceIf::sequence::tune),
			(uint8_t*)&s_param,
			sizeof (s_param)
		);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_END_SUCCESS;
			act = threadmgr::action::continue_;

		} else {
			if (s_retry > 0) {
				_UTL_LOG_W ("retry tune");
				section_id = SECTID_REQ_TUNE;
				act = threadmgr::action::continue_;
				-- s_retry;
			} else {
				_UTL_LOG_E ("retry over...");
				section_id = SECTID_END_ERROR;
				act = threadmgr::action::continue_;
			}
		}
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

void CTunerService::on_tune_advance (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	static struct _tune_param s_param;
	static struct _tune_advance_param s_adv_param; 
	threadmgr::result rslt = threadmgr::result::success;

	switch (section_id) {
	case SECTID_ENTRY: {

		memset (&s_param, 0x00, sizeof(s_param));
		memset (&s_adv_param, 0x00, sizeof(s_adv_param));

		module::module_id caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
		if (caller_module_id == module::module_id::tuner_service) {
			// 内部モジュールから呼ばれました
			// 今のとこ ここには入らない
			_tune_advance_param *p = (_tune_advance_param*)(p_if->get_source().get_message().data());
			s_adv_param.transport_stream_id = p->transport_stream_id;
			s_adv_param.original_network_id = p->original_network_id;
			s_adv_param.service_id = p->service_id;
			s_adv_param.tuner_id = p->tuner_id;
			s_adv_param.is_need_retry = p->is_need_retry;
			s_adv_param.caller_module_id = p->caller_module_id; // 引き継ぎます
		} else {
			// 外部モジュールから呼ばれました
			CTunerServiceIf::tune_advance_param_t *p = (CTunerServiceIf::tune_advance_param_t*)(p_if->get_source().get_message().data());
			s_adv_param.transport_stream_id = p->transport_stream_id;
			s_adv_param.original_network_id = p->original_network_id;
			s_adv_param.service_id = p->service_id;
			s_adv_param.tuner_id = p->tuner_id;
			s_adv_param.is_need_retry = p->is_need_retry;
			s_adv_param.caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
		}

		if (pre_check (s_adv_param.tuner_id, s_adv_param.caller_module_id, true)) {
			section_id = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		}
		break;

	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {

		CChannelManagerIf::service_id_param_t param = {
			s_adv_param.transport_stream_id,
			s_adv_param.original_network_id,
			s_adv_param.service_id
		};

		CChannelManagerIf _if (get_external_if());
		_if.request_get_pysical_channel_by_service_id (&param);

		section_id = SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {

			s_param.physical_channel = *(uint16_t*)(p_if->get_source().get_message().data());
			s_param.tuner_id = s_adv_param.tuner_id;
			s_param.caller_module_id = s_adv_param.caller_module_id;

			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("reqGetPysicalChannelByServiceId is failure.");
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE:

		request_async (
			static_cast<uint8_t>(module::module_id::tuner_service),
			s_adv_param.is_need_retry ?
				static_cast<int>(CTunerServiceIf::sequence::tune_with_retry) :
				static_cast<int>(CTunerServiceIf::sequence::tune),
			(uint8_t*)&s_param,
			sizeof (s_param)
		);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		break;

	case SECTID_WAIT_TUNE:
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

		m_resource_allcates[s_param.tuner_id]->transport_stream_id = s_adv_param.transport_stream_id;
		m_resource_allcates[s_param.tuner_id]->original_network_id = s_adv_param.original_network_id;
		m_resource_allcates[s_param.tuner_id]->service_id = s_adv_param.service_id;
		dump_allocates ();

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

void CTunerService::on_tune_stop (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	threadmgr::result rslt = threadmgr::result::success;
	static uint8_t s_tuner_id = 0;

	switch (section_id) {
	case SECTID_ENTRY: {
		p_if->lock();

		s_tuner_id = *(uint8_t*)(p_if->get_source().get_message().data());
		module::module_id caller_module_id = static_cast<module::module_id>(p_if->get_source().get_thread_idx());
		if (pre_check (s_tuner_id, caller_module_id, false)) {
			section_id = SECTID_REQ_TUNE_STOP;
			act = threadmgr::action::continue_;
		} else {
			section_id = SECTID_END_ERROR;
			act = threadmgr::action::continue_;
		}

		dump_allocates ();

		}
		break;

	case SECTID_REQ_TUNE_STOP: {
		CTunerControlIf _if (get_external_if(), s_tuner_id);
		_if.request_tune_stop ();

		section_id = SECTID_WAIT_TUNE_STOP;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
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
		p_if->unlock();

		m_resource_allcates[s_tuner_id]->is_now_tuned = false;
		m_resource_allcates[s_tuner_id]->transport_stream_id = 0;
		m_resource_allcates[s_tuner_id]->original_network_id = 0;
		m_resource_allcates[s_tuner_id]->service_id = 0;
		dump_allocates ();

		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	case SECTID_END_ERROR:
		p_if->unlock();

		p_if->reply (threadmgr::result::error);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CTunerService::on_dump_allocates (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	dump_allocates ();


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

uint8_t CTunerService::allocate_linear (module::module_id module_id)
{
	uint8_t _id = 0;
	// 未割り当てを探します
	for (const auto &a : m_resource_allcates) {
		if (a->tuner_id == 0xff) {
			break;
		}
		++ _id;
	}

	if (_id < m_tuner_resource_max) {
		m_resource_allcates[_id]->tuner_id = _id;
		m_resource_allcates[_id]->module_id = module_id;
		m_resource_allcates[_id]->priority = get_priority (module_id);
		_UTL_LOG_I ("allocated. [0x%02x]", _id);
		return _id;

	} else {
		_UTL_LOG_E ("resources full...");
		return 0xff;
	}
}

uint8_t CTunerService::allocate_round_robin (module::module_id module_id)
{
	uint8_t _init_id = m_next_allocate_tuner_id;
	int n = 0;

	while (1) {
		if (_init_id == m_next_allocate_tuner_id && n != 0) {
			break;
		}

		uint8_t _id = m_next_allocate_tuner_id;

		if (m_resource_allcates[_id]->tuner_id == 0xff) {
			m_resource_allcates[_id]->tuner_id = _id;
			m_resource_allcates[_id]->module_id = module_id;
			m_resource_allcates[_id]->priority = get_priority (module_id);
			_UTL_LOG_I ("allocated. [0x%02x]", _id);

			//set next
			++ m_next_allocate_tuner_id;
			if (m_next_allocate_tuner_id == m_tuner_resource_max) {
				m_next_allocate_tuner_id = 0;
			}

			return _id;

		} else {
			//set next
			++ m_next_allocate_tuner_id;
			if (m_next_allocate_tuner_id == m_tuner_resource_max) {
				m_next_allocate_tuner_id = 0;
			}
		}

		++ n;
	}

	_UTL_LOG_E ("resources full...");
	return 0xff;
}

bool CTunerService::release (uint8_t tuner_id)
{
	if (tuner_id < m_tuner_resource_max) {
		if (m_resource_allcates[tuner_id]->is_now_tuned) {
			_UTL_LOG_E ("not yet tuned... [0x%02x]", tuner_id);
			return false;

		} else {
			m_resource_allcates[tuner_id]->clear();
			_UTL_LOG_I ("released. [0x%02x]", tuner_id);
			return true;
		}

	} else {
		_UTL_LOG_E ("invalid tuner_id. [0x%02x]", tuner_id);
		return false;
	}
}

bool CTunerService::pre_check (uint8_t tuner_id, module::module_id module_id, bool is_req_tune) const
{
	if (tuner_id < m_tuner_resource_max) {
		if (
			(m_resource_allcates[tuner_id]->tuner_id == tuner_id) &&
			(m_resource_allcates[tuner_id]->module_id == module_id)
		) {
			if (is_req_tune) {
				if (!m_resource_allcates[tuner_id]->is_now_tuned) {
					return true;
				} else {
					_UTL_LOG_E ("already tuned... [0x%02x]", tuner_id);
					return false;
				}
			} else {
				// req tune stop
				if (m_resource_allcates[tuner_id]->is_now_tuned) {
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

CTunerService::client_priority CTunerService::get_priority (module::module_id module_id) const
{
	switch (module_id) {
	case module::module_id::rec_manager:
		return client_priority::RECORDING;

	case module::module_id::event_schedule_manager:
		return client_priority::EVENT_SCHEDULE;

	case module::module_id::channel_manager:
		return client_priority::CHANNEL_SCAN;

	case module::module_id::command_server:
		return client_priority::COMMAND;

	default:
		return client_priority::OTHER;
	}
}

void CTunerService::dump_allocates (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	for (const auto &a : m_resource_allcates) {
		a->dump();
	}
}
