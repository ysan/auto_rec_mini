#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TsParser.h"
#include "Utils.h"


const uint32_t CTsParser::INNER_BUFF_SIZE = 65535*5;

CTsParser::CTsParser (void)
	:mp_top (NULL)
	,mp_current (NULL)
	,mp_bottom (NULL)
	,m_buff_size (0)
	,m_parse_remain_len (0)
	,mp_listener (NULL)
{
//	memset (m_inner_buff, 0x00, INNER_BUFF_SIZE);
	std::unique_ptr<uint8_t[]> up (new uint8_t[INNER_BUFF_SIZE]);
	m_inner_buff.swap(up);
}

CTsParser::CTsParser (IParserListener *p_listener)
	:mp_top (NULL)
	,mp_current (NULL)
	,mp_bottom (NULL)
	,m_buff_size (0)
	,m_parse_remain_len (0)
	,mp_listener (NULL)
{
	if (p_listener) {
		mp_listener = p_listener;
	}

//	memset (m_inner_buff, 0x00, INNER_BUFF_SIZE);
	std::unique_ptr<uint8_t[]> up (new uint8_t[INNER_BUFF_SIZE]);
	m_inner_buff.swap(up);
}

CTsParser::~CTsParser (void)
{
}

void CTsParser::run (uint8_t *p_buff, size_t size)
{
	if ((!p_buff) || (size == 0)) {
		return ;
	}


#if 0
	allocInnerBuffer (p_buff, size);
	parse ();

#else
//	mp_top = &m_inner_buff[0];
	mp_top = m_inner_buff.get();
	if (copyInnerBuffer (p_buff, size) == buff_state::FULL) {

		mp_current = mp_top;
		parse ();

		if (m_parse_remain_len > 0) {
//			memcpy (m_inner_buff, mp_bottom - m_parse_remain_len, m_parse_remain_len);
//			mp_bottom = &m_inner_buff[0] + m_parse_remain_len;
			memcpy (mp_top, mp_bottom - m_parse_remain_len, m_parse_remain_len);
			mp_bottom = mp_top + m_parse_remain_len;
		} else {
//			mp_bottom = &m_inner_buff[0];
			mp_bottom = mp_top;
		}

		if (copyInnerBuffer (p_buff, size) != buff_state::CACHING) {
			_UTL_LOG_E ("unexpected copyInnerBuffer");
			return;
		}
	}
#endif

}

//TODO 足らなくなったら拡張 全部貯めるかたち
bool CTsParser::allocInnerBuffer (uint8_t *p_buff, size_t size)
{
	if ((!p_buff) || (size == 0)) {
		return false;
	}

	size_t total_data_size = (mp_bottom - mp_top) + size;

	if (m_buff_size >= total_data_size) {
		// 	実コピー
		memcpy (mp_bottom, p_buff, size);
		mp_current = mp_bottom - m_parse_remain_len;  // current update
		mp_bottom += size;
		_UTL_LOG_D ("allocInnerBuffer size=%lu remain=%lu\n", m_buff_size, m_buff_size - total_data_size);
		return true;
	}

	// 残りサイズ足りないので確保 初回もここに

	size_t n = 512;
	while (n < (size + m_buff_size)) {
		n += n;
	}

	uint8_t *p = (uint8_t*) malloc (n);
	if (!p) {
		return false;
	}
	memset (p, 0x00, n);


	int m = 0;
	if (mp_top != NULL) {
		// 既にデータ入っている場合はそれをコピーして 元は捨てます
		m = mp_bottom - mp_top;
		if (m > 0) {
			memcpy(p, mp_top, m);
		}
		free (mp_top);
		mp_top = NULL;
    }
	mp_top = p;
	mp_bottom = p + m;
    m_buff_size = n;


	// 	実コピー
	memcpy (mp_bottom, p_buff, size);
	mp_current = mp_bottom - m_parse_remain_len;  // current update
	mp_bottom += size;

	_UTL_LOG_D ("allocInnerBuffer(malloc) top=%p bottom=%p size=%lu remain=%lu\n", mp_top, mp_bottom, m_buff_size, m_buff_size - total_data_size);
	return true;
}

