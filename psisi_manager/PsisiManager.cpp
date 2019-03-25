#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PsisiManager.h"
#include "PsisiManagerIf.h"

#include "modules.h"


typedef struct {
	EN_PSISI_TYPE type;
	bool isNew; // new ts section
} _PARSER_NOTICE;


typedef struct {
	uint16_t service_id;


} _SERVICE_LINKED_DATA;


CPsisiManager::CPsisiManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_parser (this)
	,m_tuner_notify_client_id (0xff)
	,m_ts_receive_handler_id (-1)
	,m_isTuned (false)
	,mPAT (16)
	,mEIT_H (4096*20, 20)
{
	mSeqs [EN_SEQ_PSISI_MANAGER_MODULE_UP]     = {(PFN_SEQ_BASE)&CPsisiManager::moduleUp,    (char*)"moduleUp"};
	mSeqs [EN_SEQ_PSISI_MANAGER_MODULE_DOWN]   = {(PFN_SEQ_BASE)&CPsisiManager::moduleDown,  (char*)"moduleDown"};
	mSeqs [EN_SEQ_PSISI_MANAGER_CHECK_LOOP]    = {(PFN_SEQ_BASE)&CPsisiManager::checkLoop,   (char*)"checkLoop"};
	mSeqs [EN_SEQ_PSISI_MANAGER_PARSER_NOTICE] = {(PFN_SEQ_BASE)&CPsisiManager::parserNotice,(char*)"parserNotice"};
	mSeqs [EN_SEQ_PSISI_MANAGER_DUMP_TABLES]   = {(PFN_SEQ_BASE)&CPsisiManager::dumpTables,  (char*)"dumpTables"};
	setSeqs (mSeqs, EN_SEQ_PSISI_MANAGER_NUM);


	// references
	mPAT_ref = mPAT.reference();
	mEIT_H_pf_ref = mEIT_H.reference_pf();
	mEIT_H_sch_ref = mEIT_H.reference_sch();

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
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
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
		sectId = SECTID_REQ_REG_TUNER_NOTIFY;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_REG_TUNER_NOTIFY: {
		CTunerControlIf _if (getExternalIf());
		_if.reqRegisterTunerNotify ();

		sectId = SECTID_WAIT_REG_TUNER_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_TUNER_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_tuner_notify_client_id = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_HANDLER;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
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
		pIf->setTimeout (2000); // 2sec
		sectId = SECTID_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT:
//TODO
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

void CPsisiManager::parserNotice (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	_PARSER_NOTICE _notice = *(_PARSER_NOTICE*)(pIf->getSrcInfo()->msg.pMsg);
	switch (_notice.type) {
	case EN_PSISI_TYPE_PAT:
		if (_notice.isNew) {
			mPAT.dumpTables();
		}

		// 時間でも保存しておく

		break;

	case EN_PSISI_TYPE_NIT:
		if (_notice.isNew) {
			mNIT.dumpTables();
		}

		break;

	case EN_PSISI_TYPE_SDT:
		if (_notice.isNew) {
			mSDT.dumpTables();
		}

		break;

	default:
		break;
	}

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::dumpTables (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_PSISI_TYPE type = *(EN_PSISI_TYPE*)(pIf->getSrcInfo()->msg.pMsg);
	switch (type) {
	case EN_PSISI_TYPE_PAT:
		mPAT.dumpTables();
		break;

	case EN_PSISI_TYPE_PMT:
		break;

	case EN_PSISI_TYPE_EIT_H_PF:
		mEIT_H.dumpTables_pf_simple();
		mEIT_H.dumpTables_pf();
		break;

	case EN_PSISI_TYPE_EIT_H_SCH:
		mEIT_H.dumpTables_sch_simple();
		mEIT_H.dumpTables_sch();
		break;

	case EN_PSISI_TYPE_NIT:
		mNIT.dumpTables();
		break;

	case EN_PSISI_TYPE_SDT:
		mSDT.dumpTables();
		break;

	case EN_PSISI_TYPE_RST:
		mRST.dumpTables();
		break;

	case EN_PSISI_TYPE_BIT:
		mBIT.dumpTables();
		break;

	default:
		break;
	}


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReceiveNotify (CThreadMgrIf *pIf)
{
	if (!(pIf->getSrcInfo()->nClientId == m_tuner_notify_client_id)) {
		return ;
	}

	EN_TUNER_NOTIFY enNotify = *(EN_TUNER_NOTIFY*)(pIf->getSrcInfo()->msg.pMsg);
	switch (enNotify) {
	case EN_TUNER_NOTIFY__TUNING_BEGIN:
		_UTL_LOG_I ("EN_TUNER_NOTIFY__TUNING_BEGIN");

		m_isTuned = false;

		break;

	case EN_TUNER_NOTIFY__TUNING_END_SUCCESS:
		_UTL_LOG_I ("EN_TUNER_NOTIFY__TUNING_END_SUCCESS");

		m_isTuned = true;

		mPAT.clear();
		mEIT_H.clear_pf();
		mNIT.clear();
		mSDT.clear();
		mRST.clear();
		mBIT.clear();
	
		break;

	case EN_TUNER_NOTIFY__TUNING_END_ERROR:
		_UTL_LOG_I ("EN_TUNER_NOTIFY__TUNING_END_ERROR");

		m_isTuned = false;

		break;

	case EN_TUNER_NOTIFY__TUNE_STOP:
		_UTL_LOG_I ("EN_TUNER_NOTIFY__TUNE_STOP");

		m_isTuned = false;

		break;

	default:
		break;
	}
}



//////////  CTunerControlIf::ITsReceiveHandler  //////////

bool CPsisiManager::onPreTsReceive (void)
{
	getExternalIf()->createExternalCp();

	uint32_t opt = getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	return true;
}

void CPsisiManager::onPostTsReceive (void)
{
	uint32_t opt = getExternalIf()->getRequestOption ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	getExternalIf()->destroyExternalCp();
}

bool CPsisiManager::onCheckTsReceiveLoop (void)
{
	return true;
}

bool CPsisiManager::onTsReceived (void *p_ts_data, int length)
{

	// ts parser processing
	m_parser.run ((uint8_t*)p_ts_data, length);

	return true;
}



//////////  CTsParser::IParserListener  //////////

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
			_PARSER_NOTICE _notice = {EN_PSISI_TYPE_PAT, r == EN_CHECK_SECTION__COMPLETED ? true : false};
			requestAsync (
				EN_MODULE_PSISI_MANAGER,
				EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);
		}

		break;

	case PID_EIT_H:

		r = mEIT_H.checkSection (p_ts_header, p_payload, payload_size);
		break;

	case PID_NIT:

		r = mNIT.checkSection (p_ts_header, p_payload, payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED || r == EN_CHECK_SECTION__COMPLETED_ALREADY) {
			_PARSER_NOTICE _notice = {EN_PSISI_TYPE_NIT, r == EN_CHECK_SECTION__COMPLETED ? true : false};
			requestAsync (
				EN_MODULE_PSISI_MANAGER,
				EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);
		}

		break;

	case PID_SDT:

		r = mSDT.checkSection (p_ts_header, p_payload, payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED || r == EN_CHECK_SECTION__COMPLETED_ALREADY) {
			_PARSER_NOTICE _notice = {EN_PSISI_TYPE_SDT, r == EN_CHECK_SECTION__COMPLETED ? true : false};
			requestAsync (
				EN_MODULE_PSISI_MANAGER,
				EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);
		}

		break;

	case PID_RST:

		r = mRST.checkSection (p_ts_header, p_payload, payload_size);
		break;

	case PID_BIT:

		r = mBIT.checkSection (p_ts_header, p_payload, payload_size);
		break;


	default:	
		break;
	}


	return true;
}
