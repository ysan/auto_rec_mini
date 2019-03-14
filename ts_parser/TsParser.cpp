#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TsParser.h"
#include "Utils.h"


CTsParser::CTsParser (void)
	:mp_top (NULL)
	,mp_current (NULL)
	,mp_bottom (NULL)
	,m_buff_size (0)
	,m_unit_size (0)
	,m_parse_remain_len (0)
	,mp_listener (NULL)
{
	memset (m_inner_buff, 0x00, INNER_BUFF_SIZE);
}

CTsParser::CTsParser (IParserListener *p_listener)
	:mp_top (NULL)
	,mp_current (NULL)
	,mp_bottom (NULL)
	,m_buff_size (0)
	,m_unit_size (0)
	,m_parse_remain_len (0)
	,mp_listener (NULL)
{
	if (p_listener) {
		mp_listener = p_listener;
	}

	memset (m_inner_buff, 0x00, INNER_BUFF_SIZE);
}

CTsParser::~CTsParser (void)
{
}

void CTsParser::run (uint8_t *pBuff, size_t size)
{
	if ((!pBuff) || (size == 0)) {
		return ;
	}


#if 0
	allocInnerBuffer (pBuff, size);
	checkUnitSize ();
	parse ();

#else
	mp_top = &m_inner_buff[0];
	if (copyInnerBuffer (pBuff, size)) {

		mp_current = mp_top;
		checkUnitSize ();
		parse ();

		if (m_parse_remain_len > 0) {
			memcpy (m_inner_buff, mp_bottom - m_parse_remain_len, m_parse_remain_len);
			mp_bottom = &m_inner_buff[0] + m_parse_remain_len;
		} else {
			memset (m_inner_buff, 0x00, INNER_BUFF_SIZE);
			mp_bottom = &m_inner_buff[0];
		}

		if (copyInnerBuffer (pBuff, size)) {
			_UTL_LOG_E ("invalid copyInnerBuffer");
			return;
		}

	}
#endif

}

