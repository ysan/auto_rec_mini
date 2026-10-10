#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "channel_manager.h"
#include "channel_manager_if.h"
#include "modules.h"

#include "settings.h"


namespace arec {
namespace channel {

manager::manager (std::string name, uint8_t que_max)
	:coseq::module_base (name, que_max)
{
	const int _max = static_cast<int>(client_iface::sequence::max);
	coseq::sequence_t seqs [_max] = {
		{[&](coseq::iface *iface){manager::on_module_up(iface);}, std::string("on_module_up")},
		{[&](coseq::iface *iface){manager::on_module_down(iface);}, std::string("on_module_down")},
		{[&](coseq::iface *iface){manager::on_channel_scan(iface);}, std::string("on_channel_scan")},
		{[&](coseq::iface *iface){manager::on_get_physical_channel_by_service_id(iface);}, std::string("on_get_physical_channel_by_service_id")},
		{[&](coseq::iface *iface){manager::on_get_physical_channel_by_remote_control_key_id(iface);}, std::string("on_get_physical_channel_by_remote_control_key_id")},
		{[&](coseq::iface *iface){manager::on_get_service_id_by_physical_channel(iface);}, std::string("on_get_service_id_by_physical_channel")},
		{[&](coseq::iface *iface){manager::on_get_channels(iface);}, std::string("on_get_channels")},
		{[&](coseq::iface *iface){manager::on_get_original_network_name(iface);}, std::string("on_get_original_network_name")},
		{[&](coseq::iface *iface){manager::on_get_transport_stream_name(iface);}, std::string("on_get_transport_stream_name")},
		{[&](coseq::iface *iface){manager::on_get_service_name(iface);}, std::string("on_get_service_name")},
		{[&](coseq::iface *iface){manager::on_dump_channels(iface);}, std::string("on_dump_channels")},
	};
	set_sequences (seqs, _max);


	channels_.clear ();

}

manager::~manager (void)
{
	channels_.clear ();

	reset_sequences();
}


void manager::on_module_up (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	std::string path = arec::settings::get_instance()->get_params().get_channels_json_path();
	arec::common::utils::makedir(path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH, true);

	load_channels ();
	dump_channels_simple ();

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

void manager::on_channel_scan (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());

	iface->lock();

	// 先にreplyしておきます(以降はスレッド占有のスキャン)
	iface->reply (coseq::result::success);

	channels_.clear ();

	// tuner open
	uint8_t group_id = 0xff;
	{
		coseq::source &r = iface->request (
			static_cast<uint8_t>(modules::module_id::tuner_service),
			static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::open)
		);
		if (r.get_result() != coseq::result::success) {
			_UTL_LOG_E ("someone is using a tuner.");
			iface->unlock();
			return;
		}
		group_id = *(uint8_t*)(r.get_message().data());
		_UTL_LOG_I ("request_open group_id:[0x%02x]", group_id);
	}

