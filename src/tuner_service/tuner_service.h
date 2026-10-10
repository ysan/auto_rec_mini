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

#include "coseqpp.h"

#include "utils.h"
#include "modules.h"
#include "group.h"
#include "settings.h"
#include "tuner_service_if.h"
#include "tuner_control_if.h"
#include "psisi_manager_if.h"
#include "channel_manager_if.h"
#include "ts_arib_common.h"


namespace arec {
namespace tuner_service {

class manager : public coseq::module_base
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
			const coseq::module_base *p_mod =
				(module_id != modules::module_id::max) ? get_module(module_id) : nullptr;
			const char *mod_name =
				p_mod              ? p_mod->get_name().c_str() : // 正規モジュール
				(tuner_id != 0xff) ? "EXTERNAL"                : // external(コマンド等)が確保
				                     "----------";               // 未割り当て
			_UTL_LOG_I (
				"tuner_id:[0x%02x] module:[%d:%.10s] priority:[%d] [%s]",
				tuner_id,
				static_cast<int>(module_id),
				mod_name,
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
	struct _tune_param : public client_iface::tune_param_t {
		modules::module_id caller_module_id;
	};

public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_open (coseq::iface *iface);
	void on_close (coseq::iface *iface);
	void on_tune (coseq::iface *iface);
	void on_tune_with_retry (coseq::iface *iface);
	void on_tune_stop (coseq::iface *iface);
	void on_dump_allocates (coseq::iface *iface);

private:
	uint8_t allocate_linear (modules::module_id module_id);
	uint8_t allocate_round_robin (modules::module_id module_id);
	bool release (uint8_t tuner_id);
	bool pre_check (uint8_t tuner_id, modules::module_id module_id, bool is_req_tune) const;
	client_priority get_priority (modules::module_id module_id) const;
	void dump_allocates (void) const;

	// external(非モジュール: command_server/http 等)からの要求は get_thread_idx() が
	// EXTERNAL(=0xff) になり正規の module_id にならない。範囲外は max(不明) に丸める。
	static modules::module_id caller_of (coseq::iface *iface) {
		modules::module_id id =
			static_cast<modules::module_id>(iface->get_source().get_thread_idx());
		return (id < modules::module_id::max) ? id : modules::module_id::max;
	}

	int tuner_resource_max_;
	std::vector <std::shared_ptr<resource_t>> resource_allcates_;
	uint8_t next_allocate_tuner_id_;

};

} // namespace tuner_service
} // namespace arec

#endif
