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
		_UTL_LOG_I ("section fragment\n");
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
	static const uint32_t table [256] = {
		0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
		0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
		0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 
		0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
		
		0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,
		0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 
		0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
		0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
		
		0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
		0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
		0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 
		0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
		
		0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,
		0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
		0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 
		0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,

		0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,
		0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 
		0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
		0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,

		0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 
		0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
		0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
		0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
		
		0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,
		0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
		0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 
		0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,

		0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
		0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 
		0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,
		0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,

		0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 
		0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
		0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,
		0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
		
		0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
		0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
		0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
		0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,

		0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,
		0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 
		0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,
		0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
		
		0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 
		0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
		0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,
		0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
		
		0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
		0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
		0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
		0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,

		0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,
		0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
		0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,
		0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,

		0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
		0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
		0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
		0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 

		0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,
		0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
		0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 
		0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4,
	};
	
	uint32_t crc = 0xffffffff;
	uint8_t *p = mpRaw;

	// 右辺はheader含めたsectionの合計
	while (p < (mpRaw + mSectHdr.section_length + SECTION_SHORT_HEADER_LEN)) {
		crc = (crc << 8) ^ table [((crc >> 24) ^ p[0]) & 0xff];
		p += 1;
	}

	return crc ? false: true;
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
{
	mpPool = new uint8_t [mPoolSize];
	memset (mpPool, 0x00, mPoolSize);
	mType = type;
}

CSectionParser::CSectionParser (uint8_t fifoNum, EN_SECTION_TYPE type)
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
{
	mPoolSize = poolSize;
	mpPool = new uint8_t [mPoolSize];
	memset (mpPool, 0x00, mPoolSize);
	mType = type;
}

CSectionParser::CSectionParser (size_t poolSize ,uint8_t fifoNum, EN_SECTION_TYPE type)
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
		_UTL_LOG_I ("pool full\n");
		return NULL;
	}


	uint8_t* pCur = mpPool + mPoolInd;

	// sectionの生データをpoolにコピー
	memcpy (pCur, pBuff, size);
	mPoolInd += size;


	if (mpWorkSectInfo && (mpWorkSectInfo->mState == EN_SECTION_STATE__RECEIVING)) {
		// follow
		if (pCur != mpWorkSectInfo->mpTail) {
			_UTL_LOG_E ("BUG");
		}
		mpWorkSectInfo->mpTail = pCur + size;
		pRtn = mpWorkSectInfo;

	} else {
		// first
		CSectionInfo tmpSect (pCur, size, mType);

		CSectionInfo* pList = addSectionList (&tmpSect);
		if (!pList) {

			// cancel pool data
			memcpy (pCur, mpPool + mPoolInd, (mpPool + (mPoolSize -1)) - (mpPool + mPoolInd));
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
 *
 */
void CSectionParser::detachSectionList (const CSectionInfo *pSectInfo)
{
	if (!pSectInfo) {
		return ;
	}

	deleteSectionList (pSectInfo);

	// pool から消す - 間詰める
	memcpy (pSectInfo->mpRaw, pSectInfo->mpTail, mpPool + (mPoolSize -1) - pSectInfo->mpTail);
	uint32_t shift = pSectInfo->mpTail - pSectInfo->mpRaw;
	mPoolInd -= shift;

	// pool 消す時はセットで
	if (pSectInfo == mpWorkSectInfo) {
		mpWorkSectInfo->clear ();
		mpWorkSectInfo = NULL;
	}
}

/**
 * deleteSectionList
 * インスタンスの一致で削除
 * 重複していないこと前提
 */
void CSectionParser::deleteSectionList (const CSectionInfo &sectInfo)
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

				_UTL_LOG_I ("delete");
				delete pDel;
				pDel = NULL;
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
void CSectionParser::deleteSectionList (const CSectionInfo *pSectInfo)
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

				_UTL_LOG_I ("delete");
				delete pDel;
				pDel = NULL;
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

	memset (mpPool, 0x00, mPoolSize);
	mPoolInd = 0;

	// pool 消す時はセットで
	mpWorkSectInfo->clear ();
	mpWorkSectInfo = NULL;
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

			delete pDel;
			pDel = NULL;
		}
	}

	mpSectListTop = NULL;
	mpSectListBottom = NULL;
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
		memcpy (pDelSect->mpRaw, pDelSect->mpTail, mpPool + (mPoolSize -1) - pDelSect->mpTail);
		uint32_t shift = pDelSect->mpTail - pDelSect->mpRaw;
		mPoolInd -= shift;

		// pool 消す時はセットで
		if (pDelSect == mpWorkSectInfo) {
			mpWorkSectInfo->clear ();
			mpWorkSectInfo = NULL;
		}

