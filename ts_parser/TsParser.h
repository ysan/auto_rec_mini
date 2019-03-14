#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"

#include "TsAribCommon.h"

#define INNER_BUFF_SIZE		(65535*5)


class CTsParser
{
public:
	class IParserListener {
	public:
		virtual ~IParserListener (void) {};
		virtual bool onTsPacketAvailable (TS_HEADER *p_hdr, uint8_t *p_payload, size_t payload_size) = 0;
	};

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

	IParserListener *mp_listener;

	uint8_t m_inner_buff [INNER_BUFF_SIZE];

};

#endif
