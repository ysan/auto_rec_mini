#ifndef _MODULES_H_
#define _MODULES_H_


#include "ThreadMgrBase.h"


using namespace ThreadManager;

typedef enum {
	EN_MODULE_COMMAND_SERVER = 0,
	EN_MODULE_TUNE_THREAD,
	EN_MODULE_TUNER_CONTROL,
	EN_MODULE_PSISI_MANAGER,

	EN_MODULE_NUM,

} EN_MODULE;


extern CThreadMgrBase **getModules (void);
extern CThreadMgrBase *getModule (EN_MODULE enModule);

#endif
