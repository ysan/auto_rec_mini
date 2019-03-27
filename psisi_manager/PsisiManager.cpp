#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PsisiManager.h"
#include "PsisiManagerIf.h"

#include "modules.h"

#include "aribstr.h"


typedef struct {
	EN_PSISI_TYPE type;
	bool isNew; // new ts section
} _PARSER_NOTICE;


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
	mSeqs [EN_SEQ_PSISI_MANAGER_DUMP_CACHES]   = {(PFN_SEQ_BASE)&CPsisiManager::dumpCaches,  (char*)"dumpCaches"};
	mSeqs [EN_SEQ_PSISI_MANAGER_DUMP_TABLES]   = {(PFN_SEQ_BASE)&CPsisiManager::dumpTables,  (char*)"dumpTables"};
	setSeqs (mSeqs, EN_SEQ_PSISI_MANAGER_NUM);


	// references
	mPAT_ref = mPAT.reference();
	mEIT_H_pf_ref = mEIT_H.reference_pf();
	mEIT_H_sch_ref = mEIT_H.reference_sch();
	mNIT_ref =  mNIT.reference();
	mSDT_ref = mSDT.reference();

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
		SECTID_CHECK_PAT,
		SECTID_CHECK_PAT_WAIT,
		SECTID_CHECK_EVENT_PF,
		SECTID_CHECK_EVENT_PF_WAIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK_PAT;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_PAT:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_PAT_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_PAT_WAIT:
//TODO
// PAT途絶とかチェックする


		sectId = SECTID_CHECK_EVENT_PF;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK_EVENT_PF:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_EVENT_PF_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_EVENT_PF_WAIT:

		checkEventPfInfos ();
		refreshEventPfInfos ();

		sectId = SECTID_CHECK_PAT;
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
	case EN_PSISI_TYPE__PAT:
		if (_notice.isNew) {
			mPAT.dumpTables();
		}

		// PAT途絶チェック用の時間でも保存しておく

		break;

	case EN_PSISI_TYPE__NIT:
		if (_notice.isNew) {
//			mNIT.dumpTables();

		}

		break;

	case EN_PSISI_TYPE__SDT:
		if (_notice.isNew) {
//			mSDT.dumpTables();

			// ここにくるってことは選局したとゆうこと
			cacheServiceInfos (true);
//dDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
cacheEventPfInfos();
		}

		break;

	case EN_PSISI_TYPE__EIT_H_PF:
		if (_notice.isNew) {
			cacheEventPfInfos ();
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

void CPsisiManager::dumpCaches (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	int type = *(int*)(pIf->getSrcInfo()->msg.pMsg);
	switch (type) {
	case 0:
		dumpServiceInfos();
		break;

	case 1:
		dumpEventPfInfos();
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
	case EN_PSISI_TYPE__PAT:
		mPAT.dumpTables();
		break;

	case EN_PSISI_TYPE__PMT:
		break;

	case EN_PSISI_TYPE__EIT_H_PF:
		mEIT_H.dumpTables_pf();
		break;

	case EN_PSISI_TYPE__EIT_H_PF_simple:
		mEIT_H.dumpTables_pf_simple();
		break;

	case EN_PSISI_TYPE__EIT_H_SCH:
		mEIT_H.dumpTables_sch();
		break;

	case EN_PSISI_TYPE__EIT_H_SCH_simple:
		mEIT_H.dumpTables_sch_simple();
		break;

	case EN_PSISI_TYPE__NIT:
		mNIT.dumpTables();
		break;

	case EN_PSISI_TYPE__SDT:
		mSDT.dumpTables();
		break;

	case EN_PSISI_TYPE__RST:
		mRST.dumpTables();
		break;

	case EN_PSISI_TYPE__BIT:
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

		clearServiceInfos (true);

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

		clearEventPfInfos ();

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

/**
 * SDTテーブル内容を_SERVICE_INFOに保存します
 * 選局が発生しても継続して保持します
 *
 * 引数 is_atTuning は選局契機で呼ばれたかどうか
 * SDTテーブルは選局ごとクリアしてる
 */
void CPsisiManager::cacheServiceInfos (bool is_atTuning)
{
	std::lock_guard<std::mutex> lock (*mSDT_ref.mpMutex);

	std::vector<CServiceDescriptionTable::CTable*>::const_iterator iter = mSDT_ref.mpTables->begin();
	for (; iter != mSDT_ref.mpTables->end(); ++ iter) {

		CServiceDescriptionTable::CTable *pTable = *iter;
		uint8_t tbl_id = pTable->header.table_id;
		uint16_t ts_id = pTable->header.table_id_extension;
		uint16_t network_id = pTable->original_network_id;

		std::vector<CServiceDescriptionTable::CTable::CService>::const_iterator iter_svc = pTable->services.begin();
		for (; iter_svc != pTable->services.end(); ++ iter_svc) {

			_SERVICE_INFO * pInfo = findEmptyServiceInfo();
			if (!pInfo) {
				_UTL_LOG_E ("cacheServiceInfos failure.");
				return;
			}

			if (is_atTuning) {
				pInfo->is_tune_target = true;
			}
			pInfo->table_id = tbl_id;
			pInfo->transport_stream_id = ts_id;
			pInfo->original_network_id = network_id;
			pInfo->service_id = iter_svc->service_id;

			std::vector<CDescriptor>::const_iterator iter_desc = iter_svc->descriptors.begin();
			for (; iter_desc != iter_svc->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SERVICE_DESCRIPTOR) {
					CServiceDescriptor sd (*iter_desc);
					if (!sd.isValid) {
						_UTL_LOG_W ("invalid ServiceDescriptor");
						continue;
					}
					pInfo->service_type = sd.service_type;

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					memset (pInfo->service_name_char, 0x00, 64);
					AribToString (aribstr, (const char*)sd.service_name_char, (int)sd.service_name_length);
					strncpy (pInfo->service_name_char, aribstr, 64);
				}

			} // loop descriptors

			pInfo->last_update.setCurrentTime();
			pInfo->is_used = true;

		} // loop services

	} // loop tables
}

_SERVICE_INFO* CPsisiManager::findEmptyServiceInfo (void)
{
	int i = 0;
	for (i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (!m_serviceInfos [i].is_used) {
			break;
		}
	}

	if (i == SERVICE_INFOS_MAX) {
		_UTL_LOG_W ("m_serviceInfos full.");
		return NULL;
	}

	return &m_serviceInfos [i];
}

void CPsisiManager::clearServiceInfos (bool is_atTuning)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (is_atTuning) {
			m_serviceInfos [i].is_tune_target = false;
			m_serviceInfos [i].p_event_present = NULL;
			m_serviceInfos [i].p_event_follow = NULL;

		} else {
//TODO 適当クリア
			memset (&m_serviceInfos [i], 0x00, sizeof(_SERVICE_INFO));
			m_serviceInfos [i].is_used = false;
		}
	}
}

