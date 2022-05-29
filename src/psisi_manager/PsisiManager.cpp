#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include<iostream>
#include<fstream>
#include<sstream>

#include "PsisiManager.h"
#include "PsisiManagerIf.h"
#include "PsisiManagerStructsAddition.h"

#include "modules.h"

#include "aribstr.h"


typedef struct _parser_notice {
	_parser_notice (EN_PSISI_TYPE type, TS_HEADER* p_ts_header, uint8_t* p_payload, size_t payload_size) {
		this->type = type;
		this->payload_size = payload_size;
		this->ts_header = *p_ts_header;
		memcpy (this->payload, p_payload, payload_size);
	};

	EN_PSISI_TYPE type;
	TS_HEADER ts_header;
	uint8_t payload [TS_PACKET_LEN];
	size_t payload_size;
} _PARSER_NOTICE;


CPsisiManager::CPsisiManager (char *pszName, uint8_t nQueNum, uint8_t groupId)
	:CThreadMgrBase (pszName, nQueNum)
	,CGroup (groupId)
	,mp_ts_handler (this)
	,m_parser (this)
	,m_tuner_notify_client_id (0xff)
	,m_ts_receive_handler_id (-1)
	,m_tunerIsTuned (false)
	,m_isDetectedPAT (false)
	,m_state (EN_PSISI_STATE__NOT_READY)
	,mPAT (16)
	,mEIT_H (4096*100, 100)
	,mCDT (4096*100)
	,mEIT_H_sched (4096*100, 100, this)
	,m_isEnableEITSched (false)
{
	SEQ_BASE_t seqs [EN_SEQ_PSISI_MANAGER__NUM] = {
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_moduleUp, (char*)"onReq_moduleUp"},                                       // EN_SEQ_PSISI_MANAGER__MODULE_UP
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_moduleDown, (char*)"onReq_moduleDown"},                                   // EN_SEQ_PSISI_MANAGER__MODULE_DOWN
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getState, (char*)"onReq_getState"},                                       // EN_SEQ_PSISI_MANAGER__GET_STATE
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_checkLoop, (char*)"onReq_checkLoop"},                                     // EN_SEQ_PSISI_MANAGER__CHECK_LOOP
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_parserNotice, (char*)"onReq_parserNotice"},                               // EN_SEQ_PSISI_MANAGER__PARSER_NOTICE
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_stabilizationAfterTuning, (char*)"onReq_stabilizationAfterTuning"},       // EN_SEQ_PSISI_MANAGER__STABILIZATION_AFTER_TUNING
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_registerPatDetectNotify, (char*)"onReq_registerPatDetectNotify"},         // EN_SEQ_PSISI_MANAGER__REG_PAT_DETECT_NOTIFY
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_unregisterPatDetectNotify, (char*)"onReq_unregisterPatDetectNotify"},     // EN_SEQ_PSISI_MANAGER__UNREG_PAT_DETECT_NOTIFY
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_registerEventChangeNotify, (char*)"onReq_registerEventChangeNotify"},     // EN_SEQ_PSISI_MANAGER__REG_EVENT_CHANGE_NOTIFY
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_unregisterEventChangeNotify, (char*)"onReq_unregisterEventChangeNotify"}, // EN_SEQ_PSISI_MANAGER__UNREG_EVENT_CHANGE_NOTIFY
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_registerPsisiStateNotify, (char*)"onReq_registerPsisiStateNotify"},       // EN_SEQ_PSISI_MANAGER__REG_PSISI_STATE_NOTIFY
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_unregisterPsisiStateNotify, (char*)"onReq_unregisterPsisiStateNotify"},   // EN_SEQ_PSISI_MANAGER__UNREG_PSISI_STATE_NOTIFY
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getPatDetectState, (char*)"onReq_getPatDetectState"},                     // EN_SEQ_PSISI_MANAGER__GET_PAT_DETECT_STATE
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getStreamInfos, (char*)"onReq_getStreamInfos"},                           // EN_SEQ_PSISI_MANAGER__GET_STREAM_INFOS
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getCurrentServiceInfos, (char*)"onReq_getCurrentServiceInfos"},           // EN_SEQ_PSISI_MANAGER__GET_CURRENT_SERVICE_INFOS
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getPresentEventInfo, (char*)"onReq_getPresentEventInfo"},                 // EN_SEQ_PSISI_MANAGER__GET_PRESENT_EVENT_INFO
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getFollowEventInfo, (char*)"onReq_getFollowEventInfo"},                   // EN_SEQ_PSISI_MANAGER__GET_FOLLOW_EVENT_INFO
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_getCurrentNetworkInfo, (char*)"onReq_getCurrentNetworkInfo"},             // EN_SEQ_PSISI_MANAGER__GET_CURRENT_NETWORK_INFO
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_enableParseEITSched, (char*)"onReq_enableParseEITSched"},                 // EN_SEQ_PSISI_MANAGER__ENABLE_PARSE_EIT_SCHED
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_disableParseEITSched, (char*)"onReq_disableParseEITSched"},               // EN_SEQ_PSISI_MANAGER__DISABLE_PARSE_EIT_SCHED
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_dumpCaches, (char*)"onReq_dumpCaches"},                                   // EN_SEQ_PSISI_MANAGER__DUMP_CACHES
		{(PFN_SEQ_BASE)&CPsisiManager::onReq_dumpTables, (char*)"onReq_dumpTables"},                                   // EN_SEQ_PSISI_MANAGER__DUMP_TABLES
	};
	setSeqs (seqs, EN_SEQ_PSISI_MANAGER__NUM);


	mp_settings = CSettings::getInstance();

	// references
//	mPAT_ref = mPAT.reference();
//	mEIT_H_ref = mEIT_H.reference();
//	mNIT_ref =  mNIT.reference();
//	mSDT_ref = mSDT.reference();


	m_patRecvTime.clear();

	clearProgramInfos ();
	clearServiceInfos ();
	clearEventPfInfos ();
	clearNetworkInfo ();

	m_EIT_H_comp_flag.clear();
	m_NIT_comp_flag.clear();
	m_SDT_comp_flag.clear();
}

CPsisiManager::~CPsisiManager (void)
{
	m_patRecvTime.clear();

	clearProgramInfos ();
	clearServiceInfos ();
	clearEventPfInfos ();
	clearNetworkInfo ();

	m_EIT_H_comp_flag.clear();
	m_NIT_comp_flag.clear();
	m_SDT_comp_flag.clear();
}


void CPsisiManager::onCreate (void)
{
	// SCHED_FIFOにして優先度上げてみます
	// 確認コマンド (FFやRRで99のtaskとか存在する)
	// $ ps -em -o pid,tid,policy,pri,ni,rtprio,comm
	int policy = SCHED_FIFO;
	struct sched_param param;
	param.sched_priority = 80;
	if (pthread_setschedparam(pthread_self(), policy, &param) != 0) {
		_UTL_PERROR ("pthread_setschedparam");
	}
}

