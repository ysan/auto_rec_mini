#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "utils.h"
#include "verstr.h"
#include "message.h"
#ifdef __cplusplus
};
#endif

#include "it9175_extern.h"

#include "RecManager.h"
#include "RecManagerIf.h"

#include "Settings.h"

#include "modules.h"

#include "aribstr.h"


typedef struct {
	EN_REC_PROGRESS rec_progress;
	EN_RESERVE_STATE reserve_state;
} _RECORDING_NOTICE;


CRecManager::CRecManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,m_tunerNotify_clientId (0xff)
	,m_tsReceive_handlerId (-1)
	,m_patDetectNotify_clientId (0xff)
	,m_eventChangeNotify_clientId (0xff)
	,m_recProgress (EN_REC_PROGRESS__INIT)
	,mp_outputBuffer (NULL)
{
	mSeqs [EN_SEQ_REC_MANAGER__MODULE_UP] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_moduleUp,                (char*)"onReq_moduleUp"};
	mSeqs [EN_SEQ_REC_MANAGER__MODULE_DOWN] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_moduleDown,              (char*)"onReq_moduleDown"};
	mSeqs [EN_SEQ_REC_MANAGER__CHECK_LOOP] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_checkLoop,               (char*)"onReq_checkLoop"};
	mSeqs [EN_SEQ_REC_MANAGER__RECORDING_NOTICE] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_recordingNotice,         (char*)"onReq_recordingNotice"};
	mSeqs [EN_SEQ_REC_MANAGER__START_RECORDING] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_startRecording,          (char*)"onReq_startRecording"};
	mSeqs [EN_SEQ_REC_MANAGER__ADD_RESERVE_CURRENT_EVENT] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_addReserve_currentEvent, (char*)"onReq_addReserve_currentEvent"};
	mSeqs [EN_SEQ_REC_MANAGER__ADD_RESERVE_MANUAL] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_addReserve_manual,       (char*)"onReq_addReserve_manual"};
	mSeqs [EN_SEQ_REC_MANAGER__REMOVE_RESERVE] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_removeReserve,           (char*)"onReq_removeReserve"};
	mSeqs [EN_SEQ_REC_MANAGER__STOP_RECORDING] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_stopRecording,           (char*)"onReq_stopRecording"};
	mSeqs [EN_SEQ_REC_MANAGER__DUMP_RESERVES] =
		{(PFN_SEQ_BASE)&CRecManager::onReq_dumpReserves,            (char*)"onReq_dumpReserves"};
	setSeqs (mSeqs, EN_SEQ_REC_MANAGER__NUM);


	clearReserves ();
	clearResults ();
	m_recording.clear();
}

CRecManager::~CRecManager (void)
{
	clearReserves ();
	clearResults ();
	m_recording.clear();
}


