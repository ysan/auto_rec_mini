#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <memory>

#include "CommonDataTable.h"
#include "Utils.h"


CCommonDataTable::CCommonDataTable (void)
{
	mTables.clear();
}

CCommonDataTable::CCommonDataTable (int fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CCommonDataTable::CCommonDataTable (size_t poolSize)
	:CSectionParser (poolSize)
{
	mTables.clear();
}

CCommonDataTable::~CCommonDataTable (void)
{
	clear();
}

void CCommonDataTable::onSectionCompleted (const CSectionInfo *pCompSection)
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

bool CCommonDataTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();
	pTable->original_network_id = *p << 8 | *(p+1);
	pTable->data_type = *(p+2);
	pTable->reserved_future_use = (*(p+3) & 0xf0) >> 4;
	pTable->descriptors_loop_length = (*(p+3) & 0x0f) << 8 | *(p+4);

	p += CDT_FIX_LEN;
	int n = (int)pTable->descriptors_loop_length;
	while (n > 0) {
		CDescriptor desc (p);
		if (!desc.isValid) {
			_UTL_LOG_W ("invalid CDT desc");
			return false;
		}
		pTable->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	if (pTable->data_type != 0x01) {
		return true;
	}

	int dataLen = (int) (pTable->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - CDT_FIX_LEN - pTable->descriptors_loop_length);
	if (dataLen <= CDT_DATA_FIX_LEN) {
		_UTL_LOG_W ("invalid CDT data");
		return false;
	}

	pTable->data.logo_type = *p;
	pTable->data.reserved_future_use_1 = (*(p+1) & 0xfe) >> 1;
	pTable->data.logo_id = (*(p+1) & 0x01) << 8 | *(p+2);
	pTable->data.reserved_future_use_2 = (*(p+3) & 0xf0) >> 4;
	pTable->data.logo_version = (*(p+3) & 0x0f) << 8 | *(p+4);
	pTable->data.data_size =  *(p+5) << 8 | *(p+6);

	if (dataLen != (CDT_DATA_FIX_LEN + pTable->data.data_size)) {
		_UTL_LOG_W ("invalid CDT data_byte");
		return false;
	}

	p += CDT_DATA_FIX_LEN;
	std::unique_ptr<uint8_t[]> up (new uint8_t[pTable->data.data_size]);
	memcpy (up.get(), p, pTable->data.data_size);
	pTable->data.data_byte.swap(up);

	return true;
}

void CCommonDataTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	mTables.push_back (pTable);
}

void CCommonDataTable::releaseTables (void)
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

void CCommonDataTable::dumpTables (void)
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

void CCommonDataTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	pTable->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                  [0x%02x]\n", pTable->header.table_id);
	_UTL_LOG_I ("download_data_id          [0x%04x]\n", pTable->header.table_id_extension);
	_UTL_LOG_I ("original_network_id       [0x%04x]\n", pTable->original_network_id);
	_UTL_LOG_I ("data_type                 [0x%02x]\n", pTable->data_type);
	_UTL_LOG_I ("descriptor_loop_length    [0x%04x]\n", pTable->descriptors_loop_length);

	std::vector<CDescriptor>::const_iterator iter_desc = pTable->descriptors.begin();
	for (; iter_desc != pTable->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
	}

	if (pTable->data_type == 0x01) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("logo_type       [0x%02x]\n", pTable->data.logo_type);
		_UTL_LOG_I ("logo_id         [0x%04x]\n", pTable->data.logo_id);
		_UTL_LOG_I ("logo_version    [0x%04x]\n", pTable->data.logo_version);
		_UTL_LOG_I ("data_size       [0x%04x]\n", pTable->data.data_size);
		_UTL_LOG_I ("data_byte\n");
		CUtils::dumper(pTable->data.data_byte.get(), pTable->data.data_size);
	}

	_UTL_LOG_I ("\n");
}

void CCommonDataTable::clear (void)
{
	releaseTables ();

//	detachAllSectionList ();
	// detachAllSectionList in parser loop
	asyncDelete ();
}
