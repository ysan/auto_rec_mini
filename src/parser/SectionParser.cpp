#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "SectionParser.h"
#include "Utils.h"


// debug
static const char *s_szSectState [EN_SECTION_STATE_NUM] = {
	"INIT",
	"RECEIVING",
	"RECEIVED",
	"COMPLETE",
};

CSectionInfo::CSectionInfo (uint8_t *pBuff, size_t size, EN_SECTION_TYPE type)
	:mpRaw (NULL)
	,mpData (NULL)
	,mpTail (NULL)
	,mState (EN_SECTION_STATE__INIT)
	,mType (EN_SECTION_TYPE__PSISI)
	,mpNext (NULL)
{
	memset (&mSectHdr, 0x00, sizeof(mSectHdr));

	if (pBuff && (size > 0)) {
		mpRaw = pBuff;
		mpTail = pBuff + size;
		parseHeader ();
	}

	mType = type;
}

CSectionInfo::CSectionInfo (CSectionInfo *pObj)
	:mpRaw (NULL)
	,mpData (NULL)
	,mpTail (NULL)
	,mState (EN_SECTION_STATE__INIT)
	,mType (EN_SECTION_TYPE__PSISI)
	,mpNext (NULL)
{
	if (!pObj) {
		return;
	}

	memcpy (&mSectHdr, &(pObj->mSectHdr), sizeof(mSectHdr));
	mpRaw = pObj->mpRaw;
	mpData = pObj->mpData;
	mpTail = pObj->mpTail;
	mState = pObj->mState;
	mpNext = pObj->mpNext;
	mType = pObj->mType;
}

CSectionInfo::~CSectionInfo (void)
{
}

/**
 *
 */
uint8_t *CSectionInfo::getHeaderAddr (void) const
{
	return mpRaw;
}

/**
 *
 */
const ST_SECTION_HEADER *CSectionInfo::getHeader (void) const
{
	return &mSectHdr;
}

/**
 *
 */
uint8_t *CSectionInfo::getDataPartAddr (void) const
{
	return mpData;
}

/**
 *
 */
uint16_t CSectionInfo::getDataPartLen (void) const
{
	if (mType == EN_SECTION_TYPE__PSISI) {
		if (mSectHdr.section_syntax_indicator == 0) {
			// short format section header
			return mSectHdr.section_length - SECTION_CRC32_LEN;
		} else {
			return mSectHdr.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN;
		}

	} else if (mType == EN_SECTION_TYPE__DSMCC) {
		return mSectHdr.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN;

	} else {
		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
		return 0;
	}
}

/**
 *
 */
void CSectionInfo::parseHeader (void)
{
	if (!mpRaw || !mpTail) {
		return;
	}

	mpData = parseHeader (&mSectHdr, mpRaw, mpTail);
}

/**
 *
 */
uint8_t * CSectionInfo::parseHeader (ST_SECTION_HEADER *pHdrOut, uint8_t *pStart, uint8_t *pEnd, EN_SECTION_TYPE enType) 
{
	if (!pHdrOut || !pStart || !pEnd) {
		return NULL;
	}

	int size = 0;
	uint8_t *p = NULL; //work

	size = pEnd - pStart;
	if (size < SECTION_SHORT_HEADER_LEN) {
		// need more data
		return NULL;
	}

	p = pStart;
	pHdrOut->table_id                 =   *(p+0);
	pHdrOut->section_syntax_indicator =  (*(p+1) >> 7) & 0x01;
	pHdrOut->private_indicator        =  (*(p+1) >> 6) & 0x01;
	pHdrOut->section_length           = ((*(p+1) << 8) | *(p+2)) & 0x0fff;

	if (enType == EN_SECTION_TYPE__PSISI) {
		if (pHdrOut->section_syntax_indicator == 0) {
			// short format section header
			return p + SECTION_SHORT_HEADER_LEN;

		} else {
			// long format section header

			if (size < SECTION_HEADER_LEN) {
				// need more data
				return NULL;
			}

			pHdrOut->table_id_extension     = ((*(p+3) << 8) | *(p+4));
			pHdrOut->version_number         =  (*(p+5) >> 1) & 0x1f;
			pHdrOut->current_next_indicator =   *(p+5)       & 0x01;
			pHdrOut->section_number         =   *(p+6);
			pHdrOut->last_section_number    =   *(p+7);

			return p + SECTION_HEADER_LEN;
		}

	} else if (enType == EN_SECTION_TYPE__DSMCC) {
		// long format section header

		if (size < SECTION_HEADER_LEN) {
			// need more data
			return NULL;
		}

		pHdrOut->table_id_extension     = ((*(p+3) << 8) | *(p+4));
		pHdrOut->version_number         =  (*(p+5) >> 1) & 0x1f;
		pHdrOut->current_next_indicator =   *(p+5)       & 0x01;
		pHdrOut->section_number         =   *(p+6);
		pHdrOut->last_section_number    =   *(p+7);

		return p + SECTION_HEADER_LEN;

	} else {
		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
		return NULL;
	}
}

