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



CCommandServer::CCommandServer (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
{
	mSeqs [EN_SEQ_COMMAND_SERVER_START] = {(PFN_SEQ_BASE)&CCommandServer::start, (char*)"start"};
	mSeqs [EN_SEQ_COMMAND_SERVER_RECV_LOOP] = {(PFN_SEQ_BASE)&CCommandServer::recvLoop, (char*)"recvLoop"};
	setSeqs (mSeqs, EN_SEQ_COMMAND_SERVER_NUM);
}

CCommandServer::~CCommandServer (void)
{
}


void CCommandServer::start (CThreadMgrIf *pIf)
{
	uint8_t nSectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	nSectId = pIf->getSectId();
	_UTL_LOG_I ("nSectId %d\n", nSectId);

	pIf->reply (EN_THM_RSLT_SUCCESS);


	getExternalIf()->requestAsync (EN_MODULE_COMMAND_SERVER, EN_SEQ_COMMAND_SERVER_RECV_LOOP);


	nSectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (nSectId, enAct);
}

void CCommandServer::recvLoop (CThreadMgrIf *pIf)
{
	uint8_t nSectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	nSectId = pIf->getSectId();
	_UTL_LOG_I ("nSectId %d\n", nSectId);

	pIf->reply (EN_THM_RSLT_SUCCESS);


	recvLoop ();


	nSectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (nSectId, enAct);
}

void CCommandServer::recvLoop (void)
{
	int r = 0;
	int fdSockSv = 0;
	int fdSockCl = 0;
	uint8_t szBuff [128];
	uint16_t port = 0;
	struct sockaddr_in stAddrSv;
	struct sockaddr_in stAddrCl;
	socklen_t addrLenCl = sizeof(struct sockaddr_in);


	memset (&stAddrSv, 0x00, sizeof(struct sockaddr_in));
	memset (&stAddrCl, 0x00, sizeof(struct sockaddr_in));

	port = 20001;
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

		if ((fdSockCl = accept (fdSockSv, (struct sockaddr*)&stAddrCl, &addrLenCl)) < 0 ) {
			_UTL_PERROR ("accept()");
			close (fdSockSv);
			return;
		}
			
		_UTL_LOG_I (
			"clientAddr:[%s] SocketFd:[%d] --- connected.\n",
			inet_ntoa(stAddrCl.sin_addr),
			fdSockCl
		);

		int fd_copy = dup (fdSockCl);
		FILE *fp = fdopen (fd_copy, "w");
		CUtils::setLogFileptr (fp);
		setLogFileptr (fp);

		while(1){
			memset (szBuff, 0x00, sizeof(szBuff));

//			puts("recv blocking...");
			r = recv (fdSockCl, szBuff, sizeof(szBuff), 0);
			if (r < 0) {
				close (fdSockSv);
				close (fdSockCl);
				break;

			} else if (!r) {
				break;

			} else {
				CUtils::deleteLF ((char*)szBuff);
printf ("%s\n", szBuff);
			}

		}

		close (fdSockCl);
		_UTL_LOG_I (
			"clientAddr:[%s] SocketFd:[%d] --- disconnected.\n",
			inet_ntoa(stAddrCl.sin_addr),
			fdSockCl
		);

		CUtils::setLogFileptr (stdout);
		setLogFileptr (stdout);
	}

}
