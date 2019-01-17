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
#include "CommandTables.h"


using namespace ThreadManager;


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
	int recvParseDelimiter (int fd, char *pszBuff, int buffSize, const char* pszDelim);
	bool parseDelimiter (char *pszBuff, int buffSize, const char *pszDelim);
	void parseCommand (char *pszBuff);

	static void showList (const char *pszDesc);
	static void findCommand (const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase);
	// callbacks
	static void onCommandWaitBegin (void);
	static void onCommandLineAvailable (const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase);
	static void onCommandLineThrough (void);
	static void onCommandWaitEnd (void);



	ST_SEQ_BASE mSeqs [EN_SEQ_COMMAND_SERVER_NUM]; // entity

	int mClientfd;
};

#endif
