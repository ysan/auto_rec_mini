#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "EventInformationTable.h"
#include "Utils.h"


CEventInformationTable::CEventInformationTable (size_t poolSize)
	:CSectionParser (poolSize)
	,m_type (0)
	,m_isNeedParseSchedule (false)
{
	mTables_pf.clear();
	mTables_sch.clear();
}

CEventInformationTable::CEventInformationTable (size_t poolSize, uint8_t fifoNum)
	:CSectionParser (poolSize, fifoNum)
	,m_type (0)
	,m_isNeedParseSchedule (false)
{
	mTables_pf.clear();
	mTables_sch.clear();
}

CEventInformationTable::~CEventInformationTable (void)
{
	clear_pf();
	clear_sch();
}


void CEventInformationTable::onSectionCompleted (const CSectionInfo *pCompSection)
{
	if (!pCompSection) {
		return ;
	}


	// pf or schedule ?
	uint8_t _tbl_id = pCompSection->getHeader()->table_id;
	if (_tbl_id == TBLID_EIT_PF_A || _tbl_id == TBLID_EIT_PF_O) {
		m_type = 0;

	} else if (
		(_tbl_id >= TBLID_EIT_SCH_A && _tbl_id <= TBLID_EIT_SCH_A + 0xf) ||
		(_tbl_id >= TBLID_EIT_SCH_O && _tbl_id <= TBLID_EIT_SCH_O + 0xf)
	) {
		m_type = 1;

	} else {
		_UTL_LOG_W ("EIT unknown table_id [0x%02x]", _tbl_id);
	}


	// eit schedule judge
	if (!m_isNeedParseSchedule) {
		if (
			(_tbl_id >= TBLID_EIT_SCH_A && _tbl_id <= TBLID_EIT_SCH_A + 0xf) ||
			(_tbl_id >= TBLID_EIT_SCH_O && _tbl_id <= TBLID_EIT_SCH_O + 0xf)
		) {
			detachSectionList (pCompSection);
			return ;
		}
	}


	CTable *pTable = new CTable ();
	if (!parse (pCompSection, pTable)) {
		delete pTable;
		pTable = NULL;
		detachSectionList (pCompSection);
		return ;
	}

	if (pTable->header.table_id == TBLID_EIT_PF_A || pTable->header.table_id == TBLID_EIT_PF_O) {
		appendTable_pf (pTable);

		// debug dump
		if (CUtils::getLogLevel() <= EN_LOG_LEVEL_D) {
			dumpTables_pf_simple ();
			dumpTable (pTable);
		}

	} else if (
		(pTable->header.table_id >= TBLID_EIT_SCH_A && pTable->header.table_id <= TBLID_EIT_SCH_A + 0xf) ||
		(pTable->header.table_id >= TBLID_EIT_SCH_O && pTable->header.table_id <= TBLID_EIT_SCH_O + 0xf)
	) {
		appendTable_sch (pTable);

		// debug dump
		if (CUtils::getLogLevel() <= EN_LOG_LEVEL_D) {
			dumpTables_sch_simple ();
			dumpTable (pTable);
		}
	}

}

