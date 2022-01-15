#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Group.h"
#include "RecInstance.h"
#include "RecManager.h"
#include "RecManagerIf.h"

#include "Utils.h"
#include "modules.h"

#include "aribstr.h"
#include "threadmgr_if.h"



static const struct reserve_state_pair g_reserveStatePair [] = {
	{0x00000000, "INIT"},
	{0x00000001, "REMOVED"},
	{0x00000002, "RESCHEDULED"},
	{0x00000004, "START_TIME_PASSED"},
	{0x00000100, "REQ_START_RECORDING"},
	{0x00000200, "NOW_RECORDING"},
	{0x00000400, "FORCE_STOPPED"},
	{0x00010000, "END_ERROR__ALREADY_PASSED"},
	{0x00020000, "END_ERROR__HDD_FREE_SPACE_LOW"},
	{0x00040000, "END_ERROR__TUNE_ERR"},
	{0x00080000, "END_ERROR__EVENT_NOT_FOUND"},
	{0x00100000, "END_ERROR__INTERNAL_ERR"},
	{0x80000000, "END_SUCCESS"},

	{0x00000000, NULL}, // term
};

static char gsz_reserveState [256];

const char * getReserveStateString (uint32_t state)
{
	memset (gsz_reserveState, 0x00, sizeof(gsz_reserveState));
	int n = 0;
	int s = 0;
	char *pos = gsz_reserveState;
	int _size = sizeof(gsz_reserveState);

	while (g_reserveStatePair [n].psz_reserveState != NULL) {
		if (state & g_reserveStatePair [n].state) {
			s = snprintf (pos, _size, "%s,", g_reserveStatePair [n].psz_reserveState);
			pos += s;
			_size -= s;
		}
		++ n ;
	}

	// INIT
	if (pos == gsz_reserveState) {
		strncpy (
			gsz_reserveState,
			g_reserveStatePair[0].psz_reserveState,
			sizeof(gsz_reserveState) -1
		);
	}

	return gsz_reserveState;
}

const char *g_repeatability [] = {
	"NONE",
	"DAILY",
	"WEEKLY",
	"AUTO",
};


////typedef struct {
////	EN_REC_PROGRESS rec_progress;
////	uint32_t reserve_state;
////} RECORDING_NOTICE_t;


CRecManager::CRecManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
////	,m_tunerNotify_clientId (0xff)
////	,m_tsReceive_handlerId (-1)
////	,m_patDetectNotify_clientId (0xff)
////	,m_eventChangeNotify_clientId (0xff)
////	,m_recProgress (EN_REC_PROGRESS__INIT)
	,m_tuner_resource_max (0)
{
	SEQ_BASE_t seqs [EN_SEQ_REC_MANAGER__NUM] = {
		{(PFN_SEQ_BASE)&CRecManager::onReq_moduleUp, (char*)"onReq_moduleUp"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_moduleUpByGroupId, (char*)"onReq_moduleUpByGroupId"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_moduleDown, (char*)"onReq_moduleDown"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_checkLoop, (char*)"onReq_checkLoop"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_checkReservesEventLoop, (char*)"onReq_checkReservesEventLoop"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_checkRecordingsEventLoop, (char*)"onReq_checkRecordingsEventLoop"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_recordingNotice, (char*)"onReq_recordingNotice"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_startRecording, (char*)"onReq_startRecording"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_addReserve_currentEvent, (char*)"onReq_addReserve_currentEvent"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_addReserve_event, (char*)"onReq_addReserve_event"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_addReserve_eventHelper, (char*)"onReq_addReserve_eventHelper"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_addReserve_manual, (char*)"onReq_addReserve_manual"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_removeReserve, (char*)"onReq_removeReserve"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_removeReserve_byIndex, (char*)"onReq_removeReserve_byIndex"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_getReserves, (char*)"onReq_getReserves"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_stopRecording, (char*)"onReq_stopRecording"},
		{(PFN_SEQ_BASE)&CRecManager::onReq_dumpReserves, (char*)"onReq_dumpReserves"},
	};
	setSeqs (seqs, EN_SEQ_REC_MANAGER__NUM);


	mp_settings = CSettings::getInstance();

	clearReserves ();
	clearResults ();
////	m_recordings[0].clear();
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_recordings[_gr].clear();
	}

////	memset (m_recording_tmpfile, 0x00, sizeof (m_recording_tmpfile));
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		memset (m_recording_tmpfiles[_gr], 0x00, PATH_MAX);
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_tunerNotify_clientId [_gr] = 0xff;
		m_tsReceive_handlerId [_gr] = -1;
		m_patDetectNotify_clientId [_gr] = 0xff;
		m_eventChangeNotify_clientId [_gr] = 0xff;
	}
}

CRecManager::~CRecManager (void)
{
	clearReserves ();
	clearResults ();
////	m_recordings[0].clear();
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_recordings[_gr].clear();
	}

////	memset (m_recording_tmpfile, 0x00, sizeof (m_recording_tmpfile));
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		memset (m_recording_tmpfiles[_gr], 0x00, PATH_MAX);
	}

	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		m_tunerNotify_clientId [_gr] = 0xff;
		m_tsReceive_handlerId [_gr] = -1;
		m_patDetectNotify_clientId [_gr] = 0xff;
		m_eventChangeNotify_clientId [_gr] = 0xff;
	}
}


void CRecManager::onReq_moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_MODULE_UP_BY_GROUPID,
		SECTID_WAIT_MODULE_UP_BY_GROUPID,
		SECTID_REQ_CHECK_LOOP,
		SECTID_WAIT_CHECK_LOOP,
		SECTID_REQ_CHECK_RESERVES_EVENT_LOOP,
		SECTID_WAIT_CHECK_RESERVES_EVENT_LOOP,
		SECTID_REQ_CHECK_RECORDINGS_EVENT_LOOP,
		SECTID_WAIT_CHECK_RECORDINGS_EVENT_LOOP,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static uint8_t s_gr_cnt = 0;

	switch (sectId) {
	case SECTID_ENTRY:

		// settingsを使って初期化する場合はmodule upで
		m_tuner_resource_max = CSettings::getInstance()->getParams()->getTunerHalAllocates()->size(); 

		loadReserves ();
		loadResults ();

		// recInstance グループ数分の生成はここで
		// getExternalIf()メンバ使ってるからコンストラクタ上では実行不可
		for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
			std::unique_ptr <CRecInstance> _r(new CRecInstance(getExternalIf(), _gr));
			msp_rec_instances[_gr].swap(_r);
			mp_ts_handlers[_gr] = msp_rec_instances[_gr].get();
		}

		sectId = SECTID_REQ_MODULE_UP_BY_GROUPID;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_MODULE_UP_BY_GROUPID:
		requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__MODULE_UP_BY_GROUPID, (uint8_t*)&s_gr_cnt, sizeof(s_gr_cnt));
		++ s_gr_cnt;

		sectId = SECTID_WAIT_MODULE_UP_BY_GROUPID;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_MODULE_UP_BY_GROUPID:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			if (s_gr_cnt < CGroup::GROUP_MAX) {
				sectId = SECTID_REQ_MODULE_UP_BY_GROUPID;
				enAct = EN_THM_ACT_CONTINUE;
			} else {
				sectId = SECTID_REQ_CHECK_LOOP;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_CHECK_LOOP:
		requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__CHECK_LOOP);

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

		sectId = SECTID_REQ_CHECK_RESERVES_EVENT_LOOP;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_CHECK_RESERVES_EVENT_LOOP:
		requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__CHECK_RESERVES_EVENT_LOOP);

		sectId = SECTID_WAIT_CHECK_RESERVES_EVENT_LOOP;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_CHECK_RESERVES_EVENT_LOOP:
