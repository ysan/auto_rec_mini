#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ProgramMapTable.h"
#include "Utils.h"


CProgramMapTable::CProgramMapTable (void)
{
	mTables.clear();
}

CProgramMapTable::CProgramMapTable (int fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CProgramMapTable::~CProgramMapTable (void)
{
	clear();
}


void CProgramMapTable::onSectionCompleted (const CSectionInfo *pCompSection)
{
	if (!pCompSection) {
		return;
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
	if (CUtils::get_logger()->get_log_level() <= CLogger::level::debug) {
		dumpTable (pTable);
	}
}

bool CProgramMapTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();
	pTable->reserved_3 = (*p >> 5) & 0x07;
	pTable->PCR_PID = (*p & 0x1f) << 8 | *(p+1);
	pTable->reserved_4 = (*(p+2) >> 4) & 0x0f;
	pTable->program_info_length = (*(p+2) & 0x0f) << 8 | *(p+3);

	p += PMT_FIX_LEN;

	int n = (int)pTable->program_info_length;
	while (n > 0) {
		CDescriptor desc (p);
		if (!desc.isValid) {
			_UTL_LOG_W ("invalid PMT desc 1");
			_UTL_LOG_W ("program_info_length %d %d", (int)pTable->program_info_length, n);
			_UTL_LOG_W ("tag 0x%02x", *p);
			_UTL_LOG_W ("length %d", *(p+1));
			return false;
		}
		pTable->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	int streamLen = (int) (pTable->header.section_length - pTable->program_info_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - PMT_FIX_LEN);
	if (streamLen <= PMT_STREAM_FIX_LEN) {
		_UTL_LOG_W ("invalid PMT stream");
		return false;
	}

	while (streamLen > 0) {

		CTable::CStream strm ;

		strm.stream_type = *p;
		strm.reserved_1 = (*(p+1) >> 5) & 0x07;
		strm.elementary_PID = (*(p+1) & 0x1f) << 8 | *(p+2);
		strm.reserved_2 = (*(p+3) >> 4) & 0x0f;
		strm.ES_info_length = (*(p+3) & 0x0f) << 8 | *(p+4);

		p += PMT_STREAM_FIX_LEN;

		int n = (int)strm.ES_info_length ;
		while (n > 0) {
			CDescriptor desc (p);
			if (!desc.isValid) {
				_UTL_LOG_W ("invalid PMT desc 2");
				return false;
			}
			strm.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		streamLen -= (PMT_STREAM_FIX_LEN + strm.ES_info_length) ;
		if (streamLen < 0) {
			_UTL_LOG_W ("invalid PMT stream");
			return false;
		}

		pTable->streams.push_back (strm);
	}

	return true;
}

void CProgramMapTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	mTables.push_back (pTable);
}

void CProgramMapTable::releaseTables (void)
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

void CProgramMapTable::dumpTables (void)
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

void CProgramMapTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("PID [0x%04x]\n", this->getPid ());
	pTable->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("PCR_PID             [0x%04x]\n", pTable->PCR_PID);
	_UTL_LOG_I ("program_info_length [%d]\n", pTable->program_info_length);

	std::vector<CDescriptor>::const_iterator iter_desc = pTable->descriptors.begin();
	for (; iter_desc != pTable->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
	}

	std::vector<CTable::CStream>::const_iterator iter_strm = pTable->streams.begin();
	for (; iter_strm != pTable->streams.end(); ++ iter_strm) {
		_UTL_LOG_I ("\n--  stream  --\n");
		_UTL_LOG_I ("stream_type    [0x%02x]\n", iter_strm->stream_type);
		_UTL_LOG_I ("elementary_PID [0x%04x]\n", iter_strm->elementary_PID);
		_UTL_LOG_I ("ES_info_length [%d]\n", iter_strm->ES_info_length);

		std::vector<CDescriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
		for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void CProgramMapTable:: clear (void)
{
	releaseTables ();

//	detachAllSectionList ();
	// detachAllSectionList in parser loop
	asyncDelete ();
}

void CProgramMapTable:: forceClear (void)
{
	releaseTables ();

	detachAllSectionList ();
}
