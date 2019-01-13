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


typedef struct command_info {
	char *pszCommand;
	char *pszDesc;
	void (*pcbCommand) (int argc, char* argv[]);
	struct command_info *pNext;

} ST_COMMAND_INFO;


class CCommandServer : public CThreadMgrBase
{
public:
	static const uint16_t SERVER_PORT;

public:
	CCommandServer (char *pszName, uint8_t nQueNum);
	virtual ~CCommandServer (void);


	void start (CThreadMgrIf *pIf);
	void serverLoop (CThreadMgrIf *pIf);


private:
	void serverLoop (void);
	int recvParseDelimiter (int fd, char *pszBuff, int buffSize, char* pszDelim, char **pszCarryover);
	int recvCheckDelimiter (int fd, char *pszBuff, int buffSize, char* pszDelim);
	int recvData (int fd, uint8_t *pBuff, int buffSize, bool *p_isDisconnect);
	void showList (const ST_COMMAND_INFO *pTable, const char *pszDesc);
	void parseCommandLoop (const ST_COMMAND_INFO *pTable, const char *pszConsole, const char *pszDesc);

	int mClientfd;


	ST_SEQ_BASE mSeqs [EN_SEQ_COMMAND_SERVER_NUM]; // entity

};

#endif
