#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"

#include "ThreadMgrBase.h"
#include "ThreadMgr.h"

#include "Utils.h"
#include "CommandServerIf.h"


using namespace ThreadManager;


class CCommandServer : public CThreadMgrBase
{
public:
	CCommandServer (char *pszName, uint8_t nQueNum);
	virtual ~CCommandServer (void);


	void startup (CThreadMgrIf *pIf);
	void recvLoop (CThreadMgrIf *pIf);


private:
	void recvLoop (void);

	ST_SEQ_BASE mSeqs [EN_SEQ_NUM]; // entity

};

#endif
