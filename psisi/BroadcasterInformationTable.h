#ifndef _BROADCASTER_INFORMATION_TABLE_H_
#define _BROADCASTER_INFORMATION_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "SectionParser.h"
#include "DescriptorDefs.h"


#define BIT_FIX_LEN				(2) // reserved_future_use        3  bslbf
									// broadcast_view_propriety   1  uimsbf
									// first_descriptors_length  12  uimsbf

#define BIT_BROADCASTER_FIX_LEN	(3) // broadcaster_id                   8  uimsbf
									// reserved_future_use              4  bslbf
									// broadcaster_descriptors_length  12  uimsbf


class CBroadcasterInformationTable : public CSectionParser
{
public:
	class CTable {
	public:
		class CBroadcaster {
		public:
			CBroadcaster (void)
				:broadcaster_id (0)
				,reserved_future_use (0)
				,broadcaster_descriptors_length (0)
			{
				descriptors.clear();
			}
			virtual ~CBroadcaster (void) {}

			uint8_t broadcaster_id;
			uint8_t reserved_future_use;
			uint16_t broadcaster_descriptors_length;
			std::vector <CDescriptor> descriptors;
		};

	public:
		CTable (void)
			:reserved_future_use_2 (0)
			,broadcast_view_propriety (0)
			,first_descriptors_length (0)
		{
			descriptors.clear();
			broadcasters.clear();
		}
		virtual ~CTable (void) {}

		ST_SECTION_HEADER header;
		uint8_t reserved_future_use_2;
		uint8_t broadcast_view_propriety;
		uint16_t first_descriptors_length;
		std::vector <CDescriptor> descriptors;
		std::vector <CBroadcaster> broadcasters;
	};
public:
	CBroadcasterInformationTable (void);
	virtual ~CBroadcasterInformationTable (void);

	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpTables (void) const;
	void dumpTable (const CTable* pTable) const;
	void clear (void);

private:
	bool parse (const CSectionInfo *pCompSection, CTable* pOutTable);
	void releaseTables (void);

	std::vector <CTable*> mTables;

};

#endif
