#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ConditionalAccessTable.h"
#include "Utils.h"


CConditionalAccessTable::CConditionalAccessTable (void)
{
	mTables.clear();
}

CConditionalAccessTable::CConditionalAccessTable (int fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CConditionalAccessTable::~CConditionalAccessTable (void)
{
	clear();
}

void CConditionalAccessTable::onSectionCompleted (const CSectionInfo *pCompSection)
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
		dumpTable (pTable);
	}
}

bool CConditionalAccessTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();

	int n = (int) (pTable->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN);

	while (n > 0) {
		CDescriptor desc (p);
		if (!desc.isValid) {
			_UTL_LOG_W ("invalid CAT desc");
			return false;
		}
		pTable->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	return true;
}

void CConditionalAccessTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	mTables.push_back (pTable);
}

void CConditionalAccessTable::releaseTables (void)
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

void CConditionalAccessTable::dumpTables (void)
{
	if (mTables.size() == 0) {
		return;
	}

	std::vector<CTable*>::const_iterator iter = mTables.begin(); 
	for (; iter != mTables.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable (pTable);
	}
}

void CConditionalAccessTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	pTable->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                         [0x%02x]\n", pTable->header.table_id);

	std::vector<CDescriptor>::const_iterator iter_desc = pTable->descriptors.begin();
	for (; iter_desc != pTable->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
	}

	_UTL_LOG_I ("\n");
}

void CConditionalAccessTable::clear (void)
{
	releaseTables ();

//	detachAllSectionList ();
	// detachAllSectionList in parser loop
	asyncDelete ();
}
