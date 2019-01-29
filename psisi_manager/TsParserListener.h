#ifndef _TS_PARSER_LISTENER_H_
#define _TS_PARSER_LISTENER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Utils.h"

#include "TsParser.h"

#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "TimeOffsetTable.h"
#include "EventInformationTable.h"
#include "NetworkInformationTable.h"
#include "ServiceDescriptionTable.h"
#include "RunningStatusTable.h"
#include "BroadcasterInformationTable.h"


class CTsParserListener : public CTsParser::IParserListener
{
public:
	CTsParserListener (void);
	virtual ~CTsParserListener (void);

	CEventInformationTable mEIT_H;


private:
	// CTsParser::IParserListener
	bool onTsAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size) override;


};

#endif
