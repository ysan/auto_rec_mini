#ifndef _MODULEA_EXTERN_H_
#define _MODULEA_EXTERN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_STARTUP = 0,
	EN_SEQ_RECV_LOOP,
	EN_SEQ_NUM,
};


class CCommandServerIf : public CThreadMgrExternalIf
{
public:
	explicit CCommandServerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CCommandServerIf (void) {
	};


	bool reqStartup (void) {
		return requestAsync (EN_MODULE_COMMAND_SERVER, EN_SEQ_STARTUP);
	};
};

#endif