void CPsisiManager::onReq_moduleUp (CThreadMgrIf *pIf)
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
		CTunerControlIf _if (getExternalIf(), getGroupId());
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

		_UTL_LOG_I ("CTunerControlIf::ITsReceiveHandler %p", mp_ts_handler);
		CTunerControlIf _if (getExternalIf(), getGroupId());
		_if.reqRegisterTsReceiveHandler (&mp_ts_handler);

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
		requestAsync (EN_MODULE_PSISI_MANAGER + getGroupId(), EN_SEQ_PSISI_MANAGER__CHECK_LOOP);

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

void CPsisiManager::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CPsisiManager::onReq_getState (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&m_state, sizeof(m_state));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_checkLoop (CThreadMgrIf *pIf)
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

	case SECTID_CHECK_PAT_WAIT: {

		if (m_tunerIsTuned) {
			// PAT途絶チェック
			// 最後にPATを受けとった時刻から再び受け取るまで30秒以上経過していたら異常	
			CEtime tcur;
			tcur.setCurrentTime();
			CEtime ttmp = m_patRecvTime;
			ttmp.addSec (30); 
			if (tcur > ttmp) {
				_UTL_LOG_E ("PAT was not detected. probably stream is broken...");

				if (m_isDetectedPAT) {
					// true -> false
					// fire notify
					EN_PAT_DETECT_STATE _state = EN_PAT_DETECT_STATE__NOT_DETECTED;
					pIf->notify (NOTIFY_CAT__PAT_DETECT, (uint8_t*)&_state, sizeof(EN_PAT_DETECT_STATE));
				}

				m_isDetectedPAT = false;

			} else {

				if (!m_isDetectedPAT) {
					// false -> true
					// fire notify
					EN_PAT_DETECT_STATE _state = EN_PAT_DETECT_STATE__DETECTED;
					pIf->notify (NOTIFY_CAT__PAT_DETECT, (uint8_t*)&_state, sizeof(EN_PAT_DETECT_STATE));
				}

				m_isDetectedPAT = true;
			}
		}

		sectId = SECTID_CHECK_EVENT_PF;
		enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_CHECK_EVENT_PF:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_EVENT_PF_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_EVENT_PF_WAIT: {

//		if (checkEventPfInfos (pIf)) {
#ifndef _DUMMY_TUNER
// ローカルデバッグ中は消したくないので
//			refreshEventPfInfos ();
#endif
//		}

		assignFollowEventToServiceInfos ();
		checkFollowEventAtServiceInfos (pIf);


		sectId = SECTID_CHECK_PAT;
		enAct = EN_THM_ACT_CONTINUE;
		} break;

	case SECTID_END:
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_parserNotice (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	EN_CHECK_SECTION r = EN_CHECK_SECTION__COMPLETED;

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	mPAT.checkAsyncDelete ();
	mEIT_H.checkAsyncDelete ();
	mEIT_H_sched.checkAsyncDelete ();
	mNIT.checkAsyncDelete ();
	mSDT.checkAsyncDelete ();
	mCAT.checkAsyncDelete ();
	mCDT.checkAsyncDelete ();


	_PARSER_NOTICE *p_notice = (_PARSER_NOTICE*)(pIf->getSrcInfo()->msg.pMsg);
	switch (p_notice->type) {
	case EN_PSISI_TYPE__PAT:

		r = mPAT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED ) {
			_UTL_LOG_I ("notice new PAT");
			clearProgramInfos();
			cacheProgramInfos();

			// 新しいPATが取れたら PMT parserを準備します

			// PATは一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
			std::vector<CProgramAssociationTable::CTable*>::const_iterator iter = mPAT.getTables()->end();
			CProgramAssociationTable::CTable* latest = *(-- iter);

			// 一度クリアします
			m_programMap.clear();

			for (const auto& program : latest->programs) {
				if (program.program_number == 0) {
					continue;
				}
				m_programMap.add(program.program_map_PID);
			}
		}

		// PAT途絶チェック用
		m_patRecvTime.setCurrentTime ();

		break;

	case EN_PSISI_TYPE__PMT: {
		r = m_programMap.parse(&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new PMT");
			clearProgramInfos();
			cacheProgramInfos();
		}

		}
		break;

	case EN_PSISI_TYPE__CAT:

		r = mCAT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new CAT");
//TODO
		}

		break;

	case EN_PSISI_TYPE__CDT:

		r = mCDT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			_UTL_LOG_I ("notice new CDT");

			storeLogo();
		}

		break;

	case EN_PSISI_TYPE__NIT:

		r = mNIT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_NIT_comp_flag.check_update (true);

			_UTL_LOG_I ("notice new NIT");

			// ここにくるってことは選局したとゆうこと
			clearNetworkInfo();
			cacheNetworkInfo();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_NIT_comp_flag.check_update (false);
		}

		break;

	case EN_PSISI_TYPE__SDT:

		r = mSDT.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_SDT_comp_flag.check_update (true);

			_UTL_LOG_I ("notice new SDT");

			// ここにくるってことは選局したとゆうこと
			clearServiceInfos (true);
			cacheServiceInfos (true);

			// eventInfo もcacheし直し
			clearEventPfInfos();
			cacheEventPfInfos();
			checkEventPfInfos();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_SDT_comp_flag.check_update (false);
		}

		break;

	case EN_PSISI_TYPE__EIT_H_PF:

		r = mEIT_H.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
		if (r == EN_CHECK_SECTION__COMPLETED) {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_EIT_H_comp_flag.check_update (true);

			_UTL_LOG_I ("notice new EIT p/f");
			clearEventPfInfos();
			cacheEventPfInfos ();
			checkEventPfInfos();

		} else {
			// 選局後 parserが新しいセクションを取得したかチェックします
			m_EIT_H_comp_flag.check_update (false);
		}


		{
			uint32_t opt = getExternalIf()->getRequestOption ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			getExternalIf()->setRequestOption (opt);

#ifdef _DUMMY_TUNER // デバッグ中は m_isEnableEITSched 関係なしに
			mEIT_H_sched.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
#else
			if (m_isEnableEITSched) {
				mEIT_H_sched.checkSection (&p_notice->ts_header, p_notice->payload, p_notice->payload_size);
			}
#endif
			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);
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