/**
 *
 */
void CSectionInfo::dumpHeader (void) const
{
	if (mType == EN_SECTION_TYPE__PSISI) {
		dumpHeader (&mSectHdr, (mSectHdr.section_syntax_indicator == 0) ? true : false);

	} else if (mType == EN_SECTION_TYPE__DSMCC) {
		dumpHeader (&mSectHdr, false);

	} else {
		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
	}
}

/**
 *
 */
void CSectionInfo::dumpHeader (const ST_SECTION_HEADER *pHdr, bool isShortFormat) 
{
	if (!pHdr) {
		return ;
	}

	if (isShortFormat) {
		_UTL_LOG_I (
			"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d]\n",
			pHdr->table_id,
			pHdr->section_syntax_indicator,
			pHdr->private_indicator,
			pHdr->section_length
		);
	} else {
		_UTL_LOG_I (
			"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d] tbl_ext:[0x%04x] ver:[0x%02x] next:[0x%02x] num:[0x%02x] last:[0x%02x]\n",
			pHdr->table_id,
			pHdr->section_syntax_indicator,
			pHdr->private_indicator,
			pHdr->section_length,
			pHdr->table_id_extension,
			pHdr->version_number,
			pHdr->current_next_indicator,
			pHdr->section_number,
			pHdr->last_section_number
		);
	}
}

/**
 * isReceiveAll
 * 当該sectionがすべて受け取れたかどうか
 */
bool CSectionInfo::isReceiveAll (void) const
{
	if (!mpRaw || !mpTail) {
		return false;
	}

	// 右辺はheader含めたsectionの合計
	if ((mpTail - mpRaw) < (mSectHdr.section_length + SECTION_SHORT_HEADER_LEN)) {
		_UTL_LOG_D ("section fragment\n");
		return false;
	}

	return true;
}

/**
 * truncate
 */
size_t CSectionInfo::truncate (void)
{
	if (!isReceiveAll()) {
		_UTL_LOG_E ("can truncate only if isReceiveAll is true");
		return 0;
	}
	size_t len = (mpTail - mpRaw) - (mSectHdr.section_length + SECTION_SHORT_HEADER_LEN);
	mpTail -= len;
	return len;
}

/**
 * clear
 */
void CSectionInfo::clear (void)
{
	memset (&mSectHdr, 0x00, sizeof(ST_SECTION_HEADER));
	mType = EN_SECTION_TYPE__PSISI;
	mState = EN_SECTION_STATE__INIT;
	mpRaw = NULL;
	mpData = NULL;
	mpTail = NULL;
	mpNext = NULL;
}

/**
 *
 */
bool CSectionInfo::checkCRC32 (void) const
{
	return CTsAribCommon::crc32(mpRaw, mSectHdr.section_length + SECTION_SHORT_HEADER_LEN) == 0 ? true : false;
}


const uint32_t CSectionParser::POOL_SIZE = 4096;

CSectionParser::CSectionParser (EN_SECTION_TYPE type)
	:mPid (0)
	,mpSectListTop(NULL)
	,mpSectListBottom (NULL)
	,mListIsFifo (false)
	,mFifoNum (0)
	,mpWorkSectInfo (NULL)
	,mpPool (NULL)
	,mPoolInd (0)
	,mPoolSize (POOL_SIZE)
	,mType (EN_SECTION_TYPE__PSISI)
	,mIsAsyncDelete (false)
	,mIsIgnoreSection (false)
{
	mpPool = new uint8_t [mPoolSize];
	memset (mpPool, 0x00, mPoolSize);
	mType = type;
}

