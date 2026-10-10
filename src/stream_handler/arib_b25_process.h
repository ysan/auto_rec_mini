#ifndef _ARIB_B25_PROCESS_H_
#define _ARIB_B25_PROCESS_H_

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include <arib25/arib_std_b25.h>
#include <arib25/b_cas_card.h>
#include <utility>

#include "buffered_process.h"
#include "utils.h"
#include "ts_arib_common.h"
#include "tssplitter_lite.h"


namespace arec {
namespace stream_handler {

class arib_b25_process : public arec::common::buffered_process
{
public:
	arib_b25_process (size_t size, uint16_t service_id, bool use_splitter, std::function<int (const void *buff, size_t size)> b25_processed_handler)
		: arec::common::buffered_process (size > TS_PACKET_LEN * 512 ? size : TS_PACKET_LEN * 512)
		, b25_ (NULL)
		, b25cas_ (NULL)
		, service_id_ (service_id)
		, use_splitter_(use_splitter)
		, splitter_(NULL)
		, split_select_state_(TSS_ERROR)
		, b25_processed_handler_(std::move(b25_processed_handler))
	{
		init();
		init_b25();

		std::string sid = std::to_string(service_id_);
		memset (service_id_string_, 0x00, sizeof(service_id_string_));
		strncpy (service_id_string_, sid.c_str(), sid.length());

		if (use_splitter_) {
			// init tssplitter_lite
			_UTL_LOG_I ("split_startup %d (0x%04x) -> [%s]", service_id_, service_id_, service_id_string_);
			splitter_ = split_startup(service_id_string_);
			if (splitter_->sid_list == NULL) {
				_UTL_LOG_W("Cannot start TS splitter");
			}
			splitbuf_.buffer = NULL;
			splitbuf_.buffer_size = 0;
		}
	}
	
	virtual ~arib_b25_process (void) {
		finaliz_b25();

		if (use_splitter_) {
			// finaliz tssplitter_lite
			if (splitbuf_.buffer) {
				free (splitbuf_.buffer);
				splitbuf_.buffer = NULL;
				splitbuf_.buffer_size = 0;
			}
			if (splitter_) {
				split_shutdown(splitter_);
				splitter_ = NULL;
			}
		}
	}

	void set_service_id (uint16_t _id) {
		service_id_ = _id;
	}

	void init () {

		std::function<int(const uint8_t*, size_t)> _process = [this] (const uint8_t* p_buffer, size_t length) {
			int r = 0;
			uint8_t* p = const_cast<uint8_t*>(p_buffer);
			size_t len = length;
			if (len == 0) {
				return 0;
			}

			if (b25_) {
				ARIB_STD_B25_BUFFER b25_sbuf;
				ARIB_STD_B25_BUFFER b25_dbuf;
				b25_sbuf.data = p;
				b25_sbuf.size = len;
				if ((r = b25_->put(b25_, &b25_sbuf) ) < 0) {
					_UTL_LOG_W ("StdB25.put failed");
				}
				if ((r = b25_->get(b25_, &b25_dbuf) ) < 0) {
					_UTL_LOG_W ("StdB25.get failed");
				} else {
					p = b25_dbuf.data;
					len = b25_dbuf.size;
				}
			}
			
			if (use_splitter_) {
				int code = 0;
				ARIB_STD_B25_BUFFER buf;
				buf.data = p;
				buf.size = len;
				splitbuf_.buffer_filled = 0;

				/* allocate split buffer */
				if (splitbuf_.buffer_size < buf.size) {
					if (splitbuf_.buffer) {
						free(splitbuf_.buffer);
						splitbuf_.buffer = NULL;
						splitbuf_.buffer_size = 0;
					}
					splitbuf_.buffer = (uint8_t*)malloc(buf.size);
					if(splitbuf_.buffer == NULL) {
						_UTL_LOG_E("split buffer allocation failed");
						// next
						return 0;
					}
					splitbuf_.buffer_size = buf.size;
				}

				if (split_select_state_ != TSS_SUCCESS) {
					split_select_state_ = split_select(splitter_, &buf);
					if (split_select_state_ == TSS_SUCCESS) {
						_UTL_LOG_I ("split_select_state_ ==> TSS_SUCCESS");

					} else if (split_select_state_ == TSS_NULL) {
						_UTL_LOG_E("split_select inner malloc failed");
						// next
						return 0;

					} else {
						// TSS_ERROR
#if 0
						time_t cur_time;
						time(&cur_time);
						if(cur_time - tdata->start_time > 4) {
							goto fin;
						}
						break;
#endif
						// next
						return 0;
					}
				}

				code = split_ts(splitter_, &buf, &splitbuf_);
				if (code == TSS_NULL) {
					_UTL_LOG_W("PMT reading..");
				} else if(code != TSS_SUCCESS) {
					_UTL_LOG_W("split_ts failed");
				}

				p = splitbuf_.buffer;
				len = splitbuf_.buffer_filled;
			}

			if (b25_processed_handler_) {
				if (b25_processed_handler_ (p, len) < 0) {
					return -1;
				}
			}

			return 0;
		};

		set_process_handler (_process);
	}