void CPsisiManager::dumpServiceInfos (void)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_serviceInfos [i].is_used) {
			m_serviceInfos [i].dump();
		}
	}
}

void CPsisiManager::cacheEventPfInfos (void)
{
	clearEventPfInfos();

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		// 今選局中のservice_idを探します
		if (!m_serviceInfos [i].is_tune_target) {
			continue;
		}

		// 今選局中のservice_idです
		uint16_t _service_id = m_serviceInfos [i].service_id;
		if (_service_id == 0) {
			continue;
		}

		cacheEventPfInfos (_service_id);

	}
}

/**
 * service_idをキーにテーブル内容を_EVENT_PF_INFOに保存します
 * 少なくともp/fの２つ分は見つかるはず
 */
void CPsisiManager::cacheEventPfInfos (uint16_t _service_id)
{
	if (_service_id == 0) {
		return ;
	}

	int m = 0;

	std::lock_guard<std::mutex> lock (*mEIT_H_pf_ref.mpMutex);

	std::vector<CEventInformationTable::CTable*>::const_iterator iter = mEIT_H_pf_ref.mpTables->begin();
	for (; iter != mEIT_H_pf_ref.mpTables->end(); ++ iter) {

		CEventInformationTable::CTable *pTable = *iter;
		if (_service_id != pTable->header.table_id_extension) {
			continue;
		}

		_EVENT_PF_INFO * pInfo = findEmptyEventPfInfo ();
		if (!pInfo) {
			_UTL_LOG_E ("getEventPfByServiceId failure.");
			return ;
		}

		// service_idが一致したテーブルをみます

		pInfo->table_id = pTable->header.table_id;
		pInfo->transport_stream_id = pTable->transport_stream_id;
		pInfo->original_network_id = pTable->original_network_id;
		pInfo->service_id = pTable->header.table_id_extension;

		std::vector<CEventInformationTable::CTable::CEvent>::const_iterator iter_event = pTable->events.begin();
		for (; iter_event != pTable->events.end(); ++ iter_event) {
			// p/fのテーブルにつき eventは一つだと思われる...  いちおうforでまわしておく
			// p/fでそれぞれ別のテーブル

			if (iter_event->event_id == 0) {
				// event 情報が付いてない場合
				continue;
			}

			pInfo->event_id = iter_event->event_id;

			time_t stime = CTsAribCommon::getEpochFromMJD (iter_event->start_time);
			CEtime wk (stime);
			pInfo->start_time = wk;

			int dur_sec = CTsAribCommon::getSecFromBCD (iter_event->duration);
			wk.addSec (dur_sec);
			pInfo->end_time = wk;

			std::vector<CDescriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
			for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SHORT_EVENT_DESCRIPTOR) {
					CShortEventDescriptor sd (*iter_desc);
					if (!sd.isValid) {
						_UTL_LOG_W ("invalid ShortEventDescriptor");
						continue;
					}

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					AribToString (aribstr, (const char*)sd.event_name_char, (int)sd.event_name_length);
					strncpy (pInfo->event_name_char, aribstr, 1024);
				}

			} // loop descriptors

		} // loop events


		if (pInfo->event_id == 0) {
			// event情報が付いてない
			// clear
			memset (pInfo, 0x00, sizeof(_EVENT_PF_INFO));
			pInfo->is_used = false;
			continue;
		}

		pInfo->p_org_table_addr = pTable;
		pInfo->is_used = true;
