#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TsAribCommon.h"
#include "TsParser.h"
#include "Utils.h"


CTsParser::CTsParser (void)
	: mp_top (nullptr)
	, mp_current (nullptr)
	, mp_bottom (nullptr)
	, m_parse_remain_len (0)
	, mp_listener (nullptr)
{
	reset();

	m_bitrate.set_interval(60*2, [](double bitrate, uint64_t bytes, uint64_t clocks){
		_UTL_LOG_I("bitrate -- %.3f bps (%llu bytes * 8 / (%llu clocks * 27MHz))", bitrate, bytes, clocks);
	});
}

CTsParser::CTsParser (IParserListener *p_listener)
	: mp_top (nullptr)
	, mp_current (nullptr)
	, mp_bottom (nullptr)
	, m_parse_remain_len (0)
	, mp_listener (nullptr)
{
	if (p_listener) {
		mp_listener = p_listener;
	}

	reset();

	m_bitrate.set_interval(60*2, [](double bitrate, uint64_t bytes, uint64_t clocks){
		_UTL_LOG_I("bitrate -- %.3f bps (%llu bytes * 8 / (%llu clocks * 27MHz))", bitrate, bytes, clocks);
	});
}

void CTsParser::reset (void)
{
	std::unique_ptr<uint8_t[]> up (new uint8_t[INNER_BUFF_SIZE]);
	m_inner_buff = std::move(up);

	mp_top = nullptr;
	mp_current = nullptr;
	mp_bottom = nullptr;
	m_parse_remain_len = 0;

	m_bitrate.reset();
}

void CTsParser::put (uint8_t *p_buff, size_t size)
{
	if ((!p_buff) || (size == 0)) {
		return ;
	}

	mp_top = m_inner_buff.get();
	if (copy_to_inner_buffer (p_buff, size) == buff_state::FULL) {

		mp_current = mp_top;
		parse ();

		if (m_parse_remain_len > 0) {
			memcpy (mp_top, mp_bottom - m_parse_remain_len, m_parse_remain_len);
			mp_bottom = mp_top + m_parse_remain_len;
		} else {
			mp_bottom = mp_top;
		}

		if (copy_to_inner_buffer (p_buff, size) != buff_state::CACHING) {
			_UTL_LOG_E ("unexpected copy_to_inner_buffer");
			return;
		}
	}
}

CTsParser::buff_state CTsParser::copy_to_inner_buffer (uint8_t *p_buff, size_t size)
{
	if ((!p_buff) || (size == 0)) {
		return buff_state::CACHING;
	}

	if (size > INNER_BUFF_SIZE) {
		_UTL_LOG_E ("copy_to_inner_buffer: size > INNER_BUFF_SIZE\n");
		return buff_state::CACHING;
	}

	int remain = 0;
	if (mp_bottom) {
		remain = INNER_BUFF_SIZE - (mp_bottom - mp_top);

		if (remain >= (int)size) {
			memcpy (mp_bottom, p_buff, size);
			mp_bottom += size;
			return buff_state::CACHING;

		} else {
			// don't copy, do parse
			return buff_state::FULL;
		}

	} else {
		remain = INNER_BUFF_SIZE;

		memcpy (mp_top, p_buff, size);
		mp_bottom = mp_top + size;
		return buff_state::CACHING;
	}
}

int CTsParser::get_unit_size (void) const
{
	int unit_size = 0;
	uint8_t *p_cur = mp_current;

	while ((p_cur + 204*3) < mp_bottom) {
		if((*p_cur == TS_SYNC_BYTE) && (*(p_cur+188) == TS_SYNC_BYTE) && (*(p_cur+188*2) == TS_SYNC_BYTE)) {
			unit_size = 188;
			p_cur += 188*2;

		} else if((*p_cur == TS_SYNC_BYTE) && (*(p_cur+192) == TS_SYNC_BYTE) && (*(p_cur+192*2) == TS_SYNC_BYTE)) {
			// TTS(Timestamped TS)
			unit_size = 192;
			p_cur += 192*2;

		} else if((*p_cur == TS_SYNC_BYTE) && (*(p_cur+204) == TS_SYNC_BYTE) && (*(p_cur+204*2) == TS_SYNC_BYTE)) {
			// added FEC(Forward Error Correction)
			unit_size = 204;
			p_cur += 204*2;

		} else {
			++ p_cur;
		}
	}

	return unit_size;
}

