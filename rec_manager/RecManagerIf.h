#ifndef _REC_MANAGER_IF_H_
#define _REC_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_REC_MANAGER_MODULE_UP = 0,
	EN_SEQ_REC_MANAGER_MODULE_DOWN,

	EN_SEQ_REC_MANAGER_NUM,
};


class CRecManagerIf : public CThreadMgrExternalIf
{
public:
	explicit CRecManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CRecManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER_MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER_MODULE_DOWN);
	};

};

#endif