	// 全 UHF 物理チャンネルを走査
	for (uint16_t ch = UHF_PHYSICAL_CHANNEL_MIN; ch <= UHF_PHYSICAL_CHANNEL_MAX; ++ ch) {

		// 1実行専用のスクラッチはループローカルへ(ポインタ渡し先バッファ含む)
		psisi_structs::network_info_t network_info = {0};
		psisi_structs::service_info_t service_infos[10];
		memset (&service_infos, 0x00, sizeof(service_infos));

		uint32_t freq = parser::ts_arib_common::physical_ch2freq_khz (ch);
		_UTL_LOG_I ("(%s) ------  physical channel:[%d] -> freq:[%d]k_hz", iface->get_sequence_name(), ch, freq);

		// tune
		{
			arec::tuner_service::client_iface::tune_param_t param = { ch, group_id };
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::tuner_service),
				static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune),
				(uint8_t*)&param, sizeof(param)
			);
			if (r.get_result() != coseq::result::success) {
				_UTL_LOG_W ("tune is failure -> skip");
				// tune stop (結果は見ない)
				iface->request (static_cast<uint8_t>(modules::module_id::tuner_service),
								static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop),
								(uint8_t*)&group_id, sizeof(group_id));
				continue;
			}
		}

		// get current network info
		{
			psisi_structs::request_network_info_param_t nparam = { &network_info };
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
				static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_current_network_info),
				(uint8_t*)&nparam, sizeof(nparam)
			);
			if (r.get_result() != coseq::result::success) {
				_UTL_LOG_W ("network info is not found -> skip");
				iface->request (static_cast<uint8_t>(modules::module_id::tuner_service),
								static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop),
								(uint8_t*)&group_id, sizeof(group_id));
				continue;
			}
		}

		// get current service infos
		{
			psisi_structs::request_service_infos_param_t sparam = { service_infos, 10 };
			coseq::source &r = iface->request (
				static_cast<uint8_t>(modules::module_id::psisi_manager) + group_id,
				static_cast<uint8_t>(arec::psisi::client_iface::sequence::get_current_service_infos),
				(uint8_t*)&sparam, sizeof(sparam)
			);
			if (r.get_result() == coseq::result::success) {
				int n_svc = *(int*)(r.get_message().data());
				if (n_svc > 0) {

					entry::service _services [10];
					for (int i = 0; i < n_svc; ++ i) {
						_services[i].service_id = service_infos[i].service_id;
						_services[i].service_type = service_infos[i].service_type;
						_services[i].service_name = service_infos[i].p_service_name_char;
					}

					entry rec;
					rec.set (
						ch,
						network_info.transport_stream_id,
						network_info.original_network_id,
						(const char*)network_info.network_name_char,
						network_info.area_code,
						network_info.remote_control_key_id,
						(const char*)network_info.ts_name_char,
						_services,
						n_svc
					);

					if (!is_duplicate_channel (&rec)) {
						rec.dump();
						channels_.insert (std::pair<uint16_t, entry>(ch, rec));
					}

				} else {
					_UTL_LOG_W ("request_get_current_service_infos  num is 0 -> skip");
				}
			} else {
				_UTL_LOG_W ("service infos is not found -> skip");
			}
		}

		// tune stop (結果は見ない)
		iface->request (static_cast<uint8_t>(modules::module_id::tuner_service),
						static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop),
						(uint8_t*)&group_id, sizeof(group_id));
	}

	// END
	iface->unlock();

	dump_channels ();
	save_channels ();

	_UTL_LOG_I ("channel scan end.");

	// 選局を停止・クローズしときます(投げっぱなし)
	iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
					static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::tune_stop),
					(uint8_t*)&group_id, sizeof(group_id));
	iface->send (static_cast<uint8_t>(modules::module_id::tuner_service),
					static_cast<uint8_t>(arec::tuner_service::client_iface::sequence::close),
					(uint8_t*)&group_id, sizeof(group_id));
}

void manager::on_get_physical_channel_by_service_id (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::service_id_param_t param =
			*(client_iface::service_id_param_t*)(iface->get_source().get_message().data());

	uint16_t _ch = get_physical_channel_by_service_id (
						param.transport_stream_id,
						param.original_network_id,
						param.service_id
					);
	if (_ch == 0xffff) {

		_UTL_LOG_E ("get_physical_channel_by_service_id is failure.");
		iface->reply (coseq::result::error);

	} else {

		// リプライmsgに結果を乗せます
		iface->reply (coseq::result::success, (uint8_t*)&_ch, sizeof(_ch));

	}


}

void manager::on_get_physical_channel_by_remote_control_key_id (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::remote_control_id_param_t param =
			*(client_iface::remote_control_id_param_t*)(iface->get_source().get_message().data());

	uint16_t _ch = get_physical_channel_by_remote_control_key_id (
						param.transport_stream_id,
						param.original_network_id,
						param.remote_control_key_id
					);
	if (_ch == 0xffff) {

		_UTL_LOG_E ("get_physical_channel_by_remote_control_key_id is failure.");
		iface->reply (coseq::result::error);

	} else {

		// リプライmsgに結果を乗せます
		iface->reply (coseq::result::success, (uint8_t*)&_ch, sizeof(_ch));

	}


}

