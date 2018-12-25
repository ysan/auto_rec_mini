#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TsParser.h"
#include "Utils.h"


CTsParser::CTsParser (void)
	:mpTop (NULL)
	,mpCurrent (NULL)
	,mpBottom (NULL)
	,mBuffSize (0)
	,mUnitSize (0)
	,mParseRemainSize (0)
	,mTOT (5)
	,mEIT_H (65535)
	,mEIT_M (65535)
	,mEIT_L (65535)
{
}

CTsParser::~CTsParser (void)
{
}

void CTsParser::run (uint8_t *pBuff, size_t size)
{
	if ((!pBuff) || (size == 0)) {
		return ;
	}

	copyInnerBuffer (pBuff, size);
	checkUnitSize ();

	parse ();
}

//TODO 全部ため込むかたち
bool CTsParser::copyInnerBuffer (uint8_t *pBuff, size_t size)
{
	if ((!pBuff) || (size == 0)) {
		return false;
	}

	size_t totalDataSize = (mpBottom - mpTop) + size;

	if (mBuffSize >= totalDataSize) {
		// 	実コピー
		memcpy (mpBottom, pBuff, size);
		mpCurrent = mpBottom - mParseRemainSize;  // current update
		mpBottom += size;
		_UTL_LOG_I ("copyInnerBuffer size=%lu remain=%lu\n", mBuffSize, mBuffSize-totalDataSize);
		return true;
	}

	// 残りサイズ足りないので確保 初回もここに

	size_t n = 512;
	while (n < (size + mBuffSize)) {
		n += n;
	}

	uint8_t *p = (uint8_t*) malloc (n);
	if (!p) {
		return false;
	}
	memset (p, 0x00, n);


	int m = 0;
	if (mpTop != NULL) {
		// 既にデータ入っている場合はそれをコピーして 元は捨てます
		m = mpBottom - mpTop;
		if (m > 0) {
			memcpy(p, mpTop, m);
		}
		free (mpTop);
		mpTop = NULL;
    }
	mpTop = p;
	mpBottom = p + m;
    mBuffSize = n;


	// 	実コピー
	memcpy (mpBottom, pBuff, size);
	mpCurrent = mpBottom - mParseRemainSize;  // current update
	mpBottom += size;

	_UTL_LOG_I ("copyInnerBuffer(malloc) top=%p bottom=%p size=%lu remain=%lu\n", mpTop, mpBottom, mBuffSize, mBuffSize-totalDataSize);
	return true;
}