pInfo->dump();
		++ m;

	} // loop tables


	if (m == 0) {
		_UTL_LOG_W ("not match service_id [0x%04x]", _service_id);
	}
}

_EVENT_PF_INFO* CPsisiManager::findEmptyEventPfInfo (void)
{
	int i = 0;
	for (i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (!m_eventPfInfos [i].is_used) {
			break;
		}
	}

	if (i == EVENT_PF_INFOS_MAX) {
		_UTL_LOG_W ("m_eventPfInfos full.");
		return NULL;
	}

	return &m_eventPfInfos [i];
}

void CPsisiManager::checkEventPfInfos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_eventPfInfos [i].is_used) {

			CEtime cur_time;

			if (m_eventPfInfos [i].start_time <= cur_time && m_eventPfInfos [i].end_time >= cur_time) {

				if (m_eventPfInfos [i].state == EN_EVENT_PF_STATE__FOLLOW) {
					m_eventPfInfos [i].dump();
				}
				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__PRESENT;

			} else if (m_eventPfInfos [i].start_time > cur_time) {

				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__FOLLOW;

			} else if (m_eventPfInfos [i].end_time < cur_time) {

				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__ALREADY_PASSED;

			} else {
				_UTL_LOG_E ("BUG: checkEventPfInfos");
			}
		}
	}
}

void CPsisiManager::refreshEventPfInfos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_eventPfInfos [i].is_used) {
			if (m_eventPfInfos [i].state == EN_EVENT_PF_STATE__ALREADY_PASSED) {

				mEIT_H.clear_pf (m_eventPfInfos [i].p_org_table_addr);

				memset (&m_eventPfInfos [i], 0x00, sizeof(_EVENT_PF_INFO));
				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__INIT;
				m_eventPfInfos [i].is_used = false;

			}
		}
	}
}

void CPsisiManager::dumpEventPfInfos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_eventPfInfos [i].is_used) {
			m_eventPfInfos [i].dump();
		}
	}
}

void CPsisiManager::clearEventPfInfos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
//TODO 適当クリア
		memset (&m_eventPfInfos [i], 0x00, sizeof(_EVENT_PF_INFO));
		m_eventPfInfos [i].state = EN_EVENT_PF_STATE__INIT;
		m_eventPfInfos [i].is_used = false;
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
			_PARSER_NOTICE _notice = {EN_PSISI_TYPE__PAT,
										r == EN_CHECK_SECTION__COMPLETED ? true : false};
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
		if (r == EN_CHECK_SECTION__COMPLETED || r == EN_CHECK_SECTION__COMPLETED_ALREADY) {

			if (mEIT_H.m_type == 0) {
				_PARSER_NOTICE _notice = {EN_PSISI_TYPE__EIT_H_PF,
											r == EN_CHECK_SECTION__COMPLETED ? true : false};
				requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,
					(uint8_t*)&_notice,
					sizeof(_notice)
				);

			} else if (mEIT_H.m_type == 1) {
				_PARSER_NOTICE _notice = {EN_PSISI_TYPE__EIT_H_SCH,
											r == EN_CHECK_SECTION__COMPLETED ? true : false};
				requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,
					(uint8_t*)&_notice,
					sizeof(_notice)
				);
			}
		}

		break;

	case PID_NIT:

		r = mNIT.checkSection (p_ts_header, p_payload, payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED || r == EN_CHECK_SECTION__COMPLETED_ALREADY) {
			_PARSER_NOTICE _notice = {EN_PSISI_TYPE__NIT,
										r == EN_CHECK_SECTION__COMPLETED ? true : false};
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
			_PARSER_NOTICE _notice = {EN_PSISI_TYPE__SDT,
										r == EN_CHECK_SECTION__COMPLETED ? true : false};
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
