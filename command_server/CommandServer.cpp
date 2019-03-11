#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include "CommandServer.h"
#include "CommandServerIf.h"

#include "modules.h"


// callbacks
static void (*on_command_wait_begin) (void);
static void (*on_command_line_through) (void);
static void (*on_command_line_available)(const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase);
static void (*on_command_wait_end) (void);
static ST_COMMAND_INFO *gp_current_command_table = NULL;
static FILE *gp_fptr_inner = NULL;


const uint16_t CCommandServer::SERVER_PORT = 20001;

CCommandServer::CCommandServer (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,mClientfd (0)
{
	mSeqs [EN_SEQ_COMMAND_SERVER_MODULE_UP]   = {(PFN_SEQ_BASE)&CCommandServer::moduleUp, (char*)"moduleUp"};
	mSeqs [EN_SEQ_COMMAND_SERVER_MODULE_DOWN] = {(PFN_SEQ_BASE)&CCommandServer::moduleDown, (char*)"moduleDown"};
	mSeqs [EN_SEQ_COMMAND_SERVER_SERVER_LOOP] = {(PFN_SEQ_BASE)&CCommandServer::serverLoop, (char*)"serverLoop"};
	setSeqs (mSeqs, EN_SEQ_COMMAND_SERVER_NUM);


	on_command_wait_begin = onCommandWaitBegin;
	on_command_line_through = onCommandLineThrough;
	on_command_line_available = onCommandLineAvailable;
	on_command_wait_end = onCommandWaitEnd;
	gp_current_command_table = NULL;
	gp_fptr_inner = stdout;
}

CCommandServer::~CCommandServer (void)
{
}


void CCommandServer::moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	pIf->reply (EN_THM_RSLT_SUCCESS);


	uint32_t opt = getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;	
	setRequestOption (opt);

	requestAsync (EN_MODULE_COMMAND_SERVER, EN_SEQ_COMMAND_SERVER_SERVER_LOOP);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	setRequestOption (opt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CCommandServer::moduleDown (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CCommandServer::serverLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_I ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	pIf->reply (EN_THM_RSLT_SUCCESS);


	serverLoop ();


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CCommandServer::serverLoop (void)
{
	int r = 0;
	int fdSockSv = 0;
	char szBuff [128];
	uint16_t port = 0;
	struct sockaddr_in stAddrSv;
	struct sockaddr_in stAddrCl;
	socklen_t addrLenCl = sizeof(struct sockaddr_in);

	memset (szBuff, 0x00, sizeof(szBuff));
	memset (&stAddrSv, 0x00, sizeof(struct sockaddr_in));
	memset (&stAddrCl, 0x00, sizeof(struct sockaddr_in));

	port = SERVER_PORT;
	stAddrSv.sin_family = AF_INET;
	stAddrSv.sin_addr.s_addr = htonl (INADDR_ANY);
	stAddrSv.sin_port = htons (port);


	if ((fdSockSv = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		_UTL_PERROR ("socket()");
		return;
	}

	int optval = 1;
	r = setsockopt (fdSockSv, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (r < 0) {
		_UTL_PERROR ("setsockopt()");
		close (fdSockSv);
		return;
	}

	if (bind(fdSockSv, (struct sockaddr*)&stAddrSv, sizeof(struct sockaddr_in)) < 0) {
		_UTL_PERROR ("bind()");
		close (fdSockSv);
		return;
	}

	if (listen (fdSockSv, SOMAXCONN) < 0) {
		_UTL_PERROR ("listen()");
		close (fdSockSv);
		return;
	}


	while (1) {
		_UTL_LOG_I ("accept blocking...");

		if ((mClientfd = accept (fdSockSv, (struct sockaddr*)&stAddrCl, &addrLenCl)) < 0 ) {
			_UTL_PERROR ("accept()");
			close (fdSockSv);
			return;
		}
			
		_UTL_LOG_I (
			"clientAddr:[%s] SocketFd:[%d] --- connected.\n",
			inet_ntoa(stAddrCl.sin_addr),
			mClientfd
		);

		int fd_copy = dup (mClientfd);
		FILE *fp = fdopen (fd_copy, "w");
		CUtils::setLogFileptr (fp);
		setLogFileptr (fp);
		gp_fptr_inner = fp;


		// begin
		if (on_command_wait_begin) {
			on_command_wait_begin ();
		}

		while (1) {
			r = recvParseDelimiter (mClientfd, szBuff, sizeof(szBuff), (char*)"\r\n");
			if (r <= 0) {
				break;
			}
		}

		// end
		if (on_command_wait_end) {
			on_command_wait_end ();
		}


		CUtils::setLogFileptr (stdout);
		setLogFileptr (stdout);
		gp_fptr_inner = stdout;

		fclose (fp);
		close (mClientfd);

		_UTL_LOG_I (
			"client:[%s] fd:[%d] --- disconnected.\n",
			inet_ntoa(stAddrCl.sin_addr),
			mClientfd
		);

		mClientfd = 0;
	}
}

int CCommandServer::recvParseDelimiter (int fd, char *pszBuff, int buffSize, const char* pszDelim)
{
	if (!pszBuff || buffSize <= 0) {
		return -1;
	}
	if (!pszDelim || (int)strlen(pszDelim) == 0) {
		return -1;
	}

	int rSize = 0;
	int rSizeTotal = 0;
	bool isDisconnect = false;

	bool is_parse_remain = false;
	int offset = 0;

	while (1) {
		if (is_parse_remain) {
			offset = (int)strlen(pszBuff);
		} else {
			memset (pszBuff, 0x00, buffSize);
			offset = 0;
		}

		rSize = CUtils::recvData (fd, (uint8_t*)pszBuff + offset, buffSize - offset, &isDisconnect);
		if (rSize < 0) {
			return -1;

		} else if (rSize == 0) {
			return 0;

		} else {
			// recv ok
			rSizeTotal += rSize;


			is_parse_remain = parseDelimiter (pszBuff, buffSize, pszDelim);


			if (isDisconnect) {
				break;
			}
		}
	}

	return rSizeTotal;
}

bool CCommandServer::parseDelimiter (char *pszBuff, int buffSize, const char *pszDelim)
{
	if (!pszBuff || (int)strlen(pszBuff) == 0 || buffSize <= 0) {
		return NULL;
	}
	if (!pszDelim || (int)strlen(pszDelim) == 0) {
		return NULL;
	}

	bool is_remain = false;
	char *token = NULL;
	char *saveptr = NULL;
	char *s = pszBuff;
	bool is_tail_delim = false;
	int n = 0;

	while (1) {
		token = strtok_r_impl (s, pszDelim, &saveptr, &is_tail_delim);
		if (token == NULL) {
			if (n == 0) {
				// not yet recv delimiter
				// carryover remain
//printf ("=> carryover  pszBuff[%s]\n", pszBuff);
				is_remain = true;

			} else {
				// end
			}
			break;
		}

//printf ("[%s]\n", token);
		if ((int)strlen(token) == 0) {
			// through
			if (on_command_line_through) {
				on_command_line_through ();
			}

		} else {
			if ((int)strlen(saveptr) > 0) {
				// got
				// next
				CUtils::deleteHeadSp (token);
				CUtils::deleteTailSp (token);
//printf ("=> GOT - exist next [%s]\n", token);
				parseCommand (token);

			} else {
				// got
				// last

				if (is_tail_delim) {
					// tail delimiter
					CUtils::deleteHeadSp (token);
					CUtils::deleteTailSp (token);
//printf ("-> GOT - last tail delimiter [%s]\n", token);
					parseCommand (token);

				} else {
					// not yet recv delimiter
					// carryover remain
					strncpy (pszBuff, token, buffSize);
//printf ("=> carryover  pszBuff[%s]\n", pszBuff);
					is_remain = true;
				}
			}
		} 

		s = NULL;
		++ n;
	}

	return is_remain;
}

void CCommandServer::parseCommand (char *pszBuff)
{
	if (!pszBuff || ((int)strlen(pszBuff) == 0)) {
		return ;
	}

	const char *pszDelim = (const char*)" ";

	char szTmp [(int)strlen(pszBuff) + 1] = {0};
	strncpy (szTmp, pszBuff, sizeof(szTmp));

	// count args
	char *str = szTmp;
	char *token = NULL;
	char *saveptr = NULL;
	int n_arg = 0;
	while (1) {
		token = strtok_r(str, pszDelim, &saveptr);
		if (!token) {
			break;
		}
		++ n_arg;
		str = NULL;
	}

	if (n_arg == 0) {
		return;

	} else if (n_arg == 1) {
		if (on_command_line_available) {
			on_command_line_available (pszBuff, 0, NULL, this);
		}

	} else {
		// n_arg >= 2
		char *p_command = NULL;
		int argc = n_arg -1;
		char *argv[n_arg -1];

		// parse args
		char *str = pszBuff;
		char *token = NULL;
		char *saveptr = NULL;
		int n = 0;
		while (1) {
			token = strtok_r(str, pszDelim, &saveptr);
			if (!token) {
				break;
			}

			if (n == 0) {
				p_command = token;
			} else {
				argv[n-1] = token;
			}
			++ n;
			str = NULL;
		}

		if (on_command_line_available) {
			on_command_line_available (p_command, argc, argv, this);
		}
	}
}



#define N (16)
static ST_COMMAND_INFO *stack [N];
static int sp = 0;
static void push (ST_COMMAND_INFO *p)
{
	if (sp == N) {
		return;
	}

	stack [sp] = p;
	//printf ("push %d %p\n", sp, stack [sp]);
	++ sp;
}

static ST_COMMAND_INFO *pop (void)
{
	if (sp == 0) {
		return NULL;
	}

	ST_COMMAND_INFO *r = stack [sp -1];
	//printf ("pop %d %p\n", sp -1, stack [sp -1]);
	stack [sp -1] = NULL;
	-- sp; 
	return r;
}

static ST_COMMAND_INFO *peep (void)
{
	if (sp == 0) {
		return NULL;
	}
	return stack [sp -1];
}

static ST_COMMAND_INFO *stack_sub [N];
static int sp_sub = 0;
static void push_sub (ST_COMMAND_INFO *p)
{
	if (sp_sub == N) {
		return;
	}

	stack_sub [sp_sub] = p;
	++ sp_sub;
}

static ST_COMMAND_INFO *pop_sub (void)
{
	if (sp_sub == 0) {
		return NULL;
	}

	ST_COMMAND_INFO *r = stack_sub [sp_sub -1];
	stack_sub [sp_sub -1] = NULL;
	-- sp_sub; 
	return r;
}

static ST_COMMAND_INFO *peep_sub (void)
{
	if (sp_sub == 0) {
		return NULL;
	}
	return stack_sub [sp_sub -1];
}

static void peep_sub_tables_print (void)
{
	if (sp_sub < 1) {
		fprintf (gp_fptr_inner, "/");
		return;
	}

	for (int i = 0; i < sp_sub; ++ i) {
		fprintf (gp_fptr_inner, "/%s", stack_sub[i]->pszDesc);
	}
}

void CCommandServer::showList (const char *pszDesc)
{
	const ST_COMMAND_INFO *pWorkTable = gp_current_command_table;

	fprintf (gp_fptr_inner, "\n  ------ %s ------\n", pszDesc ? pszDesc: "???");

	int i = 0;
	for (i = 0; pWorkTable->pszCommand; i ++) {
		fprintf (gp_fptr_inner, "  %-20s -- %-30s\n", pWorkTable->pszCommand, pWorkTable->pszDesc);
		++ pWorkTable;
	}
	fprintf (gp_fptr_inner, "\n");

	peep_sub_tables_print();
	fprintf (gp_fptr_inner, " > ");
	fflush (gp_fptr_inner);

	fflush (gp_fptr_inner);
}

void CCommandServer::findCommand (const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase)
{
	if (((int)strlen("..") == (int)strlen(pszCommand)) && strncmp ("..", pszCommand, (int)strlen(pszCommand)) == 0) {

		ST_COMMAND_INFO *p = pop ();
		pop_sub();
		if (p) {
			gp_current_command_table = p;

			if (gp_current_command_table == g_rootCommandTable) {
				showList ("root table");
			} else{
				showList (peep_sub()->pszDesc);
			}
		}

	} else if ((int)strlen(".") == (int)strlen(pszCommand) && strncmp (".", pszCommand, (int)strlen(pszCommand)) == 0) {

		if (gp_current_command_table == g_rootCommandTable) {
			showList ("root table");
		} else{
			showList (peep_sub()->pszDesc);
		}

	} else {
		ST_COMMAND_INFO *pWorkTable = gp_current_command_table;
		bool isMatch = false;

		while (pWorkTable->pszCommand) {
			if (
				((int)strlen(pWorkTable->pszCommand) == (int)strlen(pszCommand)) &&
				(strncmp (pWorkTable->pszCommand, pszCommand, (int)strlen(pszCommand)) == 0)
			) {
				// コマンドが一致した
				isMatch = true;

				if (pWorkTable->pcbCommand) {

					// コマンド実行
					(void) (pWorkTable->pcbCommand) (argc, argv, pBase);

				} else {
					// 下位テーブル
					if (pWorkTable->pNext) {
						push (gp_current_command_table);
						push_sub (pWorkTable);
						gp_current_command_table = pWorkTable->pNext;
						showList (pWorkTable->pszDesc);
					}
				}
				break;
			}

			++ pWorkTable ;
		}

		if (!isMatch) {
			fprintf (gp_fptr_inner, "invalid command...\n");
			fflush (gp_fptr_inner);
		}
	}
}

void CCommandServer::onCommandWaitBegin (void)
{
	fprintf (gp_fptr_inner, "###  command line  begin. ###\n");

	gp_current_command_table = g_rootCommandTable ;
	showList ("root table");
}

void CCommandServer::onCommandLineThrough (void)
{
	peep_sub_tables_print();
	fprintf (gp_fptr_inner, " > ");
	fflush (gp_fptr_inner);
}

void CCommandServer::onCommandLineAvailable (const char* pszCommand, int argc, char *argv[], CThreadMgrBase *pBase)
{
	findCommand (pszCommand, argc, argv, pBase);
}

void CCommandServer::onCommandWaitEnd (void)
{
	fprintf (gp_fptr_inner, "\n###  command line  exit. ###\n");
}
