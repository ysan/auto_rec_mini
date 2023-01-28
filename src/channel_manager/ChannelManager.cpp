#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>

#include "ChannelManager.h"
#include "modules.h"

#include "Settings.h"


CChannelManager::CChannelManager (std::string name, uint8_t que_max)
	:threadmgr::CThreadMgrBase (name, que_max)
{
	const int _max = static_cast<int>(CChannelManagerIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_module_up(p_if);}, "on_module_up"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_module_down(p_if);}, "on_module_down"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_channel_scan(p_if);}, "on_channel_scan"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_get_pysical_channel_by_service_id(p_if);}, "on_get_pysical_channel_by_service_id"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_get_pysical_channel_by_remote_control_key_id(p_if);}, "on_get_pysical_channel_by_remote_control_key_id"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_get_channels(p_if);}, "on_get_channels"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_get_transport_stream_name(p_if);}, "on_get_transport_stream_name"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_get_service_name(p_if);}, "on_get_service_name"},
		{[&](threadmgr::CThreadMgrIf *p_if){CChannelManager::on_dump_channels(p_if);}, "on_dump_channels"},
	};
	set_sequences (seqs, _max);


	m_channels.clear ();

}

CChannelManager::~CChannelManager (void)
{
	m_channels.clear ();

	reset_sequences();
}


