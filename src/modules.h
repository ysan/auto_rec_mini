#ifndef _MODULES_H_
#define _MODULES_H_

#include <vector>
#include <memory>

#include "coseqpp.h"

namespace modules {

enum class module_id : uint8_t {
	tune_thread = 0,	// group0
	tune_thread_1,		// group1
	tune_thread_2,		// group2
	tune_thread_3,		// group3

	tuner_control,		// group0
	tuner_control_1,	// group1
	tuner_control_2,	// group2
	tuner_control_3,	// group3

	psisi_manager,		// group0
	psisi_manager_1,	// group1
	psisi_manager_2,	// group2
	psisi_manager_3,	// group3

	tuner_service,
	rec_manager,
	channel_manager,
	event_schedule_manager,
	event_search,
	viewing_manager,
	command_server,

	max,
};


extern std::vector<std::shared_ptr<coseq::module_base>> &get_modules (void);
extern coseq::module_base *get_module (module_id id);

// 外部スレッド/ブロッキングなコマンド処理から他モジュールへ同期要求するための
// manager アクセサ(v1 の base->get_external_if() 相当を app 側で提供)。
// main が setup 後に set_manager() する。
extern void            set_manager (coseq::manager *mgr);
extern coseq::manager *get_manager (void);

// アプリ終了要求(v1 の mgr->wait() 相当を app 側で実現)。
// command_server の quit 系で request_shutdown() し、main がこれを待って teardown する。
extern void request_shutdown      (void);
extern bool is_shutdown_requested (void);


}; // namespace modules

#endif
