#ifndef _REC_INSTANCE_H_
#define _REC_INSTANCE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Settings.h"
#include "RecManagerIf.h"
#include "RecAribB25.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"


using namespace ThreadManager;

class CRecInstance : public CTunerControlIf::ITsReceiveHandler
{
public:
	enum class progress : int {
		INIT = 0,
		PRE_PROCESS,
		NOW_RECORDING,
		END_SUCCESS,
		END_ERROR,
		POST_PROCESS,
	};

	typedef struct {
		progress rec_progress;
		uint8_t groupId;
	} RECORDING_NOTICE_t;

public:
	explicit CRecInstance (CThreadMgrExternalIf *p_ext_if, uint8_t groupId)
		: mp_ext_if (p_ext_if)
		, m_recProgress (progress::INIT)
		, m_groupId (groupId)
	{
		m_recording_tmpfile.clear();
	}

	virtual ~CRecInstance (void) {
	}


	void setRecFilename (std::string &name) {
		m_recording_tmpfile = name;
	}

	void setNextProgress (progress _progress) {
		m_recProgress = _progress;
	}

	progress getCurrentProgress (void) const {
		return m_recProgress;
	}


	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override {
		mp_ext_if->createExternalCp();

		uint32_t opt = mp_ext_if->getRequestOption ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		mp_ext_if->setRequestOption (opt);

		return true;
	}

	void onPostTsReceive (void) override {
		mp_ext_if->createExternalCp();

		uint32_t opt = mp_ext_if->getRequestOption ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		mp_ext_if->setRequestOption (opt);
	}

	bool onCheckTsReceiveLoop (void) override {
		return true;
	}

	bool onTsReceived (void *p_ts_data, int length) override {
		switch (m_recProgress) {
		case progress::PRE_PROCESS: {
			_UTL_LOG_I ("progress::PRE_PROCESS");

			std::unique_ptr<CRecAribB25> b25 (new CRecAribB25(8192, m_recording_tmpfile));
			msp_b25.swap (b25);

			RECORDING_NOTICE_t _notice = {m_recProgress, m_groupId};
			mp_ext_if->requestAsync (
				EN_MODULE_REC_MANAGER,
				EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_recProgress = progress::NOW_RECORDING;
			_UTL_LOG_I ("next  progress::NOW_RECORDING");

			}
			break;

		case progress::NOW_RECORDING: {

			// recording
			int r = msp_b25->put ((uint8_t*)p_ts_data, length);
			if (r < 0) {
				_UTL_LOG_W ("TS write failed");
			}

			}
			break;

		case progress::END_SUCCESS: {
			_UTL_LOG_I ("progress::END_SUCCESS");

			RECORDING_NOTICE_t _notice = {m_recProgress, m_groupId};
			mp_ext_if->requestAsync (
				EN_MODULE_REC_MANAGER,
				EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_recProgress = progress::POST_PROCESS;

			}
			break;

		case progress::END_ERROR: {
			_UTL_LOG_I ("progress::END_ERROR");

			RECORDING_NOTICE_t _notice = {m_recProgress, m_groupId};
			mp_ext_if->requestAsync (
				EN_MODULE_REC_MANAGER,
				EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_recProgress = progress::POST_PROCESS;

			}
			break;

		case progress::POST_PROCESS: {
			_UTL_LOG_I ("progress::POST_PROCESS");

			msp_b25->flush();
			msp_b25->release();

			RECORDING_NOTICE_t _notice = {m_recProgress, m_groupId};
			mp_ext_if->requestAsync (
				EN_MODULE_REC_MANAGER,
				EN_SEQ_REC_MANAGER__RECORDING_NOTICE,
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_recProgress = progress::INIT;

			}
			break;

		default:
			break;
		}

		return true;
	}


private:
	CThreadMgrExternalIf *mp_ext_if;
	progress m_recProgress;
	std::string m_recording_tmpfile;
	unique_ptr<CRecAribB25> msp_b25;
	uint8_t m_groupId;
};

#endif