	void init_b25 (void) {
		int r = 0;
		const unsigned MULTI2_round = 4;
		ARIB_STD_B25* const b25 = create_arib_std_b25();
		if (b25) {
			_UTL_LOG_I ("# StdB25 interface : ");
		} else {
			_UTL_LOG_E ("Create StdB25 interface failed");
			b25_ = NULL;
			b25cas_ = NULL;
			return;
		}
		r = b25->set_multi2_round(b25, MULTI2_round);
		if (r < 0) {
			_UTL_LOG_E ("StdB25.MULTI2 round=%u failed", MULTI2_round);
			b25->release(b25);
			b25_ = NULL;
			b25cas_ = NULL;
			return;
		}
		b25->set_strip(b25, 1);
		b25->set_emm_proc(b25, 0);

		B_CAS_CARD* const bc = create_b_cas_card();
		if (!bc) {
			_UTL_LOG_E ("Create CAS interface failed");
			b25->release(b25);
			b25_ = NULL;
			b25cas_ = NULL;
			return;
		}
		r = bc->init(bc);
		if(r < 0) {
			_UTL_LOG_E ("Init CAS failed");
			bc->release(bc);
			b25->release(b25);
			b25_ = NULL;
			b25cas_ = NULL;
			return;
		}
		r = b25->set_b_cas_card(b25, bc);
		if(r < 0) {
			_UTL_LOG_E ("StdB25.SetCAS failed");
			bc->release(bc);
			b25->release(b25);
			b25_ = NULL;
			b25cas_ = NULL;
			return;
		}
		b25_ = b25;
		b25cas_ = bc;

		B_CAS_ID casID;
		r = bc->get_id(bc, &casID);
		if (r < 0) {
			_UTL_LOG_E ("Get CAS ID failed");
		} else {
			for(r = 0; r < casID.count; r++) {
				const uint64_t  val = casID.data[r];
				_UTL_LOG_I ("%u%014llu, ", (val >> 45) & 0x7,val & ((1ULL << 45) - 1));
			}
		}
		_UTL_LOG_I ("done.");
	}

	void finaliz_b25 (void) {
		if (b25_) {
			b25_->release (b25_);
			b25_ = NULL;
		}
		if (b25cas_) {
			b25cas_->release (b25cas_);
			b25cas_ = NULL;
		}
	}

	void flush (void) {
		arec::common::buffered_process::finalize();
	}

private:
	ARIB_STD_B25 *b25_;
	B_CAS_CARD *b25cas_;

	uint16_t service_id_;
	char service_id_string_ [64];

	bool use_splitter_;
	splitter *splitter_;
	splitbuf_t splitbuf_;
	int split_select_state_;
	std::function<int (void *buff, size_t size)> b25_processed_handler_;
};

} // namespace stream_handler
} // namespace arec

#endif
