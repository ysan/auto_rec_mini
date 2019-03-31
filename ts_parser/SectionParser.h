#ifndef _SECTION_PARSER_H_
#define _SECTION_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsAribCommon.h"


typedef enum {
	EN_SECTION_STATE__INIT = 0,
	EN_SECTION_STATE__RECEIVING,
	EN_SECTION_STATE__RECEIVED,
	EN_SECTION_STATE__COMPLETE,

	EN_SECTION_STATE_NUM,
} EN_SECTION_STATE;

typedef enum {
	EN_SECTION_TYPE__PSISI = 0,
	EN_SECTION_TYPE__DSMCC,

	EN_SECTION_TYPE_NUM,
} EN_SECTION_TYPE;

typedef struct {

	void dump (void) const {
		if (section_syntax_indicator == 0) {
			_UTL_LOG_I (
				"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d]\n",
				table_id,
				section_syntax_indicator,
				private_indicator,
				section_length
			);
		} else {
			_UTL_LOG_I (
				"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d] tbl_ext:[0x%04x] ver:[0x%02x] next:[0x%02x] num:[0x%02x] last:[0x%02x]\n",
				table_id,
				section_syntax_indicator,
				private_indicator,
				section_length,
				table_id_extension,
				version_number,
				current_next_indicator,
				section_number,
				last_section_number
			);
		}
	}

	uint8_t table_id;						//  0- 7 :  8 bits
	uint8_t section_syntax_indicator;		//  8- 8 :  1 bit
	uint8_t private_indicator;				//  9- 9 :  1 bit
	uint8_t reserved_1;						// 10-11 :  2 bits
	uint16_t section_length;				// 12-23 : 12 bits
	uint16_t table_id_extension;			// 24-39 : 16 bits
	uint8_t reserved_2;						// 40-41 :  2 bits
	uint8_t version_number;					// 42-46 :  5 bits
	uint8_t current_next_indicator;			// 47-47 :  1 bit
	uint8_t section_number;					// 48-55 :  8 bits
	uint8_t last_section_number;			// 56-63 :  8 bits
} ST_SECTION_HEADER;


class CSectionInfo {
public:
	friend class CSectionParser;

	CSectionInfo (uint8_t *pBuff, size_t size, EN_SECTION_TYPE enType=EN_SECTION_TYPE__PSISI);
	CSectionInfo (CSectionInfo *pObj);
	~CSectionInfo (void);

	bool operator==(const CSectionInfo &obj) const {
		if (!mpRaw || !mpTail) {
			return false;
		}

		if (!obj.mpRaw || !obj.mpTail) {
			return false;
		}

		size_t naLen = obj.mpTail - obj.mpRaw;
		size_t nbLen = mpTail - mpRaw;
		if (naLen != nbLen) {
			return false;
		}

//		if (memcmp (obj.mpRaw, mpRaw, TS_PACKET_LEN - (TS_HEADER_LEN +1)) != 0) { // +1はpointer_field分
		if (memcmp (obj.mpRaw, mpRaw, naLen) != 0) {
			return false;
		}

		return true;
	}


	uint8_t *getHeaderAddr (void) const;
	const ST_SECTION_HEADER *getHeader (void) const;
	uint8_t *getDataPartAddr (void) const;
	uint16_t getDataPartLen (void) const;

	static uint8_t * parseHeader (ST_SECTION_HEADER *pHdrOut, uint8_t *pStart, uint8_t *pEnd, EN_SECTION_TYPE enType=EN_SECTION_TYPE__PSISI) ;
	static void dumpHeader (const ST_SECTION_HEADER *pHdr, bool isShortFormat) ;


private:
	void parseHeader (void);
	void dumpHeader (void) const;
	bool isReceiveAll (void) const;
	size_t truncate (void);
	void clear (void);
	bool checkCRC32 (void) const;


	ST_SECTION_HEADER mSectHdr;
	uint8_t *mpRaw; // section header
	uint8_t *mpData; // section data head
	uint8_t *mpTail; // payload tail
	EN_SECTION_STATE mState;
	EN_SECTION_TYPE mType;


	// link-list
	CSectionInfo *mpNext;
};



typedef enum {
	EN_CHECK_SECTION__COMPLETED = 0,
	EN_CHECK_SECTION__COMPLETED_ALREADY,
	EN_CHECK_SECTION__RECEIVING,
	EN_CHECK_SECTION__CANCELED,
	EN_CHECK_SECTION__CRC32_ERR,
	EN_CHECK_SECTION__INVALID,
} EN_CHECK_SECTION;

class CSectionParser
{
public:
    static const uint32_t POOL_SIZE;
	
public:
	CSectionParser (EN_SECTION_TYPE enType=EN_SECTION_TYPE__PSISI);
	CSectionParser (int fifoNum, EN_SECTION_TYPE enType=EN_SECTION_TYPE__PSISI);
	CSectionParser (size_t poolSize, EN_SECTION_TYPE enType=EN_SECTION_TYPE__PSISI);
	CSectionParser (size_t poolSize, int fifoNum, EN_SECTION_TYPE enType=EN_SECTION_TYPE__PSISI);
	virtual ~CSectionParser (void);


	EN_CHECK_SECTION checkSection (const TS_HEADER *pTsHdr, uint8_t *pPayload, size_t payloadSize);

protected:
	CSectionInfo *getLatestCompleteSection (void) const;
	void detachSectionList (const CSectionInfo *pSectInfo);
	void detachAllSectionList (void);
	CSectionInfo *searchSectionList (const CSectionInfo &sectInfo) const;
	int getSectionListNum (void) const;

	virtual bool onSectionStarted (const CSectionInfo *pSection);
	virtual void onSectionCompleted (const CSectionInfo *pCompSection);


private:
//	void convertRaw2SectionInfo (CSectionInfo *pOut, uint8_t *pBuff, size_t size);

	CSectionInfo* attachSectionList (uint8_t *pBuff, size_t size);
	CSectionInfo* addSectionList (CSectionInfo *pSectInfo);

	void deleteSectionList (const CSectionInfo &sectInfo);
	void deleteSectionList (const CSectionInfo *pSectInfo);
	void deleteAllSectionList (void);

	void checkDetachFifoSectionList (void);
	CSectionInfo* checkDeleteFifoSectionList (void);

	void dumpSectionList (void) const;

	EN_CHECK_SECTION checkSectionFirst (uint8_t *pPayload, size_t payloadSize);
	EN_CHECK_SECTION checkSectionFollow (uint8_t *pPayload, size_t payloadSize);

	void truncate (size_t truncateLen);


	uint16_t mPid;

	CSectionInfo *mpSectListTop;
	CSectionInfo *mpSectListBottom;

	bool mListIsFifo;
	int mFifoNum; // limited num of sectionList

	CSectionInfo *mpWorkSectInfo;

	uint8_t *mpPool;
	size_t mPoolInd;
	size_t mPoolSize;

	EN_SECTION_TYPE mType;
};

#endif
