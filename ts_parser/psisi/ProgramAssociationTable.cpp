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

	refreshTablesByVersionNumber (pTable) ;

	appendTable (pTable);

	// debug dump
	if (CUtils::getLogLevel() <= EN_LOG_LEVEL_D) {
//TODO mutex
		std::lock_guard<std::recursive_mutex> lock (mMutexTables);
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

	std::lock_guard<std::recursive_mutex> lock (mMutexTables);

	mTables.push_back (pTable);
}

void CProgramAssociationTable::releaseTables (void)
{
	std::lock_guard<std::recursive_mutex> lock (mMutexTables);

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

void CProgramAssociationTable::releaseTable (CTable *pErase)
{
	if (!pErase) {
		return;
	}

	std::lock_guard<std::recursive_mutex> lock (mMutexTables);

	if (mTables.size() == 0) {
		return;
	}

	std::vector<CTable*>::iterator iter = mTables.begin(); 
	for (; iter != mTables.end(); ++ iter) {
		if (*iter == pErase) {
			delete (*iter);
			(*iter) = NULL;
			iter = mTables.erase(iter);
			break;
		}
	}
}

void CProgramAssociationTable::dumpTables (void)
{
	std::lock_guard<std::recursive_mutex> lock (mMutexTables);

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

bool CProgramAssociationTable::refreshTableByVersionNumber (CTable* pNewTable)
{
	if (!pNewTable) {
		return false;
	}

	std::lock_guard<std::recursive_mutex> lock (mMutexTables);


	// sub-table identify
	uint8_t new_tblid = pNewTable->header.table_id;
	uint16_t new_tsid = pNewTable->header.table_id_extension;


	uint8_t new_ver = pNewTable->header.version_number;
	uint8_t sec_num = pNewTable->header.section_number;

	CTable *pErase = NULL;

	std::vector<CTable*>::const_iterator iter = mTables.begin();
    for (; iter != mTables.end(); ++ iter) {
		CTable *pTable = *iter;

		// check every sub-table
		if (
			(new_tblid == pTable->header.table_id) &&
			(new_tsid == pTable->header.table_id_extension)
		) {
			uint8_t ver_tmp = pTable->header.version_number;
			++ ver_tmp;
			ver_tmp &= 0x1f;

			if (new_ver >= ver_tmp) {

				// new version
				pErase = pTable;
				break;

			} else {
				if (new_ver == pTable->header.version_number) {
					if (sec_num == pTable->header.section_number) {
						// duplicate sections should be checked by CSectionParser
						_UTL_LOG_E (
							"BUG: same version.  new_ver:[0x%02x]  pTable->header.version_number:[0x%02x]",
							new_ver,
							pTable->header.version_number
						);
					}
				} else {
					_UTL_LOG_E (
						"BUG: unexpected version.  new_ver:[0x%02x]  pTable->header.version_number:[0x%02x]",
						new_ver,
						pTable->header.version_number
					);
				}
			}
		}
	}

	if (pErase) {
		releaseTable (pErase);
		return true;
	} else {
		return false;
	}
}

void CProgramAssociationTable::refreshTablesByVersionNumber (CTable* pNewTable)
{
	if (!pNewTable) {
		return ;
	}

	while (1) {
		if (!refreshTableByVersionNumber (pNewTable)) {
			break;
		}
	}
}

CProgramAssociationTable::CReference CProgramAssociationTable::reference (void)
{
	CReference ref (&mTables, &mMutexTables);
	return ref;
}
