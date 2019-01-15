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
	const char *pszCommand;
	const char *pszDesc;
	void (*pcbCommand) (int argc, char* argv[]);
	struct command_info *pNext;

} ST_COMMAND_INFO;


// callbacks
void (*on_command_wait_begin) (void);
void (*on_command_line_through) (void);
void (*on_command_line_available)(const char* pszCommand, int argc, char *argv[]);
void (*on_command_wait_end) (void);
static ST_COMMAND_INFO *gp_current_command_table;
static ST_COMMAND_INFO *gp_before_command_table;


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
	static void findCommand (const char* pszCommand, int argc, char *argv[]);



	ST_SEQ_BASE mSeqs [EN_SEQ_COMMAND_SERVER_NUM]; // entity

	int mClientfd;

	// callbacks
	static void onCommandWaitBegin (void);
	static void onCommandLineAvailable (const char* pszCommand, int argc, char *argv[]);
	static void onCommandLineThrough (void);
	static void onCommandWaitEnd (void);


};

#endif
