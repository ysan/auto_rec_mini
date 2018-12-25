#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Dsmcc.h"
#include "Utils.h"


CDsmcc::CDsmcc (size_t poolSize)
	:CSectionParser (poolSize, EN_SECTION_TYPE__DSMCC)
{
	mTables.clear();
}

CDsmcc::~CDsmcc (void)
{
}

void CDsmcc::onSectionCompleted (const CSectionInfo *pCompSection)
{
	if (!pCompSection) {
		return ;
	}

//	CTable *pTable = new CTable ();
//	if (!parse (pCompSection, pTable)) {
//		delete pTable;
//		pTable = NULL;
//		detachSectionList (pCompSection);
//		return ;
//	}

//	mTables.push_back (pTable);
//	dumpTable (pTable);

}

bool CDsmcc::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}



	return true;
}

void CDsmcc::releaseTables (void)
{
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

void CDsmcc::dumpTables (void) const
{
	if (mTables.size() == 0) {
		return;
	}

//	std::vector<CTable*>::const_iterator iter = mTables.begin(); 
//	for (; iter != mTables.end(); ++ iter) {
//		CTable *pTable = *iter;
//		dumpTable (pTable);
//	}
}

void CDsmcc::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}
	
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("========================================\n");
}

void CDsmcc::clear (void)
{
	releaseTables ();
	detachAllSectionList ();
}
