#ifndef _CHANNEL_MANAGER_H_
#define _CHANNEL_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>

#include "coseqpp.h"

#include "utils.h"
#include "ts_arib_common.h"
#include "channel_manager_if.h"

#include "tuner_service_if.h"
#include "psisi_manager_if.h"

#include "cereal/cereal.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


namespace arec {
namespace channel {

class entry {
public:
	entry (void) {
		clear ();
	}
	virtual ~entry (void) {
		clear ();
	}


	bool operator == (const entry &obj) const {
		if (
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator != (const entry &obj) const {
		if (
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id
		) {
			return true;
		} else {
			return false;
		}
	}

	uint16_t physical_channel;

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	std::string network_name;

	uint16_t area_code;
	uint8_t remote_control_key_id;
	std::string ts_name;

	struct service {
		uint16_t service_id;
		uint8_t service_type;
		std::string service_name;

		void clear (void) {
			service_id = 0;
			service_type = 0;
			service_name.clear();
			service_name.shrink_to_fit();
		}
	};
	std::vector <service> services;

	void set (
		uint16_t _physical_channel,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		const char *psz_network_name,
		uint16_t _area_code,
		uint8_t _remote_control_key_id,
		const char *psz_ts_name,
		struct service _services[],
		int _services_num
	) {
		this->physical_channel = _physical_channel;
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		if (psz_network_name) {
			this->network_name = psz_network_name ;
		}
		this->area_code = _area_code;
		this->remote_control_key_id = _remote_control_key_id;
		if (psz_ts_name) {
			this->ts_name = psz_ts_name ;
		}
		if (_services && _services_num > 0) {
			for (int i = 0; i < _services_num; ++ i) {
				services.push_back(_services[i]);
			}
		}
	}

	void clear (void) {
		physical_channel = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		network_name.clear();
		network_name.shrink_to_fit();
		area_code = 0;
		remote_control_key_id = 0;
		ts_name.clear();
		ts_name.shrink_to_fit();

		std::vector<service>::iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			iter->clear();
		}
		services.clear();
		services.shrink_to_fit();
	}

	void dump (void) const {
		uint32_t freq = parser::ts_arib_common::physical_ch2freq_khz (physical_channel);
		_UTL_LOG_I ("-------------  physical channel:[%d] ([%d]k_hz) -------------", physical_channel, freq);
		_UTL_LOG_I (
			"pych:[%d] tsid:[0x%04x] org_nid:[0x%04x]",
			physical_channel,
			transport_stream_id,
			original_network_id
		);
		_UTL_LOG_I ("network_name:[%s]", network_name.c_str());
		_UTL_LOG_I (
			"area_code:[0x%04x] remote_control_key_id:[0x%02x] ts_name:[%s]",
			area_code,
			remote_control_key_id,
			ts_name.c_str()
		);
		_UTL_LOG_I ("----- services -----");
		std::vector<service>::const_iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			_UTL_LOG_I (
				"svcid:[0x%04x] svctype:[0x%02x] [%s]",
				iter->service_id,
				iter->service_type,
				iter->service_name.c_str()
			);
		}
		_UTL_LOG_I ("\n");
	}

	void dump_simple (void) const {
		char s [256] = {0};
		int n = 0;
		std::vector<service>::const_iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			n += snprintf (
				s + n,
				sizeof(s),
				"0x%04x,",
				iter->service_id
			);
		}

		_UTL_LOG_I (
//			"pych:[%d] tsid:[0x%04x] org_nid:[0x%04x] ts_name:[%s] remote_control_key_id:[0x%02x] svc_id:[%s]",
			"pych:[%d] tsid:[0x%04x] org_nid:[0x%04x] ts_name:[%s] remote_control_key_id:[0x%02x]",
			physical_channel,
			transport_stream_id,
			original_network_id,
			ts_name.c_str(),
			remote_control_key_id
//			remote_control_key_id,
//			s
		);
	}
};


class manager : public coseq::module_base
{
public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_channel_scan (coseq::iface *iface);
	void on_get_physical_channel_by_service_id (coseq::iface *iface);
	void on_get_physical_channel_by_remote_control_key_id (coseq::iface *iface);
	void on_get_service_id_by_physical_channel (coseq::iface *iface);
	void on_get_channels (coseq::iface *iface);
	void on_get_original_network_name (coseq::iface *iface);
	void on_get_transport_stream_name (coseq::iface *iface);
	void on_get_service_name (coseq::iface *iface);
	void on_dump_channels (coseq::iface *iface);


private:
	uint16_t get_physical_channel_by_service_id (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	) const ;

	uint16_t get_physical_channel_by_remote_control_key_id (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint8_t _remote_control_key_id
	) const;

	bool get_service_id_by_physical_channel (
		uint16_t _physical_channel,
		int _service_idx,
		client_iface::service_id_param_t *_out_service_id
	) const;

	bool is_duplicate_channel (const entry* p_channel) const;

	const entry* find_channel (uint16_t pych) const;

	int get_channels (client_iface::channel_t *p_out_channels, int array_max_num) const;
	const char* get_original_network_name (uint16_t _original_network_id) const;
	const char* get_transport_stream_name (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id
	) const;
	const char* get_service_name (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	) const;

	void dump_channels (void) const;
	void dump_channels_simple (void) const;

	void save_channels (void);
	void load_channels (void);


	std::map <uint16_t, entry> channels_; // <physical_channel, entry>

};

} // namespace channel
} // namespace arec

#endif
