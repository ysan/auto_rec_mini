#ifndef _CHANNEL_MANAGER_IF_H_
#define _CHANNEL_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_CHANNEL_MANAGER__MODULE_UP = 0,
	EN_SEQ_CHANNEL_MANAGER__MODULE_DOWN,
	EN_SEQ_CHANNEL_MANAGER__CHANNEL_SCAN,

	EN_SEQ_CHANNEL_MANAGER__NUM,
};


class CChannelManagerIf : public CThreadMgrExternalIf
{
public:
	explicit CChannelManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CChannelManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__MODULE_DOWN);
	};

	bool reqChannelScan (void) {
		return requestAsync (EN_MODULE_CHANNEL_MANAGER, EN_SEQ_CHANNEL_MANAGER__CHANNEL_SCAN);
	};

};

#endif