//TODO 足らなくなったら拡張 全部貯めるかたち
bool CTsParser::allocInnerBuffer (uint8_t *pBuff, size_t size)
{
	if ((!pBuff) || (size == 0)) {
		return false;
	}

	size_t totalDataSize = (mp_bottom - mp_top) + size;

	if (m_buff_size >= totalDataSize) {
		// 	実コピー
		memcpy (mp_bottom, pBuff, size);
		mp_current = mp_bottom - m_parse_remain_len;  // current update
		mp_bottom += size;
		_UTL_LOG_D ("copyInnerBuffer size=%lu remain=%lu\n", m_buff_size, m_buff_size-totalDataSize);
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
	memcpy (mp_bottom, pBuff, size);
	mp_current = mp_bottom - m_parse_remain_len;  // current update
	mp_bottom += size;

	_UTL_LOG_D ("copyInnerBuffer(malloc) top=%p bottom=%p size=%lu remain=%lu\n", mp_top, mp_bottom, m_buff_size, m_buff_size-totalDataSize);
	return true;
}

// 静的領域にコピー
bool CTsParser::copyInnerBuffer (uint8_t *pBuff, size_t size)
{
	if ((!pBuff) || (size == 0)) {
		return false;
	}

	if (size > INNER_BUFF_SIZE) {
		_UTL_LOG_E ("copyInnerBuffer: size > INNER_BUFF_SIZE\n");
		return false;
	}

	int remain = 0;
	if (mp_bottom) {
		remain = INNER_BUFF_SIZE - (mp_bottom - mp_top);

		if (remain >= (int)size) {
			memcpy (m_inner_buff + (mp_bottom - mp_top), pBuff, size);
			mp_bottom = mp_bottom + size;
			return false;

		} else {
			// do parse
			return true;
		}

	} else {
		remain = INNER_BUFF_SIZE;

		memcpy (m_inner_buff , pBuff, size);
		mp_bottom = mp_top + size;
		return false;
	}

	return true;
}

bool CTsParser::checkUnitSize (void)
{
	int i;
	int m;
	int n;
//	int w;
	int count [320-188];

	uint8_t *p_cur;

	p_cur = mp_current;
	memset (count, 0x00, sizeof(count));

	while ((p_cur+188) < mp_bottom) {
		if (*p_cur != SYNC_BYTE) {
			p_cur ++;
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

		p_cur ++;

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

	m_unit_size = n;
	_UTL_LOG_D ("m_unit_size %d\n", n);

	return true;
}

uint8_t * CTsParser::getSyncTopAddr (uint8_t *pTop, uint8_t *p_btm, size_t unit_size) const
{
	if ((!pTop) || (!p_btm) || (unit_size == 0)) {
		return NULL;
	}

	int i;
	uint8_t *pWork = NULL;

	pWork = pTop;
	p_btm -= unit_size * 8; // unit_size 8コ分のデータがある前提

	while (pWork <= p_btm) {
		if (*pWork == SYNC_BYTE) {
			for (i = 0; i < 8; ++ i) {
				if (*(pWork+(unit_size*(i+1))) != SYNC_BYTE) {
					break;
				}
			}
			// 以降 8回全てSYNC_BYTEで一致したら pWorkは先頭だ
			if (i == 8) {
				return pWork;
			}
		}
		pWork ++;
	}

	return NULL;
}

/*
void CTsParser::getTsHeader (TS_HEADER *pDst, uint8_t* pSrc) const
{
	if ((!pSrc) || (!pDst)) {
		return;
	}

	pDst->sync                         =   *(pSrc+0);
	pDst->transport_error_indicator    =  (*(pSrc+1) >> 7) & 0x01;
	pDst->payload_unit_start_indicator =  (*(pSrc+1) >> 6) & 0x01;
	pDst->transport_priority           =  (*(pSrc+1) >> 5) & 0x01;
	pDst->pid                          = ((*(pSrc+1) & 0x1f) << 8) | *(pSrc+2);
	pDst->transport_scrambling_control =  (*(pSrc+3) >> 6) & 0x03;
	pDst->adaptation_field_control     =  (*(pSrc+3) >> 4) & 0x03;
	pDst->continuity_counter           =   *(pSrc+3)       & 0x0f;
}

void CTsParser::dumpTsHeader (const TS_HEADER *p) const
{
	if (!p) {
		return ;
	}
	_UTL_LOG_I (
		"TsHeader: sync:[0x%02x] trans_err:[0x%02x] start_ind:[0x%02x] prio:[0x%02x] pid:[0x%04x] scram:[0x%02x] adap:[0x%02x] cont:[0x%02x]\n",
		p->sync,
		p->transport_error_indicator,
		p->payload_unit_start_indicator,
		p->transport_priority,
		p->pid,
		p->transport_scrambling_control,
		p->adaptation_field_control,
		p->continuity_counter
	);
}
*/

bool CTsParser::parse (void)
{
	TS_HEADER ts_header = {0};
	uint8_t *p = NULL; //work
	uint8_t *p_payload = NULL;
	uint8_t *p_cur = mp_current;
	uint8_t *p_btm = mp_bottom;
	size_t unit_size = m_unit_size;
	size_t payload_size = 0;


	while ((p_cur+unit_size) < p_btm) {
		if ((*p_cur != SYNC_BYTE) && (*(p_cur+unit_size) != SYNC_BYTE)) {
			p = getSyncTopAddr (p_cur, p_btm, unit_size);
			if (!p) {
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
		payload_size = TS_PACKET_LEN - (p_payload - p_cur);


		if (mp_listener) {
			mp_listener->onTsPacketAvailable (&ts_header, p_payload, payload_size);
		}


		p_cur += unit_size;
	}

	m_parse_remain_len = p_btm - p_cur;
	_UTL_LOG_D ("m_parse_remain_len=[%d]\n", m_parse_remain_len);


	return true;
}
