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


const uint16_t CCommandServer::SERVER_PORT = 20001;

CCommandServer::CCommandServer (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,mClientfd (0)
{
	mSeqs [EN_SEQ_COMMAND_SERVER_START] = {(PFN_SEQ_BASE)&CCommandServer::start, (char*)"start"};
	mSeqs [EN_SEQ_COMMAND_SERVER_SERVER_LOOP] = {(PFN_SEQ_BASE)&CCommandServer::serverLoop, (char*)"serverLoop"};
	setSeqs (mSeqs, EN_SEQ_COMMAND_SERVER_NUM);
}

CCommandServer::~CCommandServer (void)
{
}


void CCommandServer::start (CThreadMgrIf *pIf)
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


	getExternalIf()->requestAsync (EN_MODULE_COMMAND_SERVER, EN_SEQ_COMMAND_SERVER_SERVER_LOOP);


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

	if( bind( fdSockSv, (struct sockaddr*)&stAddrSv, sizeof(struct sockaddr_in) ) < 0 ){
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

		char *pCarry = NULL;
		int off = 0;
		while (1) {
			memset (szBuff, 0x00, sizeof(szBuff));

			if (pCarry) {
				off = (int)strlen(pCarry);
				strncpy (szBuff, pCarry, off);
			} else {
				off = 0;
			}

			r = recvParseDelimiter (
					mClientfd,
					szBuff + off,
					sizeof(szBuff) - off,
					(char*)"\r\n",
					&pCarry
				);
			if (r <= 0) {
				break;
			}
		}

		CUtils::setLogFileptr (stdout);
		setLogFileptr (stdout);

		close (mClientfd);

		_UTL_LOG_I (
			"client:[%s] fd:[%d] --- disconnected.\n",
			inet_ntoa(stAddrCl.sin_addr),
			mClientfd
		);

		mClientfd = 0;
	}
}

int CCommandServer::recvParseDelimiter (int fd, char *pszBuff, int buffSize, char* pszDelim, char **pszCarryover)
{
	if (!pszBuff || buffSize <= 0) {
		return -1;
	}
	if (!pszDelim || (int)strlen(pszDelim) == 0) {
		return -1;
	}

	int r = recvCheckDelimiter (fd, pszBuff, buffSize, pszDelim);
	if (r < 0) {
		return -1;

	} else if (r == 0) {
		return 0;

	} else {
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
					// carryover
					if (*pszCarryover) {
						*pszCarryover = s;
					}

				} else {
					// end
				}
				break;
			}

printf ("[%s]\n", token);
			if ((int)strlen(token) == 0) {
				// through

			} else {
				if ((int)strlen(saveptr) > 0) {
					// got
					// next
					CUtils::deleteHeadSp (token);
					CUtils::deleteTailSp (token);
printf ("got next [%s]\n", token);

				} else {
					// got
					// last

					if (is_tail_delim) {
						// tail delimiter
						CUtils::deleteHeadSp (token);
						CUtils::deleteTailSp (token);
printf ("got last tail delimiter [%s]\n", token);
						if (*pszCarryover) {
							*pszCarryover = NULL;
						}

					} else {
						// not yet recv delimiter
						// carryover
						if (*pszCarryover) {
							*pszCarryover = token;
						}
					}
				}
			} 

			s = NULL;
			++ n;
		}
	}

	return r;
}

int CCommandServer::recvCheckDelimiter (int fd, char *pszBuff, int buffSize, char* pszDelim)
{
	if (!pszBuff || buffSize <= 0) {
		return -1;
	}
	if (!pszDelim || (int)strlen(pszDelim) == 0) {
		return -1;
	}

	int rSize = 0;
	int rSizeTotal = 0;
	char *pszBuffHead = pszBuff;
	bool isDisconnect = false;

	while (1) {
		if (buffSize == 0) {
			// check buffer size
			printf ("buffer over.\n");
			return -1;
		}

		rSize = CUtils::recvData (fd, (uint8_t*)pszBuff, buffSize, &isDisconnect);
		if (rSize < 0) {
			return -1;

		} else if (rSize == 0) {
			return 0;

		} else {
			// recv ok
			pszBuff += rSize;
			buffSize -= rSize;
			rSizeTotal += rSize;

			// check exist delimiter
			if (strstr (pszBuffHead, pszDelim)) {
				break;
			}

			if (isDisconnect) {
				break;
			}
		}
	}

	return rSizeTotal;
}

void CCommandServer::showList (const ST_COMMAND_INFO *pTable, const char *pszDesc)
{
	if (!pTable) {
		return;
	}

	const ST_COMMAND_INFO *pWorkTable = NULL;

	fprintf (stdout, "¥n  ------ %s ------¥n", pszDesc ? pszDesc: "???");

	pWorkTable = pTable;
	int i = 0;
	for (i = 0; pWorkTable->pszCommand; i ++) {
		fprintf (stdout, "  %-20s -- %-30s¥n", pWorkTable->pszCommand, pWorkTable->pszDesc);
		pWorkTable ++;
	}
	fprintf (stdout, "¥n");
}

void CCommandServer::parseCommandLoop (const ST_COMMAND_INFO *pTable, const char *pszConsole, const char *pszDesc)
{
	if (!pTable) {
		return;
	}

	showList (pTable, pszDesc);

	const ST_COMMAND_INFO *pWorkTable = NULL;
	bool isMatch = false;
	bool isBack = false;
	char szLine [256] = {0};
	while (1) {
		fprintf (stdout, "%s >> ", pszConsole ? pszConsole: "???"); // console
		fflush (stdout);

		isMatch = false;
		isBack = false;
		memset (szLine, 0x00, sizeof (szLine));
		read (STDIN_FILENO, szLine, sizeof(szLine));


		if (strlen(szLine) == 0) {
			continue;
		}

		pWorkTable = pTable;
		while (pWorkTable->pszCommand) {
			if (
				(strlen(pWorkTable->pszCommand) == strlen(szLine)) &&
				(strncmp (pWorkTable->pszCommand, szLine, strlen(szLine)) == 0)
			) {
				// コマンドが一致した
				isMatch = true;

				if (pWorkTable->pcbCommand) {
//TODO 引数あるときの対応 szLine
					(void) (pWorkTable->pcbCommand) (0, NULL);

				} else {
					if (pWorkTable->pNext) {
						parseCommandLoop (pWorkTable->pNext, pWorkTable->pszCommand, pWorkTable->pszDesc);
						showList (pTable, pszDesc);
					}
				}

			} else if ((strlen(".") == strlen(szLine)) && (!strncmp (".", szLine, strlen(szLine)))) {
				isBack = true;
				break;
			}

			++ pWorkTable ;
		}

		if (isBack) {
			break;
		}

		if (!isMatch) {
			fprintf (stdout, "invalid command...\n");
		}
	}
}

