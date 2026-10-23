#ifndef _TUNE_THREAD_H_
#define _TUNE_THREAD_H_

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <mutex>

#include "coseqpp.h"

#include "utils.h"
#include "group.h"
#include "tuner_control_if.h"
#include "ts_arib_common.h"
#include "settings.h"
#include "forker.h"


namespace arec {
namespace tuner_control {

class tune_thread : public coseq::module_base, public arec::group
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
		client_iface::i_ts_receive_handler ** p_ts_receive_handlers;
		bool *p_is_req_stop;
	} tune_param_t;

	class Bps {
	public:
		explicit Bps (int interval_sec) : interval_sec_(interval_sec) {
		}
		virtual ~Bps(void) = default;

		void start (uint64_t nr_bytes=0) {
			start_tp_ = std::chrono::steady_clock::now();
			nr_bytes_ = nr_bytes;
		}

		void update (uint64_t nr_bytes, std::function<void(float Bps)> interval_cb) {
			auto now = std::chrono::steady_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - start_tp_).count();
			if (diff > interval_sec_) {
				float Bps = (float)(nr_bytes - nr_bytes_) / diff;
				interval_cb(Bps);
				start(nr_bytes);
			}
		}

	private:
		int interval_sec_;
		std::chrono::steady_clock::time_point start_tp_;
		uint64_t nr_bytes_;
	};

public:
	tune_thread (std::string name, uint8_t que_max, uint8_t group_id);
	virtual ~tune_thread (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_tune (coseq::iface *iface);
	void on_force_kill (coseq::iface *iface);

	// direct getter
	state get_state (void) {
		return state_;
	}

private:
	state state_;
	arec::settings *settings_;
	arec::common::forker forker_;
};

} // namespace tuner_control
} // namespace arec

#endif