//TODO 関数化
		// リストに入ってるsectionのmpRaw,mpData,mpTailをずらす
		CSectionInfo *pTmp = NULL;
		if (!mpSectListTop) {
			// リストが空
		} else {
			pTmp = mpSectListTop;

			while (pTmp) {
				pTmp->mpRaw -= shift;
				pTmp->mpData -= shift;
				pTmp->mpTail -= shift;

				// next set
				pTmp = pTmp->mpNext;
			}
		}
	}

	delete pDelSect;
	pDelSect = NULL;
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
		_UTL_LOG_I("payloadSize=[%d]", payloadSize);
		_UTL_LOG_I("pointer_field=[%d]", pointer_field);
		_UTL_LOG_E ("input data is probably broken");
		return EN_CHECK_SECTION__INVALID;
	}

	if (pointer_field > 0) {
		_UTL_LOG_I("pointer_field=[%d]", pointer_field);
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

	// payload_unit_start_indicator == 1 のパケット内に
	// 複数のts含まれるためのループ
	while ((size > 0) && (*p != 0xff)) {

//TODO mpWorkSectInfo check
		if (mpWorkSectInfo) {
			if (mpWorkSectInfo->mState <= EN_SECTION_STATE__RECEIVING) {
				_UTL_LOG_W ("unexpected first packet came. start over...");
				detachSectionList (mpWorkSectInfo);
				mpWorkSectInfo = NULL;
			}
		}


		CSectionInfo *pAttached = attachSectionList (p, size);
		if (!pAttached) {
			return EN_CHECK_SECTION__INVALID;
		}
dumpSectionList ();

		// set working addr
		mpWorkSectInfo = pAttached;
mpWorkSectInfo->dumpHeader ();


		if (!onSectionStarted (mpWorkSectInfo)) {
			r = EN_CHECK_SECTION__CANCELED;
			size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

			detachSectionList (mpWorkSectInfo);
			mpWorkSectInfo = NULL;
//			return EN_CHECK_SECTION__CANCELED;
			continue;
		}

		if (mpWorkSectInfo->isReceiveAll()) {
			mpWorkSectInfo->mState = EN_SECTION_STATE__RECEIVED;

		} else {
			// パケットをまたいだ
			mpWorkSectInfo->mState = EN_SECTION_STATE__RECEIVING;
			return EN_CHECK_SECTION__RECEIVING;
		}


//TODO
//		if (mpWorkSectInfo->mSectHdr.section_syntax_indicator != 0) {
//			detachSectionList (mpWorkSectInfo);
//			mpWorkSectInfo = NULL;
//			return false;
//		}

		uint16_t nDataPartLen = mpWorkSectInfo->getDataPartLen ();
		_UTL_LOG_I ("nDataPartLen %d\n", nDataPartLen);


//		if (mType == EN_SECTION_TYPE__PSISI) {
			// check CRC
			if (!mpWorkSectInfo->checkCRC32()) {
				_UTL_LOG_E ("CRC32 fail");
				r = EN_CHECK_SECTION__CRC32_ERR;
				size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
				p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

				detachSectionList (mpWorkSectInfo);
				mpWorkSectInfo = NULL;
//				return EN_CHECK_SECTION__INVALID;
				continue;
			}
			_UTL_LOG_I ("CRC32 ok");

//		} else if (mType == EN_SECTION_TYPE__DSMCC) {
//
//		} else {
//			_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
//		}


		size_t trunclen = mpWorkSectInfo->truncate ();
		truncate (trunclen);
dumpSectionList ();


		// すでに持っているsectionかどうかチェック
		CSectionInfo *pFound = searchSectionList (*mpWorkSectInfo);
		if (pFound && pFound->mState == EN_SECTION_STATE__COMPLETE) {
			_UTL_LOG_N ("already know section -> detach");
			r = EN_CHECK_SECTION__CANCELED;
			size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

			detachSectionList (mpWorkSectInfo);
			mpWorkSectInfo = NULL;
//			return EN_CHECK_SECTION__CANCELED;
			continue;

		} else {
			// new section
			_UTL_LOG_N ("new section");
			r = EN_CHECK_SECTION__COMPLETED;
			size -= SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;
			p += SECTION_SHORT_HEADER_LEN + mpWorkSectInfo->getHeader()->section_length;

			mpWorkSectInfo->mState = EN_SECTION_STATE__COMPLETE;
			checkDetachFifoSectionList ();

			onSectionCompleted (mpWorkSectInfo);

			mpWorkSectInfo = NULL;
//			return EN_CHECK_SECTION__COMPLETED;
			continue;
		}

	} // while ((size > 0) && (*p != 0xff))


	return r;
}

