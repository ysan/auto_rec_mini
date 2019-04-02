#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ServiceDescriptionTable.h"
#include "Utils.h"


CServiceDescriptionTable::CServiceDescriptionTable (void)
{
	mTables.clear();
}

CServiceDescriptionTable::CServiceDescriptionTable (int fifoNum)
	:CSectionParser (fifoNum)
{
	mTables.clear();
}

CServiceDescriptionTable::~CServiceDescriptionTable (void)
{
	clear();
}

void CServiceDescriptionTable::onSectionCompleted (const CSectionInfo *pCompSection)
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

bool CServiceDescriptionTable::parse (const CSectionInfo *pCompSection, CTable* pOutTable)
{
	if (!pCompSection || !pOutTable) {
		return false;
	}

	uint8_t *p = NULL; // work
	CTable* pTable = pOutTable;

	pTable->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();
	pTable->original_network_id = *p << 8 | *(p+1);
	pTable->reserved_future_use_2 = *(p+2);

	p += SDT_FIX_LEN;

	int serviceLen = (int) (pTable->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - SDT_FIX_LEN);
	if (serviceLen <= SDT_SERVICE_FIX_LEN) {
		_UTL_LOG_W ("invalid SDT service");
		return false;
	}

	while (serviceLen > 0) {

		CTable::CService svc ;

		svc.service_id = *p << 8 | *(p+1);
		svc.reserved_future_use = *(p+2) & 0x07;
		svc.EIT_user_defined_flags = (*(p+2) >> 2) & 0x07;
		svc.EIT_schedule_flag = (*(p+2) >> 1) & 0x01;
		svc.EIT_present_following_flag = *(p+2) & 0x01;
		svc.running_status = (*(p+3) >> 5) & 0x07;
		svc.free_CA_mode = (*(p+3) >> 4) & 0x01;
		svc.descriptors_loop_length = (*(p+3) & 0x0f) << 8 | *(p+4);

		p += SDT_SERVICE_FIX_LEN;
		int n = (int)svc.descriptors_loop_length;
		while (n > 0) {
			CDescriptor desc (p);
			if (!desc.isValid) {
				_UTL_LOG_W ("invalid SDT desc");
				return false;
			}
			svc.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		serviceLen -= (SDT_SERVICE_FIX_LEN + svc.descriptors_loop_length) ;
		if (serviceLen < 0) {
			_UTL_LOG_W ("invalid SDT stream");
			return false;
		}

		pTable->services.push_back (svc);
	}

	return true;
}

void CServiceDescriptionTable::appendTable (CTable *pTable)
{
	if (!pTable) {
		return ;
	}

	std::lock_guard<std::mutex> lock (mMutexTables);

	mTables.push_back (pTable);
}

void CServiceDescriptionTable::releaseTables (void)
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

void CServiceDescriptionTable::dumpTables (void)
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

void CServiceDescriptionTable::dumpTable (const CTable* pTable) const
{
	if (!pTable) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	pTable->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                  [0x%02x]\n", pTable->header.table_id);
	_UTL_LOG_I ("transport_stream_id       [0x%04x]\n", pTable->header.table_id_extension);
	_UTL_LOG_I ("original_network_id       [0x%04x]\n", pTable->original_network_id);

	std::vector<CTable::CService>::const_iterator iter_svc = pTable->services.begin();
	for (; iter_svc != pTable->services.end(); ++ iter_svc) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("service_id                 [0x%04x]\n", iter_svc->service_id);
		_UTL_LOG_I ("EIT_user_defined_flags     [0x%01x]\n", iter_svc->EIT_user_defined_flags);
		_UTL_LOG_I ("EIT_schedule_flag          [0x%01x]\n", iter_svc->EIT_schedule_flag);
		_UTL_LOG_I ("EIT_present_following_flag [0x%01x]\n", iter_svc->EIT_present_following_flag);
		_UTL_LOG_I ("running_status             [0x%01x]\n", iter_svc->running_status);
		_UTL_LOG_I ("free_CA_mode               [0x%01x]\n", iter_svc->free_CA_mode);
		_UTL_LOG_I ("descriptors_loop_length    [%d]\n", iter_svc->descriptors_loop_length);

		std::vector<CDescriptor>::const_iterator iter_desc = iter_svc->descriptors.begin();
		for (; iter_desc != iter_svc->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			CDescriptorCommon::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void CServiceDescriptionTable::clear (void)
{
	releaseTables ();

//	detachAllSectionList ();
	// detachAllSectionList in parser loop
	asyncDelete ();
}

CServiceDescriptionTable::CReference CServiceDescriptionTable::reference (void)
{
	CReference ref (&mTables, &mMutexTables);
	return ref;
}
