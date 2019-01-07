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

#include "TuneThread.h"

#include "modules.h"


CTuneThread::CTuneThread (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
{
	mSeqs [EN_SEQ_TUNE_THREAD_START] = {(PFN_SEQ_BASE)&CTuneThread::start, (char*)"start"};
	mSeqs [EN_SEQ_TUNE_THREAD_TUNE] = {(PFN_SEQ_BASE)&CTuneThread::tune, (char*)"tune"};
	setSeqs (mSeqs, EN_SEQ_TUNE_THREAD_NUM);
}

CTuneThread::~CTuneThread (void)
{
}


void CTuneThread::start (CThreadMgrIf *pIf)
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

//
// do nothing
//

	nSectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (nSectId, enAct);
}

void CTuneThread::tune (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;

	sectId = pIf->getSectId();
	_UTL_LOG_I ("sectId %d\n", sectId);


	////////////////////////////////////////
	uint32_t freq = *(pIf->getSrcInfo()->msg.pMsg);

	if (it9175_open ()) {
		it9175_tune (freq);
	}
	////////////////////////////////////////


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}