void manager::on_get_service_id_by_physical_channel (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::request_service_id_param_t param =
			*(reinterpret_cast<client_iface::request_service_id_param_t*>(iface->get_source().get_message().data()));

	client_iface::service_id_param_t _out;
	bool r = get_service_id_by_physical_channel (
						param.physical_channel,
						param.service_idx,
						&_out
					);
	if (!r) {

		_UTL_LOG_E ("get_service_id_by_physical_channel is failure.");
		iface->reply (coseq::result::error);

	} else {

		// リプライmsgに結果を乗せます
		iface->reply (coseq::result::success, (uint8_t*)&_out, sizeof(_out));

	}


}

void manager::on_get_channels (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::request_channels_param_t _param =
				*(client_iface::request_channels_param_t*)(iface->get_source().get_message().data());
	if (!_param.p_out_channels || _param.array_max_num == 0) {
		iface->reply (coseq::result::error);

	} else {
		int n = get_channels (_param.p_out_channels, _param.array_max_num);

		// reply msgで格納数を渡します
		iface->reply (coseq::result::success, (uint8_t*)&n, sizeof(n));
	}


}

void manager::on_get_original_network_name (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::service_id_param_t _param =
				*(client_iface::service_id_param_t*)(iface->get_source().get_message().data());

	const char *pname = get_original_network_name (_param.original_network_id);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		iface->reply (coseq::result::success, (uint8_t*)pname, strlen(pname));
	} else {
		iface->reply (coseq::result::error);
	}


}

void manager::on_get_transport_stream_name (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::service_id_param_t _param =
				*(client_iface::service_id_param_t*)(iface->get_source().get_message().data());

	const char *pname = get_transport_stream_name (
							_param.transport_stream_id,
							_param.original_network_id
						);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		iface->reply (coseq::result::success, (uint8_t*)pname, strlen(pname));
	} else {
		iface->reply (coseq::result::error);
	}


}

void manager::on_get_service_name (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	client_iface::service_id_param_t _param =
				*(client_iface::service_id_param_t*)(iface->get_source().get_message().data());

	const char *pname = get_service_name (
							_param.transport_stream_id,
							_param.original_network_id,
							_param.service_id
						);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		iface->reply (coseq::result::success, (uint8_t*)pname, strlen(pname));
	} else {
		iface->reply (coseq::result::error);
	}


}

void manager::on_dump_channels (coseq::iface *iface)
{
	_UTL_LOG_D ("(%s)\n", iface->get_sequence_name());


	dump_channels ();


	iface->reply (coseq::result::success);

}

uint16_t manager::get_physical_channel_by_service_id (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		uint16_t ch = iter->first;
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				std::vector<entry::service>::const_iterator iter = p_ch->services.begin();
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

uint16_t manager::get_physical_channel_by_remote_control_key_id (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint8_t _remote_control_key_id
) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		uint16_t ch = iter->first;
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			if (
//TODO
// 暫定remote_control_key_idだけで判定
//   channels_ に同じremote_control_key_idのentry がある場合は
//   検索順で先のものが得られるので あまりよくないやりかたです
//   --> 引数 transport_stream_id, original_network_idで切り分けすれば問題ないはず
//   --> get_physical_channel_by_service_id を使うべき
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

bool manager::get_service_id_by_physical_channel (
	uint16_t _physical_channel,
	int _service_idx,
	client_iface::service_id_param_t *_out_service_id
) const
{
	if (!_out_service_id) {
		return false;
	}

	if (_physical_channel < UHF_PHYSICAL_CHANNEL_MIN || _physical_channel > UHF_PHYSICAL_CHANNEL_MAX) {
		_UTL_LOG_E ("get_service_id_by_physical_channel physical_channel[%d] is not UHF_PHYSICAL_CHANNEL...", _physical_channel);
		return false;
	}

	const entry* ch = find_channel(_physical_channel);
	if (!ch) {
		return false;
	}

	if ((int)(ch->services.size() -1) < _service_idx) {
		_UTL_LOG_E ("get_service_id_by_physical_channel service_idx[%d] is overflow. services.size[%d]", _service_idx, ch->services.size());
		return false;
	}

	_out_service_id->transport_stream_id = ch->transport_stream_id;
	_out_service_id->original_network_id = ch->original_network_id;
	_out_service_id->service_id = ch->services[_service_idx].service_id;

	return true;
}

bool manager::is_duplicate_channel (const entry* p_channel) const
{
	if (!p_channel) {
		return false;
	}

	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			if (*p_ch == *p_channel) {
				// duplicate
				return true;
			}
		}
	}

	return false;
}