// 静的領域にコピー
CTsParser::buff_state CTsParser::copyInnerBuffer (uint8_t *p_buff, size_t size)
{
	if ((!p_buff) || (size == 0)) {
		return buff_state::CACHING;
	}

	if (size > INNER_BUFF_SIZE) {
		_UTL_LOG_E ("copyInnerBuffer: size > INNER_BUFF_SIZE\n");
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

int CTsParser::getUnitSize (void) const
{
	int i = 0;
	int m = 0;
	int n = 0;
//	int w = 0;
	int count [320-188] = {0};


	uint8_t *p_cur = mp_current;
	memset (count, 0x00, sizeof(count));

	while ((p_cur + 188) < mp_bottom) {
		if (*p_cur != SYNC_BYTE) {
			++ p_cur;
			continue;
		}

		m = 320;
		if ((p_cur+m) > mp_bottom) {
			// mを 320満たないのでまでにbottomに切り詰め
			m = mp_bottom - p_cur;
		}
//printf("%p m=%d ", p_cur, m);

		for (i = 188; i < m; i ++) {
			if(*(p_cur+i) == SYNC_BYTE){
				// 今見つかったSYNC_BYTEから以降の(320-188)byte間に
				// SYNC_BYTEがあったら カウント
				count [i-188] += 1;
			}
		}

		++ p_cur;

//for (int iii=0; iii < 320-188; iii++) {
// printf ("%d", count[iii]);
//}
//printf ("\n");
	}

	// 最大回数m をみてSYNC_BYTE出現間隔nを決める
	m = 0;
	n = 0;
	for (i = 188; i < 320; i ++) {
		if (m < count [i-188]) {
			m = count [i-188];
			n = i;
		}
	}
	_UTL_LOG_D ("max_interval count m=%d  max_interval n=%d\n", m, n);

	//TODO
//	w = m*n;
//	if ((m < 8) || ((w+3*n) < (mp_bottom-mp_current))) {
//		return false;
//	}

	return n;
}

uint8_t * CTsParser::getSyncTopAddr (uint8_t *p_top, uint8_t *p_btm, size_t unit_size) const
{
	if ((!p_top) || (!p_btm) || (unit_size == 0)) {
		return NULL;
	}

	if ((uint32_t)(p_btm - p_top) >= (unit_size * 8)) {
		// unit_size 8コ分のデータがあること
		p_btm -= unit_size * 8;
	} else {
		return NULL;
	}

	int i = 0;
	while (p_top <= p_btm) {
		if (*p_top == SYNC_BYTE) {
			for (i = 0; i < 8; ++ i) {
				if (*(p_top + (unit_size * (i + 1))) != SYNC_BYTE) {
					break;
				}
			}
			// 以降 8回全てSYNC_BYTEで一致したら p_topは先頭です
			if (i == 8) {
				return p_top;
			}
		}
		++ p_top ;
	}

	return NULL;
}

bool CTsParser::parse (void)
{
	// check unit size
	size_t unit_size = 0;
	size_t skip_bytes = 512;
	int err_cnt = 0;
	while (1) {
		unit_size = getUnitSize();
		if (unit_size >= 0 && unit_size < TS_PACKET_LEN) {
			if (err_cnt < 20) {
				_UTL_LOG_W ("invalid unit_size=[%d]", unit_size);
			}

			if ((mp_current + skip_bytes) < mp_bottom) {
				// とりあえず　skip_bytes 分すすめてみます
				mp_current += skip_bytes;
				++ err_cnt;
				continue;

			} else {
				m_parse_remain_len = 0;
				return false;
			}

		} else {
			break;
		}
	}
	err_cnt = 0;


	TS_HEADER ts_header = {0};
	uint8_t *p = NULL; //work
	uint8_t *p_payload = NULL;
	uint8_t *p_cur = mp_current;
	uint8_t *p_btm = mp_bottom;
	size_t payload_size = 0;

	while ((p_cur + unit_size) < p_btm) {
		if ((*p_cur != SYNC_BYTE) && (*(p_cur + unit_size) != SYNC_BYTE)) {
			p = getSyncTopAddr (p_cur, p_btm, unit_size);
			if (!p) {
				if (err_cnt < 20) {
					_UTL_LOG_W ("invalid getSyncTopAddr (%p-%p) unit_size=[%d]", p_cur, p_btm, unit_size);
				}
				p_cur += unit_size;
				++ err_cnt;
				continue;
			}

			// sync update
			p_cur = p;
		}

		ts_header.parse (p_cur);

		p_payload = p_cur + TS_HEADER_LEN;

		// adaptation_field_control 2bit
		// 00 ISO/IECによる将来の使用のために予約されている。
		// 01 アダプテーションフィールドなし、ペイロードのみ
		// 10 アダプテーションフィールドのみ、ペイロードなし
		// 11 アダプテーションフィールドの次にペイロード
		if ((ts_header.adaptation_field_control & 0x02) == 0x02) {
			// アダプテーションフィールド付き
			p_payload += *p_payload + 1; // lengthとそれ自身の1byte分進める
		}

		// TTS(Timestamped TS)(total192bytes) や
		// FEC(Forward Error Correction:順方向誤り訂正)(total204bytes)
		// は除外します
//		payload_size = TS_PACKET_LEN - (p_payload - p_cur);
		payload_size = TS_PACKET_LEN - TS_HEADER_LEN;


		if (mp_listener) {
			mp_listener->onTsPacketAvailable (&ts_header, p_payload, payload_size);
		}

		p_cur += unit_size;
	}

	m_parse_remain_len = p_btm - p_cur;
	_UTL_LOG_D ("m_parse_remain_len=[%d]\n", m_parse_remain_len);


	return true;
}
