#ifndef _CONDITIONAL_ACCESS_TABLE_H_
#define _CONDITIONAL_ACCESS_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "SectionParser.h"
#include "DescriptorDefs.h"


class CConditionalAccessTable : public CSectionParser
{
public:
	class CTable {
	public:
		CTable (void)
		{
			descriptors.clear();
		}
		virtual ~CTable (void)
		{
			descriptors.clear();
		}

		ST_SECTION_HEADER header;
		std::vector <CDescriptor> descriptors;
	};

public:
	CConditionalAccessTable (void);
	explicit CConditionalAccessTable (int fifoNum);
	virtual ~CConditionalAccessTable (void);


	// CSectionParser
	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpTables (void);
	void dumpTable (const CTable* pTable) const;
	void clear (void);

	std::vector <CTable*> * getTables (void) {
		return &mTables;
	}

private:
	bool parse (const CSectionInfo *pCompSection, CTable* pOutTable);
	void appendTable (CTable *pTable);
	void releaseTables (void);


	std::vector <CTable*> mTables;
};

#endif
