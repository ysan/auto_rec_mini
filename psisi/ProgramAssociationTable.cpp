#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ProgramAssociationTable.h"
#include "Utils.h"



CProgramAssociationTable::CProgramAssociationTable (void)
{
}

CProgramAssociationTable::~CProgramAssociationTable (void)
{
}


int CProgramAssociationTable::getTableNum (const CSectionInfo *pSectInfo) const
{
	if (!pSectInfo) {
		return 0;
	}

	int nDataPartLen = (int)pSectInfo->getDataPartLen ();
	if (nDataPartLen < 4) {
		_UTL_LOG_E ("invalid PAT data\n");
		return 0;
	}

	if ((nDataPartLen % 4) != 0) {
		// 1ループの大きさは4の倍数
		_UTL_LOG_E ("invalid PAT data (not multiples of 4)\n");
		return 0;
	}

	return nDataPartLen / 4;
}

int CProgramAssociationTable::getTableNum (void) const
{
	CSectionInfo *pLatest = getLatestCompleteSection ();
	if (!pLatest) {
		return 0;
	}

	return getTableNum (pLatest);
}

bool CProgramAssociationTable::getTable (CTable outArr[], int outArrSize) const
{
	CSectionInfo *pLatest = getLatestCompleteSection ();
	if (!pLatest) {
		return false;
	}

	if ((!outArr) || (outArrSize == 0)) {
		return false;
	}

	uint8_t *p = pLatest->getDataPartAddr();
	int n = (int) getTableNum (pLatest);
	if (n <= 0) {
		return false;
	}

	int i = 0;
	while (i != n && outArrSize != 0) {
		outArr[i].program_number = ((*p << 8) | *(p+1)) & 0xffff;

		if (outArr[i].program_number == 0) {
			outArr[i].network_PID = (((*(p+2) & 0x01f) << 8) | *(p+3)) & 0xffff;
		} else {
			outArr[i].program_map_PID = (((*(p+2) & 0x01f) << 8) | *(p+3)) & 0xffff;
			if (!outArr[i].mpPMT) {
				outArr[i].mpPMT = new CProgramMapTable();
			}
		}

		outArr[i].isUsed = true;

		p += 4;
		++ i;
		-- outArrSize;
	}

	if (n > 0 && outArrSize == 0) {
		_UTL_LOG_W ("warn:  ProgramAssociationTable is not get all.\n");
	}

	return true;
}

void CProgramAssociationTable::dumpTable (const CTable inArr[], int arrSize) const
{
	if ((!inArr) || (arrSize == 0)) {
		return ;
	}

	for (int i = 0; i < arrSize; ++ i) {
		_UTL_LOG_I ("program_number [0x%04x]  ", inArr[i].program_number);
		if (inArr[i].program_number == 0) {
			_UTL_LOG_I ("network_PID     [0x%04x]  ", inArr[i].network_PID);
		} else {
			_UTL_LOG_I ("program_map_PID [0x%04x]  ", inArr[i].program_map_PID);
		}
		_UTL_LOG_I ("PMTparser: [%p] ", inArr[i].mpPMT);
		_UTL_LOG_I ("%s", inArr[i].isUsed ? "used " : "noused");
		_UTL_LOG_I ("\n");
	}
}
