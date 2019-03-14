#ifndef _PROGRAM_ASSOCIATION_TABLE_H_
#define _PROGRAM_ASSOCIATION_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsAribCommon.h"
#include "SectionParser.h"
#include "ProgramMapTable.h"


class CProgramAssociationTable : public CSectionParser
{
public:
	class CTable {
	public:
		CTable (void)
			:program_number (0)
			,network_PID (0)
			,program_map_PID (0)
			,mpPMT (NULL)
			,isUsed (false)
		{}
		virtual ~CTable (void) {
			if (mpPMT) {
				delete mpPMT;
				mpPMT = NULL;
			}
		}

		uint16_t program_number;
		uint16_t network_PID; // if program_number == 0 then network_PID
		uint16_t program_map_PID;

		CProgramMapTable *mpPMT; // section parser for PMT
		bool isUsed;
	};

public:
	CProgramAssociationTable (void);
	explicit CProgramAssociationTable (uint8_t fifoNum);
	virtual ~CProgramAssociationTable (void);


	int getTableNum (void) const;
	bool getTable (CTable outArr[], int outArrSize) const;
	void dumpTable (const CTable inArr[], int arrSize) const;


private:
	int getTableNum (const CSectionInfo *pSectInfo) const;

};

#endif
