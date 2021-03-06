#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "CommandServerIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Settings.h"


using namespace ThreadManager;


class CCommandServer : public CThreadMgrBase
{
public:
	CCommandServer (char *pszName, uint8_t nQueNum);
	virtual ~CCommandServer (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);
	void serverLoop (CThreadMgrIf *pIf);


	int getClientFd (void);
	void connectionClose (void);

private:
	void serverLoop (void);
	int recvParseDelimiter (int fd, char *pszBuff, int buffSize, const char* pszDelim);
	bool parseDelimiter (char *pszBuff, int buffSize, const char *pszDelim);
	void parseCommand (char *pszBuff);
	void ignoreSigpipe (void);

	static void printSubTables (void);
	static void showList (const char *pszDesc);
	static void findCommand (const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase);

	// callbacks
	static void onCommandWaitBegin (void);
	static void onCommandLineAvailable (const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase);
	static void onCommandLineThrough (void);
	static void onCommandWaitEnd (void);


	int mClientfd;
	bool m_isConnectionClose;

};

#endif