CSectionParser::CSectionParser (int fifoNum, EN_SECTION_TYPE type)
	:mPid (0)
	,mpSectListTop(NULL)
	,mpSectListBottom (NULL)
	,mListIsFifo (false)
	,mFifoNum (0)
	,mpWorkSectInfo (NULL)
	,mpPool (NULL)
	,mPoolInd (0)
	,mPoolSize (POOL_SIZE)
	,mType (EN_SECTION_TYPE__PSISI)
	,mIsAsyncDelete (false)
	,mIsIgnoreSection (false)
{
	mpPool = new uint8_t [mPoolSize];
	memset (mpPool, 0x00, mPoolSize);

	if (fifoNum > 0) {
		mListIsFifo = true;
		mFifoNum = fifoNum;
	}

	mType = type;
}

CSectionParser::CSectionParser (size_t poolSize, EN_SECTION_TYPE type)
	:mPid (0)
	,mpSectListTop(NULL)
	,mpSectListBottom (NULL)
	,mListIsFifo (false)
	,mFifoNum (0)
	,mpWorkSectInfo (NULL)
	,mpPool (NULL)
	,mPoolInd (0)
	,mPoolSize (POOL_SIZE)
	,mType (EN_SECTION_TYPE__PSISI)
	,mIsAsyncDelete (false)
	,mIsIgnoreSection (false)
{
	mPoolSize = poolSize;
	mpPool = new uint8_t [mPoolSize];
	memset (mpPool, 0x00, mPoolSize);
	mType = type;
}

CSectionParser::CSectionParser (size_t poolSize ,int fifoNum, EN_SECTION_TYPE type)
	:mPid (0)
	,mpSectListTop(NULL)
	,mpSectListBottom (NULL)
	,mListIsFifo (false)
	,mFifoNum (0)
	,mpWorkSectInfo (NULL)
	,mpPool (NULL)
	,mPoolInd (0)
	,mPoolSize (POOL_SIZE)
	,mType (EN_SECTION_TYPE__PSISI)
	,mIsAsyncDelete (false)
	,mIsIgnoreSection (false)
{
	mPoolSize = poolSize;
	mpPool = new uint8_t [mPoolSize];
	memset (mpPool, 0x00, mPoolSize);

	if (fifoNum > 0) {
		mListIsFifo = true;
		mFifoNum = fifoNum;
	}

	mType = type;
}

CSectionParser::~CSectionParser (void)
{
	delete [] mpPool;
	mpPool = NULL;
}

/**
 *
 */
CSectionInfo* CSectionParser::attachSectionList (uint8_t *pBuff, size_t size)
{
	if (!pBuff || (size == 0)) {
		return NULL;
	}

	CSectionInfo* pRtn = NULL;

	size_t remain = mPoolSize - mPoolInd;
	if (remain < size) {
		// pool full
		_UTL_LOG_E ("pid:[0x%04x] pool full!!  mPoolSize:[%lu] mPoolInd:[%lu] remain:[%lu] attachSize:[%lu]", mPid, mPoolSize, mPoolInd, remain, size);
		return NULL;
	}


	uint8_t* pCur = mpPool + mPoolInd;

	// sectionの生データをpoolにコピー
	memcpy (pCur, pBuff, size);
	mPoolInd += size;


	if (mpWorkSectInfo && (mpWorkSectInfo->mState == EN_SECTION_STATE__RECEIVING)) {
		// follow
		if (pCur != mpWorkSectInfo->mpTail) {
			_UTL_LOG_E ("BUG: pid:[0x%04x] (pCur != mpWorkSectInfo->mpTail)", mPid);
		}
		mpWorkSectInfo->mpTail = pCur + size;
		pRtn = mpWorkSectInfo;

	} else {
		// first
		CSectionInfo tmpSect (pCur, size, mType);

		CSectionInfo* pList = addSectionList (&tmpSect);
		if (!pList) {

			// cancel pool data
//TODO memcpy-param-overlap
//			memcpy (pCur, mpPool + mPoolInd, (mpPool + (mPoolSize -1)) - (mpPool + mPoolInd));
			uint8_t tmp [mPoolSize] = {0};
			size_t cpsize = (mpPool + mPoolSize) - (mpPool + mPoolInd);
			memcpy (tmp, mpPool + mPoolInd, cpsize);
			memcpy (pCur, tmp, cpsize);

			mPoolInd -= size;

			pRtn = NULL;

		} else {
			pRtn = pList;
		}
	}

	return pRtn;
}

/**
 *
 */
CSectionInfo* CSectionParser::addSectionList (CSectionInfo *pSectInfo)
{
	if (!pSectInfo) {
		return NULL;
	}


	CSectionInfo *pTmp = new CSectionInfo (pSectInfo);
	pTmp->mpNext = NULL;

	if (!mpSectListTop) {
		// リストが空
		mpSectListTop = pTmp;
		mpSectListBottom = pTmp;

	} else {
		// 末尾につなぎます
		mpSectListBottom->mpNext = pTmp;
		mpSectListBottom = pTmp;
	}

	return pTmp;
}

