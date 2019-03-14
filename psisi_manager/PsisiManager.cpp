#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PsisiManager.h"
#include "PsisiManagerIf.h"

#include "modules.h"


CPsisiManager::CPsisiManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_parser (this)
	,m_ts_receive_handler_id (-1)
	,mPAT (16)
	,mEIT_H (4096*20, 20)
{
	mSeqs [EN_SEQ_PSISI_MANAGER_MODULE_UP]   = {(PFN_SEQ_BASE)&CPsisiManager::moduleUp,   (char*)"moduleUp"};
	mSeqs [EN_SEQ_PSISI_MANAGER_MODULE_DOWN] = {(PFN_SEQ_BASE)&CPsisiManager::moduleDown, (char*)"moduleDown"};
	mSeqs [EN_SEQ_PSISI_MANAGER_CHECK_LOOP]  = {(PFN_SEQ_BASE)&CPsisiManager::checkLoop,  (char*)"checkLoop"};
	mSeqs [EN_SEQ_PSISI_MANAGER_CHECK_PAT]   = {(PFN_SEQ_BASE)&CPsisiManager::checkPAT,   (char*)"checkPAT"};
	setSeqs (mSeqs, EN_SEQ_PSISI_MANAGER_NUM);


	// references
	mPAT_ref = mPAT.reference();
	mEIT_H_ref = mEIT_H.reference();

}

CPsisiManager::~CPsisiManager (void)
{
}


void CPsisiManager::moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_HANDLER,
		SECTID_WAIT_REG_HANDLER,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:
		sectId = SECTID_REQ_REG_HANDLER;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_REG_HANDLER: {

		CTunerControlIf::ITsReceiveHandler *p = this;
		CTunerControlIf _if (getExternalIf());
		_if.reqRegisterTsReceiveHandler (&p);

		sectId = SECTID_WAIT_REG_HANDLER;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_HANDLER:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_ts_receive_handler_id = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_CHECK_LOOP;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTsReceiveHandler is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_CHECK_LOOP:
		requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_CHECK_LOOP);

		sectId = SECTID_WAIT_CHECK_LOOP;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_CHECK_LOOP:
//		enRslt = pIf->getSrcInfo()->enRslt;
//		if (enRslt == EN_THM_RSLT_SUCCESS) {
//
//		} else {
//
//		}

		// EN_THM_RSLT_SUCCESSのみ

		sectId = SECTID_END_SUCCESS;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END_SUCCESS:
		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}


	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::moduleDown (CThreadMgrIf *pIf)
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

void CPsisiManager::checkLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_LOOP,
		SECTID_WAIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_LOOP;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_LOOP:
		pIf->setTimeout (5000); // 5sec
		sectId = SECTID_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT:

// PAT途絶とかチェックする

		sectId = SECTID_LOOP;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::checkPAT (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	bool *p_isCompAlready = (bool*)(pIf->getSrcInfo()->msg.pMsg);
	if (*p_isCompAlready) {
		mPAT.dumpTables();
	}

// 時間でも保存しておく

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}



// CTunerControlIf::ITsReceiveHandler
bool CPsisiManager::onPreTsReceive (void)
{
	getExternalIf()->createExternalCp();

	uint32_t opt = getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	return true;
}

// CTunerControlIf::ITsReceiveHandler
void CPsisiManager::onPostTsReceive (void)
{
	uint32_t opt = getExternalIf()->getRequestOption ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	getExternalIf()->destroyExternalCp();
}

// CTunerControlIf::ITsReceiveHandler
bool CPsisiManager::onCheckTsReceiveLoop (void)
{
	return true;
}

// CTunerControlIf::ITsReceiveHandler
bool CPsisiManager::onTsReceived (void *p_ts_data, int length)
{

	// ts parser processing
	m_parser.run ((uint8_t*)p_ts_data, length);

	return true;
}

// CTsParser::IParserListener
bool CPsisiManager::onTsPacketAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size)
{
	if (!p_ts_header || !p_payload || payload_size == 0) {
		// through
		return true;
	}

//	p_ts_header->dump();

	EN_CHECK_SECTION r = EN_CHECK_SECTION__COMPLETED;

	switch (p_ts_header->pid) {
	case PID_PAT:

		r = mPAT.checkSection (p_ts_header, p_payload, payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED || r == EN_CHECK_SECTION__COMPLETED_ALREADY) {
			CPsisiManagerIf _if(getExternalIf());
			_if.reqCheckPAT ((r == EN_CHECK_SECTION__COMPLETED) ? true : false);
		}

		break;

	case PID_EIT_H:
//		r = mEIT_H.checkSection (p_ts_header, p_payload, payload_size);
		break;

	default:	
		break;
	}




	return true;
}
