#ifndef _PROGRAM_ASSOCIATION_TABLE_H_
#define _PROGRAM_ASSOCIATION_TABLE_H_

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


class CProgramAssociationTable : public CSectionParser
{
public:
	class CTable {
	public:
		class CProgram {
		public:
			CProgram (void)
				:program_number (0)
				,program_map_PID (0)
			{}
			virtual ~CProgram (void) {}

			uint16_t program_number;
			uint16_t program_map_PID; // if program_number == 0 then network_PID
		};

	public:
		CTable (void)
		{
			programs.clear();
		}
		virtual ~CTable (void)
		{
			programs.clear();
		}

		ST_SECTION_HEADER header;
		std::vector <CProgram> programs;
	};

public:
	class CReference {
	public:
		CReference (void) {}
		CReference (const std::vector <CTable*> *pTables, std::recursive_mutex *pMutex)
			:mpTables (pTables)
			,mpMutex (pMutex)
		{}
		virtual ~CReference (void) {}

		const std::vector <CTable*> *mpTables;
		std::recursive_mutex *mpMutex;
	};

public:
	CProgramAssociationTable (void);
	explicit CProgramAssociationTable (int fifoNum);
	virtual ~CProgramAssociationTable (void);


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
	void releaseTable (CTable* pErase);
	bool refreshTableByVersionNumber (CTable* pNewTable);
	void refreshTablesByVersionNumber (CTable* pNewTable);


	std::vector <CTable*> mTables;
	std::recursive_mutex mMutexTables;

};

#endif
