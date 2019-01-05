#ifndef _TIME_OFFSET_TABLE_H_
#define _TIME_OFFSET_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsCommon.h"
#include "SectionParser.h"


class CTimeOffsetTable : public CSectionParser
{
public:
	explicit CTimeOffsetTable (uint8_t fifoNum);
	virtual ~CTimeOffsetTable (void);

	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dump (void) const;
	void dump (const CSectionInfo *pSectInfo) const;

private:

};

#endif
