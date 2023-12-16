#ifndef _STREAM_HANDLER_H_
#define _STREAM_HANDLER_H_

#include <regex>
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
#include <utility>

#include "Group.h"
#include "ThreadMgrExternalIf.h"
#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Forker.h"
#include "Settings.h"
#include "AribB25Process.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"
#include "ViewingManagerIf.h"


class CStreamHandler : public CTunerControlIf::ITsReceiveHandler
{
public:
	enum class progress : int {
		init = 0,
		pre_process,
		processing,
		process_end_success,
		process_end_error,
		post_process,
		post_process_forcibly,
	};

	typedef struct {
		CStreamHandler::progress progress;
		uint8_t group_id;
	} notice_t;

public:
	explicit CStreamHandler (threadmgr::CThreadMgrExternalIf *p_ext_if, uint8_t group_id)
		: mp_ext_if (p_ext_if)
		, m_progress (progress::init)
		, m_group_id (group_id)
		, m_service_id (0)
		, m_processed_handler(nullptr)
	{
	}

	virtual ~CStreamHandler (void) {
	}


	void set_next_progress (progress _progress) {
		m_progress = _progress;
	}

	progress get_current_progress (void) const {
		return m_progress;
	}

	void set_service_id (uint16_t _id) {
		m_service_id = _id;
	}

	void set_use_splitter (bool _b) {
		m_use_splitter = _b;
	}

	void set_process_handler (std::function<int (const void *buff, size_t size)> handler) {
		m_processed_handler = std::move(handler);
	}


	// CTunerControlIf::ITsReceiveHandler::on_pre_ts_receive
	bool on_pre_ts_receive (void) override {
		mp_ext_if->create_external_cp();

		uint32_t opt = mp_ext_if->get_request_option ();
		opt |= REQUEST_OPTION__WITHOUT_REPLY;
		mp_ext_if->set_request_option (opt);

		return true;
	}

	// CTunerControlIf::ITsReceiveHandler::on_post_ts_receive
	void on_post_ts_receive (void) override {
		uint32_t opt = mp_ext_if->get_request_option ();
		opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
		mp_ext_if->set_request_option (opt);

		mp_ext_if->destroy_external_cp();
	}

	// CTunerControlIf::ITsReceiveHandler::on_check_ts_receive_loop
	bool on_check_ts_receive_loop (void) override {
		return true;
	}

	// CTunerControlIf::ITsReceiveHandler::on_ts_received
	bool on_ts_received (void *p_ts_data, int length) override {
		switch (m_progress) {
		case progress::init: {
//			_UTL_LOG_I ("progress::init");


			}
			break;
							
		case progress::pre_process: {
			_UTL_LOG_I ("progress::pre_process");

			std::unique_ptr<CAribB25Process> b25 (new CAribB25Process(8192, m_service_id, m_use_splitter, m_processed_handler));
			m_b25 = std::move(b25);

			notice_t _notice = {m_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(CViewingManagerIf::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_progress = progress::processing;
			_UTL_LOG_I ("next->  progress::processing");

			}
			break;

		case progress::processing: {

			if (m_b25) {
				int r = m_b25->put ((uint8_t*)p_ts_data, length);
				if (r < 0) {
					_UTL_LOG_W ("CAribB25Process abnormal...");
				}
			}

			}
			break;

		case progress::process_end_success: {
			_UTL_LOG_I ("progress::process_end_success");

			notice_t _notice = {m_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(CViewingManagerIf::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_progress = progress::post_process;

			}
			break;

		case progress::process_end_error: {
			_UTL_LOG_I ("progress::process_end_error");

			notice_t _notice = {m_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(CViewingManagerIf::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_progress = progress::post_process;

			}
			break;

		case progress::post_process: {
			_UTL_LOG_I ("progress::post_process");

			if (m_b25) {
				m_b25->flush();
				m_b25 = nullptr;
			}

			notice_t _notice = {m_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(CViewingManagerIf::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_progress = progress::init;

			}
			break;

		case progress::post_process_forcibly: {
			_UTL_LOG_I ("progress::post_process_forcibly");

			if (m_b25) {
				m_b25->flush();
				m_b25 = nullptr;
			}

			notice_t _notice = {m_progress, m_group_id};
			mp_ext_if->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(CViewingManagerIf::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			m_progress = progress::init;

			}
			break;

		default:
			break;
		}

		return true;
	}

private:
	threadmgr::CThreadMgrExternalIf *mp_ext_if;
	progress m_progress;
	std::unique_ptr<CAribB25Process> m_b25;
	uint8_t m_group_id;
	uint16_t m_service_id;
	bool m_use_splitter;
	std::function<int (const void *buff, size_t size)> m_processed_handler;
};

namespace stream_handler_funcs {
	bool setup_instance (threadmgr::CThreadMgrExternalIf *external_if);
	std::shared_ptr<CStreamHandler> get_instance (uint8_t group_id);
}; // namespace stream_handler_funcs

#endif
