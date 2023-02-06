#ifndef _STREAM_HANDLER_H_
#define _STREAM_HANDLER_H_

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

#include "AribB25Process.h"
#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Forker.h"
#include "Settings.h"
#include "AribB25Process.h"
#include "TsAribCommon.h"

#include "TunerControlIf.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "modules.h"


class CStremaHandler : public CTunerControlIf::ITsReceiveHandler
{
public:
	enum class progress : int {
		init = 0,
		pre_process,
		processing,
		process_end_success,
		process_end_error,
		post_process,
	};

	typedef struct {
		CStremaHandler::progress progress;
		uint8_t group_id;
	} notice_t;

public:
	explicit CStremaHandler (threadmgr::CThreadMgrExternalIf *p_ext_if, uint8_t group_id)
		: mp_ext_if (p_ext_if)
		, m_progress (progress::init)
		, m_group_id (group_id)
		, m_service_id (0)
	{
		m_forker.set_log_cb (forker_log_print);
	}

	virtual ~CStremaHandler (void) {
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

			if (!m_forker.create_pipes()) {
				_UTL_LOG_E("create_pipes failure.");
				m_forker.destroy_pipes();
				m_progress = progress::process_end_error;
				break;
			}

			std::string command = "/usr/bin/ffmpeg -i pipe:0 -f hls -hls_segment_type mpegts -hls_time 10 -hls_list_size 0 -hls_allow_cache 0 -hls_playlist_type event -hls_flags delete_segments -hls_segment_filename stream_%%05d.ts -hls_wrap 5 -c:v h264_omx -b:v 4000k -c:a aac -ac 2 -ab 128K -ar 48000 stream.m3u8";
			if (!m_forker.do_fork(command)) {
				_UTL_LOG_E("do_fork failure.");
				m_forker.destroy_pipes();
				m_progress = progress::process_end_error;
				break;
			}

			auto handler = [&](void *buff, size_t size) {
				int fd = m_forker.get_child_stdin_fd();
				while (size > 0) {
					size_t done = write (fd, buff, size);
					size -= done;		
				}
			};
			std::unique_ptr<CAribB25Process> b25 (new CAribB25Process(8192, m_service_id, m_use_splitter, std::move(handler)));
			m_b25 = std::move(b25);

//			notice_t _notice = {m_progress, m_group_id};
//			mp_ext_if->request_async (
//				static_cast<uint8_t>(module::module_id::rec_manager),
//				static_cast<int>(CRecManagerIf::sequence::recording_notice),
//				(uint8_t*)&_notice,
//				sizeof(_notice)
//			);

			// next
			m_progress = progress::processing;
			_UTL_LOG_I ("next->  progress::processing");

			}
			break;

		case progress::processing: {

			if (m_b25) {
				int r = m_b25->put ((uint8_t*)p_ts_data, length);
				if (r < 0) {
					_UTL_LOG_W ("TS write failed");
				}
			}

			}
			break;

		case progress::process_end_success: {
			_UTL_LOG_I ("progress::process_end_success");

//			notice_t _notice = {m_progress, m_group_id};
//			mp_ext_if->request_async (
//				static_cast<uint8_t>(module::module_id::rec_manager),
//				static_cast<int>(CRecManagerIf::sequence::recording_notice),
//				(uint8_t*)&_notice,
//				sizeof(_notice)
//			);

			// next
			m_progress = progress::post_process;

			}
			break;

		case progress::process_end_error: {
			_UTL_LOG_I ("progress::process_end_error");

//			notice_t _notice = {m_progress, m_group_id};
//			mp_ext_if->request_async (
//				static_cast<uint8_t>(module::module_id::rec_manager),
//				static_cast<int>(CRecManagerIf::sequence::recording_notice),
//				(uint8_t*)&_notice,
//				sizeof(_notice)
//			);

			// next
			m_progress = progress::post_process;

			}
			break;

		case progress::post_process: {
			_UTL_LOG_I ("progress::post_process");

			if (m_b25) {
				m_b25->process_remaining();
				m_b25->finalize();
			}

			m_forker.wait_child(SIGINT);
			m_forker.destroy_pipes();

//			notice_t _notice = {m_progress, m_group_id};
//			mp_ext_if->request_async (
//				static_cast<uint8_t>(module::module_id::rec_manager),
//				static_cast<int>(CRecManagerIf::sequence::recording_notice),
//				(uint8_t*)&_notice,
//				sizeof(_notice)
//			);

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
	static int forker_log_print (FILE *fp, const char* format, ...)
	{
		char buff [128] = {0};
		va_list va;
		va_start (va, format);
		vsnprintf (buff, sizeof(buff), format, va);
		if (fp == stderr) {
			_UTL_LOG_E (buff);
		} else {
			_UTL_LOG_I (buff);
		}
		va_end (va);
		return 0;
	}


	threadmgr::CThreadMgrExternalIf *mp_ext_if;
	progress m_progress;
	std::unique_ptr<CAribB25Process> m_b25;
	uint8_t m_group_id;
	uint16_t m_service_id;
	bool m_use_splitter;
	CForker m_forker;
};

#endif