void CChannelManager::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	switch (section_id) {
	case SECTID_ENTRY:

		load_channels ();
		dump_channels_simple ();


		section_id = SECTID_END;
		act = threadmgr::action::continue_;
		break;

	case SECTID_END:
		p_if->reply (threadmgr::result::success);
		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_module_down (threadmgr::CThreadMgrIf *p_if)
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

void CChannelManager::on_channel_scan (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_REQ_OPEN,
		SECTID_WAIT_OPEN,
		SECTID_CHECK_FREQ,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_WAIT_AFTER_TUNE,
		SECTID_REQ_GET_NETWORK_INFO,
		SECTID_WAIT_GET_NETWORK_INFO,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_NEXT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	static uint16_t s_ch = UHF_PHYSICAL_CHANNEL_MIN;	
	static psisi_structs::network_info_t s_network_info = {0};	
	static psisi_structs::service_info_t s_service_infos[10];
	threadmgr::result rslt = threadmgr::result::success;
	static uint8_t s_group_id = 0xff;


	switch (section_id) {
	case SECTID_ENTRY:
		p_if->lock();

		// 先にreplyしておきます
		p_if->reply (threadmgr::result::success);

		m_channels.clear ();

		section_id = SECTID_REQ_OPEN;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_OPEN: {

		CTunerServiceIf _if (get_external_if());
		_if.request_open ();

		section_id = SECTID_WAIT_OPEN;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_OPEN:
		rslt = get_if()->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			s_group_id = *(uint8_t*)(get_if()->get_source().get_message().data());
			_UTL_LOG_I ("req_open group_id:[0x%02x]", s_group_id);
			section_id = SECTID_CHECK_FREQ;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_E ("someone is using a tuner.");
			section_id = SECTID_END;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_CHECK_FREQ:

		if (s_ch >= UHF_PHYSICAL_CHANNEL_MIN && s_ch <= UHF_PHYSICAL_CHANNEL_MAX) {
			section_id = SECTID_REQ_TUNE;
			act = threadmgr::action::continue_;

		} else {
			section_id = SECTID_END;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE: {

		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_ch);
		_UTL_LOG_I ("(%s) ------  pysical channel:[%d] -> freq:[%d]k_hz", p_if->get_sequence_name(), s_ch, freq);

		CTunerServiceIf::tune_param_t param = {
			s_ch,
			s_group_id
		};

		CTunerServiceIf _if (get_external_if());
		_if.request_tune (&param);

		section_id = SECTID_WAIT_TUNE;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_WAIT_AFTER_TUNE;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_W ("tune is failure -> skip");
			section_id = SECTID_REQ_TUNE_STOP;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_WAIT_AFTER_TUNE:
		section_id = SECTID_REQ_GET_NETWORK_INFO;
		act = threadmgr::action::continue_;
		break;

	case SECTID_REQ_GET_NETWORK_INFO: {

		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_current_network_info (&s_network_info);

		section_id = SECTID_WAIT_GET_NETWORK_INFO;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_GET_NETWORK_INFO:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			section_id = SECTID_REQ_GET_SERVICE_INFOS;
			act = threadmgr::action::continue_;

		} else {
			_UTL_LOG_W ("network info is not found -> skip");
			section_id = SECTID_REQ_TUNE_STOP;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		CPsisiManagerIf _if (get_external_if(), s_group_id);
		_if.request_get_current_service_infos (s_service_infos, 10);

		section_id = SECTID_WAIT_GET_SERVICE_INFOS;
		act = threadmgr::action::wait;

        }
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		rslt = p_if->get_source().get_result();
		if (rslt == threadmgr::result::success) {
			int n_svc = *(int*)(p_if->get_source().get_message().data());
			if (n_svc > 0) {

				CChannel::service _services [10];
				for (int i = 0; i < n_svc; ++ i) {
					_services[i].service_id = s_service_infos[i].service_id;
					_services[i].service_type = s_service_infos[i].service_type;
					_services[i].service_name = s_service_infos[i].p_service_name_char;
				}

				CChannel r;
				r.set (
					s_ch,
					s_network_info.transport_stream_id,
					s_network_info.original_network_id,
					(const char*)s_network_info.network_name_char,
					s_network_info.area_code,
					s_network_info.remote_control_key_id,
					(const char*)s_network_info.ts_name_char,
					_services,
					n_svc
				);

				if (!is_duplicate_channel (&r)) {
					r.dump();
					m_channels.insert (std::pair<uint16_t, CChannel>(s_ch, r));
				}

				section_id = SECTID_REQ_TUNE_STOP;
				act = threadmgr::action::continue_;

			} else {
				_UTL_LOG_W ("req_get_current_service_infos  num is 0 -> skip");
				section_id = SECTID_REQ_TUNE_STOP;
				act = threadmgr::action::continue_;
			}

		} else {
			_UTL_LOG_W ("service infos is not found -> skip");
			section_id = SECTID_REQ_TUNE_STOP;
			act = threadmgr::action::continue_;
		}
		break;

	case SECTID_REQ_TUNE_STOP: {
		CTunerServiceIf _if (get_external_if());
		_if.request_tune_stop (s_group_id);

		section_id = SECTID_WAIT_TUNE_STOP;
		act = threadmgr::action::wait;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
		// とくに結果はみません
		section_id = SECTID_NEXT;
		act = threadmgr::action::continue_;
		break;

	case SECTID_NEXT:

		// inc pysical ch
		++ s_ch;

		memset (&s_network_info, 0x00, sizeof (s_network_info));
		memset (&s_service_infos, 0x00, sizeof (s_service_infos));

		section_id = SECTID_CHECK_FREQ;
		act = threadmgr::action::continue_;
		break;

	case SECTID_END:
		p_if->unlock();

		// reset pysical ch
		s_ch = UHF_PHYSICAL_CHANNEL_MIN;

		memset (&s_network_info, 0x00, sizeof (s_network_info));
		memset (&s_service_infos, 0x00, sizeof (s_service_infos));

		dump_channels ();
		save_channels ();

		_UTL_LOG_I ("channel scan end.");


		//-----------------------------//
		{
			uint32_t opt = get_request_option ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);

			// 選局を停止しときます tune stop
			// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
			CTunerServiceIf _if (get_external_if());
			_if.request_tune_stop (s_group_id);
			_if.request_close (s_group_id);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			set_request_option (opt);
		}
		//-----------------------------//


		s_group_id = 0xff;

		section_id = threadmgr::section_id::init;
		act = threadmgr::action::done;
		break;

	default:
		break;
	}

	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_get_pysical_channel_by_service_id (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CChannelManagerIf::service_id_param_t param =
			*(CChannelManagerIf::service_id_param_t*)(p_if->get_source().get_message().data());

	uint16_t _ch = get_pysical_channel_by_service_id (
						param.transport_stream_id,
						param.original_network_id,
						param.service_id
					);
	if (_ch == 0xffff) {

		_UTL_LOG_E ("get_pysical_channel_by_service_id is failure.");
		p_if->reply (threadmgr::result::error);

	} else {

		// リプライmsgに結果を乗せます
		p_if->reply (threadmgr::result::success, (uint8_t*)&_ch, sizeof(_ch));

	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_get_pysical_channel_by_remote_control_key_id (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CChannelManagerIf::remote_control_id_param_t param =
			*(CChannelManagerIf::remote_control_id_param_t*)(p_if->get_source().get_message().data());

	uint16_t _ch = get_pysical_channel_by_remote_control_key_id (
						param.transport_stream_id,
						param.original_network_id,
						param.remote_control_key_id
					);
	if (_ch == 0xffff) {

		_UTL_LOG_E ("get_pysical_channel_by_remote_control_key_id is failure.");
		p_if->reply (threadmgr::result::error);

	} else {

		// リプライmsgに結果を乗せます
		p_if->reply (threadmgr::result::success, (uint8_t*)&_ch, sizeof(_ch));

	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_get_channels (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CChannelManagerIf::request_channels_param_t _param =
				*(CChannelManagerIf::request_channels_param_t*)(p_if->get_source().get_message().data());
	if (!_param.p_out_channels || _param.array_max_num == 0) {
		p_if->reply (threadmgr::result::error);

	} else {
		int n = get_channels (_param.p_out_channels, _param.array_max_num);

		// reply msgで格納数を渡します
		p_if->reply (threadmgr::result::success, (uint8_t*)&n, sizeof(n));
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_get_transport_stream_name (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CChannelManagerIf::service_id_param_t _param =
				*(CChannelManagerIf::service_id_param_t*)(p_if->get_source().get_message().data());

	const char *pname = get_transport_stream_name (
							_param.transport_stream_id,
							_param.original_network_id
						);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		p_if->reply (threadmgr::result::success, (uint8_t*)pname, strlen(pname));
	} else {
		p_if->reply (threadmgr::result::error);
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_get_service_name (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	CChannelManagerIf::service_id_param_t _param =
				*(CChannelManagerIf::service_id_param_t*)(p_if->get_source().get_message().data());

	const char *pname = get_service_name (
							_param.transport_stream_id,
							_param.original_network_id,
							_param.service_id
						);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		p_if->reply (threadmgr::result::success, (uint8_t*)pname, strlen(pname));
	} else {
		p_if->reply (threadmgr::result::error);
	}


	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CChannelManager::on_dump_channels (threadmgr::CThreadMgrIf *p_if)
{
	threadmgr::section_id::type section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = threadmgr::section_id::init,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);


	dump_channels ();


	p_if->reply (threadmgr::result::success);

	section_id = threadmgr::section_id::init;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

uint16_t CChannelManager::get_pysical_channel_by_service_id (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		uint16_t ch = iter->first;
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				std::vector<CChannel::service>::const_iterator iter = p_ch->services.begin();
				for (; iter != p_ch->services.end(); ++ iter) {
					if (_service_id == iter->service_id) {
						return ch;
					}
				}
			}
		}
	}

	// not found
	return 0xffff;
}

uint16_t CChannelManager::get_pysical_channel_by_remote_control_key_id (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint8_t _remote_control_key_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		uint16_t ch = iter->first;
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
//TODO
// 暫定remote_control_key_idだけで判定
//   m_channels に同じremote_control_key_idのCChannel がある場合は
//   検索順で先のものが得られるので あまりよくないやりかたです
//   --> 引数 transport_stream_id, original_network_idで切り分けすれば問題ないはず
//   --> get_pysical_channel_by_service_id を使うべき
//				(p_ch->transport_stream_id == _transport_stream_id) &&
//				(p_ch->original_network_id == _original_network_id) &&
				(p_ch->remote_control_key_id == _remote_control_key_id)
			) {
				return ch;
			}
		}
	}

	// not found
	return 0xffff;
}

bool CChannelManager::is_duplicate_channel (const CChannel* p_channel) const
{
	if (!p_channel) {
		return false;
	}

	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (*p_ch == *p_channel) {
				// duplicate
				return true;
			}
		}
	}

	return false;
}

const CChannel* CChannelManager::find_channel (uint16_t pych) const
{
	std::map<uint16_t, CChannel>::const_iterator iter = m_channels.find (pych);

	if (iter == m_channels.end()) {
		return NULL;
	}

	return &(iter->second);
}

int CChannelManager::get_channels (CChannelManagerIf::channel_t *p_out_channels, int array_max_num) const
{
	if (!p_out_channels || array_max_num == 0) {
		return 0;
	}

	int n = 0;

	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			p_out_channels->pysical_channel = p_ch->pysical_channel;
			p_out_channels->transport_stream_id = p_ch->transport_stream_id;
			p_out_channels->original_network_id = p_ch->original_network_id;
			p_out_channels->remote_control_key_id = p_ch->remote_control_key_id;

			int m = 0;
			std::vector<CChannel::service>::const_iterator iter_svc = p_ch->services.begin();
			for (; iter_svc != p_ch->services.end(); ++ iter_svc) {
				p_out_channels->service_ids[m] = iter_svc->service_id;
				++ m;
			}
			p_out_channels->service_num = m;


			++ p_out_channels;

			++ n;
			if (array_max_num <= n) {
				break;
			}
		}
	}

	return n;
}

const char* CChannelManager::get_transport_stream_name (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				return p_ch->ts_name.c_str();
			}
		}
	}

	return NULL;
}