const entry* manager::find_channel (uint16_t pych) const
{
	std::map<uint16_t, entry>::const_iterator iter = channels_.find (pych);

	if (iter == channels_.end()) {
		return NULL;
	}

	return &(iter->second);
}

int manager::get_channels (client_iface::channel_t *p_out_channels, int array_max_num) const
{
	if (!p_out_channels || array_max_num == 0) {
		return 0;
	}

	int n = 0;

	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			p_out_channels->physical_channel = p_ch->physical_channel;
			p_out_channels->transport_stream_id = p_ch->transport_stream_id;
			p_out_channels->original_network_id = p_ch->original_network_id;
			p_out_channels->remote_control_key_id = p_ch->remote_control_key_id;

			int m = 0;
			std::vector<entry::service>::const_iterator iter_svc = p_ch->services.begin();
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

const char* manager::get_original_network_name (
	uint16_t _original_network_id
) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			if (p_ch->original_network_id == _original_network_id) {
				return p_ch->network_name.c_str();
			}
		}
	}

	return NULL;
}

const char* manager::get_transport_stream_name (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id
) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
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

const char* manager::get_service_name (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				std::vector<entry::service>::const_iterator iter_svc = p_ch->services.begin();
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

void manager::dump_channels (void) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			p_ch ->dump ();
		}
	}
}

void manager::dump_channels_simple (void) const
{
	std::map <uint16_t, entry>::const_iterator iter = channels_.begin();
	for (; iter != channels_.end(); ++ iter) {
		entry const *p_ch = &(iter->second);
		if (p_ch) {
			p_ch ->dump_simple ();
		}
	}
}



//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, entry::service &s)
{
	archive (cereal::make_nvp("service_id", s.service_id));
	archive (cereal::make_nvp("service_type", s.service_type));
	archive (cereal::make_nvp("service_name", s.service_name));
}

template <class Archive>
void serialize (Archive &archive, entry &r)
{
	archive (
		cereal::make_nvp("physical_channel", r.physical_channel),
		cereal::make_nvp("transport_stream_id", r.transport_stream_id),
		cereal::make_nvp("original_network_id", r.original_network_id),
		cereal::make_nvp("network_name", r.network_name),
		cereal::make_nvp("area_code", r.area_code),
		cereal::make_nvp("remote_control_key_id", r.remote_control_key_id),
		cereal::make_nvp("ts_name", r.ts_name),
		cereal::make_nvp("services", r.services)
	);
}

void manager::save_channels (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (cereal::make_nvp("m_channels", channels_));
	}

	std::string path = arec::settings::get_instance()->get_params().get_channels_json_path();
	std::ofstream ofs (path.c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void manager::load_channels (void)
{
	std::string path = arec::settings::get_instance()->get_params().get_channels_json_path();
	std::ifstream ifs (path.c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("channels.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (cereal::make_nvp("m_channels", channels_));

	ifs.close();
	ss.clear();
}

} // namespace channel
} // namespace arec
