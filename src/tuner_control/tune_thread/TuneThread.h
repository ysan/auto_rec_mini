#ifndef _TUNE_THREAD_H_
#define _TUNE_THREAD_H_

#include <functional>
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

	class Bps {
	public:
		explicit Bps (int interval_sec) : m_interval_sec(interval_sec) {
		}
		virtual ~Bps(void) = default;

		void start (uint64_t nr_bytes=0) {
			m_start_tp = std::chrono::steady_clock::now();
			m_nr_bytes = nr_bytes;
		}

		void update (uint64_t nr_bytes, std::function<void(float Bps)> interval_cb) {
			auto now = std::chrono::steady_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_tp).count();
			if (diff > m_interval_sec) {
				float Bps = (float)(nr_bytes - m_nr_bytes) / diff;
				interval_cb(Bps);
				start(nr_bytes);
			}
		}

	private:
		int m_interval_sec;
		std::chrono::steady_clock::time_point m_start_tp;
		uint64_t m_nr_bytes;
	};

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
