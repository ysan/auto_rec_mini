#ifndef _TUNER_CONTROL_H_
#define _TUNER_CONTROL_H_

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


// notify category
#define _TUNER_NOTIFY							((uint8_t)0)

class CTunerControl : public threadmgr::CThreadMgrBase, public CGroup
{
public:
	CTunerControl (std::string name, uint8_t que_max, uint8_t group_id);
	virtual ~CTunerControl (void);


	void on_module_up (threadmgr::CThreadMgrIf *p_if);
	void on_module_down (threadmgr::CThreadMgrIf *p_if);

	void on_tune (threadmgr::CThreadMgrIf *p_if);
	void on_tune_start (threadmgr::CThreadMgrIf *p_if);
	void on_tune_stop (threadmgr::CThreadMgrIf *p_if);

	void on_register_tuner_notify (threadmgr::CThreadMgrIf *p_if);
	void on_unregister_tuner_notify (threadmgr::CThreadMgrIf *p_if);

	void on_register_ts_receive_handler (threadmgr::CThreadMgrIf *p_if);
	void on_unregister_ts_receive_handler (threadmgr::CThreadMgrIf *p_if);

	void on_get_state (threadmgr::CThreadMgrIf *p_if);



	std::mutex* get_mutex_ts_receive_handlers (void) {
		return &m_mutex_ts_receive_handlers;
	}

	CTunerControlIf::ITsReceiveHandler** get_ts_receive_handlers (void) {
		return mp_reg_ts_receive_handlers;
	}

private:
	int register_ts_receive_handler (CTunerControlIf::ITsReceiveHandler *p_handler);
	void unregister_ts_receive_handler (int id);
	void set_state (CTunerControlIf::tuner_state s);


	uint32_t m_freq;
	uint32_t m_wk_freq;
	uint32_t m_start_freq;
	int m_chkcnt;

	std::mutex m_mutex_ts_receive_handlers;
	CTunerControlIf::ITsReceiveHandler *mp_reg_ts_receive_handlers [TS_RECEIVE_HANDLER_REGISTER_NUM_MAX];

	CTunerControlIf::tuner_state m_state;
	bool m_is_req_stop;
};

#endif
