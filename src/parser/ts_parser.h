#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <memory>
#include <chrono>
#include <functional>

#include "defs.h"

#include "ts_arib_common.h"
#include "utils.h"

namespace parser {



class ts_parser
{
public:
	class IParserListener {
	public:
		virtual ~IParserListener (void) {};
		virtual bool on_ts_packet_available (TS_HEADER *p_header, uint8_t *p_payload, size_t payload_size) = 0;
	};

	class bit_rate {
	public:
		static constexpr uint64_t pcr_max = 0x1ffffffffULL * 300 + 0x1ff; // pcr 42bit counter (33bit * 300 + 9bit)
		static constexpr double pcr_clock_freq = 27000000; // 27MHz

		bit_rate (void)
			: interval_sec_(0)
			, interval_cb_(nullptr)
			, total_size_(0)
			, start_pcr_(0)
			, end_pcr_(0)
		{
		}
		virtual ~bit_rate (void) = default;

		void set_interval (int interval_sec, std::function<void(double bitrate, uint64_t bytes, uint64_t clocks)> interval_cb) {
			interval_sec_ = interval_sec;
			interval_cb_ = interval_cb;
		}

		void reset (void) {
			total_size_ = 0;
			start_pcr_ = 0;
			end_pcr_ = 0;
			start_tp_ = std::chrono::steady_clock::now();
		}

		void update_size (size_t size) {
			total_size_ += size;
		}

		void update_pcr (uint64_t pcr) {
			if (pcr == 0) {
				return;
			}

			if (start_pcr_ == 0) {
				start_pcr_ = pcr;
				end_pcr_ = pcr;
			} else {
				end_pcr_ = pcr;
			}

			auto now = std::chrono::steady_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - start_tp_).count();
			if (diff > interval_sec_) {
				uint64_t pcr_d = 0;
				if (end_pcr_ >= start_pcr_) {
					pcr_d = end_pcr_ - start_pcr_;
				} else {
					pcr_d = (end_pcr_ + pcr_max) - start_pcr_;
				}
				double bitrate = (double)total_size_ * 8 / (pcr_d / pcr_clock_freq);
				if (interval_cb_) {
					interval_cb_ (bitrate, total_size_, pcr_d);
				}

				reset();
			}
		}

	private:
		int interval_sec_;
		std::function<void(double bitrate, uint64_t bytes, uint64_t clocks)> interval_cb_;
		uint64_t total_size_;
		uint64_t start_pcr_;
		uint64_t end_pcr_;
		std::chrono::steady_clock::time_point start_tp_;
	};

public:
	static constexpr uint32_t INNER_BUFF_SIZE = 65535*5;

	enum class buff_state : int {
		CACHING = 0,
		FULL,
	};

	ts_parser (void);
	explicit ts_parser (IParserListener *p_listener);
	virtual ~ts_parser (void) = default;

	void reset (void);

	void put (uint8_t *buff, size_t size);

private:
	buff_state copy_to_inner_buffer (uint8_t *p_buff, size_t size);
	int get_unit_size (void) const;
	uint8_t *get_sync_top_addr (uint8_t *p_top, uint8_t *p_bottom, size_t unit_size) const;
	int check_unit_size (void);
	bool parse (void);


	uint8_t *mp_top ;
	uint8_t *mp_current ;
	uint8_t *mp_bottom ;
	int parse_remain_len_;

	IParserListener *mp_listener;

	std::unique_ptr<uint8_t[]> inner_buff_;

	bit_rate bitrate_;
};


} // namespace parser

#endif
