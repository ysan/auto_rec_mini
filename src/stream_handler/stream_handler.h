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

#include "group.h"
#include "coseqpp.h"

#include "utils.h"
#include "forker.h"
#include "settings.h"
#include "arib_b25_process.h"
#include "ts_arib_common.h"

#include "tuner_control_if.h"
#include "viewing_manager_if.h"


namespace arec {
namespace stream_handler {

class handler : public arec::tuner_control::client_iface::ITsReceiveHandler
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
		handler::progress progress;
		uint8_t group_id;
	} notice_t;

public:
	explicit handler (coseq::manager *p_ext_if, uint8_t group_id)
		: ext_if_ (p_ext_if)
		, progress_ (progress::init)
		, group_id_ (group_id)
		, service_id_ (0)
		, processed_handler_(nullptr)
	{
	}

	virtual ~handler (void) {
	}


	void set_next_progress (progress _progress) {
		progress_ = _progress;
	}

	progress get_current_progress (void) const {
		return progress_;
	}

	void set_service_id (uint16_t _id) {
		service_id_ = _id;
	}

	void set_use_splitter (bool _b) {
		use_splitter_ = _b;
	}

	void set_process_handler (std::function<int (const void *buff, size_t size)> handler) {
		processed_handler_ = std::move(handler);
	}


	// arec::tuner_control::client_iface::ITsReceiveHandler::on_pre_ts_receive
	bool on_pre_ts_receive (void) override {
		// coseq: request_async は元来 fire-and-forget のため option/外部CP 操作は不要
		return true;
	}

	// arec::tuner_control::client_iface::ITsReceiveHandler::on_post_ts_receive
	void on_post_ts_receive (void) override {
		// coseq: 上記同様に不要
	}

	// arec::tuner_control::client_iface::ITsReceiveHandler::on_check_ts_receive_loop
	bool on_check_ts_receive_loop (void) override {
		return true;
	}

	// arec::tuner_control::client_iface::ITsReceiveHandler::on_ts_received
	bool on_ts_received (void *p_ts_data, int length) override {
		switch (progress_) {
		case progress::init: {
//			_UTL_LOG_I ("progress::init");


			}
			break;
							
		case progress::pre_process: {
			_UTL_LOG_I ("progress::pre_process");

			std::unique_ptr<arib_b25_process> b25 (new arib_b25_process(8192, service_id_, use_splitter_, processed_handler_));
			b25_ = std::move(b25);

			notice_t _notice = {progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(arec::viewing::client_iface::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			progress_ = progress::processing;
			_UTL_LOG_I ("next->  progress::processing");

			}
			break;

		case progress::processing: {

			if (b25_) {
				int r = b25_->put ((uint8_t*)p_ts_data, length);
				if (r < 0) {
					_UTL_LOG_W ("arib_b25_process abnormal...");
				}
			}

			}
			break;

		case progress::process_end_success: {
			_UTL_LOG_I ("progress::process_end_success");

			notice_t _notice = {progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(arec::viewing::client_iface::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			progress_ = progress::post_process;

			}
			break;

		case progress::process_end_error: {
			_UTL_LOG_I ("progress::process_end_error");

			notice_t _notice = {progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(arec::viewing::client_iface::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			progress_ = progress::post_process;

			}
			break;

		case progress::post_process: {
			_UTL_LOG_I ("progress::post_process");

			if (b25_) {
				b25_->flush();
				b25_ = nullptr;
			}

			notice_t _notice = {progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(arec::viewing::client_iface::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			progress_ = progress::init;

			}
			break;

		case progress::post_process_forcibly: {
			_UTL_LOG_I ("progress::post_process_forcibly");

			if (b25_) {
				b25_->flush();
				b25_ = nullptr;
			}

			notice_t _notice = {progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::viewing_manager),
				static_cast<uint8_t>(arec::viewing::client_iface::sequence::notice_by_stream_handler),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			progress_ = progress::init;

			}
			break;

		default:
			break;
		}

		return true;
	}

private:
	coseq::manager *ext_if_;
	progress progress_;
	std::unique_ptr<arib_b25_process> b25_;
	uint8_t group_id_;
	uint16_t service_id_;
	bool use_splitter_;
	std::function<int (const void *buff, size_t size)> processed_handler_;
};

} // namespace stream_handler
} // namespace arec

namespace stream_handler_funcs {
	bool setup_instance (coseq::manager *external_if);
	std::shared_ptr<arec::stream_handler::handler> get_instance (uint8_t group_id);
}; // namespace stream_handler_funcs

#endif