void CRecManager::onReq_moduleUp (CThreadMgrIf *pIf)
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

		loadReserves ();
		loadResults ();

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
			m_tunerNotify_clientId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
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
			m_tsReceive_handlerId = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_PAT_DETECT_NOTIFY;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterTsReceiveHandler is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_PAT_DETECT_NOTIFY: {
		CPsisiManagerIf _if (getExternalIf());
		_if.reqRegisterPatDetectNotify ();

		sectId = SECTID_WAIT_REG_PAT_DETECT_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_PAT_DETECT_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_patDetectNotify_clientId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_REG_EVENT_CHANGE_NOTIFY;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterPatDetectNotify is failure.");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_REG_EVENT_CHANGE_NOTIFY: {
		CPsisiManagerIf _if (getExternalIf());
		_if.reqRegisterEventChangeNotify ();

		sectId = SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_REG_EVENT_CHANGE_NOTIFY:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			m_eventChangeNotify_clientId = *(uint8_t*)(pIf->getSrcInfo()->msg.pMsg);
			sectId = SECTID_REQ_CHECK_LOOP;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("reqRegisterEventChangeNotify is failure.");
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
		SECTID_REQ_START_RECORDING,
		SECTID_WAIT_START_RECORDING,
		SECTID_REQ_GET_PRESENT_EVENT_INFO,
		SECTID_WAIT_GET_PRESENT_EVENT_INFO,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static PSISI_EVENT_INFO s_presentEventInfo;


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

	case SECTID_CHECK_WAIT:

		// reserve check
		checkReserves ();
		refreshReserves ();


		if (m_recording.state == EN_RESERVE_STATE__NOW_RECORDING) {

			// recording end check
			checkRecordingEnd ();

			sectId = SECTID_REQ_GET_PRESENT_EVENT_INFO;
			enAct = EN_THM_ACT_CONTINUE;

		} else {

			if (pickReqStartRecordingReserve ()) {
				// request start recording
				requestAsync (EN_MODULE_REC_MANAGER, EN_SEQ_REC_MANAGER__START_RECORDING);

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
			m_recording.state = EN_RESERVE_STATE__END_ERROR__INTERNAL_ERR;
			setResult (&m_recording);
			m_recording.clear();

			sectId = SECTID_CHECK;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_REQ_GET_PRESENT_EVENT_INFO: {

		PSISI_SERVICE_INFO _svc_info ;
		// 以下３つの要素が入っていればOK
		_svc_info.transport_stream_id = m_recording.transport_stream_id;
		_svc_info.original_network_id = m_recording.original_network_id;
		_svc_info.service_id = m_recording.service_id;

		CPsisiManagerIf _if (getExternalIf());
		_if.reqGetPresentEventInfo (&_svc_info, &s_presentEventInfo);

		sectId = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		enAct = EN_THM_ACT_WAIT;

		}
		break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
//s_presentEventInfo.dump();
			if (m_recording.state == EN_RESERVE_STATE__NOW_RECORDING) {
				m_recording.event_id = s_presentEventInfo.event_id;
				m_recording.title_name = s_presentEventInfo.event_name_char;
			}

		} else {
			_UTL_LOG_E ("reqGetPresentEventInfo err");
		}

		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));

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


	_RECORDING_NOTICE _notice = *(_RECORDING_NOTICE*)(pIf->getSrcInfo()->msg.pMsg);
	switch (_notice.rec_progress) {
	case EN_REC_PROGRESS__INIT:
		break;

	case EN_REC_PROGRESS__PRE_PROCESS:
		if (_notice.reserve_state != EN_RESERVE_STATE__INIT) {
			m_recording.state = _notice.reserve_state;
		}
		break;

	case EN_REC_PROGRESS__NOW_RECORDING:
		break;

	case EN_REC_PROGRESS__END_SUCCESS:
		m_recording.state = EN_RESERVE_STATE__END_SUCCESS;
		break;

	case EN_REC_PROGRESS__END_ERROR:
		break;

	case EN_REC_PROGRESS__POST_PROCESS: {
		setResult (&m_recording);


		// rename
		std::string *p_path = CSettings::getInstance()->getParams()->getRecTsPath();

		char tmpfile [PATH_MAX] = {0};
		snprintf (
			tmpfile,
			sizeof(tmpfile),
			"%s/tmp.m2ts",
			p_path->c_str()
		);

		char newfile [PATH_MAX] = {0};
		char *p_name = (char*)"rec";
		if (m_recording.title_name.c_str()) {
			p_name = (char*)m_recording.title_name.c_str();
		}
		snprintf (
			newfile,
			sizeof(newfile),
			"%s/%s_%s-%s.m2ts",
			p_path->c_str(),
			p_name,
			m_recording.start_time.toString2(),
			m_recording.end_time.toString2()
		);

		rename (tmpfile, newfile) ;

		m_recording.clear ();
		_UTL_LOG_I ("recording end...");



		uint32_t opt = getRequestOption ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		setRequestOption (opt);

		// 選局を停止しときます tune stop
		// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
		CTunerControlIf _if (getExternalIf());
		_if.reqTuneStop ();

		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		setRequestOption (opt);

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
		SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_WAIT_GET_PYSICAL_CH_BY_SERVICE_ID,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_START_RECORDING,
		SECTID_END_SUCCESS,
		SECTID_END_ERROR,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	static uint16_t s_ch = 0;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


	switch (sectId) {
	case SECTID_ENTRY:
		sectId = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {

		_SERVICE_ID_PARAM param = {
			m_recording.transport_stream_id,
			m_recording.original_network_id,
			m_recording.service_id
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

	case SECTID_REQ_TUNE: {
		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_ch);
		_UTL_LOG_I ("freq=[%d]kHz\n", freq);

		CTunerControlIf _if (getExternalIf());
		_if.reqTune (freq);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_START_RECORDING;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_W ("tune is failure  -> not start recording");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_START_RECORDING:

		// ここはm_recProgressで判断いれとく
		if (m_recProgress == EN_REC_PROGRESS__INIT) {

			_UTL_LOG_I ("start recording (on tune thread)");
			m_recProgress = EN_REC_PROGRESS__PRE_PROCESS;
			m_recording.state = EN_RESERVE_STATE__NOW_RECORDING;

			sectId = SECTID_END_SUCCESS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("m_recProgress != EN_REC_PROGRESS__INIT ???  -> not start recording");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_END_SUCCESS:

		s_ch = 0;

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		s_ch = 0;

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


	switch (sectId) {
	case SECTID_ENTRY:
		sectId = SECTID_REQ_GET_TUNER_STATE;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_TUNER_STATE: {
		CTunerControlIf _if (getExternalIf());
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
		CPsisiManagerIf _if (getExternalIf());
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
		CPsisiManagerIf _if (getExternalIf());
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
				_UTL_LOG_E ("reqGetCurrentServiceInfos err");
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
		CPsisiManagerIf _if (getExternalIf());
//TODO s_serviceInfos[0] 暫定0番目を使います 大概service_type=0x01でHD画質だろうか
		_if.reqGetPresentEventInfo (&s_serviceInfos[0], &s_presentEventInfo);

		sectId = SECTID_WAIT_GET_PRESENT_EVENT_INFO;
		enAct = EN_THM_ACT_WAIT;

		} break;

	case SECTID_WAIT_GET_PRESENT_EVENT_INFO:
		enRslt = pIf->getSrcInfo()->enRslt;
        if (enRslt == EN_THM_RSLT_SUCCESS) {
s_presentEventInfo.dump();

			// add reserve
			if (addReserve (&s_presentEventInfo)) {
				sectId = SECTID_END_SUCCESS;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				sectId = SECTID_END_ERROR;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_E ("reqGetPresentEventInfo err");
			sectId = SECTID_END_ERROR;
			enAct = EN_THM_ACT_CONTINUE;
		}

		break;

	case SECTID_END_SUCCESS:

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		memset (s_serviceInfos, 0x00, sizeof(s_serviceInfos));
		memset (&s_presentEventInfo, 0x00, sizeof(s_presentEventInfo));

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

	static _MANUAL_RESERVE_PARAM s_param;
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;


    switch (sectId) {
    case SECTID_ENTRY:

		s_param = *(_MANUAL_RESERVE_PARAM*)(pIf->getSrcInfo()->msg.pMsg);

        sectId = SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID;
        enAct = EN_THM_ACT_CONTINUE;
        break;

	case SECTID_REQ_GET_PYSICAL_CH_BY_SERVICE_ID: {
		// サービスの存在チェックします

		_SERVICE_ID_PARAM param = {
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

		bool r = addReserve (
						s_param.transport_stream_id,
						s_param .original_network_id,
						s_param.service_id,
						0x00, // event_idは不明
						&s_param.start_time,
						&s_param.end_time,
						NULL, // タイトル名も不明
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

		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	case SECTID_END_ERROR:

		memset (&s_param, 0x00, sizeof(s_param));

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


	_REMOVE_RESERVE_PARAM param = *(_REMOVE_RESERVE_PARAM*)(pIf->getSrcInfo()->msg.pMsg);

	if (removeReserve (param.index, param.isConsiderRepeatability)) {
		pIf->reply (EN_THM_RSLT_SUCCESS);
	} else {
		pIf->reply (EN_THM_RSLT_ERROR);
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


	if (m_recording.state == EN_RESERVE_STATE__NOW_RECORDING) {

		// stopRecording が呼ばれたらエラー終了にしておきます
		_UTL_LOG_W ("m_recProgress = EN_REC_PROGRESS__END_ERROR");
		m_recProgress = EN_REC_PROGRESS__END_ERROR;
		m_recording.state = EN_RESERVE_STATE__END_ERROR__FORCE_STOP;

		pIf->reply (EN_THM_RSLT_SUCCESS);

	} else {

		_UTL_LOG_E ("invalid rec state");
		pIf->reply (EN_THM_RSLT_ERROR);
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
	if (pIf->getSrcInfo()->nClientId == m_tunerNotify_clientId) {

		EN_TUNER_STATE enState = *(EN_TUNER_STATE*)(pIf->getSrcInfo()->msg.pMsg);
		switch (enState) {
		case EN_TUNER_STATE__TUNING_BEGIN:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNING_BEGIN");
			break;

		case EN_TUNER_STATE__TUNING_SUCCESS:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNING_SUCCESS");
			break;

		case EN_TUNER_STATE__TUNING_ERROR_STOP:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNING_ERROR_STOP");
			break;

		case EN_TUNER_STATE__TUNE_STOP:
			_UTL_LOG_I ("EN_TUNER_STATE__TUNE_STOP");
			break;

		default:
			break;
		}


	} else if (pIf->getSrcInfo()->nClientId == m_patDetectNotify_clientId) {

		EN_PAT_DETECT_STATE _state = *(EN_PAT_DETECT_STATE*)(pIf->getSrcInfo()->msg.pMsg);
		if (_state == EN_PAT_DETECT_STATE__DETECTED) {
			_UTL_LOG_I ("EN_PAT_DETECT_STATE__DETECTED");

		} else if (_state == EN_PAT_DETECT_STATE__NOT_DETECTED) {
			_UTL_LOG_E ("EN_PAT_DETECT_STATE__NOT_DETECTED");

			// PAT途絶したら録画中止します
			if (m_recording.state == EN_RESERVE_STATE__NOW_RECORDING) {
				m_recProgress = EN_REC_PROGRESS__POST_PROCESS;
				m_recording.state = EN_RESERVE_STATE__END_ERROR__INTERNAL_ERR;


				uint32_t opt = getRequestOption ();
				opt |= REQUEST_OPTION__WITHOUT_REPLY;
				setRequestOption (opt);

				// 自ら呼び出します
				// 内部で自リクエストするので
				// REQUEST_OPTION__WITHOUT_REPLY を入れときます
				this->onTsReceived (NULL, 0);

				opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
				setRequestOption (opt);

			}
		}

	} else if (pIf->getSrcInfo()->nClientId == m_eventChangeNotify_clientId) {

		PSISI_NOTIFY_EVENT_INFO _info = *(PSISI_NOTIFY_EVENT_INFO*)(pIf->getSrcInfo()->msg.pMsg);
		_UTL_LOG_I ("!!! event chenged !!!");
		_info.dump ();

	}

}

bool CRecManager::addReserve (PSISI_EVENT_INFO *p_info)
{
	if (!p_info) {
		return false;
	}

	bool r = addReserve (
					p_info->transport_stream_id,
					p_info->original_network_id,
					p_info->service_id,
					p_info->event_id,
					&(p_info->start_time),
					&(p_info->end_time),
					p_info->event_name_char
				);

	return r;
}

bool CRecManager::addReserve (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id,
	uint16_t _event_id,
	CEtime* p_start_time,
	CEtime* p_end_time,
	const char *psz_title_name,
	EN_RESERVE_REPEATABILITY repeatabilitiy
)
{
	if (!p_start_time || !p_end_time) {
		return false;
	}

	if (*p_start_time >= *p_end_time) {
		_UTL_LOG_E ("invalid reserve time");
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
		repeatabilitiy
	);


	if (!isExistEmptyReserve ()) {
		_UTL_LOG_E ("reserve full.");
		return false;
	}

	if (isDuplicateReserve (&tmp)) {
		_UTL_LOG_E ("reserve is duplicate.");
		return false;
	}

	if (isOverrapTimeReserve (&tmp)) {
		_UTL_LOG_E ("reserve time is overrap.");
		return false;
	}


	CRecReserve* p_reserve = searchAscendingOrderReserve (p_start_time);
	if (!p_reserve) {
		_UTL_LOG_E ("reserve full.");
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
		repeatabilitiy
	);


	// 毎回書き込み
	saveReserves ();


	return true;
}

/**
 * indexで指定した予約を削除します
 * 0始まり
 */
bool CRecManager::removeReserve (int index, bool isConsiderRepeatability)
{
	if (index >= RESERVE_NUM_MAX) {
		return false;
	}

	m_reserves [index].state = EN_RESERVE_STATE__END_ERROR__REMOVE_RESERVE;
	setResult (&m_reserves[index]);

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

		if (m_reserves [i].start_time > p_reserve->start_time && m_reserves [i].start_time > p_reserve->end_time) {
			continue;
		}

		if (m_reserves [i].end_time < p_reserve->start_time && m_reserves [i].end_time < p_reserve->end_time) {
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

		if (!m_reserves [i].state == EN_RESERVE_STATE__INIT) {
			continue;
		}

		if (m_reserves [i].start_time < current_time && m_reserves [i].end_time <= current_time) {
			m_reserves [i].state = EN_RESERVE_STATE__END_ERROR__ALREADY_PASSED;
			setResult (&m_reserves[i]);
			checkRepeatability (&m_reserves[i]);
			continue;
		}

		if (m_reserves [i].start_time <= current_time && m_reserves [i].end_time > current_time) {

			// request start recording
			m_reserves [i].state = EN_RESERVE_STATE__REQ_START_RECORDING;

		}
	}
}

/**
 * reservesのエラーのものをを除去します
 */
void CRecManager::refreshReserves (void)
{
	// 逆から見てエラーのものを詰めます
	for (int i = RESERVE_NUM_MAX -1; i >= 0; -- i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves[i].state <= EN_RESERVE_STATE__END_SUCCESS) {
			continue;
		}

		for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
			m_reserves [j] = m_reserves [j+1];
		}

		m_reserves [RESERVE_NUM_MAX -1].clear();

		saveReserves ();
	}
}

bool CRecManager::pickReqStartRecordingReserve (void)
{
	if (m_recording.is_used) {
		return false;
	}

	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (!m_reserves [i].is_used) {
			continue;
		}

		if (m_reserves[i].state == EN_RESERVE_STATE__REQ_START_RECORDING) {

			// 次に録画する予約を取り出します
			m_recording = m_reserves[i];


			// 間詰め
			for (int j = i; j < RESERVE_NUM_MAX -1; ++ j) {
				m_reserves [j] = m_reserves [j+1];
			}
			m_reserves [RESERVE_NUM_MAX -1].clear();

			checkRepeatability (&m_recording);

			saveReserves ();

			return true;
		}
	}

	return false;
}

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
				p_reserve->repeatability
			);

	if (r) {
		_UTL_LOG_I ("addReserve by repeatability -- success.");
	} else {
		_UTL_LOG_W ("addReserve by repeatability -- failure.");
	}
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
	if (!m_recording.is_used) {
		return ;
	}

	if (m_recording.state != EN_RESERVE_STATE__NOW_RECORDING) {
		return ;
	}

	CEtime current_time ;
	current_time.setCurrentTime();

	if (m_recording.end_time <= current_time) {

		// 正常終了します
		if (m_recProgress == EN_REC_PROGRESS__NOW_RECORDING) {
			_UTL_LOG_I ("m_recProgress = EN_REC_PROGRESS__END_SUCCESS");
			m_recProgress = EN_REC_PROGRESS__END_SUCCESS;
		}
	}
}

void CRecManager::dumpReserves (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	if (m_recording.is_used) {
		_UTL_LOG_I ("-----------   now recording   -----------");
		m_recording.dump();
		_UTL_LOG_I ("\n");
	}
	for (int i = 0; i < RESERVE_NUM_MAX; ++ i) {
		if (m_reserves [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			_UTL_LOG_I ("-- index=[%d] --", i);
			m_reserves [i].dump();
		}
	}
}

void CRecManager::dumpResults (void)
{
	_UTL_LOG_I (__PRETTY_FUNCTION__);

	for (int i = 0; i < RESULT_NUM_MAX; ++ i) {
		if (m_results [i].is_used) {
			_UTL_LOG_I ("-----------------------------------------");
			m_results [i].dump();
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

		char tmpfile [PATH_MAX] = {0};
		std::string *p_path = CSettings::getInstance()->getParams()->getRecTsPath();
		snprintf (
			tmpfile,
			sizeof(tmpfile),
			"%s/tmp.m2ts",
			p_path->c_str()
		);

		mp_outputBuffer = create_FileBufferedWriter (768 * 1024, tmpfile);
		if (!mp_outputBuffer) {
			_UTL_LOG_E ("failed to init FileBufferedWriter.");

			_RECORDING_NOTICE _notice = {m_recProgress, EN_RESERVE_STATE__END_ERROR__INTERNAL_ERR};
			requestAsync (
				EN_MODULE_REC_MANAGER,
				EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_recProgress = EN_REC_PROGRESS__END_ERROR;

		} else {

			struct OutputBuffer * const pFileBufferedWriter = mp_outputBuffer;
			mp_outputBuffer = create_TSParser( 8192, pFileBufferedWriter, 1);
			if (!mp_outputBuffer) {
				_UTL_LOG_E ("failed to init TS Parser.");
				OutputBuffer_release (pFileBufferedWriter);

				_RECORDING_NOTICE _notice = {m_recProgress, EN_RESERVE_STATE__END_ERROR__INTERNAL_ERR};
				requestAsync (
					EN_MODULE_REC_MANAGER,
					EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
					(uint8_t*)&_notice,
					sizeof(_notice)
				);

				// next
				m_recProgress = EN_REC_PROGRESS__END_ERROR;

			}


			_RECORDING_NOTICE _notice = {m_recProgress, EN_RESERVE_STATE__INIT};
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

		}
		break;

	case EN_REC_PROGRESS__NOW_RECORDING: {

		// recording
		int r = OutputBuffer_put (mp_outputBuffer, p_ts_data, length);
		if (r < 0) {
			_UTL_LOG_W ("TS write failed");
		}

		}
		break;

	case EN_REC_PROGRESS__END_SUCCESS: {
		_UTL_LOG_I ("EN_REC_PROGRESS__END_SUCCESS");

		_RECORDING_NOTICE _notice = {m_recProgress, EN_RESERVE_STATE__INIT};
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

		_RECORDING_NOTICE _notice = {m_recProgress, EN_RESERVE_STATE__INIT};
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

		if (mp_outputBuffer) {
			OutputBuffer_flush (mp_outputBuffer);
			OutputBuffer_release (mp_outputBuffer);
			mp_outputBuffer = NULL;
		}

		_RECORDING_NOTICE _notice = {m_recProgress, EN_RESERVE_STATE__INIT};
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


//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, struct timespec &t)
{
	archive (cereal::make_nvp("tv_sec", t.tv_sec), cereal::make_nvp("tv_nsec", t.tv_nsec));
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
		cereal::make_nvp("repeatability", r.repeatability),
		cereal::make_nvp("state", r.state),
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

	std::string *p_path = CSettings::getInstance()->getParams()->getRecReserveJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CRecManager::loadReserves (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getRecReserveJsonPath();
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
}

void CRecManager::saveResults (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_results), sizeof(CRecReserve) * RESULT_NUM_MAX);
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getRecResultJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CRecManager::loadResults (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getRecResultJsonPath();
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
}
