#ifndef _MODULES_H_
#define _MODULES_H_


#include "ThreadMgrBase.h"

namespace module {

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


extern threadmgr::CThreadMgrBase **get_modules (void);
extern threadmgr::CThreadMgrBase *get_module (module_id id);


}; // namespace module

#endif