bool CEventInformationTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();
	pTable->transport_stream_id = *p << 8 | *(p+1);
	pTable->original_network_id = *(p+2) << 8 | *(p+3);
	pTable->segment_last_section_number = *(p+4);
	pTable->last_table_id = *(p+5);

	p += EIT_FIX_LEN;

	int eventLen = (int) (pTable->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - EIT_FIX_LEN);
	if (eventLen == 0 || eventLen == EIT_EVENT_FIX_LEN) {
		// allow
	} else if (eventLen < EIT_EVENT_FIX_LEN) {
		_UTL_LOG_W ("invalid EIT event (eventLen=%d)", eventLen);
		return false;
	}

	while (eventLen > 0) {

		CTable::CEvent ev ;

		ev.event_id = *p << 8 | *(p+1);
		memcpy (ev.start_time, p+2, 5);
		memcpy (ev.duration, p+7, 3);
		ev.running_status = (*(p+10) >> 5) & 0x07;
		ev.free_CA_mode = (*(p+10) >> 4) & 0x01;
		ev.descriptors_loop_length = (*(p+10) & 0x0f) << 8 | *(p+11);

		p += EIT_EVENT_FIX_LEN;

		int n = (int)ev.descriptors_loop_length;
		while (n > 0) {
			CDescriptor desc (p);
			if (!desc.isValid) {
				_UTL_LOG_W ("invalid EIT desc");
				return false;
			}
			ev.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		eventLen -= (EIT_EVENT_FIX_LEN + ev.descriptors_loop_length) ;
		if (eventLen < 0) {
			_UTL_LOG_W ("invalid EIT event");
			return false;
		}

		pTable->events.push_back (ev);
	}

	return true;
}

void CEventInformationTable::appendTable_pf (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_pf);

	mTables_pf.push_back (pTable);
}

void CEventInformationTable::appendTable_sch (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_sch);

	mTables_sch.push_back (pTable);
}

