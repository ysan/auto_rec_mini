#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsCommon.h"
#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "TimeOffsetTable.h"
#include "EventInformationTable.h"
#include "NetworkInformationTable.h"
#include "ServiceDescriptionTable.h"
#include "RunningStatusTable.h"
#include "BroadcasterInformationTable.h"
#include "Dsmcc.h"


#define INNER_BUFF_SIZE		(65535*5)


class CDsmccControl {
public:
	CDsmccControl (void)
		:pid (0)
		,mpDsmcc (NULL)
		,isUsed (false)
	{}
	virtual ~CDsmccControl (void) {
		if (mpDsmcc) {
			delete mpDsmcc;
			mpDsmcc = NULL;
		}
	}

	uint16_t pid;
	CDsmcc *mpDsmcc;
	bool isUsed;
};

class CTsParser
{
public:
	CTsParser (void);
	CTsParser (IParserListener *p_Listener);
	virtual ~CTsParser (void);

	void run (uint8_t *pBuff, size_t nSize);

private:
	bool allocInnerBuffer (uint8_t *pBuff, size_t nSize);
	bool copyInnerBuffer (uint8_t *pBuff, size_t nSize);
	bool checkUnitSize (void);
	uint8_t *getSyncTopAddr (uint8_t *pTop, uint8_t *pBtm, size_t nUnitSize) const;
//	void getTsHeader (TS_HEADER *pDst, uint8_t *pSrc) const;
//	void dumpTsHeader (const TS_HEADER *p) const;

	bool parse (void);


	uint8_t *mp_top ;
	uint8_t *mp_current ;
	uint8_t *mp_bottom ;
	size_t m_buff_size ;
	int m_unit_size;
	int m_parse_remain_len;

	uint8_t m_inner_buff [INNER_BUFF_SIZE];



	CProgramAssociationTable mPAT;
	CProgramAssociationTable::CTable mPatTables [4 * 8];
	CTimeOffsetTable mTOT;
	CEventInformationTable mEIT_H;
	CEventInformationTable mEIT_M;
	CEventInformationTable mEIT_L;
	CNetworkInformationTable mNIT;
	CServiceDescriptionTable mSDT;
	CRunningStatusTable mRST;
	CBroadcasterInformationTable mBIT;

	CDsmccControl mDsmccCtls [256];
};

#endif