bool CTsParser::checkUnitSize (void)
{
	int i;
	int m;
	int n;
//	int w;
	int count [320-188];

	uint8_t *pCur;

	pCur = mpCurrent;
	memset (count, 0x00, sizeof(count));

	while ((pCur+188) < mpBottom) {
		if (*pCur != SYNC_BYTE) {
			pCur ++;
			continue;
		}

		m = 320;
		if ((pCur+m) > mpBottom) {
			// mを 320満たないのでまでにbottomに切り詰め
			m = mpBottom - pCur;
		}
//printf("%p m=%d ", pCur, m);

		for (i = 188; i < m; i ++) {
			if(*(pCur+i) == SYNC_BYTE){
				// 今見つかったSYNC_BYTEから以降の(320-188)byte間に
				// SYNC_BYTEがあったら カウント
				count [i-188] += 1;
			}
		}

		pCur ++;

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
	_UTL_LOG_I ("max_interval count m=%d  max_interval n=%d\n", m, n);

	//TODO
//	w = m*n;
//	if ((m < 8) || ((w+3*n) < (mpBottom-mpCurrent))) {
//		return false;
//	}

	mUnitSize = n;
	_UTL_LOG_I ("mUnitSize %d\n", n);

	return true;
}

uint8_t * CTsParser::getSyncTopAddr (uint8_t *pTop, uint8_t *pBtm, size_t unitSize) const
{
	if ((!pTop) || (!pBtm) || (unitSize == 0)) {
		return NULL;
	}

	int i;
	uint8_t *pWork = NULL;

	pWork = pTop;
	pBtm -= unitSize * 8; // unitSize 8コ分のデータがある前提

	while (pWork <= pBtm) {
		if (*pWork == SYNC_BYTE) {
			for (i = 0; i < 8; ++ i) {
				if (*(pWork+(unitSize*(i+1))) != SYNC_BYTE) {
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

void CTsParser::getTsHeader (ST_TS_HEADER *pDst, uint8_t* pSrc) const
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

void CTsParser::dumpTsHeader (const ST_TS_HEADER *p) const
{
	if (!p) {
		return ;
	}
	_UTL_LOG_I (
		"TsHeader: sync:0x%02x trans_err:0x%02x start_ind:0x%02x prio:0x%02x pid:0x%04x scram:0x%02x adap:0x%02x cont:0x%02x\n",
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

bool CTsParser::parse (void)
{
	ST_TS_HEADER stTsHdr = {0};
	uint8_t *p = NULL; //work
	uint8_t *pPayload = NULL;
	uint8_t *pCur = mpCurrent;
	uint8_t *pBtm = mpBottom;
	size_t unitSize = mUnitSize;
	size_t payloadSize = 0;
	bool isCheck = false;
	CProgramAssociationTable::CTable *pCurPatTable = NULL;
	CDsmccControl *pCurDsmccCtl = NULL;



	while ((pCur+unitSize) < pBtm) {
		if ((*pCur != SYNC_BYTE) && (*(pCur+unitSize) != SYNC_BYTE)) {
//printf ("getSyncTopAddr\n");
			p = getSyncTopAddr (pCur, pBtm, unitSize);
			if (!p) {
				return false;
			}

			// sync update
			pCur = p;
		}

		getTsHeader (&stTsHdr, pCur);

		switch (stTsHdr.pid) {
		case PID_PAT:
			_UTL_LOG_I ("###############  PAT  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_TOT:
			_UTL_LOG_I ("###############  TOT  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);

			isCheck = true;
			break;

		case PID_EIT_H:
			_UTL_LOG_I ("###############  EIT_H  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_EIT_M:
			_UTL_LOG_I ("###############  EIT_M  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_EIT_L:
			_UTL_LOG_I ("###############  EIT_L  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_NIT:
			_UTL_LOG_I ("###############  NIT  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_SDT:
			_UTL_LOG_I ("###############  SDT  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_RST:
			_UTL_LOG_I ("###############  RST  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		case PID_BIT:
			_UTL_LOG_I ("###############  BIT  ###############");
			CUtils::dumper (pCur, 188);
			dumpTsHeader (&stTsHdr);
			isCheck = true;
			break;

		default:
			// check PMT
			pCurPatTable = &mPatTables [0];
			for (int i = 0; i < 32; ++ i) {
				if (!pCurPatTable->isUsed) {
					continue;
				}
				if (pCurPatTable->program_number != 0) {
					if (stTsHdr.pid == pCurPatTable->program_map_PID) {
						_UTL_LOG_I ("###############  PMT  ###############");
						CUtils::dumper (pCur, 188);
						dumpTsHeader (&stTsHdr);
						isCheck = true;
						break;
					}
				}
				++ pCurPatTable ;
			}

			// check DSMCC
			pCurDsmccCtl = &mDsmccCtls [0];
			for (int i = 0; i < 256; ++ i) {
				if (!pCurDsmccCtl->isUsed) {
					continue;
				}
				if (stTsHdr.pid == pCurDsmccCtl->pid) {
					_UTL_LOG_I ("###############  DSMCC  ###############");
					CUtils::dumper (pCur, 188);
					dumpTsHeader (&stTsHdr);
					isCheck = true;
					break;
				}
				++ pCurDsmccCtl;
			}


			break;
		}

		pPayload = pCur + TS_HEADER_LEN;

		// adaptation_field_control 2bit
		// 00 ISO/IECによる将来の使用のために予約されている。
		// 01 アダプテーションフィールドなし、ペイロードのみ
		// 10 アダプテーションフィールドのみ、ペイロードなし
		// 11 アダプテーションフィールドの次にペイロード
		if ((stTsHdr.adaptation_field_control & 0x02) == 0x02) {
			// アダプテーションフィールド付き
			pPayload += *pPayload + 1; // lengthとそれ自身の1byte分進める
		}

		// TTS(Timestamped TS)(total192bytes) や
		// FEC(Forward Error Correction:順方向誤り訂正)(total204bytes)
		// は除外します
		payloadSize = TS_PACKET_LEN - (pPayload - pCur);


// check table_id
//if (stTsHdr.payload_unit_start_indicator == 1) {
//	uint8_t table_id = *pPayload;
//	_UTL_LOG_I ("dddddddddddddd   table_id 0x%02x   pid 0x%04x", table_id, stTsHdr.pid);
//}


		if (isCheck) {

			if (stTsHdr.pid == PID_PAT) {

				if (mPAT.checkSection (&stTsHdr, pPayload, payloadSize) == EN_CHECK_SECTION__COMPLETED) {

					memset (mPatTables, 0x00, sizeof(mPatTables));

					int n = mPAT.getTableNum ();
					mPAT.getTable (mPatTables, 32);
					mPAT.dumpTable (mPatTables, n);
				}

			} else if (stTsHdr.pid == PID_TOT) {

				mTOT.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_EIT_H) {

				mEIT_H.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_EIT_M) {

				mEIT_M.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_EIT_L) {

				mEIT_L.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_NIT) {

				mNIT.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_SDT) {

				mSDT.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_RST) {

				mRST.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == PID_BIT) {

				mBIT.checkSection (&stTsHdr, pPayload, payloadSize);

			} else if (stTsHdr.pid == pCurPatTable->program_map_PID) {

				if (pCurPatTable->mpPMT) {
					if (pCurPatTable->mpPMT->checkSection (&stTsHdr, pPayload, payloadSize) == EN_CHECK_SECTION__COMPLETED) {

						// stream_typeからDSMCCのPIDを取得する //////////
						const std::vector<CProgramMapTable::CTable*> *pTables = pCurPatTable->mpPMT->getTables();
						std::vector<CProgramMapTable::CTable*>::const_iterator iter = pTables->begin();
						for (; iter != pTables->end(); ++ iter) {
							CProgramMapTable::CTable *pTable = *iter;
							std::vector<CProgramMapTable::CTable::CStream>::const_iterator iter_strm = pTable->streams.begin();
							for (; iter_strm != pTable->streams.end(); ++ iter_strm) {
								if ((iter_strm->stream_type == 0x0b) || (iter_strm->stream_type == 0x0d)) {
									bool isExisted = false;
									for (int i = 0; i < 256; ++ i) {
										if (mDsmccCtls[i].isUsed && (mDsmccCtls[i].pid == iter_strm->elementary_PID)) {
											isExisted = true;
											break;
										}
									}
									if (!isExisted) {
										int i = 0;
										for (i = 0; i < 256; ++ i) {
											if (!mDsmccCtls[i].isUsed) {
												mDsmccCtls[i].pid = iter_strm->elementary_PID;
												mDsmccCtls[i].mpDsmcc = new CDsmcc (65535);
												mDsmccCtls[i].isUsed = true;
												break;
											}
										}
										if (i == 256) {
											_UTL_LOG_W ("mDsmccCtls is full.");
										}
									}
								}
							}

						}
						//////////////////////////////////////////////////

					}
				}

			} else if (stTsHdr.pid == pCurDsmccCtl->pid) {
				if (pCurDsmccCtl->mpDsmcc) {
					pCurDsmccCtl->mpDsmcc->checkSection (&stTsHdr, pPayload, payloadSize);
				}
			}

			isCheck = false;
		}



		pCur += unitSize;
	}

	mParseRemainSize = pBtm - pCur;
	_UTL_LOG_I ("mParseRemainSize=[%d]\n", mParseRemainSize);


	return true;
}
