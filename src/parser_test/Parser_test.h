#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "program_association_table.h"
#include "program_map_table.h"
#include "time_offset_table.h"
#include "event_information_table.h"
#include "network_information_table.h"
#include "service_description_table.h"
#include "running_status_table.h"
#include "broadcaster_information_table.h"
#include "dsmcc.h"

using namespace parser;


#define INNER_BUFF_SIZE		(65535*5)


class CProgramCache {
public:
	CProgramCache (void)
		:pid (0)
		,mpPMT (NULL)
		,isUsed (false)
	{}
	virtual ~CProgramCache (void) {
		if (mpPMT) {
			delete mpPMT;
			mpPMT = NULL;
		}
	}

	uint16_t pid;
	parser::program_map_table *mpPMT;
	bool isUsed;
};

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
	parser::dsmcc *mpDsmcc;
	bool isUsed;
};

class CParser_test
{
public:
	class IParserListener {
	public:
		virtual ~IParserListener (void) {};
		virtual bool onTsAvailable (TS_HEADER *p_hdr, uint8_t *p_payload, size_t payload_size) = 0;
	};

public:
	CParser_test (void);
	CParser_test (IParserListener *p_Listener);
	virtual ~CParser_test (void);

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
	int parse_remain_len_;

	uint8_t inner_buff_ [INNER_BUFF_SIZE];



	parser::program_association_table mPAT;
	parser::program_association_table::table mPatTables [4 * 8];
	parser::time_offset_table mTOT;
	parser::event_information_table mEIT_H;
	parser::event_information_table mEIT_M;
	parser::event_information_table mEIT_L;
	parser::network_information_table mNIT;
	parser::service_description_table mSDT;
	parser::running_status_table mRST;
	parser::broadcaster_information_table mBIT;

	CProgramCache mProgramCaches [16];
	CDsmccControl mDsmccCtls [256];
};

#endif