const char* CChannelManager::get_service_name (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				std::vector<CChannel::service>::const_iterator iter_svc = p_ch->services.begin();
				for (; iter_svc != p_ch->services.end(); ++ iter_svc) {
					if (iter_svc->service_id == _service_id) {
						return  iter_svc->service_name.c_str();
					}
				}
			}
		}
	}

	return NULL;
}

void CChannelManager::dump_channels (void) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			p_ch ->dump ();
		}
	}
}

void CChannelManager::dump_channels_simple (void) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			p_ch ->dump_simple ();
		}
	}
}



//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, CChannel::service &s)
{
	archive (cereal::make_nvp("service_id", s.service_id));
	archive (cereal::make_nvp("service_type", s.service_type));
	archive (cereal::make_nvp("service_name", s.service_name));
}

template <class Archive>
void serialize (Archive &archive, CChannel &r)
{
	archive (
		cereal::make_nvp("pysical_channel", r.pysical_channel),
		cereal::make_nvp("transport_stream_id", r.transport_stream_id),
		cereal::make_nvp("original_network_id", r.original_network_id),
		cereal::make_nvp("network_name", r.network_name),
		cereal::make_nvp("area_code", r.area_code),
		cereal::make_nvp("remote_control_key_id", r.remote_control_key_id),
		cereal::make_nvp("ts_name", r.ts_name),
		cereal::make_nvp("services", r.services)
	);
}

void CChannelManager::save_channels (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_channels));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getChannelsJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CChannelManager::load_channels (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getChannelsJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("channels.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_channels));

	ifs.close();
	ss.clear();
}
