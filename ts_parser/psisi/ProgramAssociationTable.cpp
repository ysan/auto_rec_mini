#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ProgramAssociationTable.h"
#include "Utils.h"



CProgramAssociationTable::CProgramAssociationTable (void)
{
	mTables.clear();
}

CProgramAssociationTable::CProgramAssociationTable (int fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CProgramAssociationTable::~CProgramAssociationTable (void)
{
	clear();
}

void CProgramAssociationTable::onSectionCompleted (const CSectionInfo *pCompSection)
{
	if (!pCompSection) {
		return ;
	}

	CTable *pTable = new CTable ();
	if (!parse (pCompSection, pTable)) {
		delete pTable;
		pTable = NULL;
		detachSectionList (pCompSection);
		return ;
	}

	appendTable (pTable);

	// debug dump
	if (CUtils::getLogLevel() <= EN_LOG_LEVEL_D) {
//TODO mutex
		std::lock_guard<std::mutex> lock (mMutexTables);
		dumpTable (pTable);
	}

}

bool CProgramAssociationTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();

	int dataPartLen = (int)pCompSection->getDataPartLen ();
	if (dataPartLen < 4) {
		_UTL_LOG_E ("invalid PAT  dataPartLen=[%d]\n", dataPartLen);
		return false;
	}

	if ((dataPartLen % 4) != 0) {
		// 1ループの大きさは4の倍数
		_UTL_LOG_E ("invalid PAT (not multiples of 4)\n");
		return false;
	}

	while (dataPartLen > 0) {

		CTable::CProgram prog ;

		prog.program_number = ((*p << 8) | *(p+1)) & 0xffff;
		prog.program_map_PID = (((*(p+2) & 0x01f) << 8) | *(p+3)) & 0xffff;

		p += 4;

		dataPartLen -= 4 ;
		if (dataPartLen < 0) {
			_UTL_LOG_W ("invalid PAT program");
			return false;
		}

		pTable->programs.push_back (prog);
	}

	return true;
}

void CProgramAssociationTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTables);

	mTables.push_back (pTable);
}

void CProgramAssociationTable::releaseTables (void)
{
	std::lock_guard<std::mutex> lock (mMutexTables);

	if (mTables.size() == 0) {
		return;
	}

	std::vector<CTable*>::iterator iter = mTables.begin(); 
	for (; iter != mTables.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	mTables.clear();
}

void CProgramAssociationTable::dumpTables (void)
{
	std::lock_guard<std::mutex> lock (mMutexTables);

	if (mTables.size() == 0) {
		return;
	}

	std::vector<CTable*>::const_iterator iter = mTables.begin(); 
	for (; iter != mTables.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable (pTable);
	}
}

void CProgramAssociationTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	pTable->header.dump ();
	_UTL_LOG_I ("========================================\n");

	std::vector<CTable::CProgram>::const_iterator iter_prog = pTable->programs.begin();
	for (; iter_prog != pTable->programs.end(); ++ iter_prog) {
		_UTL_LOG_I ("\n--  program  --\n");
		_UTL_LOG_I ("program_number  [0x%04x]\n", iter_prog->program_number);
		if (iter_prog->program_number == 0) {
			_UTL_LOG_I ("network_PID     [0x%04x]\n", iter_prog->program_map_PID);
		} else {
			_UTL_LOG_I ("program_map_PID [0x%04x]\n", iter_prog->program_map_PID);
		}
	}

	_UTL_LOG_I ("\n");
}

void CProgramAssociationTable::clear (void)
{
	releaseTables ();

//	detachAllSectionList ();
	// detachAllSectionList in parser loop
	asyncDelete ();
}

CProgramAssociationTable::CReference CProgramAssociationTable::reference (void)
{
	CReference ref (&mTables, &mMutexTables);
	return ref;
}