void CPsisiManager::onReq_stabilizationAfterTuning (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CLEAR,
		SECTID_CLEAR_WAIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	switch (sectId) {
	case SECTID_ENTRY:

		// workaround
		// 選局とと同時にparserNoticeに多くのキューが入り あふれてしまうことがある
		// そのため 本シーケンスのsetTimeoutの返しキューがうけとれず停止
		// -> tunerService的に選局失敗 -> リトライ
		// リトライ時に またリクエストを受け取れるように enableOverwriteを使います
		pIf->enableOverwrite();

		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CLEAR;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CLEAR:

		pIf->setTimeout (200);

		sectId = SECTID_CLEAR_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CLEAR_WAIT:

		mPAT.clear();
		mEIT_H.clear();
		mNIT.clear();
		mSDT.clear();
		mCAT.clear();
		mCDT.clear();
//		mRST.clear();
//		mBIT.clear();

		m_EIT_H_comp_flag.clear();
		m_NIT_comp_flag.clear();
		m_SDT_comp_flag.clear();

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK:

		pIf->setTimeout (100);

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT:

		// EIT,NIT,SDTのparserが完了したか確認します
		// 少なくともこの3つが完了していれば psisi manager的に選局完了とします (EN_PSISI_STATE__READY)
		// ARIB規格上では送出頻度は以下のとおりになっている... PAT,PMTは上3つより頻度高いので除外しています
		//   PAT 1回以上/100mS
		//   PMT 1回以上/100mS
		//   EIT 1回以上/2s
		//   NIT 1回以上/10s
		//   SDT 1回以上/2s
		if (
			m_EIT_H_comp_flag.is_completed() &&
			m_NIT_comp_flag.is_completed() &&
			m_SDT_comp_flag.is_completed()
		) {
			_UTL_LOG_I ("PSI/SI ready.");

			m_tunerIsTuned = true;

			// fire notify
			m_state = EN_PSISI_STATE__READY;
			pIf->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));


			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_END:
		// workaround
		pIf->disableOverwrite();

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_registerPatDetectNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	uint8_t clientId = 0;
	EN_THM_RSLT enRslt;
	bool rslt = pIf->regNotify (NOTIFY_CAT__PAT_DETECT, &clientId);
	if (rslt) {
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		enRslt = EN_THM_RSLT_ERROR;
	}

	_UTL_LOG_I ("registerd clientId=[0x%02x]\n", clientId);

	// clientIdをreply msgで返す 
	pIf->reply (enRslt, (uint8_t*)&clientId, sizeof(clientId));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_unregisterPatDetectNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_THM_RSLT enRslt;
	// msgからclientIdを取得
	uint8_t clientId = *(pIf->getSrcInfo()->msg.pMsg);
	bool rslt = pIf->unregNotify (NOTIFY_CAT__PAT_DETECT, clientId);
	if (rslt) {
		_UTL_LOG_I ("unregisterd clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		_UTL_LOG_E ("failure unregister clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_ERROR;
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_registerEventChangeNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	uint8_t clientId = 0;
	EN_THM_RSLT enRslt;
	bool rslt = pIf->regNotify (NOTIFY_CAT__EVENT_CHANGE, &clientId);
	if (rslt) {
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		enRslt = EN_THM_RSLT_ERROR;
	}

	_UTL_LOG_I ("registerd clientId=[0x%02x]\n", clientId);

	// clientIdをreply msgで返す 
	pIf->reply (enRslt, (uint8_t*)&clientId, sizeof(clientId));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_unregisterEventChangeNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_THM_RSLT enRslt;
	// msgからclientIdを取得
	uint8_t clientId = *(pIf->getSrcInfo()->msg.pMsg);
	bool rslt = pIf->unregNotify (NOTIFY_CAT__EVENT_CHANGE, clientId);
	if (rslt) {
		_UTL_LOG_I ("unregisterd clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		_UTL_LOG_E ("failure unregister clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_ERROR;
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_registerPsisiStateNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	uint8_t clientId = 0;
	EN_THM_RSLT enRslt;
	bool rslt = pIf->regNotify (NOTIFY_CAT__PSISI_STATE, &clientId);
	if (rslt) {
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		enRslt = EN_THM_RSLT_ERROR;
	}

	_UTL_LOG_I ("registerd clientId=[0x%02x]\n", clientId);

	// clientIdをreply msgで返す 
	pIf->reply (enRslt, (uint8_t*)&clientId, sizeof(clientId));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_unregisterPsisiStateNotify (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_THM_RSLT enRslt;
	// msgからclientIdを取得
	uint8_t clientId = *(pIf->getSrcInfo()->msg.pMsg);
	bool rslt = pIf->unregNotify (NOTIFY_CAT__PSISI_STATE, clientId);
	if (rslt) {
		_UTL_LOG_I ("unregisterd clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_SUCCESS;
	} else {
		_UTL_LOG_E ("failure unregister clientId=[0x%02x]\n", clientId);
		enRslt = EN_THM_RSLT_ERROR;
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_getPatDetectState (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	EN_PAT_DETECT_STATE state =
			m_isDetectedPAT ? EN_PAT_DETECT_STATE__DETECTED: EN_PAT_DETECT_STATE__NOT_DETECTED;

	pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&state, sizeof(state));


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_getStreamInfos (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	REQ_STREAM_INFOS_PARAM param = *(REQ_STREAM_INFOS_PARAM*)(pIf->getSrcInfo()->msg.pMsg);
	if (!param.p_out_streamInfos || param.array_max_num == 0) {
		_UTL_LOG_E ("REQ_STREAM_INFOS_PARAM is invalid.\n");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {
		int get_num = getStreamInfos (param.program_number, param.type, param.p_out_streamInfos, param.array_max_num);

		// reply msgで格納数を渡します
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&get_num, sizeof(get_num));
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_getCurrentServiceInfos (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	REQ_SERVICE_INFOS_PARAM param = *(REQ_SERVICE_INFOS_PARAM*)(pIf->getSrcInfo()->msg.pMsg);
	if (!param.p_out_serviceInfos || param.array_max_num == 0) {
		_UTL_LOG_E ("REQ_SERVICE_INFOS_PARAM is invalid.\n");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {
		int get_num = getCurrentServiceInfos (param.p_out_serviceInfos, param.array_max_num);

		// reply msgで格納数を渡します
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&get_num, sizeof(get_num));
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_getPresentEventInfo (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt;

	REQ_EVENT_INFO_PARAM param = *(REQ_EVENT_INFO_PARAM*)(pIf->getSrcInfo()->msg.pMsg);
	if (!param.p_out_eventInfo) {
		_UTL_LOG_E ("REQ_EVENT_INFO_PARAM is invalid.\n");
		enRslt = EN_THM_RSLT_ERROR;

	} else {
		_EVENT_PF_INFO *p_info = findEventPfInfo (
										TBLID_EIT_PF_A,
										param.key.transport_stream_id,
										param.key.original_network_id,
										param.key.service_id,
										EN_EVENT_PF_STATE__PRESENT
									);
		if (p_info) {
			param.p_out_eventInfo->table_id = p_info->table_id;
			param.p_out_eventInfo->transport_stream_id = p_info->transport_stream_id;
			param.p_out_eventInfo->original_network_id = p_info->original_network_id;
			param.p_out_eventInfo->service_id = p_info->service_id;
			param.p_out_eventInfo->event_id = p_info->event_id;
			param.p_out_eventInfo->start_time = p_info->start_time;
			param.p_out_eventInfo->end_time = p_info->end_time;
			strncpy (
				param.p_out_eventInfo->event_name_char,
				p_info->event_name_char,
				strlen(p_info->event_name_char)
			);

			enRslt = EN_THM_RSLT_SUCCESS;

		} else {
			_UTL_LOG_E ("not found PresentEventInfo");
			enRslt = EN_THM_RSLT_ERROR;
mEIT_H.dumpTables_simple();
mEIT_H.dumpSectionList();
		}
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_getFollowEventInfo (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt;

	REQ_EVENT_INFO_PARAM param = *(REQ_EVENT_INFO_PARAM*)(pIf->getSrcInfo()->msg.pMsg);
	if (!param.p_out_eventInfo) {
		_UTL_LOG_E ("REQ_EVENT_INFO_PARAM is invalid.\n");
		enRslt = EN_THM_RSLT_ERROR;

	} else {
		_EVENT_PF_INFO *p_info = findEventPfInfo (
										TBLID_EIT_PF_A,
										param.key.transport_stream_id,
										param.key.original_network_id,
										param.key.service_id,
										EN_EVENT_PF_STATE__FOLLOW
									);
		if (p_info) {
			param.p_out_eventInfo->table_id = p_info->table_id;
			param.p_out_eventInfo->transport_stream_id = p_info->transport_stream_id;
			param.p_out_eventInfo->original_network_id = p_info->original_network_id;
			param.p_out_eventInfo->service_id = p_info->service_id;
			param.p_out_eventInfo->event_id = p_info->event_id;
			param.p_out_eventInfo->start_time = p_info->start_time;
			param.p_out_eventInfo->end_time = p_info->end_time;
			strncpy (
				param.p_out_eventInfo->event_name_char,
				p_info->event_name_char,
				strlen(p_info->event_name_char)
			);

			enRslt = EN_THM_RSLT_SUCCESS;

		} else {
			_UTL_LOG_E ("not found FollowEventInfo");
			enRslt = EN_THM_RSLT_ERROR;
		}
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_getCurrentNetworkInfo (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt;

	REQ_NETWORK_INFO_PARAM param = *(REQ_NETWORK_INFO_PARAM*)(pIf->getSrcInfo()->msg.pMsg);
	if (!param.p_out_networkInfo) {
		_UTL_LOG_E ("REQ_NETWORK_INFO_PARAM is invalid.\n");
		enRslt = EN_THM_RSLT_ERROR;

	} else {
		if (m_networkInfo.is_used) {
			param.p_out_networkInfo->table_id = m_networkInfo.table_id;
			param.p_out_networkInfo->network_id = m_networkInfo.network_id;

			strncpy (
				param.p_out_networkInfo->network_name_char,
				m_networkInfo.network_name_char,
				strlen(m_networkInfo.network_name_char)
			);

			param.p_out_networkInfo->transport_stream_id = m_networkInfo.transport_stream_id;
			param.p_out_networkInfo->original_network_id = m_networkInfo.original_network_id;

			for (int i = 0; i < m_networkInfo.services_num; ++ i) {
				param.p_out_networkInfo->services[i].service_id = m_networkInfo.services[i].service_id;
				param.p_out_networkInfo->services[i].service_type = m_networkInfo.services[i].service_type;
			}
			param.p_out_networkInfo->services_num = m_networkInfo.services_num;

			param.p_out_networkInfo->area_code = m_networkInfo.area_code;
			param.p_out_networkInfo->guard_interval = m_networkInfo.guard_interval;
			param.p_out_networkInfo->transmission_mode = m_networkInfo.transmission_mode;
			param.p_out_networkInfo->remote_control_key_id = m_networkInfo.remote_control_key_id;
			strncpy (
				param.p_out_networkInfo->ts_name_char,
				m_networkInfo.ts_name_char,
				strlen(m_networkInfo.ts_name_char)
			);

			enRslt = EN_THM_RSLT_SUCCESS;

		} else {
			_UTL_LOG_E ("m_networkInfo is not availale...");
			enRslt = EN_THM_RSLT_ERROR;
		}
	}

	pIf->reply (enRslt);


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_enableParseEITSched (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	m_isEnableEITSched = true;


	// reply msgにparserのアドレスを乗せます
	Enable_PARSE_EIT_SCHED_REPLY_PARAM_t param = {&mEIT_H_sched};


	pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&param, sizeof(param));

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_disableParseEITSched (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	m_isEnableEITSched = false;


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_dumpCaches (CThreadMgrIf *pIf)
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
		dumpProgramInfos();
		break;

	case 1:
		dumpServiceInfos();
		break;

	case 2:
		dumpEventPfInfos();
		break;

	case 3:
		dumpNetworkInfo();
		break;

	default:
		break;
	}


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CPsisiManager::onReq_dumpTables (CThreadMgrIf *pIf)
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
		m_programMap.dump();
		break;

	case EN_PSISI_TYPE__EIT_H_PF:
		mEIT_H.dumpTables();
		break;

	case EN_PSISI_TYPE__EIT_H_PF_simple:
		mEIT_H.dumpTables_simple();
		break;

	case EN_PSISI_TYPE__EIT_H_SCHED:
		mEIT_H_sched.dumpTables();
		break;

	case EN_PSISI_TYPE__EIT_H_SCHED_event:
		mEIT_H_sched.dumpTables_event();
		break;

	case EN_PSISI_TYPE__EIT_H_SCHED_simple:
		mEIT_H_sched.dumpTables_simple();
		break;

	case EN_PSISI_TYPE__NIT:
		mNIT.dumpTables();
		break;

	case EN_PSISI_TYPE__SDT:
		mSDT.dumpTables();
		break;

	case EN_PSISI_TYPE__CAT:
		mCAT.dumpTables();
		break;

	case EN_PSISI_TYPE__CDT:
		mCDT.dumpTables();
		break;

//	case EN_PSISI_TYPE__RST:
//		mRST.dumpTables();
//		break;

//	case EN_PSISI_TYPE__BIT:
//		mBIT.dumpTables();
//		break;

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

	EN_TUNER_STATE enState = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
	switch (enState) {
	case EN_TUNER_STATE__TUNING_BEGIN: {
		_UTL_LOG_I ("EN_TUNER_STATE__TUNING_BEGIN");

		// fire notify
		m_state = EN_PSISI_STATE__NOT_READY;
		pIf->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));

		m_tunerIsTuned = false;

//#ifdef _DUMMY_TUNER 
//		// ダミーデバッグ中はここでクリア
//		mPAT.clear();
//		mEIT_H.clear();
//		mNIT.clear();
//		mSDT.clear();
//		mRST.clear();
//		mBIT.clear();
//
//		m_EIT_H_comp_flag.clear();
//		m_NIT_comp_flag.clear();
//		m_SDT_comp_flag.clear();
//#endif

		}
		break;

	case EN_TUNER_STATE__TUNING_SUCCESS: {
		_UTL_LOG_I ("EN_TUNER_STATE__TUNING_SUCCESS");

//#ifndef _DUMMY_TUNER
		// 選局後の安定化
		uint32_t opt = getRequestOption ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		setRequestOption (opt);

		requestAsync (EN_MODULE_PSISI_MANAGER + getGroupId(), EN_SEQ_PSISI_MANAGER__STABILIZATION_AFTER_TUNING);

		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		setRequestOption (opt);
//#endif

		}
		break;

	case EN_TUNER_STATE__TUNING_ERROR_STOP: {
		_UTL_LOG_I ("EN_TUNER_STATE__TUNING_ERROR_STOP");

		// fire notify
		m_state = EN_PSISI_STATE__NOT_READY;
		pIf->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));


		// tune stopで一連の動作が終わった時の対策
		clearProgramInfos();
		clearNetworkInfo();
		clearServiceInfos (true);
		clearEventPfInfos();

		m_tunerIsTuned = false;

		}
		break;

	case EN_TUNER_STATE__TUNE_STOP: {
		_UTL_LOG_I ("EN_TUNER_STATE__TUNE_STOP");

		// fire notify
		m_state = EN_PSISI_STATE__NOT_READY;
		pIf->notify (NOTIFY_CAT__PSISI_STATE, (uint8_t*)&m_state, sizeof(m_state));


		// tune stopで一連の動作が終わった時の対策
		clearProgramInfos();
		clearNetworkInfo();
		clearServiceInfos (true);
		clearEventPfInfos();

		m_tunerIsTuned = false;

		}
		break;

	default:
		break;
	}
}

void CPsisiManager::cacheProgramInfos (void)
{
	int n = 0;

	if (mPAT.getTables()->size() == 0) {
		return;
	}

	// PATは一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
	std::vector<CProgramAssociationTable::CTable*>::const_iterator iter = mPAT.getTables()->end();
	CProgramAssociationTable::CTable* latest = *(-- iter);
	if (!latest) {
		return;
	}
	CProgramAssociationTable::CTable *pTable = latest;

	uint8_t tbl_id = pTable->header.table_id;
	uint16_t ts_id = pTable->header.table_id_extension;

	std::vector<CProgramAssociationTable::CTable::CProgram>::const_iterator iter_prog = pTable->programs.begin();
	for (; iter_prog != pTable->programs.end(); ++ iter_prog) {

		if (n >= PROGRAM_INFOS_MAX) {
			return ;
		}

		m_programInfos[n].table_id = tbl_id;
		m_programInfos[n].transport_stream_id = ts_id;

		m_programInfos[n].program_number = iter_prog->program_number;
		m_programInfos[n].program_map_PID = iter_prog->program_map_PID;

		m_programInfos[n].p_orgTable = pTable;
		m_programInfos[n].is_used = true;

		// cache pmt streams
		std::shared_ptr<CProgramMapTable> pmt = m_programMap.get(iter_prog->program_map_PID);
		if (pmt != nullptr && pmt->getTables()->size() > 0) {
			// PMTも一つしかないことを期待していますが 念の為最新(最後尾)のものを参照します
			std::vector<CProgramMapTable::CTable*>::const_iterator iter = pmt->getTables()->end();
			CProgramMapTable::CTable* latest = *(-- iter);
			if (latest) {
				CProgramMapTable::CTable *pTable = latest;

				for (const auto & stream : pTable->streams) {
					std::shared_ptr<_PROGRAM_INFO::_stream>s = make_shared<_PROGRAM_INFO::_stream>();
					s->type = stream.stream_type;
					s->pid = stream.elementary_PID;
					m_programInfos[n].streams.push_back(s);
				}
			}
		}

		++ n;

	} // loop programs
}

void CPsisiManager::dumpProgramInfos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		if (m_programInfos [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_programInfos [i].dump();
		}
	}
}

void CPsisiManager::clearProgramInfos (void)
{
	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		m_programInfos [i].clear();
	}
}

// streamInfo for request
int CPsisiManager::getStreamInfos (uint16_t program_number, EN_STREAM_TYPE type, PSISI_STREAM_INFO *p_out_streamInfos, int num)
{
	if (!p_out_streamInfos || num == 0) {
		return 0;
	}

	int r = 0;

	for (int i = 0; i < PROGRAM_INFOS_MAX; ++ i) {
		if (m_programInfos[i].program_number == program_number) {
			for (const auto& stream : m_programInfos[i].streams) {
				if (r >= num) {
					break;
				}
		
				if (stream->type == type) {
					(p_out_streamInfos + r)->type = stream->type;
					(p_out_streamInfos + r)->pid = stream->pid;
					++ r;
				}
			}

			break;
		}
	}
	
	return r;
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
	std::vector<CServiceDescriptionTable::CTable*>::const_iterator iter = mSDT.getTables()->begin();
	for (; iter != mSDT.getTables()->end(); ++ iter) {

		CServiceDescriptionTable::CTable *pTable = *iter;
		uint8_t tbl_id = pTable->header.table_id;
		uint16_t ts_id = pTable->header.table_id_extension;
		uint16_t network_id = pTable->original_network_id;

		// 自ストリームのみ
		if (tbl_id != TBLID_SDT_A) {
			continue;
		}

		std::vector<CServiceDescriptionTable::CTable::CService>::const_iterator iter_svc = pTable->services.begin();
		for (; iter_svc != pTable->services.end(); ++ iter_svc) {

			_SERVICE_INFO * pInfo = findServiceInfo (tbl_id, ts_id, network_id, iter_svc->service_id);
			if (!pInfo) {
				pInfo = findEmptyServiceInfo();
				if (!pInfo) {
					_UTL_LOG_E ("cacheServiceInfos failure.");
					return;
				}
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

			pInfo->p_orgTable = pTable;
			pInfo->is_used = true;
			pInfo->last_update.setCurrentTime();

		} // loop services

	} // loop tables
}

_SERVICE_INFO* CPsisiManager::findServiceInfo (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
)
{
	if (
		(_table_id == 0) &&
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return NULL;
	}

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_serviceInfos [i].is_used) {
			if (
				(_table_id == m_serviceInfos [i].table_id) &&
				(_transport_stream_id == m_serviceInfos [i].transport_stream_id) &&
				(_original_network_id == m_serviceInfos [i].original_network_id) &&
				(_service_id == m_serviceInfos [i].service_id)
			) {
				return &m_serviceInfos [i];
			}
		}
	}

	// not existed
	return NULL;
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

bool CPsisiManager::isExistServiceTable (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
)
{
	if (
		(_table_id == 0) &&
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return false;
	}

	std::vector<CServiceDescriptionTable::CTable*>::const_iterator iter = mSDT.getTables()->begin();
	for (; iter != mSDT.getTables()->end(); ++ iter) {

		CServiceDescriptionTable::CTable *pTable = *iter;
		uint8_t tbl_id = pTable->header.table_id;
		uint16_t ts_id = pTable->header.table_id_extension;
		uint16_t network_id = pTable->original_network_id;

		std::vector<CServiceDescriptionTable::CTable::CService>::const_iterator iter_svc = pTable->services.begin();
		for (; iter_svc != pTable->services.end(); ++ iter_svc) {

			uint16_t service_id = iter_svc->service_id;

			if (
				(_table_id == tbl_id) &&
				(_transport_stream_id == ts_id) &&
				(_original_network_id == network_id) &&
				(_service_id == service_id)
			) {
				return true;
			}

		} // loop services

	} // loop tables


	// not existed
	return false;
}

void CPsisiManager::assignFollowEventToServiceInfos (void)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_serviceInfos [i].is_tune_target) {
			if (!m_serviceInfos [i].eventFollowInfo.is_used) {

				_EVENT_PF_INFO* p_info = findEventPfInfo (
												TBLID_EIT_PF_A,
												m_serviceInfos [i].transport_stream_id,
												m_serviceInfos [i].original_network_id,
												m_serviceInfos [i].service_id,
												EN_EVENT_PF_STATE__FOLLOW
											);
				if (p_info) {
//_UTL_LOG_I ("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
//p_info->dump();
//					m_serviceInfos [i].eventFollowInfo = *p_info;
//					memcpy (&m_serviceInfos [i].eventFollowInfo, p_info, sizeof(_EVENT_PF_INFO));
					m_serviceInfos [i].eventFollowInfo.clear();
					m_serviceInfos [i].eventFollowInfo.start_time = p_info->start_time;
					m_serviceInfos [i].eventFollowInfo.end_time = p_info->end_time;
					m_serviceInfos [i].eventFollowInfo.is_used = true;
					m_serviceInfos [i].last_update.setCurrentTime();
//_UTL_LOG_I ("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
//m_serviceInfos [i].eventFollowInfo.dump();
				}
			}
		}
	}
}

void CPsisiManager::checkFollowEventAtServiceInfos (CThreadMgrIf *pIf)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_serviceInfos [i].is_tune_target) {
			if (m_serviceInfos [i].eventFollowInfo.is_used) {

				CEtime cur_time;
				cur_time.setCurrentTime();

				if (m_serviceInfos [i].eventFollowInfo.start_time <= cur_time) {
					// 次のイベント開始時刻到来
					// event changed

					if (pIf) {
						PSISI_NOTIFY_EVENT_INFO _info;
						_info.table_id = m_serviceInfos [i].eventFollowInfo.table_id;
						_info.transport_stream_id = m_serviceInfos [i].eventFollowInfo.transport_stream_id;
						_info.original_network_id = m_serviceInfos [i].eventFollowInfo.original_network_id;
						_info.service_id = m_serviceInfos [i].eventFollowInfo.service_id;
						_info.event_id = m_serviceInfos [i].eventFollowInfo.event_id;

						// fire notify
						pIf->notify (NOTIFY_CAT__EVENT_CHANGE, (uint8_t*)&_info, sizeof(PSISI_NOTIFY_EVENT_INFO));
					}


					CEtime t_tmp = m_serviceInfos [i].eventFollowInfo.start_time;
					t_tmp.addSec(15);
					// 次イベント開始から15秒以上経過していたら
					// cacheEventPfInfos 実行する
					if (t_tmp < cur_time) {
						_UTL_LOG_E ("more than 15 seconds have passed since the start of the next event.");
						clearEventPfInfos ();
						cacheEventPfInfos ();
						checkEventPfInfos ();
					}

					// 用済みなのでクリアします
					m_serviceInfos [i].eventFollowInfo.clear();

				}

			}
		}
	}
}

void CPsisiManager::dumpServiceInfos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (m_serviceInfos [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_serviceInfos [i].dump();
		}
	}
}

void CPsisiManager::clearServiceInfos (void)
{
	clearServiceInfos (false);
}

void CPsisiManager::clearServiceInfos (bool is_atTuning)
{
	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (is_atTuning) {
			m_serviceInfos [i].clear_atTuning();

		} else {
			m_serviceInfos [i].clear();
		}
	}
}

// serviceInfo for request
int CPsisiManager::getCurrentServiceInfos (PSISI_SERVICE_INFO *p_out_serviceInfos, int num)
{
	if (!p_out_serviceInfos || num == 0) {
		return 0;
	}

	int n = 0;

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		if (n >= num) {
			break;
		}

		if (!m_serviceInfos [i].is_used) {
			continue;
		}

		if (!m_serviceInfos [i].is_tune_target) {
			continue;
		}

		(p_out_serviceInfos + n)->table_id = m_serviceInfos [i].table_id;
		(p_out_serviceInfos + n)->transport_stream_id = m_serviceInfos [i].transport_stream_id;
		(p_out_serviceInfos + n)->original_network_id = m_serviceInfos [i].original_network_id;
		(p_out_serviceInfos + n)->service_id = m_serviceInfos [i].service_id;
		(p_out_serviceInfos + n)->service_type = m_serviceInfos [i].service_type;
		(p_out_serviceInfos + n)->p_service_name_char = m_serviceInfos [i].service_name_char;

		++ n;
	}
	
	return n;
}

void CPsisiManager::cacheEventPfInfos (void)
{
	bool r = false;

	for (int i = 0; i < SERVICE_INFOS_MAX; ++ i) {
		// 今選局中のserviceを探します
		if (!m_serviceInfos [i].is_tune_target) {
			continue;
		}

		uint8_t _table_id = TBLID_EIT_PF_A; // 自ストリームのみ
		uint16_t _transport_stream_id = m_serviceInfos [i].transport_stream_id;
		uint16_t _original_network_id = m_serviceInfos [i].original_network_id;
		uint16_t _service_id = m_serviceInfos [i].service_id;
		if (
			(_table_id == 0) &&
			(_transport_stream_id == 0) &&
			(_original_network_id == 0) &&
			(_service_id == 0)
		) {
			continue;
		}

		r = cacheEventPfInfos (_table_id, _transport_stream_id, _original_network_id, _service_id);

	}

	if (!r) {
		_UTL_LOG_D ("(cacheEventPfInfos) not match");
	}
}

/**
 * 引数をキーにテーブル内容を_EVENT_PF_INFOに保存します
 * 少なくともp/fの２つ分は見つかるはず
 */
bool CPsisiManager::cacheEventPfInfos (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
)
{
	if (
		(_table_id == 0) &&
		(_transport_stream_id == 0) &&
		(_original_network_id == 0) &&
		(_service_id == 0)
	) {
		return false;
	}

	int m = 0;

//	std::lock_guard<std::recursive_mutex> lock (*mEIT_H_ref.mpMutex);

//	std::vector<CEventInformationTable::CTable*>::const_iterator iter = mEIT_H_ref.mpTables->begin();
//	for (; iter != mEIT_H_ref.mpTables->end(); ++ iter) {
	std::vector<CEventInformationTable::CTable*>::const_iterator iter = mEIT_H.getTables()->begin();
	for (; iter != mEIT_H.getTables()->end(); ++ iter) {
		// p/fでそれぞれ別のテーブル

		CEventInformationTable::CTable *pTable = *iter;

		// キーが一致したテーブルをみます
		if (
			(_table_id != pTable->header.table_id) ||
			(_transport_stream_id != pTable->transport_stream_id) ||
			(_original_network_id != pTable->original_network_id) ||
			(_service_id != pTable->header.table_id_extension)
		) {
			continue;
		}

		_EVENT_PF_INFO * pInfo = findEmptyEventPfInfo ();
		if (!pInfo) {
			_UTL_LOG_E ("getEventPfByServiceId failure.");
			return false;
		}

		pInfo->table_id = pTable->header.table_id;
		pInfo->transport_stream_id = pTable->transport_stream_id;
		pInfo->original_network_id = pTable->original_network_id;
		pInfo->service_id = pTable->header.table_id_extension;

		std::vector<CEventInformationTable::CTable::CEvent>::const_iterator iter_event = pTable->events.begin();
		for (; iter_event != pTable->events.end(); ++ iter_event) {
			// p/fのテーブルにつき eventは一つだと思われる...  いちおうforでまわしておく

			if (iter_event->event_id == 0) {
				// event情報が付いてない
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
					strncpy (pInfo->event_name_char, aribstr, MAXSECLEN);
				}

			} // loop descriptors

		} // loop events


		if (pInfo->event_id == 0) {
			// event情報が付いてない
			// cancel
			pInfo->clear();
			continue;
		}

		pInfo->p_orgTable = pTable;
		pInfo->is_used = true;
		++ m;

//		_UTL_LOG_I ("cacheEventPfInfos 0x%04x 0x%04x 0x%04x 0x%04x",
//			pInfo->transport_stream_id,
//			pInfo->original_network_id,
//			pInfo->service_id,
//			pInfo->event_id
//		);
	} // loop tables


	if (m == 0) {
		return false;
	} else {
		return true;
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
			cur_time.setCurrentTime();

			if (m_eventPfInfos [i].start_time <= cur_time && m_eventPfInfos [i].end_time >= cur_time) {

				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__PRESENT;

			} else if (m_eventPfInfos [i].start_time > cur_time) {

				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__FOLLOW;

			} else if (m_eventPfInfos [i].end_time < cur_time) {

				m_eventPfInfos [i].state = EN_EVENT_PF_STATE__ALREADY_PASSED;

			} else {
				_UTL_LOG_E ("checkEventPfInfos ???");
			}
		}
	}
}

