#ifndef _PSISI_MANAGER_IF_H_
#define _PSISI_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_PSISI_MANAGER_MODULE_UP = 0,
	EN_SEQ_PSISI_MANAGER_MODULE_DOWN,
	EN_SEQ_PSISI_MANAGER_CHECK_LOOP,
	EN_SEQ_PSISI_MANAGER_CHECK_PAT,

	EN_SEQ_PSISI_MANAGER_NUM,
};


class CPsisiManagerIf : public CThreadMgrExternalIf
{
public:
	explicit CPsisiManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CPsisiManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_MODULE_DOWN);
	};

	bool reqCheckPAT (bool isCompleteNew) {
		bool _isCompNew = isCompleteNew;
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_CHECK_PAT, (uint8_t*)&_isCompNew, sizeof(_isCompNew));
	};


};

#endif
