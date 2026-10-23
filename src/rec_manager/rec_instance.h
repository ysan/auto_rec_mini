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

#include "coseqpp.h"

#include "utils.h"
#include "settings.h"
#include "rec_manager_if.h"
#include "rec_arib_b25.h"
#include "ts_arib_common.h"

#include "tuner_control_if.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "modules.h"


namespace arec {
namespace rec {

class instance : public arec::tuner_control::client_iface::i_ts_receive_handler
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
	explicit instance (coseq::manager *p_ext_if, uint8_t group_id)
		: ext_if_ (p_ext_if)
		, rec_progress_ (progress::INIT)
		, group_id_ (group_id)
		, service_id_ (0)
	{
		recording_tmpfile_.clear();
	}

	virtual ~instance (void) {
	}


	void set_rec_filename (std::string &name) {
		recording_tmpfile_ = name;
	}

	void set_next_progress (progress _progress) {
		rec_progress_ = _progress;
	}

	progress get_current_progress (void) const {
		return rec_progress_;
	}

	void set_service_id (uint16_t _id) {
		service_id_ = _id;
	}

	void set_use_splitter (bool _b) {
		use_splitter_ = _b;
	}


	// arec::tuner_control::client_iface::i_ts_receive_handler
	bool on_pre_ts_receive (void) override {
		// coseq: request_async は fire-and-forget のため option/外部CP 操作は不要
		return true;
	}

	void on_post_ts_receive (void) override {
		// coseq: 上記同様に不要
	}

	bool on_check_ts_receive_loop (void) override {
		return true;
	}

	bool on_ts_received (void *p_ts_data, int length) override {
		switch (rec_progress_) {
		case progress::PRE_PROCESS: {
			_UTL_LOG_I ("progress::PRE_PROCESS");

			std::unique_ptr<arib_b25> b25 (new arib_b25(8192, recording_tmpfile_, service_id_, use_splitter_));
			b25_ = std::move(b25);

			RECORDING_NOTICE_t _notice = {rec_progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(client_iface::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			rec_progress_ = progress::NOW_RECORDING;
			_UTL_LOG_I ("next  progress::NOW_RECORDING");

			}
			break;

		case progress::NOW_RECORDING: {

			// recording
			int r = b25_->put ((uint8_t*)p_ts_data, length);
			if (r < 0) {
				_UTL_LOG_W ("TS write failed");
			}

			}
			break;

		case progress::END_SUCCESS: {
			_UTL_LOG_I ("progress::END_SUCCESS");

			RECORDING_NOTICE_t _notice = {rec_progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(client_iface::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			rec_progress_ = progress::POST_PROCESS;

			}
			break;

		case progress::END_ERROR: {
			_UTL_LOG_I ("progress::END_ERROR");

			RECORDING_NOTICE_t _notice = {rec_progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(client_iface::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			rec_progress_ = progress::POST_PROCESS;

			}
			break;

		case progress::POST_PROCESS: {
			_UTL_LOG_I ("progress::POST_PROCESS");

			b25_->flush();
			b25_ = nullptr;

			RECORDING_NOTICE_t _notice = {rec_progress_, group_id_};
			ext_if_->request_async (
				static_cast<uint8_t>(modules::module_id::rec_manager),
				static_cast<int>(client_iface::sequence::recording_notice),
				(uint8_t*)&_notice,
				sizeof(_notice)
			);

			// next
			rec_progress_ = progress::INIT;

			}
			break;

		default:
			break;
		}

		return true;
	}


private:
	coseq::manager *ext_if_;
	progress rec_progress_;
	std::string recording_tmpfile_;
	std::unique_ptr<arib_b25> b25_;
	uint8_t group_id_;
	uint16_t service_id_;
	bool use_splitter_;
};

} // namespace rec
} // namespace arec

#endif
