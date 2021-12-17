#ifndef _COMMON_DATA_TABLE_H_
#define _COMMON_DATA_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <memory>

#include "Defs.h"
#include "TsAribCommon.h"
#include "SectionParser.h"
#include "DescriptorDefs.h"


#define CDT_FIX_LEN			(5) // original_etwork_id          16  uimsbf
								// data_type                    8  uimsbf
								// reserved_future_use          4  bslbf
								// descriptors_loop_length     12  uimsbf

#define CDT_DATA_FIX_LEN	(7) // logo_type                    8  uimsbf
								// reserved_future_use          7  bslbf
								// logo_id                      9  uimsbf
								// reserved_future_use          4  bslbf
								// logo_version                12  uimsbf
								// data_size                   16  uimsbf


class CCommonDataTable : public CSectionParser
{
public:
	class CTable {
	public:
		class CData {
		public:
			CData (void)
				:logo_type (0)
				,reserved_future_use_1 (0)
				,logo_id (0)
				,reserved_future_use_2 (0)
				,logo_version (0)
				,data_size (0)
			{
				data_byte.reset();
			}
			virtual ~CData (void)
			{
				data_byte.reset();
			}

			uint8_t logo_type;
			uint8_t reserved_future_use_1;
			uint16_t logo_id;
			uint8_t reserved_future_use_2;
			uint16_t logo_version;
			uint16_t data_size;
			std::unique_ptr<uint8_t[]> data_byte;
		};

	public:
		CTable (void)
			:original_network_id (0)
			,data_type (0)
			,reserved_future_use (0)
			,descriptors_loop_length (0)
		{
			descriptors.clear();
		}
		virtual ~CTable (void)
		{
			descriptors.clear();
		}

		ST_SECTION_HEADER header;
		uint16_t original_network_id;
		uint8_t data_type;
		uint8_t reserved_future_use;
		uint16_t descriptors_loop_length;
		std::vector <CDescriptor> descriptors;
		CData data;
	};

public:
	CCommonDataTable (void);
	explicit CCommonDataTable (int fifoNum);
	explicit CCommonDataTable (size_t poolSize);
	virtual ~CCommonDataTable (void);


	// CSectionParser
	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpTables (void);
	void dumpTable (const CTable* pTable) const;
	void clear (void);

	std::vector <CTable*> *getTables (void) {
		return & mTables;
	}

private:
	bool parse (const CSectionInfo *pCompSection, CTable* pOutTable);
	void appendTable (CTable *pTable);
	void releaseTables (void);


	std::vector <CTable*> mTables;
};

#endif