/**
 * detachSectionList
 * 指定のセクションを削除します
 *
 * TODO 
 * これを呼ぶところは今取得中のセクションで最後尾にあたるので
 * mpRaw, mpData, mpTailのshiftは不要とします
 */
void CSectionParser::detachSectionList (const CSectionInfo *pSectInfo)
{
	if (!pSectInfo) {
		return ;
	}


	deleteSectionList (pSectInfo, false);

	// pool から消す - 間詰める
//TODO memcpy-param-overlap
//	memcpy (pSectInfo->mpRaw, pSectInfo->mpTail, mpPool + (mPoolSize -1) - pSectInfo->mpTail);
	uint8_t tmp [mPoolSize] = {0};
	size_t cpsize = mpPool + mPoolSize - pSectInfo->mpTail;
	memcpy (tmp, pSectInfo->mpTail, cpsize);
	memcpy (pSectInfo->mpRaw, tmp, cpsize);

	size_t shift = pSectInfo->mpTail - pSectInfo->mpRaw;
	mPoolInd -= shift;

	// pool 消す時はセットで
	if (pSectInfo == mpWorkSectInfo) {
// deleteSectionList内で実行済
//		mpWorkSectInfo->clear ();
		mpWorkSectInfo = NULL;
	}

	delete pSectInfo;
}

/**
 * deleteSectionList
 * インスタンスの一致で削除
 * 重複していないこと前提
 */
void CSectionParser::deleteSectionList (const CSectionInfo &sectInfo, bool isDelete)
{
	CSectionInfo *pTmp = NULL;
	CSectionInfo *pBef = NULL;
	CSectionInfo *pDel = NULL;

	if (!mpSectListTop) {
		// リストが空

	} else {
		pTmp = mpSectListTop;

		while (pTmp) {
			if (*pTmp == sectInfo) {
				// 一致

				// 後でfreeするため保存
				pDel = pTmp;

				if (pTmp == mpSectListTop) {
					// topの場合

					if (pTmp->mpNext) {
						mpSectListTop = pTmp->mpNext;
						pTmp->mpNext = NULL;
					} else {
						// リストが１つ -> リストが空になるはず
						mpSectListTop = NULL;
						mpSectListBottom = NULL;
						pTmp->mpNext = NULL;
					}

				} else if (pTmp == mpSectListBottom) {
					// bottomの場合

					if (pBef) {
						// 切り離します
						pBef->mpNext = NULL;
						mpSectListBottom = pBef;
					}

				} else {
					// 切り離します
					pBef->mpNext = pTmp->mpNext;
					pTmp->mpNext = NULL;
				}

				if (isDelete) {
					_UTL_LOG_D ("delete");
					pDel->clear();
					delete pDel;
					pDel = NULL;
				}

				break;

			} else {
				// ひとつ前のアドレス保存
				pBef = pTmp;
			}

			// next set
			pTmp = pTmp->mpNext;
		}
	}
}

/**
 * deleteSectionList
 * アドレスの一致で削除
 * 重複していないこと前提
 */
void CSectionParser::deleteSectionList (const CSectionInfo *pSectInfo, bool isDelete)
{
	if (!pSectInfo) {
		return ;
	}

	CSectionInfo *pTmp = NULL;
	CSectionInfo *pBef = NULL;
	CSectionInfo *pDel = NULL;

	if (!mpSectListTop) {
		// リストが空

	} else {
		pTmp = mpSectListTop;

		while (pTmp) {
			if (pSectInfo == pTmp) {
				// 一致

				// 後でfreeするため保存
				pDel = pTmp;

				if (pTmp == mpSectListTop) {
					// topの場合

					if (pTmp->mpNext) {
						mpSectListTop = pTmp->mpNext;
						pTmp->mpNext = NULL;
					} else {
						// リストが１つ -> リストが空になるはず
						mpSectListTop = NULL;
						mpSectListBottom = NULL;
						pTmp->mpNext = NULL;
					}

				} else if (pTmp == mpSectListBottom) {
					// bottomの場合

					if (pBef) {
						// 切り離します
						pBef->mpNext = NULL;
						mpSectListBottom = pBef;
					}

				} else {
					// 切り離します
					pBef->mpNext = pTmp->mpNext;
					pTmp->mpNext = NULL;
				}

				if (isDelete) {
					_UTL_LOG_D ("delete");
					pDel->clear();
					delete pDel;
					pDel = NULL;
				}

				break;

			} else {
				// ひとつ前のアドレス保存
				pBef = pTmp;
			}

			// next set
			pTmp = pTmp->mpNext;
		}
	}
}

