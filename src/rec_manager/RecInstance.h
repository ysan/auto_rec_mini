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
#include "modules.h"


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
		uint8_t group_id;
	} RECORDING_NOTICE_t;

public:
	explicit CRecInstance (threadmgr::CThreadMgrExternalIf *p_ext_if, uint8_t group_id)
		: mp_ext_if (p_ext_if)
		, m_rec_progress (progress::INIT)
		, m_group_id (group_id)
		, m_service_id (0)
	{
		m_recording_tmpfile.clear();
	}

	virtual ~CRecInstance (void) {
	}


	void set_rec_filename (std::string &name) {
		m_recording_tmpfile = name;
	}

	void set_next_progress (progress _progress) {
		m_rec_progress = _progress;
	}

	progress get_current_progress (void) const {
		return m_rec_progress;
	}

	void set_service_id (uint16_t _id) {
		m_service_id = _id;
	}

	void set_use_splitter (bool _b) {
		m_use_splitter = _b;
	}


	// CTunerControlIf::ITsReceiveHandler
	bool on_pre_ts_receive (void) override {
		mp_ext_if->create_external_cp();

		uint32_t opt = mp_ext_if->get_request_option ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		mp_ext_if->set_request_option (opt);

		return true;
	}

	void on_post_ts_receive (void) override {
		uint32_t opt = mp_ext_if->get_request_option ();
		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		mp_ext_if->set_request_option (opt);

		mp_ext_if->destroy_external_cp();
	}

	bool on_check_ts_receive_loop (void) override {
		return true;
	}

	bool on_ts_received (void *p_ts_data, int length) override {
		switch (m_rec_progress) {
		case progress::PRE_PROCESS: {
			_UTL_LOG_I ("progress::PRE_PROCESS");

			std::unique_ptr<CRecAribB25> b25 (new CRecAribB25(8192, m_recording_tmpfile, m_service_id, m_use_splitter));
			msp_b25 = std::move(b25);

			RECORDING_NOTICE_t _notice = {m_rec_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(CRecManagerIf::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_rec_progress = progress::NOW_RECORDING;
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

			RECORDING_NOTICE_t _notice = {m_rec_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(CRecManagerIf::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_rec_progress = progress::POST_PROCESS;

			}
			break;

		case progress::END_ERROR: {
			_UTL_LOG_I ("progress::END_ERROR");

			RECORDING_NOTICE_t _notice = {m_rec_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(CRecManagerIf::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_rec_progress = progress::POST_PROCESS;

			}
			break;

		case progress::POST_PROCESS: {
			_UTL_LOG_I ("progress::POST_PROCESS");

			msp_b25->flush();
			msp_b25 = nullptr;

			RECORDING_NOTICE_t _notice = {m_rec_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(CRecManagerIf::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_rec_progress = progress::INIT;

			}
			break;

		default:
			break;
		}

		return true;
	}


private:
	threadmgr::CThreadMgrExternalIf *mp_ext_if;
	progress m_rec_progress;
	std::string m_recording_tmpfile;
	std::unique_ptr<CRecAribB25> msp_b25;
	uint8_t m_group_id;
	uint16_t m_service_id;
	bool m_use_splitter;
};

#endif
