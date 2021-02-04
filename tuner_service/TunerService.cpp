#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TunerService.h"
#include "modules.h"

#include "Settings.h"


CTunerService::CTunerService (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tuner_resource_max (0)
{
	SEQ_BASE_t seqs [EN_SEQ_TUNER_SERVICE__NUM] = {
		{(PFN_SEQ_BASE)&CTunerService::onReq_moduleUp, (char*)"onReq_moduleUp"},     // EN_SEQ_TUNER_SERVICE__MODULE_UP
		{(PFN_SEQ_BASE)&CTunerService::onReq_moduleDown, (char*)"onReq_moduleDown"}, // EN_SEQ_TUNER_SERVICE__MODULE_DOWN
		{(PFN_SEQ_BASE)&CTunerService::onReq_open, (char*)"onReq_open"},             // EN_SEQ_TUNER_SERVICE__OPEN
		{(PFN_SEQ_BASE)&CTunerService::onReq_close, (char*)"onReq_close(ex)"},       // EN_SEQ_TUNER_SERVICE__CLOSE
	};
	setSeqs (seqs, EN_SEQ_TUNER_SERVICE__NUM);

	m_tuner_resource_max = CSettings::getInstance()->getParams()->getTunerHalAllocates()->size();
	m_resource_allcates.clear();

}

CTunerService::~CTunerService (void)
{
}


void CTunerService::onReq_moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_WAIT_REG_CACHE_SCHED_STATE_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_moduleDown (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_open (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTunerService::onReq_close (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}