//		enRslt = pIf->getSrcInfo()->enRslt;
//		if (enRslt == EN_THM_RSLT_SUCCESS) {
//
//		} else {
//
//		}
// EN_THM_RSLT_SUCCESSのみ

		sectId = SECTID_REQ_CHECK_RECORDINGS_EVENT_LOOP;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_CHECK_RECORDINGS_EVENT_LOOP:
		requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__CHECK_RECORDINGS_EVENT_LOOP);

		sectId = SECTID_WAIT_CHECK_RECORDINGS_EVENT_LOOP;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_WAIT_CHECK_RECORDINGS_EVENT_LOOP:
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
		s_gr_cnt = 0;
		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:
		s_gr_cnt = 0;
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}


	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_moduleUpByGroupId (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_REG_TUNER_NOTIFY,
		SECTID_WAIT_REG_TUNER_NOTIFY,
		SECTID_REQ_REG_HANDLER,
		SECTID_WAIT_REG_HANDLER,
		SECTID_REQ_REG_PAT_DETECT_NOTIFY,
		SECTID_WAIT_REG_PAT_DETECT_NOTIFY,
		SECTID_REQ_REG_EVENT_CHANGE_NOTIFY,
		SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static uint8_t s_groupId = 0;

	switch (sectId) {
	case SECTID_ENTRY:

		// request msg から groupId を取得します
		s_groupId = *(uint8_t*)(getIf()->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I("(%s) groupId:[%d]", pIf->getSeqName(), s_groupId);

		sectId = SECTID_REQ_REG_TUNER_NOTIFY;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_REG_TUNER_NOTIFY: {
		CTunerControlIf _if (getExternalIf(), s_groupId);
		_if.reqRegisterTunerNotify ();

		sectId = SECTID_WAIT_REG_TUNER_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_TUNER_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_tunerNotify_clientId [s_groupId] = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_HANDLER;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTunerNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_HANDLER: {

		_UTL_LOG_I ("CTunerControlIf::ITsReceiveHandler %p", msp_rec_instances[s_groupId].get());
		CTunerControlIf _if (getExternalIf(), s_groupId);
		_if.reqRegisterTsReceiveHandler (&(mp_ts_handlers[s_groupId]));

		sectId = SECTID_WAIT_REG_HANDLER;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_HANDLER:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_tsReceive_handlerId [s_groupId] = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_PAT_DETECT_NOTIFY;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTsReceiveHandler is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_PAT_DETECT_NOTIFY: {
		CPsisiManagerIf _if (getExternalIf(), s_groupId);
		_if.reqRegisterPatDetectNotify ();

		sectId = SECTID_WAIT_REG_PAT_DETECT_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_PAT_DETECT_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_patDetectNotify_clientId [s_groupId] = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterPatDetectNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_EVENT_CHANGE_NOTIFY: {
		CPsisiManagerIf _if (getExternalIf(), s_groupId);
		_if.reqRegisterEventChangeNotify ();

		sectId = SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_eventChangeNotify_clientId [s_groupId] = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterEventChangeNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
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

void CRecManager::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CRecManager::onReq_checkLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_WAIT_START_RECORDING,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK:

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT: {

		checkReserves ();

		checkRecordingEnd ();

		checkDiskFree ();

		uint8_t groupId = 0;
		if (isExistReqStartRecordingReserve ()) {
			CTunerServiceIf _if (getExternalIf());
			_if.reqOpenSync ();
			
			enRslt = getIf()->getSrcInfo()->enRslt;
			if (enRslt == EN_THM_RSLT_SUCCESS) {
				groupId = *(uint8_t*)(getIf()->getSrcInfo()->msg.pMsg);
				_UTL_LOG_I ("reqOpen groupId:[0x%02x]", groupId);

				if (m_recordings[groupId].is_used) {
					// m_recordingsのidxはreqOpenで取ったtuner_idで決まります
					// is_usedになってるはずない...
					_UTL_LOG_E ("??? m_recordings[groupId].is_used ???  groupId:[0x%02x]", groupId);
					sectId = SECTID_CHECK;
					enAct = EN_THM_ACT_CONTINUE;
					break;
				}

			} else {
				sectId = SECTID_CHECK;
				enAct = EN_THM_ACT_CONTINUE;
				break;
			}
		}

		if (pickReqStartRecordingReserve (groupId)) {
			// request start recording
			requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__START_RECORDING, (uint8_t*)&groupId, sizeof(uint8_t));

			sectId = SECTID_WAIT_START_RECORDING;
			enAct = EN_THM_ACT_WAIT;

		} else {
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_WAIT_START_RECORDING:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
//TODO imple
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
//TODO imple
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
		}

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

//TODO  eventSchedMgrの stateNotify契機でチェックする方がいいかも
void CRecManager::onReq_checkReservesEventLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK:

		pIf->setTimeout (60*1000*10); // 10min

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT: {

		// EPG取得が完了しているか確認します
		CEventScheduleManagerIf _if (getExternalIf());
		_if.syncGetCacheScheduleState ();
		EN_CACHE_SCHEDULE_STATE_t _s =  *(EN_CACHE_SCHEDULE_STATE_t*)(getIf()->getSrcInfo()->msg.pMsg);
		if (_s != EN_CACHE_SCHEDULE_STATE__READY) {
			// readyでないので以下の処理は行いません
			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
			break;
		}


		// 録画予約に対応するイベント内容ををチェックします
		for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {

			if (!m_reserves [i].is_used) {
				continue;
			}

			if (m_reserves [i].state == RESERVE_STATE__INIT) {
				continue;
			}

			if (!m_reserves [i].is_event_type) {
				continue;
			}


			// スケジュールを確認してstart_time, end_time, event_name が変わってないかチェックします

			CEventScheduleManagerIf::EVENT_t event;
			CEventScheduleManagerIf::REQ_EVENT_PARAM_t param = {
				{
					m_reserves [i].transport_stream_id,
					m_reserves [i].original_network_id,
					m_reserves [i].service_id,
					m_reserves [i].event_id
				},
				&event
			};

			CEventScheduleManagerIf _if (getExternalIf());
			_if.syncGetEvent (&param);
			
			enRslt = getIf()->getSrcInfo()->enRslt;
			if (enRslt == EN_THM_RSLT_SUCCESS) {
				if ( m_reserves [i].start_time != param.p_out_event-> start_time) {
					_UTL_LOG_I (
						"check_event  reserve start_time is update.  [%s] -> [%s]",
						m_reserves [i].start_time.toString(),
						param.p_out_event->start_time.toString()
					);
					m_reserves [i].start_time = param.p_out_event-> start_time;
				}

				if (m_reserves [i].end_time != param.p_out_event-> end_time) {
					_UTL_LOG_I (
						"check_event  reserve end_time is update.  [%s] -> [%s]",
						m_reserves [i].end_time.toString(),
						param.p_out_event->end_time.toString()
					);
					m_reserves [i].end_time = param.p_out_event-> end_time;
				}

				if (m_reserves [i].title_name != *param.p_out_event-> p_event_name) {
					_UTL_LOG_I (
						"check_event  reserve title_name is update.  [%s] -> [%s]",
						m_reserves [i].title_name.c_str(),
						param.p_out_event->p_event_name->c_str()
					);
					m_reserves [i].title_name = param.p_out_event-> p_event_name->c_str();
				}

			} else {
				// 予約に対応するイベントがなかった
				_UTL_LOG_W ("no event corresponding to the reservation...");
				m_reserves [i].dump();
			}

		}


		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;

		}
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

void CRecManager::onReq_checkRecordingsEventLoop (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK,
		SECTID_CHECK_WAIT,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_EVENT_INFO s_presentEventInfo;
	static uint8_t s_groupId = 0;

	switch (sectId) {
	case SECTID_ENTRY:
		// 先にreplyしておく
		pIf->reply (EN_THM_RSLT_SUCCESS);

		sectId = SECTID_CHECK;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_CHECK:

//		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));
		s_presentEventInfo.clear();
		if (s_groupId >= CGroup::GROUP_MAX) {
			s_groupId = 0;
		}

		pIf->setTimeout (1000); // 1sec

		sectId = SECTID_CHECK_WAIT;
		enAct = EN_THM_ACT_WAIT;
		break;

	case SECTID_CHECK_WAIT:

		if (m_recordings[s_groupId].state & RESERVE_STATE__NOW_RECORDING) {
			sectId = SECTID_REQ_GET_PRESENT_EVENT_INFO;
			enAct = EN_THM_ACT_CONTINUE;

		} else {

			++ s_groupId;

			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {

		PSISI_SERVICE_INFO _svc_info ;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = m_recordings[s_groupId].transport_stream_id;
		_svc_info.original_network_id = m_recordings[s_groupId].original_network_id;
		_svc_info.service_id = m_recordings[s_groupId].service_id;

		CPsisiManagerIf _if (getExternalIf(), s_groupId);
		if (_if.reqGetPresentEventInfo (&_svc_info, &s_presentEventInfo)) {
			sectId = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
			enAct = EN_THM_ACT_WAIT;
		} else {
			// キューが入らなかった時用

			++ s_groupId;

			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
//s_presentEventInfo.dump();
			if (m_recordings[s_groupId].state & RESERVE_STATE__NOW_RECORDING) {

				if (m_recordings[s_groupId].is_event_type) {
					// event_typeの録画は event_idとend_timeの確認
					if (m_recordings[s_groupId].event_id == s_presentEventInfo.event_id) {
						if (m_recordings[s_groupId].end_time != s_presentEventInfo.end_time) {
							_UTL_LOG_I (
								"#####  recording end_time is update.  [%s] -> [%s]  #####",
								m_recordings[s_groupId].end_time.toString (),
								s_presentEventInfo.end_time.toString ()
							);
							m_recordings[s_groupId].end_time = s_presentEventInfo.end_time;
							m_recordings[s_groupId].dump();
						}

					} else {
//TODO 録画止めてもいいかも
						// event_idが変わった とりあえずログだしとく
						_UTL_LOG_W (
							"#####  recording event_id changed.  [0x%04x] -> [0x%04x]  #####",
							s_presentEventInfo.event_id,
							m_recordings[s_groupId].event_id
						);
						m_recordings[s_groupId].dump();
						s_presentEventInfo.dump();
					}

				} else {
					// manual録画は event_name_charを代入しときます
					m_recordings[s_groupId].event_id = s_presentEventInfo.event_id;
					m_recordings[s_groupId].title_name = s_presentEventInfo.event_name_char;
				}
			}

		} else {
			_UTL_LOG_E ("(%s) reqGetPresentEventInfo err", pIf->getSeqName());
		}

		++ s_groupId;

		sectId = SECTID_CHECK;
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

void CRecManager::onReq_recordingNotice (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


////	RECORDING_NOTICE_t _notice = *(RECORDING_NOTICE_t*)(pIf->getSrcInfo()->msg.pMsg);
	CRecInstance::RECORDING_NOTICE_t _notice = *(CRecInstance::RECORDING_NOTICE_t*)(pIf->getSrcInfo()->msg.pMsg);
	switch (_notice.rec_progress) {
////	case EN_REC_PROGRESS__INIT:
	case CRecInstance::progress::INIT:
		break;

////	case EN_REC_PROGRESS__PRE_PROCESS:
	case CRecInstance::progress::PRE_PROCESS:
////		// ここでは エラーが起きることがあるのでRESERVE_STATE をチェックします
////		if (_notice.reserve_state != RESERVE_STATE__INIT) {
////			m_recordings[0].state |= _notice.reserve_state;
////		}
		break;

////	case EN_REC_PROGRESS__NOW_RECORDING:
	case CRecInstance::progress::NOW_RECORDING:
		break;

////	case EN_REC_PROGRESS__END_SUCCESS:
	case CRecInstance::progress::END_SUCCESS:
		m_recordings[_notice.groupId].state |= RESERVE_STATE__END_SUCCESS;
		break;

////	case EN_REC_PROGRESS__END_ERROR:
	case CRecInstance::progress::END_ERROR:
		break;

////	case EN_REC_PROGRESS__POST_PROCESS: {
	case CRecInstance::progress::POST_PROCESS: {

		// NOW_RECORDINGフラグは落としておきます
		m_recordings[_notice.groupId].state &= ~RESERVE_STATE__NOW_RECORDING;

		m_recordings[_notice.groupId].recording_end_time.setCurrentTime();

		setResult (&m_recordings[_notice.groupId]);


		// rename
		std::string *p_path = mp_settings->getParams()->getRecTsPath();

		char newfile [PATH_MAX] = {0};
		char *p_name = (char*)"rec";
		if (m_recordings[_notice.groupId].title_name.c_str()) {
			p_name = (char*)m_recordings[_notice.groupId].title_name.c_str();
		}
		CEtime t_end;
		t_end.setCurrentTime();
		snprintf (
			newfile,
			sizeof(newfile),
			"%s/%s_0x%08x_%s.m2ts",
			p_path->c_str(),
			p_name,
			m_recordings[_notice.groupId].state,
			t_end.toString()
		);

		rename (m_recording_tmpfiles[_notice.groupId], newfile) ;

		m_recordings[_notice.groupId].clear ();
		_UTL_LOG_I ("recording end...");


		//-----------------------------//
		{
			uint32_t opt = getRequestOption ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);

			// 選局を停止しときます tune stop
			// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
			CTunerServiceIf _if (getExternalIf());
			_if.reqTuneStop (_notice.groupId);
			_if.reqClose (_notice.groupId);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);
		}
		//-----------------------------//


		}
		break;

	default:
		break;
	};

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_startRecording (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_CHECK_DISK_FREE_SPACE,
//		SECTID_REQ_STOP_CACHE_SCHED,
//		SECTID_WAIT_STOP_CACHE_SCHED,
//		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
//		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_REQ_CACHE_SCHEDULE,
		SECTID_WAIT_CACHE_SCHEDULE,
		SECTID_REQ_ADD_RESERVE_RESCHEDULE,
		SECTID_WAIT_ADD_RESERVE_RESCHEDULE,
		SECTID_START_RECORDING,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_EVENT_INFO s_presentEventInfo;
	static int s_retry_get_event_info;
	static uint8_t s_groupId = 0xff;
//	static uint16_t s_ch = 0;


	switch (sectId) {
	case SECTID_ENTRY:

		_UTL_LOG_I ("(%s) entry", pIf->getSeqName());

		s_groupId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
		if (s_groupId >= m_tuner_resource_max) {
			_UTL_LOG_E ("invalid groupId:[0x%02x]", s_groupId);

			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			m_recordings[s_groupId].dump();

			sectId = SECTID_CHECK_DISK_FREE_SPACE;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_CHECK_DISK_FREE_SPACE: {

		std::string *p_path = mp_settings->getParams()->getRecTsPath();
		int limit = mp_settings->getParams()->getRecDiskSpaceLowLimitMB();
		int df = CUtils::getDiskFreeMB(p_path->c_str());
		_UTL_LOG_I ("(%s) disk free space [%d] Mbytes.", pIf->getSeqName(), df);
		if (df < limit) {
			// HDD残量たりないので 録画実行しません
			m_recordings[s_groupId].state |= RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW;
			setResult (&m_recordings[s_groupId]);
			m_recordings[s_groupId].clear ();

			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
//TODO
//			sectId = SECTID_REQ_STOP_CACHE_SCHED;
//			sectId = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
			sectId = SECTID_REQ_TUNE;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;
/***
	case SECTID_REQ_STOP_CACHE_SCHED: {

		// EPG取得中だったら止めてから録画開始します
		CEventScheduleManagerIf _if (getExternalIf());
		_if.reqStopCacheSchedule ();

		sectId = SECTID_WAIT_STOP_CACHE_SCHED;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_STOP_CACHE_SCHED:
		// successのみ
		sectId = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
		enAct = EN_THM_ACT_CONTINUE;
		break;
***/
/***
	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {

		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			m_recordings[s_groupId].transport_stream_id,
			m_recordings[s_groupId].original_network_id,
			m_recordings[s_groupId].service_id
		};

		CChannelManagerIf _if (getExternalIf());
		_if.reqGetPysicalChannelByServiceId (&param);

		sectId = SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {

			s_ch = *(uint16_t*)(pIf->getSrcInfo()->msg.pMsg);

			sectId = SECTID_REQ_TUNE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqGetPysicalChannelByServiceId is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;
***/
	case SECTID_REQ_TUNE: {

/***
		CTunerServiceIf::tune_param_t param = {
			s_ch,
			s_groupId
		};

		CTunerServiceIf _if (getExternalIf());
		_if.reqTune_withRetry (&param);
***/
		CTunerServiceIf::tune_advance_param_t param = {
			m_recordings[s_groupId].transport_stream_id,
			m_recordings[s_groupId].original_network_id,
			m_recordings[s_groupId].service_id,
			s_groupId,
			true // enable retry
		};

		CTunerServiceIf _if(getExternalIf());
		_if.reqTuneAdvance (&param);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {

			if (m_recordings[s_groupId].is_event_type) {
				// イベント開始時間を確認します
				sectId = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				// manual録画は即 録画開始します
				sectId = SECTID_START_RECORDING;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("reqTune is failure.");
			_UTL_LOG_E ("tune is failure  -> not start recording");

			m_recordings[s_groupId].state |= RESERVE_STATE__END_ERROR__TUNE_ERR;
			setResult (&m_recordings[s_groupId]);
			m_recordings[s_groupId].clear();

			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {

		PSISI_SERVICE_INFO _svc_info ;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = m_recordings[s_groupId].transport_stream_id;
		_svc_info.original_network_id = m_recordings[s_groupId].original_network_id;
		_svc_info.service_id = m_recordings[s_groupId].service_id;

		CPsisiManagerIf _if (getExternalIf(), s_groupId);
		_if.reqGetPresentEventInfo (&_svc_info, &s_presentEventInfo);

		sectId = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {

			if (s_presentEventInfo.event_id == m_recordings[s_groupId].event_id) {
				// 録画開始します
				sectId = SECTID_START_RECORDING;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				// イベントが一致しないので 前番組の延長とかで開始時間が遅れたと予想します
				// --> EIT schedule を取得してみます
				_UTL_LOG_E ("(%s) event tracking...", pIf->getSeqName());
				sectId = SECTID_REQ_CACHE_SCHEDULE;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			if (s_retry_get_event_info >= 10) {
				_UTL_LOG_E ("(%s) reqGetPresentEventInfo err", pIf->getSeqName());

				m_recordings[s_groupId].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
				setResult (&m_recordings[s_groupId]);
				m_recordings[s_groupId].clear();

				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				// workaround
				// たまにエラーになることがあるので 暫定対策として200mS待ってリトライしてみます
				// psi/siの選局完了時に確実にEIT p/fを取得できてないのが直接の原因だと思われます
				_UTL_LOG_W ("(%s) reqGetPresentEventInfo retry", pIf->getSeqName());

				usleep (200000); // 200mS
				++ s_retry_get_event_info;

				sectId = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				enAct = EN_THM_ACT_CONTINUE;
			}
		}
		break;

	case SECTID_REQ_CACHE_SCHEDULE: {

		CEventScheduleManagerIf _if (getExternalIf());
		_if.reqCacheSchedule_forceCurrentService (s_groupId);

		sectId = SECTID_WAIT_CACHE_SCHEDULE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_CACHE_SCHEDULE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {

			// イベントを検索して予約を入れなおしてみます
			_UTL_LOG_E ("(%s) try reserve reschedule", pIf->getSeqName());
			sectId = SECTID_REQ_ADD_RESERVE_RESCHEDULE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("(%s) reqCacheSchedule_forceCurrentService err", pIf->getSeqName());

			m_recordings[s_groupId].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			setResult (&m_recordings[s_groupId]);
			m_recordings[s_groupId].clear();

			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_ADD_RESERVE_RESCHEDULE: {

		CRecManagerIf::ADD_RESERVE_PARAM_t _param;
		_param.transport_stream_id = m_recordings[s_groupId].transport_stream_id ;
		_param.original_network_id = m_recordings[s_groupId].original_network_id;
		_param.service_id = m_recordings[s_groupId].service_id;
		_param.event_id = m_recordings[s_groupId].event_id;
		_param.repeatablity = m_recordings[s_groupId].repeatability;
		_param.dump();

		requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__ADD_RESERVE_EVENT, (uint8_t*)&_param, sizeof(_param));

		sectId = SECTID_WAIT_ADD_RESERVE_RESCHEDULE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_ADD_RESERVE_RESCHEDULE:

		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			_UTL_LOG_I ("(%s) add reserve reschedule ok.", pIf->getSeqName());

			// 今回は録画は行いません
			m_recordings[s_groupId].clear();

			// 選局止めたいのでエラーにしておきます
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("(%s) add reserve reschedule err...", pIf->getSeqName());

			m_recordings[s_groupId].state |= RESERVE_STATE__END_ERROR__EVENT_NOT_FOUND;
			setResult (&m_recordings[s_groupId]);
			m_recordings[s_groupId].clear();

			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_START_RECORDING:

		// 録画開始
		// ここはm_recProgressで判断いれとく
////		if (m_recProgress == EN_REC_PROGRESS__INIT) {
		if (msp_rec_instances[s_groupId]->getCurrentProgress() == CRecInstance::progress::INIT) {

			_UTL_LOG_I ("start recording (on tune thread)");

			memset (m_recording_tmpfiles[s_groupId], 0x00, PATH_MAX);
			std::string *p_path = mp_settings->getParams()->getRecTsPath();
			snprintf (
				m_recording_tmpfiles[s_groupId],
				PATH_MAX,
				"%s/tmp.m2ts.%lu.%d",
				p_path->c_str(),
				pthread_self(),
				s_groupId
			);


			// ######################################### //
////			m_recProgress = EN_REC_PROGRESS__PRE_PROCESS;
			std::string s = m_recording_tmpfiles[s_groupId];
			msp_rec_instances[s_groupId]->setRecFilename(s);
			msp_rec_instances[s_groupId]->setNextProgress(CRecInstance::progress::PRE_PROCESS);
			// ######################################### //


			m_recordings[s_groupId].state |= RESERVE_STATE__NOW_RECORDING;
			m_recordings[s_groupId].recording_start_time.setCurrentTime();

			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
////			_UTL_LOG_E ("m_recProgress != EN_REC_PROGRESS__INIT ???  -> not start recording");
			_UTL_LOG_E ("progress != INIT ???  -> not start recording");

			m_recordings[s_groupId].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			setResult (&m_recordings[s_groupId]);
			m_recordings[s_groupId].clear();

			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_END_SUCCESS:

//		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));
		s_presentEventInfo.clear();
		s_retry_get_event_info = 0;
		s_groupId = 0xff;
//		s_ch = 0;

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		//-----------------------------//
		{
			uint32_t opt = getRequestOption ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);

			// 選局を停止しときます tune stop
			// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
			CTunerServiceIf _if (getExternalIf());
			_if.reqTuneStop (s_groupId);
			_if.reqClose (s_groupId);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);
		}
		//-----------------------------//

//		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));
		s_presentEventInfo.clear();
		s_retry_get_event_info = 0;
		s_groupId = 0xff;
//		s_ch = 0;

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_addReserve_currentEvent (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_TUNER_STATE,
		SECTID_WAIT_GET_TUNER_STATE,
		SECTID_REQ_GET_PAT_DETECT_STATE,
		SECTID_WAIT_GET_PAT_DETECT_STATE,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_SERVICE_INFO s_serviceInfos[10];
	static PSISI_EVENT_INFO s_presentEventInfo;
	static uint8_t s_groupId = 0;


	switch (sectId) {
	case SECTID_ENTRY:
		s_groupId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
		if (s_groupId >= m_tuner_resource_max) {
			_UTL_LOG_E ("invalid groupId:[0x%02x]", s_groupId);
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_REQ_GET_TUNER_STATE;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_GET_TUNER_STATE: {
		CTunerControlIf _if (getExternalIf(), s_groupId);
		_if.reqGetState ();

		sectId = SECTID_WAIT_GET_TUNER_STATE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_TUNER_STATE: {

		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			EN_TUNER_STATE _state = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
			if (_state == EN_TUNER_STATE__TUNING_SUCCESS) {
				sectId = SECTID_REQ_GET_PAT_DETECT_STATE;
				enAct = EN_THM_ACT_CONTINUE;
			} else {
				_UTL_LOG_E ("not EN_TUNER_STATE__TUNING_SUCCESS %d", _state);
#ifdef _DUMMY_TUNER
				sectId = SECTID_REQ_GET_PAT_DETECT_STATE;
#else
				sectId = SECTID_END_ERROR;
#endif
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			// success only
		}

		}
		break;

	case SECTID_REQ_GET_PAT_DETECT_STATE: {
		CPsisiManagerIf _if (getExternalIf(), s_groupId);
		_if.reqGetPatDetectState ();

		sectId = SECTID_WAIT_GET_PAT_DETECT_STATE;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_PAT_DETECT_STATE: {
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			EN_PAT_DETECT_STATE _state = *(EN_PAT_DETECT_STATE*)(pIf->getSrcInfo()->msg.pMsg);
			if (_state == EN_PAT_DETECT_STATE__DETECTED) {
				sectId = SECTID_REQ_GET_SERVICE_INFOS;
				enAct = EN_THM_ACT_CONTINUE;
			} else {
				_UTL_LOG_E ("not EN_PAT_DETECT_STATE__DETECTED %d", _state);
#ifdef _DUMMY_TUNER
				sectId = SECTID_REQ_GET_SERVICE_INFOS;
#else
				sectId = SECTID_END_ERROR;
#endif
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			// success only
		}

		}
		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		CPsisiManagerIf _if (getExternalIf(), s_groupId);
		_if.reqGetCurrentServiceInfos (s_serviceInfos, 10);

		sectId = SECTID_WAIT_GET_SERVICE_INFOS;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			int num = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			if (num > 0) {
s_serviceInfos[0].dump();
				sectId = SECTID_REQ_GET_PRESENT_EVENT_INFO;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				_UTL_LOG_E ("reqGetCurrentServiceInfos is 0");
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("reqGetCurrentServiceInfos err");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {
		CPsisiManagerIf _if (getExternalIf(), s_groupId);
//TODO s_serviceInfos[0] 暫定0番目を使います 大概service_type=0x01でHD画質だろうか
		_if.reqGetPresentEventInfo (&s_serviceInfos[0], &s_presentEventInfo);

		sectId = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		enAct = EN_THM_ACT_WAIT;

		} break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
s_presentEventInfo.dump();

			char *p_svc_name = NULL;
			CChannelManagerIf::SERVICE_ID_PARAM_t param = {
				s_presentEventInfo.transport_stream_id,
				s_presentEventInfo.original_network_id,
				s_presentEventInfo.service_id
			};
			CChannelManagerIf _if (getExternalIf());
			_if.syncGetServiceName (&param); // sync wait
			enRslt = getIf()->getSrcInfo()->enRslt;
			if (enRslt == EN_THM_RSLT_SUCCESS) {
				p_svc_name = (char*)(getIf()->getSrcInfo()->msg.pMsg);
			}

			bool r = addReserve (
							s_presentEventInfo.transport_stream_id,
							s_presentEventInfo.original_network_id,
							s_presentEventInfo.service_id,
							s_presentEventInfo.event_id,
							&s_presentEventInfo.start_time,
							&s_presentEventInfo.end_time,
							s_presentEventInfo.event_name_char,
							p_svc_name,
							true, // is_event_type true
							EN_RESERVE_REPEATABILITY__NONE
						);
			if (r) {
				sectId = SECTID_END_SUCCESS;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("(%s) reqGetPresentEventInfo err", pIf->getSeqName());
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_END_SUCCESS:

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
//		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));
		s_presentEventInfo.clear();

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
//		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));
		s_presentEventInfo.clear();

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_addReserve_event (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_EVENT,
		SECTID_WAIT_GET_EVENT,
		SECTID_ADD_RESERVE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static CRecManagerIf::ADD_RESERVE_PARAM_t s_param;
	static CEventScheduleManagerIf::EVENT_t s_event;
	static bool s_is_rescheduled = false;


	switch (sectId) {
	case SECTID_ENTRY: {

		s_param = *(CRecManagerIf::ADD_RESERVE_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

		// repeatablityのチェック入れときます
		if (
			s_param.repeatablity == EN_RESERVE_REPEATABILITY__NONE ||
			s_param.repeatablity == EN_RESERVE_REPEATABILITY__AUTO
		) {
			sectId = SECTID_REQ_GET_EVENT;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			_UTL_LOG_E ("invalid repeatablity");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		// rescheduleかのチェックします
		uint8_t src_thread_idx = pIf->getSrcInfo()->nThreadIdx;
		uint8_t src_seq_idx = pIf->getSrcInfo()->nSeqIdx;
		if ((src_thread_idx == EN_MODULE_REC_MANAGER) && (src_seq_idx == EN_SEQ_REC_MANAGER__START_RECORDING)) {
			_UTL_LOG_I ("(%s) requst from EN_SEQ_REC_MANAGER__START_RECORDING", pIf->getSeqName());
			s_is_rescheduled = true;
		}

		}
		break;

	case SECTID_REQ_GET_EVENT: {

		CEventScheduleManagerIf::REQ_EVENT_PARAM_t param = {
			{
				s_param.transport_stream_id,
				s_param.original_network_id,
				s_param.service_id,
				s_param.event_id
			},
			&s_event
		};

		CEventScheduleManagerIf _if (getExternalIf());
		_if.reqGetEvent (&param);

		sectId = SECTID_WAIT_GET_EVENT;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_EVENT:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_ADD_RESERVE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			// 予約に対応するイベントがなかった あらら...
			_UTL_LOG_E ("onReq_addReserve_event - reqGetEvent error");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_ADD_RESERVE: {

		char *p_svc_name = NULL;
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			s_param.transport_stream_id,
			s_param.original_network_id,
			s_param.service_id
		};
		CChannelManagerIf _if (getExternalIf());
		_if.syncGetServiceName (&param); // sync wait
		enRslt = getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			p_svc_name = (char*)(getIf()->getSrcInfo()->msg.pMsg);
		}


		bool r = addReserve (
						s_event.transport_stream_id,
						s_event.original_network_id,
						s_event.service_id,
						s_event.event_id,
						&s_event.start_time,
						&s_event.end_time,
						s_event.p_event_name->c_str(),
						p_svc_name,
						true, // is_event_type true
						s_param.repeatablity,
						s_is_rescheduled
					);
		if (r) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_END_SUCCESS:

//		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_param.clear();
		s_event.clear();
		s_is_rescheduled = false;

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

//		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_param.clear();
		s_event.clear();
		s_is_rescheduled = false;

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}


	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_addReserve_eventHelper (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_EVENT,
		SECTID_WAIT_GET_EVENT,
		SECTID_ADD_RESERVE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static CRecManagerIf::ADD_RESERVE_HELPER_PARAM_t s_param;
	static CEventScheduleManagerIf::EVENT_t s_event;


	switch (sectId) {
	case SECTID_ENTRY:

		s_param = *(CRecManagerIf::ADD_RESERVE_HELPER_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);


		// repeatablityのチェック入れときます
		if (
			s_param.repeatablity == EN_RESERVE_REPEATABILITY__NONE ||
			s_param.repeatablity == EN_RESERVE_REPEATABILITY__AUTO
		) {
			sectId = SECTID_REQ_GET_EVENT;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_GET_EVENT: {

		CEventScheduleManagerIf::REQ_EVENT_PARAM_t param ;
		param.arg.index = s_param.index;
		param.p_out_event = &s_event;

		CEventScheduleManagerIf _if (getExternalIf());
		_if.reqGetEvent_latestDumpedSchedule (&param);

		sectId = SECTID_WAIT_GET_EVENT;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_EVENT:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_ADD_RESERVE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			// 予約に対応するイベントがなかった あらら...
			_UTL_LOG_E ("reqGetEvent error");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_ADD_RESERVE: {

		char *p_svc_name = NULL;
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			s_event.transport_stream_id,
			s_event.original_network_id,
			s_event.service_id
		};
		CChannelManagerIf _if (getExternalIf());
		_if.syncGetServiceName (&param); // sync wait
		enRslt = getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			p_svc_name = (char*)(getIf()->getSrcInfo()->msg.pMsg);
		}


		bool r = addReserve (
						s_event.transport_stream_id,
						s_event.original_network_id,
						s_event.service_id,
						s_event.event_id,
						&s_event.start_time,
						&s_event.end_time,
						s_event.p_event_name->c_str(),
						p_svc_name,
						true, // is_event_type true
						s_param.repeatablity
					);
		if (r) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_END_SUCCESS:

		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_event.clear();

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		memset (&s_param, 0x00, sizeof(s_param));
//		memset (&s_event, 0x00, sizeof(s_event));
		s_event.clear();

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}


	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_addReserve_manual (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_ADD_RESERVE,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	static CRecManagerIf::ADD_RESERVE_PARAM_t s_param;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:

		s_param = *(CRecManagerIf::ADD_RESERVE_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);


		// repeatablityのチェック入れときます
		if (
			s_param.repeatablity >= EN_RESERVE_REPEATABILITY__NONE &&
			s_param.repeatablity <= EN_RESERVE_REPEATABILITY__WEEKLY
		) {
			sectId = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {
		// サービスの存在チェックします

		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			s_param.transport_stream_id,
			s_param.original_network_id,
			s_param.service_id
		};

		CChannelManagerIf _if (getExternalIf());
		_if.reqGetPysicalChannelByServiceId (&param);

		sectId = SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_ADD_RESERVE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqGetPysicalChannelByServiceId is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_ADD_RESERVE: {

		char *p_svc_name = NULL;
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			s_param.transport_stream_id,
			s_param.original_network_id,
			s_param.service_id
		};
		CChannelManagerIf _if (getExternalIf());
		_if.syncGetServiceName (&param); // sync wait
		enRslt = getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			p_svc_name = (char*)(getIf()->getSrcInfo()->msg.pMsg);
		}


		bool r = addReserve (
						s_param.transport_stream_id,
						s_param.original_network_id,
						s_param.service_id,
						0x00, // event_idはこの時点では不明
						&s_param.start_time,
						&s_param.end_time,
						NULL, // タイトル名も不明 (そもそもs_paramにない)
						p_svc_name,
						false,
						s_param.repeatablity
					);
		if (r) {
			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;
		} else {
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		}
		break;

	case SECTID_END_SUCCESS:

//		memset (&s_param, 0x00, sizeof(s_param));
		s_param.clear();

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

//		memset (&s_param, 0x00, sizeof(s_param));
		s_param.clear();

		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}


	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_removeReserve (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CRecManagerIf::REMOVE_RESERVE_PARAM_t param =
			*(CRecManagerIf::REMOVE_RESERVE_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	int idx = getReserveIndex (
					param.arg.key.transport_stream_id,
					param.arg.key.original_network_id,
					param.arg.key.service_id,
					param.arg.key.event_id
				);

	if (idx < 0) {
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {
		if (removeReserve (idx, param.isConsiderRepeatability, param.isApplyResult)) {
			pIf->reply (EN_THM_RSLT_SUCCESS);
		} else {
			pIf->reply (EN_THM_RSLT_ERROR);
		}
	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_removeReserve_byIndex (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CRecManagerIf::REMOVE_RESERVE_PARAM_t param =
			*(CRecManagerIf::REMOVE_RESERVE_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (removeReserve (param.arg.index, param.isConsiderRepeatability, param.isApplyResult)) {
		pIf->reply (EN_THM_RSLT_SUCCESS);
	} else {
		pIf->reply (EN_THM_RSLT_ERROR);
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_getReserves (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CRecManagerIf::GET_RESERVES_PARAM_t _param =
			*(CRecManagerIf::GET_RESERVES_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (!_param.p_out_reserves) {
		_UTL_LOG_E ("_param.p_out_reserves is null.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else if (_param.array_max_num <= 0) {
		_UTL_LOG_E ("_param.array_max_num is invalid.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {

		int n = getReserves (_param.p_out_reserves, _param.array_max_num);
		// 取得数をreply msgで返します
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&n, sizeof(n));
	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_stopRecording (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	int groupId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
	if (groupId >= m_tuner_resource_max) {
		_UTL_LOG_E ("invalid groupId:[0x%02x]", groupId);
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {
		if (m_recordings[groupId].state & RESERVE_STATE__NOW_RECORDING) {

			// stopRecording が呼ばれたらエラー終了にしておきます
////		_UTL_LOG_W ("m_recProgress = EN_REC_PROGRESS__END_ERROR");
			_UTL_LOG_W ("progress = END_ERROR");


			// ######################################### //
////		m_recProgress = EN_REC_PROGRESS__END_ERROR;
			msp_rec_instances[groupId]->setNextProgress(CRecInstance::progress::END_ERROR);
			// ######################################### //


			m_recordings[groupId].state |= RESERVE_STATE__FORCE_STOPPED;

			pIf->reply (EN_THM_RSLT_SUCCESS);

		} else {
			_UTL_LOG_E ("invalid rec state (not recording now...)");
			pIf->reply (EN_THM_RSLT_ERROR);
		}
	}

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReq_dumpReserves (CThreadMgrIf *pIf)
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
		dumpReserves();
		break;

	case 1:
		dumpResults();
		break;

	case 2:
		dumpRecording();
		break;

	default:
		break;
	}


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CRecManager::onReceiveNotify (CThreadMgrIf *pIf)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		if (pIf->getSrcInfo()->nClientId == m_tunerNotify_clientId[_gr]) {

			EN_TUNER_STATE enState = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
			switch (enState) {
			case EN_TUNER_STATE__TUNING_BEGIN:
				_UTL_LOG_I ("EN_TUNER_STATE__TUNING_BEGIN groupId:[%d]", _gr);
				break;

			case EN_TUNER_STATE__TUNING_SUCCESS:
				_UTL_LOG_I ("EN_TUNER_STATE__TUNING_SUCCESS groupId:[%d]", _gr);
				break;

			case EN_TUNER_STATE__TUNING_ERROR_STOP:
				_UTL_LOG_I ("EN_TUNER_STATE__TUNING_ERROR_STOP groupId:[%d]", _gr);
				break;

			case EN_TUNER_STATE__TUNE_STOP:
				_UTL_LOG_I ("EN_TUNER_STATE__TUNE_STOP groupId:[%d]", _gr);
				break;

			default:
				break;
			}


		} else if (pIf->getSrcInfo()->nClientId == m_patDetectNotify_clientId[_gr]) {

			EN_PAT_DETECT_STATE _state = *(EN_PAT_DETECT_STATE*)(pIf->getSrcInfo()->msg.pMsg);
			if (_state == EN_PAT_DETECT_STATE__DETECTED) {
				_UTL_LOG_I ("EN_PAT_DETECT_STATE__DETECTED");

			} else if (_state == EN_PAT_DETECT_STATE__NOT_DETECTED) {
				_UTL_LOG_E ("EN_PAT_DETECT_STATE__NOT_DETECTED");

				// PAT途絶したら録画中止します
				if (m_recordings[_gr].state & RESERVE_STATE__NOW_RECORDING) {

					// ######################################### //
////				m_recProgress = EN_REC_PROGRESS__POST_PROCESS;
					msp_rec_instances[_gr]->setNextProgress(CRecInstance::progress::POST_PROCESS);
					// ######################################### //


					m_recordings[_gr].state |= RESERVE_STATE__END_ERROR__INTERNAL_ERR;


					uint32_t opt = getRequestOption ();
					opt |= REQUEST_OPTION__WITHOUT_REPLY;
					setRequestOption (opt);

					// 自ら呼び出します
					// 内部で自リクエストするので
					// REQUEST_OPTION__WITHOUT_REPLY を入れときます
					//
					// PAT途絶してTsReceiveHandlerは動いていない前提
////					this->onTsReceived (NULL, 0);
					msp_rec_instances[_gr]->onTsReceived (NULL, 0);

					opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
					setRequestOption (opt);

				}
			}

		} else if (pIf->getSrcInfo()->nClientId == m_eventChangeNotify_clientId[_gr]) {

			PSISI_NOTIFY_EVENT_INFO _info = *(PSISI_NOTIFY_EVENT_INFO*)(pIf->getSrcInfo()->msg.pMsg);
			_UTL_LOG_I ("!!! event changed !!! groupId:[%d]", _gr);
			_info.dump ();

		}
	}
}

bool CRecManager::addReserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id,
	CEtime* p_start_time,
	CEtime* p_end_time,
	const char *psz_title_name,
	const char *psz_service_name,
	bool _is_event_type,
	EN_RESERVE_REPEATABILITY repeatabilitiy,
	bool _is_rescheduled
)
{
	if (!p_start_time || !p_end_time) {
		return false;
	}

	if (*p_start_time >= *p_end_time) {
		_UTL_LOG_E ("invalid reserve time");
		return false;
	}

	if (!isExistEmptyReserve ()) {
		_UTL_LOG_E ("reserve full.");
		return false;
	}


	CRecReserve tmp;
	tmp.set (
		_transport_stream_id,
		_original_network_id,
		_service_id,
		_event_id,
		p_start_time,
		p_end_time,
		psz_title_name,
		psz_service_name,
		_is_event_type,
		repeatabilitiy
	);


	if (isDuplicateReserve (&tmp)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	if (isOverrapTimeReserve (&tmp)) {
		_UTL_LOG_W ("reserve time is overrap.");
// 時間被ってるけど予約入れます
//		return false;
	}

	if (!_is_rescheduled) {
		for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
			if (m_recordings[_gr].is_used) {
				if (m_recordings[_gr] == tmp) {
					_UTL_LOG_E ("reserve is now recording.");
					return false;
				}
			}
		}
	}


	CRecReserve* p_reserve = searchAscendingOrderReserve (p_start_time);
	if (!p_reserve) {
		_UTL_LOG_E ("searchAscendingOrderReserve failure.");
		return false;
	}

	p_reserve->set (
		_transport_stream_id,
		_original_network_id,
		_service_id,
		_event_id,
		p_start_time,
		p_end_time,
		psz_title_name,
		psz_service_name,
		_is_event_type,
		repeatabilitiy
	);

	if (_is_rescheduled) {
		p_reserve->state |= RESERVE_STATE__RESCHEDULED;
	}

	_UTL_LOG_I ("###########################");
	_UTL_LOG_I ("####    add reserve    ####");
	_UTL_LOG_I ("###########################");
	p_reserve->dump();


	// 毎回書き込み
	saveReserves ();


	return true;
}

/**
 * 引数をキーと同一予約のindexを返します
 * 見つからない場合は -1 で返します
 */
int CRecManager::getReserveIndex (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id
)
{
	CRecReserve tmp;
	tmp.set (
		_transport_stream_id,
		_original_network_id,
		_service_id,
		_event_id,
		NULL,
		NULL,
		NULL,
		NULL,
		false,
		EN_RESERVE_REPEATABILITY__NONE
	);

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves [i] == tmp) {
			return i;
		}
	}

	return -1;
}

/**
 * indexで指定した予約を削除します
 * 0始まり
 * isConsiderRepeatability == false で Repeatability関係なく削除します
 */
bool CRecManager::removeReserve (int index, bool isConsiderRepeatability, bool isApplyResult)
{
	if (index >= RESERVE_NUM_MAX) {
		return false;
	}

	m_reserves [index].state |= RESERVE_STATE__REMOVED;
	if (isApplyResult) {
		setResult (&m_reserves[index]);
	}

	_UTL_LOG_I ("##############################");
	_UTL_LOG_I ("####    remove reserve    ####");
	_UTL_LOG_I ("##############################");
	m_reserves [index].dump();

	if (isConsiderRepeatability) {
		checkRepeatability (&m_reserves[index]);
	}

	// 間詰め
	for (int i = index; i < RESERVE_NUM_MAX -1; ++ i) {
		m_reserves [i] = m_reserves [i+1];
	}
	m_reserves [RESERVE_NUM_MAX -1].clear();


	// 毎回書き込み
	saveReserves ();


	return true;
}

/**
 * 開始時間を基準に降順で空きをさがします
 * 空きがある前提
 */
CRecReserve* CRecManager::searchAscendingOrderReserve (CEtime *p_start_time_ref)
{
	if (!p_start_time_ref) {
		return NULL;
	}


	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {

		// 先頭から詰まっているはず
//		if (!m_reserves [i].is_used) {
//			continue;
//		}

		// 基準時間より後ろの時間を探します
		if (m_reserves [i].start_time > *p_start_time_ref) {

			// 後ろから見てずらします
			for (int j = RESERVE_NUM_MAX -1; j > i; -- j) {
				m_reserves [j] = m_reserves [j-1] ;
			}

			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		// 見つからなかったので最後尾にします
		return findEmptyReserve ();

	} else {

		m_reserves [i].clear ();
		return &m_reserves [i];
	}
}

bool CRecManager::isExistEmptyReserve (void) const
{
	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		_UTL_LOG_W ("m_reserves full.");
		return false;
	}

	return true;
}

CRecReserve* CRecManager::findEmptyReserve (void)
{
	int i = 0;
	for (i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			break;
		}
	}

	if (i == RESERVE_NUM_MAX) {
		_UTL_LOG_W ("m_reserves full.");
		return NULL;
	}

	return &m_reserves [i];
}

bool CRecManager::isDuplicateReserve (const CRecReserve* p_reserve) const
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves [i] == *p_reserve) {
			// duplicate
			return true;
		}
	}

	return false;
}

bool CRecManager::isOverrapTimeReserve (const CRecReserve* p_reserve) const
{
	if (!p_reserve) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves [i].start_time >= p_reserve->start_time && m_reserves [i].start_time >= p_reserve->end_time) {
			continue;
		}

		if (m_reserves [i].end_time <= p_reserve->start_time && m_reserves [i].end_time <= p_reserve->end_time) {
			continue;
		}

		// overrap
		return true;
	}

	return false;
}

void CRecManager::checkReserves (void)
{
	CEtime current_time;
	current_time.setCurrentTime();

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {

		if (!m_reserves [i].is_used) {
			continue;
		}

		// 開始時間/終了時間とも過ぎていたら resultsに入れときます
		if (m_reserves [i].start_time < current_time && m_reserves [i].end_time <= current_time) {
			m_reserves [i].state |= RESERVE_STATE__END_ERROR__ALREADY_PASSED;
			setResult (&m_reserves[i]);
			checkRepeatability (&m_reserves[i]);
			continue;
		}

		// 開始時間をみて 時間が来たら 録画要求を立てます
		// start_time - end_time 範囲内
		if (m_reserves [i].start_time <= current_time && m_reserves [i].end_time > current_time) {

			// 開始時間から10秒以上経過していたら フラグたてときます
			CEtime tmp = m_reserves [i].start_time;
			tmp.addSec(10);
			if (tmp <= current_time) {
				m_reserves [i].state |= RESERVE_STATE__START_TIME_PASSED;
			}

//			if (!m_recordings[0].is_used) {
//				// 録画中でなければ 録画開始フラグ立てます
// 録画してても関係なく録画開始フラグ立てます
				m_reserves [i].state |= RESERVE_STATE__REQ_START_RECORDING;

				// 見つかったのでbreakします。
				// 同時刻で予約が入っていた場合 配列の並び順で先の方を録画開始します
				// 先行の録画が終わらないと後続は始まらないです
				break;
//			}
		}
	}

	refreshReserves (RESERVE_STATE__END_ERROR__ALREADY_PASSED);
}

/**
 * 引数のstateが立っている予約を除去します
 */
void CRecManager::refreshReserves (uint32_t state)
{
	// 逆から見ます
	for (int i = RESERVE_NUM_MAX -1; i >= 0; -- i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (!(m_reserves[i].state & state)) {
			continue;
		}

		// 間詰め
		for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
			m_reserves [j] = m_reserves [j+1];
		}
		m_reserves [RESERVE_NUM_MAX -1].clear();

		saveReserves ();
	}
}

bool CRecManager::isExistReqStartRecordingReserve (void)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves[i].state & RESERVE_STATE__REQ_START_RECORDING) {
			return true;
		}
	}

	return false;
}

bool CRecManager::pickReqStartRecordingReserve (uint8_t groupId)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves[i].state & RESERVE_STATE__REQ_START_RECORDING) {

			// 次に録画する予約を取り出します
			m_recordings[groupId] = m_reserves[i];

			// フラグは落としておきます
			m_recordings[groupId].state &= ~RESERVE_STATE__REQ_START_RECORDING;

			m_recordings[groupId].group_id = groupId;


			// 間詰め
			for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
				m_reserves [j] = m_reserves [j+1];
			}
			m_reserves [RESERVE_NUM_MAX -1].clear();

			checkRepeatability (&m_recordings[groupId]);

			saveReserves ();

			return true;
		}
	}

	return false;
}

/**
 * 録画結果をリストに保存します
 */
void CRecManager::setResult (CRecReserve *p)
{
	if (!p) {
		return ;
	}


	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (!m_results [i].is_used) {
			m_results [i] = *p;

			// 毎回書き込み
			saveResults ();

			return ;
		}
	}

	// m_results full
	// 最古のものを消します
	for (int i = 0; i < RESULT_NUM_MAX -1; ++ i) {
		m_results [i] = m_results [i + 1] ;		
	}

	m_results [RESULT_NUM_MAX -1].clear ();
	m_results [RESULT_NUM_MAX -1] = *p;


	// 毎回書き込み
	saveResults ();
}

void CRecManager::checkRecordingEnd (void)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		if (!m_recordings[_gr].is_used) {
			continue;
		}

		if (!(m_recordings[_gr].state & RESERVE_STATE__NOW_RECORDING)) {
			continue;
		}

		CEtime current_time ;
		current_time.setCurrentTime();

		if (m_recordings[_gr].end_time <= current_time) {

			// 録画 正常終了します
			if (msp_rec_instances[_gr]->getCurrentProgress() == CRecInstance::progress::NOW_RECORDING) {
				_UTL_LOG_I ("progress = END_SUCCESS");


				// ######################################### //
				msp_rec_instances[_gr]->setNextProgress(CRecInstance::progress::END_SUCCESS);
				// ######################################### //


			}
		}
	}
}

void CRecManager::checkDiskFree (void)
{
	for (int _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		if (!m_recordings[_gr].is_used) {
			continue;
		}

		if (m_recordings[_gr].state & RESERVE_STATE__NOW_RECORDING) {

			std::string *p_path = mp_settings->getParams()->getRecTsPath();
			int limit = mp_settings->getParams()->getRecDiskSpaceLowLimitMB();
			int df = CUtils::getDiskFreeMB(p_path->c_str());
			if (df < limit) {
				_UTL_LOG_E ("(checkRecordingDiskFree) disk free space [%d] Mbytes !!", df);
				// HDD残量たりない場合は 録画を止めます
				m_recordings[_gr].state |= RESERVE_STATE__END_ERROR__HDD_FREE_SPACE_LOW;

				uint8_t groupId = _gr;

				uint32_t opt = getRequestOption ();
				opt |= REQUEST_OPTION__WITHOUT_REPLY;
				setRequestOption (opt);
				requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__STOP_RECORDING, (uint8_t*)&groupId, sizeof(uint8_t));
				opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
				setRequestOption (opt);
			}
		}
	}
}

//TODO event_typeの場合はどうするか
/**
 * Repeatabilityの確認して
 * 予約入れるべきものは予約します
 */
void CRecManager::checkRepeatability (const CRecReserve *p_reserve)
{
	if (!p_reserve) {
		return ;
	}

	CEtime s;
	CEtime e;
	s = p_reserve->start_time;
	e = p_reserve->end_time;

	switch (p_reserve->repeatability) {
	case EN_RESERVE_REPEATABILITY__NONE:
		return;
		break;

	case EN_RESERVE_REPEATABILITY__DAILY:
		s.addDay(1);
		e.addDay(1);
		break;

	case EN_RESERVE_REPEATABILITY__WEEKLY:
		s.addWeek(1);
		e.addWeek(1);
		break;

	default:
		_UTL_LOG_W ("invalid repeatability");
		return ;
		break;
	}

	bool r = addReserve (
				p_reserve->transport_stream_id,
				p_reserve->original_network_id,
				p_reserve->service_id,
				p_reserve->event_id,
				&s,
				&e,
				p_reserve->title_name.c_str(),
				p_reserve->service_name.c_str(),
				p_reserve->is_event_type,
				p_reserve->repeatability
			);

	if (r) {
		_UTL_LOG_I ("addReserve by repeatability -- success.");
	} else {
		_UTL_LOG_W ("addReserve by repeatability -- failure.");
	}
}

int CRecManager::getReserves (CRecManagerIf::RESERVE_t *p_out_reserves, int out_array_num) const
{
	if (!p_out_reserves || out_array_num <= 0) {
		return -1;
	}

	int n = 0;

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (out_array_num == 0) {
			break;
		}

		p_out_reserves->transport_stream_id = m_reserves [i].transport_stream_id;
		p_out_reserves->original_network_id = m_reserves [i].original_network_id;
		p_out_reserves->service_id = m_reserves [i].service_id;

		p_out_reserves->event_id = m_reserves [i].event_id;
		p_out_reserves->start_time = m_reserves [i].start_time;
		p_out_reserves->end_time = m_reserves [i].end_time;

		// アドレスで渡します
		p_out_reserves->p_title_name = const_cast<std::string*> (&m_reserves [i].title_name);
		p_out_reserves->p_service_name = const_cast<std::string*> (&m_reserves [i].service_name);

		p_out_reserves->is_event_type = m_reserves [i].is_event_type;
		p_out_reserves->repeatability = m_reserves [i].repeatability;

		++ p_out_reserves;
		-- out_array_num;
		++ n;
	}

	return n;
}

void CRecManager::dumpReserves (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (m_reserves [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			_UTL_LOG_I ("-- index=[%d] --", i);
			m_reserves [i].dump();
		}
	}
}

void CRecManager::dumpResults (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (m_results [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_results [i].dump();
		}
	}
}

void CRecManager::dumpRecording (void) const
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	int i = 0;
	for (i = 0; i < CGroup::GROUP_MAX; ++ i) {
		if (m_recordings[i].is_used) {
			_UTL_LOG_I ("-----------   now recording   -----------");
			m_recordings[i].dump();
		}
	}
}

void CRecManager::clearReserves (void)
{
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		m_reserves [i].clear();
	}
}

void CRecManager::clearResults (void)
{
	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		m_results [i].clear();
	}
}


#if 0
//////////  CTunerControlIf::ITsReceiveHandler  //////////

bool CRecManager::onPreTsReceive (void)
{
	getExternalIf()->createExternalCp();

	uint32_t opt = getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	return true;
}

void CRecManager::onPostTsReceive (void)
{
	uint32_t opt = getExternalIf()->getRequestOption ();
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	getExternalIf()->setRequestOption (opt);

	getExternalIf()->destroyExternalCp();
}

bool CRecManager::onCheckTsReceiveLoop (void)
{
	return true;
}

bool CRecManager::onTsReceived (void *p_ts_data, int length)
{
	switch (m_recProgress) {
	case EN_REC_PROGRESS__PRE_PROCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__PRE_PROCESS");

		std::unique_ptr<CRecAribB25> b25 (new CRecAribB25(8192, m_recording_tmpfile));
		msp_b25.swap (b25);

		RECORDING_NOTICE_t _notice = {m_recProgress, RESERVE_STATE__INIT};
		requestAsync (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_recProgress = EN_REC_PROGRESS__NOW_RECORDING;
		_UTL_LOG_I ("next  EN_REC_PROGRESS__NOW_RECORDING");

		}
		break;

	case EN_REC_PROGRESS__NOW_RECORDING: {

		// recording
		int r = msp_b25->put ((uint8_t*)p_ts_data, length);
		if (r < 0) {
			_UTL_LOG_W ("TS write failed");
		}

		}
		break;

	case EN_REC_PROGRESS__END_SUCCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_SUCCESS");

		RECORDING_NOTICE_t _notice = {m_recProgress, RESERVE_STATE__INIT};
		requestAsync (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_recProgress = EN_REC_PROGRESS__POST_PROCESS;

		}
		break;

	case EN_REC_PROGRESS__END_ERROR: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_ERROR");

		RECORDING_NOTICE_t _notice = {m_recProgress, RESERVE_STATE__INIT};
		requestAsync (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_recProgress = EN_REC_PROGRESS__POST_PROCESS;

		}
		break;

	case EN_REC_PROGRESS__POST_PROCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__POST_PROCESS");

		msp_b25->flush();
		msp_b25->release();

		RECORDING_NOTICE_t _notice = {m_recProgress, RESERVE_STATE__INIT};
		requestAsync (
			EN_MODULE_REC_MANAGER,
			EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
			(uint8_t*)&_notice,
			sizeof(_notice)
		);

		// next
		m_recProgress = EN_REC_PROGRESS__INIT;

		}
		break;

	default:
		break;
	}

	return true;
}
#endif

//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (
		cereal::make_nvp("tv_sec", t.tv_sec),
		cereal::make_nvp("tv_nsec", t.tv_nsec)
	);
}

template <class Archive>
void serialize (Archive &archive, CEtime &t)
{
	archive (cereal::make_nvp("m_time", t.m_time));
}

template <class Archive>
void serialize (Archive &archive, CRecReserve &r)
{
	archive (
		cereal::make_nvp("transport_stream_id", r.transport_stream_id),
		cereal::make_nvp("original_network_id", r.original_network_id),
		cereal::make_nvp("service_id", r.service_id),
		cereal::make_nvp("event_id", r.event_id),
		cereal::make_nvp("start_time", r.start_time),
		cereal::make_nvp("end_time", r.end_time),
		cereal::make_nvp("title_name", r.title_name),
		cereal::make_nvp("service_name", r.service_name),
		cereal::make_nvp("is_event_type", r.is_event_type),
		cereal::make_nvp("repeatability", r.repeatability),
		cereal::make_nvp("state", r.state),
		cereal::make_nvp("recording_start_time", r.recording_start_time),
		cereal::make_nvp("recording_end_time", r.recording_end_time),
		cereal::make_nvp("group_id", r.group_id),
		cereal::make_nvp("is_used", r.is_used)
	);
}

void CRecManager::saveReserves (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_reserves), sizeof(CRecReserve) * RESERVE_NUM_MAX);
	}

	std::string *p_path = mp_settings->getParams()->getRecReservesJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CRecManager::loadReserves (void)
{
	std::string *p_path = mp_settings->getParams()->getRecReservesJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("rec_reserves.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_reserves), sizeof(CRecReserve) * RESERVE_NUM_MAX);

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tvnsecに書いてるので toString用の文字はここで作ります
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		m_reserves [i].start_time.updateStrings();
		m_reserves [i].end_time.updateStrings();
		m_reserves [i].recording_start_time.updateStrings();
		m_reserves [i].recording_end_time.updateStrings();
	}
}

void CRecManager::saveResults (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_results), sizeof(CRecReserve) * RESULT_NUM_MAX);
	}

	std::string *p_path = mp_settings->getParams()->getRecResultsJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CRecManager::loadResults (void)
{
	std::string *p_path = mp_settings->getParams()->getRecResultsJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I("rec_results.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_results), sizeof(CRecReserve) * RESULT_NUM_MAX);

	ifs.close();
	ss.clear();


	// CEtimeの値は直接 tv_sec,tv_nsecに書いてるので toString用の文字はここで作ります
	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		m_results [i].start_time.updateStrings();
		m_results [i].end_time.updateStrings();
		m_results [i].recording_start_time.updateStrings();
		m_results [i].recording_end_time.updateStrings();
	}
}
