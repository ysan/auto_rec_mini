#ifndef _SECTION_PARSER_H_
#define _SECTION_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsCommon.h"


typedef enum {
	EN_SECTION_STATE__INIT = 0,
	EN_SECTION_STATE__RECEIVING,
	EN_SECTION_STATE__RECEIVED,
	EN_SECTION_STATE__COMPLETE,

	EN_SECTION_STATE_NUM,
} EN_SECTION_STATE;


class CSectionInfo {
public:
	friend class CSectionParser;

	CSectionInfo (uint8_t *pBuff, size_t size);
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
	ST_SECTION_HEADER *getHeader (void) ;
	uint8_t *getDataPartAddr (void) const;
	uint16_t getDataPartLen (void) const;

	static uint8_t * parseHeader (ST_SECTION_HEADER *pHdrOut, uint8_t *pStart, uint8_t *pEnd) ;
	static void dumpHeader (const ST_SECTION_HEADER *pHdr, bool isShortFormat) ;

private:
	void parseHeader (void);
	void dumpHeader (void) const;
	bool isReceiveAll (void) const;
	bool checkCRC32 (void) const;


	ST_SECTION_HEADER mSectHdr;
	uint8_t *mpRaw; // section header
	uint8_t *mpData; // section data head
	uint8_t *mpTail; // payload tail
	EN_SECTION_STATE mState;
	bool mIsShortFormatHeader;

	// link-list
	CSectionInfo *mpNext;
};


//typedef struct ts_section_list {
//	CSectionInfo sectInfo;
//	struct ts_section_list *pNext;
//} ST_SECTION_LIST;

class CSectionParser
{
public:
    static const uint32_t POOL_SIZE;
	
public:
	CSectionParser (void);
	CSectionParser (uint8_t fifoNum);
	CSectionParser (size_t poolSize);
	CSectionParser (size_t poolSize, uint8_t fifoNum);
	virtual ~CSectionParser (void);


	bool checkSection (const ST_TS_HEADER *pstTsHdr, uint8_t *pPayload, size_t payloadSize);

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

	bool checkSectionFirst (uint8_t *pPayload, size_t payloadSize);
	bool checkSectionFollow (uint8_t *pPayload, size_t payloadSize);

	void clearWorkSectionInfo (void);



	uint16_t mPid;

	CSectionInfo *mpSectListTop;
	CSectionInfo *mpSectListBottom;

	bool mListIsFifo;
	uint8_t mFifoNum; // limited num of sectionList

	CSectionInfo *mpWorkSectInfo;

	uint8_t *mpPool;
	size_t mPoolInd;
	size_t mPoolSize;
};

#endif