void CPsisiManager::refreshEventPfInfos (void)
{
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_eventPfInfos [i].is_used) {
			if (m_eventPfInfos [i].state == EN_EVENT_PF_STATE__ALREADY_PASSED) {

//TODO parserがわで同等の処理しているはず
//				mEIT_H.clear_pf (m_eventPfInfos [i].p_orgTable);

				m_eventPfInfos [i].clear();
			}
		}
	}
}

void CPsisiManager::dumpEventPfInfos (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (m_eventPfInfos [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_eventPfInfos [i].dump();
		}
	}
}

void CPsisiManager::clearEventPfInfos (void)
{
//	_UTL_LOG_I ("clearEventPfInfos");
	for (int i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		m_eventPfInfos [i].clear();
	}
}

// eventPfInfo for request
_EVENT_PF_INFO* CPsisiManager::findEventPfInfo (
	uint8_t _table_id,
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	EN_EVENT_PF_STATE state
)
{
	int i = 0;
	for (i = 0; i < EVENT_PF_INFOS_MAX; ++ i) {
		if (!m_eventPfInfos [i].is_used) {
			continue;
		}

#ifndef _DUMMY_TUNER // デバッグ中はp/f関係なしに
		if (m_eventPfInfos [i].state != state) {
			continue;
		}
#endif

		if (
			(m_eventPfInfos [i].table_id == _table_id) &&
			(m_eventPfInfos [i].transport_stream_id == _transport_stream_id) &&
			(m_eventPfInfos [i].original_network_id == _original_network_id) &&
			(m_eventPfInfos [i].service_id == _service_id)
		) {
			break;
		}
	}

	if (i == EVENT_PF_INFOS_MAX) {
//		_UTL_LOG_W ("findEventPfInfo not found.");
		return NULL;
	}

	return &m_eventPfInfos[i];
}

