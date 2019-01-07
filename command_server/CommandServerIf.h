#ifndef _COMMAND_SERVER_IF_H_
#define _COMMAND_SERVER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_COMMAND_SERVER_START = 0,
	EN_SEQ_COMMAND_SERVER_RECV_LOOP,
	EN_SEQ_COMMAND_SERVER_NUM,
};


class CCommandServerIf : public CThreadMgrExternalIf
{
public:
	explicit CCommandServerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CCommandServerIf (void) {
	};


	bool reqStart (void) {
		return requestAsync (EN_MODULE_COMMAND_SERVER, EN_SEQ_COMMAND_SERVER_START);
	};
};

#endif
