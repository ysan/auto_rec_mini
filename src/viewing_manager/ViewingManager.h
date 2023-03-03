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

#include "Group.h"
#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Settings.h"
#include "ViewingManagerIf.h"
#include "StreamHandler.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"
#include "TunerServiceIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"
#include "EventScheduleManagerIf.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"


class CViewing {
public:
	CViewing (void) {
		clear ();
	}
	virtual ~CViewing (void) {
		clear ();
	}

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	std::string title_name;
	std::string service_name;

	uint8_t group_id;

	bool is_used;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id,
		uint16_t _event_id,
		CEtime* p_start_time,
		CEtime* p_end_time,
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
			start_time.toString(),
			end_time.toString()
		);
		_UTL_LOG_I ("title:[%s]", title_name.c_str());
		_UTL_LOG_I ("service_name:[%s]", service_name.c_str());
		_UTL_LOG_I ("group_id:[0x%02x]", group_id);
	}
};

class CViewingManager : public threadmgr::CThreadMgrBase
{
public:
	CViewingManager (std::string name, uint8_t que_max);
	virtual ~CViewingManager (void);


	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);
	void on_module_up_by_group_id (threadmgr::CThreadMgrIf *p_if);
	void on_check_loop (threadmgr::CThreadMgrIf *p_if);
	void on_notice_by_stream_handler (threadmgr::CThreadMgrIf *p_if);
	void on_start_viewing (threadmgr::CThreadMgrIf *p_if);
	void on_stop_viewing (threadmgr::CThreadMgrIf *p_if);
	void on_get_viewing (threadmgr::CThreadMgrIf *p_if);
	void on_dump_viewing (threadmgr::CThreadMgrIf *p_if);
	void on_event_changed (threadmgr::CThreadMgrIf *p_if);

	void on_receive_notify (threadmgr::CThreadMgrIf *p_if) override;


private:
	void dump_viewing (void) const;
	bool set_nonblocking_io (int fd) const;
	void clean_dir (const char* path) const;
	void logging_stream_command (uint8_t group_id) const;

	CSettings *mp_settings;
	
	uint8_t m_tuner_notify_client_id [CGroup::GROUP_MAX];
	uint8_t m_pat_detect_notify_client_id [CGroup::GROUP_MAX];
	uint8_t m_event_change_notify_client_id [CGroup::GROUP_MAX];

	CViewing m_viewings [CGroup::GROUP_MAX];
	CForker m_forker [CGroup::GROUP_MAX];

	int m_tuner_resource_max;
};

#endif
