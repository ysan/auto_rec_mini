#ifndef _VIEWING_MANAGER_H_
#define _VIEWING_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "etime.h"
#include "group.h"
#include "coseqpp.h"

#include "utils.h"
#include "settings.h"
#include "viewing_manager_if.h"
#include "stream_handler.h"
#include "ts_arib_common.h"

#include "tuner_control_if.h"
#include "tuner_service_if.h"
#include "psisi_manager_if.h"
#include "channel_manager_if.h"
#include "event_schedule_manager_if.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"


namespace arec {
namespace viewing {


class entry {
public:
	entry (void) {
		clear ();
	}
	virtual ~entry (void) {
		clear ();
	}

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	arec::common::etime start_time;
	arec::common::etime end_time;

	std::string title_name;
	std::string service_name;

	bool auto_stop;
	arec::common::etime auto_stop_time;

	uint8_t group_id;

	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		arec::common::etime* p_start_time,
		arec::common::etime* p_end_time,
		const char *psz_title_name,
		const char *psz_service_name,
		uint8_t group_id
	)
	{
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		this->service_id = _service_id;
		this->event_id = _event_id;
		if (p_start_time) {
			this->start_time = *p_start_time;
		}
		if (p_end_time) {
			this->end_time = *p_end_time;
		}
		if (psz_title_name) {
			this->title_name = psz_title_name ;
		}
		if (psz_service_name) {
			this->service_name = psz_service_name ;
		}
		this->group_id = group_id;
		this->is_used = true;
	}

	void clear (void) {
		transport_stream_id = 0;
		original_network_id = 0;
		service_id = 0;
		event_id = 0;
		start_time.clear();
		end_time.clear();	
		title_name.clear();
		title_name.shrink_to_fit();
		service_name.clear();
		service_name.shrink_to_fit();
		auto_stop = false;
		auto_stop_time.clear();
		group_id = 0xff;
		is_used = false;
	}

	void dump (void) const {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s]",
			start_time.to_string(),
			end_time.to_string()
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
		_UTL_LOG_I ("service_name:[%s]", service_name.c_str());
		if (auto_stop) {
			_UTL_LOG_I ("auto_stop_time:[%s]", auto_stop_time.to_string());
		}
		_UTL_LOG_I ("group_id:[0x%02x]", group_id);
	}
};

class manager : public coseq::module_base
{
public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_module_up_by_group_id (coseq::iface *iface);
	void on_check_loop (coseq::iface *iface);
	void on_notice_by_stream_handler (coseq::iface *iface);
	void on_start_viewing (coseq::iface *iface);
	void on_stop_viewing (coseq::iface *iface);
	void on_get_viewing (coseq::iface *iface);
	void on_set_option (coseq::iface *iface);
	void on_dump_viewing (coseq::iface *iface);
	void on_event_changed (coseq::iface *iface);

	void on_receive_notify (coseq::iface *iface) override;


private:
	void dump_viewing (void) const;
	bool set_nonblocking_io (int fd) const;
	void clean_dir (const char* path) const;
	void logging_stream_command (uint8_t group_id) const;

	arec::settings *settings_;
	
	uint8_t tuner_notify_client_id_ [arec::group::GROUP_MAX];
	uint8_t pat_detect_notify_client_id_ [arec::group::GROUP_MAX];
	uint8_t event_change_notify_client_id_ [arec::group::GROUP_MAX];

	entry viewings_ [arec::group::GROUP_MAX];
	arec::common::forker forker_ [arec::group::GROUP_MAX];

	int tuner_resource_max_;
};

} // namespace viewing
} // namespace arec

#endif
