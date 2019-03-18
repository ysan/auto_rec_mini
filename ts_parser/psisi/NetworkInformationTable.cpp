#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "NetworkInformationTable.h"
#include "Utils.h"


CNetworkInformationTable::CNetworkInformationTable (void)
{
	mTables.clear();
}

CNetworkInformationTable::CNetworkInformationTable (uint8_t fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CNetworkInformationTable::~CNetworkInformationTable (void)
{
	clear();
}

void CNetworkInformationTable::onSectionCompleted (const CSectionInfo *pCompSection)
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

	dumpTable (pTable);

}

bool CNetworkInformationTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();
	pTable->reserved_future_use_2 = (*p >> 4) & 0x0f;
	pTable->network_descriptors_length = (*p & 0x0f) << 8 | *(p+1);

	p += NIT_FIX_LEN;

	int n = (int)pTable->network_descriptors_length;
	while (n > 0) {
		CDescriptor desc (p);
		if (!desc.isValid) {
			_UTL_LOG_W ("invalid NIT desc");
			return false;
		}
		pTable->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	pTable->reserved_future_use_3 = (*p >> 4) & 0x0f;
	pTable->transport_stream_loop_length = (*p & 0x0f) << 8 | *(p+1);

	p += NIT_FIX2_LEN;

	int streamLen = (int) (pTable->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - NIT_FIX_LEN - pTable->network_descriptors_length - NIT_FIX2_LEN);
	if (streamLen <= NIT_STREAM_FIX_LEN) {
		_UTL_LOG_W ("invalid NIT stream");
		return false;
	}

	if (streamLen != pTable->transport_stream_loop_length) {
		_UTL_LOG_W ("invalid NIT stream");
		return false;
	}

	while (streamLen > 0) {

		CTable::CStream strm ;

		strm.transport_stream_id = *p << 8 | *(p+1);
		strm.original_network_id = (*(p+2) << 8) | *(p+3);
		strm.reserved_future_use = (*(p+4) >> 4) & 0x0f;
		strm.transport_descriptors_length = (*(p+4) & 0x0f) << 8 | *(p+5);

		p += NIT_STREAM_FIX_LEN;

		int n = (int)strm.transport_descriptors_length;
		while (n > 0) {
			CDescriptor desc (p);
			if (!desc.isValid) {
				_UTL_LOG_W ("invalid NIT desc2");
				return false;
			}
			strm.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		streamLen -= (NIT_STREAM_FIX_LEN + strm.transport_descriptors_length) ;
		if (streamLen < 0) {
			_UTL_LOG_W ("invalid NIT stream");
			return false;
		}

		pTable->streams.push_back (strm);
	}

	return true;
}

void CNetworkInformationTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTables);

	mTables.push_back (pTable);
}

void CNetworkInformationTable::releaseTables (void)
{
	if (mTables.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables);

	std::vector<CTable*>::iterator iter = mTables.begin(); 
	for (; iter != mTables.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	mTables.clear();
}

void CNetworkInformationTable::dumpTables (void)
{
	if (mTables.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables);

	std::vector<CTable*>::const_iterator iter = mTables.begin(); 
	for (; iter != mTables.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable (pTable);
	}
}

void CNetworkInformationTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                         [0x%02x]\n", pTable->header.table_id);
	_UTL_LOG_I ("network_id                       [0x%04x]\n", pTable->header.table_id_extension);
	_UTL_LOG_I ("network_descriptors_length       [%d]\n", pTable->network_descriptors_length);

	std::vector<CDescriptor>::const_iterator iter_desc = pTable->descriptors.begin();
	for (; iter_desc != pTable->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
	}

	_UTL_LOG_I ("transport_stream_loop_length     [%d]\n", pTable->transport_stream_loop_length);

	std::vector<CTable::CStream>::const_iterator iter_strm = pTable->streams.begin();
	for (; iter_strm != pTable->streams.end(); ++ iter_strm) {
		_UTL_LOG_I ("\n--  stream  --\n");
		_UTL_LOG_I ("transport_stream_id          [0x%04x]\n", iter_strm->transport_stream_id);
		_UTL_LOG_I ("original_network_id          [0x%04x]\n", iter_strm->original_network_id);
		_UTL_LOG_I ("transport_descriptors_length [%d]\n", iter_strm->transport_descriptors_length);

		std::vector<CDescriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
		for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void CNetworkInformationTable::clear (void)
{
	releaseTables ();
//	detachAllSectionList ();
}

CNetworkInformationTable::CReference CNetworkInformationTable::reference (void)
{
	CReference ref (&mTables, &mMutexTables);
	return ref;
}

