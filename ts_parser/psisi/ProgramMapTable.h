#ifndef _PROGRAM_MAP_TABLE_H_
#define _PROGRAM_MAP_TABLE_H_

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


#define PMT_FIX_LEN				(4) // reserved             3  bslbf
									// PCR_PID             13  uimsbf
									// reserved             4  bslbf
									// program_info_length 12  uimsbf

#define PMT_STREAM_FIX_LEN		(5) // stream_type          8  uimsbf
									// reserved             3  bslbf
									// elementary_PID      13  uimsbf
									// reserved             4  bslbf
									// ES_info_length      12  uimsbf


class CProgramMapTable : public CSectionParser
{
public:
	class CTable {
	public:
		class CStream {
		public:
			CStream (void)
				:stream_type (0)
				,reserved_1 (0)
				,elementary_PID (0)
				,reserved_2 (0)
				,ES_info_length (0)
			{
				descriptors.clear();
			}
			virtual ~CStream (void) {}

			uint8_t stream_type;
			uint8_t reserved_1;
			uint16_t elementary_PID;
			uint8_t reserved_2;
			uint16_t ES_info_length;
			std::vector <CDescriptor> descriptors;
		};

	public:
		CTable (void)
			:reserved_3 (0)
			,PCR_PID (0)
			,reserved_4 (0)
			,program_info_length (0)
		{
			descriptors.clear();
			streams.clear();
		}
		virtual ~CTable (void) {}

		ST_SECTION_HEADER header;
		uint8_t reserved_3;
		uint16_t PCR_PID;
		uint8_t reserved_4;
		uint16_t program_info_length;
		std::vector <CDescriptor> descriptors;
		std::vector <CStream> streams;
	};

public:
	CProgramMapTable (void);
	virtual ~CProgramMapTable (void);


	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpTables (void) const;
	void dumpTable (const CTable* pTable) const;
	void clear (void);

	const std::vector<CTable*> *getTables (void) const;

private:
	bool parse (const CSectionInfo *pCompSection, CTable* pOutTable);
	void releaseTables (void);

//	void dump (void) const;
//	void dump (const CSectionInfo *pCompSection) const;


	std::vector <CTable*> mTables;

};

#endif
