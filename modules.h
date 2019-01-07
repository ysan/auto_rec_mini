#ifndef _MODULES_H_
#define _MODULES_H_


#include "ThreadMgrBase.h"


using namespace ThreadManager;

enum {
	EN_MODULE_COMMAND_SERVER = 0,
	EN_MODULE_TUNE_THREAD,
	EN_MODULE_TUNER_CONTROL,


	EN_MODULE_NUM,
};


extern CThreadMgrBase *gp_modules [];


#endif
