#ifndef _TUNE_THREAD_H_
#define _TUNE_THREAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <mutex>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Group.h"
#include "TunerControlIf.h"
#include "TsAribCommon.h"
#include "Settings.h"
#include "Forker.h"


class CTuneThread : public threadmgr::CThreadMgrBase, public CGroup
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		tune,
		force_kill,
		max,
	};

	enum class state : int {
		closed = 0,
		opened,
		tune_begined,
		tuned,
		tune_ended,
	};

	typedef struct tune_param {
		uint32_t freq;
		std::mutex * p_mutex_ts_receive_handlers;
		CTunerControlIf::ITsReceiveHandler ** p_ts_receive_handlers;
		bool *p_is_req_stop;
	} tune_param_t;

public:
	CTuneThread (std::string name, uint8_t que_max, uint8_t group_id);
	virtual ~CTuneThread (void);


	void on_module_up (threadmgr::CThreadMgrIf *pIf);
	void on_module_down (threadmgr::CThreadMgrIf *pIf);
	void on_tune (threadmgr::CThreadMgrIf *pIf);
	void on_force_kill (threadmgr::CThreadMgrIf *pIf);

	// direct getter
	state get_state (void) {
		return m_state;
	}

private:
	state m_state;
	CSettings *mp_settings;
	CForker m_forker;
};

#endif
