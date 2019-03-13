#ifndef _RUNNING_STATUS_TABLE_H_
#define _RUNNING_STATUS_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <mutex>

#include "Defs.h"
#include "TsCommon.h"
#include "SectionParser.h"
#include "DescriptorDefs.h"


#define RST_STATUS_FIX_LEN	(9) // transport_stream_id   16  uimsbf
								// original_network_id   16  uimsbf
								// service_id            16  uimsbf
								// event_id              16  uimsbf
								// reserved_future_use    5  bslbf
								// running_status         3  uimsbf


class CRunningStatusTable : public CSectionParser
{
public:
	class CTable {
	public:
		class CStatus {
		public:
			CStatus (void)
				:transport_stream_id (0)
				,original_network_id (0)
				,service_id (0)
				,event_id (0)
				,running_status (0) {}
			virtual ~CStatus (void) {}

			uint16_t transport_stream_id;
			uint16_t original_network_id;
			uint16_t service_id;
			uint16_t event_id;
			uint8_t running_status;
		};

	public:
		CTable (void)
		{
			statuses.clear();
		}
		virtual ~CTable (void)
		{
			statuses.clear();
		}

		ST_SECTION_HEADER header;
		std::vector <CStatus> statuses;
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
	CRunningStatusTable (void);
	explicit CRunningStatusTable (uint8_t fifoNum);
	virtual ~CRunningStatusTable (void);


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
