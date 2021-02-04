#ifndef _MODULES_H_
#define _MODULES_H_


#include "ThreadMgrBase.h"


using namespace ThreadManager;

typedef enum {
	EN_MODULE_TUNE_THREAD = 0,		// group0
	EN_MODULE_TUNE_THREAD_1,		// group1
	EN_MODULE_TUNE_THREAD_2,		// group2

	EN_MODULE_TUNER_CONTROL,		// group0
	EN_MODULE_TUNER_CONTROL_1,		// group1
	EN_MODULE_TUNER_CONTROL_2,		// group2

	EN_MODULE_PSISI_MANAGER,		// group0
	EN_MODULE_PSISI_MANAGER_1,		// group1
	EN_MODULE_PSISI_MANAGER_2,		// group2

	EN_MODULE_TUNER_SERVICE,
	EN_MODULE_REC_MANAGER,
	EN_MODULE_CHANNEL_MANAGER,
	EN_MODULE_EVENT_SCHEDULE_MANAGER,
	EN_MODULE_EVENT_SEARCH,
	EN_MODULE_COMMAND_SERVER,

	EN_MODULE_NUM,

} EN_MODULE;


extern CThreadMgrBase **getModules (void);
extern CThreadMgrBase *getModule (EN_MODULE enModule);

#endif