void CEventInformationTable::releaseTables_pf (void)
{
	if (mTables_pf.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_pf);

	std::vector<CTable*>::iterator iter = mTables_pf.begin(); 
	for (; iter != mTables_pf.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	mTables_pf.clear();
}

void CEventInformationTable::releaseTables_pf (CTable *pErase)
{
	if (!pErase) {
		return;
	}
	if (mTables_pf.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_pf);

	std::vector<CTable*>::iterator iter = mTables_pf.begin(); 
	for (; iter != mTables_pf.end(); ++ iter) {
		if (*iter == pErase) {
			delete (*iter);
			(*iter) = NULL;
			iter = mTables_pf.erase(iter);
			break;
		}
	}
}

void CEventInformationTable::releaseTables_sch (void)
{
	if (mTables_sch.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_sch);

	std::vector<CTable*>::iterator iter = mTables_sch.begin(); 
	for (; iter != mTables_sch.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	mTables_sch.clear();
}

void CEventInformationTable::releaseTables_sch (CTable *pErase)
{
	if (!pErase) {
		return;
	}
	if (mTables_sch.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_sch);

	std::vector<CTable*>::iterator iter = mTables_sch.begin(); 
	for (; iter != mTables_sch.end(); ++ iter) {
		if (*iter == pErase) {
			delete (*iter);
			(*iter) = NULL;
			iter = mTables_sch.erase(iter);
			break;
		}
	}
}

void CEventInformationTable::dumpTables_pf (void)
{
	if (mTables_pf.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_pf);

	std::vector<CTable*>::const_iterator iter = mTables_pf.begin(); 
	for (; iter != mTables_pf.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable (pTable);
	}
}

void CEventInformationTable::dumpTables_sch (void)
{
	if (mTables_sch.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_sch);

	std::vector<CTable*>::const_iterator iter = mTables_sch.begin(); 
	for (; iter != mTables_sch.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable (pTable);
	}
}

void CEventInformationTable::dumpTables_pf_simple (void)
{
	if (mTables_pf.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_pf);

	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<CTable*>::const_iterator iter = mTables_pf.begin(); 
	for (; iter != mTables_pf.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable_simple (pTable);
	}
}

void CEventInformationTable::dumpTables_sch_simple (void)
{
	if (mTables_sch.size() == 0) {
		return;
	}

	std::lock_guard<std::mutex> lock (mMutexTables_sch);

	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<CTable*>::const_iterator iter = mTables_sch.begin(); 
	for (; iter != mTables_sch.end(); ++ iter) {
		CTable *pTable = *iter;
		dumpTable_simple (pTable);
	}
}

void CEventInformationTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}
	
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                    [0x%02x]\n", pTable->header.table_id);
	_UTL_LOG_I ("service_id                  [0x%04x]\n", pTable->header.table_id_extension);
	_UTL_LOG_I ("transport_stream_id         [0x%04x]\n", pTable->transport_stream_id);
	_UTL_LOG_I ("original_network_id         [0x%04x]\n", pTable->original_network_id);
	_UTL_LOG_I ("segment_last_section_number [0x%02x]\n", pTable->segment_last_section_number);
	_UTL_LOG_I ("last_table_id               [0x%02x]\n", pTable->last_table_id);

	std::vector<CTable::CEvent>::const_iterator iter_event = pTable->events.begin();
	for (; iter_event != pTable->events.end(); ++ iter_event) {
		_UTL_LOG_I ("\n--  event  --\n");
		_UTL_LOG_I ("event_id                [0x%04x]\n", iter_event->event_id);
		char szStime [32];
		memset (szStime, 0x00, sizeof(szStime));
		CTsAribCommon::getStrEpoch (CTsAribCommon::getEpochFromMJD (iter_event->start_time), "%Y/%m/%d %H:%M:%S", szStime, sizeof(szStime));
		_UTL_LOG_I ("start_time              [%s]\n", szStime);
		char szDuration [32];
		memset (szDuration, 0x00, sizeof(szDuration));
		CTsAribCommon::getStrSecond (CTsAribCommon::getSecFromBCD (iter_event->duration), szDuration, sizeof(szDuration));
		_UTL_LOG_I ("duration                [%s]\n", szDuration);
		_UTL_LOG_I ("running_status          [0x%02x]\n", iter_event->running_status);
		_UTL_LOG_I ("free_CA_mode            [0x%02x]\n", iter_event->free_CA_mode);
		_UTL_LOG_I ("descriptors_loop_length [%d]\n", iter_event->descriptors_loop_length);

		std::vector<CDescriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
		for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void CEventInformationTable::dumpTable_simple (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}
	
	char buf[128] = {0};
	snprintf (buf, sizeof(buf) -1, "event_id:");
	std::vector<CTable::CEvent>::const_iterator iter_event = pTable->events.begin();
	for (; iter_event != pTable->events.end(); ++ iter_event) {
		int len = strlen (buf);
		if (len > 120) {
			return;
		}
		snprintf (buf+len, sizeof(buf) -1, "[0x%04x]", iter_event->event_id);
	}

	_UTL_LOG_I (
		"tblid:[0x%02x][%s] tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] ver:[0x%02x] curind:[%d] %s\n",
		pTable->header.table_id,
		pTable->header.table_id == 0x4e ? "PF,A " :
			pTable->header.table_id == 0x4f ? "PF,O " :
			pTable->header.table_id >= 0x50 && pTable->header.table_id < 0x58 ? "Sh,A " :
			pTable->header.table_id == 0x58 ? "Sh,AE" :
			pTable->header.table_id >= 0x60 && pTable->header.table_id < 0x68 ? "Sh,O " :
			pTable->header.table_id == 0x68 ? "Sh,OE" :
			"unsup",
		pTable->transport_stream_id,
		pTable->original_network_id,
		pTable->header.table_id_extension,
		pTable->header.version_number,
		pTable->header.current_next_indicator,
		buf
	);
}

void CEventInformationTable::clear_pf (void)
{
	releaseTables_pf ();
//	detachAllSectionList ();
}

void CEventInformationTable::clear_pf (CTable *pErase)
{
	releaseTables_pf (pErase);
}

void CEventInformationTable::clear_sch (void)
{
	releaseTables_sch ();
//	detachAllSectionList ();
}

void CEventInformationTable::clear_sch (CTable *pErase)
{
	releaseTables_sch (pErase);
}

CEventInformationTable::CReference CEventInformationTable::reference_pf (void)
{
    CReference ref (&mTables_pf, &mMutexTables_pf);
    return ref;
}

CEventInformationTable::CReference CEventInformationTable::reference_sch (void)
{
    CReference ref (&mTables_sch, &mMutexTables_sch);
    return ref;
}

