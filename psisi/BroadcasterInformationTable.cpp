#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "BroadcasterInformationTable.h"
#include "Utils.h"


CBroadcasterInformationTable::CBroadcasterInformationTable (void)
{
	mTables.clear();
}

CBroadcasterInformationTable::~CBroadcasterInformationTable (void)
{
}

void CBroadcasterInformationTable::onSectionCompleted (const CSectionInfo *pCompSection)
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

	mTables.push_back (pTable);
	dumpTable (pTable);

}

bool CBroadcasterInformationTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();
	pTable->reserved_future_use_2 = (*p >> 5) & 0x07;
	pTable->broadcast_view_propriety = (*p >> 4) & 0x01;
	pTable->first_descriptors_length = (*p & 0x0f) << 8 | *(p+1);

	p += BIT_FIX_LEN;

	int n = (int)pTable->first_descriptors_length;
	while (n > 0) {
		CDescriptor desc (p);
		if (!desc.isValid) {
			_UTL_LOG_W ("invalid BIT desc");
			return false;
		}
		pTable->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	int barodcasterLen = (int) (pTable->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - BIT_FIX_LEN - pTable->first_descriptors_length);
	if (barodcasterLen <= BIT_BROADCASTER_FIX_LEN) {
		_UTL_LOG_W ("invalid BIT broadcaster");
		return false;
	}

	while (barodcasterLen > 0) {

		CTable::CBroadcaster brd;

		brd.broadcaster_id = *p;
		brd.reserved_future_use = (*(p+1) >> 4) & 0x0f;
		brd.broadcaster_descriptors_length = (*(p+1) & 0xf) << 8 | *(p+2);

		p += BIT_BROADCASTER_FIX_LEN;

		int n = (int)brd.broadcaster_descriptors_length;
		while (n > 0) {
			CDescriptor desc (p);
			if (!desc.isValid) {
				_UTL_LOG_W ("invalid BIT desc2");
				return false;
			}
			brd.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		barodcasterLen -= (BIT_BROADCASTER_FIX_LEN + brd.broadcaster_descriptors_length) ;
		if (barodcasterLen < 0) {
			_UTL_LOG_W ("invalid BIT broadcaster");
			return false;
		}

		pTable->broadcasters.push_back (brd);
	}

	return true;
}

void CBroadcasterInformationTable::releaseTables (void)
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

void CBroadcasterInformationTable::dumpTables (void) const
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

void CBroadcasterInformationTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}
	
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                           [0x%02x]\n", pTable->header.table_id);
	_UTL_LOG_I ("broadcast_view_propriety           [0x%02x]\n", pTable->broadcast_view_propriety);
	_UTL_LOG_I ("first_descriptors_length           [%d]\n", pTable->first_descriptors_length);

	std::vector<CDescriptor>::const_iterator iter_desc = pTable->descriptors.begin();
	for (; iter_desc != pTable->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
	}

	std::vector<CTable::CBroadcaster>::const_iterator iter_brd = pTable->broadcasters.begin();
	for (; iter_brd != pTable->broadcasters.end(); ++ iter_brd) {
		_UTL_LOG_I ("\n--  broadcaster  --\n");
		_UTL_LOG_I ("broadcaster_id                 [0x%02x]\n", iter_brd->broadcaster_id);
		_UTL_LOG_I ("broadcaster_descriptors_length [%d]\n", iter_brd->broadcaster_descriptors_length);

		std::vector<CDescriptor>::const_iterator iter_desc = iter_brd->descriptors.begin();
		for (; iter_desc != iter_brd->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("========================================\n");
}

void CBroadcasterInformationTable::clear (void)
{
	releaseTables ();
	detachAllSectionList ();
}
