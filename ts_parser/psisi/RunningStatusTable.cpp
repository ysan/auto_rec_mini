#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "RunningStatusTable.h"
#include "Utils.h"


CRunningStatusTable::CRunningStatusTable (void)
{
	mTables.clear();
}

CRunningStatusTable::CRunningStatusTable (uint8_t fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CRunningStatusTable::~CRunningStatusTable (void)
{
	clear();
}

void CRunningStatusTable::onSectionCompleted (const CSectionInfo *pCompSection)
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

bool CRunningStatusTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();

	int statusLen = (int) pTable->header.section_length ;
	if (statusLen <= RST_STATUS_FIX_LEN) {
		_UTL_LOG_W ("invalid RST");
		return false;
	}

	while (statusLen > 0) {

		CTable::CStatus stt ;

		stt.transport_stream_id = *p << 8 | *(p+1);
		stt.original_network_id = *(p+2) << 8 | *(p+3);
		stt.service_id = *(p+4) << 8 | *(p+5);
		stt.event_id = *(p+6) << 8 | *(p+7);
		stt.running_status = *(p+8) & 0x07;

		p += RST_STATUS_FIX_LEN;

		statusLen -= RST_STATUS_FIX_LEN ;
		if (statusLen < 0) {
			_UTL_LOG_W ("invalid RST status");
			return false;
		}

		pTable->statuses.push_back (stt);
	}

	return true;
}

void CRunningStatusTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTables);

	mTables.push_back (pTable);
}

void CRunningStatusTable::releaseTables (void)
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

void CRunningStatusTable::dumpTables (void)
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

void CRunningStatusTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	std::vector<CTable::CStatus>::const_iterator iter_stt = pTable->statuses.begin();
	for (; iter_stt != pTable->statuses.end(); ++ iter_stt) {
		_UTL_LOG_I ("\n--  statuses  --\n");
		_UTL_LOG_I ("transport_stream_id [0x%04x]\n", iter_stt->transport_stream_id);
		_UTL_LOG_I ("original_network_id [0x%04x]\n", iter_stt->original_network_id);
		_UTL_LOG_I ("service_id          [0x%04x]\n", iter_stt->service_id);
		_UTL_LOG_I ("event_id            [0x%04x]\n", iter_stt->event_id);
		_UTL_LOG_I ("running_status      [0x%01x]\n", iter_stt->running_status);
	}

	_UTL_LOG_I ("\n");
}

void CRunningStatusTable::clear (void)
{
	releaseTables ();
//	detachAllSectionList ();
}

CRunningStatusTable::CReference CRunningStatusTable::reference (void)
{
	CReference ref (&mTables, &mMutexTables);
	return ref;
}
