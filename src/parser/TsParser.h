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

#include "Defs.h"

#include "TsAribCommon.h"
#include "Utils.h"


class CTsParser
{
public:
	class IParserListener {
	public:
		virtual ~IParserListener (void) {};
		virtual bool on_ts_packet_available (TS_HEADER *p_header, uint8_t *p_payload, size_t payload_size) = 0;
	};

	class CBitRate {
	public:
		static constexpr uint64_t pcr_max = 0x1ffffffffULL * 300 + 0x1ff; // pcr 42bit counter (33bit * 300 + 9bit)
		static constexpr double pcr_clock_freq = 27000000; // 27MHz

		CBitRate (void)
			: m_interval_sec(0)
			, m_interval_cb(nullptr)
			, m_total_size(0)
			, m_start_pcr(0)
			, m_end_pcr(0)
		{
		}
		virtual ~CBitRate (void) = default;

		void set_interval (int interval_sec, std::function<void(double bitrate, uint64_t bytes, uint64_t clocks)> interval_cb) {
			m_interval_sec = interval_sec;
			m_interval_cb = interval_cb;
		}

		void reset (void) {
			m_total_size = 0;
			m_start_pcr = 0;
			m_end_pcr = 0;
			m_start_tp = std::chrono::steady_clock::now();
		}

		void update_size (size_t size) {
			m_total_size += size;
		}

		void update_pcr (uint64_t pcr) {
			if (pcr == 0) {
				return;
			}

			if (m_start_pcr == 0) {
				m_start_pcr = pcr;
				m_end_pcr = pcr;
			} else {
				m_end_pcr = pcr;
			}

			auto now = std::chrono::steady_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_tp).count();
			if (diff > m_interval_sec) {
				uint64_t pcr_d = 0;
				if (m_end_pcr >= m_start_pcr) {
					pcr_d = m_end_pcr - m_start_pcr;
				} else {
					pcr_d = (m_end_pcr + pcr_max) - m_start_pcr;
				}
				double bitrate = (double)m_total_size * 8 / (pcr_d / pcr_clock_freq);
				if (m_interval_cb) {
					m_interval_cb (bitrate, m_total_size, pcr_d);
				}

				reset();
			}
		}

	private:
		int m_interval_sec;
		std::function<void(double bitrate, uint64_t bytes, uint64_t clocks)> m_interval_cb;
		uint64_t m_total_size;
		uint64_t m_start_pcr;
		uint64_t m_end_pcr;
		std::chrono::steady_clock::time_point m_start_tp;
	};

public:
	static constexpr uint32_t INNER_BUFF_SIZE = 65535*5;

	enum class buff_state : int {
		CACHING = 0,
		FULL,
	};

	CTsParser (void);
	explicit CTsParser (IParserListener *p_listener);
	virtual ~CTsParser (void) = default;

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
	int m_parse_remain_len;

	IParserListener *mp_listener;

	std::unique_ptr<uint8_t[]> m_inner_buff;

	CBitRate m_bitrate;
};

#endif