void CPsisiManager::cacheNetworkInfo (void)
{
	std::vector<CNetworkInformationTable::CTable*>::const_iterator iter = mNIT.getTables()->begin();
	for (; iter != mNIT.getTables()->end(); ++ iter) {

		CNetworkInformationTable::CTable *pTable = *iter;
		uint8_t tbl_id = pTable->header.table_id;
		uint16_t network_id = pTable->header.table_id_extension;

		// 自ストリームのみ
		if (tbl_id != TBLID_NIT_A) {
			continue;
		}

		m_networkInfo.is_used = true;

		m_networkInfo.table_id = tbl_id;
		m_networkInfo.network_id = network_id;

		std::vector<CDescriptor>::const_iterator iter_desc = pTable->descriptors.begin();
		for (; iter_desc != pTable->descriptors.end(); ++ iter_desc) {

			if (iter_desc->tag == DESC_TAG__NETWORK_NAME_DESCRIPTOR) {
				CNetworkNameDescriptor nnd (*iter_desc);
				if (!nnd.isValid) {
					_UTL_LOG_W ("invalid NetworkNameDescriptor");
					continue;
				}

				char aribstr [MAXSECLEN];
				memset (aribstr, 0x00, MAXSECLEN);
				memset (m_networkInfo.network_name_char, 0x00, 64);
				AribToString (aribstr, (const char*)nnd.name_char, (int)nnd.length);
				strncpy (m_networkInfo.network_name_char, aribstr, 64);
			}
		} // loop descriptors

		std::vector<CNetworkInformationTable::CTable::CStream>::const_iterator iter_strm = pTable->streams.begin();
		for (; iter_strm != pTable->streams.end(); ++ iter_strm) {

			m_networkInfo.transport_stream_id = iter_strm->transport_stream_id;
			m_networkInfo.original_network_id = iter_strm->original_network_id;

			std::vector<CDescriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
			for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {

				if (iter_desc->tag == DESC_TAG__SERVICE_LIST_DESCRIPTOR) {
					CServiceListDescriptor sld (*iter_desc);
					if (!sld.isValid) {
						_UTL_LOG_W ("invalid ServiceListDescriptor");
						continue;
					}

					int n = 0;
					std::vector<CServiceListDescriptor::CService>::const_iterator iter_sv = sld.services.begin();
					for (; iter_sv != sld.services.end(); ++ iter_sv) {
						if (n >= 16) {
							break;
						}
						m_networkInfo.services[n].service_id = iter_sv->service_id;
						m_networkInfo.services[n].service_type = iter_sv->service_type;
						++ n;
					}
					m_networkInfo.services_num = n;

				} else if (iter_desc->tag == DESC_TAG__TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR) {
					CTerrestrialDeliverySystemDescriptor tdsd (*iter_desc);
					if (!tdsd.isValid) {
						_UTL_LOG_W ("invalid TerrestrialDeliverySystemDescriptor");
						continue;
					}

					m_networkInfo.area_code = tdsd.area_code;
					m_networkInfo.guard_interval = tdsd.guard_interval;
					m_networkInfo.transmission_mode = tdsd.transmission_mode;

				} else if (iter_desc->tag == DESC_TAG__TS_INFORMATION_DESCRIPTOR) {
					CTSInformationDescriptor tsid (*iter_desc);
					if (!tsid.isValid) {
						_UTL_LOG_W ("invalid TSInformationDescriptor");
						continue;
					}

					m_networkInfo.remote_control_key_id = tsid.remote_control_key_id;

					char aribstr [MAXSECLEN];
					memset (aribstr, 0x00, MAXSECLEN);
					memset (m_networkInfo.ts_name_char, 0x00, 64);
					AribToString (aribstr, (const char*)tsid.ts_name_char, (int)tsid.length_of_ts_name);
					strncpy (m_networkInfo.ts_name_char, aribstr, 64);
				}

			} // loop descriptors

			// streamは基本１つだろうことを期待
			break;

		} // loop streams

		// tableも基本１つだろうことを期待
		break;

	} // loop tables
}

