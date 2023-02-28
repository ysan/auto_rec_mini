#ifndef _TUNER_SERVICE_H_
#define _TUNER_SERVICE_H_

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "modules.h"
#include "Group.h"
#include "Settings.h"
#include "TunerServiceIf.h"
#include "TunerControlIf.h"
#include "PsisiManagerIf.h"
#include "ChannelManagerIf.h"
#include "TsAribCommon.h" 


class CTunerService : public threadmgr::CThreadMgrBase
{
public:
	enum class client_priority : int {
		OTHER = 0,
		COMMAND,
		EVENT_SCHEDULE,
		LIVE_VIEW,
		CHANNEL_SCAN,
		RECORDING,
	};

	typedef struct _resource {
		_resource (void) 
			:tuner_id (0xff)
			,module_id (modules::module_id::max)
			,priority (client_priority::OTHER)
			,is_now_tuned (false)
		{
			clear ();
		}

		virtual ~_resource (void) {
		}

		void clear (void) {
			tuner_id = 0xff;
			module_id = modules::module_id::max;
			priority = client_priority::OTHER;
			is_now_tuned = false;
		}

		void dump (void) const {
			_UTL_LOG_I (
				"tuner_id:[0x%02x] module:[%d:%.10s] priority:[%d] [%s]",
				tuner_id,
				module_id,
				module_id != modules::module_id::max ? get_module(module_id)->get_name() : "----------",
				static_cast<int>(priority),
				is_now_tuned ? "now_tuned" : "---"
			);
		}

		uint8_t tuner_id;
		modules::module_id module_id;
		client_priority priority;
		bool is_now_tuned;
	} resource_t;

	// tune_withRetryの時に呼び元モジュールを取っておきたいため
	struct _tune_param : public CTunerServiceIf::tune_param_t {
		modules::module_id caller_module_id;
	};

public:
	CTunerService (std::string name, uint8_t que_max);
	virtual ~CTunerService (void);


	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);
	void on_open (threadmgr::CThreadMgrIf *p_if);
	void on_close (threadmgr::CThreadMgrIf *p_if);
	void on_tune (threadmgr::CThreadMgrIf *p_if);
	void on_tune_with_retry (threadmgr::CThreadMgrIf *p_if);
	void on_tune_stop (threadmgr::CThreadMgrIf *p_if);
	void on_dump_allocates (threadmgr::CThreadMgrIf *p_if);

private:
	uint8_t allocate_linear (modules::module_id module_id);
	uint8_t allocate_round_robin (modules::module_id module_id);
	bool release (uint8_t tuner_id);
	bool pre_check (uint8_t tuner_id, modules::module_id module_id, bool is_req_tune) const;
	client_priority get_priority (modules::module_id module_id) const;
	void dump_allocates (void) const;

	int m_tuner_resource_max;
	std::vector <std::shared_ptr<resource_t>> m_resource_allcates;
	uint8_t m_next_allocate_tuner_id;

};

#endif