/**
 *
 */
void CSectionParser::detachAllSectionList (void)
{
	deleteAllSectionList ();

	if (mpPool) {
		memset (mpPool, 0x00, mPoolSize);
		mPoolInd = 0;
	}

	// pool 消す時はセットで
	if (mpWorkSectInfo) {
// deleteAllSectionList内で実行済
//		mpWorkSectInfo->clear ();
		mpWorkSectInfo = NULL;
	}
}

/**
 * deleteAllSectionList
 * リスト先頭から順に削除
 */
void CSectionParser::deleteAllSectionList (void)
{
	CSectionInfo *pTmp = NULL;
	CSectionInfo *pDel = NULL;


	if (!mpSectListTop) {
		// リストが空

	} else {
		pTmp = mpSectListTop;

		while (pTmp) {
			pDel = pTmp;

			// next set
			pTmp = pTmp->mpNext;

			pDel->clear();
			delete pDel;
			pDel = NULL;
		}
	}

	mpSectListTop = NULL;
	mpSectListBottom = NULL;
}

/**
 * shiftAllSectionList
 * リストのsectionのmpRaw,mpData,mpTailをずらす
 */
void CSectionParser::shiftAllSectionList (size_t shiftSize)
{
	CSectionInfo *pTmp = NULL;


	if (!mpSectListTop) {
		// リストが空

	} else {
		pTmp = mpSectListTop;

		while (pTmp) {

			pTmp->mpRaw -= shiftSize;
			pTmp->mpData -= shiftSize;
			pTmp->mpTail -= shiftSize;

			// next set
			pTmp = pTmp->mpNext;
		}
	}
}

/**
 * searchSectionList
 * リスト先頭から検索 (登録順で古いものから)
 * インスタンスの一致
 */
CSectionInfo *CSectionParser::searchSectionList (const CSectionInfo &sectInfo) const
{
	CSectionInfo *pTmp = NULL;


	if (!mpSectListTop) {
		// リストが空

	} else {
		pTmp = mpSectListTop;

		while (pTmp) {
			if (*pTmp == sectInfo) {
				// 一致
				return pTmp;
			}

			// next set
			pTmp = pTmp->mpNext;
		}
	}

	// not found
	return NULL;
}

/**
 *
 */
void CSectionParser::checkDetachFifoSectionList (void)
{
	if (!mListIsFifo) {
		return;
	}

	if (mFifoNum >= getSectionListNum()) {
		return;
	}

	CSectionInfo *pDelSect = checkDeleteFifoSectionList ();
	if (pDelSect) {
		// pool から消す - 間詰める
//TODO memcpy-param-overlap
//		memcpy (pDelSect->mpRaw, pDelSect->mpTail, mpPool + (mPoolSize -1) - pDelSect->mpTail);
		uint8_t tmp [mPoolSize] = {0};
		size_t cpsize = mpPool + mPoolSize - pDelSect->mpTail;
		memcpy (tmp, pDelSect->mpTail, cpsize);
		memcpy (pDelSect->mpRaw, tmp, cpsize);

		uint32_t shift = pDelSect->mpTail - pDelSect->mpRaw;
		mPoolInd -= shift;

		shiftAllSectionList (shift);

		// pool 消す時はセットで
		if (pDelSect == mpWorkSectInfo) {
			mpWorkSectInfo->clear ();
			mpWorkSectInfo = NULL;
		}

		delete pDelSect;
		pDelSect = NULL;
	}
}

/**
 * 最も古いもの(top)のアドレスをリストから切り離し、そのアドレスを返します。
 */
CSectionInfo* CSectionParser::checkDeleteFifoSectionList (void)
{
	CSectionInfo *pDel = NULL;


	if (!mpSectListTop) {
		// リストが空

	} else {
		if (!mpSectListTop->mpNext) {
			// リストが一つ
			pDel = mpSectListTop;

			mpSectListTop = NULL;
			mpSectListBottom = NULL;

		} else {
			pDel = mpSectListTop;
			mpSectListTop = mpSectListTop->mpNext;
		}
	}

	return pDel;
}