void CPsisiManager::dumpNetworkInfo (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	if (m_networkInfo.is_used) {
		m_networkInfo.dump();
	}
}

void CPsisiManager::clearNetworkInfo (void)
{
	m_networkInfo.clear();
}

#include "Pngcrc.h"
void CPsisiManager::storeLogo (void)
{
	if (m_state == EN_PSISI_STATE__NOT_READY) {
		return;
	}

	const auto *p_tables = mCDT.getTables();
	if (!p_tables) {
		return;
	}

	CPngcrc pngcrc;

	for (auto it = p_tables->cbegin(); it != p_tables->cend(); ++ it) {
		if ((*it)->data_type != 0x01) {
			continue;
		}

		size_t buff_plte_size = 4 + 4 + (128 * 3) + 4;
		uint8_t buff_plte [buff_plte_size] = {0};
		{
			uint8_t *p = &buff_plte[0];
			*p = ((128 * 3) >> 24) & 0xff; ++ p;
			*p = ((128 * 3) >> 16) & 0xff; ++ p;
			*p = ((128 * 3) >> 8) & 0xff; ++ p;
			*p = (128 * 3) & 0xff; ++ p;
			*p = 'P'; ++ p;
			*p = 'L'; ++ p;
			*p = 'T'; ++ p;
			*p = 'E'; ++ p;
			for (int i = 0; i < 128; ++ i) {
				*p = g_commonPaletCLUT[i][0];
				++ p;
				*p = g_commonPaletCLUT[i][1];
				++ p;
				*p = g_commonPaletCLUT[i][2];
				++ p;
			}
			uint32_t _crc = pngcrc.crc(&buff_plte[4], 4 + (128 * 3));
			*p = (_crc >> 24) & 0xff; ++ p;
			*p = (_crc >> 16) & 0xff; ++ p;
			*p = (_crc >> 8) & 0xff; ++ p;
			*p = _crc & 0xff;
		}

		size_t buff_trns_size = 4 + 4 + 128 + 4;
		uint8_t buff_trns [buff_trns_size] = {0};
		{
			uint8_t *p = &buff_trns[0];
			*p = (128 >> 24) & 0xff; ++ p;
			*p = (128 >> 16) & 0xff; ++ p;
			*p = (128 >> 8) & 0xff; ++ p;
			*p = 128 & 0xff; ++ p;
			*p = 't'; ++ p;
			*p = 'R'; ++ p;
			*p = 'N'; ++ p;
			*p = 'S'; ++ p;
			for (int i = 0; i < 128; ++ i) {
				*p = g_commonPaletCLUT[i][3];
				++ p;
			}
			uint32_t _crc = pngcrc.crc(&buff_trns[4], 4 + 128);
			*p = (_crc >> 24) & 0xff; ++ p;
			*p = (_crc >> 16) & 0xff; ++ p;
			*p = (_crc >> 8) & 0xff; ++ p;
			*p = _crc & 0xff;
		}

		size_t png_include_plte_size = buff_plte_size + buff_trns_size + (*it)->data.data_size;
		uint8_t png_include_plte [png_include_plte_size] = {0};
		{
			uint8_t *p = png_include_plte;
			// 元pngのヘッダをコピーします
			memcpy (p, (*it)->data.data_byte.get(), 33);
			p += 33;

			// PLTEチャンクをコピー(挿入)します
			memcpy (p, buff_plte, buff_plte_size); 
			p += buff_plte_size;

			// tRNSチャンクをコピー(挿入)します
			memcpy (p, buff_trns, buff_trns_size);
			p += buff_trns_size;

			// 元pngの残りをコピーします
			memcpy (p, (*it)->data.data_byte.get() + 33, (*it)->data.data_size - 33);
		}

		std::string *p_path = mp_settings->getParams()->getLogoPath();
		char _name[PATH_MAX] = {0};
		snprintf (
			_name,
			sizeof(_name),
			"%s/logo_%s_0x%04x_0x%04x_0x%04x_0x%02x%04x%04x.png",
			p_path->c_str(),
			m_networkInfo.ts_name_char,
			m_networkInfo.transport_stream_id,
			m_networkInfo.original_network_id,
			(*it)->header.table_id_extension,
			(*it)->data.logo_type,
			(*it)->data.logo_id,
			(*it)->data.logo_version
		);
		std::ofstream ofs (_name, std::ios::out|ios::binary);
		ofs.write((char*)png_include_plte, png_include_plte_size);
		ofs.flush();
		ofs.close();
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


	switch (p_ts_header->pid) {
	case PID_PAT: {

		_PARSER_NOTICE _notice (EN_PSISI_TYPE__PAT, p_ts_header, p_payload, payload_size);
		requestAsync (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_EIT_H: {

		_PARSER_NOTICE _notice (EN_PSISI_TYPE__EIT_H_PF, p_ts_header, p_payload, payload_size);
		requestAsync (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_NIT: {

		_PARSER_NOTICE _notice (EN_PSISI_TYPE__NIT, p_ts_header, p_payload, payload_size);
		requestAsync (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_SDT: {

		_PARSER_NOTICE _notice (EN_PSISI_TYPE__SDT, p_ts_header, p_payload, payload_size);
		requestAsync (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_CAT: {

		_PARSER_NOTICE _notice (EN_PSISI_TYPE__CAT, p_ts_header, p_payload, payload_size);
		requestAsync (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

	case PID_CDT: {

		_PARSER_NOTICE _notice (EN_PSISI_TYPE__CDT, p_ts_header, p_payload, payload_size);
		requestAsync (
			EN_MODULE_PSISI_MANAGER + getGroupId(),
			EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		}
		break;

//	case PID_RST:
//		r = mRST.checkSection (p_ts_header, p_payload, payload_size);
//		break;

//	case PID_BIT:
//		r = mBIT.checkSection (p_ts_header, p_payload, payload_size);
//		break;

	default:
		// parse PMT
		if (m_programMap.has(p_ts_header->pid)) {
			_PARSER_NOTICE _notice (EN_PSISI_TYPE__PMT, p_ts_header, p_payload, payload_size);
			requestAsync (
				EN_MODULE_PSISI_MANAGER + getGroupId(),
				EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);
		}

		break;
	}


	return true;
}


//////////  CEventInformationTable_sched::IEventScheduleHandler  //////////

void CPsisiManager::onScheduleUpdate (void)
{
//	_UTL_LOG_I (__PRETTY_FUNCTION__);


	EN_PSISI_TYPE _type = EN_PSISI_TYPE__EIT_H_SCHED;

	requestAsync (
		EN_MODULE_EVENT_SCHEDULE_MANAGER,
		EN_SEQ_EVENT_SCHEDULE_MANAGER__PARSER_NOTICE,
		(uint8_t*)&_type,
		sizeof(_type)
	);

}
