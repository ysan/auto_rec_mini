#ifndef _NETWORK_INFORMATION_TABLE_H_
#define _NETWORK_INFORMATION_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <mutex>

#include "Defs.h"
#include "TsAribCommon.h"
#include "SectionParser.h"
#include "DescriptorDefs.h"


#define NIT_FIX_LEN			(2) // reserved_future_use_2        4  bslbf
								// network_descriptors_length  12  uimsbf

#define NIT_FIX2_LEN		(2) // reserved_future_use_3          4  bslbf
								// transport_stream_loop_length  12  uimsbf

#define NIT_STREAM_FIX_LEN	(6) // transport_stream_id      16  uimsbf
								// original_network_id      16  uimsbf
								// reserved_future_use       4  bslbf
								// descriptors_loop_length  12  uimsbf


class CNetworkInformationTable : public CSectionParser
{
public:
	class CTable {
	public:
		class CStream {
		public:
			CStream (void)
				:transport_stream_id (0)
				,original_network_id (0)
				,reserved_future_use (0)
				,transport_descriptors_length (0)
			{
				descriptors.clear();
			}
			virtual ~CStream (void)
			{
				descriptors.clear();
			}

			uint16_t transport_stream_id;
			uint16_t original_network_id;
			uint8_t reserved_future_use;
			uint16_t transport_descriptors_length;
			std::vector <CDescriptor> descriptors;
		};

	public:
		CTable (void)
			:reserved_future_use_2 (0)
			,network_descriptors_length (0)
			,reserved_future_use_3 (0)
			,transport_stream_loop_length (0)
		{
			descriptors.clear();
			streams.clear();
		}
		virtual ~CTable (void)
		{
			descriptors.clear();
			streams.clear();
		}

		ST_SECTION_HEADER header;
		uint8_t reserved_future_use_2;
		uint16_t network_descriptors_length;
		std::vector <CDescriptor> descriptors;
		uint8_t reserved_future_use_3;
		uint16_t transport_stream_loop_length;
		std::vector <CStream> streams;
	};

public:
	class CReference {
	public:
		CReference (void) {}
		CReference (const std::vector <CTable*> *pTables, std::mutex *pMutex)
			:mpTables (pTables)
			,mpMutex (pMutex)
		{}
		virtual ~CReference (void) {}

		const std::vector <CTable*> *mpTables;
		std::mutex *mpMutex;
	};

public:
	CNetworkInformationTable (void);
	explicit CNetworkInformationTable (int fifoNum);
	virtual ~CNetworkInformationTable (void);


	// CSectionParser
	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpTables (void);
	void dumpTable (const CTable* pTable) const;
	void clear (void);

	CReference reference (void);

private:
	bool parse (const CSectionInfo *pCompSection, CTable* pOutTable);
	void appendTable (CTable *pTable);
	void releaseTables (void);


	std::vector <CTable*> mTables;
	std::mutex mMutexTables;
};

#endif