/**
 *
 */
int CSectionParser::getSectionListNum (void) const
{
	CSectionInfo *pTmp = NULL;
	int n = 0;

	pTmp = mpSectListTop;
	while (pTmp) {
		// next set
		pTmp = pTmp->mpNext;
		++ n ;
	}

	return n;
}

/**
 *
 */
uint16_t CSectionParser::getPid (void) const
{
	return mPid;
}

/**
 *
 */
void CSectionParser::dumpSectionList (void) const
{
	_UTL_LOG_I ("- section list ----------");
	CSectionInfo *pTmp = NULL;
	int n = 0;

	pTmp = mpSectListTop;
	while (pTmp) {

		_UTL_LOG_I (
			" %d: table_id:[0x%02x] state:[%s] tail-raw:[%d] (%p -> %p)\n",
			n,
			pTmp->mSectHdr.table_id,
			s_szSectState[pTmp->mState],
			int(pTmp->mpTail - pTmp->mpRaw),
			pTmp,
			pTmp->mpNext
		);

		// next set
		pTmp = pTmp->mpNext;
		++ n ;
	}
	_UTL_LOG_I ("-------------------------");
}

/**
 *
 */
EN_CHECK_SECTION CSectionParser::checkSectionFirst (uint8_t *pPayload, size_t payloadSize)
{
	if (!pPayload || (payloadSize == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}

	uint8_t *p = pPayload;
	size_t size = payloadSize;

	uint8_t pointer_field = *p;
	p += 1;
	size = size -1;


	if((p + pointer_field) >= (pPayload + payloadSize)) {
//TODO
		_UTL_LOG_W ("pid:[0x%04x] payloadSize=[%d]", mPid, payloadSize);
		_UTL_LOG_W ("pid:[0x%04x] pointer_field=[%d]", mPid, pointer_field);
		_UTL_LOG_W ("pid:[0x%04x] input data is probably broken", mPid);
		return EN_CHECK_SECTION__INVALID;
	}

	if (pointer_field > 0) {
		_UTL_LOG_D ("pointer_field=[%d]", pointer_field);
		EN_CHECK_SECTION chk = checkSectionFollow (p, pointer_field);
		switch ((int)chk) {
		case EN_CHECK_SECTION__RECEIVING:
		case EN_CHECK_SECTION__INVALID:
			return EN_CHECK_SECTION__INVALID;
			break;
		}
		p += pointer_field;
		size = size - pointer_field;
	}


	EN_CHECK_SECTION r = EN_CHECK_SECTION__COMPLETED;
	bool isEvenOnceCompleted = false;

	// payload_unit_start_indicator == 1 のパケット内に
	// 複数のセクションが含まれる場合があるので そのためのループ
	while ((size > 0) && (*p != 0xff)) {

//TODO mpWorkSectInfo check
		if (mpWorkSectInfo) {
			if (mpWorkSectInfo->mState <= EN_SECTION_STATE__RECEIVING) {
				_UTL_LOG_W ("pid:[0x%04x] unexpected first packet came. start over...", mPid);
				detachSectionList (mpWorkSectInfo);
				mpWorkSectInfo = NULL;
			}
		}


		CSectionInfo *pAttached = attachSectionList (p, size);
		if (!pAttached) {
			return EN_CHECK_SECTION__INVALID;
		}

		// debug dump
		if (CUtils::get_logger()->get_log_level() <= CLogger::level::debug) {
			dumpSectionList ();
		}

		// set working addr
		mpWorkSectInfo = pAttached;

		// debug dump
		if (CUtils::get_logger()->get_log_level() <= CLogger::level::debug) {
			mpWorkSectInfo->dumpHeader ();
		}

		if (!onSectionStarted (mpWorkSectInfo)) {
			r = EN_CHECK_SECTION__IGNORE;
			size_t tmp_total_len = SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			if (tmp_total_len >= size) {
				// ignoreしたセクションがパケットをまたいだものだとすると
				// tmp_total_len分はsizeを超えてくるので ループを抜けて次のパケットを見に行きます
				size = 0;
			} else {
				size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
				p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			}

			detachSectionList (mpWorkSectInfo);
			mpWorkSectInfo = NULL;

			// mIsIgnoreSection 開始します
			mIsIgnoreSection = true;

			continue;
		}

		// mIsIgnoreSection 解除します
		mIsIgnoreSection = false;


		if (mpWorkSectInfo->isReceiveAll()) {
			mpWorkSectInfo->mState = EN_SECTION_STATE__RECEIVED;

		} else {
			// パケットをまたいだ
			mpWorkSectInfo->mState = EN_SECTION_STATE__RECEIVING;
			if (isEvenOnceCompleted) {
				return EN_CHECK_SECTION__COMPLETED;
			} else {
				return EN_CHECK_SECTION__RECEIVING;
			}
		}


//TODO
//		if (mpWorkSectInfo->mSectHdr.section_syntax_indicator != 0) {
//			detachSectionList (mpWorkSectInfo);
//			mpWorkSectInfo = NULL;
//			return false;
//		}

		uint16_t nDataPartLen = mpWorkSectInfo->getDataPartLen ();
		_UTL_LOG_D ("nDataPartLen %d\n", nDataPartLen);


//		if (mType == EN_SECTION_TYPE__PSISI) {
			// check CRC
			if (!mpWorkSectInfo->checkCRC32()) {
				_UTL_LOG_E ("pid:[0x%04x] CRC32 fail", mPid);
				r = EN_CHECK_SECTION__CRC32_ERR;
				size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
				p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

				detachSectionList (mpWorkSectInfo);
				mpWorkSectInfo = NULL;
				continue;
			}
			_UTL_LOG_D ("CRC32 ok");

//		} else if (mType == EN_SECTION_TYPE__DSMCC) {
//
//		} else {
//			_UTL_LOG_E ("pid:[0x%04x] EN_SECTION_TYPE is invalid.", mPid);
//		}


		size_t trunclen = mpWorkSectInfo->truncate ();
		truncate (trunclen);

		// debug dump
		if (CUtils::get_logger()->get_log_level() <= CLogger::level::debug) {
			dumpSectionList ();
		}

		// すでに持っているsectionかどうかチェック
		CSectionInfo *pFound = searchSectionList (*mpWorkSectInfo);
		if (pFound && pFound->mState == EN_SECTION_STATE__COMPLETE) {
			_UTL_LOG_D ("already know section -> detach");
			r = EN_CHECK_SECTION__COMPLETED_ALREADY;
			size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

			detachSectionList (mpWorkSectInfo);
			mpWorkSectInfo = NULL;
			continue;

		} else {
			// new section
			_UTL_LOG_D ("new section");
			isEvenOnceCompleted = true;
			r = EN_CHECK_SECTION__COMPLETED;
			size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

			mpWorkSectInfo->mState = EN_SECTION_STATE__COMPLETE;
			checkDetachFifoSectionList ();

			onSectionCompleted (mpWorkSectInfo);

			mpWorkSectInfo = NULL;
			continue;
		}

	} // while ((size > 0) && (*p != 0xff))

	if (isEvenOnceCompleted) {
		return EN_CHECK_SECTION__COMPLETED;
	} else {
		return r;
	}
}

/**
 *
 */
EN_CHECK_SECTION CSectionParser::checkSectionFollow (uint8_t *pPayload, size_t payloadSize)
{
	if (mIsIgnoreSection) {
		return EN_CHECK_SECTION__IGNORE;
	}

	if (!pPayload || (payloadSize == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}

	if (!mpWorkSectInfo) {
		_UTL_LOG_W ("pid:[0x%04x] head packet has not arrived yet", mPid);
		return EN_CHECK_SECTION__INVALID;
	}

	if (mpWorkSectInfo->mState != EN_SECTION_STATE__RECEIVING) {
		_UTL_LOG_W ("pid:[0x%04x] head packet has not arrived yet (not EN_SECTION_STATE__RECEIVING)", mPid);
		return EN_CHECK_SECTION__INVALID;
	}

	uint8_t *p = pPayload;

	CSectionInfo *pAttached = attachSectionList (p, payloadSize);
	if (!pAttached) {
		return EN_CHECK_SECTION__INVALID;
	}

	if (pAttached != mpWorkSectInfo) {
		_UTL_LOG_E ("BUG: pid:[0x%04x] (pAttached != mpWorkSectInfo)", mPid);
		return EN_CHECK_SECTION__INVALID;
	}

	// debug dump
	if (CUtils::get_logger()->get_log_level() <= CLogger::level::debug) {
		dumpSectionList ();
		mpWorkSectInfo->dumpHeader ();
	}

	if (mpWorkSectInfo->isReceiveAll()) {
		mpWorkSectInfo->mState = EN_SECTION_STATE__RECEIVED;

	} else {
		// パケットをまたいだ
		mpWorkSectInfo->mState = EN_SECTION_STATE__RECEIVING;
		return EN_CHECK_SECTION__RECEIVING;
	}


//TODO
//	if (mpWorkSectInfo->mSectHdr.section_syntax_indicator != 0) {
//		detachSectionList (mpWorkSectInfo);
//		mpWorkSectInfo = NULL;
//		return false;
//	}

	uint16_t nDataPartLen = mpWorkSectInfo->getDataPartLen ();
	_UTL_LOG_D ("nDataPartLen %d\n", nDataPartLen);


//	if (mType == EN_SECTION_TYPE__PSISI) {
		// check CRC
		if (!mpWorkSectInfo->checkCRC32()) {
			_UTL_LOG_E ("pid:[0x%04x] CRC32 fail", mPid);
			detachSectionList (mpWorkSectInfo);
			mpWorkSectInfo = NULL;
			return EN_CHECK_SECTION__CRC32_ERR;
		}
		_UTL_LOG_D ("CRC32 ok");

//	} else if (mType == EN_SECTION_TYPE__DSMCC) {
//
//	} else {
//		_UTL_LOG_E ("pid:[0x%04x] EN_SECTION_TYPE is invalid.", mPid);
//	}


	size_t trunclen = mpWorkSectInfo->truncate ();
	truncate (trunclen);

	// debug dump
	if (CUtils::get_logger()->get_log_level() <= CLogger::level::debug) {
		dumpSectionList ();
	}

	// すでに持っているsectionかどうかチェック
	CSectionInfo *pFound = searchSectionList (*mpWorkSectInfo);
	if (pFound && pFound->mState == EN_SECTION_STATE__COMPLETE) {
		_UTL_LOG_D ("already know section -> detach");
		detachSectionList (mpWorkSectInfo);
		mpWorkSectInfo = NULL;
		return EN_CHECK_SECTION__COMPLETED_ALREADY;

	} else {
		// new section
		_UTL_LOG_D ("new section");
		mpWorkSectInfo->mState = EN_SECTION_STATE__COMPLETE;
		checkDetachFifoSectionList ();

		onSectionCompleted (mpWorkSectInfo);

		return EN_CHECK_SECTION__COMPLETED;
	}
}

/**
 *
 */
EN_CHECK_SECTION CSectionParser::checkSection (const TS_HEADER *pTsHdr, uint8_t *pPayload, size_t payloadSize)
{
	if (!pTsHdr || !pPayload || (payloadSize == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}


	mPid = pTsHdr->pid;

	if (pTsHdr->payload_unit_start_indicator == 0) {

		_UTL_LOG_D ("checkSectionFollow");
		return checkSectionFollow (pPayload, payloadSize);

	} else {

//		if (mIsAsyncDelete) {
//			_UTL_LOG_I ("pid:[0x%04x] asyncDelete -> detachAllSectionList", mPid);
//			detachAllSectionList ();
//			mIsAsyncDelete = false;
//		}

		_UTL_LOG_D ("checkSectionFirst");
		return checkSectionFirst (pPayload, payloadSize);
	}
}

/**
 *
 */
CSectionInfo *CSectionParser::getLatestCompleteSection (void) const
{
	CSectionInfo *pTmp = NULL;
	CSectionInfo *pLatest = NULL;

	pTmp = mpSectListTop;
	while (pTmp) {
		if (pTmp->mState == EN_SECTION_STATE__COMPLETE) {
			// update latest section
			pLatest = pTmp;
		}

		// next set
		pTmp = pTmp->mpNext;
	}

	return pLatest;
}

/**
 * truncate
 */
void CSectionParser::truncate (size_t truncateLen)
{
	memset (mpPool + mPoolInd - truncateLen, 0x00, truncateLen);
	mPoolInd -= truncateLen;
}

/**
 * onSectionStarted
 */
bool CSectionParser::onSectionStarted (const CSectionInfo *pCompSection)
{
	return true;
}

/**
 * onSectionCompleted
 */
void CSectionParser::onSectionCompleted (const CSectionInfo *pCompSection)
{
	return;
}

/**
 *
 */
void CSectionParser::asyncDelete (void)
{
	mIsAsyncDelete = true;
}

/**
 *
 */
void CSectionParser::checkAsyncDelete (void)
{
	if (mIsAsyncDelete) {
		_UTL_LOG_I ("pid:[0x%04x] asyncDelete -> detachAllSectionList", mPid);
		detachAllSectionList ();
		mIsAsyncDelete = false;
	}
}