/**
 *
 */
EN_CHECK_SECTION CSectionParser::checkSectionFollow (uint8_t *pPayload, size_t payloadSize)
{
	if (!pPayload || (payloadSize == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}

	if (!mpWorkSectInfo) {
		_UTL_LOG_W ("head packet has not arrived yet");
		return EN_CHECK_SECTION__INVALID;
	}

	if (mpWorkSectInfo->mState != EN_SECTION_STATE__RECEIVING) {
		_UTL_LOG_W ("head packet has not arrived yet (not EN_SECTION_STATE__RECEIVING)");
		return EN_CHECK_SECTION__INVALID;
	}

	uint8_t *p = pPayload;

	CSectionInfo *pAttached = attachSectionList (p, payloadSize);
	if (!pAttached) {
		return EN_CHECK_SECTION__INVALID;
	}

	if (pAttached != mpWorkSectInfo) {
		_UTL_LOG_E ("BUG");
		return EN_CHECK_SECTION__INVALID;
	}

dumpSectionList ();
mpWorkSectInfo->dumpHeader ();


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
	_UTL_LOG_I ("nDataPartLen %d\n", nDataPartLen);


//	if (mType == EN_SECTION_TYPE__PSISI) {
		// check CRC
		if (!mpWorkSectInfo->checkCRC32()) {
			_UTL_LOG_E ("CRC32 fail");
			detachSectionList (mpWorkSectInfo);
			mpWorkSectInfo = NULL;
			return EN_CHECK_SECTION__CRC32_ERR;
		}
		_UTL_LOG_I ("CRC32 ok");

//	} else if (mType == EN_SECTION_TYPE__DSMCC) {
//
//	} else {
//		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
//	}


	size_t trunclen = mpWorkSectInfo->truncate ();
	truncate (trunclen);
dumpSectionList ();


	// すでに持っているsectionかどうかチェック
	CSectionInfo *pFound = searchSectionList (*mpWorkSectInfo);
	if (pFound && pFound->mState == EN_SECTION_STATE__COMPLETE) {
		_UTL_LOG_N ("already know section -> detach");
		detachSectionList (mpWorkSectInfo);
		mpWorkSectInfo = NULL;
		return EN_CHECK_SECTION__CANCELED;

	} else {
		// new section
		_UTL_LOG_N ("new section");
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
		_UTL_LOG_I("checkSectionFollow");
		return checkSectionFollow (pPayload, payloadSize);
	} else {
		_UTL_LOG_I("checkSectionFirst");
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
