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
#include "TsAribCommon.h" 


using namespace ThreadManager;


class CTunerService : public CThreadMgrBase
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
		}

		void dump (void) const {
			_UTL_LOG_I (
				"tuner_id:[0x%02x] module:[%d] priority:[%d] [%s]",
				tuner_id,
				module,
				static_cast<int>(priority),
				is_now_tuned ? "now_tuned" : "---"
			);
		}

		uint8_t tuner_id;
		EN_MODULE module;
		client_priority priority;
		bool is_now_tuned;

	} resource_t;

	// tune_withRetryの時に呼び元モジュールを取っておきたいため
	struct _tune_param : public CTunerServiceIf::tune_param_t {
		uint8_t caller_module;
	};

public:
	CTunerService (char *pszName, uint8_t nQueNum);
	virtual ~CTunerService (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_open (CThreadMgrIf *pIf);
	void onReq_close (CThreadMgrIf *pIf);
	void onReq_tune (CThreadMgrIf *pIf);
	void onReq_tune_withRetry (CThreadMgrIf *pIf);
	void onReq_tuneStop (CThreadMgrIf *pIf);
	void onReq_dumpAllocates (CThreadMgrIf *pIf);

private:
	uint8_t allocate (EN_MODULE module);
	bool release (uint8_t tuner_id);
	bool preCheck (uint8_t tuner_id, EN_MODULE module, bool is_req_tune) const;
	client_priority getPriority (EN_MODULE module) const;
	void dumpAllocates (void) const;

	int m_tuner_resource_max;
	std::vector <std::shared_ptr<resource_t>> m_resource_allcates;

};

#endif
