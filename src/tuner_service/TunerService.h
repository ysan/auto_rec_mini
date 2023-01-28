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
			,module (EN_MODULE_NUM)
			,priority (client_priority::OTHER)
			,is_now_tuned (false)
			,transport_stream_id (0)
			,original_network_id (0)
			,service_id (0)
		{
			clear ();
		}

		virtual ~_resource (void) {
		}

		void clear (void) {
			tuner_id = 0xff;
			module = EN_MODULE_NUM;
			priority = client_priority::OTHER;
			is_now_tuned = false;

			transport_stream_id = 0;
			original_network_id = 0;
			service_id = 0;
		}

		void dump (void) const {
			if (transport_stream_id != 0 || original_network_id != 0 || service_id != 0) {
				_UTL_LOG_I (
					"tuner_id:[0x%02x] module:[%d:%.10s] priority:[%d] [%s] -> tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x]",
					tuner_id,
					module,
					module != EN_MODULE_NUM ? getModule(module)->get_name() : "----------",
					static_cast<int>(priority),
					is_now_tuned ? "now_tuned" : "---",
					transport_stream_id,
					original_network_id,
					service_id
				);

			} else {
				_UTL_LOG_I (
					"tuner_id:[0x%02x] module:[%d:%.10s] priority:[%d] [%s]",
					tuner_id,
					module,
					module != EN_MODULE_NUM ? getModule(module)->get_name() : "----------",
					static_cast<int>(priority),
					is_now_tuned ? "now_tuned" : "---"
				);
			}
		}

		uint8_t tuner_id;
		EN_MODULE module;
		client_priority priority;
		bool is_now_tuned;

		//-- advance parameters --//
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint16_t service_id;

	} resource_t;

	// tune_withRetryの時に呼び元モジュールを取っておきたいため
	struct _tune_param : public CTunerServiceIf::tune_param_t {
		uint8_t caller_module;
	};
	// tuneAdvance_withRetryの時に呼び元モジュールを取っておきたいため
	struct _tune_advance_param : public CTunerServiceIf::tune_advance_param_t {
		uint8_t caller_module;
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
	void on_tune_advance (threadmgr::CThreadMgrIf *p_if);
	void on_tune_stop (threadmgr::CThreadMgrIf *p_if);
	void on_dump_allocates (threadmgr::CThreadMgrIf *p_if);

private:
	uint8_t allocate_linear (EN_MODULE module);
	uint8_t allocate_round_robin (EN_MODULE module);
	bool release (uint8_t tuner_id);
	bool pre_check (uint8_t tuner_id, EN_MODULE module, bool is_req_tune) const;
	client_priority get_priority (EN_MODULE module) const;
	void dump_allocates (void) const;

	int m_tuner_resource_max;
	std::vector <std::shared_ptr<resource_t>> m_resource_allcates;
	uint8_t m_next_allocate_tuner_id;

};

#endif