uint8_t * CTsParser::get_sync_top_addr (uint8_t *p_start, uint8_t *p_end, size_t unit_size) const
{
	if ((!p_start) || (!p_end) || (unit_size == 0)) {
		return nullptr;
	}

	if ((uint32_t)(p_end - p_start) >= (unit_size * 8)) {
		// unit_size 8コ分のデータがあること
		p_end -= unit_size * 8;
	} else {
		return nullptr;
	}

	int i = 0;
	while (p_start <= p_end) {
		if (*p_start == TS_SYNC_BYTE) {
			for (i = 0; i < 8; ++ i) {
				if (*(p_start + (unit_size * (i + 1))) != TS_SYNC_BYTE) {
					break;
				}
			}
			// 以降 8回全てTS_SYNC_BYTEで一致したら startは先頭です
			if (i == 8) {
				return p_start;
			}
		}
		++ p_start ;
	}

	return nullptr;
}

int CTsParser::check_unit_size (void)
{
	size_t unit_size = 0;
	size_t skip_bytes = 512;
	int err_cnt = 0;
	while (1) {
		unit_size = get_unit_size();
		if (unit_size == 0) {
			if (err_cnt < 20) {
				_UTL_LOG_W ("invalid unit_size=[%d]", unit_size);
			}

			if ((mp_current + skip_bytes) < mp_bottom) {
				// とりあえず skip_bytes 分すすめてみます
				mp_current += skip_bytes;
				++ err_cnt;
				continue;

			} else {
				m_parse_remain_len = 0;
				return 0;
			}

		} else {
			break;
		}
	}
	return unit_size;
}

bool CTsParser::parse (void)
{
	int unit_size = 0;
	int err_cnt = 0;
	TS_HEADER ts_header = {0};
	adaptation_field_header_t af_header = {0};
	uint8_t *p_payload = nullptr;
	uint8_t *p_cur = mp_current;
	uint8_t *p_btm = mp_bottom;
	size_t payload_size = 0;

	unit_size = check_unit_size();
	if (unit_size == 0) {
		return false;
	}

	while ((p_cur + unit_size) < p_btm) {
		if ((*p_cur != TS_SYNC_BYTE) && (*(p_cur + unit_size) != TS_SYNC_BYTE)) {
			uint8_t *top = get_sync_top_addr (p_cur, p_btm, unit_size);
			if (!top) {
				if (err_cnt < 20) {
					_UTL_LOG_W ("invalid get_sync_top_addr (%p-%p) unit_size=[%d]", p_cur, p_btm, unit_size);
				}
				p_cur += unit_size;
				++ err_cnt;
				continue;
			}

			// sync update
			p_cur = top;
		}

		ts_header.parse (p_cur);
		af_header.clear();

		m_bitrate.update_size(unit_size);

		// adaptation_field_control 2bit
		// 00 ISO/IECによる将来の使用のために予約されている。
		// 01 アダプテーションフィールドなし、ペイロードのみ
		// 10 アダプテーションフィールドのみ、ペイロードなし
		// 11 アダプテーションフィールドの次にペイロード
		if ((ts_header.adaptation_field_control & 0x2) == 0x2) {
			// アダプテーションフィールド付き
			size_t adaptation_field_length = *(p_cur + TS_HEADER_LEN);
			p_payload += adaptation_field_length + 1; // lengthとそれ自身の1byte分進める

			if (adaptation_field_length > 0) {
				af_header.parse(p_cur + TS_HEADER_LEN + 1);
				if (af_header.PCR_flag) {
					pcr_t pcr(p_cur + TS_HEADER_LEN + 2);
					m_bitrate.update_pcr(pcr.pcr);
				}
			}

		} else {
			// アダプテーションフィールドなし
			p_payload = p_cur + TS_HEADER_LEN;
		}

		// TTS(Timestamped TS)(total192bytes) や
		// FEC(Forward Error Correction:順方向誤り訂正)(total204bytes)
		// は除外します
//		payload_size = TS_PACKET_LEN - (p_payload - p_cur);
		payload_size = TS_PACKET_LEN - TS_HEADER_LEN;

		if (ts_header.adaptation_field_control != 0x2) {
			// ペイロード有る時のみ
			if (mp_listener) {
				mp_listener->on_ts_packet_available (&ts_header, p_payload, payload_size);
			}
		}

		p_cur += unit_size;
	}

	m_parse_remain_len = p_btm - p_cur;
	_UTL_LOG_D ("m_parse_remain_len=[%d]\n", m_parse_remain_len);


	return true;
}
